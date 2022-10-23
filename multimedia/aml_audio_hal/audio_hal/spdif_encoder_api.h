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

#ifndef _SPDIF_ENCODER_API_H_
#define _SPDIF_ENCODER_API_H_

/*
 *@brief get the spdif encoder output buffer
 */
int config_spdif_encoder_output_buffer(audio_format_t format, struct aml_audio_device *adev);

/*
 *@brief release the spdif encoder output buffer
 */
void release_spdif_encoder_output_buffer(struct audio_stream_out *stream);

/*
 *@brief get spdif encoder prepared
 */
int get_the_spdif_encoder_prepared(audio_format_t format, struct aml_stream_out *aml_out);

#endif // _ALSA_MANAGER_H_
