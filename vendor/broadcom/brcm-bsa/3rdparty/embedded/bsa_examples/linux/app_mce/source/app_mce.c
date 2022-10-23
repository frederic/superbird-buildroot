/*****************************************************************************
**
**  Name:           app_mce.c
**
**  Description:    Bluetooth Message Access Profile client sample application
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
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
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"


#define MAX_PATH_LENGTH 512
#define DEFAULT_SEC_MASK (BSA_SEC_AUTHENTICATION | BSA_SEC_ENCRYPTION)

#define BSA_MA_32BIT_HEX_STR_SIZE               (8+1)


/* Callback to application for connection events */
typedef void (tMceCallback)(tBSA_MCE_EVT event, tBSA_MCE_MSG *p_data);
static tMceCallback *s_pMceCallback = NULL;

/*
* Types
*/
typedef struct
{
    int state;
    tBSA_MCE_INST_ID  instance_id;
    tBSA_MCE_SESS_HANDLE session_handle;
} tAPP_MCE_CONN;

#define APP_MCE_CONN_ST_IDLE            0
#define APP_MCE_CONN_ST_OPEN_PENDING    1
#define APP_MCE_CONN_ST_CONNECTED       2

#define APP_MCE_MAX_CONN    2

typedef struct
{
    BOOLEAN is_open;
    tAPP_MCE_CONN conn[APP_MCE_MAX_CONN];
    tBSA_STATUS last_open_status;
    tBSA_STATUS last_start_status;
    BD_ADDR bda_connected;

    BOOLEAN is_mns_started;
    BOOLEAN is_channel_open;
    tUIPC_CH_ID uipc_mce_rx_channel;
    tUIPC_CH_ID uipc_mce_tx_channel;
    BOOLEAN remove;
    int fd;
} tAPP_MCE_CB;

/*
* Global Variables
*/

tAPP_MCE_CB app_mce_cb;

int progress_sum;


/*******************************************************************************
**
** Function         app_mce_open_file
**
** Description      Open the temporary file that contains the push message that
**                  needs to be sent out in bmessage format.
**
** Parameters       none
**
** Returns          return the file descriptor
**
*******************************************************************************/
int app_mce_open_file()
{
    static char path[260];
    char* pStr = NULL;

    memset(path, 0, sizeof(path));
    pStr = getcwd(path, 260);
    strcat(path, "/tempoutmsg.txt");

    printf("app_mce_open_file\n");

    if(app_mce_cb.fd <= 0)
    {
        app_mce_cb.fd = open(pStr, O_RDONLY);
    }

    if(app_mce_cb.fd <= 0)
    {
        printf("Error could not open input file %s\n", pStr);
        return -1;
    }
    return app_mce_cb.fd;
}

/*******************************************************************************
**
** Function         app_mce_push_msg_close
**
** Description      Close the temporary file that was opened for push msg data
**
** Parameters       none
**
** Returns          none
**
*******************************************************************************/
void app_mce_push_msg_close()
{
    if(app_mce_cb.fd != -1)
    {
        close(app_mce_cb.fd);
        app_mce_cb.fd = -1;
    }
}

/*******************************************************************************
**
** Function         app_mce_send_push_msg_next_buf
**
** Description      BSA requests application to provide push_msg data that needs to be sent
**                  to peer device. Push message data needs to be a complete buffer of the outgoing
**                  message in bmessage format.
**
** Parameters       int nb_bytes  - number of bytes requested by BSA
**
** Returns          0 if success -1 if failure
**
*******************************************************************************/
int app_mce_send_push_msg_data_req(tBSA_MCE_MSG *p_data)
{
    int nb_bytes = p_data->push_data_req_msg.bytes_req;

    /*  Need to allocate the buffer for specfied number of bytes */
    char    *p_buf;
    if(  (p_buf = (char *) GKI_getbuf(nb_bytes + 1)) == NULL)
    {
        printf("app_mce_send_push_msg_data_req - COULD NOT ALLOCATE BUFFER \n");
        return FALSE;
    }

    memset(p_buf, 0, nb_bytes + 1);

    /* Read temporary file with outgoing message content in BMessage format and fill the buffer
       upto nb_bytes

       Your implementation could be different and might not necessarily use a temporary file to
       store outgoing message in BMessage format, in which case read() operation could be eliminated
       and supply data buffer directly to UIPC_Send.

       Data supplied to UIPC_Send has to be in BMessage format.
    */

    int num_read = read(app_mce_cb.fd, p_buf, nb_bytes);

    if(num_read < nb_bytes)
    {
        app_mce_push_msg_close();
    }

    if (num_read < 0)
    {
        printf(" app_mce_send_push_msg_data_req - error in reading file\n");
    }
    else if (TRUE != UIPC_Send(app_mce_cb.uipc_mce_tx_channel, 0, (UINT8 *) p_buf, num_read))
    {
        printf(" app_mce_send_push_msg_data_req - error in UIPC send could not send data \n");
    }

    GKI_freebuf(p_buf);

    return 0;
}

/*******************************************************************************
**
** Function         app_mce_new_connection
**
** Description      Get control block for a new connection
**
** Parameters
**
** Returns          tAPP_MCE_CONN *
**
*******************************************************************************/
static tAPP_MCE_CONN *app_mce_new_connection()
{
    tAPP_MCE_CONN *p_conn = NULL;
    int i;

    for (i = 0; i < APP_MCE_MAX_CONN; i++)
    {
        if (app_mce_cb.conn[i].state == APP_MCE_CONN_ST_IDLE)
        {
            p_conn = &app_mce_cb.conn[i];
            memset(p_conn, 0, sizeof(tAPP_MCE_CONN));
            break;
        }
    }

    if (p_conn == NULL)
    {
        printf("app_mce_new_connection could not find empty entry \n");
    }

    return p_conn;
}

