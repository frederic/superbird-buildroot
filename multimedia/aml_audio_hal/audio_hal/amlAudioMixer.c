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

#define LOG_TAG "amlaudioMixer"
//#define LOG_NDEBUG 0
#define DEBUG_DUMP 0

#define __USE_GNU
#include <cutils/log.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <system/audio.h>
#include <aml_volume_utils.h>

#include "amlAudioMixer.h"
#include "audio_hw_utils.h"
#include "hw_avsync.h"
#include "audio_hwsync.h"
#include "audio_data_process.h"

#include "audio_hw.h"

#define MIXER_IN_BUFFER_SIZE (512*4)
#define MIXER_OUT_BUFFER_SIZE MIXER_IN_BUFFER_SIZE
/* in/out frame count should be same */
#define MIXER_IN_FRAME_COUNT 512
#define MIXER_OUT_FRAME_COUNT 512
#define STANDBY_TIME_US 1000000

struct ring_buf_desc {
    struct ring_buffer *buffer;
    struct pcm_config cfg;
    int period_time;
    int valid;
};

enum mixer_state {
    MIXER_IDLE,             // no active tracks
    MIXER_INPORTS_ENABLED,  // at least one active track, but no track has any data ready
    MIXER_INPORTS_READY,    // at least one active track, and at least one track has data
    MIXER_DRAIN_TRACK,      // drain currently playing track
    MIXER_DRAIN_ALL,        // fully drain the hardware
};

//simple mixer support: 2 in , 1 out
struct amlAudioMixer {
    struct input_port *in_ports[MIXER_INPUT_PORT_NUM];
    struct output_port *out_ports[MIXER_OUTPUT_PORT_NUM];

    /* protect inports */
    pthread_mutex_t inport_lock;

    ssize_t (*write)(struct amlAudioMixer *mixer, void *buffer, int bytes);
    //int period_time;
    void *tmp_buffer;
    size_t buf_frames;
    int idle_sleep_time_us;
    size_t frame_size_tmp;
    uint32_t hwsync_frame_size;
    pthread_t out_mixer_tid;

    /* condition and lock pair */
    pthread_mutex_t lock;
    pthread_cond_t cond;

    int exit_thread : 1;
    int mixing_enable : 1;
    enum mixer_state state;
    struct timespec tval_last_write;
    struct timespec tval_last_run;
    void *adev_data;
    struct aml_audio_device *adev;
    bool continuous_output;
    //int init_ok : 1;
    bool standby;
};

int mixer_set_state(struct amlAudioMixer *audio_mixer, enum mixer_state state)
{
    audio_mixer->state = state;
    return 0;
}

int mixer_set_continuous_output(struct amlAudioMixer *audio_mixer,
        bool continuous_output)
{
    audio_mixer->continuous_output = continuous_output;
    return 0;
}

bool mixer_is_continuous_enabled(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->continuous_output;
}

enum mixer_state mixer_get_state(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->state;
}

int init_mixer_input_port(struct amlAudioMixer *audio_mixer,
        struct audio_config *config,
        audio_output_flags_t flags,
        int (*on_notify_cbk)(void *data),
        void *notify_data,
        int (*on_input_avail_cbk)(void *data),
        void *input_avail_data,
        meta_data_cbk_t on_meta_data_cbk,
        void *meta_data,
        float volume)
{
    size_t buf_frames = MIXER_IN_FRAME_COUNT;
    struct input_port *port = NULL;
    enum MIXER_INPUT_PORT port_index;
    struct aml_stream_out *aml_out = notify_data;
    bool direct_on = false;

    if (audio_mixer == NULL || config == NULL || notify_data == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    /* if direct on, ie. the ALSA buffer is full, no need padding data anymore  */
    pthread_mutex_lock(&audio_mixer->inport_lock);
    direct_on = (audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_DIRECT] != NULL);
    port = new_input_port(buf_frames, config, flags, volume, direct_on);
    port_index = port->port_index;
    if (audio_mixer->in_ports[port_index] == NULL) {
        ALOGI("++%s port index %d, size %d frames", __func__, port_index, buf_frames);
        audio_mixer->in_ports[port_index] = port;
    } else {
        ALOGE("%s(), fatal error, inport index %d already exists!", __func__, port_index);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    set_port_notify_cbk(port, on_notify_cbk, notify_data);
    set_port_input_avail_cbk(port, on_input_avail_cbk, input_avail_data);
    if (on_meta_data_cbk && meta_data) {
        //mixer_set_hwsync_input_port(audio_mixer, port_index);
        set_inport_hwsync(port);
        set_port_meta_data_cbk(port, on_meta_data_cbk, meta_data);
    }

    port->initial_frames = aml_out->frame_write_sum;
    ALOGI("%s(), port->initial_frames: %lld", __func__, port->initial_frames);

    return 0;
}

int delete_mixer_input_port(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    ALOGI("++%s port ID: %d", __func__, port_index);
    pthread_mutex_lock(&audio_mixer->inport_lock);
    if (audio_mixer->in_ports[port_index]) {
        free_input_port(audio_mixer->in_ports[port_index]);
        audio_mixer->in_ports[port_index] = NULL;
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return 0;
}

int send_mixer_inport_message(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index , enum PORT_MSG msg)
{
    struct input_port *port;
    int ret;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];

    if (port == NULL) {
        ALOGE("%s(), port index %d, inval", __func__, port_index);
        ret = -EINVAL;
    } else {
        ret = send_inport_message(port, msg);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

void mixer_set_hwsync_input_port(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port;
    ALOGI("++%s port ID: %d", __func__, port_index);
    pthread_mutex_lock(&audio_mixer->inport_lock);

    port = audio_mixer->in_ports[port_index];
    if (port)
        set_inport_hwsync(port);

    pthread_mutex_unlock(&audio_mixer->inport_lock);
}

void set_mixer_hwsync_frame_size(struct amlAudioMixer *audio_mixer,
        uint32_t frame_size)
{
#if 0
    enum MIXER_INPUT_PORT port_index = 0;
    enum MIXER_OUTPUT_PORT out_port_index = MIXER_OUTPUT_PORT_PCM;
    struct input_port *in_port = NULL;
    struct output_port *out_port = audio_mixer->out_ports[out_port_index];
    int port_cnt = 0;
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port) {
            //resize_input_port_buffer(in_port, MIXER_IN_BUFFER_SIZE);
        }
    }

    //resize_output_port_buffer(out_port, MIXER_IN_BUFFER_SIZE);
#endif
    ALOGV("%s framesize %d", __func__, frame_size);
    audio_mixer->hwsync_frame_size = frame_size;
}

uint32_t get_mixer_hwsync_frame_size(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->hwsync_frame_size;
}

uint32_t get_mixer_inport_consumed_frames(
        struct amlAudioMixer *audio_mixer, enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port;
    uint32_t ret = 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
    } else {
        if (port->frame_size)
            ret = get_inport_consumed_size(port) / port->frame_size;
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

int set_mixer_inport_volume(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, float vol)
{
    struct input_port *port;
    int ret = 0;

    if (vol > 1.0 || vol < 0) {
        ALOGE("%s(), invalid vol %f", __func__, vol);
        return -EINVAL;
    }

    pthread_mutex_lock(&audio_mixer->inport_lock);

    port = audio_mixer->in_ports[port_index];
    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        ret = -EINVAL;
    } else {
        set_inport_volume(port, vol);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

float get_mixer_inport_volume(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port;
    float vol = 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
    } else {
        vol = get_inport_volume(port);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return vol;
}

int mixer_broadcast(struct amlAudioMixer *audio_mixer)
{
    pthread_cond_signal(&audio_mixer->cond);
    return 0;
}

int mixer_write_inport(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, const void *buffer, int bytes)
{
    struct input_port *port;
    int written = 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        pthread_mutex_unlock(&audio_mixer->inport_lock);
        return -EINVAL;
    }
    //if (get_inport_state(port) == RESUMING) {
    //    ALOGI("resuming port index %d", port_index);
        //audio_fade_func(buffer, bytes, 1);
        //aml_hwsync_set_tsync_resume();
    //}
    written = port->write(port, buffer, bytes);
    if (/*port_index == MIXER_INPUT_PORT_PCM_SYSTEM && */get_inport_state(port) != ACTIVE) {
        ALOGI("port index %d is active now", port_index);
        set_inport_state(port, ACTIVE);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);

    ALOGV("%s(), signal line %d portIndex %d",
            __func__, __LINE__, port_index);
    mixer_broadcast(audio_mixer);
    return written;
}

static int mixer_read_inport_l(struct input_port *port)
{
    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return 0;
    }
    return port->read(port, port->data, port->data_len_bytes);
}

int mixer_set_inport_state(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, enum port_state state)
{
    struct input_port *port;
    int ret = 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];
    if (port)
        ret = set_inport_state(port, state);
    else
        ret = -EINVAL;
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

enum port_state mixer_get_inport_state(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port;
    enum port_state st = IDLE;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];
    if (port)
        st = get_inport_state(port);
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return st;
}
//TODO: handle message queue
static void mixer_procs_msg_queue(struct amlAudioMixer *audio_mixer __unused)
{
    ALOGV("++%s start", __func__);
    return;
}

