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

#ifndef __IPC_OSD_MODULE_H__
#define __IPC_OSD_MODULE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FONT_FILE "/usr/share/directfb-1.7.7/decker.ttf"

struct osd_attr_color {
  int fg; // fg color
  int bg; // bg color
};

struct osd_attr_font {
  int size;   // font size
  char *file; // font file
  struct osd_attr_color color;
};

struct osd_attr_rect {
  int border; // border size
  int color;  // rect color
};

struct AmlIPCOSDModuleBuffer {
  TAILQ_ENTRY(AmlIPCOSDModuleBuffer) node;
  struct AmlIPCOSDFrame *frame;
  struct AmlIPCOSDModule *owner;
};

struct AmlIPCOSDModuleBufferList {
  TAILQ_HEAD(AmlIPCOSDModuleBufferListHead, AmlIPCOSDModuleBuffer) head;
};

struct AmlIPCOSDModule {
  TAILQ_ENTRY(AmlIPCOSDModule) node;
  void *priv;
  struct {
    int width;
    int height;
  } screen;
  AmlStatus (*start)(struct AmlIPCOSDModule *obj);
  AmlStatus (*stop)(struct AmlIPCOSDModule *obj);
  AmlStatus (*update)(struct AmlIPCOSDModule *obj);
  AmlStatus (*render)(struct AmlIPCOSDModule *obj, struct AmlIPCGE2D *render,
                      struct AmlIPCVideoFrame *frame);
  AmlStatus (*release)(struct AmlIPCOSDModule *obj);

  // apis for partial refresh mode
  // get all of the osd frames
  // and take the owner ship for display module
  // return NULL if no update
  struct AmlIPCOSDFrameList *(*getframes)(struct AmlIPCOSDModule *obj);
};

struct AmlIPCOSDModuleList {
  TAILQ_HEAD(AmlIPCOSDModuleListHead, AmlIPCOSDModule) head;

  // members and apis for partial refresh mode
  struct AmlIPCOSDModuleBufferList buffers;

  pthread_mutex_t lock;
};

struct AmlIPCOSDModuleList *ipc_osd_module_new();
AmlStatus ipc_osd_module_append(struct AmlIPCOSDModuleList *list,
                                struct AmlIPCOSDModule *object);
AmlStatus ipc_osd_module_remove(struct AmlIPCOSDModuleList *list,
                                struct AmlIPCOSDModule *object);
AmlStatus ipc_osd_module_start(struct AmlIPCOSDModuleList *list);
AmlStatus ipc_osd_module_update(struct AmlIPCOSDModuleList *list);
AmlStatus ipc_osd_module_render(struct AmlIPCOSDModuleList *list,
                                struct AmlIPCGE2D *render,
                                struct AmlIPCVideoFrame *frame);
AmlStatus ipc_osd_module_stop(struct AmlIPCOSDModuleList *list);
AmlStatus ipc_osd_module_release(struct AmlIPCOSDModuleList *list);

// for partial refresh mode only
AmlStatus ipc_osd_module_set_buffer(struct AmlIPCOSDModuleList *list,
                                    struct AmlIPCGE2D *blender);
AmlStatus ipc_osd_module_set_resolution(struct AmlIPCOSDModuleList *list,
                                        int width, int height);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IPC_OSD_MODULE_H__ */
