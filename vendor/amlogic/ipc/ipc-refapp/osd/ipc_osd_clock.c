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

AML_LOG_DEFINE(osd_clock);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(osd_clock)

struct osd_config {
  bool enabled;
  struct osd_attr_font font;
  enum osd_position position;
  bool changed;
};

#define CONFIG_PARENT_KEY "/ipc/overlay/datetime/"

struct private_data {
  struct AmlIPCOSDContext *ctx;
  struct AmlIPCOSDFrame *surface;
  struct {
    bool request;
    unsigned long handle;
  } timer;
};

static struct osd_config gs_config = {.font.file = NULL, .changed = false};
static struct ipc_config_kv gs_kvs[] = {
    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "enabled", &gs_config.enabled, true,
                       NULL),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "font/size", &gs_config.font.size, 48,
                    NULL),
    IPC_CONFIG_KV_COLOR(CONFIG_PARENT_KEY "font/color",
                        &gs_config.font.color.fg,
                        AML_RGBA(0xff, 0xff, 0xff, 0xff), NULL),
    IPC_CONFIG_KV_COLOR(CONFIG_PARENT_KEY "font/background-color",
                        &gs_config.font.color.bg, AML_RGBA(0, 0, 0, 0), NULL),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "font/font-file", &gs_config.font.file,
                      DEFAULT_FONT_FILE, NULL),
    IPC_CONFIG_KV_POS(CONFIG_PARENT_KEY "position", &gs_config.position,
                      OSD_POS_TOP_RIGHT, NULL),
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

static bool timely_refresh_request(void *data) {
  struct private_data *priv = (struct private_data *)data;
  priv->timer.request = true;
  return IPC_TIMEOUT_TASK_CONTINUE;
}

static void calc_pos(int src_w, int src_h, int dst_w, int dst_h,
                     enum osd_position pos, int *x, int *y) {

  int xpos, ypos;
  int hvpad = 5;
  switch (pos) {
  case OSD_POS_TOP_LEFT:
    xpos = hvpad;
    ypos = hvpad;
    break;

  case OSD_POS_TOP_MID:
    xpos = (dst_w - src_w) / 2;
    ypos = hvpad;
    break;

  case OSD_POS_TOP_RIGHT:
    xpos = dst_w - src_w - hvpad;
    ypos = hvpad;
    break;

  case OSD_POS_MID_LEFT:
    xpos = hvpad;
    ypos = (dst_h - src_h) / 2;
    break;

  case OSD_POS_MID_RIGHT:
    xpos = dst_w - src_w - hvpad;
    ypos = (dst_h - src_h) / 2;
    break;

  case OSD_POS_CENTER:
    xpos = (dst_w - src_w) / 2;
    ypos = (dst_h - src_h) / 2;
    break;

  case OSD_POS_BOTTOM_LEFT:
    xpos = hvpad;
    ypos = dst_h - src_h - hvpad;
    break;

  case OSD_POS_BOTTOM_MID:
    xpos = (dst_w - src_w) / 2;
    ypos = dst_h - src_h - hvpad;
    break;

  case OSD_POS_BOTTOM_RIGHT:
    xpos = dst_w - src_w - hvpad;
    ypos = dst_h - src_h - hvpad;
    break;
  default:
    xpos = hvpad;
    ypos = hvpad;
    break;
  }

  if (xpos < 0 || xpos > dst_w)
    xpos = 0;
  if (ypos < 0 || ypos > dst_h)
    ypos = 0;

  if (x)
    *x = xpos;
  if (y)
    *y = ypos;
}

