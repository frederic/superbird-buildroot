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

#ifndef _AUDIO_AEC_PROCESS_H_
#define _AUDIO_AEC_PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

int32_t* aec_spk_mic_process(int32_t *spk_buf, int32_t *mic_buf, int *cleaned_samples_per_channel);
int16_t* aec_spk_mic_process_int16(int16_t *spk_buf, int16_t *mic_buf, int *cleaned_samples_per_channel);
int aec_spk_mic_init(int sample_rate_hz, int num_loudspeaker_feeds,
            int num_microphone_channels);
void aec_spk_mic_reset(void);
void aec_spk_mic_release(void);
int aec_set_spk_buf_info(int samples_per_channel, uint64_t timestamp, bool valid_timestamp);
int aec_set_mic_buf_info(int samples_per_channel, uint64_t timestamp, bool valid_timestamp);

#ifdef __cplusplus
}
#endif
#endif
