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

#ifndef __AML_RESAMPLE_WRAP_H__
#define __AML_RESAMPLE_WRAP_H__
#include <stdint.h>
#include <stdlib.h>
#include "aml_ringbuffer.h"


typedef size_t (*read_func)(void *handle, void* buffer, size_t bytes);

typedef struct android_resample_handle {
    unsigned int input_sr;
    unsigned int output_sr;
    unsigned int channels;
    unsigned int ringbuf_size;
    ring_buffer_t ring_buf;
    void * resampler;
    void * provider;
    unsigned char * temp_buf;
    unsigned int temp_buf_size;
}android_resample_handle_t;

int android_resample_init(android_resample_handle_t *handle,
                          int sr,
                          audio_format_t aformat,
                          int ch,
                          read_func read,
                          void *read_handle);

int android_resample_read(android_resample_handle_t *handle, void *buf, size_t in_size);

int android_resample_release(android_resample_handle_t *handle);

#endif  //__AML_RESAMPLE_WRAP_H__
