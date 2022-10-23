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

#define LOG_TAG "aml_channel_vol"


#include <unistd.h>
#include "aml_audio_volume.h"
#include "log.h"
#include "aml_audio_log.h"

#define MASTER_VOLUME_NAME "master_vol"
#define SOURCE_PRE_SCALE "pre_scale"

#define   Clip(acc,min,max) ((acc) > max ? max : ((acc) < min ? min : (acc)))


static struct ch_name_pair ch_vol_pair[ ] = {
    {"lf_ch_vol",  CHANNEL_LEFT_FRONT},
    {"rf_ch_vol",  CHANNEL_RIGHT_FRONT},
    {"c_ch_vol",   CHANNEL_CENTER},
    {"lfe_ch_vol", CHANNEL_LFE},
    {"ls_ch_vol",  CHANNEL_LEFT_SURROUND},
    {"rs_ch_vol",  CHANNEL_RIGHT_SURROUND},
    {"ltf_ch_vol", CHANNEL_LEFT_TOP_FRONT},
    {"rtf_ch_vol", CHANNEL_RIGHT_TOP_FRONT},
    {"ltm_ch_vol", CHANNEL_LEFT_TOP_MIDDLE},
    {"rtm_ch_vol", CHANNEL_RIGHT_TOP_MIDDLE},
    {"le_ch_vol", CHANNEL_LEFT_DOLBY_ENABLE},
    {"re_ch_vol", CHANNEL_RIGHT_DOLBY_ENABLE},
};


static struct ch_name_pair ch_on_pair[ ] = {
    {"lf_ch_on",  CHANNEL_LEFT_FRONT},
    {"rf_ch_on",  CHANNEL_RIGHT_FRONT},
    {"c_ch_on",   CHANNEL_CENTER},
    {"lfe_ch_on", CHANNEL_LFE},
    {"ls_ch_on",  CHANNEL_LEFT_SURROUND},
    {"rs_ch_on",  CHANNEL_RIGHT_SURROUND},
    {"ltf_ch_on", CHANNEL_LEFT_TOP_FRONT},
    {"rtf_ch_on", CHANNEL_RIGHT_TOP_FRONT},
    {"ltm_ch_on", CHANNEL_LEFT_TOP_MIDDLE},
    {"rtm_ch_on", CHANNEL_RIGHT_TOP_MIDDLE},
    {"le_ch_on",  CHANNEL_LEFT_DOLBY_ENABLE},
    {"re_ch_on",  CHANNEL_RIGHT_DOLBY_ENABLE},
};



static void set_channel_vol(volume_info_t *volume_info, channel_id_t ch, float volume)
{
    int i = 0;

    if (volume < 0.0) {
        volume = 0.0;
    }

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if (volume_info->volume_item[i].ch_id == ch) {
            volume_info->volume_item[i].volume = volume;
            break;
        }
    }
    return;
}

static void set_channel_mute(volume_info_t *volume_info, channel_id_t ch, int bOn)
{
    int i = 0;

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if (volume_info->volume_item[i].ch_id == ch) {
            volume_info->volume_item[i].bOn = bOn;
            break;
        }
    }
    return;
}


void aml_audiovolume_dumpinfo(void * private)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) private;
    volume_info_t *volume_info = &adev->volume_info;
    int item = 0;
    int i = 0, j = 0;
    item = sizeof(ch_vol_pair) / sizeof(struct ch_name_pair);

    printf("master volume=%f\n" , volume_info->master_vol);
    for (i = 0; i < item; i++) {
        for (j = 0; j < AML_MAX_CHANNELS; j++) {
            if (ch_vol_pair[i].ch_id == volume_info->volume_item[j].ch_id) {
                ALOGA("ch name=%s volume=%f\n", ch_vol_pair[i].name, volume_info->volume_item[j].volume);
                break;
            }
        }
    }

    item = sizeof(ch_on_pair) / sizeof(struct ch_name_pair);

    for (i = 0; i < item; i++) {
        for (j = 0; j < AML_MAX_CHANNELS; j++) {
            if (ch_on_pair[i].ch_id == volume_info->volume_item[i].ch_id) {
                ALOGA("ch name=%s on=%d\n", ch_on_pair[i].name, volume_info->volume_item[i].bOn);
                break;
            }
        }
    }



}


