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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <audio_utils/primitives.h>
#include <audio_utils/sndfile.h>
//#include <utils/Vector.h>
#include <media/AudioBufferProvider.h>
#include <media/AudioResampler.h>
#include "aml_resample_wrap.h"
#include <cutils/log.h>
#undef LOG_TAG
#define LOG_TAG "audio_resample_wrap"


namespace android
{

class Provider: public AudioBufferProvider
{
public:
    Provider(size_t frameSize, read_func read, void *handle)
    : mFrameSize(frameSize),
    mRead(read),
    mHandle(handle) {
        mWorkBuf = (unsigned char*)malloc (8192);
        if (!mWorkBuf) {
            ALOGE("fail failed check!!!\n");
        } else {
            mWorkBufSize = 8192;
        }
    }

    virtual status_t getNextBuffer(Buffer* buffer) {
        size_t byte_read;
        size_t requestedFrames = buffer->frameCount;
        size_t input_size = requestedFrames * mFrameSize;

        if (input_size > mWorkBufSize) {
            mWorkBuf = (unsigned char *)realloc(mWorkBuf, input_size);
            if (!mWorkBuf) {
                ALOGE("malloc %d fail\n",input_size);
            } else {
                mWorkBufSize = input_size;
            }
        }
        byte_read = mRead(mHandle, mWorkBuf, input_size);

        if (byte_read == 0) {
            buffer->frameCount = 0;
            buffer->raw = NULL;
            //ALOGE("There is no enough data\n");
            return NOT_ENOUGH_DATA;
        }
        if (input_size != byte_read) {
            //ALOGE("reading failed,check! need=%d read=%d\n",input_size, byte_read);
            buffer->frameCount = byte_read/mFrameSize;
        }

        buffer->raw = (char *)mWorkBuf;

        return NO_ERROR;
    }
    virtual void releaseBuffer(Buffer* buffer) {
        buffer->frameCount = 0;
        buffer->raw = NULL;
    }

    ~Provider() {
        if (mWorkBuf) {
            free(mWorkBuf);
            mWorkBuf = NULL;
        }
    }
private:
    const size_t    mFrameSize; // size of each frame in bytes
    unsigned char   *mWorkBuf;
    size_t          mWorkBufSize;
    read_func       mRead;
    void            *mHandle;
};

#define AUDIO_OUTPUT_SR (48000)

extern "C" int android_resample_init(android_resample_handle_t *handle,
                                     int sr,
                                     audio_format_t aformat,
                                     int ch,
                                     read_func read,
                                     void *read_handle)
{

    AudioResampler* resampler = NULL;
    Provider*       provider = NULL;

    int sample_size = audio_bytes_per_sample(aformat);

    resampler = AudioResampler::create(AUDIO_FORMAT_PCM_16_BIT,
                                    ch,
                                    handle->output_sr,
                                    //AudioResampler::DYN_HIGH_QUALITY
                                    AudioResampler::DYN_MED_QUALITY
                                    //AudioResampler::DYN_LOW_QUALITY
                                    );
    if (!resampler) {
        ALOGE("create resample failed\n");
        return -1;
    }
    resampler->setSampleRate(sr);
    resampler->setVolume(AudioResampler::UNITY_GAIN_FLOAT,AudioResampler::UNITY_GAIN_FLOAT);
    provider = new Provider(ch*sample_size, read, read_handle);
    if (!provider) {
      ALOGE("new provider failed\n");
      delete resampler;
      resampler = NULL;
      return -1;
    }

    handle->resampler = (void *)resampler;
    handle->provider  = (void* )provider;

    ALOGI("%s input sr = %d ch=%d output sr =%d\n",__func__, sr , ch, handle->output_sr);
    return 0;
}

//we are using the same buffer for audio resamping hook function
extern "C" int android_resample_read(android_resample_handle_t *handle, void *buf, size_t in_size)
{
    size_t resampled_frame = 0;
    size_t resampled_size = 0;

    AudioResampler* resampler = NULL;
    Provider*       provider = NULL;


    if (handle == NULL) {
        return -1;
    }

    resampler = (AudioResampler*) handle->resampler;
    provider  = (Provider*) handle->provider;
    if (resampler) {

        /*resample out is Q4.27 in 32bits, we need 16bit*/
        if ((in_size >> 2) > (handle->temp_buf_size>> 3)) {
            ALOGE("Resamle input is too big\n");
            return -1;
        }
        // 2ch, 16bit
        /*must init the buf, otherwise it will cause noise*/
        memset(handle->temp_buf,0,handle->temp_buf_size);

        resampled_frame = resampler->resample((int32_t *)handle->temp_buf, in_size >> 2, provider);
        //ALOGE("in_frames=%d resampled_frame=%d size=%d\n",in_size >> 2,resampled_frame, handle->temp_buf_size);

        memcpy_to_i16_from_q4_27((int16_t *)buf, (int32_t *)handle->temp_buf, resampled_frame*2);
        resampled_size = resampled_frame * 2 * 2;
    }

#if 0
            {
                FILE *fp1 = fopen("/data/audio_hal/resampleout_ori.pcm", "a+");
                if (fp1) {
                    fwrite((char *)buf, 1, resampled_size, fp1);
                    fclose(fp1);
                } else {
                    ALOGD("could not open files! error:%d", errno);
                }
            }
#endif

    return resampled_size;
}

extern "C" int android_resample_release(android_resample_handle_t *handle)
{
    AudioResampler* resampler = NULL;
    Provider*       provider = NULL;

    if (handle == NULL) {
        return -1;
    }

    resampler = (AudioResampler*) handle->resampler;
    provider  = (Provider*) handle->provider;


    if (resampler) {
        resampler->reset();
        delete resampler;
        resampler = NULL;
    }

    if (provider) {
        delete provider;
        provider = NULL;
    }

    ALOGI("%s done\n",__func__);
    return 0;
}

}
