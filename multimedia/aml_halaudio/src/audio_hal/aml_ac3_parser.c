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

#define LOG_TAG "audio_hw_primary"
// #define LOG_NDEBUG 0

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/prctl.h>
#include "log.h"
#include <string.h>

#include "ac3_parser_utils.h"
#include "aml_ac3_parser.h"
#include "audio_format_parse.h"

#define IEC61937_HEADER_SIZE 8


/*
 *Find the position of 61937 sync word in the buffer, need PA/PB/PC/PD, 4*sizeof(short)
 */
static int seek_61937_sync_word(char *buffer, int size)
{
    int i = -1;
    if (size < 8) {
        return i;
    }

    for (i = 0; i < (size - 3); i++) {
        if (buffer[i + 0] == 0x72 && buffer[i + 1] == 0xf8 && buffer[i + 2] == 0x1f && buffer[i + 3] == 0x4e) {
            return i;
        }
        if (buffer[i + 0] == 0x4e && buffer[i + 1] == 0x1f && buffer[i + 2] == 0xf8 && buffer[i + 3] == 0x72) {
            return i;
        }
    }
    return -1;
}

int scan_dolby_main_associate_frame(void *input_buffer
                                    , size_t bytes
                                    , int *used_size
                                    , void **main_frame_buffer
                                    , int *main_frame_size
                                    , void **associate_frame_buffer
                                    , int *associate_frame_size)
{
    ALOGV("\n+%s() bytes %zu\n", __FUNCTION__, bytes);
    int ret = 0;
    int sync_word_offset = -1;
    char *temp_buffer = (char*) input_buffer;

    uint32_t *tmp_pcpd;
    uint32_t pcpd = 0;
    uint32_t tmp = 0;
    int payload_size = 0;

    int scan_frame_offset;
    int scan_frame_size;
    int scan_channel_num;
    int scan_numblks;
    int scan_timeslice_61937;
    int scan_framevalid_flag;
    int is_iec61937_packat = 0;
    int is_ddp = 0;

    sync_word_offset = seek_61937_sync_word((char*) temp_buffer, bytes);

    if (sync_word_offset >= 0) {
        tmp_pcpd = (uint32_t*)(temp_buffer + sync_word_offset + 4);
        pcpd = *tmp_pcpd;
        int pc = (pcpd & 0x1f);
        ALOGV("%s sync_word_offset %d pcpd %#x pc %#x\n", __FUNCTION__, sync_word_offset, pcpd, pc);

        /*Value of 0-4bit is data type*/
        if (pc == 0x01) {
            is_ddp = 0;
            payload_size = (pcpd >> 16) / 8;
            if (bytes - sync_word_offset >= (size_t) payload_size) {
                if (bytes - sync_word_offset >= AC3_PERIOD_SIZE) {
                    *used_size = sync_word_offset + AC3_PERIOD_SIZE;
                    is_iec61937_packat = 1;
                } else {
                    *used_size = sync_word_offset + payload_size;
                }
                ret = 0;
            } else {
                ALOGD("%s useful data len %lu ac3 iec61937 packet size %#x payload_size %#x",
                      __FUNCTION__, (unsigned long)(bytes - sync_word_offset), AC3_PERIOD_SIZE, payload_size);
                ret = -1;
            }
        } else if (pc == 0x15) {
            is_ddp = 1;
            payload_size = (pcpd >> 16);
            if (bytes - sync_word_offset >= (size_t) payload_size) {
                if (bytes - sync_word_offset >= EAC3_PERIOD_SIZE) {
                    *used_size = sync_word_offset + EAC3_PERIOD_SIZE;
                    is_iec61937_packat = 1;
                } else {
                    *used_size = sync_word_offset + payload_size;
                }
                ret = 0;
            } else {
                ALOGD("%s useful data len %lu eac3 iec61937 packet size %#x payload_size %#x",
                      __FUNCTION__, (unsigned long)(bytes - sync_word_offset), EAC3_PERIOD_SIZE, payload_size);
                ret = -1;
            }
        } else {
            ret = -1;
            ALOGE("%s error pc %x\n", __FUNCTION__, pc);
        }
    } else {
        *used_size = (int) bytes;
        ret = 0;
        ALOGD("%s() none iec61937 format header, skip this part\n", __FUNCTION__);
    }
    if (is_iec61937_packat == 0) {
        ALOGD("%s() ret %d *used_size %d payload_size %d has iec61937 packat %d \n",
              __FUNCTION__, ret, *used_size, payload_size, is_iec61937_packat);
    }

    if ((ret == 0) && (payload_size > 0)) {
        *main_frame_buffer = (void *)((char *) input_buffer + sync_word_offset + IEC61937_HEADER_SIZE);
        if (0 == parse_dolby_frame_header(*main_frame_buffer, *used_size, &scan_frame_offset, &scan_frame_size
                                          , &scan_channel_num, &scan_numblks, &scan_timeslice_61937, &scan_framevalid_flag)) {
            *main_frame_buffer = (void *)((char *) *main_frame_buffer + scan_frame_offset);
            *main_frame_size = scan_frame_size;
            if (payload_size > *main_frame_size) {
                /*contain the associate data*/
                *associate_frame_buffer = (void *)((char *) *main_frame_buffer + *main_frame_size);
                *associate_frame_size = payload_size - *main_frame_size;
                ALOGV("%s main size %d associate size %d payload size %d used_size %d\n",
                      __FUNCTION__, *main_frame_size, *associate_frame_size, payload_size, *used_size);
            } else {
                //there is no associate data
                *associate_frame_buffer = NULL;
                *associate_frame_size = 0;
            }
        } else {
            ret = -1;
            *main_frame_buffer = NULL;
            *main_frame_size = 0;
            *associate_frame_buffer = NULL;
            *associate_frame_size = 0;
        }
    } else {
        *main_frame_buffer = NULL;
        *main_frame_size = 0;
        *associate_frame_buffer = NULL;
        *associate_frame_size = 0;
    }

    if (*main_frame_size >=  *associate_frame_size) {
        ALOGV("-%s() main frame addr %p size %d associate frame addr %p size %d\n",
              __FUNCTION__, *main_frame_buffer, *main_frame_size, *associate_frame_buffer, *associate_frame_size);
    }

    return ret;
}

