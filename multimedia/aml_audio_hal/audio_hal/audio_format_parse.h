/*
 * hardware/amlogic/audio/TvAudio/audio_format_parse.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#ifndef __AUDIO_FORMAT_PARSE_H__
#define __AUDIO_FORMAT_PARSE_H__

#include <system/audio.h>
#include <tinyalsa/asoundlib.h>
#include <aml_alsa_mixer.h>

/*IEC61937 package presamble Pc value 0-4bit */
enum IEC61937_PC_Value {
    IEC61937_NULL               = 0x00,          ///< NULL data
    IEC61937_AC3                = 0x01,          ///< AC-3 data
    IEC61937_DTS1               = 0x0B,          ///< DTS type I   (512 samples)
    IEC61937_DTS2               = 0x0C,          ///< DTS type II  (1024 samples)
    IEC61937_DTS3               = 0x0D,          ///< DTS type III (2048 samples)
    IEC61937_DTSHD              = 0x11,          ///< DTS HD data
    IEC61937_EAC3               = 0x15,          ///< E-AC-3 data
    IEC61937_TRUEHD             = 0x16,          ///< TrueHD data
    IEC61937_PAUSE              = 0x03,          ///< Pause
};

enum audio_type {
    LPCM = 0,
    AC3,
    EAC3,
    DTS,
    DTSHD,
    TRUEHD,
    PAUSE,
    DTSCD,
    MUTE,
};

#define PARSER_DEFAULT_PERIOD_SIZE  (1024)

/*Period of data burst in IEC60958 frames*/
#define AC3_PERIOD_SIZE  (6144)
#define EAC3_PERIOD_SIZE (24576)
#define MAT_PERIOD_SIZE  (61440)
#define THD_PERIOD_SIZE  (61440)

#define DTS1_PERIOD_SIZE (2048)
#define DTS2_PERIOD_SIZE (4096)
#define DTS3_PERIOD_SIZE (8192)
/*min DTSHD Period 2048; max DTSHD Period 65536*/
#define DTSHD_PERIOD_SIZE (2048)

enum parser_state {
    IEC61937_UNSYNC,
    IEC61937_SYNCING,
    IEC61937_SYNCED,
};


typedef struct audio_type_parse {
    struct pcm_config config_in;
    struct pcm *in;
    struct aml_mixer_handle *mixer_handle;
    unsigned int card;
    unsigned int device;
    unsigned int flags;

    int period_bytes;
    char *parse_buffer;

    int audio_type;
    int cur_audio_type;

    audio_channel_mask_t audio_ch_mask;

    int read_bytes;
    int package_size;

    int running_flag;
    // used for software detection
    int state;
    int parsed_size;
    audio_devices_t input_src;

} audio_type_parse_t;

int creat_pthread_for_audio_type_parse(
    pthread_t *audio_type_parse_ThreadID,
                     void **status,
                     struct aml_mixer_handle *mixer,
                     audio_devices_t input_src);
void exit_pthread_for_audio_type_parse(
    pthread_t audio_type_parse_ThreadID,
    void **status);

/*
 *@brief convert the audio type to android audio format
 */
audio_format_t audio_type_convert_to_android_audio_format_t(int codec_type);

/*
 *@brief convert the audio type to string format
 */
char* audio_type_convert_to_string(int s32AudioType);

/*
 *@brief convert android audio format to the audio type
 */
int android_audio_format_t_convert_to_andio_type(audio_format_t format);
/*
 *@brief get current android audio fromat from audio parser thread
 */
audio_format_t audio_parse_get_audio_type(audio_type_parse_t *status);
/*
 *@brief get current audio channel mask from audio parser thread
 */
audio_channel_mask_t audio_parse_get_audio_channel_mask(audio_type_parse_t *status);
/*
 *@brief gget current audio fromat from audio parser thread
 */
int audio_parse_get_audio_type_direct(audio_type_parse_t *status);
/*
 *@brief gget current audio type from buffer data
 */
int audio_type_parse(void *buffer, size_t bytes, int *package_size, audio_channel_mask_t *cur_ch_mask);

/*
 *@brief feed data to software format detection
 */
void feeddata_audio_type_parse(void **status, char * input, int size);


#endif
