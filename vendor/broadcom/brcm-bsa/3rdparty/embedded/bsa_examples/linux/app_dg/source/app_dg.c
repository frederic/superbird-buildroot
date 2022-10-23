/*****************************************************************************
 **
 **  Name:           app_dg.c
 **
 **  Description:    Bluetooth Manager application
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
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include "gki.h"
#include "uipc.h"
#include "bsa_api.h"
#include "app_dg.h"
#include "bsa_trace.h"

#include "app_mutex.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"


/* Default BdAddr */
#define APP_DG_TX_BUFFER_SIZE   1024
#define APP_DG_RX_BUFFER_SIZE   1024

#define APP_DG_NB_CON_MAX       20   /* Maximum number of connections */
#define APP_DG_BAD_HANDLE       0xFF
#define APP_DG_BAD_UIPC         0xFF

/*
 * Globales Variables
 */

UINT8 rx_buffer[APP_DG_RX_BUFFER_SIZE];

typedef struct
{
    BOOLEAN in_use;
    BOOLEAN is_open;
    BOOLEAN is_server;
    BD_ADDR bd_addr;
    tBSA_DG_HNDL handle;
    tUIPC_CH_ID uipc_channel;
    BT_HDR *p_tx_pending;
    tBSA_SERVICE_ID service;
#ifdef BSA_USE_SERIAL_PORT
    int socat_pid;
    int serial_fd;
    pthread_t read_thread;
    int stop_tty_read;
#endif
} tAPP_DG_CON;

typedef struct
{
    tAPP_DG_CON connections[APP_DG_NB_CON_MAX];
    tAPP_MUTEX app_dg_tx_mutex[APP_DG_NB_CON_MAX];
    BOOLEAN loopback;
} tAPP_DG_CB;

tAPP_DG_CB app_dg_cb;

/*
 * Local functions
 */
int app_dg_con_alloc(void);
void app_dg_con_free(int connection);
void app_dg_con_display(void);
void app_dg_con_display_debug(void);
int app_dg_con_get_from_handle(tBSA_DG_HNDL handle, BOOLEAN *p_is_server);
int app_dg_con_get_from_uipc(tUIPC_CH_ID channel_id);
char *app_dg_get_srv_desc(tBSA_SERVICE_ID service);
int app_dg_sendbuf(int connection, BT_HDR *p_msg);
void app_dg_rx_open_evt(tBSA_DG_MSG *p_data);
void app_dg_rx_close_evt(tBSA_DG_MSG *p_data);
void app_dg_find_service_evt(tBSA_DG_MSG *p_data);
int app_dg_open_tty(int connection);
void app_dg_close_tty(int connection);
static void app_dg_sendto_vtty(char * rx_buffer,int length,int connection);
#ifdef BSA_USE_SERIAL_PORT
static void *app_dg_read_proc( void *ptr );
#endif

/* Common UIPC Callback function */
void app_dg_uipc_cback(BT_HDR *p_msg);

/*******************************************************************************
 **
 ** Function         app_dg_cback
 **
 ** Description      Example of DG callback function
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_cback(tBSA_DG_EVT event, tBSA_DG_MSG *p_data)
{
    switch (event)
    {
    case BSA_DG_OPEN_EVT: /* DG Connection Open  */
        app_dg_rx_open_evt(p_data);
        break;

    case BSA_DG_CLOSE_EVT:
        app_dg_rx_close_evt(p_data);
        break;

    case BSA_DG_FIND_SERVICE_EVT:
        app_dg_find_service_evt(p_data);
        break;
    default:
        APP_ERROR1("app_dg_cback unknown event:%d", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_dg_rx_close_evt
 **
 ** Description     handle close event from DG callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_rx_close_evt(tBSA_DG_MSG *p_data)
{
    int connection;
    BOOLEAN is_server;
    int status;

    APP_DEBUG1("BSA_DG_CLOSE_EVT Handle:%d", p_data->close.handle);

    /* Look for the connection structure associated */
    connection = app_dg_con_get_from_handle(p_data->close.handle, &is_server);
    if (connection >= APP_DG_NB_CON_MAX)
    {
        APP_ERROR0("No Connection structure match this DG link");
        return;
    }

    app_dg_close_tty(connection);
    /* free pending GKI buffer */
    if (app_dg_cb.connections[connection].p_tx_pending)
    {
        GKI_freebuf(app_dg_cb.connections[connection].p_tx_pending);
        app_dg_cb.connections[connection].p_tx_pending = NULL;
        APP_DEBUG1("DG Server Connection p_tx_pending free :%d", connection);
    }
    APP_DEBUG0("app_dg_cback unlock mutex");
    status = app_unlock_mutex(&app_dg_cb.app_dg_tx_mutex[connection]);
    if(status < 0)
    {
        APP_ERROR1("app_unlock_mutex failed status:%d", status);
    }
    if (is_server)
    {
        APP_DEBUG1("DG Server Connection :%d closed", connection);
        if (app_dg_cb.connections[connection].is_open)
        {
            /* For Server, mark this link as not connected but keep the
             * structure for future re-connection */
            app_dg_cb.connections[connection].is_open = FALSE;
            /* Close the UIPC Channel */
            UIPC_Close(app_dg_cb.connections[connection].uipc_channel);
            app_dg_cb.connections[connection].uipc_channel = UIPC_CH_ID_BAD;
        }
    }
    else
    {
        /* For client, free this connection structure */
        APP_DEBUG1("DG Client Connection:%d closed", connection);
        if (app_dg_cb.connections[connection].is_open)
        {
            UIPC_Close(app_dg_cb.connections[connection].uipc_channel);
        }
        app_dg_con_free(connection);
    }
    app_dg_con_display_debug();
}

/*******************************************************************************
 **
 ** Function         app_dg_rx_open_evt
 **
 ** Description     handle open event from DG callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_rx_open_evt(tBSA_DG_MSG *p_data)
{
    int connection;
    BOOLEAN is_server;
    int status;
    int index;

    APP_DEBUG1("BSA_DG_OPEN_EVT status:%d", p_data->open.status);
    APP_DEBUG1("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->open.bd_addr[0], p_data->open.bd_addr[1],
            p_data->open.bd_addr[2], p_data->open.bd_addr[3],
            p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
    APP_DEBUG1("\tHandle:%d UIPC Channel:%d Service:%s (%d)",
            p_data->open.handle,
            p_data->open.uipc_channel,
            app_dg_get_srv_desc(p_data->open.service),
            p_data->open.service);
    /* Look if the Handle match one of our server connection structure */
    connection = app_dg_con_get_from_handle(p_data->open.handle, &is_server);
    if (connection >= APP_DG_NB_CON_MAX)
    {
        /* no handle match, try to reuse a client one */
        for(index = 0 ; index < APP_DG_NB_CON_MAX ; index++)
        {
            /* If this connection is in use and BdAddr match */
            if ((app_dg_cb.connections[index].in_use == TRUE) &&
                (app_dg_cb.connections[index].is_open == FALSE) &&
                (app_dg_cb.connections[index].is_server == FALSE) &&
                (bdcmp(app_dg_cb.connections[index].bd_addr, p_data->open.bd_addr) == 0))
            {
                APP_DEBUG1("Found DG connection:%d matching BdAddr", index);
                connection = index;
                break;
            }
        }
        if (connection >= APP_DG_NB_CON_MAX)
        {
            APP_ERROR0("No Connection structure match this DG Connection");
            return;
        }
        is_server = FALSE;
    }
    if (is_server)
    {
        APP_DEBUG1("Incoming DG Connection for Server connection:%d", connection);
    }
    else
    {
        APP_DEBUG1("Outgoing Connection for Client:%d", connection);
    }
    if (p_data->open.status == BSA_SUCCESS)
    {
        app_dg_cb.connections[connection].is_open = TRUE;
        bdcpy(app_dg_cb.connections[connection].bd_addr, p_data->open.bd_addr);
        app_dg_cb.connections[connection].handle = p_data->open.handle;
        app_dg_cb.connections[connection].uipc_channel = p_data->open.uipc_channel;
        app_dg_cb.connections[connection].service = p_data->open.service;
        app_dg_open_tty(connection);
        /*  Open the UIPC Channel with the associated Callback */
        if (UIPC_Open(p_data->open.uipc_channel, app_dg_uipc_cback) == FALSE)
        {
            APP_ERROR1("UIPC_Open failed channel:%d", p_data->open.uipc_channel);
        }
        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();
        /* Add SPP, DUN services for this devices in XML database */
        app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->open.bd_addr,
                BSA_SPP_SERVICE_MASK | BSA_DUN_SERVICE_MASK );
        /* Update database => write on disk */
        app_write_xml_remote_devices();
        /* init Mutex */
        status = app_init_mutex(&app_dg_cb.app_dg_tx_mutex[connection]);
        if (status < 0)
        {
            APP_ERROR1("app_init_mutex failed status:%d", status);
            return;
        }
        status = app_lock_mutex(&app_dg_cb.app_dg_tx_mutex[connection]);
        if (status < 0)
        {
            APP_ERROR1("app_lock_mutex failed status:%d", status);
            return;
        }
    }
    else
    {
        APP_ERROR0("Fail : BSA_DG_OPEN_EVT");
        app_dg_con_free(connection);
    }
    /* Display all DG connections */
    app_dg_con_display();
    app_dg_con_display_debug();

}

