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

#include <compiler.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include <cutils/properties.h>

#include "audio_eq_drc_compensation.h"

#undef  LOG_TAG
#define LOG_TAG  "audio_hw_primary"

#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"

static struct audio_file_config_s dev_cfg[2] = {
    {/*amlogic inner EQ & DRC*/
        "AMLOGIC_SOC_INI_PATH",
        "",
    },
    {/*ext amp EQ & DRC*/
        "EXT_AMP_INI_PATH",
        "",
    }
};

static inline float DbToAmpl(float decibels)
{
    if (decibels <= -758) {
        return 0.0f;
    }
    return exp( decibels * 0.115129f); // exp( dB * ln(10) / 20 )
}

uint32_t swapInt32(uint32_t value)
{
    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0xFF000000) >> 24) ;
}

int16_t swapInt16(int16_t value)
{
    return ((value & 0x00FF) << 8) |
           ((value & 0xFF00) >> 8) ;
}

static int get_model_name(char *model_name, int size)
{
    int ret = -1;
    char node[PROPERTY_VALUE_MAX];

    ret = property_get("tv.model_name", node, NULL);
    if (ret < 0) {
        ALOGD("%s: Can't get model name!", __FUNCTION__);
    } else {
        snprintf(model_name, size, "%s", node);
        ALOGD("%s: Model Name (%s)", __FUNCTION__, model_name);
    }

    return ret;
}

static int eq_drc_ctl_value_set(int card, int val, char *name)
{
    int ret = -1;
    struct mixer_ctl *ctl;
    struct mixer *mixer;

    mixer = mixer_open(card);
    if (mixer == NULL) {
        ALOGE("%s: mixer is closed", __FUNCTION__);
        return -1;
    }
    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL) {
        ALOGE("%s: get mixer ctl failed, name %s", __FUNCTION__,name);
        goto ERROR;
    }
    if (mixer_ctl_set_value(ctl, 0, val)) {
        ALOGE("%s: set value = %d failed", __FUNCTION__, val);
        goto ERROR;
    }

    ret = 0;
ERROR:
    mixer_close(mixer);
    return ret;
}

static int eq_drc_ctl_array_set(int card, int val_count, unsigned int *values, char *name, int reg_mode)
{
    int i, num_values;
    struct mixer_ctl *ctl;
    struct mixer *mixer;

    mixer = mixer_open(card);
    if (mixer == NULL) {
        ALOGE("%s: mixer is closed", __FUNCTION__);
        return -1;
    }
    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL) {
        ALOGE("%s: get mixer ctl failed", __FUNCTION__);
        goto ERROR;
    }
    num_values = mixer_ctl_get_num_values(ctl);
    if (num_values != val_count * reg_mode) {
        ALOGE("%s: num_values[%d] != val_count[%d] failed", __FUNCTION__, num_values, val_count);
        goto ERROR;
    }

    ALOGI("%s: reg_mode = %d", __FUNCTION__, reg_mode);
    if (reg_mode == 1) {
        /* register type is u8 */
        char *buf;
        buf = (char *)calloc(1, num_values);
        if (buf == NULL) {
            ALOGE("%s: Failed to alloc mem for bytes: %d", __FUNCTION__, num_values);
            goto ERROR;
        }
        for (i = 0; i < num_values; i++) {
            /*ALOGI("buf[%d] = 0x%x", i, values[i]);*/
            buf[i] = (char)values[i];
        }
        mixer_ctl_set_array(ctl, buf, (size_t)num_values);
    } else if (reg_mode == 2) {
        /* register type is u16 */
        int16_t *buf;
        buf = (int16_t *)calloc(2, num_values);
        if (buf == NULL) {
            ALOGE("%s: Failed to alloc mem for bytes: %d", __FUNCTION__, num_values);
            goto ERROR;
        }
        for (i = 0; i < val_count; i++) {
            /*ALOGI("buf[%d] = 0x%x", i, values[i]);*/
            buf[i] = swapInt16((int16_t)values[i]);
        }
        mixer_ctl_set_array(ctl, buf, (size_t)num_values);
    } else if (reg_mode == 4) {
        /*register type is u32*/
        for (i = 0; i < val_count; i++) {
            /*ALOGI("buf[%d] = 0x%x", i, values[i]);*/
            values[i] = swapInt32(values[i]);
        }
        mixer_ctl_set_array(ctl, values, (size_t)num_values);
    }

ERROR:
    mixer_close(mixer);
    return 0;
}


