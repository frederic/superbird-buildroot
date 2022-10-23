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


#ifndef _AML_AUDIO_MIXER_H_
#define _AML_AUDIO_MIXER_H_

#include <tinyalsa/asoundlib.h>
#include "aml_ringbuffer.h"
#include "audio_port.h"

__BEGIN_DECLS

/**
 * Audio mixer:
 * mixing two pcm streams with same configs(16-bits, 2-channels).
 * only support mixing two steams, one main stream and one aux stream.
 * And the output streams configs must be same as input streams.
 * TODO: add formats adaptions.
 */
struct aml_audio_mixer;

/**
 * constructor with mixer output pcm configs
 * return NULL if no enough memory.
 */
//struct aml_audio_mixer *new_aml_audio_mixer(struct pcm_config out_config);
struct aml_audio_mixer *new_aml_audio_mixer(struct pcm *pcm_handle);

/**
 * distructor to free the mixer
 */
void free_aml_audio_mixer(struct aml_audio_mixer *audio_mixer);
void mixer_set_hwsync_input_port(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index);
void set_mixer_hwsync_frame_size(struct aml_audio_mixer *audio_mixer,
        uint32_t frame_size);
uint32_t get_mixer_hwsync_frame_size(struct aml_audio_mixer *audio_mixer);
uint32_t get_mixer_inport_consumed_frames(
        struct aml_audio_mixer *audio_mixer, enum MIXER_INPUT_PORT port_index);
int set_mixer_inport_volume(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, float vol);

int init_mixer_input_port(struct aml_audio_mixer *audio_mixer,
        struct audio_config *config,
        audio_output_flags_t flags,
        int (*on_notify_cbk)(void *data),
        void *notify_data,
        int (*on_input_avail_cbk)(void *data),
        void *input_avail_data,
        //int (*on_meta_data_cbk)(void *data,
        //        uint32_t offset,
        //        struct hw_avsync_header *header,
        //        int *delay_ms),
        meta_data_cbk_t on_meta_data_cbk,
        void *meta_data);

int delete_mixer_input_port(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index);
int send_mixer_inport_message(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index , enum PORT_MSG msg);

int mixer_write_inport(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, const void *buffer, int bytes);

int mixer_read_inport(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, void *buffer, int bytes);
int mixer_set_inport_state(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index, enum port_state state);

int mixer_flush_inport(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index);

int mixer_thread_run(struct aml_audio_mixer *audio_mixer);
int mixer_thread_exit(struct aml_audio_mixer *audio_mixer);
uint32_t mixer_get_inport_latency_frames(struct aml_audio_mixer *audio_mixer,
        enum MIXER_INPUT_PORT port_index);
uint32_t mixer_get_outport_latency_frames(struct aml_audio_mixer *audio_mixer);

__END_DECLS

#endif
