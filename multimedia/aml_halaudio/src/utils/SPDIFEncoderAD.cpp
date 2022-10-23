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
#define LOG_TAG "SPDIFEncoderAD"
#include <stdint.h>
#include "log.h"
#include "audio_core.h"
#include <audio_utils/spdif/SPDIFEncoder.h>
#include <tinyalsa/asoundlib.h>
#include "properties.h"
#include "SPDIFEncoderAD.h"
#include "aml_android_utils.h"

extern "C"
{
//#include "audio_hw_utils.h"
}


namespace android
{
class SPDIFEncoderAD : public SPDIFEncoder
{
public:
    SPDIFEncoderAD(audio_format_t format, const void *output, size_t max_output_size)
        : SPDIFEncoder(format)
        , mTotalBytes(0)
        , eac3_frame(0)
        , mformat(format)
        , outBuf(output)
        , outBufSize(max_output_size)
        , outBufCurrentPos(0)
    {
        ALOGI("%s() format %#x outBuf %p outBufSize %zu\n", __FUNCTION__, format, outBuf, outBufSize);
    };
    virtual ssize_t writeOutput(const void* buffer, size_t bytes)
    {
        int ret = -1;
        ALOGV("write size %zu , outBufCurrentPos %zu\n", bytes, outBufCurrentPos);

        // ret = pcm_write(buffer, bytes);
        char *iec61937_buffer = (char *)outBuf + outBufCurrentPos;
        size_t actual_write_size =
            ((outBufSize - outBufCurrentPos) > bytes) ? (bytes) : (outBufSize - outBufCurrentPos);
        if (outBuf && (actual_write_size > 0))
            memcpy((void *)iec61937_buffer, (const void*)buffer, actual_write_size);
        else
            return -1;
        if (actual_write_size > 0) {
            outBufCurrentPos += actual_write_size;
            mTotalBytes += actual_write_size;
            ALOGV("%s() actual_write_size %zu outBufCurrentPos %zu\n", __FUNCTION__, actual_write_size, outBufCurrentPos);
#if 1
            if (aml_getprop_bool("media.audiohal.outdump")) {
                FILE *fp1 = fopen("/data/audio_out/enc_output.spdif", "a+");
                if (fp1) {
                    int flen = fwrite((char *)iec61937_buffer, 1, actual_write_size, fp1);
                    ALOGV("%s iec61937_buffer %p write_size %zu\n", __FUNCTION__, iec61937_buffer, actual_write_size);
                    fclose(fp1);
                } else {
                    //ALOGD("could not open file:/data/hdmi_audio_out.pcm");
                }
            }
#endif
            return actual_write_size;
        }
        else
            return -1;
    }
    /*
     *@brief get current iec61937 data size
     */
    virtual size_t getCurrentIEC61937DataSize(void) {
        return outBufCurrentPos;
    }
    /*
     *@brief flush output iec61937 data current position to zero!
     */
    virtual void flushOutputCurrentPosition() {
        outBufCurrentPos = 0;
    }
    /*
     *@brief get total tytes that through the spdif encoder
     */
    virtual uint64_t total_bytes()
    {
        return mTotalBytes;
    }
protected:

private:
    uint64_t mTotalBytes;
    uint64_t eac3_frame;
    audio_format_t mformat;
    const void *outBuf;
    size_t outBufSize;
    size_t outBufCurrentPos;

};
static SPDIFEncoderAD *spdif_encoder_ad = NULL;
extern "C" int spdif_encoder_ad_init(audio_format_t format, const void *output, int max_output_size)
{
    if (spdif_encoder_ad) {
        delete spdif_encoder_ad;
        spdif_encoder_ad = NULL;
    }
    spdif_encoder_ad = new SPDIFEncoderAD(format, output, max_output_size);
    if (spdif_encoder_ad == NULL) {
        ALOGE("init SPDIFEncoderAD failed \n");
        return  -1;
    }
    ALOGI("init SPDIFEncoderAD done\n");
    return 0;
}
extern "C" int spdif_encoder_ad_write(const void *buffer, size_t numBytes)
{
#if 1
    if (aml_getprop_bool("media.audiohal.outdump")) {
        FILE *fp1 = fopen("/data/audio_out/enc_input.spdif", "a+");
        if (fp1) {
            int flen = fwrite((char *)buffer, 1, numBytes, fp1);
            fclose(fp1);
        }
    }
#endif
    return spdif_encoder_ad->write(buffer, numBytes);
}
extern "C" uint64_t spdif_encoder_ad_get_total()
{
    return spdif_encoder_ad->total_bytes();
}
/*
 *@brief get current iec61937 data size
 */
extern "C" size_t spdif_encoder_ad_get_current_position(void)
{
    return spdif_encoder_ad->getCurrentIEC61937DataSize();
}
/*
 *@brief flush output iec61937 data current position to zero!
 */
extern "C" void spdif_encoder_ad_flush_output_current_position(void)
{
    return spdif_encoder_ad->flushOutputCurrentPosition();
}

extern "C" void spdif_encoder_ad_reset(void)
{
    return spdif_encoder_ad->reset();
}

}
