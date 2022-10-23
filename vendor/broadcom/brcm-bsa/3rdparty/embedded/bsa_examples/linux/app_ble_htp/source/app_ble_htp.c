/*****************************************************************************
**
**  Name:           app_ble_htp.c
**
**  Description:    Bluetooth BLE Health Thermometer Collector application
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
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
#include "app_ble_htp.h"

/*
 * Global Variables
 */
/* BLE Blood Pressure Collector app UUID */
#define APP_BLE_HTP_APP_UUID      0x9995
static tBT_UUID app_ble_htp_app_uuid = {2, {APP_BLE_HTP_APP_UUID}};


/*
 * Local functions
 */
int app_ble_htp_main(void);
int app_ble_htp_register(void);
static int app_ble_htp_register_temperature_measurement_indication(int client);
static int app_ble_htp_deregister_temperature_measurement_indication(int client);
static int app_ble_htp_configure_temperature_measurement(int client, BOOLEAN start);
static int app_ble_htp_register_notification(int client);
static int app_ble_htp_deregister_notification(int client);
static int app_ble_htp_configure_intermediate_temperature(int client, BOOLEAN start);
static int app_ble_htp_register_interval_measurement_indication(int client);
static int app_ble_htp_deregister_interval_measurement_indication(int client);
static int app_ble_htp_configure_interval_measurement(int client, BOOLEAN start);

/*
 * BLE common functions
 */

