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
#ifndef AML_IPC_AIN_H

#define AML_IPC_AIN_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlAInChannel { AML_AIN_OUTPUT };
enum AmlAInSource { AML_AIN_SRC_ALSA, AML_AIN_SRC_FILE };
struct AmlIPCAInPriv;
struct AmlIPCAIn {
    AML_OBJ_EXTENDS(AmlIPCAIn, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCAInPriv *priv;
    struct AmlIPCAudioFormat format;
    unsigned int buffer_time;
    unsigned int period_time;
    int block_size;
    char *device;
    char *vol_device;
    char *vol_control;
    float volume;
    int active_mode;
    int source; // enum AmlAInSource
};
AML_OBJ_DECLARE_TYPEID(AmlIPCAIn, AmlIPCComponent, AmlIPCComponentClass);

struct AmlIPCAIn *aml_ipc_ain_new();

AmlStatus aml_ipc_ain_set_volume(struct AmlIPCAIn *ain, float vol);

/**
 * @brief set AudioIn active mode
 *
 * @param ain
 * @param active_moode
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_active(struct AmlIPCAIn *ain, int active);

/**
 * @brief set AudioIn device
 *
 * @param ain
 * @param device
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_device(struct AmlIPCAIn *ain, char *device);

/**
 * @brief set AudioIn buffer time in us
 *
 * @param ain
 * @param time
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_buffer_time(struct AmlIPCAIn *ain, int time);

/**
 * @brief set AudioIn period time in us
 *
 * @param ain
 * @param time
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_period_time(struct AmlIPCAIn *ain, int time);

/**
 * @brief set AudioIn block size
 *
 * @param ain
 * @param size
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_blksize(struct AmlIPCAIn *ain, int size);

/**
 * @brief set AudioIn codec
 *
 * @param ain
 * @param codec
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_codec(struct AmlIPCAIn *ain, enum AmlIPCAudioCodec codec);

/**
 * @brief set AudioIn sample rate
 *
 * @param ain
 * @param rate
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_rate(struct AmlIPCAIn *ain, int rate);

/**
 * @brief set AudioIn channel
 *
 * @param ain
 * @param channel
 *   number of channel
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_channel(struct AmlIPCAIn *ain, int channel);

/**
 * @brief set AudioIn alsa volume device
 *
 * @param ain
 * @param vol_dev
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_vol_device(struct AmlIPCAIn *ain, char *vol_dev);

/**
 * @brief set AudioIn alsa volume control
 *
 * @param ain
 * @param vol_con
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_vol_control(struct AmlIPCAIn *ain, char *vol_ctl);

/**
 * @brief set AudioIn source
 *
 * @param ain
 * @param src
 *
 * @return
 */
AmlStatus aml_ipc_ain_set_source(struct AmlIPCAIn *ain, enum AmlAInSource src);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_AIN_H */
