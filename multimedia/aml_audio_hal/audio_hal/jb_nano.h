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

#ifndef __JB_NANO_H__
#define __JB_NANO_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <tinyalsa/asoundlib.h>
#include <audio_hw.h>

void nano_init(void);
int  nano_input_open(struct audio_stream_in *stream_in, struct audio_config *config);
void nano_close(struct audio_stream_in *stream);
int  nano_is_connected(void);


#ifdef __cplusplus
}
#endif
#endif

