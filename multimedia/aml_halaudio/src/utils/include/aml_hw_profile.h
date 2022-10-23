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

#ifndef _AML_HW_PROFILE_H_
#define _AML_HW_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CARD_NUM    2

#define SOUND_CARDS_PATH  "/proc/asound/cards"
#define SOUND_PCM_PATH    "/proc/asound/pcm"

#define PCM_I2S_STRING    "I2S"
#define PCM_SPDIF_STRING  "SPDIF"
#define PCM_PCM2BT_STRING "pcm2bt-pcm"

typedef enum PCM_STREAM_TYPE {
	AML_STREAM_TYPE_PLAYBACK = 0,
	AML_STREAM_TYPE_CAPTURE  = 1,
	AML_STREAM_TYPE_MAX,
} ePcmStreamType;


enum {
    TYPE_PCM = 0,
    TYPE_AC3 = 2,
    TYPE_DTS = 3,
    TYPE_EAC3 = 4,
    TYPE_DTS_HD = 5 ,
    TYPE_MULTI_PCM = 6,
    TYPE_TRUE_HD = 7,
    TYPE_DTS_HD_MA = 8,//should not used after we unify DTS-HD&DTS-HD MA
    TYPE_PCM_HIGH_SR = 9,
};

int aml_get_sound_card_main(void);
int aml_get_sound_card_ext(ePcmStreamType type);

int aml_get_i2s_port(void);
int aml_get_spdif_port(void);
int aml_get_pcm2bt_port(void);

char* aml_get_hdmi_sink_cap(const char *keys);
char* aml_get_hdmi_arc_cap(unsigned *ad, int maxsize, const char *keys);

void aml_set_codec_type(int type);
int  aml_get_codec_type(int format);


#ifdef __cplusplus
}
#endif

#endif
