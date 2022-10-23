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
#include <nn_detect.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/prctl.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <unistd.h>

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_jpegenc.h"
#include "ipc_nn.h"
#include "ipc_timeout_task.h"
#include "recognition_db.h"

AML_LOG_DEFINE(nn);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(nn)

struct nn_result_buffer {
  int amount; // amount of result
  struct nn_result *results;
};

struct model_info {
  det_model_type model;
  det_param_t param;
  bool initialized;
  int width;
  int height;
  int channel;
  int rowbytes;
  int stride;
  struct AmlIPCVideoFrame *vframe;
};

struct db_param {
  char *format;
  float threshold;
  char *file;
  void *handle;
  bool bstore_face;
};

struct recognized_info {
  LIST_ENTRY(recognized_info) node;
  int uid;
  unsigned long timestamp;
};

struct AmlIPCNNPriv {
  struct AmlIPCNN *parent;
  bool enabled;
  bool flushing;
  struct db_param db_param;

  /*< private >*/
  struct model_info face_det, face_recog;
  struct AmlIPCVideoFrame *input_frame;
  struct AmlIPCGE2D *ge2d;

  pthread_t tid;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  bool ready;
  bool running;
  bool bypass_frame_handle;

  char *custimg;

  struct {
    LIST_HEAD(recognized_info_head, recognized_info) faces;
    char *script;
    int interval;
  } recognized_action;
};

static struct AmlValueEnum detection_models[] = {
    {"yoloface-v2", AML_NN_DET_YOLOFACE},
    {"mtcnn-v1", AML_NN_DET_MTCNN},
    {"aml_face_detection", AML_NN_DET_FACE},
    {"disable", AML_NN_DET_DISABLE},
    {NULL, 0}};

static struct AmlValueEnum recognition_models[] = {
    {"facenet", AML_NN_RECOG_FACENET},
    {"aml_face_recognition", AML_NN_RECOG_FACE},
    {"disable", AML_NN_RECOG_DISABLE},
    {NULL, 0}};

static AmlStatus send_frame(struct AmlIPCNNPriv *priv,
                            struct nn_result_buffer *resbuf);

struct configs {
  bool enabled;
  struct {
    char *model;
    int limits;
  } detection;
  struct {
    char *db_path;
    char *info_string;
    char *model;
    float threshold;
    char *on_recognized_script;
    int script_trigger_interval;
  } recognition;
  char *custom_image;
  bool store_face;
};

static struct configs gs_config;
#define CONFIG_PARENT_KEY "/ipc/nn/"
#define CONFIG_STORE_FACE_KEY CONFIG_PARENT_KEY "store-face"
#define CONFIG_CUSTOM_IMAGE_KEY CONFIG_PARENT_KEY "custom-image"

static det_model_type det_model_to_internal(enum AmlIPCNNDetectionModel model) {
  det_model_type type = DET_BUTT;
  switch (model) {
  case AML_NN_DET_YOLOFACE:
    type = DET_YOLOFACE_V2;
    break;
  case AML_NN_DET_MTCNN:
    type = DET_MTCNN_V1;
    break;
  case AML_NN_DET_FACE:
    type = DET_AML_FACE_DETECTION;
    break;
  default:
    break;
  }
  return type;
}

static det_model_type
recog_model_to_internal(enum AmlIPCNNRecognitionModel model) {
  det_model_type type = DET_BUTT;
  switch (model) {
  case AML_NN_RECOG_FACENET:
    type = DET_FACENET;
    break;
  case AML_NN_RECOG_FACE:
    type = DET_AML_FACE_RECOGNITION;
    break;
  default:
    break;
  }
  return type;
}

static enum AmlIPCNNDetectionModel
det_model_str_to_model(const char *strmodel) {
  int i = 0;
  enum AmlIPCNNDetectionModel m = AML_NN_DET_DISABLE;
  while (detection_models[i].name) {
    if (strcmp(detection_models[i].name, strmodel) == 0) {
      m = (enum AmlIPCNNDetectionModel)detection_models[i].val;
      break;
    }
    i++;
  }
  return m;
}

static enum AmlIPCNNRecognitionModel
recog_model_str_to_model(const char *strmodel) {
  int i = 0;
  enum AmlIPCNNRecognitionModel m = AML_NN_RECOG_DISABLE;
  while (recognition_models[i].name) {
    if (strcmp(recognition_models[i].name, strmodel) == 0) {
      m = (enum AmlIPCNNRecognitionModel)recognition_models[i].val;
      break;
    }
    i++;
  }
  return m;
}

void onchanged_module_enabled(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  aml_ipc_nn_set_enabled(priv->parent, gs_config.enabled);
}

void onchange_detection_model(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  aml_ipc_nn_set_detection_model(
      priv->parent, det_model_str_to_model(gs_config.detection.model));
}

void onchange_detection_limits(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  aml_ipc_nn_set_detection_limits(priv->parent, gs_config.detection.limits);
}

void onchange_recognition_model(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  aml_ipc_nn_set_recognition_model(
      priv->parent, recog_model_str_to_model(gs_config.recognition.model));
}

