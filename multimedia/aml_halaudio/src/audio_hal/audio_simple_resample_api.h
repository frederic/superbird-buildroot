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

#ifndef AUDIO_SIMPLE_RESAMPLE_H
#define AUDIO_SIMPLE_RESAMPLE_H

#include "aml_audio_resample_manager.h"


int simple_resample_open(void **handle, audio_resample_config_t *resample_config);
void simple_resample_close(void *handle);
int simple_resample_process(void *handle, void * in_buffer, size_t bytes, void * out_buffer, size_t * out_size);

extern audio_resample_func_t audio_simple_resample_func;

#endif
