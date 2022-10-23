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

#define LOG_TAG "audio_hw_mixer"
//#define LOG_NDEBUG 0

#define __USE_GNU
#include <cutils/log.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <system/audio.h>

#include "aml_audio_mixer.h"
#include "audio_hw_utils.h"
#include "hw_avsync.h"
#include "audio_hwsync.h"

#define MIXER_IN_BUFFER_SIZE (1024*4)
#define MIXER_OUT_BUFFER_SIZE MIXER_IN_BUFFER_SIZE

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
struct aml_audio_mixer {
    struct input_port *in_ports[MIXER_INPUT_PORT_NUM];
    struct output_port *out_ports[MIXER_OUTPUT_PORT_NUM];
    pthread_mutex_t inport_lock;
    ssize_t (*write)(struct aml_audio_mixer *mixer, void *buffer, int bytes);
    //struct pcm_config mixer_cfg;
    //int period_time;
    //void *out_buffer;
    //void *aux_buffer;
    //int out_buffer_size;
    uint32_t hwsync_frame_size;
    pthread_t out_mixer_tid;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int exit_thread : 1;
    int mixing_enable : 1;
    enum mixer_state state;
    uint64_t last_process_finished_ns;
    //int init_ok : 1;
};

int mixer_set_state(struct aml_audio_mixer *audio_mixer, enum mixer_state state)
{
    audio_mixer->state = state;
    return 0;
}

enum mixer_state mixer_get_state(struct aml_audio_mixer *audio_mixer)
{
    return audio_mixer->state;
}

int init_mixer_input_port(struct aml_audio_mixer *audio_mixer,
        struct audio_config *config,
        audio_output_flags_t flags,
        int (*on_notify_cbk)(void *data),
        void *notify_data,
        int (*on_input_avail_cbk)(void *data),
        void *input_avail_data,
        //int (*on_meta_data_cbk)(void *cookie,
        //        uint32_t offset,
        //        struct hw_avsync_header *header,
        //        int *delay_ms),
        meta_data_cbk_t on_meta_data_cbk,
        void *meta_data)
{
    //size_t buf_size = get_mixer_hwsync_frame_size(audio_mixer);
    size_t buf_size = 0;
    struct input_port *port = NULL;
    enum MIXER_INPUT_PORT port_index;

    if (buf_size == 0) {
        buf_size = MIXER_IN_BUFFER_SIZE;
    }

    port = new_input_port(buf_size, config, flags);
    port_index = port->port_index;
    ALOGI("++%s port index %d, size %d", __func__, port_index, buf_size);
    audio_mixer->in_ports[port_index] = port;
    set_port_notify_cbk(port, on_notify_cbk, notify_data);
    set_port_input_avail_cbk(port, on_input_avail_cbk, input_avail_data);
    if (on_meta_data_cbk && meta_data) {
        //mixer_set_hwsync_input_port(audio_mixer, port_index);
        set_inport_hwsync(port);
        set_port_meta_data_cbk(port, on_meta_data_cbk, meta_data);
    }
    return 0;
}

int delete_mixer_input_port(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    ALOGI("++%s port ID: %d", __func__, port_index);
    free_input_port(audio_mixer->in_ports[port_index]);
    audio_mixer->in_ports[port_index] = NULL;
    return 0;
}

int send_mixer_inport_message(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index , enum PORT_MSG msg)
{
    struct input_port *port = audio_mixer->in_ports[port_index];
    return send_inport_message(port, msg);
}

void mixer_set_hwsync_input_port(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];
    ALOGI("++%s port ID: %d", __func__, port_index);
    if (port)
        set_inport_hwsync(port);
}

void set_mixer_hwsync_frame_size(struct aml_audio_mixer *audio_mixer,
        uint32_t frame_size)
{
    enum MIXER_INPUT_PORT port_index = 0;
    enum MIXER_OUTPUT_PORT out_port_index = MIXER_OUTPUT_PORT_PCM;
    struct input_port *in_port = NULL;
    struct output_port *out_port = audio_mixer->out_ports[out_port_index];
    int port_cnt = 0;
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port) {
            resize_input_port_buffer(in_port, MIXER_IN_BUFFER_SIZE);
        }
    }

    resize_output_port_buffer(out_port, MIXER_IN_BUFFER_SIZE);
    ALOGI("%s framesize %d", __func__, frame_size);
    audio_mixer->hwsync_frame_size = frame_size;
}