void onchange_db_param(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  aml_ipc_nn_set_recognition_threshold(priv->parent,
                                       gs_config.recognition.threshold);
  aml_ipc_nn_set_result_format(priv->parent, gs_config.recognition.info_string);
  aml_ipc_nn_set_db_path(priv->parent, gs_config.recognition.db_path);
}

void onchange_custom_img(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  aml_ipc_nn_trigger_face_capture_from_image(priv->parent,
                                             gs_config.custom_image);
  ipc_config_remove(CONFIG_CUSTOM_IMAGE_KEY);
}

void onchange_store_face(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  if (gs_config.store_face) {
    aml_ipc_nn_trigger_face_capture(priv->parent);
  }
  ipc_config_remove(CONFIG_STORE_FACE_KEY);
}

void onchange_others(void *user_data) {
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)user_data;
  aml_ipc_nn_set_recognition_action_hook(
      priv->parent, gs_config.recognition.on_recognized_script);
  aml_ipc_nn_set_recognition_trigger_interval(
      priv->parent, gs_config.recognition.script_trigger_interval);
}

static struct ipc_config_kv gs_kvs[] = {
    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "enabled", &gs_config.enabled, true,
                       onchanged_module_enabled),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "detection/model",
                      &gs_config.detection.model, "aml_face_detection",
                      onchange_detection_model),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "detection/limits",
                    &gs_config.detection.limits, 10, onchange_detection_limits),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "recognition/model",
                      &gs_config.recognition.model, "aml_face_recognition",
                      onchange_recognition_model),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "recognition/info-string",
                      &gs_config.recognition.info_string, "name",
                      onchange_db_param),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "recognition/db-path",
                      &gs_config.recognition.db_path, "", onchange_db_param),
    IPC_CONFIG_KV_FLOAT(CONFIG_PARENT_KEY "recognition/threshold",
                        &gs_config.recognition.threshold, "",
                        onchange_db_param),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "recognition/on-recognized-script",
                      &gs_config.recognition.on_recognized_script, "",
                      onchange_others),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "recognition/script-trigger-interval",
                    &gs_config.recognition.script_trigger_interval, 1,
                    onchange_others),
    IPC_CONFIG_KV_STR(CONFIG_CUSTOM_IMAGE_KEY, &gs_config.custom_image, "",
                      onchange_custom_img),
    IPC_CONFIG_KV_BOOL(CONFIG_STORE_FACE_KEY, &gs_config.store_face, false,
                       onchange_store_face),
    {NULL},
};

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

static AmlStatus register_config_handle(struct AmlIPCNNPriv *priv) {
  return ipc_config_register_handle(CONFIG_PARENT_KEY, on_config_changed, priv);
}

static AmlStatus unregister_config_handle() {
  return ipc_config_unregister_handle(CONFIG_PARENT_KEY, on_config_changed);
}

static AmlStatus load_configs(struct AmlIPCNNPriv *priv) {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    ipc_config_load_kv(&gs_kvs[i]);
    if (gs_kvs[i].handle) {
      gs_kvs[i].handle((void *)priv);
    }
  }
  return AML_STATUS_OK;
}
static void fork_exec_command(const char *cmd) {
  if (NULL == cmd)
    return;
  pid_t pid = fork();
  if (pid == 0) {
    execlp("/bin/sh", "sh", "-c", cmd, NULL);
  } else if (pid < 0) {
    AML_LOGE("fatal: fork process failed!");
  }
  return;
}

static void trigger_recognized(struct AmlIPCNNPriv *priv, int uid, char *name) {
  bool found = false;
  bool trigger = false;
  struct timeval tv;
  gettimeofday(&tv, 0);
  unsigned long now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  AML_LOGD("uid=%d name=%s\n", uid, name);
  struct recognized_info *info;
  LIST_FOREACH(info, &priv->recognized_action.faces, node) {
    if (info->uid == uid) {
      found = true;
      unsigned long diff = now - info->timestamp;
      if ((diff / 1000) >= priv->recognized_action.interval) {
        info->timestamp = now;
        trigger = true;
      }
    }
  }

  if (!found) {
    info = ipc_mem_new0(struct recognized_info, 1);
    info->uid = uid;
    info->timestamp = now;
    LIST_INSERT_HEAD(&priv->recognized_action.faces, info, node);
    trigger = true;
  }

  if (trigger && priv->recognized_action.script[0] != '\0') {
    int sz = snprintf(NULL, 0, "%s %d \"%s\"", priv->recognized_action.script,
                      uid, name) +
             1;
    char *cmd = (char *)malloc(sz);
    snprintf(cmd, sz, "%s %d \"%s\"", priv->recognized_action.script, uid,
             name);
    fork_exec_command(cmd);
    free(cmd);
  }
}

static void cleanup_recognized_list(struct AmlIPCNNPriv *priv) {
  struct recognized_info *info;
  while ((info = LIST_FIRST(&priv->recognized_action.faces))) {
    LIST_REMOVE(info, node);
  }
}

static bool open_model(struct model_info *m) {
  if (m->initialized)
    return true;

  if (m->model == DET_BUTT)
    return false;

  if (det_set_model(m->model) != DET_STATUS_OK) {
    return false;
  }

  if (det_get_param(m->model, &m->param) != DET_STATUS_OK) {
    return false;
  }

  if (det_get_model_size(m->model, &m->width, &m->height, &m->channel) !=
      DET_STATUS_OK) {
    det_release_model(m->model);
    return false;
  }

  m->rowbytes = m->width * m->channel;
  m->stride = (m->rowbytes + 31) & ~31;
  m->initialized = true;
  AML_LOGD("model set to %d", m->model);
  return true;
}

