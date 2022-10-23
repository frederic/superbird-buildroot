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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <unistd.h>
#include <cutils/log.h>
#include <sys/prctl.h>
#include <cutils/properties.h>
#include <dlfcn.h>

#include "dolby_lib_api.h"

#ifndef RET_OK
#define RET_OK 0
#endif

#ifndef RET_FAIL
#define RET_FAIL -1
#endif


#define DOLBY_MS12_LIB_PATH_A "/vendor/lib/libdolbyms12.so"
#define DOLBY_MS12_LIB_PATH_B "/system/vendor/lib/libdolbyms12.so"

#define DOLBY_DCV_LIB_PATH_A "/vendor/lib/libHwAudio_dcvdec.so"


/*
 *@brief file_accessible
 */
static int file_accessible(char *path)
{
    // file is readable or not
    if (access(path, R_OK) == 0) {
        return RET_OK;
    } else {
        return RET_FAIL;
    }
}


/*
 *@brief detect_dolby_lib_type
 */
enum eDolbyLibType detect_dolby_lib_type(void) {
    enum eDolbyLibType retVal;

    void *hDolbyMS12LibHanle = NULL;
    void *hDolbyDcvLibHanle = NULL;

    hDolbyMS12LibHanle = dlopen(DOLBY_MS12_LIB_PATH_A, RTLD_NOW);
    if (!hDolbyMS12LibHanle)
    {
        hDolbyMS12LibHanle = dlopen(DOLBY_MS12_LIB_PATH_B, RTLD_NOW);
        if (!hDolbyMS12LibHanle) {
            ALOGI("%s, failed to load libdolbyms12 lib %s\n", __FUNCTION__, dlerror());
        }
    }

    // the priority would be "MS12 > DCV" lib
    if (RET_OK == file_accessible(DOLBY_MS12_LIB_PATH_A))
    {
        retVal = eDolbyMS12Lib;
    } else if (RET_OK == file_accessible(DOLBY_MS12_LIB_PATH_B))
    {
        retVal = eDolbyMS12Lib;
    } else if (RET_OK == file_accessible(DOLBY_DCV_LIB_PATH_A))
    {
        retVal = eDolbyDcvLib;
    } else {
        retVal = eDolbyNull;
    }

    // MS12 is first priority
    if (eDolbyMS12Lib == retVal)
    {
        //try to open lib see if it's OK?
        hDolbyMS12LibHanle = dlopen(DOLBY_MS12_LIB_PATH_A, RTLD_NOW);
        if (!hDolbyMS12LibHanle) {
            ALOGI("%s, failed to load libdolbyms12 lib from %s (%s)\n",
                __FUNCTION__, DOLBY_MS12_LIB_PATH_A, dlerror());

            hDolbyMS12LibHanle = dlopen(DOLBY_MS12_LIB_PATH_B, RTLD_NOW);
            if (!hDolbyMS12LibHanle) {
                ALOGI("%s, failed to load libdolbyms12 lib from %s (%s)\n",
                    __FUNCTION__, DOLBY_MS12_LIB_PATH_B, dlerror());
            }
        }
    }
    if (hDolbyMS12LibHanle != NULL)
    {
        dlclose(hDolbyMS12LibHanle);
        hDolbyMS12LibHanle = NULL;
        ALOGI("%s,FOUND libdolbyms12 lib %s\n", __FUNCTION__, dlerror());
        return eDolbyMS12Lib;
    }

    // dcv is second priority
    if (eDolbyDcvLib == retVal)
    {
        //try to open lib see if it's OK?
        hDolbyDcvLibHanle  = dlopen(DOLBY_DCV_LIB_PATH_A, RTLD_NOW);
        if (!hDolbyDcvLibHanle) {
            ALOGI("%s, failed to load libHwAudio_dcvdec.so, %s\n", __FUNCTION__, dlerror());
        }
    }
    if (hDolbyDcvLibHanle != NULL)
    {
        dlclose(hDolbyDcvLibHanle);
        hDolbyDcvLibHanle = NULL;
        ALOGI("%s,FOUND libHwAudio_dcvdec lib %s\n", __FUNCTION__, dlerror());
        return eDolbyDcvLib;
    }

    ALOGE("%s, failed to FIND libdolbyms12.so and libHwAudio_dcvdec.so, %s\n", __FUNCTION__, dlerror());
    return eDolbyNull;
}


