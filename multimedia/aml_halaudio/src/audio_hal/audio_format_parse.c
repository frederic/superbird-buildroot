/*
 * hardware/amlogic/audio/TvAudio/audio_format_parse.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#define LOG_TAG "audio_format_parse"
//#define LOG_NDEBUG 0

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "log.h"

#include "aml_audio_stream.h"
#include "audio_format_parse.h"
#include "aml_hw_profile.h"
#include "aml_dump_debug.h"
#include "ac3_parser_utils.h"

#include "aml_alsa_mixer.h"
#include "audio_hw_utils.h"

#define IEC61937_HEADER_BYTES 8
#define AUDIO_FORMAT_STRING(format) ((format) == TRUEHD) ? ("TRUEHD") : (((format) == AC3) ? ("AC3") : (((format) == EAC3) ? ("EAC3") : ("LPCM")))

/*Find the position of 61937 sync word in the buffer*/
static int seek_61937_sync_word(char *buffer, int size)
{
    int i = -1;
    if (size < 4) {
        return i;
    }

    //DoDumpData(buffer, size, CC_DUMP_SRC_TYPE_INPUT_PARSE);

    for (i = 0; i < (size - 3); i++) {
        //ALOGV("%s() 61937 sync word %x %x %x %x\n", __FUNCTION__, buffer[i + 0], buffer[i + 1], buffer[i + 2], buffer[i + 3]);
        if (buffer[i + 0] == 0x72 && buffer[i + 1] == 0xf8 && buffer[i + 2] == 0x1f && buffer[i + 3] == 0x4e) {
            return i;
        }
        if (buffer[i + 0] == 0x4e && buffer[i + 1] == 0x1f && buffer[i + 2] == 0xf8 && buffer[i + 3] == 0x72) {
            return i;
        }
    }
    return -1;
}

/*DTSCD format is a special dts format without 61937 header*/
static int seek_dts_cd_sync_word(char *buffer, int size)
{
    int i = -1;
    if (size < 6) {
        return i;
    }

    for (i = 0; i < (size - 5); i++) {
        if ((buffer[i + 0] == 0xFF && buffer[i + 1] == 0x1F
             && buffer[i + 2] == 0x00 && buffer[i + 3] == 0xE8
             && buffer[i + 4] == 0xF1 && buffer[i + 5] == 0x07) ||
            (buffer[i + 0] == 0xE8 && buffer[i + 1] == 0x00
             && buffer[i + 2] == 0x1F && buffer[i + 3] == 0xFF
             && buffer[i + 6] == 0x07 && buffer[i + 7] == 0xF1)) {
            return i;
        }
    }
    return -1;
}

static audio_channel_mask_t get_dolby_channel_mask(const unsigned char *frameBuf
        , int length)
{
    int scan_frame_offset;
    int scan_frame_size;
    int scan_channel_num;
    int scan_numblks;
    int scan_timeslice_61937;
    int scan_framevalid_flag;
    int ret = 0;
    int total_channel_num  = 0;

    if ((frameBuf == NULL) || (length <= 0)) {
        ret = -1;
    } else {
        ret = parse_dolby_frame_header(frameBuf, length, &scan_frame_offset, &scan_frame_size
                                       , &scan_channel_num, &scan_numblks, &scan_timeslice_61937, &scan_framevalid_flag);
        ALOGV("%s A:scan_channel_num %d scan_numblks %d scan_timeslice_61937 %d\n",
              __FUNCTION__, scan_channel_num, scan_numblks, scan_timeslice_61937);
    }

    if (ret) {
        return AUDIO_CHANNEL_OUT_STEREO;
    } else {
        total_channel_num += scan_channel_num;
        if (length - scan_frame_offset - scan_frame_size > 0) {
            ret = parse_dolby_frame_header(frameBuf + scan_frame_offset + scan_frame_size
                                           , length - scan_frame_offset - scan_frame_size, &scan_frame_offset, &scan_frame_size
                                           , &scan_channel_num, &scan_numblks, &scan_timeslice_61937, &scan_framevalid_flag);
            ALOGV("%s B:scan_channel_num %d scan_numblks %d scan_timeslice_61937 %d\n",
                  __FUNCTION__, scan_channel_num, scan_numblks, scan_timeslice_61937);
            if ((ret == 0) && (scan_timeslice_61937 == 3)) {
                /*dolby frame contain that DEPENDENT frame, that means 7.1ch*/
                total_channel_num = 8;
            }
        }
        return audio_channel_out_mask_from_count(total_channel_num);
    }
}

