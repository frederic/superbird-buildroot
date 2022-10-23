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

AML_LOG_DEFINE(osd_image);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(osd_image)

struct OverlayItem {
  TAILQ_ENTRY(OverlayItem) node;
  struct {
    int top;
    int left;
    int width;
    int height;
  } position;
  char *file;
};

struct osd_config {
  TAILQ_HEAD(OverlayItemHead, OverlayItem) items;
  pthread_mutex_t lock;
  unsigned long timer;
  bool changed;
};

#define CONFIG_PARENT_KEY "/ipc/overlay/watermark/image"

struct private_data {
  struct AmlIPCOSDFrameList *surfaces;
  struct AmlIPCOSDContext *ctx;
  struct AmlIPCGE2D *ge2d;
};

static void cleanup_items(struct osd_config *config) {
  while (!TAILQ_EMPTY(&config->items)) {
    struct OverlayItem *item = TAILQ_FIRST(&config->items);
    TAILQ_REMOVE(&config->items, item, node);
    if (item->file)
      free(item->file);
    free(item);
  }
}
static void load_items(struct osd_config *config) {
  int num_items = ipc_config_array_get_size(CONFIG_PARENT_KEY);

  for (int i = 0; i < num_items; i++) {
    struct OverlayItem *item = ipc_mem_new0(struct OverlayItem, 1);

    item->file = ipc_config_array_get_string(CONFIG_PARENT_KEY, i, "file", "");
    item->position.top =
        ipc_config_array_get_int32_t(CONFIG_PARENT_KEY, i, "position/top", 0);
    item->position.left =
        ipc_config_array_get_int32_t(CONFIG_PARENT_KEY, i, "position/left", 0);
    item->position.width =
        ipc_config_array_get_int32_t(CONFIG_PARENT_KEY, i, "position/width", 0);
    item->position.height = ipc_config_array_get_int32_t(CONFIG_PARENT_KEY, i,
                                                         "position/height", 0);
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

enum image_type { IMG_INVALID = 0, IMG_JPG, IMG_PNG };
static enum image_type detect_image_type(const char *file) {
  enum image_type ret = IMG_INVALID;
  FILE *fp = fopen(file, "rb");
  if (NULL == fp) {
    return ret;
  }
  char jpeg_header[] = {0xff, 0xd8, 0xff, 0xe0};
  char png_header[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
  char file_header[8];
  int r = fread(file_header, 1, sizeof(file_header), fp);
  fclose(fp);
  if (r != sizeof(file_header)) {
    return ret;
  }
  if (memcmp(file_header, jpeg_header, sizeof(jpeg_header)) == 0) {
    ret = IMG_JPG;
  } else if (memcmp(file_header, png_header, sizeof(png_header)) == 0) {
    ret = IMG_PNG;
  }
  return ret;
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
  if (NULL == priv->ctx) {
    priv->ctx = aml_ipc_osd_context_new();
  }

  priv->surfaces = aml_ipc_osd_frame_list_new();

  struct OverlayItem *item;
  pthread_mutex_lock(&gs_config.lock);
  TAILQ_FOREACH(item, &gs_config.items, node) {
    if (item->file == NULL || item->file[0] == '\0') {
      continue;
    }
    enum image_type t = detect_image_type(item->file);
    if (t == IMG_INVALID) {
      AML_LOGE("file [%s] is neither png nor jpeg", item->file);
      continue;
    }
    int w, h;
    if (t == IMG_PNG) {
      if (AML_STATUS_OK !=
          aml_ipc_osd_preload_png(priv->ctx, item->file, &w, &h)) {
        continue;
      }
    } else {
      if (AML_STATUS_OK !=
          aml_ipc_osd_preload_jpeg(priv->ctx, item->file, &w, &h)) {
        continue;
      }
    }

    // check if request size is greater than screen size
    if (item->position.width > object->screen.width ||
        item->position.height > object->screen.height) {
      AML_LOGW("requested image size[%dx%d] exceeds screen[%dx%d]",
               item->position.width, item->position.height,
               object->screen.width, object->screen.height);
      continue;
    }

    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, w, h, 0};
    struct AmlIPCOSDFrame *surface = aml_ipc_osd_frame_new_alloc(&fmt);
    aml_ipc_osd_start_paint(priv->ctx, AML_IPC_VIDEO_FRAME(surface));
    if (t == IMG_PNG) {
      aml_ipc_osd_load_png(priv->ctx, item->file, 0, 0);
    } else {
      aml_ipc_osd_load_jpeg(priv->ctx, item->file, 0, 0);
    }
    aml_ipc_osd_done_paint(priv->ctx);

    // use the orignal size while the width/height of item is set to 0
    // resize input image if the request width/height not matched
    if ((item->position.width != 0 || item->position.height != 0) &&
        (item->position.width != w || item->position.height != h)) {
      struct AmlRect srect = {0, 0, w, h};
      struct AmlRect drect = {
          0, 0, item->position.width == 0 ? w : item->position.width,
          item->position.height == 0 ? h : item->position.height};
      if (priv->ge2d == NULL) {
        priv->ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_TO);
        if (priv->ge2d == NULL) {
          AML_LOGE("failed to create ge2d engine");
        }
      }
      if (priv->ge2d) {
        struct AmlIPCVideoFormat dfmt = {AML_PIXFMT_RGBA_8888, drect.width,
                                         drect.height, 0};
        struct AmlIPCOSDFrame *dsurface = aml_ipc_osd_frame_new_alloc(&dfmt);
        if (AML_STATUS_OK ==
            aml_ipc_ge2d_stretchblt(priv->ge2d, AML_IPC_VIDEO_FRAME(surface),
                                    AML_IPC_VIDEO_FRAME(dsurface), &srect,
                                    &drect)) {
          aml_obj_release(AML_OBJECT(surface));
          surface = dsurface;
          w = drect.width;
          h = drect.height;
        } else {
          aml_obj_release(AML_OBJECT(dsurface));
          AML_LOGE("failed to resize the input image");
        }
      }
    }
    aml_ipc_osd_frame_set_src_rect(surface, 0, 0, w, h);
    aml_ipc_osd_frame_set_dst_rect(surface, item->position.left,
                                   item->position.top, w, h);
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
  if (priv->ctx) {
    aml_obj_release(AML_OBJECT(priv->ctx));
    priv->ctx = NULL;
  }
  if (priv->ge2d) {
    aml_obj_release(AML_OBJECT(priv->ge2d));
    priv->ge2d = NULL;
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

struct AmlIPCOSDModule *aml_ipc_osd_image_new() {
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
