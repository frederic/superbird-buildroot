#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#define LOG_TAG "MT7668"
#include "dongle_info.h"

static const char mt7668_MODULE_NAME[]  = "wlan_mt76x8_sdio";
static const char mt7668_MODULE_TAG[]   = "wlan_mt76x8_sdio";
static const char mt7668_MODULE_PATH[]  = "/system/lib/wlan_mt76x8_sdio.ko";
static const char mt7668_MODULE_ARG[]   = "";

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

//static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";


int mt7668_load_driver()
{
    if (wifi_insmod(mt7668_MODULE_PATH, mt7668_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod mt7668! \n");
        return -1;
    }
    ALOGD("Success to insmod mt7668 driver! \n");
    return 0;

}

int mt7668_unload_driver()
{
    if (wifi_rmmod(mt7668_MODULE_NAME) != 0) {
        ALOGE("Failed to rmmod mt7668 driver ! \n");
        return -1;
    }
    ALOGD("Success to rmmod mt7668 driver ! \n");
    return 0;
}

int search_mt7668(unsigned int x,unsigned int y)
{
    int fd,len;
    char sdio_buf[128];
    FILE *fp = fopen(file_name,"r");
    if (!fp) {
        ALOGE("Open sdio wifi file failed !!! \n");
        return -1;
    }
    memset(sdio_buf,0,sizeof(sdio_buf));
    if (fread(sdio_buf, 1,128,fp) < 1) {
        ALOGE("Read error for %m\n", errno);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    ALOGE("board id=%s\n",sdio_buf);
    if (strstr(sdio_buf,"7608")) {
        ALOGE("Found MTK7668!!!\n");
        write_no("mtk7668");
        return 1;
    }
    return 0;
}
