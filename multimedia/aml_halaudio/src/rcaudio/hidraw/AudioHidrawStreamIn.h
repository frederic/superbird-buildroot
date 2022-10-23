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
**  - 1 bluetooth rc audio stream in from hidraw node
*/

#ifndef ANDROID_AUDIO_HIDRAW_STREAM_IN_H
#define ANDROID_AUDIO_HIDRAW_STREAM_IN_H

#include <audio_utils/resampler.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include "AudioStreamIn.h"
#include "AudioHotplugThread.h"

namespace android {

class AudioHardwareInput;

class AudioHidrawStreamIn : public AudioStreamIn{
  public:
    AudioHidrawStreamIn(AudioHardwareInput& owner);
    ~AudioHidrawStreamIn();

    virtual uint32_t          getSampleRate();
    virtual size_t            getBufferSize();
    virtual status_t          standby();
    virtual ssize_t           read(void* buffer, size_t bytes);
    virtual status_t          set(struct audio_stream_in *stream,
                                audio_format_t *pFormat,
                                uint32_t       *pChannelMask,
                                uint32_t       *pRate);

  private:
      struct pcm_config mPcmConfig;
      struct audio_stream_in *mStream;
      static int m_refNum;
};

}; // namespace android

#endif  // ANDROID_AUDIO_HIDRAW_STREAM_IN_H
