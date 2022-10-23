/*****************************************************************************
**
**  Name:           app_cce.c
**
**  Description:    Bluetooth CTN client sample application
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "gki.h"
#include "uipc.h"
#include "bsa_api.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_cce.h"


#define DEFAULT_SEC_MASK (BSA_SEC_AUTHENTICATION | BSA_SEC_ENCRYPTION)

/*
* Types
*/
#define APP_CCE_MAX_CONN    2

typedef struct
{
    tCceCallback    *p_callback;
    BOOLEAN         is_open;
    int             num_conn;
    tAPP_CCE_CONN   conn[APP_CCE_MAX_CONN];
    int             fd;
    tBSA_CNS_HANDLE cns_handle;
} tAPP_CCE_CB;

/*
* Global Variables
*/

tAPP_CCE_CB app_cce_cb;

/*******************************************************************************
**
** Function         app_cce_add_connection
**
** Description      Save a connection when it is opened
**
** Returns          void
**
*******************************************************************************/
static void app_cce_add_connection(tBTM_BD_NAME dev_name, BD_ADDR bd_addr,
    UINT8 inst_id, UINT8 cce_handle)
{
    int i = app_cce_cb.num_conn++;

    APP_INFO1("app_cce_add_connection Device:%s BD Address:%02x:%02x:%02x:%02x:%02x:%02x ID:%d Handle:%d",
        dev_name, bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5], inst_id, cce_handle);

    strncpy((char *)app_cce_cb.conn[i].dev_name, (char *)dev_name, BTM_MAX_REM_BD_NAME_LEN);
    memcpy(app_cce_cb.conn[i].bd_addr, bd_addr, BD_ADDR_LEN);
    app_cce_cb.conn[i].instance_id = inst_id;
    app_cce_cb.conn[i].cce_handle = cce_handle;
}

/*******************************************************************************
**
** Function         app_cce_remove_connection
**
** Description      Remove a connection when it is closed
**
** Returns          void
**
*******************************************************************************/
static void app_cce_remove_connection(UINT8 cce_handle)
{
    int i;

    APP_INFO1("app_cce_remove_connection Handle:%d", cce_handle);

    for (i = 0; i < app_cce_cb.num_conn; i++)
    {
        if (app_cce_cb.conn[i].cce_handle == cce_handle)
        {
            app_cce_cb.num_conn--;
            for (; i < app_cce_cb.num_conn; i++)
            {
                app_cce_cb.conn[i] = app_cce_cb.conn[i + 1];
            }
            break;
        }
    }
}

