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
#ifndef AML_IPC_ACODEC_FDK_H

#define AML_IPC_ACODEC_FDK_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlAACAOT {
    AML_AAC_LC = 2,  // MPEG-4 AAC Low Complexity.
    AML_AAC_SBR = 5, // MPEG-4 AAC Low Complexity with Spectral Band Replication (HE-AAC).
    AML_AAC_PS = 29, // MPEG-4 AAC Low Complexity with Spectral Band Replication and Parametric
                     // Stereo (HE-AAC v2).
    AML_AAC_LD = 23, //  MPEG-4 AAC Low-Delay.
    AML_AAC_ELD = 39 // MPEG-4 AAC Enhanced Low-Delay.
};

enum AmlAACBitrateMode {
    AML_AAC_BITRATE_CBR = 0,
    AML_AAC_BITRATE_VERY_LOW,
    AML_AAC_BITRATE_LOW,
    AML_AAC_BITRATE_MEDIUM,
    AML_AAC_BITRATE_HIGH,
    AML_AAC_BITRATE_VERY_HIGH
};

struct AmlIPCAudioAACFrame {
    AML_OBJ_EXTENDS(AmlIPCAudioAACFrame, AmlIPCAudioFrame, AmlObjectClass);
    int aac_aot;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCAudioAACFrame, AmlIPCAudioFrame, AmlObjectClass);

struct AmlIPCACodecFDKPriv;
struct AmlIPCACodecFDK {
    AML_OBJ_EXTENDS(AmlIPCACodecFDK, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCACodecFDKPriv *priv;
    struct AmlIPCAudioFormat format;
    int aac_aot; //  enum AmlAACAOT
    int aac_bitrate;
    int aac_bitrate_mode;
    int adts_header;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCACodecFDK, AmlIPCComponent, AmlIPCComponentClass);

/**
 * @brief create the encoder/decoder
 *
 * @param codec, destination codec:
 *   AML_ACODEC_PCM_S16LE: create an decoder and decode aac to pcm s16le
 *   AML_ACODEC_AAC: create an encoder and encode pcm s16le to AAC
 * @param aot, audio object type
 *
 * @return
 */
struct AmlIPCACodecFDK *aml_ipc_fdkaac_new(enum AmlIPCAudioCodec codec, enum AmlAACAOT aot);

/**
 * @brief configure AAC bitrate, only used in encoder
 *
 * @param bitrate
 *   1-5: VBR range enum AmlAACBitrateMode
 *   other value: specify the CBR
 *
 * @return
 */
AmlStatus aml_ipc_fdkaac_set_bitrate(struct AmlIPCACodecFDK *fdk, int bitrate);

/**
 * @brief set if adts header present in the bitstream
 *
 * @param fdk
 * @param present
 *
 * @return
 */
AmlStatus aml_ipc_fdkaac_set_adts_header(struct AmlIPCACodecFDK *fdk, int present);

/**
 * @brief set AAC codec
 *
 * @param fdk
 * @param codec
 *
 * @return
 */
AmlStatus aml_ipc_fdkaac_set_codec(struct AmlIPCACodecFDK *fdk, enum AmlIPCAudioCodec codec);

/**
 * @brief set AAC bitrate mode
 *
 * @param fdk
 * @param bitrate_mode
 *   very-low: 1
 *   low: 2
 *   medium: 3
 *   high: 4
 *   very-high: 5
 *
 * @return
 */
AmlStatus aml_ipc_fdkaac_set_bitrate_mode(struct AmlIPCACodecFDK *fdk, int aac_bitrate_mode);

/**
 * @brief set AAC audio object type
 *
 * @param fdk
 * @param aot
 *
 * @return
 */
AmlStatus aml_ipc_fdkaac_set_object_type(struct AmlIPCACodecFDK *fdk, enum AmlAACAOT aot);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_ACODEC_FDK_H */
