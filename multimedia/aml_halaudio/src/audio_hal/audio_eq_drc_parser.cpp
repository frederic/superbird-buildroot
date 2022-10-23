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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "log.h"

//#include "tv_key.h"
//#include "project_summary.h"
#include "audio_eq_drc_parser.h"
#include "IniParser.h"

#undef  LOG_TAG
#define LOG_TAG "audio_eq_drc_parser"

#define ITEM_DEBUG

#ifdef ITEM_DEBUG
#define ITEM_LOGD(x...) ALOGD(x)
#define ITEM_LOGE(x...) ALOGE(x)
#else
#define ITEM_LOGD(x...)
#define ITEM_LOGE(x...)
#endif

static int handle_integrity_flag(IniParser* pIniParser);

static int handle_regs_data(IniParser* pIniParser, int max_reg_cnt, struct audio_reg_s *regs, char *section, char *name);
static int handle_one_audio_eq_drc_data(IniParser* pIniParser, struct audio_eq_drc_reg_s* p_eq_reg, char *section, char *name);

static int handle_audio_model_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr);

static int handle_audio_source_gain_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr);
static int handle_audio_post_gain_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr);
static int handle_audio_ng_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr);

static int handle_audio_volume_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr);

static int handle_audio_eq_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr);
static int handle_audio_drc_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr);

static void PrintRegData(int byte_mode, int reg_cnt, struct audio_reg_s* regs);

int handle_audio_sum_ini(const char *file_name, char *model_name, struct eq_drc_device_config_s *dev_cfg)
{
    IniParser* pIniParser = NULL;
    const char *ini_value = NULL;

    pIniParser = new IniParser();
    if (pIniParser->parse(file_name) < 0) {
        ALOGD("%s, INI load file error, use default file\n", __FUNCTION__);
        goto exit;
    }

    ini_value = pIniParser->GetString(model_name, dev_cfg->ini_header, NULL);
    if (ini_value == NULL || access(ini_value, F_OK) == -1) {
        ITEM_LOGD("%s, INI File is not exist, use default file\n", __FUNCTION__);
        goto exit;
    }
    memset(dev_cfg->ini_file, 0, sizeof(dev_cfg->ini_file));
    strcpy(dev_cfg->ini_file, ini_value);

exit:
    ITEM_LOGD("%s, INI File -> %s\n", __FUNCTION__, dev_cfg->ini_file);
    delete pIniParser;
    pIniParser = NULL;

    return 0;
}

