/*****************************************************************************
**
**  Name:           app_ble_wifi_init.c
**
**  Description:    Bluetooth BLE WiFi initiator main application
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "app_ble.h"
#include "app_thread.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_dm.h"
#include "app_ble_client.h"
#include "app_ble_server.h"
#include "app_ble_client_db.h"
#include "app_ble_wifi_init.h"
#include "app_ble_wifi_resp.h"

/*
 * Global Variables
 */
UINT8 app_ble_wifi_init_write_data_value[APP_BLE_WIFI_INIT_INFORMATION_DATA_LEN] =
                   {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00};
tAPP_DISCOVERY_CB app_ble_wifi_init_disc_cb;

static UINT8 app_ble_wifi_init_scan_rsp_value[APP_BLE_WIFI_RESP_SCAN_RSP_VALUE_LEN] =
        APP_BLE_WIFI_RESP_SCAN_RSP_VALUE;

/*
 * Local functions
 */
static BOOLEAN app_ble_wifi_init_find_vs_eir(UINT8 *p_eir);
static void app_ble_wifi_init_display_devices(void);
static int app_ble_wifi_init_open(void);

/*
 * BLE common functions
 */

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
int app_ble_wifi_init_start(void)
{
    int ret_value = 0;

    /* register initiator app */
    ret_value = app_ble_client_register(APP_BLE_WIFI_INIT_APP_UUID);
    if (ret_value < 0)
    {
        APP_ERROR1("app_ble_client_register failed ret_value = %d", ret_value);
        return ret_value;
    }
    /* connect responder */
    ret_value = app_ble_wifi_init_open();
    if (ret_value < 0)
    {
        APP_ERROR1("app_ble_client_open failed ret_value = %d", ret_value);
        return ret_value;
    }

    return ret_value;
}

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
int app_ble_wifi_init_register_notification(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = APP_BLE_WIFI_RESP_SERV_UUID;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = APP_BLE_WIFI_RESP_CHAR_UUID;

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
 ** Function        app_ble_wifi_init_send_data
 **
 ** Description     send wifi data.
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wifi_init_send_data(void)
{
    /* write wifi information to responder */
    app_ble_wifi_init_write_information();

    return 0;
}


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
int app_ble_wifi_init_stop(void)
{
    /* close connection */
    app_ble_wifi_init_close();

    /* Deregister application */
    app_ble_wifi_init_deregister();

    return 0;
}


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
int app_ble_wifi_init_write_information(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    if(app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].write_pending)
    {
        APP_ERROR0("app_ble_wifi_init_write_information failed : write pending!");
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    ble_write_param.char_id.char_id.uuid.len=2;
    ble_write_param.char_id.char_id.uuid.uu.uuid16 = APP_BLE_WIFI_RESP_CHAR_UUID;
    ble_write_param.char_id.srvc_id.id.uuid.len=2;
    ble_write_param.char_id.srvc_id.id.uuid.uu.uuid16 = APP_BLE_WIFI_RESP_SERV_UUID;
    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.len = APP_BLE_WIFI_INIT_INFORMATION_DATA_LEN;
    memcpy(ble_write_param.value, app_ble_wifi_init_write_data_value, APP_BLE_WIFI_INIT_INFORMATION_DATA_LEN);
    ble_write_param.descr = FALSE;
    ble_write_param.conn_id = app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE_NO_RSP;

    app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}

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
int app_ble_wifi_init_close(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_CLOSE ble_close_param;

    if(app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].enabled == FALSE)
    {
        APP_ERROR1("Wrong client number! = %d", APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1);
        return -1;
    }
    status = BSA_BleClCloseInit(&ble_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClCLoseInit failed status = %d", status);
        return -1;
    }
    ble_close_param.conn_id = app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].conn_id;
    status = BSA_BleClClose(&ble_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClClose failed status = %d", status);
        return -1;
    }

    return 0;
}

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
int app_ble_wifi_init_deregister(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_DEREGISTER ble_appdereg_param;

    if(app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].enabled == FALSE)
    {
        APP_ERROR1("Wrong client number! = %d", APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1);
        return -1;
    }
    status = BSA_BleClAppDeregisterInit(&ble_appdereg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppDeregisterInit failed status = %d", status);
        return -1;
    }
    ble_appdereg_param.client_if = app_ble_cb.ble_client[APP_BLE_WIFI_INIT_NUM_OF_CLIENT-1].client_if;
    status = BSA_BleClAppDeregister(&ble_appdereg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleAppDeregister failed status = %d", status);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_ble_wifi_init_disc_cback
 **
 ** Description      Discovery callback
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_wifi_init_disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data)
{
    UINT16 index;

    switch (event)
    {
    /* a New Device has been discovered */
    case BSA_DISC_NEW_EVT:
        APP_INFO1("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->disc_new.bd_addr[0],
                p_data->disc_new.bd_addr[1],
                p_data->disc_new.bd_addr[2],
                p_data->disc_new.bd_addr[3],
                p_data->disc_new.bd_addr[4],
                p_data->disc_new.bd_addr[5]);
        APP_INFO1("\tName:%s", p_data->disc_new.name);

        if (p_data->disc_new.eir_data[0])
        {
            /* discover EIR to find a specific ADV message */
            if (app_ble_wifi_init_find_vs_eir(p_data->disc_new.eir_data))
            {
                /* check if this device has already been received (update) */
                for (index = 0; index < APP_DISC_NB_DEVICES; index++)
                {
                    if ((app_ble_wifi_init_disc_cb.devs[index].in_use == TRUE) &&
                        (!bdcmp(app_ble_wifi_init_disc_cb.devs[index].device.bd_addr, p_data->disc_new.bd_addr)))
                    {
                        /* Update device */
                        APP_INFO1("Update device:%d", index);
                        app_ble_wifi_init_disc_cb.devs[index].device = p_data->disc_new;
                        break;
                    }
                }

                /* if this is new discovered device */
                if (index >= APP_DISC_NB_DEVICES)
                {
                    /* Look for a free place to store dev info */
                    for (index = 0; index < APP_DISC_NB_DEVICES; index++)
                    {
                        if (app_ble_wifi_init_disc_cb.devs[index].in_use == FALSE)
                        {
                            APP_INFO1("New Discovered device:%d", index);
                            app_ble_wifi_init_disc_cb.devs[index].in_use = TRUE;
                            memcpy(&app_ble_wifi_init_disc_cb.devs[index].device, &p_data->disc_new,
                                sizeof(tBSA_DISC_REMOTE_DEV));
                            break;
                        }
                    }
                }
            }
        }
        break;

    case BSA_DISC_CMPL_EVT: /* Discovery complete. */
        break;

    case BSA_DISC_DEV_INFO_EVT: /* Discovery Device Info */
        break;

    default:
        break;
    }
}