uint32_t get_mixer_hwsync_frame_size(struct aml_audio_mixer *audio_mixer)
{
    return audio_mixer->hwsync_frame_size;
}

uint32_t get_mixer_inport_consumed_frames(
        struct aml_audio_mixer *audio_mixer, enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }
    return get_inport_consumed_size(port) / port->frame_size;
}

int set_mixer_inport_volume(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, float vol)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("NULL pointer");
        return -EINVAL;
    }

    if (vol > 1.0 || vol < 0) {
        ALOGE("%s(), invalid vol %f", __func__, vol);
        return -EINVAL;
    }
    set_inport_volume(port, vol);
    return 0;
}

float get_mixer_inport_volume(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    if (!port) {
        ALOGE("NULL pointer");
        return 0;
    }
    return get_inport_volume(port);
}

int mixer_broadcast(struct aml_audio_mixer *audio_mixer)
{
    pthread_cond_signal(&audio_mixer->cond);
    return 0;
}

int mixer_write_inport(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, const void *buffer, int bytes)
{
    struct input_port *port = audio_mixer->in_ports[port_index];
    int written = 0;

    if (!port) {
        ALOGE("NULL pointer");
        return 0;
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
    ALOGV("%s(), signal line %d portIndex %d",
            __func__, __LINE__, port_index);
    mixer_broadcast(audio_mixer);
    return written;
}

int mixer_read_inport(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, void *buffer, int bytes)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    return port->read(port, buffer, bytes);
}

int mixer_set_inport_state(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, enum port_state state)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    return set_inport_state(port, state);
}

enum port_state mixer_get_inport_state(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];

    return get_inport_state(port);
}
//TODO: handle message queue
static void mixer_procs_msg_queue(struct aml_audio_mixer *audio_mixer __unused)
{
    ALOGV("++%s start", __func__);
    return;
}