/*******************************************************************************
**
** Function         app_mce_get_conn_by_inst_id
**
** Description      Get connection by instance ID
**
** Parameters       instance_id
**
** Returns          tAPP_MCE_CONN *
**
*******************************************************************************/
static tAPP_MCE_CONN *app_mce_get_conn_by_inst_id(tBSA_MCE_INST_ID  instance_id)
{
    tAPP_MCE_CONN *p_conn = NULL;
    int i;

    for (i = 0; i < APP_MCE_MAX_CONN; i++)
    {
        if (app_mce_cb.conn[i].state != APP_MCE_CONN_ST_IDLE &&
            app_mce_cb.conn[i].instance_id == instance_id)
        {
            p_conn = &app_mce_cb.conn[i];
            break;
        }
    }

    if (p_conn == NULL)
    {
        printf("app_mce_get_conn_by_inst_id could not find connection \n");
    }

    return p_conn;
}

/*******************************************************************************
**
** Function         app_mce_get_conn_by_sess_id
**
** Description      Get connection by session ID
**
** Parameters       session_id
**
** Returns          tAPP_MCE_CONN *
**
*******************************************************************************/
static tAPP_MCE_CONN *app_mce_get_conn_by_sess_id(tBSA_MCE_SESS_HANDLE  session_id)
{
    tAPP_MCE_CONN *p_conn = NULL;
    int i;

    for (i = 0; i < APP_MCE_MAX_CONN; i++)
    {
        if (app_mce_cb.conn[i].state != APP_MCE_CONN_ST_IDLE &&
            app_mce_cb.conn[i].session_handle == session_id)
        {
            p_conn = &app_mce_cb.conn[i];
            break;
        }
    }

    if (p_conn == NULL)
    {
        printf("app_mce_get_conn_by_sess_id could not find connection \n");
    }

    return p_conn;
}

/*******************************************************************************
**
** Function         app_mce_find_open_pending_conn
**
** Description      Find the open pending connection
**
** Parameters
**
** Returns          tAPP_MCE_CONN *
**
*******************************************************************************/
static tAPP_MCE_CONN *app_mce_find_open_pending_conn()
{
    tAPP_MCE_CONN *p_conn = NULL;
    int i;

    for (i = 0; i < APP_MCE_MAX_CONN; i++)
    {
        if (app_mce_cb.conn[i].state == APP_MCE_CONN_ST_OPEN_PENDING)
        {
            p_conn = &app_mce_cb.conn[i];
            break;
        }
    }

    if (p_conn == NULL)
    {
        printf("app_mce_find_open_pending_conn could not find open pending connection \n");
    }

    return p_conn;
}

