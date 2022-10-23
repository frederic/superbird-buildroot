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

#ifndef _AML_CALLBACK_API_H_
#define _AML_CALLBACK_API_H_

#include "list.h"
#include "audio_hal.h"


typedef struct aml_audiocb_handle aml_audiocb_handle_t;


int init_audio_callback (aml_audiocb_handle_t **ppcallback_handle);
int exit_audio_callback (aml_audiocb_handle_t **ppcallback_handle);
int install_audio_callback(aml_audiocb_handle_t *pcallback_handle, audio_callback_info_t * callback_info, audio_callback_func_t   callback_func);
int remove_audio_callback(aml_audiocb_handle_t *pcallback_handle, audio_callback_info_t * callback_info);
void trigger_audio_callback (aml_audiocb_handle_t *pcallback_handle, audio_callback_type_t type, audio_callback_data_t * data);


#endif
