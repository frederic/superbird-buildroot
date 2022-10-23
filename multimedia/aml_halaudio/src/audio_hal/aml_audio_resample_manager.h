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
#ifndef AML_AUDIO_RESAMPLE_H
#define AML_AUDIO_RESAMPLE_H


//#include "aml_audio_stream.h"

typedef enum {
    AML_AUDIO_SIMPLE_RESAMPLE,
    AML_AUDIO_ANDROID_RESAMPLE,
    AML_AUDIO_SPEEX_RESAMPLE,
} resample_type_t;


typedef struct audio_resample_config {
    int aformat;
    unsigned int input_sr;
    unsigned int output_sr;
    unsigned int channels;
} audio_resample_config_t;


typedef struct aml_audio_resample {
    resample_type_t resample_type;
    audio_resample_config_t resample_config;
    float resample_rate;
    unsigned int frame_bytes;
    size_t resample_size;   /*the resampled data size*/
    size_t resample_buffer_size; /*the total buffer size*/
    void *resample_buffer;
    void * resample_handle;
    size_t total_in;
    size_t total_out;
} aml_audio_resample_t;


typedef struct audio_resample_func {
    int (*resample_open)(void **handle, audio_resample_config_t *resample_config);
    void (*resample_close)(void *handle);
    int (*resample_process)(void *handle, void * in_buffer, size_t bytes, void * out_buffer, size_t * out_size);

} audio_resample_func_t;



int aml_audio_resample_init(aml_audio_resample_t ** ppresample_handle, resample_type_t resample_type, audio_resample_config_t *resample_config);

int aml_audio_resample_close(aml_audio_resample_t * resample_handle);

int aml_audio_resample_process(aml_audio_resample_t * resample_handle, void * in_data, size_t size);


#endif

