/*****************************************************************************
**
**  Name:           app_ble_server.h
**
**  Description:    Bluetooth BLE include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_QT_H
#define APP_BLE_QT_H

#include "bsa_api.h"
#include "app_ble.h"

/*
 * BLE Server functions
 */

void app_ble_server_deregister_all();

/*******************************************************************************
 **
 ** Function        app_ble_server_register
 **
 ** Description     Register server app
 **
 ** Parameters      uuid: uuid number of application
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_register_qt(UINT16 uuid, tBSA_BLE_CBACK *p_cback);


/*******************************************************************************
 **
 ** Function        app_ble_server_deregister
 **
 ** Description     Deregister server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_deregister_qt(int num);


/*******************************************************************************
 **
 ** Function        app_ble_server_create_service
 **
 ** Description     create service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_create_service_qt(int server_num, UINT16 service, UINT16  num_handle, int is_primary);

/*******************************************************************************
 **
 ** Function        app_ble_server_add_char
 **
 ** Description     Add charcter to service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_add_char_qt(int server_num,int srvc_attr_num,UINT16 char_uuid,int attribute_permission,int is_descript,int characteristic_property);

/*******************************************************************************
 **
 ** Function        app_ble_server_start_service
 **
 ** Description     Start Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_start_service_qt(int num,int attr_num);

/*******************************************************************************
 **
 ** Function        app_ble_server_stop_service
 **
 ** Description     Stop Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_stop_service_qt(int num, int attr_num);

/*******************************************************************************
 **
 ** Function        app_ble_server_send_indication
 **
 ** Description     Send indication to client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_send_indication_qt(int num, int length_of_data, int attr_num, UINT8 *data);

/*******************************************************************************
**
** Function         app_ble_server_profile_cback
**
** Description      BLE Server Profile callback.
**
** Returns          void
**
*******************************************************************************/
void app_ble_server_profile_cback_qt(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);

/*************************************************************************************/

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"

/*
 * BLE Client functions
 */
/*******************************************************************************
 **
 ** Function        app_ble_client_register
 **
 ** Description     This is the ble client register command
 **
 ** Parameters      uuid: uuid number of application
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_register_qt(UINT16 uuid);


/*******************************************************************************
 **
 ** Function        app_ble_client_read
 **
 ** Description     This is the read function to read a characteristec or characteristic descriptor from BLE server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_read_qt(int client_num,int ser_inst_id, int char_inst_id, int is_primary, int is_descript,
                        UINT16 service, UINT16 char_id, UINT16 descr_id);

/*******************************************************************************
 **
 ** Function        app_ble_client_write
 **
 ** Description     This is the write function to write a characteristic or characteristic discriptor to BLE server.
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_write_qt(int client_num,int ser_inst_id, int char_inst_id, int is_primary, int is_descript,
                 UINT16 service, UINT16 char_id, UINT16 descr_id, int num_bytes, UINT8 data1, UINT8 data2, int write_type,UINT8 desc_value);

/*******************************************************************************
 **
 ** Function        app_ble_client_register_notification
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_register_notification_qt(int client_num, UINT16 service_id, UINT16 character_id,int ser_inst_id, int char_inst_id, int is_primary);


/*******************************************************************************
 **
 ** Function        app_ble_client_close
 **
 ** Description     This is the ble close connection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_close_qt(int);


/*******************************************************************************
 **
 ** Function        app_ble_client_deregister
 **
 ** Description     This is the ble deregister app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_deregister_qt(int client);


/*******************************************************************************
 **
 ** Function        app_ble_client_deregister_notification
 **
 ** Description     This is the deregister function for a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_deregister_notification_qt(int client_num, UINT16 all,UINT16 service_id, UINT16 character_id,
                                           int ser_inst_id, int char_inst_id, int is_primary);

/*******************************************************************************
**
** Function         app_ble_client_profile_cback
**
** Description      BLE Client Profile callback.
**
** Returns          void
**
*******************************************************************************/
void app_ble_client_profile_cback_qt(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);

/*******************************************************************************
 **
 ** Function        app_ble_wake_configure_qt
 **
 ** Description     Configure for Wake on BLE
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wake_configure_qt(BOOLEAN enable);

#endif
