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
#ifndef AML_AUDIO_EASE_H
#define AML_AUDIO_EASE_H


#include "aml_audio_stream.h"

typedef struct aml_audio_ease aml_audio_ease_t;

int aml_audio_ease_init(aml_audio_ease_t ** ppease_handle);

int aml_audio_ease_close(aml_audio_ease_t * ease_handle);

int aml_audio_ease_config(aml_audio_ease_t * ease_handle, ease_setting_t *setting);

int aml_audio_ease_process(aml_audio_ease_t * ease_handle, void * in_data, size_t size, aml_data_format_t *format);


#endif

