
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

#ifndef __INI_CONFIG_H__
#define __INI_CONFIG_H__

#if MODE_COMPILE_IN_PC
    #define CS_DEFAULT_PANEL_INI_FILE_NAME             "input/panel/ID_0_Pnl_FACTORY.ini"
    #define CS_DEFAULT_PANEL_PQ_DB_FILE_NAME           "input/pq/pq_default.bin"
    #define CS_DEFAULT_AUDIO_PARAM_INI_FILE_NAME       "input/audio/eq_drc/aml_internal_hw_eq.ini"
#else
    #define CS_DEFAULT_PANEL_INI_FILE_NAME             "/system/etc/ID_0_Pnl_FACTORY.ini"
    #define CS_DEFAULT_PANEL_PQ_DB_FILE_NAME           "/system/etc/pq_default.bin"
    #define CS_DEFAULT_AUDIO_PARAM_INI_FILE_NAME       "/system/etc/ID_0_Aud_eq_drc_FACTORY.ini"
#endif

#define CC_RW_KEY_USE_OTHER_MODULE    (0)

#if MODE_COMPILE_IN_PC
    #define CC_RW_KEY_USE_OTHER_MODULE    (0)
    #define CC_UBOOT_RW_SIMULATE
#endif

#endif //__INI_CONFIG_H__