/*******************************************************************************
 **
 ** Function         app_dg_find_service_evt
 **
 ** Description     handle find service event from DG callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_find_service_evt(tBSA_DG_MSG *p_data)
{
    APP_DEBUG1("BSA_DG_FIND_SERVICE_EVT status:%d", p_data->find_service.status);
    APP_DEBUG1("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->find_service.bd_addr[0], p_data->find_service.bd_addr[1],
            p_data->find_service.bd_addr[2], p_data->find_service.bd_addr[3],
            p_data->find_service.bd_addr[4], p_data->find_service.bd_addr[5]);

    APP_DEBUG1("\t Status:%d Found:%d Name:%s (SCN: %d)",
            p_data->find_service.status,
            p_data->find_service.found,
            p_data->find_service.name,
            p_data->find_service.scn);

}

/*******************************************************************************
 **
 ** Function         app_dg_uipc_cback
 **
 ** Description     uipc dg call back function.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_uipc_cback(BT_HDR *p_msg)
{
    int connection;
    tAPP_DG_CON *p_connection;
    tUIPC_CH_ID uipc_channel;
    UINT32 length;
    BT_HDR *p_tx_msg;
    UINT16 msg_evt;
    int status;

    /* Extract Channel Id from layer_specific field */
    uipc_channel = p_msg->layer_specific;

    connection = app_dg_con_get_from_uipc(uipc_channel);
    if (connection >= APP_DG_NB_CON_MAX)
    {
        APP_ERROR1("Bad Connection:%d", connection);
        return;
    }

    p_connection = &app_dg_cb.connections[connection];

    /* Data ready to be read */
    if (p_msg->event == UIPC_RX_DATA_READY_EVT)
    {
        APP_DEBUG1("UIPC UIPC_RX_DATA_READY_EVT ChId:%d", uipc_channel);
        do {
            /* Read the buffer */
            length = UIPC_Read(uipc_channel, &msg_evt, rx_buffer, sizeof(rx_buffer));

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
                scru_dump_hex(rx_buffer, "DG Rx Data", length, TRACE_LAYER_NONE,
                        TRACE_TYPE_DEBUG);

                app_dg_sendto_vtty((char *)rx_buffer,length,connection);

                if (app_dg_cb.loopback == TRUE)
                {
                    if (p_connection->p_tx_pending != NULL)
                    {
                        APP_ERROR1("UIPC Channel:%d Tx flow is Off (Tx buffer pending).=> No loopback", uipc_channel);
                    }
                    else
                    {
                        APP_DEBUG0("Data Loopback (for test)");
                        /* Allocate buffer for Tx data */
                        if ((p_tx_msg = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR) + length))
                                != NULL)
                        {
                            p_tx_msg->offset = 0;
                            p_tx_msg->len = length;
                            memcpy((UINT8 *)(p_tx_msg + 1), rx_buffer, length);
                            app_dg_sendbuf(connection, p_tx_msg);
                        }
                    }
                }
            }
        } while (length > 0);
    }
    else if (p_msg->event == UIPC_TX_DATA_READY_EVT)
    {
        APP_DEBUG1("UIPC UIPC_TX_DATA_READY_EVT ChId:%d", uipc_channel);
        if (p_connection->p_tx_pending != NULL)
        {
            APP_DEBUG0("cback unlock mutex");
            status = app_unlock_mutex(&app_dg_cb.app_dg_tx_mutex[connection]);
            if(status < 0)
            {
                APP_ERROR1("app_unlock_mutex failed status:%d", status);
            }
        }
        else
        {
            APP_DEBUG0("No pending Tx buffer");
        }
    }
    else if (p_msg->event == UIPC_RX_DATA_EVT)
    {
        APP_ERROR1("UIPC_RX_DATA_EVT should not be received form DG UIPC ChId:%d",
                uipc_channel);
    }
    /* Free the received buffer */
    GKI_freebuf(p_msg);
}

/*******************************************************************************
 **
 ** Function        app_dg_sendbuf
 **
 ** Description     Send buffer on data gateway UIPC channel
 **
 ** Parameters      connection: identifier of the connection to which send data
 **                 p_msg: GKI buffer containing the message to send
 **
 ** Returns         status: -1 if there was an error but message was freed, 0 if
 **                 successfully sent, 1 if not sent because it was flowcontrolled
 **
 *******************************************************************************/
int app_dg_sendbuf(int connection, BT_HDR *p_msg)
{
    BOOLEAN send_status;
    tAPP_DG_CON *p_connection;

    p_connection = &app_dg_cb.connections[connection];

    if ((p_connection->uipc_channel < UIPC_CH_ID_DG_FIRST) || (p_connection->uipc_channel > UIPC_CH_ID_DG_LAST))
    {
        APP_ERROR0("uipc connection is invalid");
        GKI_freebuf(p_msg);
        return -1;
    }

    if ((p_connection->p_tx_pending != NULL) &&
        (p_connection->p_tx_pending != p_msg))
    {
        APP_ERROR0("Another Tx buffer was pending");
        GKI_freebuf(p_msg);
        return -1;
    }

    send_status = UIPC_SendBuf(p_connection->uipc_channel, p_msg);

    if (send_status == TRUE)
    {
        /* Buffer sent correctly to BSA server => free it */
        p_connection->p_tx_pending = NULL;
        return 0;
    }
    /* Not able to send the buffer: check the cause */
    else
    {
        /* Check if it's because flow is off */
        if (p_msg->layer_specific == UIPC_LS_TX_FLOW_OFF)
        {
            APP_DEBUG0("flow control");
            /* Save this buffer for later TX (when flow will be restored) */
            p_connection->p_tx_pending = p_msg;

            /* Request UIPC to send an event when flow will be restored */
            UIPC_Ioctl(p_connection->uipc_channel, UIPC_REQ_TX_READY, NULL);
        }
        /* Unrecoverable error (e.g. UIPC not open) */
        else
        {
            APP_DEBUG1("SHOULD not print this message %s,%s,%d",__FILE__,__FUNCTION__,__LINE__);
            GKI_freebuf(p_msg);
        }

        return 1;
    }

}

