/*****************************************************************************
 **
 **  Name:           app_pan.c
 **
 **  Description:    Bluetooth PAN application
 **
 **  Copyright (c) 2011-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <string.h>
#include <stddef.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/ethernet.h>

#include "gki.h"
#include "uipc.h"

#include "bsa_api.h"

#include "app_pan.h"
#include "app_pan_util.h"
#include "app_pan_netif.h"

#include "bsa_trace.h"

#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_mutex.h"
#include "app_thread.h"


#define APP_PAN_NB_CON_MAX       7   /* Maximum number of connections */
#define APP_PAN_UIPC_BUF_SZ      (sizeof(tBSA_PAN_UIPC_HDR) + \
                                  sizeof(tBSA_PAN_UIPC_DATA_REQ) + \
                                  ETH_FRAME_LEN)
#define APP_PAN_UIPC_FLOW_HWM    8
#define APP_PAN_UIPC_FLOW_LWM    2

#define APP_PAN_BAD_HANDLE       0xFF
#define APP_PAN_BAD_UIPC         0xFF
#define APP_PAN_BAD_NETIF_FD     (-1)

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct
{
    BOOLEAN in_use;
    BD_ADDR bd_addr;
    tBSA_PAN_HNDL handle;
    tBSA_PAN_ROLE local_role;
    tBSA_PAN_ROLE peer_role;
    tUIPC_CH_ID uipc_channel;
    BOOLEAN uipc_is_open;
    BUFFER_Q uipc_q;
    BUFFER_Q tx_q;
    BOOLEAN tx_flow_stop;
    tAPP_MUTEX tx_mutex;
    BUFFER_Q rx_q;
    BOOLEAN rx_flow_stop;
    UINT16 pfilter_num;
    UINT16 pfilter_start[BSA_PAN_MAX_PROT_FILTERS];
    UINT16 pfilter_end[BSA_PAN_MAX_PROT_FILTERS];
    UINT16 mfilter_num;
    BD_ADDR mfilter_start[BSA_PAN_MAX_MULTI_FILTERS];
    BD_ADDR mfilter_end[BSA_PAN_MAX_MULTI_FILTERS];
} tAPP_PAN_CON;

typedef struct
{
    int netif_fd;
    tAPP_THREAD netif_thread;
    tAPP_PAN_CON con[APP_PAN_NB_CON_MAX];
} tAPP_PAN_CB;

tAPP_PAN_CB app_pan_cb;

/*
 * sample protocol and multicast filters for test
 */
typedef struct
{
    UINT16 start;
    UINT16 end;
} tAPP_PAN_PFILTER;

typedef struct
{
    UINT16 size;
    tAPP_PAN_PFILTER *filter;
} tAPP_PAN_PFILTER_LIST;

typedef struct
{
    BD_ADDR start;
    BD_ADDR end;
} tAPP_PAN_MFILTER;

typedef struct
{
    UINT16 size;
    tAPP_PAN_MFILTER *filter;
} tAPP_PAN_MFILTER_LIST;

tAPP_PAN_PFILTER app_pan_pfilter1[] = {
    { 0x0800, 0x0800 },
    { 0x86DD, 0x86DD },
    { 0x0806, 0x0806 },
};

tAPP_PAN_PFILTER app_pan_pfilter2[] = {
    { 0x86DD, 0x86DD },
    { 0x0806, 0x0806 },
};

tAPP_PAN_PFILTER_LIST app_pan_pfilter_list[] = {
    { sizeof(app_pan_pfilter1), app_pan_pfilter1 },
    { sizeof(app_pan_pfilter2), app_pan_pfilter2 },
};

tAPP_PAN_MFILTER app_pan_mfilter1[] = {
    {{ 0x01, 0x00, 0x5E, 0x00, 0x00, 0x01 },
            { 0x01, 0x00, 0x5E, 0x00, 0x00, 0x01 }},
    {{ 0x01, 0x00, 0x5E, 0x7F, 0xFF, 0xFA },
            { 0x01, 0x00, 0x5E, 0x7F, 0xFF, 0xFA }},
    {{ 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB },
            { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB }},
    {{ 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFC },
            { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFC }},
    {{ 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFD },
            { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFD }},
    {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
            { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }},
};

tAPP_PAN_MFILTER app_pan_mfilter2[] = {
    {{ 0x01, 0x00, 0x5E, 0x7F, 0xFF, 0xFA },
            { 0x01, 0x00, 0x5E, 0x7F, 0xFF, 0xFA }},
    {{ 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB },
            { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB }},
    {{ 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFC },
            { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFC }},
    {{ 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFD },
            { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFD }},
    {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
            { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }},
};

tAPP_PAN_MFILTER app_pan_mfilter3[] = {
    {{ 0x03, 0x00, 0x02, 0x30, 0x00, 0x02},
            { 0x03, 0x00, 0x02, 0x30, 0x00, 0x04}},
    {{ 0x03, 0x00, 0x02, 0x30, 0x00, 0x06},
            { 0x03, 0x00, 0x02, 0x30, 0x00, 0x08}},
};

tAPP_PAN_MFILTER_LIST app_pan_mfilter_list[] = {
    { sizeof(app_pan_mfilter1), app_pan_mfilter1 },
    { sizeof(app_pan_mfilter2), app_pan_mfilter2 },
    { sizeof(app_pan_mfilter3), app_pan_mfilter3 },
};

/*
 * Local functions
 */
static int app_pan_con_alloc(void);
static int app_pan_con_setup(tBSA_PAN_HNDL handle, BD_ADDR bd_addr,
        tBSA_PAN_ROLE local_role, tBSA_PAN_ROLE peer_role,
        tUIPC_CH_ID uipc_channel);
static int app_pan_con_find_by_handle(tBSA_PAN_HNDL handle);
static int app_pan_con_find_by_uipc_channel(tUIPC_CH_ID uipc_channel);
static void app_pan_con_free(int con_index);
static void app_pan_con_free_all(void);

static inline int is_empty_eth_addr(const BD_ADDR addr);
static inline int is_valid_bt_eth_addr(const BD_ADDR addr);

static void app_pan_cback(tBSA_PAN_EVT event, tBSA_PAN_MSG *p_data);
static void app_pan_rx_open_evt(tBSA_PAN_MSG *p_data);
static void app_pan_rx_close_evt(tBSA_PAN_MSG *p_data);
static void app_pan_rx_pfilt_evt(tBSA_PAN_MSG *p_data);
static void app_pan_rx_mfilt_evt(tBSA_PAN_MSG *p_data);

