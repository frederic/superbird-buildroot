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


#ifndef _AUDIO_DATA_PROCESS_H_
#define _AUDIO_DATA_PROCESS_H_

#include "sub_mixing_factory.h"
int processing_and_convert(void *data_mixed,
        void *data_sys, size_t frames, struct audioCfg inCfg, struct audioCfg mixerCfg);
int do_mixing_2ch(void *data_mixed,
        void *data_in, size_t frames,
        struct audioCfg inCfg, struct audioCfg mixerCfg);
int extend_channel_2_8(void *data_out, void *data_in,
        size_t frames, int ch_cnt_out, int ch_cnt_in);
int extend_channel_5_8(void *data_out, void *data_in,
        size_t frames, int ch_cnt_out, int ch_cnt_in);

#endif
