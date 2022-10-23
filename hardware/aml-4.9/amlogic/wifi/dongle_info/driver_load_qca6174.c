#define LOG_TAG "QCA6174"
#include "dongle_info.h"

static const char QCA6174_MODULE_NAME[]  = "wlan";
static const char QCA6174_MODULE_TAG[]   = "wlan";
static const char QCA6174_MODULE_PATH[]  = "/system/lib/wlan_6174.ko";
static const char QCA6174_MODULE_ARG[]   = "country_code=CN";

int qca6174_load_driver()
{
    if (wifi_insmod(QCA6174_MODULE_PATH, QCA6174_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod qca6174 ! \n");
        return -1;
    }
    ALOGD("Success to insmod qca6174 driver! \n");
    return 0;
}

int qca6174_unload_driver()
{
    if (wifi_rmmod(QCA6174_MODULE_NAME) != 0) {
        ALOGE("Failed to rmmod qca6174 driver ! \n");
        return -1;
    }
    ALOGD("Success to rmmod qca6174 driver ! \n");
    return 0;
}

int search_qca6174(unsigned int x,unsigned int y)
{
    int fd,len;
    char sdio_buf[128];
    FILE *fp = fopen(file_name,"r");
    if (!fp)
        return -1;

    memset(sdio_buf,0,sizeof(sdio_buf));
    if (fread(sdio_buf, 1,128,fp) < 1) {
        ALOGE("Read error for\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    if (strstr(sdio_buf,"050a")) {
        write_no("qca6174");
        ALOGE("Found qca6174 !!!\n");
        return 1;
    }
    return 0;
}
