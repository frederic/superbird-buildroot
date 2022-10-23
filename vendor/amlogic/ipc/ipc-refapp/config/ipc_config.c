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

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_timeout_task.h"

AML_LOG_DEFINE(ipc_config);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(ipc_config)

struct IPCConfigCallbackInfo {
  LIST_ENTRY(IPCConfigCallbackInfo) node;
  char *key;
  void *user_data;
  struct {
    char *key;
    char *val;
  } changing;
  unsigned int timer;
  pthread_mutex_t lock;
  IPCConfigOnChangeFunc func;
};

struct IPCConfigCallbackInfoList {
  LIST_HEAD(IPCConfigCallbackInfoListHead, IPCConfigCallbackInfo) head;
  pthread_mutex_t lock;
};

struct IPCConfig {
  bool initialized;
  struct IPCConfigCallbackInfoList *callbacks;
};

static struct IPCConfig *gs_config = NULL;

#define IPC_CONFIG_INITIALIZED(ret)                                            \
  do {                                                                         \
    if (NULL == gs_config || !gs_config->initialized) {                        \
      AML_LOGE("config not initialized");                                      \
      return ret;                                                              \
    }                                                                          \
  } while (0)

static bool delay_trigger_callback(void *data) {
  struct IPCConfigCallbackInfo *callback = (struct IPCConfigCallbackInfo *)data;
  pthread_mutex_lock(&callback->lock);
  callback->func(callback->changing.key, callback->changing.val,
                 callback->user_data);
  callback->timer = 0;
  pthread_mutex_unlock(&callback->lock);
  return IPC_TIMEOUT_TASK_STOP;
}

static void on_property_changed(const char *key, const char *val) {
  if (NULL == gs_config || !gs_config->initialized || NULL == key)
    return;
  struct IPCConfigCallbackInfo *callback;
  pthread_mutex_lock(&gs_config->callbacks->lock);
  LIST_FOREACH(callback, &gs_config->callbacks->head, node) {
    if (strncmp(key, callback->key, strlen(callback->key)) == 0) {
      pthread_mutex_lock(&callback->lock);
      if (callback->timer > 0) {
        ipc_timeout_task_remove(callback->timer);
      }
      if (callback->changing.key)
        free(callback->changing.key);
      callback->changing.key = strdup(key);

      if (callback->changing.val)
        free(callback->changing.val);
      if (val) {
        callback->changing.val = strdup(val);
      } else {
        callback->changing.val = NULL;
      }
      callback->timer =
          ipc_timeout_task_add(1, delay_trigger_callback, callback);
      pthread_mutex_unlock(&callback->lock);
    }
  }
  pthread_mutex_unlock(&gs_config->callbacks->lock);
}

AmlStatus ipc_config_init() {
  if (gs_config) {
    AML_LOGD("already initialized");
    return AML_STATUS_OK;
  }

  gs_config = ipc_mem_new0(struct IPCConfig, 1);
  gs_config->callbacks = ipc_mem_new0(struct IPCConfigCallbackInfoList, 1);
  LIST_INIT(&gs_config->callbacks->head);
  pthread_mutex_init(&gs_config->callbacks->lock, NULL);

  ipc_property_register_callback(on_property_changed);
  gs_config->initialized = true;
  return AML_STATUS_OK;
}

