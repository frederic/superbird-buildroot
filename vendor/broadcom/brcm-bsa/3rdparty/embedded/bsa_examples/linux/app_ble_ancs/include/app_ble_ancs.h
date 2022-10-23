/*****************************************************************************
**
**  Name:           app_ble_ancs.h
**
**  Description:    BLE Running Speed and Cadence Collector include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_ANCS_H
#define APP_BLE_ANCS_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

/*
 * BLE common functions
 */
/*******************************************************************************
 **
 ** Function        app_ble_ancs_main
 **
 ** Description     main function of ANCS consumer
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_ancs_main(void);

/*******************************************************************************
 **
 ** Function        app_ble_ancs_register
 **
 ** Description     Register ANCS consumer
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_ancs_register(void);

/*******************************************************************************
 **
 ** Function        app_ble_ancs_register_notifications
 **
 ** Description     Register/ Deregister for ANCS Notifications
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_ancs_register_notifications(BOOLEAN bRegister);

/*******************************************************************************
 **
 ** Function        app_ble_ancs_disable_advertisment
 **
 ** Description     Disable BLE ANCS Advertisements
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
// *******************************************************************************/
void app_ble_ancs_disable_advertisment();

#endif
