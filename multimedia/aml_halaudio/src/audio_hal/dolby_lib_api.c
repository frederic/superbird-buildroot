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
#include "log.h"
#include <sys/prctl.h>
#include "properties.h"
#include <dlfcn.h>
#include <sys/stat.h>

#include "dolby_lib_api.h"

#ifndef RET_OK
#define RET_OK 0
#endif

#ifndef RET_FAIL
#define RET_FAIL -1
#endif


#define DOLBY_MS12_LIB_PATH_A "/vendor/lib/libdolbyms12.so"
#define DOLBY_MS12_LIB_PATH_B "/system/vendor/lib/libdolbyms12.so"

//#define DOLBY_DCV_LIB_PATH_A "/vendor/lib/libHwAudio_dcvdec.so"
#define DOLBY_DCV_LIB_PATH_A "/usr/lib/libdcv.so"
#define DOLBY_ATMOS_LIB_PATH_A "/usr/lib/libdolby_atmos.so"


/*
 *@brief file_accessible
 */
static int file_accessible(char *path)
{
    // file is readable or not
    if (access(path, R_OK) == 0) {
        return RET_OK;
    } else {
        ALOGE("path %s can not access!\n", path);
        return RET_FAIL;
    }
}


int get_file_size(char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    int size=statbuf.st_size;

    return size;
}



/*
 *@brief detect_dolby_lib_type
 */
enum eDolbyLibType detect_dolby_lib_type(void) {
    enum eDolbyLibType retVal;
    int ms12_file_size = 0;
    int ms12_len1 = get_file_size(DOLBY_MS12_LIB_PATH_A);
    int ms12_len2 = get_file_size(DOLBY_MS12_LIB_PATH_B);
    int datmos_len  = get_file_size(DOLBY_ATMOS_LIB_PATH_A);
    int dcv_len = get_file_size(DOLBY_DCV_LIB_PATH_A);

    if (ms12_len1 > 0)
        ms12_file_size = ms12_len1;
    else if (ms12_len2 > 0)
        ms12_file_size = ms12_len2;

    // the priority would be "MS12 > Atmos >DCV" lib
    ALOGE("file_size ms12 %d datmos %d dcv %d\n", ms12_file_size, datmos_len, dcv_len);
    if ((RET_OK == file_accessible(DOLBY_MS12_LIB_PATH_A)) && (ms12_file_size > 0))
    {
        retVal = eDolbyMS12Lib;
    } else if ((RET_OK == file_accessible(DOLBY_MS12_LIB_PATH_B)) && (ms12_file_size > 0))
    {
        retVal = eDolbyMS12Lib;
    } else if ((RET_OK == file_accessible(DOLBY_ATMOS_LIB_PATH_A)) && (datmos_len > 0))
    {
        retVal = eDolbyAtmosLib;
    } else if ((RET_OK == file_accessible(DOLBY_DCV_LIB_PATH_A)) && (dcv_len > 0))
    {
        retVal = eDolbyDcvLib;
    } else {
        retVal = eDolbyNull;
    }

    return retVal;
}


