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
#ifndef AML_IPC_ACODEC_G726_H

#define AML_IPC_ACODEC_G726_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlG726BitRate {
    AML_G726_16K = 16000,
    AML_G726_24K = 24000,
    AML_G726_32K = 32000,
    AML_G726_40K = 40000
};
enum AmlG726Endian { AML_G726_BE, AML_G726_LE };

// adpcm samples A,B,C,D,E,F,G,H
// g726, aabbccdd eeffgghh
// g726le, ddccbbaa hhggffee
struct AmlIPCACodecG726Priv;
struct AmlIPCACodecG726 {
    AML_OBJ_EXTENDS(AmlIPCACodecG726, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCAudioFormat format;
    struct AmlIPCACodecG726Priv *priv;
    int bit_rate; // enum AmlG726BitRate
    int endian;   // enum AmlG726Endian
};
AML_OBJ_DECLARE_TYPEID(AmlIPCACodecG726, AmlIPCComponent, AmlIPCComponentClass);

/**
 * @brief create g726 encoder/decoder
 *
 * @param codec
 *  AML_ACODEC_PCM_S16LE decode g726 bitstream to pcm s16le
 *  AML_ACODEC_PCM_ULAW decode g726 bitstream to ulaw
 *  AML_ACODEC_PCM_ALAW decode g726 bitstream to alaw
 *  AML_ACODEC_ADPCM_G726 encode pcm s16le/alaw/ulaw to g726
 *
 * @param b, specify G.726 bitrate
 * @param e, specify G.726 endian
 *
 * @return
 */
struct AmlIPCACodecG726 *aml_ipc_g726_new(enum AmlIPCAudioCodec codec, enum AmlG726BitRate b,
                                          enum AmlG726Endian e);

/**
 * @brief set g726 bitrate
 *
 * @param g726
 * @param bps
 *
 * @return
 */
AmlStatus aml_ipc_g726_set_bitrate(struct AmlIPCACodecG726 *g726, enum AmlG726BitRate bps);

/**
 * @brief set g726 codec
 *
 * @param g726
 * @param codec
 *
 * @return
 */
AmlStatus aml_ipc_g726_set_codec(struct AmlIPCACodecG726 *g726, enum AmlIPCAudioCodec codec);

/**
 * @brief set g726 code endianess
 *
 * @param g726
 * @param endian
 *
 * @return
 */
AmlStatus aml_ipc_g726_set_endian(struct AmlIPCACodecG726 *g726, enum AmlG726Endian endian);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_ACODEC_G726_H */