static int eq_status_set(struct audio_eq_drc_info_s *p_attr, int card)
{
    int ret;

    ret = eq_drc_ctl_value_set(card, p_attr->eq.enable, p_attr->eq.eq_enable_name);
    if (ret < 0) {
        ALOGE("%s: set EQ status failed", __FUNCTION__);
    }

    return ret;
}


static int drc_status_set(struct audio_eq_drc_info_s *p_attr, int card)
{
    int ret;

    ret = eq_drc_ctl_value_set(card, p_attr->fdrc.enable, p_attr->fdrc.fdrc_enable_name);
    if (ret < 0) {
        ALOGE("%s: set DRC status failed", __FUNCTION__);
    };

    ret = eq_drc_ctl_value_set(card, p_attr->mdrc.enable, p_attr->mdrc.mdrc_enable_name);
    if (ret < 0) {
        ALOGE("%s: set MDRC status failed", __FUNCTION__);
    };

    return ret;
}

static int volume_set(struct audio_eq_drc_info_s *p_attr, int card)
{
    int ret = 0;

    if (!p_attr->volume.enable)
        goto exit;

    ret = eq_drc_ctl_value_set(card, p_attr->volume.master, p_attr->volume.master_name);
    if (ret < 0) {
        ALOGE("%s: set Master volume failed", __FUNCTION__);
        goto exit;
    }

    ret = eq_drc_ctl_value_set(card, p_attr->volume.ch1, p_attr->volume.ch1_name);
    if (ret < 0) {
        ALOGE("%s: set CH1 volume failed", __FUNCTION__);
        goto exit;
    }

    ret = eq_drc_ctl_value_set(card, p_attr->volume.ch2, p_attr->volume.ch2_name);
    if (ret < 0) {
        ALOGE("%s: set CH2 volume failed", __FUNCTION__);
        goto exit;
    }

exit:
    return ret;
}

static int clip_threshold_set(struct audio_eq_drc_info_s *p_attr, int card)
{
    int ret = 0;
    int value = 0;
    float clip_threshold = 0;

    if (!p_attr->clip.enable)
        goto exit;

    if (p_attr->clip.clip_threshold > 0) {
        ALOGE("%s: Clip threshold is invalid!", __FUNCTION__);
        goto exit;
    }

    clip_threshold = DbToAmpl(p_attr->clip.clip_threshold);
    value = (int)(round(clip_threshold * (1 << 23)));

    ret = eq_drc_ctl_value_set(card, value,
            p_attr->clip.clip_threshold_name);
    if (ret < 0) {
        ALOGE("%s: set Clip threshold failed!", __FUNCTION__);
    }

exit:
    return ret;
}


static int aml_table_set(struct audio_data_s *table, int card, char *name)
{
    struct mixer_ctl *ctl;
    struct mixer *mixer;
    char param_buf[1024];
    int byte_size = 0;
    unsigned int *ptr;

    mixer = mixer_open(card);
    if (mixer == NULL) {
        ALOGE("%s: mixer is closed", __FUNCTION__);
        return -1;
    }

    ctl = mixer_get_ctl_by_name(mixer, name);
    if (ctl == NULL) {
        ALOGE("%s: get mixer ctl failed", __FUNCTION__);
        goto ERROR;
    }

    for (int i = 0; i < table->reg_cnt; i++) {
        ptr = table->regs[i].data;
        memset(param_buf, 0, 1024);
        byte_size = sprintf(param_buf, "0x%x ", *ptr++);
        for (int j = 1; j < table->regs[i].len; j++) {
            byte_size += sprintf(param_buf + byte_size, "0x%8.8x ", *ptr++);
        }
        int ret = mixer_ctl_set_array(ctl, param_buf, byte_size);
        if (ret < 0)
            ALOGE("[%s:%d] failed to set array, error: %d\n",
                    __FUNCTION__, __LINE__, ret);
    }

ERROR:
    mixer_close(mixer);
    return 0;
}

static int aml_drc_set(struct eq_drc_data *pdata)
{
    char *name = pdata->aml_attr->fdrc.fdrc_table_name;
    struct audio_data_s *table = pdata->aml_attr->fdrc.fdrc_table;

    /*fullband drc setting*/
    if (pdata->aml_attr->fdrc.enable)
        aml_table_set(table, pdata->card, name);

    if (pdata->aml_attr->mdrc.enable) {
        name = pdata->aml_attr->mdrc.mdrc_table_name;
        table = pdata->aml_attr->mdrc.mdrc_table;

        /*mulitband drc setting*/
        aml_table_set(table, pdata->card, name);

        name = pdata->aml_attr->mdrc.crossover_table_name;
        table = pdata->aml_attr->mdrc.crossover_table;

        /*mulitband drc crossover setting*/
        aml_table_set(table, pdata->card, name);
    }
    return 0;
}

