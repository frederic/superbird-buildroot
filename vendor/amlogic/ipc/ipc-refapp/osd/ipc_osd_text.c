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
#include <aml_ipc_sdk.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_osd_module.h"
#include "ipc_timeout_task.h"

AML_LOG_DEFINE(osd_text);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(osd_text)

struct OverlayItem {
  TAILQ_ENTRY(OverlayItem) node;
  struct osd_attr_font font;
  struct {
    int top;
    int left;
  } position;
  char *text;
};

struct osd_config {
  TAILQ_HEAD(OverlayItemHead, OverlayItem) items;
  pthread_mutex_t lock;
  unsigned long timer;
  bool changed;
};

#define CONFIG_PARENT_KEY "/ipc/overlay/watermark/text"

struct private_data {
  struct AmlIPCOSDFrameList *surfaces;
};

static void cleanup_items(struct osd_config *config) {
  while (!TAILQ_EMPTY(&config->items)) {
    struct OverlayItem *item = TAILQ_FIRST(&config->items);
    TAILQ_REMOVE(&config->items, item, node);
    FREE_STR(item->font.file);
    FREE_STR(item->text);
    free(item);
  }
}
static void load_items(struct osd_config *config) {
  int num_items = ipc_config_array_get_size(CONFIG_PARENT_KEY);

  for (int i = 0; i < num_items; i++) {
    struct OverlayItem *item = ipc_mem_new0(struct OverlayItem, 1);

    item->font.color.fg = ipc_config_array_get_color(
        CONFIG_PARENT_KEY, i, "font/color", AML_RGBA(0xff, 0xff, 0xff, 0xff));
    item->font.color.bg = ipc_config_array_get_color(
        CONFIG_PARENT_KEY, i, "font/background-color", 0);
    item->font.file = ipc_config_array_get_string(
        CONFIG_PARENT_KEY, i, "font/font-file", DEFAULT_FONT_FILE);
    item->font.size =
        ipc_config_array_get_int32_t(CONFIG_PARENT_KEY, i, "font/size", 30);
    item->position.top =
        ipc_config_array_get_int32_t(CONFIG_PARENT_KEY, i, "position/top", 0);
    item->position.left =
        ipc_config_array_get_int32_t(CONFIG_PARENT_KEY, i, "position/left", 0);
    item->text = ipc_config_array_get_string(CONFIG_PARENT_KEY, i, "text", "");
    TAILQ_INSERT_TAIL(&config->items, item, node);
  }
}

static bool delay_load_rects(void *data) {
  struct osd_config *config = (struct osd_config *)data;
  pthread_mutex_lock(&config->lock);
  cleanup_items(config);
  load_items(config);
  pthread_mutex_unlock(&config->lock);
  config->timer = 0;
  config->changed = true;
  return IPC_TIMEOUT_TASK_STOP;
}

static void on_config_changed(const char *key, const char *val,
                              void *user_data) {
  struct osd_config *config = (struct osd_config *)user_data;
  if (config->timer > 0) {
    ipc_timeout_task_remove(config->timer);
  }
  config->timer = ipc_timeout_task_add(100, delay_load_rects, config);
}

static struct osd_config gs_config;

static AmlStatus register_config_handle() {
  return ipc_config_register_handle(CONFIG_PARENT_KEY, on_config_changed,
                                    &gs_config);
}

static AmlStatus unregister_config_handle() {
  return ipc_config_unregister_handle(CONFIG_PARENT_KEY, on_config_changed);
}

