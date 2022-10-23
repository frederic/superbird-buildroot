#ifndef _AUDIO_EQ_DRC_PARSER_H_
#define _AUDIO_EQ_DRC_PARSER_H_

#define CC_REG_DATA_MAX            (280)

enum {
    EQ_MODE_DESK = 0,
    EQ_MODE_WALL,
    EQ_MODE_CLASSIC,
    EQ_MODE_POP,
    EQ_MODE_JAZZ,
    EQ_MODE_ROCK,
    EQ_MODE_NORMAL,
    EQ_MODE_SOFT,
    EQ_MODE_BASS,
    EQ_MODE_AUTO,
    EQ_MODE_SRS_SURROUND_ON,
    EQ_MODE_SRS_SURROUND_OFF,
    EQ_MODE_NUM
};

struct audio_model_s {
    int support;
    int model; // 0:AD 1:BD
};

struct audio_source_gain_s {
    int support;
    float atv;
    float dtv;
    float hdmi;
    float av;
};

struct audio_post_gain_s {
    int support;
    float speaker;
    float spdif_arc;
    float headphone;
};

struct audio_volume_s {
    int support;
    int master;
    int ch1;
    int ch2;
};

struct audio_reg_s {
    int len;
    int addr;
    int data[CC_REG_DATA_MAX];
};

#define CC_AUDIO_EQ_REG_CNT_MAX    (32)

struct audio_eq_drc_reg_s {
    int reg_cnt;
    struct audio_reg_s regs[CC_AUDIO_EQ_REG_CNT_MAX];
};

struct audio_eq_drc_info_s {
    int version;

    struct audio_model_s mod;
    struct audio_source_gain_s s_gain;
    struct audio_post_gain_s p_gain;
    struct audio_volume_s volume;

    char eq_name[32];

    int eq_enable;
    int eq_mode;
    int eq_byte_mode;

    struct audio_eq_drc_reg_s eq_desk;
    struct audio_eq_drc_reg_s eq_wall;
    struct audio_eq_drc_reg_s eq_classic;
    struct audio_eq_drc_reg_s eq_pop;
    struct audio_eq_drc_reg_s eq_jazz;
    struct audio_eq_drc_reg_s eq_rock;
    struct audio_eq_drc_reg_s eq_normal;
    struct audio_eq_drc_reg_s eq_soft;
    struct audio_eq_drc_reg_s eq_bass;
    struct audio_eq_drc_reg_s eq_auto;
    struct audio_eq_drc_reg_s eq_srs_surround_on;
    struct audio_eq_drc_reg_s eq_srs_surround_off;

    int drc_enable;
    char drc_name[32];
    int drc_byte_mode;
    struct audio_eq_drc_reg_s drc_table;

    int aml_ng_enable;
    float aml_ng_level;
    int aml_ng_attrack_time;
    int aml_ng_release_time;
};

struct eq_drc_device_config_s {
    char ini_header[50];
    char ini_file[100];
    char eq[50];
    char drc[50];
    char eq_table[50];
    char drc_table[50];
    char model[50];
    char master_vol[50];
    char ch1_vol[50];
    char ch2_vol[50];
};
#ifdef __cplusplus
extern "C" {
#endif

int handle_audio_sum_ini(const char *file_name, char *model_name, struct eq_drc_device_config_s *dev_cfg);
int handle_audio_eq_drc_ini(char *file_name, struct audio_eq_drc_info_s *p_attr); // file_name is hdr ini file name
int handle_audio_eq_drc_ini_by_id(int proj_id, char *file_name, struct audio_eq_drc_info_s *p_attr); // file_name is summary ini file name

#ifdef __cplusplus
}
#endif

#endif //_AUDIO_EQ_DRC_PARSER_H_
