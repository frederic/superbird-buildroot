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
#include <stdlib.h>
#include <string.h>
#include "ipc_config.h"
#include "ipc_common.h"
#include "ipc_timeout_task.h"
#include "ipc_shielding.h"

#define PARENT_KEY "/ipc/overlay/shielding"

struct ShieldingRectInfo {
  TAILQ_ENTRY(ShieldingRectInfo) node;
  int id;
  struct {
    float left;
    float top;
    float width;
    float height;
  } location;
};

struct AmlIPCShieldingInfo {
  TAILQ_HEAD(ShieldingRectInfoListHead, ShieldingRectInfo) head;
  pthread_mutex_t lock;
  unsigned long timer;
};

struct AmlIPCShieldingPriv {

  /*< private >*/
  struct AmlIPCGE2D *fillrect;
  struct AmlIPCShieldingInfo shielding;
};

static void cleanup_shielding_rects(struct AmlIPCShieldingInfo *shielding) {
  struct ShieldingRectInfo *shielding_rect;
  shielding_rect = TAILQ_FIRST(&shielding->head);
  while (shielding_rect != NULL) {
    struct ShieldingRectInfo *next = TAILQ_NEXT(shielding_rect, node);
    TAILQ_REMOVE(&shielding->head, shielding_rect, node);
    free(shielding_rect);
    shielding_rect = next;
  }
}
static void load_shielding_rects(struct AmlIPCShieldingInfo *shielding) {
  struct ShieldingRectInfo *shielding_rect;
  int array_size = ipc_config_array_get_size(PARENT_KEY);

  for (int i = 0; i < array_size; i++) {
    shielding_rect = ipc_mem_new0(struct ShieldingRectInfo, 1);

    shielding_rect->id = i;
    shielding_rect->location.left =
        ipc_config_array_get_float(PARENT_KEY, i, "position/left", 0);
    shielding_rect->location.top =
        ipc_config_array_get_float(PARENT_KEY, i, "position/top", 0);
    shielding_rect->location.width =
        ipc_config_array_get_float(PARENT_KEY, i, "position/width", 0);
    shielding_rect->location.height =
        ipc_config_array_get_float(PARENT_KEY, i, "position/height", 0);
    TAILQ_INSERT_TAIL(&shielding->head, shielding_rect, node);
  }
}

static bool delay_load_rects(void *data) {
  struct AmlIPCShieldingInfo *shielding = (struct AmlIPCShieldingInfo *)data;
  pthread_mutex_lock(&shielding->lock);
  cleanup_shielding_rects(shielding);
  load_shielding_rects(shielding);
  pthread_mutex_unlock(&shielding->lock);
  shielding->timer = 0;
  return IPC_TIMEOUT_TASK_STOP;
}

static void on_config_changed(const char *key, const char *val,
                              void *user_data) {
  struct AmlIPCShieldingInfo *shielding =
      (struct AmlIPCShieldingInfo *)user_data;
  if (shielding->timer > 0) {
    ipc_timeout_task_remove(shielding->timer);
  }
  shielding->timer = ipc_timeout_task_add(100, delay_load_rects, shielding);
}

static AmlStatus
register_shielding_handle(struct AmlIPCShieldingInfo *shielding) {
  return ipc_config_register_handle(PARENT_KEY, on_config_changed, shielding);
}

static AmlStatus unregister_shielding_handle() {
  return ipc_config_unregister_handle(PARENT_KEY, on_config_changed);
}

static void ipc_shielding_init(struct AmlIPCShieldingInfo *shielding) {
  shielding->timer = 0;
  TAILQ_INIT(&shielding->head);
  pthread_mutex_init(&shielding->lock, NULL);
  load_shielding_rects(shielding);
  register_shielding_handle(shielding);
  return;
}

static void ipc_shielding_deinit(struct AmlIPCShieldingInfo *shielding) {
  if (shielding->timer > 0) {
    ipc_timeout_task_remove(shielding->timer);
    shielding->timer = 0;
  }
  unregister_shielding_handle();
  pthread_mutex_lock(&shielding->lock);
  cleanup_shielding_rects(shielding);
  pthread_mutex_unlock(&shielding->lock);
  pthread_mutex_destroy(&shielding->lock);
  return;
}

struct AmlIPCShielding *aml_ipc_shielding_new() {
  struct AmlIPCShielding *shielding = ipc_mem_new0(struct AmlIPCShielding, 1);
  shielding->priv = ipc_mem_new0(struct AmlIPCShieldingPriv, 1);

  shielding->priv->fillrect = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
  ipc_shielding_init(&shielding->priv->shielding);

  return shielding;
}

void aml_ipc_shielding_free(struct AmlIPCShielding *shielding) {
  if (shielding == NULL || shielding->priv == NULL)
    return;

  if (shielding->priv->fillrect) {
    aml_obj_release(AML_OBJECT(shielding->priv->fillrect));
    shielding->priv->fillrect = NULL;
  }

  ipc_shielding_deinit(&shielding->priv->shielding);
  free(shielding->priv);
  shielding->priv = NULL;
  free(shielding);
}

AmlStatus on_shielding_frame(struct AmlIPCFrame *frame, void *param) {
  struct AmlIPCVideoFrame *vframe = AML_IPC_VIDEO_FRAME(frame);
  struct AmlIPCShieldingPriv *priv = ((struct AmlIPCShielding *)param)->priv;
  struct AmlRect dst_rect;
  struct ShieldingRectInfo *shielding_rect;

  if (priv == NULL)
    return AML_STATUS_HOOK_CONTINUE;

  pthread_mutex_lock(&priv->shielding.lock);
  TAILQ_FOREACH(shielding_rect, &priv->shielding.head, node) {
    dst_rect.left = (int)(shielding_rect->location.left * vframe->format.width);
    dst_rect.top = (int)(shielding_rect->location.top * vframe->format.height);
    dst_rect.width =
        (int)(shielding_rect->location.width * vframe->format.width);
    dst_rect.height =
        (int)(shielding_rect->location.height * vframe->format.height);

    if (priv->fillrect) {
      aml_ipc_ge2d_fill_rect(priv->fillrect, vframe, &dst_rect,
                             AML_RGBA(0, 0, 0, 0));
    }
  }
  pthread_mutex_unlock(&priv->shielding.lock);

  return AML_STATUS_HOOK_CONTINUE;
}