/*******************************************************************************
 **
 ** Function         app_dg_read
 **
 ** Description      Example of function to read a data gateway port
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_dg_read(int connection)
{
    UINT16 event;
    UINT32 len;

    if (connection < 0)
    {
        app_dg_con_display();
        connection = app_get_choice("Select connection");
    }
    if ((connection < 0) || (connection >= APP_DG_NB_CON_MAX))
    {
        APP_ERROR1("Bad Connection:%d", connection);
        return -1;
    }
    /* If connection not used or not opened */
    if ((app_dg_cb.connections[connection].in_use == FALSE) ||
         ((app_dg_cb.connections[connection].in_use != FALSE)
           && (app_dg_cb.connections[connection].is_open == FALSE)))
    {
        APP_ERROR1("Connection:%d not opened", connection);
        return -1;
    }

    len = UIPC_Read(app_dg_cb.connections[connection].uipc_channel, &event, rx_buffer, APP_DG_RX_BUFFER_SIZE);
    if(len > 0)
    {
        scru_dump_hex(rx_buffer, "DG Read Data", len, TRACE_LAYER_NONE,
                TRACE_TYPE_DEBUG);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_send_file
 **
 ** Description      Example of function to send a file
 **
 ** Returns          status: -1 in case of error, 0 if successful
 **
 *******************************************************************************/
int app_dg_send_file(char * p_file_name, int connection)
{
    int fd, result;
    ssize_t length = 0;
    BT_HDR *p_msg;
    int status;
    tAPP_DG_CON *p_connection;
    UINT32 total_bytes = 0;
    long total_time, seconds, useconds;
    struct timeval start_time;
    struct timeval end_time;

    APP_DEBUG0("app_dg_send_file send enter");

    if(p_file_name == NULL)
    {
        APP_ERROR0("ERROR app_dg_send_file null file name !!");
        return -1;
    }

    if (connection < 0)
    {
        app_dg_con_display();
        connection = app_get_choice("Select connection");
    }
    if ((connection < 0) || (connection >= APP_DG_NB_CON_MAX))
    {
        APP_ERROR1("Bad Connection:%d", connection);
        return -1;
    }

    p_connection = &app_dg_cb.connections[connection];

    /* If connection not used or not opened */
    if ((p_connection->in_use == FALSE) ||
         ((p_connection->in_use != FALSE)
            && (p_connection->is_open == FALSE)))
    {
        APP_ERROR1("Connection:%d not opened", connection);
        return -1;
    }

    if (p_connection->p_tx_pending != NULL)
    {
        APP_ERROR0("A Tx buffer is already pending for this connection");
        APP_ERROR0("This means that the Tx flow is Off");
        return -1;
    }

    /* By default, return success */
    result = 0;

    /* Open the file */
    fd = open(p_file_name, O_RDONLY);
    if (fd >= 0)
    {
        gettimeofday(&start_time, NULL);
        do
        {
            /* Allocate buffer for RX data */
            p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) + APP_DG_TX_BUFFER_SIZE);
            if (p_msg != NULL)
            {
                p_msg->offset = 0;

                /* Read the data from the file and write it in the buffer */
                length = read(fd, (UINT8 *)(p_msg + 1), APP_DG_TX_BUFFER_SIZE);
                total_bytes += length;
                if (length > 0)
                {
                    p_msg->len = length;

                    /*  Try to send the buffer to the peer device */
                    app_dg_send_file_send:
                    status = app_dg_sendbuf(connection, p_msg);

                    if (status == 0)
                    {
                        /* Buffer sent to BSA server */
                    }
                    /* Not able to send the buffer */
                    else if (status < 0)
                    {
                        APP_ERROR0("No able to send buffer");
                        result = -1;
                        break;
                    }
                    else
                    {
                        APP_DEBUG1("UIPC Tx Flow On ChId:%d", p_connection->uipc_channel);
                        APP_DEBUG1("wait EVENT..:%d", p_connection->uipc_channel);
                        status = app_lock_mutex(&app_dg_cb.app_dg_tx_mutex[connection]);
                        if(status < 0)
                        {
                            APP_ERROR1("app_lock_mutex failed: %d", status);
                            result = -1;
                            break;
                        }
                        APP_DEBUG1("received EVENT..:%d", p_connection->uipc_channel);
                        /* Check pending buffer */
                        if(app_dg_cb.connections[connection].p_tx_pending == NULL)
                        {
                            APP_ERROR0("tx_pending status changed");
                            result = -1;
                            break;
                        }
                        APP_DEBUG0("Re-Send packet");
                        goto app_dg_send_file_send;
                    }
                }
                else
                {
                    /* Nothing read => free buffer */
                    GKI_freebuf(p_msg);
                }
            }
            else
            {
                /* Buffer allocation failed */
                APP_ERROR0("GKI_getbuf failed");
                result = -1;
                break;
            }
        } while(length > 0);

        close(fd);
        gettimeofday(&end_time, NULL);
        seconds = end_time.tv_sec - start_time.tv_sec;
        useconds = end_time.tv_usec - start_time.tv_usec;
        total_time = (seconds * 1000) + (useconds/1000);
        if (total_time > 0)
        {
            APP_INFO1("File TX Throughput %d bytes/%ld msecs = %ldbps",
                total_bytes, total_time, ((long)(total_bytes*8)/total_time)*1000);
        }
        else
        {
            APP_INFO1("File size is too small:%d bytes", total_bytes);
        }
    }
    else
    {
        APP_ERROR1("ERROR app_dg_send_file could not open %s", p_file_name);
        result = -1;
    }

    return result;
}

/*******************************************************************************
 **
 ** Function         app_dg_send_data
 **
 ** Description      Example of function to send data
 **
 ** Returns          status: -1 in case of error, 0 if successful
 **
 *******************************************************************************/
