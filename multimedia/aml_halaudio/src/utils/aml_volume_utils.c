/*
 * hardware/amlogic/audio/utils/aml_volume_utils.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

#define LOG_TAG "aml_volume_utils"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "log.h"

#include "aml_volume_utils.h"

/* default volume cruve in dB. 101 index.
   0 is mute, 100 is the max volume.
   normally, 0dB is the max volume*/
#define AUDIO_VOLUME_INDEX 101
static float volume_cruve_in_dB[AUDIO_VOLUME_INDEX] = {
    VOLUME_MIN_DB, /*mute*/
    -60,   -53,   -47.5, -43.5, -39,   -36,   -34,   -32,   -30,   -28,    /*1-10*/
    -27,   -26,   -25,   -24,   -23,   -22.2, -21.5, -21,   -20.6, -20.3,  /*11-20*/
    -19.9, -19.5, -19,   -18.7, -18.4, -18.2, -18,   -17.8, -17.5, -17.3,  /*21-30*/
    -17,   -16.8, -16.5, -16.2, -15.9, -15.6, -15.4, -15.2, -14.9, -14.7,  /*31-40*/
    -14.4, -14.1, -13.9, -13.7, -13.5, -13.3, -13.1, -12.9, -12.7, -12.4,  /*41-50*/
    -12.1, -11.8, -11.6, -11.4, -11.2, -11,   -10.8, -10.6, -10.3, -10,    /*51-60*/
    -9.8,  -9.6,  -9.4,  -9.2,  -9,    -8.7,  -8.4,  -8.1,  -7.8,  -7.5,   /*61-70*/
    -7.2,  -6.9,  -6.7,  -6.4,  -6.1,  -5.8,  -5.5,  -5.2,  -5,    -4.8,   /*71-80*/
    -4.7,  -4.5,  -4.3,  -4.1,  -3.8,  -3.6,  -3.3,  -3,    -2.7,  -2.5,   /*81-90*/
    -2.2,  -2,    -1.8,  -1.5,  -1.3,  -1,    -0.8,  -0.5,  -0.3,  0,      /*91-100*/
};

static inline int16_t clamp16(int32_t sample) {
    if ((sample >> 15) ^ (sample >> 31))
        sample = 0x7FFF ^ (sample >> 31);
    return sample;
}

static inline int32_t clamp32(int64_t sample)
{
    if ((sample >> 31) ^ (sample >> 63))
        sample = 0x7FFFFFFF ^ (sample >> 63);
    return sample;
}

void apply_volume(float volume, void *buf, int sample_size, int bytes) {
    int16_t *input16 = (int16_t *)buf;
    int32_t *input32 = (int32_t *)buf;
    unsigned int i = 0;

    if (sample_size == 2) {
        for (i = 0; i < bytes/sizeof(int16_t); i++) {
            int32_t samp = (int32_t)(input16[i]);
            input16[i] = clamp16((int32_t)(volume * samp));
        }
    } else if (sample_size == 4) {
        for (i = 0; i < bytes/sizeof(int32_t); i++) {
            int64_t samp = (int64_t)(input32[i]);
            input32[i] = clamp32((int64_t)(volume * samp));
        }
    } else {
        ALOGE("%s, unsupported audio format: %d!\n", __FUNCTION__, sample_size);
    }
    return;
}

float get_volume_by_index(int volume_index) {
    float volume = 1.0;
    if (volume_index >= AUDIO_VOLUME_INDEX) {
        ALOGE("%s, invalid index!\n", __FUNCTION__);
        return volume;
    }
    if (volume_index >= 0)
        volume *= DbToAmpl(volume_cruve_in_dB[volume_index]);

    return volume;
}

