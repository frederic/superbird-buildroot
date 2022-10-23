#define LOG_TAG "QCA9379"
#include "dongle_info.h"

static const char QCA9379_MODULE_NAME[]  = "wlan";
static const char QCA9379_MODULE_TAG[]   = "wlan";
static const char QCA9379_MODULE_PATH[]  = "/system/lib/wlan_9379.ko";
static const char QCA9379_MODULE_ARG[]   = "";

int qca9379_load_driver()
{
    if (wifi_insmod(QCA9379_MODULE_PATH, QCA9379_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod qca9379 ! \n");
        return -1;
    }
    ALOGD("Success to insmod qca9379 driver! \n");
    return 0;
}

int qca9379_unload_driver()
{
    if (wifi_rmmod(QCA9379_MODULE_NAME) != 0) {
        ALOGE("Failed to rmmod qca9379 driver ! \n");
        return -1;
    }
    ALOGD("Success to rmmod qca9379 driver ! \n");
    return 0;
}

int search_qca9379(unsigned int x,unsigned int y)
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
    if (strstr(sdio_buf,"0801")) {
        write_no("qca9379");
        ALOGE("Found qca9379 !!!\n");
        return 1;
    }
    return 0;
}
