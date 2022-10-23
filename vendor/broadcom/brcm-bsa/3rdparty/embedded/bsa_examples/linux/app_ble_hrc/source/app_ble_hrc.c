/*****************************************************************************
**
**  Name:           app_ble_hrc.c
**
**  Description:    Bluetooth BLE Heart Rate collector application
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "app_ble.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_tm_vse.h"
#include "app_dm.h"
#include "app_ble_client.h"
#include "app_ble_client_db.h"
#include "app_ble_hrc.h"

/*
 * Global Variables
 */
/* BLE heart rate collector app UUID */
#define APP_BLE_HRC_APP_UUID      0x9999
static tBT_UUID app_ble_hrc_app_uuid = {2, {APP_BLE_HRC_APP_UUID}};


/*
 * Local functions
 */
int app_ble_hrc_main(void);
int app_ble_hrc_register(void);



/*
 * BLE common functions
 */

/*******************************************************************************
 **
 ** Function        app_ble_hrc_main
 **
 ** Description     main function of Heart Rate collector 
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_main(void)
{
    int ret_value;

    /* register HRC app */
    ret_value = app_ble_hrc_register();
    if(ret_value < 0)
    {
        APP_ERROR0("app_ble_hrc_register returns error");
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_hrc_register
 **
 ** Description     Register Heart Rate collector services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_register(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_REGISTER ble_appreg_param;
    int index;

    index = app_ble_client_find_free_space();
    if( index < 0)
    {
        APP_ERROR0("app_ble_client_register no more space");
        return -1;
    }
    status = BSA_BleClAppRegisterInit(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppRegisterInit failed status = %d", status);
    }

    ble_appreg_param.uuid = app_ble_hrc_app_uuid;
    ble_appreg_param.p_cback = app_ble_client_profile_cback;

    status = BSA_BleClAppRegister(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppRegister failed status = %d", status);
        return -1;
    }

    app_ble_cb.ble_client[index].enabled = TRUE;
    app_ble_cb.ble_client[index].client_if = ble_appreg_param.client_if;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_hrc_monitor
 **
 ** Description     Register Heart Rate collector services
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_monitor(int enable)
{
    int client_num;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_hrc_monitor : Invalid client number entered");
        return -1;
    }

    if(enable)
    {
        app_ble_hrc_register_notification(client_num);
        app_ble_hrc_configure_monitor(client_num);
    }
    else /* stop monitoring */
    {
        app_ble_hrc_deregister_notification(client_num);
        app_ble_hrc_configure_stop_monitor(client_num);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_hrc_configure_monitor
 **
 ** Description     
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_configure_monitor(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_HEART_RATE; /* Heart Rate org.bluetooth.service.heart_rate 0x180D Adopted */
    char_id = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT; /* Heart Rate Measurement */

    ble_write_param.descr_id.char_id.char_id.uuid.len=2;
    ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.len = 2;
    ble_write_param.value[0] = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG_ENABLE_NOTI;
    ble_write_param.value[1] = 0;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function        app_ble_hrc_configure_stop_monitor
 **
 ** Description     
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_configure_stop_monitor(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_HEART_RATE; /* Heart Rate org.bluetooth.service.heart_rate 0x180D Adopted */
    char_id = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT; /* Heart Rate Measurement */

    ble_write_param.descr_id.char_id.char_id.uuid.len=2;
    ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.value[0] = 0;
    ble_write_param.value[1] = 0;
    ble_write_param.len = 2;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}



/*******************************************************************************
 **
 ** Function        app_ble_hrc_register_notification
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_register_notification(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    /* register all notification */
    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;

    status = BSA_BleClNotifRegister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegister failed status = %d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_hrc_deregister_notification
 **
 ** Description     This is the deregister function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_deregister_notification(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFDEREG ble_notireg_param;

    status = BSA_BleClNotifDeregisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifDeregisterInit failed status = %d", status);
    }

    /* register all notification */
    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_HEART_RATE_MEASUREMENT;

    status = BSA_BleClNotifDeregister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifDeregister failed status = %d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_hrc_read_body_sensor_location
 **
 ** Description     Read sensor location from BLE heart rate server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_hrc_read_body_sensor_location(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_READ ble_read_param;
    UINT16 service, char_id;
    int client_num;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_hrc_read_body_sensor_location : Invalid client number entered");
        return -1;
    }

    service = BSA_BLE_UUID_SERVCLASS_HEART_RATE;
    char_id = BSA_BLE_GATT_UUID_BODY_SENSOR_LOCATION;

    status = BSA_BleClReadInit(&ble_read_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble_client_read failed status = %d", status);
    }

    ble_read_param.descr = FALSE;
    ble_read_param.char_id.srvc_id.id.inst_id= 0x00;
    ble_read_param.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_read_param.char_id.srvc_id.id.uuid.len = 2;
    ble_read_param.char_id.srvc_id.is_primary = 1;

    ble_read_param.char_id.char_id.inst_id = 0x00;
    ble_read_param.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_read_param.char_id.char_id.uuid.len = 2;
    ble_read_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_read_param.auth_req = 0x00;

    status = BSA_BleClRead(&ble_read_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble_client_read failed status = %d", status);
        return -1;
    }
    return 0;
}