static bool close_model(struct model_info *m) {
  if (!m->initialized)
    return true;

  if (m->vframe) {
    aml_obj_release(AML_OBJECT(m->vframe));
    m->vframe = NULL;
  }

  if (m->model == DET_BUTT)
    return false;

  if (det_release_model(m->model) != DET_STATUS_OK) {
    return false;
  }

  m->initialized = false;
  return true;
}

static bool open_db(struct db_param *param) {
  if (param->handle != NULL)
    return true;

  if (param->file[0] != '\0') {
    AML_LOGD("set database to :%s\n", param->file);
    param->handle = recognition_db_init(param->file);
    recognition_db_set_threshold(param->threshold);
  }
  return param->handle != NULL;
}

static bool close_db(struct db_param *param) {
  if (param->handle == NULL)
    return true;
  recognition_db_deinit(param->handle);
  param->handle = NULL;
  return true;
}

struct idle_task_data {
  struct AmlIPCNNPriv *priv;
  union {
    struct _model {
      struct model_info *minfo;
      det_model_type new_model;
    } model;
    struct _db {
      char *file;
    } db;
    struct _img {
      char *file;
    } img;
  } u;
};

static bool idle_close_model(struct idle_task_data *data) {
  if (data == NULL || data->priv == NULL || data->priv->running == false ||
      data->u.model.minfo == NULL) {
    return IPC_TIMEOUT_TASK_STOP;
  }

  struct AmlIPCNNPriv *priv = data->priv;
  struct model_info *minfo = data->u.model.minfo;

  pthread_mutex_lock(&priv->lock);
  // close detect model for the next reinitialization
  close_model(minfo);
  minfo->model = data->u.model.new_model;
  if (data->u.model.new_model == DET_BUTT) {
    // notify the overlay to clear info
    send_frame(priv, NULL);
  }
  pthread_mutex_unlock(&priv->lock);

  free(data);
  return IPC_TIMEOUT_TASK_STOP;
}

static bool idle_close_db(struct idle_task_data *data) {
  if (data == NULL || data->priv == NULL || data->priv->running == false ||
      data->u.db.file == NULL) {
    return IPC_TIMEOUT_TASK_STOP;
  }

  struct AmlIPCNNPriv *priv = data->priv;

  pthread_mutex_lock(&priv->lock);
  close_db(&priv->db_param);
  FREE_STR(priv->db_param.file);
  priv->db_param.file = data->u.db.file;
  pthread_mutex_unlock(&priv->lock);

  free(data);
  return IPC_TIMEOUT_TASK_STOP;
}

static bool idle_request_capface(struct idle_task_data *data) {
  if (data == NULL || data->priv == NULL || data->priv->running == false) {
    return IPC_TIMEOUT_TASK_STOP;
  }

  struct AmlIPCNNPriv *priv = data->priv;

  pthread_mutex_lock(&priv->lock);
  priv->db_param.bstore_face = true;
  pthread_mutex_unlock(&priv->lock);

  free(data);
  return IPC_TIMEOUT_TASK_STOP;
}

static bool idle_request_capface_from_image(struct idle_task_data *data) {
  if (data == NULL || data->priv == NULL || data->priv->running == false ||
      data->u.img.file == NULL) {
    return IPC_TIMEOUT_TASK_STOP;
  }

  struct AmlIPCNNPriv *priv = data->priv;

  pthread_mutex_lock(&priv->lock);
  if (priv->custimg)
    free(priv->custimg);
  priv->custimg = data->u.img.file;
  pthread_mutex_unlock(&priv->lock);

  free(data);
  return IPC_TIMEOUT_TASK_STOP;
}

static struct AmlIPCVideoFrame *load_custom_image(char *custimg) {
  int width, height, stride;
  struct AmlIPCVideoFrame *img_frame = NULL;

  AML_LOGI("processing image: %s", custimg);

  // read the image size
  if (!aml_ipc_jpegencsw_decode_file(custimg, &width, &height, &stride, NULL)) {
    AML_LOGE("failed to read the input jpeg image");
    goto fail_exit;
  }
  AML_LOGI("image size: %dx%d", width, height);
  struct AmlIPCVideoFormat format = {AML_PIXFMT_RGB_888, width, height, 0};
  img_frame = AML_IPC_VIDEO_FRAME(aml_ipc_osd_frame_new_alloc(&format));
  if (NULL == img_frame) {
    AML_LOGE("failed to create input buffer");
    goto fail_exit;
  }
  // load jpeg
  struct AmlIPCVideoBuffer *dmabuf = img_frame->dmabuf;
  unsigned char *buffer =
      aml_ipc_dma_buffer_map(dmabuf, AML_MAP_READ | AML_MAP_WRITE);

