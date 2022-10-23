/*****************************************************************************
**
**  Name:           app_ble_hrc.h
**
**  Description:    Bluetooth BLE Heart Rate Collector include file
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_HRC_H
#define APP_BLE_HRC_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

/*
 * BLE common functions
 */

/*******************************************************************************
 **
 ** Function        app_ble_hrc_main
 **
 ** Description     main function of Heart Rate collector 
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_main(void);

/*******************************************************************************
 **
 ** Function        app_ble_hrc_register
 **
 ** Description     Register Heart Rate collector services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_register(void);

/*******************************************************************************
 **
 ** Function        app_ble_hrc_monitor
 **
 ** Description     Register Heart Rate collector services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_monitor(int enable);

/*******************************************************************************
 **
 ** Function        app_ble_hrc_configure_monitor
 **
 ** Description     
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_configure_monitor(int client);

/*******************************************************************************
 **
 ** Function        app_ble_hrc_configure_stop_monitor
 **
 ** Description     
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_configure_stop_monitor(int client);

/*******************************************************************************
 **
 ** Function        app_ble_hrc_register_notification
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_register_notification(int client);

/*******************************************************************************
 **
 ** Function        app_ble_hrc_deregister_notification
 **
 ** Description     This is the deregister function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_deregister_notification(int client);

/*******************************************************************************
 **
 ** Function        app_ble_hrc_read_body_sensor_location
 **
 ** Description     Read sensor location from BLE heart rate server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_read_body_sensor_location(void);
#endif
