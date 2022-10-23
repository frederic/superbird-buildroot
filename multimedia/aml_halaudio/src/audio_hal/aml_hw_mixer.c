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

#define LOG_TAG "audio_hw_mixer"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "log.h"

#include "aml_hw_mixer.h"

//code here for audio hw mixer when hwsync with af mixer output stream output
//at the same,need do a software mixer in audio hw c.
int aml_hw_mixer_init(struct aml_hw_mixer *mixer)
{
    int ret = 0;

    pthread_mutex_init(&mixer->lock, NULL);
    pthread_mutex_lock(&mixer->lock);
    mixer->wp = 0;
    mixer->rp = 0;
    mixer->buf_size = AML_HW_MIXER_BUF_SIZE;
    mixer->start_buf = calloc(1, mixer->buf_size);
    if (!mixer->start_buf) {
        ALOGE("%s(), no mem", __func__);
        ret = -ENOMEM;
        goto exit;
    }

    mixer->need_cache_flag = 1;

exit:
    pthread_mutex_unlock(&mixer->lock);
    ALOGI("%s done\n",__func__);
    return ret;
}

void aml_hw_mixer_deinit(struct aml_hw_mixer *mixer)
{
    pthread_mutex_lock(&mixer->lock);
    free(mixer->start_buf);
    mixer->start_buf = NULL;
    mixer->wp = 0;
    mixer->rp = 0;
    mixer->buf_size = 0;
    mixer->need_cache_flag = 0;
    pthread_mutex_unlock(&mixer->lock);
}

void aml_hw_mixer_reset(struct aml_hw_mixer *mixer)
{
    pthread_mutex_lock(&mixer->lock);
    mixer->wp = 0;
    mixer->rp = 0;
    mixer->need_cache_flag = 1;
    pthread_mutex_unlock(&mixer->lock);
}


static uint aml_hw_mixer_get_space(struct aml_hw_mixer *mixer)
{
    unsigned space;
    if (mixer->wp >= mixer->rp)
        space = mixer->buf_size - (mixer->wp - mixer->rp);
    else
        space = mixer->rp - mixer->wp;
    return space > 64 ? (space - 64) : 0;
}

int aml_hw_mixer_get_content_l(struct aml_hw_mixer *mixer)
{
    unsigned content = 0;

    if (mixer->wp >= mixer->rp) {
        content = mixer->wp - mixer->rp;
    } else {
        content = mixer->wp - mixer->rp + mixer->buf_size;
    }
    //ALOGI("wp %d,rp %d\n",mixer->wp,mixer->rp);
    return content;
}

//we assume the cached size is always smaller than buffer size
//need called by device mutux locked
int aml_hw_mixer_write(struct aml_hw_mixer *mixer, const void *w_buf, uint size)
{
    unsigned space;
    unsigned write_size = size;
    unsigned tail = 0;

    if (!mixer || !mixer->start_buf) {
        return size;
    }

    pthread_mutex_lock(&mixer->lock);
    space = aml_hw_mixer_get_space(mixer);
    if (space < size) {
        ALOGI("write data no space,space %d,size %d,rp %d,wp %d,reset all ptr\n", space, size, mixer->rp, mixer->wp);
        mixer->wp = 0;
        mixer->rp = 0;
    }
    //TODO
    if (write_size > space) {
        write_size = space;
    }
    if (write_size + mixer->wp > mixer->buf_size) {
        tail = mixer->buf_size - mixer->wp;
        memcpy(mixer->start_buf + mixer->wp, w_buf, tail);
        write_size -= tail;
        memcpy(mixer->start_buf, (unsigned char*)w_buf + tail, write_size);
        mixer->wp = write_size;
    } else {
        memcpy(mixer->start_buf + mixer->wp, w_buf, write_size);
        mixer->wp += write_size;
        mixer->wp %= AML_HW_MIXER_BUF_SIZE;
    }
    pthread_mutex_unlock(&mixer->lock);

    return size;
}
static inline short CLIPSHORT(int r)
{
    return (r >  0x7fff) ? 0x7fff :
           (r < -0x8000) ? 0x8000 :
           r;
}

int aml_hw_mixer_mixing(struct aml_hw_mixer *mixer, void *mbuf, int size)
{
    int16_t *main_buf = (int16_t *)mbuf;
    int16_t *cached_buf;
    int32_t read_size = size;
    int32_t cached_size = 0;
    int32_t tail = 0, tmp;
    int i;

    if (!mixer || !mbuf)
        return 0;

    pthread_mutex_lock(&mixer->lock);
    cached_size = aml_hw_mixer_get_content_l(mixer);
    // no enough aux cached data, mix next turn
    if (cached_size < size) {
        //ALOGV("not enough aux data for mixing,cached %d,need %d\n",cached_size,size);
        pthread_mutex_unlock(&mixer->lock);
        return 0;
    }

    cached_buf = (int16_t *)(mixer->start_buf + mixer->rp);
    if (read_size + mixer->rp > mixer->buf_size) {
        tail = mixer->buf_size - mixer->rp;
        for (i = 0; i < tail/2; i++) {
            tmp = *main_buf + *cached_buf++;
            *main_buf++ = CLIPSHORT(tmp);
        }
        read_size -= tail;
        cached_buf = (int16_t *)mixer->start_buf;
        for (i = 0; i < read_size/2; i++) {
            tmp = *main_buf + *cached_buf++;
            *main_buf++ = CLIPSHORT(tmp);
        }
        mixer->rp = read_size;
    } else {
        for (i = 0; i < read_size/2; i++) {
            tmp = *main_buf + *cached_buf++;
            *main_buf++ = CLIPSHORT(tmp);
        }
        mixer->rp += read_size;
        mixer->rp %= AML_HW_MIXER_BUF_SIZE;
    }
    pthread_mutex_unlock(&mixer->lock);

    return 0;
}

//need called by device mutux locked
int aml_hw_mixer_read(struct aml_hw_mixer *mixer, void *r_buf, uint size)
{
    unsigned cached_size;
    unsigned read_size = size;
    unsigned tail = 0;

    pthread_mutex_lock(&mixer->lock);
    cached_size = aml_hw_mixer_get_content_l(mixer);
    // we always assue we have enough data to read when hwsync enabled.
    // if we do not have,insert zero data.
    if (cached_size < size) {
        ALOGI("read data has not enough data to mixer,read %d, have %d,rp %d,wp %d\n", size, cached_size, mixer->rp, mixer->wp);
        memset((unsigned char*)r_buf + cached_size, 0, size - cached_size);
        read_size = cached_size;
    }
    if (read_size + mixer->rp > mixer->buf_size) {
        tail = mixer->buf_size - mixer->rp;
        memcpy(r_buf, mixer->start_buf + mixer->rp, tail);
        read_size -= tail;
        memcpy((unsigned char*)r_buf + tail, mixer->start_buf, read_size);
        mixer->rp = read_size;
    } else {
        memcpy(r_buf, mixer->start_buf + mixer->rp, read_size);
        mixer->rp += read_size;
        mixer->rp %= AML_HW_MIXER_BUF_SIZE;
    }
    pthread_mutex_unlock(&mixer->lock);

    return size;
}
// aml audio hw c mixer code end