static struct output_port *get_outport(struct aml_audio_mixer *audio_mixer,
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

static bool is_output_data_avail(struct aml_audio_mixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    struct output_port *outport = NULL;

    ALOGV("++%s start", __func__);
    /* init check */
    //if(aml_audio_mixer_check_status(audio_mixer))
    //    return false;

    outport = get_outport(audio_mixer, port_index);
    return !!get_outport_data_avail(outport);
    //return true;
}

int init_mixer_output_port(struct aml_audio_mixer *audio_mixer,
        struct pcm *pcm_handle,
        int buf_size
        //,struct audio_config *config,
        //audio_output_flags_t flags,
        //int (*on_notify_cbk)(void *data),
        //void *notify_data,
        //int (*on_input_avail_cbk)(void *data),
        ///void *input_avail_data
        )
{
    struct output_port *port = new_output_port(MIXER_OUTPUT_PORT_PCM,
            pcm_handle, buf_size);
    enum MIXER_OUTPUT_PORT port_index = port->port_index;

    //set_port_notify_cbk(port, on_notify_cbk, notify_data);
    //set_port_input_avail_cbk(port, on_input_avail_cbk, input_avail_data);
    audio_mixer->out_ports[port_index] = port;
    return 0;
}

int delete_mixer_output_port(struct aml_audio_mixer *audio_mixer,
        enum MIXER_OUTPUT_PORT port_index)
{
    free_output_port(audio_mixer->out_ports[port_index]);
    audio_mixer->out_ports[port_index] = NULL;
    return 0;
}

static int mixer_output_write(struct aml_audio_mixer *audio_mixer)
{
    enum MIXER_OUTPUT_PORT port_index = 0;
    struct output_port *out_port = audio_mixer->out_ports[port_index];

    while (is_output_data_avail(audio_mixer, port_index)) {
        // out_write_callbacks();
        ALOGV("++%s start", __func__);
        out_port->write(out_port, out_port->data_buf, out_port->bytes_avail);
        set_outport_data_avail(out_port, 0);
    };
    return 0;
}

static bool is_mixer_inports_ready(struct aml_audio_mixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    int port_cnt = 0, ready = 0;
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

    return (port_cnt == ready);
}

inline float get_fade_step_by_size(int fade_size, int frame_size)
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

static int mixer_inports_read(struct aml_audio_mixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    uint ret = 0;

    ALOGV("++%s(), line %d", __func__, __LINE__);
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        enum port_state state = ACTIVE;
        if (in_port && in_port->rbuf_ready(in_port) && !in_port->data_valid) {
            // only direct stream need to judge the states
            if (port_index == MIXER_INPUT_PORT_PCM_DIRECT) {
                state = get_inport_state(in_port);
                ALOGV("%s() portIndex %d, toReadBytes = %d",
                        __func__, port_index, in_port->data_len_bytes);

                if (state == FLUSHING) {
                    mixer_flush_inport(audio_mixer, port_index);
                    ALOGI("%s(), flushing->flushed", __func__);
                    set_inport_state(in_port, FLUSHED);
                    continue;
                } else if (state == PAUSED) {
                    ALOGI("%s(), status in paused, no need to read inport buf", __func__);
                    continue;
                } else if (state == STOPPED) {
                    ALOGI("starting port index %d from stopped", port_index);
                    //audio_fade_func(in_port->data, in_port->data_len_bytes, 1);
                    //ALOGI("%s(), tsync resume audio", __func__);
                    set_inport_state(in_port, ACTIVE);
                }
            }
            ret = mixer_read_inport(audio_mixer,
                    port_index, in_port->data, in_port->data_len_bytes);

            ALOGV("%s() ret %d, portIndex %d", __func__, ret, port_index);
            if (ret == in_port->data_len_bytes) {
                in_port->data_valid = 1;
                if (port_index == MIXER_INPUT_PORT_PCM_DIRECT) {
                    //state = mixer_get_inport_state(audio_mixer, port_index);
                    // we are in easing, when finished set to paused status
                    if (state == PAUSING) {
                        int bytes = in_port->data_len_bytes;

                        init_fade(&in_port->fout, bytes, 2, 2);
                        process_fade_out(in_port->data, in_port->data_len_bytes, &in_port->fout);
                        //if (in_port->fout.fade_size == 0) {
                            //mixer_set_inport_state(audio_mixer, port_index, PAUSED);
                            ALOGI("fade out finished pausing->pausing_1");
                            ALOGI("%s(), tsync pause audio", __func__);
                            aml_hwsync_set_tsync_pause();
                        //}
                        set_inport_state(in_port, PAUSING_1);
                    } else if (state == RESUMING) {
                        ALOGI("resuming port index %d", port_index);
                        audio_fade_func(in_port->data, in_port->data_len_bytes, 1);
                        ALOGI("%s(), tsync resume audio", __func__);
                        aml_hwsync_set_tsync_resume();
                        set_inport_state(in_port, ACTIVE);
                    }
                }
            }else {
                ALOGE("%s() read ret = %d, portIndex %d", __func__, ret, port_index);
            }
        }
    }

    return 0;
}

