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

#ifndef _AUDIO_POST_PROCESS_H_
#define _AUDIO_POST_PROCESS_H_

#include "audio_effect_hal.h"

#define MAX_POSTPROCESSORS 5
struct aml_native_postprocess {
    int num_postprocessors;
    effect_handle_t postprocessors[MAX_POSTPROCESSORS];
};

int audio_post_process(effect_handle_t effect, int16_t *in_buffer, size_t out_frames);

#endif
