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

#include <math.h>
#include "log.h"
#include <aml_audio_resampler.h>

#undef  LOG_TAG
#define LOG_TAG "aml_audio_resampler"

//Clip from 16.16 fixed-point to 0.15 fixed-point.
inline static short clip(int x) {
    if (x < -32768) {
        return -32768;
    } else if (x > 32767) {
        return 32767;
    } else {
        return x;
    }
}

int resampler_init(struct resample_para *resample) {

    ALOGD("%s, Init Resampler: input_sr = %d, output_sr = %d \n",
        __FUNCTION__,resample->input_sr,resample->output_sr);

    static const double kPhaseMultiplier = 1L << 28;
    unsigned int i;

    if (resample->channels > MAX_RESAMPLE_CHANNEL) {
        ALOGE("Error: %s, max support channels: %d\n",
        __FUNCTION__, MAX_RESAMPLE_CHANNEL);
        return -1;
    }

    resample->FractionStep = (unsigned int) (resample->input_sr * kPhaseMultiplier
                            / resample->output_sr);
    resample->SampleFraction = 0;
    for (i = 0; i < resample->channels; i++)
        resample->lastsample[i] = 0;

    return 0;
}

int resample_process(struct resample_para *resample, unsigned int in_frame,
        short* input, short* output) {
    unsigned int inputIndex = 0;
    unsigned int outputIndex = 0;
    unsigned int FractionStep = resample->FractionStep;
    short last_sample[MAX_RESAMPLE_CHANNEL];
    unsigned int i;
    unsigned int channels = resample->channels;

    static const unsigned int kPhaseMask = (1LU << 28) - 1;
    unsigned int frac = resample->SampleFraction;

    for (i = 0; i < channels; i++)
        last_sample[i] = resample->lastsample[i];


    while (inputIndex == 0) {
        for (i = 0; i < channels; i++) {
            *output++ = clip((int) last_sample[i] +
                ((((int) input[i] - (int) last_sample[i]) * ((int) frac >> 13)) >> 15));
        }

        frac += FractionStep;
        inputIndex += (frac >> 28);
        frac = (frac & kPhaseMask);
        outputIndex++;
    }

    while (inputIndex < in_frame) {
        for (i = 0; i < channels; i++) {
            *output++ = clip((int) input[channels * (inputIndex - 1) + i] +
                ((((int) input[channels * inputIndex + i]
                - (int) input[channels * (inputIndex - 1) + i]) * ((int) frac >> 13)) >> 15));
        }

        frac += FractionStep;
        inputIndex += (frac >> 28);
        frac = (frac & kPhaseMask);
        outputIndex++;
    }

    resample->SampleFraction = frac;

    for (i = 0; i < channels; i++)
        resample->lastsample[i] = input[channels * (in_frame - 1) + i];

    return outputIndex;
}

