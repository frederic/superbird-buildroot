/*****************************************************************************
**
**  Name:           app_ble_wifi_init.h
**
**  Description:    Bluetooth BLE WiFi initiatore include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_WIFI_INIT_H
#define APP_BLE_WIFI_INIT_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

#define APP_BLE_WIFI_INIT_APP_UUID                 0x9991
#define APP_BLE_WIFI_INIT_NUM_OF_CLIENT            1
#define APP_BLE_WIFI_INIT_INFORMATION_DATA_LEN     10

/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_start
 **
 ** Description     start initiator to create wifi connection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_start(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_register_notification
 **
 ** Description     Register notification to receive data from responder
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_register_notification(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_send_data
 **
 ** Description     send wifi data.
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_send_data(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_stop
 **
 ** Description     stop initiator
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_stop(void);


/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_write_information
 **
 ** Description     Write Miracast information to Responder
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_write_information(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_close
 **
 ** Description     This is the ble close connection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_close(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_deregister
 **
 ** Description     This is the ble deregister app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_deregister(void);

/*******************************************************************************
 **
 ** Function         app_ble_wifi_init_disc_cback
 **
 ** Description      Discovery callback
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_wifi_init_disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data);

#endif
