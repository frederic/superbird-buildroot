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

#include <string.h>

#ifndef AML_IPC_ACONVERT_H

#define AML_IPC_ACONVERT_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlAConvertChannel { AML_ACONVERT_INPUT, AML_ACONVERT_OUTPUT };

struct AmlIPCAConvertPriv;
struct AmlIPCAConvert {
    AML_OBJ_EXTENDS(AmlIPCAConvert, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCAudioFormat format;
    struct AmlIPCAConvertPriv *priv;
    char *channel_mapping;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCAConvert, AmlIPCComponent, AmlIPCComponentClass);

/**
 * @brief create a new audio converter
 *
 * @param fmt, set the dst format
 *   codec currently only support AML_ACODEC_PCM_S16LE
 *   sample_width set to 16 for AML_ACODEC_PCM_S16LE
 *   sample_rate
 *   num_channel
 *
 * @return
 */
struct AmlIPCAConvert *aml_ipc_aconv_new(struct AmlIPCAudioFormat *fmt);

/**
 * @brief set channel mapping of source channel and dst channel
 *
 * @param aconv
 * @param mapping, each character represent an channel index from source data,
 * is it a string value, the characters must larger than dst channels
 *   here's examples for how channels are mapped to 2ch audio
 *   "01": src ch0 -> dst ch0, src ch1 -> dst ch1, this is the default behavior
 *   "10": src ch1 -> dst ch0, src ch0 -> dst ch1
 *   "11": all use src ch1, src ch0 is droped
 *
 * @return
 */
AmlStatus aml_ipc_aconv_set_channel_mapping(struct AmlIPCAConvert *aconv, const char *mapping);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_ACONVERT_H */