/*******************************************************************************
 **
 ** Function         app_ble_wifi_init_find_vs_eir
 **
 ** Description      This function is used to find
 **                   a specific ADV content in EIR data
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_ble_wifi_init_find_vs_eir(UINT8 *p_eir)
{
    UINT8 *p = p_eir;
    UINT8 eir_length;
    UINT8 eir_tag;
    BOOLEAN discovered = FALSE;

    while(1)
    {
        /* Read Tag's length */
        STREAM_TO_UINT8(eir_length, p);
        if (eir_length == 0)
        {
            break;    /* Last Tag */
        }
        eir_length--;

        /* Read Tag Id */
        STREAM_TO_UINT8(eir_tag, p);

        switch(eir_tag)
        {
        case HCI_EIR_MANUFACTURER_SPECIFIC_TYPE:
            if (eir_length == APP_BLE_WIFI_RESP_ADV_VALUE_LEN)
            {
                if (!memcmp(p, &app_ble_wifi_init_scan_rsp_value, APP_BLE_WIFI_RESP_SCAN_RSP_VALUE_LEN))
                {
                    APP_INFO0("Discovered WiFi Responder!");
                    discovered = TRUE;
                }
            }
            APP_DUMP("MANUFACTURER_SPECIFIC_TYPE", p, eir_length);
            break;
        default:
            break;
        }
        p += eir_length;
    }

    return discovered;
}

