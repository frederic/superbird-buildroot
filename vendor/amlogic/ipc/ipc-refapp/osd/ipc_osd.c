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
#include <aml_ipc_internal.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>

#include "ipc_common.h"
#include "ipc_osd.h"
#include "ipc_osd_module.h"
#include "ipc_timeout_task.h"

AML_LOG_DEFINE(osdapp);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(osdapp)

#define IPC_OSD_BUFFER_NUM 2

struct AmlIPCOSDAppPriv {
  enum AmlIPCOSDAppRefreshMode mode;
  struct AmlIPCOSDApp *parent;
  struct AmlIPCGE2D *blender; // external blender
  struct AmlIPCGE2D *render;  // internal render
  int plane_idx;
  struct {
    pthread_t tid;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool running;
    bool update;
  } thread;

  struct {
    int width;
    int height;
    struct AmlIPCOSDFrame *frames[IPC_OSD_BUFFER_NUM]; // fullscreen osd buffer
  } surface;

  struct {
    unsigned long refresh;
  } timer;

  struct AmlIPCOSDModuleList *modules;
};

static void *osd_process(void *data) {
  struct AmlIPCOSDApp *osd = AML_IPC_OSD_APP(data);
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  prctl(PR_SET_NAME, "osd-process", NULL, NULL, NULL);
  while (priv->thread.running) {
    pthread_mutex_lock(&priv->thread.lock);
    while (priv->thread.update == false) {
      pthread_cond_wait(&priv->thread.cond, &priv->thread.lock);
    }
    priv->thread.update = false;

    if (!priv->thread.running) {
      goto process_exit;
    }

    if (priv->surface.width == 0 || priv->surface.height == 0) {
      pthread_mutex_unlock(&priv->thread.lock);
      continue;
    }

    ipc_osd_module_set_resolution(priv->modules, priv->surface.width,
                                  priv->surface.height);

    ipc_osd_module_update(priv->modules);

    // create blender
    if (priv->blender == NULL) {
      priv->blender = aml_ipc_ge2d_new(AML_GE2D_OP_ALPHABLEND_TO);
      aml_ipc_bind(AML_IPC_COMPONENT(priv->blender), AML_GE2D_OUTPUT,
                   AML_IPC_COMPONENT(priv->parent), AML_OSD_APP_INTERNAL);
    }

    if (priv->mode == AML_OSD_APP_REFRESH_GLOBAL) {
      if (priv->surface.frames[0] &&
          (AML_IPC_VIDEO_FRAME(priv->surface.frames[0])->format.width !=
               priv->surface.width ||
           AML_IPC_VIDEO_FRAME(priv->surface.frames[0])->format.height !=
               priv->surface.height)) {
        for (int i = 0; i < IPC_OSD_BUFFER_NUM; i++) {
          aml_obj_release(AML_OBJECT(priv->surface.frames[i]));
          priv->surface.frames[i] = NULL;
        }
      }

      if (priv->surface.frames[0] == NULL) {
        // osd surface not initialized
        struct AmlIPCVideoFormat fmt = {
            AML_PIXFMT_RGBA_8888, priv->surface.width, priv->surface.height, 0};
        for (int i = 0; i < IPC_OSD_BUFFER_NUM; i++) {
          priv->surface.frames[i] = aml_ipc_osd_frame_new_alloc(&fmt);
          aml_ipc_osd_frame_set_src_rect(priv->surface.frames[i], 0, 0,
                                         fmt.width, fmt.height);
          aml_ipc_osd_frame_set_dst_rect(priv->surface.frames[i], 0, 0,
                                         fmt.width, fmt.height);
        }
      }

      struct AmlIPCOSDFrame *current_surface =
          priv->surface.frames[priv->plane_idx];
      // clear the buffer
      aml_ipc_ge2d_fill_rect(priv->render, AML_IPC_VIDEO_FRAME(current_surface),
                             NULL, AML_RGBA(0, 0, 0, 0));
      // do overlay render
      ipc_osd_module_render(priv->modules, priv->render,
                            AML_IPC_VIDEO_FRAME(current_surface));

      // flip new buffer
      // surface would be unref while removing from static buffer
      // add ref here
      aml_obj_addref(AML_OBJECT(current_surface));
      aml_ipc_ge2d_set_static_buffer(priv->blender,
                                     AML_IPC_FRAME(current_surface));
      priv->plane_idx = (priv->plane_idx + 1) % IPC_OSD_BUFFER_NUM;
    } else {
      ipc_osd_module_set_buffer(priv->modules, priv->blender);
    }

    pthread_mutex_unlock(&priv->thread.lock);
  }

process_exit:
  ipc_osd_module_release(priv->modules);
  priv->modules = NULL;
  return NULL;
}

static bool request_refresh_osd(void *data) {
  struct AmlIPCOSDApp *osd = AML_IPC_OSD_APP(data);
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  pthread_mutex_lock(&priv->thread.lock);
  priv->thread.update = true;
  pthread_cond_signal(&priv->thread.cond);
  pthread_mutex_unlock(&priv->thread.lock);
  // continue to run this timer
  return IPC_TIMEOUT_TASK_CONTINUE;
}

