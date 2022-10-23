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

#include <ipc_property.h>
#include "ipc_property_priv.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <cstdlib>

using namespace std;

static int
connect_socket(const char *name) {
  const int fd = socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd == -1) {
    return -1;
  }

  const size_t namelen = strlen(name);

  sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  strncpy(addr.sun_path, name, sizeof(addr.sun_path));
  addr.sun_family = AF_LOCAL;
  socklen_t alen = namelen + offsetof(sockaddr_un, sun_path) + 1;
  if (TEMP_FAILURE_RETRY(connect(fd, reinterpret_cast<sockaddr *>(&addr), alen)) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}

static int
__send_prop_msg(struct ipc_prop_msg **_msg) {
  int fd = connect_socket(IPC_PROPERTY_SERVICE_CMD_SOCKET);
  if (fd < 0) return -1;
  struct ipc_prop_msg *msg = *_msg;
  ssize_t num_bytes = TEMP_FAILURE_RETRY(send(fd, msg,
     sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz, 0));

  int result = -1;
  if (num_bytes == sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz) {
    if(msg->cmd == IPC_PROP_MSG_GETPROP
    || msg->cmd == IPC_PROP_MSG_LISTPROP
    || msg->cmd == IPC_PROP_MSG_GETPROP_ARRAY_SIZE) {
      pollfd pollfds[1];
      pollfds[0].fd = fd;
      pollfds[0].events = POLLIN;
      pollfds[0].revents = 0;
      const int poll_result = TEMP_FAILURE_RETRY(poll(pollfds, 1, 250 /* ms */));
      if (poll_result > 0 && (pollfds[0].revents & POLLIN) != 0) {
        num_bytes = TEMP_FAILURE_RETRY(recv(fd, msg, sizeof(struct ipc_prop_msg), 0));
        if (num_bytes == sizeof(ipc_prop_msg)) {
          msg = (struct ipc_prop_msg *)realloc(msg,
              sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
          {
            // split receive
            size_t bufsize = msg->valsz + msg->keysz;
            char *buf = &msg->buf[0];

            do {
              num_bytes = TEMP_FAILURE_RETRY(recv(fd, buf, bufsize > 1024 ? 1024 : bufsize, 0));
              if (num_bytes <= 0) {
                break;
              }
              buf += num_bytes;
              bufsize -= num_bytes;
            } while (bufsize > 0);

            if (bufsize == 0) {
              result = 0;
            }
          }
          *_msg = msg;
        }
      }
    } else {
      result = 0;
    }
  }

  close(fd);
  return result;
}

bool ipc_property_set(const char *key, const char *value) {
  if (key == nullptr) return false;
  if (value == nullptr) value = "";

  struct ipc_prop_msg *msg;
  msg = (struct ipc_prop_msg *) malloc (sizeof(struct ipc_prop_msg));
  msg->cmd = IPC_PROP_MSG_SETPROP;
  msg->keysz = strlen(key) + 1;
  msg->valsz = strlen(value) + 1;
  msg = (struct ipc_prop_msg *) realloc (msg,
                                         sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
  strcpy(&msg->buf[0], key);
  strcpy(&msg->buf[msg->keysz], value);

  const int err = __send_prop_msg(&msg);

  free(msg);
  return err >= 0;

}

void ipc_property_list(const char *key) {
  struct ipc_prop_msg *msg;
  msg = (struct ipc_prop_msg *) malloc (sizeof(struct ipc_prop_msg));
  msg->cmd = IPC_PROP_MSG_LISTPROP;
  msg->keysz = key == nullptr ? 0 : strlen(key) + 1;
  msg->valsz = 0;
  if (key) {
    msg = (struct ipc_prop_msg *) realloc (msg,
        sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
    strcpy(&msg->buf[0], key);
  }

  const int err = __send_prop_msg(&msg);
  if (err < 0 || msg->valsz == 0) {
    free(msg);
    return;
  }

  printf("%s\n", &msg->buf[msg->keysz]);

  free(msg);
}


/*
 * key consists of keys and slash the same as the file path
 * /key/to/the/location for example.
 * as of the array, visit the value with index,
 * /key/to/the/array/0/item for example.
 */
size_t ipc_property_get(const char *key, char *value, size_t bufsize) {
  if (key == nullptr || value == nullptr || bufsize == 0) return 0;

  struct ipc_prop_msg *msg;
  msg = (struct ipc_prop_msg *) malloc (sizeof(struct ipc_prop_msg));
  msg->cmd = IPC_PROP_MSG_GETPROP;
  msg->keysz = strlen(key) + 1;
  msg->valsz = 0;
  msg = (struct ipc_prop_msg *) realloc (msg,
                                         sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
  strcpy(&msg->buf[0], key);

  const int err = __send_prop_msg(&msg);
  if (err < 0 || msg->valsz == 0) {
    free(msg);
    return 0;
  }

  size_t dstsz = msg->valsz > bufsize ? bufsize : msg->valsz;
  memcpy(value, &msg->buf[msg->keysz], dstsz);
  value[bufsize - 1] = '\0';

  free(msg);

  return strlen(value);
}

bool ipc_property_remove(const char *key) {
  struct ipc_prop_msg *msg;
  bool ret = false;

  if (key == nullptr || strlen(key) == 0) return ret;

  msg = (struct ipc_prop_msg *) malloc (sizeof(struct ipc_prop_msg));
  msg->cmd = IPC_PROP_MSG_REMOVEPROP;
  msg->keysz = strlen(key) + 1;
  msg->valsz = 0;
  msg = (struct ipc_prop_msg *) realloc (msg,
      sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
  strcpy(&msg->buf[0], key);

  const int err = __send_prop_msg(&msg);
  if (err > 0) {
    ret = true;
  }

  free(msg);
  return ret;
}

int ipc_property_get_array_size(const char *key) {
  if (key == nullptr) return 0;

  struct ipc_prop_msg *msg;
  msg = (struct ipc_prop_msg *) malloc (sizeof(struct ipc_prop_msg));
  msg->cmd = IPC_PROP_MSG_GETPROP_ARRAY_SIZE;
  msg->keysz = strlen(key) + 1;
  msg->valsz = 0;
  msg = (struct ipc_prop_msg *) realloc (msg,
                                         sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
  strcpy(&msg->buf[0], key);

  const int err = __send_prop_msg(&msg);
  if (err < 0 || msg->valsz == 0) {
    free(msg);
    return 0;
  }

  int arraysize = 0;
  memcpy(&arraysize, &msg->buf[msg->keysz], sizeof(int));

  free(msg);

  return arraysize;

}

struct event_cb_data {
  int fd;
  PropEventCallback event_cb;
};

static void *
handle_event_callback(void *arg) {
  struct event_cb_data *cb_data = (struct event_cb_data *) arg;
  struct ipc_prop_msg *msg;

  pollfd pollfds[1];
  pollfds[0].fd = cb_data->fd;
  pollfds[0].events = POLLIN;
  pollfds[0].revents = 0;

  msg = (struct ipc_prop_msg *) malloc (sizeof(struct ipc_prop_msg));
  while (cb_data->fd > 0) {
    const int poll_result = TEMP_FAILURE_RETRY(poll(pollfds, 1, -1));
    if (poll_result > 0 && (pollfds[0].revents & POLLIN) != 0) {
      ssize_t num_bytes = TEMP_FAILURE_RETRY(recv(cb_data->fd, msg, sizeof(struct ipc_prop_msg), 0));
      if (num_bytes <= 0) { cb_data->fd = -1; continue; }
      if (num_bytes == sizeof(ipc_prop_msg)) {
        msg = (struct ipc_prop_msg *)realloc(msg,
                                             sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
        num_bytes = TEMP_FAILURE_RETRY(recv(cb_data->fd, &msg->buf[0], msg->keysz + msg->valsz, 0));
        if (num_bytes == msg->keysz + msg->valsz) {
          cb_data->event_cb(&msg->buf[0], &msg->buf[msg->keysz]);
        }
      }
    }
  }
  free(msg);
  free(cb_data);
  return nullptr;
}

bool ipc_property_register_callback(PropEventCallback callback) {
  pthread_t tid;
  int fd = connect_socket(IPC_PROPERTY_SERVICE_EVENT_SOCKET);
  if (fd > 0) {
    struct event_cb_data *data = (struct event_cb_data *) malloc (sizeof(struct event_cb_data));
    data->fd = fd;
    data->event_cb = callback;
    pthread_create(&tid, nullptr, handle_event_callback, (void *) data);
    return true;
  }
  return false;
}

