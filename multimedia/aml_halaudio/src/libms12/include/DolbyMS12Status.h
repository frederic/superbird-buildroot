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

#ifndef _DOLBY_MS12_STATUS_H_
#define _DOLBY_MS12_STATUS_H_


#include "audio_core.h"
#include <system/audio_policy.h>
#include <utils/Mutex.h>

namespace android {

class DolbyMS12Status
{
public:
    static DolbyMS12Status* getInstance();
    DolbyMS12Status();
    virtual ~DolbyMS12Status();
    virtual void setMixerPlayBackStatus(int flags) {
                mDoblyMS12StatusInputPCM = flags;
                if (!mDoblyMS12StatusInputPCM) {
                    mMixerAudioStreamOutformat = AUDIO_FORMAT_INVALID;
                    mMixerAudioChannelMask = AUDIO_CHANNEL_INVALID;
                    mMixerAudioStreamOutSampleRate = 0;
                    mMixerOutDevices = AUDIO_DEVICE_NONE ;
                }
                // ALOGV("%s mDoblyMS12StatusInputPCM %s", __FUNCTION__, (mDoblyMS12StatusInputPCM == 1)? "MixerPlayback(Input PCM) Start" : "MixerPlayback(Input PCM) Stop");
            }
    virtual int getMixerPlaybackStatus(void) {
                return mDoblyMS12StatusInputPCM;
            }
    virtual void setDirectOrOffloadPlaybackStatus(int flags) {
                mDolbyMS12StatusInputRaw = flags;
                if (!mDolbyMS12StatusInputRaw) {
                    mDirectOrOffloadAudioStreamOutformat = AUDIO_FORMAT_INVALID;
                    mDirectOrOffloadAudioChannelMask = AUDIO_CHANNEL_INVALID;
                    mDirectOrOffloadAudioStreamOutSampleRate = 0;
                    mDirectOrOffloadOutDevices = AUDIO_DEVICE_NONE ;
                }
                // ALOGV("%s mDolbyMS12StatusInputRaw %s", __FUNCTION__, (mDolbyMS12StatusInputRaw == 1)? "OffloadPlayback(Input raw) Start" : "OffloadPlayback(Input raw) Stop");
            }
    virtual int getDirectOrOffloadPlaybackStatus(void) {
                return mDolbyMS12StatusInputRaw;
            }

    virtual void setMixerPlaybackAudioStreamOutParams(audio_format_t format, audio_channel_mask_t channel_mask, uint32_t sample_rate, audio_devices_t devices)
            {
                // ALOGV("%s() format %#x channel_mask %#x sample_rate %d devices %x\n", __FUNCTION__, format, channel_mask, sample_rate, devices);
                mMixerAudioStreamOutformat = format;
                mMixerAudioChannelMask = channel_mask;
                mMixerAudioStreamOutSampleRate = sample_rate;
                mMixerOutDevices = devices;
            }
    virtual void getMixerPlaybackAudioStreamOutParams(audio_format_t *format, audio_channel_mask_t *channel_mask, uint32_t *sample_rate, audio_devices_t *devices)
            {
                if (format && channel_mask && sample_rate && devices) {
                    if (mMixerAudioStreamOutformat)
                        *format = mMixerAudioStreamOutformat;
                    if (mMixerAudioChannelMask)
                        *channel_mask = mMixerAudioChannelMask;
                    if (mMixerAudioStreamOutSampleRate)
                        *sample_rate = mMixerAudioStreamOutSampleRate;
                    if (mMixerOutDevices)
                        *devices = mMixerOutDevices;
                    // ALOGV("%s() *format %#x *channel_mask %#x *sample_rate %d *devices %x\n", __FUNCTION__, *format, *channel_mask, *sample_rate, *devices);
                }
            }
    virtual void setDirectOrOffloadPlaybackAudioStreamOutParams(audio_format_t format, audio_channel_mask_t channel_mask, uint32_t sample_rate, audio_devices_t devices)
            {
                // ALOGV("%s() format %#x channel_mask %#x sample_rate %d devices %x\n", __FUNCTION__, format, channel_mask, sample_rate, devices);
                mDirectOrOffloadAudioStreamOutformat = format;
                mDirectOrOffloadAudioChannelMask = channel_mask;
                mDirectOrOffloadAudioStreamOutSampleRate = sample_rate;
                mDirectOrOffloadOutDevices = devices;
            }
    virtual void getDirectOrOffloadPlaybackAudioStreamOutParams(audio_format_t *format, audio_channel_mask_t *channel_mask, uint32_t *sample_rate, audio_devices_t *devices)
            {
                if (format && channel_mask && sample_rate && devices) {
                    if (mDirectOrOffloadAudioStreamOutformat)
                        *format = mDirectOrOffloadAudioStreamOutformat;
                    if (mDirectOrOffloadAudioChannelMask)
                        *channel_mask = mDirectOrOffloadAudioChannelMask;
                    if (mDirectOrOffloadAudioStreamOutSampleRate)
                        *sample_rate = mDirectOrOffloadAudioStreamOutSampleRate;
                    if (mDirectOrOffloadOutDevices)
                        *devices = mDirectOrOffloadOutDevices;
                    // ALOGV("%s() *format %#x *channel_mask %#x *sample_rate %d *devices %x\n", __FUNCTION__, *format, *channel_mask, *sample_rate, *devices);
                }
            }
    virtual void setOffloadPlaybackDolbyMS12OutputFormat(audio_format_t format) {
                ALOGV("%s() format %#x\n", __FUNCTION__, format);
                mOffloadPlaybackDolbyMS12Outputformat = format;
            }

