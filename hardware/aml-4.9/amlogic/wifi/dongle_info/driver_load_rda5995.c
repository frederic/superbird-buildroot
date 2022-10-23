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
#define LOG_TAG "RDA5995"
#include "dongle_info.h"
static const char RDA5995_MODULE_NAME[]  = "rdawfmac";
static const char RDA5995_MODULE_TAG[]   = "rdawfmac";
static const char RDA5995_MODULE_PATH[]  = "/system/lib/rdawfmac.ko";
static const char RDA5995_MODULE_ARG[]   = "firmware_path=sta";


static struct wifi_vid_pid rda5995_vid_pid_tables[] =
{
    {0x1e04,0x8888}
};

static int rda5995_table_len = sizeof(rda5995_vid_pid_tables)/sizeof(struct wifi_vid_pid);

int rda5995_unload_driver()
{
    if (wifi_rmmod(RDA5995_MODULE_NAME) != 0) {
        ALOGE("Failed to rmmod rda5995 driver ! \n");
        return -1;
    }
    ALOGD("Success to rmmod rda5995 driver ! \n");
    return 0;
}

int rda5995_load_driver()
{

    if (wifi_insmod(RDA5995_MODULE_PATH, RDA5995_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod dhd ! \n");
        return -1;
    }

    ALOGD("Success to insmod rda5995 driver! \n");
    return 0;
}

int search_rda5995(unsigned int vid,unsigned int pid)
{
	int k = 0;
	int count=0;

	for (k = 0;k < rda5995_table_len;k++) {
		if (vid == rda5995_vid_pid_tables[k].vid && pid == rda5995_vid_pid_tables[k].pid) {
			count=1;
			write_no("rda5995");
		}
	}
	return count;
}
