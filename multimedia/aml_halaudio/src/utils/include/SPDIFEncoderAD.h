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

#ifndef __SPDIFENCODER_AD_H__
#define __SPDIFENCODER_AD_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 *@brief init the spdif encoder advanced
 */
int spdif_encoder_ad_init(audio_format_t format, const void *output, int max_output_size);

/*
 *@brief send the data to spdif encoder advaned
 */
int spdif_encoder_ad_write(const void *buffer, size_t numBytes);

/*
 *@brief get total iec61937 data size
 */
uint64_t spdif_encoder_ad_get_total();

/*
 *@brief get current iec61937 data size
 */
size_t spdif_encoder_ad_get_current_position(void);

/*
 *@brief flush output iec61937 data current position to zero!
 */
void spdif_encoder_ad_flush_output_current_position(void);

#ifdef __cplusplus
}
#endif

#endif  //__SPDIFENCODER_AD_H__