/*******************************************************************************
**
** Function         app_mce_is_open
**
** Description      Check if there is an open connection
**
** Parameters
**
** Returns          BOOLEAN
**
*******************************************************************************/
static BOOLEAN app_mce_is_open()
{
    int i;

    for (i = 0; i < APP_MCE_MAX_CONN; i++)
    {
        if (app_mce_cb.conn[i].state == APP_MCE_CONN_ST_CONNECTED)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
 **
 ** Function        app_mce_uipc_cback
 **
 ** Description     uipc mce call back function.
 **
 ** Parameters      pointer on a buffer containing Message data with a BT_HDR header
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_mce_uipc_cback(BT_HDR *p_msg)
{
    UINT8 *p_buffer = NULL;
    int dummy = 0;
    int fd = -1;
    static char path[260];

    APPL_TRACE_DEBUG1("app_mce_uipc_cback response received... p_msg = %x", p_msg);

    if (p_msg == NULL)
    {
        return;
    }

    /* Make sure we are only handling UIPC_RX_DATA_EVT and other UIPC events. Ignore UIPC_OPEN_EVT and UIPC_CLOSE_EVT */
    if(p_msg->event != UIPC_RX_DATA_EVT)
    {
        APPL_TRACE_DEBUG1("app_mce_uipc_cback response received...event = %d",p_msg->event);
        GKI_freebuf(p_msg);
        return;
    }

    /* APPL_TRACE_DEBUG2("app_mce_uipc_cback response received...msg len = %d, p_buffer = %s", p_msg->len, p_buffer); */
    APPL_TRACE_DEBUG2("app_mce_uipc_cback response received...event = %d, msg len = %d",p_msg->event, p_msg->len);

    /* Check msg len */
    if(!(p_msg->len))
    {
        APPL_TRACE_DEBUG0("app_mce_uipc_cback response received. DATA Len = 0");
        GKI_freebuf(p_msg);
        return;
    }

    /* Get to the data buffer */
    p_buffer = ((UINT8 *)(p_msg + 1)) + p_msg->offset;

    /* Write received buffer to file */
    memset(path, 0, sizeof(path));
    getcwd(path, 260);
    strcat(path, "/get_msg.txt");

     APPL_TRACE_DEBUG1("app_mce_uipc_cback file path = %s", path);

    /* Delete temporary file */
    if(app_mce_cb.remove)
    {
        unlink(path);
        app_mce_cb.remove = FALSE;
    }

    /* Save incoming message content in temporary file */
    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    APPL_TRACE_DEBUG1("app_mce_uipc_cback fd = %d", fd);

    if(fd != -1)
    {
        dummy = write(fd, p_buffer, p_msg->len);
        APPL_TRACE_DEBUG2("app_mce_uipc_cback dummy = %d err = %d", dummy, errno);
        (void)dummy;
        close(fd);
    }

    /* APP_DUMP("MCE Data", p_buffer, p_msg->len); */
    GKI_freebuf(p_msg);
}

/*******************************************************************************
**
** Function         app_mce_handle_list_evt
**
** Description      Example of list event callback function
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_mce_handle_list_evt(tBSA_MCE_MSG *p_data)
{
    UINT8 * p_buf = p_data->list_data.data;
    UINT16 Index;

    for(Index = 0; Index < p_data->list_data.len; Index++)
    {
        printf("%c", p_buf[Index]);
    }
    printf("\n");

    printf("BSA_MCE_LIST_EVT num entry %d,is final %d, xml %d, len %d\n",
        p_data->list_data.num_entry, p_data->list_data.is_final, p_data->list_data.is_xml, p_data->list_data.len);
}

/*******************************************************************************
**
** Function         app_mce_handle_notif_evt
**
** Description      Example of notif event callback function
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_mce_handle_notif_evt(tBSA_MCE_MSG *p_data)
{
    UINT8 * p_buf = p_data->notif.data;
    UINT16 Index;

    printf("BSA_MCE_NOTIF_EVT - final %d, len %d\n", p_data->notif.final, p_data->notif.len);
    printf("Status : %d \n", p_data->notif.status);
    printf("Session handle: %d \n",  p_data->notif.session_handle);
    printf("MCE Instance ID: %d \n",  p_data->notif.instance_id);

    printf("Event Data: \n");
    /* Print out data buffer */
    for(Index = 0; Index < p_data->notif.len; Index++)
        printf("%c", p_buf[Index]);

    printf("\n");
}

/*******************************************************************************
**
** Function         app_mce_abort_evt
**
** Description      Response events received in response to ABORT command
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_mce_abort_evt(tBSA_MCE_MSG *p_data)
{
    APPL_TRACE_DEBUG0("app_mce_abort_evt received...");

}

/*******************************************************************************
**
** Function         app_mce_get_mas_instances_evt
**
** Description      Response to get MAS instances call
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_mce_get_mas_instances_evt(tBSA_MCE_MSG *p_data)
{
    APPL_TRACE_DEBUG0("app_mce_get_mas_instances_evt received...");

    int cnt = p_data->mas_instances.count;
    int i = 0;
    printf("Number of Instances: %d \n", cnt);

    for(i = 0; i < cnt; i++)
    {
        printf("Instance ID: %d \n", p_data->mas_instances.mas_ins[i].instance_id);
        printf("Message Type: %d \n",  p_data->mas_instances.mas_ins[i].msg_type);
        printf("Instance Name: %s \n",  p_data->mas_instances.mas_ins[i].instance_name);
    }
}

/*******************************************************************************
**
** Function         app_mce_open_evt
**
** Description      Response to get MAS instances call
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_mce_open_evt(tBSA_MCE_MSG *p_data)
{
    tAPP_MCE_CONN *p_conn;
    APPL_TRACE_DEBUG0("app_mce_open_evt received...");

    printf("Status : %d \n", p_data->open.status);
    printf("Session ID: %d \n",  p_data->open.session_handle);
    printf("MCE Instance ID: %d \n",  p_data->open.instance_id);
    printf("Remote bdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
           p_data->open.bd_addr[0], p_data->open.bd_addr[1], p_data->open.bd_addr[2], p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);

    if ((p_conn = app_mce_get_conn_by_inst_id(p_data->open.instance_id)) == NULL)
    {
        APP_ERROR0("Connection not found for open event");
        return;
    }

    app_mce_cb.last_open_status = p_data->open.status;

    if (p_data->open.status == BSA_SUCCESS)
    {
        bdcpy(app_mce_cb.bda_connected, p_data->open.bd_addr);
        app_mce_cb.is_open = TRUE;
        p_conn->session_handle = p_data->open.session_handle;
        p_conn->state = APP_MCE_CONN_ST_CONNECTED;

        if(!app_mce_cb.is_channel_open)
        {
            /* Once connected to PBAP Server, UIPC channel is returned. */
            /* Open the channel and register a callback to get phone book data. */
            /* Save UIPC channel */
            app_mce_cb.uipc_mce_rx_channel = p_data->open.uipc_mce_rx_channel;
            app_mce_cb.uipc_mce_tx_channel = p_data->open.uipc_mce_tx_channel;

            app_mce_cb.is_channel_open = TRUE;

            /* open the UIPC channel to receive the message related data */
            if (UIPC_Open(app_mce_cb.uipc_mce_rx_channel, app_mce_uipc_cback) == FALSE)
            {
                app_mce_cb.is_channel_open = FALSE;
                APP_ERROR0("Unable to open UIPC channel");
                return;
            }

            /* open the UIPC channel to transmit push message data to BSA Server*/
            if (UIPC_Open(app_mce_cb.uipc_mce_tx_channel, NULL) == FALSE)
            {
                app_mce_cb.is_channel_open = FALSE;
                APP_ERROR0("Unable to open UIPC channel");
                return;
            }
        }
    }
    else
    {
        p_conn->state = APP_MCE_CONN_ST_IDLE;
        APP_ERROR1("MCE open failed with error:%d", p_data->open.status);
    }
}

