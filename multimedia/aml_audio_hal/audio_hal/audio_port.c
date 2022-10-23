/*
 * Copyright (C) 2018 The Android Open Source Project
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


#define LOG_TAG "audio_hw_port"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/log.h>
#include <cutils/list.h>
#include <linux/ioctl.h>
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>

#include "audio_port.h"
#include "aml_ringbuffer.h"
#include "audio_hw_utils.h"
#include "audio_hwsync.h"

#ifndef list_for_each_safe
#define list_for_each_safe(node, n, list) \
    for (node = (list)->next, n = node->next; \
         node != (list); \
         node = n, n = node->next)
#endif

#define BUFF_CNT 4
#define SYS_BUFF_CNT 4
static ssize_t input_port_write(struct input_port *port, const void *buffer, int bytes)
{
    unsigned char *data = (unsigned char *)buffer;
    int bytes_to_write = bytes;
    int written = 0;

    written = ring_buffer_write(port->r_buf, data, bytes_to_write, UNCOVER_WRITE);
    if (getprop_bool("media.audiohal.inport")) {
        if (port->port_index == MIXER_INPUT_PORT_PCM_SYSTEM)
            aml_audio_dump_audio_bitstreams("/data/audio/inportSys.raw", buffer, written);
        else if (port->port_index == MIXER_INPUT_PORT_PCM_DIRECT)
            aml_audio_dump_audio_bitstreams("/data/audio/inportDirect.raw", buffer, written);
    }

    ALOGV("%s() written %d", __func__, written);
    return written;
}

static ssize_t input_port_read(struct input_port *port, void *buffer, int bytes)
{
    int read = 0;
    read = ring_buffer_read(port->r_buf, buffer, bytes);
    if (read > 0)
        port->consumed_bytes += read;

    return read;
}

int inport_buffer_level(struct input_port *port)
{
    return get_buffer_read_space(port->r_buf);
}

bool ring_buf_ready(struct input_port *port)
{
    int read_avail = get_buffer_read_space(port->r_buf);

    if (0) {
        ALOGI("%s, port index %d, avail %d, chunk len %d",
            __func__, port->port_index, read_avail, port->data_len_bytes);
    }

    return (read_avail >= (int)port->data_len_bytes);
}

bool is_direct_flags(audio_output_flags_t flags) {
    return flags & (AUDIO_OUTPUT_FLAG_DIRECT | AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD);
}

uint32_t inport_get_latency_frames(struct input_port *port) {
    int frame_size = 4;
    uint32_t latency_frames = inport_buffer_level(port) / frame_size;
    // return full frames latency when no data in ring buffer
    if (latency_frames == 0)
        return port->rbuf_size / frame_size;

    return latency_frames;
}

enum MIXER_INPUT_PORT get_input_port_index(struct audio_config *config,
        audio_output_flags_t flags)
{
    int channel_cnt = 2;
    enum MIXER_INPUT_PORT port_index = MIXER_INPUT_PORT_PCM_SYSTEM;

    channel_cnt = audio_channel_count_from_out_mask(config->channel_mask);
    switch (config->format) {
        case AUDIO_FORMAT_PCM_16_BIT:
        case AUDIO_FORMAT_PCM_32_BIT:
            //if (config->sample_rate == 48000) {
            if (1) {
                ALOGI("%s(), samplerate %d", __func__, config->sample_rate);
                // FIXME: remove channel check when PCM_SYSTEM_SOUND supports multi-channel
                if (is_direct_flags(flags) || channel_cnt > 2) {
                    port_index = MIXER_INPUT_PORT_PCM_DIRECT;
                } else {
                    port_index = MIXER_INPUT_PORT_PCM_SYSTEM;
                }
                break;
            }
        case AUDIO_FORMAT_AC3:
        case AUDIO_FORMAT_E_AC3:
        case AUDIO_FORMAT_MAT:
            //port_index = MIXER_INPUT_PORT_BITSTREAM_RAW;
            //break;
        default:
            ALOGE("%s() stream not supported for mFormat:%#x",
                    __func__, config->format);
    }

    return port_index;
}

void inport_reset(struct input_port *port)
{
    ALOGD("%s()", __func__);
    port->port_status = STOPPED;
    //port->is_hwsync = false;
    port->consumed_bytes = 0;
}

int send_inport_message(struct input_port *port, enum PORT_MSG msg)
{
    struct port_message *p_msg = calloc(1, sizeof(struct port_message));
    if (p_msg == NULL) {
        ALOGE("%s(), no memory", __func__);
        return -ENOMEM;
    }

    p_msg->msg_what = msg;
    pthread_mutex_lock(&port->msg_lock);
    list_add_tail(&port->msg_list, &p_msg->list);
    pthread_mutex_unlock(&port->msg_lock);

    return 0;
}

const char *str_port_msg[MSG_CNT] = {
    "MSG_PAUSE",
    "MSG_FLUSH",
    "MSG_RESUME"
};

const char *port_msg_to_str(enum PORT_MSG msg)
{
    return str_port_msg[msg];
}

struct port_message *get_inport_message(struct input_port *port)
{
    struct port_message *p_msg = NULL;
    struct listnode *item = NULL;

    pthread_mutex_lock(&port->msg_lock);
    if (!list_empty(&port->msg_list)) {
        item = list_head(&port->msg_list);
        p_msg = node_to_item(item, struct port_message, list);
        ALOGD("%s(), msg: %s", __func__, port_msg_to_str(p_msg->msg_what));
    }
    pthread_mutex_unlock(&port->msg_lock);
    return p_msg;
}

int remove_inport_message(struct input_port *port, struct port_message *p_msg)
{
    if (port == NULL || p_msg == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }
    pthread_mutex_lock(&port->msg_lock);
    list_remove(&p_msg->list);
    pthread_mutex_unlock(&port->msg_lock);
    free(p_msg);

    return 0;
}

int remove_all_inport_messages(struct input_port *port)
{
    struct port_message *p_msg = NULL;
    struct listnode *node = NULL, *n = NULL;
    pthread_mutex_lock(&port->msg_lock);
    list_for_each_safe(node, n, &port->msg_list) {
        p_msg = node_to_item(node, struct port_message, list);
        ALOGI("%s(), msg what %s", __func__, port_msg_to_str(p_msg->msg_what));
        if (p_msg->msg_what == MSG_PAUSE)
            aml_hwsync_set_tsync_pause();
        list_remove(&p_msg->list);
        free(p_msg);
    }
    pthread_mutex_unlock(&port->msg_lock);
    return 0;
}

static int setPortConfig(struct audioCfg *cfg, struct audio_config *config)
{
    if (cfg == NULL || config == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    cfg->channelCnt = audio_channel_count_from_out_mask(config->channel_mask);
    cfg->format = config->format;
    cfg->sampleRate = config->sample_rate;
    cfg->frame_size = cfg->channelCnt * audio_bytes_per_sample(config->format);
    return 0;
}

/* padding buf with zero to avoid underrun of audioflinger */
static int inport_padding_zero(struct input_port *port, size_t bytes)
{
    char *feed_mem = NULL;
    ALOGI("%s(), padding size %d 0s to inport %d",
            __func__, bytes, port->port_index);
    feed_mem = calloc(1, bytes);
    if (!feed_mem) {
        ALOGE("%s(), no memory", __func__);
        return -ENOMEM;
    }

    input_port_write(port, feed_mem, bytes);
    port->padding_frames = bytes / port->cfg.frame_size;
    free(feed_mem);
    return 0;
}

