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

#ifndef _AML_AC3_PARSER_H_
#define _AML_AC3_PARSER_H_

#define BS_STD           8
#define ISDD(bsid)       ((bsid) <= BS_STD)
#define BS_AXE           16
#define ISDDP(bsid)      ((bsid) <= BS_AXE && (bsid) > 10)

//#define AC3_PERIOD_SIZE  0x1800 //#include "audio_format_parse.h"
//#define EAC3_PERIOD_SIZE 0x6000 //#include "audio_format_parse.h"

#define IEC61937_HEADER_SIZE 8

/*
 *@brief scan_dolby_main_associate_frame
 * input params:
 *          void *input_buffer: input data address
 *          size_t bytes: input data size
 *          int *used_size: data used size in this sequence
 * output params:
 *          void **main_frame_buffer: main frame address
 *          int *main_frame_size: main frame size
 *          void **associate_frame_buffer: associate frame address
 *          int *associate_frame_size: associate frame address
 *
 * return value:
 *          0, success to find one packet(as IEC61937 packet format, contained main / main&associate)
 *          1, fail to find one packet(as IEC61937 packet format)
 */
int scan_dolby_main_associate_frame(void *input_buffer
                                    , size_t bytes
                                    , int *used_size
                                    , void **main_frame_buffer
                                    , int *main_frame_size
                                    , void **associate_frame_buffer
                                    , int *associate_frame_size);

int scan_dolby_main_frame(void *input_buffer
                          , size_t bytes
                          , int *used_size
                          , void **main_frame_buffer
                          , int *main_frame_size);


/*
 *@brief scan_dolby_main_associate_frame
 * input params:
 *          void *input_buffer: input data address
 *          size_t bytes: input data size
 *          int *used_size: data used size in this sequence
 * output params:
 *          void **main_frame_buffer: main frame address
 *          int *main_frame_size: main frame size
 *
 * return value:
 *          0, success to find one packet(as IEC61937 packet format
 *          1, fail to find one packet(as IEC61937 packet format)
 */
int scan_dolby_main_frame_ext(void *input_buffer
                              , size_t bytes
                              , int *used_size
                              , void **main_frame_buffer
                              , int *main_frame_size
                              , size_t *payload_deficiency);

#endif