static struct output_port *get_outport(struct amlAudioMixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    return audio_mixer->out_ports[port_index];
}

size_t get_outport_data_avail(struct output_port *outport)
{
    return outport->bytes_avail;
}

int set_outport_data_avail(struct output_port *outport, size_t avail)
{
    if (avail > outport->data_buf_len) {
        ALOGE("%s(), invalid avail %u", __func__, avail);
        return -EINVAL;
    }
    outport->bytes_avail = avail;
    return 0;
}

static bool is_output_data_avail(struct amlAudioMixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    struct output_port *outport = NULL;

    ALOGV("++%s start", __func__);
    /* init check */
    //if(amlAudioMixer_check_status(audio_mixer))
    //    return false;

    outport = get_outport(audio_mixer, port_index);
    return !!get_outport_data_avail(outport);
    //return true;
}

int init_mixer_output_port(struct amlAudioMixer *audio_mixer,
        struct audioCfg cfg,
        size_t buf_frames)
{
    struct output_port *port = new_output_port(MIXER_OUTPUT_PORT_PCM,
            cfg, buf_frames);
    enum MIXER_OUTPUT_PORT port_index = port->port_index;

    audio_mixer->standby = 1;
    audio_mixer->out_ports[port_index] = port;
    audio_mixer->idle_sleep_time_us = (((audio_mixer->buf_frames * 1000) / cfg.sampleRate) * 1000) / 2;
    return 0;
}

int delete_mixer_output_port(struct amlAudioMixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    free_output_port(audio_mixer->out_ports[port_index]);
    audio_mixer->out_ports[port_index] = NULL;
    return 0;
}

static int mixer_output_startup(struct amlAudioMixer *audio_mixer)
{
    //enum MIXER_OUTPUT_PORT port_index = 0;
    struct output_port *out_port = audio_mixer->out_ports[0];

    ALOGI("++%s start open", __func__);
    return  out_port->start(out_port);
}

static int mixer_output_standby(struct amlAudioMixer *audio_mixer)
{
    //enum MIXER_OUTPUT_PORT port_index = 0;
    struct output_port *out_port = audio_mixer->out_ports[0];

    ALOGI("++%s standby", __func__);
    return out_port->standby(out_port);
}

static int mixer_output_write(struct amlAudioMixer *audio_mixer)
{
    int ret = 0;
    enum MIXER_OUTPUT_PORT port_index = 0;
    struct output_port *out_port = audio_mixer->out_ports[port_index];

    if (audio_mixer->standby) {
        ret = mixer_output_startup(audio_mixer);
    }
    if (ret) {
        ALOGE("%s cat not start output port", __func__);
        return ret;
    } else {
        audio_mixer->standby = 0;
    }

    out_port->sound_track_mode = audio_mixer->adev->sound_track_mode;
    while (is_output_data_avail(audio_mixer, port_index)) {
        // out_write_callbacks();
        ALOGV("++%s start", __func__);
        out_port->write(out_port, out_port->data_buf, out_port->bytes_avail);
        set_outport_data_avail(out_port, 0);
    };
    return 0;
}

#define DEFAULT_KERNEL_FRAMES (DEFAULT_PLAYBACK_PERIOD_SIZE*DEFAULT_PLAYBACK_PERIOD_CNT)

static int mixer_update_tstamp(struct amlAudioMixer *audio_mixer)
{
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    struct input_port *in_port;
    unsigned int avail;
    //struct timespec *timestamp;

    /*only deal with system audio */
    if (out_port == NULL)
        return 0;

    if (audio_mixer->standby)
        return 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    in_port = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_SYSTEM];
    if (in_port != NULL) {
        if (pcm_get_htimestamp(out_port->pcm_handle, &avail, &in_port->timestamp) == 0) {
            size_t kernel_buf_size = DEFAULT_KERNEL_FRAMES;
            int64_t signed_frames = in_port->mix_consumed_frames - kernel_buf_size + avail;
            if (signed_frames < 0) {
                signed_frames = 0;
            }
            in_port->presentation_frames = in_port->initial_frames + signed_frames;
            ALOGV("%s() present frames:%lld, initial %lld, consumed %lld, sec:%ld, nanosec:%ld",
                    __func__,
                    in_port->presentation_frames,
                    in_port->initial_frames,
                    in_port->mix_consumed_frames,
                    in_port->timestamp.tv_sec,
                    in_port->timestamp.tv_nsec);
        }
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);

    return 0;
}

static bool is_mixer_inports_ready(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    int port_cnt = 0, ready = 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        ALOGV("%s() port index %d, port ptr %p", __func__, port_index, in_port);
        if (in_port) {
            port_cnt++;
            if (in_port->rbuf_ready(in_port)) {
                ALOGV("port %d data ready", port_index);
                ready++;
            } else {
                ALOGV("port %d data not ready", port_index);
            }
        }
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);

    return (port_cnt == ready);
}

static inline float get_fade_step_by_size(int fade_size, int frame_size)
{
    return 1.0/(fade_size/frame_size);
}

