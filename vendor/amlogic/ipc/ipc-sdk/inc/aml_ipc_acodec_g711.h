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

#ifndef AML_IPC_ACODEC_G711_H

#define AML_IPC_ACODEC_G711_H

#ifdef __cplusplus
extern "C" {
#endif

struct AmlIPCACodecG711 {
    AML_OBJ_EXTENDS(AmlIPCACodecG711, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCAudioFormat format;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCACodecG711, AmlIPCComponent, AmlIPCComponentClass);

/**
 * @brief create g711 encoder/decoder
 *
 * @param codec
 *  AML_ACODEC_PCM_S16LE decode alaw/ulaw to pcm s16le
 *  AML_ACODEC_PCM_ULAW encode pcm s16le to ulaw
 *  AML_ACODEC_PCM_ALAW encode pcm s16le to alaw
 *
 * @return
 */
struct AmlIPCACodecG711 *aml_ipc_g711_new(enum AmlIPCAudioCodec codec);

/**
 * @brief set g711 codec
 *
 * @param g711
 * @param codec
 *
 * @return
 **/
AmlStatus aml_ipc_g711_set_codec(struct AmlIPCACodecG711 *g711, enum AmlIPCAudioCodec codec);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_ACODEC_G711_H */