static int hw_audio_format_detection()
{
    int type = 0;
    type = aml_mixer_ctrl_get_int(AML_MIXER_ID_SPDIFIN_AUDIO_TYPE);
    if (type >= LPCM && type <= DTS) {
        return type;
    } else {
        return LPCM;
    }

}

int audio_type_parse(void *buffer, size_t bytes, int *package_size, audio_channel_mask_t *cur_ch_mask, int *raw_size, int *offset)
{
    int pos_sync_word = -1, pos_dtscd_sync_word = -1;
    char *temp_buffer = (char*)buffer;
    int AudioType = LPCM;
    uint32_t *tmp_pc = NULL;
    uint32_t *tmp_pd = NULL;
    uint32_t pc = 0;
    uint32_t tmp = 0;
    uint32_t pd = 0;

    pos_sync_word = seek_61937_sync_word((char*)temp_buffer, bytes);
    /*due to this dtc cd detection is not strong enough, it will detect
     some normal pcm as dts cd, which cause audio break*/
#if 0
    pos_dtscd_sync_word = seek_dts_cd_sync_word((char*)temp_buffer, bytes);

    DoDumpData(temp_buffer, bytes, CC_DUMP_SRC_TYPE_INPUT_PARSE);

    if (pos_dtscd_sync_word >= 0) {
        AudioType = DTSCD;
        *package_size = DTSHD_PERIOD_SIZE * 2;
        ALOGV("%s() %d data format: %d *package_size %d\n", __FUNCTION__, __LINE__, AudioType, *package_size);
        return AudioType;
    }
#endif
    if (pos_sync_word >= 0) {
        tmp_pc = (uint32_t*)(temp_buffer + pos_sync_word + 4);
        pc = *tmp_pc;
        *cur_ch_mask = AUDIO_CHANNEL_OUT_STEREO;
        /*Value of 0-4bit is data type*/
        //ALOGE("0xpc=%x size=0x%x pos=0x%x\n",pc,bytes,pos_sync_word);
        switch (pc & 0x1f) {
        case IEC61937_NULL:
            AudioType = MUTE;
            // not defined, suggestion is 4096 samples must have one
            *package_size = 4096 * 4;
            break;
        case IEC61937_AC3:
            AudioType = AC3;
            /* *cur_ch_mask = get_dolby_channel_mask((const unsigned char *)(temp_buffer + pos_sync_word + 8),
                    (bytes - pos_sync_word - 8));
            ALOGV("%d channel mask %#x", __LINE__, *cur_ch_mask);*/
            *package_size = AC3_PERIOD_SIZE;
            break;
        case IEC61937_EAC3:
            AudioType = EAC3;
            /* *cur_ch_mask = get_dolby_channel_mask((const unsigned char *)(temp_buffer + pos_sync_word + 8),
                    (bytes - pos_sync_word - 8));
            ALOGV("%d channel mask %#x", __LINE__, *cur_ch_mask);*/
            *package_size = EAC3_PERIOD_SIZE;
            break;
        case IEC61937_DTS1:
            AudioType = DTS;
            *package_size = DTS1_PERIOD_SIZE;
            break;
        case IEC61937_DTS2:
            AudioType = DTS;
            *package_size = DTS2_PERIOD_SIZE;
            break;
        case IEC61937_DTS3:
            AudioType = DTS;
            *package_size = DTS3_PERIOD_SIZE;
            break;
        case IEC61937_DTSHD:
            /*Value of 8-12bit is framesize*/
            tmp = (pc & 0x7ff) >> 8;
            AudioType = DTSHD;
            /*refer to IEC 61937-5 pdf, table 6*/
            *package_size = DTSHD_PERIOD_SIZE << tmp ;
            break;
        case IEC61937_TRUEHD:
            AudioType = TRUEHD;
            *package_size = THD_PERIOD_SIZE;
            break;
        case IEC61937_PAUSE:
            AudioType = PAUSE;
            // Not defined, set it as 1024*4
            *package_size = 1024 * 4;
            break;
        default:
            AudioType = MUTE;
            break;
        }
    }

    if (pos_sync_word >= 0) {
        unsigned int idx, data_offset;
        char *p_data = (char *)buffer;

        data_offset = pos_sync_word + 8;
        tmp_pd = (uint32_t *)(temp_buffer + pos_sync_word + 6);
        pd = *tmp_pd & 0xffff;
        *raw_size = pd;

        if (AudioType == AC3) {
            *raw_size = *raw_size / 8;
        }

        if ((p_data[data_offset] == 0x07 && p_data[data_offset + 1] == 0x9e) ||
            (p_data[data_offset] == 0x9e && p_data[data_offset + 1] == 0x07)) {
            AudioType = TRUEHD;
        }
    }

    *offset = pos_sync_word;
    return AudioType;
}

