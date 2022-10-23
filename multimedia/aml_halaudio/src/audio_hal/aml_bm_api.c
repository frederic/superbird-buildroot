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

#define LOG_TAG "aml_audio_bm"
#include <string.h>

#include "log.h"
#include "aml_audio_stream.h"
#include "aml_bm_api.h"
#include "lib_bassmanagement.h"
#if 1 //ndef ANDROID
#include "aml_audio_log.h"

#define BGO_COEF_NAME "bgo_coef"
#define LFE_POST_SCALE   "lfe_post_scale"

#define   Clip(acc,min,max) ((acc) > max ? max : ((acc) < min ? min : (acc)))

static struct ch_name_pair ch_coef_pair[ ] = {
    {"lf_ch_coef",  CHANNEL_LEFT_FRONT},
    {"rf_ch_coef",  CHANNEL_RIGHT_FRONT},
    {"c_ch_coef",   CHANNEL_CENTER},
    {"lfe_ch_coef", CHANNEL_LFE},
    {"ls_ch_coef",  CHANNEL_LEFT_SURROUND},
    {"rs_ch_coef",  CHANNEL_RIGHT_SURROUND},
    {"ltf_ch_coef", CHANNEL_LEFT_TOP_FRONT},
    {"rtf_ch_coef", CHANNEL_RIGHT_TOP_FRONT},
    {"ltm_ch_coef", CHANNEL_LEFT_TOP_MIDDLE},
    {"rtm_ch_coef", CHANNEL_RIGHT_TOP_MIDDLE},
    {"le_ch_coef",  CHANNEL_LEFT_DOLBY_ENABLE},
    {"re_ch_coef",  CHANNEL_RIGHT_DOLBY_ENABLE},
};

static void set_channel_coef(ch_coef_info_t *ch_coef_info, channel_id_t ch, float coef)
{
    int i = 0;
    if (coef > 1.0) {
        coef = 1.0;
    }

    if (coef < 0.0) {
        coef = 0.0;
    }

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if (ch_coef_info->coef_item[i].ch_id == ch) {
            ch_coef_info->coef_item[i].coef = coef;
            break;
        }
    }
    return;
}

static void set_coef_table(ch_coef_info_t *ch_coef_info, channel_info_t * channel_info, float * coef_table)
{
    int i = 0, j = 0;

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        for (j = 0; j < AML_MAX_CHANNELS; j++) {
            //ALOGI("coef ch_id=%d channel id=%d\n",ch_coef_info->coef_item[i].ch_id,channel_info->channel_items[j].ch_id);
            if (ch_coef_info->coef_item[i].ch_id == channel_info->channel_items[j].ch_id) {
                //ALOGI("id=%d present=%d order=%d\n",channel_info->channel_items[j].ch_id,channel_info->channel_items[j].present, channel_info->channel_items[j].order);
                if (channel_info->channel_items[j].present && (channel_info->channel_items[j].order != -1)) {
                    coef_table[channel_info->channel_items[j].order] = ch_coef_info->coef_item[i].coef;
                }
            }
        }
    }


}

static void aml_bm_dumpinfo(void * private)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) private;
    ch_coef_info_t *ch_coef_info = &adev->ch_coef_info;
    int item = 0;
    int i = 0, j = 0;
    item = sizeof(ch_coef_pair) / sizeof(struct ch_name_pair);

    ALOGA("bgo_coef =%f \n", ch_coef_info->bgo_coef);
    ALOGA("lfe_post_coef =%f \n", ch_coef_info->lfe_post_scale);

    for (i = 0; i < item; i++) {
        for (j = 0; j < AML_MAX_CHANNELS; j++) {
            if (ch_coef_pair[i].ch_id == ch_coef_info->coef_item[j].ch_id) {
                ALOGA("ch name=%s bm coef=%f\n", ch_coef_pair[i].name, ch_coef_info->coef_item[j].coef);
                break;
            }
        }
    }
}


