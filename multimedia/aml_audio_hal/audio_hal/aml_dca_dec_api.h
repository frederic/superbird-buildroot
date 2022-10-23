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

#ifndef _AML_DCA_DEC_API_H_
#define _AML_DCA_DEC_API_H_

#include <hardware/audio.h>
#include "aml_ringbuffer.h"
#include "aml_audio_parser.h"
#include "aml_audio_types_def.h"
#include "aml_audio_resample_manager.h"




struct dca_dts_dec {
    unsigned char *inbuf;
    unsigned char *outbuf;
    unsigned char *outbuf_raw;
    int status;
    int remain_size;
    int outlen_pcm;
    int outlen_raw;
    int is_dtscd;
    int digital_raw;
    //int (*get_parameters) (void *, int *, int *, int *);
    int (*decoder_process)(unsigned char*, int, unsigned char *, int *, char *, int *,struct pcm_info *);
    pthread_mutex_t lock;
    struct pcm_info pcm_out_info;
    aml_audio_resample_t *resample_handle;
    ring_buffer_t output_ring_buf;

};

int dca_decode_init(struct aml_audio_parser *parser);
int dca_decode_release(struct aml_audio_parser *parser);
int dca_decoder_init_patch(struct dca_dts_dec *dts_dec);
int dca_decoder_release_patch(struct dca_dts_dec *dts_dec);
int dca_decoder_process_patch(struct dca_dts_dec *dts_dec, unsigned char*buffer, int bytes);

#endif