/*******************************************************************************
**
** Function         app_mce_close_evt
**
** Description      Response events received in response to CLOSE command
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_mce_close_evt(tBSA_MCE_MSG *p_data)
{
    tAPP_MCE_CONN *p_conn;
    APPL_TRACE_DEBUG0("app_mce_close_evt received...");

    p_conn = app_mce_get_conn_by_sess_id(p_data->close.session_handle);
    if (p_conn == NULL)
        p_conn = app_mce_find_open_pending_conn();

    if (p_conn  != NULL)
    {
        p_conn->state = APP_MCE_CONN_ST_IDLE;
    }

    app_mce_cb.is_open = app_mce_is_open();

    if(!app_mce_cb.is_open && app_mce_cb.is_channel_open)
    {
        /* close the UIPC channel receiving data */
        UIPC_Close(app_mce_cb.uipc_mce_rx_channel);
        UIPC_Close(app_mce_cb.uipc_mce_tx_channel);
        app_mce_cb.uipc_mce_rx_channel = UIPC_CH_ID_BAD;
        app_mce_cb.uipc_mce_tx_channel = UIPC_CH_ID_BAD;
        app_mce_cb.is_channel_open = FALSE;
    }
}

/*******************************************************************************
**
** Function         app_mce_cback
**
** Description      Example of MCE callback function to handle MCE related events
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_mce_cback(tBSA_MCE_EVT event, tBSA_MCE_MSG *p_data)
{
    switch (event)
    {
    case BSA_MCE_OPEN_EVT: /* Connection Open (for info) */
        progress_sum = 0;
        printf("BSA_MCE_OPEN_EVT Status %d\n", p_data->open.status);
        app_mce_open_evt(p_data);
        break;
    case BSA_MCE_CLOSE_EVT:
        printf("BSA_MCE_CLOSE_EVT\n");
        app_mce_close_evt(p_data);
        break;
    case BSA_MCE_START_EVT:
        printf("BSA_MCE_START_EVT\n");
        /* app_mce_start_evt(p_data); */
        if(p_data->mn_start.status == BSA_SUCCESS)
            app_mce_cb.is_mns_started = TRUE;
        else
            app_mce_cb.is_mns_started = FALSE;
        break;
    case BSA_MCE_STOP_EVT:
        printf("BSA_MCE_STOP_EVT\n");
        /* app_mce_stop_evt(p_data); */
        if(p_data->mn_stop.status == BSA_SUCCESS)
            app_mce_cb.is_mns_started = FALSE;
        break;
    case BSA_MCE_MN_OPEN_EVT:
        printf("BSA_MCE_MN_OPEN_EVT\n");
        /* app_mce_mn_open_evt(p_data); */
        break;
    case BSA_MCE_MN_CLOSE_EVT:
        printf("BSA_MCE_MN_CLOSE_EVT\n");
        /* app_mce_mn_close_evt(p_data); */
        break;
    case BSA_MCE_NOTIF_EVT:
        printf("BSA_MCE_NOTIF_EVT\n");
        app_mce_handle_notif_evt(p_data);
        break;
    case BSA_MCE_NOTIF_REG_EVT:
        printf("BSA_MCE_NOTIF_REG_EVT\n");
        /* app_mce_notif_reg_evt(p_data); */
        break;
    case BSA_MCE_SET_MSG_STATUS_EVT:
        printf("BSA_MCE_SET_MSG_STATUS_EVT\n");
        /* app_mce_set_msg_status_evt(p_data); */
        break;
    case BSA_MCE_UPDATE_INBOX_EVT:
        printf("BSA_MCE_UPDATE_INBOX_EVT\n");
        /* app_mce_update_inbox_evt(p_data); */
        break;
    case BSA_MCE_SET_FOLDER_EVT:
        printf("BSA_MCE_SET_FOLDER_EVT\n");
        /* app_mce_set_folder_evt(p_data); */
        break;
    case BSA_MCE_FOLDER_LIST_EVT:
        printf("BSA_MCE_FOLDER_LIST_EVT\n");
        app_mce_handle_list_evt(p_data);
        break;
    case BSA_MCE_MSG_LIST_EVT:
        printf("BSA_MCE_MSG_LIST_EVT\n");
        app_mce_handle_list_evt(p_data);
        break;
    case BSA_MCE_GET_MSG_EVT:
        printf("BSA_MCE_GET_MSG_EVT\n");
        /* app_mce_get_msg_evt(p_data); */
        break;
    case BSA_MCE_PUSH_MSG_DATA_REQ_EVT:
        printf("BSA_MCE_PUSH_MSG_DATA_REQ_EVT\n");
        app_mce_send_push_msg_data_req(p_data);
        break;
    case BSA_MCE_PUSH_MSG_EVT:
        printf("BSA_MCE_PUSH_MSG_EVT\n");
        app_mce_push_msg_close();
        /* app_mce_push_msg_evt(p_data); */
        break;
    case BSA_MCE_MSG_PROG_EVT:
        printf("BSA_MCE_MSG_PROG_EVT\n");
        /* app_mce_msg_prog_evt(p_data); */
        break;
    case BSA_MCE_OBEX_PUT_RSP_EVT:
        printf("BSA_MCE_OBEX_PUT_RSP_EVT\n");
        /* app_mce_obex_put_rsp_evt(p_data); */
        break;
    case BSA_MCE_OBEX_GET_RSP_EVT:
        printf("BSA_MCE_OBEX_GET_RSP_EVT\n");
        /* app_mce_obex_get_rsp_evt(p_data); */
        break;
    case BSA_MCE_DISABLE_EVT:
        printf("BSA_MCE_DISABLE_EVT\n");
        /* app_mce_stop_evt(p_data); */
        break;
    case BSA_MCE_INVALID_EVT:
        printf("BSA_MCE_INVALID_EVT\n");
        /* app_mce_invalid_evt(p_data); */
        break;
    case BSA_MCE_ABORT_EVT:
        printf("BSA_MCE_ABORT_EVT\n");
        app_mce_abort_evt(p_data);
        break;
    case BSA_MCE_GET_MAS_INSTANCES_EVT:
        printf("BSA_MCE_GET_MAS_INSTANCES_EVT\n");
        app_mce_get_mas_instances_evt(p_data);
        break;

    case BSA_MCE_GET_MAS_INS_INFO_EVT:
        APPL_TRACE_EVENT3("BSA_MCE_GET_MAS_INS_INFO_EVT status %d sess_id=%d  inst_id=%d",
                          p_data->get_mas_ins_info.status, p_data->get_mas_ins_info.session_handle,
                          p_data->get_mas_ins_info.instance_id);

        if(p_data->get_mas_ins_info.status == BSA_SUCCESS)
        {
            printf("BSA_MCE_GET_MAS_INS_INFO_EVT - Supported\n");
        }
        /*
        else if (p_data->get_mas_ins_info.status == BSA_ERROR_MCE_STATUS_UNSUP_FEATURES)
               {
                   printf("BSA_MCE_GET_MAS_INS_INFO_EVT - Unsupported Feature\n");
               }
        */
        else
        {
            printf("BSA_MCE_GET_MAS_INS_INFO_EVT - Failure");
        }
        break;

    default:
        fprintf(stderr, "app_mce_cback unknown event:%d\n", event);
        break;
    }

    /* forward the callback to registered applications */
    if(s_pMceCallback)
        s_pMceCallback(event, p_data);
}


