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

#define LOG_TAG "audio_hw_primary"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include "am_ad.h"
#include "aml_ringbuffer.h"
#include "audio_dtv_ad.h"
#include "aml_audio_stream.h"

#define AD_DEMUX_ID 0
#define CACHE_TIME 200
#define DEFAULT_ASSOC_AUDIO_BUFFER_SIZE 512 * 6 * 8 * 8

static pthread_mutex_t assoc_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct _dtv_assoc_audio {
    int assoc_enable;
    int sub_apid;
    int sub_afmt;
    int cache;
    int cache_start_time;
    int main_frame_size;
    int ad_frame_size;
    ring_buffer_t sub_abuf;
    int bufinited;
    pthread_t tid;
    void *ad_handle;
    void *g_assoc_bst;
    int nAssocBufSize;
    void *pargs;
} dtv_assoc_audio;

static dtv_assoc_audio assoc_bst = {
    .assoc_enable = 0,
    .sub_apid = 0,
    .sub_afmt = 0,
    .cache = 0,
    .main_frame_size = 0,
    .ad_frame_size = 0,
    //sub_abuf ,
    .bufinited = 0,
    .tid = -1,
    .ad_handle = NULL,
    .g_assoc_bst = NULL,
    .nAssocBufSize = 0,
    .pargs = NULL,
};

enum {
    DTV_ASSOC_STAT_DISABLE = 0,
    DTV_ASSOC_STAT_ENABLE,
};

static int64_t get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec)/1000;//ms
}

static dtv_assoc_audio *get_assoc_audio(void)
{
    return &assoc_bst;
}

static void audio_adcallback(const unsigned char * data, int len, void * handle)
{
    UNUSED(handle);
    //ALOGI("ASSOC %s,len =%d,data:%02x %02x %02x %02x %02x %02x %02x %02x %02x", __FUNCTION__, len, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
    //pthread_mutex_lock(&assoc_mutex);
    dtv_assoc_audio *param = get_assoc_audio();

    ring_buffer_t *ringbuffer = &(param->sub_abuf);
    int left;
    if (param->assoc_enable == DTV_ASSOC_STAT_ENABLE && param->bufinited == 1 && param->cache > 0) {
        unsigned short head1 = data[0] << 8 | data[1];
        left = get_buffer_write_space(ringbuffer);
        if (left < len) {
            ALOGI("buffer is full left = %d reset buffer",left);
            ring_buffer_reset(ringbuffer);
        } else if (head1 == 0x0b77 || head1 == 0x770b) {

            //  adec_print("write buffer size:%d,bs->buf_length=%d,bs->buf_level=%d",len,param->g_assoc_bst->buf_length,param->g_assoc_bst->buf_level);
            ring_buffer_write(ringbuffer, (unsigned char *)data, len, UNCOVER_WRITE);
        } else {
            ALOGI("audio_adcallback,not ac3/eac3 data len=%d\n", len);
        }
    } else {
        ALOGI("[%s]-[associate_dec_supported:%d]-[g_assoc_bst:%p]\n", __FUNCTION__, param->assoc_enable, param->g_assoc_bst);
    }
    //   pthread_mutex_unlock(&assoc_mutex);
}

static int audio_ad_set_source(int enable, int pid, int fmt, void *user)
{
    AM_ErrorCode_t err = AM_SUCCESS;
    dtv_assoc_audio *param = get_assoc_audio();
    ALOGI("AD set source enable[%d] pid[%d] fmt[%d]", enable, pid, fmt);
    if ((enable == DTV_ASSOC_STAT_ENABLE) && VALID_PID(pid)) {
        AM_AD_Para_t para = {.dmx_id = AD_DEMUX_ID, .pid = pid, .fmt = fmt};
        err = AM_AD_Create(&param->ad_handle, &para);
        if (err == AM_SUCCESS) {
            ALOGI("AM_AD_Create success\n");
            err = AM_AD_SetCallback(param->ad_handle, audio_adcallback, user);
            err = AM_AD_Start(param->ad_handle);
            ALOGI("AM_AD_start err=%d\n", err);
            if (err != AM_SUCCESS) {
                ALOGI("AM_AD_start failed ,err=%d\n", err);
                AM_AD_Destroy(param->ad_handle);
            } else {
                return 0;
            }
        } else {
            ALOGI("AM_AD_Create error,err=%d\n", err);
        }
    } else {
        ALOGI("disable AD\n");
        err = AM_AD_Stop(param->ad_handle);
        err = AM_AD_Destroy(param->ad_handle);
        ALOGI("disable AD, success,ret=%d\n", err);
    }
    return 1;
}

