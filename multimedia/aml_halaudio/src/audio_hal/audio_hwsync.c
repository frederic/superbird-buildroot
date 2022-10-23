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



#define LOG_TAG "audio_hwsync"
//#define LOG_NDEBUG 0
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include "log.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "audio_hw_utils.h"
#include "audio_hwsync.h"
#include "audio_hw.h"

static int aml_audio_hwsync_get_pcr(audio_hwsync_t *p_hwsync, uint *value)
{
    int fd = -1;
    char valstr[64];
    uint val = 0;
    off_t offset;
    if (!p_hwsync) {
        ALOGE("invalid pointer %s", __func__);
        return -1;
    }
    if (p_hwsync->tsync_fd < 0) {
        fd = open(TSYNC_PCRSCR, O_RDONLY);
        p_hwsync->tsync_fd = fd;
        ALOGI("%s open tsync fd %d", __func__, fd);
    } else {
        fd = p_hwsync->tsync_fd;
    }
    if (fd >= 0) {
        memset(valstr, 0, 64);
        offset = lseek(fd, 0, SEEK_SET);
        read(fd, valstr, 64 - 1);
        valstr[strlen(valstr)] = '\0';
    } else {
        ALOGE("%s unable to open file %s\n", __func__, TSYNC_PCRSCR);
        return -1;
    }
    if (sscanf(valstr, "0x%x", &val) < 1) {
        ALOGE("%s unable to get pcr from: %s,fd %d,offset %ld", __func__, valstr, fd, offset);
        return -1;
    }
    *value = val;
    return 0;
}

void aml_audio_hwsync_init(audio_hwsync_t *p_hwsync, struct aml_stream_out  *out)
{
    int fd = -1;
    if (p_hwsync == NULL) {
        return;
    }
    p_hwsync->first_apts_flag = false;
    p_hwsync->hw_sync_state = HW_SYNC_STATE_HEADER;
    p_hwsync->hw_sync_header_cnt = 0;
    memset(p_hwsync->pts_tab, 0, sizeof(apts_tab_t)*HWSYNC_APTS_NUM);
    pthread_mutex_init(&p_hwsync->lock, NULL);
    p_hwsync->payload_offset = 0;
    p_hwsync->aout = out;
    if (p_hwsync->tsync_fd < 0) {
        fd = open(TSYNC_PCRSCR, O_RDONLY);
        p_hwsync->tsync_fd = fd;
        ALOGI("%s open tsync fd %d", __func__, fd);
    }
    ALOGI("%s done", __func__);
    return;
}
void aml_audio_hwsync_release(audio_hwsync_t *p_hwsync)
{
    if (!p_hwsync) {
        return;
    }
    if (p_hwsync->tsync_fd >= 0) {
        close(p_hwsync->tsync_fd);
    }
    p_hwsync->tsync_fd = -1;
    ALOGI("%s done", __func__);
}
//return bytes cost from input,
int aml_audio_hwsync_find_frame(audio_hwsync_t *p_hwsync,
                                const void *in_buffer, size_t in_bytes, uint64_t *cur_pts, int *outsize)
{
    size_t remain = in_bytes;
    uint8_t *p = (uint8_t *)in_buffer;
    uint64_t time_diff = 0;
    int pts_found = 0;
    struct aml_audio_device *adev = p_hwsync->aout->dev;
    int debug_enable = (adev->debug_flag > 8);
    if (p_hwsync == NULL || in_buffer == NULL) {
        return 0;
    }

    //ALOGI(" --- out_write %d, cache cnt = %d, body = %d, hw_sync_state = %d", out_frames * frame_size, out->body_align_cnt, out->hw_sync_body_cnt, out->hw_sync_state);
    while (remain > 0) {
        //if (p_hwsync->hw_sync_state == HW_SYNC_STATE_RESYNC) {
        //}
        if (p_hwsync->hw_sync_state == HW_SYNC_STATE_HEADER) {
            //ALOGI("Add to header buffer [%d], 0x%x", p_hwsync->hw_sync_header_cnt, *p);
            p_hwsync->hw_sync_header[p_hwsync->hw_sync_header_cnt++] = *p++;
            remain--;
            if (p_hwsync->hw_sync_header_cnt == HW_SYNC_HEADER_CNT) {
                uint64_t pts;
                if (!hwsync_header_valid(&p_hwsync->hw_sync_header[0])) {
                    //ALOGE("!!!!!!hwsync header out of sync! Resync.should not happen????");
                    p_hwsync->hw_sync_state = HW_SYNC_STATE_HEADER;
                    memcpy(p_hwsync->hw_sync_header, p_hwsync->hw_sync_header + 1, HW_SYNC_HEADER_CNT - 1);
                    p_hwsync->hw_sync_header_cnt--;
                    continue;
                }
                if ((in_bytes - remain) > HW_SYNC_HEADER_CNT) {
                    ALOGI("got the frame sync header cost %zu", in_bytes - remain);
                }
                p_hwsync->hw_sync_state = HW_SYNC_STATE_BODY;
                p_hwsync->hw_sync_body_cnt = hwsync_header_get_size(&p_hwsync->hw_sync_header[0]);
                p_hwsync->hw_sync_frame_size = p_hwsync->hw_sync_body_cnt;
                p_hwsync->body_align_cnt = 0; //  alisan zz
                p_hwsync->hw_sync_header_cnt = 0; //8.1
                pts = hwsync_header_get_pts(&p_hwsync->hw_sync_header[0]);
                pts = pts * 90 / 1000000;
                time_diff = get_pts_gap(pts, p_hwsync->last_apts_from_header) / 90;
                if (debug_enable > 8) {
                    ALOGV("pts %"PRIx64",frame len %u\n", pts, p_hwsync->hw_sync_body_cnt);
                    ALOGV("last pts %"PRIx64",diff %"PRIx64" ms\n", p_hwsync->last_apts_from_header, time_diff);
                }
                if (time_diff > 32) {
                    ALOGI("pts  time gap %"PRIx64" ms,last %"PRIx64",cur %"PRIx64"\n", time_diff,
                          p_hwsync->last_apts_from_header, pts);
                }
                p_hwsync->last_apts_from_header = pts;
                *cur_pts = pts;
                pts_found = 1;
                //ALOGI("get header body_cnt = %d, pts = %lld", out->hw_sync_body_cnt, pts);
            }
            continue;
        } else if (p_hwsync->hw_sync_state == HW_SYNC_STATE_BODY) {
            int m = (p_hwsync->hw_sync_body_cnt < remain) ? p_hwsync->hw_sync_body_cnt : remain;
            // process m bytes body with an empty fragment for alignment
            if (m  > 0) {
                //ret = pcm_write(out->pcm, p, m - align);
#if 0
                FILE *fp1 = fopen("/data/hwsync_body.pcm", "a+");
                if (fp1) {
                    int flen = fwrite((char *)p, 1, m, fp1);
                    //ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
                    fclose(fp1);
                } else {
                    ALOGE("could not open file:/data/hwsync_body.pcm");
                }
#endif
                memcpy(p_hwsync->hw_sync_body_buf + p_hwsync->hw_sync_frame_size - p_hwsync->hw_sync_body_cnt, p, m);
                p += m;
                remain -= m;
                p_hwsync->hw_sync_body_cnt -= m;
                if (p_hwsync->hw_sync_body_cnt == 0) {
                    p_hwsync->hw_sync_state = HW_SYNC_STATE_HEADER;
                    p_hwsync->hw_sync_header_cnt = 0;
                    *outsize = p_hwsync->hw_sync_frame_size;
                    /*
                    sometimes the audioflinger burst size is smaller than hwsync payload
                    we need use the last found pts when got a complete hwsync payload
                    */
                    if (!pts_found) {
                        *cur_pts = p_hwsync->last_apts_from_header;
                    }
                    if (debug_enable > 8) {
                        ALOGV("we found the frame total body,yeah\n");
                    }
                    break;//continue;
                }
            }
        }
    }
    return in_bytes - remain;
}

