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

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_nn.h"
#include "ipc_osd_module.h"
#include "ipc_timeout_task.h"

AML_LOG_DEFINE(osd_nn);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(osd_nn)

struct osd_config {
  bool enabled;
  bool show_points;
  struct osd_attr_font font;
  struct osd_attr_rect rect;
  bool changed;
};

struct private_data {
  struct {
    struct AmlIPCOSDContext *rect;      // osd context for drawing rects
    struct AmlIPCOSDContext *rect_info; // osd context for drawing rect info
  } ctx;
  struct AmlIPCOSDFrameList *surfaces;
  struct AmlIPCFrame *result;
  bool update_request;
  pthread_mutex_t lock;
};

static struct osd_config gs_config = {
    .font.file = NULL, .changed = false, .rect.border = 2};

#define CONFIG_PARENT_KEY "/ipc/overlay/nn/"

static struct ipc_config_kv gs_kvs[] = {
    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "show", &gs_config.enabled, true,
                       NULL),
    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "show-points", &gs_config.show_points,
                       false, NULL),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "font/size", &gs_config.font.size, 48,
                    NULL),
    IPC_CONFIG_KV_COLOR(CONFIG_PARENT_KEY "font/color",
                        &gs_config.font.color.fg,
                        AML_RGBA(0xff, 0xff, 0xff, 0xe0), NULL),
    IPC_CONFIG_KV_COLOR(CONFIG_PARENT_KEY "font/background-color",
                        &gs_config.font.color.bg,
                        AML_RGBA(0x80, 0x80, 0x80, 0x80), NULL),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "font/font-file", &gs_config.font.file,
                      DEFAULT_FONT_FILE, NULL),
    IPC_CONFIG_KV_COLOR(CONFIG_PARENT_KEY "rect-color", &gs_config.rect.color,
                        AML_RGBA(0xf0, 0xf0, 0xf0, 0xff), NULL),
    {NULL},
};

static void on_config_changed(const char *key, const char *val,
                              void *user_data) {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    if (strcmp(gs_kvs[i].key, key) == 0) {
      ipc_config_load_kv(&gs_kvs[i]);
      break;
    }
  }
  gs_config.changed = true;
}

static AmlStatus register_config_handle() {
  return ipc_config_register_handle(CONFIG_PARENT_KEY, on_config_changed, NULL);
}

static AmlStatus unregister_config_handle() {
  return ipc_config_unregister_handle(CONFIG_PARENT_KEY, on_config_changed);
}

static AmlStatus load_configs() {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    ipc_config_load_kv(&gs_kvs[i]);
  }
  return AML_STATUS_OK;
}

static AmlStatus update(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)object->priv;

  if (object->screen.width == 0) {
    // ignore while the video size not set
    return AML_STATUS_OK;
  }

  if (!priv->update_request)
    return AML_STATUS_OK;
  priv->update_request = false;

  // cleanup nn result
  if (priv->surfaces) {
    aml_obj_release(AML_OBJECT(priv->surfaces));
    priv->surfaces = NULL;
  }

  if (!gs_config.enabled)
    return AML_STATUS_OK;

  if (gs_config.changed) {
    if (priv->ctx.rect) {
      aml_obj_release(AML_OBJECT(priv->ctx.rect));
      priv->ctx.rect = NULL;
    }
    if (priv->ctx.rect_info) {
      aml_obj_release(AML_OBJECT(priv->ctx.rect_info));
      priv->ctx.rect_info = NULL;
    }
    gs_config.changed = false;
  }

  if (priv->ctx.rect == NULL) {
    priv->ctx.rect = aml_ipc_osd_context_new();
    aml_ipc_osd_set_color(priv->ctx.rect, gs_config.rect.color);
    aml_ipc_osd_set_bgcolor(priv->ctx.rect, 0);
    aml_ipc_osd_set_width(priv->ctx.rect, gs_config.rect.border);
  }
  if (priv->ctx.rect_info == NULL) {
    priv->ctx.rect_info = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(priv->ctx.rect_info, gs_config.font.file,
                          gs_config.font.size);
    aml_ipc_osd_set_color(priv->ctx.rect_info, gs_config.font.color.fg);
    aml_ipc_osd_set_bgcolor(priv->ctx.rect_info, gs_config.font.color.bg);
  }

  AML_SCTIME_LOGD("update nn osd");
  pthread_mutex_lock(&priv->lock);

  if (NULL == priv->result || priv->result->size == 0 ||
      priv->result->data == NULL) {
    pthread_mutex_unlock(&priv->lock);
    return AML_STATUS_OK;
  }

  priv->surfaces = aml_ipc_osd_frame_list_new();

  struct AmlIPCNNResultBuffer *buf =
      (struct AmlIPCNNResultBuffer *)priv->result->data;
  AML_LOGD("detection result amount: %d", buf->amount);
  for (int i = 0; i < buf->amount; i++) {
    struct nn_result *res = &buf->results[i];
    struct relative_pos *pt = &res->pos;
    int width = (int)((pt->right - pt->left) * object->screen.width);
    int height = (int)((pt->bottom - pt->top) * object->screen.height);
    int left = (int)(pt->left * object->screen.width);
    int top = (int)(pt->top * object->screen.height);

    {
      // draw rect
      struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, width, height, 0};
      struct AmlIPCOSDFrame *frame = aml_ipc_osd_frame_new_alloc(&fmt);
      aml_ipc_osd_frame_set_src_rect(frame, 0, 0, width, height);
      aml_ipc_osd_frame_set_dst_rect(frame, left, top, width, height);

      struct AmlRect rc = {0, 0, width, height};
      aml_ipc_osd_start_paint(priv->ctx.rect, AML_IPC_VIDEO_FRAME(frame));
      aml_ipc_osd_clear_rect(priv->ctx.rect, NULL);
      aml_ipc_osd_draw_rect(priv->ctx.rect, &rc);
      if (gs_config.show_points) {
        for (int j = 0; j < 5; j++) {
          struct facepoint_pos *fpt = &res->fpos[j];
          int x = (int)(((fpt->x - pt->left) / (pt->right - pt->left)) * width);
          int y = (int)(((fpt->y - pt->top) / (pt->bottom - pt->top)) * height);
          struct AmlRect rc_facepoint = {x - 2, y - 2, 4, 4};
          aml_ipc_osd_fill_rect(priv->ctx.rect, &rc_facepoint,
                                AML_RGBA(255, 0, 0, 250));
        }
      }
      aml_ipc_osd_done_paint(priv->ctx.rect);
      aml_ipc_osd_frame_list_append(priv->surfaces, frame);
    }

    if (strlen(res->info) > 0) {
      int x = 0, y = 0, t;
      // draw face info
      struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 0, 0, 0};
      // preload text to check the surface size
      aml_ipc_osd_preload_text(priv->ctx.rect_info, res->info, &fmt.width,
                               &fmt.height);
      struct AmlIPCOSDFrame *frame_info = aml_ipc_osd_frame_new_alloc(&fmt);
      t = top > fmt.height ? top - fmt.height : top;
      aml_ipc_osd_frame_set_src_rect(frame_info, 0, 0, fmt.width, fmt.height);
      aml_ipc_osd_frame_set_dst_rect(frame_info, left, t, fmt.width,
                                     fmt.height);
      aml_ipc_osd_start_paint(priv->ctx.rect_info,
                              AML_IPC_VIDEO_FRAME(frame_info));
      aml_ipc_osd_clear_rect(priv->ctx.rect_info, NULL);
      aml_ipc_osd_draw_text(priv->ctx.rect_info, res->info, &x, &y);
      aml_ipc_osd_done_paint(priv->ctx.rect_info);
      aml_ipc_osd_frame_list_append(priv->surfaces, frame_info);
    }
  }
  pthread_mutex_unlock(&priv->lock);

  return AML_STATUS_OK;
}