/* Common UIPC Callback function */
static void app_pan_uipc_cback(BT_HDR *p_msg);
static BT_HDR *app_pan_uipc_evt_close_hdlr(int con_index, BT_HDR *p_msg);
static BT_HDR *app_pan_uipc_evt_rx_data_ready_hdlr(int con_index,
        BT_HDR *p_msg);
static BT_HDR *app_pan_uipc_evt_tx_data_ready_hdlr(int con_index,
        BT_HDR *p_msg);
static void app_pan_uipc_rx_runq(int con_index);
static void app_pan_uipc_rx_msg_hdlr(int con_index, BT_HDR *p_msg);
static BT_HDR *app_pan_uipc_rx_data_hdlr(int con_index, BT_HDR *p_msg);
static void app_pan_uipc_rx_data_runq(int con_index);
static void app_pan_uipc_tx_data_runq(int con_index);

static const char *app_pan_get_role_desc(tBSA_PAN_ROLE role);

static int app_pan_netif_open(void);
static int app_pan_netif_close(void);
static int app_pan_netif_send(BT_HDR *p_msg);
static void app_pan_netif_read_thread(void);

/******************************************************************************
 **
 ** Function         app_pan_con_alloc
 **
 ** Description      Allocate con entry
 **
 ** Returns          con_index
 **
 ******************************************************************************/
static int app_pan_con_alloc(void)
{
    int con_index;

    for(con_index = 0; con_index < APP_PAN_NB_CON_MAX; con_index++)
    {
        if (app_pan_cb.con[con_index].in_use == FALSE)
        {
            memset(app_pan_cb.con + con_index, 0, sizeof(app_pan_cb.con[0]));
            app_pan_cb.con[con_index].in_use = TRUE;
            app_pan_cb.con[con_index].tx_flow_stop = FALSE;
            app_pan_cb.con[con_index].rx_flow_stop = FALSE;
            app_pan_cb.con[con_index].pfilter_num = 0;
            app_pan_cb.con[con_index].mfilter_num = 0;
            
            app_init_mutex(&app_pan_cb.con[con_index].tx_mutex);

            GKI_init_q(&app_pan_cb.con[con_index].uipc_q);
            GKI_init_q(&app_pan_cb.con[con_index].tx_q);
            GKI_init_q(&app_pan_cb.con[con_index].rx_q);

            break;
        }
    }

    return con_index;
}

/******************************************************************************
 **
 ** Function         app_pan_con_setup
 **
 ** Description      Allocate and setup con entry
 **
 ** Returns          con_index
 **
 ******************************************************************************/
static int app_pan_con_setup(tBSA_PAN_HNDL handle, BD_ADDR bd_addr,
        tBSA_PAN_ROLE local_role, tBSA_PAN_ROLE peer_role,
        tUIPC_CH_ID uipc_channel)
{
    int con_index;

    con_index = app_pan_con_alloc();
    if (con_index < APP_PAN_NB_CON_MAX)
    {
        app_pan_cb.con[con_index].handle = handle;
        bdcpy(app_pan_cb.con[con_index].bd_addr, bd_addr);
        app_pan_cb.con[con_index].local_role = local_role;
        app_pan_cb.con[con_index].peer_role = peer_role;
        app_pan_cb.con[con_index].uipc_channel = uipc_channel;

        if (UIPC_Open(uipc_channel, app_pan_uipc_cback) == TRUE)
        {
            app_pan_cb.con[con_index].uipc_is_open = TRUE;
        }
    }

    return con_index;
}

/******************************************************************************
 **
 ** Function         app_pan_con_find_by_handle
 **
 ** Description      Find valid con entry by handle
 **
 ** Returns          con_index
 **
 ******************************************************************************/
static int app_pan_con_find_by_handle(tBSA_PAN_HNDL handle)
{
    int con_index;

    for(con_index = 0; con_index < APP_PAN_NB_CON_MAX; con_index++)
    {
        if ((app_pan_cb.con[con_index].in_use != FALSE) &&
                (app_pan_cb.con[con_index].handle == handle))
        {
            break;
        }
    }

    return con_index;
}

/******************************************************************************
 **
 ** Function         app_pan_con_find_by_uipc_channel
 **
 ** Description      Find valid con entry by UIPC channel
 **
 ** Returns          con_index
 **
 ******************************************************************************/
static int app_pan_con_find_by_uipc_channel(tUIPC_CH_ID uipc_channel)
{
    int con_index;

    for(con_index = 0; con_index < APP_PAN_NB_CON_MAX; con_index++)
    {
        if ((app_pan_cb.con[con_index].in_use != FALSE) &&
                (app_pan_cb.con[con_index].uipc_channel == uipc_channel))
        {
            break;
        }
    }

    return con_index;
}

/******************************************************************************
 **
 ** Function         app_pan_con_free
 **
 ** Description      Free con entry
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_con_free(int con_index)
{
    BT_HDR *p_buf;

    app_pan_cb.con[con_index].in_use = FALSE;

    if (app_pan_cb.con[con_index].tx_flow_stop)
    {
        app_pan_cb.con[con_index].tx_flow_stop = FALSE;
        app_unlock_mutex(&app_pan_cb.con[con_index].tx_mutex);
    }
    app_delete_mutex(&app_pan_cb.con[con_index].tx_mutex);

    while((p_buf = GKI_dequeue(&app_pan_cb.con[con_index].uipc_q)) != NULL)
    {
        GKI_freebuf(p_buf);
    }
    while((p_buf = GKI_dequeue(&app_pan_cb.con[con_index].tx_q)) != NULL)
    {
        GKI_freebuf(p_buf);
    }
    while((p_buf = GKI_dequeue(&app_pan_cb.con[con_index].rx_q)) != NULL)
    {
        GKI_freebuf(p_buf);
    }

    UIPC_Close(app_pan_cb.con[con_index].uipc_channel);
}

/******************************************************************************
 **
 ** Function         app_pan_con_free_all
 **
 ** Description      Free all con entries
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_con_free_all(void)
{
    int con_index;

    for(con_index = 0; con_index < APP_PAN_NB_CON_MAX; con_index++)
    {
        if (app_pan_cb.con[con_index].in_use != FALSE)
        {
            app_pan_con_free(con_index);
        }
    }
}

/******************************************************************************
 **
 ** Function         is_empty_eth_addr
 **
 ** Description      Check whether ether address is empty or not
 **
 ** Returns          boolean (1 = empty address)
 **
 ******************************************************************************/
