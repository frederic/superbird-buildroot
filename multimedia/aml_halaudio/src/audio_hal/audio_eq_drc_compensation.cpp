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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <tinyalsa/asoundlib.h>

#include "log.h"
#include "properties.h"

#include "audio_eq_drc_compensation.h"
#include "aml_volume_utils.h"

#undef  LOG_TAG
#define LOG_TAG  "audio_eq_drc_compensation"

#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"
#define EQ_DRC_SOC_DEFAULT_PATH "/vendor/etc/tvconfig/audio/AMLOGIC_SOC_DEFAULT.ini"
#define EXT_AMP_DEFAULT_PATH "/vendor/etc/tvconfig/audio/EXT_AMP_DEFAULT.ini"

static struct eq_drc_device_config_s dev_cfg[] = {
    {/*amlogic inner EQ & DRC*/
        "AMLOGIC_SOC_INI_PATH",
        EQ_DRC_SOC_DEFAULT_PATH,
        "EQ enable",
        "DRC enable",
        "EQ table",
        "DRC table",
        "EQ Model ID",
        "EQ master volume",
        "EQ ch1 volume",
        "EQ ch2 volume"
    },
    {/*ext amp EQ & DRC*/
        "EXT_AMP_INI_PATH",
        EXT_AMP_DEFAULT_PATH,
        "AMP Set EQ Enable",
        "AMP Set DRC Enable",
        "AMP EQ table",
        "AMP DRC table",
        "AMP Set Model ID",
        "AMP Master Volume",
        "AMP Ch1 Volume",
        "AMP Ch2 Volume"
    },
};

static int get_model_name(char *model_name, int size)
{


    int ret = -1;
    char node[PROPERTY_VALUE_MAX];

    ret = property_get("tv.model_name", node, NULL);

    if (ret < 0)
        snprintf(model_name, size, "DEFAULT");
    else
        snprintf(model_name, size, "%s", node);
    ALOGD("%s: Model Name -> %s", __FUNCTION__, model_name);
    return ret;
}

static int eq_drc_ctl_value_set(struct eq_drc_data *pdata, int val, char *name)
{
    int ret = -1;
    struct mixer_ctl *ctl;

    pdata->mixer = mixer_open(pdata->card);
    if (pdata->mixer == NULL) {
        ALOGE("%s: mixer is closed", __FUNCTION__);
        return -1;
    }
    ctl = mixer_get_ctl_by_name(pdata->mixer, name);
    if (ctl == NULL) {
        ALOGE("%s: get mixer ctl failed", __FUNCTION__);
        goto ERROR;
    }
    if (mixer_ctl_set_value(ctl, 0, val)) {
        ALOGE("%s: set value = %d failed", __FUNCTION__, val);
        goto ERROR;
    }

    ret = 0;
ERROR:
    mixer_close(pdata->mixer);
    return ret;
}

unsigned int swapInt32(unsigned  value)
{
     return ((value & 0x000000FF) << 24) |
               ((value & 0x0000FF00) << 8) |
               ((value & 0x00FF0000) >> 8) |
               ((value & 0xFF000000) >> 24) ;
}

short swapInt16(short value)
{
     return ((value & 0x00FF) << 8) |
               ((value & 0xFF00) >> 8) ;
}

static int eq_drc_ctl_array_set(struct eq_drc_data *pdata, int val_count, unsigned int *values, char *name, int reg_mode)
{
    int i, num_values;
    struct mixer_ctl *ctl;

    pdata->mixer = mixer_open(pdata->card);
    if (pdata->mixer == NULL) {
        ALOGE("%s: mixer is closed", __FUNCTION__);
        return -1;
    }
    ctl = mixer_get_ctl_by_name(pdata->mixer, name);
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
    mixer_close(pdata->mixer);
    return 0;
}

static int eq_status_set(struct eq_drc_data *pdata, int id, int status)
{
    int val = status;

    ALOGD("%s: name = %s val = %d", __FUNCTION__, dev_cfg[id].eq, val);
    return eq_drc_ctl_value_set(pdata, val, dev_cfg[id].eq);
}

static int drc_status_set(struct eq_drc_data *pdata, int id, int status)
{
    int val = status;

    ALOGD("%s: name = %s val = %d", __FUNCTION__, dev_cfg[id].drc, val);
    return eq_drc_ctl_value_set(pdata, val, dev_cfg[id].drc);
}