int init_fade(struct fade_out *fade_out, int fade_size,
        int sample_size, int channel_cnt)
{
    fade_out->vol = 1.0;
    fade_out->target_vol = 0;
    fade_out->fade_size = fade_size;
    fade_out->sample_size = sample_size;
    fade_out->channel_cnt = channel_cnt;
    fade_out->stride = get_fade_step_by_size(fade_size, sample_size * channel_cnt);
    ALOGI("%s() size %d, stride %f", __func__, fade_size, fade_out->stride);
    return 0;
}

int process_fade_out(void *buf, int bytes, struct fade_out *fout)
{
    int i = 0;
    int frame_cnt = bytes / fout->sample_size / fout->channel_cnt;
    int16_t *sample = (int16_t *)buf;

    if (fout->channel_cnt != 2 || fout->sample_size != 2)
        ALOGE("%s(), not support yet", __func__);
    ALOGI("++++fade out vol %f, size %d", fout->vol, fout->fade_size);
    for (i = 0; i < frame_cnt; i++) {
        sample[i] = sample[i]*fout->vol;
        sample[i+1] = sample[i+1]*fout->vol;
        fout->vol -= fout->stride;
        if (fout->vol < 0)
            fout->vol = 0;
    }
    fout->fade_size -= bytes;
    ALOGI("----fade out vol %f, size %d", fout->vol, fout->fade_size);

    return 0;
}

static int update_inport_avail_l(struct input_port *in_port)
{
    // first throw away the padding frames
    if (in_port->padding_frames > 0) {
        in_port->padding_frames -= in_port->data_buf_frame_cnt;
        set_inport_pts_valid(in_port, false);
    } else {
        in_port->mix_consumed_frames += in_port->data_buf_frame_cnt;
        set_inport_pts_valid(in_port, true);
    }
    in_port->data_valid = 1;
    return 0;
}

static void process_port_msg(struct input_port *port)
{
    struct port_message *msg = get_inport_message(port);
    if (msg) {
        ALOGD("%s(), msg: %s", __func__, port_msg_to_str(msg->msg_what));
        switch (msg->msg_what) {
        case MSG_PAUSE:
            set_inport_state(port, PAUSING);
            break;
        case MSG_FLUSH:
            set_inport_state(port, FLUSHING);
            break;
        case MSG_RESUME:
            set_inport_state(port, RESUMING);
            break;
        default:
            ALOGE("%s(), not support", __func__);
        }

        remove_inport_message(port, msg);
    }
}

static int mixer_flush_inport_l(struct input_port *in_port)
{
    if (in_port) {
        return reset_input_port(in_port);
    }
    return -EINVAL;
}

static int notify_mixer_input_avail_l(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port && in_port->on_input_avail_cbk)
            in_port->on_input_avail_cbk(in_port->input_avail_cbk_data);
    }

    return 0;
}

static int mixer_inports_read(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    bool ready = false;
    bool direct_ready = false;
    bool is_direct_alive = false;

    ALOGV("++%s(), line %d", __func__, __LINE__);
    pthread_mutex_lock(&audio_mixer->inport_lock);
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        int ret = 0, fade_out = 0, fade_in = 0;
        //enum port_state state = ACTIVE;
        if (!in_port)
            continue;

        process_port_msg(in_port);
        enum port_state state = get_inport_state(in_port);

        if (port_index == MIXER_INPUT_PORT_PCM_DIRECT) {
            //if in pausing states, don't retrieve data
            if (state == PAUSING) {
                fade_out = 1;
            } else if (state == RESUMING) {
                fade_in = 1;
                ALOGI("%s(), tsync resume", __func__);
                aml_hwsync_set_tsync_resume();
                set_inport_state(in_port, ACTIVE);
            } else if (state == STOPPED || state == PAUSED || state == FLUSHED) {
                ALOGV("%s(), stopped, paused or flushed", __func__);
                //in_port->data_valid = 1;
                //memset(in_port->data, 0, in_port->data_len_bytes);
                continue;
            } else if (state == FLUSHING) {
                mixer_flush_inport_l(in_port);
                ALOGI("%s(), flushing->flushed", __func__);
                set_inport_state(in_port, FLUSHED);
                //in_port->data_valid = 1;
                //memset(in_port->data, 0, in_port->data_len_bytes);
                continue;
            }
        }
        // we are inserting frames
        if (in_port->data_valid) {
            ALOGV("%s() portIndex %d already valid", __func__, port_index);
            if (port_index == MIXER_INPUT_PORT_PCM_DIRECT && get_inport_state(in_port) == ACTIVE) {
                is_direct_alive = true;
                direct_ready = true;
                ready = true;
            } else if (port_index == MIXER_INPUT_PORT_PCM_SYSTEM) {
                ready = true;
            }
            continue;
        }
        if (port_index == MIXER_INPUT_PORT_PCM_DIRECT && get_inport_state(in_port) == ACTIVE)
            is_direct_alive = true;
        if (in_port->rbuf_ready(in_port)) {
            int read_cnt;
            read_cnt = mixer_read_inport_l(in_port);

            ALOGV("%s() read_cnt %d, portIndex %d", __func__, read_cnt, port_index);
            if (read_cnt == (int)in_port->data_len_bytes) {
                if (fade_out) {
                    ALOGI("%s(), fade out finished pausing->pausing_1", __func__);
                    ALOGI("%s(), tsync pause audio", __func__);
                    aml_hwsync_set_tsync_pause();
                    audio_fade_func(in_port->data, read_cnt, 0);
                    set_inport_state(in_port, PAUSED);
                } else if (fade_in) {
                    ALOGI("%s(), resuming port index %d", __func__, port_index);
                    ALOGI("%s(), tsync resume audio", __func__);
                    //aml_hwsync_set_tsync_resume();
                    audio_fade_func(in_port->data, read_cnt, 1);
                    set_inport_state(in_port, ACTIVE);
                }
                update_inport_avail_l(in_port);
                ready = true;
                if (port_index == MIXER_INPUT_PORT_PCM_DIRECT)
                    direct_ready = true;

                if (getprop_bool("media.audiohal.inport") &&
                        (in_port->port_index == MIXER_INPUT_PORT_PCM_DIRECT)) {
                    aml_audio_dump_audio_bitstreams("/data/audio/inportDirectFade.raw",
                            in_port->data, in_port->data_len_bytes);
                }
            }else {
                ALOGE("%s() read read_cnt = %d, portIndex %d", __func__, read_cnt, port_index);
            }
        }
    }
    notify_mixer_input_avail_l(audio_mixer);
    pthread_mutex_unlock(&audio_mixer->inport_lock);

    // if one port ready, propagate to do mixing out
    if (ready) {
        if (is_direct_alive && !direct_ready) {
            ALOGV("%s(), -----------------direct not ready, next turn", __func__);
            return -EAGAIN;
        }
        return 0;
    } else
        return -EAGAIN;
}

#if 0
static int check_mixer_state(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    int inport_avail = 0, inport_ready = 0;

    ALOGV("++%s(), line %d", __func__, __LINE__);
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port) {
            inport_avail = 1;

            // only when one or more inport is active, mixer is ready
            if (get_inport_state(in_port) == ACTIVE || get_inport_state(in_port) == PAUSING
                    || get_inport_state(in_port) == PAUSING_1)
                inport_ready = 1;
        }
    }

    if (inport_ready)
        audio_mixer->state = MIXER_INPORTS_READY;
    else if (inport_avail)
        audio_mixer->state = MIXER_INPORTS_ENABLED;
    else
        audio_mixer->state = MIXER_IDLE;

    return 0;
}