int set_inport_padding_size(struct input_port *port, size_t bytes)
{
    port->padding_frames = bytes / port->cfg.frame_size;
    return 0;
}

struct input_port *new_input_port(
        //enum MIXER_INPUT_PORT port_index,
        //audio_format_t format//,
        size_t buf_frames,
        struct audio_config *config,
        audio_output_flags_t flags,
        float volume,
        bool direct_on)
{
    struct input_port *port = NULL;
    struct ring_buffer *ringbuf = NULL;
    enum MIXER_INPUT_PORT port_index;
    int channel_cnt = 2;
    char *data = NULL;
    int rbuf_size = 0, thunk_size = 0;
    int ret = 0;

    port = calloc(1, sizeof(struct input_port));
    if (!port) {
        ALOGE("%s(), no memory", __func__);
        goto err;
    }

    setPortConfig(&port->cfg, config);
    thunk_size = buf_frames * port->cfg.frame_size;
    data = calloc(1, thunk_size);
    if (!data) {
        ALOGE("%s(), no memory", __func__);
        goto err_data;
    }

    ringbuf = calloc(1, sizeof(struct ring_buffer));
    if (!ringbuf) {
        ALOGE("%s(), no memory", __func__);
        goto err_rbuf;
    }

    port_index = get_input_port_index(config, flags);
    // system buffer larger than direct to cache more for mixing?
    if (port_index == MIXER_INPUT_PORT_PCM_SYSTEM) {
        rbuf_size = thunk_size * SYS_BUFF_CNT;
    } else {
        rbuf_size = thunk_size * BUFF_CNT;
    }

    ALOGD("%s(), index:%d, rbuf size:%d, direct_on:%d",
            __func__, port_index, rbuf_size, direct_on);
    ALOGD("%s(), fmt %#x, rate %d",
            __func__, port->cfg.format, port->cfg.sampleRate);
    ret = ring_buffer_init(ringbuf, rbuf_size);
    if (ret) {
        ALOGE("init ring buffer fail, buffer_size = %d", rbuf_size);
        goto err_rbuf_init;
    }

    port->port_index = port_index;
    //port->format = config->format;
    port->r_buf = ringbuf;
    port->rbuf_size = rbuf_size;
    port->data_valid = 0;
    port->data = data;
    port->data_buf_frame_cnt = buf_frames;
    port->data_len_bytes = thunk_size;
    port->read = input_port_read;
    port->write = input_port_write;
    port->rbuf_ready = ring_buf_ready;
    port->get_latency_frames = inport_get_latency_frames;
    port->port_status = STOPPED;
    port->is_hwsync = false;
    port->consumed_bytes = 0;
    port->volume = volume;
    list_init(&port->msg_list);
    //TODO
    //set_inport_hwsync(port);
    //if (port_index == MIXER_INPUT_PORT_PCM_SYSTEM && !direct_on) {
    //if (port_index == MIXER_INPUT_PORT_PCM_SYSTEM) {
    //    inport_padding_zero(port, rbuf_size);
    //}
    return port;

err_rbuf_init:
    free(ringbuf);
err_rbuf:
    free(data);
err_data:
    free(port);
err:
    return NULL;
}