  if (!aml_ipc_jpegencsw_decode_file(custimg, &width, &height, &stride,
                                     buffer)) {
    AML_LOGE("failed to generate rgb data from %s", custimg);
    aml_ipc_dma_buffer_unmap(dmabuf);
    goto fail_exit;
  }
  aml_ipc_dma_buffer_unmap(dmabuf);
  return img_frame;
fail_exit:
  if (img_frame)
    aml_obj_release(AML_OBJECT(img_frame));
  return NULL;
}

static bool detection_process(struct AmlIPCNNPriv *priv,
                              struct nn_result_buffer *resbuf) {
  bool ret = true;

  AML_LOGD("face detection process begin");

  if (!priv->face_det.initialized) {
    open_model(&priv->face_det);
    priv->face_det.param.param.det_param.detect_num =
        gs_config.detection.limits;
    det_set_param(priv->face_det.model, priv->face_det.param);
  }

  if (!priv->face_det.initialized) {
    AML_LOGE("face detection model not initialized");
    return false;
  }

  if (priv->face_det.vframe == NULL) {
    struct AmlIPCVideoFormat format = {AML_PIXFMT_RGB_888, priv->face_det.width,
                                       priv->face_det.height, 0};
    priv->face_det.vframe =
        AML_IPC_VIDEO_FRAME(aml_ipc_osd_frame_new_alloc(&format));
    if (priv->face_det.vframe == NULL) {
      AML_LOGE("failed to allocate the input frame to detection model");
      return false;
    }
  }

  {
    struct AmlRect srect = {0, 0, priv->input_frame->format.width,
                            priv->input_frame->format.height};
    struct AmlRect drect = {0, 0, priv->face_det.width, priv->face_det.height};
    AML_SCTIME_LOGD("detection: stretch image from %dx%d to %dx%d", srect.width,
                    srect.height, drect.width, drect.height);
    if (AML_STATUS_OK != aml_ipc_ge2d_stretchblt(priv->ge2d, priv->input_frame,
                                                 priv->face_det.vframe, &srect,
                                                 &drect)) {
      AML_LOGE("failed to stretch the input image");
      return false;
    }
  }

  struct AmlIPCVideoBuffer *dmabuf = priv->face_det.vframe->dmabuf;
  unsigned char *buffer =
      aml_ipc_dma_buffer_map(dmabuf, AML_MAP_READ | AML_MAP_WRITE);

  if (priv->face_det.rowbytes != priv->face_det.stride) {
    int rowbytes = priv->face_det.rowbytes;
    int stride = priv->face_det.stride;
    for (int i = 0; i < priv->face_det.height; i++) {
      memcpy(&buffer[rowbytes * i], &buffer[stride * i], rowbytes);
    }
  }

  input_image_t im;
  im.data = buffer;
  im.pixel_format = PIX_FMT_RGB888;
  im.width = priv->face_det.width;
  im.height = priv->face_det.height;
  im.channel = priv->face_det.channel;
  det_status_t rc = det_set_input(im, priv->face_det.model);

  if (rc != DET_STATUS_OK) {
    AML_LOGE("failed to set input to detection model");
    ret = false;
    goto bail;
  }

  AML_LOGD("waiting for detection result");
  DetectResult res;
  res.result.det_result.detect_num = 0;
  res.result.det_result.point = ipc_mem_new0(
      det_position_float_t, priv->face_det.param.param.det_param.detect_num);
  res.result.det_result.result_name = ipc_mem_new0(
      det_classify_result_t, priv->face_det.param.param.det_param.detect_num);
  {
    AML_SCTIME_LOGD("detection: waiting for result");
    det_get_result(&res, priv->face_det.model);
  }
  AML_LOGD("detection result got, facenum: %d",
           res.result.det_result.detect_num);

  resbuf->amount = res.result.det_result.detect_num;
  resbuf->results = NULL;
  if (resbuf->amount > 0) {
    resbuf->results = calloc(resbuf->amount, sizeof(struct nn_result));
    for (int i = 0; i < resbuf->amount; i++) {
      resbuf->results[i].pos.left =
          res.result.det_result.point[i].point.rectPoint.left;
      resbuf->results[i].pos.top =
          res.result.det_result.point[i].point.rectPoint.top;
      resbuf->results[i].pos.right =
          res.result.det_result.point[i].point.rectPoint.right;
      resbuf->results[i].pos.bottom =
          res.result.det_result.point[i].point.rectPoint.bottom;
      for (int j = 0; j < 5; j++) {
        resbuf->results[i].fpos[j].x =
            res.result.det_result.point[i].tpts.floatX[j];
        resbuf->results[i].fpos[j].y =
            res.result.det_result.point[i].tpts.floatY[j];
      }
      resbuf->results[i].info[0] = '\0';
    }
  }

  free(res.result.det_result.point);
  free(res.result.det_result.result_name);

  AML_LOGD("face detection process finished");
bail:
  aml_ipc_dma_buffer_unmap(dmabuf);
  return ret;
}

