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

#ifndef _AML_DCV_DEC_API_H_
#define _AML_DCV_DEC_API_H_

#include <hardware/audio.h>
#include "aml_ringbuffer.h"
#include "aml_audio_resampler.h"
#include "aml_audio_parser.h"
#include "aml_audio_types_def.h"

struct dolby_ddp_dec {
    unsigned char *inbuf;
    unsigned char *outbuf;
    unsigned char *outbuf_raw;
    int status;
    int remain_size;
    int outlen_pcm;
    int outlen_raw;
    int nIsEc3;
    int digital_raw;
    bool is_iec61937;
    int curFrmSize;
    int (*get_parameters)(void *, int *, int *, int *,int *);
    int (*decoder_process)(unsigned char*, int, unsigned char *, int *, char *, int *, int, struct pcm_info *);
    pthread_mutex_t lock;
    struct pcm_info pcm_out_info;
    struct resample_para aml_resample;
    unsigned char *resample_outbuf;
    ring_buffer_t output_ring_buf;
    int is_dolby_atmos;
};


int load_ddp_decoder_lib();
int unload_ddp_decoder_lib();

int dcv_decode_init(struct aml_audio_parser *parser);
int dcv_decode_release(struct aml_audio_parser *parser);


int dcv_decoder_init_patch(struct dolby_ddp_dec *ddp_dec);
int dcv_decoder_release_patch(struct dolby_ddp_dec *ddp_dec);
int dcv_decoder_process_patch(struct dolby_ddp_dec *ddp_dec, unsigned char*buffer, int bytes);
int dcv_decoder_get_framesize(unsigned char*buffer, int bytes, int* p_head_offset);

#endif