static int audio_type_parse_init(audio_type_parse_t *status)
{
    audio_type_parse_t *audio_type_status = status;
    struct pcm_config *config_in = &(audio_type_status->config_in);
    struct pcm *in;
    int ret, bytes;

    audio_type_status->card = (unsigned int)aml_get_sound_card_main();
    audio_type_status->device = (unsigned int)aml_get_i2s_port();
    audio_type_status->flags = PCM_IN;

    if (config_in->channels == 0) {
        config_in->channels = 2;
    }
    if (config_in->rate == 0) {
        config_in->rate = 48000;
    }
    if (config_in->period_size == 0) {
        config_in->period_size = PARSER_DEFAULT_PERIOD_SIZE * 4;
    }
    if (config_in->period_count == 0) {
        config_in->period_count = 4;
    }
    config_in->format = PCM_FORMAT_S16_LE;

    bytes = config_in->period_size * config_in->channels * 2;
    audio_type_status->period_bytes = bytes;

    /*malloc max audio type size, save last 3 byte*/
    audio_type_status->parse_buffer = (char*) malloc(sizeof(char) * (bytes + 16));
    if (NULL == audio_type_status->parse_buffer) {
        ALOGE("%s, no memory\n", __FUNCTION__);
        return -1;
    }

    in = pcm_open(audio_type_status->card, audio_type_status->device,
                  PCM_IN, &audio_type_status->config_in);
    if (!pcm_is_ready(in)) {
        ALOGE("open device failed: %s\n", pcm_get_error(in));
        pcm_close(in);
        goto error;
    }

    audio_type_status->in = in;
    enable_HW_resample(config_in->rate, 1);

    ALOGD("init parser success: (%d), (%d), (%p)",
          audio_type_status->card, audio_type_status->device, audio_type_status->in);
    return 0;
error:
    free(audio_type_status->parse_buffer);
    return -1;
}

static int audio_type_parse_release(audio_type_parse_t *status)
{
    audio_type_parse_t *audio_type_status = status;

    pcm_close(audio_type_status->in);
    audio_type_status->in = NULL;
    free(audio_type_status->parse_buffer);

    return 0;
}


