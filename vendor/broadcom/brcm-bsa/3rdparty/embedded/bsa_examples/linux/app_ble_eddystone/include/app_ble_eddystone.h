/*****************************************************************************
**
**  Name:           app_ble_eddystone.h
**
**  Description:     Bluetooth BLE Eddystone advertiser application include file
*                    Implements Google Eddysone spec
*                    https://github.com/google/eddystone/blob/master/protocol-specification.md
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_EDDYSTONE_H
#define APP_BLE_EDDYSTONE_H

#include "bsa_api.h"
#include "app_ble.h"
#include "app_ble_client_db.h"
#include "app_ble_server.h"
#include "app_ble_client.h"

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_init
 **
 ** Description     init
 **
 ** Parameters      None
 **
 **
 *******************************************************************************/
void app_ble_eddystone_init(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_uid_adv
 **
 ** Description     start eddystone UID advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_uid_adv(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_url_adv
 **
 ** Description     start eddystone URL advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_url_adv(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_tlm_adv
 **
 ** Description     start eddystone TLM advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_tlm_adv(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_stop_eddystone_adv
 **
 ** Description     stop eddystone UID advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_stop_eddystone_adv(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_uid_service
 **
 ** Description     start eddystone UID service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_uid_service(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_stop_eddystone_uid_service
 **
 ** Description     stop eddystone UID advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_stop_eddystone_uid_service(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_le_client
 **
 ** Description     start LE client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_le_client(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_stop_le_client
 **
 ** Description     stop LE client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_stop_le_client(void);

/*******************************************************************************
 **
 ** Function        app_ble_server_register_eddystone
 **
 ** Description     Register server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_register_eddystone(tBSA_BLE_CBACK *p_cback);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_create_service
 **
 ** Description     create service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_create_service(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_add_char
 **
 ** Description     Add character to service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_add_char(UINT8 *uuid_char, tBSA_BLE_PERM  perm, tBSA_BLE_CHAR_PROP prop);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_service
 **
 ** Description     Start Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_service(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_deregister
 **
 ** Description     Deregister server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_deregister(void);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_send_data_to_eddystone_server
 **
 ** Description     Write data to Eddystone Server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_send_data_to_eddystone_server(void);

/*******************************************************************************
 **
 ** Function         app_ble_eddystone_init_disc_cback
 **
 ** Description      Discovery callback
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_eddystone_init_disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data);

#endif
