/*
 * Copyright (C) 2010 Amlogic Corporation.
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



#ifndef _AUDIO_HWSYNC_H_
#define _AUDIO_HWSYNC_H_

#include <stdbool.h>

#define TSYNC_FIRSTAPTS "/sys/class/tsync/firstapts"
#define TSYNC_FIRSTVPTS "/sys/class/tsync/firstvpts"
#define TSYNC_PCRSCR    "/sys/class/tsync/pts_pcrscr"
#define TSYNC_EVENT     "/sys/class/tsync/event"
#define TSYNC_APTS      "/sys/class/tsync/pts_audio"
#define TSYNC_VPTS      "/sys/class/tsync/pts_video"
#define TSYNC_ENABLE    "/sys/class/tsync/enable"
#define TSYNC_MODE      "/sys/class/tsync/mode"
#define SYSTIME_CORRECTION_THRESHOLD        (90000*10/100)
#define NSEC_PER_SECOND 1000000000ULL
#define HW_SYNC_STATE_HEADER 0
#define HW_SYNC_STATE_BODY   1
#define HW_SYNC_STATE_RESYNC 2
#define HW_SYNC_HEADER_CNT 20

#define APTS_DISCONTINUE_THRESHOLD          (90000)
#define APTS_DISCONTINUE_THRESHOLD_MIN    (90000/1000*100)
#define APTS_DISCONTINUE_THRESHOLD_MAX    (5*90000)

#define HWSYNC_APTS_NUM     512

typedef struct apts_tab {
    int  valid;
    size_t offset;
    unsigned pts;
} apts_tab_t;

typedef struct  audio_hwsync {
    uint8_t hw_sync_header[HW_SYNC_HEADER_CNT];
    size_t hw_sync_header_cnt;
    int hw_sync_state;
    uint32_t hw_sync_body_cnt;
    uint32_t hw_sync_frame_size;
    uint8_t hw_sync_body_buf[8192];  // 4096
    uint8_t body_align[64];
    uint8_t body_align_cnt;
    bool first_apts_flag;//flag to indicate set first apts
    uint64_t first_apts;
    uint64_t last_apts_from_header;
    apts_tab_t pts_tab[HWSYNC_APTS_NUM];
    pthread_mutex_t lock;
    size_t payload_offset;
    struct aml_stream_out  *aout;
    int tsync_fd;
} audio_hwsync_t;
static inline bool hwsync_header_valid(uint8_t *header)
{
    return (header[0] == 0x55) &&
           (header[1] == 0x55) &&
           (header[2] == 0x00) &&
           //(header[3] == 0x01 || header[3] == 0x02);
           (header[3] == 0x02);
}

static inline uint64_t hwsync_header_get_pts(uint8_t *header)
{
    return (((uint64_t)header[8]) << 56) |
           (((uint64_t)header[9]) << 48) |
           (((uint64_t)header[10]) << 40) |
           (((uint64_t)header[11]) << 32) |
           (((uint64_t)header[12]) << 24) |
           (((uint64_t)header[13]) << 16) |
           (((uint64_t)header[14]) << 8) |
           ((uint64_t)header[15]);
}

static inline uint32_t hwsync_header_get_size(uint8_t *header)
{
    return (((uint32_t)header[4]) << 24) |
           (((uint32_t)header[5]) << 16) |
           (((uint32_t)header[6]) << 8) |
           ((uint32_t)header[7]);
}

static inline uint64_t get_pts_gap(uint64_t a, uint64_t b)
{
    if (a >= b) {
        return (a - b);
    } else {
        return (b - a);
    }
}

void aml_audio_hwsync_init(audio_hwsync_t *p_hwsync, struct aml_stream_out  *out);
int aml_audio_hwsync_find_frame(audio_hwsync_t *p_hwsync,
                                const void *in_buffer, size_t in_bytes, uint64_t *cur_pts, int *outsize);
int aml_audio_hwsync_set_first_pts(audio_hwsync_t *p_hwsync, uint64_t pts);
int aml_audio_hwsync_checkin_apts(audio_hwsync_t *p_hwsync, size_t offset, unsigned apts);
int aml_audio_hwsync_lookup_apts(audio_hwsync_t *p_hwsync, size_t offset, unsigned *p_apts);
int aml_audio_hwsync_audio_process(audio_hwsync_t *p_hwsync, size_t offset, int *p_adjust_ms);
void aml_audio_hwsync_release(audio_hwsync_t *p_hwsync);
#endif