static int update_audio_type(audio_type_parse_t *status, int update_bytes)
{
    audio_type_parse_t *audio_type_status = status;
    struct pcm_config *config_in = &(audio_type_status->config_in);

    if (audio_type_status->audio_type == audio_type_status->cur_audio_type) {
        audio_type_status->read_bytes = 0;
        return 0;
    }
    if (audio_type_status->audio_type != LPCM && audio_type_status->cur_audio_type == LPCM) {
        /* check 2 period size of IEC61937 burst data to find syncword*/
        if (audio_type_status->read_bytes > (audio_type_status->package_size * 2)) {
            audio_type_status->audio_type = audio_type_status->cur_audio_type;
            audio_type_status->read_bytes = 0;
            ALOGI("no IEC61937 header found, PCM data!\n");
            enable_HW_resample(config_in->rate, 1);
        }
        audio_type_status->read_bytes += update_bytes;
    } else {
        /* if find 61937 syncword or raw audio type changed,
        immediately update audio type*/
        audio_type_status->audio_type = audio_type_status->cur_audio_type;
        audio_type_status->read_bytes = 0;
        ALOGI("Raw data found: type(%d)\n", audio_type_status->audio_type);
        enable_HW_resample(config_in->rate, 0);
    }
    return 0;
}

void* audio_type_parse_threadloop(void *data)
{
    audio_type_parse_t *audio_type_status = (audio_type_parse_t *)data;
    int bytes = 0;
    int ret = -1;
    int raw_size = 0;
    int offset = 0;


    int txlx_chip = is_txlx_chip();

    ret = audio_type_parse_init(audio_type_status);
    if (ret < 0) {
        ALOGE("fail to init parser\n");
        return ((void *) 0);
    }

    prctl(PR_SET_NAME, (unsigned long)"audio_type_parse");

    bytes = audio_type_status->period_bytes;

    ALOGV("Start thread loop for android audio data parse! data = %p, bytes = %d, in = %p\n",
          data, bytes, audio_type_status->in);

    while (audio_type_status->running_flag && audio_type_status->in != NULL) {
        ret = pcm_read(audio_type_status->in, audio_type_status->parse_buffer + 3, bytes);
        if (ret >= 0) {
#if 1
            if (getprop_bool("media.audiohal.parsedump")) {
                FILE *dump_fp = NULL;
                dump_fp = fopen("/data/audio_hal/audio_parse.dump", "a+");
                if (dump_fp != NULL) {
                    fwrite(audio_type_status->parse_buffer + 3, bytes, 1, dump_fp);
                    fclose(dump_fp);
                } else {
                    ALOGW("[Error] Can't write to /data/audio_hal/audio_parse.dump");
                }
            }
#endif
            audio_type_status->cur_audio_type =
                audio_type_parse(
                    audio_type_status->parse_buffer
                    , bytes
                    , &(audio_type_status->package_size)
                    , &(audio_type_status->audio_ch_mask)
                    , &raw_size
                    , &offset);
            // ALOGD("cur_audio_type=%d\n", audio_type_status->cur_audio_type);
            //for txl chip,the PAO sw audio format detection is not ready yet.
            //we use the hw audio format detection.
            //TODO
            if (!txlx_chip && audio_type_status->cur_audio_type == LPCM) {
                audio_type_status->cur_audio_type = hw_audio_format_detection();
            }

            memcpy(audio_type_status->parse_buffer, audio_type_status->parse_buffer + bytes, 3);
            update_audio_type(audio_type_status, bytes);
        } else {
            usleep(5 * 1000);
            //ALOGE("fail to read bytes = %d\n", bytes);
        }
    }

    audio_type_parse_release(audio_type_status);

    ALOGI("Exit thread loop for audio type parse!\n");
    return ((void *) 0);
}

