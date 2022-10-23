/*
 * Copyright (C) 2014 The Android Open Source Project
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
 * @author   Hugo Hong
 *  @version  1.0
 *  @date     2018/04/01
 *  @par function description:
 *  - 1 bluetooth rc audio hal
 */


/*******************************************************************************
 *
 * Audio device stubs
 *
 ******************************************************************************/
#ifndef AUDIO_HAL_RC_THUNKS_H
#define AUDIO_HAL_RC_THUNKS_H

#include "../audio_hal/audio_hw.h"

/*****************************************************************************
**  external function declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

int rc_open_input_stream(struct aml_stream_in **stream,
                        struct audio_config *config);

void rc_close_input_stream(struct aml_stream_in *stream);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //AUDIO_HAL_RC_THUNKS_H
