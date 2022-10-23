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

#define LOG_TAG "standard_alsa"

#include <alsa/asoundlib.h>
#include <unistd.h>
#include "standard_alsa_manager.h"
#include "log.h"

#define PCM_DEVICE_DEFAULT      "halaudio"
#define PCM_DEVICE_2CH         "2ch_48K"//"2to8"
#define PCM_DEVICE_8CH_48K     "8ch_48K"//"8to8"
#define PCM_DEVICE_8CH_44K     "8ch_44K"//"8to8"
#define PCM_DEVICE_8CH_DIRECT  "8to8"



#define PERIOD_SIZE         512
#define PERIOD_NUM          4
#define DEFAULT_FRAME_SIZE  512

typedef struct alsa_param {
    snd_pcm_t *handle;
    snd_pcm_format_t format;
    size_t bitwidth;
    int buffer_size;
    unsigned int channels;
    unsigned int rate;
    unsigned int framesize;
    unsigned int start_threshold_coef;

} alsa_param_t;


static int set_alsa_params(alsa_param_t *alsa_params)
{
    int err;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t bufsize;
    unsigned int rate;
    snd_pcm_uframes_t period_size = PERIOD_SIZE;
    snd_pcm_uframes_t start_threshold, stop_threshold;
    unsigned int rate_coefficient = 1;
    unsigned int framesize_coefficient = 1;
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    if (alsa_params->rate > 96000) {
        rate_coefficient = 4;
    } else if (alsa_params->rate > 48000) {
        rate_coefficient = 2;
    } else {
        rate_coefficient = 1;
    }
    /*README:
     * framesize_coefficient means
     * if <= 512frame/48kHz or 1024frame/96kHz or 2048frame/192kHz
     * value it as 1
     * if <= 1024frame/48kHz or 2048frame/96kHz or 4096frame/192kHz
     * value it as 2
     * and so on
     */
    framesize_coefficient = ((alsa_params->framesize/rate_coefficient) > DEFAULT_FRAME_SIZE) ? 2 : 1;
    period_size = period_size * rate_coefficient * framesize_coefficient;
    ALOGD("rate_coefficient %d framesize_coefficient %d\n", rate_coefficient, framesize_coefficient);
    bufsize = PERIOD_NUM * period_size;

    ALOGD("set period=%d bufsize=%d rate=%d\n", period_size, bufsize, alsa_params->rate);
    err = snd_pcm_hw_params_any(alsa_params->handle, hwparams);
    if (err < 0) {
        ALOGE("Broken configuration for this PCM: no configurations available: %s", snd_strerror(err));
        return err;
    }

    err = snd_pcm_hw_params_set_access(alsa_params->handle, hwparams,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        ALOGE("Access type not available");
        return err;
    }

    err = snd_pcm_hw_params_set_format(alsa_params->handle, hwparams, alsa_params->format);
    if (err < 0) {
        ALOGE("Sample format non available");
        return err;
    }

    err = snd_pcm_hw_params_set_channels(alsa_params->handle, hwparams, alsa_params->channels);


    rate = alsa_params->rate;
    err = snd_pcm_hw_params_set_rate_near(alsa_params->handle, hwparams, &alsa_params->rate, 0);

    alsa_params->bitwidth = snd_pcm_format_physical_width(alsa_params->format);

    err = snd_pcm_hw_params_set_period_size_near(alsa_params->handle, hwparams, &period_size, NULL);
    ALOGD("period_size=%d\n", period_size);
    if (err < 0) {
        ALOGE("Unable to set period size \n");
        return err;
    }

    err = snd_pcm_hw_params_set_buffer_size_near(alsa_params->handle, hwparams, &bufsize);
    ALOGD("bufsize=%d\n", bufsize);
    if (err < 0) {
        ALOGE("Unable to set	buffer	size \n");
        return err;
    }

    err = snd_pcm_hw_params(alsa_params->handle, hwparams);
    if (err < 0) {
        ALOGE("Unable to install hw params: %s\n", snd_strerror(err));
        return err;
    }

    err = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    if (err < 0) {
        ALOGE("Unable to get buffersize \n");
        return err;
    }

    alsa_params->buffer_size = bufsize * (alsa_params->bitwidth >> 3);


    err = snd_pcm_sw_params_current(alsa_params->handle, swparams);
    if (err < 0) {
        ALOGE("Unable to get sw-parameters\n");
        return err;
    }

    err = snd_pcm_sw_params_set_start_threshold(alsa_params->handle, swparams, bufsize / alsa_params->start_threshold_coef);
    if (err < 0) {
        ALOGE("Unable to set start threshold \n");
        return err;
    }
#if 0

    err = snd_pcm_sw_params_set_stop_threshold(alsa_params->handle, swparams, bufsize);
    if (err < 0) {
        ALOGD("Unable to set stop threshold \n");
        return err;
    }

    ALOGD("buffer size=%d\n", bufsize);
    snd_pcm_sw_params_set_silence_threshold(alsa_params->handle, swparams, 128);

    if (err < 0) {
        ALOGE("Unable to set silence size \n");
        return err;
    }

    err = snd_pcm_sw_params_set_silence_size(alsa_params->handle, swparams, 128);
    if (err < 0) {
        ALOGE("Unable to set silence size \n");
        return err;
    }
#endif
    err = snd_pcm_sw_params(alsa_params->handle, swparams);
    if (err < 0) {
        ALOGE("[%s::%d]--[alsa sw set params error: %s]\n", __FUNCTION__, __LINE__, snd_strerror(err));
        return err;
    }

    return 0;
}



