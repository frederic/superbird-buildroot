/*****************************************************************************
**
**  Name:           app_ble_pm.c
**
**  Description:    Bluetooth BLE Proximity Monitor application
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
#include "app_ble_pm.h"

/*
 * Global Variables
 */
/* BLE heart rate collector app UUID */
#define APP_BLE_PM_APP_UUID      0x9998
static tBT_UUID app_ble_pm_app_uuid = {2, {APP_BLE_PM_APP_UUID}};


/*
 * Local functions
 */


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
int app_ble_pm_main(void)
{
    int ret_value;

    /* register PM app */
    ret_value = app_ble_pm_register();
    if(ret_value < 0)
    {
        APP_ERROR0("app_ble_pm_register returns error");
    }
    return 0;
}

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
int app_ble_pm_register(void)
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

    ble_appreg_param.uuid = app_ble_pm_app_uuid;
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
 ** Function        app_ble_pm_write_linkloss_alert_level
 **
 ** Description     Configure PM alert level
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_pm_write_linkloss_alert_level(void)
{
    int client_num, alert_level;
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_pm_write_linkloss_alert_level : Invalid client number entered");
        return -1;
    }

    if(app_ble_cb.ble_client[client_num].write_pending)
    {
        APP_ERROR0("app_ble_pm_write_linkloss_alert_level failed : write pending!");
        return -1;
    }

    APP_INFO0("Select Alert Level(0:No Alert, 1:Mid Alert, 2:High Alert:");
    alert_level = app_get_choice("Select");

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_LINKLOSS;
    char_id = BSA_BLE_GATT_UUID_ALERT_LEVEL;

    ble_write_param.char_id.char_id.uuid.len=2;
    ble_write_param.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.len = 1;
    ble_write_param.value[0] = alert_level;
    ble_write_param.descr = FALSE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client_num].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client_num].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}

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
int app_ble_pm_write_immediate_alert_level(void)
{
    int client_num, alert_level;
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_pm_write_immediate_alert_level : Invalid client number entered");
        return -1;
    }

    if(app_ble_cb.ble_client[client_num].write_pending)
    {
        APP_ERROR0("app_ble_pm_write_immediate_alert_level failed : write pending!");
        return -1;
    }

    APP_INFO0("Select Alert Level(0:No Alert, 1:Mid Alert, 2:High Alert:");
    alert_level = app_get_choice("Select");

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_IMMEDIATE_ALERT;
    char_id = BSA_BLE_GATT_UUID_ALERT_LEVEL;

    ble_write_param.char_id.char_id.uuid.len=2;
    ble_write_param.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.len = 1;
    ble_write_param.value[0] = alert_level;
    ble_write_param.descr = FALSE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE_NO_RSP;

    app_ble_cb.ble_client[client_num].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client_num].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}



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
int app_ble_pm_read_tx_power(void)
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
        APP_ERROR0("app_ble_pm_read_tx_power : Invalid client number entered");
        return -1;
    }

    if(app_ble_cb.ble_client[client_num].read_pending)
    {
        APP_ERROR0("app_ble_pm_read_tx_power failed : read pending!");
        return -1;
    }

    service = BSA_BLE_UUID_SERVCLASS_TX_POWER;
    char_id = BSA_BLE_GATT_UUID_TX_POWER_LEVEL;

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
    app_ble_cb.ble_client[client_num].read_pending = TRUE;

    status = BSA_BleClRead(&ble_read_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client_num].read_pending = FALSE;
        APP_ERROR1("app_ble_client_read failed status = %d", status);
        return -1;
    }
    return 0;
}

