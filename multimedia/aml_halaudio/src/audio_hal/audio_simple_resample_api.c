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
#include "audio_simple_resample_api.h"
#include "aml_audio_resampler.h"


int simple_resample_open(void **handle, audio_resample_config_t *resample_config)
{
    struct resample_para *resample = NULL;

    if (resample_config->aformat != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("Not support Format =%d \n", resample_config->aformat);
        return -1;
    }

    resample = (struct resample_para *)calloc(1, sizeof(struct resample_para));

    if (resample == NULL) {
        ALOGE("malloc resample_para failed\n");
        return -1;
    }

    resample->channels  = resample_config->channels;
    resample->input_sr  = resample_config->input_sr;
    resample->output_sr = resample_config->output_sr;
    resampler_init(resample);

    *handle = resample;

    return 0;
}

void simple_resample_close(void *handle)
{
    struct resample_para *resample = NULL;
    resample = (struct resample_para *)handle;
    if (resample) {
        free(resample);
    }
    return;
}
int simple_resample_process(void *handle, void * in_buffer, size_t bytes, void * out_buffer, size_t * out_size)
{
    struct resample_para *resample = NULL;
    int out_frame = 0;
    int in_frame = 0;

    if (handle == NULL) {
        ALOGE("simple resample is NULL\n");
        return -1;
    }
    resample = (struct resample_para *)handle;

    in_frame = bytes / (2 * resample->channels); // 2 means 16 bit data
    out_frame = resample_process(resample, in_frame, (int16_t *) in_buffer, (int16_t *) out_buffer);

    * out_size = out_frame * 2 * resample->channels;

    return 0;
}


audio_resample_func_t audio_simple_resample_func = {
    .resample_open                 = simple_resample_open,
    .resample_close                = simple_resample_close,
    .resample_process              = simple_resample_process,
};
