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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <tinyalsa/asoundlib.h>
#include <unistd.h>

#include "audio_hw.h"
#include "alsa_manager.h"
#include "audio_hw_utils.h"
#include "alsa_device_parser.h"
#include "aml_audio_stream.h"


#include <cjson/cJSON.h>

// 0 means non block, 1 means block
#define ALSA_BLOCK_MODE  (0)


typedef struct alsa_handle {
    unsigned int card;
    alsa_device_t alsa_device;
    struct pcm_config config;
    struct pcm *pcm;
    int    block_mode;

} alsa_handle_t;



static const struct pcm_config pcm_config_out = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_PLAYBACK_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static const struct pcm_config pcm_config_in = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_CAPTURE_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

/* these name should be same in jason*/
#define ALSA_CONFIG   "ALSA_Config"
#define ALSA_CARD     "card"
#define ALSA_DEVICE   "device"
#define HDMI_IN       "HDMI_IN"
#define SPDIF_IN      "SPDIF_IN"
#define LINE_IN       "LINE_IN"
#define BLUETOOTH_IN  "BT_IN"
#define LOOPBACK_IN   "LOOPBACK_IN"
#define SPEAKER_OUT   "Speaker_Out"
#define SPDIF_OUT     "Spdif_out"

typedef struct alsa_pair {
    unsigned char * name;
    audio_devices_t device;
    int alsa_card;
    int alsa_device;
} alsa_pair_t;

static alsa_pair_t alsapairs[] = {
    {
        .name = HDMI_IN,
        .device = AUDIO_DEVICE_IN_HDMI,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = SPDIF_IN,
        .device = AUDIO_DEVICE_IN_SPDIF,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = LINE_IN,
        .device = AUDIO_DEVICE_IN_LINE,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = BLUETOOTH_IN,
        .device = AUDIO_DEVICE_IN_BLUETOOTH_A2DP,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = LOOPBACK_IN,
        .device = AUDIO_DEVICE_IN_LOOPBACK,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = SPEAKER_OUT,
        .device = AUDIO_DEVICE_OUT_SPEAKER,
        .alsa_card = 0,
        .alsa_device = 0,
    },
    {
        .name = SPDIF_OUT,
        .device = AUDIO_DEVICE_OUT_SPDIF,
        .alsa_card = 0,
        .alsa_device = 0,
    }
};

void aml_alsa_init(cJSON * config)
{
    cJSON *temp;
    alsa_pair_t * alsa_item;
    int i = 0;
    cJSON *alsa_root;
    cJSON * device_config = NULL;
    if (config == NULL) {
        return;
    }
    alsa_root = cJSON_GetObjectItem(config, ALSA_CONFIG);

    for (i = 0 ; i < sizeof(alsapairs) / sizeof(alsa_pair_t); i++) {

        device_config = cJSON_GetObjectItem(alsa_root, alsapairs[i].name);
        if (device_config == NULL) {
            ALOGD("%s is NULL\n", alsapairs[i].name);
            continue;
        }
        temp = cJSON_GetObjectItem(device_config, ALSA_CARD);
        if (temp) {
            alsapairs[i].alsa_card = temp->valueint;
        }
        temp = cJSON_GetObjectItem(device_config, ALSA_DEVICE);
        if (temp) {
            alsapairs[i].alsa_device = temp->valueint;
        }

        printf("Device name=%s card=%d device=%d\n", alsapairs[i].name, alsapairs[i].alsa_card, alsapairs[i].alsa_device);

    }
    return;
}


int aml_alsa_getdevice(audio_devices_t device, int * alsa_card, int *alsa_device)
{
    int ret = -1;
    int i = 0;
    for (i = 0 ; i < sizeof(alsapairs) / sizeof(alsa_pair_t); i++) {
        if (alsapairs[i].device & device) {
            *alsa_card = alsapairs[i].alsa_card;
            *alsa_device = alsapairs[i].alsa_device;
            ret = 0;
            break;
        }
    }

    return ret;
}

