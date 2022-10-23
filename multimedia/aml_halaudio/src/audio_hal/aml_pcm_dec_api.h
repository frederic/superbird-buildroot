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

#ifndef _AML_PCM_DEC_API_H_
#define _AML_PCM_DEC_API_H_

#include "audio_hal.h"
#include "aml_dec_api.h"

struct pcm_dec_t {
    aml_dec_t  aml_dec;
    aml_pcm_config_t pcm_config;
};

int pcm_decoder_init_patch(aml_dec_t **ppdts_dec, audio_format_t format, aml_dec_config_t * dec_config);
int pcm_decoder_release_patch(aml_dec_t *dts_dec);
int pcm_decoder_process_patch(aml_dec_t *dts_dec, unsigned char*buffer, int bytes);

extern aml_dec_func_t aml_pcm_func;

#endif