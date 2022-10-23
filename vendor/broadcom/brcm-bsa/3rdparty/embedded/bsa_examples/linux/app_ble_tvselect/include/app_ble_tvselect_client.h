/*****************************************************************************
**
**  Name:           app_ble_tvselect_client.h
**
**  Description:    Bluetooth BLE TVSELECT client include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_TVSELECT_CLIENT_H
#define APP_BLE_TVSELECT_CLIENT_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

#define APP_BLE_TVSELECT_CLIENT_APP_UUID                 0x8020
#define APP_BLE_TVSELECT_CLIENT_NUM_OF_CLIENT            1

/*******************************************************************************
**
** Function        app_ble_tvselect_open_server
**
** Description     start TVSELECT client
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_open_server(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_client_register_notification
**
** Description     Register notification to receive data from responder
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_client_register_notification(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_close_server
**
** Description     stop initiator
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_close_server(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_client_write_information
**
** Description     Write Miracast information to Responder
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_client_write_information(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_client_close
**
** Description     This is the ble close connection
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_client_close(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_client_deregister
**
** Description     This is the ble deregister app
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_client_deregister(void);

/*******************************************************************************
**
** Function         app_ble_tvselect_client_disc_cback
**
** Description      Discovery callback
**
** Returns          void
**
*******************************************************************************/
void app_ble_tvselect_client_disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data);

/*******************************************************************************
**
** Function        app_ble_tvselect_send_command_cp_state
**
** Description     Write data to tvselect Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_send_command_cp_state(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_send_command_cp_version
**
** Description     Write data to tvselect Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_send_command_cp_version(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_send_command_cp_channel_name
**
** Description     Write data to tvselect Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_send_command_cp_channel_name(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_send_command_cp_channel_icon
**
** Description     Write data to tvselect Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_send_command_cp_channel_icon(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_send_command_cp_media_name
**
** Description     Write data to tvselect Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_send_command_cp_media_name(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_send_command_cp_media_icon
**
** Description     Write data to tvselect Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_send_command_cp_media_icon(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_client_send_command
**
** Description     Write Miracast information to Responder
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_client_send_command(UINT8 *p, int len);

#endif