static void process_port_msg(struct input_port *port)
{
    struct port_message *msg = get_inport_message(port);
    if (msg) {
        ALOGI("%s(), msg %d", __func__, msg->msg_what);
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

static int mixer_inports_read1(struct aml_audio_mixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    bool ready = false;

    ALOGV("++%s(), line %d", __func__, __LINE__);
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        //enum port_state state = ACTIVE;
        if (in_port) {
            int ret = 0, fade_out = 0, fade_in = 0;

            process_port_msg(in_port);
            enum port_state state = get_inport_state(in_port);

            if (0)
                ALOGI("port %d, bufferlvl %d state %d",
                    port_index, inport_buffer_level(in_port), state);

            if (port_index == MIXER_INPUT_PORT_PCM_DIRECT) {
                //if in pausing states, don't retrieve data
                if (state == PAUSING) {
                    fade_out = 1;
                } else if (state == RESUMING) {
                    fade_in = 1;
                } else if (state == STOPPED || state == PAUSED || state == FLUSHED) {
                    ALOGV("%s(), stopped, paused or flushed", __func__);
                    in_port->data_valid = 1;
                    memset(in_port->data, 0, in_port->data_len_bytes);
                    //continue;
                } else if (state == FLUSHING) {
                    mixer_flush_inport(audio_mixer, port_index);
                    ALOGI("%s(), flushing->flushed", __func__);
                    set_inport_state(in_port, FLUSHED);
                    in_port->data_valid = 1;
                    memset(in_port->data, 0, in_port->data_len_bytes);
                    continue;
                }
            }
            // we are inserting frames
            if (in_port->data_valid) {
                ALOGV("%s() portIndex %d already valid", __func__, port_index);
                ready = true;
                continue;
            }
            if (in_port->rbuf_ready(in_port)) {
                ret = mixer_read_inport(audio_mixer,
                        port_index, in_port->data, in_port->data_len_bytes);

                ALOGV("%s() ret %d, portIndex %d", __func__, ret, port_index);
                if (ret == (int)in_port->data_len_bytes) {
                    if (fade_out) {
                        ALOGI("%s(), fade out finished pausing->pausing_1", __func__);
                        ALOGI("%s(), tsync pause audio", __func__);
                        aml_hwsync_set_tsync_pause();
                        audio_fade_func(in_port->data, ret, 0);
                        set_inport_state(in_port, PAUSED);
                    } else if (fade_in) {
                        ALOGI("%s(), resuming port index %d", __func__, port_index);
                        ALOGI("%s(), tsync resume audio", __func__);
                        aml_hwsync_set_tsync_resume();
                        audio_fade_func(in_port->data, ret, 1);
                        set_inport_state(in_port, ACTIVE);
                    }
                    in_port->data_valid = 1;
                    ready = true;

                    if (getprop_bool("media.audiohal.inport") &&
                            (in_port->port_index == MIXER_INPUT_PORT_PCM_DIRECT)) {
                            aml_audio_dump_audio_bitstreams("/data/audio/inportDirectFade.raw",
                                    in_port->data, in_port->data_len_bytes);
                    }
                }else {
                    ALOGE("%s() read ret = %d, portIndex %d", __func__, ret, port_index);
                }
            }
        }
    }

    // if one port ready, propagate to do mixing out
    if (ready)
        return 0;
    else
        return -EAGAIN;
}

int mixer_flush_inport(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *in_port = audio_mixer->in_ports[port_index];

    if (!in_port) {
        return -EINVAL;
    }

    return reset_input_port(in_port);
}

int check_mixer_state(struct aml_audio_mixer *audio_mixer)
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

int mixer_need_wait_forever(struct aml_audio_mixer *audio_mixer)
{
    return mixer_get_state(audio_mixer) != MIXER_INPORTS_READY;
}
static int mixer_prepare_inport_data(struct aml_audio_mixer *audio_mixer)
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
            }// else {
             //   mixer_inports_read(audio_mixer);
           // }
        }
    }

    ALOGV("--%s(), line %d", __func__, __LINE__);
    return 0;
}

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
static int retrieve_hwsync_header(struct aml_audio_mixer *audio_mixer,
        struct input_port *in_port, struct output_port *out_port)
{
    uint32_t frame_size = get_mixer_hwsync_frame_size(audio_mixer);
    uint32_t port_consumed_size = get_inport_consumed_size(in_port);
    uint32_t aligned_offset = 0;
    struct hw_avsync_header header;
    int ret = 0;

    if (frame_size == 0) {
        ALOGV("%s(), invalid frame size", __func__);
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
        ret = in_port->meta_data_cbk(in_port->meta_data_cbk_data,
                    aligned_offset, &header, &diff_ms);
        if (ret < 0) {
            if (ret != -EAGAIN)
                ALOGE("meta_data_cbk fail err = %d!!", ret);
            return ret;
        }
        if (diff_ms > 0) {
            in_port->bytes_to_insert = diff_ms * 48 * 4;
        }
#if 0
        if (0) {
            memcpy(out_port->data_buf, header.header, HW_AVSYNC_HEADER_SIZE_V1);
            set_outport_data_avail(out_port, HW_AVSYNC_HEADER_SIZE_V1);
            ALOGV("direct out port bytes after cbk %d", get_outport_data_avail(out_port));
            mixer_output_write(audio_mixer);
        }
#endif
    }

