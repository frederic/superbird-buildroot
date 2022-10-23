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

#define LOG_TAG "aml_audio_mixer"
//#define LOG_NDEBUG 0

#include "log.h"
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <unistd.h>

#include <aml_audio_mixer.h>

struct ring_buf_desc {
    struct ring_buffer *buffer;
    struct pcm_config cfg;
    int period_time;
    int valid;
};

//simple mixer support: 2 in , 1 out
struct aml_audio_mixer {
    struct ring_buf_desc main_in_buf;
    struct ring_buf_desc aux_in_buf;
    unsigned int internal_main : 1;
    unsigned int internal_aux : 1;
    struct pcm_config mixer_cfg;
    int period_time;
    void *out_buffer;
    void *aux_buffer;
    int out_buffer_size;
    pthread_t out_mixer_tid;
    //pthread_mutex_t lock;
    //pthread_cond_t cond;
    int running : 1;
    int init_ok : 1;
    void *priv_data;
    int (*mixer_out_write) (void *buffer, void *priv_data, size_t size);
};

static void aml_dump_audio_pcm_config (struct pcm_config cfg)
{
    ALOGI ("\t-format:%#x, rate:%d, channels:%d", cfg.format, cfg.rate, cfg.channels);
    ALOGI ("\t-period_size:%d, period_count:%d", cfg.period_size, cfg.period_count);

}

static void aml_dump_audio_ring_buf_desc (struct ring_buf_desc *buf_desc)
{
    aml_dump_audio_pcm_config (buf_desc->cfg);
    ALOGI ("\t-period_time:%d, valid:%d", buf_desc->period_time, buf_desc->valid);
    ring_buffer_dump (buf_desc->buffer);
}

void aml_dump_audio_mixer (struct aml_audio_mixer *audio_mixer)
{
    ALOGI ("+dump audio mixer out config:");
    aml_dump_audio_pcm_config (audio_mixer->mixer_cfg);
    ALOGI ("\tperiod_time:%d, out_buffer_size:%d", audio_mixer->period_time, audio_mixer->out_buffer_size);
    aml_dump_audio_ring_buf_desc (&audio_mixer->main_in_buf);
    aml_dump_audio_ring_buf_desc (&audio_mixer->aux_in_buf);
}

static inline int aml_audio_mixer_check_status (struct aml_audio_mixer *audio_mixer)
{
    if (audio_mixer->init_ok) {
        return 1;
    } else if (audio_mixer->main_in_buf.valid && audio_mixer->aux_in_buf.valid) {
        audio_mixer->init_ok;
        return 1;
    }
    return 0;
}

static inline unsigned int calc_config_frame_size (struct pcm_config config)
{
    return config.channels * (pcm_format_to_bits (config.format) >> 3);
}

static inline unsigned int bytes_to_frames (struct pcm_config config, size_t bytes)
{
    return bytes / calc_config_frame_size (config);
}

struct aml_audio_mixer *new_aml_audio_mixer (struct pcm_config out_config)
{
    struct aml_audio_mixer *audio_mixer = NULL;

    audio_mixer = calloc (1, sizeof (*audio_mixer) );
    if (!audio_mixer)
        return NULL;

    audio_mixer->mixer_cfg = out_config;
    audio_mixer->period_time = out_config.period_size * 1000 / out_config.rate;
    //pthread_mutex_init(&audio_mixer->lock, NULL);
    //pthread_cond_init(&audio_mixer->cond, NULL);
    return audio_mixer;
}

void aml_delete_audio_mixer (struct aml_audio_mixer *audio_mixer)
{
    if (audio_mixer) {
        //pthread_mutex_destroy(&audio_mixer->lock);
        //pthread_cond_destroy(&audio_mixer->cond);
        free (audio_mixer);
    }
}

static void main_aux_buffer_mix (struct aml_audio_mixer *audio_mixer)
{
    //TODO: do nothing now, only ouput main buffer data;
    //add mixing methods latter.
    return;
}