int free_input_port(struct input_port *port)
{
    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }

    remove_all_inport_messages(port);
    ring_buffer_release(port->r_buf);
    free(port->r_buf);
    free(port->data);
    free(port);

    return 0;
}

int reset_input_port(struct input_port *port)
{
    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }

    inport_reset(port);
    return ring_buffer_reset(port->r_buf);
}

int resize_input_port_buffer(struct input_port *port, uint buf_size)
{
    int ret = 0;

    if (!port) {
        ALOGE("%s() NULL pointer", __func__);
        return -EINVAL;
    }

    if (port->data_len_bytes == buf_size) {
        return 0;
    }
    ALOGI("%s(), new size %d", __func__, buf_size);
    ring_buffer_release(port->r_buf);
    ret = ring_buffer_init(port->r_buf, buf_size * 4);
    if (ret) {
        ALOGE("init ring buffer fail, buffer_size = %d", buf_size * 4);
        ret = -ENOMEM;
        goto err_rbuf_init;
    }

    port->data = (char *)realloc(port->data, buf_size);
    if (!port->data) {
        ALOGE("%s() no mem", __func__);
        ret = -ENOMEM;
        goto err_data;
    }
    port->data_len_bytes = buf_size;

    return 0;
err_data:
    ring_buffer_release(port->r_buf);
err_rbuf_init:
    return ret;
}

void set_inport_hwsync(struct input_port *port)
{
    ALOGD("%s()", __func__);
    port->is_hwsync = true;
}
bool is_inport_hwsync(struct input_port *port)
{
    return port->is_hwsync;
}

void set_inport_volume(struct input_port *port, float vol)
{
    ALOGD("%s(), volume %f", __func__, vol);
    port->volume = vol;
}

float get_inport_volume(struct input_port *port)
{
    return port->volume;
}

int set_port_notify_cbk(struct input_port *port,
        int (*on_notify_cbk)(void *data), void *data)
{
    port->on_notify_cbk = on_notify_cbk;
    port->notify_cbk_data = data;
    return 0;
}

int set_port_input_avail_cbk(struct input_port *port,
        int (*on_input_avail_cbk)(void *data), void *data)
{
    port->on_input_avail_cbk = on_input_avail_cbk;
    port->input_avail_cbk_data = data;
    return 0;
}

