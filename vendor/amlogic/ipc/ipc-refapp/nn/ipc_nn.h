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

#ifndef __IPC_NN_H__
#define __IPC_NN_H__

#include <aml_ipc_component.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AmlIPCNNChannel { AML_NN_INPUT, AML_NN_OUTPUT };

enum AmlIPCNNDetectionModel {
  AML_NN_DET_YOLOFACE,
  AML_NN_DET_MTCNN,
  AML_NN_DET_FACE,
  AML_NN_DET_DISABLE,
};

enum AmlIPCNNRecognitionModel {
  AML_NN_RECOG_FACENET,
  AML_NN_RECOG_FACE,
  AML_NN_RECOG_DISABLE,
};

struct AmlIPCNNPriv;
struct AmlIPCNN {
  AML_OBJ_EXTENDS(AmlIPCNN, AmlIPCComponent, AmlIPCComponentClass);
  struct AmlIPCNNPriv *priv;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCNN, AmlIPCComponent, AmlIPCComponentClass);
#define AML_IPC_NN(obj) ((struct AmlIPCNN *)(obj))

struct AmlIPCNN *aml_ipc_nn_new();

AmlStatus aml_ipc_nn_set_enabled(struct AmlIPCNN *nn, bool enabled);
// properties
AmlStatus aml_ipc_nn_set_detection_model(struct AmlIPCNN *nn,
                                         enum AmlIPCNNDetectionModel model);
AmlStatus aml_ipc_nn_set_detection_limits(struct AmlIPCNN *nn, int max);
AmlStatus aml_ipc_nn_set_recognition_model(struct AmlIPCNN *nn,
                                           enum AmlIPCNNRecognitionModel model);
AmlStatus aml_ipc_nn_set_db_path(struct AmlIPCNN *nn, const char *path);
AmlStatus aml_ipc_nn_set_result_format(struct AmlIPCNN *nn, const char *format);
AmlStatus aml_ipc_nn_set_recognition_threshold(struct AmlIPCNN *nn, float th);
AmlStatus aml_ipc_nn_set_recognition_action_hook(struct AmlIPCNN *nn,
                                                 const char *script_file);
AmlStatus aml_ipc_nn_set_recognition_trigger_interval(struct AmlIPCNN *nn,
                                                      int seconds);

// actions
AmlStatus aml_ipc_nn_trigger_face_capture(struct AmlIPCNN *nn);
AmlStatus aml_ipc_nn_trigger_face_capture_from_image(struct AmlIPCNN *nn,
                                                     const char *img);

struct relative_pos {
  float left;
  float top;
  float right;
  float bottom;
};

struct facepoint_pos {
  float x;
  float y;
};

#define FACE_INFO_BUFSIZE 1024
struct nn_result {
  struct relative_pos pos;
  struct facepoint_pos fpos[5];
  char info[FACE_INFO_BUFSIZE];
};

struct AmlIPCNNResultBuffer {
  int amount; // amount of result
  struct nn_result results[0];
};

// AmlIPCFrame for nn result
struct AmlIPCNNResult {
  AML_OBJ_EXTENDS(AmlIPCNNResult, AmlIPCFrame, AmlObjectClass);
};
AML_OBJ_DECLARE_TYPEID(AmlIPCNNResult, AmlIPCFrame, AmlObjectClass);
#define AML_IPC_NN_RESULT(obj) ((struct AmlIPCNNResult *)(obj))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IPC_NN_H__ */
