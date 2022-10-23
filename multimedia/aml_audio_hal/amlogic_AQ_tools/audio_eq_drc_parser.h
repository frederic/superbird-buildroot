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

#ifndef _AUDIO_EQ_DRC_PARSER_H_
#define _AUDIO_EQ_DRC_PARSER_H_

#include <stdbool.h>

#define MAX_NAME_SIZE 32
#define MAX_FILE_NAME_SIZE 128
#define MAX_EXT_AMP_NUM 8
#define MAX_EQ_NUM 16

/*ini file parser*/
struct audio_file_config_s {
    char ini_header[MAX_NAME_SIZE];
    char ini_file[MAX_FILE_NAME_SIZE];
};

/*software source gain in audio hal*/
struct audio_source_gain_s {
    bool enable;
    float atv;
    float dtv;
    float hdmi;
    float av;
};

/*software post gain in audio hal*/
struct audio_post_gain_s {
    bool enable;
    float speaker;
    float spdif_arc;
    float headphone;
};

/*software noise gate in audio hal*/
struct audio_noise_gate_s {
    bool aml_ng_enable;
    float aml_ng_level;
    int aml_ng_attack_time;
    int aml_ng_release_time;
};

/*hw volume control in soc or AMP*/
struct audio_volume_s {
    bool enable;
    int master;
    char master_name[MAX_NAME_SIZE];
    int ch1;
    char ch1_name[MAX_NAME_SIZE];
    int ch2;
    char ch2_name[MAX_NAME_SIZE];
};

#define CC_AUDIO_REG_DATA_MAX   (64)
#define CC_AUDIO_REG_CNT_MAX    (32)

struct audio_reg_s {
    int len;
    unsigned int data[CC_AUDIO_REG_DATA_MAX];
};
struct audio_data_s {
    int reg_cnt;
    char section_name[MAX_NAME_SIZE];
    struct audio_reg_s regs[CC_AUDIO_REG_CNT_MAX];
};

/*hw eq*/
struct audio_eq_s {
    bool enable;
    char eq_name[MAX_NAME_SIZE];
    char eq_enable_name[MAX_NAME_SIZE];
    int eq_byte_mode;
    char eq_table_name[MAX_NAME_SIZE];
    int eq_table_num;
    struct audio_data_s *eq_table[MAX_EQ_NUM];
};

/*hw fullband drc*/
struct audio_fdrc_s {
    bool enable;
    char fdrc_name[MAX_NAME_SIZE];
    char fdrc_enable_name[MAX_NAME_SIZE];
    int drc_byte_mode;
    char fdrc_table_name[MAX_NAME_SIZE];
    struct audio_data_s *fdrc_table;
};

/*clip threshold*/
struct audio_clip_s {
    bool enable;
    float clip_threshold;
    char clip_threshold_name[MAX_NAME_SIZE];
};

/*hw multiband drc*/
struct audio_mdrc_s {
    bool enable;
    char mdrc_enable_name[MAX_NAME_SIZE];
    int drc_byte_mode;
    char crossover_table_name[MAX_NAME_SIZE];
    struct audio_data_s *crossover_table;
    char mdrc_table_name[MAX_NAME_SIZE];
    struct audio_data_s *mdrc_table;
};

struct audio_eq_drc_info_s {
    struct audio_volume_s volume;
    struct audio_eq_s eq;
    struct audio_fdrc_s fdrc;
    struct audio_mdrc_s mdrc;
    struct audio_clip_s clip;
};

struct eq_drc_data {
    int     card;

    struct  audio_source_gain_s s_gain;
    struct  audio_post_gain_s p_gain;
    struct  audio_noise_gate_s noise_gate;

    struct  audio_eq_drc_info_s *aml_attr;
    int     ext_amp_num;
    struct  audio_eq_drc_info_s *ext_attr[MAX_EXT_AMP_NUM];
};

#ifdef __cplusplus
extern "C" {
#endif

int handle_audio_sum_ini(const char *file_name, char *model_name, struct audio_file_config_s *dev_cfg);
int handle_audio_gain_ini(char *file_name, struct eq_drc_data *p_attr);
int handle_audio_eq_drc_ini(char *file_name, struct audio_eq_drc_info_s *p_attr);

#ifdef __cplusplus
}
#endif

#endif //_AUDIO_EQ_DRC_PARSER_H_