int set_port_meta_data_cbk(struct input_port *port,
        meta_data_cbk_t meta_data_cbk,
        void *data)
{
    if (!is_inport_hwsync(port)) {
        ALOGE("%s(), can't set meta data callback", __func__);
        return -EINVAL;
    }
    port->meta_data_cbk = meta_data_cbk;
    port->meta_data_cbk_data = data;
    return 0;
}

int set_inport_state(struct input_port *port, enum port_state status)
{
    port->port_status = status;
    return 0;
}

enum port_state get_inport_state(struct input_port *port)
{
    return port->port_status;
}

size_t get_inport_consumed_size(struct input_port *port)
{
    return port->consumed_bytes;
}

static ssize_t output_port_start(struct output_port *port)
{
    struct audioCfg cfg = port->cfg;
    struct pcm_config pcm_cfg;
    int card = port->cfg.card;
    int device = port->cfg.device;
    struct pcm *pcm = NULL;

    memset(&pcm_cfg, 0, sizeof(struct pcm_config));
    pcm_cfg.channels = cfg.channelCnt;
    pcm_cfg.rate = cfg.sampleRate;
    pcm_cfg.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE;
    pcm_cfg.period_count = DEFAULT_PLAYBACK_PERIOD_CNT;
    //pcm_cfg.stop_threshold = pcm_cfg.period_size * pcm_cfg.period_count - 128;
    //pcm_cfg.silence_threshold = pcm_cfg.stop_threshold;
    //pcm_cfg.silence_size = 1024;

    if (cfg.format == AUDIO_FORMAT_PCM_16_BIT)
        pcm_cfg.format = PCM_FORMAT_S16_LE;
    else if (cfg.format == AUDIO_FORMAT_PCM_32_BIT)
        pcm_cfg.format = PCM_FORMAT_S32_LE;
    else {
        ALOGE("%s(), unsupport", __func__);
        pcm_cfg.format = PCM_FORMAT_S16_LE;
    }
    ALOGI("%s(), port:%p open ALSA hw:%d,%d", __func__, port, card, device);
    pcm = pcm_open(card, device, PCM_OUT | PCM_MONOTONIC, &pcm_cfg);
    if ((pcm == NULL) || !pcm_is_ready(pcm)) {
        ALOGE("cannot open pcm_out driver: %s", pcm_get_error(pcm));
        pcm_close(pcm);
        return -EINVAL;
    }
    if (!pcm_is_ready (pcm) ) {
        ALOGE ("cannot open pcm_out driver: %s", pcm_get_error (pcm) );
        pcm_close (pcm);
        return -EINVAL;
    }
    port->pcm_handle = pcm;
    port->port_status = ACTIVE;

    return 0;
}

static int output_port_standby(struct output_port *port)
{
    if (port && port->pcm_handle) {
        ALOGI("%s()", __func__);
        pcm_close(port->pcm_handle);
        port->pcm_handle = NULL;
        port->port_status = STOPPED;
        return 0;
    }
    return -EINVAL;
}

int outport_stop_pcm(struct output_port *port)
{
    if (port == NULL)
        return -EINVAL;

    if (port->port_status == ACTIVE && port->pcm_handle) {
        pcm_stop(port->pcm_handle);
    }
    return 0;
}

static ssize_t output_port_write(struct output_port *port, void *buffer, int bytes)
{
    int bytes_to_write = bytes;
    (void *)port;
    do {
        int written = 0;
        ALOGV("%s(), line %d", __func__, __LINE__);

        aml_audio_dump_audio_bitstreams("/data/audio/audioOutPort.raw", buffer, bytes);
        //usleep(bytes*1000/4/48);
        written = bytes;
        bytes_to_write -= written;
    } while (bytes_to_write > 0);
    return bytes;
}

static ssize_t output_port_write_alsa(struct output_port *port, void *buffer, int bytes)
{
    int bytes_to_write = bytes;
    int ret = 0;
    if (port->sound_track_mode == 3)
           port->sound_track_mode = AM_AOUT_OUTPUT_LRMIX;
    aml_audio_switch_output_mode((int16_t *)buffer, bytes, port->sound_track_mode);
    do {
        int written = 0;
        ALOGV("%s(), line %d", __func__, __LINE__);
        ret = pcm_write(port->pcm_handle, (void *)buffer, bytes);
        if (ret == 0) {
           written += bytes;
        } else {
           ALOGE("pcm_write port:%p failed ret = %d, pcm_get_error(out->pcm):%s",
                port, ret, pcm_get_error(port->pcm_handle));
           if (errno == EBADF)
               return 0;
           pcm_stop(port->pcm_handle);
           usleep(1000);
        }
        if (written > 0 && getprop_bool("media.audiohal.inport")) {
            aml_audio_dump_audio_bitstreams("/data/audio/audioOutPort.raw", buffer, written);
        }
        bytes_to_write -= written;
    } while (bytes_to_write > 0);

    return bytes;
}