int app_dg_send_data(int connection)
{
    int result;
    ssize_t length = 0;
    BT_HDR *p_msg;
    int status;
    tAPP_DG_CON *p_connection;
    char buf[128];
    UINT8* p_data = NULL;
    char* p_buf = NULL;
    int len = 0;

    APP_DEBUG0("app_dg_send_data enter");

    if (connection < 0)
    {
        app_dg_con_display();
        connection = app_get_choice("Select connection");
    }
    if ((connection < 0) || (connection >= APP_DG_NB_CON_MAX))
    {
        APP_ERROR1("Bad Connection:%d", connection);
        return -1;
    }

    p_connection = &app_dg_cb.connections[connection];

    /* If connection not used or not opened */
    if ((p_connection->in_use == FALSE) ||
         ((p_connection->in_use != FALSE)
            && (p_connection->is_open == FALSE)))
    {
        APP_ERROR1("Connection:%d not opened", connection);
        return -1;
    }

    if (p_connection->p_tx_pending != NULL)
    {
        APP_ERROR0("A Tx buffer is already pending for this connection");
        APP_ERROR0("This means that the Tx flow is Off");
        return -1;
    }

    /* By default, return success */
    result = 0;

    do
    {
        /* Allocate buffer for RX data */
        p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) + APP_DG_TX_BUFFER_SIZE);
        if (p_msg != NULL)
        {
            p_msg->offset = 0;

            memset(buf, 0, sizeof(buf));

            length = app_get_string("Enter data to send in hex bytes A0B1C2D3 (length should be even)", buf, sizeof(buf));

            p_data = (UINT8 *)(p_msg + 1);

                APP_INFO1("Opening DG data length is :%d", length);
                APP_INFO1("Opening DG buf read is :%s", (char*)(buf));

            if (length > 0 && (length%2 == 0))
            {
                p_msg->len = length / 2;
                p_buf = buf;

                while (length >= 0)
                {
                    sscanf (p_buf, "%02x", &p_data[len]);
                    p_buf += 2*sizeof(char);
                    length = length - 2;
                    len = len + 1;
                }


                APP_DUMP("DG outgoing Data", p_data, p_msg->len);

                /*  Try to send the buffer to the peer device */
                app_dg_send_file_send:
                status = app_dg_sendbuf(connection, p_msg);

                if (status == 0)
                {
                    /* Buffer sent to BSA server */
                }
                /* Not able to send the buffer */
                else if (status < 0)
                {
                    APP_ERROR0("No able to send buffer");
                    result = -1;
                    break;
                }
                else
                {
                    APP_DEBUG1("UIPC Tx Flow On ChId:%d", p_connection->uipc_channel);
                    APP_DEBUG1("wait EVENT..:%d", p_connection->uipc_channel);
                    status = app_lock_mutex(&app_dg_cb.app_dg_tx_mutex[connection]);
                    if(status < 0)
                    {
                        APP_ERROR1("app_lock_mutex failed: %d", status);
                        result = -1;
                        break;
                    }
                    APP_DEBUG1("received EVENT..:%d", p_connection->uipc_channel);
                    /* Check pending buffer */
                    if(app_dg_cb.connections[connection].p_tx_pending == NULL)
                    {
                        APP_ERROR0("tx_pending status changed");
                        result = -1;
                        break;
                    }
                    APP_DEBUG0("Re-Send packet");
                    goto app_dg_send_file_send;
                }
            }
            else
            {
                /* Nothing read => free buffer */
                GKI_freebuf(p_msg);
            }
        }
        else
        {
            /* Buffer allocation failed */
            APP_ERROR0("GKI_getbuf failed");
            result = -1;
            break;
        }
    } while(length > 0);


    return result;
}

int app_dg_send(int connection, char * data, int nBytes)
{
    BT_HDR *p_msg;
    tAPP_DG_CON *p_connection;
    char* p_buf = NULL;
    int nBytesLeft=nBytes;
    int status = 0,result=-1;

    if (connection < 0)
    {
        app_dg_con_display();
        connection = app_get_choice("Select connection");
    }
    if ((connection < 0) || (connection >= APP_DG_NB_CON_MAX))
    {
        APP_ERROR1("Bad Connection:%d", connection);
        return -1;
    }

    p_connection = &app_dg_cb.connections[connection];

    /* If connection not used or not opened */
    if ((p_connection->in_use == FALSE) ||
         ((p_connection->in_use != FALSE)
            && (p_connection->is_open == FALSE)))
    {
        APP_ERROR1("Connection:%d not opened", connection);
        return -1;
    }

    if (p_connection->p_tx_pending != NULL)
    {
        APP_ERROR0("A Tx buffer is already pending for this connection");
        APP_ERROR0("This means that the Tx flow is Off");
        return -1;
    }

    do
    {
        result = -1;
        /* Allocate buffer for RX data */
        if (NULL == (p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) + APP_DG_TX_BUFFER_SIZE)))
            break;

        memset((char *)p_msg, 0, sizeof(BT_HDR) + APP_DG_TX_BUFFER_SIZE);

        p_msg->offset = 0;
        p_buf = ((char *)p_msg)+sizeof(BT_HDR);

        p_msg->len = (nBytesLeft > APP_DG_TX_BUFFER_SIZE) ? APP_DG_TX_BUFFER_SIZE : nBytesLeft;

        memcpy(p_buf, data, p_msg->len);

        APP_DUMP("DG outgoing Data", (UINT8 *)p_buf, p_msg->len);

        for (;;)
        {
            /*  Try to send the buffer to the peer device */
            if ( 0 == (status = app_dg_sendbuf(connection, p_msg)))
            {
                nBytesLeft -= p_msg->len;
                data += p_msg->len;
                result = 0;
                break;
            }

            /* Not able to send the buffer */
            if (status < 0)
            {
                APP_ERROR0("No able to send buffer");
                result = -1;
                break;
            }

            APP_DEBUG1("UIPC Tx Flow On ChId:%d", p_connection->uipc_channel);
            APP_DEBUG1("wait EVENT..:%d", p_connection->uipc_channel);

            if(app_lock_mutex(&app_dg_cb.app_dg_tx_mutex[connection]) < 0)
            {
                APP_ERROR1("app_lock_mutex failed: %d", status);
                result = -1;
                break;
            }
            APP_DEBUG1("received EVENT..:%d", p_connection->uipc_channel);
            /* Check pending buffer */
            if(app_dg_cb.connections[connection].p_tx_pending == NULL)
            {
                APP_ERROR0("tx_pending status changed");
                result = -1;
                break;
            }
            APP_DEBUG0("Re-Send packet");
        }
    } while(nBytesLeft > 0 && result != -1);

    return result;
}

