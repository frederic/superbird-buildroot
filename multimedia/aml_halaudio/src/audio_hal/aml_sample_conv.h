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

#ifndef AML_SAMPLE_CONV_H
#define AML_SAMPLE_CONV_H

#include "aml_audio_stream.h"

typedef struct aml_sample_conv {
    void *convert_buffer;
    size_t convert_size;
    size_t buf_size;
} aml_sample_conv_t;


int aml_sampleconv_init(aml_sample_conv_t ** handle);
int aml_sampleconv_close(aml_sample_conv_t * handle);
int aml_sampleconv_process(aml_sample_conv_t * handle, aml_data_format_t *src, void * in_data, aml_data_format_t *dst, size_t nsamples);




#endif
