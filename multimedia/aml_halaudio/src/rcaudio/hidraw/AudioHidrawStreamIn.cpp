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

#define LOG_TAG "AudioHAL:AudioHidrawStreamIn"
#include <utils/log.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utils/String8.h>
#include <media/AudioParameter.h>
#include "AudioHardwareInput.h"
#include "AudioHidrawStreamIn.h"
#include "../../audio_hal/audio_hw.h"
#include "huitong_audio.h"

namespace android {
int AudioHidrawStreamIn::m_refNum = 0;

AudioHidrawStreamIn::AudioHidrawStreamIn(AudioHardwareInput& owner)
    : AudioStreamIn(owner)
{
    mStream = NULL;
    mPcmConfig.channels = 1;
    mPcmConfig.rate = 16000;
    mPcmConfig.period_size = DEFAULT_PERIOD_SIZE;
    mPcmConfig.period_count = PLAYBACK_PERIOD_COUNT;
    mPcmConfig.format = PCM_FORMAT_S16_LE;

    if (m_refNum++ == 0) {
        huitong_in_open_stream(getDeviceInfo()->hidraw_index);
    }
}

AudioHidrawStreamIn::~AudioHidrawStreamIn()
{
    if (--m_refNum == 0) {
        AudioStreamIn::standby();
        huitong_in_close_stream();
    }
}

// Perform stream initialization that may fail.
// Must only be called once at construction time.
status_t AudioHidrawStreamIn::set(struct audio_stream_in *stream,
                            audio_format_t *pFormat,
                            uint32_t *pChannelMask,
                            uint32_t *pRate)
{
    mStream = stream;
    struct aml_stream_in* tstream =
        reinterpret_cast<struct aml_stream_in*>(stream);

    memcpy (&tstream->config, &mPcmConfig, sizeof (mPcmConfig));

    return AudioStreamIn::set(stream, pFormat, pChannelMask, pRate);
}


uint32_t AudioHidrawStreamIn::getSampleRate()
{
    Mutex::Autolock _l(mLock);

    return huitong_in_get_sample_rate((const struct audio_stream*)mStream);
}

size_t AudioHidrawStreamIn::getBufferSize()
{
    Mutex::Autolock _l(mLock);

    return huitong_in_get_buffer_size((const struct audio_stream*)mStream);
}

status_t AudioHidrawStreamIn::standby()
{
    Mutex::Autolock _l(mLock);

    return huitong_in_standby((struct audio_stream*)mStream);
}


ssize_t AudioHidrawStreamIn::read(void* buffer, size_t bytes)
{
    AudioStreamIn::read(buffer, bytes);

    Mutex::Autolock _l(mLock);

    return huitong_in_read(mStream, buffer, bytes);
}

}; // namespace android
