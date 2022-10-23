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

#define LOG_TAG "libms12"
// #define LOG_NDEBUG 0
// #define LOG_NALOGV 0

#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
//#include <media/AudioSystem.h>

#include "DolbyMS12Status.h"
#include "dolby_ms12_status.h"


static android::Mutex mLock;
static android::DolbyMS12Status* gInstance = NULL;
static android::DolbyMS12Status* getInstance()
{
    ALOGV("+DolbyMS12Status %s()\n", __FUNCTION__);
    android::Mutex::Autolock autoLock(mLock);
    if (gInstance == NULL) {
        gInstance = new android::DolbyMS12Status();
    }
    ALOGV("-DolbyMS12Status %s() gInstance %p\n", __FUNCTION__, gInstance);
    return gInstance;
}

extern "C" void dolby_ms12_status_self_cleanup(void)
{
    ALOGV("+%s()\n", __FUNCTION__);
    android::Mutex::Autolock autoLock(mLock);
    if (gInstance) {
        delete gInstance;
        gInstance = NULL;
    }
    ALOGV("-%s() gInstance %p\n", __FUNCTION__, gInstance);
    return ;
}


extern "C" void set_mixer_playback_status(int flags)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setMixerPlayBackStatus(flags);
}

extern "C" int get_mixer_playback_status(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getMixerPlaybackStatus();
    else
        return MIXER_PLAYBACK_STOP;
}

extern "C" void set_direct_or_offload_playback_status(int flags)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setDirectOrOffloadPlaybackStatus(flags);
}

extern "C" int get_direct_or_offload_playback_status(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getDirectOrOffloadPlaybackStatus();
    else
        return DIRECT_0R_OFFLOAD_PLAYBACK_STOP;
}

extern "C" void set_mixer_playback_audio_stream_out_params(
    audio_format_t format
    , audio_channel_mask_t channel_mask
    , uint32_t sample_rate
    , audio_devices_t devices)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setMixerPlaybackAudioStreamOutParams(format, channel_mask, sample_rate, devices);
}

extern "C" void get_mixer_playback_audio_stream_out_params(
    audio_format_t *format
    , audio_channel_mask_t *channel_mask
    , uint32_t *sample_rate
    , audio_devices_t *devices)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getMixerPlaybackAudioStreamOutParams(format, channel_mask, sample_rate, devices);
}

extern "C" void set_direct_or_offload_playback_audio_stream_out_params(
    audio_format_t format
    , audio_channel_mask_t channel_mask
    , uint32_t sample_rate
    , audio_devices_t devices)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setDirectOrOffloadPlaybackAudioStreamOutParams(format, channel_mask, sample_rate, devices);
}

extern "C" void get_direct_or_offload_playback_audio_stream_out_params(
    audio_format_t *format
    , audio_channel_mask_t *channel_mask
    , uint32_t *sample_rate
    , audio_devices_t *devices)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getDirectOrOffloadPlaybackAudioStreamOutParams(format, channel_mask, sample_rate, devices);
}

extern "C" void set_offload_playback_dolby_ms12_output_format(audio_format_t format)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setOffloadPlaybackDolbyMS12OutputFormat(format);
}

extern "C" audio_format_t get_offload_playback_dolby_ms12_output_format(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getOffloadPlaybackDolbyMS12OutputFormat();
    else
        return AUDIO_FORMAT_DEFAULT;
}

extern "C" void set_offload_playback_audio_stream_out_format(audio_format_t format)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setOffloadPlaybackAudioStreamOutFormat(format);
}

extern "C" audio_format_t get_offload_playback_audio_stream_out_format(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getOffloadPlaybackAudioStreamOutFormat();
    else
        return AUDIO_FORMAT_INVALID;
}


/*
 *@brief set Audio Hal main format
 */
extern "C" void set_audio_main_format(audio_format_t format)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setAudioMainFormat(format);
}

/*
 *@brief get Audio Hal main format
 */
extern "C" audio_format_t get_audio_main_format(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getAudioMainFormat();
    else
        return AUDIO_FORMAT_INVALID;
}


/*
 *@brief set Audio Hal associate format
 */
extern "C" void set_audio_associate_format(audio_format_t format)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setAudioAssociateFormat(format);
}

/*
 *@brief get Audio Hal associate format
 */
extern "C" audio_format_t get_audio_associate_format(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getAudioAssociateFormat();
    else
        return AUDIO_FORMAT_INVALID;
}
/*
 *@brief set Audio Hal system format
 */
extern "C" void set_audio_system_format(audio_format_t format)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setAudioSystemFormat(format);
}

/*
 *@brief get Audio Hal system format
 */
extern "C" audio_format_t get_audio_system_format(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getAudioSystemFormat();
    else
        return AUDIO_FORMAT_INVALID;
}


/*
 *@brief set TV audio main format
 */
extern "C" void set_dd_support_flag(bool flag)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setDDSupportFlag(flag);
}

/*
 *@brief get TV audio main format
 */
extern "C" bool get_dd_support_flag(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getDDSupportFlag();
    else
        return false;
}

/*
 *@brief set ddp support flag
 */
extern "C" void set_ddp_support_flag(bool flag)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setDDPSupportFlag(flag);
}

/*
 *@brief get ddp support flag
 */
extern "C" bool get_ddp_support_flag(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getDDPSupportFlag();
    else
        return false;
}

/*
 *@brief set dd max audio channel mask
 */
extern "C" void set_dd_max_audio_channel_mask(audio_channel_mask_t channel_mask)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setDDMaxAudioChannelMask(channel_mask);
}

/*
 *@brief get dd max audio channel mask
 */
extern "C" audio_channel_mask_t get_dd_max_audio_channel_mask(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getDDMaxAudioChannelMask();
    else
        return AUDIO_CHANNEL_NONE;
}


/*
 *@brief set ddp max channel mask
 */
extern "C" void set_ddp_max_audio_channel_mask(audio_channel_mask_t channel_mask)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setDDPMaxAudioChannelMask(channel_mask);
}

/*
 *@brief get ddp max channel mask
 */
extern "C" audio_channel_mask_t get_ddp_max_audio_channel_mask(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getDDPMaxAudioChannelMask();
    else
        return AUDIO_CHANNEL_NONE;
}


/*
 *@brief set the Need Output Max Format according to User Setting
 */
extern "C" void set_max_format_by_user_setting(audio_format_t format)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->setMaxFormatByUserSetting(format);
}

/*
 *@brief get the Need Output Max Format according to User Setting
 */
extern "C" audio_format_t get_max_format_by_user_setting(void)
{
    android::DolbyMS12Status* dolby_ms12_status_instance = getInstance();
    if (dolby_ms12_status_instance)
        return dolby_ms12_status_instance->getMaxFormatByUserSetting();
    else
        return AUDIO_FORMAT_PCM;
}