int aml_alsa_output_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config)
{
    int ret = -1;
    struct pcm_config *config = NULL;
    int card = 0;
    int device = 0;
    int port = -1;
    struct pcm *pcm = NULL;
    alsa_handle_t * alsa_handle = NULL;


    alsa_handle = (alsa_handle_t *)calloc(1, sizeof(alsa_handle_t));
    if (alsa_handle == NULL) {
        ALOGE("malloc alsa_handle failed\n");
        return -1;
    }

    config = &alsa_handle->config;

    memcpy(config, &pcm_config_out, sizeof(struct pcm_config));

    config->channels = stream_config->channels;
    config->rate     = stream_config->rate;

    if (config->rate == 0 || config->channels == 0) {

        ALOGE("Invalid sampleate=%d channel=%d\n", config->rate == 0, config->channels);
        goto exit;
    }

    if (stream_config->format == AUDIO_FORMAT_PCM_16_BIT) {
        config->format = PCM_FORMAT_S16_LE;
    } else if (stream_config->format == AUDIO_FORMAT_PCM_32_BIT) {
        config->format = PCM_FORMAT_S32_LE;
    } else {
        config->format = PCM_FORMAT_S16_LE;

    }

    config->start_threshold = config->period_size * PLAYBACK_PERIOD_COUNT;
    config->avail_min = 0;

    aml_alsa_getdevice(device_config->device, &card, &device);

    if (device < 0) {
        ALOGE("Wrong device ID\n");
        return -1;
    }

    ALOGD("In pcm open ch=%d rate=%d\n", config->channels, config->rate);
    ALOGI("%s, audio open card(%d), device(%d) \n", __func__, card, device);
    ALOGI("ALSA open configs: channels %d format %d period_count %d period_size %d rate %d \n",
          config->channels, config->format, config->period_count, config->period_size, config->rate);
    ALOGI("ALSA open configs: threshold start %u stop %u silence %u silence_size %d avail_min %d \n",
          config->start_threshold, config->stop_threshold, config->silence_threshold, config->silence_size, config->avail_min);
    pcm = pcm_open(card, device, PCM_OUT, config);
    if (!pcm || !pcm_is_ready(pcm)) {
        ALOGE("%s, pcm %p open [ready %d] failed \n", __func__, pcm, pcm_is_ready(pcm));
        goto exit;
    }

    alsa_handle->card = card;
    alsa_handle->alsa_device = device;
    alsa_handle->pcm = pcm;


    *handle = (void*)alsa_handle;

    return 0;

exit:
    if (alsa_handle) {
        free(alsa_handle);
    }
    *handle = NULL;
    return -1;

}


void aml_alsa_output_close(void *handle)
{
    ALOGI("\n+%s() hanlde %p\n", __func__, handle);
    alsa_handle_t * alsa_handle = NULL;
    struct pcm *pcm = NULL;

    alsa_handle = (alsa_handle_t *)handle;

    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return;
    }


    pcm = alsa_handle->pcm;
    pcm_close(pcm);
    free(alsa_handle);

    ALOGI("-%s()\n\n", __func__);
}

size_t aml_alsa_output_write(void *handle, const void *buffer, size_t bytes)
{
    int ret = -1;
    alsa_handle_t * alsa_handle = NULL;
    struct pcm *pcm = NULL;
    alsa_handle = (alsa_handle_t *)handle;
    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return -1;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return -1;
    }

    //ALOGD("handle=%p pcm=%p\n",alsa_handle,alsa_handle->pcm);

    ret = pcm_write(alsa_handle->pcm, buffer, bytes);

    return ret;
}