/*******************************************************************************
 **
 ** Function         app_dg_listen
 **
 ** Description      Example of function to start an SPP server
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_listen(void)
{
    tBSA_STATUS status;
    tBSA_DG_LISTEN param;
    int connection;
    int choice, i;
    int uarr[16];
    char szKeyName[256];
    /* char szKeyName[] = "453994D5-D58B-96F9-6616-B37F586BA2EC"; PandoraLink*/
    /* char szKeyName[] = "FA87C0D0-AFAC-11DE-8A39-0800200C9A66"; BtChat Android App - Secure auth*/

    APP_DEBUG0("app_dg_listen");

    connection = app_dg_con_alloc();
    if (connection >= APP_DG_NB_CON_MAX)
    {
        APP_ERROR0("not able to allocate connection for server");
        return -1;
    }

    status = BSA_DgListenInit(&param);

    APP_INFO0("Which DG service do you listen to?:");
    APP_INFO1("   Serial Port Profile: %d", BSA_SPP_SERVICE_ID);
    APP_INFO1("   dial Up Networking Profile: %d", BSA_DUN_SERVICE_ID);

    choice = app_get_choice("Select service");

    if (choice == BSA_SPP_SERVICE_ID)
    {
        param.service = BSA_SPP_SERVICE_ID;

        APP_INFO0("Create a custom 128-bit UUID serial service ?:");
        APP_INFO0("    0 No");
        APP_INFO0("    1 Yes");
        choice = app_get_choice("Select choice");

        if(choice)
        {
            /* Creating custom serial serivce with 128 bit UUID */
            memset(szKeyName, 0, sizeof(szKeyName));
            APP_INFO0("Enter 128-bit UUID in this format: FA87C0D0-AFAC-11DE-8A39-0800200C9A66");
            APP_INFO0("Sample BtChat Android App - Secure Auth 128-bit UUID is: FA87C0D0-AFAC-11DE-8A39-0800200C9A66");

            app_get_string("Enter 128-bit UUID: ", szKeyName, sizeof(szKeyName));
            /* strcpy(param.service_name, "PandoraLink"); */
            APP_INFO0("Sample BtChat Android App - Secure Auth service name is: BluetoothChatSecure");
            memset(param.service_name, 0, sizeof(param.service_name));
            app_get_string("Enter service name", param.service_name, sizeof(param.service_name));

            param.uuid.len = LEN_UUID_128;

            APP_INFO1("Creating DG connection: %s", szKeyName);

            sscanf (szKeyName, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                           &uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5], &uarr[6], &uarr[7], &uarr[8],
                    &uarr[9], &uarr[10], &uarr[11], &uarr[12], &uarr[13], &uarr[14], &uarr[15]);


            for(i=0; i < 16; i++)
                param.uuid.uu.uuid128[i] = (UINT8) uarr[i];

            APP_INFO1("Creating DG connection uuid scanned: %s", (char*) param.uuid.uu.uuid128);
        }
        else
        {
            APP_INFO0("Listen for specific service name?:");
            APP_INFO0("    0 No");
            APP_INFO0("    1 Yes");
            choice = app_get_choice("Select choice");
            memset(param.service_name, 0, sizeof(param.service_name));
            if (choice)
            {
                app_get_string("service name: ",param.service_name, sizeof(param.service_name)-1);
            }

            /*strcpy(param.service_name, "Bluetooth Serial Port");*/
            /*strcpy(param.service_name, "ActiveSync"); */
        }
    }
    else if (choice == BSA_DUN_SERVICE_ID)
    {
        param.service = BSA_DUN_SERVICE_ID;
        /* Create a Service name based on connection number */
        snprintf(param.service_name, sizeof(param.service_name), "DUN%d", connection);
    }
    else
    {
        APP_ERROR1("Bad Service selected :%d", choice);
        app_dg_con_free(connection);
        return -1;
    }

#if (defined(BSA_TEST) && (BSA_TEST==TRUE))
    /*
     * DG Sec mask
     */
    APP_INFO0("Security?");
    APP_INFO1("   NONE: %d", 0);
    APP_INFO1("   Authentication: %d", 1);
    APP_INFO1("   Encryption+Authentication: %d", 3);
    APP_INFO1("   Authorization: %d", 4);

    choice = app_get_choice("Select sec_mask");
    if ((choice & ~7) == 0)
    {
        param.sec_mask = BTA_SEC_NONE;
        if (choice & 1)
        {
            param.sec_mask |= BSA_SEC_AUTHENTICATION;
        }
        if (choice & 2)
        {
            param.sec_mask |= BSA_SEC_ENCRYPTION;
        }
        if (choice & 4)
        {
            param.sec_mask |= BSA_SEC_AUTHORIZATION;
        }
    }
    else
    {
        APP_ERROR1("Bad Security Mask selected :%d", choice);
        app_dg_con_free(connection);
        return -1;
    }