int aml_bm_set(struct aml_audio_device *adev, int val)
{
    int bm_init_param = 0;
    int i = 0;

    if (!adev) {
        ALOGE("adev %p", adev);
        return 1;
    }

    adev->lowerpass_corner = val;

    if (adev->lowerpass_corner == 0) {
        adev->bm_enable = false;
    } else {
        if (adev->lowerpass_corner < BM_MIN_CORNER_FREQUENCY) {
            bm_init_param = 0;
        } else if (adev->lowerpass_corner > BM_MAX_CORNER_FREQUENCY) {
            bm_init_param = (BM_MAX_CORNER_FREQUENCY - BM_MIN_CORNER_FREQUENCY) / BM_STEP_LEN_CORNER_FREQ;
        } else {
            bm_init_param = (adev->lowerpass_corner - BM_MIN_CORNER_FREQUENCY) / BM_STEP_LEN_CORNER_FREQ;
        }
        adev->bm_enable = !aml_bass_management_init(bm_init_param);
    }
    ALOGE("lowerpass_corner %d HZ bm_enable %d init params %d\n",
          adev->lowerpass_corner, adev->bm_enable, bm_init_param);
    return 0;
}


int aml_bm_init(struct audio_hw_device *dev)
{
    int i = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *)dev;
    if (adev->bm_init == 0) {
        ch_coef_info_t *ch_coef_info = &adev->ch_coef_info;
        ch_coef_info->bgo_coef = 0.0;
        ch_coef_info->lfe_post_scale = 1.0;
        for (i = 0; i < AML_MAX_CHANNELS; i++) {
            ch_coef_info->coef_item[i].ch_id = CHANNEL_BASE + i;
            ch_coef_info->coef_item[i].coef  = 1.0;;
        }
        adev->bm_init = 1;
        aml_log_dumpinfo_install(LOG_TAG, aml_bm_dumpinfo, adev);
    }
    return 0;
}

int aml_bm_close(struct audio_hw_device *dev)
{
    aml_log_dumpinfo_remove(LOG_TAG);
    return 0;
}


int aml_bm_setparameters(struct audio_hw_device *dev, struct str_parms *parms)
{
    float coef = 1.0;
    int ret = 0;
    int i = 0;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    ch_coef_info_t *ch_coef_info = NULL;

    int item = sizeof(ch_coef_pair) / sizeof(struct ch_name_pair);


    if (!adev || !parms) {
        ALOGE("Fatal Error adev %p parms %p", adev, parms);
        return -1;
    }

    ch_coef_info = &adev->ch_coef_info;


    ret = str_parms_get_float(parms, BGO_COEF_NAME, &coef);
    if (ret >= 0) {
        if (coef > 1.0) {
            coef = 1.0;
        }
        if (coef < 0.0) {
            coef = 0.0;
        }
        ALOGI("set bgo_coef =%f\n",coef);
        ch_coef_info->bgo_coef= coef;
        return 0;
    }

    ret = str_parms_get_float(parms, LFE_POST_SCALE, &coef);
    if (ret >= 0) {
        if (coef < 0.0) {
            coef = 0.0;
        }
        ALOGI("set lfe_post_scale =%f\n",coef);
        ch_coef_info->lfe_post_scale = coef;
        return 0;
    }



    for (i = 0; i < item; i++) {
        ret = str_parms_get_float(parms, ch_coef_pair[i].name, &coef);
        if (ret >= 0) {
            ALOGI("set ch=%s coef=%f\n", ch_coef_pair[i].name, coef);
            set_channel_coef(ch_coef_info, ch_coef_pair[i].ch_id, coef);
            return 0;
        }
    }

    return -1;
}


static int aml_lfe_mixing(void * in_data, size_t size, aml_data_format_t *format, int lfe_index, float bgo_coef)
{

    int nframes = 0;
    int ch = 0;
    int i = 0, j = 0;
    int bitwidth = SAMPLE_16BITS;
    bitwidth = format->bitwidth;
    ch = format->ch;
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
        int temp;
        for (i = 0 ; i < 2 ; i++) {
            for (j = 0; j < nframes; j++) {
                temp = (int)data[j * ch + i] + (int)data[j * ch + lfe_index]*bgo_coef;
                data[j * ch + i] = Clip(temp, -(1<<15), (1<<15)-1);
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
        for (i = 0 ; i < 2 ; i++) {
            for (j = 0; j < nframes; j++) {
                temp = (long long)data[j * ch + i] + (long long)data[j * ch + lfe_index]*bgo_coef;
                data[j * ch + i] = Clip(temp, -(1ll<<31), (1ll<<31)-1);
            }
        }
    }
    break;
    default:
        break;
    }

    return 0;

}