int creat_pthread_for_audio_type_parse(
    pthread_t *audio_type_parse_ThreadID,
    void **status,
    int input_sr __unused, int input_ch __unused)
{
    pthread_attr_t attr;
    struct sched_param param;
    audio_type_parse_t *audio_type_status = NULL;
    int ret;

    if (*status) {
        ALOGE("Aml TV audio format check is exist!");
        return -1;
    }

    audio_type_status = (audio_type_parse_t*) malloc(sizeof(audio_type_parse_t));
    if (NULL == audio_type_status) {
        ALOGE("%s, no memory\n", __FUNCTION__);
        return -1;
    }

    memset(audio_type_status, 0, sizeof(audio_type_parse_t));
    audio_type_status->running_flag = 1;
    audio_type_status->audio_type = LPCM;
    audio_type_status->audio_ch_mask = AUDIO_CHANNEL_OUT_STEREO;

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = 50;//sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(audio_type_parse_ThreadID, &attr,
                         &audio_type_parse_threadloop, audio_type_status);
    pthread_attr_destroy(&attr);
    if (ret != 0) {
        ALOGE("%s, Create thread fail!\n", __FUNCTION__);
        free(audio_type_status);
        return -1;
    }

    ALOGI("Creat thread ID: %lu! audio_type_status: %p\n", *audio_type_parse_ThreadID, audio_type_status);
    *status = audio_type_status;
    return 0;
}

void exit_pthread_for_audio_type_parse(
    pthread_t audio_type_parse_ThreadID,
    void **status)
{
    audio_type_parse_t *audio_type_status = (audio_type_parse_t *)(*status);
    if (audio_type_status == NULL) {
        ALOGD("parse is not existed\n");
        return;
    }
    audio_type_status->running_flag = 0;
    pthread_join(audio_type_parse_ThreadID, NULL);
    free(audio_type_status);
    *status = NULL;
    ALOGI("Exit parse thread,thread ID: %ld!\n", audio_type_parse_ThreadID);
    return;
}

/*
 *@brief convert the audio type to android audio format
 */
audio_format_t andio_type_convert_to_android_audio_format_t(int codec_type)
{
    switch (codec_type) {
    case AC3:
        return AUDIO_FORMAT_AC3;
    case EAC3:
        return AUDIO_FORMAT_E_AC3;
    case DTS:
    case DTSCD:
        return AUDIO_FORMAT_DTS;
    case DTSHD:
        return AUDIO_FORMAT_DTS_HD;
    case TRUEHD:
        return AUDIO_FORMAT_DOLBY_TRUEHD;
    case LPCM:
        return AUDIO_FORMAT_PCM_16_BIT;
    default:
        return AUDIO_FORMAT_PCM_16_BIT;
    }
}

/*
 *@brief convert android audio format to the audio type
 */
int android_audio_format_t_convert_to_andio_type(audio_format_t format)
{
    switch (format) {
    case AUDIO_FORMAT_AC3:
        return AC3;
    case AUDIO_FORMAT_E_AC3:
        return EAC3;
    case AUDIO_FORMAT_DTS:
        return  DTS;//DTSCD;
    case AUDIO_FORMAT_DTS_HD:
        return DTSHD;
    case AUDIO_FORMAT_DOLBY_TRUEHD:
        return TRUEHD;
    case AUDIO_FORMAT_PCM:
        return LPCM;
    default:
        return LPCM;
    }
}

int audio_parse_get_audio_type_direct(audio_type_parse_t *status)
{
    if (!status) {
        ALOGE("NULL pointer of audio_type_parse_t\n");
        return -1;
    }
    return status->audio_type;
}

audio_format_t audio_parse_get_audio_type(audio_type_parse_t *status)
{
    if (!status) {
        ALOGE("NULL pointer of audio_type_parse_t\n");
        return AUDIO_FORMAT_INVALID;
    }
    if (status->audio_type == PAUSE || status->audio_type == MUTE) {
        return AUDIO_FORMAT_INVALID;
    }
    return andio_type_convert_to_android_audio_format_t(status->audio_type);
}

audio_channel_mask_t audio_parse_get_audio_channel_mask(audio_type_parse_t *status)
{
    if (!status) {
        ALOGE("NULL pointer of audio_type_parse_t, return AUDIO_CHANNEL_OUT_STEREO\n");
        return AUDIO_CHANNEL_OUT_STEREO;
    }
    return status->audio_ch_mask;
}