int handle_audio_eq_drc_ini(char *file_name, struct audio_eq_drc_info_s *p_attr)
{
    int i = 0, tmp_ret = 0;
    IniParser* pIniParser = NULL;

    memset((void *)p_attr, 0, sizeof(struct audio_eq_drc_info_s));

    pIniParser = new IniParser();
    if (pIniParser->parse(file_name) < 0) {
        ALOGD("%s, ini load file error!\n", __FUNCTION__);
        delete pIniParser;
        pIniParser = NULL;
        return -1;
    }

    // handle integrity flag
 //   if (handle_integrity_flag(pIniParser) < 0) {
 //       delete pIniParser;
 //       pIniParser = NULL;
 //       return -1;
 //   }

    tmp_ret = 0;

    handle_audio_model_data(pIniParser, p_attr);

    handle_audio_source_gain_data(pIniParser, p_attr);
    handle_audio_post_gain_data(pIniParser, p_attr);

    handle_audio_ng_data(pIniParser, p_attr);
    handle_audio_volume_data(pIniParser, p_attr);

    handle_audio_eq_data(pIniParser, p_attr);
    handle_audio_drc_data(pIniParser, p_attr);

    delete pIniParser;
    pIniParser = NULL;

    return 0;
}
/*
int handle_audio_eq_drc_ini_by_id(int proj_id, char *file_name, struct audio_eq_drc_info_s *p_attr) {
    int i = 0, audio_cfg_cnt = 0, default_flag = 0;
    struct project_info_s *pInfoPtr = NULL;

    pInfoPtr = new struct project_info_s[CC_MAX_SUPPORT_PROJECT_CNT];
    if (pInfoPtr == NULL) {
        ALOGE("%s, malloc project info memory error!!!\n", __FUNCTION__);
        return -1;
    }

    ALOGD("%s, proj_id = %d, project_info_name is (%s)\n", __FUNCTION__, proj_id, file_name);

    default_flag = CC_AUD_EQ_DRC_PATH_MASK;
    audio_cfg_cnt = handle_get_project_all_info(file_name, &default_flag, pInfoPtr);
    ALOGD("%s, audio_cfg_cnt = %d, default flag (0x%08x)\n", __FUNCTION__, audio_cfg_cnt, default_flag);
    if (default_flag & CC_AUD_EQ_DRC_PATH_MASK) {
        audio_cfg_cnt += 1;
    }

    if (audio_cfg_cnt > 0) {
        //refresh current project id's audio ini
        if (proj_id >= 0 && proj_id < audio_cfg_cnt - 1) {
            ALOGD("%s, start refresh current project id(%d)'s audio ini path.\n", __FUNCTION__, proj_id);

            handle_audio_eq_drc_ini(pInfoPtr[proj_id].aud_eq_drc_ini_path, p_attr);
        } else {
            ALOGD("%s, use default audio ini path id(%d).\n", __FUNCTION__, audio_cfg_cnt - 1);

            if (default_flag & CC_AUD_EQ_DRC_PATH_MASK) {
                handle_audio_eq_drc_ini(pInfoPtr[audio_cfg_cnt - 1].aud_eq_drc_ini_path, p_attr);
            } else {
                delete pInfoPtr;
                pInfoPtr = NULL;

                ALOGE("%s, there is no default audio ini path.\n", __FUNCTION__);
                return -1;
            }
        }
    } else {
        delete pInfoPtr;
        pInfoPtr = NULL;

        ALOGE("%s, there is no audio ini path.\n", __FUNCTION__);
        return -1;
    }

    delete pInfoPtr;
    pInfoPtr = NULL;

    return 0;
}

static int handle_integrity_flag(IniParser* pIniParser) {
    const char *ini_value = NULL;

    ini_value = pIniParser->GetString("start", "start_tag", "null");
    ITEM_LOGD("%s, start_tag is (%s)\n", __FUNCTION__, ini_value);
    if (strcasecmp(ini_value, "amlogic_start")) {
        ALOGE("%s, start_tag (%s) is error!!!\n", __FUNCTION__, ini_value);
        return -1;
    }

    ini_value = pIniParser->GetString("end", "end_tag", "null");
    ITEM_LOGD("%s, end_tag is (%s)\n", __FUNCTION__, ini_value);
    if (strcasecmp(ini_value, "amlogic_end")) {
        ITEM_LOGE("%s, end_tag (%s) is error!!!\n", __FUNCTION__, ini_value);
        return -1;
    }

    return 0;
}
*/

static int handle_audio_model_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    const char *ini_value = NULL;

    if (!pIniParser->getSection(pIniParser, "model")) {
        ITEM_LOGD("%s, Section -> [model] is not exist\n", __FUNCTION__);
        p_attr->mod.support = 0;
        return 0;
    } else {
        p_attr->mod.support = 1;
    }

    ini_value = pIniParser->GetString("model", "id", "0");
    p_attr->mod.model = atoi(ini_value);
    if (p_attr->mod.model == 0) {
        ITEM_LOGD("%s, model is (AD)\n", __FUNCTION__);
    }
    else if (p_attr->mod.model == 1) {
        ITEM_LOGD("%s, model is (BD)\n", __FUNCTION__);
    }
    else {
        ITEM_LOGD("%s, model is unknown\n", __FUNCTION__);
    }
    
    return 0;
}

