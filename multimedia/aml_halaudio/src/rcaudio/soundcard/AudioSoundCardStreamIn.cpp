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

#define LOG_TAG "AudioHAL:AudioSoundCardStreamIn"
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
#include "AudioSoundCardStreamIn.h"

namespace android {

//const audio_format_t AudioStreamIn::kAudioFormat = AUDIO_FORMAT_pr_info_16_BIT;
//const uint32_t AudioStreamIn::kChannelMask = AUDIO_CHANNEL_IN_MONO;

// number of periods in the ALSA buffer
const int AudioSoundCardStreamIn::kPeriodCount = 4;

AudioSoundCardStreamIn::AudioSoundCardStreamIn(AudioHardwareInput& owner)
    : AudioStreamIn(owner)
    , mPcm(NULL)
    , mResampler(NULL)
    , mBuffer(NULL)
    , mBufferSize(0)
    , mFramesIn(0)
{
    struct resampler_buffer_provider& provider =
            mResamplerProviderWrapper.provider;
    provider.get_next_buffer = getNextBufferThunk;
    provider.release_buffer = releaseBufferThunk;
    mResamplerProviderWrapper.thiz = this;
}

AudioSoundCardStreamIn::~AudioSoundCardStreamIn()
{
    Mutex::Autolock _l(mLock);
    standby_l();
}


status_t AudioSoundCardStreamIn::standby()
{
    Mutex::Autolock _l(mLock);
    return standby_l();
}

status_t AudioSoundCardStreamIn::standby_l()
{
    if (mPcm) {
        ALOGD("AudioSoundCardStreamIn::standby_l, call pcm_close()");
        pcm_close(mPcm);
        mPcm = NULL;
    }

    if (mResampler) {
        release_resampler(mResampler);
        mResampler = NULL;
    }
    if (mBuffer) {
        delete [] mBuffer;
        mBuffer = NULL;
    }

    return AudioStreamIn::standby_l();
}

#define DUMP(a...) \
    snprintf(buffer, SIZE, a); \
    buffer[SIZE - 1] = 0; \
    result.append(buffer);

status_t AudioSoundCardStreamIn::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    DUMP("\n AudioStreamIn::dump\n");

    {
        DUMP("\toutput sample rate: %d\n", mRequestedSampleRate);
        if (mPcm) {
            DUMP("\tinput sample rate: %d\n", mPcmConfig.rate);
            DUMP("\tinput channels: %d\n", mPcmConfig.channels);
        }
    }

    ::write(fd, result.string(), result.size());

    return NO_ERROR;
}

status_t AudioSoundCardStreamIn::setParameters(struct audio_stream* stream,
                                      const char* kvpairs)
{
    (void) stream;
    AudioParameter param = AudioParameter(String8(kvpairs));
    status_t status = NO_ERROR;
    String8 keySource = String8(AudioParameter::keyInputSource);
    int intVal;

    if (param.getInt(keySource, intVal) == NO_ERROR) {
        ALOGI("AudioStreamIn::setParameters, mInputSource set to %d", intVal);
        mInputSource = intVal;
    }

    return status;
}

ssize_t AudioSoundCardStreamIn::read(void* buffer, size_t bytes)
{
    Mutex::Autolock _l(mLock);

    status_t status = NO_ERROR;

    if (mStandby) {
        status = startInputStream_l();
        // Only try to start once to prevent pointless spew.
        // If mic is not available then read will return silence.
        // This is needed to prevent apps from hanging.
        mStandby = false;
        if (status != NO_ERROR) {
            mDisabled = true;
        }
    }

    if ((status == NO_ERROR) && !mDisabled) {
        int ret = readFrames_l(buffer, bytes / getFrameSize());
        status = (ret < 0) ? INVALID_OPERATION : NO_ERROR;
    }

    if ((status != NO_ERROR) || mDisabled) {
        memset(buffer, 0, bytes);

        // TODO: This code needs to project a timeline based on the number
        // of audio frames synthesized from the last time we returned data
        // from an actual audio device (or establish a fake timeline to obey
        // if we have never returned any data from an actual device and need
        // to synth on the first call to read)
        usleep(bytes * 1000000 / getFrameSize() / mRequestedSampleRate);
    } else {
        bool mute;
        mOwnerHAL.getMicMute(&mute);
        if (mute) {
            memset(buffer, 0, bytes);
        }
    }

    return bytes;
}