enum parser_state {
    IEC61937_UNSYNC,
    IEC61937_SYNCING,
    IEC61937_SYNCED,
};

int creat_audio_type_parse(void **status, int iec_check_size, int type_preset)
{
    audio_type_parse_t *audio_type_status = NULL;
    int ret;
    int period_size = IEC61937_CHECK_SIZE;

    if (*status) {
        ALOGE("Aml TV audio format check is exist!");
        return -1;
    }

    audio_type_status = (audio_type_parse_t*) malloc(sizeof(audio_type_parse_t));
    if (NULL == audio_type_status) {
        ALOGE("%s, no memory\n", __FUNCTION__);
        return -1;
    }

    memset(audio_type_status, 0, sizeof(audio_type_parse_t));
    audio_type_status->running_flag = 1;
    audio_type_status->audio_type = LPCM;
    audio_type_status->audio_ch_mask = AUDIO_CHANNEL_OUT_STEREO;

    audio_type_status->period_bytes = period_size;
    audio_type_status->state = IEC61937_UNSYNC;
    audio_type_status->audio_type = MUTE;
    audio_type_status->cur_audio_type = MUTE;
    audio_type_status->iec_check_size = iec_check_size;
    audio_type_status->type_preset = type_preset;
    /*malloc max audio type size, save last 3 byte*/
    audio_type_status->parse_buffer = (char*) malloc(sizeof(char) * (period_size));
    if (NULL == audio_type_status->parse_buffer) {
        ALOGE("%s, no memory\n", __FUNCTION__);
        return -1;
    }

    *status = audio_type_status;
    return 0;
}


void release_audio_type_parse(void **status)
{
    audio_type_parse_t *audio_type_status = (audio_type_parse_t *)(*status);

    if (audio_type_status == NULL) {
        ALOGD("parse is not existed\n");
        return;
    }

    if (audio_type_status->parse_buffer) {
        free(audio_type_status->parse_buffer);
        audio_type_status->parse_buffer = NULL;
    }
    free(audio_type_status);
    *status = NULL;

    return;
}

