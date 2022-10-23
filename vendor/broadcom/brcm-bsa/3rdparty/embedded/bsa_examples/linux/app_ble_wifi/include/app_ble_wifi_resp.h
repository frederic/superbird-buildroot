/*****************************************************************************
**
**  Name:           app_ble_wifi_resp.h
**
**  Description:    Bluetooth BLE WiFi responder include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_WIFI_RESP_H
#define APP_BLE_WIFI_RESP_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"
#include "app_thread.h"

/* WiFi Helper UUID Definitions */
#define APP_BLE_WIFI_RESP_APP_UUID          0x9990
#define APP_BLE_WIFI_RESP_SERV_UUID          0xA108
#define APP_BLE_WIFI_RESP_CHAR_UUID          0xA155
#define APP_BLE_WIFI_RESP_NUM_OF_HANDLES     10
#define APP_BLE_WIFI_RESP_NUM_OF_SERVER      1
#define APP_BLE_WIFI_RESP_ATTR_NUM           1

#define APP_BLE_WIFI_RESP_ADV_VALUE_LEN        6
#define APP_BLE_WIFI_RESP_SCAN_RSP_VALUE_LEN   6
#define APP_BLE_WIFI_RESP_NOTI_VALUE_LEN       10

#define APP_BLE_WIFI_RESP_ADV_VALUE       {0x2b, 0x1a, 0xaa, 0xbb, 0xcc, 0xdd};
#define APP_BLE_WIFI_RESP_SCAN_RSP_VALUE  {0x77, 0x11, 0x22, 0x33, 0x44, 0x55};
#define APP_BLE_WIFI_RESP_NOTI_VALUE \
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00};

typedef struct
{
    tAPP_THREAD  resp_thread;
    BOOLEAN      notification_send;
    UINT16       conn_id;
    UINT16       attr_id;
} tAPP_BLE_WIFI_RESP_CB;

/*******************************************************************************
 **
 ** Function        app_ble_wifi_resp_start
 **
 ** Description     start responder to create wifi connection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_resp_start(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_resp_stop
 **
 ** Description     stop responder
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_resp_stop(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_resp_deregister
 **
 ** Description     Deregister server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_resp_deregister(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_resp_create_service
 **
 ** Description     create service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_resp_create_service(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_resp_start_service
 **
 ** Description     Start Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_resp_start_service(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_resp_send_notification
 **
 ** Description     Send notification to initiator
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_resp_send_notification(void);

/*******************************************************************************
 **
 ** Function        app_ble_wifi_resp_add_char
 **
 ** Description     Add character to service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_resp_add_char(void);

#endif
