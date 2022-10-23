/*****************************************************************************
**
**  Name:           app_ble_client_otafwdl.h
**
**  Description:    Bluetooth LE OTA fw download header file
**
**  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef  APP_BLE_CLIENT_OTAFWDL_H
#define  APP_BLE_CLIENT_OTAFWDL_H

typedef enum
{
    APP_BLE_CLIENT_OTAFWDL_STATUS_SUCCESS = 0,
    APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL
} tAPP_BLE_CLIENT_OTAFWDL_STATUS;

/*******************************************************************************
**
**  Function        app_ble_client_otafwdl_init
**
**  Description     initial fw control block
**
**  Returns         none
**
***************************************************************************/
int app_ble_client_otafwdl_init(void);

/*******************************************************************************
 **
 ** Function        app_ble_client_otafwdl
 **
 ** Description     do preparation for fw upgrade
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 ***************************************************************************/
int app_ble_client_otafwdl(void);
/*******************************************************************************
**
** Function         app_ble_client_otafwdl_event_handler
**
** Description      handle fw upgrade state machine
**
** Returns          None
**
 ***************************************************************************/
void app_ble_client_otafwdl_event_handler(tBSA_BLE_EVT event ,tBSA_BLE_MSG *p_data);

#endif