static inline int is_empty_eth_addr(const BD_ADDR addr)
{
    int i;

    for(i = 0; i < BD_ADDR_LEN; i++)
    {
        if(addr[i] != 0)
        {
            return 0;
        }
    }

    return 1;
}

/******************************************************************************
 **
 ** Function         is_valid_bt_eth_addr
 **
 ** Description      Check whether bt ether address is valid or not
 **
 ** Returns          boolean (1 = valid)
 **
 ******************************************************************************/
static inline int is_valid_bt_eth_addr(const BD_ADDR addr)
{
    if(is_empty_eth_addr(addr))
    {
        return 0;
    }

    return (addr[0] & 1) ? 0 : 1; /* Cannot be multicasting address */
}

/******************************************************************************
 **
 ** Function         app_pan_cback
 **
 ** Description      Example of PAN callback function
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_cback(tBSA_PAN_EVT event, tBSA_PAN_MSG *p_data)
{
    APP_DEBUG1("app_pan_cback event:%d", event);

    switch (event)
    {
    case BSA_PAN_OPEN_EVT:
        app_pan_rx_open_evt(p_data);

        break;

    case BSA_PAN_CLOSE_EVT:
        app_pan_rx_close_evt(p_data);

        break;

    case BSA_PAN_PFILT_EVT:
        app_pan_rx_pfilt_evt(p_data);

        break;

    case BSA_PAN_MFILT_EVT:
        app_pan_rx_mfilt_evt(p_data);

        break;

    default:
        APP_ERROR1("app_pan_cback unknown event:%d", event);

        break;
    }
}

/******************************************************************************
 **
 ** Function         app_pan_rx_open_evt
 **
 ** Description      handle open event from PAN callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_rx_open_evt(tBSA_PAN_MSG *p_data)
{
    int con_index;

    APP_DEBUG1("BSA_PAN_OPEN_EVT status:%d", p_data->open.status);
    APP_DEBUG1("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->open.bd_addr[0], p_data->open.bd_addr[1],
            p_data->open.bd_addr[2], p_data->open.bd_addr[3],
            p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
    APP_DEBUG1("\tHandle:%d Local role:%s (%d) Peer:%s (%d)",
            p_data->open.handle,
            app_pan_get_role_desc(p_data->open.local_role),
            p_data->open.local_role,
            app_pan_get_role_desc(p_data->open.peer_role),
            p_data->open.peer_role);

    if (p_data->open.status == BSA_SUCCESS)
    {
        con_index = app_pan_con_setup(p_data->open.handle,
            p_data->open.bd_addr, p_data->open.local_role,
            p_data->open.peer_role, p_data->open.uipc_channel);
        if (con_index != APP_PAN_NB_CON_MAX)
        {
            /* Update the Remote device xml file */
            app_read_xml_remote_devices();
            if (app_pan_cb.con[con_index].peer_role & BSA_PAN_ROLE_PANU)
            {
                app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_data->open.bd_addr,
                        BSA_PANU_SERVICE_MASK);
            }
            if (app_pan_cb.con[con_index].peer_role & BSA_PAN_ROLE_GN)
            {
                app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_data->open.bd_addr,
                        BSA_GN_SERVICE_MASK);
            }
            if (app_pan_cb.con[con_index].peer_role & BSA_PAN_ROLE_NAP)
            {
                app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_data->open.bd_addr,
                        BSA_NAP_SERVICE_MASK);
            }
            app_write_xml_remote_devices();
        }
    }
    else
    {
        APP_ERROR0("Fail : BSA_PAN_OPEN_EVT");
    }

    /* Display all PAN connections */
    app_pan_con_display();
}

