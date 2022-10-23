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
**  - 1 bluetooth rc audio stream in from soundcard driver
*/

#ifndef ANDROID_AUDIO_SOUNDCARD_STREAM_IN_H
#define ANDROID_AUDIO_SOUNDCARD_STREAM_IN_H

#include <audio_utils/resampler.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include "AudioStreamIn.h"
#include "AudioHotplugThread.h"

namespace android {

class AudioHardwareInput;

class AudioSoundCardStreamIn : public  AudioStreamIn{
  public:
    AudioSoundCardStreamIn(AudioHardwareInput& owner);
    ~AudioSoundCardStreamIn();


    virtual status_t          standby();
    virtual status_t          dump(int fd);
    virtual status_t          setParameters(struct audio_stream* stream,
                                    const char* kvpairs);
    virtual ssize_t           read(void* buffer, size_t bytes);

    virtual status_t          startInputStream_l();
    virtual status_t          standby_l();

  private:

    static uint32_t   getFrameSize() {
        return getChannelCount() * audio_bytes_per_sample(kAudioFormat);
    }

    ssize_t           readFrames_l(void* buffer, ssize_t frames);

    // resampler buffer provider thunks
    static int        getNextBufferThunk(
            struct resampler_buffer_provider* bufferProvider,
            struct resampler_buffer* buffer);
    static void       releaseBufferThunk(
            struct resampler_buffer_provider* bufferProvider,
            struct resampler_buffer* buffer);

    // resampler buffer provider methods
    int               getNextBuffer(struct resampler_buffer* buffer);
    void              releaseBuffer(struct resampler_buffer* buffer);

    static const int  kPeriodCount;

    struct pcm*       mPcm;
    struct pcm_config mPcmConfig;

    struct resampler_itfe*         mResampler;
    struct ResamplerBufferProviderWrapper {
        struct resampler_buffer_provider provider;
        AudioSoundCardStreamIn* thiz;
    } mResamplerProviderWrapper;

    int16_t*          mBuffer;
    size_t            mBufferSize;
    unsigned int      mFramesIn;
};

}; // namespace android

#endif  // ANDROID_AUDIO_SOUNDCARD_STREAM_IN_H