static int handle_audio_source_gain_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    const char *ini_value = NULL;

    if (!pIniParser->getSection(pIniParser, "source_gain")) {
        ITEM_LOGD("%s, Section -> [source_gain] is not exist\n", __FUNCTION__);
        p_attr->s_gain.support = 0;
        return 0;
    } else {
        p_attr->s_gain.support = 1;
    }

    ini_value = pIniParser->GetString("source_gain", "atv", "0.0");
    ITEM_LOGD("%s, atv is (%s)\n", __FUNCTION__, ini_value);
    p_attr->s_gain.atv = atof(ini_value);

    ini_value = pIniParser->GetString("source_gain", "dtv", "0.0");
    ITEM_LOGD("%s, dtv is (%s)\n", __FUNCTION__, ini_value);
    p_attr->s_gain.dtv = atof(ini_value);

    ini_value = pIniParser->GetString("source_gain", "hdmi", "0.0");
    ITEM_LOGD("%s, hdmi is (%s)\n", __FUNCTION__, ini_value);
    p_attr->s_gain.hdmi = atof(ini_value);

    ini_value = pIniParser->GetString("source_gain", "av", "0.0");
    ITEM_LOGD("%s, av is (%s)\n", __FUNCTION__, ini_value);
    p_attr->s_gain.av = atof(ini_value);

    return 0;
}

static int handle_audio_post_gain_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    const char *ini_value = NULL;

    if (!pIniParser->getSection(pIniParser, "post_gain")) {
        ITEM_LOGD("%s, Section -> [post_gain] is not exist\n", __FUNCTION__);
        p_attr->p_gain.support = 0;
        return 0;
    } else {
        p_attr->p_gain.support = 1;
    }

    ini_value = pIniParser->GetString("post_gain", "speaker", "0.0");
    ITEM_LOGD("%s, speaker is (%s)\n", __FUNCTION__, ini_value);
    p_attr->p_gain.speaker = atof(ini_value);

    ini_value = pIniParser->GetString("post_gain", "spdif_arc", "0.0");
    ITEM_LOGD("%s, spdif_arc is (%s)\n", __FUNCTION__, ini_value);
    p_attr->p_gain.spdif_arc = atof(ini_value);

    ini_value = pIniParser->GetString("post_gain", "headphone", "0.0");
    ITEM_LOGD("%s, headphone is (%s)\n", __FUNCTION__, ini_value);
    p_attr->p_gain.headphone = atof(ini_value);

    return 0;
}

static int handle_audio_volume_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    const char *ini_value = NULL;

    if (!pIniParser->getSection(pIniParser, "volume")) {
        ITEM_LOGD("%s, Section -> [volume] is not exist\n", __FUNCTION__);
        p_attr->volume.support = 0;
        return 0;
    } else {
        p_attr->volume.support = 1;
    }

    ini_value = pIniParser->GetString("volume", "master", "0");
    ITEM_LOGD("%s, Master Volume is (%s)\n", __FUNCTION__, ini_value);
    p_attr->volume.master = atof(ini_value);

    ini_value = pIniParser->GetString("volume", "ch1", "0");
    ITEM_LOGD("%s, CH1 Volume is (%s)\n", __FUNCTION__, ini_value);
    p_attr->volume.ch1 = atof(ini_value);

    ini_value = pIniParser->GetString("volume", "ch2", "0");
    ITEM_LOGD("%s, CH2 Volume is (%s)\n", __FUNCTION__, ini_value);
    p_attr->volume.ch2 = atof(ini_value);

    return 0;
}

static int handle_audio_ng_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    const char *ini_value = NULL;
    if (!pIniParser->getSection(pIniParser, "noise_gate")) {
        ITEM_LOGD("%s, Section -> [noise_gate] is not exist\n", __FUNCTION__);
        return 0;
    }
    ini_value = pIniParser->GetString("noise_gate", "ng_enable", "0");
    ITEM_LOGD("%s, noise gate enable is (%s)\n", __FUNCTION__, ini_value);
    p_attr->aml_ng_enable = strtoul(ini_value, NULL, 0);
    ini_value = pIniParser->GetString("noise_gate", "ng_level", "-75.0");
    ITEM_LOGD("%s, noise gate level is (%s)\n", __FUNCTION__, ini_value);
    p_attr->aml_ng_level = atof(ini_value);
    ini_value = pIniParser->GetString("noise_gate", "ng_attrack_time", "3000");
    ITEM_LOGD("%s, noise gate attrack time (%s)\n", __FUNCTION__, ini_value);
    p_attr->aml_ng_attrack_time = strtoul(ini_value, NULL, 0);
    ini_value = pIniParser->GetString("noise_gate", "ng_release_time", "50");
    ITEM_LOGD("%s, noise gate release time (%s)\n", __FUNCTION__, ini_value);
    p_attr->aml_ng_release_time = strtoul(ini_value, NULL, 0);
    return 0;
}

