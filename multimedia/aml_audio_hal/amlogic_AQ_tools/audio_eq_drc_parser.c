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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cutils/log.h>

#include "audio_eq_drc_parser.h"
#include "iniparser.h"

#undef  LOG_TAG
#define LOG_TAG "audio_hw_primary"

#define ITEM_DEBUG

#ifdef ITEM_DEBUG
#define ITEM_LOGD(x...) ALOGD(x)
#define ITEM_LOGE(x...) ALOGE(x)
#else
#define ITEM_LOGD(x...)
#define ITEM_LOGE(x...)
#endif

static int parse_audio_source_gain_data(dictionary *pIniParser, struct eq_drc_data *p_attr)
{
    p_attr->s_gain.enable = iniparser_getboolean(pIniParser, "source_gain:sg_enable", 0);
    if (!p_attr->s_gain.enable) {
        ITEM_LOGD("%s, Section -> [source_gain] is not exist\n", __FUNCTION__);
        return 0;
    }

    p_attr->s_gain.atv = iniparser_getdouble(pIniParser, "source_gain:atv", 0);
    ITEM_LOGD("%s, atv is (%f)\n", __FUNCTION__, p_attr->s_gain.atv);

    p_attr->s_gain.dtv = iniparser_getdouble(pIniParser, "source_gain:dtv", 0);
    ITEM_LOGD("%s, dtv is (%f)\n", __FUNCTION__, p_attr->s_gain.dtv);

    p_attr->s_gain.hdmi = iniparser_getdouble(pIniParser, "source_gain:hdmi", 0);
    ITEM_LOGD("%s, hdmi is (%f)\n", __FUNCTION__, p_attr->s_gain.hdmi);

    p_attr->s_gain.av = iniparser_getdouble(pIniParser, "source_gain:av", 0);
    ITEM_LOGD("%s, av is ((%f)\n", __FUNCTION__, p_attr->s_gain.av);

    return 0;
}

static int parse_audio_post_gain_data(dictionary *pIniParser, struct eq_drc_data *p_attr)
{
    p_attr->p_gain.enable = iniparser_getboolean(pIniParser, "post_gain:pg_enable", 0);
    if (!p_attr->p_gain.enable) {
        ITEM_LOGD("%s, Section -> [post_gain] is not exist\n", __FUNCTION__);
        return 0;
    }

    p_attr->p_gain.speaker = iniparser_getdouble(pIniParser, "post_gain:speaker", 0);
    ITEM_LOGD("%s, speaker is (%f)\n", __FUNCTION__, p_attr->p_gain.speaker);

    p_attr->p_gain.spdif_arc = iniparser_getdouble(pIniParser, "post_gain:spdif_arc", 0);
    ITEM_LOGD("%s, spdif_arc is (%f)\n", __FUNCTION__, p_attr->p_gain.spdif_arc);

    p_attr->p_gain.headphone = iniparser_getdouble(pIniParser, "post_gain:headphone", 0);
    ITEM_LOGD("%s, headphone is (%f)\n", __FUNCTION__, p_attr->p_gain.headphone);

    return 0;
}

static int parse_audio_ng_data(dictionary *pIniParser, struct eq_drc_data *p_attr)
{

    p_attr->noise_gate.aml_ng_enable = iniparser_getboolean(pIniParser, "noise_gate:ng_enable", 0);
    if (!p_attr->noise_gate.aml_ng_enable) {
        ITEM_LOGD("%s, noise gate is disable!\n", __FUNCTION__);
        return 0;
    }

    p_attr->noise_gate.aml_ng_level = iniparser_getdouble(pIniParser, "noise_gate:ng_level", -75.0f);
    ITEM_LOGD("%s, noise gate level is (%f)\n", __FUNCTION__, p_attr->noise_gate.aml_ng_level);

    p_attr->noise_gate.aml_ng_attack_time = iniparser_getint(pIniParser, "noise_gate:ng_attack_time", 3000);
    ITEM_LOGD("%s, noise gate attack time (%d)\n", __FUNCTION__, p_attr->noise_gate.aml_ng_attack_time);

    p_attr->noise_gate.aml_ng_release_time = iniparser_getint(pIniParser, "noise_gate:ng_release_time", 50);
    ITEM_LOGD("%s, noise gate release time (%d)\n", __FUNCTION__, p_attr->noise_gate.aml_ng_release_time);

    return 0;
}

