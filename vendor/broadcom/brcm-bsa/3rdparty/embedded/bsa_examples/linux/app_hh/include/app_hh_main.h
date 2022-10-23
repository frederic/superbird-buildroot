/*****************************************************************************
 **
 **  Name:           app_hh_main.h
 **
 **  Description:    Bluetooth HID Host Main application
 **
 **  Copyright (c) 2010-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/


#ifndef APP_HH_MAIN_H
#define APP_HH_MAIN_H

#include "bsa_mgt_api.h"

extern void app_management_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data);
extern int app_read_xml_remote_devices(void);
extern int app_write_xml_remote_devices(void);

#endif
