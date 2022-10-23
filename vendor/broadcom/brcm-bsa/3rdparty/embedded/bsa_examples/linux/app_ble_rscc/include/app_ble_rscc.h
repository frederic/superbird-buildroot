/*****************************************************************************
**
**  Name:           app_ble_rscc.h
**
**  Description:    BLE Running Speed and Cadence Collector include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_RSCC_H
#define APP_BLE_RSCC_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

/*
 * BLE common functions
 */
/*******************************************************************************
 **
 ** Function        app_ble_rscc_main
 **
 ** Description     main function of RSC collector
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_main(void);

/*******************************************************************************
 **
 ** Function        app_ble_rscc_register
 **
 ** Description     Register RSC Collector services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_register(void);

/*******************************************************************************
 **
 ** Function        app_ble_rscc_read_rscs_feature
 **
 ** Description     Read RSC Sensor's feature Character
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_read_rscs_feature(void);

/*******************************************************************************
 **
 ** Function        app_ble_rscc_read_rscs_location
 **
 ** Description     Read RSC Sensor's location Character
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_read_rscs_location(void);

/*******************************************************************************
 **
 ** Function        app_ble_rscc_measure
 **
 ** Description     Measure RSC Sensor Data
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_measure(BOOLEAN enable);

/*******************************************************************************
 **
 ** Function        app_ble_rscc_set_cumul_value
 **
 ** Description     Configure Cumulative value on RSCS control point
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_set_cumul_value(void);

/*******************************************************************************
 **
 ** Function        app_ble_rscc_start_ss_cali
 **
 ** Description     Start Sensor Calibration on RSCS control point
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_start_ss_cali(void);

/*******************************************************************************
 **
 ** Function        app_ble_rscc_enable_sc_control_point
 **
 ** Description     This is the register function to receive a indication
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_rscc_enable_sc_control_point(BOOLEAN enable);
#endif