static int parse_audio_volume_status(dictionary *pIniParser, struct audio_eq_drc_info_s *p_attr)
{
    const char  *str;

    p_attr->volume.enable = iniparser_getboolean(pIniParser, "volume:vol_enable", 0);
    if (!p_attr->volume.enable) {
        ITEM_LOGD("%s, Volume is disable!\n", __FUNCTION__);
        return 0;
    }

    p_attr->volume.master = iniparser_getint(pIniParser, "volume:master", 0);
    ITEM_LOGD("%s, Master Volume is (%d)\n", __FUNCTION__, p_attr->volume.master);

    str = iniparser_getstring(pIniParser, "volume:master_name", NULL);
    strcpy(p_attr->volume.master_name, str);
    ITEM_LOGD("%s, Master Volume Name is (%s)\n", __FUNCTION__, p_attr->volume.master_name);

    p_attr->volume.ch1 = iniparser_getint(pIniParser, "volume:ch1", 0);
    ITEM_LOGD("%s, CH1 Volume is (%d)\n", __FUNCTION__, p_attr->volume.ch1);

    str = iniparser_getstring(pIniParser, "volume:ch1_name", NULL);
    strcpy(p_attr->volume.ch1_name, str);
    ITEM_LOGD("%s, Master Ch1 Name is (%s)\n", __FUNCTION__, p_attr->volume.ch1_name);

    p_attr->volume.ch2 = iniparser_getint(pIniParser, "volume:ch2", 0);
    ITEM_LOGD("%s, CH2 Volume is  (%d)\n", __FUNCTION__, p_attr->volume.ch2);

    str = iniparser_getstring(pIniParser, "volume:ch2_name", NULL);
    strcpy(p_attr->volume.ch2_name, str);
    ITEM_LOGD("%s, Master Ch2 Name is (%s)\n", __FUNCTION__, p_attr->volume.ch2_name);

    return 0;
}

static int parse_audio_eq_status(dictionary *pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    const char  *str;

    p_attr->eq.enable = iniparser_getboolean(pIniParser, "eq_param:eq_enable", 0);
    if (!p_attr->eq.enable) {
        ITEM_LOGD("%s, eq is disable!\n", __FUNCTION__);
        return 0;
    }

    str = iniparser_getstring(pIniParser, "eq_param:eq_name", NULL);
    strcpy(p_attr->eq.eq_name, str);
    ITEM_LOGD("%s, EQ is from (%s)\n", __FUNCTION__, p_attr->eq.eq_name);

    str = iniparser_getstring(pIniParser, "eq_param:eq_enable_name", NULL);
    strcpy(p_attr->eq.eq_enable_name, str);
    ITEM_LOGD("%s, EQ enable name is (%s)\n", __FUNCTION__, p_attr->eq.eq_enable_name);

    p_attr->eq.eq_byte_mode = iniparser_getint(pIniParser, "eq_param:eq_byte_mode", 0);
    ITEM_LOGD("%s, EQ byte is (%d)\n", __FUNCTION__, p_attr->eq.eq_byte_mode);

    p_attr->eq.eq_table_num = iniparser_getint(pIniParser, "eq_param:eq_table_num", 0);
    ITEM_LOGD("%s, EQ table num is (%d)\n", __FUNCTION__, p_attr->eq.eq_table_num);

    str = iniparser_getstring(pIniParser, "eq_param:eq_table_name", NULL);
    strcpy(p_attr->eq.eq_table_name, str);
    ITEM_LOGD("%s, EQ table name is (%s)\n", __FUNCTION__, p_attr->eq.eq_table_name);

    return 0;
}