/*******************************************************************************
**
** Function         app_cce_cback
**
** Description      Callback function to handle CCE events from BSA
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_cce_cback(tBSA_CCE_EVT event, tBSA_CCE_MSG *p_data)
{
    int i;

    switch (event)
    {
    case BSA_CCE_ENABLE_EVT:
        APP_INFO1("BSA_CCE_ENABLE_EVT Status %d", p_data->status);
        break;
    case BSA_CCE_DISABLE_EVT:
        APP_INFO1("BSA_CCE_DISABLE_EVT Status %d", p_data->status);
        break;
    case BSA_CCE_GET_ACCOUNTS_EVT:
        APP_INFO1("BSA_CCE_GET_ACCOUNTS_EVT Status %d", p_data->get_accounts.status);
        if(p_data->get_accounts.status == BSA_SUCCESS)
        {
            for (i = 0; i < p_data->get_accounts.num_rec; i++)
            {
                APP_INFO1("  Name:%s  Instance ID:%d", p_data->get_accounts.rec[i].name,
                    p_data->get_accounts.rec[i].inst_id);
            }
        }
        break;
    case BSA_CCE_OPEN_EVT:
        APP_INFO1("BSA_CCE_OPEN_EVT Status %d", p_data->open.status);
        if (p_data->open.status == BSA_SUCCESS)
        {
            app_cce_add_connection(p_data->open.dev_name, p_data->open.bd_addr,
                p_data->open.inst_id, p_data->open.cce_handle);
        }
        break;
    case BSA_CCE_CLOSE_EVT:
        APP_INFO1("BSA_CCE_CLOSE_EVT Status %d", p_data->close.status);
        app_cce_remove_connection(p_data->close.cce_handle);
        break;
    case BSA_CCE_GET_LISTING_EVT:
        APP_INFO1("BSA_CCE_GET_LISTING_EVT Status %d", p_data->get_listing.status);
        if(p_data->get_listing.status == BSA_SUCCESS)
        {
            APP_INFO1("Data length:%d Final:%d List size:%d CSETime:%s", p_data->get_listing.data_len,
                p_data->get_listing.is_final, p_data->get_listing.param.list_size,
                (char *)p_data->get_listing.param.ltime);
            APP_INFO1("%s", (char *)p_data->get_listing.p_data);
        }
        else
            APP_ERROR1("OBEX error code:0x%02x", p_data->get_listing.obx_rsp_code);
        break;
    case BSA_CCE_GET_OBJECT_EVT:
        APP_INFO1("BSA_CCE_GET_OBJECT_EVT Status %d", p_data->get_object.status);
        if(p_data->get_object.status == BSA_SUCCESS)
        {
            APP_INFO1("Data length:%d Final:%d", p_data->get_object.data_len,
                p_data->get_object.is_final);
            APP_INFO1("%s", (char *)p_data->get_object.p_data);
        }
        else
            APP_ERROR1("OBEX error code:0x%02x", p_data->get_object.obx_rsp_code);
        break;
    case BSA_CCE_SET_STATUS_EVT:
        APP_INFO1("BSA_CCE_SET_STATUS_EVT Status %d OBEX rsp 0x%02x", p_data->set_status.status,
            p_data->set_status.obx_rsp_code);
        break;
    case BSA_CCE_PUSH_DATA_REQ_EVT:
        APP_INFO1("BSA_CCE_PUSH_DATA_REQ_EVT Bytes %d", p_data->push_data_req.nbytes);
        if(app_cce_cb.fd > 0)
        {
            UINT8 *p_buf = GKI_getbuf(p_data->push_data_req.nbytes);
            if (p_buf)
            {
                int nbytes = read(app_cce_cb.fd, p_buf, p_data->push_data_req.nbytes);
                if (nbytes >= 0)
                {
                    tBSA_CCE_PUSH_OBJECT_DATA push_data;
                    BSA_CcePushObjectDataInit(&push_data);
                    push_data.cce_handle = p_data->push_data_req.cce_handle;
                    push_data.p_data = p_buf;
                    push_data.data_len = nbytes;
                    push_data.is_final = nbytes < p_data->push_data_req.nbytes ? TRUE : FALSE;
                    BSA_CcePushObjectData(&push_data);
                }
                else
                    APP_ERROR1("Error reading file errno %x", errno);
                GKI_freebuf(p_buf);
            }
        }
        break;
    case BSA_CCE_PUSH_OBJECT_EVT:
        APP_INFO1("BSA_CCE_PUSH_OBJECT_EVT Status %d", p_data->push_object.status);
        if(p_data->push_object.status == BSA_SUCCESS)
        {
            UINT8* p = p_data->push_object.obj_handle;
            APP_INFO1("Object handle:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10],
                p[11], p[12], p[13], p[14], p[15]);
        }
        if(app_cce_cb.fd > 0)
        {
            close(app_cce_cb.fd);
            app_cce_cb.fd = 0;
        }
        break;
    case BSA_CCE_FORWARD_OBJECT_EVT:
        APP_INFO1("BSA_CCE_FORWARD_OBJECT_EVT Status %d", p_data->forward_object.status);
        break;
    case BSA_CCE_GET_ACCOUNT_INFO_EVT:
        APP_INFO1("BSA_CCE_GET_ACCOUNT_INFO_EVT Status %d", p_data->get_account_info.status);
        if(p_data->get_account_info.status == BSA_SUCCESS)
            APP_INFO1("Account info:%s", (char *)p_data->get_account_info.account_info);
        else
            APP_ERROR1("OBEX error code:0x%02x", p_data->get_account_info.obx_rsp_code);
        break;
    case BSA_CCE_SYNC_ACCOUNT_EVT:
        APP_INFO1("BSA_CCE_SYNC_ACCOUNT_EVT Status %d", p_data->sync_account.status);
        break;
    case BSA_CCE_NOTIF_REG_EVT:
        APP_INFO1("BSA_CCE_NOTIF_REG_EVT Status %d", p_data->notif_reg.status);
        break;
    case BSA_CCE_CNS_START_EVT:
        APP_INFO1("BSA_CCE_CNS_START_EVT Status %d", p_data->cns_start.status);
        if(p_data->cns_start.status == BSA_SUCCESS)
        {
            app_cce_cb.cns_handle = p_data->cns_start.cns_handle;
        }
        break;
    case BSA_CCE_CNS_STOP_EVT:
        APP_INFO1("BSA_CCE_CNS_STOP_EVT Status %d", p_data->cns_stop.status);
        break;
    case BSA_CCE_CNS_OPEN_EVT:
        APP_INFO1("BSA_CCE_CNS_OPEN_EVT Status %d", p_data->cns_open.status);
        if(p_data->cns_open.status == BSA_SUCCESS)
        {
            APP_INFO1("Connected Device:%s  BD Address:%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->cns_open.dev_name, p_data->cns_open.bd_addr[0], p_data->cns_open.bd_addr[1],
                p_data->cns_open.bd_addr[2], p_data->cns_open.bd_addr[3], p_data->cns_open.bd_addr[4],
                p_data->cns_open.bd_addr[5]);
        }
        break;
    case BSA_CCE_CNS_CLOSE_EVT:
        APP_INFO1("BSA_CCE_CNS_CLOSE_EVT Status %d", p_data->cns_close.status);
        break;
    case BSA_CCE_CNS_NOTIF_EVT:
        APP_INFO1("BSA_CCE_CNS_NOTIF_EVT Status %d", p_data->cns_notif.status);
        if(p_data->cns_notif.status == BSA_SUCCESS)
        {
            APP_INFO1("Notif from instance %d Data length:%d Final:%d", p_data->cns_notif.inst_id,
                p_data->cns_notif.data_len, p_data->cns_notif.is_final);
            APP_INFO1("%s", (char *)p_data->cns_notif.p_data);
        }
        break;
    default:
        fprintf(stderr, "app_cce_cback unknown event:%d\n", event);
        break;
    }

    /* forward the callback to registered applications */
    if(app_cce_cb.p_callback)
        app_cce_cb.p_callback(event, p_data);
}


