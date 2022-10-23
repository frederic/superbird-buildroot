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

#include "ipc_imgcap.h"
#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_jpegenc.h"
#include "ipc_timeout_task.h"
#include <aml_ipc_internal.h>
#include <dirent.h>
#include <ipc_property.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

AML_LOG_DEFINE(ipc_imgcap);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(ipc_imgcap)

#define IMGCAP_PARENT_KEY "/ipc/image-capture/"

typedef AmlStatus (*EncodeFrameFunc)(struct AmlIPCComponent *,
                                     struct AmlIPCVideoFrame *,
                                     struct AmlIPCFrame **);
typedef AmlStatus (*SetQualityFunc)(struct AmlIPCComponent *, int);

struct AmlIPCImgCapPriv {
  /*< private >*/
  struct AmlIPCComponent *jpegenc;
  struct AmlIPCGE2D *ge2d;
  EncodeFrameFunc encode_frame;
  SetQualityFunc set_quality;
  // thread related
  pthread_t tid;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  bool running;
  bool ready;
  struct AmlIPCVideoFrame *iframe;
};

struct imgcap_configs {
  char *location;
  bool use_hw_enc;
  int quality;
  int width;
  int height;
  bool trigger;
};

static struct imgcap_configs gs_config;

static void create_dir(const char *path) {
  DIR *dir = opendir(path);
  if (!dir) {
    if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
      AML_LOGE("create directory failed: %s", path);
    }
  }
  closedir(dir);
}

static char *build_filepath(const char *dir) {
  time_t t = time(NULL);
  char stime[64];
  strftime(stime, sizeof(stime), "%Y-%m-%d_%H-%M-%S", localtime(&t));
  int sz = snprintf(NULL, 0, "%s/%s.jpg", dir, stime) + 1;
  char *filepath = ipc_mem_new0(char, sz);
  snprintf(filepath, sz, "%s/%s.jpg", dir, stime);
  return filepath;
}

static void setup_encoder(struct AmlIPCImgCapPriv *priv) {
  if (priv->jpegenc) {
    aml_obj_release(AML_OBJECT(priv->jpegenc));
  }
  if (gs_config.use_hw_enc) {
    // hardware jpeg encoder
    priv->jpegenc = AML_IPC_COMPONENT(aml_ipc_jpegenc_new());
    priv->encode_frame = (EncodeFrameFunc)aml_ipc_jpegenc_encode_frame;
    priv->set_quality = (SetQualityFunc)aml_ipc_jpegenc_set_quality;
    AML_LOGD("set to use hw encoder");
  } else {
    // software jpeg encoder
    priv->jpegenc = AML_IPC_COMPONENT(aml_ipc_jpegencsw_new());
    priv->encode_frame = (EncodeFrameFunc)aml_ipc_jpegencsw_encode_frame;
    priv->set_quality = (SetQualityFunc)aml_ipc_jpegencsw_set_quality;
    AML_LOGD("set to use sw encoder");
  }
  priv->set_quality(priv->jpegenc, gs_config.quality);
}

void onchange_quality(void *user_data) {
  struct AmlIPCImgCap *imgcap = AML_IPC_IMG_CAP(user_data);
  struct AmlIPCImgCapPriv *priv = imgcap->priv;
  pthread_mutex_lock(&priv->lock);
  priv->set_quality(priv->jpegenc, gs_config.quality);
  pthread_mutex_unlock(&priv->lock);
}

void onchange_encoder_type(void *user_data) {
  struct AmlIPCImgCap *imgcap = AML_IPC_IMG_CAP(user_data);
  struct AmlIPCImgCapPriv *priv = imgcap->priv;
  pthread_mutex_lock(&priv->lock);
  setup_encoder(priv);
  pthread_mutex_unlock(&priv->lock);
}

