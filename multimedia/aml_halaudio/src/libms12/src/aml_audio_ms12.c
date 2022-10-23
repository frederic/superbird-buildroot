/*
 * hardware/amlogic/audio/TvAudio/aml_audio_ms12.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */


//#ifdef DOLBY_MS12_ENABLE
#define LOG_TAG "libms12"
// #define LOG_NDEBUG 0

#include "log.h"
#include <dolby_ms12.h>
#include <dolby_ms12_config_params.h>
#include <dolby_ms12_status.h>

#include "aml_audio_ms12.h"


#define DOLBY_SAMPLE_SIZE 4//2ch x 2bytes(16bits) = 4 bytes


int get_dolby_ms12_output_details(struct dolby_ms12_desc *ms12_desc)
{
    ms12_desc->dolby_ms12_init_argv = dolby_ms12_config_params_get_config_params(&ms12_desc->dolby_ms12_init_argc);
    ms12_desc->output_samplerate = dolby_ms12_config_params_get_dolby_config_output_samplerate();
    ms12_desc->output_channelmask = dolby_ms12_config_params_get_dolby_config_output_channelmask();
    ALOGD("%s() dolby_ms12_init_argv %p argc %d dolby ms12 output format %#x samplerate %d channelmask %#x\n",
          __func__, ms12_desc->dolby_ms12_init_argv, ms12_desc->dolby_ms12_init_argc, ms12_desc->output_format,
          ms12_desc->output_samplerate, ms12_desc->output_channelmask);
    return 0;
}

int get_dolby_ms12_init(struct dolby_ms12_desc *ms12_desc)
{
    int ret = 0;

    ALOGD("+%s()\n", __FUNCTION__);
    ret = get_libdolbyms12_handle();
    if (ret) {
        ALOGE("%s, fail to get ms12 handle\n", __FUNCTION__);
        return ret;
    }

    if (ms12_desc->dolby_ms12_init_argv) {
        ms12_desc->dolby_ms12_ptr = dolby_ms12_init(ms12_desc->dolby_ms12_init_argc, ms12_desc->dolby_ms12_init_argv);
        ms12_desc->dolby_ms12_out_max_size = get_dolby_ms12_output_max_size();
        ALOGV("%s() dolby ms12 init return %p dolby_ms12_out_max_size %d\n",
              __FUNCTION__, ms12_desc->dolby_ms12_ptr, ms12_desc->dolby_ms12_out_max_size);
#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK
        if (ms12_desc->dolby_ms12_ptr == NULL) {
            ALOGD("%s() dolby ms12 init fail!\n", __FUNCTION__);
            ms12_desc->dolby_ms12_enable = false;
        }
#else
        if (ms12_desc->dolby_ms12_out_max_size > 0) {
            ms12_desc->dolby_ms12_out_data = (char *)malloc(ms12_desc->dolby_ms12_out_max_size);
        }
        if ((ms12_desc->dolby_ms12_ptr == NULL) || (ms12_desc->dolby_ms12_out_data == NULL)) {
            ALOGD("%s() dolby ms12 init fail!\n", __FUNCTION__);
            ms12_desc->dolby_ms12_enable = false;
        }
#endif
        else {
            ms12_desc->dolby_ms12_enable = true;
            ALOGD("%s() dolby ms12 output format %#x\n", __FUNCTION__, ms12_desc->output_format);
            set_offload_playback_dolby_ms12_output_format(ms12_desc->output_format);
            ALOGD("%s() init DolbyMS12 success\n", __FUNCTION__);
        }
    }
    ALOGD("-%s() dolby_ms12_enable %d\n", __FUNCTION__, ms12_desc->dolby_ms12_enable);
    return 0;
}

int aml_ms12_config(struct dolby_ms12_desc *ms12_desc
                    , audio_format_t config_format
                    , audio_channel_mask_t config_channel_mask
                    , int config_sample_rate
                    , audio_format_t output_format)
{
    ALOGI("+%s() %d\n", __FUNCTION__, __LINE__);
    ms12_desc->input_config_format = config_format;
    ms12_desc->config_channel_mask = config_channel_mask;
    ms12_desc->config_sample_rate = config_sample_rate;
    //ms12_desc->output_format = get_dolby_ms12_suitable_output_format();
    ms12_desc->output_format = output_format;
    ALOGV("%s() config input format %#x channle mask %#x samplerate %d output format %#x\n",
          __FUNCTION__, config_format, config_channel_mask, config_sample_rate, output_format);
    dolby_ms12_config_params_reset_config_params();
    if (get_audio_system_format() == AUDIO_FORMAT_PCM_16_BIT) {
        dolby_ms12_config_params_set_system_flag(true);
    }
    if ((get_audio_associate_format() == AUDIO_FORMAT_AC3) || (get_audio_associate_format() == AUDIO_FORMAT_E_AC3)) {
        dolby_ms12_config_params_set_associate_flag(true);
    }
    dolby_ms12_config_params_set_audio_stream_out_params(
        2 //AUDIO_OUTPUT_FLAG_PRIMARY
        , ms12_desc->input_config_format
        , ms12_desc->config_channel_mask
        , ms12_desc->config_sample_rate
        , ms12_desc->output_format);
    get_dolby_ms12_output_details(ms12_desc);

    get_dolby_ms12_init(ms12_desc);
    ALOGI("-%s() %d\n", __FUNCTION__, __LINE__);
    return 0;
}

int aml_ms12_cleanup(struct dolby_ms12_desc *ms12_desc)
{
    dolby_ms12_status_self_cleanup();
    dolby_ms12_config_params_self_cleanup();
    dolby_ms12_release(ms12_desc->dolby_ms12_ptr);
    dolby_ms12_self_cleanup();
#ifdef REPLACE_OUTPUT_BUFFER_WITH_CALLBACK

#else
    if (ms12_desc->dolby_ms12_out_data) {
        free(ms12_desc->dolby_ms12_out_data);
        ms12_desc->dolby_ms12_out_data = NULL;
    }
#endif
    ms12_desc->dolby_ms12_ptr = NULL;
    ALOGI("%s", __func__);
    return 0;
}

int aml_ms12_update_runtime_params(struct dolby_ms12_desc *ms12_desc)
{
    ALOGI("+%s()\n", __FUNCTION__);
    int ret = -1;
    if (ms12_desc->dolby_ms12_init_argv) {
        dolby_ms12_config_params_reset_config_params();
        if (get_audio_system_format() == AUDIO_FORMAT_PCM_16_BIT) {
            dolby_ms12_config_params_set_system_flag(true);
        }
        if ((get_audio_associate_format() == AUDIO_FORMAT_AC3) || (get_audio_associate_format() == AUDIO_FORMAT_E_AC3)) {
            dolby_ms12_config_params_set_associate_flag(true);
        }
        ms12_desc->dolby_ms12_init_argv = dolby_ms12_config_params_get_runtime_config_params(&ms12_desc->dolby_ms12_init_argc);
        if (ms12_desc->dolby_ms12_ptr) {
            ret = dolby_ms12_update_runtime_params(ms12_desc->dolby_ms12_ptr, ms12_desc->dolby_ms12_init_argc, ms12_desc->dolby_ms12_init_argv);
        }
    }
    ALOGI("-%s() ret %d\n", __FUNCTION__, ret);
    return ret;
}


//#endif