static AmlStatus update(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)object->priv;
  if (!gs_config.changed)
    return AML_STATUS_OK;

  gs_config.changed = false;

  if (priv->surfaces) {
    aml_obj_release(AML_OBJECT(priv->surfaces));
    priv->surfaces = NULL;
  }

  priv->surfaces = aml_ipc_osd_frame_list_new();

  struct OverlayItem *item;
  pthread_mutex_lock(&gs_config.lock);
  TAILQ_FOREACH(item, &gs_config.items, node) {
    if (item->text == NULL || item->text[0] == '\0')
      continue;
    struct AmlIPCOSDContext *ctx;
    ctx = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(ctx, item->font.file, item->font.size);
    aml_ipc_osd_set_color(ctx, item->font.color.fg);
    aml_ipc_osd_set_bgcolor(ctx, item->font.color.bg);

    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 0, 0, 0};
    // preload text to check the surface size
    aml_ipc_osd_preload_text(ctx, item->text, &fmt.width, &fmt.height);

    if (fmt.width > object->screen.width)
      fmt.width = object->screen.width;

    struct AmlIPCOSDFrame *surface = aml_ipc_osd_frame_new_alloc(&fmt);
    aml_ipc_osd_frame_set_src_rect(surface, 0, 0, fmt.width, fmt.height);
    aml_ipc_osd_frame_set_dst_rect(surface, item->position.left,
                                   item->position.top, fmt.width, fmt.height);

    // draw text surface
    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(surface));
    aml_ipc_osd_clear_rect(ctx, NULL); // slow
    int top = 0, left = 0, bottom = top, right = left;
    aml_ipc_osd_draw_text(ctx, item->text, &bottom, &right);
    aml_ipc_osd_done_paint(ctx);

    aml_obj_release(AML_OBJECT(ctx));
    aml_ipc_osd_frame_list_append(priv->surfaces, surface);
  }
  pthread_mutex_unlock(&gs_config.lock);

  return AML_STATUS_OK;
}

static AmlStatus render(struct AmlIPCOSDModule *object,
                        struct AmlIPCGE2D *render,
                        struct AmlIPCVideoFrame *dest_frame) {
  struct private_data *priv = (struct private_data *)(object->priv);
  struct AmlIPCOSDFrameList *list = priv->surfaces;
  if (NULL == list)
    return AML_STATUS_OK;

  struct AmlIPCOSDFrame *frame;
  TAILQ_FOREACH(frame, &list->head, node) {
    struct AmlRect *dstrc = &frame->dst_rect;
    aml_ipc_ge2d_blend(render, AML_IPC_VIDEO_FRAME(frame), dest_frame,
                       dest_frame, NULL, dstrc->left, dstrc->top, dstrc->left,
                       dstrc->top);
  }

  return AML_STATUS_OK;
}

static AmlStatus release(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);

  if (gs_config.timer > 0) {
    ipc_timeout_task_remove(gs_config.timer);
    gs_config.timer = 0;
  }

  cleanup_items(&gs_config);

  if (priv->surfaces) {
    aml_obj_release(AML_OBJECT(priv->surfaces));
    priv->surfaces = NULL;
  }
  free(priv);
  free(object);
  return AML_STATUS_OK;
}

static AmlStatus start(struct AmlIPCOSDModule *object) {
  TAILQ_INIT(&gs_config.items);
  load_items(&gs_config);
  gs_config.changed = true;
  register_config_handle();

  return AML_STATUS_OK;
}

static AmlStatus stop(struct AmlIPCOSDModule *object) {
  unregister_config_handle();
  return AML_STATUS_OK;
}

static struct AmlIPCOSDFrameList *getframes(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);
  struct AmlIPCOSDFrameList *frames = NULL;
  if (priv->surfaces) {
    frames = priv->surfaces;
    // take away
    priv->surfaces = NULL;
  } else {
    if (TAILQ_EMPTY(&gs_config.items)) {
      frames = aml_ipc_osd_frame_list_new();
    }
  }

  return frames;
}

struct AmlIPCOSDModule *aml_ipc_osd_text_new() {
  struct AmlIPCOSDModule *obj = ipc_mem_new0(struct AmlIPCOSDModule, 1);
  if (obj) {
    obj->priv = ipc_mem_new0(struct private_data, 1);
    obj->update = update;
    obj->render = render;
    obj->release = release;
    obj->start = start;
    obj->stop = stop;
    obj->getframes = getframes;
  }
  return obj;
}