static int handle_audio_eq_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    int tmp_ret = 0;
    const char *ini_value = NULL;

    ini_value = pIniParser->GetString("eq_param", "eq_name", "0");
    ITEM_LOGD("%s, eq_name is (%s)\n", __FUNCTION__, ini_value);
    strcpy((char *)p_attr->eq_name, ini_value);

    ini_value = pIniParser->GetString("eq_param", "eq_enable", "0");
    ITEM_LOGD("%s, eq_enable is (%s)\n", __FUNCTION__, ini_value);
    p_attr->eq_enable = strtoul(ini_value, NULL, 0);

    ini_value = pIniParser->GetString("eq_param", "eq_mode", "0");
    ITEM_LOGD("%s, eq_mode is (%s)\n", __FUNCTION__, ini_value);
    p_attr->eq_mode = strtoul(ini_value, NULL, 0);

    ini_value = pIniParser->GetString("eq_param", "eq_byte_mode", "0");
    ITEM_LOGD("%s, eq_byte_mode is (%s)\n", __FUNCTION__, ini_value);
    p_attr->eq_byte_mode = strtoul(ini_value, NULL, 0);

    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_desk, (char *)"eq_param", (char *)"desk_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_wall, (char *)"eq_param", (char *)"wall_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_classic, (char *)"eq_param", (char *)"classic_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_pop, (char *)"eq_param", (char *)"pop_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_jazz, (char *)"eq_param", (char *)"jazz_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_rock, (char *)"eq_param", (char *)"rock_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_normal, (char *)"eq_param", (char *)"normal_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_soft, (char *)"eq_param", (char *)"soft_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_bass, (char *)"eq_param", (char *)"bass_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_auto, (char *)"eq_param", (char *)"auto_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_srs_surround_on, (char *)"eq_param", (char *)"srs_surround_on_mode_eq_data");
    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->eq_srs_surround_off, (char *)"eq_param", (char *)"srs_surround_off_mode_eq_data");
#if 0
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_desk.reg_cnt, p_attr->eq_desk.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_wall.reg_cnt, p_attr->eq_wall.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_classic.reg_cnt, p_attr->eq_classic.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_pop.reg_cnt, p_attr->eq_pop.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_jazz.reg_cnt, p_attr->eq_jazz.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_rock.reg_cnt, p_attr->eq_rock.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_normal.reg_cnt, p_attr->eq_normal.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_soft.reg_cnt, p_attr->eq_soft.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_bass.reg_cnt, p_attr->eq_bass.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_auto.reg_cnt, p_attr->eq_auto.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_srs_surround_on.reg_cnt, p_attr->eq_srs_surround_on.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->eq_srs_surround_off.reg_cnt, p_attr->eq_srs_surround_off.regs);
#endif
    return tmp_ret;
}

static int handle_audio_drc_data(IniParser* pIniParser, struct audio_eq_drc_info_s* p_attr)
{
    int tmp_ret = 0;
    const char *ini_value = NULL;

    ini_value = pIniParser->GetString("drc_param", "drc_enable", "0");
    ITEM_LOGD("%s, drc_enable is (%s)\n", __FUNCTION__, ini_value);
    p_attr->drc_enable = strtoul(ini_value, NULL, 0);

    ini_value = pIniParser->GetString("drc_param", "drc_name", "0");
    ITEM_LOGD("%s, drc_name is (%s)\n", __FUNCTION__, ini_value);
    strcpy((char *)p_attr->drc_name, ini_value);

    ini_value = pIniParser->GetString("drc_param", "drc_byte_mode", "0");
    ITEM_LOGD("%s, drc_byte_mode is (%s)\n", __FUNCTION__, ini_value);
    p_attr->drc_byte_mode = strtoul(ini_value, NULL, 0);

    tmp_ret |= handle_one_audio_eq_drc_data(pIniParser, &p_attr->drc_table, (char *)"drc_param", (char *)"drc_table");
#if 0
    PrintRegData(p_attr->eq_byte_mode, p_attr->drc_ead.reg_cnt, p_attr->drc_ead.regs);
    PrintRegData(p_attr->eq_byte_mode, p_attr->drc_tko.reg_cnt, p_attr->drc_tko.regs);
#endif
    return tmp_ret;
}

