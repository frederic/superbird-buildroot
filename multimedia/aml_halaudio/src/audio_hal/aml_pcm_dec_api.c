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

#define LOG_TAG "aml_audio_pcm_dec"

#include "log.h"
#include "aml_pcm_dec_api.h"
#include "aml_audio_stream.h"
#include "audio_hw.h"

#define PCM_MAX_LENGTH (4096*2*2)


int pcm_decoder_init_patch(aml_dec_t **pppcm_dec, audio_format_t format, aml_dec_config_t * dec_config)
{
    struct pcm_dec_t *pcm_dec;
    aml_dec_t  *aml_dec = NULL;
    aml_pcm_config_t *pcm_config = NULL;

    if (dec_config == NULL) {
        ALOGE("PCM config is NULL\n");
        return -1;
    }
    pcm_config = (aml_pcm_config_t *)dec_config;

    if (pcm_config->channel <= 0 || pcm_config->channel > AML_MAX_CHANNELS) {
        ALOGE("PCM config channel is invalid=%d\n", pcm_config->channel);
        return -1;
    }

    if (pcm_config->samplerate <= 0 || pcm_config->samplerate > 192000) {
        ALOGE("PCM config samplerate is invalid=%d\n", pcm_config->samplerate);
        return -1;
    }

    if (pcm_config->bitwidth <= 0 || pcm_config->bitwidth > 32) {
        ALOGE("PCM config bitwidth is invalid=%d\n", pcm_config->bitwidth);
        return -1;
    }

    if (!IS_PCM_FORMAT(pcm_config->pcm_format)) {
        ALOGE("PCM config format is not supported =%d\n", pcm_config->pcm_format);
        return -1;
    }

    pcm_dec = calloc(1, sizeof(struct pcm_dec_t));
    if (pcm_dec == NULL) {
        ALOGE("malloc ddp_dec failed\n");
        return -1;
    }

    aml_dec = &pcm_dec->aml_dec;
    memcpy(&pcm_dec->pcm_config, pcm_config, sizeof(aml_pcm_config_t));
    ALOGD("PCM format=%d samplerate =%d ch=%d bitwidth=%d\n", pcm_config->pcm_format,
          pcm_config->samplerate, pcm_config->channel, pcm_config->bitwidth);


    aml_dec->outbuf = (unsigned char*) malloc(PCM_MAX_LENGTH);
    if (!aml_dec->outbuf) {
        ALOGE("malloc buffer failed\n");
        goto exit;
    }
    aml_dec->outbuf_max_len = PCM_MAX_LENGTH;

    aml_dec->status = 1;
    *pppcm_dec = (aml_dec_t *)pcm_dec;
    return 0;

exit:
    if (aml_dec->outbuf) {
        free(aml_dec->outbuf);
    }
    if (pcm_dec) {
        free(pcm_dec);
    }
    *pppcm_dec = NULL;
    return -1;
}

int pcm_decoder_release_patch(aml_dec_t * aml_dec)
{
    if (aml_dec != NULL) {
        if (aml_dec->outbuf) {
            free(aml_dec->outbuf);
        }

        free(aml_dec);
    }

    return 0;
}

int pcm_decoder_process_patch(aml_dec_t * aml_dec, unsigned char*buffer, int bytes)
{
    struct pcm_dec_t *pcm_dec = NULL;
    aml_pcm_config_t *pcm_config = NULL;

    pcm_dec = (struct pcm_dec_t *)aml_dec;
    pcm_config = &pcm_dec->pcm_config;

    if (aml_dec->outbuf_max_len < bytes) {
        ALOGI("realloc outbuf_max_len  from %zu to %zu\n", aml_dec->outbuf_max_len, bytes);
        aml_dec->outbuf = realloc(aml_dec->outbuf, bytes);
        if (aml_dec->outbuf == NULL) {
            ALOGE("realloc pcm buffer failed size %zu\n", bytes);
            return -1;
        }
        aml_dec->outbuf_max_len = bytes;
        memset(aml_dec->outbuf, 0, bytes);
    }

    /*now we only support bypass PCM data*/

    memcpy(aml_dec->outbuf, buffer, bytes);

    aml_dec->dec_info.output_sr = pcm_config->samplerate;
    aml_dec->dec_info.output_ch = pcm_config->channel;
    aml_dec->dec_info.output_bitwidth = pcm_config->bitwidth;
    aml_dec->outlen_pcm         = bytes;


    return 0;
}



aml_dec_func_t aml_pcm_func = {
    .f_init                 = pcm_decoder_init_patch,
    .f_release              = pcm_decoder_release_patch,
    .f_process              = pcm_decoder_process_patch,
    .f_dynamic_param_set    = NULL,
};
