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

#include "aml_ipc_venc.h"

#ifndef AML_IPC_JPEGENC_H

#define AML_IPC_JPEGENC_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlJpegEncChannel { AML_JPEGENC_INPUT, AML_JPEGENC_OUTPUT };
struct AmlIPCJpegEncPriv;
struct AmlIPCJpegEnc {
    AML_OBJ_EXTENDS(AmlIPCJpegEnc, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCJpegEncPriv *priv;
    int quality;
    int qtbl_id;
    int pixfmt;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCJpegEnc, AmlIPCComponent, AmlIPCComponentClass);

struct AmlIPCJpegEnc *aml_ipc_jpegenc_new();

AmlStatus aml_ipc_jpegenc_set_quality(struct AmlIPCJpegEnc *enc, int quality);

/**
 * @brief helper function to encode one frame
 *
 * @param enc
 * @param frame, caller manage the refcount, this function will not addref/release
 * @param out, output the jpeg data, caller shall release it when done
 *
 * @return  AML_STATUS_OK: encode success and the jpeg data is output from *out
 *          AML_STATUS_FAIL: encode fail
 */
AmlStatus aml_ipc_jpegenc_encode_frame(struct AmlIPCJpegEnc *enc, struct AmlIPCVideoFrame *frame,
                                       struct AmlIPCFrame **out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_JPEGENC_H */