static int parse_audio_drc_status(dictionary *pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    const char  *str;

    p_attr->fdrc.enable = iniparser_getboolean(pIniParser, "drc_param:drc_enable", 0);
    if (!p_attr->fdrc.enable) {
        ITEM_LOGD("%s, fullband drc is disable!\n", __FUNCTION__);
    } else {
        str = iniparser_getstring(pIniParser, "drc_param:drc_name", NULL);
        strcpy(p_attr->fdrc.fdrc_name, str);
        ITEM_LOGD("%s, DRC is from (%s)\n", __FUNCTION__, p_attr->fdrc.fdrc_name);

        str = iniparser_getstring(pIniParser, "drc_param:drc_enable_name", NULL);
        strcpy(p_attr->fdrc.fdrc_enable_name, str);
        ITEM_LOGD("%s, DRC enable name is (%s)\n", __FUNCTION__, p_attr->fdrc.fdrc_enable_name);

        p_attr->fdrc.drc_byte_mode = iniparser_getint(pIniParser, "drc_param:drc_byte_mode", 4);
        ITEM_LOGD("%s, DRC byte is (%d)\n", __FUNCTION__, p_attr->fdrc.drc_byte_mode);
        p_attr->mdrc.drc_byte_mode = p_attr->fdrc.drc_byte_mode;

        str = iniparser_getstring(pIniParser, "drc_param:drc_table_name", NULL);
        strcpy(p_attr->fdrc.fdrc_table_name, str);
        ITEM_LOGD("%s, DRC table name is (%s)\n", __FUNCTION__, p_attr->fdrc.fdrc_table_name);
    }

    p_attr->mdrc.enable = iniparser_getboolean(pIniParser, "drc_param:mdrc_enable", 0);
    if (!p_attr->mdrc.enable) {
        ITEM_LOGD("%s, multiband drc is disable!\n", __FUNCTION__);
    } else {
        str = iniparser_getstring(pIniParser, "drc_param:mdrc_enable_name", NULL);
        strcpy(p_attr->mdrc.mdrc_enable_name, str);
        ITEM_LOGD("%s, multiband DRC enable name is (%s)\n", __FUNCTION__, p_attr->mdrc.mdrc_enable_name);

        str = iniparser_getstring(pIniParser, "drc_param:mdrc_table_name", NULL);
        strcpy(p_attr->mdrc.mdrc_table_name, str);
        ITEM_LOGD("%s, multiband DRC table name is (%s)\n", __FUNCTION__, p_attr->mdrc.mdrc_table_name);

        str = iniparser_getstring(pIniParser, "drc_param:crossover_table_name", NULL);
        strcpy(p_attr->mdrc.crossover_table_name, str);
        ITEM_LOGD("%s, multiband DRC crossover name is (%s)\n", __FUNCTION__, p_attr->mdrc.crossover_table_name);

        if (p_attr->mdrc.drc_byte_mode == 0)
            p_attr->mdrc.drc_byte_mode = 4;
    }

    p_attr->clip.enable = iniparser_getboolean(pIniParser, "hw_clip:clip_enable", 0);
    if (!p_attr->clip.enable) {
        ITEM_LOGD("%s, hw clip is disable!\n", __FUNCTION__);
    } else {
        p_attr->clip.clip_threshold = iniparser_getdouble(pIniParser, "hw_clip:clip_threshold", 0.0f);
        ITEM_LOGD("%s, hw clip threshold is (%f)\n", __FUNCTION__, p_attr->clip.clip_threshold);

        str = iniparser_getstring(pIniParser, "hw_clip:clip_threshold_name", NULL);
        strcpy(p_attr->clip.clip_threshold_name, str);
        ITEM_LOGD("%s, hw clip threshold name is (%s)\n", __FUNCTION__, p_attr->clip.clip_threshold_name);
    }

    return 0;
}

