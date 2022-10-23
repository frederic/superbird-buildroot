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

#ifndef _DOLBY_MS12_STATUS_C_H_
#define _DOLBY_MS12_STATUS_C_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MIXER_PLAYBACK_START 1
#define MIXER_PLAYBACK_STOP 0
#define DIRECT_OR_OFFLOAD_PLAYBACK_START 1
#define DIRECT_0R_OFFLOAD_PLAYBACK_STOP 0


void set_mixer_playback_status(int flags);
int get_mixer_playback_status(void);
void set_direct_or_offload_playback_status(int flags);
int get_direct_or_offload_playback_status(void);
void set_mixer_playback_audio_stream_out_params(audio_format_t format
    , audio_channel_mask_t channel_mask
    , uint32_t sample_rate
    , audio_devices_t devices);
void get_mixer_playback_audio_stream_out_params(audio_format_t *format
    , audio_channel_mask_t *channel_mask
    , uint32_t *sample_rate
    , audio_devices_t *devices);
void set_direct_or_offload_playback_audio_stream_out_params(audio_format_t format
    , audio_channel_mask_t channel_mask
    , uint32_t sample_rate
    , audio_devices_t devices);
void get_direct_or_offload_playback_audio_stream_out_params(audio_format_t *format
    , audio_channel_mask_t *channel_mask
    , uint32_t *sample_rate
    , audio_devices_t *devices);
void set_offload_playback_dolby_ms12_output_format(audio_format_t format);
audio_format_t get_offload_playback_dolby_ms12_output_format(void);
// void set_current_force_use(audio_policy_forced_cfg_t force_use);
// audio_policy_forced_cfg_t get_current_force_use(void);
void set_offload_playback_audio_stream_out_format(audio_format_t format);
audio_format_t get_offload_playback_audio_stream_out_format(void);


/*
 *@brief self cleanup dolby ms12 status
 */
void dolby_ms12_status_self_cleanup(void);
/*
 *@brief set Audio Hal main format
 */
void set_audio_main_format(audio_format_t format);

/*
 *@brief get Audio Hal main format
 */
audio_format_t get_audio_main_format(void);

/*
 *@brief set Audio Hal associate format
 */
void set_audio_associate_format(audio_format_t format);

/*
 *@brief get Audio Hal associate format
 */
audio_format_t get_audio_associate_format(void);

/*
 *@brief set Audio Hal system format
 */
void set_audio_system_format(audio_format_t format);

/*
 *@brief get Audio Hal system format
 */
audio_format_t get_audio_system_format(void);

/*
 *@brief set TV audio main format
 */
void set_dd_support_flag(bool flag);

/*
 *@brief get TV audio main format
 */
bool get_dd_support_flag(void);
/*
 *@brief set ddp support flag
 */
void set_ddp_support_flag(bool flag);

/*
 *@brief get ddp support flag
 */
bool get_ddp_support_flag(void);

/*
 *@brief set dd max audio channel mask
 */
void set_dd_max_audio_channel_mask(audio_channel_mask_t channel_mask);

/*
 *@brief get dd max audio channel mask
 */
audio_channel_mask_t get_dd_max_audio_channel_mask(void);

/*
 *@brief set ddp max channel mask
 */
void set_ddp_max_audio_channel_mask(audio_channel_mask_t channel_mask);

/*
 *@brief get ddp max channel mask
 */
audio_channel_mask_t get_ddp_max_audio_channel_mask(void);

/*
 *@brief set the Need Output Max Format according to User Setting
 */
void set_max_format_by_user_setting(audio_format_t format);

/*
 *@brief get the Need Output Max Format according to User Setting
 */
audio_format_t get_max_format_by_user_setting(void);


#ifdef __cplusplus
}
#endif

#endif //end of _DOLBY_MS12_STATUS_C_H_