int aml_alsa_input_open(void **handle, aml_stream_config_t * stream_config, aml_device_config_t *device_config)
{
    int ret = -1;
    struct pcm_config *config = NULL;
    int card = 0;
    int device = 0;
    int port = -1;
    struct pcm *pcm = NULL;
    alsa_handle_t * alsa_handle = NULL;


    alsa_handle = (alsa_handle_t *)calloc(1, sizeof(alsa_handle_t));
    if (alsa_handle == NULL) {
        ALOGE("malloc alsa_handle failed\n");
        return -1;
    }

    config = &alsa_handle->config;

    memcpy(config, &pcm_config_in, sizeof(struct pcm_config));

    config->channels    = stream_config->channels;
    config->rate        = stream_config->rate;
    //config->period_size = stream_config->framesize;

    if (config->rate == 0 || config->channels == 0) {

        ALOGE("Invalid sampleate=%d channel=%d\n", config->rate, config->channels);
        goto exit;
    }

    /*according to the sample rate to change period size, then
      we can keep the period size to the same time*/
    if (stream_config->rate >= 176400) {
        config->period_size *= 4;
    } else if (stream_config->rate >= 88200) {
        config->period_size *= 2;
    }

    ALOGD("rate=%d channel=%d", stream_config->rate, stream_config->channels);

    if (stream_config->format == AUDIO_FORMAT_PCM_16_BIT) {
        config->format = PCM_FORMAT_S16_LE;
    } else if (stream_config->format == AUDIO_FORMAT_PCM_32_BIT) {
        config->format = PCM_FORMAT_S32_LE;
    } else {
        config->format = PCM_FORMAT_S16_LE;
    }

    aml_alsa_getdevice(device_config->device, &card, &device);

    if (device < 0) {
        ALOGE("Wrong device ID\n");
        return -1;
    }
#ifdef USE_AUDIOSERVICE_S400_SBR
    /*skew setting for input*/
    if (device_config->device & AUDIO_DEVICE_IN_HDMI) {
        int value = 0;
        int inskew = 2;
        value = device << 16 | inskew;
        set_audio_inskew(value);
    } else if (device_config->device & AUDIO_DEVICE_IN_LINE) {
        int value = 0;
        int inskew = 3;
        value = device<< 16 | inskew;
        set_audio_inskew(value);
    }
#endif
#ifdef USE_AUDIOSERVICE_S410_SBR
    /*skew setting for input*/
    if (device_config->device & AUDIO_DEVICE_IN_HDMI) {
        int value = 0;
        int inskew = 3;
        value = device << 16 | inskew;
        set_audio_inskew(value);
    } else if (device_config->device & AUDIO_DEVICE_IN_LINE) {
        int value = 0;
        int inskew = 3;
        value = device<< 16 | inskew;
        set_audio_inskew(value);
    }
#endif

    config->start_threshold = config->period_size * config->period_count;
    config->avail_min = 0;

    ALOGD("In device=%d alsa device=%d\n", device_config->device, device);
    ALOGD("%s period size=%d\n", __func__, config->period_size);

    if (ALSA_BLOCK_MODE == 0) {
        pcm = pcm_open(card, device, PCM_IN | PCM_NONEBLOCK, config);
        alsa_handle->block_mode = 0;
    } else {
        pcm = pcm_open(card, device, PCM_IN, config);
        alsa_handle->block_mode = 1;
    }
    if (!pcm || !pcm_is_ready(pcm)) {
        ALOGE("%s, pcm %p open [ready %d] failed \n", __func__, pcm, pcm_is_ready(pcm));
        goto exit;
    }

    alsa_handle->card = card;
    alsa_handle->alsa_device = device;
    alsa_handle->pcm = pcm;

    *handle = (void*)alsa_handle;

    return 0;

exit:
    if (alsa_handle) {
        free(alsa_handle);
    }
    *handle = NULL;
    return -1;

}


void aml_alsa_input_close(void *handle)
{
    ALOGI("\n+%s() hanlde %p\n", __func__, handle);
    alsa_handle_t * alsa_handle = NULL;
    struct pcm *pcm = NULL;

    alsa_handle = (alsa_handle_t *)handle;

    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return;
    }


    pcm = alsa_handle->pcm;
    pcm_stop(pcm);
    pcm_close(pcm);
    free(alsa_handle);

    ALOGI("-%s()\n\n", __func__);
}
size_t aml_alsa_input_read(void *handle, void *buffer, size_t bytes)
{
    int ret = -1;
    alsa_handle_t * alsa_handle = NULL;
    struct pcm_config *config = NULL;
    size_t  read_bytes = 0;
    size_t frame_size = 0;
    char  *read_buf = NULL;
    struct timespec before;
    struct timespec now;
    int64_t interval_us;


    alsa_handle = (alsa_handle_t *)handle;

    if (alsa_handle == NULL) {
        ALOGE("%s handle is NULL\n", __func__);
        return -1;
    }

    if (alsa_handle->pcm == NULL) {
        ALOGE("%s PCM is NULL\n", __func__);
        return -1;
    }
    //ALOGD("alsa read =%d\n",bytes);
    if (alsa_handle->block_mode == 1) {
        ret = pcm_read(alsa_handle->pcm, buffer, bytes);
    } else {
        read_buf = (char *)buffer;
        config = &alsa_handle->config;
        frame_size = config->channels * pcm_format_to_bits(config->format) / 8;

        clock_gettime(CLOCK_MONOTONIC, &before);
        while (read_bytes < bytes) {
            ret = pcm_read_noblock(alsa_handle->pcm, (unsigned char *)buffer + read_bytes, bytes - read_bytes);
            //ALOGI("pcm read=%d need =%d\n", ret , bytes - read_bytes);
            if (ret >= 0) {
                read_bytes += ret*frame_size;
            } else {
                if (ret != -EAGAIN) {
                    ALOGI("ret != -EAGAIN");
                    return ret;
                } else {
                     usleep(5*1000);
                }
            }
            clock_gettime(CLOCK_MONOTONIC, &now);
            interval_us = (now.tv_sec * 1000000LL + now.tv_nsec / 1000LL) - (before.tv_sec * 1000000LL + before.tv_nsec / 1000LL);

            if (interval_us > 50 * 1000) {
                ALOGI("tried 50 ms but still failed, we return read_bytes=%d\n",read_bytes);
                return read_bytes;
            }
        }
        ret = read_bytes;
    }

    return ret;
}