int aml_volctl_init(struct audio_hw_device *dev)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    volume_info_t *volume_info = &adev->volume_info;

    int i = 0;

    volume_info->master_vol = 1.0;
    volume_info->pre_scale  = 1.0;

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        volume_info->volume_item[i].ch_id = CHANNEL_BASE + i;
        volume_info->volume_item[i].volume = 1.0;
        volume_info->volume_item[i].bOn = 1;
    }
    aml_log_dumpinfo_install(LOG_TAG, aml_audiovolume_dumpinfo, dev);
    return 0;
}

int aml_volctl_setparameters(struct audio_hw_device *dev, struct str_parms *parms)
{
    float volume = 1.0;
    int bOn = 1;
    int ret = 0;
    int i = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    volume_info_t *volume_info = &adev->volume_info;

    int item = sizeof(ch_on_pair) / sizeof(struct ch_name_pair);


    if (!adev || !parms) {
        ALOGE("Fatal Error adev %p parms %p", adev, parms);
        return -1;
    }

    ret = str_parms_get_float(parms, MASTER_VOLUME_NAME, &volume);
    if (ret >= 0) {

        if (volume < 0.0) {
            volume = 0.0;
        }

        volume_info->master_vol = volume;

        return 0;
    }

    ret = str_parms_get_float(parms, SOURCE_PRE_SCALE, &volume);
    if (ret >= 0) {

        if (volume < 0.0) {
            volume = 0.0;
        }

        volume_info->pre_scale = volume;

        return 0;
    }


    for (i = 0; i < item; i++) {

        ret = str_parms_get_float(parms, ch_vol_pair[i].name, &volume);
        if (ret >= 0) {
            ALOGD("set ch=%s volume=%f\n", ch_vol_pair[i].name, volume);
            set_channel_vol(volume_info, ch_vol_pair[i].ch_id, volume);
            return 0;
        }

        ret = str_parms_get_int(parms, ch_on_pair[i].name, &bOn);
        if (ret >= 0) {
            ALOGD("set ch=%s volume=%f\n", ch_on_pair[i].name, bOn);
            set_channel_mute(volume_info, ch_vol_pair[i].ch_id, bOn);
            return 0;
        }
    }

    return -1;
}

static ch_volume_t* volume_find_chinfo(volume_info_t *volume_info , channel_id_t ch_id)
{
    int i = 0;
    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if (volume_info->volume_item[i].ch_id == ch_id) {
            return &volume_info->volume_item[i];
        }

    }
    return NULL;
}

int aml_volctl_process(struct audio_hw_device *dev, void * in_data, size_t size, aml_data_format_t *format)
{

    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    volume_info_t *volume_info = &adev->volume_info;
    int nframes = 0;
    int ch = 0;
    int i = 0, j = 0;
    int bitwidth = SAMPLE_16BITS;
    ch_volume_t * ch_volume = NULL;
    ch = format->ch;
    bitwidth = format->bitwidth;
    if (ch == 0 || bitwidth == 0) {
        return -1;
    }

    nframes  = size / ((bitwidth >> 3) * ch);

    switch (bitwidth) {
    case SAMPLE_8BITS:
        // not support
        break;
    case SAMPLE_16BITS: {
        short * data = (short*) in_data;
        int temp = 0;
        for (i = 0 ; i < ch; i++) {
            ch_volume = volume_find_chinfo(volume_info, format->channel_info.channel_items[i].ch_id);
            if (ch_volume != NULL) {
                for (j = 0; j < nframes; j++) {
                    if (ch_volume->bOn == 0) {
                        data[j * ch + i] = 0;
                    } else {
                        temp = (int)data[j * ch + i] * ch_volume->volume * volume_info->master_vol * volume_info->pre_scale;
                        data[j * ch + i] = Clip(temp, -(1<<15), (1<<15)-1);
                    }
                }
            }
        }
    }
    break;
    case SAMPLE_24BITS:
        // not suppport
        break;
    case SAMPLE_32BITS: {
        int * data = (int*) in_data;
        long long temp = 0;

        for (i = 0 ; i < ch; i++) {
            ch_volume = volume_find_chinfo(volume_info, format->channel_info.channel_items[i].ch_id);

            if (ch_volume != NULL) {
                //ALOGE("ch=%d volume=%f\n",i, ch_volume->volume);
                for (j = 0; j < nframes; j++) {
                    if (ch_volume->bOn == 0) {
                        data[j * ch + i] = 0;
                    } else {
                        temp = (long long)data[j * ch + i] * ch_volume->volume * volume_info->master_vol * volume_info->pre_scale;
                        data[j * ch + i] = Clip(temp, -(1ll<<31), (1ll<<31)-1);
                    }
                }
            }
        }
    }
    break;
    default:
        break;
    }

    return 0;

}