static int mixer_need_wait_forever(struct amlAudioMixer *audio_mixer)
{
    return mixer_get_state(audio_mixer) != MIXER_INPORTS_READY;
}

static int mixer_prepare_inport_data(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    ALOGV("++%s(), line %d", __func__, __LINE__);

    // in port data not ready
    if (!is_mixer_inports_ready(audio_mixer)) {
        struct timespec ts;
        ALOGV("++%s(), line %d, inport not ready", __func__, __LINE__);

        memset(&ts, 0, sizeof(struct timespec));
        ts_wait_time_us(&ts, 5000);
        pthread_mutex_lock(&audio_mixer->lock);
        check_mixer_state(audio_mixer);
        if (mixer_need_wait_forever(audio_mixer)) {
            ALOGI("%s(), wait forever %d", __func__, __LINE__);
            pthread_cond_wait(&audio_mixer->cond, &audio_mixer->lock);
            ALOGI("%s(),wait wakeup line %d", __func__, __LINE__);
        } else {
            ALOGV("%s(),wait timed %d", __func__, __LINE__);
            pthread_cond_timedwait(&audio_mixer->cond, &audio_mixer->lock, &ts);
            ALOGV("%s(),wait wakeup line %d", __func__, __LINE__);
        }
        ALOGV("%s(),wait wakeup line %d", __func__, __LINE__);
        pthread_mutex_unlock(&audio_mixer->lock);
    }

    if (is_mixer_inports_ready(audio_mixer)) {
        mixer_inports_read(audio_mixer);
    } else {
        struct input_port *in_port = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_DIRECT];
        if (in_port) {
            if (!in_port->rbuf_ready(in_port) && get_inport_state(in_port) == ACTIVE) {
                ALOGE("%s() inport data not ready, state (%d)", __func__, get_inport_state(in_port));
                usleep(5000);
                return -EAGAIN;
            }
        }
    }

    ALOGV("--%s(), line %d", __func__, __LINE__);
    return 0;
}
#endif

static inline int16_t CLIP16(int r)
{
    return (r >  0x7fff) ? 0x7fff :
           (r < -0x8000) ? 0x8000 :
           r;
}

static uint32_t hwsync_align_to_frame(uint32_t consumed_size, uint32_t frame_size)
{
    return consumed_size - (consumed_size % frame_size);
}

static int retrieve_hwsync_header(struct amlAudioMixer *audio_mixer,
        struct input_port *in_port, struct output_port *out_port)
{
    uint32_t frame_size = get_mixer_hwsync_frame_size(audio_mixer);
    uint32_t port_consumed_size = get_inport_consumed_size(in_port);
    uint32_t aligned_offset = 0;
    struct hw_avsync_header header;
    int ret = 0;

    if (frame_size == 0) {
        ALOGV("%s(), invalid frame size 0", __func__);
        return -EINVAL;
    }

    if (!in_port->is_hwsync) {
        ALOGE("%s(), not hwsync port", __func__);
        return -EINVAL;
    }

    aligned_offset = hwsync_align_to_frame(port_consumed_size, frame_size);
    memset(&header, 0, sizeof(struct hw_avsync_header));
    if (1) {
        int diff_ms = 0;
        ALOGV("direct out port bytes before cbk %d", get_outport_data_avail(out_port));
        if (!in_port->meta_data_cbk) {
            ALOGE("no meta_data_cbk set!!");
            return -EINVAL;
        }
        ALOGV("%s(), port %p, data %p", __func__, in_port, in_port->meta_data_cbk_data);
        ret = in_port->meta_data_cbk(in_port->meta_data_cbk_data,
                    aligned_offset, &header, &diff_ms);
        if (ret < 0) {
            if (ret != -EAGAIN)
                ALOGE("meta_data_cbk fail err = %d!!", ret);
            return ret;
        }
        ALOGV("%s(), meta data cbk, diffms = %d", __func__, diff_ms);
        if (diff_ms > 0) {
            in_port->bytes_to_insert = diff_ms * 48 * 4;
        } else if (diff_ms < 0) {
            in_port->bytes_to_skip = -diff_ms * 48 * 4;
        }
    }

    return 0;
}

static int mixer_do_mixing_32bit(struct amlAudioMixer *audio_mixer)
{
    int ret;
    struct input_port *in_port_sys,*in_port_drct;
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    struct aml_audio_device *adev = audio_mixer->adev;
    int16_t *data_sys, *data_drct, *data_mixed;
    int mixing = 0, sys_only = 0, direct_only = 0;
    int dirct_okay = 0, sys_okay = 0;
    float dirct_vol = 1.0, sys_vol = 1.0;
    int mixed_32 = 0;
    size_t i = 0, mixing_len_bytes = 0;
    size_t frames = 0;
    size_t frames_written = 0;
    float gain_speaker = adev->sink_gain[OUTPORT_SPEAKER];

    if (!out_port) {
        ALOGE("%s(), out null !!!", __func__);
        return 0;
    }

    pthread_mutex_lock(&audio_mixer->inport_lock);

    in_port_sys = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_SYSTEM];
    in_port_drct = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_DIRECT];

    if (!in_port_sys && !in_port_drct) {
        ALOGE("%s(), sys or direct pcm must exist!!!", __func__);
        ret = -1;
        goto _error;
    }

    if (in_port_sys && in_port_sys->data_valid) {
        sys_okay = 1;
    }
    if (in_port_drct && in_port_drct->data_valid) {
        dirct_okay = 1;
    }
    if (sys_okay && dirct_okay) {
        mixing = 1;
    } else if (dirct_okay) {
        ALOGV("only direct okay");
        direct_only = 1;
    } else if (sys_okay) {
        sys_only = 1;
    } else {
        ALOGV("%s(), sys direct both not ready!", __func__);
        ret = -EINVAL;
        goto _error;
    }

    data_mixed = (int16_t *)out_port->data_buf;
    memset(audio_mixer->tmp_buffer, 0 , audio_mixer->buf_frames * 8);
    if (mixing) {
        ALOGV("%s() mixing", __func__);
        data_sys = (int16_t *)in_port_sys->data;
        data_drct = (int16_t *)in_port_drct->data;
        mixing_len_bytes = in_port_drct->data_len_bytes;
        //TODO: check if the two stream's frames are equal
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, in_port_drct->data_len_bytes);
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, in_port_sys->data_len_bytes);
        }
        if (is_inport_hwsync(in_port_drct) && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
        if (frames > MIXER_OUT_FRAME_COUNT) {
            ALOGE("%s() %d too many frames %d", __func__, __LINE__, frames);
            frames = MIXER_OUT_FRAME_COUNT;
        }

        // insert data for direct hwsync case, only send system sound
        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            ALOGD("%s() insert mixing data, need %zu, insert length %zu",
                    __func__, in_port_drct->bytes_to_insert, mixing_len_bytes);
            //memcpy(data_mixed, data_sys, mixing_len_bytes);
            //memcpy(audio_mixer->tmp_buffer, data_sys, mixing_len_bytes);
            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/systbeforemix.raw",
                        data_sys, in_port_sys->data_len_bytes);
            }
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);

            in_port_drct->bytes_to_insert -= mixing_len_bytes;
            in_port_sys->data_valid = 0;
            pthread_mutex_unlock(&audio_mixer->inport_lock);

            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/sysAftermix.raw",
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
            }
            apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);