/******************************************************************************
 **
 ** Function         app_pan_rx_close_evt
 **
 ** Description      handle close event from PAN callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_rx_close_evt(tBSA_PAN_MSG *p_data)
{
    int con_index;

    APP_DEBUG1("BSA_PAN_CLOSE_EVT Handle:%d", p_data->close.handle);

    con_index = app_pan_con_find_by_handle(p_data->close.handle);
    if (con_index == APP_PAN_NB_CON_MAX)
    {
        return;
    }

    app_pan_con_free(con_index);

    app_pan_con_display();
}

/******************************************************************************
 **
 ** Function         app_pan_rx_pfilt_evt
 **
 ** Description      handle protocol filter event from PAN callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_rx_pfilt_evt(tBSA_PAN_MSG *p_data)
{
    int con_index;
    UINT8 i;

    APP_DEBUG1("BSA_PAN_PFILT_EVT  indication:%d, length:%d, status:%d",
         p_data->pfilt.indication, p_data->pfilt.len, p_data->pfilt.status);

    con_index = app_pan_con_find_by_handle(p_data->pfilt.handle);
    if (con_index == APP_PAN_NB_CON_MAX)
    {
        APP_ERROR1("app_pan_rx_pfilt_evt Handle:%d", p_data->pfilt.handle);

        return;
    }

    app_pan_cb.con[con_index].pfilter_num = p_data->pfilt.len /
            (sizeof(UINT16) * 2);
    if (app_pan_cb.con[con_index].pfilter_num > BSA_PAN_MAX_PROT_FILTERS)
    {
        app_pan_cb.con[con_index].pfilter_num = 0;

        APP_ERROR1("app_pan_rx_pfilt_evt length:%d",
                p_data->pfilt.len / (sizeof(UINT16) * 2));

        return;
    }

    for (i = 0; i < app_pan_cb.con[con_index].pfilter_num ; i ++)
    {
        app_pan_cb.con[con_index].pfilter_start[i] =
                ntohs(p_data->pfilt.data[(i * 2)]);
        app_pan_cb.con[con_index].pfilter_end[i] =
                ntohs(p_data->pfilt.data[(i * 2) + 1]);

        APP_DEBUG1("    Start 0x%04x    End 0x%04x",
                app_pan_cb.con[con_index].pfilter_start[i],
                app_pan_cb.con[con_index].pfilter_end[i]);
    }
}

/******************************************************************************
 **
 ** Function         app_pan_rx_pfilt_evt
 **
 ** Description      handle protocol filter event from PAN callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_rx_mfilt_evt(tBSA_PAN_MSG *p_data)
{
    int con_index;
    UINT8 *p;
    UINT8 i;

    APP_DEBUG1("BSA_PAN_MFILT_EVT  indication:%d, length:%d status:%d",
         p_data->mfilt.indication, p_data->mfilt.len, p_data->mfilt.status);

    con_index = app_pan_con_find_by_handle(p_data->mfilt.handle);
    if (con_index == APP_PAN_NB_CON_MAX)
    {
        APP_ERROR1("app_pan_rx_mfilt_evt Handle:%d", p_data->mfilt.handle);

        return;
    }

    app_pan_cb.con[con_index].mfilter_num = p_data->mfilt.len /
            (sizeof(BD_ADDR) * 2);
    if (app_pan_cb.con[con_index].mfilter_num > BSA_PAN_MAX_MULTI_FILTERS)
    {
        APP_ERROR1("app_pan_rx_mfilt_evt length:%d",
                p_data->mfilt.len / (sizeof(BD_ADDR) * 2));

        return;
    }

    p = (UINT8*)p_data->mfilt.data;
    for (i = 0; i < app_pan_cb.con[con_index].mfilter_num; i++)
    {
        bdcpy((UINT8 *)&app_pan_cb.con[con_index].mfilter_start[i], p);
        p += sizeof(BD_ADDR);
        bdcpy((UINT8 *)&app_pan_cb.con[con_index].mfilter_end[i], p);
        p += sizeof(BD_ADDR);

        APP_DEBUG1("    Start  %02X:%02X:%02X:%02X:%02X:%02X",
            app_pan_cb.con[con_index].mfilter_start[i][0],
            app_pan_cb.con[con_index].mfilter_start[i][1],
            app_pan_cb.con[con_index].mfilter_start[i][2],
            app_pan_cb.con[con_index].mfilter_start[i][3],
            app_pan_cb.con[con_index].mfilter_start[i][4],
            app_pan_cb.con[con_index].mfilter_start[i][5]);
        APP_DEBUG1("    End    %02X:%02X:%02X:%02X:%02X:%02X",
            app_pan_cb.con[con_index].mfilter_end[i][0],
            app_pan_cb.con[con_index].mfilter_end[i][1],
            app_pan_cb.con[con_index].mfilter_end[i][2],
            app_pan_cb.con[con_index].mfilter_end[i][3],
            app_pan_cb.con[con_index].mfilter_end[i][4],
            app_pan_cb.con[con_index].mfilter_end[i][5]);
    }
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_cback
 **
 ** Description      uipc pan call back function.
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_uipc_cback(BT_HDR *p_msg)
{
    tUIPC_CH_ID uipc_channel;
    int con_index;

    /* Extract Channel Id from layer_specific field */
    uipc_channel = p_msg->layer_specific;

    con_index = app_pan_con_find_by_uipc_channel(uipc_channel);
    if (con_index == APP_PAN_NB_CON_MAX)
    {
        APP_ERROR1("UIPC invalid ChId:%d Event:%d", uipc_channel, p_msg->event);
    }
    else
    {
        /* Data ready to be read */
        switch (p_msg->event)
        {
        case UIPC_CLOSE_EVT:
            p_msg = app_pan_uipc_evt_close_hdlr(con_index, p_msg);

            break;

        case UIPC_RX_DATA_READY_EVT:
            p_msg = app_pan_uipc_evt_rx_data_ready_hdlr(con_index, p_msg);

            break;

        case UIPC_TX_DATA_READY_EVT:
            p_msg = app_pan_uipc_evt_tx_data_ready_hdlr(con_index, p_msg);

            break;

        default:
            APP_ERROR1("UIPC unknown event ChId:%d Event:%d", uipc_channel,
                    p_msg->event);

            break;
        }
    }

    /* Free the received buffer */
    if (p_msg != NULL)
    {
        GKI_freebuf(p_msg);
    }
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_evt_close_hdlr
 **
 ** Description      handle uipc close.
 **
 ** Parameters
 **
 ** Returns
 **
 ******************************************************************************/