int outport_get_latency_frames(struct output_port *port)
{
    int ret = 0, frames = 0;

    if (!port)
        return -EINVAL;

    if (!port->pcm_handle || !pcm_is_ready(port->pcm_handle)) {
        return -EINVAL;
    }
    ret = pcm_ioctl(port->pcm_handle, SNDRV_PCM_IOCTL_DELAY, &frames);
    if (ret < 0)
        return ret;

    return frames;
}

struct output_port *new_output_port(
        enum MIXER_OUTPUT_PORT port_index,
        struct audioCfg cfg,
        size_t buf_frames)
{
    struct output_port *port = NULL;
    struct ring_buffer *ringbuf = NULL;
    char *data = NULL;
    int rbuf_size = buf_frames * cfg.frame_size;
    int ret = 0;

    port = calloc(1, sizeof(struct output_port));
    if (!port) {
        ALOGE("%s(), no memory", __func__);
        goto err;
    }

    data = calloc(1, rbuf_size);
    if (!data) {
        ALOGE("%s(), no memory", __func__);
        goto err_data;
    }
    /* not used */
    ringbuf = calloc(1, sizeof(struct ring_buffer));
    if (!ringbuf) {
        ALOGE("%s(), no memory", __func__);
        goto err_rbuf;
    }
    ret = ring_buffer_init(ringbuf, rbuf_size *2);
    if (ret) {
        ALOGE("init ring buffer fail, buffer_size = %d", rbuf_size*2);
        goto err_rbuf_init;
    }
    //ret = pthread_create(&port->out_port_tid, NULL, port_threadloop, port);
    //if (ret < 0)
    //    ALOGE("%s() thread run failed.", __func__);

    port->port_index = port_index;
    port->cfg = cfg;
    port->r_buf = ringbuf;
    port->data_buf_frame_cnt = buf_frames;
    port->data_buf_len = rbuf_size;
    port->data_buf = data;
    port->start = output_port_start;
    port->standby = output_port_standby;
    //port->write = output_port_write;
    port->write = output_port_write_alsa;
    port->port_status = STOPPED;

    return port;

err_rbuf_init:
    free(ringbuf);
err_rbuf:
    free(data);
err_data:
    free(port);
err:
    return NULL;
}

int free_output_port(struct output_port *port)
{
    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }

    ALOGI("%s(), %d port:%p", __func__, __LINE__, port);
    if (port->port_status != STOPPED &&
            port->port_status != IDLE)
        pcm_close(port->pcm_handle);
    ring_buffer_release(port->r_buf);
    free(port->r_buf);
    free(port->data_buf);
    free(port);

    return 0;
}

int resize_output_port_buffer(struct output_port *port, size_t buf_frames)
{
    int ret = 0;
    size_t buf_length = 0;

    if (!port) {
        ALOGE("%s() NULL pointer", __func__);
        return -EINVAL;
    }

    if (port->buf_frames == buf_frames) {
        return 0;
    }
    ALOGI("%s(), new buf_frames %d", __func__, buf_frames);
    buf_length = buf_frames * port->cfg.frame_size;
    port->data_buf = (char *)realloc(port->data_buf, buf_length);
    if (!port->data_buf) {
        ALOGE("%s() no mem", __func__);
        ret = -ENOMEM;
        goto err_data;
    }
    port->data_buf_len = buf_length;

    return 0;
err_data:
    return ret;
}

bool is_inport_valid(enum MIXER_INPUT_PORT index)
{
    return (index >= 0 && index < MIXER_INPUT_PORT_NUM);
}

bool is_outport_valid(enum MIXER_OUTPUT_PORT index)
{
    return (index >= 0 && index < MIXER_OUTPUT_PORT_NUM);
}

int set_inport_pts_valid(struct input_port *in_port, bool valid)
{
    in_port->pts_valid = valid;
    return 0;
}

bool is_inport_pts_valid(struct input_port *in_port)
{
    return in_port->pts_valid;
}
