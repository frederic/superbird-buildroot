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

#ifndef __AML_DUMP_DEBUG_H__
#define __AML_DUMP_DEBUG_H__

#define CC_DUMP_SRC_TYPE_INPUT         (0)
#define CC_DUMP_SRC_TYPE_OUTPUT        (1)
#define CC_DUMP_SRC_TYPE_INPUT_PARSE   (2)

void DoDumpData(const void *data_buf, int size, int aud_src_type);

#endif

