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


#define LOG_TAG "aml_output"
//#define LOG_NDEBUG 0

#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <unistd.h>

#include "aml_output_manager.h"
#include "alsa_manager.h"
#ifdef USE_PULSE_AUDIO
#include "pulse_manager.h"
#endif

#include "standard_alsa_manager.h"
#include "aml_audio_log.h"
#include "aml_audio_ease.h"

#define DELAY_BUF_SIZE_TIME  100    /*the total delay buffer is 100ms*/
#define DELAY_TIME  10              /*the delay time is 20ms*/
#define OUTPUT_FRAMES        512

static struct aml_output_function aml_output_function = {
    .output_open = NULL,
    .output_close = NULL,
    .output_write = NULL,
};

void aml_output_init(void)
{

    ALOGD("Init the output module\n");
#ifndef USE_PULSE_AUDIO
#if 1//def USE_ALSA_PLUGINS
    aml_output_function.output_open  = standard_alsa_output_open;
    aml_output_function.output_close = standard_alsa_output_close;
    aml_output_function.output_write = standard_alsa_output_write;
    aml_output_function.output_getinfo = standard_alsa_output_getinfo;
#else
    aml_output_function.output_open  = aml_alsa_output_open;
    aml_output_function.output_close = aml_alsa_output_close;
    aml_output_function.output_write = aml_alsa_output_write;
#endif
#else
    aml_output_function.output_open  = aml_pa_output_open;
    aml_output_function.output_close = aml_pa_output_close;
    aml_output_function.output_write = aml_pa_output_write;
#endif
    return;
}


int aml_output_open(struct audio_stream_out *stream, aml_stream_config_t * stream_config, aml_device_config_t *device_config)
{
    int ret = -1;
    struct aml_output_handle *handle = NULL;
    output_device_t output_device = 0;
    struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
    int bitwidth = 0;
    ease_setting_t ease_setting;
    ALOGD("Enter %s device=%d\n", __func__, device_config->device);

    if (stream == NULL || stream_config == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }

    if (aml_output_function.output_open == NULL) {
        ALOGE("Output function is NULL\n");
        return -1;
    }



    handle = (struct aml_output_handle*)calloc(1, sizeof(struct aml_output_handle));

    if (handle == NULL) {
        ALOGD("malloc for aml_output_handle failed\n");
        return -1;
    }

    memset((void*)handle, 0, sizeof(struct aml_output_handle));
    memcpy((void*)&handle->stream_config, stream_config, sizeof(aml_stream_config_t));

    handle->device = device_config->device;

    if (device_config->device == AUDIO_DEVICE_OUT_SPEAKER) {
        output_device = PCM_OUTPUT_DEVICE;
    } else if (device_config->device == AUDIO_DEVICE_OUT_SPDIF) {
        output_device = RAW_OUTPUT_DEVICE;
    }

    ret = aml_output_function.output_open(&handle->device_handle, stream_config, device_config);

    if (ret == OUTPUT_ERROR) {
        goto err_open;
    }

    aml_stream->output_handle[output_device] = handle;

    bitwidth = FormatToBit(stream_config->format);

    handle->delay_buf_size = (DELAY_BUF_SIZE_TIME * stream_config->channels * (bitwidth >> 3) * stream_config->rate / 1000 + 1023) >> 10;
    handle->delay_buf_size = handle->delay_buf_size << 10; /*align to 1024*/

    handle->delay_size = (DELAY_TIME * stream_config->channels * (bitwidth >> 3) * stream_config->rate / 1000);

    ring_buffer_init(&handle->delay_ring_buf, handle->delay_buf_size);
    ALOGD("Init delay buffer =0x%x\n", handle->delay_buf_size);
    ret = aml_audio_ease_init(&handle->audio_ease);
    if (ret < 0) {
        ALOGE("aml_audio_ease_init faild\n");
        goto err_open;
    }

    /*set ease volume to 0*/
    ease_setting.ease_type = EaseLinear;
    ease_setting.duration = 0;
    ease_setting.target_volume = 0.0;
    aml_audio_ease_config(handle->audio_ease, &ease_setting);

    /*ease in the audio*/
    ease_setting.ease_type = EaseLinear;
    ease_setting.duration = 200;
    ease_setting.target_volume = 1.0;
    aml_audio_ease_config(handle->audio_ease, &ease_setting);



    ALOGD("Exit %s = %d handle=%p\n", __func__, ret, handle);
    return ret;
err_open:

    ring_buffer_release(&handle->delay_ring_buf);
    if (handle) {
        free(handle);
    }
    aml_stream->output_handle[output_device] = NULL;
    return -1;

}

