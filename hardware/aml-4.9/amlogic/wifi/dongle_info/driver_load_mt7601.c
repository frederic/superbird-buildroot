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
#define LOG_TAG "MT7601"
#include "dongle_info.h"
static const char MT7601_MODULE_NAME[]  = "mt7601usta";
static const char MT7601_MODULE_TAG[]   = "mt7601usta";
static const char MT7601_MODULE_PATH[]  = "/system/lib/mt7601usta.ko";
static const char MT7601_MODULE_ARG[]   = "";

static const char MTK_MODULE_NAME[]  = "mtprealloc";
static const char MTK_MODULE_TAG[]   = "mtprealloc";
static const char MTK_MODULE_PATH[]  = "/system/lib/mtprealloc.ko";
static const char MTK_MODULE_ARG[]   = "";

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

//static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";

static struct wifi_vid_pid mt7601_vid_pid_tables[] =
{
    {0x148f,0x7601}
};

static int mt7601_table_len = sizeof(mt7601_vid_pid_tables)/sizeof(struct wifi_vid_pid);
char dongle_id[] = "/data/misc/wifi/wid_fp";

int write_no(char *wifi_type)
{
    int fd,len;
    fd = open(dongle_id,O_CREAT|O_RDWR, S_IRWXU);
    if (fd == -1) {
        ALOGE("write no Open file failed !!!\n");
        return -1;
    }
    len = write(fd,wifi_type,strlen(wifi_type));
    if (len == -1) {
        ALOGE("Write file failed !!!\n");
        close(fd);
        return -1;
    }
    close(fd);
    if (chmod(dongle_id, 0664) < 0 || chown(dongle_id, AID_SYSTEM, AID_WIFI) < 0) {
       ALOGE("Error changing permissions of %s to 0664: %s",
             dongle_id, strerror(errno));
       return -1;
    }
    return 0;
}

int read_no(char *wifi_type)
{
    int fd,len;
    fd = open(dongle_id,O_RDONLY, S_IRWXU);
    if (fd == -1) {
        ALOGE("Open file failed !!!\n");
        return -1;
    }
    len = read(fd,wifi_type,15);
    if (len == -1) {
        ALOGE("Read file failed !!!\n");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int wifi_insmod(const char *filename, const char *args)
{
    void *module;
    unsigned int size;
    int ret;

    module = load_file(filename, &size);
    if (!module)
        return -1;

    ret = init_module(module, size, args);

    free(module);

    return ret;
}

int wifi_rmmod(const char *modname)
{
    int ret = -1;
    int maxtry = 10;

    while (maxtry-- > 0) {
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if (ret < 0 && errno == EAGAIN)
            usleep(500000);
        else
            break;
    }

    if (ret != 0)
       ALOGE("Unable to unload driver module \"%s\": %s\n",
             modname, strerror(errno));
    return ret;
}

static int is_mtprealloc_driver_loaded() {
    FILE *proc;
    char line[sizeof(MTK_MODULE_TAG)+10];

    /*
     * If the property says the driver is loaded, check to
     * make sure that the property setting isn't just left
     * over from a previous manual shutdown or a runtime
     * crash.
     */
    if ((proc = fopen("/proc/modules", "r")) == NULL) {
        ALOGW("Could not open %s", strerror(errno));
        return 0;
    }
    while ((fgets(line, sizeof(line), proc)) != NULL) {
        if (strncmp(line, MTK_MODULE_TAG, strlen(MTK_MODULE_TAG)) == 0) {
            fclose(proc);
            return 1;
        }
    }
    fclose(proc);
    return 0;
}

int mt7601_unload_driver()
{
    if (wifi_rmmod(MT7601_MODULE_NAME) != 0) {
        ALOGE("Failed to rmmod mt7601 driver ! \n");
        return -1;
    }
    ALOGD("Success to rmmod mt7601 driver ! \n");
    return 0;
}

int mt7601_load_driver()
{
    if (!is_mtprealloc_driver_loaded())
       wifi_insmod(MTK_MODULE_PATH, MTK_MODULE_ARG);

    if (wifi_insmod(MT7601_MODULE_PATH, MT7601_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod dhd ! \n");
        return -1;
    }

    ALOGD("Success to insmod mt7601 driver! \n");
    return 0;
}

int search_mt7601(unsigned int vid,unsigned int pid)
{
	int k = 0;
	int count=0;

	for (k = 0;k < mt7601_table_len;k++) {
		if (vid == mt7601_vid_pid_tables[k].vid && pid == mt7601_vid_pid_tables[k].pid) {
			count=1;
			write_no("mtk7601");
		}
	}
	return count;
}
