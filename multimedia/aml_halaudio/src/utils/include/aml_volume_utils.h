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

#ifndef __AML_VOLUME_UTILS_H__
#define __AML_VOLUME_UTILS_H__

#include <math.h>

// Absolute min volume in dB (can be represented in single precision normal float value)
#define VOLUME_MIN_DB (-758)

static inline float DbToAmpl(float decibels)
{
    if (decibels <= VOLUME_MIN_DB) {
        return 0.0f;
    }
    return exp( decibels * 0.115129f); // exp( dB * ln(10) / 20 )
}

static inline float AmplToDb(float amplification)
{
    if (amplification == 0) {
        return VOLUME_MIN_DB;
    }
    return 20 * log10(amplification);
}

float get_volume_by_index(int volume_index);
void apply_volume(float volume, void *buf, int sample_size, int bytes);

#endif

