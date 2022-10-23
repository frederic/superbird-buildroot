/*
 * hardware/amlogic/audio/TvAudio/aml_audio_ms12.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */
#ifndef __AML_AUDIO_MS12_H__
#define __AML_AUDIO_MS12_H__

//#ifdef DOLBY_MS12_ENABLE

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dolby_ms12.h>
#include <dolby_ms12_config_params.h>
#include <dolby_ms12_status.h>
#include "audio_core.h"



#define DOLBY_SAMPLE_SIZE 4//2ch x 2bytes(16bits) = 4 bytes


struct dolby_ms12_desc {
    bool dolby_ms12_enable;
    bool dolby_ms12_init_flags;
    audio_format_t input_config_format;
    audio_channel_mask_t config_channel_mask;
    int config_sample_rate;
    audio_format_t output_format;
    int output_samplerate;
    audio_channel_mask_t output_channelmask;
    int ms12_out_bytes;
    //audio_policy_forced_cfg_t force_use;
    int dolby_ms12_init_argc;
    char **dolby_ms12_init_argv;
    void *dolby_ms12_ptr;
#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK

#else
    char *dolby_ms12_out_data;
#endif
    int dolby_ms12_out_max_size;
    /*
    there are some risk when aux write thread and direct thread
    access the ms12 module at the same time.
    1) aux thread is writing. direct thread is on standby and clear up the ms12 module.
    2) aux thread is writing. direct thread is preparing the ms12 module.
    */
    pthread_mutex_t lock;
    /*
    for higher effiency we dot use the the lock for main write
    function,as ms clear up may called by binder  thread
    we need protect the risk situation
    */
    pthread_mutex_t main_lock;
    pthread_t dolby_ms12_threadID;
    bool dolby_ms12_thread_exit;
    bool is_continuous_paused;
    int device;//alsa_device_t
    struct timespec timestamp;
    uint64_t last_frames_postion;
    /*
    latency frame is maintained by the whole device output.
    whatever what bistream is outputed we need use this latency frames.
    */
    int latency_frame;
};

/*
 *@brief this function is get the ms12 suitalbe output format
 *       1.input format
 *       2.EDID pcm/dd/dd+
 *       3.system settting
 * TODO, get the suitable format
 */
audio_format_t get_dolby_ms12_suitable_output_format(void);

/*
 *@brief get the ms12 output details, samplerate/formate/channelnum
 */
int get_dolby_ms12_output_details(struct dolby_ms12_desc *ms12_desc);

/*
 *@brief init the dolby ms12
 */
int get_dolby_ms12_init(struct dolby_ms12_desc *ms12_desc);

/*
 *@brief get the dolby ms12 config parameters
 * ms12_desc: ms12 handle
 * ms12_config_format: AUDIO_FORMAT_PCM_16_BIT/AUDIO_FORMAT_PCM_32_BIT/AUDIO_FORMAT_AC3/AUDIO_FORMAT_E_AC3
 * config_channel_mask: AUDIO_CHANNEL_OUT_STEREO/AUDIO_CHANNEL_OUT_5POINT1/AUDIO_CHANNEL_OUT_7POINT1
 * config_sample_rate: sample rate.
 */
int aml_ms12_config(struct dolby_ms12_desc *ms12_desc
                    , audio_format_t config_format
                    , audio_channel_mask_t config_channel_mask
                    , int config_sample_rate
                    , audio_format_t output_format);
/*
 *@brief cleanup the dolby ms12
 */
int aml_ms12_cleanup(struct dolby_ms12_desc *ms12_desc);

int aml_ms12_update_runtime_params(struct dolby_ms12_desc *ms12_desc);
//#endif


#endif //end of __AML_AUDIO_MS12_H__