template<typename T>
int transBufferData(const char *data_str, T data_buf[])
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

static int handle_regs_data(IniParser* pIniParser, int max_reg_cnt, struct audio_reg_s *regs, char *section, char *name)
{
    int i = 0, j = 0, k = 0, tmp_cnt = 0, tmp_line_cnt = 0;
    const char *ini_value = NULL;
    unsigned int tmp_buf[4096];

    ini_value = pIniParser->GetString(section, name, "null");
    //ITEM_LOGD("%s, [%s] %s = %s\n", __FUNCTION__, section, name, ini_value);

    tmp_cnt = transBufferData(ini_value, tmp_buf);
    ITEM_LOGD("%s, reg buffer data cnt = %d\n", __FUNCTION__, tmp_cnt);

    i = 0;
    j = 0;
    while (1) {
        if (j >= max_reg_cnt) {
            break;
        }

        regs[j].len = tmp_buf[i + 0];
        if (regs[j].len == 0) {
            break;
        }

        regs[j].addr = tmp_buf[i + 1];

        for (k = 0; k < regs[j].len; k++) {
            regs[j].data[k] = tmp_buf[i + 2 + k];
        }
        i += regs[j].len + 2;
        j += 1;
    }

    return j;
}

static int handle_one_audio_eq_drc_data(IniParser* pIniParser, struct audio_eq_drc_reg_s* p_reg, char *section, char *name)
{
    int tmp_ret = 0;

    memset((void *)p_reg, 0, sizeof(struct audio_eq_drc_reg_s));

    p_reg->reg_cnt = CC_AUDIO_EQ_REG_CNT_MAX;

    tmp_ret = handle_regs_data(pIniParser, p_reg->reg_cnt, p_reg->regs, section, name);
    if (tmp_ret <= 0 || tmp_ret > p_reg->reg_cnt) {
        memset((void *)p_reg, 0, sizeof(struct audio_eq_drc_reg_s));
        return -1;
    }

    p_reg->reg_cnt = tmp_ret;

    return 0;
}

static int ExportDataAsBin(const char *fname_str, void *data, int data_len)
{
    int i = 0, total_len = 0;
    int fd = -1;

    if (fname_str == NULL || data == NULL || data_len == 0) {
        return -1;
    }

    fd = open(fname_str, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        ALOGE("%s, Open %s ERROR(%s)!!\n", __FUNCTION__, fname_str, strerror(errno));
        return -1;
    }

    write(fd, data, data_len);

    close(fd);
    fd = -1;

    return 0;
}

static void PrintRegData(int byte_mode, int reg_cnt, struct audio_reg_s* regs)
{
    int i = 0, j = 0, tmp_len = 0;
    char tmp_buf[1024] = {'\0'};

    ITEM_LOGD("%s, reg_cnt = %d\n", __FUNCTION__, reg_cnt);
    memset(tmp_buf, 0, 1024);
    for (i = 0; i < reg_cnt; i++) {
        tmp_len = strlen(tmp_buf);
        sprintf((char *)tmp_buf + tmp_len, "%d, ", regs[i].len);

        tmp_len = strlen(tmp_buf);
        if (byte_mode == 4) {
            sprintf((char *)tmp_buf + tmp_len, "0x%08X, ", regs[i].addr);
        } else {
            sprintf((char *)tmp_buf + tmp_len, "0x%02X, ", regs[i].addr);
        }

        for (j = 0; j < regs[i].len; j++) {
            tmp_len = strlen(tmp_buf);
            if (byte_mode == 4) {
                sprintf((char *)tmp_buf + tmp_len, "0x%08X, ", regs[i].data[j]);
            } else {
                sprintf((char *)tmp_buf + tmp_len, "0x%02X, ", regs[i].data[j]);
            }
        }

        ITEM_LOGD("%s", tmp_buf);

        memset(tmp_buf, 0, 1024);
    }

    ITEM_LOGD("\n\n");
}