#ifdef IS_ATOM_PROJECT
            if (adev->has_dsp_lib) {
                dsp_process_output(audio_mixer->adev,
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
                extend_channel_5_8(data_mixed,
                        audio_mixer->adev->effect_buf, frames, 5, 8);
            } else
#endif
                extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                        frames, 2, 8);

            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/dataInsertMixed.raw",
                        data_mixed, frames * out_port->cfg.frame_size);
            }
            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        } else {
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_drct,
                frames, in_port_drct->cfg, out_port->cfg);
            if (DEBUG_DUMP)
                aml_audio_dump_audio_bitstreams("/data/audio/tmpMixed0.raw",
                    audio_mixer->tmp_buffer, frames * audio_mixer->frame_size_tmp);
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);

            in_port_drct->data_valid = 0;
            in_port_sys->data_valid = 0;
            pthread_mutex_unlock(&audio_mixer->inport_lock);

            if (DEBUG_DUMP)
                aml_audio_dump_audio_bitstreams("/data/audio/tmpMixed1.raw",
                    audio_mixer->tmp_buffer, frames * audio_mixer->frame_size_tmp);
            apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);
#ifdef IS_ATOM_PROJECT
            if (adev->has_dsp_lib) {
                dsp_process_output(audio_mixer->adev,
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
                extend_channel_5_8(data_mixed,
                        audio_mixer->adev->effect_buf, frames, 5, 8);
            } else
#endif
                extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                        frames, 2, 8);

            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        }
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/data_mixed.raw",
                out_port->data_buf, frames * out_port->cfg.frame_size);
        }
    } else if (sys_only) {
        frames = in_port_sys->data_buf_frame_cnt;
        if (frames > MIXER_OUT_FRAME_COUNT) {
            ALOGE("%s() %d too many frames %d", __func__, __LINE__, frames);
            frames = MIXER_OUT_FRAME_COUNT;
        }

        ALOGV("%s() sys_only, frames %d", __func__, frames);
        mixing_len_bytes = in_port_sys->data_len_bytes;
        data_sys = (int16_t *)in_port_sys->data;
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, mixing_len_bytes);
        }
        // processing data and make convertion according to cfg
        // processing_and_convert(data_mixed, data_sys, frames, in_port_sys->cfg, out_port->cfg);
        frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);

        in_port_sys->data_valid = 0;
        pthread_mutex_unlock(&audio_mixer->inport_lock);

        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/sysTmp.raw",
                    audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
        }
        apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/sysvol.raw",
                    audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
        }
#ifdef IS_ATOM_PROJECT
        if (adev->has_dsp_lib) {
            dsp_process_output(audio_mixer->adev,
                    audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
            extend_channel_5_8(data_mixed,
                    audio_mixer->adev->effect_buf, frames, 5, 8);
        } else
#endif
            extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer, frames, 2, 8);

        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/extandsys.raw",
                    data_mixed, frames * out_port->cfg.frame_size);
        }
        set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
    } else if (direct_only) {
        ALOGV("%s() direct_only", __func__);
        //dirct_vol = get_inport_volume(in_port_drct);
        mixing_len_bytes = in_port_drct->data_len_bytes;
        data_drct = (int16_t *)in_port_drct->data;
        ALOGV("%s() direct_only, inport consumed %d",
                __func__, get_inport_consumed_size(in_port_drct));

        if (is_inport_hwsync(in_port_drct) && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, mixing_len_bytes);
        }

        // insert 0 data to delay audio
        frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
        if (frames > MIXER_OUT_FRAME_COUNT) {
            ALOGE("%s() %d too many frames %d", __func__, __LINE__, frames);
            frames = MIXER_OUT_FRAME_COUNT;
        }

        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            ALOGD("%s() inserting direct_only, need %zu, insert length %zu",
                    __func__, in_port_drct->bytes_to_insert, mixing_len_bytes);
            memset(data_mixed, 0, mixing_len_bytes);
            extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                    frames, 2, 8);
            in_port_drct->bytes_to_insert -= mixing_len_bytes;

            pthread_mutex_unlock(&audio_mixer->inport_lock);
            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        } else {
            ALOGV("%s() direct_only, vol %f", __func__, dirct_vol);
            //cpy_16bit_data_with_gain(data_mixed, data_drct,
            //        in_port_drct->data_len_bytes, dirct_vol);
            ALOGV("%s() direct_only, frames %d, bytes %d", __func__, frames, mixing_len_bytes);

            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_drct,
                frames, in_port_drct->cfg, out_port->cfg);
            in_port_drct->data_valid = 0;
            pthread_mutex_unlock(&audio_mixer->inport_lock);

            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/dirctTmp.raw",
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
            }
            apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint32_t), frames * FRAMESIZE_32BIT_STEREO);

#ifdef IS_ATOM_PROJECT
            if (adev->has_dsp_lib) {
                dsp_process_output(audio_mixer->adev,
                        audio_mixer->tmp_buffer, frames * FRAMESIZE_32BIT_STEREO);
                extend_channel_5_8(data_mixed,
                        audio_mixer->adev->effect_buf, frames, 5, 8);
            } else
#endif
                extend_channel_2_8(data_mixed, audio_mixer->tmp_buffer,
                        frames, 2, 8);

            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/exDrct.raw",
                        data_mixed, frames * out_port->cfg.frame_size);
            }
            set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
        }
    }

    if (0) {
        aml_audio_dump_audio_bitstreams("/data/audio/data_mixed.raw",
                out_port->data_buf, mixing_len_bytes);
    }
    return 0;

_error:
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

