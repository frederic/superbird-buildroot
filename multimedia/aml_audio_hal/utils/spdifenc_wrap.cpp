/*
 * Copyright (C) 2010 Amlogic Corporation.
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



//#define LOG_NDEBUG 0
#define LOG_TAG "AudioSPDIF-wrap"
#include <stdint.h>
#include <cutils/log.h>
#include <system/audio.h>
#include <audio_utils/spdif/SPDIFEncoder.h>
#include <tinyalsa/asoundlib.h>
#include <cutils/properties.h>
#include <string.h>

extern "C"
{
//#include "audio_hw_utils.h"
}

#define DD_FRAME_SIZE 1536
#define DDP_FRAME_SIZE 6144
// add array of dd/ddp mute frame for mute function
static const unsigned int muted_frame_dd[DD_FRAME_SIZE] = {
 0x4e1ff872, 0x8000001, 0xb49e0b77, 0x43e10840, 0x10806f0, 0x21010808, 0x571f0104, 0xf9f33e7c, 0x9f3ee7cf, 0xfe757cfb, 0xf3e77cf9, 0x3e7ccf9f, 0x9d5ff8ff, 0xf9f33e7c, 0x9f3ee7cf, 0x24432ff,
 0x8920, 0xb5f30001, 0x9b6de7cf, 0x6f1eb6db, 0x3c00, 0x9f3e0daf, 0x6db67cdb, 0xf1e1db78, 0x8000, 0xd7cf0006, 0x6db69f3e, 0xbc78db6d, 0xf000, 0x7cf936be, 0xb6dbf36d, 0xc7866de3,
 0x0, 0x5f3e001b, 0xb6db7cf9, 0xf1e36db6, 0xc000, 0xf3e7daf9, 0xdb6dcdb6, 0x1e18b78f, 0x0, 0x7cf9006d, 0xdb6df3e6, 0xc78fb6db, 0x30000, 0xcf9f6be7, 0x6db636db, 0x7860de3c,
 0x0, 0xf3e701b5, 0x6db6cf9b, 0x1e3cdb6f, 0xd0000, 0x3e7caf9f, 0xb6dbdb6d, 0xe18078f1, 0x0, 0xcf9f06d7, 0xb6db3e6d, 0x78f06dbc, 0x360000, 0xf9f3be7c, 0xdb6d6db6, 0x8000e3c7,
 0x0, 0xe2560000
};

static const unsigned int muted_frame_ddp[DDP_FRAME_SIZE] = {
 0x4e1ff872, 0x3000015, 0x7f01770b, 0x20e06734, 0x2004, 0x8084500, 0x404046c, 0x1010104, 0xe7630001, 0x7c3e9fcf, 0xcfe7f3f9, 0xf97c3e9f, 0x9fcfe7f3, 0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0xce7f9fcf, 0x7c3e9faf,
 0xcfe7f3f9, 0xf97c3e9f, 0x9fcfe7f3, 0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0xf37f9fcf, 0x9fcfe7ab, 0xf3f97c3e, 0x3e9fcfe7, 0xe7f3f97c, 0x7c3e9fcf, 0xcfe7f3f9, 0xf97c3e9f, 0x53dee7f3, 0xf0e9,
 0x6d3c0000, 0xf178dbb6, 0x7777c7e3, 0x70, 0x0, 0x0, 0x0, 0xeeee0e00, 0x6d3c1eef, 0x6b3edfb6, 0xadb5d65a, 0xb5d65a6b, 0xd65a6bad, 0x406badb5, 0x3c000000, 0x78dbb66d,
 0x77c7e3f1, 0x7077, 0x0, 0x0, 0x0, 0xee0e0000, 0x3c1eefee, 0x3edfb66d, 0xb5d65a6b, 0x20606bad, 0x0, 0xdbb66d3c, 0xc7e3f178, 0x707777, 0x0, 0x0,
 0x0, 0xe000000, 0x1eefeeee, 0xdfb66d3c, 0xd65a6b3e, 0x5a6badb5, 0x6badb5d6, 0xadb5d65a, 0x406b, 0xb66d3c00, 0xe3f178db, 0x707777c7, 0x0, 0x0, 0x0, 0x0,
 0xefeeee0e, 0xb66d3c1e, 0x5a6b3edf, 0x6badb5d6, 0x2060, 0x6d3c0000, 0xf178dbb6, 0x7777c7e3, 0x70, 0x0, 0x0, 0x0, 0xeeee0e00, 0x6d3c1eef, 0x6b3edfb6, 0xadb5d65a,
 0xb5d65a6b, 0xd65a6bad, 0x406badb5, 0x3c000000, 0x78dbb66d, 0x77c7e3f1, 0x7077, 0x0, 0x0, 0x0, 0xee0e0000, 0x3c1eefee, 0x3edfb66d, 0xb5d65a6b, 0x20606bad, 0x0,
 0xdbb66d3c, 0xc7e3f178, 0x707777, 0x0, 0x0, 0x0, 0xe000000, 0x1eefeeee, 0xdfb66d3c, 0xd65a6b3e, 0x5a6badb5, 0x6badb5d6, 0xadb5d65a, 0x406b, 0xb66d3c00, 0xe3f178db,
 0x707777c7, 0x0, 0x0, 0x0, 0x0, 0xefeeee0e, 0xb66d3c1e, 0x5a6b3edf, 0x6badb5d6, 0x2060, 0x6d3c0000, 0xf178dbb6, 0x7777c7e3, 0x70, 0x0, 0x0,
 0x0, 0xeeee0e00, 0x6d3c1eef, 0x6b3edfb6, 0xadb5d65a, 0xb5d65a6b, 0xd65a6bad, 0x406badb5, 0x3c000000, 0x78dbb66d, 0x77c7e3f1, 0x7077, 0x0, 0x0, 0x0, 0xee0e0000,
 0x3c1eefee, 0x3edfb66d, 0xb5d65a6b, 0x20606bad, 0x0, 0xdbb66d3c, 0xc7e3f178, 0x707777, 0x0, 0x0, 0x0, 0xe000000, 0x1eefeeee, 0xdfb66d3c, 0xd65a6b3e, 0x5a6badb5,
 0x6badb5d6, 0xadb5d65a, 0x406b, 0xb66d3c00, 0xe3f178db, 0x707777c7, 0x0, 0x0, 0x0, 0x0, 0xefeeee0e, 0xb66d3c1e, 0x5a6b3edf, 0x6badb5d6, 0x40, 0x7f227c55
};

static int getprop_bool(const char * path)
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;
    ret = property_get(path, buf, NULL);
    if (ret > 0) {
        if (strcasecmp(buf,"true") == 0 || strcmp(buf,"1") == 0) {
            return 1;
        }
    }
    return 0;
}

static int mute_dd_frame(void *buf, size_t bytes)
{
    if (bytes == (DD_FRAME_SIZE * 4))
        memcpy(buf, muted_frame_dd, bytes);
    else
        memset(buf, 0, bytes);
    return 0;
}

static int mute_ddp_frame(void *buf, size_t bytes)
{
    if (bytes == (DDP_FRAME_SIZE * 4))
        memcpy(buf, muted_frame_ddp, bytes);
    else
        memset(buf, 0, bytes);
    return 0;
}

namespace android
{
class MySPDIFEncoder : public SPDIFEncoder
{
public:
    MySPDIFEncoder(struct pcm *mypcm, audio_format_t format)
        : SPDIFEncoder(format),
          pcm_handle(mypcm), mTotalBytes(0),
          mMute(false),// eac3_frame(0),
          mFormat(format),
          mFirstFrameMuted(false)
    {};
    virtual ssize_t writeOutput(const void* buffer, size_t bytes)
    {
        int ret = -1;
        char *buf = (char *)buffer;
        ALOGV("write size %zu \n", bytes);
#if 1
        if (getprop_bool("media.audiohal.outdump")) {
            FILE *fp1 = fopen("/data/audio_out/hdmi_audio_out.spdif", "a+");
            if (fp1) {
                fwrite((char *)buffer, 1, bytes, fp1);
                //ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
                fclose(fp1);
            } else {
                //ALOGD("could not open file:/data/hdmi_audio_out.pcm");
            }
        }
#endif
        mTotalBytes += bytes;
        if (mMute) {
            muteFrame(buf, bytes);
        }
        if (mFirstFrameMuted == false) {
            memset(buf, 0, bytes);
            mFirstFrameMuted = true;
        }
        ret = pcm_write(pcm_handle, buffer, bytes);
        if (ret)
            return ret;

        return bytes;
    }
    virtual uint64_t total_bytes()
    {
        return mTotalBytes;
    }
    int setMute(bool mute)
    {
        ALOGD("%s(), mMute = %d \n", __func__, mute);
        return mMute = mute;
    }
    bool getMute(void)
    {
        return mMute;
    }
    int muteFrame(void *buf, size_t bytes)
    {
        if (mFormat == AUDIO_FORMAT_E_AC3) {
            mute_ddp_frame(buf, bytes);
        } else if (mFormat == AUDIO_FORMAT_AC3) {
            mute_dd_frame(buf, bytes);
        } else {
            // add new if needed
            memset(buf, 0, bytes);
        }
        return 0;
    }
protected:
    struct pcm *pcm_handle;
private:
    uint64_t mTotalBytes;
    bool mMute;
//    uint64_t eac3_frame;
    audio_format_t mFormat;
    bool mFirstFrameMuted;
};
static MySPDIFEncoder *myencoder = NULL;
extern "C" int spdifenc_init(struct pcm *mypcm, audio_format_t format)
{
    if (myencoder) {
        delete myencoder;
        myencoder = NULL;
    }
    myencoder = new MySPDIFEncoder(mypcm, format);
    if (myencoder == NULL) {
        ALOGE("init SPDIFEncoder failed \n");
        return  -1;
    }
    ALOGI("init SPDIFEncoder done\n");
    return 0;
}
extern "C" int  spdifenc_write(const void *buffer, size_t numBytes)
{
    return myencoder->write(buffer, numBytes);
}
extern "C" uint64_t  spdifenc_get_total()
{
    return myencoder->total_bytes();
}
extern "C" int spdifenc_set_mute(bool mute)
{
    if (myencoder)
        return myencoder->setMute(mute);
    else
        return -1;
}
}