    return 0;
}

static int mixer_do_mixing1(struct aml_audio_mixer *audio_mixer)
{
    struct input_port *in_port_sys = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_SYSTEM];
    struct input_port *in_port_drct = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_DIRECT];
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    int16_t *data_sys, *data_drct, *data_mixed;
    int mixing = 0, sys_only = 0, direct_only = 0;
    int dirct_okay = 0, sys_okay = 0;
    float dirct_vol = 1.0, sys_vol = 1.0;
    int mixed_32 = 0;
    size_t i = 0, mixing_len_bytes = 0;

    if (!out_port) {
        ALOGE("%s(), out null !!!", __func__);
        return 0;
    }
    if (!in_port_sys && !in_port_drct) {
        ALOGE("%s(), sys or direct pcm must exist!!!", __func__);
        return 0;
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
        return -EINVAL;
    }

    data_mixed = (int16_t *)out_port->data_buf;

    if (mixing) {
        ALOGV("%s() mixing", __func__);
        dirct_vol = get_inport_volume(in_port_drct);
        data_sys = (int16_t *)in_port_sys->data;
        data_drct = (int16_t *)in_port_drct->data;
        mixing_len_bytes = in_port_drct->data_len_bytes;
        if (0) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, mixing_len_bytes);
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, mixing_len_bytes);
        }
        if (is_inport_hwsync(in_port_drct) && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        // insert data for direct hwsync case, only send system sound
        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            ALOGW("%s(), inserting data to sync %d bytes", __func__, mixing_len_bytes);
            memcpy(data_mixed, data_sys, mixing_len_bytes);
            in_port_drct->bytes_to_insert -= mixing_len_bytes;
            in_port_sys->data_valid = 0;
        } else {
            cpy_16bit_data_with_gain(data_drct, data_drct,
                    mixing_len_bytes, dirct_vol);

            for (i = 0; i < mixing_len_bytes/2; i++) {
                mixed_32 = *data_sys++ + *data_drct++;
                *data_mixed++ = CLIP16(mixed_32);
            }
            in_port_drct->data_valid = 0;
            in_port_sys->data_valid = 0;
        }
        set_outport_data_avail(out_port, mixing_len_bytes);
    }

    if (sys_only) {
        ALOGV("%s() sys_only", __func__);
        mixing_len_bytes = in_port_sys->data_len_bytes;
        data_sys = (int16_t *)in_port_sys->data;
        if (0)
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw", in_port_sys->data, mixing_len_bytes);
        memcpy(data_mixed, data_sys, in_port_sys->data_len_bytes);
        in_port_sys->data_valid = 0;
        set_outport_data_avail(out_port, mixing_len_bytes);
    }

    if (direct_only) {
        ALOGV("%s() direct_only", __func__);
        dirct_vol = get_inport_volume(in_port_drct);
        mixing_len_bytes = in_port_drct->data_len_bytes;
        data_drct = (int16_t *)in_port_drct->data;
        ALOGV("%s() direct_only, inport consumed %d", __func__, get_inport_consumed_size(in_port_drct));

        if (is_inport_hwsync(in_port_drct) && in_port_drct->bytes_to_insert < mixing_len_bytes) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        if (0) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, mixing_len_bytes);
        }
        // insert 0 data to delay audio
        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            ALOGD("%s() inserting data direct_only, length %zu", __func__, mixing_len_bytes);
            memset(data_mixed, 0, mixing_len_bytes);
            in_port_drct->bytes_to_insert -= mixing_len_bytes;
        } else {
            ALOGV("%s() direct_only, vol %f", __func__, dirct_vol);
            cpy_16bit_data_with_gain(data_mixed, data_drct,
                    in_port_drct->data_len_bytes, dirct_vol);
            in_port_drct->data_valid = 0;
        }
        set_outport_data_avail(out_port, mixing_len_bytes);
    }

    if (0) {
        aml_audio_dump_audio_bitstreams("/data/audio/data_mixed.raw",
                out_port->data_buf, mixing_len_bytes);
    }
    return 0;
}
#if 0
static int mixer_do_mixing(struct aml_audio_mixer *audio_mixer)
{
    struct input_port *in_port_sys = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_SYSTEM];
    struct input_port *in_port_drct = audio_mixer->in_ports[MIXER_INPUT_PORT_PCM_DIRECT];
    struct output_port *out_port = audio_mixer->out_ports[MIXER_OUTPUT_PORT_PCM];
    int16_t *data_sys, *data_drct, *data_mixed;
    int mixing = 0, sys_only = 0, direct_only = 0;
    int dirct_okay = 0, sys_okay = 0;
    float dirct_vol = 1.0, sys_vol = 1.0;
    int mixed_32 = 0;
    size_t i = 0, mixing_len_bytes = 0;
    int DBG = 0;
    if (!out_port) {
        ALOGE("%s(), out null !!!", __func__);
        return 0;
    }
    if (!in_port_sys && !in_port_drct) {
        ALOGE("%s(), sys or direct pcm must exist!!!", __func__);
        return 0;
    }

    if (in_port_sys && in_port_sys->data_valid) {
        sys_okay = 1;
    }
    if (in_port_drct) {
        if (in_port_drct->data_valid) {
            dirct_okay = 1;
        } else if (get_inport_state(in_port_drct) == ACTIVE) {
            ALOGE("%s(), direct pcm must exist when direct active!!!", __func__);
            return 0;
        } else if (get_inport_state(in_port_drct) == PAUSING_1) {
            ALOGW("%s(), direct pcm pausing_l->paused!!!", __func__);
            DBG = 1;
            set_inport_state(in_port_drct, PAUSED);
            //return 0;
        }
    }

    if (DBG) {
        ALOGE("%s(), sys_okay = %d, dirct_okay = %d!",
                __func__, sys_okay, dirct_okay);
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
        return 0;
    }

    data_mixed = (int16_t *)out_port->data_buf;

    if (mixing) {
        ALOGV("%s() mixing", __func__);
        dirct_vol = get_inport_volume(in_port_drct);
        data_sys = (int16_t *)in_port_sys->data;
        data_drct = (int16_t *)in_port_drct->data;
        mixing_len_bytes = in_port_drct->data_len_bytes;
        if (0) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, mixing_len_bytes);
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw",
                    in_port_sys->data, mixing_len_bytes);
        }
        if (is_inport_hwsync(in_port_drct)) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            memcpy(data_mixed, data_sys, mixing_len_bytes);
            in_port_drct->bytes_to_insert -= mixing_len_bytes;
        } else {
            if (dirct_vol != 1.0) {
                cpy_16bit_data_with_gain(data_drct, data_drct,
                        mixing_len_bytes, dirct_vol);
            }
            for (i = 0; i < mixing_len_bytes/2; i++) {
                mixed_32 = *data_sys++ + *data_drct++;
                *data_mixed++ = CLIP16(mixed_32);
            }
            in_port_drct->data_valid = 0;
        }
        in_port_sys->data_valid = 0;
        set_outport_data_avail(out_port, mixing_len_bytes);
    }

    if (sys_only) {
        ALOGV("%s() sys_only", __func__);
        mixing_len_bytes = in_port_sys->data_len_bytes;
        data_sys = (int16_t *)in_port_sys->data;
        if (0)
            aml_audio_dump_audio_bitstreams("/data/audio/audiosyst.raw", in_port_sys->data, mixing_len_bytes);
        memcpy(data_mixed, data_sys, in_port_sys->data_len_bytes);
        in_port_sys->data_valid = 0;
        set_outport_data_avail(out_port, mixing_len_bytes);
    }

    if (direct_only) {
        ALOGV("%s() direct_only", __func__);
        dirct_vol = get_inport_volume(in_port_drct);
        mixing_len_bytes = in_port_drct->data_len_bytes;
        data_drct = (int16_t *)in_port_drct->data;
        ALOGV("%s() direct_only, inport consumed %d", __func__, get_inport_consumed_size(in_port_drct));

        if (is_inport_hwsync(in_port_drct)) {
            retrieve_hwsync_header(audio_mixer, in_port_drct, out_port);
        }

        if (0) {
            aml_audio_dump_audio_bitstreams("/data/audio/audiodrct.raw",
                    in_port_drct->data, mixing_len_bytes);
        }
        if (in_port_drct->bytes_to_insert >= mixing_len_bytes) {
            ALOGV("%s() direct_only, vol %f", __func__, dirct_vol);
            if (dirct_vol == 1.0)
                memcpy(data_mixed, data_drct, mixing_len_bytes);
            else
                cpy_16bit_data_with_gain(data_mixed, data_drct,
                        mixing_len_bytes, dirct_vol);
            //memset(data_mixed, 0, mixing_len_bytes);
            in_port_drct->bytes_to_insert -= mixing_len_bytes;
        } else {
            ALOGV("%s() direct_only, vol %f", __func__, dirct_vol);
            if (dirct_vol == 1.0)
                memcpy(data_mixed, data_drct, in_port_drct->data_len_bytes);
            else
                cpy_16bit_data_with_gain(data_mixed, data_drct,
                        in_port_drct->data_len_bytes, dirct_vol);
        }
        in_port_drct->data_valid = 0;
        set_outport_data_avail(out_port, mixing_len_bytes);
    }

    if (0) {
        aml_audio_dump_audio_bitstreams("/data/audio/data_mixed.raw",
                out_port->data_buf, mixing_len_bytes);
    }
    return 0;
}
#endif
int notify_mixer_input_avail(struct aml_audio_mixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port && in_port->on_notify_cbk)
            in_port->on_input_avail_cbk(in_port->input_avail_cbk_data);
    }

    return 0;
}