int standard_alsa_output_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config)
{
    int ret = -1;
    alsa_param_t *alsa_param;
    int block = 1;
    char * device_name = NULL;
    alsa_param = (alsa_param_t *)calloc(1, sizeof(alsa_param_t));
    if (alsa_param == NULL) {
        ALOGE("malloc alsa_param failed\n");
        *handle = NULL;
        return -1;
    }

    if (stream_config->channels == 0 || stream_config->rate == 0) {
        ALOGE("Wrong Input parameter ch=%d rate=%d\n",stream_config->channels,stream_config->rate);
        return -1;
    }

    if (stream_config->format == AUDIO_FORMAT_PCM_16_BIT) {
        alsa_param->format = SND_PCM_FORMAT_S16_LE;
    } else if (stream_config->format == AUDIO_FORMAT_PCM_32_BIT) {
        alsa_param->format = SND_PCM_FORMAT_S32_LE;
    } else {
        alsa_param->format = SND_PCM_FORMAT_S16_LE;
    }

    alsa_param->channels = stream_config->channels;
    alsa_param->rate     = stream_config->rate;
    alsa_param->framesize = stream_config->framesize;
    alsa_param->start_threshold_coef = (stream_config->start_threshold_coef) ? (stream_config->start_threshold_coef) : 1;
    if (alsa_param->channels == 8) {
        // due to resample performance issue, we currently only support bypass
        if (alsa_param->rate == 176400 || alsa_param->rate == 88200) {
            device_name = PCM_DEVICE_8CH_44K;
        } else if (alsa_param->rate == 192000 || alsa_param->rate == 96000) {
            device_name = PCM_DEVICE_8CH_48K;
        } else {
            device_name = PCM_DEVICE_DEFAULT;
        }
    } else {
        device_name = PCM_DEVICE_DEFAULT;
    }
    ALOGD("Open ALSA device=%s rate=%d ch=%d format=%d in format=%d\n", device_name, alsa_param->rate, alsa_param->channels, alsa_param->format, stream_config->format);
    ret = snd_pcm_open(&alsa_param->handle, device_name, SND_PCM_STREAM_PLAYBACK, block ? 0 : SND_PCM_NONBLOCK);

    if (ret < 0) {
        ALOGE("[%s::%d]--[audio open error: %s]\n", __FUNCTION__, __LINE__, snd_strerror(ret));
        return -1;
    } else {
        ALOGE("[%s::%d]--[audio open(snd_pcm_open) successfully]\n", __FUNCTION__, __LINE__);
    }

    ret  = set_alsa_params(alsa_param);

    if (ret < 0) {
        ALOGE("set_params error\n");
        snd_pcm_close(alsa_param->handle);
        free(alsa_param);
        *handle = NULL;
        return -1;
    }


    *handle = alsa_param;
    return ret;
}

void standard_alsa_output_close(void *handle)
{
    int ret = -1;
    alsa_param_t *alsa_param = (alsa_param_t *)handle;
    snd_pcm_sframes_t delay = 0;

    if (handle == NULL) {
        return;
    }

    snd_pcm_delay(alsa_param->handle, &delay);
    ALOGE("close alsa delay=%d\n", delay);
    //snd_pcm_drain(alsa_param->handle);
    //snd_pcm_drop(alsa_param->handle);
    usleep(100*1000);
    ret = snd_pcm_close(alsa_param->handle);
    if (ret < 0) {
        ALOGE("[%s::%d]--[audio close error: %s]\n", __FUNCTION__, __LINE__, snd_strerror(ret));
    }

    free(alsa_param);

    return;
}