static int transBufferData(const char *data_str, unsigned int data_buf[])
{
    int item_ind = 0;
    char *token;
    char *pSave;
    char tmp_buf[4096];

    if (data_str == NULL) {
        return 0;
    }

    memset((void *)tmp_buf, 0, sizeof(tmp_buf));
    strncpy(tmp_buf, data_str, sizeof(tmp_buf) - 1);
    token = strtok_r(tmp_buf, ",", &pSave);
    while (token != NULL) {
        data_buf[item_ind] = strtoul(token, NULL, 0);
        item_ind++;
        token = strtok_r(NULL, ",", &pSave);
    }

    return item_ind;
}

static int parse_audio_table_data(dictionary *pIniParser, struct audio_data_s *table)
{
    int i = 0, j = 0, k = 0, data_cnt = 0;
    const char  *str = NULL;
    unsigned int tmp_buf[4096];

    str = iniparser_getstring(pIniParser, table->section_name, NULL);
    //ITEM_LOGD("%s, reg buffer data = %s\n", __FUNCTION__, str);

    memset(tmp_buf, 0, 4096);
    data_cnt = transBufferData(str, tmp_buf);
    ITEM_LOGD("%s, reg buffer data cnt = %d\n", __FUNCTION__, data_cnt);

    while (i <= data_cnt) {
        if (j >= CC_AUDIO_REG_DATA_MAX) {
            break;
        }

        if (tmp_buf[i] == 0) {
            break;
        }

        table->regs[j].len = tmp_buf[i];

        for (k = 0; k < table->regs[j].len; k++) {
            table->regs[j].data[k] = tmp_buf[i + 1 + k];
        }

        i += table->regs[j].len + 1;
        j += 1;
    }

    table->reg_cnt = j;
    return 0;
}

static void PrintRegData(int byte_mode, struct audio_data_s *table)
{
    int i = 0, j = 0, tmp_len = 0;
    char tmp_buf[1024] = {'\0'};

    ITEM_LOGD("%s, reg_cnt = %d\n", __FUNCTION__, table->reg_cnt);

    memset(tmp_buf, 0, 1024);

    for (i = 0; i < table->reg_cnt; i++) {
        tmp_len = strlen(tmp_buf);
        sprintf((char *)tmp_buf + tmp_len, "%d, ", table->regs[i].len);

        for (j = 0; j < table->regs[i].len; j++) {
            tmp_len = strlen(tmp_buf);
            if (byte_mode == 4) {
                sprintf((char *)tmp_buf + tmp_len, "0x%08X, ", table->regs[i].data[j]);
            } else if (byte_mode == 2){
                sprintf((char *)tmp_buf + tmp_len, "0x%02X, ", table->regs[i].data[j]);
            }
        }

        ITEM_LOGD("%s", tmp_buf);
        memset(tmp_buf, 0, 1024);
    }

    ITEM_LOGD("\n\n");
}

static int parse_audio_eq_data(dictionary *pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    int eq_tabel_num = p_attr->eq.eq_table_num;
    int i = 0;
    struct audio_data_s *table;
    char buf[64];

    for (i = 0; i < eq_tabel_num; i++) {
        table = (struct audio_data_s *)calloc(1, sizeof(struct audio_data_s));
        if (!table) {
            ALOGE("%s: calloc audio_data_s failed!", __FUNCTION__);
        } else {
            p_attr->eq.eq_table[i] = table;
            memset(buf, 0, 64);
            sprintf(buf, "eq_param:eq_table_%d", i);
            strcpy(table->section_name, buf);
            ITEM_LOGD("%s, section_name = %s\n", __FUNCTION__, table->section_name);
            parse_audio_table_data(pIniParser, table);
            PrintRegData(p_attr->eq.eq_byte_mode, table);
        }
    }

    return 0;
}

static int parse_audio_fdrc_data(dictionary *pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    struct audio_data_s *table;

    table = (struct audio_data_s *)calloc(1, sizeof(struct audio_data_s));
    if (!table) {
        ALOGE("%s: calloc audio_data_s failed!", __FUNCTION__);
    } else {
        p_attr->fdrc.fdrc_table = table;
        strcpy(table->section_name, "drc_param:drc_table");
        ITEM_LOGD("%s, section_name = %s\n", __FUNCTION__, table->section_name);
        parse_audio_table_data(pIniParser, table);
        PrintRegData(p_attr->fdrc.drc_byte_mode, table);
    }

    return 0;
}