static AmlStatus osd_app_start(struct AmlIPCComponent *comp) {
  struct AmlIPCOSDApp *osd = AML_IPC_OSD_APP(comp);
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  if (priv->thread.running) {
    AML_LOGD("osd already started");
    return AML_STATUS_OK;
  }
  ipc_osd_module_start(priv->modules);
  priv->thread.running = true;
  // trigger first time blend
  priv->thread.update = true;
  pthread_create(&priv->thread.tid, NULL, osd_process, osd);
  AML_LOGD("osd started");
  priv->timer.refresh = ipc_timeout_task_add(33, request_refresh_osd, osd);
  return AML_STATUS_OK;
}

static AmlStatus osd_app_stop(struct AmlIPCComponent *comp) {
  struct AmlIPCOSDApp *osd = AML_IPC_OSD_APP(comp);
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  if (!priv->thread.running) {
    AML_LOGD("osd already stopped");
    return AML_STATUS_OK;
  }
  aml_ipc_comp_flush(comp, true);
  ipc_osd_module_stop(priv->modules);
  ipc_timeout_task_remove(priv->timer.refresh);
  pthread_mutex_lock(&priv->thread.lock);
  priv->thread.running = false;
  priv->thread.update = true;
  pthread_cond_signal(&priv->thread.cond);
  pthread_mutex_unlock(&priv->thread.lock);
  pthread_join(priv->thread.tid, NULL);
  priv->thread.tid = 0;
  for (int i = 0; i < IPC_OSD_BUFFER_NUM; i++) {
    if (priv->surface.frames[i]) {
      aml_obj_release(AML_OBJECT(priv->surface.frames[i]));
      priv->surface.frames[i] = NULL;
    }
  }
  AML_LOGD("osd stopped");
  return AML_STATUS_OK;
}

static AmlStatus osd_app_handle_frame(struct AmlIPCComponent *self, int ch,
                                      struct AmlIPCFrame *frame) {
  struct AmlIPCOSDApp *osd = AML_IPC_OSD_APP(self);
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  struct AmlIPCVideoFrame *vfrm = AML_IPC_VIDEO_FRAME(frame);

  if (priv->surface.width != vfrm->format.width ||
      priv->surface.height != vfrm->format.height) {
    priv->surface.width = vfrm->format.width;
    priv->surface.height = vfrm->format.height;
  }

  if (priv->blender && ch == AML_OSD_APP_INPUT) {
    return aml_ipc_send_frame(AML_IPC_COMPONENT(priv->blender), AML_GE2D_CH0,
                              frame);
  }
  return aml_ipc_send_frame(self, AML_OSD_APP_OUTPUT, frame);
}

static void osd_app_class_init(struct AmlIPCComponentClass *cls) {
  cls->start = osd_app_start;
  cls->handle_frame = osd_app_handle_frame;
  cls->stop = osd_app_stop;
}

static void osd_app_init(struct AmlIPCOSDApp *osd) {
  AML_ALLOC_PRIV(osd->priv);
  aml_ipc_add_channel(AML_IPC_COMPONENT(osd), AML_OSD_APP_INPUT,
                      AML_CHANNEL_INPUT, "input");
  aml_ipc_add_channel(AML_IPC_COMPONENT(osd), AML_OSD_APP_INTERNAL,
                      AML_CHANNEL_INPUT, "internal");
  aml_ipc_add_channel(AML_IPC_COMPONENT(osd), AML_OSD_APP_OUTPUT,
                      AML_CHANNEL_OUTPUT, "output");
}

static void osd_app_free(struct AmlIPCOSDApp *osd) {
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  osd_app_stop(AML_IPC_COMPONENT(osd));
  aml_obj_release(AML_OBJECT(priv->render));
  priv->render = NULL;
  if (priv->blender) {
    aml_obj_release(AML_OBJECT(priv->blender));
    priv->blender = NULL;
  }
  free(osd->priv);
  osd->priv = NULL;
  AML_LOGD("osd deinitialized");
}

AML_OBJ_DEFINE_TYPEID(AmlIPCOSDApp, AmlIPCComponent, AmlIPCComponentClass,
                      osd_app_class_init, osd_app_init, osd_app_free);

AmlStatus aml_ipc_osd_app_set_refresh_mode(struct AmlIPCOSDApp *osd,
                                           enum AmlIPCOSDAppRefreshMode mode) {
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  priv->mode = mode;
  return AML_STATUS_OK;
}

struct AmlIPCOSDApp *aml_ipc_osd_app_new() {
  struct AmlIPCOSDApp *osd = AML_OBJ_NEW(AmlIPCOSDApp);
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  priv->mode = AML_OSD_APP_REFRESH_GLOBAL;
  priv->parent = osd;
  priv->render = aml_ipc_ge2d_new(AML_GE2D_OP_ALPHABLEND_TO);
  priv->plane_idx = 0;

  pthread_mutex_init(&priv->thread.lock, NULL);
  pthread_cond_init(&priv->thread.cond, NULL);
  priv->thread.update = false;

  priv->modules = ipc_osd_module_new();

  return osd;
}

AmlStatus aml_ipc_osd_app_add_module(struct AmlIPCOSDApp *osd,
                                     struct AmlIPCOSDModule *module) {
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  return ipc_osd_module_append(priv->modules, module);
}

AmlStatus aml_ipc_osd_app_remove_module(struct AmlIPCOSDApp *osd,
                                        struct AmlIPCOSDModule *module) {
  struct AmlIPCOSDAppPriv *priv = osd->priv;
  return ipc_osd_module_remove(priv->modules, module);
}
