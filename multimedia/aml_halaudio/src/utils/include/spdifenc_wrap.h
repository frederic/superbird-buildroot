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

#ifndef __SPDIFENC_WRAP_H__
#define __SPDIFENC_WRAP_H__

//#ifdef __cplusplus
//extern "C" {
//#endif

int spdifenc_init(struct pcm *mypcm, audio_format_t format);
int spdifenc_write(const void *buffer, size_t numBytes);
uint64_t  spdifenc_get_total(void);

//#ifdef __cplusplus
//}
//#endif

#endif  //__SPDIFENC_WRAP_H__