static BT_HDR *app_pan_uipc_evt_close_hdlr(int con_index, BT_HDR *p_msg)
{
    app_pan_cb.con[con_index].uipc_is_open = FALSE;
    app_pan_cb.con[con_index].uipc_channel = UIPC_CH_ID_BAD;

    return p_msg;
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_evt_rx_data_ready_hdlr
 **
 ** Description      handle uipc rx data ready.
 **
 ** Parameters
 **
 ** Returns
 **
 ******************************************************************************/
static BT_HDR *app_pan_uipc_evt_rx_data_ready_hdlr(int con_index, BT_HDR *p_msg)
{
    BT_HDR *p_buf;
    int ret;

    while(app_pan_cb.con[con_index].uipc_is_open == TRUE &&
            app_pan_cb.con[con_index].rx_flow_stop == FALSE &&
            (app_pan_cb.con[con_index].uipc_q.count +
            app_pan_cb.con[con_index].rx_q.count) < APP_PAN_UIPC_FLOW_HWM)
    {
        p_buf = GKI_getbuf(sizeof(BT_HDR) + APP_PAN_UIPC_BUF_SZ);
        if (p_buf == NULL)
        {
            break;
        }
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = app_pan_cb.con[con_index].uipc_channel;

        ret = UIPC_Read(app_pan_cb.con[con_index].uipc_channel, &(p_buf->event),
                (UINT8 *)(p_buf + 1) + p_buf->offset, APP_PAN_UIPC_BUF_SZ);
        if (ret <= 0)
        {
            GKI_freebuf(p_buf);

            break;
        }
        p_buf->len = ret;

        GKI_enqueue(&app_pan_cb.con[con_index].uipc_q, p_buf);
    }

    app_pan_uipc_rx_runq(con_index);

    return p_msg;
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_evt_tx_data_ready_hdlr
 **
 ** Description      handle uipc rx data ready.
 **
 ** Parameters
 **
 ** Returns
 **
 ******************************************************************************/
static BT_HDR *app_pan_uipc_evt_tx_data_ready_hdlr(int con_index, BT_HDR *p_msg)
{
    if (app_pan_cb.con[con_index].tx_flow_stop == TRUE)
    {
        app_pan_cb.con[con_index].tx_flow_stop = FALSE;
        app_unlock_mutex(&app_pan_cb.con[con_index].tx_mutex);
    }

    app_pan_uipc_tx_data_runq(con_index);

    return p_msg;
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_rx_runq
 **
 ** Description      runq uipc rx message.
 **
 ** Parameters
 **
 ** Returns
 **
 ******************************************************************************/
static void app_pan_uipc_rx_runq(int con_index)
{
    BT_HDR *p_buf;

    while (1)
    {
        p_buf = app_pan_util_uipc_get_msg(&app_pan_cb.con[con_index].uipc_q);
        if (p_buf == NULL)
        {
            break;
        }

        app_pan_uipc_rx_msg_hdlr(con_index, p_buf);
    }

    if (app_pan_cb.con[con_index].rx_flow_stop == FALSE &&
            (app_pan_cb.con[con_index].uipc_q.count +
            app_pan_cb.con[con_index].rx_q.count) < APP_PAN_UIPC_FLOW_LWM)
    {
        UIPC_Ioctl(app_pan_cb.con[con_index].uipc_channel, UIPC_REQ_RX_READY,
                NULL);
    }
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_rx_msg_hdlr
 **
 ** Description      handle rx data packet.
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_uipc_rx_msg_hdlr(int con_index, BT_HDR *p_msg)
{
    switch(p_msg->event)
    {
    case BSA_PAN_UIPC_DATA_EVT:
        p_msg = app_pan_uipc_rx_data_hdlr(con_index, p_msg);

        break;

    default:
        APP_ERROR1("app_pan_uipc_rx_msg_hdlr Event:%d", p_msg->event);

        break;
    }

    /* Free the received buffer */
    if (p_msg != NULL)
    {
        GKI_freebuf(p_msg);
    }
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_rx_data_hdlr
 **
 ** Description      handle rx data packet.
 **
 ** Parameters
 **
 ** Returns
 **
 ******************************************************************************/
static BT_HDR *app_pan_uipc_rx_data_hdlr(int con_index, BT_HDR *p_msg)
{
    if (p_msg->len <= sizeof(tBSA_PAN_UIPC_DATA_MSG))
    {
        return p_msg;
    }

    GKI_enqueue(&app_pan_cb.con[con_index].rx_q, p_msg);

    app_pan_uipc_rx_data_runq(con_index);

    return NULL;
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_rx_data_runq
 **
 ** Description      runq uipc rx data.
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_uipc_rx_data_runq(int con_index)
{
    BT_HDR *p_buf;

    while(1)
    {
        p_buf = GKI_dequeue(&app_pan_cb.con[con_index].rx_q);
        if (p_buf == NULL)
        {
            break;
        }

        app_pan_netif_send(p_buf);

        GKI_freebuf(p_buf);
    }
}

/******************************************************************************
 **
 ** Function         app_pan_uipc_tx_data_runq
 **
 ** Description      runq uipc tx data.
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_uipc_tx_data_runq(int con_index)
{
    BT_HDR *p_buf;

    while (app_pan_cb.con[con_index].tx_flow_stop == FALSE)
    {
        p_buf = GKI_dequeue(&app_pan_cb.con[con_index].tx_q);
        if (p_buf == NULL)
        {
            break;
        }
        if (app_pan_cb.con[con_index].uipc_is_open == FALSE)
        {
            GKI_freebuf(p_buf);
        }
        else if (UIPC_SendBuf(app_pan_cb.con[con_index].uipc_channel, p_buf) ==
                FALSE)
        {
            if (p_buf->layer_specific == UIPC_LS_TX_FLOW_OFF)
            {
                GKI_enqueue_head(&app_pan_cb.con[con_index].tx_q, p_buf);
                UIPC_Ioctl(app_pan_cb.con[con_index].uipc_channel,
                        UIPC_REQ_TX_READY, NULL);
                app_lock_mutex(&app_pan_cb.con[con_index].tx_mutex);
                app_pan_cb.con[con_index].tx_flow_stop = TRUE;
            }
            else
            {
                GKI_freebuf(p_buf);
            }

            break;
        }
    }
}

/******************************************************************************
 **
 ** Function         app_pan_netif_open
 **
 ** Description      open and up network interface
 **
 ** Parameters
 **
 ** Returns          fd (-1 = error)
 **
 ******************************************************************************/
int app_pan_netif_open()
{
    int fd;
    int err;
    tAPP_XML_CONFIG app_xml_config;

    if (app_pan_cb.netif_fd != APP_PAN_BAD_NETIF_FD)
    {
        APP_ERROR1("Network interface already opened fd:%d",
                app_pan_cb.netif_fd);

        return -1;
    }

    err = app_read_xml_config(&app_xml_config);
    if (err < 0)
    {
        return -1;
    }

    fd = app_pan_netif.open(app_xml_config.bd_addr);
    if (fd < 0)
    {
        return -1;
    }

    app_pan_cb.netif_fd = fd;

    return app_pan_cb.netif_fd;
}

/******************************************************************************
 **
 ** Function         app_pan_netif_close
 **
 ** Description      close network interface
 **
 ** Parameters
 **
 ** Returns          stats (0 = success, -1 = error)
 **
 ******************************************************************************/
int app_pan_netif_close()
{
    int ret;

    if (app_pan_cb.netif_fd == APP_PAN_BAD_NETIF_FD)
    {
        return -1;
    }

    ret = app_pan_netif.close(app_pan_cb.netif_fd);

    app_pan_cb.netif_fd = APP_PAN_BAD_NETIF_FD;

    return ret;
}

/******************************************************************************
 **
 ** Function         app_pan_netif_send
 **
 ** Description      send data to network interface
 **
 ** Parameters
 **
 ** Returns          stats (0 = success, -1 = error)
 **
 ******************************************************************************/
static int app_pan_netif_send(BT_HDR *p_msg)
{
    tBSA_PAN_UIPC_DATA_MSG hdr;
    struct ether_header ehdr;
    int ret;
    int con_index;

    if (app_pan_cb.netif_fd < 0)
    {
        return -1;
    }

    app_pan_util_get_header(p_msg, &hdr, sizeof(hdr));

    con_index = app_pan_con_find_by_handle(hdr.handle);
    if (con_index == APP_PAN_NB_CON_MAX)
    {
        return -1;
    }

    if (is_empty_eth_addr(app_pan_cb.con[con_index].bd_addr))
    {
        if (is_valid_bt_eth_addr(hdr.src))
        {
            bdcpy(app_pan_cb.con[con_index].bd_addr, hdr.src);
        }
    }

    memcpy(&ehdr.ether_dhost, hdr.dst, ETH_ALEN);
    memcpy(&ehdr.ether_shost, hdr.src, ETH_ALEN);
    ehdr.ether_type = htons(hdr.protocol);

    app_pan_util_add_header(&p_msg, &ehdr, sizeof(ehdr), 0, 0);

    ret = app_pan_netif.write(app_pan_cb.netif_fd,
            (UINT8 *)(p_msg + 1) + p_msg->offset,
            p_msg->len);
    if (ret != (ssize_t)(p_msg->len))
    {
        return -1;
    }

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_netif_read_thread
 **
 ** Description      receive data from network interface
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_netif_read_thread(void)
{
    tBSA_PAN_UIPC_HDR hdr;
    tBSA_PAN_UIPC_DATA_REQ dreq;
    struct ether_header ehdr;
    BT_HDR *p_msg;
    BT_HDR *p_msg2;
    int ret;
    int con_index;

    APP_DEBUG0("app_pan_netif_read_thread");

    while (app_pan_cb.netif_fd < 0)
    {
        GKI_delay(1000);
    }

    APP_DEBUG0("network interface opened");

    memset(&dreq, 0, sizeof(dreq));

    p_msg = NULL;
    while (1)
    {
        if (p_msg == NULL)
        {
            p_msg = GKI_getbuf(sizeof(BT_HDR) + APP_PAN_UIPC_BUF_SZ);
            if (p_msg == NULL)
            {
                GKI_delay(100);

                continue;
            }
        }

        p_msg->offset = sizeof(hdr) + sizeof(dreq) - sizeof(ehdr);

        ret = app_pan_netif.read(app_pan_cb.netif_fd,
                (UINT8 *)(p_msg + 1) + p_msg->offset, ETH_FRAME_LEN);
        if (ret < 0)
        {
            APP_DEBUG1("read data from network interface. Status:%d", ret);
            GKI_freebuf(p_msg);

            break;
        }
        else if (ret <= (signed int)sizeof(ehdr))
        {
            APP_DEBUG1("Too short data from TAP. Length:%d", ret);

            continue;
        }
        p_msg->len = ret;

        app_pan_util_get_header(p_msg, &ehdr, sizeof(ehdr));

        p_msg->event = BSA_PAN_UIPC_DATA_CMD;
        memcpy(dreq.dst, &ehdr.ether_dhost, ETH_ALEN);
        memcpy(dreq.src, &ehdr.ether_shost, ETH_ALEN);
        dreq.protocol = ntohs(ehdr.ether_type);
        dreq.ext = 0;

        for(con_index = 0; con_index < APP_PAN_NB_CON_MAX; con_index++)
        {
            if (app_pan_cb.con[con_index].in_use != FALSE)
            {
                if ((dreq.dst[0] & 1) != 0 ||
                        bdcmp(app_pan_cb.con[con_index].bd_addr, dreq.dst) == 0
                        || app_pan_cb.con[con_index].local_role ==
                        BSA_PAN_ROLE_PANU)
                {
                    dreq.handle = app_pan_cb.con[con_index].handle;

                    /* wait for TX */
                    app_lock_mutex(&app_pan_cb.con[con_index].tx_mutex);
                    app_unlock_mutex(&app_pan_cb.con[con_index].tx_mutex);

                    p_msg2 = app_pan_util_dup(p_msg, sizeof(hdr) + sizeof(dreq),
                            0);
                    app_pan_util_add_header(&p_msg2, &dreq, sizeof(dreq), 0, 0);

                    hdr.evt = p_msg2->event;
                    hdr.len = p_msg2->len;
                    app_pan_util_add_header(&p_msg2, &hdr, sizeof(hdr), 0, 0);

                    GKI_enqueue(&app_pan_cb.con[con_index].tx_q, p_msg2);

                    app_pan_uipc_tx_data_runq(con_index);

                    /* if multicast packet, only one packet is forwarded */
                    if (dreq.dst[0] & 1)
                    {
                        break;
                    }
                }
            }
        }
    }
}

/******************************************************************************
 **
 ** Function         app_pan_con_display
 **
 ** Description      Function to display PAN connection status
 **
 ** Returns          void
 **
 ******************************************************************************/
void app_pan_con_display(void)
{
    int con_index;

    APP_DEBUG0("PAN Connections:");

    for(con_index = 0 ; con_index < APP_PAN_NB_CON_MAX ; con_index++)
    {
        APP_INFO1("Connection:%d Handle:%d InUse:%d local:%d(%s) peer:%d(%s) "
            "TxFC:%d RxFC:%d",
            con_index,
            app_pan_cb.con[con_index].handle,
            app_pan_cb.con[con_index].in_use,
            app_pan_cb.con[con_index].local_role,
            app_pan_get_role_desc(app_pan_cb.con[con_index].local_role),
            app_pan_cb.con[con_index].peer_role,
            app_pan_get_role_desc(app_pan_cb.con[con_index].peer_role),
            app_pan_cb.con[con_index].tx_flow_stop,
            app_pan_cb.con[con_index].rx_flow_stop);
    }
}

/******************************************************************************
 **
 ** Function         app_pan_get_role_desc
 **
 ** Description      Get PAN Role description.
 **
 ** Returns          PAN service Description
 **
 ******************************************************************************/
const char *app_pan_get_role_desc(tBSA_PAN_ROLE role)
{
    static const char *desc[8] =
    {
        "none",
        "PANU",
        "GN",
        "PANU GN",
        "NAP",
        "PANU NAP",
        "GN NAP",
        "PANU GN NAP",
    };

    if (role & ~7)
    {
        return "Bad PAN Role";
    }
    else
    {
        return desc[role & 7];
    }
}

/******************************************************************************
 **
 ** Function         app_pan_init
 **
 ** Description      Init PAN application
 **
 ** Returns          status
 **
 ******************************************************************************/
int app_pan_init(void)
{
    int ret;

    /* Reset structure */
    memset(&app_pan_cb, 0, sizeof(app_pan_cb));
    app_pan_cb.netif_fd = APP_PAN_BAD_NETIF_FD;

    ret = app_pan_netif.init();
    if (ret < 0)
    {
        return -1;
    }

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_start
 **
 ** Description      Start PAN application
 **
 ** Returns          status
 **
 ******************************************************************************/
int app_pan_start(void)
{
    tBSA_STATUS status = BSA_SUCCESS;

    tBSA_PAN_ENABLE enable_param;

    APP_DEBUG0("app_pan_start");

    status = BSA_PanEnableInit(&enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable PAN status:%d", status);
        return -1;
    }

    enable_param.p_cback = app_pan_cback;

    status = BSA_PanEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable PAN status:%d", status);

        return -1;
    }

    if (app_pan_netif_open() < 0)
    {
        APP_ERROR0("Unable to open network interface");

        app_pan_stop();

        return -1;
    }

    app_create_thread(app_pan_netif_read_thread, 0, 0,
            &app_pan_cb.netif_thread);

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_stop
 **
 ** Description      Stop PAN application
 **
 ** Returns          status
 **
 ******************************************************************************/
int app_pan_stop(void)
{
    tBSA_STATUS status;
    tBSA_PAN_DISABLE disable_param;

    APP_DEBUG0("app_pan_stop");

    status = BSA_PanDisableInit(&disable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanDisable failed status:%d", status);
        return -1;
    }

    status = BSA_PanDisable(&disable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanDisable failed status:%d", status);
        return -1;
    }

    app_stop_thread(app_pan_cb.netif_thread);

    app_pan_netif_close();

    app_pan_con_free_all();

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_set_role
 **
 ** Description      Example of function to set role
 **
 ** Returns          status
 **
 ******************************************************************************/
int app_pan_set_role(void)
{
    tBSA_STATUS status;
    tBSA_PAN_SET_ROLE param;
#define NUM_ROLE 3
    static struct {
        char *name;
        tBSA_PAN_ROLE role;
        char *srv_name;
        int  offset;
    } def_param[NUM_ROLE] = {
        {
            "PANU",
            BSA_PAN_ROLE_PANU,
            "BSA Network User",
            offsetof(tBSA_PAN_SET_ROLE, user_info)
        },
        {
            "GN",
            BSA_PAN_ROLE_GN,
            "BSA Group Adhoc Network",
            offsetof(tBSA_PAN_SET_ROLE, gn_info)
        },
        {
            "NAP",
            BSA_PAN_ROLE_NAP,
            "BSA Network Access Point",
            offsetof(tBSA_PAN_SET_ROLE, nap_info)
        }
    };
    tBSA_PAN_ROLE_INFO *p_role_info;
    int choice;
    int i;

    APP_DEBUG0("app_pan_set_role");

    status = BSA_PanSetRoleInit(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanSetRole fail:%d", status);

        return -1;
    }

    APP_INFO0("Which PAN role do you set ? [bit mask]:");
    for(i = 0; i < NUM_ROLE; i++)
    {
        APP_INFO1("   %s: %d", def_param[i].name, def_param[i].role);
    }

    choice = app_get_choice("Role");
    if (choice == 0 || (choice & ~(BSA_PAN_ROLE_PANU | BSA_PAN_ROLE_NAP |
        BSA_PAN_ROLE_GN)) != 0)
    {
        APP_ERROR1("Bad Role :%d", choice);
        return -1;
    }

    param.role = choice;

    for(i = 0; i < NUM_ROLE; i++)
    {
        if (param.role & def_param[i].role)
        {
            p_role_info = (tBSA_PAN_ROLE_INFO *)
                (((char *)&param) + def_param[i].offset);
            strncpy(p_role_info->srv_name, def_param[i].srv_name,
                BSA_SERVICE_NAME_LEN);
            p_role_info->srv_name[sizeof(p_role_info->srv_name)-1] = 0;
            p_role_info->app_id = def_param[i].role;
            p_role_info->sec_mask = BSA_SEC_AUTHENTICATION | BSA_SEC_ENCRYPTION;
        }
    }

    status = BSA_PanSetRole(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanSetRole fail:%d", status);
        return -1;
    }
    APP_INFO1("BSA_PanSetRole: role = %d", param.role);

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_open
 **
 ** Description      Example of function to open a PAN connection
 **
 ** Returns          void
 **
 ******************************************************************************/
int app_pan_open(void)
{
    tBSA_STATUS status = 0;
    int device_index;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_PAN_OPEN param;
    int choice;

    APP_DEBUG0("app_pan_open");

    app_pan_con_display();

    APP_INFO0("Bluetooth PAN menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    device_index = app_get_choice("Select choice");
    /* Devices from XML database */
    if (device_index == 0)
    {
        /* Read the Remote device XML file to have a fresh view */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
            p_name = app_xml_remote_devices_db[device_index].name;
        }
        else
        {
            APP_ERROR1("Bad Device index:%d", device_index);

            return -1;
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_disc_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) && (device_index < APP_DISC_NB_DEVICES) &&
                (app_discovery_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
            p_name = app_discovery_cb.devs[device_index].device.name;
        }
        else
        {
            APP_ERROR1("Bad Device index:%d", device_index);

            return -1;
        }
    }
    else
    {
        APP_ERROR1("Bad choice:%d", device_index);

        return -1;
    }

    status = BSA_PanOpenInit(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanOpen fail:%d", status);

        return -1;
    }

    /*
     * Local PAN Role selection
     */
    APP_INFO0("Which PAN role is local device ?");
    APP_INFO1("   PANU: %d", BSA_PAN_ROLE_PANU);
    APP_INFO1("   GN:   %d", BSA_PAN_ROLE_GN);
    APP_INFO1("   NAP:  %d", BSA_PAN_ROLE_NAP);

    choice = app_get_choice("Select local role");

    if (choice == BSA_PAN_ROLE_PANU)
    {
        param.local_role = BSA_PAN_ROLE_PANU;
    }
    else if (choice == BSA_PAN_ROLE_GN)
    {
        param.local_role = BSA_PAN_ROLE_GN;
    }
    else if (choice == BSA_PAN_ROLE_NAP)
    {
        param.local_role = BSA_PAN_ROLE_NAP;
    }
    else
    {
        APP_ERROR1("Bad local role selected :%d", choice);

        return -1;
    }

    /*
     * Peer PAN Role selection
     */
    APP_INFO0("Which PAN role is peer device ?");
    APP_INFO1("   PANU: %d", BSA_PAN_ROLE_PANU);
    APP_INFO1("   GN:   %d", BSA_PAN_ROLE_GN);
    APP_INFO1("   NAP:  %d", BSA_PAN_ROLE_NAP);

    choice = app_get_choice("Select peer role");

    if (choice == BSA_PAN_ROLE_PANU)
    {
        param.peer_role = BSA_PAN_ROLE_PANU;
    }
    else if (choice == BSA_PAN_ROLE_GN)
    {
        param.peer_role = BSA_PAN_ROLE_GN;
    }
    else if (choice == BSA_PAN_ROLE_NAP)
    {
        param.peer_role = BSA_PAN_ROLE_NAP;
    }
    else
    {
        APP_ERROR1("Bad peer role selected :%d", choice);

        return -1;
    }

    /* Save the BdAddr in the Application connection structure */
    bdcpy(param.bd_addr, bd_addr);

    APP_INFO1("Opening PAN connection to:%s", p_name);
    status = BSA_PanOpen(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanOpen fail:%d", status);

        return -1;
    }

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_close
 **
 ** Description      Example of function to close a PAN connection
 **
 ** Returns          int
 **
 ******************************************************************************/
int  app_pan_close()
{
    tBSA_STATUS status;
    tBSA_PAN_CLOSE close_param;
    int connection;

    APP_DEBUG0("app_pan_close");

    app_pan_con_display();
    connection = app_get_choice("Select connection to close");
    if ((connection < 0) ||
        (connection >= APP_PAN_NB_CON_MAX) ||
        (app_pan_cb.con[connection].in_use == FALSE))
    {
        APP_ERROR1("Bad Connection:%d", connection);

        return -1;
    }

    status = BSA_PanCloseInit(&close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanCloseInit fail:%d", status);

        return -1;
    }

    close_param.handle = app_pan_cb.con[connection].handle;

    APP_INFO0("Closing PAN connection");
    status = BSA_PanClose(&close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanClose fail:%d", status);

        return -1;
    }

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_set_pfilter
 **
 ** Description      Example of function to set a Network Protocol Type filter
 **
 ** Returns          int
 **
 ******************************************************************************/
int app_pan_set_pfilter()
{
    tBSA_PAN_PFILT pfilter;
    tBSA_STATUS status;
    int connection;
    UINT8 i;
    UINT8 j;

    status = BSA_PanSetPFilterInit(&pfilter);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanSetPFilterInit failed status:%d", status);

        return -1;
    }

    app_pan_con_display();
    connection = app_get_choice("Select connection to set pfilter");
    if ((connection < 0) ||
        (connection >= APP_PAN_NB_CON_MAX) ||
        (app_pan_cb.con[connection].in_use == FALSE))
    {
        APP_ERROR1("Bad Connection:%d", connection);

        return -1;
    }
    pfilter.handle = app_pan_cb.con[connection].handle;

    APP_INFO0("pfilter list:");
    for (i = 0; i < (sizeof(app_pan_pfilter_list) /
            sizeof(app_pan_pfilter_list[0])); i++)
    {
        APP_INFO1("List %d", i);
        for (j = 0; j < (app_pan_pfilter_list[i].size /
                sizeof(app_pan_pfilter_list[0].filter[0])); j++)
        {
            APP_INFO1("    Start:0x%04X    End:%04X",
                    app_pan_pfilter_list[i].filter[j].start,
                    app_pan_pfilter_list[i].filter[j].end);
        }
    }

    i = app_get_choice("Select pfilter");
    if (i >= (sizeof(app_pan_pfilter_list) / sizeof(app_pan_pfilter_list[0])))
    {
        APP_ERROR1("Bad pfilter:%d", i);

        return -1;
    }

    pfilter.num_filter = app_pan_pfilter_list[i].size /
            sizeof(app_pan_pfilter_list[0].filter[0]);
    if (pfilter.num_filter > BSA_PAN_MAX_PROT_FILTERS)
    {
        APP_ERROR0("Too big array");

        return -1;
    }

    for (j = 0; j < pfilter.num_filter; j++)
    {
        pfilter.data[j] = app_pan_pfilter_list[i].filter[j].start;
        pfilter.data[j + pfilter.num_filter] =
                app_pan_pfilter_list[i].filter[j].end;
    }

    status = BSA_PanSetPFilter(&pfilter);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanSetPFilter failed status:%d", status);

        return -1;
    }

    return 0;
}

/******************************************************************************
 **
 ** Function         app_pan_set_mfilter
 **
 ** Description      Example of function to set Multicast Address Type filter
 **
 ** Returns          int
 **
 ******************************************************************************/
int app_pan_set_mfilter()
{
    tBSA_PAN_MFILT mfilter;
    tBSA_STATUS status;
    int connection;
    BD_ADDR *p;
    BD_ADDR *q;
    UINT8 i;
    UINT8 j;

    status = BSA_PanSetMFilterInit(&mfilter);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanSetMFilterInit failed status:%d", status);

        return -1;
    }

    app_pan_con_display();
    connection = app_get_choice("Select connection to set mfilter");
    if ((connection < 0) ||
            (connection >= APP_PAN_NB_CON_MAX) ||
            (app_pan_cb.con[connection].in_use == FALSE))
    {
        APP_ERROR1("Bad Connection:%d", connection);

        return -1;
    }
    mfilter.handle = app_pan_cb.con[connection].handle;

    APP_INFO0("mfilter list:");
    for (i = 0; i < (sizeof(app_pan_mfilter_list) /
            sizeof(app_pan_mfilter_list[0])); i++)
    {
        APP_INFO1("List %d", i);
        for (j = 0; j < (app_pan_mfilter_list[i].size /
                sizeof(app_pan_mfilter_list[0].filter[0])); j++)
        {
            APP_INFO1("    Start  %02X:%02X:%02X:%02X:%02X:%02X     "
                    "End  %02X:%02X:%02X:%02X:%02X:%02X",
                    app_pan_mfilter_list[i].filter[j].start[0],
                    app_pan_mfilter_list[i].filter[j].start[1],
                    app_pan_mfilter_list[i].filter[j].start[2],
                    app_pan_mfilter_list[i].filter[j].start[3],
                    app_pan_mfilter_list[i].filter[j].start[4],
                    app_pan_mfilter_list[i].filter[j].start[5],
                    app_pan_mfilter_list[i].filter[j].end[0],
                    app_pan_mfilter_list[i].filter[j].end[1],
                    app_pan_mfilter_list[i].filter[j].end[2],
                    app_pan_mfilter_list[i].filter[j].end[3],
                    app_pan_mfilter_list[i].filter[j].end[4],
                    app_pan_mfilter_list[i].filter[j].end[5]);
        }
    }

    i = app_get_choice("Select mfilter");
    if (i >= (sizeof(app_pan_mfilter_list) / sizeof(app_pan_mfilter_list[0])))
    {
        APP_ERROR1("Bad mfilter:%d", i);

        return -1;
    }

    mfilter.num_filter = app_pan_mfilter_list[i].size /
            sizeof(app_pan_mfilter_list[0].filter[0]);
    if (mfilter.num_filter > BSA_PAN_MAX_MULTI_FILTERS)
    {
        APP_ERROR0("Too big array");

        return -1;
    }

    p = (BD_ADDR*)&mfilter.data[0];
    q = (BD_ADDR*)&mfilter.data[mfilter.num_filter];
    for (j = 0; j < mfilter.num_filter; j++)
    {
        bdcpy((UINT8 *)(p++), app_pan_mfilter_list[i].filter[j].start);
        bdcpy((UINT8 *)(q++), app_pan_mfilter_list[i].filter[j].end);
    }

    status = BSA_PanSetMFilter(&mfilter);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_PanSetMFilter failed status:%d", status);

        return -1;
    }

    return 0;
}