static bool recognition_process(struct AmlIPCNNPriv *priv,
                                struct nn_result_buffer *resbuf) {
  bool ret = true;

  if (priv->face_recog.model == DET_BUTT)
    return ret;

  if (resbuf == NULL || resbuf->amount == 0) {
    return ret;
  }

  AML_LOGD("face recognition process begin");

  if (!priv->face_recog.initialized) {
    open_model(&priv->face_recog);
  }

  if (!priv->face_recog.initialized) {
    AML_LOGE("face recognition model not initialized");
    return false;
  }

  if (priv->db_param.handle == NULL) {
    recognition_db_set_scale(priv->face_recog.param.param.reg_param.scale);
    open_db(&priv->db_param);
  }

  if (priv->db_param.handle == NULL) {
    AML_LOGD("database not set");
    return false;
  }

  if (priv->face_recog.vframe == NULL) {
    struct AmlIPCVideoFormat format = {
        AML_PIXFMT_RGB_888, priv->face_recog.width, priv->face_recog.height, 0};
    priv->face_recog.vframe =
        AML_IPC_VIDEO_FRAME(aml_ipc_osd_frame_new_alloc(&format));
    if (priv->face_recog.vframe == NULL) {
      AML_LOGE("failed to allocate the input frame to recognition model");
      return false;
    }
  }

  struct AmlIPCVideoBuffer *dmabuf = priv->face_recog.vframe->dmabuf;
  unsigned char *buffer =
      aml_ipc_dma_buffer_map(dmabuf, AML_MAP_READ | AML_MAP_WRITE);
  struct AmlRect srect = {0, 0, priv->input_frame->format.width,
                          priv->input_frame->format.height};
  struct AmlRect drect = {0, 0, priv->face_recog.width,
                          priv->face_recog.height};

  for (int i = 0; i < resbuf->amount; i++) {
    if (priv->flushing) {
      AML_LOGD("break out @%d", i);
      break;
    }

    struct {
      int x0;
      int y0;
      int x1;
      int y1;
      int w;
      int h;
    } detect_rect, recog_rect;

    detect_rect.x0 =
        (int)(resbuf->results[i].pos.left * priv->input_frame->format.width);
    detect_rect.y0 =
        (int)(resbuf->results[i].pos.top * priv->input_frame->format.height);
    detect_rect.x1 =
        (int)(resbuf->results[i].pos.right * priv->input_frame->format.width);
    detect_rect.y1 =
        (int)(resbuf->results[i].pos.bottom * priv->input_frame->format.height);
    detect_rect.w = detect_rect.x1 - detect_rect.x0;
    detect_rect.h = detect_rect.y1 - detect_rect.y0;

    float recog_rel_x, recog_rel_y;
    // adjust the detection rect into to recog square
    if (detect_rect.w > detect_rect.h) {
      recog_rect.w = priv->face_recog.width;
      recog_rect.h = (detect_rect.h * priv->face_recog.width) / detect_rect.w;
      recog_rect.x0 = 0;
      recog_rect.y0 = (priv->face_recog.height - recog_rect.h) / 2;
    } else if (detect_rect.w < detect_rect.h) {
      recog_rect.h = priv->face_recog.height;
      recog_rect.w = (detect_rect.w * priv->face_recog.height) / detect_rect.h;
      recog_rect.x0 = (priv->face_recog.width - recog_rect.w) / 2;
      recog_rect.y0 = 0;
    } else {
      recog_rect.h = priv->face_recog.height;
      recog_rect.w = priv->face_recog.width;
      recog_rect.x0 = 0;
      recog_rect.y0 = 0;
    }

    recog_rel_x = (float)recog_rect.x0 / (float)priv->face_recog.width;
    recog_rel_y = (float)recog_rect.y0 / (float)priv->face_recog.height;

    drect.left = 0;
    drect.top = 0;
    drect.width = priv->face_recog.width;
    drect.height = priv->face_recog.height;
    aml_ipc_ge2d_fill_rect(priv->ge2d, priv->face_recog.vframe, &drect,
                           0xffffffff);

    srect.left = detect_rect.x0;
    srect.top = detect_rect.y0;
    srect.width = detect_rect.w;
    srect.height = detect_rect.h;

    drect.left = recog_rect.x0;
    drect.top = recog_rect.y0;
    drect.width = recog_rect.w;
    drect.height = recog_rect.h;

    {
      AML_SCTIME_LOGD("recognition: crop image %dx%d@%dx%d", srect.width,
                      srect.height, srect.left, srect.top);
      if (AML_STATUS_OK !=
          aml_ipc_ge2d_stretchblt(priv->ge2d, priv->input_frame,
                                  priv->face_recog.vframe, &srect, &drect)) {
        AML_LOGE("failed to crop the input image");
        ret = false;
        goto recognition_process_exit;
      }
    }

    if (priv->face_recog.rowbytes != priv->face_recog.stride) {
      int rowbytes = priv->face_recog.rowbytes;
      int stride = priv->face_recog.stride;
      for (int i = 0; i < priv->face_recog.height; i++) {
        memmove(&buffer[rowbytes * i], &buffer[stride * i], rowbytes);
      }
    }

    // due to the input image would be modified inside nn recoginition module,
    // orignal image data should be saved for database recording purpose
    int szimg = priv->face_recog.rowbytes * priv->face_recog.height;
    unsigned char *img_data = (unsigned char *)malloc(szimg);
    memcpy(img_data, buffer, szimg);

    input_image_t im;
    im.data = img_data;
    im.pixel_format = PIX_FMT_RGB888;
    im.width = priv->face_recog.width;
    im.height = priv->face_recog.height;
    im.channel = priv->face_recog.channel;

    // set input face points
    for (int fcnt = 0; fcnt < 5; fcnt++) {
      if (resbuf->results[i].fpos[fcnt].x >= resbuf->results[i].pos.left &&
          resbuf->results[i].fpos[fcnt].y >= resbuf->results[i].pos.top) {
        im.inPoint[fcnt].x =
            (resbuf->results[i].fpos[fcnt].x - resbuf->results[i].pos.left) /
                (resbuf->results[i].pos.right - resbuf->results[i].pos.left) +
            recog_rel_x;
        im.inPoint[fcnt].y =
            (resbuf->results[i].fpos[fcnt].y - resbuf->results[i].pos.top) /
                (resbuf->results[i].pos.bottom - resbuf->results[i].pos.top) +
            recog_rel_y;
      } else {
        im.inPoint[fcnt].x = 0.0;
        im.inPoint[fcnt].y = 0.0;
      }
    }

    det_status_t rc = det_set_input(im, priv->face_recog.model);

    free(img_data);

    if (rc != DET_STATUS_OK) {
      AML_LOGE("failed to set input to recognition model");
      ret = false;
      goto recognition_process_exit;
    }

    DetectResult fn_res;
    AML_LOGD("waiting for recognition result");
    {
      AML_SCTIME_LOGD("recognition: waiting for result");
      rc = det_get_result(&fn_res, priv->face_recog.model);
    }
    AML_LOGD("recognition result got");
    if (rc != DET_STATUS_OK) {
      AML_LOGE("failed to get recognition result");
      continue;
    }

    int szfaceid = priv->face_recog.param.param.reg_param.length;
    AML_LOGD("recognition vector number = %d", szfaceid);

    {
      AML_SCTIME_LOGD("recognition: search db");
      int uid = recognition_db_search_result(
          priv->db_param.handle, fn_res.result.reg_result.uint8, szfaceid,
          priv->db_param.bstore_face ? buffer : NULL, priv->face_recog.width,
          priv->face_recog.height, priv->db_param.format,
          resbuf->results[i].info, FACE_INFO_BUFSIZE);

      if (uid < 0) {
        resbuf->results[i].info[0] = '\0';
      } else {
        trigger_recognized(priv, uid, resbuf->results[i].info);
      }
    }
  }

recognition_process_exit:
  aml_ipc_dma_buffer_unmap(dmabuf);
  priv->db_param.bstore_face = false;
  AML_LOGD("face recognition process finished");
  return ret;
}

