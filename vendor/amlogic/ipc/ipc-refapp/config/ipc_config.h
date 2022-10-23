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

#ifndef __IPC_CONFIG_H__
#define __IPC_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <aml_ipc_sdk.h>

enum osd_position {
  OSD_POS_TOP_LEFT,
  OSD_POS_TOP_MID,
  OSD_POS_TOP_RIGHT,
  OSD_POS_MID_LEFT,
  OSD_POS_MID_RIGHT,
  OSD_POS_CENTER,
  OSD_POS_BOTTOM_LEFT,
  OSD_POS_BOTTOM_MID,
  OSD_POS_BOTTOM_RIGHT,
};

#define TYPE_T(type) type_##type
#define _T(type) TYPE_T(type)

enum ipc_config_vtype {
  _T(bool) = 0,
  _T(int32_t),
  _T(uint32_t),
  _T(int64_t),
  _T(uint64_t),
  _T(float),
  _T(string),
  _T(color),
  _T(resolution),
  _T(position),
};

struct ipc_resolution {
  int32_t width;
  int32_t height;
};

struct ipc_config_kv {
  const char *key;
  void *pval;
  union {
    bool b;
    int32_t i;
    uint32_t ui;
    int64_t l;
    uint64_t ul;
    float f;
    char *s;
    struct ipc_resolution r;
  } defval;
  enum ipc_config_vtype type;
  void (*handle)(void *user_data);
};

#define IPC_CONFIG_KV_BOOL(k, v, d, f)                                         \
  { k, v, .defval.b = d, _T(bool), f }
#define IPC_CONFIG_KV_I(k, v, d, f)                                            \
  { k, v, .defval.i = d, _T(uint32_t), f }
#define IPC_CONFIG_KV_STR(k, v, d, f)                                          \
  { k, v, .defval.s = d, _T(string), f }
#define IPC_CONFIG_KV_FLOAT(k, v, d, f)                                        \
  { k, v, .defval.s = d, _T(float), f }
#define IPC_CONFIG_KV_COLOR(k, v, d, f)                                        \
  { k, v, .defval.i = d, _T(color), f }
#define IPC_CONFIG_KV_POS(k, v, d, f)                                          \
  { k, v, .defval.i = d, _T(position), f }
#define IPC_CONFIG_KV_RESOLUTION(k, v, dw, dh, f)                              \
  { k, v, .defval.r.width = dw, .defval.r.height = dh, _T(resolution), f }

#define DECLARE_IPC_CONFIG_GET_TYPE(ret, def, name)                            \
  ret ipc_config_get_##name(const char *key, def defval)

#define DECLARE_IPC_CONFIG_ARRAY_GET_TYPE(ret, def, name)                      \
  ret ipc_config_array_get_##name(const char *key, int index,                  \
                                  const char *item, def defval)

#define DECLARE_IPC_CONFIG_GET(ret, def, name)                                 \
  DECLARE_IPC_CONFIG_GET_TYPE(ret, def, name);                                 \
  DECLARE_IPC_CONFIG_ARRAY_GET_TYPE(ret, def, name)

#define DECLARE_IPC_CONFIG_SIMP_GET(type)                                      \
  DECLARE_IPC_CONFIG_GET(type, type, type)

DECLARE_IPC_CONFIG_SIMP_GET(int32_t);
DECLARE_IPC_CONFIG_SIMP_GET(uint32_t);
DECLARE_IPC_CONFIG_SIMP_GET(int64_t);
DECLARE_IPC_CONFIG_SIMP_GET(uint64_t);
DECLARE_IPC_CONFIG_SIMP_GET(float);
DECLARE_IPC_CONFIG_GET(bool, bool, boolean);
DECLARE_IPC_CONFIG_GET(char *, const char *, string);
DECLARE_IPC_CONFIG_GET(int32_t, int32_t, color);
DECLARE_IPC_CONFIG_GET(enum osd_position, enum osd_position, position);
DECLARE_IPC_CONFIG_GET(struct ipc_resolution *, struct ipc_resolution *,
                       resolution);

typedef void (*IPCConfigOnChangeFunc)(const char *key, const char *val,
                                      void *user_data);

AmlStatus ipc_config_init();
AmlStatus ipc_config_deinit();
AmlStatus ipc_config_register_handle(const char *key,
                                     IPCConfigOnChangeFunc func,
                                     void *user_data);
AmlStatus ipc_config_unregister_handle(const char *key,
                                       IPCConfigOnChangeFunc func);
int ipc_config_array_get_size(const char *key);
void ipc_config_remove(const char *key);

#define IPC_CONFIG_ACT_(act, type, key, defval)                                \
  ipc_config_##act##_##type(key, defval)
#define ipc_config_get(type, key, defval)                                      \
  IPC_CONFIG_ACT_(get, type, key, defval)

#define IPC_CONFIG_ARRAY_ACT_(act, type, key, index, item, defval)             \
  ipc_config_array_##act##_##type(key, index, item, defval)
#define ipc_config_array_get(type, key, index, item, defval)                   \
  IPC_CONFIG_ARRAY_ACT_(get, type, key, index, item, defval)

AmlStatus ipc_config_load_kv(struct ipc_config_kv *kv);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IPC_CONFIG_H__ */
