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

#ifndef _PA_MANAGER_H_
#define _PA_MANAGER_H_

#include "audio_hw.h"




int aml_pa_output_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config);

void aml_pa_output_close(void *handle);

size_t aml_pa_output_write(void *handle, const void *buffer, size_t bytes);





#endif // _PA_MANAGER_H_
