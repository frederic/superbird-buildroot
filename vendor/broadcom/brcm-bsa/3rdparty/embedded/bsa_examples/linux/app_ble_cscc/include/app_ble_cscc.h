/*****************************************************************************
**
**  Name:           app_ble_cscc.h
**
**  Description:    BLE Cycling Speed and Cadence Collector include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_CSCC_H
#define APP_BLE_CSCC_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

/*
 * BLE common functions
 */
/*******************************************************************************
 **
 ** Function        app_ble_cscc_main
 **
 ** Description     main function of CSC Collector
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_cscc_main(void);


/*******************************************************************************
 **
 ** Function        app_ble_cscc_register
 **
 ** Description     Register CSC Collector services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_cscc_register(void);


/*******************************************************************************
 **
 ** Function        app_ble_cscc_read_cscs_feature
 **
 ** Description     Read CSC Sensor's feature Character
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_cscc_read_cscs_feature(void);


/*******************************************************************************
 **
 ** Function        app_ble_cscc_read_cscs_location
 **
 ** Description     Read CSC Sensor's location Character
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_cscc_read_cscs_location(void);


/*******************************************************************************
 **
 ** Function        app_ble_cscc_measure
 **
 ** Description     Measure CSC Sensor Data
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_cscc_measure(BOOLEAN enable);

#endif
