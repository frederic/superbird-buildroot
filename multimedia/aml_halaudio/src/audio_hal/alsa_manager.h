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

#ifndef _ALSA_MANAGER_H_
#define _ALSA_MANAGER_H_

#include "audio_hw.h"
#include <cjson/cJSON.h>


/**
 * pcm open with configs in streams: card, device, pcm_config
 * If device has been opened, close it and reopen with new params
 * and increase the refs count
 */
int aml_alsa_output_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config);

/**
 * decrease the pcm refs count and do pcm close when refs count equals zero.
 */
void aml_alsa_output_close(void *handle);

/**
 * pcm_write to the pcm handle saved in stream instance.
 */
size_t aml_alsa_output_write(void *handle, const void *buffer, size_t bytes);

int aml_alsa_input_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config) ;

void aml_alsa_input_close(void *handle);

size_t aml_alsa_input_read(void *handle, void *buffer, size_t bytes);

void aml_alsa_init(cJSON * config);





#endif // _ALSA_MANAGER_H_
