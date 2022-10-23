/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  2.0
 *  @date     2014/11/04
 *  @par function description:
 *  - 1 common define for system control both Android and recovery mode
 */

#ifndef _SYS_COMMON_H
#define _SYS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define CC_MAX_LINE_LEN     512
#define MAX_STR_LEN         4096
#define MODE_LEN            64

#ifdef RECOVERY_MODE
#include <cutils/klog.h>
#define SYS_LOGE(x...)      KLOG_ERROR("systemcontrol", x)
#define SYS_LOGD(x...)      KLOG_DEBUG("systemcontrol", x)
#define SYS_LOGV(x...)      KLOG_NOTICE("systemcontrol", x)
#define SYS_LOGI(x...)      KLOG_INFO("systemcontrol", x)
#else
#include <utils/Log.h>
#define SYS_LOGE            ALOGE
#define SYS_LOGD            ALOGD
#define SYS_LOGV            ALOGV
#define SYS_LOGI            ALOGI
#endif

enum {
    LOG_LEVEL_0             = 0,
    LOG_LEVEL_1             = 1,//debug property
    LOG_LEVEL_2             = 2,//debug sysfs read write, parse config file
    LOG_LEVEL_TOTAL         = 3
};

#define LOG_LEVEL_DEFAULT   LOG_LEVEL_1

#ifdef __cplusplus
}
#endif
#endif // _SYS_COMMON_H