    virtual audio_format_t getOffloadPlaybackDolbyMS12OutputFormat(void) { return mOffloadPlaybackDolbyMS12Outputformat; }

    virtual void setCurrentForceUse(audio_policy_forced_cfg_t force_use) { mForceUse = force_use; }

    virtual audio_policy_forced_cfg_t getCurrentForceUse(void) { return mForceUse; }

    virtual void setOffloadPlaybackAudioStreamOutFormat(audio_format_t format) {
                ALOGV("%s() format %#x\n", __FUNCTION__, format);
                mOffloadPlaybackAudioStreamOutformat = format;
            }

    virtual audio_format_t getOffloadPlaybackAudioStreamOutFormat(void) { return mDirectOrOffloadAudioStreamOutformat; }


    /*
     *@brief set Audio Hal main format
     */
    virtual void setAudioMainFormat(audio_format_t format);

    /*
     *@brief get Audio Hal main format
     */
    virtual audio_format_t getAudioMainFormat(void);

    /*
     *@brief set Audio Hal associate format
     */
    virtual void setAudioAssociateFormat(audio_format_t format);

    /*
     *@brief get Audio Hal associate format
     */
    virtual audio_format_t getAudioAssociateFormat(void);

    /*
     *@brief set Audio Hal System format
     */
    virtual void setAudioSystemFormat(audio_format_t format);

    /*
     *@brief get Audio Hal System format
     */
    virtual audio_format_t getAudioSystemFormat(void);

    /*
     *@brief set dd support flag
     */
    virtual void setDDSupportFlag(bool flag);

    /*
     *@brief get dd support flag
     */
    virtual bool getDDSupportFlag(void);

    /*
     *@brief set ddp support flag
     */
    virtual void setDDPSupportFlag(bool flag);

    /*
     *@brief get ddp support flag
     */
    virtual bool getDDPSupportFlag(void);

    /*
     *@brief set dd max channel mask
     */
    virtual void setDDMaxAudioChannelMask(audio_channel_mask_t channel_mask);

    /*
     *@brief get dd max channel mask
     */
    virtual audio_channel_mask_t getDDMaxAudioChannelMask(void);

    /*
     *@brief set ddp max channel mask
     */
    virtual void setDDPMaxAudioChannelMask(audio_channel_mask_t channel_mask);

    /*
     *@brief get ddp max channel mask
     */
    virtual audio_channel_mask_t getDDPMaxAudioChannelMask(void);

    /*
     *@brief set the Need Output Max Format according to User Setting
     */
    virtual void setMaxFormatByUserSetting(audio_format_t format);

    /*
     *@brief set the Need Output Max Format according to User Setting
     */
    virtual audio_format_t getMaxFormatByUserSetting(void);


private:
    DolbyMS12Status(const DolbyMS12Status&);
    DolbyMS12Status& operator = (const DolbyMS12Status&);
    static android::Mutex mLock;
    static DolbyMS12Status *gInstance;
    int mDoblyMS12StatusInputPCM;
    int mDolbyMS12StatusInputRaw;//dd/dd+/aac/heaac-v1/heaac-v2

    //mixer
    audio_format_t mMixerAudioStreamOutformat;
    audio_channel_mask_t mMixerAudioChannelMask;
    uint32_t mMixerAudioStreamOutSampleRate;
    audio_devices_t mMixerOutDevices;
    //direct or offload
    audio_format_t mDirectOrOffloadAudioStreamOutformat;
    audio_channel_mask_t mDirectOrOffloadAudioChannelMask;
    uint32_t mDirectOrOffloadAudioStreamOutSampleRate;
    audio_devices_t mDirectOrOffloadOutDevices;

    audio_format_t mOffloadPlaybackDolbyMS12Outputformat;
    audio_policy_forced_cfg_t mForceUse;
    audio_format_t mOffloadPlaybackAudioStreamOutformat;

    //for audio hal
    audio_format_t mAudioMainFormat;
    audio_format_t mAudioAssociateFormat;
    audio_format_t mAudioSystemFormat;

    //for dd(ac3) support
    bool mDDSupportFlag;
    audio_channel_mask_t mDDMaxAudioChannelMask;
    //for ddp(eac3) support
    bool mDDPSupportFlag;
    audio_channel_mask_t mDDPMaxAudioChannelMask;

    //for User Setting
    audio_format_t mMaxFormatByUserSetting;//we suppose that "pcm < dd < ddp" is true!
};

}   // android

#endif  // _DOLBY_MS12_STATUS_H_
