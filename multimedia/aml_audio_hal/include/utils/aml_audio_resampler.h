/*
 * Copyright (C) 2017 Amlogic Corporation.
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

#ifndef __AUDIO_RESAMPLER_H__
#define __AUDIO_RESAMPLER_H__

#define MAX_RESAMPLE_CHANNEL 8

#include <stdint.h>

struct resample_para {
    unsigned int FractionStep;
    unsigned int SampleFraction;
    unsigned int input_sr;
    unsigned int output_sr;
    unsigned int channels;
    int16_t lastsample[MAX_RESAMPLE_CHANNEL];
};

int resampler_init(struct resample_para *resample);
int resample_process(struct resample_para *resample, unsigned int in_frame,
    int16_t* input, int16_t* output);

#endif
