/*
**
** Copyright 2012, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**  @author   Hugo Hong
**  @version  1.0
**  @date     2018/04/01
**  @par function description:
**  - 1 bluetooth rc audio stream in base class
*/

#ifndef ANDROID_AUDIO_STREAM_IN_H
#define ANDROID_AUDIO_STREAM_IN_H

#include <audio_utils/resampler.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <utils/Errors.h>
#include <utils/threads.h>

#include "AudioHotplugThread.h"

#ifndef REMOTE_CONTROL_INTERFACE
#define REMOTE_CONTROL_INTERFACE 1
#endif

namespace android {

class AudioHardwareInput;

class AudioStreamIn {
  public:
    AudioStreamIn(AudioHardwareInput& owner);
    virtual ~AudioStreamIn();

    virtual uint32_t          getSampleRate();
    virtual status_t          setSampleRate(uint32_t rate);
    virtual size_t            getBufferSize();
    virtual uint32_t          getChannelMask();
    virtual audio_format_t    getFormat();
    virtual status_t          setFormat(audio_format_t format);
    virtual status_t          standby();
    virtual status_t          dump(int fd);
    virtual status_t          setParameters(struct audio_stream* stream,
                                    const char* kvpairs);
    virtual char*             getParameters(const char* keys);
    virtual status_t          setGain(float gain);
    virtual ssize_t           read(void* buffer, size_t bytes);
    virtual uint32_t          getInputFramesLost();
    virtual status_t          addAudioEffect(effect_handle_t effect);
    virtual status_t          removeAudioEffect(effect_handle_t effect);

    virtual status_t          set(struct audio_stream_in *stream,
                                audio_format_t *pFormat,
                                uint32_t       *pChannelMask,
                                uint32_t       *pRate);

    virtual const AudioHotplugThread::DeviceInfo* getDeviceInfo() { return mCurrentDeviceInfo; }

    static uint32_t   getChannelCount() {
        return audio_channel_count_from_in_mask(kChannelMask);
    }

public:
    static const uint32_t kChannelMask;
    static const uint32_t kChannelCount;
    static const audio_format_t kAudioFormat;
    static bool       mStandby;

    AudioHardwareInput& mOwnerHAL;
    const AudioHotplugThread::DeviceInfo* mCurrentDeviceInfo;
    Mutex mLock;

    uint32_t          mRequestedSampleRate;
    bool              mDisabled;
    int               mInputSource;
    int               mReadStatus;

protected:
    status_t          startInputStream_l();
    status_t          standby_l();
    void setRemoteControlMicEnabled(bool flag);

private:

};

}; // namespace android

#endif  // ANDROID_AUDIO_STREAM_IN_H
