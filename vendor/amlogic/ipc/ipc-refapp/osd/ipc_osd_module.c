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

#include <sys/queue.h>
#include <stdlib.h>
#include <aml_ipc_internal.h>

#include "ipc_common.h"
#include "ipc_osd_module.h"

struct AmlIPCOSDModuleList *ipc_osd_module_new() {
  struct AmlIPCOSDModuleList *l = ipc_mem_new0(struct AmlIPCOSDModuleList, 1);
  TAILQ_INIT(&l->head);
  TAILQ_INIT(&l->buffers.head);
  pthread_mutex_init(&l->lock, NULL);
  return l;
}

AmlStatus ipc_osd_module_append(struct AmlIPCOSDModuleList *list,
                                struct AmlIPCOSDModule *object) {
  pthread_mutex_lock(&list->lock);
  TAILQ_INSERT_TAIL(&list->head, object, node);
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_osd_module_remove(struct AmlIPCOSDModuleList *list,
                                struct AmlIPCOSDModule *object) {
  pthread_mutex_lock(&list->lock);
  TAILQ_REMOVE(&list->head, object, node);
  // remove buffers owned by this object
  struct AmlIPCOSDModuleBuffer *b;
  b = TAILQ_FIRST(&list->buffers.head);
  while (b) {
    struct AmlIPCOSDModuleBuffer *next = TAILQ_NEXT(b, node);
    if (b->owner == object) {
      if (aml_obj_refcnt(AML_OBJECT(b->frame)) > 1) {
        aml_obj_release(AML_OBJECT(b->frame));
      }
      TAILQ_REMOVE(&list->buffers.head, b, node);
      free(b);
    }
    b = next;
  }
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_osd_module_start(struct AmlIPCOSDModuleList *list) {
  struct AmlIPCOSDModule *object;
  pthread_mutex_lock(&list->lock);
  TAILQ_FOREACH(object, &list->head, node) {
    if (object->start) {
      object->start(object);
    }
  }
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_osd_module_update(struct AmlIPCOSDModuleList *list) {
  struct AmlIPCOSDModule *object;
  pthread_mutex_lock(&list->lock);
  TAILQ_FOREACH(object, &list->head, node) {
    if (object->update) {
      object->update(object);
    }
  }
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_osd_module_render(struct AmlIPCOSDModuleList *list,
                                struct AmlIPCGE2D *render,
                                struct AmlIPCVideoFrame *frame) {
  struct AmlIPCOSDModule *object;
  pthread_mutex_lock(&list->lock);
  TAILQ_FOREACH(object, &list->head, node) {
    if (object->render) {
      object->render(object, render, frame);
    }
  }
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_osd_module_stop(struct AmlIPCOSDModuleList *list) {
  struct AmlIPCOSDModule *object;
  pthread_mutex_lock(&list->lock);
  TAILQ_FOREACH(object, &list->head, node) {
    if (object->stop) {
      object->stop(object);
    }
  }
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_osd_module_release(struct AmlIPCOSDModuleList *list) {
  struct AmlIPCOSDModule *object;
  pthread_mutex_lock(&list->lock);
  while ((object = TAILQ_FIRST(&list->head))) {
    TAILQ_REMOVE(&list->head, object, node);
    if (object->release) {
      object->release(object);
    }
  }
  // release all of the buffers
  while (!TAILQ_EMPTY(&list->buffers.head)) {
    struct AmlIPCOSDModuleBuffer *b = TAILQ_FIRST(&list->buffers.head);
    if (aml_obj_refcnt(AML_OBJECT(b->frame)) > 1) {
      aml_obj_release(AML_OBJECT(b->frame));
    }
    TAILQ_REMOVE(&list->buffers.head, b, node);
    free(b);
  }
  pthread_mutex_unlock(&list->lock);
  free(list);
  return AML_STATUS_OK;
}

// for partial refresh mode only
AmlStatus ipc_osd_module_set_buffer(struct AmlIPCOSDModuleList *list,
                                    struct AmlIPCGE2D *blender) {
  struct AmlIPCOSDModule *object;
  pthread_mutex_lock(&list->lock);
  TAILQ_FOREACH(object, &list->head, node) {
    struct AmlIPCOSDFrameList *buffers = NULL;
    if (object->getframes) {
      buffers = object->getframes(object);
      // NULL means no update, use the previous
    }
    // update frames to buffer list if new list is not null
    if (buffers) {
      struct AmlIPCOSDModuleBuffer *buffer;
      struct AmlIPCOSDModuleBuffer *pos = NULL;

      // find the old buffer
      TAILQ_FOREACH(buffer, &list->buffers.head, node) {
        if (buffer->owner == object) {
          buffer->owner = NULL;
          if (pos == NULL) {
            pos = buffer;
          }
        }
      }
      // insert new buffers
      struct AmlIPCOSDFrame *frame;
      TAILQ_FOREACH(frame, &buffers->head, node) {
        struct AmlIPCOSDModuleBuffer *new_buffer =
            ipc_mem_new0(struct AmlIPCOSDModuleBuffer, 1);
        new_buffer->frame = frame;
        new_buffer->owner = object;
        // insert into the original position
        // prevent blink
        if (pos) {
          TAILQ_INSERT_BEFORE(pos, new_buffer, node);
        } else {
          TAILQ_INSERT_TAIL(&list->buffers.head, new_buffer, node);
        }
      }
      TAILQ_INIT(&buffers->head);
      aml_obj_release(AML_OBJECT(buffers));
    } else {
      // add ref to prevent release
      struct AmlIPCOSDModuleBuffer *b;
      TAILQ_FOREACH(b, &list->buffers.head, node) {
        if (b->owner == object) {
          aml_obj_addref(AML_OBJECT(b->frame));
        }
      }
    }
  }

  struct AmlIPCOSDFrameList *frames = aml_ipc_osd_frame_list_new();
  // release the previous buffer list
  // prevent the damage to the linkage of the chain
  aml_ipc_ge2d_set_static_buffer(blender, AML_IPC_FRAME(frames));

  struct AmlIPCOSDModuleBuffer *b;

  // link buffers to the buffer list
  aml_ipc_ge2d_lock_static_buffer(blender);
  TAILQ_FOREACH(b, &list->buffers.head, node) {
    if (b->owner) {
      aml_ipc_osd_frame_list_append(frames, b->frame);
    }
  }
  aml_ipc_ge2d_unlock_static_buffer(blender);

  // remove unused buffers
  b = TAILQ_FIRST(&list->buffers.head);
  while (b) {
    struct AmlIPCOSDModuleBuffer *next = TAILQ_NEXT(b, node);
    if (b->owner == NULL) {
      TAILQ_REMOVE(&list->buffers.head, b, node);
      free(b);
    }
    b = next;
  }
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}

AmlStatus ipc_osd_module_set_resolution(struct AmlIPCOSDModuleList *list,
                                        int width, int height) {
  struct AmlIPCOSDModule *object;
  pthread_mutex_lock(&list->lock);
  TAILQ_FOREACH(object, &list->head, node) {
    if (object->screen.width != width || object->screen.height != height) {
      object->screen.width = width;
      object->screen.height = height;
    }
  }
  pthread_mutex_unlock(&list->lock);
  return AML_STATUS_OK;
}
