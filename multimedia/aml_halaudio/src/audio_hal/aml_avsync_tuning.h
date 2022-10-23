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

#ifndef _AML_AVSYNC_TUNING_H_
#define _AML_AVSYNC_TUNING_H_

/* FIXME: add more SAMPLERATE and CHANNEL COUNT support */
#define SAMPLE_RATE_MS (48)
#define CHANNEL_CNT (2)
/* 16 bits */
#define FRAME_SIZE (2)

struct aml_audio_patch;
struct aml_audio_device;

int aml_dev_try_avsync(struct aml_audio_patch *patch);
int tuning_spker_latency(struct aml_audio_device *adev,
                         int16_t *sink_buffer, int16_t *src_buffer, size_t bytes);

#endif /*_AML_AVSYNC_TUNING_H_ */