static int mixer_do_mixing_16bit(struct amlAudioMixer *audio_mixer)
{
    int ret;
    struct input_port *in_port_sys, *in_port_drct;
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    struct aml_audio_device *adev = audio_mixer->adev;
    int16_t *data_sys, *data_drct, *data_mixed;
    int mixing = 0, sys_only = 0, direct_only = 0;
    int dirct_okay = 0, sys_okay = 0;
    float dirct_vol = 1.0, sys_vol = 1.0;
    int mixed_32 = 0;
    size_t i = 0, mixing_len_bytes = 0;
    size_t frames = 0;
    size_t frames_written = 0;
    float gain_speaker = adev->sink_gain[OUTPORT_SPEAKER];
    float gain_outport = adev->sink_gain[adev->active_outport];
    ALOGV("%s(), speaker gain %f, outport gain %f",
        __func__, gain_speaker, gain_outport);

    if (!out_port) {
        ALOGE("%s(), out null !!!", __func__);
        return 0;
    }

    pthread_mutex_lock(&audio_mixer->inport_lock);

    in_port_sys = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_SYSTEM];
    in_port_drct = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_DIRECT];

    if (!in_port_sys && !in_port_drct) {
        ALOGE("%s(), sys or direct pcm must exist!!!", __func__);
        ret = -1;
        goto _error;
    }

    if (in_port_sys) {
        if (in_port_sys->data_valid)
            sys_okay = 1;
        else
            ALOGV("%s(), sys port ready, but no valid data, maybe underrun", __func__);
    }
    if (in_port_drct) {
        if (in_port_drct->data_valid)
            dirct_okay = 1;
        else {
            ALOGW("%s(), direct port ready, but no valid data, maybe underrun, state%d",
                __func__, get_inport_state(in_port_drct));
        //dirct_okay = 1;
        }
    }
    if (sys_okay && dirct_okay) {
        mixing = 1;
    } else if (dirct_okay) {
        ALOGV("only direct okay");
        direct_only = 1;
    } else if (sys_okay) {
        sys_only = 1;
    } else {
        ALOGV("%s(), sys direct both not ready!", __func__);
        ret = -EINVAL;
        goto _error;
    }

    data_mixed = (int16_t *)out_port->data_buf;
    memset(audio_mixer->tmp_buffer, 0 , audio_mixer->buf_frames * 8);
    if (mixing) {
        ALOGV("%s() mixing", __func__);
        data_sys = (int16_t *)in_port_sys->data;
        data_drct = (int16_t *)in_port_drct->data;
        mixing_len_bytes = in_port_drct->data_len_bytes;
        //TODO: check if the two stream's frames are equal
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, in_port_drct->data_len_bytes);
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, in_port_sys->data_len_bytes);
        }
        if (is_inport_hwsync(in_port_drct) && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
        if (frames > MIXER_OUT_FRAME_COUNT) {
            ALOGE("%s() %d too many frames %d", __func__, __LINE__, frames);
            frames = MIXER_OUT_FRAME_COUNT;
        }
        // insert data for direct hwsync case, only send system sound
        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            ALOGD("%s() insert mixing data, need %zu, insert length %zu",
                    __func__, in_port_drct->bytes_to_insert, mixing_len_bytes);
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                    frames, in_port_sys->cfg, out_port->cfg);

            in_port_drct->bytes_to_insert -= mixing_len_bytes;
            in_port_sys->data_valid = 0;
            pthread_mutex_unlock(&audio_mixer->inport_lock);

            memcpy(data_mixed, audio_mixer->tmp_buffer, frames * out_port->cfg.frame_size);
        } else {
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_drct,
                frames, in_port_drct->cfg, out_port->cfg);
            if (DEBUG_DUMP)
                aml_audio_dump_audio_bitstreams("/data/audio/tmpMixed0.raw",
                    audio_mixer->tmp_buffer, frames * audio_mixer->frame_size_tmp);
            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);

            in_port_drct->data_valid = 0;
            in_port_sys->data_valid = 0;
            pthread_mutex_unlock(&audio_mixer->inport_lock);

            if (DEBUG_DUMP)
                aml_audio_dump_audio_bitstreams("/data/audio/tmpMixed1.raw",
                    audio_mixer->tmp_buffer, frames * audio_mixer->frame_size_tmp);
            apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint16_t), frames * out_port->cfg.frame_size);

            memcpy(data_mixed, audio_mixer->tmp_buffer, frames * out_port->cfg.frame_size);
        }
        set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
    } else if (sys_only) {
        frames = in_port_sys->data_buf_frame_cnt;
        if (frames > MIXER_OUT_FRAME_COUNT) {
            ALOGE("%s() %d too many frames %d", __func__, __LINE__, frames);
            frames = MIXER_OUT_FRAME_COUNT;
        }
        ALOGV("%s() sys_only, frames %d", __func__, frames);
        mixing_len_bytes = in_port_sys->data_len_bytes;
        data_sys = (int16_t *)in_port_sys->data;
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, mixing_len_bytes);
        }
        // processing data and make convertion according to cfg
        // processing_and_convert(data_mixed, data_sys, frames, in_port_sys->cfg, out_port->cfg);
        frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_sys,
                frames, in_port_sys->cfg, out_port->cfg);
        in_port_sys->data_valid = 0;
        pthread_mutex_unlock(&audio_mixer->inport_lock);

        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/sysTmp.raw",
                    audio_mixer->tmp_buffer, frames * out_port->cfg.frame_size);
        }

        apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                sizeof(uint16_t), frames * out_port->cfg.frame_size);
        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/sysvol.raw",
                    audio_mixer->tmp_buffer, frames * out_port->cfg.frame_size);
        }

        memcpy(data_mixed, audio_mixer->tmp_buffer, frames * out_port->cfg.frame_size);
        set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
    } else if (direct_only) {
        ALOGV("%s() direct_only", __func__);
        //dirct_vol = get_inport_volume(in_port_drct);
        mixing_len_bytes = in_port_drct->data_len_bytes;
        data_drct = (int16_t *)in_port_drct->data;
        ALOGV("%s() direct_only, inport consumed %d",
                __func__, get_inport_consumed_size(in_port_drct));

        if (is_inport_hwsync(in_port_drct) && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        if (DEBUG_DUMP) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, mixing_len_bytes);
        }
        // insert 0 data to delay audio
        frames = mixing_len_bytes / in_port_drct->cfg.frame_size;
        if (frames > MIXER_OUT_FRAME_COUNT) {
            ALOGE("%s() %d too many frames %d", __func__, __LINE__, frames);
            frames = MIXER_OUT_FRAME_COUNT;
        }

        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            ALOGD("%s() inserting direct_only, need %zu, insert length %zu",
                    __func__, in_port_drct->bytes_to_insert, mixing_len_bytes);
            in_port_drct->bytes_to_insert -= mixing_len_bytes;
            pthread_mutex_unlock(&audio_mixer->inport_lock);

            memset(data_mixed, 0, mixing_len_bytes);
        } else {
            ALOGV("%s() direct_only, vol %f", __func__, dirct_vol);
            ALOGV("%s() direct_only, frames %d, bytes %d", __func__, frames, mixing_len_bytes);

            frames_written = do_mixing_2ch(audio_mixer->tmp_buffer, data_drct,
                frames, in_port_drct->cfg, out_port->cfg);
            in_port_drct->data_valid = 0;
            pthread_mutex_unlock(&audio_mixer->inport_lock);

            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/dirctTmp.raw",
                        audio_mixer->tmp_buffer, frames * out_port->cfg.frame_size);
            }
            apply_volume(gain_speaker, audio_mixer->tmp_buffer,
                    sizeof(uint16_t), frames * out_port->cfg.frame_size);

            memcpy(data_mixed, audio_mixer->tmp_buffer, frames * out_port->cfg.frame_size);
            if (DEBUG_DUMP) {
                aml_audio_dump_audio_bitstreams("/data/audio/volumeDirect.raw",
                        data_mixed, frames * out_port->cfg.frame_size);
            }
        }
        set_outport_data_avail(out_port, frames * out_port->cfg.frame_size);
    }
    return 0;

_error:
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

int notify_mixer_exit(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    pthread_mutex_lock(&audio_mixer->inport_lock);
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port && in_port->on_notify_cbk)
            in_port->on_notify_cbk(in_port->notify_cbk_data);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);

    return 0;
}

