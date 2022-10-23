#define LOG_TAG "RTL8723DU"
#include "dongle_info.h"

#define DU8723_DRIVER_KO "8723du"
#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG          "ifname=wlan0 if2name=p2p0"
#endif

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";

static struct wifi_vid_pid du8723_vid_pid_tables[] =
{
    {0x0bda,0xD723}
};

static int du8723_table_len = sizeof(du8723_vid_pid_tables)/sizeof(struct wifi_vid_pid);

int du8723_unload_driver()
{
    if (wifi_rmmod(DU8723_DRIVER_KO) != 0) {
        ALOGE("Failed to rmmod rtl8723du driver ! \n");
        return -1;
    }
    ALOGD("Success to rmmod rtl8723du driver ! \n");
    return 0;
}

int du8723_load_driver()
{
    char mod_path[SYSFS_PATH_MAX];
    snprintf(mod_path, SYSFS_PATH_MAX, "%s/%s.ko",WIFI_DRIVER_MODULE_PATH,DU8723_DRIVER_KO);
    if (wifi_insmod(mod_path, DRIVER_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod rtl8723du driver ! \n");
        return -1;
    }

    ALOGD("Success to insmod rtl8723du driver! \n");
    return 0;
}

int search_8723du(unsigned int vid,unsigned int pid)
{
	int k = 0;
	int count=0;

	for (k = 0;k < du8723_table_len;k++) {
		if (vid == du8723_vid_pid_tables[k].vid && pid == du8723_vid_pid_tables[k].pid) {
			count=1;
			write_no("rtl8723du");
		}
	}
	return count;
}
