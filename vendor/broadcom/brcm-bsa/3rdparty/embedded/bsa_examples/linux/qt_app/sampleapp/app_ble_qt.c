/*****************************************************************************
**
**  Name:           app_ble_qt.c
**
**  Description:    Bluetooth BLE Server general application
**
**  Copyright (c) 2015-2016, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "app_ble.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_dm.h"
#include "app_ble_qt.h"
#include "app_ble_server.h"

#include "app_ble_client_db.h"
#include "app_ble_client.h"
#if (defined APP_BLE_OTA_FW_DL_INCLUDED) && (APP_BLE_OTA_FW_DL_INCLUDED == TRUE)
#include "app_ble_client_otafwdl.h"
#endif

void app_ble_server_profile_cback_qt(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);
extern int bleGetAttrValue(tBSA_BLE_MSG* p_data, UINT8 attribute_value[], int max);
extern void Wake();
extern void BleServerCb(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);
extern void bleSetCharAttrId(int index, int attr_index,int attr_id);
extern void app_ble_server_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);


/*
 * BLE Server functions
 */
/*******************************************************************************
 **
 ** Function        app_ble_server_register_qt
 **
 ** Description     Register server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_register_qt(UINT16 uuid, tBSA_BLE_CBACK *p_cback)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_REGISTER ble_register_param;
    int server_num;

    server_num = app_ble_server_find_free_server();
    if (server_num < 0)
    {
        APP_ERROR0("No more spaces!!");
        return -1;
    }

    status = BSA_BleSeAppRegisterInit(&ble_register_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppRegisterInit failed status = %d", status);
        return -1;
    }

    ble_register_param.uuid.len = 2;
    ble_register_param.uuid.uu.uuid16 = uuid;
    if (p_cback == NULL)
    {
        ble_register_param.p_cback = app_ble_server_profile_cback_qt;
    }
    else
    {
        ble_register_param.p_cback = p_cback;
    }

    status = BSA_BleSeAppRegister(&ble_register_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppRegister failed status = %d", status);
        return -1;
    }
    app_ble_cb.ble_server[server_num].enabled = TRUE;
    app_ble_cb.ble_server[server_num].server_if = ble_register_param.server_if;
    APP_INFO1("enabled:%d, server_if:%d", app_ble_cb.ble_server[server_num].enabled,
                    app_ble_cb.ble_server[server_num].server_if);
    return server_num;
}


/*******************************************************************************
 **
 ** Function        app_ble_server_deregister_qt
 **
 ** Description     Deregister server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_deregister_qt(int num)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_DEREGISTER ble_deregister_param;

    if (app_ble_cb.ble_server[num].enabled != TRUE)
    {
        APP_ERROR1("Server was not registered! = %d", num);
        return -1;
    }

    status = BSA_BleSeAppDeregisterInit(&ble_deregister_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppDeregisterInit failed status = %d", status);
        return -1;
    }

    ble_deregister_param.server_if = app_ble_cb.ble_server[num].server_if;

    status = BSA_BleSeAppDeregister(&ble_deregister_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppDeregister failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_deregister_all
 **
 ** Description     Deregister all server apps
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
void app_ble_server_deregister_all()
{
    int i;
    for (i= 0; i < BSA_BLE_SERVER_MAX; i++)
        app_ble_server_deregister_qt(i);
}

/*******************************************************************************
 **
 ** Function        app_ble_server_create_service_qt
 **
 ** Description     create service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_create_service_qt(int server_num, UINT16 service, UINT16  num_handle, int is_primary)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_CREATE ble_create_param;
    int attr_num=-1;

    attr_num = app_ble_server_find_free_attr(server_num);
    if (attr_num < 0)
    {
        APP_ERROR1("Wrong attr number! = %d", attr_num);
        return -1;
    }
    status = BSA_BleSeCreateServiceInit(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeCreateServiceInit failed status = %d", status);
        return -1;
    }

    ble_create_param.service_uuid.uu.uuid16 = service;
    ble_create_param.service_uuid.len = 2;
    ble_create_param.server_if = app_ble_cb.ble_server[server_num].server_if;
    ble_create_param.num_handle = num_handle;
    if (is_primary != 0)
    {
        ble_create_param.is_primary = TRUE;
    }
    else
    {
        ble_create_param.is_primary = FALSE;
    }

    app_ble_cb.ble_server[server_num].attr[attr_num].wait_flag = TRUE;

    status = BSA_BleSeCreateService(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeCreateService failed status = %d", status);
        app_ble_cb.ble_server[server_num].attr[attr_num].wait_flag = FALSE;
        return -1;
    }

    /* store information on control block */
    app_ble_cb.ble_server[server_num].attr[attr_num].attr_UUID.len = 2;
    app_ble_cb.ble_server[server_num].attr[attr_num].attr_UUID.uu.uuid16 = service;
    app_ble_cb.ble_server[server_num].attr[attr_num].is_pri = ble_create_param.is_primary;
    app_ble_cb.ble_server[server_num].attr[attr_num].attr_type = BSA_GATTC_ATTR_TYPE_SRVC;
    return attr_num;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_add_char_qt
 **
 ** Description     Add character to service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_add_char_qt(int server_num,int srvc_attr_num,UINT16 char_uuid,int attribute_permission,int is_descript,int characteristic_property )
{
    tBSA_STATUS status;
    tBSA_BLE_SE_ADDCHAR ble_addchar_param;

    int char_attr_num;

    char_attr_num = app_ble_server_find_free_attr(server_num);

    status = BSA_BleSeAddCharInit(&ble_addchar_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAddCharInit failed status = %d", status);
        return -1;
    }
    ble_addchar_param.service_id = app_ble_cb.ble_server[server_num].attr[srvc_attr_num].service_id;
    ble_addchar_param.char_uuid.len = 2;
    ble_addchar_param.char_uuid.uu.uuid16 = char_uuid;

    if(is_descript)
    {
        ble_addchar_param.is_descr = TRUE;

        // Attribute Permissions[Read-0x1, Write-0x10, Read|Write-0x11]
        ble_addchar_param.perm = attribute_permission;
    }
    else
    {
        ble_addchar_param.is_descr = FALSE;

        ble_addchar_param.perm = attribute_permission;

        // READ|WRITE|NOTIFY|INDICATE 0x3A
        ble_addchar_param.property = characteristic_property;
    }
    app_ble_cb.ble_server[server_num].attr[char_attr_num].wait_flag = TRUE;
    status = BSA_BleSeAddChar(&ble_addchar_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAddChar failed status = %d", status);
        return -1;
    }

    /* save all information */
    app_ble_cb.ble_server[server_num].attr[char_attr_num].attr_UUID.len = 2;
    app_ble_cb.ble_server[server_num].attr[char_attr_num].attr_UUID.uu.uuid16 = char_uuid;
    app_ble_cb.ble_server[server_num].attr[char_attr_num].prop = characteristic_property;
    app_ble_cb.ble_server[server_num].attr[char_attr_num].attr_type = BSA_GATTC_ATTR_TYPE_CHAR;

    return char_attr_num;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_start_service_qt
 **
 ** Description     Start Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_start_service_qt(int num,int attr_num)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_START ble_start_param;

    status = BSA_BleSeStartServiceInit(&ble_start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeStartServiceInit failed status = %d", status);
        return -1;
    }

    ble_start_param.service_id = app_ble_cb.ble_server[num].attr[attr_num].service_id;
    ble_start_param.sup_transport = BSA_BLE_GATT_TRANSPORT_LE_BR_EDR;

    APP_INFO1("service_id:%d, num:%d", ble_start_param.service_id, num);

    status = BSA_BleSeStartService(&ble_start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeStartService failed status = %d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_stop_service_qt
 **
 ** Description     Stop Service
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_stop_service_qt(int num, int attr_num)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_STOP ble_stop_param;

    status = BSA_BleSeStopServiceInit(&ble_stop_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeStopServiceInit failed status = %d", status);
        return -1;
    }

    ble_stop_param.service_id = app_ble_cb.ble_server[num].attr[attr_num].service_id;

    APP_INFO1("service_id:%d, num:%d", ble_stop_param.service_id, num);

    status = BSA_BleSeStopService(&ble_stop_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeStopService failed status = %d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_send_indication_qt
 **
 ** Description     Send indication to client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_send_indication_qt(int num, int length_of_data, int attr_num, UINT8 *data)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_SENDIND ble_sendind_param;
    int index,len = sizeof(ble_sendind_param.value);

    status = BSA_BleSeSendIndInit(&ble_sendind_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeSendIndInit failed status = %d", status);
        return -1;
    }

    ble_sendind_param.conn_id = app_ble_cb.ble_server[num].conn_id;
    ble_sendind_param.attr_id = app_ble_cb.ble_server[num].attr[attr_num].attr_id;

    if (length_of_data < len)
        len = length_of_data;

    ble_sendind_param.data_len = len;
    memcpy(ble_sendind_param.value, data,len );

    ble_sendind_param.need_confirm = FALSE;

    status = BSA_BleSeSendInd(&ble_sendind_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeSendInd failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
**
** Function         app_ble_server_profile_cback_qt
**
** Description      BLE Server Profile callback.
**
** Returns          void
**
*******************************************************************************/
void app_ble_server_profile_cback_qt(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    int num, attr_index ;
    tBSA_BLE_SE_SENDRSP send_server_resp;
    UINT8 attribute_value[BSA_BLE_MAX_ATTR_LEN]={0x11,0x22,0x33,0x44};

    switch (event)
    {
    case BSA_BLE_SE_ADDCHAR_EVT:
        APP_INFO1("BSA_BLE_SE_ADDCHAR_EVT status:%d", p_data->ser_addchar.status);
        APP_INFO1("attr_id:0x%x", p_data->ser_addchar.attr_id);
        num = app_ble_server_find_index_by_interface(p_data->ser_addchar.server_if);

        /* search interface number */
        if(num < 0)
        {
            APP_ERROR0("no interface!!");
            break;
        }

        for (attr_index = 0 ; attr_index < BSA_BLE_ATTRIBUTE_MAX ; attr_index++)
        {
            if (app_ble_cb.ble_server[num].attr[attr_index].wait_flag == TRUE)
            {
                APP_INFO1("if_num:%d, attr_num:%d", num, attr_index);
                if (p_data->ser_addchar.status == BSA_SUCCESS)
                {
                    app_ble_cb.ble_server[num].attr[attr_index].service_id = p_data->ser_addchar.service_id;
                    app_ble_cb.ble_server[num].attr[attr_index].attr_id = p_data->ser_addchar.attr_id;
                    app_ble_cb.ble_server[num].attr[attr_index].wait_flag = FALSE;
                    bleSetCharAttrId(num,attr_index,p_data->ser_addchar.attr_id);
                    break;
                }
                else  /* if CREATE fail */
                {
                    memset(&app_ble_cb.ble_server[num].attr[attr_index], 0, sizeof(tAPP_BLE_ATTRIBUTE));
                    break;
                }
            }
        }
        if (attr_index >= BSA_BLE_ATTRIBUTE_MAX)
        {
            APP_ERROR0("BSA_BLE_SE_ADDCHAR_EVT no waiting!!");
            break;
        }
        break;

    case BSA_BLE_SE_READ_EVT:
        APP_INFO1("BSA_BLE_SE_READ_EVT status:%d", p_data->ser_read.status);
        BSA_BleSeSendRspInit(&send_server_resp);
        send_server_resp.conn_id = p_data->ser_read.conn_id;
        send_server_resp.trans_id = p_data->ser_read.trans_id;
        send_server_resp.status = p_data->ser_read.status;
        send_server_resp.handle = p_data->ser_read.handle;
        send_server_resp.offset = p_data->ser_read.offset;
        send_server_resp.auth_req = GATT_AUTH_REQ_NONE;
        send_server_resp.len = bleGetAttrValue(p_data, attribute_value, BSA_BLE_MAX_ATTR_LEN);
        memcpy(send_server_resp.value, attribute_value, send_server_resp.len);
        APP_INFO1("BSA_BLE_SE_READ_EVT: send_server_resp.conn_id:%d, send_server_resp.trans_id:%d", send_server_resp.conn_id, send_server_resp.trans_id, send_server_resp.status);
        APP_INFO1("BSA_BLE_SE_READ_EVT: send_server_resp.status:%d,send_server_resp.auth_req:%d", send_server_resp.status,send_server_resp.auth_req);
        APP_INFO1("BSA_BLE_SE_READ_EVT: send_server_resp.handle:%d, send_server_resp.offset:%d, send_server_resp.len:%d", send_server_resp.handle,send_server_resp.offset,send_server_resp.len );
        BSA_BleSeSendRsp(&send_server_resp);
        break;

    default:
        app_ble_server_profile_cback(event,  p_data);
        break;
    }

    if (event == BSA_BLE_SE_CREATE_EVT || event == BSA_BLE_SE_ADDCHAR_EVT)
        Wake();

    BleServerCb(event, p_data);
}

/*
**  BLE client functions
*/

void app_ble_client_profile_cback_qt(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);

/*******************************************************************************
 **
 ** Function        app_ble_client_register_qt
 **
 ** Description     This is the ble client register command
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_register_qt(UINT16 uuid)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_REGISTER ble_appreg_param;
    int index;

    index = app_ble_client_find_free_space();
    if( index < 0)
    {
        APP_ERROR0("app_ble_client_register maximum number of BLE client already registered");
        return -1;
    }
    status = BSA_BleClAppRegisterInit(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BLE client registration failed with status %d", status);
        return -1;
    }

    ble_appreg_param.uuid.len = 2;
    ble_appreg_param.uuid.uu.uuid16 = uuid;
    ble_appreg_param.p_cback = app_ble_client_profile_cback_qt;

    status = BSA_BleClAppRegister(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppRegister failed status = %d", status);
        return -1;
    }
    app_ble_cb.ble_client[index].enabled = TRUE;
    app_ble_cb.ble_client[index].client_if = ble_appreg_param.client_if;

    return index;
}

/*******************************************************************************
 **
 ** Function        app_ble_client_read_qt
 **
 ** Description     This is the read function to read a characteristec or characteristic descriptor from BLE server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_read_qt(int client_num,int ser_inst_id, int char_inst_id, int is_primary, int is_descript,
                        UINT16 service, UINT16 char_id, UINT16 descr_id)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_READ ble_read_param;

    status = BSA_BleClReadInit(&ble_read_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble_client_read failed status = %d", status);
        return -1;
    }

    if(is_descript == 1)
    {
        ble_read_param.descr_id.char_id.char_id.uuid.len = 2;
        ble_read_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
        ble_read_param.descr_id.char_id.srvc_id.id.uuid.len = 2;
        ble_read_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
        ble_read_param.descr_id.char_id.srvc_id.is_primary = is_primary;
        ble_read_param.descr_id.descr_id.uuid.uu.uuid16 = descr_id;
        ble_read_param.descr_id.descr_id.uuid.len = 2;

        ble_read_param.descr = TRUE;
    }
    else
    {
        ble_read_param.char_id.srvc_id.id.inst_id = ser_inst_id;
        ble_read_param.char_id.srvc_id.id.uuid.uu.uuid16 = service;
        ble_read_param.char_id.srvc_id.id.uuid.len = 2;
        ble_read_param.char_id.srvc_id.is_primary = is_primary;

        ble_read_param.char_id.char_id.inst_id = char_inst_id;
        ble_read_param.char_id.char_id.uuid.uu.uuid16 = char_id;
        ble_read_param.char_id.char_id.uuid.len = 2;
        ble_read_param.descr = FALSE;
    }
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
 ** Function        app_ble_client_write_qt
 **
 ** Description     This is the write function to write a characteristic or characteristic discriptor to BLE server.
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_write_qt(int client_num,int ser_inst_id, int char_inst_id, int is_primary, int is_descript,
                 UINT16 service, UINT16 char_id, UINT16 descr_id, int num_bytes, UINT8 data1, UINT8 data2, int write_type,UINT8 desc_value)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    if(app_ble_cb.ble_client[client_num].write_pending ||
        app_ble_cb.ble_client[client_num].congested)
    {
        APP_ERROR1("fail : write pending(%d), congested(%d)!",
            app_ble_cb.ble_client[client_num].write_pending,
            app_ble_cb.ble_client[client_num].congested);
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
        return -1;
    }

    if(is_descript == 1)
    {
        ble_write_param.descr_id.char_id.char_id.uuid.len=2;
        ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid16 = char_id;
        ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=2;
        ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service;
        ble_write_param.descr_id.char_id.srvc_id.is_primary = is_primary;

        ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = descr_id;
        ble_write_param.descr_id.descr_id.uuid.len = 2;
        ble_write_param.len = 1;
        ble_write_param.value[0] = desc_value;
        ble_write_param.descr = TRUE;
    }
    else
    {
        ble_write_param.value[0] = data1;
        ble_write_param.value[1] = data2;
        ble_write_param.len = num_bytes;
        ble_write_param.descr = FALSE;
        ble_write_param.char_id.srvc_id.id.inst_id= ser_inst_id;
        ble_write_param.char_id.srvc_id.id.uuid.uu.uuid16 = service;
        ble_write_param.char_id.srvc_id.id.uuid.len = 2;
        ble_write_param.char_id.srvc_id.is_primary = is_primary;

        ble_write_param.char_id.char_id.inst_id = char_inst_id;
        ble_write_param.char_id.char_id.uuid.uu.uuid16 = char_id;
        ble_write_param.char_id.char_id.uuid.len = 2;
    }

    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;

    if (write_type == BTA_GATTC_TYPE_WRITE_NO_RSP)
    {
        ble_write_param.write_type = BTA_GATTC_TYPE_WRITE_NO_RSP;
    }
    else if (write_type == BTA_GATTC_TYPE_WRITE)
    {
        ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;
    }
    else
    {
        APP_ERROR1("BSA_BleClWrite failed wrong write_type:%d", write_type);
        return -1;
    }

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
 ** Function        app_ble_client_register_notification_qt
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_register_notification_qt(int client_num, UINT16 service_id, UINT16 character_id,int ser_inst_id, int char_inst_id, int is_primary)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
        return -1;
    }

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 2;
    ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = service_id;
    ble_notireg_param.notification_id.char_id.uuid.len = 2;
    ble_notireg_param.notification_id.char_id.uuid.uu.uuid16 = character_id;
    ble_notireg_param.notification_id.srvc_id.id.inst_id = ser_inst_id;
    ble_notireg_param.notification_id.srvc_id.is_primary = is_primary;
    ble_notireg_param.notification_id.char_id.inst_id = char_inst_id;
    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client_num].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client_num].client_if;

    APP_DEBUG1("size of ble_notireg_param:%d", sizeof(ble_notireg_param));
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
 ** Function        app_ble_client_close_qt
 **
 ** Description     This is the ble close connection
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_close_qt(int client_num)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_CLOSE ble_close_param;

    status = BSA_BleClCloseInit(&ble_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClCLoseInit failed status = %d", status);
        return -1;
    }
    ble_close_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
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
 ** Function        app_ble_client_deregister_qt
 **
 ** Description     This is the ble deregister app
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_deregister_qt(int client_num)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_DEREGISTER ble_appdereg_param;
    status = BSA_BleClAppDeregisterInit(&ble_appdereg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppDeregisterInit failed status = %d", status);
        return -1;
    }
    ble_appdereg_param.client_if = app_ble_cb.ble_client[client_num].client_if;
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
 ** Function        app_ble_client_deregister_notification_qt
 **
 ** Description     This is the deregister function for a notification
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_client_deregister_notification_qt(int client_num, UINT16 all,UINT16 service_id, UINT16 character_id,
                                           int ser_inst_id, int char_inst_id, int is_primary)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFDEREG ble_notidereg_param;

    status = BSA_BleClNotifDeregisterInit(&ble_notidereg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifDeregisterInit failed status = %d", status);
        return -1;
    }
    bdcpy(ble_notidereg_param.bd_addr, app_ble_cb.ble_client[client_num].server_addr);
    ble_notidereg_param.client_if = app_ble_cb.ble_client[client_num].client_if;

    if(all == 1)
    {
        APP_INFO0("Deregister all notifications");
    }
    else if(all == 0)
    {

        ble_notidereg_param.notification_id.srvc_id.id.uuid.len = 2;
        ble_notidereg_param.notification_id.srvc_id.id.uuid.uu.uuid16 = service_id;
        ble_notidereg_param.notification_id.char_id.uuid.len = 2;
        ble_notidereg_param.notification_id.char_id.uuid.uu.uuid16 = character_id;
        ble_notidereg_param.notification_id.srvc_id.id.inst_id = ser_inst_id;
        ble_notidereg_param.notification_id.srvc_id.is_primary = is_primary;
        ble_notidereg_param.notification_id.char_id.inst_id = char_inst_id;
        bdcpy(ble_notidereg_param.bd_addr, app_ble_cb.ble_client[client_num].server_addr);
        ble_notidereg_param.client_if = app_ble_cb.ble_client[client_num].client_if;

        APP_DEBUG1("size of ble_notidereg_param:%d", sizeof(ble_notidereg_param));
    }
    else
    {
        APP_ERROR1("wrong select = %d", all);
        return -1;
    }
    status = BSA_BleClNotifDeregister(&ble_notidereg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifDeregister failed status = %d", status);
        return -1;
    }
    return 0;
}

extern void BleClientCb(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);
extern void app_ble_client_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);

/*******************************************************************************
**
** Function         app_ble_client_profile_cback_qt
**
** Description      BLE Client Profile callback.
**
** Returns          void
**
*******************************************************************************/
void app_ble_client_profile_cback_qt(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    app_ble_client_profile_cback(event, p_data);
    BleClientCb(event, p_data);
}

#define APP_BLE_ENABLE_WAKE_GPIO_VSC 0xFD60
#define APP_BLE_PF_MANU_DATA_CO_ID   0x000F  /* Company ID for BRCM */
#define APP_BLE_PF_MANU_DATA_COND_TYPE 5

/*******************************************************************************
 **
 ** Function        app_ble_wake_configure_qt
 **
 ** Description     Configure for Wake on BLE
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_wake_configure_qt(BOOLEAN enable)
{
    tBSA_BLE_WAKE_CFG bsa_ble_wake_cfg;
    tBSA_BLE_WAKE_ENABLE bsa_ble_wake_enable;
    tBSA_STATUS bsa_status;
    tBSA_TM_VSC bsa_vsc;
    char manu_pattern[] = "WAKEUP";

    bsa_status = BSA_BleWakeCfgInit(&bsa_ble_wake_cfg);

    bsa_ble_wake_cfg.cond.manu_data.company_id = APP_BLE_PF_MANU_DATA_CO_ID;
    memcpy(bsa_ble_wake_cfg.cond.manu_data.pattern, manu_pattern,sizeof(manu_pattern));
    bsa_ble_wake_cfg.cond.manu_data.data_len = sizeof(manu_pattern) - 1;
    bsa_ble_wake_cfg.cond_type = APP_BLE_PF_MANU_DATA_COND_TYPE;

    bsa_status = BSA_BleWakeCfg(&bsa_ble_wake_cfg);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleWakeCfg failed status:%d", bsa_status);
        return(-1);
    }

    bsa_status = BSA_BleWakeEnableInit(&bsa_ble_wake_enable);
    bsa_ble_wake_enable.enable = enable;
    bsa_status = BSA_BleWakeEnable(&bsa_ble_wake_enable);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleWake failed status:%d", bsa_status);
        return(-1);
    }

    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = APP_BLE_ENABLE_WAKE_GPIO_VSC;
    bsa_vsc.length = 1;
    bsa_vsc.data[0] = (enable) ? 1 : 0; /* 1=ENABLE 0=DISABLE */

    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send Enable Wake on BLE GPIO VSC status:%d", bsa_status);
        return(-1);
    }

    return 0;
}
