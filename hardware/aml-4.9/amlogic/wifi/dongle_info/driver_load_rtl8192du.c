#define LOG_TAG "RTL8192DU"
#include "dongle_info.h"

#define DU8192_DRIVER_KO "8192du"

#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG          "ifname=wlan0 if2name=p2p0"
#endif

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";

static struct wifi_vid_pid du8192_vid_pid_tables[] =
{
	/*=== Realtek demoboard ===*/
	/****** 8192DU ********/
	{0x0bda,0x8193},//8192DU-VC
	{0x0bda,0x8194},//8192DU-VS
	{0x0bda,0x8111},//Realtek 5G dongle for WiFi Display
	{0x0bda,0x0193},//8192DE-VAU
	{0x0bda,0x8171},//8192DU-VC

	/*=== Customer ID ===*/
	/****** 8192DU-VC ********/
	{0x2019,0xAB2C},//PCI - Abocm
	{0x2019,0x4903},//PCI - ETOP
	{0x2019,0x4904},//PCI - ETOP
	{0x07B8,0x8193},//Abocom - Abocom

	/****** 8192DU-VS ********/
	{0x20F4,0x664B},//TRENDnet

	/****** 8192DU-WiFi Display Dongle ********/
	{0x2019,0xAB2D}//Planex - Abocom ,5G dongle for WiFi Display
};

static int du8192_table_len = sizeof(du8192_vid_pid_tables)/sizeof(struct wifi_vid_pid);

int du8192_unload_driver()
{
    if (wifi_rmmod(DU8192_DRIVER_KO) != 0) {
        ALOGE("Failed to rmmod CU8192_DRIVER_KO !\n");
        return -1;
    }

    ALOGD("SUCCESS to rmsmod rtl8192cu driver! \n");

		return 0;
}


int du8192_load_driver()
{
    char mod_path[SYSFS_PATH_MAX];


		snprintf(mod_path, SYSFS_PATH_MAX, "%s/%s.ko",WIFI_DRIVER_MODULE_PATH,DU8192_DRIVER_KO);
    if (wifi_insmod(mod_path, DRIVER_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod rtl8192du driver! \n");
        return -1;
    }

    ALOGD("Success to insmod rtl8192du driver! \n");
    return 0;
}

int search_du(unsigned int vid,unsigned int pid)
{
	int k = 0;
	int count=0;

	for (k = 0;k < du8192_table_len;k++) {
		if (vid == du8192_vid_pid_tables[k].vid && pid == du8192_vid_pid_tables[k].pid) {
			count=1;
			write_no("rtl8192du");
		}
	}
	return count;
}
