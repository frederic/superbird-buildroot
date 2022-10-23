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

#define LOG_TAG "aml_audio_resample"

#include "log.h"
#include <stdlib.h>
#include "aml_audio_stream.h"
#include "speex_resampler.h"
#include "audio_speex_resample_api.h"


struct rate_src {
    int quality;
    int in_sr;
    int out_sr;
    unsigned int channels;
    SpeexResamplerState *st;
};

//FILE * speex_file = NULL;
int speex_resample_open(void **handle, audio_resample_config_t *resample_config)
{
    struct rate_src *resample = NULL;
    int err;

    if (resample_config->aformat != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("Not support Format =%d \n", resample_config->aformat);
        return -1;
    }

    resample = (struct rate_src *)calloc(1, sizeof(struct rate_src));

    if (resample == NULL) {
        ALOGE("malloc resample_para failed\n");
        return -1;
    }

    resample->channels  = resample_config->channels;
    resample->in_sr     = resample_config->input_sr;
    resample->out_sr    = resample_config->output_sr;
    resample->quality   = 5;
    resample->st = speex_resampler_init(resample->channels, resample_config->input_sr, resample_config->output_sr, resample->quality, &err);
    ALOGI("st=%p channel=%d in=%d out=%d err=%d", resample->st, resample->channels, resample_config->input_sr, resample_config->output_sr, err);
    if (resample->st == NULL) {
        free(resample);
        return -1;
    }

    *handle = resample;
    //speex_file = fopen("./speex_out.pcm", "w");

    return 0;
}

void speex_resample_close(void *handle)
{
    struct rate_src *resample = NULL;
    resample = (struct rate_src *)handle;
    if (resample) {
        if (resample->st) {
            speex_resampler_destroy(resample->st);
            resample->st = NULL;
        }
        free(resample);
    }
    //fclose(speex_file);
    return;
}
int speex_resample_process(void *handle, void * in_buffer, size_t bytes, void * out_buffer, size_t * out_size)
{
    struct rate_src *resample = NULL;
    int out_frame = 0;
    int in_frame = 0;

    if (handle == NULL) {
        ALOGE("simple resample is NULL\n");
        return -1;
    }
    resample = (struct rate_src *)handle;
    in_frame = bytes / (2 * resample->channels); // 2 means 16 bit data
    out_frame = (int)((float)resample->out_sr / resample->in_sr * in_frame);
    speex_resampler_process_interleaved_int(resample->st, (int16_t *) in_buffer, &in_frame, out_buffer, &out_frame);

    *out_size = out_frame * 2 * resample->channels;

    return 0;
}


audio_resample_func_t audio_speex_resample_func = {
    .resample_open                 = speex_resample_open,
    .resample_close                = speex_resample_close,
    .resample_process              = speex_resample_process,
};