/*******************************************************************************
**
** Function         app_mce_enable
**
** Description      Example of function to Enable MCE Client application
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mce_enable(tMceCallback pcb)
{
    tBSA_STATUS status = BSA_SUCCESS;

    tBSA_MCE_ENABLE enable_param;
    /* register callback */
    s_pMceCallback = pcb;

    printf("app_start_mce\n");

    /* Initialize the control structure */
    memset(&app_mce_cb, 0, sizeof(app_mce_cb));
    app_mce_cb.uipc_mce_rx_channel = UIPC_CH_ID_BAD;
    app_mce_cb.uipc_mce_tx_channel = UIPC_CH_ID_BAD;

    status = BSA_MceEnableInit(&enable_param);

    enable_param.p_cback = app_mce_cback;

    status = BSA_MceEnable(&enable_param);
    printf("app_start_mce Status: %d \n", status);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_start_mce: Unable to enable MCE status:%d\n", status);
    }
    return status;
}
/*******************************************************************************
**
** Function         app_mce_disable
**
** Description      Example of function to stop and disable MCE client module
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_disable(void)
{
    tBSA_STATUS status;

    tBSA_MCE_DISABLE disable_param;

    printf("app_stop_mce\n");

    if(app_mce_cb.is_channel_open)
    {
        /* close the UIPC channel receiving data */
        UIPC_Close(app_mce_cb.uipc_mce_rx_channel);
        UIPC_Close(app_mce_cb.uipc_mce_tx_channel);
        app_mce_cb.uipc_mce_rx_channel = UIPC_CH_ID_BAD;
        app_mce_cb.uipc_mce_tx_channel = UIPC_CH_ID_BAD;
        app_mce_cb.is_channel_open = FALSE;
    }

    status = BSA_MceDisableInit(&disable_param);

    status = BSA_MceDisable(&disable_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_stop_mce: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_open
**
** Description      Example of function to open up MCE connection to peer device
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_open(BD_ADDR bda, UINT8 instance_id)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_OPEN param;
    tAPP_MCE_CONN *p_conn;

    printf("app_mce_open\n");

    if ((p_conn = app_mce_new_connection()) == NULL)
        return -1;
    p_conn->instance_id = instance_id;
    p_conn->state = APP_MCE_CONN_ST_OPEN_PENDING;

    BSA_MceOpenInit(&param);
    bdcpy(param.bd_addr, bda);

    param.instance_id = instance_id;
    param.sec_mask = DEFAULT_SEC_MASK;

    status = BSA_MceOpen(&param);

    if (status != BSA_SUCCESS)
    {
        p_conn->state = APP_MCE_CONN_ST_IDLE;
        fprintf(stderr, "app_mce_open: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_close
**
** Description      Example of function to disconnect current connection
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_close(UINT8 instance_id)
{
    tBSA_STATUS status;
    tAPP_MCE_CONN *p_conn;
    tBSA_MCE_CLOSE close_param;

    printf("app_mce_close\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    status = BSA_MceCloseInit(&close_param);

    close_param.session_handle = p_conn->session_handle;

    status = BSA_MceClose(&close_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_close: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_close_all
**
** Description      Example of function to disconnect all connections
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_close_all()
{
    int i;

    printf("app_mce_close_all\n");

    for (i = 0; i < APP_MCE_MAX_CONN; i++)
    {
        if (app_mce_cb.conn[i].state != APP_MCE_CONN_ST_IDLE)
        {
            app_mce_close(app_mce_cb.conn[i].instance_id);
        }
    }

    return BSA_SUCCESS;
}

/*******************************************************************************
**
** Function         app_mce_cancel
**
** Description      cancel connections
**
** Returns          0 if success -1 if failure
*******************************************************************************/
int app_mce_cancel(UINT8 instance_id)
{
    tBSA_STATUS status;
    tBSA_MCE_CANCEL cancel_param;

    printf("app_mce_cancel\n");

    status = BSA_MceCancelInit(&cancel_param);

    cancel_param.instance_id = instance_id;

    status = BSA_MceCancel(&cancel_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_cancel: Error:%d\n", status);
    }
    return status;
}


/*******************************************************************************
**
** Function         app_mce_abort
**
** Description      Example of function to abort current operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_abort(UINT8 instance_id)
{
    tBSA_STATUS status;
    tBSA_MCE_ABORT abort_param;

    printf("app_mce_abort\n");

    status = BSA_MceAbortInit(&abort_param);

    bdcpy(abort_param.bd_addr, app_mce_cb.bda_connected);
    abort_param.instance_id = instance_id;

    status = BSA_MceAbort(&abort_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_abort: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_get_folder_list
**
** Description      Get Folder listing at the current level on existing connection to MAS
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_folder_list(UINT8 instance_id, UINT16 max_list_count, UINT16 start_offset)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_GET mceGet;
    tAPP_MCE_CONN *p_conn;

    printf("app_mce_get_folder_list\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    status = BSA_MceGetInit(&mceGet);

    mceGet.type = BSA_MCE_GET_FOLDER_LIST;

    mceGet.param.folderlist.session_handle = p_conn->session_handle;
    mceGet.param.folderlist.max_list_count = max_list_count;
    mceGet.param.folderlist.start_offset = start_offset;

    status = BSA_MceGet(&mceGet);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_get_folder_list: Error:%d\n", status);
    }

    return status;
}


/*******************************************************************************
**
** Function         bsa_mce_convert_hex_str_to_64bit_handle
**
** Description      Convert a hex string to a 64 bit message handle in Big Endian
**                  format
**
** Returns          void
**
*******************************************************************************/
void bsa_mce_convert_hex_str_to_64bit_handle(const char *p_hex_str, tBSA_MCE_MSG_HANDLE handle)
{
    UINT32 ul1, ul2;
    UINT8  *p;
    UINT8   str_len;
    char   tmp[BSA_MA_32BIT_HEX_STR_SIZE];

    APPL_TRACE_EVENT0("bsa_mce_convert_hex_str_to_64bit_handle");

    str_len = strlen(p_hex_str);
    memset(handle,0,sizeof(tBSA_MCE_MSG_HANDLE));

    if (str_len >= 8)
    {
        /* most significant 4 bytes */
        memcpy(tmp,p_hex_str,(str_len-8));
        tmp[str_len-8]='\0';
        ul1 = strtoul(tmp,0,16);
        p=handle;
        UINT32_TO_BE_STREAM(p, ul1);

        /* least significant 4 bytes */
        memcpy(tmp,&(p_hex_str[str_len-8]),8);
        tmp[8]='\0';
        ul2 = strtoul(tmp,0,16);
        p=&handle[4];
        UINT32_TO_BE_STREAM(p, ul2);
    }
    else
    {
        /* least significant 4 bytes */
        ul1 = strtoul(p_hex_str,0,16);
        p=&handle[4];
        UINT32_TO_BE_STREAM(p, ul1);
    }
}


/*******************************************************************************
**
** Function         app_mce_get_msg
**
** Description      Get the message for specified handle from MAS server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_msg(UINT8 instance_id, const char* phandlestr, BOOLEAN attachment)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_GET mceGet;

    tBSA_MCE_CHARSET    charset = BSA_MCE_CHARSET_UTF_8;
    tBSA_MCE_FRAC_REQ       frac_req = BSA_MCE_FRAC_REQ_NO;

    tBSA_MCE_MSG_HANDLE handle = {0};

    tAPP_MCE_CONN *p_conn;

    printf("app_mce_get_msg\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    bsa_mce_convert_hex_str_to_64bit_handle(phandlestr, handle);

    app_mce_cb.remove = TRUE;

    status = BSA_MceGetInit(&mceGet);
    mceGet.type = BSA_MCE_GET_MSG;
    mceGet.param.msg.session_handle = p_conn->session_handle;
    mceGet.param.msg.msg_param.attachment = attachment;
    mceGet.param.msg.msg_param.charset = charset;
    mceGet.param.msg.msg_param.fraction_request = frac_req;
    memcpy(mceGet.param.msg.msg_param.handle, handle, sizeof(tBSA_MCE_MSG_HANDLE));

    status = BSA_MceGet(&mceGet);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_get_msg: Error:%d\n", status);
    }

    return status;
}

/*******************************************************************************
**
** Function         app_mce_get_msg_list
**
** Description      Get message listing from the specified MAS server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_msg_list(UINT8 instance_id, const char* pfolder,
        tBSA_MCE_MSG_LIST_FILTER_PARAM* p_filter_param)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_GET mceGet;

    tAPP_MCE_CONN *p_conn;

    printf("app_mce_get_msg_list\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    status = BSA_MceGetInit(&mceGet);

    mceGet.type = BSA_MCE_GET_MSG_LIST;

    strncpy(mceGet.param.msglist.folder, pfolder, sizeof(mceGet.param.msglist.folder)-1);
    mceGet.param.msglist.folder[sizeof(mceGet.param.msglist.folder)-1] = 0;
    mceGet.param.msglist.session_handle = p_conn->session_handle;

    if (p_filter_param)
    {
        memcpy(&mceGet.param.msglist.filter_param, p_filter_param,
                sizeof(tBSA_MCE_MSG_LIST_FILTER_PARAM));
    }

    status = BSA_MceGet(&mceGet);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_get_msg_list: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_get_mas_ins_info
**
** Description      Get MAS info for the specified MAS instance id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_mas_ins_info(UINT8 instance_id)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_GET mceGet;

    tAPP_MCE_CONN *p_conn = NULL;
    int i;

    printf("app_mce_get_mas_ins_info\n");

    for (i = 0; i < APP_MCE_MAX_CONN; i++)
    {
        if (app_mce_cb.conn[i].state == APP_MCE_CONN_ST_CONNECTED)
        {
            p_conn = &app_mce_cb.conn[i];
            break;
        }
    }
    if (p_conn == NULL)
    {
        printf("No connected instance found \n");
        return -1;
    }

    status = BSA_MceGetInit(&mceGet);

    mceGet.type = BSA_MCE_GET_MAS_INST_INFO;
    mceGet.param.mas_info.session_handle = p_conn->session_handle;
    mceGet.param.mas_info.instance_id = instance_id;

    status = BSA_MceGet(&mceGet);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_get_mas_ins_info: Error:%d\n", status);
    }

    return status;
}

/*******************************************************************************
**
** Function         app_mce_get_mas_instances
**
** Description      Get MAS instances for the specified BD address
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_mas_instances(BD_ADDR bd_addr)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_GET mceGet;

    printf("app_mce_get_mas_instances\n");

    status = BSA_MceGetInit(&mceGet);
    mceGet.type = BSA_MCE_GET_MAS_INSTANCES;
    bdcpy(mceGet.param.mas_instances.bd_addr, bd_addr);
    status = BSA_MceGet(&mceGet);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_get_mas_instances: Error:%d\n", status);
    }

    return status;
}

/*******************************************************************************
**
** Function         app_mce_mn_start
**
** Description      Example of function to start notification service
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_mn_start(void)
{
    tBSA_STATUS status;
    tBSA_MCE_MN_START mn_start_param;
    char svc_name[BSA_MCE_SERVICE_NAME_LEN_MAX] = "MAP Notification Service";

    printf("app_mce_mn_start\n");

    status = BSA_MceMnStartInit(&mn_start_param);

    strcpy(mn_start_param.service_name, svc_name);

    mn_start_param.sec_mask = DEFAULT_SEC_MASK;

    status = BSA_MceMnStart(&mn_start_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_mn_start: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_mn_stop
**
** Description      Example of function to stop notification service
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_mn_stop(void)
{
    tBSA_STATUS status;

    tBSA_MCE_MN_STOP mn_stop_param;

    printf("app_mce_mn_stop\n");

    if(!app_mce_cb.is_mns_started)
        return BSA_ERROR_SRV_BAD_PARAM;

    status = BSA_MceMnStopInit(&mn_stop_param);

    status = BSA_MceMnStop(&mn_stop_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_mn_stop: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_notif_reg
**
** Description      Set the Message Notification status to On or OFF on the MSE.
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_notif_reg(UINT8 instance_id, BOOLEAN bReg)
{
    tBSA_STATUS status;
    tBSA_MCE_NOTIFYREG notif_reg_param;

    tAPP_MCE_CONN *p_conn;

    printf("app_mce_notif_reg\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    status = BSA_MceNotifRegInit(&notif_reg_param);

    notif_reg_param.session_handle = p_conn->session_handle;

    notif_reg_param.status = bReg ?   BSA_MCE_NOTIF_ON : BSA_MCE_NOTIF_OFF;

    status = BSA_MceNotifReg(&notif_reg_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_notif_reg: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_set_filter
**
** Description      Example of function to perform a set filter operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_set_filter(UINT8 filter)
{
    tBSA_STATUS status = 0;
    /*     TODO use a global FILTER PARAM AND
    Get values and store them
    TO BE USED IN SUBSEQUENT API CALLS WHERE NECESSARY/
    */
    return status;
}

/*******************************************************************************
**
** Function         app_mce_set_msg_sts
**
** Description      Example of function to perform a set message status operation
**
** Parameters
**                  BSA_MCE_STS_INDTR_DELETE : BSA_MCE_STS_INDTR_READ
**                  BSA_MCE_STS_VALUE_YES : BSA_MCE_STS_VALUE_NO
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_set_msg_sts(UINT8 instance_id, const char* phandlestr, int status_indicator,
        int status_value)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_SET set_param;

    tBSA_MCE_MSG_HANDLE handle = {0};

    tAPP_MCE_CONN *p_conn;

    printf("app_mce_set_msg_sts\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    status = BSA_MceSetInit(&set_param);

    if (!phandlestr || !strlen(phandlestr))
    {
        APP_ERROR0("app_mce_set_msg_sts invalid handle");
        return -1;
    }

    bsa_mce_convert_hex_str_to_64bit_handle(phandlestr,  handle);
    memcpy(set_param.param.msg_status.msg_handle, handle, sizeof(tBSA_MCE_MSG_HANDLE));

    set_param.param.msg_status.session_handle = p_conn->session_handle;
    set_param.param.msg_status.status_indicator = status_indicator;
    set_param.param.msg_status.status_value = status_value;

    set_param.type = BSA_MCE_SET_MSG_STATUS;

    status = BSA_MceSet(&set_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_set_msg_sts: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_set_folder
**
** Description      Example of function to perform a set folder operation
**
** Parameters
**                  BSA_MCE_DIR_NAV_UP_ONE_LVL / BSA_MCE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_set_folder(UINT8 instance_id, const char* p_string_dir, int direction_flag)
{
    tBSA_STATUS status = 0;
    tBSA_MCE_SET set_param;

    tAPP_MCE_CONN *p_conn;

    printf("app_mce_set_folder\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    if (!p_string_dir)
    {
        APP_ERROR0("app_mce_set_folder invalid folder");
        return -1;
    }

    status = BSA_MceSetInit(&set_param);

    set_param.param.folder.direction_flag = direction_flag;
    set_param.type = BSA_MCE_SET_FOLDER;
    set_param.param.folder.session_handle = p_conn->session_handle;
    strncpy(set_param.param.folder.folder, p_string_dir, sizeof(set_param.param.folder.folder)-1);
    set_param.param.folder.folder[sizeof(set_param.param.folder.folder)-1] = 0;

    status = BSA_MceSet(&set_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_set_folder: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_upd_inbox
**
** Description      Example of function to send update inbox request to MAS
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_upd_inbox(UINT8 instance_id)
{
    tBSA_STATUS status;
    tBSA_MCE_UPDATEINBOX upd_inbox;

    tAPP_MCE_CONN *p_conn;

    printf("app_mce_upd_inbox\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

    status = BSA_MceUpdateInboxInit(&upd_inbox);

    upd_inbox.session_handle = p_conn->session_handle;

    status = BSA_MceUpdateInbox(&upd_inbox);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_upd_inbox: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_mce_push_msg
**
** Description      Example of function to push a message to MAS
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_push_msg(UINT8 instance_id, const char* folder)
{
    tBSA_MCE_TRANSP_TYPE     trans = BSA_MCE_TRANSP_OFF;
    tBSA_MCE_RETRY_TYPE      retry = BSA_MCE_RETRY_OFF;
    tBSA_MCE_CHARSET         charset = BSA_MCE_CHARSET_UTF_8;

    tBSA_STATUS status;
    tBSA_MCE_PUSHMSG push_msg;

    tAPP_MCE_CONN *p_conn;

    printf("app_mce_push_msg\n");

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return -1;

/*     Open the file we just wrote out and be ready to send data to stack
    when it requests it. Data is sent over UIPC data channel */

    app_mce_open_file();

    status = BSA_McePushMsgInit(&push_msg);

    push_msg.session_handle = p_conn->session_handle;

    push_msg.msg_param.charset = charset;
    push_msg.msg_param.retry = retry;
    push_msg.msg_param.transparent = trans;

    strncpy(push_msg.msg_param.folder, folder, sizeof(push_msg.msg_param.folder)-1);
    push_msg.msg_param.folder[sizeof(push_msg.msg_param.folder)-1] = 0;

    status = BSA_McePushMsg(&push_msg);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_mce_push_msg: Error:%d\n", status);
    }
    return status;
}


/*******************************************************************************
**
** Function         mce_is_connected
**
** Description      Check if an mce instance is connected
**
** Parameters
**
** Returns          TRUE if connected, FALSE otherwise.
**
*******************************************************************************/
BOOLEAN mce_is_connected(BD_ADDR bda, UINT8 instance_id)
{
    tAPP_MCE_CONN *p_conn;

    if (app_mce_cb.is_open && bdcmp(bda, app_mce_cb.bda_connected) == 0)
    {
        if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) != NULL)
            return p_conn->state == APP_MCE_CONN_ST_CONNECTED;
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         mce_is_open_pending
**
** Description      Check if mce Open is pending
**
** Parameters
**
** Returns          TRUE if open is pending, FALSE otherwise
**
*******************************************************************************/
BOOLEAN mce_is_open_pending(UINT8 instance_id)
{
    tAPP_MCE_CONN *p_conn;

    if ((p_conn = app_mce_get_conn_by_inst_id(instance_id)) == NULL)
        return FALSE;

    return p_conn->state == APP_MCE_CONN_ST_OPEN_PENDING;
}

/*******************************************************************************
**
** Function         mce_is_open
**
** Description      Check if mce is open
**
** Parameters
**
** Returns          TRUE if mce is open, FALSE otherwise. Return BDA of the connected device
**
*******************************************************************************/
BOOLEAN mce_is_open(BD_ADDR bda)
{
    BOOLEAN bopen = app_mce_cb.is_open;
    if(bopen && bda)
    {
        bdcpy(bda, app_mce_cb.bda_connected);
    }

    return bopen;
}


/*******************************************************************************
**
** Function         mce_last_open_status
**
** Description      Get the last mce open status
**
** Parameters
**
** Returns          open status
**
*******************************************************************************/
tBSA_STATUS mce_last_open_status()
{
    return app_mce_cb.last_open_status;
}


/*******************************************************************************
**
** Function         mce_is_mns_started
**
** Description      Is MNS service started
**
** Parameters
**
** Returns          true if mns service has been started
**
*******************************************************************************/
tBSA_STATUS mce_is_mns_started()
{
    return app_mce_cb.is_mns_started;
}