#endif

    /* Ask BSA Server to listen on this DG Server */
    status = BSA_DgListen(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DgListen fail:%d", status);
        app_dg_con_free(connection);
        return -1;
    }

    APP_DEBUG1("DG Listen Connection:%d Handle:%d", connection, param.handle);
    /* BSA_DgListen returns an Handle */
    app_dg_cb.connections[connection].handle = param.handle;
    app_dg_cb.connections[connection].is_server = TRUE;
    app_dg_cb.connections[connection].service = param.service;

    app_dg_con_display();
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_shutdown
 **
 ** Description      Example of function to stop an SPP server
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_shutdown(int connection)
{
    tBSA_STATUS status;
    tBSA_DG_SHUTDOWN param;

    APP_DEBUG0("app_dg_shutdown");

    if (connection < 0)
    {
        app_dg_con_display();
        connection = app_get_choice("Select server connection to shutdown");
    }
    if ((connection < 0) ||
        (connection >= APP_DG_NB_CON_MAX) ||
        (app_dg_cb.connections[connection].in_use == FALSE))
    {
        APP_ERROR1("Bad Connection:%d", connection);
        return -1;
    }
    /* If connection is not a server */
    if ((app_dg_cb.connections[connection].in_use != FALSE) &&
        (app_dg_cb.connections[connection].is_server == FALSE))
    {
        APP_ERROR1("Connection:%d not a server", connection);
        return -1;
    }

    status = BSA_DgShutdownInit(&param);
    param.handle = app_dg_cb.connections[connection].handle;

    status = BSA_DgShutdown(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DgShutdownInit fail:%d", status);
        return -1;
    }

    app_dg_cb.connections[connection].in_use = FALSE;

    app_dg_con_display();
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_any_connections
 **
 ** Description      Determins if any connections exist
 **
 ** Returns          TRUE if connection is found, FALSE otherwise
 **
 *******************************************************************************/
BOOLEAN app_dg_any_connections()
{
    int index;

    for(index = 0 ; index < APP_DG_NB_CON_MAX ; index++)
    {
        if (app_dg_cb.connections[index].in_use == TRUE)
            return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
 **
 ** Function         app_dg_close
 **
 ** Description      Example of function to close a DG connection
 **
 ** Returns          int
 **
 *******************************************************************************/
int  app_dg_close(int connection)
{
    tBSA_STATUS status;
    tBSA_DG_CLOSE close_param;

    APP_DEBUG0("app_dg_close");

    if (connection < 0)
    {
        app_dg_con_display();
        connection = app_get_choice("Select connection to close");
    }
    if ((connection < 0) ||
        (connection >= APP_DG_NB_CON_MAX) ||
        (app_dg_cb.connections[connection].in_use == FALSE))
    {
        APP_ERROR1("Bad Connection:%d", connection);
        return -1;
    }
    /* If connection is not a server */
    if ((app_dg_cb.connections[connection].in_use != FALSE) &&
        (app_dg_cb.connections[connection].is_open == FALSE))
    {
        APP_ERROR1("Connection:%d not opened", connection);
        return -1;
    }

    status = BSA_DgCloseInit(&close_param);
    close_param.handle = app_dg_cb.connections[connection].handle;
    status = BSA_DgClose(&close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DgClose fail:%d", status);
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_dg_open
 **
 ** Description      Example of function to open a DG connection
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_open()
{
    return app_dg_open_ex(1);
}

/*******************************************************************************
 **
 ** Function         app_dg_open_ex
 **
 ** Description      Example of function to open a DG connection for multiple
 **                  or single connections.
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_open_ex(int single_conn_only)
{
    tBSA_STATUS status = 0;
    int device_index;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_DG_OPEN param;
    int connection;
    int choice;
    int i;
    int uarr[16];
    char szKeyName[256];

    /* char szKeyName[] = "453994D5-D58B-96F9-6616-B37F586BA2EC"; PandoraLink */
    /* char szKeyName[] = "FA87C0D0-AFAC-11DE-8A39-0800200C9A66"; BtChat Android App - Secure auth*/

    APP_DEBUG0("app_dg_open");

    if (single_conn_only)
    {
        if (app_dg_any_connections())
        {
            APP_INFO0("Only one connection allowed");
            return -1;
        }
    }

    connection = app_dg_con_alloc();
    app_dg_con_display_debug();
    if (connection >= APP_DG_NB_CON_MAX)
    {
        APP_ERROR0("Unable to alloc connection");
        return -1;
    }

    APP_INFO0("Bluetooth DG menu:");
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
            app_dg_con_free(connection);
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
            app_dg_con_free(connection);
            return -1;
        }
    }
    else
    {
        APP_ERROR1("Bad choice:%d", device_index);
        app_dg_con_free(connection);
        return -1;
    }

    BSA_DgOpenInit(&param);

    /*
     * DG Service selection
     */
    APP_INFO0("Which DG service do you want to connect to?:");
    APP_INFO1("   Serial Port Profile: %d", BSA_SPP_SERVICE_ID);
    APP_INFO1("   dial Up Networking Profile: %d", BSA_DUN_SERVICE_ID);


    choice = app_get_choice("Select service");

    if (choice == BSA_SPP_SERVICE_ID)
    {
        param.service = BSA_SPP_SERVICE_ID;

        APP_INFO0("Connect to 128-bit UUID serial service ?:");
        APP_INFO0("    0 No");
        APP_INFO0("    1 Yes");
        choice = app_get_choice("Select choice");

        if(choice)
        {
            memset(szKeyName, 0, sizeof(szKeyName));
            APP_INFO0("Enter 128-bit UUID in this format: FA87C0D0-AFAC-11DE-8A39-0800200C9A66");
            APP_INFO0("Sample BtChat Android App - Secure Auth 128-bit UUID is: FA87C0D0-AFAC-11DE-8A39-0800200C9A66");

            app_get_string("Enter remote 128-bit UUID: ", szKeyName, sizeof(szKeyName));
            /* strcpy(param.service_name, "PandoraLink"); */
            APP_INFO0("Sample BtChat Android App - Secure Auth service name is: BluetoothChatSecure");
            memset(param.service_name, 0, sizeof(param.service_name));
            app_get_string("Enter remote service name", param.service_name, sizeof(param.service_name));

            param.uuid.len = LEN_UUID_128;

            APP_INFO1("Opening DG connection before scanf %s", szKeyName);

            sscanf (szKeyName, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                           &uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5], &uarr[6], &uarr[7], &uarr[8],
                    &uarr[9], &uarr[10], &uarr[11], &uarr[12], &uarr[13], &uarr[14], &uarr[15]);

            for(i=0; i < 16; i++)
                param.uuid.uu.uuid128[i] = (UINT8) uarr[i];

            APP_INFO1("Opening DG connection uuid is :%s", (char*) param.uuid.uu.uuid128);
        }
        else
        {
            APP_INFO0("Connect to specific service name?:");
            APP_INFO0("    0 No");
            APP_INFO0("    1 Yes");
            choice = app_get_choice("Select choice");
            memset(param.service_name, 0, sizeof(param.service_name));
            if (choice)
            {
                app_get_string("service name: ",param.service_name, sizeof(param.service_name)-1);
            }

            /*strcpy(param.service_name, "Bluetooth Serial Port");*/
            /*strcpy(param.service_name, "ActiveSync"); */
        }

    }
    else if (choice == BSA_DUN_SERVICE_ID)
    {
        param.service = BSA_DUN_SERVICE_ID;
    }
    else
    {
        APP_ERROR1("Bad Service selected :%d", choice);
        app_dg_con_free(connection);
        return -1;
    }

#if (defined(BSA_TEST) && (BSA_TEST==TRUE))
    /*
     * DG Sec mask
     */
    APP_INFO0("Security?");
    APP_INFO1("   NONE: %d", 0);
    APP_INFO1("   Authentication: %d", 1);
    APP_INFO1("   Encryption+Authentication: %d", 3);
    APP_INFO1("   Authorization: %d", 4);

    choice = app_get_choice("Select sec_mask");
    if ((choice & ~7) == 0)
    {
        param.sec_mask = BTA_SEC_NONE;
        if (choice & 1)
        {
            param.sec_mask |= BSA_SEC_AUTHENTICATION;
        }
        if (choice & 2)
        {
            param.sec_mask |= BSA_SEC_ENCRYPTION;
        }
        if (choice & 4)
        {
            param.sec_mask |= BSA_SEC_AUTHORIZATION;
        }
    }
    else
    {
        APP_ERROR1("Bad Security Mask selected :%d", choice);
        app_dg_con_free(connection);
        return -1;
    }
#endif

    /* Save the BdAddr in the Application connection structure */
    bdcpy(app_dg_cb.connections[connection].bd_addr, bd_addr);
    bdcpy(param.bd_addr, bd_addr);

    APP_INFO1("Opening DG connection to:%s", p_name);
    status = BSA_DgOpen(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DgOpen fail:%d", status);
        app_dg_con_free(connection);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_find_service
 **
 ** Description      Example of function to find custom bluetooth service
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_find_service(void)
{
    tBSA_STATUS status = 0;
    int device_index;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_DG_FIND_SERVICE param;
    int i;
    int uarr[16];
    char szKeyName[256];

    /* Pandora Serial Service UUID */
    /*char szKeyName[] = "453994D5-D58B-96F9-6616-B37F586BA2EC"; PandoraLink*/
    /*char szKeyName[] = "FA87C0D0-AFAC-11DE-8A39-0800200C9A66"; BtChat Android App - Secure auth*/

    APP_DEBUG0("app_dg_find_service");

    APP_INFO0("Bluetooth DG menu:");
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

    BSA_DgFindServiceInit(&param);

    bdcpy(param.bd_addr, bd_addr);

    param.uuid.len = LEN_UUID_128;

    memset(szKeyName, 0, sizeof(szKeyName));
    APP_INFO0("Enter 128-bit UUID in this format: FA87C0D0-AFAC-11DE-8A39-0800200C9A66");
    APP_INFO0("Sample BtChat Android App - Secure Auth 128-bit UUID is: FA87C0D0-AFAC-11DE-8A39-0800200C9A66");

    app_get_string("Enter remote 128-bit UUID: ", szKeyName, sizeof(szKeyName));

    sscanf (szKeyName, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                   &uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5], &uarr[6], &uarr[7], &uarr[8],
            &uarr[9], &uarr[10], &uarr[11], &uarr[12], &uarr[13], &uarr[14], &uarr[15]);

    for(i=0; i < 16; i++)
        param.uuid.uu.uuid128[i] = (UINT8) uarr[i];

    APP_INFO1("Opening DG connection uuid is :%s", (char*) param.uuid.uu.uuid128);

    APP_DUMP("BSA_DgFindService UUID:", (UINT8 *) param.uuid.uu.uuid128, 16);

    APP_INFO1("Opening DG connection to:%s", p_name);
    status = BSA_DgFindService(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DgFindService fail:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_init
 **
 ** Description      Init DG application
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_init(void)
{
    int status;
    int index;

    /* Reset structure */
    memset(&app_dg_cb, 0, sizeof(app_dg_cb));

    for (index = 0; index < APP_DG_NB_CON_MAX ; index++)
    {
        status = app_init_mutex(&app_dg_cb.app_dg_tx_mutex[index]);
        if (status < 0)
        {
            return -1;
        }

        status = app_lock_mutex(&app_dg_cb.app_dg_tx_mutex[index]);
        if (status < 0)
        {
            return -1;
        }
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_start
 **
 ** Description      Start DG application
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_start(void)
{
    tBSA_STATUS status = BSA_SUCCESS;

    tBSA_DG_ENABLE enable_param;

    APP_DEBUG0("app_dg_start");

    status = BSA_DgEnableInit(&enable_param);
    enable_param.p_cback = app_dg_cback;
    status = BSA_DgEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable DG status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_stop
 **
 ** Description      Stop DG application
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_stop(void)
{
    tBSA_STATUS status;
    tBSA_DG_DISABLE disable_param;

    APP_DEBUG0("app_dg_stop");

    /* free all pending tx buffer if any */
    app_dg_con_free_all();

    status = BSA_DgDisableInit(&disable_param);
    status = BSA_DgDisable(&disable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DgDisable failed status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dg_sendto_vtty
 **
 ** Description      Send data to virtual serial port.  Called when data is
 **                  received over BT SPP connection.
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_dg_sendto_vtty(char * rx_buffer,int length,int connection)
{
#ifdef BSA_USE_SERIAL_PORT
    if (app_dg_cb.connections[connection].serial_fd == -1)
        return;

    write(app_dg_cb.connections[connection].serial_fd, rx_buffer, length);
#endif
}

/*******************************************************************************
 **
 ** Function         get_socat_pid
 **
 ** Description      Finds and stores process id of socat program associated
 **                  with the connection.
 **
 ** Returns          1 on success, 0 on failure
 **
 *******************************************************************************/
int get_socat_pid(int conn, char * cmd)
{
#ifdef BSA_USE_SERIAL_PORT
    char buf[200];
    int i,pid,n;
    char * p=buf;
    FILE * fp_cmd=NULL;
    FILE * fp_ps=NULL;
    char ps_cmd[100];
    char socat_cmd[200];

    /* wait up to 1 second for socat to create virtual serial port */
    sprintf(buf, "/dev/ttyBT%d", conn);
    for (i = 0; i < 4; i++)
    {
        struct stat st;
        if (stat(buf,&st) != 0)
        {
            usleep(250*1000);
            continue;
        }
        break;
    }

    if (i == 4)
    {
        APP_ERROR1("app_dg: socat failed to launch for conn %d", conn);
        return 0;
    }

    /* find pid of socat process for this connection */
    if (fp_cmd = popen("pidof socat", "r"))
    {
        /* cmd returns pids for all socats running, need to find
         * the one for this connection */

        memset(buf,0,sizeof(buf));
        fgets(buf, sizeof(buf)-1, fp_cmd);
        p=buf;
        for (i = 0; i < sizeof(buf); i++)
        {
            /* for each socat PID found look for a matching command line */
            if (buf[i] == ' ' || buf[i] == 0)
            {
                /* parse for the next socat pid */
                buf[i] = 0;
                pid = strtoul(p, NULL, 10);
                p = &(buf[i+1]);
                i++;

                /* get cmd line for the socat process */
                sprintf(ps_cmd, "ps -p %d --no-heading -o cmd", pid);
                if (fp_ps= popen(ps_cmd, "r"))
                {
                    memset(socat_cmd, 0, sizeof(socat_cmd));
                    fgets(socat_cmd, sizeof(socat_cmd)-1, fp_ps);
                    fclose(fp_ps);
                    cmd[strlen(cmd)-2] = 0; /* remove last two chars or it won't match */
                    n = strlen(cmd) < strlen(socat_cmd) ? strlen(cmd) : strlen(socat_cmd);
                    if (strncmp(cmd,socat_cmd,n) == 0)
                    {
                        APP_INFO1("get_socat_pid(): found socat pid: %d", pid);
                        app_dg_cb.connections[conn].socat_pid = pid;
                        break;
                    }
                }
            }
        }

        pclose(fp_cmd);
    }
    else
        APP_ERROR1("get_socat_pid(): error executing pidof socat, errno: %d", errno);

    return (app_dg_cb.connections[conn].socat_pid != -1);
#endif
    return 1;
}

/*******************************************************************************
 **
 ** Function         app_dg_open_tty
 **
 ** Description      Create a virtual serial port for the connection using
 **                  socat utility.
 **
 ** Returns          1 on success, 0 on failure
 **
 *******************************************************************************/
int app_dg_open_tty(int connection)
{
#ifdef BSA_USE_SERIAL_PORT
    int err  = 0;
    char cmd[500];

    /* use socat to create a virtual serial port */
    sprintf(cmd, "socat pty,link=/dev/ttyBT%d,raw,echo=0 pty,link=/dev/ttyConBT%d,raw,echo=0 &", connection, connection);
    if (-1 == system(cmd))
    {
        APP_ERROR1("app_dg_open_tty: error trying to start socat: %d, conn: %d", errno, connection);
        return 0;
    }

    app_dg_cb.connections[connection].socat_pid = 0;
    app_dg_cb.connections[connection].serial_fd = -1;

    get_socat_pid(connection,cmd);

    /* open new serial port */
    sprintf(cmd, "/dev/ttyConBT%d", connection);
    if ((app_dg_cb.connections[connection].serial_fd = open(cmd, O_RDWR|O_NOCTTY)) != -1)
    {
        app_dg_cb.connections[connection].stop_tty_read = 0;
        if (err= pthread_create( &(app_dg_cb.connections[connection].read_thread), NULL, app_dg_read_proc, (void*) connection))
        {
            APP_ERROR1("Error - pthread_create() return code: %d\n",err);
            close(app_dg_cb.connections[connection].serial_fd);
            app_dg_cb.connections[connection].serial_fd = -1;
            return 0;
        }
        APP_INFO1("app_dg: virtual serial port /dev/ttyBT%d created.\n", connection);
    }
    else
    {
        APP_ERROR1("app_dg: error opening virtual serial port for connection %d", connection);
        return 0;
    }
#endif
    return 1;
}

/*******************************************************************************
 **
 ** Function         app_dg_close_tty
 **
 ** Description      Close virtual serial port for the connection.
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_dg_close_tty(int connection)
{
#ifdef BSA_USE_SERIAL_PORT
    app_dg_cb.connections[connection].stop_tty_read = 1;
    if (app_dg_cb.connections[connection].serial_fd != -1)
    {
        close(app_dg_cb.connections[connection].serial_fd);
        app_dg_cb.connections[connection].serial_fd = -1;
    }

    if (app_dg_cb.connections[connection].socat_pid && app_dg_cb.connections[connection].socat_pid != -1)
    {
        kill(app_dg_cb.connections[connection].socat_pid, SIGTERM);
        sleep(1);
        kill(app_dg_cb.connections[connection].socat_pid, SIGKILL);
    }
    app_dg_cb.connections[connection].socat_pid = -1;
    if (app_dg_cb.connections[connection].read_thread != 0)
        pthread_join( app_dg_cb.connections[connection].read_thread, NULL);
#endif
}

#ifdef BSA_USE_SERIAL_PORT
/*******************************************************************************
 **
 ** Function         app_dg_read_proc
 **
 ** Description      Read data from virtual serial port and send to remote device.
 **                  Data on virtual serial port is coming from local app.
 **
 ** Returns          none
 **
 *******************************************************************************/
void * app_dg_read_proc(void *data)
{
    unsigned char buffer[APP_DG_RX_BUFFER_SIZE];
    int conn = (int)data;
    int i = 0;

    if (conn >= APP_DG_NB_CON_MAX)
    {
        APP_ERROR1("app_dg_read_proc() invalid connection: %d",conn);
        return 0;
    }

    /* open port if not alreay opened */
    while (app_dg_cb.connections[conn].serial_fd == -1 && !app_dg_cb.connections[conn].stop_tty_read)
    {
        if (app_dg_open_tty(conn) != -1)
            break;

        if (i++ >= 4)
        {
            APP_ERROR1("app_dg_read_proc() unable to open serial port for connection: %d",conn);
            return 0;
        }

        usleep(200 * 1000); // 200 ms
    }

    while (!app_dg_cb.connections[conn].stop_tty_read)
    {
        memset(buffer, 0, sizeof(buffer));
        int n = read(app_dg_cb.connections[conn].serial_fd, buffer, sizeof(buffer));

        if (n < 0)
        {
            APP_ERROR1("app_dg_read_proc() read failed: %d", errno);
        }
        else
        {
            buffer[n] = 0;
            APP_DEBUG1("app_dg_read_proc() recv %d bytes", n);
            // send to connected device over SPP
            app_dg_send(conn,buffer,n);
        }
    }
    APP_DEBUG1("app_dg_read_proc() conn=%d, exiting", conn);
}
#endif

/*******************************************************************************
 **
 ** Function         app_dg_loopback_toggle
 **
 ** Description      This function is used to stop DG
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_loopback_toggle(void)
{
    if (app_dg_cb.loopback == TRUE)
    {
        APP_INFO0("Data Loopback in now FALSE");
        app_dg_cb.loopback = FALSE;
    }
    else
    {
        APP_INFO0("Data Loopback in now TRUE");
        app_dg_cb.loopback = TRUE;
    }
}

/*******************************************************************************
 **
 ** Function         app_dg_con_alloc
 **
 ** Description      Function to allocate a DG connection structure
 **
 ** Returns          connection index
 **
 *******************************************************************************/
int app_dg_con_alloc(void)
{
    int i;

    for(i = 0 ; i < APP_DG_NB_CON_MAX ; i++)
    {
        if (app_dg_cb.connections[i].in_use == FALSE)
        {
            /* Reset structure */
            APP_DEBUG1("Allocate DG connection:%d", i);
            memset(&app_dg_cb.connections[i], 0, sizeof(app_dg_cb.connections[i]));
            app_dg_cb.connections[i].in_use = TRUE;
            app_dg_cb.connections[i].handle = APP_DG_BAD_HANDLE;
            app_dg_cb.connections[i].uipc_channel = APP_DG_BAD_UIPC;

            return i;
        }
    }
    return APP_DG_NB_CON_MAX;
}

/*******************************************************************************
 **
 ** Function         app_dg_con_free
 **
 ** Description      Function to free a DG connection structure
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_con_free(int connection)
{
    if ((connection >= 0 ) &&
        (connection < APP_DG_NB_CON_MAX))
    {
        if (app_dg_cb.connections[connection].in_use == FALSE)
        {
            APP_ERROR1("Connection:%d was not in use", connection);
        }
        /* If a Tx pending message was present, free the GKI buffer */
        if (app_dg_cb.connections[connection].p_tx_pending)
        {
            GKI_freebuf(app_dg_cb.connections[connection].p_tx_pending);
        }
        /* Reset structure */
        memset(&app_dg_cb.connections[connection], 0, sizeof(app_dg_cb.connections[connection]));
    }
    else
    {
        APP_ERROR1("Bad connection:%d", connection);
    }
}

/*******************************************************************************
 **
 ** Function         app_dg_con_free_all
 **
 ** Description      Function to free all DG connection structure
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_con_free_all(void)
{
    UINT8 index;
    for (index = 0; index< APP_DG_NB_CON_MAX; index++)
    {
        app_dg_close_tty(index);
        /* If a Tx pending message was present, free the GKI buffer */
        if (app_dg_cb.connections[index].p_tx_pending)
        {
            GKI_freebuf(app_dg_cb.connections[index].p_tx_pending);
        }
        /* Reset structure */
        memset(&app_dg_cb.connections[index], 0, sizeof(app_dg_cb.connections[index]));
    }
}

/*******************************************************************************
 **
 ** Function         app_dg_con_display
 **
 ** Description      Function to display DG connection status
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_con_display(void)
{
    int index;

    APP_DEBUG0("AG Connections:");

    for(index = 0 ; index < APP_DG_NB_CON_MAX ; index++)
    {
        if (app_dg_cb.connections[index].in_use != FALSE)
        {
            APP_INFO1("\tConnection:%d Handle:%d IsServer:%d IsOpen:%d UIPC:%d Service:%d",
                    index,
                    app_dg_cb.connections[index].handle,
                    app_dg_cb.connections[index].is_server,
                    app_dg_cb.connections[index].is_open,
                    app_dg_cb.connections[index].uipc_channel,
                    app_dg_cb.connections[index].service);
        }
    }
}

/*******************************************************************************
 **
 ** Function         app_dg_con_display_debug
 **
 ** Description      Function to display DG connection status
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_con_display_debug(void)
{
    int index;

    APP_INFO0("AG Connections(Debug):");

    for(index = 0 ; index < APP_DG_NB_CON_MAX ; index++)
    {
        APP_INFO1("Connection:%d Handle:%d IsServer:%d InUse:%d IsOpen:%d UIPC:%d Service:%d",
            index,
            app_dg_cb.connections[index].handle,
            app_dg_cb.connections[index].is_server,
            app_dg_cb.connections[index].in_use,
            app_dg_cb.connections[index].is_open,
            app_dg_cb.connections[index].uipc_channel,
            app_dg_cb.connections[index].service);
    }
}

/*******************************************************************************
 **
 ** Function         app_dg_con_get_from_handle
 **
 ** Description      Function to get DG connection from handle
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_con_get_from_handle(tBSA_DG_HNDL handle, BOOLEAN *p_is_server)
{
    int index;

    for(index = 0 ; index < APP_DG_NB_CON_MAX ; index++)
    {
        if ((app_dg_cb.connections[index].in_use != FALSE) &&
            (app_dg_cb.connections[index].handle == handle))
        {
            APP_DEBUG1("Found DG connection:%d matching Handle:%d IsServer:%d", index, handle,
                    app_dg_cb.connections[index].is_server);
            if (p_is_server != NULL)
            {
                *p_is_server = app_dg_cb.connections[index].is_server;
            }
            return index;
        }
    }
    return APP_DG_NB_CON_MAX;
}

/*******************************************************************************
 **
 ** Function         app_dg_con_get_from_uipc
 **
 ** Description      Function to get DG connection from UIPC Channel Id
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_con_get_from_uipc(tUIPC_CH_ID channel_id)
{
    int index;

    for(index = 0 ; index < APP_DG_NB_CON_MAX ; index++)
    {
        if ((app_dg_cb.connections[index].in_use != FALSE) &&
            (app_dg_cb.connections[index].uipc_channel == channel_id))
        {
            APP_DEBUG1("Found DG connection:%d matching Channel:%d", index, channel_id);
            return index;
        }
    }
    return APP_DG_NB_CON_MAX;
}

/*******************************************************************************
 **
 ** Function         app_dg_get_srv_desc
 **
 ** Description      Get DG Service description.
 **
 ** Returns          DG service Description
 **
 *******************************************************************************/
char *app_dg_get_srv_desc(tBSA_SERVICE_ID service)
{
    if (service == BSA_SPP_SERVICE_ID)
    {
        return "SPP";
    }
    else if (service == BSA_DUN_SERVICE_ID)
    {
        return "DUN";
    }
    else
    {
        return "Bad DG Service";
    }
}

