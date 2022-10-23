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

/**
 ** aml_alsa_mixer.c
 **
 ** This program is APIs for read/write mixers of alsa.
 ** author: shen pengru
 **
 */
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "log.h"
#include <fcntl.h>
#include <pthread.h>
#include <tinyalsa/asoundlib.h>
#include <aml_hw_profile.h>
#include <aml_alsa_mixer.h>

#undef  LOG_TAG
#define LOG_TAG "audio_alsa_mixer"

struct aml_mixer_handle {
    struct mixer *pMixer;
    pthread_mutex_t lock;
};


static struct aml_mixer_list gAmlMixerList[] = {
    /* for i2s out status */
    {AML_MIXER_ID_I2S_MUTE,             "Audio i2s mute"},
    /* for spdif out status */
    {AML_MIXER_ID_SPDIF_MUTE,           "Audio spdif mute"},
    /* for HDMI TX status */
    {AML_MIXER_ID_HDMI_OUT_AUDIO_MUTE,  "Audio hdmi-out mute"},
    /* for HDMI ARC status */
    {AML_MIXER_ID_HDMI_ARC_AUDIO_ENABLE, "HDMI ARC Switch"},
    {AML_MIXER_ID_AUDIO_IN_SRC,         "Audio In Source"},
    {AML_MIXER_ID_I2SIN_AUDIO_TYPE,     "I2SIN Audio Type"},
    {AML_MIXER_ID_SPDIFIN_AUDIO_TYPE,   "SPDIFIN Audio Type"},
    {AML_MIXER_ID_HW_RESAMPLE_ENABLE,   "Hardware resample enable"},
    {AML_MIXER_ID_OUTPUT_SWAP,          "Output Swap"},
    /* for HDMI RX status */
    {AML_MIXER_ID_HDMI_IN_AUDIO_STABLE, "HDMIIN audio stable"},
    {AML_MIXER_ID_HDMI_IN_SAMPLERATE,   "HDMIIN audio samplerate"},
    {AML_MIXER_ID_HDMI_IN_CHANNELS,     "HDMIIN audio channels"},
    {AML_MIXER_ID_HDMI_IN_FORMATS,      "HDMIIN audio format"},
    {AML_MIXER_ID_HDMI_ATMOS_EDID,      "HDMI ATMOS EDID Switch"},
    /* for ATV status */
    {AML_MIXER_ID_ATV_IN_AUDIO_STABLE,  "ATV audio stable"},
    {AML_MIXER_ID_SPDIF_FORMAT,         "Audio spdif format"},
    /* for AV status */
    {AML_MIXER_ID_AV_IN_AUDIO_STABLE,   "AV audio stable"},
    /* for Speaker master volume */
    {AML_MIXER_ID_EQ_MASTER_VOLUME,     "EQ master volume"},
    /* for enable PAO on SPDIFIN*/
    {AML_MIXER_ID_SPDIFIN_PAO,          "SPDIFIN PAO"},
    {AML_MIXER_ID_SPDIFIN_SAMPLE_RATE,  "SPDIFIN audio samplerate"},
    {AML_MIXER_ID_HW_RESAMPLE_MODULE,   "Hw resample module"},
    {AML_MIXER_ID_AUDIO_INSKEW,         "audio inskew set"},
    {AML_MIXER_ID_TDMOUT_C_BINV,        "tdmout_c binv set"},
	{AML_MIXER_ID_HDMI_IN_I2SCLK,      "I2SIn CLK"}
};

static struct aml_mixer_ctrl gCtlI2sMute[] = {
    {I2S_MUTE_ON,  "On"},
    {I2S_MUTE_OFF, "Off"},
};

static struct aml_mixer_ctrl gCtlSpdifMute[] = {
    {SPDIF_MUTE_ON,  "On"},
    {SPDIF_MUTE_OFF, "Off"},
};

static struct aml_mixer_ctrl gCtlAudioInSrc[] = {
    {AUDIOIN_SRC_LINEIN,  "LINEIN"},
    {AUDIOIN_SRC_ATV,     "ATV"},
    {AUDIOIN_SRC_HDMI,    "HDMI"},
    {AUDIOIN_SRC_SPDIFIN, "SPDIFIN"},
};

static struct aml_mixer_ctrl gCtlI2SInType[] = {
    {I2SIN_AUDIO_TYPE_LPCM,      "LPCM"},
    {I2SIN_AUDIO_TYPE_NONE_LPCM, "NONE-LPCM"},
    {I2SIN_AUDIO_TYPE_UN_KNOWN,  "UN-KNOW"},
};

static struct aml_mixer_ctrl gCtlSpdifInType[] = {
    {SPDIFIN_AUDIO_TYPE_LPCM,   "LPCM"},
    {SPDIFIN_AUDIO_TYPE_AC3,    "AC3"},
    {SPDIFIN_AUDIO_TYPE_EAC3,   "EAC3"},
    {SPDIFIN_AUDIO_TYPE_DTS,    "DTS"},
    {SPDIFIN_AUDIO_TYPE_DTSHD,  "DTS-HD"},
    {SPDIFIN_AUDIO_TYPE_TRUEHD, "TRUEHD"},
    {SPDIFIN_AUDIO_TYPE_PAUSE,  "PAUSE"},
};