int scan_dolby_main_frame(void *input_buffer
                          , size_t bytes
                          , int *used_size
                          , void **main_frame_buffer
                          , int *main_frame_size)
{
    ALOGV("\n+%s() bytes %zu\n", __FUNCTION__, bytes);
    int ret = 0;
    int sync_word_offset = -1;
    char *temp_buffer = (char*)input_buffer;

    uint32_t *tmp_pcpd;
    uint32_t pcpd = 0;
    uint32_t tmp = 0;
    int payload_size = 0;

    int scan_frame_offset;
    int scan_frame_size;
    int scan_channel_num;
    int scan_numblks;
    int scan_timeslice_61937;
    int scan_framevalid_flag;
    int is_iec61937_packat = 0;
    int is_ddp = 0;

    sync_word_offset = seek_61937_sync_word((char*)temp_buffer, bytes);

    if (sync_word_offset >= 0) {
        tmp_pcpd = (uint32_t*)(temp_buffer + sync_word_offset + 4);
        pcpd = *tmp_pcpd;
        int pc = (pcpd & 0x1f);
        ALOGV("%s sync_word_offset %d pcpd %#x pc %#x\n", __FUNCTION__, sync_word_offset, pcpd, pc);

        /*Value of 0-4bit is data type*/
        if (pc == 0x01) {
            is_ddp = 0;
            payload_size = (pcpd >> 16) / 8;
            if (bytes - sync_word_offset >= (size_t)payload_size) {
                if (bytes - sync_word_offset >= AC3_PERIOD_SIZE) {
                    *used_size = sync_word_offset + AC3_PERIOD_SIZE;
                    is_iec61937_packat = 1;
                } else {
                    *used_size = sync_word_offset + payload_size;
                }
                ret = 0;
            } else {
                ALOGD("%s useful data len %lu ac3 iec61937 packet size %#x payload_size %#x",
                      __FUNCTION__, (unsigned long) (bytes - sync_word_offset), AC3_PERIOD_SIZE, payload_size);
                ret = -1;
            }
        } else if (pc == 0x15) {
            is_ddp = 1;
            payload_size = (pcpd >> 16);
            if (bytes - sync_word_offset >= (size_t)payload_size) {
                if (bytes - sync_word_offset >= EAC3_PERIOD_SIZE) {
                    *used_size = sync_word_offset + EAC3_PERIOD_SIZE;
                    is_iec61937_packat = 1;
                } else {
                    *used_size = sync_word_offset + payload_size;
                }
                ret = 0;
            } else {
                ALOGD("%s useful data len %lu eac3 iec61937 packet size %#x payload_size %#x",
                      __FUNCTION__,(unsigned long) (bytes - sync_word_offset), EAC3_PERIOD_SIZE, payload_size);
                ret = -1;
            }
        } else {
            ret = -1;
            ALOGE("%s error pc %x\n", __FUNCTION__, pc);
        }
    } else {
        *used_size = (int)bytes;
        ret = 0;
        ALOGV("%s() none iec61937 format header, skip this part\n", __FUNCTION__);
    }
    if (is_iec61937_packat == 0) {
        ALOGV("%s() ret %d *used_size %d payload_size %d has iec61937 packat %d \n",
              __FUNCTION__, ret, *used_size, payload_size, is_iec61937_packat);
    }

    if ((ret == 0) && (payload_size > 0)) {
        *main_frame_buffer = (void *)((char *)input_buffer + sync_word_offset + IEC61937_HEADER_SIZE);
        *main_frame_size = payload_size;
    } else {
        *main_frame_buffer = NULL;
        *main_frame_size = 0;
    }

    ALOGV("-%s() main frame addr %p size %d\n", __FUNCTION__, *main_frame_buffer, *main_frame_size);

    return ret;
}


