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

#ifndef _AML_DEC_API_H_
#define _AML_DEC_API_H_

#include "audio_hal.h"
#include "aml_ringbuffer.h"
#include "aml_audio_resampler.h"
#include "aml_audio_parser.h"

typedef struct aml_dec_info {
    int stream_sr;    /** the sample rate in stream*/
    int stream_ch;    /** the original channels in stream*/
    int stream_bitwidth; /** the original bitwidth in stream*/

    int output_sr ;   /** the decoded data samplerate*/
    int output_ch ;   /** the decoded data channels*/
    int output_bitwidth; /**the decoded sample bit width*/

    int output_multi_sr ;   /** the decoded multi channel data samplerate*/
    int output_multi_ch ;   /** the decoded multi channel data channels*/
    int output_multi_bitwidth; /**the decoded multi channel sample bit width*/
    int output_bLFE;

    int bitstream_type;
    int bitstream_subtype;

} aml_dec_info_t;

typedef struct aml_dec {
    audio_format_t format;
    unsigned char *inbuf;
    unsigned char *outbuf;
    unsigned char *outbuf_raw;
    int status;
    int remain_size;
    int outlen_pcm;
    int outlen_raw;
    int inbuf_max_len;
    int outbuf_max_len;
    int outbuf_raw_max_len;
    unsigned char *outbuf_multi;
    int outlen_multi;
    int digital_raw;
    bool is_iec61937;

    /**after decoder success, we can query these info*/
    aml_dec_info_t dec_info;
    void * dec_func;
    void *dec_ptr;/*an handle which is used in datmos*/
    size_t raw_deficiency;
    size_t inbuf_wt;
    size_t burst_payload_size;
    /*if format is MAT[TrueHD/PCM]*/
    /*61440bytes |PA PB PC PD ....|*/
    /*61440bytes |PA PB PC PD ....|*/
    int first_PAPB;
    int next_PAPB;
    int last_consumed_bytes;
    int is_truehd_within_mat;
    int is_dolby_atmos;
    int init_argc;
    char **init_argv;

    float min_time;
    float max_time;
    float average_time;
    int   frame_total_ms;
    int   dec_total_ms;
    int   frame_cnt;
} aml_dec_t;

typedef struct aml_dcv_config {
    int digital_raw;
    int nIsEc3;
    int dcv_output_ch;   /*config the output for dcv*/
    void *reserved;//dlopen handle
} aml_dcv_config_t;

typedef struct aml_dca_config {
    int digital_raw;
    int is_dtscd;
    void *reserved;//dlopen handle
} aml_dca_config_t;

typedef struct aml_datmos_config {
    /*to storage the audio input information*/
    int pcm_format;
    int samplerate;
    int channel;
    int bitwidth;

    int audio_type;
    int is_eb3_extension;
    void *reserved;//dlopen handle
} aml_datmos_config_t;

typedef struct aml_pcm_config {
    int pcm_format; // refer to audio_format_t
    int samplerate;
    int channel;
    int bitwidth;
} aml_pcm_config_t;

typedef union aml_dec_config {
    aml_dcv_config_t dcv_config;
    aml_dca_config_t dca_config;
    aml_datmos_config_t datmos_config;
    aml_pcm_config_t pcm_config;
} aml_dec_config_t;

enum {
    AML_CONFIG_DECODER,
    AML_CONFIF_OUTPUT
};

enum dolby_strategy{
    AML_DOLBY_DECODER,
    AML_DOLBY_MS12,
    AML_DOLBY_ATMOS,
};



typedef int (*F_Init)(aml_dec_t **ppaml_dec, audio_format_t format, aml_dec_config_t * dec_config);
typedef int (*F_Release)(aml_dec_t *aml_dec);
typedef int (*F_Process)(aml_dec_t *aml_dec, unsigned char* buffer, int bytes);
typedef int (*F_Config)(aml_dec_t *aml_dec, int config_type, aml_dec_config_t * config_value);
typedef int (*F_Dynamic_param_set)(aml_dec_t *aml_dec);

typedef struct aml_dec_func {
    F_Init                  f_init;
    F_Release               f_release;
    F_Process               f_process;
    F_Config                f_config;
    F_Dynamic_param_set     f_dynamic_param_set;
} aml_dec_func_t;



int aml_decoder_init(aml_dec_t **aml_dec, audio_format_t format, aml_dec_config_t * dec_config);
int aml_decoder_release(aml_dec_t *aml_dec);
int aml_decoder_config(aml_dec_t *aml_dec, aml_dec_config_t * config);
int aml_decoder_process(aml_dec_t *aml_dec, unsigned char*buffer, int bytes, int * used_bytes);

/*
 *@brief amlogic decoder dynamic param set
 * input params:
 *          aml_dec_t *aml_dec: decoder handle
 *
 * return value:
 *          0: success;
*           otherwise errror occur.
 */
int aml_decoder_dynamic_param_set(aml_dec_t *aml_dec);

#endif
