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
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <ctype.h>
#include <unistd.h>

#include <gpiod.h>

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_timeout_task.h"
#include "ipc_day_night_switch.h"

AML_LOG_DEFINE(day_night_switch);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(day_night_switch)

enum operation_type { OP_IDLE = 0, OP_SET_LINE, OP_MAX };

struct sequence_info {
  TAILQ_ENTRY(sequence_info) node;
  enum operation_type type;
  union {
    int msec;
    struct {
      int offset;
      bool status;
    } line;
  } operation;
};

struct Config {
  enum { MODE_AUTO, MODE_DAY, MODE_NIGHT, MODE_SCHEDULE } mode;
  struct {
    struct daytime {
      int h;
      int m;
      int s;
    } begin, end;
  } schedule;
  int delay;
  struct {
    struct GPIO {
      int num_line;
      char *chip;
      struct GPIO_LINE {
        int line;
        bool active_low;
        int direction; // 0: output 1: input
        const char *consumer;
      } * gpios;
      struct {
        TAILQ_HEAD(sequence_info_head, sequence_info) init, enable, disable;
      } sequence;
    } ircut, irled, ldr;
  } hardware;
};

struct DayNightSwitchInfo {
  struct AmlIPCISP *isp;
  struct Config config;
  struct {
    unsigned long schedule;
    unsigned long delay_auto;
  } timer;
  struct {
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool running;
  } ldr_monitor;
  enum {
    STATE_DAY,
    STATE_NIGHT,
    STATE_INVALID,
  } current_state,
      next_state;
};

struct DayNightSwitchInfo *gs_dn_sw = NULL;

#define PROP_KEY "/ipc/day-night-switch/"
#define PROP_KEY_HARDWARE "/ipc/day-night-switch/hardware/"

static bool parse_sequence(struct sequence_info_head *head, char *sequence);
static void do_manual_switch(struct DayNightSwitchInfo *info);
static void do_auto_switch(struct DayNightSwitchInfo *info);

static int get_mode(const char *v) {
  int mode = MODE_DAY;
  if (strcasecmp(v, "day") == 0) {
    mode = MODE_DAY;
  } else if (strcasecmp(v, "night") == 0) {
    mode = MODE_NIGHT;
  } else if (strcasecmp(v, "auto") == 0) {
    mode = MODE_AUTO;
  } else if (strcasecmp(v, "schedule") == 0) {
    mode = MODE_SCHEDULE;
  } else {
    AML_LOGE("invalid mode got: %s", v);
  }
  return mode;
}

