/*
 * Copyright (C) 2019 Amlogic Corporation.
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

#ifndef AUDIO_VIRTUAL_BUF_H
#define AUDIO_VIRTUAL_BUF_H


#include <stdint.h>

typedef struct audio_virtual_buf audio_virtual_buf_t;


int audio_virtual_buf_open(void ** pphandle, char * buf_name, uint64_t buf_ns_begin, uint64_t buf_ns_target, int ease_time_ms);

int audio_virtual_buf_close(void **pphandle);

int audio_virtual_buf_process(void *phandle, uint64_t frame_ns);

#endif