//patch for ms12 pasing ,data not sync at buffer boundary
int scan_dolby_main_frame_ext(void *input_buffer
                              , size_t bytes
                              , int *used_size
                              , void **main_frame_buffer
                              , int *main_frame_size
                              , size_t *payload_deficiency)
{
    ALOGV("\n+%s() bytes %zu\n", __FUNCTION__, bytes);
    int ret = 0;
    int sync_word_offset = -1;
    char *temp_buffer = (char*)input_buffer;

    uint32_t *tmp_pcpd;
    uint32_t pcpd = 0;
    uint32_t tmp = 0;
    int payload_size = 0;

    int scan_frame_offset;
    int scan_frame_size;
    int scan_channel_num;
    int scan_numblks;
    int scan_timeslice_61937;
    int scan_framevalid_flag;
    int is_iec61937_packat = 0;
    int is_ddp = 0;

    // Pa   16-bit  Sync word 1  F872h
    // Pb   16-bit  Sync word 2  4E1Fh
    // Pc   16-bit  Burst-info  Table 1
    // Pd   16-bit  Length-code  Number of bits or number of bytes according to data-type

    // Table 1 Fields of burst-info
    // Bits  Contents
    // 0-4   Data-type
    // 5-6   Subdata-type
    // 7     Error-flag
    // 8-12  Data-type-dependent info
    // 13-15 Bit-stream-number

    *payload_deficiency = 0;
    sync_word_offset = seek_61937_sync_word((char*)temp_buffer, bytes);

    if (sync_word_offset >= 0) {
        tmp_pcpd = (uint32_t*)(temp_buffer + sync_word_offset + 4);
        pcpd = *tmp_pcpd;
        // Data type defined in PC bits 0-6 in IEC 61937-1 consists of conventional data-type (0-4) and
        // subdata-type (5-6) for historical reasons. All data-types are defined in Table 2.
        int pc = (pcpd & 0x1f);
        ALOGV("%s sync_word_offset %d pcpd %#x pc %#x\n", __FUNCTION__, sync_word_offset, pcpd, pc);

        /*Value of 0-4bit is data type*/
        if (pc == 0x01) {
            is_ddp = 0;
            payload_size = (pcpd >> 16) / 8;
            //ALOGI("%s sync_word_offset %d payload_size %#x pc %#x\n", __FUNCTION__, sync_word_offset, payload_size, pc);
            // enough data in current packate . payload_size normally 1536
            if (bytes - sync_word_offset >= (size_t)payload_size) {
                if (bytes - sync_word_offset >= AC3_PERIOD_SIZE) {
                    *used_size = sync_word_offset + AC3_PERIOD_SIZE;
                    is_iec61937_packat = 1;
                } else {
                    *used_size = sync_word_offset + payload_size;
                }
                ret = 0;
            } else {
                if (bytes - sync_word_offset > 0) {
                    *used_size = bytes;
                    *payload_deficiency = payload_size - (bytes - sync_word_offset - IEC61937_HEADER_SIZE);
                    ret = 1;
                } else {
                    ALOGD("%s useful data len %lu ac3 iec61937 packet size %#x payload_size %#x",
                          __FUNCTION__, (unsigned long)(bytes - sync_word_offset), AC3_PERIOD_SIZE, payload_size);
                    *used_size = bytes;
                    ret = -1;
                }
            }
        } else if (pc == 0x15) {
            is_ddp = 1;
            payload_size = (pcpd >> 16);
            if (bytes - sync_word_offset >= (size_t)payload_size) {
                if (bytes - sync_word_offset >= EAC3_PERIOD_SIZE) {
                    *used_size = sync_word_offset + EAC3_PERIOD_SIZE;
                    is_iec61937_packat = 1;
                } else {
                    *used_size = sync_word_offset + payload_size;
                }
                ret = 0;
            } else {
                if (bytes - sync_word_offset > 0) {
                    *used_size = bytes;
                    *payload_deficiency = payload_size - (bytes - sync_word_offset - IEC61937_HEADER_SIZE);
                    ret = 1;
                } else {
                    *used_size = bytes;
                    ALOGD("%s useful data len %lu eac3 iec61937 packet size %#x payload_size %#x",
                          __FUNCTION__, (unsigned long)(bytes - sync_word_offset), EAC3_PERIOD_SIZE, payload_size);
                    ret = -1;
                }
            }
        } else {
            ret = -1;
            ALOGE("%s error pc %x\n", __FUNCTION__, pc);
        }
    } else {
        *used_size = (int)bytes;
        ret = 0;
        ALOGV("%s() bytes = %zu,none iec61937 format header, skip this part\n", __FUNCTION__, bytes); //zz
    }
    if (is_iec61937_packat == 0) {
        ALOGV("%s() ret %d *used_size %d payload_size %d has iec61937 packat %d \n",
              __FUNCTION__, ret, *used_size, payload_size, is_iec61937_packat);
    }

    if ((ret >= 0) && (payload_size > 0)) {
        *main_frame_buffer = (void *)((char *)input_buffer + sync_word_offset + IEC61937_HEADER_SIZE);
        *main_frame_size = payload_size;
        ret = 0;
    } else {
        *main_frame_buffer = NULL;
        *main_frame_size = 0;
        *payload_deficiency = 0;
        ALOGV("-%s() main frame addr null size 0\n", __FUNCTION__);
    }

    ALOGV("-%s() main frame addr %p size %d payload_deficiency %zu\n",
          __FUNCTION__, *main_frame_buffer, *main_frame_size, *payload_deficiency);

    return ret;
}

