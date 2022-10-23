/*****************************************************************************
 **
 **  Name:           app_headless_ble.h
 **
 **  Description:    BLE Headless utility functions for applications
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef _APP_HEADLESS_BLE_H_
#define _APP_HEADLESS_BLE_H_

/* Self sufficiency */
#include "bsa_api.h"
#include "app_utils.h"

/*
 * Definitions
 */
#define APP_HEADLESS_BLE_RECORD_LEN_MAX     10

typedef struct
{
    UINT8 len;                  /* Record length */
    UINT16 attribute_handle;
    /* Record data */
    UINT8 data[APP_HEADLESS_BLE_RECORD_LEN_MAX];
} tAPP_HEADLESS_BLE_RECORD;

/*******************************************************************************
 **
 ** Function        app_headless_ble_hid_add_send
 **
 ** Description     Send a BLE HID Device Add command to the local BT Chip
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_headless_ble_hid_add_send(BD_ADDR bdaddr, tBLE_ADDR_TYPE bdaddr_type,
        UINT8 *p_class_of_device, UINT16 appearance, BT_OCTET16  ltk,
        BT_OCTET8 rand, UINT16 ediv);

/*******************************************************************************
 **
 ** Function        app_headless_ble_add_tv_wake_records_send
 **
 ** Description     Send a BLE Add TV Wake records command to the local BT Chip
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_headless_ble_add_tv_wake_records_send(BD_ADDR bdaddr, UINT8 nb_records,
        tAPP_HEADLESS_BLE_RECORD *p_records);

#endif