static void *out_mixer_threadloop (void *data)
{
    struct aml_audio_mixer *audio_mixer = data;
    struct ring_buf_desc *main_desc = &audio_mixer->main_in_buf;
    struct ring_buf_desc *aux_desc = &audio_mixer->aux_in_buf;
    struct ring_buffer *main_buffer = main_desc->buffer;
    struct ring_buffer *aux_buffer = aux_desc->buffer;
    // calc write block size
    struct pcm_config *cfg = &main_desc->cfg;
    int period_byte = audio_mixer->out_buffer_size;
    int ready = 0;
    // sleep half period
    int msleep_time = cfg->period_size * 400 / cfg->rate;

    if (!audio_mixer->mixer_out_write)
        return NULL;

    audio_mixer->running = 1;
    prctl (PR_SET_NAME, "aml_audio_mixer");
    ALOGI ("%s start", __func__);
    while (audio_mixer->running) {
        //pthread_mutex_lock(&audio_mixer->lock);
        if (get_buffer_read_space (main_buffer) >= period_byte) {
            ring_buffer_read (main_buffer, audio_mixer->out_buffer, period_byte);
            if (get_buffer_read_space (aux_buffer) >= period_byte) {
                ring_buffer_read (aux_buffer, audio_mixer->aux_buffer, period_byte);
                main_aux_buffer_mix (audio_mixer);
            }
            audio_mixer->mixer_out_write (audio_mixer->out_buffer, audio_mixer->priv_data, period_byte);
        } else {
            //pthread_cond_wait(&audio_mixer->cond, &audio_mixer->lock);
            //ALOGV("%s main mixer sleeping time %d", __func__, msleep_time);
            // Not enough data, wait for half period
            usleep (msleep_time * 1000);
        }
        //pthread_mutex_unlock(&audio_mixer->lock);
    }

    ALOGI ("%s", __func__);
    return NULL;
}

int aml_start_audio_mixer (struct aml_audio_mixer *audio_mixer)
{
    struct ring_buf_desc *main_desc = NULL;
    int period_byte = 0;

    if (audio_mixer == NULL && !aml_audio_mixer_check_status (audio_mixer) )
        return -EINVAL;

    main_desc = &audio_mixer->main_in_buf;
    period_byte = main_desc->cfg.period_size * calc_config_frame_size (main_desc->cfg);

    audio_mixer->out_buffer = malloc (period_byte);
    if (!audio_mixer->out_buffer)
        return -ENOMEM;

    audio_mixer->aux_buffer = malloc (period_byte);
    if (!audio_mixer->aux_buffer) {
        free (audio_mixer->out_buffer);
        return -EINVAL;
    }
    audio_mixer->out_buffer_size = period_byte;

    return pthread_create (&audio_mixer->out_mixer_tid, NULL, out_mixer_threadloop, audio_mixer);
}

int aml_stop_audio_mixer (struct aml_audio_mixer *audio_mixer)
{
    if (audio_mixer == NULL)
        return -EINVAL;

    audio_mixer->running = 0;
    //pthread_cond_signal(&audio_mixer->cond);
    pthread_join (audio_mixer->out_mixer_tid, NULL);
    ALOGI ("%s", __func__);
    if (audio_mixer->out_buffer) {
        free (audio_mixer->out_buffer);
        audio_mixer->out_buffer = NULL;
    }

    return 0;
}

static int aml_malloc_internal_ring_buffer (struct ring_buf_desc *buf_desc,
        struct pcm_config cfg)
{
    struct ring_buffer *ringbuf = NULL;
    int buffer_size, frame_size;
    int ret = 0;

    frame_size = calc_config_frame_size (cfg);
    buffer_size = cfg.period_size * cfg.period_count * frame_size;
    ringbuf = calloc (1, sizeof (*ringbuf) );
    if (!ringbuf) {
        ALOGE ("no memory");
        return -ENOMEM ;
    }
    ret = ring_buffer_init (ringbuf, buffer_size);
    if (ret) {
        ALOGE ("init ring buffer fail, buffer_size = %d", buffer_size);
        goto exit;
    }
    buf_desc->buffer = ringbuf;
    buf_desc->valid = 1;

    return 0;

exit:
    free (ringbuf);
    return -EINVAL;
}

static void aml_free_internal_ring_buffer (struct ring_buf_desc *buf_desc)
{
    ring_buffer_release (buf_desc->buffer);
    free (buf_desc->buffer);
    buf_desc->buffer = NULL;
    buf_desc->valid = 0;
}

int aml_register_mixer_main_in_buffer (struct aml_audio_mixer *audio_mixer,
                                       struct ring_buffer *ringbuffer, struct pcm_config cfg)
{
    struct ring_buf_desc *main_inbuf = NULL;
    int ret;

    if (audio_mixer == NULL)
        return -EINVAL;

    main_inbuf = &audio_mixer->main_in_buf;
    if (ringbuffer) {
        main_inbuf->buffer = ringbuffer;
        audio_mixer->internal_main = 0;
    } else {
        ret = aml_malloc_internal_ring_buffer (main_inbuf, cfg);
        if (ret < 0)
            return ret;

        audio_mixer->internal_main = 1;
    }

