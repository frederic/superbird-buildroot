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


#ifndef _HW_AVSYNC_CALLBACKS_H
#define _HW_AVSYNC_CALLBACKS_H
#include "hw_avsync.h"

//int on_notify_cbk(void *data);
//int on_input_avail_cbk(void *data);
int on_meta_data_cbk(void *cookie,
            uint64_t offset,
            struct hw_avsync_header *header,
            int *delay_ms);
enum hwsync_status pcm_check_hwsync_status(unsigned int apts_gap);
#endif