static struct ipc_config_kv gs_kvs[] = {
    IPC_CONFIG_KV_BOOL(IMGCAP_PARENT_KEY "trigger", &gs_config.trigger, false,
                       NULL),
    IPC_CONFIG_KV_STR(IMGCAP_PARENT_KEY "location", &gs_config.location,
                      "/tmp/ipc-webui-downloads", NULL),
    IPC_CONFIG_KV_I(IMGCAP_PARENT_KEY "quality", &gs_config.quality, 80,
                    onchange_quality),
    IPC_CONFIG_KV_BOOL(IMGCAP_PARENT_KEY "use-hw-encoder",
                       &gs_config.use_hw_enc, false, onchange_encoder_type),
    IPC_CONFIG_KV_I(IMGCAP_PARENT_KEY "width", &gs_config.width, 0, NULL),
    IPC_CONFIG_KV_I(IMGCAP_PARENT_KEY "height", &gs_config.height, 0, NULL),
    {NULL},
};

static void load_configs() {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    ipc_config_load_kv(&gs_kvs[i]);
  }
}

static void on_config_changed(const char *key, const char *val,
                              void *user_data) {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    if (strcmp(gs_kvs[i].key, key) == 0) {
      ipc_config_load_kv(&gs_kvs[i]);
      if (gs_kvs[i].handle) {
        gs_kvs[i].handle(user_data);
      }
      break;
    }
  }
}

static AmlStatus register_config_handle(struct AmlIPCImgCap *imgcap) {
  return ipc_config_register_handle(IMGCAP_PARENT_KEY, on_config_changed,
                                    imgcap);
}

static AmlStatus unregister_config_handle() {
  return ipc_config_unregister_handle(IMGCAP_PARENT_KEY, on_config_changed);
}

static void *imgcap_process(void *data) {
  struct AmlIPCImgCapPriv *priv = (struct AmlIPCImgCapPriv *)data;
  struct AmlIPCFrame *outframe = NULL;
  while (priv->running) {
    pthread_mutex_lock(&priv->lock);
    while (!priv->ready) {
      pthread_cond_wait(&priv->cond, &priv->lock);
    }
    if (priv->iframe == NULL)
      goto loop_continue;
    // use jpegenc to encode one video frame to file
    if (priv->encode_frame(priv->jpegenc, priv->iframe, &outframe) ==
        AML_STATUS_OK) {
      // create filepath
      create_dir(gs_config.location);
      char *filepath = build_filepath(gs_config.location);
      FILE *fp = NULL;
      if ((fp = fopen(filepath, "wb")) == NULL) {
        AML_LOGE("can't open %s", filepath);
        goto loop_continue;
      }
      fwrite(outframe->data, outframe->size, 1, fp);
      fclose(fp);

      aml_obj_release(AML_OBJECT(outframe));

      AML_LOGD("frame captured: %s", filepath);

      ipc_property_set(IMGCAP_PARENT_KEY "imagefile", filepath);
      free(filepath);
    }
  loop_continue:
    priv->ready = false;
    if (priv->iframe) {
      aml_obj_release(AML_OBJECT(priv->iframe));
      priv->iframe = NULL;
    }
    pthread_mutex_unlock(&priv->lock);
  }
  return NULL;
}

static AmlStatus handle_frame(struct AmlIPCComponent *self, int ch,
                              struct AmlIPCFrame *frame) {
  struct AmlIPCVideoFrame *vframe = AML_IPC_VIDEO_FRAME(frame);
  struct AmlIPCImgCap *imgcap = AML_IPC_IMG_CAP(self);
  struct AmlIPCImgCapPriv *priv = imgcap->priv;
  AmlStatus ret = AML_STATUS_OK;

  if (gs_config.trigger) {
    // need to remove imagefile property first
    ipc_config_remove(IMGCAP_PARENT_KEY "trigger");
    ipc_config_remove(IMGCAP_PARENT_KEY "imagefile");
    gs_config.trigger = false;

    if (pthread_mutex_trylock(&priv->lock) == 0) {
      int w = gs_config.width;
      int h = gs_config.height;
      if (gs_config.width == 0 || gs_config.width > vframe->format.width) {
        w = vframe->format.width;
      }

      if (gs_config.height == 0 || gs_config.height > vframe->format.height) {
        h = vframe->format.height;
      }
      // duplicate input image
      // prevent image polluted by the afterwards steps
      struct AmlIPCVideoFormat format = {
          gs_config.use_hw_enc ? AML_PIXFMT_NV12 : AML_PIXFMT_RGB_888, w, h, 0};
      struct AmlIPCVideoFrame *iframe =
          AML_IPC_VIDEO_FRAME(aml_ipc_osd_frame_new_alloc(&format));

      if (priv->ge2d) {
        if (aml_ipc_ge2d_stretchblt(priv->ge2d, vframe, iframe, NULL, NULL) !=
            AML_STATUS_OK) {
          aml_obj_release(AML_OBJECT(iframe));
          iframe = NULL;
          AML_LOGE("failed to duplicate the frame");
        }
      }
      if (iframe) {
        if (priv->iframe)
          aml_obj_release(AML_OBJECT(priv->iframe));
        priv->iframe = iframe;
        priv->ready = true;
        pthread_cond_signal(&priv->cond);
      }
      pthread_mutex_unlock(&priv->lock);
    }
  }

  aml_obj_release(AML_OBJECT(frame));

  return ret;
}

