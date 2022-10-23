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


#ifndef AML_AUDIO_LOG_H
#define AML_AUDIO_LOG_H

typedef void (*dump_info_func_t)(void *);

void aml_log_init();
void aml_log_exit();
void aml_log_dumpinfo_install(char * name, dump_info_func_t dump_func, void * private);
void aml_log_dumpinfo_remove(char * name);
int aml_log_get_dumpfile_enable(char * name);


#endif