void feeddata_audio_type_parse(void **status, char * input, int size)
{
    audio_type_parse_t *audio_type_status = (audio_type_parse_t *)(*status);
    int type = 0;
    int raw_size = 0;
    int offset = 0;
    int i = 0;
    int no_zero = 0;
    if (audio_type_status == NULL) {
        ALOGD("parse is not existed\n");
        return;
    }

    for (i = 0; i < size; i++) {
        if (input[i] != 0 ) {
            no_zero++;
        }
    }
    if ((no_zero == 0) && (audio_type_status->audio_type == MUTE) && (audio_type_status->parsed_size == 0)) {
        ALOGV("Detect data is all zero keep the original detecting");
        return;
    }

    type = audio_type_parse(input, size, &(audio_type_status->package_size), &(audio_type_status->audio_ch_mask), &raw_size, &offset);
    // if (type == TRUEHD)
    //     ALOGI("parser audio type %s\n", AUDIO_FORMAT_STRING(type));
    switch (audio_type_status->state) {
    case IEC61937_UNSYNC: {
        if (type == LPCM) {
            if (audio_type_status->audio_type == MUTE) {
                audio_type_status->parsed_size += size;
                if (audio_type_status->parsed_size <= audio_type_status->iec_check_size) {
                    // during IEC header finding,we mute it
                    memset(input, 0, size);
                    audio_type_status->audio_type = MUTE;
                } else {
                    // we alread checked some bytes, not found IEC header, we will treate it as PCM
                    audio_type_status->audio_type = LPCM;
                }
            } else {
                audio_type_status->parsed_size = 0;
                audio_type_status->audio_type = LPCM;
            }

        } else if (type == PAUSE || type == MUTE) {
            // set all the data as 0, keep the original data type
            memset(input, 0, size);
            audio_type_status->parsed_size = 0;
            audio_type_status->audio_type = MUTE;

        } else {
            audio_type_status->state = IEC61937_SYNCING;
            memset(input, 0, size);
            audio_type_status->audio_type = type;
            audio_type_status->parsed_size = 0;
        }
        break;
    }
    case IEC61937_SYNCING: {
        if (type == LPCM) {
            audio_type_status->parsed_size += size;
            // during two package, we don't find a new IEC header
            if (audio_type_status->parsed_size > audio_type_status->package_size * 2) {
                // no found new IEC header, back to unsync state
                audio_type_status->state = IEC61937_UNSYNC;
                audio_type_status->audio_type = MUTE;
                audio_type_status->parsed_size = 0;
            }

        } else if (type == PAUSE || type == MUTE) {
            memset(input, 0, size);
            audio_type_status->state = IEC61937_UNSYNC;
            audio_type_status->audio_type = MUTE;
            audio_type_status->parsed_size = 0;
        } else {
            // find a new IEC header, we set it as synced
            audio_type_status->audio_type = type;
            audio_type_status->state = IEC61937_SYNCED;
            audio_type_status->parsed_size = 0;

        }
        // mute all the data during syncing
        // memset(input, 0, size);
        break;
    }
    case IEC61937_SYNCED: {
        if (type == LPCM) {
            audio_type_status->parsed_size += size;
            // from raw to pcm, we check more data
            if (audio_type_status->parsed_size > audio_type_status->package_size * 2) {
                // no found new IEC header, back to unsync state
                audio_type_status->state = IEC61937_UNSYNC;
                audio_type_status->audio_type = MUTE;
                audio_type_status->parsed_size = 0;
            }

        } else if (type == PAUSE || type == MUTE) {
            memset(input, 0, size);
            audio_type_status->state = IEC61937_UNSYNC;
            audio_type_status->audio_type = MUTE;
            audio_type_status->parsed_size = 0;

        } else {
            audio_type_status->audio_type = type;
            audio_type_status->parsed_size = 0;
        }
        break;
    }
    default:
        return;
    }

    /*if we know it is bitstream, but we detect it as pcm,
      mute these data*/
    if (audio_type_status->type_preset == 1 && audio_type_status->audio_type == LPCM) {
        ALOGV("input is bitstream, but wrong detect type to pcm\n");
        memset(input, 0, size);
        audio_type_status->parsed_size = 0;
        audio_type_status->audio_type = MUTE;
    }

#if 0
    if (audio_type_status->audio_type == LPCM) {
        int i = 0;
        int found = 0;
        char * data = (char *)input;
        for (i = 0; i < size; i++) {
            if (data[i] != 0) {
                ALOGF("Noise data is found i=%d 0x%hx \n", i, data[i]);
                found = 1;
                break;
            }
        }
        if (0) {
            {
                FILE *dump_fp = NULL;
                dump_fp = fopen("/tmp/noise.data", "a+");
                if (dump_fp != NULL) {
                    fwrite(input, size, 1, dump_fp);
                    fclose(dump_fp);
                } else {
                    ALOGW("[Error] Can't write to /data/audio_hal/audioraw.dump");
                }
            }



        }
    }
#endif
    if (audio_type_status->audio_type != audio_type_status->cur_audio_type) {
        ALOGE("Parsing state=%d cur type=%d type=%d parse type=%d  preset=%d\n", audio_type_status->state,audio_type_status->cur_audio_type, audio_type_status->audio_type, type,audio_type_status->type_preset);
        audio_type_status->cur_audio_type = audio_type_status->audio_type;
    }
    return;
}
static bool is_datmos_suitable_format(int audio_type)
{
    return ((audio_type == TRUEHD) || (audio_type == AC3) || (audio_type == EAC3));
}