int dtv_assoc_init(void)
{
    int ret;
    dtv_assoc_audio *param = get_assoc_audio();
    if (param->bufinited) {
        return 0;
    }
    //pthread_mutex_init(&assoc_mutex, NULL);
    pthread_mutex_lock(&assoc_mutex);

    param->nAssocBufSize = DEFAULT_ASSOC_AUDIO_BUFFER_SIZE;

    param->assoc_enable = DTV_ASSOC_STAT_DISABLE;
    param->bufinited = 1;
    param->sub_apid = 0;
    param->sub_afmt = 0;
    param->cache= 0;
    param->main_frame_size= 0;
    param->ad_frame_size= 0;
    ret = ring_buffer_init(&(param->sub_abuf),
                           DEFAULT_ASSOC_AUDIO_BUFFER_SIZE);
    if (ret < 0) {
        pthread_mutex_unlock(&assoc_mutex);
        ALOGE("Fail to init audio ringbuffer!");
        return -1;
    }
    ALOGI("[%s %d] associate audio init success! \n", __FUNCTION__, __LINE__);
    pthread_mutex_unlock(&assoc_mutex);
    return 0;
}

int dtv_assoc_deinit(void)
{
    pthread_mutex_lock(&assoc_mutex);
    dtv_assoc_audio *param = get_assoc_audio();
    if (param->assoc_enable == DTV_ASSOC_STAT_ENABLE) {
        audio_ad_set_source(DTV_ASSOC_STAT_DISABLE, param->sub_apid, param->sub_afmt, NULL);
    }
    param->assoc_enable = DTV_ASSOC_STAT_DISABLE;
    if (param->bufinited) {
        ring_buffer_release(&(param->sub_abuf));
        param->bufinited = 0;
    }
    param->sub_apid = 0;
    param->sub_afmt = 0;
    param->cache= 0;
    param->main_frame_size= 0;
    param->ad_frame_size= 0;
	ALOGI("[%s %d] dtv_assoc_deinit success! \n", __FUNCTION__, __LINE__);
    pthread_mutex_unlock(&assoc_mutex);
    //pthread_mutex_destroy(&assoc_mutex);
    return 0;
}

int dtv_assoc_get_avail(void)
{
    //ALOGI(" dtv_assoc_read enter,size=%d", size);
    dtv_assoc_audio *param = get_assoc_audio();
    ring_buffer_t *ringbuffer = &(param->sub_abuf);
    int avail = 0;
    int cache_time = get_time() - param->cache_start_time;

    if (cache_time >= CACHE_TIME) {
        avail = get_buffer_read_space(ringbuffer);
    }

    return avail;
}

//To get the associate data if assoc is able
int dtv_assoc_read(unsigned char *data, int size)
{
    int ret = 0;
    //ALOGI(" dtv_assoc_read enter,size=%d", size);
    dtv_assoc_audio *param = get_assoc_audio();
    ring_buffer_t *ringbuffer = &(param->sub_abuf);

    if (param->assoc_enable == DTV_ASSOC_STAT_ENABLE
        && param->bufinited == 1) {
        ret = ring_buffer_read(ringbuffer, (unsigned char *)data, size);
    } else {
        ret = 0;
    }
    /*
        head1 = data[0] << 8 | data[1];
        if (head1 == 0x0b77 || head1 == 0x770b)
            return ret;
        else
            ALOGI("!!!!!!!!!!!!!!!!!!! read_assoc_data buffer is error\n");
            return 0;*/

    //  pthread_mutex_unlock(&assoc_mutex);
    //ALOGI("dtv_assoc_read size=%d ret=%d", size, ret);
    return ret;
}

void dtv_assoc_set_main_frame_size(int main_frame_size)
{
    dtv_assoc_audio *param = get_assoc_audio();

    param->main_frame_size = main_frame_size;
}

void dtv_assoc_get_main_frame_size(int* main_frame_size)
{
    dtv_assoc_audio *param = get_assoc_audio();

    *main_frame_size = param->main_frame_size;
}

void dtv_assoc_set_ad_frame_size(int ad_frame_size)
{
    dtv_assoc_audio *param = get_assoc_audio();

    param->ad_frame_size = ad_frame_size;
}

void dtv_assoc_get_ad_frame_size(int* ad_frame_size)
{
    dtv_assoc_audio *param = get_assoc_audio();

    *ad_frame_size = param->ad_frame_size;
}

void dtv_assoc_audio_cache(int value)
{
    dtv_assoc_audio *param = get_assoc_audio();

    if (value < 0) {
        param->cache = -1000;
        ring_buffer_reset(&param->sub_abuf);
    } else {
        param->cache += value;
        if (param->cache == 1) {
            param->cache_start_time = get_time();
        }
    }
}

