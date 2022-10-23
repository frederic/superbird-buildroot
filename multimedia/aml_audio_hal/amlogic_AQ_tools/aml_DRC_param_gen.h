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

#ifndef _AML_DRC_PARAM_GEN_H_
#define _AML_DRC_PARAM_GEN_H_

#ifdef __cplusplus
extern "C"  {
#endif
int setcsfilter_drc(unsigned int band_id,unsigned int fc);
int setmb_drc(unsigned int band_id,unsigned int attrack_time,unsigned int release_time,unsigned int estimate_time,float K,float threshold);
int setfb_drc(unsigned int band_id,unsigned int attrack_time,unsigned int release_time,unsigned int estimate_time,float K,float threshold,unsigned int delays);
#ifdef __cplusplus
}
#endif

#endif