int aml_audio_hwsync_set_first_pts(audio_hwsync_t *p_hwsync, uint64_t pts)
{
    uint32_t pts32;
    char tempbuf[128];

    if (p_hwsync == NULL) {
        return -1;
    }

    if (pts > 0xffffffff) {
        ALOGE("APTS exeed the 32bit range!");
        return -1;
    }

    pts32 = (uint32_t)pts;
    p_hwsync->first_apts_flag = true;
    p_hwsync->first_apts = pts;
    sprintf(tempbuf, "AUDIO_START:0x%x", pts32);
    ALOGI("hwsync set tsync -> %s", tempbuf);
    if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
        ALOGE("set AUDIO_START failed \n");
        return -1;
    }

    return 0;
}
/*
@offset :ms12 real costed offset
@p_adjust_ms: a/v adjust ms.if return a minus,means
 audio slow,need skip,need slow.return a plus value,means audio quick,need insert zero.
*/
int aml_audio_hwsync_audio_process(audio_hwsync_t *p_hwsync, size_t offset, int *p_adjust_ms)
{
    uint32_t apts = 0;
    int ret = 0;
    *p_adjust_ms = 0;
    uint pcr = 0;
    uint gap = 0;
    int gap_ms = 0;
    char tempbuf[32] = {0};
    struct aml_audio_device *adev = p_hwsync->aout->dev;
    int debug_enable = (adev->debug_flag > 8);
    ret = aml_audio_hwsync_lookup_apts(p_hwsync, offset, &apts);
    if (ret) {
        ALOGE("%s lookup failed", __func__);
        return 0;
    }
    if (p_hwsync->first_apts_flag == false && offset > 0) {
        aml_audio_hwsync_set_first_pts(p_hwsync, apts);
    } else  if (p_hwsync->first_apts_flag) {
        ret = aml_audio_hwsync_get_pcr(p_hwsync, &pcr);
        if (ret == 0) {
            gap = get_pts_gap(pcr, apts);
            gap_ms = gap / 90;
            if (debug_enable) {
                ALOGI("%s pcr 0x%x,apts 0x%x,gap 0x%x,gap duration %d ms", __func__, pcr, apts, gap, gap_ms);
            }
            if (gap > APTS_DISCONTINUE_THRESHOLD_MIN && gap < APTS_DISCONTINUE_THRESHOLD_MAX) {
                if (apts > pcr) {
                    *p_adjust_ms = gap_ms;
                } else {
                    ALOGI("tsync -> reset pcrscr 0x%x -> 0x%x, %s big,diff %"PRIx64" ms",
                          pcr, apts, apts > pcr ? "apts" : "pcr", get_pts_gap(apts, pcr) / 90);
                    sprintf(tempbuf, "0x%x", apts);
                    int ret_val = sysfs_set_sysfs_str(TSYNC_APTS, tempbuf);
                    if (ret_val == -1) {
                        ALOGE("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
                    }
                }
            } else if (gap > APTS_DISCONTINUE_THRESHOLD_MAX) {
                ALOGE("%s apts exceed the adjust range,need check apts 0x%x,pcr 0x%x",
                      __func__, apts, pcr);
            }
        }
    }
    return ret;
}
int aml_audio_hwsync_checkin_apts(audio_hwsync_t *p_hwsync, size_t offset, unsigned apts)
{
    int i = 0;
    int ret = -1;
    struct aml_audio_device *adev = p_hwsync->aout->dev;
    int debug_enable = (adev->debug_flag > 8);
    apts_tab_t *pts_tab = NULL;
    if (!p_hwsync) {
        ALOGE("%s null point", __func__);
        return -1;
    }
    if (debug_enable) {
        ALOGI("++ %s checkin ,offset %zu,apts 0x%x", __func__, offset, apts);
    }
    pthread_mutex_lock(&p_hwsync->lock);
    pts_tab = p_hwsync->pts_tab;
    for (i = 0; i < HWSYNC_APTS_NUM; i++) {
        if (!pts_tab[i].valid) {
            pts_tab[i].pts = apts;
            pts_tab[i].offset = offset;
            pts_tab[i].valid = 1;
            if (debug_enable) {
                ALOGI("%s checkin done,offset %zu,apts 0x%x", __func__, offset, apts);
            }
            ret = 0;
            break;
        }
    }
    pthread_mutex_unlock(&p_hwsync->lock);
    return ret;
}

/*
for audio tunnel mode.the apts checkin align is based on
the audio payload size.
normally for Netflix case:
1)dd+ 768 byte per frame
2)LPCM from aac decoder, normally 4096 for LC AAC.8192 for SBR AAC
for the LPCM, the MS12 normally drain 6144 bytes per times which is
the each DD+ decoder output,we need align the size to 4096/8192 align
to checkout the apts.
*/
int aml_audio_hwsync_lookup_apts(audio_hwsync_t *p_hwsync, size_t offset, unsigned *p_apts)
{
    int i = 0;
    size_t align  = 0;
    int ret = -1;
    struct aml_audio_device *adev = p_hwsync->aout->dev;
    struct audio_stream_out *stream = (struct audio_stream_out *)p_hwsync->aout;
    int    debug_enable = (adev->debug_flag > 8);
    uint32_t latency_frames = 0;
    uint32_t latency_pts = 0;
    apts_tab_t *pts_tab = NULL;
    if (!p_hwsync) {
        ALOGE("%s null point", __func__);
        return -1;
    }
    if (debug_enable) {
        ALOGI("%s offset %zu,first %d", __func__, offset, p_hwsync->first_apts_flag);
    }
    pthread_mutex_lock(&p_hwsync->lock);
    if (p_hwsync->first_apts_flag == false) {
        align = 0;
    } else {
        align = offset - offset % p_hwsync->hw_sync_frame_size;
    }
    pts_tab = p_hwsync->pts_tab;
    for (i = 0; i < HWSYNC_APTS_NUM; i++) {
        if (pts_tab[i].valid) {
            if (pts_tab[i].offset == align) {
                *p_apts = pts_tab[i].pts;
                ret = 0;
                if (debug_enable) {
                    ALOGI("%s first flag %d,pts checkout done,offset %zu,align %zu,pts 0x%x",
                          __func__, p_hwsync->first_apts_flag, offset, align, *p_apts);
                }
                break;
            } else if (pts_tab[i].offset < align) {
                pts_tab[i].valid = 0;
            }
        }
    }
    if (i == HWSYNC_APTS_NUM) {
        ALOGE("%s,apts lookup failed,align %zu,offset %zu", __func__, align, offset);
    } else if (p_hwsync->first_apts_flag) {
        latency_frames = out_get_latency_frames(stream);
        if (get_output_format(stream) == AUDIO_FORMAT_E_AC3) {
            latency_frames /= 4;
        }
        latency_pts = latency_frames / 48 * 90;
        if (*p_apts > latency_pts) {
            *p_apts -= latency_pts;
        }
    }
    pthread_mutex_unlock(&p_hwsync->lock);
    return ret;
}