static AmlStatus render(struct AmlIPCOSDModule *object,
                        struct AmlIPCGE2D *render,
                        struct AmlIPCVideoFrame *dest_frame) {
  struct private_data *priv = (struct private_data *)(object->priv);
  struct AmlIPCOSDFrameList *list = priv->surfaces;
  if (object->screen.width == 0) {
    // screen size not set
    return AML_STATUS_OK;
  }

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

  if (priv->result) {
    aml_obj_release(AML_OBJECT(priv->result));
  }

  if (priv->ctx.rect) {
    aml_obj_release(AML_OBJECT(priv->ctx.rect));
    priv->ctx.rect = NULL;
  }
  if (priv->ctx.rect_info) {
    aml_obj_release(AML_OBJECT(priv->ctx.rect_info));
    priv->ctx.rect_info = NULL;
  }

  if (priv->surfaces) {
    aml_obj_release(AML_OBJECT(priv->surfaces));
    priv->surfaces = NULL;
  }

  free(priv);
  free(object);
  return AML_STATUS_OK;
}

static AmlStatus start(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);
  pthread_mutex_init(&priv->lock, NULL);
  load_configs();
  register_config_handle();
  return AML_STATUS_OK;
}

static AmlStatus stop(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);
  priv->update_request = false;
  unregister_config_handle();
  return AML_STATUS_OK;
}

static struct AmlIPCOSDFrameList *getframes(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);
  struct AmlIPCOSDFrameList *frames = NULL;
  if (gs_config.enabled) {
    if (priv->surfaces) {
      frames = priv->surfaces;
      // take away
      priv->surfaces = NULL;
    } else {
      if (NULL == priv->result || priv->result->size == 0) {
        // no recognized rects, return empty list
        frames = aml_ipc_osd_frame_list_new();
      }
    }
  } else {
    // if is not enabled, return empty list
    frames = aml_ipc_osd_frame_list_new();
  }
  return frames;
}

struct AmlIPCOSDModule *aml_ipc_osd_nn_new() {
  struct AmlIPCOSDModule *object = ipc_mem_new0(struct AmlIPCOSDModule, 1);
  if (object) {
    object->priv = ipc_mem_new0(struct private_data, 1);
    object->update = update;
    object->render = render;
    object->release = release;
    object->start = start;
    object->stop = stop;
    object->getframes = getframes;
  }
  return object;
}

AmlStatus aml_ipc_osd_nn_push_result(struct AmlIPCOSDModule *object,
                                     struct AmlIPCFrame *result) {
  struct private_data *priv = (struct private_data *)(object->priv);
  if (result->size > 0) {
    if (pthread_mutex_trylock(&priv->lock) != 0) {
      AML_LOGD("busy, skip result");
      aml_obj_release(AML_OBJECT(result));
      return AML_STATUS_OK;
    }
  } else {
    // clear signal must be send
    pthread_mutex_lock(&priv->lock);
  }
  if (priv->result) {
    aml_obj_release(AML_OBJECT(priv->result));
  }
  priv->result = result;
  priv->update_request = true;
  pthread_mutex_unlock(&priv->lock);
  return AML_STATUS_OK;
}
