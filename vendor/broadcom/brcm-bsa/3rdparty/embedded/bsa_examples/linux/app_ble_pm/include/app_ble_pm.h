/*****************************************************************************
**
**  Name:           app_ble_pm.h
**
**  Description:    Bluetooth BLE Proximity Monitor include file
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_PM_H
#define APP_BLE_PM_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

/*
 * BLE common functions
 */
/*******************************************************************************
 **
 ** Function        app_ble_pm_main
 **
 ** Description     main function of Proximity Monitor
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_pm_main(void);

/*******************************************************************************
 **
 ** Function        app_ble_pm_register
 **
 ** Description     Register Proximity Monitor services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_pm_register(void);


/*******************************************************************************
 **
 ** Function        app_ble_pm_write_linkloss_alert_level
 **
 ** Description     Configure PM alert level
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_pm_write_linkloss_alert_level(void);

/*******************************************************************************
 **
 ** Function        app_ble_pm_write_immediate_alert_level
 **
 ** Description     Configure PM alert level
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_pm_write_immediate_alert_level(void);


/*******************************************************************************
 **
 ** Function        app_ble_pm_read_tx_power
 **
 ** Description     Read TX power of Proximity Reporter
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_pm_read_tx_power(void);

#endif
