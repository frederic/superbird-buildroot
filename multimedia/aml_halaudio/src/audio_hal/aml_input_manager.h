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

#ifndef AML_INTPUT_MANAGER_H
#define AML_INTPUT_MANAGER_H

#include <tinyalsa/asoundlib.h>

#include "audio_hw.h"



enum {
    input_OK    =  0,
    input_ERROR = -1,
    input_INUSE = -2
};


struct aml_input_handle {
    audio_devices_t device; /* speaker/spdif etc*/
    aml_stream_config_t stream_config;  /*stream basic info*/

    void * device_handle;


};

struct aml_input_function {
    int (*input_open)(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config);
    void (*input_close)(void *handle);
    size_t (*input_read)(void *handle, void *buffer, size_t bytes);

};

void aml_input_init(void);

int aml_input_open(struct audio_stream_in *stream, aml_stream_config_t * stream_config, aml_device_config_t *device_config);

int aml_input_close(struct audio_stream_in *stream);

int aml_input_read(struct audio_stream_in *stream, const void *buffer, int bytes);

#endif