static int set_thread_affinity(void)
{
    cpu_set_t cpuSet;
    int sastat = 0;

    CPU_ZERO(&cpuSet);
    CPU_SET(2, &cpuSet);
    CPU_SET(3, &cpuSet);
    sastat = sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
    if (sastat) {
        ALOGW("%s(), failed to set cpu affinity", __FUNCTION__);
        return sastat;
    }

    return 0;
}

static int mixer_do_continous_output(struct amlAudioMixer *audio_mixer)
{
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    int16_t *data_mixed = (int16_t *)out_port->data_buf;
    size_t frames = 4;
    size_t bytes = frames * out_port->cfg.frame_size;

    memset(data_mixed, 0 , bytes);
    set_outport_data_avail(out_port, bytes);
    mixer_output_write(audio_mixer);
    return 0;
}

static bool mixer_inports_exist(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    pthread_mutex_lock(&audio_mixer->inport_lock);
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port) {
            pthread_mutex_unlock(&audio_mixer->inport_lock);
            return true;
        }
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return false;
}

#define THROTTLE_TIME_US 5000
static void *mixer_32b_threadloop(void *data)
{
    struct amlAudioMixer *audio_mixer = data;
    enum MIXER_OUTPUT_PORT port_index = MIXER_OUTPUT_PORT_PCM;
    enum MIXER_INPUT_PORT in_index = MIXER_INPUT_PORT_PCM_SYSTEM;
    int ret = 0;

    ALOGI("++%s start", __func__);

    audio_mixer->exit_thread = 0;
    prctl(PR_SET_NAME, "amlAudioMixer32");
    set_thread_affinity();
    while (!audio_mixer->exit_thread) {
        //pthread_mutex_lock(&audio_mixer->lock);
        //mixer_procs_msg_queue(audio_mixer);
#if 0
        // TODO: throttle rate
        if (!is_output_data_avail(audio_mixer, port_index)) {
            ret = mixer_prepare_inport_data(audio_mixer);
            if (ret < 0) {
                notify_mixer_input_avail(audio_mixer);
                continue;
            }
            ALOGV("%s %d mixing", __func__, __LINE__);
            mixer_do_mixing(audio_mixer);
            //ALOGI("%s %d", __func__, __LINE__);
            notify_mixer_input_avail(audio_mixer);
            //ALOGI("%s %d", __func__, __LINE__);
        } else {
            mixer_do_mixing(audio_mixer);
            ALOGW("%s %d, data already avail, should not be here####", __func__, __LINE__);
            notify_mixer_input_avail(audio_mixer);
        }

        //Retrieve buffers to "bufferout", and fill them into shared buffer
        mixer_output_write(audio_mixer);
#endif
        // processing throttle
        struct timespec tval_new;
        clock_gettime(CLOCK_MONOTONIC, &tval_new);
        const uint32_t delta_us = tspec_diff_to_us(audio_mixer->tval_last_run, tval_new);

        if (delta_us <= THROTTLE_TIME_US) {
            struct timespec ts;
            uint32_t throttle_us = THROTTLE_TIME_US - delta_us;

            ALOGV("%s throttle time_us %d", __func__, throttle_us);
            //usleep(delta_us);
            ts_wait_time_us(&ts, throttle_us);
            pthread_mutex_lock(&audio_mixer->lock);
            pthread_cond_timedwait(&audio_mixer->cond, &audio_mixer->lock, &ts);
            //pthread_cond_wait(&audio_mixer->cond, &audio_mixer->lock);
            pthread_mutex_unlock(&audio_mixer->lock);
            if (audio_mixer->exit_thread)
                break;
        }

        ret = mixer_inports_read(audio_mixer);
        if (ret < 0) {
            struct timespec ts_now;
            uint64_t delta_us;

            ALOGV("%s %d data not enough, next turn", __func__, __LINE__);
            if (mixer_is_continuous_enabled(audio_mixer) && !mixer_inports_exist(audio_mixer)) {
                ALOGI("%s %d data not enough, do continue output", __func__, __LINE__);
                mixer_do_continous_output(audio_mixer);
                clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_write);
            } else {
                clock_gettime(CLOCK_MONOTONIC, &tval_new);
                delta_us = tspec_diff_to_us(audio_mixer->tval_last_write, tval_new);
                if (delta_us >= STANDBY_TIME_US) {
                    if (!audio_mixer->standby) {
                        mixer_output_standby(audio_mixer);
                        audio_mixer->standby = 1;
                    }
                    pthread_mutex_lock(&audio_mixer->lock);
                    ALOGV("%s() sleep", __func__);
                    pthread_cond_wait(&audio_mixer->cond, &audio_mixer->lock);
                    ALOGV("%s() wakeup", __func__);
                    pthread_mutex_unlock(&audio_mixer->lock);
                }
            }
            clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_run);
            continue;
        }
        ALOGV("%s %d do mixing", __func__, __LINE__);
        mixer_do_mixing_32bit(audio_mixer);
        // audio patching should not in this write
        // TODO: fix me, make compatible with source output
        if (!audio_mixer->adev->audio_patching) {
            mixer_output_write(audio_mixer);
            mixer_update_tstamp(audio_mixer);
            clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_write);
        }
        clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_run);
    }

    ALOGI("--%s", __func__);
    return NULL;
}

int mixerIdleSleepTimeUs(struct amlAudioMixer *audio_mixer)
{
    return audio_mixer->idle_sleep_time_us;
}

static void *mixer_16b_threadloop(void *data)
{
    struct amlAudioMixer *audio_mixer = data;
    enum MIXER_OUTPUT_PORT port_index = MIXER_OUTPUT_PORT_PCM;
    enum MIXER_INPUT_PORT in_index = MIXER_INPUT_PORT_PCM_SYSTEM;
    int ret = 0;

    ALOGI("++%s start", __func__);
    audio_mixer->exit_thread = 0;
    prctl(PR_SET_NAME, "amlAudioMixer16");
    set_thread_affinity();
    while (!audio_mixer->exit_thread) {
        // processing throttle
        struct timespec tval_new;
        clock_gettime(CLOCK_MONOTONIC, &tval_new);
        uint32_t delta_us = tspec_diff_to_us(audio_mixer->tval_last_run, tval_new);

        if (delta_us <= THROTTLE_TIME_US) {
            struct timespec ts;
            uint32_t throttle_us = THROTTLE_TIME_US - delta_us;

            ALOGV("%s throttle time_us %d", __func__, throttle_us);
            ts_wait_time_us(&ts, throttle_us);
            pthread_mutex_lock(&audio_mixer->lock);
            pthread_cond_timedwait(&audio_mixer->cond, &audio_mixer->lock, &ts);
            pthread_mutex_unlock(&audio_mixer->lock);
            if (audio_mixer->exit_thread)
                break;
        }
        ret = mixer_inports_read(audio_mixer);
        if (ret < 0) {
            struct timespec ts_now;
            uint64_t delta_us;

            ALOGV("%s %d data not enough, next turn", __func__, __LINE__);
            if (mixer_is_continuous_enabled(audio_mixer) && !mixer_inports_exist(audio_mixer)) {
                ALOGV("%s %d data not enough, do continue output", __func__, __LINE__);
                mixer_do_continous_output(audio_mixer);
                clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_write);
            } else {
                clock_gettime(CLOCK_MONOTONIC, &tval_new);
                delta_us = tspec_diff_to_us(audio_mixer->tval_last_write, tval_new);
                if (delta_us >= STANDBY_TIME_US) {
                    if (!audio_mixer->standby) {
                        mixer_output_standby(audio_mixer);
                        audio_mixer->standby = 1;
                    }
                    pthread_mutex_lock(&audio_mixer->lock);
                    ALOGV("%s() sleep", __func__);
                    pthread_cond_wait(&audio_mixer->cond, &audio_mixer->lock);
                    ALOGV("%s() wakeup", __func__);
                    pthread_mutex_unlock(&audio_mixer->lock);
                }
            }
            clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_run);
            continue;
        }
        ALOGV("%s %d do mixing", __func__, __LINE__);
        mixer_do_mixing_16bit(audio_mixer);
        if (!audio_mixer->adev->audio_patching) {
            mixer_output_write(audio_mixer);
            mixer_update_tstamp(audio_mixer);
            clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_write);
        }
        clock_gettime(CLOCK_MONOTONIC, &audio_mixer->tval_last_run);
    }

    ALOGI("--%s", __func__);
    return NULL;
}

