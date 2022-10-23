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
#define LOG_TAG "RTL8822BU"
#include "dongle_info.h"
#define BU8822_DRIVER_KO "8822bu"
#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG          "ifname=wlan0 if2name=p2p0"
#endif

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";

static struct wifi_vid_pid bu8822_vid_pid_tables[] =
{
    {0x0bda,0xb82c}
};

static int bu8822_table_len = sizeof(bu8822_vid_pid_tables)/sizeof(struct wifi_vid_pid);

int bu8822_unload_driver()
{
    if (wifi_rmmod(BU8822_DRIVER_KO) != 0) {
        ALOGE("Failed to rmmod rtl8822bu driver ! \n");
        return -1;
    }
    ALOGD("Success to rmmod rtl8822bu driver ! \n");
    return 0;
}

int bu8822_load_driver()
{
    char mod_path[SYSFS_PATH_MAX];
    snprintf(mod_path, SYSFS_PATH_MAX, "%s/%s.ko",WIFI_DRIVER_MODULE_PATH,BU8822_DRIVER_KO);
    if (wifi_insmod(mod_path, DRIVER_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod rtl8822bu driver ! \n");
        return -1;
    }

    ALOGD("Success to insmod rtl8822bu driver! \n");
    return 0;
}

int search_8822bu(unsigned int vid,unsigned int pid)
{
	int k = 0;
	int count=0;

	for (k = 0;k < bu8822_table_len;k++) {
		if (vid == bu8822_vid_pid_tables[k].vid && pid == bu8822_vid_pid_tables[k].pid) {
			count=1;
			write_no("rtl8822bu");
		}
	}
	return count;
}
