/*****************************************************************************
 **
 **  Name:           app_headless.h
 **
 **  Description:    Bluetooth Headless functions
 **
 **  Copyright (c) 2012-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_HEADLESS_H
#define APP_HEADLESS_H

#include "bsa_api.h"
#include "bt_types.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_dm.h"

/*
 * Definitions
 */
#ifndef APP_HEADLESS_DEV_NB
#define APP_HEADLESS_DEV_NB     16
#endif

typedef struct
{
    BD_ADDR bdaddr;
    DEV_CLASS class_of_device;
} tAPP_HEADLESS_DEV_LIST;

/*******************************************************************************
 **
 ** Function         app_headless_send_add_device
 **
 ** Description      This function sends a VSC to add one Headless device
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_add_device(BD_ADDR bdaddr, UINT8 *p_class_of_device,
        LINK_KEY link_key);

/*******************************************************************************
 **
 ** Function         app_headless_send_enable_uhe
 **
 ** Description      This function sends a VSC to Control Headless mode
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_enable_uhe(int enable);

/*******************************************************************************
 **
 ** Function         app_headless_send_del_device
 **
 ** Description      This function sends a VSC to delete one Headless device
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_del_device(BD_ADDR bdaddr);

/*******************************************************************************
 **
 ** Function         app_headless_send_list_device
 **
 ** Description      This function sends a VSC to list Headless devices
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_list_device(tAPP_HEADLESS_DEV_LIST *p_dev_list,
        UINT8 table_size);

/*******************************************************************************
 **
 ** Function         app_headless_scan_control
 **
 ** Description      Enable/Disable Headless Scan mode
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_scan_control(void);

#endif

