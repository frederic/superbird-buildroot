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
#include <string.h>
#include <stdlib.h>
//#include <cutils/properties.h>
#include "aml_audio_stream.h"
#include "audio_hw_utils.h"
#include "audio_simple_resample_api.h"
#include "audio_speex_resample_api.h"


#define RESAMPLE_LENGTH (1024);

static audio_resample_func_t * get_resample_function(resample_type_t resample_type)
{
    switch (resample_type) {
    case AML_AUDIO_SIMPLE_RESAMPLE:
        return &audio_simple_resample_func;
        break;
    case AML_AUDIO_ANDROID_RESAMPLE:
        return NULL;
        break;
    case AML_AUDIO_SPEEX_RESAMPLE:
        return &audio_speex_resample_func;
        break;

    default:
        return NULL;
    }

    return NULL;
}

int aml_audio_resample_init(aml_audio_resample_t ** ppaml_audio_resample, resample_type_t resample_type, audio_resample_config_t *resample_config)
{
    int ret = -1;

    aml_audio_resample_t *aml_audio_resample = NULL;
    audio_resample_func_t * resample_func = NULL;
    int bitwidth = SAMPLE_16BITS;

    if (resample_config == NULL) {
        ALOGE("resample_config is NULL\n");
        return -1;
    }

    if (resample_config->channels == 0 ||
        resample_config->input_sr == 0 ||
        resample_config->output_sr == 0) {
        ALOGE("Invalid resample config\n");
        return -1;
    }


    if (resample_config->aformat == AUDIO_FORMAT_PCM_16_BIT) {
        bitwidth = SAMPLE_16BITS;
    } else if (resample_config->aformat == AUDIO_FORMAT_PCM_32_BIT) {
        bitwidth = SAMPLE_32BITS;
    } else {
        ALOGE("Not supported aformat=0x%x\n", resample_config->aformat);
        return -1;
    }


    aml_audio_resample = (aml_audio_resample_t *)calloc(1, sizeof(aml_audio_resample_t));

    if (aml_audio_resample == NULL) {
        ALOGE("malloc aml_audio_resample failed\n");
        return -1;
    }

    memcpy(&aml_audio_resample->resample_config, resample_config, sizeof(audio_resample_config_t));

    resample_func = get_resample_function(resample_type);

    if (resample_func == NULL) {
        ALOGE("resample_func is NULL\n");
        goto exit;
    }

    aml_audio_resample->resample_type = resample_type;

    aml_audio_resample->resample_rate = (float)resample_config->output_sr / (float)resample_config->input_sr;

    aml_audio_resample->frame_bytes = (bitwidth >> 3) * resample_config->channels;

    aml_audio_resample->resample_buffer_size = aml_audio_resample->frame_bytes * RESAMPLE_LENGTH;

    aml_audio_resample->resample_buffer = calloc(1, aml_audio_resample->resample_buffer_size);

    if (aml_audio_resample->resample_buffer == NULL) {
        ALOGE("resample_buffer is NULL\n");
        goto exit;
    }


    ret = resample_func->resample_open(&aml_audio_resample->resample_handle, &aml_audio_resample->resample_config);
    if (ret < 0) {
        ALOGE("resample_open failed\n");
        goto exit;

    }

    * ppaml_audio_resample = aml_audio_resample;

    return 0;

exit:

    if (aml_audio_resample->resample_buffer) {
        free(aml_audio_resample->resample_buffer);
        aml_audio_resample->resample_buffer = NULL;
    }

    if (aml_audio_resample) {
        free(aml_audio_resample);
    }
    * ppaml_audio_resample = NULL;
    return -1;

}

int aml_audio_resample_close(aml_audio_resample_t * aml_audio_resample)
{

    audio_resample_func_t * resample_func = NULL;

    if (aml_audio_resample == NULL) {
        ALOGE("resample_handle is NULL\n");
        return -1;
    }

    resample_func = get_resample_function(aml_audio_resample->resample_type);
    if (resample_func == NULL) {
        ALOGE("resample_func is NULL\n");
    }

    if (resample_func) {
        resample_func->resample_close(aml_audio_resample->resample_handle);
    }

    if (aml_audio_resample->resample_buffer) {
        free(aml_audio_resample->resample_buffer);
        aml_audio_resample->resample_buffer = NULL;
    }

    free(aml_audio_resample);

    return 0;
}

int aml_audio_resample_process(aml_audio_resample_t * aml_audio_resample, void * in_data, size_t size)
{
    size_t out_size = 0;
    int ret = -1;
    unsigned int frame_bytes = 0;
    audio_resample_func_t * resample_func = NULL;

    if (aml_audio_resample == NULL) {
        ALOGE("resample_handle is NULL\n");
        return -1;
    }

    frame_bytes = aml_audio_resample->frame_bytes;

    out_size = size * aml_audio_resample->resample_rate * 2 ; /*we make it for slightly larger*/

    if (out_size > aml_audio_resample->resample_buffer_size) {
        int new_buf_size = out_size;
        aml_audio_resample->resample_buffer = realloc(aml_audio_resample->resample_buffer, new_buf_size);
        if (aml_audio_resample->resample_buffer == NULL) {
            ALOGE("realloc resample_buffer is failed\n");
            return -1;
        }
        ALOGD("realloc resample_buffer size from %d to %d\n", aml_audio_resample->resample_buffer_size, new_buf_size);
        aml_audio_resample->resample_buffer_size = new_buf_size;
    }

    resample_func = get_resample_function(aml_audio_resample->resample_type);
    if (resample_func == NULL) {
        ALOGE("resample_func is NULL\n");
        return -1;
    }

    out_size = aml_audio_resample->resample_buffer_size;
    memset(aml_audio_resample->resample_buffer, 0, out_size);
    ret = resample_func->resample_process(aml_audio_resample->resample_handle, in_data, size, aml_audio_resample->resample_buffer, &out_size);
    if (ret < 0) {
        ALOGE("resmaple error=%d\n", ret);
        aml_audio_resample->resample_size = 0;
        return ret;
    }

    if (out_size > aml_audio_resample->resample_buffer_size) {
        ALOGE("output size=%d , buf size=%d\n", out_size, aml_audio_resample->resample_buffer_size);
        return -1;
    }

    aml_audio_resample->resample_size = out_size;
    aml_audio_resample->total_in += size;
    aml_audio_resample->total_out += out_size;
    //ALOGE("total rate=%f\n",(float)aml_audio_resample->total_out/(float)aml_audio_resample->total_in);

#if 0
        if (getprop_bool("media.audiohal.resample")) {
            FILE *dump_fp = NULL;
            dump_fp = fopen("/data/audio_hal/resamplein.pcm", "a+");
            if (dump_fp != NULL) {
                fwrite(in_data, size, 1, dump_fp);
                fclose(dump_fp);
            } else {
                ALOGW("[Error] Can't write to /data/audio_hal/resamplein.pcm");
            }

            dump_fp = fopen("/data/audio_hal/resampleout.pcm", "a+");
            if (dump_fp != NULL) {
                fwrite(aml_audio_resample->resample_buffer, aml_audio_resample->resample_size, 1, dump_fp);
                fclose(dump_fp);
            } else {
                ALOGW("[Error] Can't write to /data/audio_hal/resampleout.pcm");
            }


        }
#endif

    return 0;
}


