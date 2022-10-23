/*
 * Copyright (C) 2018 Amlogic Corporation.
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

#ifndef _AUDIO_EQ_DRC_COMPENSATION_H_
#define _AUDIO_EQ_DRC_COMPENSATION_H_

#include "audio_eq_drc_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

int eq_mode_set(struct eq_drc_data *pdata, int eq_mode);
int eq_drc_init(struct eq_drc_data *pdata);
int eq_drc_release(struct eq_drc_data *pdata);

#ifdef __cplusplus
}
#endif
#endif
