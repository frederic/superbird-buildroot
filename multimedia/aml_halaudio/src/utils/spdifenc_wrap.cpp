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
#include <strings.h>
#include <stdint.h>
#include "log.h"
#include <system/audio.h>
#include <audio_utils/spdif/SPDIFEncoder.h>
#include <tinyalsa/asoundlib.h>
#include "properties.h"

#include <cstring>



extern "C"
{
//#include "audio_hw_utils.h"
}

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

namespace android
{
class MySPDIFEncoder : public SPDIFEncoder
{
public:
    MySPDIFEncoder(struct pcm *mypcm, audio_format_t format)
        : SPDIFEncoder(format),
          pcm_handle(mypcm), mTotalBytes(0), eac3_frame(0), mformat(format)
    {};
    virtual ssize_t writeOutput(const void* buffer, size_t bytes)
    {
        int ret = -1;
        ALOGV("write size %zu \n", bytes);
#if 1
        if (getprop_bool("media.audiohal.outdump")) {
            FILE *fp1 = fopen("/data/audio_out/hdmi_audio_out.spdif", "a+");
            if (fp1) {
                int flen = fwrite((char *)buffer, 1, bytes, fp1);
                //ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
                fclose(fp1);
            } else {
                //ALOGD("could not open file:/data/hdmi_audio_out.pcm");
            }
        }
#endif
        mTotalBytes += bytes;
        ret = pcm_write(pcm_handle, buffer, bytes);
        if (ret)
            return ret;

        return bytes;
    }
    virtual uint64_t total_bytes()
    {
        return mTotalBytes;
    }
protected:
    struct pcm *pcm_handle;
private:
    uint64_t mTotalBytes;
    uint64_t eac3_frame;
    audio_format_t mformat;
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
}
