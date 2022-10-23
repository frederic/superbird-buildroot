/*****************************************************************************
**
**  Name:           app_ble_htp.h
**
**  Description:     Bluetooth BLE Health Thermometer Collector include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_HTP_H
#define APP_BLE_HTP_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

/*
 * BLE common functions
 */

/*******************************************************************************
 **
 ** Function        app_ble_htp_main
 **
 ** Description     main function of Thermometer Collector
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_main(void);

/*******************************************************************************
 **
 ** Function        app_ble_htp_register
 **
 ** Description     Register Health Thermometer Collector services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_register(void);

/*******************************************************************************
 **
 ** Function        app_ble_htp_read_temperature_type
 **
 ** Description     Read temperature measurement type from BLE Health Thermometer server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_read_temperature_type(void);

/*******************************************************************************
 **
 ** Function        app_ble_htp_enable_temperature_measurement
 **
 ** Description     This is the register function to receive a indication
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_enable_temperature_measurement(BOOLEAN enable);

/*******************************************************************************
 **
 ** Function        app_ble_htp_intermediate_temperature
 **
 ** Description     Measure Intermediate Temperature
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_intermediate_temperature(BOOLEAN enable);

/*******************************************************************************
 **
 ** Function        app_ble_htp_read_measurement_interval
 **
 ** Description     Read temperature measurement interval from BLE Health Thermometer server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_read_measurement_interval(void);

/*******************************************************************************
 **
 ** Function        app_ble_htp_read_measurement_interval_range
 **
 ** Description     Read valid range of interval from BLE Health Thermometer server to enable temperature measurement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_read_measurement_interval_range(void);

/*******************************************************************************
 **
 ** Function        app_ble_htp_write_measurement_interval
 **
 ** Description     Writes the interval between consecutive temperature measurements.
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_write_measurement_interval(void);

/*******************************************************************************
 **
 ** Function        app_ble_htp_enable_interval_measurement
 **
 ** Description     This is the register function to receive a indication
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_enable_interval_measurement(BOOLEAN enable);
#endif