uint32_t mixer_get_inport_latency_frames(struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port;
    uint32_t frame = 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];
    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
    } else {
        frame = port->get_latency_frames(port);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);

    return frame;
}

uint32_t mixer_get_outport_latency_frames(struct amlAudioMixer *audio_mixer)
{
    struct output_port *port = get_outport(audio_mixer, MIXER_OUTPUT_PORT_PCM);
    if (port == NULL) {
        ALOGE("%s(), port invalid", __func__);
    }
    return outport_get_latency_frames(port);
}

int pcm_mixer_thread_run(struct amlAudioMixer *audio_mixer)
{
    struct output_port *out_pcm_port = NULL;
    int ret = 0;

    ALOGI("++%s()", __func__);
    if (audio_mixer == NULL) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    out_pcm_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    if (out_pcm_port == NULL) {
        ALOGE("%s(), out port not initialized", __func__);
        return -EINVAL;
    }

    if (audio_mixer->out_mixer_tid > 0) {
        ALOGE("%s(), out mixer thread already running", __func__);
        return -EINVAL;
    }
    switch (out_pcm_port->cfg.format) {
    case AUDIO_FORMAT_PCM_32_BIT:
        ret = pthread_create(&audio_mixer->out_mixer_tid,
                NULL, mixer_32b_threadloop, audio_mixer);
        if (ret < 0)
            ALOGE("%s() thread run failed.", __func__);
        break;
    case AUDIO_FORMAT_PCM_16_BIT:
        ret = pthread_create(&audio_mixer->out_mixer_tid,
                NULL, mixer_16b_threadloop, audio_mixer);
        if (ret < 0)
            ALOGE("%s() thread run failed.", __func__);
        break;
    default:
        ALOGE("%s(), format not supported", __func__);
        break;
    }

    return ret;
}

int pcm_mixer_thread_exit(struct amlAudioMixer *audio_mixer)
{
    ALOGD("+%s()", __func__);
    // block exit
    audio_mixer->exit_thread = 1;
    pthread_cond_broadcast(&audio_mixer->cond);
    pthread_join(audio_mixer->out_mixer_tid, NULL);
    audio_mixer->out_mixer_tid = 0;

    notify_mixer_exit(audio_mixer);
    return 0;
}
struct amlAudioMixer *newAmlAudioMixer(
        struct audioCfg cfg,
        struct aml_audio_device *adev)
{
    struct amlAudioMixer *audio_mixer = NULL;
    int ret = 0;
    ALOGD("%s()", __func__);

    audio_mixer = calloc(1, sizeof(*audio_mixer));
    if (audio_mixer == NULL) {
        ALOGE("%s(), no memory", __func__);
        return NULL;
    }

    // 2 channel  32bit
    audio_mixer->tmp_buffer = calloc(1, MIXER_OUT_FRAME_COUNT * 8);
    if (audio_mixer->tmp_buffer == NULL) {
        ALOGE("%s(), no memory", __func__);
        goto err_tmp;
    }
    audio_mixer->buf_frames = MIXER_OUT_FRAME_COUNT;
    // 2 channel X sample bytes;
    audio_mixer->frame_size_tmp = 2 * audio_bytes_per_sample(cfg.format);

    mixer_set_state(audio_mixer, MIXER_IDLE);
    ret = init_mixer_output_port(audio_mixer,
            cfg, MIXER_OUT_FRAME_COUNT);
    if (ret < 0) {
        ALOGE("%s(), init mixer out port failed", __func__);
        goto err_state;
    }
    audio_mixer->adev = adev;
    pthread_mutex_init(&audio_mixer->inport_lock, NULL);
    pthread_mutex_init(&audio_mixer->lock, NULL);
    pthread_cond_init(&audio_mixer->cond, NULL);
    return audio_mixer;

err_state:
    free(audio_mixer->tmp_buffer);
    audio_mixer->tmp_buffer = NULL;
err_tmp:
    free(audio_mixer);
    audio_mixer = NULL;

    return audio_mixer;
}

void freeAmlAudioMixer(struct amlAudioMixer *audio_mixer)
{
    if (audio_mixer) {
        pthread_mutex_destroy(&audio_mixer->inport_lock);
        pthread_mutex_destroy(&audio_mixer->lock);
        pthread_cond_destroy(&audio_mixer->cond);
        delete_mixer_output_port(audio_mixer, MIXER_OUTPUT_PORT_PCM);
        free(audio_mixer);
    }
}

int64_t mixer_latency_frames(struct amlAudioMixer *audio_mixer)
{
    (void)audio_mixer;
    /* TODO: calc the mixer buf latency
    * Now using estimated buffer length
    */
    return MIXER_IN_FRAME_COUNT;
}

int mixer_get_presentation_position(
        struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index,
        uint64_t *frames,
        struct timespec *timestamp)
{
    struct input_port *port;
    int ret = 0;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGV("%s(), port not ready now", __func__);
        ret = -EINVAL;
    } else {
        *frames = port->presentation_frames;
        *timestamp = port->timestamp;
        if (!is_inport_pts_valid(port)) {
            ALOGV("%s(), not valid now", __func__);
            ret = -EINVAL;
        }
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

int mixer_set_padding_size(
        struct amlAudioMixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index,
        int padding_bytes)
{
    struct input_port *port;
    int ret;

    pthread_mutex_lock(&audio_mixer->inport_lock);
    port = audio_mixer->in_ports[port_index];
    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        ret = -EINVAL;
    } else  {
        ret = set_inport_padding_size(port, padding_bytes);
    }
    pthread_mutex_unlock(&audio_mixer->inport_lock);
    return ret;
}

int mixer_stop_outport_pcm(struct amlAudioMixer *audio_mixer)
{
    enum MIXER_OUTPUT_PORT port_index = MIXER_OUTPUT_PORT_PCM;
    struct output_port *port = audio_mixer->out_ports[port_index];
    if (!port) {
        ALOGE("%s(), NULL pointer", __func__);
        return -EINVAL;
    }

    return outport_stop_pcm(port);
}
