#define LOG_TAG "RTL8723DS"
#include "dongle_info.h"
#define ES8192_DRIVER_KO "8723ds"

#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG          "ifname=wlan0 if2name=p2p0"
#endif

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";

int ds8723_unload_driver()
{
    if (wifi_rmmod(ES8192_DRIVER_KO) != 0) {
        ALOGE("Failed to rmmod rtl8723ds driver ! \n");
        return -1;
    }

    ALOGD("Success to rmmod rtl8723ds driver! \n");

    return 0;
}

int ds8723_load_driver()
{
    char mod_path[SYSFS_PATH_MAX];


	  snprintf(mod_path, SYSFS_PATH_MAX, "%s/%s.ko",WIFI_DRIVER_MODULE_PATH,ES8192_DRIVER_KO);
    if (wifi_insmod(mod_path, DRIVER_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod rtl8723ds driver!\n");
        return -1;
    }

    ALOGD("Success to insmod rtl8723ds driver! \n");
    return 0;
}


int search_ds8723(unsigned int x,unsigned int y)
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
    if (strstr(sdio_buf,"d723")) {
        write_no("rtl8723ds");
        ALOGE("Found 8723ds !!!\n");
        return 1;
    }
    return 0;
}