size_t standard_alsa_output_write(void *handle, const void *buffer, size_t bytes)
{
    snd_pcm_sframes_t ret = -1;
    alsa_param_t *alsa_param = (alsa_param_t *)handle;
    snd_pcm_uframes_t frames = 0;
    snd_pcm_uframes_t count = 0;
    size_t result = 0;
    unsigned char * data = NULL;
    int bytes_per_frame = 0;
    struct timespec before;
    struct timespec now;
    int64_t interval_us;

    if (handle == NULL) {
        return -1;
    }

    bytes_per_frame = alsa_param->channels * (alsa_param->bitwidth >> 3);
    frames = bytes / (bytes_per_frame);

    data = (unsigned char *)buffer;
    count = frames;

    clock_gettime(CLOCK_MONOTONIC, &before);

    while (count > 0) {
        ret = snd_pcm_writei(alsa_param->handle, buffer, count);
        if (ret  < 0) {
            ALOGE("[%s::%d]--[audio write error: %s]\n", __FUNCTION__, __LINE__, snd_strerror(ret));
        }
        clock_gettime(CLOCK_MONOTONIC, &now);

        interval_us = (now.tv_sec * 1000000LL + now.tv_nsec / 1000LL) - (before.tv_sec * 1000000LL + before.tv_nsec / 1000LL);

        if (interval_us > 1 * 1000 * 1000) {
            ALOGE("tried 1s but still failed, we return\n");
            break;
        }


        if (ret == -EINTR) {
            ret = 0;
        }
        if (ret == -ESTRPIPE) {
            while ((ret = snd_pcm_resume(alsa_param->handle)) == -EAGAIN) {
                usleep(1000);
            }
        }

        if (ret < 0) {
            ALOGE("xun in =%d -EPIPE=%d\n", ret, -EPIPE);
            if ((ret = snd_pcm_prepare(alsa_param->handle)) < 0) {
                ALOGE("snd_pcm_prepare error=%s \n", snd_strerror(ret));
                result = 0;
                return result;
            }
        }

        if (ret > 0) {
            result += ret;
            count -= ret;
            data += ret * bytes_per_frame;
        }
    }




    //ALOGD("Write frames=%d return =%d\n",frames, ret);

    if (result != frames) {
        ALOGE(" write error =%d frames=%d\n", ret, frames);
        return -1;
    }

    return ret;
}
static output_state_t output_state_convert(snd_pcm_state_t alsa_state)
{
    output_state_t state = OUTPUT_IDLE;
    switch (alsa_state) {
    case SND_PCM_STATE_OPEN: {
        state = OUTPUT_IDLE;
        break;
    }
    case SND_PCM_STATE_SETUP: {
        state = OUTPUT_OPENED;
        break;
    }
    case SND_PCM_STATE_PREPARED: {
        state = OUTPUT_STARTED;
        break;
    }
    case SND_PCM_STATE_RUNNING: {
        state = OUTPUT_RUNNING;
        break;
    }
    case SND_PCM_STATE_PAUSED: {
        state = OUTPUT_PAUSED;
        break;
    }
    case SND_PCM_STATE_XRUN:
    case SND_PCM_STATE_DRAINING: {
        state = OUTPUT_STOPED;
        break;
    }
    case SND_PCM_STATE_SUSPENDED:
    case SND_PCM_STATE_DISCONNECTED: {
        state = OUTPUT_CLOSED;
        break;
    }
    default:
        state = OUTPUT_IDLE;
    }
    return state;
}
int standard_alsa_output_getinfo(void *handle, info_type_t type, output_info_t * info)
{
    int ret = -1;
    alsa_param_t *alsa_param = (alsa_param_t *)handle;
    snd_pcm_sframes_t delay = 0;

    if (handle == NULL) {
        return -1;
    }
    switch (type) {
    case OUTPUT_INFO_STATUS: {
        output_state_t state;
        snd_pcm_status_t *status;
        snd_pcm_status_alloca(&status);
        if ((ret = snd_pcm_status(alsa_param->handle, status)) < 0) {
            return -1;
        }
        state = output_state_convert(snd_pcm_status_get_state(status));
        info->output_state = state;
        return 0;
    }
    case OUTPUT_INFO_DELAYFRAME: {
        snd_pcm_sframes_t delay = 0;
        snd_pcm_delay(alsa_param->handle, &delay);
        info->delay_frame = delay;
        return 0;
    }
    default:
        return -1;
    }
    return -1;
}


