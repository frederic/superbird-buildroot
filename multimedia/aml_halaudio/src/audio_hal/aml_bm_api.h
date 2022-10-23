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

#ifndef _AML_BM_API_H_//bass management
#define _AML_BM_API_H_

#include "audio_hw.h"
#include "aml_audio_stream.h"

#define BM_MIN_CORNER_FREQUENCY     40//HZ
#define BM_MAX_CORNER_FREQUENCY     250//HZ
#define BM_STEP_LEN_CORNER_FREQ     10//HZ
#define BM_CHANNEL_COUNT            8

/*L R C LFE LS RS LTF RTF*/
/*L R C LFE LS RS*/
/*L R C LFE*/
#define LFE_CH_INDEX                3

#define DATMOS_ORIGINAL_LFE                 "/tmp/datmos_original_lfe.data"
#define DATMOS_BASSMANAGEMENT_LFE           "/tmp/datmos_bassmanagement_lfe.data"
/*
 *@brief aml bass management process
 * input params:
 *          struct aml_audio_device *adev : aml_audio_device handle
 * return value:
 *          0, success
            1, occur some error
 */
int aml_bm_init(struct audio_hw_device *dev);

/*
 *@brief aml bm process

 *fix the input as 8ch-32bits format and suppose the LFE is the ch3 of ch0~7

 * input params:
 *          struct audio_stream_out *stream : audio stream out handle
 *          const void *buffer: pcm data buffer address
 *          size_t bytes :pcm data length
 *          aml_data_format_t *data_format
 * return value:
 *          0, success
            1, occur some error
 */
int aml_bm_process(struct audio_stream_out *stream
                    , const void *buffer
                    , size_t bytes
                    , aml_data_format_t *data_format);

int aml_bm_close(struct audio_hw_device *dev);

int aml_bm_setparameters(struct audio_hw_device *dev, struct str_parms *parms);

int aml_bm_set(struct aml_audio_device *adev, int val);

#endif//end of _AML_BM_API_H_