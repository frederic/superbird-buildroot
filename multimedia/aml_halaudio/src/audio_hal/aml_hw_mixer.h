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

#ifndef _AML_HW_MIXER_H_
#define _AML_HW_MIXER_H_

#include <pthread.h>

// just keep the original size. Have no idea why
//#define AML_HW_MIXER_BUF_SIZE  64*1024
#define AML_HW_MIXER_BUF_SIZE  256*1024
struct aml_hw_mixer {
    unsigned char *start_buf;
    unsigned int wp;
    unsigned int rp;
    unsigned int buf_size;
    unsigned char need_cache_flag;//flag to check if need cache some data before write to mix
    pthread_mutex_t lock;
};

int aml_hw_mixer_init(struct aml_hw_mixer *mixer);
void aml_hw_mixer_deinit(struct aml_hw_mixer *mixer);
//static uint aml_hw_mixer_get_space(struct aml_hw_mixer *mixer);
int aml_hw_mixer_get_content(struct aml_hw_mixer *mixer);
//we assue the cached size is always smaller then buffer size
//need called by device mutux locked
int aml_hw_mixer_write(struct aml_hw_mixer *mixer, const void *w_buf, uint size);
int aml_hw_mixer_mixing(struct aml_hw_mixer *mixer, void *w_buf, int size);
//need called by device mutux locked
int aml_hw_mixer_read(struct aml_hw_mixer *mixer, void *r_buf, uint size);
void aml_hw_mixer_reset(struct aml_hw_mixer *mixer);

#endif //_AML_HW_MIXER_H_