static AmlStatus start(struct AmlIPCComponent *comp) {
  struct AmlIPCImgCap *imgcap = AML_IPC_IMG_CAP(comp);
  struct AmlIPCImgCapPriv *priv = imgcap->priv;

  load_configs();

  setup_encoder(priv);

  priv->running = true;
  pthread_create(&priv->tid, NULL, imgcap_process, priv);

  register_config_handle(imgcap);
  return AML_STATUS_OK;
}

static AmlStatus stop(struct AmlIPCComponent *comp) {
  struct AmlIPCImgCap *imgcap = AML_IPC_IMG_CAP(comp);
  struct AmlIPCImgCapPriv *priv = imgcap->priv;
  unregister_config_handle();
  if (priv->tid > 0) {
    priv->running = false;
    pthread_mutex_lock(&priv->lock);
    priv->ready = true;
    pthread_cond_signal(&priv->cond);
    pthread_mutex_unlock(&priv->lock);
    pthread_join(priv->tid, NULL);

    priv->tid = 0;
  }
  return AML_STATUS_OK;
}

static void class_init(struct AmlIPCComponentClass *cls) {
  cls->start = start;
  cls->handle_frame = handle_frame;
  cls->stop = stop;
}

static void comp_init(struct AmlIPCImgCap *imgcap) {
  AML_ALLOC_PRIV(imgcap->priv);
  aml_ipc_add_channel(AML_IPC_COMPONENT(imgcap), AML_IMG_CAP_INPUT,
                      AML_CHANNEL_INPUT, "input");
}

static void comp_free(struct AmlIPCImgCap *imgcap) {
  if (imgcap == NULL || imgcap->priv == NULL)
    return;
  struct AmlIPCImgCapPriv *priv = imgcap->priv;
  stop(AML_IPC_COMPONENT(imgcap));
  if (priv->ge2d) {
    aml_obj_release(AML_OBJECT(priv->ge2d));
    priv->ge2d = NULL;
  }
  if (priv->jpegenc) {
    aml_obj_release(AML_OBJECT(priv->jpegenc));
    priv->jpegenc = NULL;
  }

  pthread_cond_destroy(&priv->cond);
  pthread_mutex_destroy(&priv->lock);

  free(imgcap->priv);
  imgcap->priv = NULL;
}

AML_OBJ_DEFINE_TYPEID(AmlIPCImgCap, AmlIPCComponent, AmlIPCComponentClass,
                      class_init, comp_init, comp_free);

struct AmlIPCImgCap *aml_ipc_imgcap_new() {
  struct AmlIPCImgCap *imgcap = AML_OBJ_NEW(AmlIPCImgCap);
  struct AmlIPCImgCapPriv *priv = imgcap->priv;

  ipc_config_remove(IMGCAP_PARENT_KEY "trigger");
  ipc_config_remove(IMGCAP_PARENT_KEY "imagefile");

  priv->ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_TO);
  pthread_cond_init(&priv->cond, NULL);
  pthread_mutex_init(&priv->lock, NULL);
  priv->ready = false;

  return imgcap;
}