int eq_mode_set(struct eq_drc_data *pdata __unused, int eq_mode __unused)
{
#if 0
    int i, ret;

    ALOGD("%s: eq_mode -> %d", __FUNCTION__, eq_mode);
    for (i = 0; i < pdata->dev_num; i++) {
        if (strstr(dev_cfg[i].ini_header, "AMLOGIC_SOC_INI_PATH")) {
            pdata->p_attr[i].eq_mode = eq_mode;
            ret = eq_param_init(pdata, i);
            if (ret < 0)
                return ret;
            break;
        }
    }

    if (i >= pdata->dev_num) {
        ALOGE("%s: header -> %s not found", __FUNCTION__, "AMLOGIC_SOC_INI_PATH");
        return -1;
    }
#endif
    return 0;
}

int aml_eq_mode_set(struct eq_drc_data *pdata, int eq_mode)
{
    struct audio_data_s *table = pdata->aml_attr->eq.eq_table[eq_mode];
    char *name = pdata->aml_attr->eq.eq_table_name;

    if (eq_mode >= pdata->aml_attr->eq.eq_table_num) {
        ALOGE("%s: eq mode is invalid", __FUNCTION__);
        return -1;
    }

    if (pdata->aml_attr->eq.enable)
        aml_table_set(table, pdata->card, name);

    return 0;
}

int eq_drc_init(struct eq_drc_data *pdata)
{
    int i, ret;
    char model_name[50] = {0};
    const char *filename = MODEL_SUM_DEFAULT_PATH;

    pdata->s_gain.atv = 1.0;
    pdata->s_gain.dtv = 1.0;
    pdata->s_gain.hdmi= 1.0;
    pdata->s_gain.av = 1.0;
    pdata->p_gain.speaker= 1.0;
    pdata->p_gain.spdif_arc = 1.0;
    pdata->p_gain.headphone = 1.0;

    ret = get_model_name(model_name, sizeof(model_name));
    if (ret < 0) {
        return -1;
    }

    /*parse amlogic ini file*/
    ret = handle_audio_sum_ini(filename, model_name, &dev_cfg[0]);
    if (ret == 0) {
        ret = handle_audio_gain_ini(dev_cfg[0].ini_file, pdata);
        if (ret < 0 ) {
            ALOGE("%s: Get amlogic gain config failed!", __FUNCTION__);
        } else {
            if (pdata->s_gain.enable) {
                pdata->s_gain.atv = DbToAmpl(pdata->s_gain.atv);
                pdata->s_gain.dtv = DbToAmpl(pdata->s_gain.dtv);
                pdata->s_gain.hdmi = DbToAmpl(pdata->s_gain.hdmi);
                pdata->s_gain.av = DbToAmpl(pdata->s_gain.av);
            }
            if (pdata->p_gain.enable) {
                pdata->p_gain.speaker = DbToAmpl(pdata->p_gain.speaker);
                pdata->p_gain.spdif_arc = DbToAmpl(pdata->p_gain.spdif_arc);
                pdata->p_gain.headphone = DbToAmpl(pdata->p_gain.headphone);
            }
        }

        pdata->aml_attr = (struct audio_eq_drc_info_s *)calloc(1, sizeof(struct audio_eq_drc_info_s));
        if (!pdata->aml_attr) {
            ALOGE("%s: calloc amlogic audio_eq_drc_info_s failed", __FUNCTION__);
            return -1;
        }

        handle_audio_eq_drc_ini(dev_cfg[0].ini_file, pdata->aml_attr);
        volume_set(pdata->aml_attr, pdata->card);
        eq_status_set(pdata->aml_attr, pdata->card);
        drc_status_set(pdata->aml_attr, pdata->card);
        aml_eq_mode_set(pdata, 0);
        aml_drc_set(pdata);
        clip_threshold_set(pdata->aml_attr, pdata->card);
    }

    return 0;
}

int eq_drc_release(struct eq_drc_data *pdata)
{
    if (pdata->aml_attr) {
        free(pdata->aml_attr);
        pdata->aml_attr = NULL;
    }

    return 0;
}