static struct aml_mixer_ctrl gCtlHwResample[] = {
    {HW_RESAMPLE_DISABLE, "Disable"},
    {HW_RESAMPLE_32K,     "Enable:32K"},
    {HW_RESAMPLE_44K,     "Enable:44K"},
    {HW_RESAMPLE_48K,     "Enable:48K"},
    {HW_RESAMPLE_88K,     "Enable:88K"},
    {HW_RESAMPLE_96K,     "Enable:96K"},
    {HW_RESAMPLE_96K,     "Enable:176K"},
    {HW_RESAMPLE_96K,     "Enable:192K"},
};

static struct aml_mixer_ctrl gCtlOutputSwap[] = {
    {OUTPUT_SWAP_LR, "LR"},
    {OUTPUT_SWAP_LL, "LL"},
    {OUTPUT_SWAP_RR, "RR"},
    {OUTPUT_SWAP_RL, "RL"},
};


static struct aml_mixer_handle * mixer_handle = NULL;

static char *_get_mixer_name_by_id(int mixer_id)
{
    int i;
    int cnt_mixer = sizeof(gAmlMixerList) / sizeof(struct aml_mixer_list);

    for (i = 0; i < cnt_mixer; i++) {
        if (gAmlMixerList[i].id == mixer_id) {
            return gAmlMixerList[i].mixer_name;
        }
    }

    return NULL;
}

static struct mixer *_open_mixer_handle(void)
{
    int card = 0;
    struct mixer *pmixer = NULL;

    if (mixer_handle != NULL) {
        return mixer_handle->pMixer;
    }

    mixer_handle = calloc(1,sizeof(struct aml_mixer_handle));

    card = aml_get_sound_card_main();
    if (card < 0) {
        ALOGE("[%s:%d] Failed to get sound card\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    pmixer = mixer_open(card);
    if (NULL == pmixer) {
        ALOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    mixer_handle->pMixer = pmixer;
    pthread_mutex_init(&mixer_handle->lock, NULL);

    return pmixer;
}

int close_mixer_handle( )
{
    if (mixer_handle == NULL)
        return -1;
    mixer_close(mixer_handle->pMixer);
    free(mixer_handle);
    mixer_handle = NULL;
    return 0;
}

static struct mixer_ctl *_get_mixer_ctl_handle(struct mixer *pmixer, int mixer_id)
{
    struct mixer_ctl *pCtrl = NULL;

    if (_get_mixer_name_by_id(mixer_id) != NULL) {
        pCtrl = mixer_get_ctl_by_name(pmixer,
                                      _get_mixer_name_by_id(mixer_id));
    }

    return pCtrl;
}

int aml_mixer_ctrl_get_int(int mixer_id)
{
    struct mixer     *pMixer;
    struct mixer_ctl *pCtrl;
    int value = -1;

    pMixer = _open_mixer_handle();
    if (pMixer == NULL) {
        ALOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pthread_mutex_lock (&mixer_handle->lock);

    pCtrl = _get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        ALOGE("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              _get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock (&mixer_handle->lock);
        return -1;
    }

    value = mixer_ctl_get_value(pCtrl, 0);
    pthread_mutex_unlock (&mixer_handle->lock);
    return value;
}

int aml_mixer_ctrl_get_enum_str_to_int(int mixer_id, int *ret)
{
    struct mixer     *pMixer;
    struct mixer_ctl *pCtrl;
    const char *string = NULL;
    int value = -1;

    pMixer = _open_mixer_handle();
    if (pMixer == NULL) {
        ALOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pthread_mutex_lock (&mixer_handle->lock);

    pCtrl = _get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        ALOGE("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              _get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock (&mixer_handle->lock);
        return -1;
    }

    value = mixer_ctl_get_value(pCtrl, 0);
    string = mixer_ctl_get_enum_string(pCtrl, value);
    pthread_mutex_unlock (&mixer_handle->lock);
    
    if (string) {
        *ret = atoi(string);
        return 0;
    } else {
        return -1;
    }
}


int aml_mixer_ctrl_set_int(int mixer_id, int value)
{
    struct mixer     *pMixer;
    struct mixer_ctl *pCtrl;

    pMixer = _open_mixer_handle();
    if (pMixer == NULL) {
        ALOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pthread_mutex_lock (&mixer_handle->lock);

    pCtrl = _get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        ALOGE("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              _get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock (&mixer_handle->lock);
        return -1;
    }

    mixer_ctl_set_value(pCtrl, 0, value);
    pthread_mutex_unlock (&mixer_handle->lock);


    return 0;
}

int aml_mixer_ctrl_set_str(int mixer_id, char *value)
{
    struct mixer     *pMixer;
    struct mixer_ctl *pCtrl;

    pMixer = _open_mixer_handle();
    if (pMixer == NULL) {
        ALOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        return -1;
    }
    pthread_mutex_lock (&mixer_handle->lock);

    pCtrl = _get_mixer_ctl_handle(pMixer, mixer_id);
    if (pCtrl == NULL) {
        ALOGE("[%s:%d] Failed to open mixer %s\n", __FUNCTION__, __LINE__,
              _get_mixer_name_by_id(mixer_id));
        pthread_mutex_unlock (&mixer_handle->lock);
        return -1;
    }
    mixer_ctl_set_enum_by_string(pCtrl, value);
    pthread_mutex_unlock (&mixer_handle->lock);

    return 0;
}
