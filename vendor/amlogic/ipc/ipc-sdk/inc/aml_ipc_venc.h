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

#ifndef AML_IPC_VENC_H

#define AML_IPC_VENC_H

#include "aml_ipc_component.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AmlRectFloat {
    float left;
    float top;
    float width;
    float height;
};
enum AmlIPCVencChannel { AML_VENC_INPUT, AML_VENC_OUTPUT };

#define AML_VENC_MAX_QUALITY_LEVEL 6

// must match AMVGOPModeOPT in AML_MultiEncoder.h
enum AmlIPCVencGopOpt {
    AML_VENC_GOP_OPT_NONE,
    AML_VENC_GOP_OPT_ALL_I,
    AML_VENC_GOP_OPT_IP,
    AML_VENC_GOP_OPT_IBBBP,
    AML_VENC_GOP_OPT_IP_SVC1,
    AML_VENC_GOP_OPT_IP_SVC2,
    AML_VENC_GOP_OPT_IP_SVC3,
    AML_VENC_GOP_OPT_IP_SVC4
};

struct AmlIPCVencPriv;
struct AmlIPCVenc {
    AML_OBJ_EXTENDS(AmlIPCVenc, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCVencPriv *priv;
    enum AmlIPCVideoCodec codec;
    int width;
    int height;
    enum AmlPixelFormat pixfmt;
    int gop;
    int framerate;
    int bitrate;
    int min_buffers;
    int max_buffers;
    int encoder_bufsize;
    int quality_level;
    int gop_opt;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCVenc, AmlIPCComponent, AmlIPCComponentClass);

struct AmlIPCVenc *aml_ipc_venc_new(enum AmlIPCVideoCodec codec);

AmlStatus aml_ipc_venc_set_input_fmt(struct AmlIPCVenc *enc, struct AmlIPCVideoFormat *fmt);
AmlStatus aml_ipc_venc_set_bitrate(struct AmlIPCVenc *enc, int bitrate);
AmlStatus aml_ipc_venc_set_gop_size(struct AmlIPCVenc *enc, int gopsize);
AmlStatus aml_ipc_venc_force_insert_idr(struct AmlIPCVenc *enc);
AmlStatus aml_ipc_venc_set_roi(struct AmlIPCVenc *enc, int quality, struct AmlRectFloat *rect,
                               int *index);
AmlStatus aml_ipc_venc_clear_roi(struct AmlIPCVenc *enc, int index);
AmlStatus aml_ipc_venc_set_quality_level(struct AmlIPCVenc *enc, int quality_level);

AmlStatus aml_ipc_venc_get_status(struct AmlIPCVenc *enc, int *pics, int *frames);

/**
 * @brief set venc codec
 *
 * @param venc
 * @param codec
 *
 * @return
 */
AmlStatus aml_ipc_venc_set_codec(struct AmlIPCVenc *venc, enum AmlIPCVideoCodec codec);

/**
 * @brief set venc buffer size in KB
 *
 * @param venc
 * @param size
 *
 * @return
 */
AmlStatus aml_ipc_venc_set_bufsize(struct AmlIPCVenc *venc, int size);

/**
 * @brief  set venc gop options
 *
 * @param venc
 * @param opt
 *
 * @return
 */
AmlStatus aml_ipc_venc_set_gop_option(struct AmlIPCVenc *venc, enum AmlIPCVencGopOpt opt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_VENC_H */
