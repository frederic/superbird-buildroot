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

#ifndef __IPC_JPEGENCSW_H__
#define __IPC_JPEGENCSW_H__

#include <aml_ipc_component.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AmlIPCJpegEncSWChannel { AML_JPEGENCSW_INPUT, AML_JPEGENCSW_OUTPUT };

struct AmlIPCJpegEncSWPriv;
struct AmlIPCJpegEncSW {
  AML_OBJ_EXTENDS(AmlIPCJpegEncSW, AmlIPCComponent, AmlIPCComponentClass);
  struct AmlIPCJpegEncSWPriv *priv;
  int pass_through;
  int quality;
};

AML_OBJ_DECLARE_TYPEID(AmlIPCJpegEncSW, AmlIPCComponent, AmlIPCComponentClass);
#define AML_IPC_JpegEncSW(obj) ((struct AmlIPCJpegEncSW *)(obj))

static inline struct AmlIPCJpegEncSW *aml_ipc_jpegencsw_new() {
  struct AmlIPCJpegEncSW *jpegenc = AML_OBJ_NEW(AmlIPCJpegEncSW);
  return jpegenc;
}

AmlStatus aml_ipc_jpegencsw_set_passthrough(struct AmlIPCJpegEncSW *jpgenc,
                                            bool passthrough);
AmlStatus aml_ipc_jpegencsw_set_quality(struct AmlIPCJpegEncSW *jpgenc,
                                        int quality);
AmlStatus aml_ipc_jpegencsw_encode_frame(struct AmlIPCJpegEncSW *enc,
                                         struct AmlIPCVideoFrame *frame,
                                         struct AmlIPCFrame **out);

size_t aml_ipc_jpegencsw_encode(unsigned char *imgbuf, int w, int h,
                                int quality, unsigned char **outbuf);
bool aml_ipc_jpegencsw_decode_file(char *filename, int *pwidth, int *pheight,
                                   int *pstride, unsigned char *outbuf);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IPC_JPEGENCSW_H__ */
