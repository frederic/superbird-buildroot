/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "RTL8192ES"
#include "dongle_info.h"
#define ES8192_DRIVER_KO "8192es"

#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG          "ifname=wlan0 if2name=p2p0"
#endif

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";

int es8192_unload_driver()
{
    if (wifi_rmmod(ES8192_DRIVER_KO) != 0) {
        ALOGE("Failed to rmmod rtl8192es driver ! \n");
        return -1;
    }

    ALOGD("Success to rmmod rtl8192es driver! \n");

    return 0;
}

int es8192_load_driver()
{
    char mod_path[SYSFS_PATH_MAX];


	  snprintf(mod_path, SYSFS_PATH_MAX, "%s/%s.ko",WIFI_DRIVER_MODULE_PATH,ES8192_DRIVER_KO);
    if (wifi_insmod(mod_path, DRIVER_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod rtl8192es driver!\n");
        return -1;
    }

    ALOGD("Success to insmod rtl8192es driver! \n");
    return 0;
}


int search_es8192(unsigned int x,unsigned int y)
{
    int fd,len;
    char sdio_buf[128];
    FILE *fp = fopen(file_name,"r");
    if (!fp)
        return -1;

    memset(sdio_buf,0,sizeof(sdio_buf));
    if (fread(sdio_buf, 1,128,fp) < 1) {
        ALOGE("Read error \n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    if (strstr(sdio_buf,"818b")) {
        write_no("rtl8192es");
        ALOGE("Found 8192es !!!\n");
        return 1;
    }
    return 0;
}
