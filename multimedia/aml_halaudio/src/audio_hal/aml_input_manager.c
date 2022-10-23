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


#define LOG_TAG "aml_input"
//#define LOG_NDEBUG 0

#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <unistd.h>

#include "aml_input_manager.h"
#include "alsa_manager.h"

static struct aml_input_function aml_input_function = {
    .input_open = NULL,
    .input_close = NULL,
    .input_read = NULL,
};

void aml_input_init(void)
{

    ALOGD("Init the input module\n");
#if 1//ndef USE_PULSE_AUDIO
    aml_input_function.input_open  = aml_alsa_input_open;
    aml_input_function.input_close = aml_alsa_input_close;
    aml_input_function.input_read = aml_alsa_input_read;
#else
    aml_input_function.input_open  = aml_pa_input_open;
    aml_input_function.input_close = aml_pa_input_close;
    aml_input_function.input_read  = aml_pa_input_write;

#endif
    return;
}


int aml_input_open(struct audio_stream_in *stream, aml_stream_config_t * stream_config, aml_device_config_t *device_config)
{
    int ret = -1;
    struct aml_input_handle *handle = NULL;
    input_device_t input_device = 0;
    struct aml_stream_in *aml_stream = (struct aml_stream_in *)stream;
    ALOGD("Enter %s device=%d\n", __func__, device_config->device);

    if (stream == NULL || stream_config == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }

    if (aml_input_function.input_open == NULL) {
        ALOGE("input function is NULL\n");
        return -1;
    }



    handle = (struct aml_input_handle*)calloc(1, sizeof(struct aml_input_handle));

    if (handle == NULL) {
        ALOGD("malloc for aml_input_handle failed\n");
        return -1;
    }

    memset((void*)handle, 0, sizeof(struct aml_input_handle));
    memcpy((void*)&handle->stream_config, stream_config, sizeof(aml_stream_config_t));


    input_device = PCM_INPUT_DEVICE;
    handle->device = device_config->device;


    ret = aml_input_function.input_open(&handle->device_handle, stream_config, device_config);

    if (ret == input_ERROR) {
        free(handle);
        return -1;
    }

    aml_stream->input_handle[input_device] = handle;


    ALOGD("Exit %s = %d\n", __func__, ret);
    return ret;
}

int aml_input_close(struct audio_stream_in *stream)
{
    int i = 0;
    ALOGD("Enter %s \n", __func__);
    struct aml_stream_in *aml_stream = (struct aml_stream_in *)stream;
    struct aml_input_handle *input_handle = NULL;

    if (stream == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }


    if (aml_input_function.input_close == NULL) {
        ALOGE("input function is NULL\n");
        return -1;
    }


    for (i = PCM_INPUT_DEVICE; i < INPUT_DEVICE_CNT; i ++) {
        input_handle = (struct aml_input_handle *)aml_stream->input_handle[i];
        // we will close all the opened input
        if (input_handle) {
            aml_input_function.input_close(input_handle->device_handle);
            input_handle->device_handle = NULL;
            free(input_handle);
            aml_stream->input_handle[i] = NULL;
        }
    }

    ALOGD("Exit %s\n", __func__);

    return 0;
}


int aml_input_read(struct audio_stream_in *stream, const void *buffer, int bytes)
{
    int ret = -1;
    struct aml_stream_in *aml_stream = (struct aml_stream_in *)stream;
    struct aml_input_handle *input_handle = NULL;

    if (stream == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }

    if (aml_input_function.input_read == NULL) {
        ALOGE("input function is NULL\n");
        return -1;
    }

    //ALOGD("read pcm bytes=%d\n",bytes);
    input_handle = (struct aml_input_handle *)aml_stream->input_handle[PCM_INPUT_DEVICE];

    ret = aml_input_function.input_read(input_handle->device_handle, (void*)buffer, bytes);

    return ret;
}



