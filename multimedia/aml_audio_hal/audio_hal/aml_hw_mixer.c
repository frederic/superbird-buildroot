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
#include <cutils/log.h>

#include "aml_hw_mixer.h"
#include "audio_hw_utils.h"

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
    memset(mixer->start_buf, 0, mixer->buf_size);
    pthread_mutex_unlock(&mixer->lock);
}

uint aml_hw_mixer_get_space(struct aml_hw_mixer *mixer)
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
    //ALOGI("%s(), buffer level = %d", __func__, content);
    return content;
}

//we assume the cached size is always smaller than buffer size
//need called by device mutux locked
int aml_hw_mixer_write(struct aml_hw_mixer *mixer, const void *buffer, size_t bytes)
{
    int retry = 5;
    unsigned tail, space, write_bytes = bytes;

    if (!mixer || !mixer->start_buf) {
        ALOGE("%s(), no mixer or mixer not inited!", __func__);
        return bytes;
    }

    while (retry--) {
        pthread_mutex_lock(&mixer->lock);
        space = aml_hw_mixer_get_space(mixer);
        if (space < bytes) {
            pthread_mutex_unlock(&mixer->lock);
            usleep(10 * 1000);
        } else
            break;
    }

    if (retry < 0) {
        ALOGE("%s: write data no space,space %d,bytes %zu,rp %d,wp %d, reset all ptr",
            __func__, space, bytes, mixer->rp, mixer->wp);
        mixer->wp = 0;
        mixer->rp = 0;
        pthread_mutex_unlock(&mixer->lock);
        return bytes;
    }

    if (write_bytes + mixer->wp > mixer->buf_size) {
        tail = mixer->buf_size - mixer->wp;
        memcpy(mixer->start_buf + mixer->wp, buffer, tail);
        write_bytes -= tail;
        memcpy(mixer->start_buf, (unsigned char*)buffer + tail, write_bytes);
        mixer->wp = write_bytes;
    } else {
        memcpy(mixer->start_buf + mixer->wp, buffer, write_bytes);
        mixer->wp += write_bytes;
        mixer->wp %= AML_HW_MIXER_BUF_SIZE;
    }
    pthread_mutex_unlock(&mixer->lock);

    return bytes;
}

static inline short CLIPSHORT(int32_t r)
{
    if (r > 32767)
        r = 32767;
    else if (r < -32768)
        r = -32768;
    return r;
}

static inline int CLIPINT(int64_t r)
{
    if (r > 2147483647)
        r = 2147483647;
    else if (r < -2147483648)
        r = -2147483648;
    return r;
}

int aml_hw_mixer_mixing(struct aml_hw_mixer *mixer, void *buffer, int bytes, audio_format_t format)
{
    int32_t i, tail;
    int32_t cached_bytes, read_bytes = bytes;

    if (getprop_bool("media.audiohal.mixer"))
        aml_audio_dump_audio_bitstreams("/data/audio/beforemix.raw", buffer, bytes);

    pthread_mutex_lock(&mixer->lock);
    cached_bytes = aml_hw_mixer_get_content_l(mixer);
    if (cached_bytes < bytes) {
        ALOGV("%s: no enough aux data for mixing, cached %d, need %d\n", __func__, cached_bytes, bytes);
        pthread_mutex_unlock(&mixer->lock);
        return 0;
    }

    if (format == AUDIO_FORMAT_PCM_32_BIT) {
        int64_t tmp;
        int32_t *tmp_buffer = (int32_t *)buffer;
        int32_t *cached_buf = (int32_t *)(mixer->start_buf + mixer->rp);
        if (read_bytes + mixer->rp > mixer->buf_size) {
            tail = mixer->buf_size - mixer->rp;
            for (i = 0; i < tail / 4; i++) {
                tmp = (int64_t)*tmp_buffer + (int64_t)*cached_buf++;
                *tmp_buffer++ = CLIPINT(tmp);
            }
            read_bytes -= tail;
            cached_buf = (int32_t *)mixer->start_buf;
            for (i = 0; i < read_bytes / 4; i++) {
                tmp = (int64_t)*tmp_buffer + (int64_t)*cached_buf++;
                *tmp_buffer++ = CLIPINT(tmp);
            }
            mixer->rp = read_bytes;
        } else {
            for (i = 0; i < read_bytes / 4; i++) {
                tmp = (int64_t)*tmp_buffer + (int64_t)*cached_buf++;
                *tmp_buffer++ = CLIPINT(tmp);
            }
            mixer->rp += read_bytes;
            mixer->rp %= AML_HW_MIXER_BUF_SIZE;
        }
    } else if (format == AUDIO_FORMAT_PCM_16_BIT) {
        int32_t tmp;
        int16_t *tmp_buffer = (int16_t *)buffer;
        int16_t *cached_buf = (int16_t *)(mixer->start_buf + mixer->rp);
        if (read_bytes + mixer->rp > mixer->buf_size) {
            tail = mixer->buf_size - mixer->rp;
            for (i = 0; i < tail / 2; i++) {
                tmp = (int32_t)*tmp_buffer + (int32_t)*cached_buf++;
                *tmp_buffer++ = CLIPSHORT(tmp);
            }
            read_bytes -= tail;
            cached_buf = (int16_t *)mixer->start_buf;
            for (i = 0; i < read_bytes / 2; i++) {
                tmp = (int32_t)*tmp_buffer + (int32_t)*cached_buf++;
                *tmp_buffer++ = CLIPSHORT(tmp);
            }
            mixer->rp = read_bytes;
        } else {
            for (i = 0; i < read_bytes / 2; i++) {
                tmp = (int32_t)*tmp_buffer + (int32_t)*cached_buf++;
                *tmp_buffer++ = CLIPSHORT(tmp);
            }
            mixer->rp += read_bytes;
            mixer->rp %= AML_HW_MIXER_BUF_SIZE;
        }
    } else {
        ALOGE("%s(), format %#x not supporte!", __func__, format);
    }
    pthread_mutex_unlock(&mixer->lock);

    if (getprop_bool("media.audiohal.mixer"))
        aml_audio_dump_audio_bitstreams("/data/audio/mixed.raw", buffer, bytes);

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
