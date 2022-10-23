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

#ifndef _AUDIO_HW_MS12_H_
#define _AUDIO_HW_MS12_H_

//#ifdef DOLBY_MS12_ENABLE
#include <tinyalsa/asoundlib.h>
#include "audio_core.h"
#include <stdbool.h>
#include <aml_audio_ms12.h>

#include "audio_hw.h"

/*
 *@brief get dolby ms12 prepared
 */
int get_the_dolby_ms12_prepared(
    struct aml_stream_out *aml_out
    , audio_format_t input_format
    , audio_channel_mask_t input_channel_mask
    , int input_sample_rate);

/*
 *@brief get the data of direct thread
 *
 * input parameters
 *     stream: audio_stream_out handle
 *     buffer: data buffer address
 *     bytes: data size
 * output parameters
 *     used_size: buffer used size
 */
int dolby_ms12_main_process(
    struct audio_stream_out *stream
    , const void *buffer
    , size_t bytes
    , size_t *used_size);


/*
 *@brief dolby ms12 system process
 *
 * input parameters
 *     stream: audio_stream_out handle
 *     buffer: data buffer address
 *     bytes: data size
 * output parameters
 *     used_size: buffer used size
 */
int dolby_ms12_system_process(
    struct audio_stream_out *stream
    , const void *buffer
    , size_t bytes
    , size_t *used_size);

/*
 *@brief get dolby ms12 cleanup
 */
int get_dolby_ms12_cleanup(struct dolby_ms12_desc *ms12);

/*
 *@brief set dolby ms12 primary gain
 */
int set_dolby_ms12_primary_input_db_gain(struct dolby_ms12_desc *ms12,
        int db_gain);

/*
 *@brief set system app mixing status
 * if normal pcm stream status is STANDBY, set mixing off(-xs 0)
 * if normal pcm stream status is active, set mixing on(-xs 1)
 */
int set_system_app_mixing_status(struct aml_stream_out *aml_out, int stream_status);

/*
 *@brief an callback for dolby ms12 pcm output
 */
int pcm_output(void *buffer, void *priv_data, size_t size);

/*
 *@brief an callback for dolby ms12 bitstream output
 */
int bitstream_output(void *buffer, void *priv_data, size_t size);

/*
 *@brief dolby ms12 register the callback
 */
int dolby_ms12_register_callback(struct aml_stream_out *aml_out);
//#endif //end of DOLBY_MS12_ENABLE

#endif //end of _AUDIO_HW_MS12_H_