static int parse_audio_mdrc_data(dictionary *pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    struct audio_data_s *table;

    table = (struct audio_data_s *)calloc(1, sizeof(struct audio_data_s));
    if (!table) {
        ALOGE("%s: calloc audio_data_s failed!", __FUNCTION__);
    } else {
        p_attr->mdrc.mdrc_table = table;
        strcpy(table->section_name, "drc_param:mdrc_table");
        ITEM_LOGD("%s, section_name = %s\n", __FUNCTION__, table->section_name);
        parse_audio_table_data(pIniParser, table);
        PrintRegData(p_attr->mdrc.drc_byte_mode, table);
    }

    table = (struct audio_data_s *)calloc(1, sizeof(struct audio_data_s));
    if (!table) {
        ALOGE("%s: calloc audio_data_s failed!", __FUNCTION__);
    } else {
        p_attr->mdrc.crossover_table = table;
        strcpy(table->section_name, "drc_param:crossover_table");
        ITEM_LOGD("%s, section_name = %s\n", __FUNCTION__, table->section_name);
        parse_audio_table_data(pIniParser, table);
        PrintRegData(p_attr->mdrc.drc_byte_mode, table);
    }

    return 0;
}

int handle_audio_sum_ini(const char *file_name, char *model_name, struct audio_file_config_s *dev_cfg)
{
    dictionary *ini = NULL;
    const char *ini_value = NULL;
    char buf[128];

    ini = iniparser_load(file_name);
    if (ini == NULL) {
        ALOGD("%s, INI load file error, use default file\n", __FUNCTION__);
        goto exit;
    }

    sprintf(buf, "%s:%s", model_name, dev_cfg->ini_header);
    ini_value = iniparser_getstring(ini, buf, NULL);

    if (ini_value == NULL || access(ini_value, F_OK) == -1) {
        ITEM_LOGD("%s, INI File is not exist, use default file\n", __FUNCTION__);
        goto exit;
    }

    memset(dev_cfg->ini_file, 0, sizeof(dev_cfg->ini_file));
    strcpy(dev_cfg->ini_file, ini_value);
    ITEM_LOGD("%s, INI File -> (%s)\n", __FUNCTION__, dev_cfg->ini_file);

    iniparser_freedict(ini);
    return 0;

exit:
    iniparser_freedict(ini);
    return -1;
}

int handle_audio_gain_ini(char *file_name, struct eq_drc_data *p_attr)
{
    dictionary *ini = NULL;

    ini = iniparser_load(file_name);
    if (ini == NULL) {
        ALOGD("%s, INI load file error!\n", __FUNCTION__);
        goto exit;
    }

    parse_audio_source_gain_data(ini, p_attr);
    parse_audio_post_gain_data(ini, p_attr);
    parse_audio_ng_data(ini, p_attr);

exit:
    iniparser_freedict(ini);

    return 0;
}

int handle_audio_eq_drc_ini(char *file_name, struct audio_eq_drc_info_s *p_attr)
{
    dictionary *ini = NULL;
    const char *ini_value = NULL;

    ini = iniparser_load(file_name);
    if (ini == NULL) {
        ALOGD("%s, INI load file error!\n", __FUNCTION__);
        goto exit;
    }

    parse_audio_volume_status(ini, p_attr);
    parse_audio_eq_status(ini, p_attr);
    parse_audio_drc_status(ini, p_attr);
    if (p_attr->eq.enable) {
        parse_audio_eq_data(ini, p_attr);
    }
    if (p_attr->fdrc.enable) {
        parse_audio_fdrc_data(ini, p_attr);
    }
    if (p_attr->mdrc.enable) {
        parse_audio_mdrc_data(ini, p_attr);
    }

exit:
    iniparser_freedict(ini);
    return 0;
}