//suppose the raw_buf has enough space!
int decode_IEC61937_to_raw_data(char *buffer
                                , size_t bytes
                                , char *raw_buf
                                , size_t *raw_wt
                                , size_t raw_max_bytes
                                , size_t *raw_deficiency
                                , int *raw_size
                                , int *offset
                                , int *got_format)
{
    int ret = 0;
    audio_channel_mask_t cur_ch_mask;
    audio_format_t cur_aformat;
    int package_size;
    size_t valid_bytes = 0;

    *got_format = audio_type_parse(buffer, bytes, &package_size, &cur_ch_mask, raw_size, offset);

    if (is_datmos_suitable_format(*got_format)) {
        ALOGV("curent audio type %s\n", AUDIO_FORMAT_STRING(*got_format));
        if (bytes - (*offset + IEC61937_HEADER_BYTES) >= *raw_size) {
            valid_bytes = *raw_size;
        } else {
            valid_bytes = bytes - (*offset + IEC61937_HEADER_BYTES);
        }

        if (valid_bytes <= (raw_max_bytes - *raw_wt)) {
            //copy the data to raw buffer
            // if (*raw_wt > IEC61937_HEADER_BYTES) {
            //     char *last8bytes = raw_buf + *raw_wt - IEC61937_HEADER_BYTES;
            //     char *new8bytes = buffer + *offset + IEC61937_HEADER_BYTES;
            //     printf("L4B %2x %2x %2x %2x N4B %2x %2x %2x %2x wt %#x r 0xeff0 oset %#x Bytes %#x vB %#x\n",
            //         last8bytes[0], last8bytes[1], last8bytes[2], last8bytes[3], new8bytes[0], new8bytes[1], new8bytes[2], new8bytes[3], *raw_wt, *offset, bytes, valid_bytes);
            // }
            memcpy(raw_buf + *raw_wt, buffer + *offset + IEC61937_HEADER_BYTES, valid_bytes);
            //update the write pointer
            *raw_wt += valid_bytes;
            //get the deficiency
            *raw_deficiency = *raw_size - valid_bytes;
            // ALOGE("*raw_size %#x raw_wt %#x\n", *raw_size, *raw_wt);
            ret = valid_bytes + (*offset + IEC61937_HEADER_BYTES);
        } else {
            ret = -1;
            ALOGE("raw_size %#x valid bytes %#x valid space %#x\n", *raw_size, valid_bytes, raw_max_bytes - *raw_wt);
        }
    } else {
        // ALOGE("curent audio type %s\n", AUDIO_FORMAT_STRING(cur_audio_type));
        ret = bytes;
    }

    return ret;
}

//suppose the raw_buf has enough space!
int fill_in_the_remaining_data(char *input_data, size_t input_bytes, size_t *raw_deficiency, char *raw_buf, size_t *raw_wt, size_t raw_max_bytes)
{
    size_t valid_bytes = 0;

    if (*raw_deficiency > 0) {
        // ALOGI("raw_deficiency %#x valid bytes = %#x\n", *raw_deficiency , raw_max_bytes - *raw_wt);
        if (*raw_deficiency <= input_bytes) {
            valid_bytes = *raw_deficiency;
        } else {
            valid_bytes = input_bytes;
        }
        if (valid_bytes <= (raw_max_bytes - *raw_wt)) {
            /*deficiency is less than input_bytes, get all the deficiency this time*/
            //copy the data to raw buffer
            memcpy(raw_buf + *raw_wt, input_data, valid_bytes);
            //update the write pointer
            *raw_wt += valid_bytes;
            //get the deficiency
            *raw_deficiency -= valid_bytes;
            // ALOGV("raw_deficiency %#x *raw_wt %#x\n", *raw_deficiency , *raw_wt);
            return valid_bytes;
        } else {
            ALOGE("raw_deficiency %#x > valid bytes = %#x\n", *raw_deficiency , raw_max_bytes - *raw_wt);
            return -1;
        }
    } else {
        return 0;
    }
}