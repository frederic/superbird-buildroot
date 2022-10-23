/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/


#define LOG_TAG "audio_data_process"
#define LOG_NDEBUG 0

#include <errno.h>
#include <cutils/log.h>
#include <system/audio.h>

#include "audio_data_process.h"

static int ch2_ch8_n_b16_b32(void *data_mixed, void *data_sys, size_t frames)
{
    uint32_t *out_buf = data_mixed;
    uint16_t *in_buf = data_sys;
    uint i = 0;
    for (i = 0; i < frames; i++) {
        out_buf[8 * i] = in_buf[2 * i] << 16;
        out_buf[8 * i + 1] = in_buf[2 * i + 1] << 16;
        out_buf[8 * i + 2] = in_buf[2 * i] << 16;
        out_buf[8 * i + 3] = in_buf[2 * i + 1] << 16;
        out_buf[8 * i + 4] = in_buf[2 * i] << 16;
        out_buf[8 * i + 5] = in_buf[2 * i + 1] << 16;
        out_buf[8 * i + 6] = in_buf[2 * i] << 16;
        out_buf[8 * i + 7] = in_buf[2 * i + 1] << 16;
    }
    return 0;
}

static inline short CLIPSHORT(int32_t r)
{
    if (r > 32767)
        r = 32767;
    else if (r < -32768)
        r = -32768;
    return r;
}

static inline int CLIPINT(int64_t r)
{
    if (r > 2147483647)
        r = 2147483647;
    else if (r < -2147483648)
        r = -2147483648;
    return r;
}

//should be same channelCnt 2
int do_mixing_2ch(void *data_mixed,
        void *data_in, size_t frames,
        struct audioCfg inCfg, struct audioCfg mixerCfg)
{
    int i = 0;
    int samples = frames * inCfg.channelCnt;

    if (mixerCfg.format == AUDIO_FORMAT_PCM_32_BIT) {
        if (inCfg.format == AUDIO_FORMAT_PCM_16_BIT) {
            int16_t *in = data_in;
            int32_t *out = data_mixed;
            int64_t tmp = 0;
            for (i = 0; i < samples; i++) {
                tmp = (int64_t)*out + (int64_t)((*in++) << 16);
                *out++ = CLIPINT(tmp);
            }
        } else if (inCfg.format == AUDIO_FORMAT_PCM_32_BIT) {
            int32_t *in = data_in;
            int32_t *out = data_mixed;
            int64_t tmp = 0;
            for (i = 0; i < samples; i++) {
                tmp = (int64_t)*out + (int64_t)*in++;
                *out++ = CLIPINT(tmp);
            }
        }
    } else if (mixerCfg.format == AUDIO_FORMAT_PCM_16_BIT) {
        if (inCfg.format == AUDIO_FORMAT_PCM_16_BIT) {
            int16_t *in = data_in;
            int16_t *out = data_mixed;
            int32_t tmp = 0;
            for (i = 0; i < samples; i++) {
                tmp = (int32_t)*out + (int32_t)*in++;
                *out++ = CLIPSHORT(tmp);
            }
        } else if (inCfg.format == AUDIO_FORMAT_PCM_32_BIT) {
            int32_t *in = data_in;
            int16_t *out = data_mixed;
            int32_t tmp = 0;
            for (i = 0; i < samples; i++) {
                tmp = (int32_t)*out + ((*in++) >> 16);
                *out++ = CLIPSHORT(tmp);
            }
        }
    } else {
        ALOGE("%s(), format %#x inval", __func__, inCfg.format);
        return 0;
    }
    return frames;
}
// 2->8ch, 32bit, in out no overlap
int extend_channel_2_8(void *data_out, void *data_in,
        size_t frames, int ch_cnt_in, int ch_cnt_out)
{
    (void *)ch_cnt_out;
    (void *)ch_cnt_in;
    int32_t *in = (int32_t *)data_in;
    int32_t *out = (int32_t *)data_out;
    int32_t Lval = 0, Rval = 0;
    uint i = 0 , j = 0;

    for (i = 0; i < frames; i++) {
        Lval = *in++;
        Rval = *in++;
        for (j = 0; j < 4; j++) {
            *out++ = Lval;
            *out++ = Rval;
        }
    }
    return 0;
}

int extend_channel_5_8(void *data_out, void *data_in,
        size_t frames, int ch_cnt_in, int ch_cnt_out)
{
    (void *)ch_cnt_out;
    (void *)ch_cnt_in;
    int32_t *out = data_out;
    int32_t *in = data_in;
    uint i = 0;
    for (i = 0; i < frames; i++) {
        out[8 * i] = in[5 * i + 2];
        out[8 * i + 1] = in[5 * i + 3];
        out[8 * i + 2] = in[5 * i];
        out[8 * i + 3] = in[5 * i + 1];
        out[8 * i + 4] = in[5 * i + 4];
        out[8 * i + 5] = 0;
        out[8 * i + 6] = 0;
        out[8 * i + 7] = 0;
    }

    return 0;
}
int processing_and_convert(void *data_mixed,
        void *data_sys, size_t frames,
        struct audioCfg inCfg, struct audioCfg mixerCfg)
{
    if (data_mixed == NULL || data_sys == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    if (inCfg.format == AUDIO_FORMAT_PCM_16_BIT && mixerCfg.format == AUDIO_FORMAT_PCM_32_BIT
            && inCfg.channelCnt == 2 && mixerCfg.channelCnt == 8) {
        ch2_ch8_n_b16_b32(data_mixed, data_sys, frames);
    } else {
        ALOGE("%s(), not support", __func__);
    }

    return 0;
}