int notify_mixer_exit(struct aml_audio_mixer *audio_mixer)
{
    enum MIXER_INPUT_PORT port_index = 0;
    for (port_index = 0; port_index < MIXER_INPUT_PORT_NUM; port_index++) {
        struct input_port *in_port = audio_mixer->in_ports[port_index];
        if (in_port && in_port->on_notify_cbk)
            in_port->on_notify_cbk(in_port->notify_cbk_data);
    }

    return 0;
}


uint64_t last_write_time_us;
#define THROTTLE_TIME_US 5000
static void *mixer_key_threadloop(void *data)
{
    struct aml_audio_mixer *audio_mixer = data;
    enum MIXER_OUTPUT_PORT port_index = MIXER_OUTPUT_PORT_PCM;
    enum MIXER_INPUT_PORT in_index = MIXER_INPUT_PORT_PCM_SYSTEM;
    int ret = 0;
    // calc write block size
    //struct pcm_config *cfg = &main_desc->cfg;
    //int period_byte = audio_mixer->out_buffer_size;
    //int ready = 0;
    // sleep half period
    //int msleep_time = cfg->period_size * 400 / cfg->rate;

    //if (!audio_mixer->mixer_out_write)
    //    return NULL;

    audio_mixer->exit_thread = 0;
    prctl(PR_SET_NAME, "aml_audio_mixer");
    ALOGI("++%s start", __func__);
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(2, &cpuSet);
    CPU_SET(3, &cpuSet);
    int sastat = sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
    if (sastat) {
        ALOGW("%s(), failed to set cpu affinity", __FUNCTION__);
    }
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
        const uint64_t delta_us =
                (get_systime_ns() - audio_mixer->last_process_finished_ns) / 1000;

        if (delta_us <= THROTTLE_TIME_US) {
	    struct timespec ts;
	    uint32_t throttle_us = THROTTLE_TIME_US - delta_us;
            ALOGV("%s throttle time_us %d", __func__, throttle_us);
            //usleep(delta_us);
	    ts_wait_time_us(&ts, throttle_us);
	    pthread_mutex_lock(&audio_mixer->lock);
	    pthread_cond_timedwait(&audio_mixer->cond, &audio_mixer->lock, &ts);
	    pthread_mutex_unlock(&audio_mixer->lock);
            if (audio_mixer->exit_thread)
                break;
        }
        ret = mixer_inports_read1(audio_mixer);
        if (ret < 0) {
            //usleep(5000);
            ALOGV("%s %d data not enough, next turn", __func__, __LINE__);
            notify_mixer_input_avail(audio_mixer);
            audio_mixer->last_process_finished_ns = get_systime_ns();
            continue;
            //notify_mixer_input_avail(audio_mixer);
            //continue;
        }
        notify_mixer_input_avail(audio_mixer);
        ALOGV("%s %d do mixing", __func__, __LINE__);
        mixer_do_mixing1(audio_mixer);
        struct timespec tval;
        uint64_t now_in_us = 0, tpast_us = 0;
        clock_gettime(CLOCK_MONOTONIC, &tval);
        now_in_us = tval.tv_sec * 1000000 + tval.tv_nsec / 1000;
        tpast_us = now_in_us - last_write_time_us;
        //if (tpast_us > (audio_mixer->out_ports[port_index]->bytes_avail*2 * 1000 / 48/4))
        //    ALOGW("%s(), actual write time %lld, estimated %d", __func__,
        //        tpast_us, (audio_mixer->out_ports[port_index]->bytes_avail * 1000 / 48 /4));
        last_write_time_us = now_in_us;
        mixer_output_write(audio_mixer);
        audio_mixer->last_process_finished_ns = get_systime_ns();
    }

    ALOGI("--%s", __func__);
    return NULL;
}