void dtv_assoc_audio_start(unsigned int handle, int pid, int fmt)
{
    int ret = -1;
    pthread_mutex_lock(&assoc_mutex);
    dtv_assoc_audio *param = get_assoc_audio();
    ALOGI("%s, pid=%d, fmt= %d", __FUNCTION__, pid, fmt);
    if (handle == 0 || param->bufinited == 0) {
        ALOGI("%s, buffer was not inited, the handle is %d,return", __FUNCTION__, handle);
    } else if (VALID_PID(pid) && param->assoc_enable == DTV_ASSOC_STAT_DISABLE) {
        ALOGI("%s, pid %d, fmt %d,return", __FUNCTION__, pid, fmt);
        param->sub_apid = pid;
        param->sub_afmt = fmt;
        param->cache = 0;
        param->main_frame_size= 0;
        param->ad_frame_size= 0;
        ret = audio_ad_set_source(DTV_ASSOC_STAT_ENABLE, param->sub_apid, param->sub_afmt, NULL);
        if (ret == 0) {
            ALOGI("%s, DTV_ASSOC_STAT_ENABLE", __FUNCTION__);
            param->assoc_enable = DTV_ASSOC_STAT_ENABLE;
        }

    } else if (!VALID_PID(pid) && param->assoc_enable == DTV_ASSOC_STAT_ENABLE) {
        ALOGI("%s, assoc is enable, disable it", __FUNCTION__);
        param->sub_apid = 0;
        param->sub_afmt = 0;
        param->cache= 0;
        param->main_frame_size= 0;
        param->ad_frame_size= 0;
        param->assoc_enable = DTV_ASSOC_STAT_DISABLE;
        audio_ad_set_source(DTV_ASSOC_STAT_DISABLE, param->sub_apid, param->sub_afmt, NULL);
    } else {
        ALOGI("%s, invaled", __FUNCTION__);
    }
    pthread_mutex_unlock(&assoc_mutex);
    if (ret == 0) {
        ring_buffer_reset(&param->sub_abuf);
    }
}

void dtv_assoc_audio_stop(unsigned int handle)
{
    pthread_mutex_lock(&assoc_mutex);
    dtv_assoc_audio *param = get_assoc_audio();
    if (handle == 0) {
        ALOGI("%s, handle is error\n", __FUNCTION__);
    } else if (param->assoc_enable == DTV_ASSOC_STAT_ENABLE) {
        ALOGI("%s, disable it\n", __FUNCTION__);
        param->assoc_enable = DTV_ASSOC_STAT_DISABLE;
        param->sub_apid = 0;
        param->sub_afmt = 0;
        param->cache= 0;
        param->main_frame_size= 0;
        param->ad_frame_size= 0;
        audio_ad_set_source(DTV_ASSOC_STAT_DISABLE, param->sub_apid, param->sub_afmt, NULL);
    } else {
        ALOGI("%s, nothing to do\n", __FUNCTION__);
    }
    pthread_mutex_unlock(&assoc_mutex);
}

static void dtv_assoc_audio_pause(unsigned int handle)
{
    if (handle == 0) {
        return;
    }
    ALOGI("%s, paused\n", __FUNCTION__);
    dtv_assoc_audio *param = get_assoc_audio();
    if (handle == 0) {
        return ;
    }
    if (param->assoc_enable == DTV_ASSOC_STAT_ENABLE) {
        audio_ad_set_source(DTV_ASSOC_STAT_DISABLE, param->sub_apid, param->sub_afmt, NULL);
    }
    param->assoc_enable = DTV_ASSOC_STAT_DISABLE;
    param->sub_apid = 0;
    return ;
}
static void dtv_assoc_audio_resume(unsigned int handle, int pid)
{
    dtv_assoc_audio *param = get_assoc_audio();
    if (handle == 0) {
        return;
    }
    ALOGI("%s, pid=%d\n", __FUNCTION__, pid);
    if (pid == 0) {
        param->sub_apid = 0;
        ALOGI("%s,disable... param->assoc_enable=%d", __FUNCTION__, param->assoc_enable);
        if (param->assoc_enable == DTV_ASSOC_STAT_ENABLE) {
            audio_ad_set_source(DTV_ASSOC_STAT_DISABLE, param->sub_apid, param->sub_afmt, NULL);

        }
        param->assoc_enable = DTV_ASSOC_STAT_DISABLE;
        return ;
    } else if (pid > 0 && pid < 0x1fff) {
        param->sub_apid = pid;
        ALOGI("%s,enable... param->assoc_enable=%d", __FUNCTION__, param->assoc_enable);
        if ((param->bufinited) && (!param->assoc_enable)) {
            audio_ad_set_source(DTV_ASSOC_STAT_ENABLE, param->sub_apid, param->sub_afmt, NULL);
            param->assoc_enable = DTV_ASSOC_STAT_ENABLE;
        }
    } else {
        ALOGI("%s, err... param->assoc_enable=%d", __FUNCTION__, param->assoc_enable);
    }
}

