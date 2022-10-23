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
#include <stdlib.h>
#include <errno.h>
#include <cutils/log.h>
#include "google_aec.h"
#include "audio_aec_process.h"

#undef  LOG_TAG
#define LOG_TAG  "audio_hw_primary"

static audio_ears::GoogleAec *pGoogleAec;
static audio_ears::GoogleAec::AudioBufferInfo *p_spk_buf_info;
static audio_ears::GoogleAec::AudioBufferInfo *p_mic_buf_info;

int32_t* aec_spk_mic_process(int32_t *spk_buf, int32_t *mic_buf, int *cleaned_samples_per_channel)
{
    const int32_t *spk_samples = spk_buf;
    const int32_t *mic_samples = mic_buf;
    const audio_ears::GoogleAec::AudioBufferInfo &spk_buf_info = *p_spk_buf_info;
    const audio_ears::GoogleAec::AudioBufferInfo &mic_buf_info = *p_mic_buf_info;
    const int32_t *out_buf;

    out_buf = pGoogleAec->ProcessInt32InterleavedAudio(spk_samples, spk_buf_info,
        mic_samples, mic_buf_info, cleaned_samples_per_channel);
    if (!out_buf) {
        //ALOGE("%s: AEC process failed, cleaned_samples_per_channel = %d", __func__, *cleaned_samples_per_channel);
        //pGoogleAec->Reset();
        return NULL;
    }
    return (int32_t*)out_buf;
}
int16_t* aec_spk_mic_process_int16(int16_t *spk_buf, int16_t *mic_buf, int *cleaned_samples_per_channel)
{
    const int16_t *spk_samples = spk_buf;
    const int16_t *mic_samples = mic_buf;
    const audio_ears::GoogleAec::AudioBufferInfo &spk_buf_info = *p_spk_buf_info;
    const audio_ears::GoogleAec::AudioBufferInfo &mic_buf_info = *p_mic_buf_info;
    const int16_t *out_buf;

    out_buf = pGoogleAec->ProcessInt16InterleavedAudio(spk_samples, spk_buf_info,
        mic_samples, mic_buf_info, cleaned_samples_per_channel);
    if (!out_buf) {
        //ALOGE("%s: AEC process failed, cleaned_samples_per_channel = %d", __func__, *cleaned_samples_per_channel);
        //pGoogleAec->Reset();
        return NULL;
    }
    return (int16_t*)out_buf;
}

int aec_spk_mic_init(int sample_rate_hz, int num_loudspeaker_feeds,
            int num_microphone_channels)
{
    ALOGD("%s: enter", __func__);
    if (!pGoogleAec) {
        pGoogleAec = new audio_ears::GoogleAec(sample_rate_hz,
            num_loudspeaker_feeds, num_microphone_channels, "GoogleAecMode3", false);
        if (!pGoogleAec) {
            ALOGE("%s: alloc GoogleAec failed", __func__);
            return -ENOMEM;
        }
    }
    pGoogleAec->Reset();
    ALOGD("%s: exit", __func__);

    return 0;
}

void aec_spk_mic_reset(void)
{
    //ALOGD("%s: enter", __func__);
    if (pGoogleAec) {
        pGoogleAec->Reset();
    }
    //ALOGD("%s: exit", __func__);
}

void aec_spk_mic_release(void)
{
    ALOGD("%s: enter", __func__);
    delete p_spk_buf_info;
    p_spk_buf_info = NULL;
    delete p_mic_buf_info;
    p_mic_buf_info = NULL;
    //ALOGD("%s: exit", __func__);
}

int aec_set_spk_buf_info(int samples_per_channel, uint64_t timestamp, bool valid_timestamp)
{
    if (!p_spk_buf_info) {
        p_spk_buf_info = new audio_ears::GoogleAec::AudioBufferInfo(samples_per_channel);
        if (!p_spk_buf_info) {
            ALOGE("%s: alloc p_spk_buf_info failed", __func__);
            return -ENOMEM;
        }
    }
    valid_timestamp = false;
    p_spk_buf_info->samples_per_channel = samples_per_channel;
    p_spk_buf_info->timestamp_microseconds = timestamp;
    p_spk_buf_info->valid_timestamp = valid_timestamp;

    return 0;
}

int aec_set_mic_buf_info(int samples_per_channel, uint64_t timestamp, bool valid_timestamp)
{
    if (!p_mic_buf_info) {
        p_mic_buf_info = new audio_ears::GoogleAec::AudioBufferInfo(samples_per_channel);
        if (!p_mic_buf_info) {
            ALOGE("%s: alloc p_mic_buf_info failed", __func__);
            return -ENOMEM;
        }
    }
    valid_timestamp = false;
    p_mic_buf_info->samples_per_channel = samples_per_channel;
    p_mic_buf_info->timestamp_microseconds = timestamp;
    p_mic_buf_info->valid_timestamp = valid_timestamp;

    return 0;
}
