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

#ifndef _NOISE_GATE_H_
#define _NOISE_GATE_H_

#ifdef __cplusplus
extern "C"  {
#endif

enum ng_status {
    NG_ERROR = -1,
    NG_UNMUTE = 0,
    NG_MUTE,
};

void* init_noise_gate(float noise_level, int attrack_time, int release_time);
void release_noise_gate(void *ng_handle);
int noise_evaluation(void *ng_handle, void *buffer, int sample_counter);

#ifdef __cplusplus
}
#endif

#endif