int aml_output_close(struct audio_stream_out *stream)
{
    int i = 0;
    int write_bytes = 0;
    int buf_bytes = 0;
    int bitwidth = 0;
    int available = 0;
    char * buffer = NULL;
    int ret = -1;
    aml_data_format_t data_format;
    ease_setting_t ease_setting;
    ALOGD("Enter %s \n", __func__);
    struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
    struct aml_output_handle *output_handle = NULL;

    if (stream == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }


    if (aml_output_function.output_close == NULL) {
        ALOGE("Output function is NULL\n");
        return -1;
    }


    for (i = PCM_OUTPUT_DEVICE; i < OUTPUT_DEVICE_CNT; i ++) {
        output_info_t info = {0};
        output_state_t  output_state;
        int delay_frame = 0;
        output_handle = (struct aml_output_handle *)aml_stream->output_handle[i];

        if (output_handle == NULL) {
            continue;
        }

        bitwidth = FormatToBit(output_handle->stream_config.format);
        buf_bytes = OUTPUT_FRAMES * output_handle->stream_config.channels * (bitwidth >> 3);
        /*temp allocate the buffer, free it later*/
        buffer = calloc(1, buf_bytes);

        data_format.bitwidth = bitwidth;
        data_format.ch = output_handle->stream_config.channels;
        data_format.sr = output_handle->stream_config.rate;


        if (aml_output_function.output_getinfo) {
            ret = aml_output_function.output_getinfo(output_handle->device_handle, OUTPUT_INFO_STATUS, &info);
            output_state = info.output_state;
            ret = aml_output_function.output_getinfo(output_handle->device_handle, OUTPUT_INFO_DELAYFRAME, &info);
            delay_frame = info.delay_frame;

            ALOGE("ret =%d state=%d delay frame=%d\n", ret, output_state, delay_frame);

            /*we only do fade out when it is running*/
            if (ret == 0 && output_state == OUTPUT_RUNNING && delay_frame > 0) {

                /*set ease volume to 1.0*/
                ease_setting.ease_type = EaseLinear;
                ease_setting.duration = 0;
                ease_setting.target_volume = 1.0;
                aml_audio_ease_config(output_handle->audio_ease, &ease_setting);


                /*ease in the audio*/
                ease_setting.ease_type = EaseLinear;
                ease_setting.duration = DELAY_TIME;
                ease_setting.target_volume = 0.0;
                aml_audio_ease_config(output_handle->audio_ease, &ease_setting);

                // fade out the delay data
                while (1) {
                    available = get_buffer_read_space(&output_handle->delay_ring_buf);
                    if (available <= 0) {
                        break;
                    }

                    if (available < buf_bytes) {
                        write_bytes = get_buffer_read_space(&output_handle->delay_ring_buf);
                    } else {
                        write_bytes = buf_bytes;
                    }
                    //ALOGE("writes=%d need bytes=%d\n",write_bytes, buf_bytes);
                    ring_buffer_read(&output_handle->delay_ring_buf, buffer, write_bytes);
                    aml_audio_ease_process(output_handle->audio_ease, buffer, write_bytes, &data_format);

                    if (aml_log_get_dumpfile_enable("dump_output")) {
                        FILE *dump_fp = NULL;
                        dump_fp = fopen("/tmp/output.pcm", "a+");
                        if (dump_fp != NULL) {
                            fwrite(buffer, 1, write_bytes, dump_fp);
                            fclose(dump_fp);
                        } else {
                            ALOGW("[Error] Can't write to /tmp/output.pcm");
                        }
                    }
                    ret = aml_output_function.output_write(output_handle->device_handle, (void*)buffer, write_bytes);
                }


            }
        }
        // add some silence data
        if (output_state == OUTPUT_RUNNING && delay_frame > 0)
        {
            /*we will send about 100ms silence data*/
            int silence_frame = output_handle->stream_config.rate / 10; 
            write_bytes = buf_bytes;
            memset(buffer, 0, buf_bytes);
            while (silence_frame > 0) {
                ret = aml_output_function.output_write(output_handle->device_handle, (void*)buffer, write_bytes);
                silence_frame -= OUTPUT_FRAMES;
            }

        }
        free(buffer);

        // we will close all the opened output
        if (output_handle) {
            aml_output_function.output_close(output_handle->device_handle);
            output_handle->device_handle = NULL;
            ring_buffer_release(&output_handle->delay_ring_buf);
            aml_audio_ease_close(output_handle->audio_ease);
            free(output_handle);
            aml_stream->output_handle[i] = NULL;
        }
    }

    ALOGD("Exit %s\n", __func__);

    return 0;
}


