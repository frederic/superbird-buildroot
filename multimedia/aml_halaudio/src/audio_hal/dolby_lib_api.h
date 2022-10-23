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

#ifndef _DOLBY_LIB_API_H_
#define _DOLBY_LIB_API_H_


/** Dolby Lib Type used in Current System */
typedef enum eDolbyLibType {
    eDolbyNull  = 0,
    eDolbyDcvLib  = 1,
    eDolbyMS12Lib = 2,
    eDolbyAtmosLib = 3,
} eDolbyLibType_t;


enum eDolbyLibType detect_dolby_lib_type(void);


#endif //_DOLBY_LIB_API_H_