static void *nn_process(void *data) {
  struct nn_result_buffer resbuf;
  struct AmlIPCNNPriv *priv = (struct AmlIPCNNPriv *)data;

  prctl(PR_SET_NAME, "nn-process", NULL, NULL, NULL);

  while (priv->running) {
    bool is_normal_process = true;
    pthread_mutex_lock(&priv->lock);
    while (!priv->ready) {
      pthread_cond_wait(&priv->cond, &priv->lock);
    }

    if (!priv->running) {
      goto loop_continue;
    }

    if (priv->face_det.model == DET_BUTT) {
      // face detection not enabled,
      // ignore the request and exit
      AML_LOGD("face detection model disabled");
      goto loop_continue;
    }

    if (priv->custimg) {
      is_normal_process = false;
      struct AmlIPCVideoFrame *img_frame = load_custom_image(priv->custimg);
      if (img_frame) {
        aml_obj_release(AML_OBJECT(priv->input_frame));
        priv->input_frame = img_frame;
        priv->db_param.bstore_face = true;
      }
      free(priv->custimg);
      priv->custimg = NULL;
    }

    {
      AML_SCTIME_LOGD("detection process");
      detection_process(priv, &resbuf);
    }

    {
      AML_SCTIME_LOGD("recognition process");
      recognition_process(priv, &resbuf);
    }

    if (is_normal_process && priv->running) {
      send_frame(priv, &resbuf);
    }

  loop_continue:
    if (priv->input_frame) {
      aml_obj_release(AML_OBJECT(priv->input_frame));
      priv->input_frame = NULL;
    }
    if (resbuf.amount > 0) {
      free(resbuf.results);
      resbuf.amount = 0;
      resbuf.results = NULL;
    }

    // continue process buffer
    if (priv->running) {
      priv->bypass_frame_handle = false;
    }
    priv->ready = false;
    pthread_mutex_unlock(&priv->lock);
  }

  // exiting
  close_db(&priv->db_param);
  close_model(&priv->face_det);
  close_model(&priv->face_recog);

  return NULL;
}

static AmlStatus nn_start(struct AmlIPCComponent *comp) {
  struct AmlIPCNN *nn = (struct AmlIPCNN *)comp;
  struct AmlIPCNNPriv *priv = nn->priv;
  det_set_log_config(DET_DEBUG_LEVEL_ERROR, DET_LOG_TERMINAL);

  priv->input_frame = NULL;

  priv->ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_TO);
  if (priv->ge2d == NULL) {
    AML_LOGE("failed to initialize ge2d");
    return AML_STATUS_FAIL;
  }

  priv->running = true;
  priv->flushing = false;
  pthread_create(&priv->tid, NULL, nn_process, priv);
  return AML_STATUS_OK;
}

