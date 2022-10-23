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
#ifndef _AML_MAT_PARSER_H_
#define _AML_MAT_PARSER_H_

#define IEC61937_MAT_BYTES 61440

/*
 *@brief is_truehd_within_mat
 * input params:
 *          const void *audio_buffer:audio buffer
 *          size_t audio_bytes:buffer bytes
 *
 * return value:
 *          0, truehd is inside
 *         -1, pcm is inside
 */
int is_truehd_within_mat(const void *audio_buffer, size_t audio_bytes);

/*
 *@brief is_truehd_within_mat
 * input params:
 *          int first_papb: first PAPB
 *          int next_papb: second PAPB
 *
 * return value:
 *          0, data is not available
 *         -1, data is available
 */
int is_dolby_mat_available(int first_papb, int next_papb);
#endif