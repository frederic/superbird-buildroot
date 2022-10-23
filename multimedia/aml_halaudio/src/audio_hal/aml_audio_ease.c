/*
 * Copyright (C) 2018 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "aml_audio_ease"

#include "aml_audio_ease.h"
#include "log.h"
#include "aml_audio_log.h"


typedef struct aml_audio_ease {
    ease_type_t ease_type;
    int ease_time;
    float current_volume;
    float start_volume;
    float target_volume;
    unsigned int ease_frames_elapsed;
    unsigned int ease_frames;

} aml_audio_ease_t;


/*
    Robert Penner's Easing Equations v1.5  May 1, 2003
    (ported to C++ by tmyles)
    (c) 2003 Robert Penner, all rights reserved.
    This work is subject to the terms in http://www.robertpenner.com/easing_terms_of_use.html

    (Following copy/pasted from http://www.robertpenner.com/easing_terms_of_use.html on 7/12/2010)
    TERMS OF USE - EASING EQUATIONS

    Open source under the BSD License.

    Copyright 2001 Robert Penner All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. Redistributions in
    binary form must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution. Neither the name of the
    author nor the names of contributors may be used to endorse or promote
    products derived from this software without specific prior written
    permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
    BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
    FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
    USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

float floatEaseNext(ease_type_t ease, float t, float b, float c, float d)
{
    switch (ease) {
    default:
    case EaseLinear:
        return (c * t) / d + b;
    case EaseInQuad:
        t /= d;
        return c * t * t + b;
    case EaseOutQuad:
        t /= d;
        return -c * t * (t - 2) + b;
    case EaseInOutQuad:
        if ((t /= d / 2) < 1) {
            return ((c / 2) * (t * t)) + b;
        } else {
            --t;
            return -c / 2 * (((t - 2) * t) - 1) + b;
        }
    case EaseInCubic:
        t /= d;
        return c * t * t * t + b;
    case EaseOutCubic:
        t = t / d - 1;
        return c * (t * t * t + 1) + b;
    case EaseInOutCubic:
        if ((t /= d / 2) < 1) {
            return c / 2 * t * t * t + b;
        } else {
            t -= 2;
            return c / 2 * (t * t * t + 2) + b;
        }
    case EaseInQuart:
        t /= d;
        return c * t * t * t * t + b;
    case EaseOutQuart:
        t = t / d - 1;
        return -c * (t * t * t * t - 1) + b;
    case EaseInOutQuart:
        if ((t /= d / 2) < 1) {
            return c / 2 * t * t * t * t + b;
        } else {
            t -= 2;
            return -c / 2 * (t * t * t * t - 2) + b;
        }
    case EaseInQuint:
        t /= d;
        return c * t * t * t * t * t + b;
    case EaseOutQuint:
        t = t / d - 1;
        return c * (t * t * t * t * t + 1) + b;
    case EaseInOutQuint:
        if ((t /= d / 2) < 1) {
            return c / 2 * t * t * t * t * t + b;
        } else {
            t -= 2;
            return c / 2 * (t * t * t * t * t + 2) + b;
        }
    }
}


void aml_audio_ease_dumpinfo(void * private)
{
    aml_audio_ease_t * ease_handle = NULL;
    if (private == NULL) {
        return;
    }
    ease_handle = (aml_audio_ease_t *)private;

    ALOGA("Ease Type=%d ease time=%d \n", ease_handle->ease_type, ease_handle->ease_time);
    ALOGA("Ease Vol Cur=%f Start=%f Target=%f\n", ease_handle->current_volume, ease_handle->start_volume, ease_handle->target_volume);
    ALOGA("Ease Frames=%d Elapsed Frames=%d\n", ease_handle->ease_frames, ease_handle->ease_frames_elapsed);

    return;
}


int aml_audio_ease_init(aml_audio_ease_t ** ppease_handle)
{
    aml_audio_ease_t * ease_handle = NULL;
    ease_handle = (aml_audio_ease_t *)calloc(1, sizeof(aml_audio_ease_t));
    if (ease_handle == NULL) {
        ALOGE("malloc failed\n");
        return -1;
    }
    ease_handle->ease_type      = 0;
    ease_handle->current_volume = 1.0;
    ease_handle->start_volume   = 1.0;
    ease_handle->target_volume  = 1.0;
    ease_handle->ease_time      = 0;
    ease_handle->ease_frames    = 0;
    ease_handle->ease_frames_elapsed = 0;

    * ppease_handle = ease_handle;

    aml_log_dumpinfo_install(LOG_TAG, aml_audio_ease_dumpinfo, ease_handle);


    return 0;
}

int aml_audio_ease_close(aml_audio_ease_t * ease_handle)
{
    aml_log_dumpinfo_remove(LOG_TAG);
    if (ease_handle) {
        free(ease_handle);
    }
    return 0;
}

int aml_audio_ease_config(aml_audio_ease_t * ease_handle, ease_setting_t *setting)
{

    if (ease_handle == NULL || setting == NULL) {
        return -1;
    }

    ease_handle->target_volume = setting->target_volume;
    ease_handle->ease_type     = setting->ease_type;
    ease_handle->ease_time     = setting->duration;
    ease_handle->ease_frames_elapsed = 0;
    if (ease_handle->ease_time == 0) {
        ease_handle->current_volume = setting->target_volume;
    }
    ease_handle->start_volume  = ease_handle->current_volume;


    return 0;
}

int aml_audio_ease_process(aml_audio_ease_t * ease_handle, void * in_data, size_t size, aml_data_format_t *format)
{

    int nframes = 0;
    int ch = 0;
    int i = 0, j = 0;
    int bitwidth = SAMPLE_16BITS;
    float vol_delta;
    if (ease_handle == NULL || format == NULL) {
        return -1;
    }


    ch = format->ch;
    bitwidth = format->bitwidth;
    if (ch == 0 || bitwidth == 0) {
        return -1;
    }
    nframes  = size / ((bitwidth >> 3) * ch);


    ease_handle->ease_frames = ((long long)ease_handle->ease_time * format->sr) / 1000;

    vol_delta = ease_handle->target_volume - ease_handle->start_volume;

    switch (bitwidth) {
    case SAMPLE_8BITS:
        // not support
        break;
    case SAMPLE_16BITS: {
        short * data = (short*) in_data;

        for (j = 0; j < nframes; j++) {
            if (ease_handle->ease_frames == 0) {
                ease_handle->current_volume = ease_handle->target_volume;
            } else if (ease_handle->ease_frames_elapsed < ease_handle->ease_frames) {

                ease_handle->current_volume = floatEaseNext(ease_handle->ease_type, (float)ease_handle->ease_frames_elapsed,
                                              ease_handle->start_volume, vol_delta, (float)(ease_handle->ease_frames - 1));
                ease_handle->ease_frames_elapsed++;
            }

            for (i = 0 ; i < ch; i++) {
                data[j * ch + i] = data[j * ch + i] * ease_handle->current_volume;
            }
        }
    }
    break;
    case SAMPLE_24BITS:
        // not suppport
        break;
    case SAMPLE_32BITS: {
        int * data = (int*) in_data;
        //ALOGD("ease frames=%d time=%d cur=%f target=%f\n",ease_handle->ease_frames, ease_handle->ease_time,ease_handle->current_volume,ease_handle->target_volume);
        for (j = 0; j < nframes; j++) {
            if (ease_handle->ease_frames == 0) {
                ease_handle->current_volume = ease_handle->target_volume;
            } else if (ease_handle->ease_frames_elapsed < ease_handle->ease_frames) {

                ease_handle->current_volume = floatEaseNext(ease_handle->ease_type, (float)ease_handle->ease_frames_elapsed,
                                              ease_handle->start_volume, vol_delta, (float)(ease_handle->ease_frames - 1));

                ease_handle->ease_frames_elapsed++;
            }
            //ALOGD("frame=%d ease volume=%f\n",j, ease_handle->current_volume);
            for (i = 0 ; i < ch; i++) {
                data[j * ch + i] = data[j * ch + i] * ease_handle->current_volume;
            }
        }

    }
    break;
    default:
        break;
    }

    return 0;
}