static AmlStatus nn_stop(struct AmlIPCComponent *comp) {
  struct AmlIPCNN *nn = (struct AmlIPCNN *)comp;
  struct AmlIPCNNPriv *priv = nn->priv;
  if (!priv->running) {
    return AML_STATUS_OK;
  }
  aml_ipc_comp_flush(comp, true);
  priv->flushing = true;
  AML_LOGD("closing, waiting for lock");
  unregister_config_handle();
  priv->running = false;
  pthread_mutex_lock(&priv->lock);
  priv->ready = true;
  pthread_cond_signal(&priv->cond);
  pthread_mutex_unlock(&priv->lock);
  pthread_join(priv->tid, NULL);

  priv->tid = 0;

  if (priv->input_frame) {
    aml_obj_release(AML_OBJECT(priv->input_frame));
    priv->input_frame = NULL;
  }

  if (priv->custimg) {
    free(priv->custimg);
    priv->custimg = NULL;
  }
  if (priv->ge2d) {
    aml_obj_release(AML_OBJECT(priv->ge2d));
    priv->ge2d = NULL;
  }

  cleanup_recognized_list(priv);

  FREE_STR(priv->db_param.file);
  FREE_STR(priv->db_param.format);
  FREE_STR(priv->recognized_action.script);

  AML_LOGD("closed");

  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_trigger_face_capture(struct AmlIPCNN *nn) {
  if (!nn->priv->running)
    return AML_STATUS_OK;
  struct idle_task_data *data = ipc_mem_new0(struct idle_task_data, 1);
  data->priv = nn->priv;
  ipc_timeout_task_add(100, (IPCTimeoutCallback)idle_request_capface, data);
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_trigger_face_capture_from_image(struct AmlIPCNN *nn,
                                                     const char *img) {
  if (!nn->priv->running || img == NULL || strlen(img) == 0)
    return AML_STATUS_OK;
  struct idle_task_data *data = ipc_mem_new0(struct idle_task_data, 1);
  data->priv = nn->priv;
  data->u.img.file = strdup(img);
  ipc_timeout_task_add(100, (IPCTimeoutCallback)idle_request_capface_from_image,
                       data);
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_enabled(struct AmlIPCNN *nn, bool enabled) {
  struct AmlIPCNNPriv *priv = nn->priv;
  priv->enabled = enabled;
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_detection_model(struct AmlIPCNN *nn,
                                         enum AmlIPCNNDetectionModel model) {
  struct AmlIPCNNPriv *priv = nn->priv;
  det_model_type m = det_model_to_internal(model);
  if (m != priv->face_det.model) {
    // close detect model for the next reinitialization
    if (priv->face_det.initialized) {
      struct idle_task_data *data = ipc_mem_new0(struct idle_task_data, 1);
      data->priv = priv;
      data->u.model.minfo = &priv->face_det;
      data->u.model.new_model = m;
      ipc_timeout_task_add(10, (IPCTimeoutCallback)idle_close_model, data);
    } else {
      priv->face_det.model = m;
    }
  }
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_detection_limits(struct AmlIPCNN *nn, int max) {
  struct AmlIPCNNPriv *priv = nn->priv;

  if (max < DETECT_NUM) {
    max = DETECT_NUM;
  }

  if (max > MAX_DETECT_NUM) {
    max = MAX_DETECT_NUM;
  }
  // param would be reinitialized while model opened
  // save the configuration
  gs_config.detection.limits = max;
  priv->face_det.param.param.det_param.detect_num = max;
  if (priv->face_det.initialized) {
    det_set_param(priv->face_det.model, priv->face_det.param);
  }
  return AML_STATUS_OK;
}

AmlStatus
aml_ipc_nn_set_recognition_model(struct AmlIPCNN *nn,
                                 enum AmlIPCNNRecognitionModel model) {
  struct AmlIPCNNPriv *priv = nn->priv;
  det_model_type m = recog_model_to_internal(model);
  if (m != priv->face_recog.model) {
    // close detect model for the next reinitialization
    if (priv->face_recog.initialized) {
      struct idle_task_data *data = ipc_mem_new0(struct idle_task_data, 1);
      data->priv = priv;
      data->u.model.minfo = &priv->face_recog;
      data->u.model.new_model = m;
      ipc_timeout_task_add(10, (IPCTimeoutCallback)idle_close_model, data);
    } else {
      priv->face_recog.model = m;
    }
  }
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_db_path(struct AmlIPCNN *nn, const char *path) {
  struct AmlIPCNNPriv *priv = nn->priv;
  if (NULL == path)
    return AML_STATUS_FAIL;
  if (priv->db_param.file == NULL || strcmp(path, priv->db_param.file)) {
    if (priv->db_param.handle != NULL) {
      struct idle_task_data *data = ipc_mem_new0(struct idle_task_data, 1);
      data->priv = priv;
      data->u.db.file = strdup(path);
      ipc_timeout_task_add(10, (IPCTimeoutCallback)idle_close_db, data);
    } else {
      FREE_STR(priv->db_param.file);
      priv->db_param.file = strdup(path);
    }
  }
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_result_format(struct AmlIPCNN *nn,
                                       const char *format) {
  struct AmlIPCNNPriv *priv = nn->priv;
  if (format) {
    FREE_STR(priv->db_param.format);
    priv->db_param.format = strdup(format);
  }
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_recognition_threshold(struct AmlIPCNN *nn, float th) {
  struct AmlIPCNNPriv *priv = nn->priv;
  priv->db_param.threshold = th;
  recognition_db_set_threshold(priv->db_param.threshold);
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_recognition_action_hook(struct AmlIPCNN *nn,
                                                 const char *script_file) {
  struct AmlIPCNNPriv *priv = nn->priv;
  if (script_file) {
    FREE_STR(priv->recognized_action.script);
    priv->recognized_action.script = strdup(script_file);
  }
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_nn_set_recognition_trigger_interval(struct AmlIPCNN *nn,
                                                      int seconds) {
  struct AmlIPCNNPriv *priv = nn->priv;
  priv->recognized_action.interval = seconds;
  return AML_STATUS_OK;
}

struct AmlIPCNN *aml_ipc_nn_new() {
  struct AmlIPCNN *nn = AML_OBJ_NEW(AmlIPCNN);
  struct AmlIPCNNPriv *priv = nn->priv;

  pthread_cond_init(&priv->cond, NULL);
  pthread_mutex_init(&priv->lock, NULL);
  priv->ready = false;

  LIST_INIT(&priv->recognized_action.faces);

  load_configs(priv);
  register_config_handle(priv);

  return nn;
}

static AmlStatus nn_frame_process(struct AmlIPCComponent *comp, int ch,
                                  struct AmlIPCFrame *frame) {
  struct AmlIPCNN *nn = (struct AmlIPCNN *)comp;
  struct AmlIPCNNPriv *priv = nn->priv;
  AmlStatus ret = AML_STATUS_FAIL;

  if (!priv->enabled) {
    // notify the overlay to clear info
    send_frame(priv, NULL);
    ret = AML_STATUS_OK;
    goto process_end;
  }

  if (!priv->running) {
    AML_LOGE("nn module not started");
    ret = AML_STATUS_FAIL;
    goto process_end;
  }

  if (priv->bypass_frame_handle) {
    // previous process not finished
    ret = AML_STATUS_OK;
    goto process_end;
  }

  if (pthread_mutex_trylock(&priv->lock) == 0) {
    // skip the following frame process
    priv->bypass_frame_handle = true;
    aml_obj_addref(AML_OBJECT(frame));
    priv->input_frame = AML_IPC_VIDEO_FRAME(frame);
    // trigger nn process
    priv->ready = true;
    pthread_cond_signal(&priv->cond);
    pthread_mutex_unlock(&priv->lock);
  }

  ret = AML_STATUS_OK;

process_end:
  aml_obj_release(AML_OBJECT(frame));
  return ret;
}

static void nn_init(struct AmlIPCNN *nn) {
  AML_ALLOC_PRIV(nn->priv);
  nn->priv->parent = nn;
  aml_ipc_add_channel(AML_IPC_COMPONENT(nn), AML_NN_INPUT, AML_CHANNEL_INPUT,
                      "input");
  aml_ipc_add_channel(AML_IPC_COMPONENT(nn), AML_NN_OUTPUT, AML_CHANNEL_OUTPUT,
                      "output");
}

static void nn_free(struct AmlIPCNN *nn) {
  nn_stop(AML_IPC_COMPONENT(nn));
  free(nn->priv);
  nn->priv = NULL;
}

static void nn_class_init(struct AmlIPCComponentClass *cls) {
  cls->start = nn_start;
  cls->handle_frame = nn_frame_process;
  cls->stop = nn_stop;
}
AML_OBJ_DEFINE_TYPEID(AmlIPCNN, AmlIPCComponent, AmlIPCComponentClass,
                      nn_class_init, nn_init, nn_free);

static AmlStatus send_frame(struct AmlIPCNNPriv *priv,
                            struct nn_result_buffer *resbuf) {
  struct AmlIPCNNResult *result = AML_OBJ_NEW(AmlIPCNNResult);
  struct AmlIPCFrame *frm = AML_IPC_FRAME(result);
  if (resbuf == NULL || resbuf->amount <= 0) {
    frm->size = 0;
    frm->data = NULL;
    frm->pts_us = 0;
    return aml_ipc_send_frame(AML_IPC_COMPONENT(priv->parent), AML_NN_OUTPUT,
                              frm);
  }

  int st_size = sizeof(struct AmlIPCNNResultBuffer);
  int res_size = resbuf->amount * sizeof(struct nn_result);
  frm->size = st_size + res_size;
  frm->data = malloc(frm->size);
  memcpy(frm->data, resbuf, st_size);
  memcpy(frm->data + st_size, resbuf->results, res_size);
  return aml_ipc_send_frame(AML_IPC_COMPONENT(priv->parent), AML_NN_OUTPUT,
                            frm);
}

static void nn_result_free(struct AmlIPCNNResult *result) {
  AML_LOGD("result released");
  struct AmlIPCFrame *frm = AML_IPC_FRAME(result);
  if (frm->data)
    free(frm->data);
  frm->data = NULL;
}

AML_OBJ_DEFINE_TYPEID(AmlIPCNNResult, AmlIPCFrame, AmlObjectClass, NULL, NULL,
                      nn_result_free);