#define LOAD_SCHEDULE_TIME(x, dv)                                              \
  do {                                                                         \
    int h, m, s;                                                               \
    char *v = ipc_config_get_string(PROP_KEY "schedule/" #x, dv);              \
    if (3 == sscanf(v, "%d:%d:%d", &h, &m, &s)) {                              \
      config->schedule.x.h = h;                                                \
      config->schedule.x.m = m;                                                \
      config->schedule.x.s = s;                                                \
    }                                                                          \
    free(v);                                                                   \
  } while (0)

static void load_app_config(struct Config *config) {
  char *v = ipc_config_get_string(PROP_KEY "mode", "day");
  config->mode = get_mode(v);
  free(v);
  config->delay = ipc_config_get_int32_t(PROP_KEY "delay", 5);
  LOAD_SCHEDULE_TIME(begin, "06:00:00");
  LOAD_SCHEDULE_TIME(end, "18:00:00");
#if 0
  if (config->schedule.begin.h == config->schedule.end.h &&
      config->schedule.begin.m == config->schedule.end.m) {
    int diff = config->schedule.begin.s - config->schedule.end.s;
    diff = diff > 0 ? diff : -diff;
    if (diff < 10) {
      AML_LOGW("scheduled interval is too short to apply");
    }
  }
#endif
}

#define LOAD_GPIO(portname, dir)                                               \
  do {                                                                         \
    config->hardware.portname.chip = ipc_config_get_string(                    \
        PROP_KEY_HARDWARE #portname "/gpio/chip", "gpiochip0");                \
    config->hardware.portname.num_line =                                       \
        ipc_config_array_get_size(PROP_KEY_HARDWARE #portname "/gpio/line");   \
    config->hardware.portname.gpios =                                          \
        ipc_mem_new0(struct GPIO_LINE, config->hardware.portname.num_line);    \
    for (int i = 0; i < config->hardware.portname.num_line; i++) {             \
      config->hardware.portname.gpios[i].line = ipc_config_array_get_int32_t(  \
          PROP_KEY_HARDWARE #portname "/gpio/line", i, "", -1);                \
      config->hardware.portname.gpios[i].active_low =                          \
          ipc_config_array_get_int32_t(                                        \
              PROP_KEY_HARDWARE #portname "/gpio/active-low", i, "", -1);      \
      config->hardware.portname.gpios[i].consumer = #portname;                 \
      config->hardware.portname.gpios[i].direction = dir;                      \
    }                                                                          \
  } while (0)

#define LOAD_SEQUENCE(name)                                                    \
  do {                                                                         \
    sequence =                                                                 \
        ipc_config_get_string(PROP_KEY_HARDWARE #name "/sequence/init", "");   \
    if (sequence[0] != '\0') {                                                 \
      TAILQ_INIT(&config->hardware.name.sequence.init);                        \
      parse_sequence(&config->hardware.name.sequence.init, sequence);          \
    }                                                                          \
    free(sequence);                                                            \
    sequence =                                                                 \
        ipc_config_get_string(PROP_KEY_HARDWARE #name "/sequence/enable", ""); \
    if (sequence[0] != '\0') {                                                 \
      TAILQ_INIT(&config->hardware.name.sequence.enable);                      \
      parse_sequence(&config->hardware.name.sequence.enable, sequence);        \
    }                                                                          \
    free(sequence);                                                            \
    sequence = ipc_config_get_string(                                          \
        PROP_KEY_HARDWARE #name "/sequence/disable", "");                      \
    if (sequence[0] != '\0') {                                                 \
      TAILQ_INIT(&config->hardware.name.sequence.disable);                     \
      parse_sequence(&config->hardware.name.sequence.disable, sequence);       \
    }                                                                          \
    free(sequence);                                                            \
  } while (0)

static void load_hardware_config(struct Config *config) {
  char *sequence;
  LOAD_GPIO(ircut, 0);
  LOAD_SEQUENCE(ircut);
  LOAD_GPIO(irled, 0);
  LOAD_SEQUENCE(irled);
  LOAD_GPIO(ldr, 1);
}

static void on_config_changed(const char *key, const char *val,
                              void *user_data) {
  struct DayNightSwitchInfo *info = (struct DayNightSwitchInfo *)user_data;
  struct Config *config = &info->config;
  if (val == NULL)
    return;
  if (strcmp(key, PROP_KEY "mode") == 0) {
    AML_LOGD("mode changed to %s", val);
    config->mode = get_mode(val);
    do_manual_switch(info);
    do_auto_switch(info);
    pthread_mutex_lock(&info->ldr_monitor.mutex);
    pthread_cond_signal(&info->ldr_monitor.cond);
    pthread_mutex_unlock(&info->ldr_monitor.mutex);
    return;
  }
  if (strcmp(key, PROP_KEY "delay") == 0) {
    config->delay = ipc_strtoul(val);
    return;
  }
  if (strcmp(key, PROP_KEY "schedule/begin") == 0) {
    LOAD_SCHEDULE_TIME(begin, "06:00:00");
    return;
  }
  if (strcmp(key, PROP_KEY "schedule/end") == 0) {
    LOAD_SCHEDULE_TIME(end, "18:00:00");
    return;
  }
  if (strncmp(key, PROP_KEY_HARDWARE, strlen(PROP_KEY_HARDWARE)) == 0) {
    load_hardware_config(config);
    return;
  }
}

static void load_config(struct DayNightSwitchInfo *info) {
  struct Config *cfg = &info->config;
  load_app_config(cfg);
  load_hardware_config(cfg);
  ipc_config_register_handle(PROP_KEY, on_config_changed, info);
}

static void release_sequence(struct sequence_info_head *head) {
  if (head == NULL)
    return;
  while (!TAILQ_EMPTY(head)) {
    struct sequence_info *item = TAILQ_FIRST(head);
    TAILQ_REMOVE(head, item, node);
    free(item);
  }
}

#define CFG_FREE(v)                                                            \
  if (v) {                                                                     \
    free(v);                                                                   \
    v = NULL;                                                                  \
  }

#define CFG_HW_FREE(f)                                                         \
  do {                                                                         \
    CFG_FREE(cfg->hardware.f.chip);                                            \
    CFG_FREE(cfg->hardware.f.gpios);                                           \
    release_sequence(&cfg->hardware.f.sequence.init);                          \
    release_sequence(&cfg->hardware.f.sequence.enable);                        \
    release_sequence(&cfg->hardware.f.sequence.disable);                       \
  } while (0)

static void unload_config(struct DayNightSwitchInfo *info) {
  struct Config *cfg = &info->config;
  ipc_config_unregister_handle(PROP_KEY, on_config_changed);
  CFG_HW_FREE(ircut);
  CFG_HW_FREE(irled);
  CFG_HW_FREE(ldr);
}

static bool parse_sequence(struct sequence_info_head *head, char *sequence) {
  if (sequence == NULL || head == NULL)
    return false;
  // drop the old sequence
  release_sequence(head);

  char *s = strdup(sequence);
  char *op = strtok(s, ",");
  while (op) {
    char opchar;
    int opval;
    if (sscanf(op, "%c=%d", &opchar, &opval) != 2) {
      AML_LOGE("parse error, malformat sequence string found: %s", op);
      goto parse_abort;
    }
    if (opchar != 'w' && !isdigit(opchar)) {
      AML_LOGE("parse error, not supported operation charactor found: %c",
               opchar);
      goto parse_abort;
    }
    struct sequence_info *info = ipc_mem_new0(struct sequence_info, 1);
    if (opchar == 'w' && opval > 0) {
      info->type = OP_IDLE;
      info->operation.msec = opval;
      AML_LOGD("insert wait operation for %d msec", info->operation.msec);
    } else {
      info->type = OP_SET_LINE;
      info->operation.line.offset = opchar - '0';
      info->operation.line.status = opval > 0;
      AML_LOGD("insert port operation line %d : %d",
               info->operation.line.offset, info->operation.line.status);
    }
    TAILQ_INSERT_TAIL(head, info, node);
    op = strtok(NULL, ",");
  }
  free(s);
  return true;
parse_abort:
  free(s);
  release_sequence(head);
  return false;
}

static bool gpio_check(struct GPIO *gpio) {
  if (gpio == NULL)
    return false;
  if (gpio->chip == NULL) {
    AML_LOGE("gpio chip not defined");
    return false;
  }
  if (gpio->num_line <= 0) {
    AML_LOGE("no gpio lines defined");
    return false;
  }
  return true;
}

static bool gpio_operation(struct GPIO *gpio, struct sequence_info_head *head) {
  if (!gpio_check(gpio))
    return false;

  if (TAILQ_EMPTY(head)) {
    AML_LOGD("no sequence defined");
    return true;
  }
  struct sequence_info *info;
  TAILQ_FOREACH(info, head, node) {
    switch (info->type) {
    case OP_IDLE:
      AML_LOGD("sleep for %d msec", info->operation.msec);
      usleep(info->operation.msec * 1000);
      break;
    case OP_SET_LINE:
      if (info->operation.line.offset < gpio->num_line) {
        struct GPIO_LINE *l = &gpio->gpios[info->operation.line.offset];
        if (l->line < 0 || gpiod_ctxless_set_value(
                               gpio->chip, l->line, info->operation.line.status,
                               l->active_low, l->consumer, NULL, NULL) < 0) {
          AML_LOGE("set value to %s:%d failed", gpio->chip, l->line);
        } else {
          AML_LOGD("line operation %s:%d -> %d", gpio->chip, l->line,
                   info->operation.line.status);
        }
      } else {
        AML_LOGW("line offset exceed the definition:%d",
                 info->operation.line.offset);
      }
      break;
    default:
      break;
    }
  }
  return true;
}

static bool gpio_init(struct GPIO *gpio) {
  return gpio_operation(gpio, &gpio->sequence.init);
}

static bool gpio_enable(struct GPIO *gpio) {
  return gpio_operation(gpio, &gpio->sequence.enable);
}

static bool gpio_disable(struct GPIO *gpio) {
  return gpio_operation(gpio, &gpio->sequence.disable);
}

static bool switch2night(struct DayNightSwitchInfo *info) {
  if (info->current_state == STATE_NIGHT)
    return true;
  // turn off ircut
  // open led
  // set isp to blackwhite mode
  bool ret = gpio_disable(&info->config.hardware.ircut) &&
             gpio_enable(&info->config.hardware.irled) &&
             aml_ipc_isp_set_bw_mode(info->isp, true) == AML_STATUS_OK;
  if (ret) {
    info->current_state = STATE_NIGHT;
    AML_LOGD("switched to night mode");
  } else {
    AML_LOGE("switch to night mode failed");
  }
  return ret;
}

static bool switch2day(struct DayNightSwitchInfo *info) {
  if (info->current_state == STATE_DAY)
    return true;
  // set isp to color mode
  // close led
  // turn on ircut
  bool ret = gpio_disable(&info->config.hardware.irled) &&
             gpio_enable(&info->config.hardware.ircut) &&
             aml_ipc_isp_set_bw_mode(info->isp, false) == AML_STATUS_OK;
  if (ret) {
    info->current_state = STATE_DAY;
    AML_LOGD("switched to day mode");
  } else {
    AML_LOGE("switch to day mode failed");
  }
  return ret;
}

static bool tmcmp(struct daytime *tm1, struct daytime *tm2) {
  return tm1->h == tm2->h
             ? (tm1->m == tm2->m ? tm1->s > tm2->s : tm1->m > tm2->m)
             : tm1->h > tm2->h;
}

static bool check_scheduled_switcing(void *data) {
  struct DayNightSwitchInfo *info = (struct DayNightSwitchInfo *)data;
  if (info->config.mode != MODE_SCHEDULE) {
    return IPC_TIMEOUT_TASK_CONTINUE;
  }
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  struct daytime tnow = {tm->tm_hour, tm->tm_min, tm->tm_sec};
  // schedule end > begin
  if (tmcmp(&info->config.schedule.end, &info->config.schedule.begin)) {
    // |-------begin---^---end-------|
    // |---^---begin-------end-------|
    // |-------begin-------end---^---|
    if (tmcmp(&tnow, &info->config.schedule.begin) &&
        tmcmp(&info->config.schedule.end, &tnow)) {
      switch2day(info);
    } else {
      switch2night(info);
    }
  } else {
    // |-------end---^---begin-------|
    // |---^---end-------begin-------|
    // |-------end-------begin---^---|
    if (tmcmp(&tnow, &info->config.schedule.end) &&
        tmcmp(&info->config.schedule.begin, &tnow)) {
      switch2night(info);
    } else {
      switch2day(info);
    }
  }
  return IPC_TIMEOUT_TASK_CONTINUE;
}

static void do_manual_switch(struct DayNightSwitchInfo *info) {
  int mode = info->config.mode;
  if (mode == MODE_DAY) {
    switch2day(info);
  } else if (mode == MODE_NIGHT) {
    switch2night(info);
  }
}

static bool delay_auto_switch(void *data) {
  struct DayNightSwitchInfo *info = (struct DayNightSwitchInfo *)data;
  if (info->config.mode == MODE_AUTO) {
    if (info->next_state == STATE_NIGHT) {
      switch2night(info);
    } else {
      switch2day(info);
    }
  }
  return IPC_TIMEOUT_TASK_STOP;
}

int ldr_gpio_event_handle(int type, unsigned int offset,
                          const struct timespec *ts, void *data) {
  struct DayNightSwitchInfo *info = (struct DayNightSwitchInfo *)data;
  struct GPIO *gpio = &info->config.hardware.ldr;
  struct GPIO_LINE *line = &gpio->gpios[0];
  if (type != GPIOD_CTXLESS_EVENT_CB_TIMEOUT) {
    AML_LOGD("gpio event got : type %d", type);
    info->next_state = STATE_DAY;
    if ((!line->active_low && type == GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE) ||
        (line->active_low && type == GPIOD_CTXLESS_EVENT_CB_RISING_EDGE)) {
      info->next_state = STATE_NIGHT;
    }
    if (info->timer.delay_auto) {
      ipc_timeout_task_remove(info->timer.delay_auto);
      info->timer.delay_auto = 0;
    }
    info->timer.delay_auto = ipc_timeout_task_add(info->config.delay * 1000,
                                                  delay_auto_switch, info);
  }
  if (info->ldr_monitor.running && info->config.mode == MODE_AUTO) {
    return GPIOD_CTXLESS_EVENT_CB_RET_OK;
  } else {
    return GPIOD_CTXLESS_EVENT_CB_RET_STOP;
  }
}

static void *ldr_gpio_monitor(void *data) {
  struct DayNightSwitchInfo *info = (struct DayNightSwitchInfo *)data;
  struct GPIO *gpio = &info->config.hardware.ldr;
  struct GPIO_LINE *line = &gpio->gpios[0];
  while (info->ldr_monitor.running) {
    pthread_mutex_lock(&info->ldr_monitor.mutex);
    while (info->config.mode != MODE_AUTO) {
      pthread_cond_wait(&info->ldr_monitor.cond, &info->ldr_monitor.mutex);
    }
    pthread_mutex_unlock(&info->ldr_monitor.mutex);
    if (!info->ldr_monitor.running) {
      break;
    }
    // timeout 100ms
    struct timespec timeout = {.tv_sec = 0, .tv_nsec = 100 * 1000 * 1000};

    gpiod_ctxless_event_monitor(gpio->chip, GPIOD_CTXLESS_EVENT_BOTH_EDGES,
                                line->line, line->active_low, line->consumer,
                                &timeout, NULL, ldr_gpio_event_handle, data);
  }
  return NULL;
}

static void do_auto_switch(struct DayNightSwitchInfo *info) {
  int mode = info->config.mode;
  if (mode != MODE_AUTO)
    return;

  struct GPIO *gpio = &info->config.hardware.ldr;
  struct GPIO_LINE *line = &gpio->gpios[0];
  int v = gpiod_ctxless_get_value(gpio->chip, line->line, line->active_low,
                                  line->consumer);
  AML_LOGD("ldr init status: %d, active_low: %d", v, line->active_low);
  if ((v != 0) == line->active_low) {
    switch2night(info);
  } else {
    switch2day(info);
  }
}

AmlStatus ipc_day_night_switch_init(struct AmlIPCISP *isp) {
  if (gs_dn_sw) {
    AML_LOGW("day night switch sequence already initialized");
    return AML_STATUS_OK;
  }
  gs_dn_sw = ipc_mem_new0(struct DayNightSwitchInfo, 1);
  gs_dn_sw->isp = isp;
  gs_dn_sw->current_state = STATE_INVALID;
  load_config(gs_dn_sw);
  gpio_init(&gs_dn_sw->config.hardware.ircut);
  gpio_init(&gs_dn_sw->config.hardware.irled);
  gpio_init(&gs_dn_sw->config.hardware.ldr);

  // if mode is manual, set the init status
  do_manual_switch(gs_dn_sw);
  // if mode is auto, set the init status
  do_auto_switch(gs_dn_sw);

  pthread_mutex_init(&gs_dn_sw->ldr_monitor.mutex, NULL);
  pthread_cond_init(&gs_dn_sw->ldr_monitor.cond, NULL);
  gs_dn_sw->ldr_monitor.running = true;
  pthread_create(&gs_dn_sw->ldr_monitor.tid, NULL, ldr_gpio_monitor, gs_dn_sw);

  gs_dn_sw->timer.schedule =
      ipc_timeout_task_add(1000, check_scheduled_switcing, gs_dn_sw);
  return AML_STATUS_OK;
}

AmlStatus ipc_day_night_switch_deinit() {
  if (gs_dn_sw == NULL) {
    AML_LOGW("day night switch sequence not initialized");
    return AML_STATUS_OK;
  }
  if (gs_dn_sw->timer.schedule) {
    ipc_timeout_task_remove(gs_dn_sw->timer.schedule);
    gs_dn_sw->timer.schedule = 0;
  }
  if (gs_dn_sw->timer.delay_auto) {
    ipc_timeout_task_remove(gs_dn_sw->timer.delay_auto);
    gs_dn_sw->timer.delay_auto = 0;
  }
  pthread_mutex_lock(&gs_dn_sw->ldr_monitor.mutex);
  gs_dn_sw->ldr_monitor.running = false;
  gs_dn_sw->config.mode = MODE_AUTO;
  pthread_cond_signal(&gs_dn_sw->ldr_monitor.cond);
  pthread_mutex_unlock(&gs_dn_sw->ldr_monitor.mutex);
  pthread_join(gs_dn_sw->ldr_monitor.tid, NULL);
  unload_config(gs_dn_sw);
  free(gs_dn_sw);
  gs_dn_sw = NULL;
  return AML_STATUS_OK;
}