int aml_output_write_pcm(struct audio_stream_out *stream, void *buffer, int bytes)
{
    int ret = 0;
    struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
    struct aml_output_handle *output_handle = NULL;
    int write_bytes = 0;
    int need_bytes = 0;
    int avail_size = 0;
    int bitwidth = 0;
    aml_data_format_t data_format;

    if (stream == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }

    if (aml_output_function.output_write == NULL) {
        ALOGE("Output function is NULL\n");
        return -1;
    }

    output_handle = (struct aml_output_handle *)aml_stream->output_handle[PCM_OUTPUT_DEVICE];


    if (get_buffer_write_space(&output_handle->delay_ring_buf) > bytes) {
        ring_buffer_write(&output_handle->delay_ring_buf, buffer , bytes, UNCOVER_WRITE);
    } else {
        ALOGE("Can't write to delay buffer\n");
    }

    bitwidth = FormatToBit(output_handle->stream_config.format);
    need_bytes = OUTPUT_FRAMES * output_handle->stream_config.channels * (bitwidth >> 3);

    data_format.bitwidth = bitwidth;
    data_format.ch = output_handle->stream_config.channels;
    data_format.sr = output_handle->stream_config.rate;


    while (1) {
        avail_size = get_buffer_read_space(&output_handle->delay_ring_buf) - output_handle->delay_size;

        if (avail_size <= 0) {
            break;
        }
        if (avail_size < need_bytes) {
            write_bytes = avail_size;
        } else {
            write_bytes = need_bytes;
        }

        ring_buffer_read(&output_handle->delay_ring_buf, buffer, write_bytes);

        if (aml_log_get_dumpfile_enable("dump_output")) {
            FILE *dump_fp = NULL;
            dump_fp = fopen("/tmp/output.pcm", "a+");
            if (dump_fp != NULL) {
                fwrite(buffer, 1, write_bytes, dump_fp);
                fclose(dump_fp);
            } else {
                ALOGW("[Error] Can't write to /tmp/output.pcm");
            }
        }

        aml_audio_ease_process(output_handle->audio_ease, buffer, write_bytes, &data_format);

        ret = aml_output_function.output_write(output_handle->device_handle, (void*)buffer, write_bytes);
    }

    return (ret >= 0) ? bytes : -1;
}

int aml_output_write_raw(struct audio_stream_out *stream, void *buffer, int bytes)
{
    int ret = -1;
    struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
    struct aml_output_handle *output_handle = NULL;

    if (stream == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }

    if (aml_output_function.output_write == NULL) {
        ALOGE("Output function is NULL\n");
        return -1;
    }


    output_handle = (struct aml_output_handle *)aml_stream->output_handle[RAW_OUTPUT_DEVICE];

    ret = aml_output_function.output_write(output_handle->device_handle, (void*)buffer, bytes);

    return ret;
}



int aml_output_getinfo(struct audio_stream_out *stream, info_type_t type, output_info_t * info)
{
    int ret = -1;
    struct aml_stream_out *aml_stream = (struct aml_stream_out *)stream;
    struct aml_output_handle *output_handle = NULL;

    if (stream == NULL || info == NULL) {
        ALOGE("Input parameter is NULL\n");
        return -1;
    }

    switch (type) {
    case PCMOUTPUT_CONFIG_INFO:
        output_handle = (struct aml_output_handle *)aml_stream->output_handle[PCM_OUTPUT_DEVICE];

        if (output_handle == NULL) {
            ALOGE("output_handle is NULL\n");
            return -1;
        }
        memcpy(&info->config_info, &output_handle->stream_config, sizeof(aml_stream_config_t));
        return 0;

    default:
        return -1;
    }


    return -1;
}


