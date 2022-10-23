/*****************************************************************************
 **
 **  Name:           app_hl.c
 **
 **  Description:    Bluetooth Health application
 **
 **  Copyright (c) 2011-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bsa_api.h"

#include "bsa_trace.h"
#include "app_mutex.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_hl.h"
#include "app_hl_main.h"
#include "app_hl_db.h"
#include "app_dm.h"

#define APP_HL_NB_CON_MAX       4   /* Maximum number of connections */
#define APP_HL_BAD_HANDLE       0xFF
#define APP_HL_BAD_UIPC         0xFF

/*
 * Globals
 */

#define APP_HL_PULSE_OXIMETER         0
#define APP_HL_BLOOD_PRESSURE_MON     1
#define APP_HL_BODY_THERMOMETER       2
#define APP_HL_BODY_WEIGHT_SCALE      3
#define APP_HL_GLUCOSE_METER          4
#define APP_HL_PEDOMETER              5
#define APP_HL_STEP_COUNTER           6
#define APP_HL_SPECIALIZATION_MAX     7

#define APP_HL_ROLE_DESC(role)   (role==0?"Source":"Sink")

/* Structure to hold */
typedef struct
{
    UINT16 data_type;   /* IEEE data specialization*/
    char *desc;         /* Description of the specialization*/
    UINT16 tx_mtu;      /* Tx Maximum Transmit Unit (size) */
    UINT16 rx_mtu;      /* Rx Maximum Transmit Unit (size) */
} tAPP_HL_DATA_TYPE_CFG;

/* From Bluetooth SIG's HDP whitepaper */
/* Note that the MTU sizes are on Source point of view */
static const tAPP_HL_DATA_TYPE_CFG data_type_table[APP_HL_SPECIALIZATION_MAX] =
{
    /* Data Specialization                 Description         tx_mtu rx_mtu */
    {APP_HL_DATA_TYPE_PULSE_OXIMETER,     "Pulse Oximeter",    9216,  256},
    {APP_HL_DATA_TYPE_BLOOD_PRESSURE_MON, "Blood Pressure",    896,   224},
    {APP_HL_DATA_TYPE_BODY_THERMOMETER,   "Thermometer",       896,   224},
    {APP_HL_DATA_TYPE_BODY_WEIGHT_SCALE,  "Weight Scale",      896,   224},
    {APP_HL_DATA_TYPE_GLUCOSE_METER,      "Glucose Meter",     896,   224},
    {APP_HL_DATA_TYPE_PEDOMETER,          "Pedometer",         896,   224},
    {APP_HL_DATA_TYPE_STEP_COUNTER,       "Step Counter",      896,   224}
};

/* Maximum Number of peer devices */
#define APP_HL_PEER_DEVICE_MAX      5

/* UIPC Read data size */
#define APP_HL_UIPC_READ_SIZE       1024

/* Structure used to store/save local configured application/service */
typedef struct
{
    BOOLEAN registered; /* TRUE if this application is registered */
    tBSA_HL_REGISTER configuration; /* Local configuration */
} tAPP_HL_APP;

/* Structure used to Health Connection(s) */
typedef struct
{
    BOOLEAN in_use;  /* TRUE if this structure is used */
    BOOLEAN is_open;
    BD_ADDR bd_addr; /* Address of Peer device */
    tUIPC_CH_ID uipc_channel;
    tAPP_MUTEX uipc_tx_mutex;
    UINT16 mtu;
    tBSA_HL_MCL_HANDLE mcl_handle;
    tBSA_HL_DATA_HANDLE data_handle;
} tAPP_HL_CONNECTION;

typedef struct
{
    tAPP_HL_APP local_app[APP_HL_LOCAL_APP_MAX]; /* Local applications */
    tBSA_HL_SDP_QUERY_MSG peer_device[APP_HL_PEER_DEVICE_MAX]; /* Peer device applications */
    tAPP_HL_CONNECTION connections[APP_HL_PEER_DEVICE_MAX]; /* Connections info */
} tAPP_HL_CB;

tAPP_HL_CB app_hl_cb;

/*
 * Local functions
 */
tAPP_HL_CONNECTION *app_hl_con_alloc(void);
void app_hl_con_free(tAPP_HL_CONNECTION *p_connection);
tAPP_HL_CONNECTION *app_hl_con_get_from_data_handle(tBSA_HL_DATA_HANDLE data_handle);
tAPP_HL_CONNECTION *app_hl_con_get_from_uipc(tUIPC_CH_ID channel_id);
int app_hl_send(tAPP_HL_CONNECTION *p_connection, BT_HDR *p_msg);
void app_hl_uipc_cback(BT_HDR *p_msg);



