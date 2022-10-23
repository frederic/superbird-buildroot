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

#define LOG_TAG "aml_channel_level"


#include <unistd.h>
#include <math.h>
#include "aml_audio_level.h"
#include "log.h"
#include "aml_audio_log.h"

#define AUDIO_LEVEL_CAL_THRESHOLD 500   // ms


typedef struct level_update_item {
    channel_id_t ch_id;
    int          present;
    float        level;
} level_update_item_t;

typedef struct level_cal_item {
    channel_id_t ch_id;
    int   cal_frames;
    float cal_level;
} level_cal_item_t;

/*this handle will store all the channel level*/
typedef struct level_handle {
    level_update_item_t  level_update[AML_MAX_CHANNELS];
    level_cal_item_t level_cal[AML_MAX_CHANNELS];
} level_handle_t;

struct level_name_pair {
    char name[32];
    channel_id_t ch_id;
};


static struct level_name_pair ch_level_pair[ ] = {
    {"lf_ch_level",  CHANNEL_LEFT_FRONT},
    {"rf_ch_level",  CHANNEL_RIGHT_FRONT},
    {"c_ch_level",   CHANNEL_CENTER},
    {"lfe_ch_level", CHANNEL_LFE},
    {"ls_ch_level",  CHANNEL_LEFT_SURROUND},
    {"rs_ch_level",  CHANNEL_RIGHT_SURROUND},
    {"ltf_ch_level", CHANNEL_LEFT_TOP_FRONT},
    {"rtf_ch_level", CHANNEL_RIGHT_TOP_FRONT},
    {"ltm_ch_level", CHANNEL_LEFT_TOP_MIDDLE},
    {"rtm_ch_level", CHANNEL_RIGHT_TOP_MIDDLE},
    {"le_ch_level",  CHANNEL_LEFT_DOLBY_ENABLE},
    {"re_ch_level",  CHANNEL_RIGHT_DOLBY_ENABLE},
};

void aml_audiolevel_dumpinfo(void * private);


static inline level_update_item_t* find_chlevel_item(level_handle_t *handle , channel_id_t ch_id)
{
    int i = 0;
    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if (handle->level_update[i].ch_id == ch_id) {
            return &handle->level_update[i];
        }

    }

    return NULL;
}

static inline level_cal_item_t* find_cal_ch_item(level_handle_t *handle , channel_id_t ch_id)
{
    int i = 0;
    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if (handle->level_cal[i].ch_id == ch_id) {
            return &handle->level_cal[i];
        }

    }
    return NULL;
}


int aml_audiolevel_init(struct audio_hw_device *dev)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    level_handle_t * level_handle = NULL;
    int i = 0;

    if (adev->level_handle == NULL) {
        level_handle = calloc(1, sizeof(level_handle_t));

        if (level_handle == NULL) {
            return -1;
        }
        adev->level_handle = level_handle;

    }
    level_handle = adev->level_handle;

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        /*this info is used for update to APP*/
        level_handle->level_update[i].ch_id = CHANNEL_BASE + i;
        level_handle->level_update[i].present = 0;
        level_handle->level_update[i].level = 0.0;


        /*this handle will store all the calculated level*/
        level_handle->level_cal[i].cal_frames = 0;
        level_handle->level_cal[i].cal_level  = 0.0;
        level_handle->level_cal[i].ch_id = CHANNEL_BASE + i;

    }

    aml_log_dumpinfo_install(LOG_TAG, aml_audiolevel_dumpinfo, level_handle);


    return 0;
}

int aml_audiolevel_reset(struct audio_hw_device *dev)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    level_handle_t * level_handle = NULL;
    int i = 0;

    level_handle = adev->level_handle;

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        level_handle->level_update[i].ch_id = CHANNEL_BASE + i;
        level_handle->level_update[i].level = 0.0;

        level_handle->level_cal[i].cal_frames = 0;
        level_handle->level_cal[i].cal_level  = 0.0;
        level_handle->level_cal[i].ch_id = CHANNEL_BASE + i;

    }

    return 0;
}


int aml_audiolevel_close(struct audio_hw_device *dev)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;

    aml_log_dumpinfo_remove(LOG_TAG);

    if (adev->level_handle) {
        free(adev->level_handle);
    }
    return 0;
}


