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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <tinyalsa/asoundlib.h>

#define PERIOD_SIZE                     1024
#define PLAYBACK_PERIOD_COUNT           4
#define HARDWARE_CHANNEL_STEREO         2
#define HARDWARE_CHANNEL_7_1_MULTI      8
#define __unused

/*
 *@brief get the hardware config parameters when the output format is DTS-HD/TRUE-HD
 */
static void get_dts_hd_hardware_config_parameters(
    struct pcm_config *hardware_config
    , unsigned int channels __unused
    , unsigned int rate)
{
    hardware_config->channels = 2;
    hardware_config->format = PCM_FORMAT_S16_LE;
    //TODO, maybe we should use "/sys/class/audiodsp/digtal_codec" as 4
    hardware_config->rate = rate * 4;
    hardware_config->period_count = PLAYBACK_PERIOD_COUNT;
    hardware_config->period_size = PERIOD_SIZE * 4 * 2;
    hardware_config->start_threshold = PLAYBACK_PERIOD_COUNT * hardware_config->period_size;
    hardware_config->avail_min = 0;

    return ;
}

/*
 *@brief get the hardware config parameters when the output format is DDP
*/
static void get_ddp_hardware_config_parameters(
    struct pcm_config *hardware_config
    , unsigned int channels __unused
    , unsigned int rate)
{
    hardware_config->channels = 2;
    hardware_config->format = PCM_FORMAT_S16_LE;
    /*
    for dd+ audio,ALSA driver has X4 clock,here we use 48K sample rate
    */
    hardware_config->rate = rate/* * 4 */;
    hardware_config->period_count = PLAYBACK_PERIOD_COUNT;
    hardware_config->period_size = PERIOD_SIZE * 4;
    hardware_config->start_threshold = hardware_config->period_size * hardware_config->period_count;
    hardware_config->avail_min = 0;

    return ;
}

/*
 *@brief get the hardware config parameters when the output format is DD
*/
static void get_dd_hardware_config_parameters(
    struct pcm_config *hardware_config
    , unsigned int channels __unused
    , unsigned int rate)
{
    hardware_config->channels = 2;
    hardware_config->format = PCM_FORMAT_S16_LE;
    hardware_config->rate = rate;
    hardware_config->period_count = PLAYBACK_PERIOD_COUNT;
    hardware_config->period_size = PERIOD_SIZE;
    hardware_config->start_threshold = hardware_config->period_size * hardware_config->period_count;
    hardware_config->avail_min = 0;

    return ;
}

/*
 *@brief get the hardware config parameters when the output format is PCM
*/
static void get_pcm_hardware_config_parameters(
    struct pcm_config *hardware_config
    , unsigned int channels
    , unsigned int rate
    , bool platform_is_tv)
{
    if (platform_is_tv == false) {
        if (channels <= 2) {
            hardware_config->channels = HARDWARE_CHANNEL_STEREO;
            hardware_config->format = PCM_FORMAT_S16_LE;
        } else {
            hardware_config->channels = HARDWARE_CHANNEL_7_1_MULTI;
            hardware_config->format = PCM_FORMAT_S32_LE;
        }
    } else {
        hardware_config->channels = HARDWARE_CHANNEL_7_1_MULTI;
        hardware_config->format = PCM_FORMAT_S32_LE;
    }
    hardware_config->rate = rate;//defualt sample rate = 48KHz
    hardware_config->period_count = PLAYBACK_PERIOD_COUNT;
    hardware_config->period_size = PERIOD_SIZE;
    hardware_config->start_threshold = hardware_config->period_size * hardware_config->period_count;
    hardware_config->avail_min = 0;

    return ;
}

/*
 *@brief get the hardware config parameters
*/
int get_hardware_config_parameters(
    struct pcm_config *final_config
    , audio_format_t output_format
    , unsigned int channels
    , unsigned int rate
    , bool platform_is_tv)
{
    ALOGI("%s()\n", __FUNCTION__);
    //DD+
    if (output_format == AUDIO_FORMAT_E_AC3) {
        get_ddp_hardware_config_parameters(final_config, channels, rate);
    }
    //DD
    else if (output_format == AUDIO_FORMAT_AC3) {
        get_dd_hardware_config_parameters(final_config, channels, rate);
    }
    //DTS-HD/TRUE-HD
    else if ((output_format == AUDIO_FORMAT_DTS_HD) || (output_format == AUDIO_FORMAT_DOLBY_TRUEHD)) {
        get_dd_hardware_config_parameters(final_config, channels, rate);
    }
    //PCM
    else {
        get_pcm_hardware_config_parameters(final_config, channels, rate, platform_is_tv);
    }
    ALOGI("%s() channels %d format %d period_count %d period_size %d rate %d\n",
          __FUNCTION__, final_config->channels, final_config->format, final_config->period_count,
          final_config->period_size, final_config->rate);

    return 0;
}