static int eq_param_init(struct eq_drc_data *pdata, int id)
{
    int i, j, k;
    int eq_mode = pdata->p_attr[id].eq_mode;
    unsigned int eq_vale[CC_REG_DATA_MAX] = {0};
    struct audio_eq_drc_info_s *p_attr = &pdata->p_attr[id];

    if (eq_status_set(pdata, id, 0) < 0) {
        ALOGD("%s: Set EQ Off failed", __FUNCTION__);
        return -1;
    }
    if (p_attr == NULL) {
        ALOGE("%s: pointer p_attr is NULL", __FUNCTION__);
        return -1;
    }
    if (!p_attr->eq_enable) {
        ALOGD("%s: %s EQ config is disabled", __FUNCTION__, p_attr->eq_name);
        return 0;
    }

    k = 0;
    switch (eq_mode) {
    case EQ_MODE_DESK:
        for (i = 0; i < p_attr->eq_desk.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_desk.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_desk.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_desk.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_WALL:
        for (i = 0; i < p_attr->eq_wall.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_wall.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_wall.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_wall.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_CLASSIC:
        for (i = 0; i < p_attr->eq_classic.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_classic.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_classic.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_classic.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_POP:
        for (i = 0; i < p_attr->eq_pop.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_pop.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_pop.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_pop.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_JAZZ:
        for (i = 0; i < p_attr->eq_jazz.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_jazz.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_jazz.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_jazz.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_ROCK:
        for (i = 0; i < p_attr->eq_rock.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_rock.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_rock.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_rock.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_NORMAL:
        for (i = 0; i < p_attr->eq_normal.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_normal.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_normal.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_normal.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_SOFT:
        for (i = 0; i < p_attr->eq_soft.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_soft.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_soft.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_soft.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_BASS:
        for (i = 0; i < p_attr->eq_bass.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_bass.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_bass.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_bass.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_AUTO:
        for (i = 0; i < p_attr->eq_auto.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_auto.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_auto.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_auto.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_SRS_SURROUND_ON:
        for (i = 0; i < p_attr->eq_srs_surround_on.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_srs_surround_on.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_srs_surround_on.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_srs_surround_on.regs[i].data[j]);
            }
        }
        break;
    case EQ_MODE_SRS_SURROUND_OFF:
        for (i = 0; i < p_attr->eq_srs_surround_off.reg_cnt; i++) {
            for (j = 0; j < p_attr->eq_srs_surround_off.regs[i].len; j++, k++) {
                eq_vale[k] = p_attr->eq_srs_surround_off.regs[i].data[j];
                //ALOGD("%s: %d %d", __FUNCTION__, k, p_attr->eq_srs_surround_off.regs[i].data[j]);
            }
        }
        break;
    default:
        ALOGD("%s: unknown eq_mode = %d", __FUNCTION__, eq_mode);
        break;
    }

    if (eq_drc_ctl_array_set(pdata, k, eq_vale, dev_cfg[id].eq_table,
                p_attr->drc_byte_mode) < 0) {
        ALOGD("%s: eq_param_set failed", __FUNCTION__);
        return -1;
    }

    if (eq_status_set(pdata, id, 1) < 0) {
        ALOGD("%s: Set EQ On failed", __FUNCTION__);
        return -1;
    }

    ALOGD("%s: %s EQ is enabled", __FUNCTION__, p_attr->eq_name);

    return 0;
}

static int drc_param_init(struct eq_drc_data *pdata, int id)
{
    int i, j, k;
    unsigned int drc_value[64] = {0};
    struct audio_eq_drc_info_s *p_attr = &pdata->p_attr[id];

    if (drc_status_set(pdata, id, 0) < 0) {
        ALOGD("%s: Set DRC Off failed", __FUNCTION__);
        return -1;
    }
    if (p_attr == NULL) {
        ALOGE("%s: pointer p_attr is NULL", __FUNCTION__);
        return -1;
    }
    if (!p_attr->drc_enable) {
        ALOGD("%s: %s DRC config is disabled", __FUNCTION__, p_attr->drc_name);
        return 0;
    }

    k = 0;
    for (i = 0; i < p_attr->drc_table.reg_cnt; i++) {
        for (j = 0; j < p_attr->drc_table.regs[i].len; j++, k++) {
            drc_value[k] = p_attr->drc_table.regs[i].data[j];
        }
    }
    if (eq_drc_ctl_array_set(pdata, k, drc_value, dev_cfg[id].drc_table,
            p_attr->eq_byte_mode) < 0) {
        ALOGD("%s: drc_param_set failed", __FUNCTION__);
        return -1;
    }

    if (drc_status_set(pdata, id , 1) < 0) {
        ALOGD("%s: Set Drc On failed", __FUNCTION__);
        return -1;
    }

    ALOGD("%s: %s DRC is enabled", __FUNCTION__, p_attr->drc_name);

    return 0;
}

static int volume_set(struct eq_drc_data *pdata, int id)
{
    int ret = 0;
    struct audio_eq_drc_info_s *p_attr = &pdata->p_attr[id];

    if (!p_attr->volume.support)
        goto exit;

    ret = eq_drc_ctl_value_set(pdata, p_attr->volume.master, dev_cfg[id].master_vol);
    if (ret < 0) {
        ALOGE("%s: set Device(%d) Master volume failed", __FUNCTION__, id);
        goto exit;
    }

    ret = eq_drc_ctl_value_set(pdata, p_attr->volume.ch1, dev_cfg[id].ch1_vol);
    if (ret < 0) {
        ALOGE("%s: set Device(%d) CH1 volume failed", __FUNCTION__, id);
        goto exit;
    }

    ret = eq_drc_ctl_value_set(pdata, p_attr->volume.ch2, dev_cfg[id].ch2_vol);
    if (ret < 0) {
        ALOGE("%s: set Device(%d) CH2 volume failed", __FUNCTION__, id);
        goto exit;
    }

exit:
    return ret;
}

static int model_id_set(struct eq_drc_data *pdata, int id)
{
    int ret = 0;
    struct audio_eq_drc_info_s *p_attr = &pdata->p_attr[id];

    if (!p_attr->mod.support)
        return ret;

    ret = eq_drc_ctl_value_set(pdata, p_attr->mod.model, dev_cfg[id].model);
    if (ret < 0)
        ALOGE("%s: set Device(%d) Model ID failed", __FUNCTION__, id);
    return ret;
}

static int eq_drc_param_init(struct eq_drc_data *pdata, int eq_init, int drc_init)
{
    int i, ret = 0;

    ALOGD("%s: eq_init = %d drc_init = %d", __FUNCTION__, eq_init, drc_init);
    for (i = 0; i < pdata->dev_num; i++) {
        if (eq_init) {
            ret = eq_param_init(pdata, i);
            if (ret < 0) {
                ALOGD("%s: eq_param_init failed", __FUNCTION__);
                return ret;
            }
        }
        if (drc_init) {
            ret = drc_param_init(pdata, i);
            if (ret < 0) {
                ALOGD("%s: drc_param_init failed", __FUNCTION__);
                return ret;
            }
        }
    }

    ALOGD("%s: sucessful", __FUNCTION__);

    return ret;
}

int eq_mode_set(struct eq_drc_data *pdata, int eq_mode)
{
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

    return 0;
}

int eq_drc_init(struct eq_drc_data *pdata)
{
    int i, ret;
    char node[50] = {0};
    char buffer[100] = {0};
    char model_name[50] = {0};
    const char *filename = MODEL_SUM_DEFAULT_PATH;

    pdata->s_gain.atv = 1.0;
    pdata->s_gain.dtv = 1.0;
    pdata->s_gain.hdmi= 1.0;
    pdata->s_gain.av = 1.0;
    pdata->p_gain.speaker= 1.0;
    pdata->p_gain.spdif_arc = 1.0;
    pdata->p_gain.headphone = 1.0;

    pdata->dev_num = sizeof(dev_cfg) / sizeof(struct eq_drc_device_config_s);
    ALOGD("%s: device num = %d", __FUNCTION__, pdata->dev_num);

    pdata->p_attr = (struct audio_eq_drc_info_s *)calloc(pdata->dev_num, sizeof(struct audio_eq_drc_info_s));
    if (!pdata->p_attr) {
        ALOGE("%s: calloc audio_eq_drc_info_s failed", __FUNCTION__);
        return -1;
    }

    get_model_name(model_name, sizeof(model_name));
    for (i = 0; i < pdata->dev_num; i++) {
        handle_audio_sum_ini(filename, model_name, &dev_cfg[i]);
        ret = handle_audio_eq_drc_ini(dev_cfg[i].ini_file, &pdata->p_attr[i]);
        if (ret < 0 ) {
            ALOGE("%s: Get device %d  EQ&DRC config failed", __FUNCTION__, i);
            goto ERROR;
        }

        model_id_set(pdata, i);
        volume_set(pdata, i);
        if (strcmp(pdata->p_attr[i].eq_name, pdata->p_attr[i].drc_name) != 0)
            ALOGD("%s: EQ name = %s DRC name = %s", __FUNCTION__, pdata->p_attr[i].eq_name, pdata->p_attr[i].drc_name);
        if (strstr(dev_cfg[i].ini_header, "AMLOGIC_SOC_INI_PATH")) {
            if (pdata->p_attr[i].s_gain.support) {
                pdata->s_gain.atv = DbToAmpl(pdata->p_attr[i].s_gain.atv);
                pdata->s_gain.dtv = DbToAmpl(pdata->p_attr[i].s_gain.dtv);
                pdata->s_gain.hdmi= DbToAmpl(pdata->p_attr[i].s_gain.hdmi);
                pdata->s_gain.av = DbToAmpl(pdata->p_attr[i].s_gain.av);
            }
            if (pdata->p_attr[i].p_gain.support) {
                pdata->p_gain.speaker= DbToAmpl(pdata->p_attr[i].p_gain.speaker);
                pdata->p_gain.spdif_arc = DbToAmpl(pdata->p_attr[i].p_gain.spdif_arc);
                pdata->p_gain.headphone = DbToAmpl(pdata->p_attr[i].p_gain.headphone);
            }
        } else {
            strcpy(node, pdata->p_attr[i].eq_name);
        }

        memset(buffer, 0, sizeof(buffer));
        memset(node, 0, sizeof(node));
    }

    ret = eq_drc_param_init(pdata, 1, 1);
    if (ret < 0) {
        ALOGE("%s: EQ&DRC init failed", __FUNCTION__);
        goto ERROR;
    }

    return 0;
ERROR:
    if (pdata->p_attr) {
        free(pdata->p_attr);
        pdata->p_attr = NULL;
    }
    return ret;
}

int eq_drc_release(struct eq_drc_data *pdata)
{
    pdata->mixer = NULL;
    if (pdata->p_attr) {
        free(pdata->p_attr);
        pdata->p_attr = NULL;
    }
    return 0;
}
