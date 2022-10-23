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

#include <cutils/log.h>
#include <stdlib.h>
#include <string.h>
#include "aml_audio_stream.h"
#include "audio_android_resample_api.h"
#include "aml_resample_wrap.h"

#define RING_BUF_FRAMES 16384
#define TEMP_BUF_FRAMES 16384
#define RESAMPLE_FRAME  512

static size_t in_read_func(void *ring_buffer, void *buf, size_t size)
{
    int ret = -1;
    ring_buffer_t *ringbuffer = ring_buffer;
    ret = ring_buffer_read(ringbuffer, (unsigned char*)buf, size);
    return ret;
}


int android_resample_open(void **handle, audio_resample_config_t *resample_config)
{
    int ret = -1;
    android_resample_handle_t *resample = NULL;
    int bitwidth = SAMPLE_16BITS;

    if (resample_config->aformat != AUDIO_FORMAT_PCM_16_BIT /*&& resample_config->aformat != AUDIO_FORMAT_PCM_32_BIT*/) {
        ALOGE("Not support Format =%d \n", resample_config->aformat);
        return -1;
    }

    resample = (android_resample_handle_t *)calloc(1, sizeof(android_resample_handle_t));

    if (resample == NULL) {
        ALOGE("malloc resample_para failed\n");
        return -1;
    }

    resample->channels  = resample_config->channels;
    resample->input_sr  = resample_config->input_sr;
    resample->output_sr = resample_config->output_sr;

    if (resample_config->aformat == AUDIO_FORMAT_PCM_16_BIT) {
        bitwidth = SAMPLE_16BITS;
    } else if (resample_config->aformat == AUDIO_FORMAT_PCM_32_BIT) {
        bitwidth = SAMPLE_32BITS;
    } else {
        ALOGE("Not supported aformat=0x%x\n", resample_config->aformat);
        goto exit;
    }

    if (resample->channels > 2) {
        ALOGE("Not supported chanel=0x%x\n", resample->channels);
        goto exit;
    }



    resample->ringbuf_size = resample->channels * (bitwidth >> 3) * RING_BUF_FRAMES;
    ret = ring_buffer_init(&resample->ring_buf, resample->ringbuf_size);

    if (ret < 0) {
        goto exit;
    }

    resample->temp_buf_size = resample->channels * 4 * TEMP_BUF_FRAMES; // 4 means int, it is used for resample out

    resample->temp_buf = malloc(resample->temp_buf_size);

    if (resample->temp_buf == NULL) {

        goto exit;
    }

    ret = android_resample_init(resample, resample->input_sr, resample_config->aformat, resample->channels, in_read_func, &resample->ring_buf);

    if (ret < 0) {
        goto exit;
    }

    *handle = resample;

    return 0;

exit:
    if (resample) {
        ring_buffer_release(&resample->ring_buf);

        if (resample->temp_buf) {
            free(resample->temp_buf);
        }

        free(resample);
        *handle = 0;
    }
    ALOGE("android resample open failed\n");
    return -1;

}

void android_resample_close(void *handle)
{
    android_resample_handle_t *resample = (android_resample_handle_t *)handle;

    if (resample == NULL) {
        ALOGE("android resample is NULL\n");
        return;
    }
    ALOGD("resmaple close\n");
    android_resample_release(handle);

    ring_buffer_release(&resample->ring_buf);

    if (resample->temp_buf) {
        free(resample->temp_buf);
    }

    free(resample);

    return;
}
int android_resample_process(void *handle, void * in_buffer, size_t bytes, void * out_buffer, size_t * out_size)
{
    android_resample_handle_t *resample = NULL;
    int out_frame = 0;
    int in_frame = 0;
    int offset = 0;
    int left_buf_size = 0;
    int ret = -1;
    unsigned int input_sr;
    unsigned int output_sr;
    size_t need_size = 0;

    int resample_size = RESAMPLE_FRAME * 2 * 2;
    if (handle == NULL) {
        ALOGE("simple resample is NULL\n");
        return -1;
    }

    resample = (android_resample_handle_t *)handle;
    input_sr = resample->input_sr;
    output_sr = resample->output_sr;
    need_size = (uint64_t)(resample_size*input_sr + (output_sr - 1))/output_sr;

    if (get_buffer_write_space(&resample->ring_buf) > (int)bytes) {
        ring_buffer_write(&resample->ring_buf, in_buffer, bytes, UNCOVER_WRITE);
    } else {
        ALOGE("Lost data, bytes:%d\n", bytes);
    }

    left_buf_size = * out_size;
    while (left_buf_size >= resample_size) {

        ret = android_resample_read(resample, (char *)out_buffer + offset, resample_size);
        offset += ret;
        left_buf_size -= ret;
        if (ret < resample_size) {
            break;
        }
        /*
           We can't let android resampler to consume all the data in the ring buffer,
           otherwise it will cause noise.
        */
        if (get_buffer_read_space(&resample->ring_buf) < (int)need_size) {
            //ALOGE("avali=%d need=%d\n",get_buffer_read_space(&resample->ring_buf), need_size);
            break;
        }
    }

    *out_size = offset;

    return 0;
}


audio_resample_func_t audio_android_resample_func = {
    .resample_open                 = android_resample_open,
    .resample_close                = android_resample_close,
    .resample_process              = android_resample_process,
};