/*******************************************************************************
 **
 ** Function         app_hl_cback
 **
 ** Description      Example of Health callback function
 **
 ** Parameters       event: event
 **                  p_data: pointer of message
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_hl_cback(tBSA_HL_EVT event, tBSA_HL_MSG *p_data)
{
    int record_index;
    int mdep_index;
    int data_type_index;
    tBSA_HL_SDP_REC *p_sdp;
    tBSA_HL_OPEN_RSP hl_open_rsp;
    tAPP_HL_CONNECTION *p_connection;
    int status;
    tBSA_STATUS bsa_status;

    switch (event)
    {
    /* SDP Query result => print the received data */
    case BSA_HL_SDP_QUERY_EVT:
        APP_INFO1("app_hl_cback receive BSA_HL_SDP_QUERY_EVT status:%d", p_data->sdp_query.status);
        APP_INFO1("BdAddr:%02x-%02x-%02x-%02x-%02x-%02x",
                p_data->sdp_query.bd_addr[0],
                p_data->sdp_query.bd_addr[1],
                p_data->sdp_query.bd_addr[2],
                p_data->sdp_query.bd_addr[3],
                p_data->sdp_query.bd_addr[4],
                p_data->sdp_query.bd_addr[5]);
        APP_INFO1("Number of Records:%d", p_data->sdp_query.num_records);
        for (record_index = 0 ; record_index < p_data->sdp_query.num_records ; record_index++)
        {
            p_sdp = &p_data->sdp_query.sdp_records[record_index];
            APP_INFO1("SDP[%d]", record_index);
            APP_INFO1("    Service Name:%s", p_sdp->service_name);
            APP_INFO1("    Service Desc:%s", p_sdp->service_desc);
            APP_INFO1("    Provider Name:%s", p_sdp->provider_name);
            APP_INFO1("    ctrl_psm:%d(0x%x)", p_sdp->ctrl_psm, p_sdp->ctrl_psm);
            APP_INFO1("    mcap_sup_features:0x%d", p_sdp->mcap_sup_features);
            APP_INFO1("    num_mdeps:%d", p_sdp->num_mdeps);
            for (mdep_index = 0 ; mdep_index < p_sdp->num_mdeps ; mdep_index++)
            {
                APP_INFO1("        MdepId:%d Role:%s NumDataType:%d",
                        p_sdp->mdep[mdep_index].mdep_id,
                        APP_HL_ROLE_DESC(p_sdp->mdep[mdep_index].mdep_role),
                        p_sdp->mdep[mdep_index].num_of_mdep_data_types);
                for (data_type_index = 0 ; data_type_index < p_sdp->mdep[mdep_index].num_of_mdep_data_types ; data_type_index++)
                {
                    APP_INFO1("            Type:%x Desc:%s",
                            p_sdp->mdep[mdep_index].data_cfg[data_type_index].data_type,
                            p_sdp->mdep[mdep_index].data_cfg[data_type_index].desc);
                }
            }
        }
        break;

    /* Peer device asks to Open a Data Connection*/
    case BSA_HL_OPEN_REQ_EVT:
        APP_INFO0("BSA_HL_OPEN_REQ_EVT received");
        APP_INFO1("BdAddr:%02x-%02x-%02x-%02x-%02x-%02x",
                p_data->open_req.bd_addr[0],
                p_data->open_req.bd_addr[1],
                p_data->open_req.bd_addr[2],
                p_data->open_req.bd_addr[3],
                p_data->open_req.bd_addr[4],
                p_data->open_req.bd_addr[5]);
        APP_INFO1("    app_handle:%d local_mdep_id:%d channel_cfg:%s(%d) mcl_handle:%d mdl_id:%d",
                p_data->open_req.app_handle,  p_data->open_req.local_mdep_id,
                app_hl_get_channel_cfg_desc(p_data->open_req.channel_cfg),
                p_data->open_req.channel_cfg,
                p_data->open_req.mcl_handle, p_data->open_req.mdl_id);
        APP_INFO0("Accept this connection");
        /* Accept it */
        BSA_HlOpenRspInit(&hl_open_rsp);
        bdcpy(hl_open_rsp.bd_addr, p_data->open_req.bd_addr);
        hl_open_rsp.app_handle =  p_data->open_req.app_handle;
        switch(p_data->open_req.channel_cfg)
        {
        case BTA_HL_DCH_CFG_NO_PREF:
            hl_open_rsp.channel_cfg =  BTA_HL_DCH_CFG_RELIABLE;
            break;
        case BTA_HL_DCH_CFG_RELIABLE:
            hl_open_rsp.channel_cfg =  BTA_HL_DCH_CFG_RELIABLE;
            break;
        case BTA_HL_DCH_CFG_STREAMING:
            hl_open_rsp.channel_cfg =  BTA_HL_DCH_CFG_STREAMING;
            break;
        default:
            hl_open_rsp.channel_cfg =  BTA_HL_DCH_CFG_RELIABLE;
            break;
        }
        hl_open_rsp.mcl_handle =  p_data->open_req.mcl_handle;
        hl_open_rsp.mdl_id =  p_data->open_req.mdl_id;
        hl_open_rsp.local_mdep_id =  p_data->open_req.local_mdep_id;
        hl_open_rsp.response_code =  BSA_HL_OPEN_RSP_SUCCESS;
        bsa_status = BSA_HlOpenRsp(&hl_open_rsp);
        if (bsa_status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_HlOpenRsp failed status:%d", bsa_status);
        }
        break;

    /* A Data Connection has been Opened */
    case BSA_HL_OPEN_EVT:
        APP_INFO1("BSA_HL_OPEN_EVT received Status:%d", p_data->open.status);
        APP_INFO1("    BdAddr:%02x-%02x-%02x-%02x-%02x-%02x",
                p_data->open.bd_addr[0],
                p_data->open.bd_addr[1],
                p_data->open.bd_addr[2],
                p_data->open.bd_addr[3],
                p_data->open.bd_addr[4],
                p_data->open.bd_addr[5]);
        APP_INFO1("    app_handle:%d data_handle:%d local_mdep_id:%d channel_mode:%s(%d)",
                p_data->open.app_handle, p_data->open.data_handle,
                p_data->open.local_mdep_id,
                app_hl_get_channel_mode_desc(p_data->open.channel_mode),
                p_data->open.channel_mode);
        APP_INFO1("    mcl_handle:%d uipc_channel:%d mtu:%d",
                p_data->open.mcl_handle,
                p_data->open.uipc_channel,
                p_data->open.mtu);
        if (p_data->open.status == BSA_SUCCESS)
        {
            /* Open the UIPC Channel with the associated Callback */
            if (UIPC_Open(p_data->open.uipc_channel, app_hl_uipc_cback) == FALSE)
            {
                APP_ERROR1("Not able to open UIPC channel:%d", p_data->open.uipc_channel);
                break;
            }
            APP_INFO1("UIPC_Open ChId:%d opened",p_data->open.uipc_channel);
            p_connection = app_hl_con_alloc();
            if (p_connection == NULL)
            {
                APP_ERROR0("Unable to allocate connection");
                break;
            }

            p_connection->is_open = TRUE;
            bdcpy(p_connection->bd_addr, p_data->open.bd_addr);
            p_connection->uipc_channel = p_data->open.uipc_channel;
            p_connection->mtu = p_data->open.mtu;
            p_connection->mcl_handle = p_data->open.mcl_handle;
            p_connection->data_handle = p_data->open.data_handle;
            APP_DEBUG0("Unlock Tx Mutex");
            status = app_unlock_mutex(&p_connection->uipc_tx_mutex);
            if(status < 0)
            {
                APP_ERROR0("Unable to Unlock Tx Mutex");
            }
        }
        break;

        /* A Data Connection has been Reconnected */
        case BSA_HL_RECONNECT_EVT:
            APP_INFO1("BSA_HL_RECONNECT_EVT received Status:%d", p_data->reconnect.status);
            APP_INFO1("    BdAddr:%02x-%02x-%02x-%02x-%02x-%02x",
                    p_data->reconnect.bd_addr[0],
                    p_data->reconnect.bd_addr[1],
                    p_data->reconnect.bd_addr[2],
                    p_data->reconnect.bd_addr[3],
                    p_data->reconnect.bd_addr[4],
                    p_data->reconnect.bd_addr[5]);
            APP_INFO1("    app_handle:%d data_handle:%d local_mdep_id:%d channel_mode:%d",
                    p_data->reconnect.app_handle, p_data->reconnect.data_handle,
                    p_data->reconnect.local_mdep_id, p_data->reconnect.channel_mode);
            APP_INFO1("    mcl_handle:%d tuipc_channel:%d mtu:%d",
                    p_data->reconnect.mcl_handle,
                    p_data->reconnect.uipc_channel,
                    p_data->reconnect.mtu);
            if (p_data->reconnect.status == BSA_SUCCESS)
            {
                /* Open the UIPC Channel with the associated Callback */
                if (UIPC_Open(p_data->reconnect.uipc_channel, app_hl_uipc_cback) == FALSE)
                {
                    APP_ERROR1("Not able to open UIPC channel:%d", p_data->reconnect.uipc_channel);
                    break;
                }
                APP_INFO1("UIPC_Open ChId:%d opened",p_data->reconnect.uipc_channel);
                p_connection = app_hl_con_alloc();
                if (p_connection == NULL)
                {
                    APP_ERROR0("Unable to allocate connection");
                    break;
                }

                p_connection->is_open = TRUE;
                bdcpy(p_connection->bd_addr, p_data->open.bd_addr);
                p_connection->uipc_channel = p_data->reconnect.uipc_channel;
                p_connection->mtu = p_data->reconnect.mtu;
                p_connection->mcl_handle = p_data->reconnect.mcl_handle;
                p_connection->data_handle = p_data->reconnect.data_handle;
                status = app_unlock_mutex(&p_connection->uipc_tx_mutex);
                if(status < 0)
                {
                    APP_ERROR0("Unable to Unlock Tx Mutex");
                }
            }
            break;

    /* A Data Connection has been Closed */
    case BSA_HL_CLOSE_EVT:
        APP_INFO1("BSA_HL_CLOSE_EVT received Status:%d Intentional:%d",
                p_data->close.status, p_data->close.intentional);
        APP_INFO1("    BdAddr:%02x-%02x-%02x-%02x-%02x-%02x",
                p_data->close.bd_addr[0],
                p_data->close.bd_addr[1],
                p_data->close.bd_addr[2],
                p_data->close.bd_addr[3],
                p_data->close.bd_addr[4],
                p_data->close.bd_addr[5]);
        APP_INFO1("    data_handle:%d ", p_data->close.data_handle);
        APP_INFO1("    channel number:%d ", p_data->close.uipc_channel);
        p_connection = app_hl_con_get_from_data_handle(p_data->close.data_handle);

        if (p_connection == NULL)
        {
            APP_ERROR0("No Connection structure match this HL link");
            break;
        }
        /* Close the UIPC Channel */
        UIPC_Close(p_connection->uipc_channel);
        status = app_unlock_mutex(&p_connection->uipc_tx_mutex);
        if(status < 0)
        {
            APP_ERROR0("Unable to Unlock Tx Mutex");
        }
        app_hl_con_free(p_connection);
        break;

    case BSA_HL_SEND_DATA_CFM_EVT:
        APP_INFO1("BSA_HL_SEND_DATA_CFM_EVT received Status:%d", p_data->send_data_cfm.status);
        APP_INFO1("    app_handle:%d data_handle:%d mcl_handle:%d",
                p_data->send_data_cfm.app_handle,
                p_data->send_data_cfm.data_handle,
                p_data->send_data_cfm.mcl_handle);

        p_connection = app_hl_con_get_from_data_handle(p_data->send_data_cfm.data_handle);

        if (p_connection == NULL)
        {
            APP_ERROR0("No Connection structure match this HL link");
            break;
        }
        break;

    case BSA_HL_SAVE_MDL_EVT:
        APP_INFO0("BSA_HL_SAVE_MDL_EVT received.");
        APP_INFO1("    app_handle:%d mdl_index:%d", p_data->save_mdl.app_handle, p_data->save_mdl.mdl_index);
        APP_INFO1("    BdAddr:%02x-%02x-%02x-%02x-%02x-%02x",
                p_data->save_mdl.mdl_cfg.peer_bd_addr[0],
                p_data->save_mdl.mdl_cfg.peer_bd_addr[1],
                p_data->save_mdl.mdl_cfg.peer_bd_addr[2],
                p_data->save_mdl.mdl_cfg.peer_bd_addr[3],
                p_data->save_mdl.mdl_cfg.peer_bd_addr[4],
                p_data->save_mdl.mdl_cfg.peer_bd_addr[5]);
        APP_INFO1("    active:%d dch_mode:%s (%d) fcs:%d local_mdep_id:%d",
                p_data->save_mdl.mdl_cfg.active,
                app_hl_get_channel_mode_desc(p_data->save_mdl.mdl_cfg.dch_mode),
                p_data->save_mdl.mdl_cfg.dch_mode,
                p_data->save_mdl.mdl_cfg.fcs,
                p_data->save_mdl.mdl_cfg.local_mdep_id);

        APP_INFO1("    local_mdep_role:%s (%d) mdl_id:%d mtu:%d time:%d",
                app_hl_get_role_desc(p_data->save_mdl.mdl_cfg.local_mdep_role),
                p_data->save_mdl.mdl_cfg.local_mdep_role,
                p_data->save_mdl.mdl_cfg.mdl_id,
                p_data->save_mdl.mdl_cfg.mtu,
                p_data->save_mdl.mdl_cfg.time);
        if (p_data->save_mdl.mdl_cfg.active)
        {
            app_hl_db_mdl_add(p_data->save_mdl.app_handle,
                    p_data->save_mdl.mdl_index,
                    &p_data->save_mdl.mdl_cfg);
        }
        else
        {
            app_hl_db_mdl_remove_by_index(p_data->save_mdl.app_handle,
                    p_data->save_mdl.mdl_index);
        }
        break;

    case BSA_HL_DELETE_MDL_EVT:
        APP_INFO0("BSA_HL_DELETE_MDL_EVT received.");
        APP_INFO1("    status:%d app_handle:%d mcl_handle:%d mdl_id:%d",
                p_data->delete_mdl.status,
                p_data->delete_mdl.app_handle,
                p_data->delete_mdl.mcl_handle,
                p_data->delete_mdl.mdl_id);
        break;

    default:
        APP_ERROR1("app_hl_cback bad event:%d", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_hl_start
 **
 ** Description      Example of function to start Health application
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_start(void)
{
    tBSA_STATUS bsa_status;
    tBSA_HL_ENABLE enable_param;

    APP_DEBUG0("app_hl_start");

    BSA_HlEnableInit(&enable_param);
    enable_param.p_cback = app_hl_cback;
    bsa_status = BSA_HlEnable(&enable_param);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HlEnable failed status:%d", bsa_status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_manager_init
 **
 ** Description      Init Manager application
 **
 ** Parameters       boot: boot flag
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_init(BOOLEAN boot)
{
    int status;
    int i;

    memset(&app_hl_cb, 0, sizeof(app_hl_cb));

    /* For every connection */
    for(i=0 ; i < APP_HL_PEER_DEVICE_MAX ; i++)
    {
        /* Create an UIPC Tx Mutex */
        status = app_init_mutex(&app_hl_cb.connections[i].uipc_tx_mutex);
        if (status < 0)
        {
            return -1;
        }
        /* and lock it */
        status = app_unlock_mutex(&app_hl_cb.connections[i].uipc_tx_mutex);
        if (status < 0)
        {
            return -1;
        }
    }
    /* If boot param is TRUE => the application just start  */
    if (boot == TRUE)
    {
        app_xml_init();
    }

    /* Example of function to get the Local Bluetooth configuration */
    if (app_dm_get_local_bt_config() == 1)
    {
        /*
         * The rest of the initialization function must be done for application
         * boot and when Bluetooth is enabled
         */

        /* Example of function to start Health application */
        status = app_hl_start();
        if (status < 0)
        {
            APP_ERROR0("app_hl_init: Unable to start Health");
            return -1;
        }

        /* Register some Health applications */
        app_hl_register_sink1();
        app_hl_register_source1();
        /* app_hl_register_dual1(); */
    }
    else
    {
        APP_INFO0("Bluetooth is disabled");
        APP_INFO0("Postpone Health Initialization");
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_exit
 **
 ** Description      Exit HL application
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_exit(void)
{
    tBSA_STATUS bsa_status;
    tBSA_HL_DISABLE dis_param;
    tBSA_HL_DEREGISTER dereg_param;
    int index;

    APP_DEBUG0("app_hl_exit");

    /* Deregister every Registered application/service */
    for (index = 0 ; index < APP_HL_LOCAL_APP_MAX; index++)
    {
        if (app_hl_cb.local_app[index].registered)
        {
            BSA_HlDeregisterInit(&dereg_param);
            dereg_param.app_handle = app_hl_cb.local_app[index].configuration.app_handle;
            bsa_status = BSA_HlDeregister(&dereg_param);
            if (bsa_status != BSA_SUCCESS)
            {
                APP_ERROR1("BSA_HlDeregister failed status:%d", bsa_status);
            }
        }
    }

    /* Disable Health */
    BSA_HlDisableInit(&dis_param);
    bsa_status = BSA_HlDisable(&dis_param);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HlDisable failed status:%d", bsa_status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_fill_data_cfg
 **
 ** Description      Function used to fill an MDEP Data Config with a specific
 **                  HL Data type
 **
 ** Parameters       p_mdep_data_cfg: pointer to mdep data structure
 **                  data_type_index: index of data type
 **                  role: role of the endpoint (source/sink)
 **
 ** Returns          status
 **
 *******************************************************************************/
static int app_hl_fill_mdep_data_cfg(tBSA_HL_MDEP_DATA_TYPE_CFG *p_mdep_data_cfg,
        int data_type_index, tBTA_HL_MDEP_ROLE role)
{
    if ((role != BSA_HL_MDEP_ROLE_SINK) &&
        (role != BSA_HL_MDEP_ROLE_SOURCE))
    {
        APP_ERROR1("Bad role:%d", role);
        return -1;
    }

    if ((data_type_index >= 0) && (data_type_index < APP_HL_SPECIALIZATION_MAX))
    {
        p_mdep_data_cfg->data_type = data_type_table[data_type_index].data_type;
        if (role != BSA_HL_MDEP_ROLE_SINK)
        {
            p_mdep_data_cfg->max_tx_apdu_size = data_type_table[data_type_index].tx_mtu;
            p_mdep_data_cfg->max_rx_apdu_size = data_type_table[data_type_index].rx_mtu;
        }
        else
        {
            p_mdep_data_cfg->max_tx_apdu_size = data_type_table[data_type_index].rx_mtu;
            p_mdep_data_cfg->max_rx_apdu_size = data_type_table[data_type_index].tx_mtu;
        }
        strncpy(p_mdep_data_cfg->desc, data_type_table[data_type_index].desc, sizeof(p_mdep_data_cfg->desc));
        p_mdep_data_cfg->desc[sizeof(p_mdep_data_cfg->desc) - 1] = '\0';
        return 0;
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_hl_alloc_app
 **
 ** Description      Allocate and save a Registered Health application
 **
 ** Parameters       p_register: pointer to register structure
 **
 ** Returns          application index allocated (-1 if error)
 **
 *******************************************************************************/
static int app_hl_alloc_app(tBSA_HL_REGISTER *p_register)
{
    int app_idx;

    for (app_idx = 0 ; app_idx < APP_HL_LOCAL_APP_MAX; app_idx++)
    {
        if (app_hl_cb.local_app[app_idx].registered == FALSE)
        {
            app_hl_cb.local_app[app_idx].registered = TRUE;
            /* Save the registered application for later use */
            memcpy(&app_hl_cb.local_app[app_idx].configuration, p_register,
                    sizeof(app_hl_cb.local_app[app_idx].configuration));
            return app_idx;
        }
    }
    APP_ERROR0("No room to save application information");
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_hl_display_register
 **
 ** Description      Display a Registered Health application
 **
 ** Parameters       p_register: pointer to register structure
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hl_display_register(tBSA_HL_REGISTER *p_register)
{
    int mdep_index;
    int data_type_index;

    APP_INFO0("app_hl_display_register");
    APP_INFO1("   Application Handle:%d", p_register->app_handle);
    APP_INFO1("   Service Name:%s", p_register->service_name);
    APP_INFO1("   Service Description:%s", p_register->service_desc);
    APP_INFO1("   Provider Name:%s", p_register->provider_name);
    APP_INFO1("   Advertise source:%d", p_register->advertize_source);
    for(mdep_index = 0 ; mdep_index < p_register->num_of_mdeps ; mdep_index++)
    {
        APP_INFO1("   MDEP[%d] MdepId:%d Role:%s", mdep_index,
                p_register->mdep[mdep_index].mdep_id,
                APP_HL_ROLE_DESC(p_register->mdep[mdep_index].mdep_role));
        for(data_type_index = 0 ; data_type_index < p_register->mdep[mdep_index].num_of_mdep_data_types ; data_type_index++)
        {
            APP_INFO1("      DataType:%s", p_register->mdep[mdep_index].data_cfg[data_type_index].desc);
        }
    }
}

/*******************************************************************************
 **
 ** Function         app_hl_register_sink1
 **
 ** Description      Register Health application Sink
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_register_sink1(void)
{
    tBSA_HL_REGISTER hl_register;
    tBSA_STATUS status;
    UINT8 mdep_index;

    BSA_HlRegisterInit(&hl_register);
    strncpy(hl_register.service_name, "HL Sink1 Service", sizeof(hl_register.service_name));
    strncpy(hl_register.service_desc, "Sink1 Service Description", sizeof(hl_register.service_desc));
    strncpy(hl_register.provider_name, "BRCM Provider", sizeof(hl_register.provider_name));

    hl_register.num_of_mdeps = 2; /* 2 MDEPs*/

    mdep_index = 0;
    hl_register.mdep[mdep_index].mdep_role = BSA_HL_MDEP_ROLE_SINK;
    hl_register.mdep[mdep_index].num_of_mdep_data_types = 3; /* 3 Data Types */
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[0],
            APP_HL_PULSE_OXIMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[1],
            APP_HL_BLOOD_PRESSURE_MON, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[2],
            APP_HL_BODY_THERMOMETER, hl_register.mdep[mdep_index].mdep_role);

    mdep_index = 1;
    hl_register.mdep[mdep_index].mdep_role = BSA_HL_MDEP_ROLE_SINK;
    hl_register.mdep[mdep_index].num_of_mdep_data_types = 3; /* 3 Data Types */
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[0],
            APP_HL_BODY_WEIGHT_SCALE, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[1],
            APP_HL_PEDOMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[2],
            APP_HL_STEP_COUNTER, hl_register.mdep[mdep_index].mdep_role);

    /* Retrieve the saved MDL CFG for this application */
    app_hl_db_get_by_app_hdl(1, hl_register.saved_mdl);

    status = BSA_HlRegister(&hl_register);
    if (status == BSA_SUCCESS)
    {
        APP_DEBUG1("app_hl_register_sink1 success app_handle:%d", hl_register.app_handle);
        app_hl_display_register(&hl_register);
    }
    else
    {
        APP_ERROR1("app_hl_register_sink1 fail status:%d", status);
        return -1;
    }

    /* Save the registered application */
    app_hl_alloc_app(&hl_register);

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_register_source1
 **
 ** Description      Register Health application source
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_register_source1(void)
{
    tBSA_HL_REGISTER hl_register;
    tBSA_STATUS status;
    UINT8 mdep_index;

    BSA_HlRegisterInit(&hl_register);
    strncpy(hl_register.service_name, "HL Source1 Service", sizeof(hl_register.service_name));
    strncpy(hl_register.service_desc, "Source1 Service Description", sizeof(hl_register.service_desc));
    strncpy(hl_register.provider_name, "BRCM Provider", sizeof(hl_register.provider_name));

    hl_register.num_of_mdeps = 2; /* 2 MDEPs */

    hl_register.advertize_source = TRUE;

    mdep_index = 0;
    hl_register.mdep[mdep_index].mdep_role = BSA_HL_MDEP_ROLE_SOURCE;
    hl_register.mdep[mdep_index].num_of_mdep_data_types = 2; /* 2 Data Types*/
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[0],
            APP_HL_PULSE_OXIMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[1],
            APP_HL_BLOOD_PRESSURE_MON, hl_register.mdep[mdep_index].mdep_role);

    mdep_index = 1;
    hl_register.mdep[mdep_index].mdep_role = BSA_HL_MDEP_ROLE_SOURCE;
    hl_register.mdep[mdep_index].num_of_mdep_data_types = 3; /* 3 Data Types */
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[0],
            APP_HL_BODY_THERMOMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[1],
            APP_HL_BODY_WEIGHT_SCALE, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[2],
            APP_HL_STEP_COUNTER, hl_register.mdep[mdep_index].mdep_role);

    /* Retrieve the saved MDL CFG for this application */
    app_hl_db_get_by_app_hdl(2, hl_register.saved_mdl);

    status = BSA_HlRegister(&hl_register);
    if (status == BSA_SUCCESS)
    {
        APP_INFO1("app_hl_register_source1 success app_handle:%d", hl_register.app_handle);
        app_hl_display_register(&hl_register);
    }
    else
    {
        APP_ERROR1("app_hl_register_source1 fail status:%d", status);
        return -1;
    }

    /* Save the registered application */
    app_hl_alloc_app(&hl_register);

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_register_dual1
 **
 ** Description      Register Health application Source and Sink
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_register_dual1(void)
{
    tBSA_HL_REGISTER hl_register;
    tBSA_STATUS status;
    UINT8 mdep_index;

    BSA_HlRegisterInit(&hl_register);
    strncpy(hl_register.service_name, "HL Dual1 Service", sizeof(hl_register.service_name));
    strncpy(hl_register.service_desc, "Dual Service Description", sizeof(hl_register.service_desc));
    strncpy(hl_register.provider_name, "BRCM Provider", sizeof(hl_register.provider_name));

    hl_register.num_of_mdeps = 2; /* 2 MDEPs */
    hl_register.advertize_source = TRUE;

    mdep_index = 0;
    hl_register.mdep[mdep_index].mdep_role = BSA_HL_MDEP_ROLE_SINK;
    hl_register.mdep[mdep_index].num_of_mdep_data_types = 5; /* 5 Data Types */
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[0],
            APP_HL_PULSE_OXIMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[1],
            APP_HL_BLOOD_PRESSURE_MON, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[2],
            APP_HL_BODY_THERMOMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[3],
            APP_HL_BODY_WEIGHT_SCALE, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[4],
            APP_HL_STEP_COUNTER, hl_register.mdep[mdep_index].mdep_role);

    mdep_index = 1;
    hl_register.mdep[mdep_index].mdep_role = BSA_HL_MDEP_ROLE_SOURCE;
    hl_register.mdep[mdep_index].num_of_mdep_data_types = 5; /* 5 Data Types */
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[0],
            APP_HL_PULSE_OXIMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[1],
            APP_HL_BLOOD_PRESSURE_MON, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[2],
            APP_HL_BODY_THERMOMETER, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[3],
            APP_HL_BODY_WEIGHT_SCALE, hl_register.mdep[mdep_index].mdep_role);
    app_hl_fill_mdep_data_cfg(&hl_register.mdep[mdep_index].data_cfg[4],
            APP_HL_STEP_COUNTER, hl_register.mdep[mdep_index].mdep_role);

    /* Retrieve the saved MDL CFG for this application */
    app_hl_db_get_by_app_hdl(3, hl_register.saved_mdl);

    status = BSA_HlRegister(&hl_register);
    if (status == BSA_SUCCESS)
    {
        APP_INFO1("app_hl_register_dual1 success app_handle:%d", hl_register.app_handle);
        app_hl_display_register(&hl_register);
    }
    else
    {
        APP_ERROR1("app_hl_register_dual1 fail status:%d", status);
        return -1;
    }

    /* Save the registered application */
    app_hl_alloc_app(&hl_register);

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_sdp_query
 **
 ** Description      SDP Query on a peer device
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_sdp_query(void)
{
    int status = -1;
    int device_index;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_HL_SDP_QUERY sdp_query_param;

    APP_INFO0("Bluetooth Health SDP Query menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    device_index = app_get_choice("Entrer Choice");

    /* Devices from XML databased */
    if (device_index == 0)
    {
        /* Read the xml file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Entrer device number");
        if ((device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (device_index >= 0) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
            p_name = app_xml_remote_devices_db[device_index].name;
            status = 0;
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_disc_display_devices();
        device_index = app_get_choice("Enter device number");
        if ((device_index < APP_DISC_NB_DEVICES) &&
            (device_index >= 0) &&
            (app_discovery_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
            p_name = app_discovery_cb.devs[device_index].device.name;
            status = 0;
        }
    }
    else
    {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
    }

    if (status == 0)
    {
        BSA_HlSdpQueryInit(&sdp_query_param);
        bdcpy(sdp_query_param.bd_addr, bd_addr);
        status = BSA_HlSdpQuery(&sdp_query_param);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_HlSdpQuery failed status:%d", status);
        }
        else
        {
            APP_INFO1("Health SDP Quering :%s...", p_name);
        }
    }
    else
    {
        APP_ERROR1("app_hl_sdp_query: Bad Device index:%d", device_index);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hl_open
 **
 ** Description      Open an HL connection
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_open(void)
{
    tBSA_HL_OPEN hl_open;
    tBSA_STATUS bsa_status;
    int device_index;
    BD_ADDR bd_addr;
    int app_idx;

    APP_INFO0("Bluetooth Health Open menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last Discovery");
    device_index = app_get_choice("Entrer Choice");

    /* Devices from XML databased */
    if (device_index == 0)
    {
        /* Read the xml file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Entrer device number");
        if ((device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (device_index >= 0) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_disc_display_devices();
        device_index = app_get_choice("Enter device number");
        if ((device_index < APP_DISC_NB_DEVICES) &&
            (device_index >= 0) &&
            (app_discovery_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
        }
    }
    else
    {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
        return -1;
    }

    BSA_HlOpenInit(&hl_open);
    bdcpy(hl_open.bd_addr, bd_addr);
    /* Display all the registered application to help user to select parameters */
    for (app_idx = 0 ; app_idx < APP_HL_LOCAL_APP_MAX; app_idx++)
    {
        if (app_hl_cb.local_app[app_idx].registered != FALSE)
        {
            app_hl_display_register(&app_hl_cb.local_app[app_idx].configuration);
        }
    }
    hl_open.sec_mask = 0;
    hl_open.app_handle = app_get_choice("Enter app_handle");
    hl_open.ctrl_psm = app_get_choice("Enter peer's ctrl_psm");
    hl_open.local_mdep_id = app_get_choice("Enter local_mdep_id");
    hl_open.peer_mdep_id = app_get_choice("Enter peer_mdep_id");

    /* Ask for the Channel configuration requested */
    APP_INFO0("Select Channel configuration:");
    APP_INFO1("    %d No Preference", BSA_HL_DCH_CFG_NO_PREF);
    APP_INFO1("    %d Reliable", BSA_HL_DCH_CFG_RELIABLE);
    APP_INFO1("    %d Stream", BSA_HL_DCH_CFG_STREAMING);
    hl_open.channel_cfg = app_get_choice("Channel mode");
    if ((hl_open.channel_cfg != BSA_HL_DCH_CFG_NO_PREF) &&
        (hl_open.channel_cfg != BSA_HL_DCH_CFG_RELIABLE) &&
        (hl_open.channel_cfg != BSA_HL_DCH_CFG_STREAMING))
    {
        APP_ERROR1("Bad Channel configuration selected:%d", hl_open.channel_cfg);
        return -1;
    }

    bsa_status = BSA_HlOpen(&hl_open);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HlOpen failed status:%d", bsa_status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_reconnect
 **
 ** Description      Reconnect an HL connection
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_reconnect(void)
{
    tBSA_HL_RECONNECT hl_reconnect;
    int status = -1;
    int device_index;
    BD_ADDR bd_addr;
    int app_idx;

    APP_INFO0("Bluetooth Health Reconnect menu:");

    /* Read the xml file which contains all the bonded devices */
    app_read_xml_remote_devices();

    app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    device_index = app_get_choice("Entrer device number");
    if ((device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
        (device_index >= 0) &&
        (app_xml_remote_devices_db[device_index].in_use != FALSE))
    {
        bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
        status = 0;
    }
    else
    {
        APP_ERROR1("Bad device index:%d", device_index);
    }

    if (status == 0)
    {
        BSA_HlReconnectInit(&hl_reconnect);
        bdcpy(hl_reconnect.bd_addr, bd_addr);
        /* Display all the registered application to help user to select parameters */
        for (app_idx = 0 ; app_idx < APP_HL_LOCAL_APP_MAX; app_idx++)
        {
            if (app_hl_cb.local_app[app_idx].registered != FALSE)
            {
                app_hl_display_register(&app_hl_cb.local_app[app_idx].configuration);
            }
        }
        /* Display all saved MDL to help user to select parameters  */
        app_hl_db_display();

        hl_reconnect.app_handle = app_get_choice("Enter app_handle");
        hl_reconnect.ctrl_psm = app_get_choice("Enter peer's ctrl_psm");
        hl_reconnect.sec_mask = 0;
        hl_reconnect.mdl_id = app_get_choice("Enter mdl_id");

        status = BSA_HlReconnect(&hl_reconnect);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_HlReconnect failed status:%d", status);
        }
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_hl_delete
 **
 ** Description      Delete an HL connection
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_delete(void)
{
    tBSA_HL_DELETE_MDL hl_delete;
    int status = -1;

    APP_INFO0("Bluetooth Health Delete MDL menu:");

    /* Display all saved MDL to help user to select parameters  */
    app_hl_db_display();

    /* Display all HL connections to help user to select parameters  */
    app_hl_con_display();

    BSA_HlDeleteMdlInit(&hl_delete);

    hl_delete.mcl_handle = app_get_choice("Enter mcl_handle");

    APP_INFO0("    0 Delete one MDL");
    APP_INFO0("    1 Delete ALL MDLs");
    hl_delete.mdl_id = app_get_choice("");
    if (hl_delete.mdl_id == 0)
    {
        hl_delete.mdl_id = app_get_choice("Enter Mdl_id");
    }
    else
    {
        hl_delete.mdl_id = BSA_HL_DELETE_ALL_MDL_IDS;
    }

    status = BSA_HlDeleteMdl(&hl_delete);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HlDelete failed status:%d", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_hl_close
 **
 ** Description      Close an HL connection
 **
 ** Parameters       none
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hl_close(void)
{
    tBSA_HL_CLOSE hl_close;
    tBSA_STATUS status;

    /* Print current HL connections */
    app_hl_con_display();

    BSA_HlCloseInit(&hl_close);

    hl_close.data_handle = app_get_choice("Data Handle to close");

    status = BSA_HlClose(&hl_close);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HlClose failed status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_con_alloc
 **
 ** Description      Function to allocate a HL connection structure
 **
 ** Parameters       none
 **
 ** Returns          Pointer on allocated connecton
 **
 *******************************************************************************/
tAPP_HL_CONNECTION *app_hl_con_alloc(void)
{
    int i;

    for(i = 0 ; i < APP_HL_NB_CON_MAX ; i++)
    {
        if (app_hl_cb.connections[i].in_use == FALSE)
        {
            /* Reset structure */
            APP_DEBUG1("Allocate HL connection:%d", i);
            memset(&app_hl_cb.connections[i], 0, sizeof(app_hl_cb.connections[i]));
            app_hl_cb.connections[i].in_use = TRUE;
            app_hl_cb.connections[i].uipc_channel = APP_HL_BAD_UIPC;
            return &app_hl_cb.connections[i];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hl_con_free
 **
 ** Description      Function to free a HL connection structure
 **
 ** Parameters       connection: connection pointer
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hl_con_free(tAPP_HL_CONNECTION *p_connection)
{
    if (p_connection == NULL)
    {
        APP_ERROR0("app_hl_con_free null pointer");
        return;
    }
    if (p_connection->in_use == FALSE)
    {
        APP_ERROR0("p_connection Connection was not in use");
    }

    /* Reset structure */
    memset(p_connection, 0, sizeof(*p_connection));
}

/*******************************************************************************
 **
 ** Function         app_hl_con_display
 **
 ** Description      Function to display all HL connections
 **
 ** Parameters       none
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hl_con_display(void)
{
    int i;
    tAPP_HL_CONNECTION *p_hl_con;

    APP_INFO0("app_hl_con_display");

    p_hl_con = &app_hl_cb.connections[0];
    for(i = 0 ; i < APP_HL_NB_CON_MAX ; i++, p_hl_con++)
    {
        if (p_hl_con->in_use == TRUE)
        {
            APP_INFO1("Con:%d BdAddr:%02X-%02X-%02X-%02X-%02X-%02X mcl_handle:%d data_handle:%d uipc_channel:%d",
                    i,
                    p_hl_con->bd_addr[0], p_hl_con->bd_addr[1], p_hl_con->bd_addr[2],
                    p_hl_con->bd_addr[3], p_hl_con->bd_addr[4], p_hl_con->bd_addr[5],
                    p_hl_con->mcl_handle, p_hl_con->data_handle, p_hl_con->uipc_channel);
        }
    }
}

/*******************************************************************************
 **
 ** Function         app_hl_send_file
 **
 ** Description      Example of function to send a file
 **
 ** Parameters       p_file_name: file name to send
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hl_send_file(char * p_file_name)
{
    int fd;
    ssize_t length;
    BT_HDR *p_msg;
    int status;
    tAPP_HL_CONNECTION *p_connection;
    BOOLEAN packet_sent;
    tBSA_HL_DATA_HANDLE data_handle;

    APP_DEBUG0("app_hl_send_file send enter");

    if(p_file_name == NULL)
    {
        APP_ERROR0("ERROR app_hl_send_file null file name !!");
        return -1;
    }

    /* Display the current connections */
    app_hl_con_display();

    /* Ask the user to select a Data Connection */
    data_handle = app_get_choice("Enter data_handle");

    p_connection = app_hl_con_get_from_data_handle(data_handle);

    if (p_connection == NULL)
    {
        APP_ERROR1("Bad data_handle:%d", data_handle);
        return -1;
    }

    /* If connection not used or not opened */
    if ((p_connection->in_use == FALSE) ||
         ((p_connection->in_use != FALSE)
            && (p_connection->is_open == FALSE)))
    {
        APP_ERROR1("data_handle:%d not opened", data_handle);
        return -1;
    }

    /* Open the file to send */
    fd = open(p_file_name, O_RDONLY);
    if (fd >= 0)
    {
        do
        {
            /* Allocate buffer for Tx data */
            if ((p_msg =  (BT_HDR *) GKI_getbuf(sizeof(BT_HDR) + p_connection->mtu))!= NULL)
            {
                p_msg->offset = 0;

                /* Read the data from the file and write it in the buffer */
                length = read(fd, (UINT8 *)(p_msg + 1), p_connection->mtu);

                if (length > 0)
                {
                    APP_DEBUG1("app_hl_send_file send %d bytes to peer(mtu size is %d)",
                            (int)length, p_connection->mtu);
                    p_msg->len = length;

                    packet_sent = FALSE;
                    do
                    {
                        /* Try to send the buffer to the peer device */
                        status = app_hl_send(p_connection, p_msg);

                        if (status == 0)
                        {
                            packet_sent = TRUE;
                            /* Buffer send to BSA server */
                            APP_DEBUG0("app_hl_send OKAY");
                        }
                        /* Not able to send the buffer */
                        else if (status < 0)
                        {
                            packet_sent = TRUE;
                            APP_ERROR0("No able to send buffer");
                        }
                        else
                        {
                            APP_DEBUG1("UIPC Tx Flow Off ChId:%d", p_connection->uipc_channel);
                            /* Wait on Tx Mutex */
                            APP_DEBUG1("Wait TX_READY EVENT on UIPC:%d", p_connection->uipc_channel);
                            if(app_lock_mutex(&p_connection->uipc_tx_mutex) < 0)
                            {
                                APP_ERROR0("Unable to lock mutex");
                                /* Exit loops */
                                packet_sent = TRUE;
                                length = 0;
                                break;
                            }
                            /* Handle the case where the connection is closed
                             * while we where waiting for the Mutex */
                            if (p_connection->is_open)
                            {
                                APP_DEBUG1("Received TX_READY EVENT on UIPC:%d. Retrying...",
                                        p_connection->uipc_channel);
                            }
                            else
                            {
                                packet_sent = TRUE;
                            }
                        }
                    } while(packet_sent == FALSE);
                }
                else
                {
                    /* Nothing read => free buffer */
                    GKI_freebuf(p_msg);
                }
            }
            else
            {
                APP_ERROR0("No able to allocate Tx buffer");
                /* Exit loop */
                length = 0;
            }
        } while((length > 0) && (p_connection->is_open != FALSE));

        /* Close the file */
        close(fd);
    }
    else
    {
        APP_ERROR1("ERROR app_hl_send_file could not open %s", p_file_name);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hl_send_data
 **
 ** Description      Example of function to send a data
 **
 ** Parameters       none
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hl_send_data(void)
{
    ssize_t length;
    BT_HDR *p_msg;
    int status;
    tAPP_HL_CONNECTION *p_connection;
    int i;
    UINT8 *test_read;
    BOOLEAN packet_sent;
    tBSA_HL_DATA_HANDLE data_handle;

    APP_DEBUG0("app_hl_send_data");

    /* Display the current connections */
    app_hl_con_display();

    /* Ask the user to select a Data Connection */
    data_handle = app_get_choice("Enter data_handle");

    p_connection = app_hl_con_get_from_data_handle(data_handle);

    if (p_connection == NULL)
    {
        APP_ERROR1("Bad data_handle:%d", data_handle);
        return -1;
    }

    /* If connection not used or not opened */
    if ((p_connection->in_use == FALSE) ||
         ((p_connection->in_use != FALSE) && (p_connection->is_open == FALSE)))
    {
        APP_ERROR1("data_handle:%d not opened", data_handle);
        return -1;
    }

    APP_INFO1("The mtu for this connection is:%d bytes", p_connection->mtu);
    length = app_get_choice("Enter the size of the packet to send");
    if (length > p_connection->mtu)
    {
        APP_ERROR1("Packet size:%d is too big", p_connection->mtu);
        return -1;
    }

    if (length <= 0)
    {
        APP_ERROR1("Nothing to send, Packet size is :%d", p_connection->mtu);
        return -1;
    }

    /* Allocate a GKI buffer */
    if ((p_msg =  (BT_HDR *) GKI_getbuf(sizeof(BT_HDR) + length))!= NULL)
    {
        p_msg->offset = 0;
        p_msg->len = length;

        /* Write test Pattern in buffer */
        test_read = (UINT8 *)(p_msg + 1);
        for(i=0 ; i < length ; i++)
        {
            *(test_read + i) = i;
        }
        APP_DEBUG1("app_hl_send_file send %d bytes to peer(mtu size is %d)", (int)length, length);

        packet_sent = FALSE;

        do
        {
            /* Try to send the buffer to the peer device */
            status = app_hl_send(p_connection, p_msg);

            if (status == 0)
            {
                packet_sent = TRUE;
                /* Buffer send to BSA server */
                APP_DEBUG0("app_hl_send OKAY");
            }
            /* Not able to send the buffer */
            else if (status < 0)
            {
                packet_sent = TRUE;
                APP_ERROR0("No able to send buffer");
            }
            else
            {
                APP_DEBUG1("UIPC Tx Flow Off ChId:%d", p_connection->uipc_channel);
                /* Wait on Tx Mutex */
                APP_DEBUG1("Wait TX_READY EVENT on UIPC:%d", p_connection->uipc_channel);
                if(app_lock_mutex(&p_connection->uipc_tx_mutex) < 0)
                {
                    APP_ERROR0("unable to lock mutex");
                    /* Exit loops */
                    packet_sent = TRUE;
                    length = 0;
                    break;
                }
                /* Handle the case where the connection is closed while waiting for the Mutex */
                if (p_connection->is_open)
                {
                    APP_DEBUG1("Received TX_READY EVENT on UIPC:%d. Retrying...",
                            p_connection->uipc_channel);
                }
                else
                {
                    packet_sent = TRUE;
                }
            }
        } while (packet_sent == FALSE);
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hl_send
 **
 ** Description      Example of function to send
 **
 ** Parameters       connection: connection value
 **                  p_msg: pointer of data to send
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hl_send(tAPP_HL_CONNECTION *p_connection, BT_HDR *p_msg)
{
    BOOLEAN send_status;

    if (p_connection == NULL)
    {
        APP_ERROR0("Null Connection pointer");
        return -1;
    }
    if (p_msg == NULL)
    {
        APP_ERROR0("Null Message pointer");
        return -1;
    }

    send_status = UIPC_SendBuf(p_connection->uipc_channel, p_msg);

    if (send_status == TRUE)
    {
        /* Buffer send to BSA server */
        APP_DEBUG0("Buffer sent into UIPC");
    }
    /* Not able to send the buffer: check the cause */
    else
    {
        /* Check if it's because flow is off */
        if (p_msg->layer_specific == UIPC_LS_TX_FLOW_OFF)
        {
            APP_DEBUG0("Flow control OFF");

            /* Ask UIPC to send us an event when flow will become On */
            UIPC_Ioctl(p_connection->uipc_channel, UIPC_REQ_TX_READY, NULL);
            return 1;
        }
        /* Unrecoverable error (e.g. UIPC not open) */
        else
        {
            APP_DEBUG0("Unable to send UIPC buffer");
            return -1;
        }
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hl_uipc_evt_desc
 **
 ** Description     Get UIPC Event description
 **
 ** Parameters      event: Event to decode
 **
 ** Returns         Event description
 **
 *******************************************************************************/
char *app_hl_uipc_evt_desc(UINT16 event)
{
    switch (event)
    {
    case UIPC_OPEN_EVT:
        return "UIPC_OPEN_EVT";
    case UIPC_CLOSE_EVT:
        return "UIPC_CLOSE_EVT";
    case UIPC_RX_DATA_EVT:
        return "UIPC_RX_DATA_EVT";
    case UIPC_RX_DATA_READY_EVT:
        return "UIPC_RX_DATA_READY_EVT";
    case UIPC_TX_DATA_READY_EVT:
        return "UIPC_TX_DATA_READY_EVT";
    default:
        return "Unknown Event";
    }
}

/*******************************************************************************
 **
 ** Function         app_hl_uipc_cback
 **
 ** Description     uipc hl call back function.
 **
 ** Parameters      p_msg: pointer of message data
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hl_uipc_cback(BT_HDR *p_msg)
{
    tAPP_HL_CONNECTION *p_connection;
    tUIPC_CH_ID uipc_channel;
    UINT32 length;
    UINT8 *p_rx_msg;
    UINT16 msg_evt;
    int status;

    /* Extract Channel Id from layer_specific field */
    uipc_channel = p_msg->layer_specific;

    APP_DEBUG1("app_hl_uipc_cback event:%s (%d) ChId:%d Len:%d",
            app_hl_uipc_evt_desc(p_msg->event), p_msg->event,
            p_msg->layer_specific, p_msg->len);

    p_connection = app_hl_con_get_from_uipc(uipc_channel);
    if (p_connection == NULL)
    {
        APP_ERROR1("No Connection connected with UIPC ChId:%d", uipc_channel);
        /* Free the received buffer */
         GKI_freebuf(p_msg);
        return;
    }

    /* Data ready to be read */
    switch(p_msg->event)
    {
    case UIPC_RX_DATA_READY_EVT:
        /* There are data to read. Read all data available */
        do {
            /* allocate p_rx_msg */
            if ((p_rx_msg = GKI_getbuf(APP_HL_UIPC_READ_SIZE)) == NULL)
            {
                APP_ERROR1("GKI_getbuf failed (buffer size:%d)", APP_HL_UIPC_READ_SIZE);
                break; /* Exit loop */
            }

            /* Read the buffer */
            length = UIPC_Read(uipc_channel, &msg_evt, p_rx_msg, APP_HL_UIPC_READ_SIZE);

            /*
             * If UIPC_Read return 0, this means that there is not anymore data
             * to read => Ask the UIPC channel to send an UIPC_RX_DATA_READY_EVT
             * when data will be ready to be read
             */
            if (length == 0)
            {
                UIPC_Ioctl(uipc_channel, UIPC_REQ_RX_READY, NULL);
            }
            else
            {
                /* Dump the Received buffer */
                scru_dump_hex(p_rx_msg, "    HL Rx Data", length, TRACE_LAYER_NONE,
                        TRACE_TYPE_DEBUG);
            }
            GKI_freebuf(p_rx_msg);
        } while (length > 0);
        break;

    case UIPC_TX_DATA_READY_EVT:
        /* UIPC channel is now able to accept data (UIPC_SendBuf) */
        APP_DEBUG0("Unlock Tx Mutex");
        status = app_unlock_mutex(&p_connection->uipc_tx_mutex);
        if(status < 0)
        {
            APP_ERROR0("Unable to Unlock Tx Mutex");
        }
        break;

    case UIPC_RX_DATA_EVT:
        APP_ERROR1("UIPC_RX_DATA_EVT should not be received form HL UIPC ChId:%d",
                uipc_channel);
        break;

    case UIPC_OPEN_EVT:
        /* UIPC channel opened. Should not be received on client side */
        break;

    case UIPC_CLOSE_EVT:
        /* UIPC channel closed */
        APP_DEBUG0("Unlock Tx Mutex");
        status = app_unlock_mutex(&p_connection->uipc_tx_mutex);
        if(status < 0)
        {
            APP_ERROR0("Unable to Unlock Tx Mutex");
        }
        break;

    default:
        break;
    }

    /* Free the received buffer */
    GKI_freebuf(p_msg);
}


/*******************************************************************************
 **
 ** Function         app_hl_con_get_from_uipc
 **
 ** Description      Function to get HL connection from UIPC Channel Id
 **
 ** Parameters       channel_id: uipc channel id
 **
 ** Returns          Pointer on HL connection or NULL
 **
 *******************************************************************************/
tAPP_HL_CONNECTION *app_hl_con_get_from_uipc(tUIPC_CH_ID channel_id)
{
    int i;

    for(i = 0 ; i < APP_HL_NB_CON_MAX ; i++)
    {
        if ((app_hl_cb.connections[i].in_use != FALSE) &&
            (app_hl_cb.connections[i].uipc_channel == channel_id))
        {
            APP_DEBUG1("Found HL connection:%d matching Channel:%d", i, channel_id);
            return &app_hl_cb.connections[i];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hl_con_get_from_data_handle
 **
 ** Description      Function to get HL connection from HL data handle
 **
 ** Parameters       data_handle: Data Handle
 **
  ** Returns          Pointer on HL connection or NULL
 **
 *******************************************************************************/
tAPP_HL_CONNECTION *app_hl_con_get_from_data_handle(tBSA_HL_DATA_HANDLE data_handle)
{
    int i;

    for(i = 0 ; i < APP_HL_NB_CON_MAX ; i++)
    {
        if ((app_hl_cb.connections[i].in_use != FALSE) &&
            (app_hl_cb.connections[i].data_handle == data_handle))
        {
            APP_DEBUG1("Found HL connection:%d matching data_handle:%d", i, data_handle);
            return &app_hl_cb.connections[i];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hl_get_channel_cfg_desc
 **
 ** Description      Function to get Channel Configuration Description
 **
 ** Parameters       channel_cfg: Channel Configuration
 **
 ** Returns          Description string
 **
 *******************************************************************************/
char *app_hl_get_channel_cfg_desc(tBSA_HL_DCH_CFG channel_cfg)
{
    switch(channel_cfg)
    {
    case BSA_HL_DCH_CFG_NO_PREF:
        return "No Preference";
    case BSA_HL_DCH_CFG_RELIABLE:
        return "Reliable";
    case BSA_HL_DCH_CFG_STREAMING:
        return "Streaming;";
    default:
        return "Unknown";
    }
}

/*******************************************************************************
 **
 ** Function         app_hl_get_channel_mode_desc
 **
 ** Description      Function to get Channel Mode Description
 **
 ** Parameters       channel_mode: Channel Mode
 **
 ** Returns          Description string
 **
 *******************************************************************************/
char *app_hl_get_channel_mode_desc(tBSA_HL_DCH_MODE channel_mode)
{
    switch(channel_mode)
    {
    case BSA_HL_DCH_MODE_RELIABLE:
        return "Reliable";
    case BSA_HL_DCH_MODE_STREAMING:
        return "Streaming";
    default:
        return "Unknown";
    }
}

/*******************************************************************************
 **
 ** Function         app_hl_get_role_desc
 **
 ** Description      Function to get role Description
 **
 ** Parameters       channel_mode: Channel Mode
 **
 ** Returns          Description string
 **
 *******************************************************************************/
char *app_hl_get_role_desc(tBTA_HL_MDEP_ROLE role)
{
    switch(role)
    {
    case BSA_HL_MDEP_ROLE_SOURCE:
        return "source";
    case BSA_HL_MDEP_ROLE_SINK:
        return "Sink";
    default:
        return "Unknown";
    }
}
