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


namespace android {

#define MIXER_PLAYBACK_START 1
#define MIXER_PLAYBACK_STOP 0
#define DIRECT_OR_OFFLOAD_PLAYBACK_START 1
#define DIRECT_0R_OFFLOAD_PLAYBACK_STOP 0


DolbyMS12Status::DolbyMS12Status() :
    mDoblyMS12StatusInputPCM(MIXER_PLAYBACK_START)
    , mDolbyMS12StatusInputRaw(DIRECT_0R_OFFLOAD_PLAYBACK_STOP)
    , mOffloadPlaybackDolbyMS12Outputformat(AUDIO_FORMAT_DEFAULT)
    , mForceUse(AUDIO_POLICY_FORCE_NONE)
    , mAudioMainFormat(AUDIO_FORMAT_INVALID)
    , mAudioAssociateFormat(AUDIO_FORMAT_INVALID)
    , mAudioSystemFormat(AUDIO_FORMAT_INVALID)
    , mDDSupportFlag(false)
    , mDDPSupportFlag(false)
{
    ALOGD("%s()", __FUNCTION__);
}


DolbyMS12Status::~DolbyMS12Status()
{
    ALOGD("%s()", __FUNCTION__);
}


/*
 *@brief set Audio Hal main format
 */
void DolbyMS12Status::setAudioMainFormat(audio_format_t format)
{
    mAudioMainFormat = format;
}

/*
 *@brief get Audio Hal main format
 */
audio_format_t DolbyMS12Status::getAudioMainFormat(void)
{
    return mAudioMainFormat;
}

/*
 *@brief set Audio Hal associate format
 */
void DolbyMS12Status::setAudioAssociateFormat(audio_format_t format)
{
    mAudioAssociateFormat = format;
}

/*
 *@brief get Audio Hal associate format
 */
audio_format_t DolbyMS12Status::getAudioAssociateFormat(void)
{
    return mAudioAssociateFormat;
}

/*
 *@brief set Audio Hal System format
 */
void DolbyMS12Status::setAudioSystemFormat(audio_format_t format)
{
    mAudioSystemFormat = format;
}

/*
 *@brief get Audio Hal System format
 */
audio_format_t DolbyMS12Status::getAudioSystemFormat(void)
{
    return mAudioSystemFormat;
}

/*
 *@brief set dd support flag
 */
void DolbyMS12Status::setDDSupportFlag(bool flag)
{
    mDDSupportFlag = flag;
}

/*
 *@brief get dd support flag
 */
bool DolbyMS12Status::getDDSupportFlag(void)
{
    return mDDSupportFlag;
}

/*
 *@brief set ddp support flag
 */
void DolbyMS12Status::setDDPSupportFlag(bool flag)
{
    mDDPSupportFlag = flag;
}

/*
 *@brief get ddp support flag
 */
bool DolbyMS12Status::getDDPSupportFlag(void)
{
    return mDDPSupportFlag;
}

/*
 *@brief set dd max channel mask
 */
void DolbyMS12Status::setDDMaxAudioChannelMask(audio_channel_mask_t channel_mask)
{
    mDDMaxAudioChannelMask = channel_mask;
}

/*
 *@brief get dd max channel mask
 */
audio_channel_mask_t DolbyMS12Status::getDDMaxAudioChannelMask(void)
{
    return mDDMaxAudioChannelMask;
}

/*
 *@brief set ddp max channel mask
 */
void DolbyMS12Status::setDDPMaxAudioChannelMask(audio_channel_mask_t channel_mask)
{
    mDDPMaxAudioChannelMask = channel_mask;
}

/*
 *@brief get ddp max channel mask
 */
audio_channel_mask_t DolbyMS12Status::getDDPMaxAudioChannelMask(void)
{
    return mDDPMaxAudioChannelMask;
}

/*
 *@brief set the Need Output Max Format according to User Setting
 */
void DolbyMS12Status::setMaxFormatByUserSetting(audio_format_t format)
{
    mMaxFormatByUserSetting = format;
}

/*
 *@brief set the Need Output Max Format according to User Setting
 */
audio_format_t DolbyMS12Status::getMaxFormatByUserSetting(void)
{
    return mMaxFormatByUserSetting;
}
}   // namespace android