status_t AudioSoundCardStreamIn::startInputStream_l()
{
    //ALOGI("AudioSoundCardStreamIn::startInputStream_l, entry, built %s", __DATE__);

    AudioStreamIn::startInputStream_l();

    memset(&mPcmConfig, 0, sizeof(mPcmConfig));

    unsigned int requestedChannelCount = getChannelCount();

    // Clip to min/max available.
    if (requestedChannelCount < mCurrentDeviceInfo->minChannelCount ) {
        mPcmConfig.channels = mCurrentDeviceInfo->minChannelCount;
    } else if (requestedChannelCount > mCurrentDeviceInfo->maxChannelCount ) {
        mPcmConfig.channels = mCurrentDeviceInfo->maxChannelCount;
    } else {
        mPcmConfig.channels = requestedChannelCount;
    }

    // Clip to min/max available from driver.
    uint32_t chosenSampleRate = mRequestedSampleRate;
    if (chosenSampleRate < mCurrentDeviceInfo->minSampleRate) {
        chosenSampleRate = mCurrentDeviceInfo->minSampleRate;
    } else if (chosenSampleRate > mCurrentDeviceInfo->maxSampleRate) {
        chosenSampleRate = mCurrentDeviceInfo->maxSampleRate;
    }


    mPcmConfig.rate = chosenSampleRate;

    mPcmConfig.period_size =
            AudioHardwareInput::kPeriodMsec * mPcmConfig.rate / 1000;
    mPcmConfig.period_count = kPeriodCount;
    mPcmConfig.format = PCM_FORMAT_S16_LE;

    ALOGD("AudioStreamIn::startInputStream_l, call pcm_open()");
    struct pcm* pcm = pcm_open(mCurrentDeviceInfo->pcmCard, mCurrentDeviceInfo->pcmDevice,
                               PCM_IN, &mPcmConfig);

    if (!pcm_is_ready(pcm)) {
        ALOGE("ERROR AudioStreamIn::startInputStream_l, pcm_open failed");
        pcm_close(pcm);
        if (mCurrentDeviceInfo->forVoiceRecognition) {
            //setRemoteControlMicEnabled(false);
        }
        return NO_MEMORY;
    }

    mBufferSize = pcm_frames_to_bytes(pcm, mPcmConfig.period_size);
    if (mBuffer) {
        delete [] mBuffer;
    }
    mBuffer = new int16_t[mBufferSize / sizeof(uint16_t)];

    if (mResampler) {
        release_resampler(mResampler);
        mResampler = NULL;
    }
    if (mPcmConfig.rate != mRequestedSampleRate) {
        ALOGD("AudioStreamIn::startInputStream_l, call create_resampler( %d  to %d)",
            mPcmConfig.rate, mRequestedSampleRate);
        int ret = create_resampler(mPcmConfig.rate,
                                   mRequestedSampleRate,
                                   1,
                                   RESAMPLER_QUALITY_DEFAULT,
                                   &mResamplerProviderWrapper.provider,
                                   &mResampler);
        if (ret != 0) {
            ALOGW("AudioStreamIn: unable to create resampler");
            pcm_close(pcm);
            return static_cast<status_t>(ret);
        }
    }

    mPcm = pcm;

    return NO_ERROR;
}

// readFrames() reads frames from kernel driver, down samples to the capture
// rate if necessary and outputs the number of frames requested to the buffer
// specified
ssize_t AudioSoundCardStreamIn::readFrames_l(void* buffer, ssize_t frames)
{
    ssize_t framesWr = 0;
    size_t frameSize = getFrameSize();

    while (framesWr < frames) {
        size_t framesRd = frames - framesWr;
        if (mResampler) {
            char* outFrame = static_cast<char*>(buffer) +
                    (framesWr * frameSize);
            mResampler->resample_from_provider(
                mResampler,
                reinterpret_cast<int16_t*>(outFrame),
                &framesRd);
        } else {
            struct resampler_buffer buf;
            buf.raw = NULL;
            buf.frame_count = framesRd;

            getNextBuffer(&buf);
            if (buf.raw != NULL) {
                memcpy(static_cast<char*>(buffer) + (framesWr * frameSize),
                       buf.raw,
                       buf.frame_count * frameSize);
                framesRd = buf.frame_count;
            }
            releaseBuffer(&buf);
        }
        // mReadStatus is updated by getNextBuffer(), which is called by the
        // resampler
        if (mReadStatus != 0)
            return mReadStatus;

        framesWr += framesRd;
    }
    return framesWr;
}

int AudioSoundCardStreamIn::getNextBufferThunk(
        struct resampler_buffer_provider* bufferProvider,
        struct resampler_buffer* buffer)
{
    ResamplerBufferProviderWrapper* wrapper =
            reinterpret_cast<ResamplerBufferProviderWrapper*>(
                reinterpret_cast<char*>(bufferProvider) -
                offsetof(ResamplerBufferProviderWrapper, provider));

    return wrapper->thiz->getNextBuffer(buffer);
}

void AudioSoundCardStreamIn::releaseBufferThunk(
        struct resampler_buffer_provider* bufferProvider,
        struct resampler_buffer* buffer)
{
    ResamplerBufferProviderWrapper* wrapper =
            reinterpret_cast<ResamplerBufferProviderWrapper*>(
                reinterpret_cast<char*>(bufferProvider) -
                offsetof(ResamplerBufferProviderWrapper, provider));

    wrapper->thiz->releaseBuffer(buffer);
}

// called while holding mLock
int AudioSoundCardStreamIn::getNextBuffer(struct resampler_buffer* buffer)
{
    if (buffer == NULL) {
        return -EINVAL;
    }

    if (mPcm == NULL) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        mReadStatus = -ENODEV;
        return -ENODEV;
    }

    if (mFramesIn == 0) {
        mReadStatus = pcm_read(mPcm, mBuffer, mBufferSize);
        if (mReadStatus) {
            ALOGE("get_next_buffer() pcm_read error %d", mReadStatus);
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return mReadStatus;
        }

        mFramesIn = mPcmConfig.period_size;
        if (mPcmConfig.channels == 2) {
            // Discard the right channel.
            // TODO: this is what other HALs are doing to handle stereo input
            // devices.  Need to verify if this is appropriate for ATV Remote.
            for (unsigned int i = 1; i < mFramesIn; i++) {
                mBuffer[i] = mBuffer[i * 2];
            }
        }
    }

    buffer->frame_count = (buffer->frame_count > mFramesIn) ?
            mFramesIn : buffer->frame_count;
    buffer->i16 = mBuffer + (mPcmConfig.period_size - mFramesIn);

    return mReadStatus;
}

// called while holding mLock
void AudioSoundCardStreamIn::releaseBuffer(struct resampler_buffer* buffer)
{
    if (buffer == NULL) {
        return;
    }

    mFramesIn -= buffer->frame_count;
}

}; // namespace android
