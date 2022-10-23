/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "ipc_common.h"
#include "ipc_timeout_task.h"

struct timer_task_info {
  LIST_ENTRY(timer_task_info) node;
  unsigned long id;
  unsigned int interval;
  IPCTimeoutCallback function;
  void *data;
  pthread_t tid;
  bool running;
  bool cancel;
  bool onetime;
};

static struct tasks {
  LIST_HEAD(timer_task_info_head, timer_task_info) pending, living, removing;
  pthread_t tid;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  bool running;
  bool update;
} timer_tasks;

static void insert_task_to_list(struct timer_task_info *t,
                                struct timer_task_info_head *head) {
  pthread_mutex_lock(&timer_tasks.lock);
  LIST_INSERT_HEAD(head, t, node);
  timer_tasks.update = true;
  pthread_cond_signal(&timer_tasks.cond);
  pthread_mutex_unlock(&timer_tasks.lock);
}

static void *timer_process(void *data) {
  struct timer_task_info *info = (struct timer_task_info *)data;
  bool next_valid = true;
  bool waiting_remove = false;

  prctl(PR_SET_NAME, "timeout-task", NULL, NULL, NULL);

  while (info->running) {
    struct timeval tv = {info->interval / 1000, (info->interval % 1000) * 1000};
    select(0, NULL, NULL, NULL, &tv);
    if (!info->cancel && next_valid) {
      next_valid = info->function(info->data);
      if (info->onetime) {
        next_valid = false;
      }
    }
    if (info->cancel) {
      info->running = false;
      continue;
    }
    if (!next_valid && !waiting_remove) {
      // add self to removing list
      struct timer_task_info *r = ipc_mem_new0(struct timer_task_info, 1);
      r->id = info->id;
      insert_task_to_list(r, &timer_tasks.removing);
      waiting_remove = true;
    }
  }

  // exit
  free(info);
  return NULL;
}

static void *check_task(void *data) {
  struct tasks *t = (struct tasks *)data;
  prctl(PR_SET_NAME, "timer-checker", NULL, NULL, NULL);
  while (t->running) {
    pthread_mutex_lock(&t->lock);
    while (!t->update) {
      pthread_cond_wait(&t->cond, &t->lock);
    }
    t->update = false;
    // check pending tasks
    struct timer_task_info *task = LIST_FIRST(&t->pending);
    while ((task = LIST_FIRST(&t->pending))) {
      // move pending task to living task list
      LIST_REMOVE(task, node);
      LIST_INSERT_HEAD(&t->living, task, node);
      // run the pending task immediately
      task->running = true;
      pthread_create(&task->tid, NULL, timer_process, task);
      pthread_detach(task->tid);
    }

    struct timer_task_info *task_removing, *task_living;
    // check removing tasks
    while ((task_removing = LIST_FIRST(&t->removing))) {
      // remove from removing list
      LIST_REMOVE(task_removing, node);
      // search for the task in the living list
      task_living = LIST_FIRST(&t->living);
      while (task_living) {
        struct timer_task_info *next = LIST_NEXT(task_living, node);
        if (task_living->id == task_removing->id) {
          task_living->cancel = true;
          LIST_REMOVE(task_living, node);
          break;
        }
        task_living = next;
      }
      free(task_removing);
    }
    pthread_mutex_unlock(&t->lock);
  }
  return NULL;
}

static unsigned long task_add(unsigned int interval,
                              IPCTimeoutCallback function, void *data,
                              bool onetime) {
  if (!timer_tasks.running || interval == 0 || function == NULL)
    return 0;
  struct timer_task_info *info = ipc_mem_new0(struct timer_task_info, 1);
  info->interval = interval;
  info->function = function;
  info->data = data;
  info->cancel = false;
  info->running = false;
  info->onetime = onetime;
  info->id = (unsigned long)info;
  insert_task_to_list(info, &timer_tasks.pending);
  return info->id;
}

unsigned long ipc_timeout_task_add(unsigned int interval,
                                   IPCTimeoutCallback function, void *data) {
  return task_add(interval, function, data, false);
}

unsigned long ipc_onetime_task_add(unsigned int interval,
                                   IPCTimeoutCallback function, void *data) {
  return task_add(interval, function, data, true);
}

void ipc_timeout_task_remove(unsigned long timer_id) {
  if (!timer_tasks.running || timer_id == 0)
    return;
  struct timer_task_info *info = ipc_mem_new0(struct timer_task_info, 1);
  info->id = timer_id;
  insert_task_to_list(info, &timer_tasks.removing);
}

void ipc_timeout_task_init() {
  LIST_INIT(&timer_tasks.pending);
  LIST_INIT(&timer_tasks.living);
  LIST_INIT(&timer_tasks.removing);
  pthread_mutex_init(&timer_tasks.lock, NULL);
  pthread_cond_init(&timer_tasks.cond, NULL);
  timer_tasks.running = true;
  timer_tasks.update = false;
  pthread_create(&timer_tasks.tid, NULL, check_task, &timer_tasks);
}

void ipc_timeout_task_deinit() {
  pthread_mutex_lock(&timer_tasks.lock);
  struct timer_task_info *task;
  while ((task = LIST_FIRST(&timer_tasks.living))) {
    // cancel the running tasks
    task->cancel = true;
    LIST_REMOVE(task, node);
  }
  // remove the pending tasks
  while ((task = LIST_FIRST(&timer_tasks.pending))) {
    LIST_REMOVE(task, node);
    free(task);
  }
  // remove the removing tasks
  while ((task = LIST_FIRST(&timer_tasks.removing))) {
    LIST_REMOVE(task, node);
    free(task);
  }
  timer_tasks.running = false;
  timer_tasks.update = true;
  pthread_cond_signal(&timer_tasks.cond);
  pthread_mutex_unlock(&timer_tasks.lock);
  pthread_join(timer_tasks.tid, NULL);
}
