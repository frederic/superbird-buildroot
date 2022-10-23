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

#ifndef _AUDIO_PORT_H_
#define _AUDIO_PORT_H_

#include <system/audio.h>
#include <tinyalsa/asoundlib.h>
#include <cutils/list.h>

#include "hw_avsync.h"
#include "sub_mixing_factory.h"

enum port_state {
    IDLE,
    ACTIVE,
    STOPPED,
    FLUSHING,
    FLUSHED,
    RESUMING,
    PAUSING,
    PAUSING_1, //pausing and easing
    PAUSED,
};

enum PORT_MSG {
    MSG_PAUSE,
    MSG_FLUSH,
    MSG_RESUME,
    MSG_CNT
};
const char *port_msg_to_str(enum PORT_MSG msg);

enum MIXER_INPUT_PORT {
    MIXER_INPUT_PORT_INVAL          = -1,
    MIXER_INPUT_PORT_PCM_SYSTEM     = 0,
    MIXER_INPUT_PORT_PCM_DIRECT     = 1,
    //MIXER_INPUT_PORT_BITSTREAM_RAW  = 2,
    MIXER_INPUT_PORT_NUM
};

struct fade_out {
    float vol;
    float target_vol;
    int fade_size;
    int sample_size;
    int channel_cnt;
    //int frame_size;
    float stride;
};

struct port_message {
    enum PORT_MSG msg_what;
    struct listnode list;
};

typedef int (*meta_data_cbk_t)(void *cookie,
            uint64_t offset,
            struct hw_avsync_header *header,
            int *diff_ms);

struct input_port {
    enum MIXER_INPUT_PORT port_index;
    struct audioCfg cfg;
    struct ring_buffer *r_buf;
    size_t rbuf_frames;
    size_t rbuf_size;
    char *data;
    size_t data_buf_frame_cnt;
    size_t data_len_bytes;
    int data_valid;
    size_t bytes_to_insert;
    size_t bytes_to_skip;
    bool is_hwsync;
    size_t consumed_bytes;
    audio_format_t format;
    size_t frame_size;
    enum port_state port_status;
    ssize_t (*write)(struct input_port *port, const void *buffer, int bytes);
    ssize_t (*read)(struct input_port *port, void *buffer, int bytes);
    uint32_t (*get_latency_frames)(struct input_port *port);
    bool (*rbuf_ready)(struct input_port *port);
    void *notify_cbk_data;
    int (*on_notify_cbk)(void *data);
    void *input_avail_cbk_data;
    int (*on_input_avail_cbk)(void *data);
    void *meta_data_cbk_data;
    //int (*meta_data_cbk)(void *cookie,
    //        uint32_t offset,
    //        struct hw_avsync_header *header,
    //        int *diff_ms);
    meta_data_cbk_t meta_data_cbk;
    float volume;
    struct fade_out fout;
    struct listnode msg_list;
    pthread_mutex_t msg_lock;
    //nsecs_t last_write_nsec;
    struct timespec timestamp;
    /* get from out stream when init */
    uint64_t initial_frames;
    /* consumed by read after init */
    uint64_t mix_consumed_frames;
    uint64_t presentation_frames;
    int padding_frames;
    bool pts_valid;
};

enum MIXER_OUTPUT_PORT {
    MIXER_OUTPUT_PORT_INVAL         = -1,
    MIXER_OUTPUT_PORT_PCM           = 0,
    //MIXER_OUTPUT_PORT_BITSTREAM_RAW = 1,
    MIXER_OUTPUT_PORT_NUM
};

struct output_port {
    enum MIXER_OUTPUT_PORT port_index;
    struct audioCfg cfg;
    struct ring_buffer *r_buf;
    // data buf to hold tmp out data
    char *data_buf;
    size_t buf_frames;
    size_t frames_avail;
    size_t bytes_avail;
    size_t data_buf_frame_cnt;
    size_t data_buf_len;
    struct pcm *pcm_handle;
    //audio_format_t format;
    //size_t frame_size;
    enum port_state port_status;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    ssize_t (*write)(struct output_port *port, void *buffer, int bytes);
    int (*start)(struct output_port *port);
    int (*standby)(struct output_port *port);
    struct timespec tval_last;
    int sound_track_mode;
};
bool is_inport_valid(enum MIXER_INPUT_PORT index);
bool is_outport_valid(enum MIXER_OUTPUT_PORT index);

enum MIXER_INPUT_PORT get_input_port_index(struct audio_config *config,
        audio_output_flags_t flags);

struct input_port *new_input_port(
        size_t buf_size,
        struct audio_config *config,
        audio_output_flags_t flags,
        float volume,
        bool direct_on);
int set_inport_padding_size(struct input_port *port, size_t bytes);
int reset_input_port(struct input_port *port);
int resize_input_port_buffer(struct input_port *port, uint buf_size);
int free_input_port(struct input_port *port);
int set_port_notify_cbk(struct input_port *port,
        int (*on_notify_cbk)(void *data), void *data);
int set_port_input_avail_cbk(struct input_port *port,
        int (*on_input_avail_cbk)(void *data), void *data);
int set_port_meta_data_cbk(struct input_port *port,
        meta_data_cbk_t meta_data_cbk,
        void *data);
int send_inport_message(struct input_port *port, enum PORT_MSG msg);
struct port_message *get_inport_message(struct input_port *port);
int remove_inport_message(struct input_port *port, struct port_message *p_msg);
int remove_all_inport_messages(struct input_port *port);

int set_inport_state(struct input_port *port, enum port_state status);
enum port_state get_inport_state(struct input_port *port);
void set_inport_hwsync(struct input_port *port);
bool is_inport_hwsync(struct input_port *port);
void set_inport_volume(struct input_port *port, float vol);
float get_inport_volume(struct input_port *port);
size_t get_inport_consumed_size(struct input_port *port);
int inport_buffer_level(struct input_port *port);

struct output_port *new_output_port(
        enum MIXER_OUTPUT_PORT port_index,
        struct audioCfg cfg,
        size_t buf_frames);

int free_output_port(struct output_port *port);
int resize_output_port_buffer(struct output_port *port, size_t buf_frames);
int outport_get_latency_frames(struct output_port *port);
int set_inport_pts_valid(struct input_port *in_port, bool valid);
bool is_inport_pts_valid(struct input_port *in_port);
int outport_stop_pcm(struct output_port *port);
#endif /* _AUDIO_PORT_H_ */
