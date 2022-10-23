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

#ifndef _AC3_PARSER_UTILS_H_
#define _AC3_PARSER_UTILS_H_

#define BS_STD           8
#define ISDD(bsid)       ((bsid) <= BS_STD)
#define BS_AXE           16
#define ISDDP(bsid)      ((bsid) <= BS_AXE && (bsid) > 10)


/*
 *parse ac3/eac3 frame header[ATSC Standard,Digital Audio Compression (AC-3, E-AC-3)]
 * input params:
 *          frameBuf: input data address
 *          length: data length
 *          frame_offset: frame begin offset
 *          frame_size: frame size
 *          chanenel_num: frame channel num
 *          numblks: numblks of one frame
 *          timeslice_61937: time slice of iec61937
 *          framevalid_flag: frame valid flag
 * return value:
 *          0, success find one frame
 *          1, fail to find one frame
 */
int parse_dolby_frame_header
    (const unsigned char *frameBuf
    , int length
    , int *frame_offset
    , int *frame_size
    , int *channel_num
    , int *numblks
    , int *timeslice_61937
    , int *framevalid_flag);


#endif //end of _AC3_PARSER_UTILS_H_