AmlStatus ipc_config_register_handle(const char *key,
                                     IPCConfigOnChangeFunc func,
                                     void *user_data) {
  IPC_CONFIG_INITIALIZED(AML_STATUS_FAIL);
  if (key == NULL || func == NULL) {
    AML_LOGW("parameter not correct");
    return AML_STATUS_FAIL;
  }
  struct IPCConfigCallbackInfo *info =
      ipc_mem_new0(struct IPCConfigCallbackInfo, 1);
  info->key = strdup(key);
  info->func = func;
  info->user_data = user_data;
  pthread_mutex_init(&info->lock, NULL);
  pthread_mutex_lock(&gs_config->callbacks->lock);
  LIST_INSERT_HEAD(&gs_config->callbacks->head, info, node);
  pthread_mutex_unlock(&gs_config->callbacks->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_config_unregister_handle(const char *key,
                                       IPCConfigOnChangeFunc func) {
  if (func == NULL)
    return AML_STATUS_OK;
  struct IPCConfigCallbackInfo *callback;
  pthread_mutex_lock(&gs_config->callbacks->lock);
  callback = LIST_FIRST(&gs_config->callbacks->head);
  while (callback) {
    struct IPCConfigCallbackInfo *next = LIST_NEXT(callback, node);
    if (callback->func == func) {
      if (key) {
        if (strcmp(key, callback->key) != 0)
          goto unregister_next;
      }
      LIST_REMOVE(callback, node);
      pthread_mutex_lock(&callback->lock);
      if (callback->timer > 0) {
        ipc_timeout_task_remove(callback->timer);
      }
      if (callback->changing.key)
        free(callback->changing.key);
      if (callback->changing.val)
        free(callback->changing.val);
      if (callback->key)
        free(callback->key);
      pthread_mutex_unlock(&callback->lock);
      free(callback);
    }
  unregister_next:
    callback = next;
  }
  pthread_mutex_unlock(&gs_config->callbacks->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_config_deinit() {
  IPC_CONFIG_INITIALIZED(AML_STATUS_OK);
  gs_config->initialized = false;
  struct IPCConfigCallbackInfo *callback;
  pthread_mutex_lock(&gs_config->callbacks->lock);
  while ((callback = LIST_FIRST(&gs_config->callbacks->head))) {
    LIST_REMOVE(callback, node);
    pthread_mutex_lock(&callback->lock);
    if (callback->timer > 0) {
      ipc_timeout_task_remove(callback->timer);
    }
    if (callback->changing.key)
      free(callback->changing.key);
    if (callback->changing.val)
      free(callback->changing.val);
    if (callback->key)
      free(callback->key);
    pthread_mutex_unlock(&callback->lock);
    free(callback);
  }
  pthread_mutex_unlock(&gs_config->callbacks->lock);
  free(gs_config->callbacks);
  free(gs_config);
  gs_config = NULL;
  return AML_STATUS_OK;
}

#define IPC_PROPERTY_VALUE_MAXLEN 256

void ipc_config_remove(const char *key) {
  ipc_property_remove(key);
  return;
}

// array
int ipc_config_array_get_size(const char *key) {
  return ipc_property_get_array_size(key);
}

static char *make_array_key(const char *key, int index, const char *item) {
  if (key == NULL || index < 0)
    return NULL;
  char *cmd = NULL;
  int sz = 0;
  if (item == NULL || item[0] == '\0') {
    sz = snprintf(NULL, 0, "%s/%d", key, index) + 1;
    cmd = ipc_mem_new0(char, sz);
    snprintf(cmd, sz, "%s/%d", key, index);
  } else {
    sz = snprintf(NULL, 0, "%s/%d/%s", key, index, item) + 1;
    cmd = ipc_mem_new0(char, sz);
    snprintf(cmd, sz, "%s/%d/%s", key, index, item);
  }
  return cmd;
}

struct ipc_resolution *ipc_config_get_resolution(const char *key,
                                                 struct ipc_resolution *pval) {
  if (pval == NULL)
    return NULL;
  char *retval = ipc_config_get(string, key, NULL);
  if (retval) {
    int w, h;
    char sign[2];
    int n = sscanf(retval, "%d%1[xX]%d", &w, sign, &h);
    if (n == 3) {
      pval->width = w;
      pval->height = h;
    }
    free(retval);
  }
  return pval;
}

int ipc_config_get_color(const char *key, int defval) {
  char r, g, b, a;
  int color = ipc_config_get(int32_t, key, defval);
  a = color & 0xff;
  b = (color & 0xff00) >> 8;
  g = (color & 0xff0000) >> 16;
  r = (color & 0xff000000) >> 24;
  color = AML_RGBA(r, g, b, a);
  return color;
}

enum osd_position ipc_config_get_position(const char *key,
                                          enum osd_position defval) {
  enum osd_position pos = defval;
  char *retval = ipc_config_get(string, key, NULL);
  if (retval) {
    if (strcmp(retval, "top-left") == 0) {
      pos = OSD_POS_TOP_LEFT;
    } else if (strcmp(retval, "top-mid") == 0) {
      pos = OSD_POS_TOP_MID;
    } else if (strcmp(retval, "top-right") == 0) {
      pos = OSD_POS_TOP_RIGHT;
    } else if (strcmp(retval, "mid-left") == 0) {
      pos = OSD_POS_MID_LEFT;
    } else if (strcmp(retval, "center") == 0) {
      pos = OSD_POS_CENTER;
    } else if (strcmp(retval, "mid-right") == 0) {
      pos = OSD_POS_MID_RIGHT;
    } else if (strcmp(retval, "bot-left") == 0) {
      pos = OSD_POS_BOTTOM_LEFT;
    } else if (strcmp(retval, "bot-mid") == 0) {
      pos = OSD_POS_BOTTOM_MID;
    } else if (strcmp(retval, "bot-right") == 0) {
      pos = OSD_POS_BOTTOM_RIGHT;
    }
    free(retval);
  }
  return pos;
}

#define GET_VAL_INTEGER *val = ipc_strtoul(value)

#define GET_VAL_boolean *val = ipc_strtob(value)
#define GET_VAL_int32_t GET_VAL_INTEGER
#define GET_VAL_uint32_t GET_VAL_INTEGER
#define GET_VAL_int64_t GET_VAL_INTEGER
#define GET_VAL_uint64_t GET_VAL_INTEGER
#define GET_VAL_float *val = ipc_strtof(value)
#define GET_VAL_string *val = strdup(value)

#define DEFINE_INTERNAL_CONFIG_GET_TYPE(ret, name)                             \
  static AmlStatus config_get_##name(const char *key, ret *val) {              \
    char value[IPC_PROPERTY_VALUE_MAXLEN];                                     \
    if (key == NULL || val == NULL)                                            \
      return AML_STATUS_FAIL;                                                  \
    if (ipc_property_get(key, value, sizeof(value))) {                         \
      GET_VAL_##name;                                                          \
      return AML_STATUS_OK;                                                    \
    }                                                                          \
    return AML_STATUS_FAIL;                                                    \
  }

#define ASSIGN_VAL_INTEGER retval = defval

#define ASSIGN_VAL_boolean ASSIGN_VAL_INTEGER
#define ASSIGN_VAL_int32_t ASSIGN_VAL_INTEGER
#define ASSIGN_VAL_uint32_t ASSIGN_VAL_INTEGER
#define ASSIGN_VAL_int64_t ASSIGN_VAL_INTEGER
#define ASSIGN_VAL_uint64_t ASSIGN_VAL_INTEGER
#define ASSIGN_VAL_float ASSIGN_VAL_INTEGER
#define ASSIGN_VAL_string retval = defval == NULL ? NULL : strdup(defval)

#define DEFINE_IPC_CONFIG_GET_TYPE(ret, def, name)                             \
  DECLARE_IPC_CONFIG_GET_TYPE(ret, def, name) {                                \
    ret retval;                                                                \
    if (config_get_##name(key, &retval) != AML_STATUS_OK) {                    \
      ASSIGN_VAL_##name;                                                       \
    }                                                                          \
    return retval;                                                             \
  }

#define DEFINE_IPC_CONFIG_ARRAY_GET_TYPE(ret, def, name)                       \
  DECLARE_IPC_CONFIG_ARRAY_GET_TYPE(ret, def, name) {                          \
    char *array_key = make_array_key(key, index, item);                        \
    ret retval = ipc_config_get_##name(array_key, defval);                     \
    if (array_key)                                                             \
      free(array_key);                                                         \
    return retval;                                                             \
  }

#define DEFINE_CONFIG_GET_TYPE(ret, def, name)                                 \
  DEFINE_INTERNAL_CONFIG_GET_TYPE(ret, name)                                   \
  DEFINE_IPC_CONFIG_GET_TYPE(ret, def, name)                                   \
  DEFINE_IPC_CONFIG_ARRAY_GET_TYPE(ret, def, name)

#define DEFINE_CONFIG_SIMP_GET_TYPE(type)                                      \
  DEFINE_INTERNAL_CONFIG_GET_TYPE(type, type)                                  \
  DEFINE_IPC_CONFIG_GET_TYPE(type, type, type)                                 \
  DEFINE_IPC_CONFIG_ARRAY_GET_TYPE(type, type, type)

DEFINE_CONFIG_SIMP_GET_TYPE(int32_t)
DEFINE_CONFIG_SIMP_GET_TYPE(uint32_t)
DEFINE_CONFIG_SIMP_GET_TYPE(int64_t)
DEFINE_CONFIG_SIMP_GET_TYPE(uint64_t)
DEFINE_CONFIG_SIMP_GET_TYPE(float)
DEFINE_CONFIG_GET_TYPE(bool, bool, boolean)
DEFINE_CONFIG_GET_TYPE(char *, const char *, string)
DEFINE_IPC_CONFIG_ARRAY_GET_TYPE(int32_t, int32_t, color);
DEFINE_IPC_CONFIG_ARRAY_GET_TYPE(enum osd_position, enum osd_position,
                                 position);
DEFINE_IPC_CONFIG_ARRAY_GET_TYPE(struct ipc_resolution *,
                                 struct ipc_resolution *, resolution);

AmlStatus ipc_config_load_kv(struct ipc_config_kv *kv) {
  switch (kv->type) {
  case _T(bool):
    *((bool *)kv->pval) = ipc_config_get(boolean, kv->key, kv->defval.b);
    break;
  case _T(int32_t):
    *((int32_t *)kv->pval) = ipc_config_get(int32_t, kv->key, kv->defval.i);
    break;
  case _T(uint32_t):
    *((uint32_t *)kv->pval) = ipc_config_get(uint32_t, kv->key, kv->defval.ui);
    break;
  case _T(int64_t):
    *((int64_t *)kv->pval) = ipc_config_get(int64_t, kv->key, kv->defval.l);
    break;
  case _T(uint64_t):
    *((uint64_t *)kv->pval) = ipc_config_get(uint64_t, kv->key, kv->defval.ul);
    break;
  case _T(float):
    *((float *)kv->pval) = ipc_config_get(float, kv->key, kv->defval.f);
    break;
  case _T(string):
    if (kv->pval)
      free(*((char **)kv->pval));
    *((char **)kv->pval) = ipc_config_get(string, kv->key, kv->defval.s);
    break;
  case _T(color):
    *((int32_t *)kv->pval) = ipc_config_get(color, kv->key, kv->defval.i);
    break;
  case _T(position):
    *((int32_t *)kv->pval) = ipc_config_get(position, kv->key, kv->defval.i);
    break;
  case _T(resolution):
    if (kv->pval) {
      struct ipc_resolution *pval = (struct ipc_resolution *)kv->pval;
      pval->width = kv->defval.r.width;
      pval->height = kv->defval.r.height;
      ipc_config_get(resolution, kv->key, pval);
    }
    break;
  default:
    break;
  }
  return AML_STATUS_OK;
}