static int aml_lfe_post_scale(void * in_data, size_t size, aml_data_format_t *format, int lfe_index, float lfe_post_scale)
{

    int nframes = 0;
    int ch = 0;
    int j = 0;
    int bitwidth = SAMPLE_16BITS;
    bitwidth = format->bitwidth;
    ch = format->ch;
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
        int temp;
            for (j = 0; j < nframes; j++) {
                temp = (int)data[j * ch + lfe_index] * lfe_post_scale;
                data[j * ch + lfe_index] = Clip(temp, -(1<<15), (1<<15)-1);
            }

    }
    break;
    case SAMPLE_24BITS:
        // not suppport
        break;
    case SAMPLE_32BITS: {
        int * data = (int*) in_data;
        long long temp = 0;
        for (j = 0; j < nframes; j++) {
            temp = (long long)data[j * ch + lfe_index] * lfe_post_scale;
            data[j * ch + lfe_index] = Clip(temp, -(1ll<<31), (1ll<<31)-1);
        }
    }
    break;
    default:
        break;
    }

    return 0;

}



int aml_bm_process(struct audio_stream_out *stream
                    , const void *buffer
                    , size_t bytes
                    , aml_data_format_t *data_format)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = NULL;
    /*this is bass management part!*/
    int dump_bm = 0;
    int i = 0;
    int j = 0;
    int32_t *sample = (int32_t *)buffer;
    int sample_num =  0;
    int ret = 0;
    float coef_table[AML_MAX_CHANNELS] = {0.0};
    int lfe_index = -1;
    ch_coef_info_t *ch_coef_info = NULL;
    if (!stream || !buffer || !data_format || (bytes == 0)) {
        ALOGE("stream %p buffer %p data_format %p bytes %d");
        return 1;
    }

    adev = aml_out->dev;
    sample_num = bytes/(data_format->ch * (data_format->bitwidth >> 3));

    ch_coef_info = &adev->ch_coef_info;

    if (dump_bm && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
        FILE *fp_original=fopen(DATMOS_ORIGINAL_LFE,"a+");
        fwrite(buffer, 1, bytes,fp_original);
        fclose(fp_original);
    }
    set_coef_table(ch_coef_info, &data_format->channel_info, coef_table);

    for (i = 0; i < AML_MAX_CHANNELS; i++) {
        if ((data_format->channel_info.channel_items[i].ch_id == CHANNEL_LFE)
            && data_format->channel_info.channel_items[i].present) {
            lfe_index = data_format->channel_info.channel_items[i].order;
        }

    }
    //ALOGI("coef table=%f %f %f %f bgo_coef=%f lef_index=%d\n",coef_table[0],coef_table[1],coef_table[2],coef_table[3],ch_coef_info->bgo_coef,lfe_index);
    /* due to performance issue, we disable 192K&176K BM process */
    if ((adev->bm_enable) && (data_format->bitwidth == SAMPLE_32BITS) && (lfe_index != -1)) {
        ret = aml_bm_lowerpass_process(buffer, bytes, sample_num, data_format->ch, lfe_index, coef_table, data_format->bitwidth);
    }

    /*mix the lfe to left, right channel*/
    if (lfe_index <= data_format->ch && ch_coef_info->bgo_coef != 0.0) {
        //ALOGD("lfe index=%d ch total=%d bgo=%f\n",lfe_index, data_format->ch, ch_coef_info->bgo_coef);
        aml_lfe_mixing((void*)buffer, bytes, data_format, lfe_index, ch_coef_info->bgo_coef);
    }

    aml_lfe_post_scale((void*)buffer, bytes, data_format, lfe_index, ch_coef_info->lfe_post_scale);

    if (dump_bm && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
        FILE *fp_bm=fopen(DATMOS_BASSMANAGEMENT_LFE,"a+");
        fwrite(buffer, 1, bytes,fp_bm);
        fclose(fp_bm);
    }

    return ret;
}
#else
int aml_bm_init(struct audio_hw_device *dev)
{   
    struct aml_audio_device *adev = (struct aml_audio_device *)dev;
    if (!adev) {
        ALOGE("adev %p", adev);
        return 1;
    }
    
    adev->bm_enable = 0;

    return 0;
}

int aml_bm_set(struct aml_audio_device *adev, int val)
{
    return 0;
}


int aml_bm_setparameters(struct audio_hw_device *dev, struct str_parms *parms) {
    return 0;
}


int aml_bm_process(struct audio_stream_out *stream
                    , const void *buffer
                    , size_t bytes
                    , aml_data_format_t *data_format)
{
    return 0;
}

int aml_bm_close(struct audio_hw_device *dev)
{
    return 0;
}
#endif
