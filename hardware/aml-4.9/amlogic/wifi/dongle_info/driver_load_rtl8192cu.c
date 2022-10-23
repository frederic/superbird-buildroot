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
#define LOG_TAG "RTL8912CU"
#include "dongle_info.h"
//CU8192 
#define CU8192_DRIVER_KO "8192cu"

#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG          "ifname=wlan0 if2name=p2p0"
#endif

#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER		""
#endif

static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";

static struct wifi_vid_pid cu8192_vid_pid_tables[] =
{
    {0x2001,0x330a},
    {0x0bda,0x8178},
    {0x0bda,0x8177},
    {0x0bda,0x8176},
    {0x0bda,0x817e},
    {0x0bda,0x018a},
    {0x0b05,0x1791},
    {0x13d3,0x3311},
    {0x13d3,0x3359},
    {0x0bda,0x8191},
	{0x0bda,0x8170},//8188CE-VAU USB minCard
	{0x0bda,0x817E},//8188CE-VAU USB minCard
	{0x0bda,0x817A},//8188cu Slim Solo
	{0x0bda,0x817B},//8188cu Slim Combo
	{0x0bda,0x817D},//8188RU High-power USB Dongle
	{0x0bda,0x8754},//8188 Combo for BC4
	{0x0bda,0x817F},//8188RU
	{0x0bda,0x818A},//RTL8188CUS-VL
	{0x0bda,0x018A},//RTL8188CTV
		
	/****** 8192CUS ********/
	{0x0bda,0x817C},//8192CE-VAU USB minCard
	
	/*=== Customer ID ===*/	
	/****** 8188CUS Dongle ********/
	{0x2019,0xED17},//PCI - Edimax
	{0x0DF6,0x0052},//Sitecom - Edimax
	{0x7392,0x7811},//Edimax - Edimax
	{0x07B8,0x8189},//Abocom - Abocom
	{0x0EB0,0x9071},//NO Brand - Etop
	{0x06F8,0xE033},//Hercules - Edimax
	{0x103C,0x1629},//HP - Lite-On ,8188CUS Slim Combo
	{0x2001,0x3308},//D-Link - Alpha
	{0x050D,0x1102},//Belkin - Edimax
	{0x2019,0xAB2A},//Planex - Abocom
	{0x20F4,0x648B},//TRENDnet - Cameo
	{0x4855,0x0090},// - Feixun
	{0x13D3,0x3357},// - AzureWave
	{0x0DF6,0x005C},//Sitecom - Edimax
	{0x0BDA,0x5088},//Thinkware - CC&C
	{0x4856,0x0091},//NetweeN - Feixun
	{0x2019,0x4902},//Planex - Etop
	{0x2019,0xAB2E},//SW-WF02-AD15 -Abocom

	/****** 8188 RU ********/
	{0x0BDA,0x317F},//Netcore,Netcore
	
	/****** 8188CE-VAU ********/
	{0x13D3,0x3359},// - Azwave
	{0x13D3,0x3358},// - Azwave

	/****** 8188CUS Slim Solo********/
	{0x04F2,0xAFF7},//XAVI - XAVI
	{0x04F2,0xAFF9},//XAVI - XAVI
	{0x04F2,0xAFFA},//XAVI - XAVI

	/****** 8188CUS Slim Combo ********/
	{0x04F2,0xAFF8},//XAVI - XAVI
	{0x04F2,0xAFFB},//XAVI - XAVI
	{0x04F2,0xAFFC},//XAVI - XAVI
	{0x2019,0x1201},//Planex - Vencer
	
	/****** 8192CUS Dongle ********/
	{0x2001,0x3307},//D-Link - Cameo
	{0x2001,0x330A},//D-Link - Alpha
	{0x2001,0x3309},//D-Link - Alpha
	{0x0586,0x341F},//Zyxel - Abocom
	{0x7392,0x7822},//Edimax - Edimax
	{0x2019,0xAB2B},//Planex - Abocom
	{0x07B8,0x8178},//Abocom - Abocom
	{0x07AA,0x0056},//ATKK - Gemtek
	{0x4855,0x0091},// - Feixun
	{0x2001,0x3307},//D-Link-Cameo
	{0x050D,0x2102},//Belkin - Sercomm
	{0x050D,0x2103},//Belkin - Edimax
	{0x20F4,0x624D},//TRENDnet
	{0x0DF6,0x0061},//Sitecom - Edimax
	{0x0B05,0x17AB},//ASUS - Edimax
	{0x0846,0x9021},//Netgear - Sercomm
	{0x0E66,0x0019},//Hawking,Edimax 

	/****** 8192CE-VAU  ********/
	{0x0bda,0x8186}//Intel-Xavi( Azwave)
};

static int cu8192_table_len = sizeof(cu8192_vid_pid_tables)/sizeof(struct wifi_vid_pid);

int cu8192_unload_driver()
{
    if(wifi_rmmod(CU8192_DRIVER_KO) != 0){
        ALOGE("Failed to rmmod rtl8192cu driver !\n");
        return -1;
    }

    ALOGD("Success to rmmod rtl8192cu driver !\n");

    return 0;
}

int cu8192_load_driver()
{
    char mod_path[SYSFS_PATH_MAX];


    snprintf(mod_path, SYSFS_PATH_MAX, "%s/%s.ko",WIFI_DRIVER_MODULE_PATH,CU8192_DRIVER_KO);
    if (wifi_insmod(mod_path, DRIVER_MODULE_ARG) !=0) {
        ALOGE("Failed to insmod rtl8192cu driver !\n");
        return -1;
    }
    ALOGD("Success to insmod rtl8192cu driver !\n");

    return 0;
}

int search_cu(unsigned int vid,unsigned int pid)
{
	int k = 0;
	int count=0;

	for (k = 0;k < cu8192_table_len;k++) {
		if (vid == cu8192_vid_pid_tables[k].vid && pid == cu8192_vid_pid_tables[k].pid) {
			count=1;
			write_no("rtl8192cu");
		}
  }
	return count;
}