    main_inbuf->cfg = cfg;
    main_inbuf->period_time = cfg.period_size * 1000 / cfg.rate;

    return 0;
}

int aml_release_main_in_buffer (struct aml_audio_mixer *audio_mixer)
{
    struct ring_buf_desc *main_inbuf = NULL;
    int ret;

    if (audio_mixer == NULL)
        return -EINVAL;

    main_inbuf = &audio_mixer->main_in_buf;

    aml_free_internal_ring_buffer (main_inbuf);
    return 0;
}

int aml_register_mixer_aux_in_buffer (struct aml_audio_mixer *audio_mixer,
                                      struct ring_buffer *ringbuffer, struct pcm_config cfg)
{
    struct ring_buf_desc *aux_inbuf = NULL;
    int ret;

    if (audio_mixer == NULL)
        return -EINVAL;

    aux_inbuf = &audio_mixer->aux_in_buf;
    if (ringbuffer) {
        aux_inbuf->buffer = ringbuffer;
        audio_mixer->internal_aux = 0;
    } else {
        ret = aml_malloc_internal_ring_buffer (aux_inbuf, cfg);
        if (ret < 0) {
            return ret;
        }

        audio_mixer->internal_aux = 1;
    }

    aux_inbuf->cfg = cfg;
    aux_inbuf->period_time = cfg.period_size * 1000 / cfg.rate;

    return 0;
}

int aml_release_aux_in_buffer (struct aml_audio_mixer *audio_mixer)
{
    struct ring_buf_desc *aux_inbuf = NULL;
    int ret;

    if (audio_mixer == NULL)
        return -EINVAL;

    aux_inbuf = &audio_mixer->aux_in_buf;

    aml_free_internal_ring_buffer (aux_inbuf);
    return 0;
}

int aml_write_mixer_main_in_buf (struct aml_audio_mixer *audio_mixer, void *buffer, size_t size)
{
    struct ring_buf_desc *main_inbuf = NULL;
    struct ring_buffer *rbuffer = NULL;
    int frames = 0;
    int msleep_time = 0;
    int ret = 0;

    if (audio_mixer == NULL)
        return -EINVAL;

    main_inbuf = &audio_mixer->main_in_buf;
    if (!main_inbuf->valid)
        return -ENOENT;

    rbuffer = main_inbuf->buffer;
    frames = bytes_to_frames (main_inbuf->cfg, size);
    // wait half time of the data size
    msleep_time = main_inbuf->period_time / 8;
    if (get_buffer_write_space (rbuffer) >= (int) size) {
        ring_buffer_write (rbuffer, buffer, size, UNCOVER_WRITE);
        //ALOGV("%s ...filling in buffer", __func__);
        //pthread_cond_signal(&audio_mixer->cond);
    } else {
        ALOGI ("%s ... in buffer sleep %dms", __func__, msleep_time);
        usleep (1000 * msleep_time);
        return -EAGAIN;
    }

    return 0;
}

int aml_write_mixer_aux_in_buf (struct aml_audio_mixer *audio_mixer, void *buffer, size_t size)
{
    struct ring_buf_desc *aux_inbuf = NULL;
    struct ring_buffer *rbuffer = NULL;
    int ret;

    if (audio_mixer == NULL)
        return -EINVAL;

    aux_inbuf = &audio_mixer->aux_in_buf;
    if (!aux_inbuf->valid)
        return -ENOENT;

    rbuffer = aux_inbuf->buffer;
    // for aux in, just overwrite data if no enough space
    ring_buffer_write (rbuffer, buffer, size, COVER_WRITE);
    //if (get_buffer_write_space(rbuffer) > (int)size)
    //    ring_buffer_write(rbuffer, buffer, size, UNCOVER_WRITE);
    //else
    //    return -EAGAIN;

    return 0;
}

int aml_register_audio_mixer_callback (struct aml_audio_mixer *audio_mixer,
                                       mixer_write_callback cbk, void *priv_data)
{
    if (audio_mixer == NULL)
        return -EINVAL;

    audio_mixer->mixer_out_write = cbk;
    audio_mixer->priv_data = priv_data;
    return 0;
}

int aml_release_audio_mixer_callback (struct aml_audio_mixer *audio_mixer)
{
    if (audio_mixer == NULL)
        return -EINVAL;

    audio_mixer->mixer_out_write = NULL;
    audio_mixer->priv_data = NULL;
    return 0;
}