/*******************************************************************************
**
** Function         app_cce_enable
**
** Description      Enable CCE functionality
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_enable(tCceCallback *pcb)
{
    tBSA_STATUS status;
    tBSA_CCE_ENABLE enable;

    APP_INFO0("app_cce_enable");

    /* Initialize the control structure */
    memset(&app_cce_cb, 0, sizeof(app_cce_cb));
    app_cce_cb.p_callback = pcb;

    BSA_CceEnableInit(&enable);
    enable.p_cback = app_cce_cback;
    status = BSA_CceEnable(&enable);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable CCE status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_disable
**
** Description      Stop CCE functionality
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_disable(void)
{
    tBSA_STATUS status;
    tBSA_CCE_DISABLE disable;

    APP_INFO0("app_cce_disable");

    app_cce_close_all();

    BSA_CceDisableInit(&disable);
    status = BSA_CceDisable(&disable);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_get_accounts
**
** Description      Get all available accounts on a CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_accounts(BD_ADDR bd_addr)
{
    tBSA_STATUS status;
    tBSA_CCE_GET_ACCOUNTS get;

    APP_INFO0("app_cce_get_accounts");

    BSA_CceGetAccountsInit(&get);
    bdcpy(get.bd_addr, bd_addr);
    status = BSA_CceGetAccounts(&get);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_open
**
** Description      Open a connection to a CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_open(BD_ADDR bda, UINT8 instance_id)
{
    tBSA_STATUS status;
    tBSA_CCE_OPEN open;

    APP_INFO0("app_cce_open");

    BSA_CceOpenInit(&open);
    bdcpy(open.bd_addr, bda);
    open.instance_id = instance_id;
    open.sec_mask = DEFAULT_SEC_MASK;
    status = BSA_CceOpen(&open);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_close
**
** Description      Close a CTN connection
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_close(UINT8 cce_handle)
{
    tBSA_STATUS status;
    tBSA_CCE_CLOSE close;

    APP_INFO0("app_cce_close");

    BSA_CceCloseInit(&close);
    close.cce_handle = cce_handle;
    status = BSA_CceClose(&close);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_close_all
**
** Description      Close all CTN connections
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_close_all()
{
    int i;

    APP_INFO0("app_cce_close_all");

    for (i = 0; i < app_cce_cb.num_conn; i++)
    {
        app_cce_close(app_cce_cb.conn[i].cce_handle);
    }

    return BSA_SUCCESS;
}

/*******************************************************************************
**
** Function         app_cce_get_obj_list
**
** Description      Get object listing from the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_obj_list(UINT8 cce_handle, UINT8 obj_type)
{
    tBSA_STATUS status;
    tBSA_CCE_GET_LISTING get;

    APP_INFO0("app_cce_get_obj_list");

    BSA_CceGetListingInit(&get);
    get.cce_handle = cce_handle;
    get.obj_type = obj_type;
    get.max_list_cnt = 1024;
    status = BSA_CceGetListing(&get);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         hex_str_to_128bit_handle
**
** Description      Convert a hex string to a 128 bit object handle in Big Endian
**                  format
**
** Returns          void
**
*******************************************************************************/
void hex_str_to_128bit_handle(char *p_hex_str, tBSA_CCE_OBJ_HANDLE handle)
{
    size_t str_len, sub_str_len;
    char   *p_str;
    char   tmp[9];
    UINT32 ul;
    UINT8  *p;

    p_str = p_hex_str;
    str_len = strlen(p_hex_str);
    memset(handle, 0, sizeof(tBSA_CCE_OBJ_HANDLE));

    if (str_len > 24)
    {
        /* most significant 4 bytes */
        sub_str_len = str_len - 24;
        memcpy(tmp, p_str, sub_str_len);
        tmp[sub_str_len] = 0;
        ul = strtoul(tmp, NULL, 16);
        p = &handle[0];
        UINT32_TO_BE_STREAM(p, ul);
        str_len -= sub_str_len;
        p_str += sub_str_len;
    }

    if (str_len > 16)
    {
        /* second 4 bytes */
        sub_str_len = str_len - 16;
        memcpy(tmp, p_str, sub_str_len);
        tmp[sub_str_len] = 0;
        ul = strtoul(tmp, NULL, 16);
        p = &handle[4];
        UINT32_TO_BE_STREAM(p, ul);
        str_len -= sub_str_len;
        p_str += sub_str_len;
    }

    if (str_len > 8)
    {
        /* third 4 bytes */
        sub_str_len = str_len - 8;
        memcpy(tmp, p_str, sub_str_len);
        tmp[sub_str_len] = 0;
        ul = strtoul(tmp, NULL, 16);
        p = &handle[8];
        UINT32_TO_BE_STREAM(p, ul);
        str_len -= sub_str_len;
        p_str += sub_str_len;
    }

    if (str_len > 0)
    {
        /* least significant 4 bytes */
        memcpy(tmp, p_str, str_len);
        tmp[str_len] = 0;
        ul = strtoul(tmp, NULL, 16);
        p = &handle[12];
        UINT32_TO_BE_STREAM(p, ul);
    }
}

/*******************************************************************************
**
** Function         app_cce_get_obj
**
** Description      Get an object from the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_obj(UINT8 cce_handle, char *obj_handle, BOOLEAN recurrent)
{
    tBSA_STATUS status;
    tBSA_CCE_GET_OBJECT get;

    APP_INFO0("app_cce_get_obj");

    BSA_CceGetObjectInit(&get);
    get.cce_handle = cce_handle;
    hex_str_to_128bit_handle(obj_handle, get.obj_handle);
    get.recurrent = recurrent;
    status = BSA_CceGetObject(&get);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_set_obj_status
**
** Description      Set status of an object on the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_set_obj_status(UINT8 cce_handle, char *obj_handle,
        UINT8 indicator, UINT8 value, UINT32 postpone_val)
{
    tBSA_STATUS status;
    tBSA_CCE_SET_STATUS set;

    APP_INFO0("app_cce_set_obj_status");

    BSA_CceSetStatusInit(&set);
    set.cce_handle = cce_handle;
    hex_str_to_128bit_handle(obj_handle, set.obj_handle);
    set.status_ind = indicator;
    set.status_val = value;
    set.postpone_val = postpone_val;
    status = BSA_CceSetStatus(&set);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_push_obj
**
** Description      Push an object to the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_push_obj(UINT8 cce_handle, UINT8 obj_type, BOOLEAN send,
        char *file_name)
{
    tBSA_STATUS status;
    tBSA_CCE_PUSH_OBJECT push;

    APP_INFO0("app_cce_push_obj");

    app_cce_cb.fd = open(file_name, O_RDONLY);
    if(app_cce_cb.fd <= 0)
    {
        APP_ERROR1("Error could not open input file %s", file_name);
        return BSA_ERROR_CLI_BAD_PARAM;
    }

    BSA_CcePushObjectInit(&push);
    push.cce_handle = cce_handle;
    push.obj_type = obj_type;
    push.send = send;
    strncpy(push.path, file_name, sizeof(push.path) - 1);
    status = BSA_CcePushObject(&push);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_forward_obj
**
** Description      Forward an object to one or more additional recipients
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_forward_obj(UINT8 cce_handle, char *obj_handle,
        char *recipients)
{
    tBSA_STATUS status;
    tBSA_CCE_FORWARD_OBJECT fwd;

    APP_INFO0("app_cce_forward_obj");

    BSA_CceForwardObjectInit(&fwd);
    fwd.cce_handle = cce_handle;
    hex_str_to_128bit_handle(obj_handle, fwd.obj_handle);
    strncpy(fwd.recipients, recipients, sizeof(fwd.recipients) - 1);
    status = BSA_CceForwardObject(&fwd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_get_account_info
**
** Description      Get CAS info for the specified CAS instance ID
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_account_info(UINT8 cce_handle, UINT8 instance_id)
{
    tBSA_STATUS status;
    tBSA_CCE_GET_ACCOUNT_INFO get;

    APP_INFO0("app_cce_get_account_info");

    BSA_CceGetAccountInfoInit(&get);
    get.cce_handle = cce_handle;
    get.instance_id = instance_id;
    status = BSA_CceGetAccountInfo(&get);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_sync_account
**
** Description      Synchronize CTN server account with an external server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_sync_account(UINT8 cce_handle)
{
    tBSA_STATUS status;
    tBSA_CCE_SYNC_ACCOUNT sync;

    APP_INFO0("app_cce_sync_account");

    BSA_CceSyncAccountInit(&sync);
    sync.cce_handle = cce_handle;
    status = BSA_CceSyncAccount(&sync);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_cns_start
**
** Description      Start CTN notification service
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_cns_start(char *service_name)
{
    tBSA_STATUS status;
    tBSA_CCE_CNS_START start;

    APP_INFO0("app_cce_cns_start");

    BSA_CceCnsStartInit(&start);
    strncpy(start.service_name, service_name, sizeof(start.service_name) - 1);
    start.sec_mask = DEFAULT_SEC_MASK;
    start.features = BSA_CCE_SUP_FEA_NOTIF;
    status = BSA_CceCnsStart(&start);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_cns_stop
**
** Description      Stop CTN notification service
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_cns_stop()
{
    tBSA_STATUS status;
    tBSA_CCE_CNS_STOP stop;

    APP_INFO0("app_cce_cns_stop");

    BSA_CceCnsStopInit(&stop);
    stop.cns_handle = app_cce_cb.cns_handle;
    status = BSA_CceCnsStop(&stop);

    app_cce_cb.cns_handle = 0;

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_notif_reg
**
** Description      Register for notification indicating changes in CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_notif_reg(UINT8 cce_handle, BOOLEAN notif_on, BOOLEAN alarm_on)
{
    tBSA_STATUS status;
    tBSA_CCE_NOTIF_REG reg;

    APP_INFO0("app_cce_notif_reg");

    BSA_CceNotifRegInit(&reg);
    reg.cce_handle = cce_handle;
    reg.notif_status = notif_on ? BSA_CCE_NOTIF_ON : BSA_CCE_NOTIF_OFF;
    reg.alarm_status = alarm_on ? BSA_CCE_ACOUSTIC_ALARM_ON : BSA_CCE_ACOUSTIC_ALARM_OFF;
    status = BSA_CceNotifReg(&reg);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Error status:%d", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_cce_get_connections
**
** Description      Get all open connections
**
** Parameter        p_num : returns number of connections
**
** Returns          Pointer to connections
**
*******************************************************************************/
tAPP_CCE_CONN *app_cce_get_connections(UINT8 *p_num)
{
    *p_num = app_cce_cb.num_conn;
    return app_cce_cb.conn;
}



