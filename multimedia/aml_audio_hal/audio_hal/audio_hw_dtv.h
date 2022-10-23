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

#ifndef _AUDIO_HW_DTV_H_
#define _AUDIO_HW_DTV_H_

//{reference from " /amcodec/include/amports/aformat.h"
#define ACODEC_FMT_NULL -1
#define ACODEC_FMT_MPEG 0
#define ACODEC_FMT_PCM_S16LE 1
#define ACODEC_FMT_AAC 2
#define ACODEC_FMT_AC3 3
#define ACODEC_FMT_ALAW 4
#define ACODEC_FMT_MULAW 5
#define ACODEC_FMT_DTS 6
#define ACODEC_FMT_PCM_S16BE 7
#define ACODEC_FMT_FLAC 8
#define ACODEC_FMT_COOK 9
#define ACODEC_FMT_PCM_U8 10
#define ACODEC_FMT_ADPCM 11
#define ACODEC_FMT_AMR 12
#define ACODEC_FMT_RAAC 13
#define ACODEC_FMT_WMA 14
#define ACODEC_FMT_WMAPRO 15
#define ACODEC_FMT_PCM_BLURAY 16
#define ACODEC_FMT_ALAC 17
#define ACODEC_FMT_VORBIS 18
#define ACODEC_FMT_AAC_LATM 19
#define ACODEC_FMT_APE 20
#define ACODEC_FMT_EAC3 21
#define ACODEC_FMT_WIFIDISPLAY 22
#define ACODEC_FMT_DRA 23
#define ACODEC_FMT_TRUEHD 25
#define ACODEC_FMT_MPEG1 26 // AFORMAT_MPEG-->mp3,AFORMAT_MPEG1-->mp1,AFROMAT_MPEG2-->mp2
#define ACODEC_FMT_MPEG2 27
#define ACODEC_FMT_WMAVOI 28

//}

enum {
    AUDIO_DTV_PATCH_DECODER_STATE_INIT,
    AUDIO_DTV_PATCH_DECODER_STATE_START,
    AUDIO_DTV_PATCH_DECODER_STATE_RUNING,
    AUDIO_DTV_PATCH_DECODER_STATE_PAUSE,
    AUDIO_DTV_PATCH_DECODER_STATE_RESUME,
    AUDIO_DTV_PATCH_DECODER_RELEASE,
};

enum {
    AUDIO_DTV_PATCH_CMD_NULL,
    AUDIO_DTV_PATCH_CMD_START,
    AUDIO_DTV_PATCH_CMD_STOP,
    AUDIO_DTV_PATCH_CMD_PAUSE,
    AUDIO_DTV_PATCH_CMD_RESUME,
    AUDIO_DTV_PATCH_CMD_NUM,
};

int create_dtv_patch(struct audio_hw_device *dev, audio_devices_t input, audio_devices_t output __unused);
int release_dtv_patch(struct aml_audio_device *dev);
int release_dtv_patch_l(struct aml_audio_device *dev);
int dtv_patch_add_cmd(int cmd);
int dtv_in_read(struct audio_stream_in *stream, void* buffer, size_t bytes);
void dtv_in_write(struct audio_stream_out *stream, const void* buffer, size_t bytes);
void save_latest_dtv_aformat(int afmt);
int audio_set_spdif_clock(struct aml_stream_out *stream,int type);
#endif
