/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include <stdlib.h>
#include "log.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <tinyalsa/asoundlib.h>

#include "SPDIFEncoderAD.h"
#include "audio_hw.h"

#define IEC61937_PACKET_SIZE_OF_AC3     0x1800
#define IEC61937_PACKET_SIZE_OF_EAC3    0x6000
/*
 *@brief get the spdif encoder output buffer
 */
int config_spdif_encoder_output_buffer(audio_format_t format, struct aml_audio_device *adev)
{
    /*
     *audiofinger write 4096bytes to hal
     *ac3 min frame size is 64bytes, so ac3 coefficient is 4096/64 = 64
     *eac3 min frame size is 128bytes, so ac3 coefficient is 4096/128 = 32
     */
    int ac3_coef = 64;
    int eac3_coef = 32;
    if (format == AUDIO_FORMAT_AC3)
        adev->temp_buf_size = IEC61937_PACKET_SIZE_OF_AC3 * ac3_coef;//0x1800bytes is 6block of ac3
    else
        adev->temp_buf_size = IEC61937_PACKET_SIZE_OF_EAC3 * eac3_coef;//0x6000bytes is 6block of eac3
    adev->temp_buf_pos = 0;
    adev->temp_buf = (void *)malloc(adev->temp_buf_size);
    ALOGV("-%s() temp_buf_size %x\n", __FUNCTION__, adev->temp_buf_size);
    if (adev->temp_buf == NULL) {
        ALOGE("-%s() malloc fail", __FUNCTION__);
        return -1;
    }
    memset(adev->temp_buf, 0, adev->temp_buf_size);

    return 0;
}

/*
 *@brief release the spdif encoder output buffer
 */
void release_spdif_encoder_output_buffer(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;

    if (adev->temp_buf) {
        free(adev->temp_buf);
        adev->temp_buf = NULL;
    }
    adev->temp_buf_size = 0;
    adev->temp_buf_pos = 0;
    return ;
}

/*
 *@brief get spdif encoder prepared
 */
int get_the_spdif_encoder_prepared(audio_format_t format, struct aml_stream_out *aml_out)
{
    ALOGI("+%s() format %#x\n", __FUNCTION__, format);
    int ret = -1;
    struct aml_audio_device *adev = aml_out->dev;

    if ((format == AUDIO_FORMAT_AC3) || (format == AUDIO_FORMAT_E_AC3)) {
        ret = config_spdif_encoder_output_buffer(format, adev);
        if (ret != 0) {
            ALOGE("-%s() config_spdif_encoder_output_buffer fail", __FUNCTION__);
            return ret;
        }
        ret = spdif_encoder_ad_init(
                    format
                    , (const void *)adev->temp_buf
                    , adev->temp_buf_size);
        if (ret != 0) {
            ALOGE("-%s() spdif_encoder_ad_init fail", __FUNCTION__);
            return ret;
        }
        aml_out->spdif_enc_init_frame_write_sum = aml_out->frame_write_sum;
    }
    ALOGI("-%s() ret %d\n", __FUNCTION__, ret);
    return ret;
}