/*******************************************************************************
 **
 ** Function        app_ble_htp_main
 **
 ** Description     main function of Health Thermometer Collector
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_main(void)
{
    int ret_value;

    /* register htp app */
    ret_value = app_ble_htp_register();
    if(ret_value < 0)
    {
        APP_ERROR0("app_ble_htp_register returns error");
    }
    return 0;
}

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
int app_ble_htp_register(void)
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

    ble_appreg_param.uuid = app_ble_htp_app_uuid;
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
 ** Function        app_ble_htp_read_temperature_type
 **
 ** Description     Read temperature measurement type from BLE Health Thermometer server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_read_temperature_type(void)
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
        APP_ERROR0("app_ble_htp_read_temperature_type : Invalid client number entered");
        return -1;
    }

    service = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    char_id = BSA_BLE_GATT_UUID_TEMPERATURE_TYPE;

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
int app_ble_htp_enable_temperature_measurement(BOOLEAN enable)
{
    int client_num;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_htp_enable_temperature_measurement : Invalid client number entered");
        return -1;
    }

    if(enable)
    {
        app_ble_htp_register_temperature_measurement_indication(client_num);
        app_ble_htp_configure_temperature_measurement(client_num, TRUE);
    }
    else /* stop monitoring */
    {
        app_ble_htp_configure_temperature_measurement(client_num, FALSE);
        app_ble_htp_deregister_temperature_measurement_indication(client_num);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_htp_register_temperature_measurement_indication
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_register_temperature_measurement_indication(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT;

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
 ** Function        app_ble_htp_deregister_temperature_measurement_indication
 **
 ** Description     This is the deregister function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_deregister_temperature_measurement_indication(int client)
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
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT;

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
 ** Function        app_ble_htp_configure_temperature_measurement
 **
 ** Description
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_configure_temperature_measurement(int client, BOOLEAN start)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    if(app_ble_cb.ble_client[client].write_pending)
    {
        APP_ERROR0("app_ble_htp_configure_temperature_measurement failed : write pending!");
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    char_id = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT;

    ble_write_param.descr_id.char_id.char_id.uuid.len=2;
    ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.len = 2;
    if(start)
        ble_write_param.value[0] = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG_ENABLE_INDI;
    ble_write_param.value[1] = 0;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}

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
int app_ble_htp_intermediate_temperature(BOOLEAN enable)
{
    int client_num;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_htp_intermediate_temperature : Invalid client number entered");
        return -1;
    }

    if(enable)
    {
        app_ble_htp_register_notification(client_num);
        app_ble_htp_configure_intermediate_temperature(client_num, TRUE);
    }
    else /* stop monitoring */
    {
        app_ble_htp_configure_intermediate_temperature(client_num, FALSE);
        app_ble_htp_deregister_notification(client_num);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_htp_register_notification
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_register_notification(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_INTERMEDIATE_TEMPERATURE;

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
 ** Function        app_ble_htp_deregister_notification
 **
 ** Description     This is the deregister function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_deregister_notification(int client)
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
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_INTERMEDIATE_TEMPERATURE;

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
 ** Function        app_ble_htp_configure_intermediate_temperature
 **
 ** Description
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_configure_intermediate_temperature(int client, BOOLEAN start)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    if(app_ble_cb.ble_client[client].write_pending)
    {
        APP_ERROR0("app_ble_htp_configure_intermediate_temperature failed : write pending!");
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    char_id = BSA_BLE_GATT_UUID_INTERMEDIATE_TEMPERATURE;

    ble_write_param.descr_id.char_id.char_id.uuid.len=2;
    ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.len = 2;
    if(start)
        ble_write_param.value[0] = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG_ENABLE_NOTI;
    ble_write_param.value[1] = 0;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}

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
int app_ble_htp_read_measurement_interval(void)
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
        APP_ERROR0("app_ble_htp_read_measurement_interval : Invalid client number entered");
        return -1;
    }

    service = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    char_id = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL;

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
int app_ble_htp_read_measurement_interval_range(void)
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
        APP_ERROR0("app_ble_htp_read_measurement_interval_range : Invalid client number entered");
        return -1;
    }

    service = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    char_id = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL;

    status = BSA_BleClReadInit(&ble_read_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble_client_read failed status = %d", status);
    }

    ble_read_param.descr = TRUE;
    ble_read_param.descr_id.char_id.char_id.uuid.len=2;
    ble_read_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_read_param.descr_id.char_id.srvc_id.id.uuid.len=2;
    ble_read_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_read_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_read_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_VALID_RANGE;
    ble_read_param.descr_id.descr_id.uuid.len = 2;
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
int app_ble_htp_write_measurement_interval(void)
{
    int client_num;
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;
    UINT8 measurement_interval[2];

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_htp_write_measurement_interval : Invalid client number entered");
        return -1;
    }

    if(app_ble_cb.ble_client[client_num].write_pending)
    {
        APP_ERROR0("app_ble_htp_write_measurement_interval failed : write pending!");
        return -1;
    }

    APP_INFO0("Enter the interval between consecutive temperature measurements in <0-FFFF> Seconds in byte");
    measurement_interval[0] = app_get_choice("Enter the 1st byte value in Hex");
    measurement_interval[1] = app_get_choice("Enter the 2nd byte value in Hex");

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    char_id = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL;

    ble_write_param.char_id.char_id.uuid.len=2;
    ble_write_param.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.len = 2;
    ble_write_param.value[0] = measurement_interval[0];
    ble_write_param.value[1] = measurement_interval[1];
    ble_write_param.descr = FALSE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NO_MITM;
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
 ** Function        app_ble_htp_enable_interval_measurement
 **
 ** Description     This is the register function to receive a indication
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_htp_enable_interval_measurement(BOOLEAN enable)
{
    int client_num;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_htp_enable_interval_measurement : Invalid client number entered");
        return -1;
    }

    if(enable)
    {
        app_ble_htp_register_interval_measurement_indication(client_num);
        app_ble_htp_configure_interval_measurement(client_num, TRUE);
    }
    else /* stop monitoring */
    {
        app_ble_htp_configure_interval_measurement(client_num, FALSE);
        app_ble_htp_deregister_interval_measurement_indication(client_num);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_htp_register_interval_measurement_indication
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_register_interval_measurement_indication(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL;

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
 ** Function        app_ble_htp_deregister_interval_measurement_indication
 **
 ** Description     This is the deregister function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_deregister_interval_measurement_indication(int client)
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
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL;

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
 ** Function        app_ble_htp_configure_interval_measurement
 **
 ** Description
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_htp_configure_interval_measurement(int client, BOOLEAN start)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 char_id, service;

    if(app_ble_cb.ble_client[client].write_pending)
    {
        APP_ERROR0("app_ble_htp_configure_interval_measurement failed : write pending!");
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    service = BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER;
    char_id = BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL;

    ble_write_param.descr_id.char_id.char_id.uuid.len=2;
    ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.len = 2;
    if(start)
        ble_write_param.value[0] = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG_ENABLE_INDI;
    ble_write_param.value[1] = 0;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}