/*******************************************************************************
 **
 ** Function        app_ble_wifi_init_open
 **
 ** Description     This is the ble open connection to ble server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_wifi_init_open(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_OPEN ble_open_param;
    int device_index;
    BD_ADDR bd_addr;
    int client_num;
    int direct;

    APP_INFO0("Bluetooth BLE connect menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    device_index = app_get_choice("Select source");
    /* Devices from XML database */
    if (device_index == 0)
    {
        /* Read the XML file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_ble_wifi_init_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_DISC_NB_DEVICES) &&
            (app_ble_wifi_init_disc_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_ble_wifi_init_disc_cb.devs[device_index].device.bd_addr);
        }
    }
    else
    {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
        return -1;
    }

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if((client_num < 0) ||
       (client_num >= BSA_BLE_CLIENT_MAX) ||
       (app_ble_cb.ble_client[client_num].enabled == FALSE))
    {
        APP_ERROR1("Wrong client number! = %d", client_num);
        return -1;
    }

    if (app_ble_cb.ble_client[client_num].conn_id != BSA_BLE_INVALID_CONN)
    {
        APP_ERROR1("Connection already exist, conn_id = %d", app_ble_cb.ble_client[client_num].conn_id );
        return -1;
    }

    status = BSA_BleClConnectInit(&ble_open_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClConnectInit failed status = %d", status);
        return -1;
    }

    ble_open_param.client_if = app_ble_cb.ble_client[client_num].client_if;
    direct = app_get_choice("Direct connection:1, Background connection:0");
    if(direct == 1)
    {
        ble_open_param.is_direct = TRUE;
    }
    else if(direct == 0)
    {
        ble_open_param.is_direct = FALSE;
    }
    else
    {
        APP_ERROR1("Wrong selection! = %d", direct);
        return -1;
    }
    bdcpy(app_ble_cb.ble_client[client_num].server_addr, bd_addr);
    bdcpy(ble_open_param.bd_addr, bd_addr);

    status = BSA_BleClConnect(&ble_open_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClConnect failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_ble_wifi_init_display_devices
 **
 ** Description      This function is used to list discovered devices
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_ble_wifi_init_display_devices(void)
{
    int index;

    for (index = 0; index < APP_DISC_NB_DEVICES; index++)
    {
        if (app_ble_wifi_init_disc_cb.devs[index].in_use != FALSE)
        {
            APP_INFO1("Dev:%d", index);
            APP_INFO1("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x",
                    app_ble_wifi_init_disc_cb.devs[index].device.bd_addr[0],
                    app_ble_wifi_init_disc_cb.devs[index].device.bd_addr[1],
                    app_ble_wifi_init_disc_cb.devs[index].device.bd_addr[2],
                    app_ble_wifi_init_disc_cb.devs[index].device.bd_addr[3],
                    app_ble_wifi_init_disc_cb.devs[index].device.bd_addr[4],
                    app_ble_wifi_init_disc_cb.devs[index].device.bd_addr[5]);
            APP_INFO1("\tName:%s", app_ble_wifi_init_disc_cb.devs[index].device.name);
            APP_INFO1("\tClassOfDevice:%02x:%02x:%02x => %s",
                    app_ble_wifi_init_disc_cb.devs[index].device.class_of_device[0],
                    app_ble_wifi_init_disc_cb.devs[index].device.class_of_device[1],
                    app_ble_wifi_init_disc_cb.devs[index].device.class_of_device[2],
                    app_get_cod_string(
                            app_ble_wifi_init_disc_cb.devs[index].device.class_of_device));
            APP_INFO1("\tRssi:%d", app_ble_wifi_init_disc_cb.devs[index].device.rssi);
            if (app_ble_wifi_init_disc_cb.devs[index].device.eir_vid_pid[0].valid)
            {
                APP_INFO1("\tVidSrc:%d Vid:0x%04X Pid:0x%04X Version:0x%04X",
                        app_ble_wifi_init_disc_cb.devs[index].device.eir_vid_pid[0].vendor_id_source,
                        app_ble_wifi_init_disc_cb.devs[index].device.eir_vid_pid[0].vendor,
                        app_ble_wifi_init_disc_cb.devs[index].device.eir_vid_pid[0].product,
                        app_ble_wifi_init_disc_cb.devs[index].device.eir_vid_pid[0].version);
            }
        }
    }
}
