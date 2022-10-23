/*
 * Copyright (C) 2019 Amlogic Corporation.
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

#define LOG_TAG "audio_virtual_buf"
//#define LOG_TAG "audio_hw_primary"


#include <sys/time.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <string.h>


#include "aml_audio_timer.h"
#include "audio_virtual_buf.h"

#define MAX_NAME_LENGHT  128

typedef enum {
    VIRTUAL_BUF_IDLE,
    VIRTUAL_BUF_RUN,
    VIRTUAL_BUF_RESET,
} buf_state_t;


struct audio_virtual_buf {
    char buf_name[MAX_NAME_LENGHT];
    buf_state_t state;
    uint64_t buf_ns_cur;
    uint64_t buf_ns_begin;
    uint64_t buf_ns_target;
    uint64_t ease_time_ns;
    uint64_t buf_write_ns;
    uint64_t buf_read_ns;
    uint64_t last_process_ns;

};

int audio_virtual_buf_open(void ** pphandle, char * buf_name, uint64_t buf_ns_begin, uint64_t buf_ns_target, int ease_time_ms)
{
    int ret = -1;
    audio_virtual_buf_t * phandle = NULL;
    int name_length = 0;
    phandle = calloc(1, sizeof(struct audio_virtual_buf));
    if (phandle == NULL) {
        ALOGE("malloc failed\n");
        return -1;
    }
    if (buf_name) {
        name_length = strlen(buf_name);
        if (name_length >= (MAX_NAME_LENGHT - 1)) {
            name_length = (MAX_NAME_LENGHT - 1);
        }

        strncpy(phandle->buf_name, buf_name, name_length);
        phandle->buf_name[name_length + 1] = '\0';

    } else {
        ALOGE("buf name is NULL\n");
        return -1;
    }
    phandle->state = VIRTUAL_BUF_IDLE;
    phandle->buf_write_ns = 0;
    phandle->buf_read_ns  = 0;
    phandle->last_process_ns = 0;
    phandle->buf_ns_cur = buf_ns_begin;
    phandle->buf_ns_begin = buf_ns_begin;
    phandle->buf_ns_target = buf_ns_target;
    phandle->ease_time_ns = (uint64_t)ease_time_ms * 1000000LL;

    ALOGD("%s %s buf_begin=%lld buf_target=%lld time=%lld", __FUNCTION__ , phandle->buf_name,
          phandle->buf_ns_begin, phandle->buf_ns_target, phandle->ease_time_ns);
    * pphandle = (void*)phandle;
    return 0;
}

int audio_virtual_buf_close(void **pphandle)
{
    if (*pphandle) {
        free(*pphandle);
        *pphandle = NULL;
    }

    return 0;
}


int audio_virtual_buf_process(void *phandle, uint64_t frame_ns)
{
    int ret = -1;
    int break_while = 0;
    uint64_t current_ns = 0;
    uint64_t process_gap = 0;

    uint64_t read_ns = 0;
    uint64_t write_ns = 0;
    uint64_t buf_ns = 0;
    uint64_t sleep_ns = 0;
    uint64_t delay_ns = 0;
    uint64_t buf_ns_cur = 0;
    uint64_t elapsed_time_ns = 0;
    uint64_t ease_time_ns = 0;
    audio_virtual_buf_t *virtual_handle = NULL;
    float t;


    if (phandle == NULL) {
        return -1;
    }
    virtual_handle = (audio_virtual_buf_t *)phandle;
    current_ns = aml_audio_get_systime_ns();

    process_gap = current_ns - virtual_handle->last_process_ns;
    virtual_handle->last_process_ns = current_ns;

    read_ns  = virtual_handle->buf_read_ns;
    write_ns = virtual_handle->buf_write_ns;
    ease_time_ns = virtual_handle->ease_time_ns;
    buf_ns_cur = virtual_handle->buf_ns_cur;


    while (1) {
        switch (virtual_handle->state) {
        case VIRTUAL_BUF_IDLE: {
            write_ns = 0;
            read_ns  = 0;
            process_gap = 0;
            virtual_handle->state = VIRTUAL_BUF_RUN;
        }
        break;
        case VIRTUAL_BUF_RUN: {
            write_ns += frame_ns;
            read_ns += process_gap;
            break_while = 1;
        }
        break;
        case VIRTUAL_BUF_RESET: {
            write_ns = 0;
            read_ns  = 0;
            process_gap = 0;
            virtual_handle->state = VIRTUAL_BUF_RUN;
        }
        break;

        default:
            break;
        }

        if (read_ns > write_ns) {
            virtual_handle->state = VIRTUAL_BUF_RESET;
            break_while = 0;
            ALOGE("%s underrun happens read=%lld write=%lld diff=%lld", virtual_handle->buf_name, read_ns, write_ns, read_ns - write_ns);
        }

        if (break_while) {
            break;
        }
    }

    /*calculate the buf size according to elapsed time*/
    if (ease_time_ns != 0) {
        elapsed_time_ns = read_ns;
        t = (float)(((double)elapsed_time_ns) / ease_time_ns);

        if (t >= 1.0) {
            buf_ns_cur = virtual_handle->buf_ns_target;
        } else {
            buf_ns_cur = (virtual_handle->buf_ns_target - virtual_handle->buf_ns_begin) * t * t + virtual_handle->buf_ns_begin;
        }
        virtual_handle->buf_ns_cur = buf_ns_cur;
    }

    if (read_ns <= write_ns) {
        delay_ns = write_ns - read_ns;
        buf_ns = buf_ns_cur;

        if (delay_ns < buf_ns) {
            sleep_ns = 0;
        } else {
            sleep_ns = delay_ns - buf_ns;
        }
        //ALOGD("%s buffered ns =%lld buf_size_ns=%lld gap=%lld read=%lld write=%lld sleep=%lld",
        //  virtual_handle->buf_name,delay_ns, buf_ns, process_gap, read_ns, write_ns, sleep_ns);

    } else {
        ALOGE("wrong state, we can't get here");
        virtual_handle->state = VIRTUAL_BUF_RESET;
        return -1;
    }

    virtual_handle->buf_read_ns = read_ns;
    virtual_handle->buf_write_ns = write_ns;

    if (sleep_ns / 1000 >= 100) {
        aml_audio_sleep(sleep_ns / 1000);
    } else {
        aml_audio_sleep(100);
    }

    return 0;
}



