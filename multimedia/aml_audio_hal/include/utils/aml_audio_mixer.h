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

#ifndef _AML_AUDIO_MIXER_H_
#define _AML_AUDIO_MIXER_H_

#include <tinyalsa/asoundlib.h>
#include <aml_ringbuffer.h>

__BEGIN_DECLS

/**
 * Audio mixer:
 * only support mixing two steams, one main stream and one aux stream.
 * And the output streams configs must be same as input streams.
 * TODO: add formats adaptions.
 */
struct aml_audio_mixer;

/**
 * callback function that user should register to the mixer,
 * mixer will pop mixed data out to buffer when ready.
 */
typedef int (*mixer_write_callback)(void *buffer, void *priv, size_t size);

/**
 * constructor with mixer output pcm configs
 * return NULL if no enough memory.
 */
struct aml_audio_mixer *new_aml_audio_mixer(struct pcm_config out_config);

/**
 * distructor to free the mixer
 */
void aml_delete_audio_mixer(struct aml_audio_mixer *audio_mixer);

/**
 * register the main input pcm config and ringbuffer to mixer.
 * if ringbuffer == NULL, then alloc the input ringbuffer according to cfg internally.
 * return < 0 if failed.
 */
int aml_register_mixer_main_in_buffer(struct aml_audio_mixer *audio_mixer,
            struct ring_buffer *ringbuffer, struct pcm_config cfg);


/**
 * register the aux input pcm config and ringbuffer to mixer.
 * if ringbuffer == NULL, then alloc the input ringbuffer according to cfg internally.
 * return < 0 if failed.
 */
int aml_register_mixer_aux_in_buffer(struct aml_audio_mixer *audio_mixer,
            struct ring_buffer *ringbuffer, struct pcm_config cfg);

/**
 * release the main in ringbuffer
 */
int aml_release_main_in_buffer(struct aml_audio_mixer *audio_mixer);

/**
 * release the aux in ringbuffer
 */
int aml_release_aux_in_buffer(struct aml_audio_mixer *audio_mixer);

/**
 * write size of data to mixer main in ringbuffer from buffer
 */
int aml_write_mixer_main_in_buf(struct aml_audio_mixer *audio_mixer, void *buffer, size_t size);

/**
 * write size of data to mixer aux in ringbuffer from buffer
 */
int aml_write_mixer_aux_in_buf(struct aml_audio_mixer *audio_mixer, void *buffer, size_t size);

/**
 * register user callback to audio mixer which will be called when data ready
 * priv_data is used in callback function
 */
int aml_register_audio_mixer_callback(struct aml_audio_mixer *audio_mixer, mixer_write_callback cbk, void *priv_data);

/**
 * release user callback to audio mixer
 */
int aml_release_audio_mixer_callback(struct aml_audio_mixer *audio_mixer);

/**
 * start the mixing thread
 * return < 0 if failed.
 */
int aml_start_audio_mixer(struct aml_audio_mixer *audio_mixer);

/**
 * stop the mixing thread
 * return < 0 if failed.
 */
int aml_stop_audio_mixer(struct aml_audio_mixer *audio_mixer);

/**
 * dump the audiomixer status
 */
void aml_dump_audio_mixer(struct aml_audio_mixer *audio_mixer);

__END_DECLS

#endif
