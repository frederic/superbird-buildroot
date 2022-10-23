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

#ifndef _AML_ALSA_MIXER_H_
#define _AML_ALSA_MIXER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Value of the Alsa Mixer Control Point
 **/
/* Audio i2s mute */
typedef enum MIXER_AUDIO_I2S_MUTE {
    I2S_MUTE_OFF = 0,
    I2S_MUTE_ON  = 1,
    I2S_MUTE_MAX,
} eMixerAudioI2sMute;

/* Audio spdif mute */
typedef enum MIXER_SPDIF_MUTE {
    SPDIF_MUTE_OFF = 0,
    SPDIF_MUTE_ON  = 1,
    SPDIF_MUTE_MAX,
} eMixerSpdifMute;

/* Audio In Source */
typedef enum MIXER_AUDIO_IN_SOURCE {
    AUDIOIN_SRC_LINEIN  = 0,
    AUDIOIN_SRC_ATV     = 1,
    AUDIOIN_SRC_HDMI    = 2,
    AUDIOIN_SRC_SPDIFIN = 3,
    AUDIOIN_SRC_MAX,
} eMixerAudioInSrc;

/* Audio I2SIN Audio Type */
typedef enum MIXER_I2SIN_AUDIO_TYPE {
    I2SIN_AUDIO_TYPE_LPCM      = 0,
    I2SIN_AUDIO_TYPE_NONE_LPCM = 1,
    I2SIN_AUDIO_TYPE_UN_KNOWN  = 2,
    I2SIN_AUDIO_TYPE_MAX,
} eMixerI2sInAudioType;

/* Audio SPDIFIN Audio Type */
typedef enum MIXER_SPDIFIN_AUDIO_TYPE {
    SPDIFIN_AUDIO_TYPE_LPCM   = 0,
    SPDIFIN_AUDIO_TYPE_AC3    = 1,
    SPDIFIN_AUDIO_TYPE_EAC3   = 2,
    SPDIFIN_AUDIO_TYPE_DTS    = 3,
    SPDIFIN_AUDIO_TYPE_DTSHD  = 4,
    SPDIFIN_AUDIO_TYPE_TRUEHD = 5,
    SPDIFIN_AUDIO_TYPE_PAUSE  = 6,
    SPDIFIN_AUDIO_TYPE_MAX,
} eMixerApdifinAudioType;

/* Hardware resample enable */
typedef enum MIXER_HW_RESAMPLE_ENABLE {
    HW_RESAMPLE_DISABLE = 0,
    HW_RESAMPLE_32K     = 1,
    HW_RESAMPLE_44K     = 2,
    HW_RESAMPLE_48K     = 3,
    HW_RESAMPLE_88K     = 4,
    HW_RESAMPLE_96K     = 5,
    HW_RESAMPLE_176K    = 6,
    HW_RESAMPLE_192K    = 7,
    HW_RESAMPLE_MAX,
} eMixerHwResample;

/* Output Swap */
typedef enum MIXER_OUTPUT_SWAP {
    OUTPUT_SWAP_LR = 0,
    OUTPUT_SWAP_LL = 1,
    OUTPUT_SWAP_RR = 2,
    OUTPUT_SWAP_RL = 3,
    OUTPUT_SWAP_MAX,
} eMixerOutputSwap;

struct aml_mixer_ctrl {
    int  ctrl_idx;
    char ctrl_name[50];
};

/*
 *  Alsa Mixer Control Command List
 **/
typedef enum AML_MIXER_CTRL_ID {
    AML_MIXER_ID_I2S_MUTE = 0,
    AML_MIXER_ID_SPDIF_MUTE,
    AML_MIXER_ID_HDMI_OUT_AUDIO_MUTE,
    AML_MIXER_ID_HDMI_ARC_AUDIO_ENABLE,
    AML_MIXER_ID_AUDIO_IN_SRC,
    AML_MIXER_ID_I2SIN_AUDIO_TYPE,
    AML_MIXER_ID_SPDIFIN_AUDIO_TYPE,
    AML_MIXER_ID_HW_RESAMPLE_ENABLE,
    AML_MIXER_ID_OUTPUT_SWAP,
    AML_MIXER_ID_HDMI_IN_AUDIO_STABLE,
    AML_MIXER_ID_HDMI_IN_SAMPLERATE,
    AML_MIXER_ID_HDMI_IN_CHANNELS,
    AML_MIXER_ID_HDMI_IN_FORMATS,
    AML_MIXER_ID_HDMI_ATMOS_EDID,
    AML_MIXER_ID_ATV_IN_AUDIO_STABLE,
    AML_MIXER_ID_SPDIF_FORMAT,
    AML_MIXER_ID_AV_IN_AUDIO_STABLE,
    AML_MIXER_ID_EQ_MASTER_VOLUME,
    AML_MIXER_ID_SPDIFIN_PAO,
    AML_MIXER_ID_SPDIFIN_SAMPLE_RATE,
    AML_MIXER_ID_HW_RESAMPLE_MODULE,
    AML_MIXER_ID_AUDIO_INSKEW,
    AML_MIXER_ID_TDMOUT_C_BINV,
	AML_MIXER_ID_HDMI_IN_I2SCLK,
    AML_MIXER_ID_MAX,
} eMixerCtrlID;

/*
 *tinymix "Audio spdif format" list
 */
enum AML_SPDIF_FORMAT {
    AML_STEREO_PCM = 0,
    AML_DTS_RAW_MODE = 1,
    AML_DOLBY_DIGITAL = 2,
    AML_DTS = 3,
    AML_DOLBY_DIGITAL_PLUS = 4,
    AML_DTS_HD = 5,
    AML_MULTI_CH_LPCM = 6,
    AML_TRUE_HD = 7,
    AML_DTS_HD_MA = 8,
    AML_HIGH_SR_STEREO_LPCM = 9,
};

struct aml_mixer_list {
    int  id;
    char mixer_name[50];
};


/*
 * get interface
 **/
int aml_mixer_ctrl_get_int(int mixer_id);
int aml_mixer_ctrl_get_enum_str_to_int(int mixer_id, int *ret);
int close_mixer_handle();

//int aml_mixer_ctrl_get_str(int mixer_id, char *value);
// or
#if 0
int aml_mixer_get_audioin_src(int mixer_id);
int aml_mixer_get_i2sin_type(int mixer_id);
int aml_mixer_get_spdifin_type(int mixer_id);
#endif

/*
 * set interface
 **/
int aml_mixer_ctrl_set_int(int mixer_id, int value);
int aml_mixer_ctrl_set_str(int mixer_id, char *value);

#ifdef __cplusplus
}
#endif

#endif
