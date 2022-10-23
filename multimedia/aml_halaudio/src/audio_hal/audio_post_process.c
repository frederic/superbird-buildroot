/*
 * hardware/amlogic/audio/audioeffect/audio_post_process.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

#define LOG_TAG "audio_post_process"

#include "log.h"
#include "audio_post_process.h"
int audio_post_process(effect_handle_t effect, int16_t *in_buffer, size_t out_frames)
{
    int ret = 0;
    audio_buffer_t in_buf;
    audio_buffer_t out_buf;

    in_buf.frameCount = out_buf.frameCount = out_frames;
    in_buf.s16 = out_buf.s16 = in_buffer;

    ret = (*effect)->process(effect, &in_buf, &out_buf);

    if (ret < 0) {
        ALOGE("postprocess failed\n");
    }

    return ret;
}