int aml_audiolevel_getparam(struct audio_hw_device *dev, const char *keys, char *temp_buf, size_t temp_buf_size)
{
    int ret = 0;
    int i = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    level_handle_t * level_handle = NULL;
    level_update_item_t  * level_item = NULL;

    int item = sizeof(ch_level_pair) / sizeof(struct level_name_pair);

    if (!adev || !keys) {
        ALOGE("Fatal Error adev %p parms %p", adev, keys);
        return -1;
    }

    level_handle = adev->level_handle;

    if (level_handle == NULL) {
        return -1;
    }

    for (i = 0; i < item; i++) {
        if (strstr(keys, ch_level_pair[i].name)) {
            level_item = find_chlevel_item(level_handle, ch_level_pair[i].ch_id);
            if (level_item) {
                snprintf(temp_buf, temp_buf_size, "%s=%f", ch_level_pair[i].name, level_item->level);
                return 0;
            }
        }
    }
    return -1;
}

int aml_audiolevel_cal(struct audio_hw_device *dev, void * in_data, size_t size, aml_data_format_t *format)
{

    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    level_handle_t * level_handle = NULL;
    level_cal_item_t * cal_item = NULL;
    level_update_item_t  * level_item = NULL;
    int nframes = 0;
    int ch = 0;
    int i = 0, j = 0;
    int bitwidth = SAMPLE_16BITS;
    ch = format->ch;
    bitwidth = format->bitwidth;
    if (ch == 0 || bitwidth == 0 || format->sr == 0) {
        return -1;
    }

    level_handle = adev->level_handle;

    nframes  = size / ((bitwidth >> 3) * ch);

    switch (bitwidth) {
    case SAMPLE_8BITS:
        // not support
        return -1;
    case SAMPLE_16BITS: {
        short * data = (short*) in_data;
        for (i = 0 ; i < ch; i++) {
            float temp = 0.0;
            float sample_value = 0.0;
            channel_id_t ch_id = format->channel_info.channel_items[i].ch_id;
            cal_item = find_cal_ch_item(level_handle, ch_id);
            if (cal_item != NULL) {
                for (j = 0; j < nframes; j++) {
                    sample_value = data[j * ch + i] / 32768.0;
                    temp += sample_value * sample_value;
                }
                cal_item->cal_level += temp;
                cal_item->cal_frames += nframes;
            }
        }
    }
    break;
    case SAMPLE_24BITS: /*the 24 bit is store in 32bit*/
    case SAMPLE_32BITS: {
        int * data = (int*) in_data;
        for (i = 0 ; i < ch; i++) {
            float temp = 0.0;
            float sample_value = 0.0;
            channel_id_t ch_id = format->channel_info.channel_items[i].ch_id;
            cal_item = find_cal_ch_item(level_handle, ch_id);
            if (cal_item != NULL) {
                for (j = 0; j < nframes; j++) {
                    sample_value = (data[j * ch + i] >> 16) / 32768.0;
                    temp += sample_value * sample_value;
                }
                cal_item->cal_level += temp;
                cal_item->cal_frames += nframes;
            }
        }

    }
    break;
    default:
        break;
    }

    /*if the calculated frame time is bigger than the threshold, then update it*/
    cal_item = find_cal_ch_item(level_handle, format->channel_info.channel_items[0].ch_id);
    if (cal_item->cal_frames / (format->sr / 1000) >= AUDIO_LEVEL_CAL_THRESHOLD) {
        for (i = 0 ; i < ch; i++) {
            channel_id_t ch_id = format->channel_info.channel_items[i].ch_id;
            cal_item = find_cal_ch_item(level_handle, ch_id);
            level_item = find_chlevel_item(level_handle, ch_id);
            if (cal_item && level_item) {
                level_item->level = sqrt(cal_item->cal_level / cal_item->cal_frames);
                level_item->present = 1;
                //ALOGD("ch =%d id=%d level=%f total=%f total frame=%d\n",i,ch_id,level_item->level, cal_item->cal_level, cal_item->cal_frames);
            }
            if (cal_item) {
                cal_item->cal_frames = 0;
                cal_item->cal_level  = 0.0;
            }
        }
    }

    return 0;

}

void aml_audiolevel_dumpinfo(void * private)
{
    int i = 0;
    channel_id_t ch_id = 0;
    if (private == NULL) {
        return;
    }
    level_handle_t * level_handle = (level_handle_t *)private;
    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        ch_id = level_handle->level_update[i].ch_id;
        if (level_handle->level_update[i].present) {
            ALOGA("ch_id=%d level=%f\n", ch_id, level_handle->level_update[i].level);
        }
    }

    return;
}