uint32_t mixer_get_inport_latency_frames(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index)
{
    struct input_port *port = audio_mixer->in_ports[port_index];
    int written = 0;

    if (!port) {
        ALOGE("NULL pointer");
        return 0;
    }

    return port->get_latency_frames(port);
}

uint32_t mixer_get_outport_latency_frames(struct aml_audio_mixer *audio_mixer)
{
    struct output_port *port = get_outport(audio_mixer, MIXER_OUTPUT_PORT_PCM);
    return outport_get_latency_frames(port);
}

int mixer_thread_run(struct aml_audio_mixer *audio_mixer)
{
    int ret = 0;

    ALOGI("%s(), line %d", __func__, __LINE__);
    ret = pthread_create(&audio_mixer->out_mixer_tid, NULL, mixer_key_threadloop, audio_mixer);
    if (ret < 0)
        ALOGE("%s() thread run failed.", __func__);
    return ret;
}

int mixer_thread_exit(struct aml_audio_mixer *audio_mixer)
{
    // block exit
    audio_mixer->exit_thread = 1;
    pthread_cond_broadcast(&audio_mixer->cond);
    pthread_join(audio_mixer->out_mixer_tid, NULL);
    ALOGD("%s(), line %d", __func__, __LINE__);

    notify_mixer_exit(audio_mixer);
    return 0;
}
struct aml_audio_mixer *new_aml_audio_mixer(struct pcm *pcm_handle)
{
    struct aml_audio_mixer *audio_mixer = NULL;
    int ret = 0;
    ALOGD("%s(), line %d", __func__, __LINE__);

    if (!pcm_handle) {
        ALOGE("%s(), NULL pcm handle", __func__);
        return NULL;
    }
    audio_mixer = calloc(1, sizeof(*audio_mixer));
    if (!audio_mixer) {
        ALOGE("%s(), no memory", __func__);
        return NULL;
    }

    mixer_set_state(audio_mixer, MIXER_IDLE);
    ret = init_mixer_output_port(audio_mixer, pcm_handle, MIXER_OUT_BUFFER_SIZE);

    pthread_mutex_init(&audio_mixer->lock, NULL);
    pthread_cond_init(&audio_mixer->cond, NULL);
    return audio_mixer;
}

void free_aml_audio_mixer(struct aml_audio_mixer *audio_mixer)
{
    if (audio_mixer) {
        pthread_mutex_destroy(&audio_mixer->lock);
        pthread_cond_destroy(&audio_mixer->cond);
        free(audio_mixer);
    }
}
