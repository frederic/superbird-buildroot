/*
 * Copyright (C) 2018 Amlogic Corporation.
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
#ifndef AML_AUDIO_EASE_H
#define AML_AUDIO_EASE_H

#include <system/audio.h>
#include <pthread.h>

typedef struct aml_data_format {
    audio_format_t format;
    int sr ;   /**  samplerate*/
    int ch ;   /**  channels*/
    int endian;   /*refer to sample_endian*/
} aml_data_format_t;

typedef enum ease_type{
    EaseLinear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    EaseMax
} ease_type_t;

typedef enum ease_status {
    Invalid,
    EaseIn,
    EaseOut,
} ease_status_t;

typedef struct ease_setting{
    float start_volume;     /*the vol range is 0.0~1.0*/
    float target_volume;    /*the vol range is 0.0~1.0*/
    int duration;           /*the value is ms*/
} ease_setting_t;

typedef struct aml_audio_ease {
    int ease_type;
    int ease_time;
    float current_volume;
    float start_volume;
    float target_volume;
    unsigned int ease_frames_elapsed;
    unsigned int ease_frames;
    aml_data_format_t data_format;
    ease_status_t ease_status;
    pthread_mutex_t ease_lock;
} aml_audio_ease_t;

int aml_audio_ease_init(aml_audio_ease_t ** ppease_handle);

int aml_audio_ease_close(aml_audio_ease_t * ease_handle);

int aml_audio_ease_config(aml_audio_ease_t * ease_handle, ease_setting_t *setting);

int aml_audio_ease_process(aml_audio_ease_t * ease_handle, void * in_data, size_t size);
#endif