static AmlStatus update(struct AmlIPCOSDModule *object) {
  struct tm *t;
  time_t now;
  char buf[256];

  struct private_data *priv = (struct private_data *)object->priv;
  if (object->screen.width == 0) {
    // ignore while the video size not set
    return AML_STATUS_OK;
  }
  if (!gs_config.enabled || !priv->timer.request)
    return AML_STATUS_OK;

  priv->timer.request = false;

  now = time(NULL);

  t = localtime(&now);

  if (t == NULL)
    sprintf(buf, "--:--:--");

  if (strftime(buf, sizeof(buf), "%F %H:%M:%S", t) == 0)
    return AML_STATUS_FAIL;

  if (gs_config.changed) {
    if (priv->ctx) {
      aml_obj_release(AML_OBJECT(priv->ctx));
      priv->ctx = NULL;
    }
    if (priv->surface) {
      aml_obj_release(AML_OBJECT(priv->surface));
      priv->surface = NULL;
    }
    gs_config.changed = false;
  }

  if (priv->ctx == NULL) {
    priv->ctx = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(priv->ctx, gs_config.font.file, gs_config.font.size);
    aml_ipc_osd_set_color(priv->ctx, gs_config.font.color.fg);
    aml_ipc_osd_set_bgcolor(priv->ctx, gs_config.font.color.bg);
  }

  struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 0, 0, 0};
  // preload text to check the surface size
  aml_ipc_osd_preload_text(priv->ctx, buf, &fmt.width, &fmt.height);

  if (priv->surface) {
    if (AML_IPC_VIDEO_FRAME(priv->surface)->format.width < fmt.width ||
        AML_IPC_VIDEO_FRAME(priv->surface)->format.height < fmt.height) {
      // release surface if width / height is not match
      aml_obj_release(AML_OBJECT(priv->surface));
      priv->surface = NULL;
    }
  }
  if (priv->surface == NULL) {
    priv->surface = aml_ipc_osd_frame_new_alloc(&fmt);
  }

  // draw text surface
  aml_ipc_osd_start_paint(priv->ctx, AML_IPC_VIDEO_FRAME(priv->surface));
  aml_ipc_osd_clear_rect(priv->ctx, NULL); // slow
  int top = 0, left = 0, bottom = top, right = left;
  aml_ipc_osd_draw_text(priv->ctx, buf, &bottom, &right);
  aml_ipc_osd_done_paint(priv->ctx);

  int src_w = fmt.width;
  int src_h = fmt.height;
  int dst_w = object->screen.width;
  int dst_h = object->screen.height;
  calc_pos(src_w, src_h, dst_w, dst_h, gs_config.position, &left, &top);
  aml_ipc_osd_frame_set_src_rect(priv->surface, 0, 0, fmt.width, fmt.height);
  aml_ipc_osd_frame_set_dst_rect(priv->surface, left, top, fmt.width,
                                 fmt.height);

  return AML_STATUS_OK;
}

static AmlStatus render(struct AmlIPCOSDModule *object,
                        struct AmlIPCGE2D *render,
                        struct AmlIPCVideoFrame *dest_frame) {
  struct private_data *priv = (struct private_data *)(object->priv);
  if (!gs_config.enabled || priv->surface == NULL)
    return AML_STATUS_OK;
  if (object->screen.width == 0) {
    // screen size not set
    return AML_STATUS_OK;
  }
  struct AmlRect *dstrc = &priv->surface->dst_rect;
  aml_ipc_ge2d_blend(render, AML_IPC_VIDEO_FRAME(priv->surface), dest_frame,
                     dest_frame, NULL, dstrc->left, dstrc->top, dstrc->left,
                     dstrc->top);

  return AML_STATUS_OK;
}

static AmlStatus release(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);

  if (priv->surface) {
    aml_obj_release(AML_OBJECT(priv->surface));
    priv->surface = NULL;
  }
  if (priv->ctx) {
    aml_obj_release(AML_OBJECT(priv->ctx));
    priv->ctx = NULL;
  }
  free(priv);
  free(object);
  return AML_STATUS_OK;
}

static AmlStatus start(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);
  if (priv->timer.handle == 0) {
    load_configs();
    register_config_handle();

    priv->timer.request = true; // quickly start the first update
    priv->timer.handle =
        ipc_timeout_task_add(1000, timely_refresh_request, object->priv);
  }
  return AML_STATUS_OK;
}

static AmlStatus stop(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);
  unregister_config_handle();
  if (priv->timer.handle) {
    ipc_timeout_task_remove(priv->timer.handle);
    priv->timer.request = false;
  }
  FREE_STR(gs_config.font.file);
  return AML_STATUS_OK;
}

static struct AmlIPCOSDFrameList *getframes(struct AmlIPCOSDModule *object) {
  struct private_data *priv = (struct private_data *)(object->priv);
  if (gs_config.enabled && priv->surface == NULL) {
    return NULL;
  }

  struct AmlIPCOSDFrameList *frames = aml_ipc_osd_frame_list_new();
  if (gs_config.enabled && priv->surface) {
    aml_ipc_osd_frame_list_append(frames, priv->surface);
    // take away
    priv->surface = NULL;
  }
  return frames;
}

struct AmlIPCOSDModule *aml_ipc_osd_clock_new() {
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
