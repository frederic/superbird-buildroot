/*****************************************************************************
**
**  Name:           app_sac.c
**
**  Description:    Bluetooth SIM Access Profile client sample application
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
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
#include "app_mgt.h"
#include "app_utils.h"

#define DEFAULT_SEC_MASK (BSA_SEC_AUTHENTICATION | BSA_SEC_ENCRYPTION)
#define DEFAULT_MESSAGE_SIZE 512

#define APDU_DATA_FILE_NAME "apdu_resp"
#define ATR_DATA_FILE_NAME "atr_data"

/*
* Types
*/
typedef struct
{
    UINT16 max_msg_size;
    tBSA_SAC_HANDLE handle;
} tAPP_SAC_CB;

/*
* Global Variables
*/

tAPP_SAC_CB app_sac_cb;

/*
* Local Variables
*/
static UINT8 apdu_sample1[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x00, 0x01 };     /* Select file (ID = 1) */
static UINT8 apdu_sample2[] = { 0xA0, 0xF2, 0x00, 0x00, 0x00 };     /* Status */

/* ui keypress definition */
enum
{
    APP_SAC_MENU_NULL,
    APP_SAC_MENU_CONNECT,
    APP_SAC_MENU_DISCONNECT,
    APP_SAC_MENU_GET_ATR,
    APP_SAC_MENU_TRANSFER_APDU,
    APP_SAC_MENU_POWER_SIM_ON,
    APP_SAC_MENU_POWER_SIM_OFF,
    APP_SAC_MENU_RESET_SIM,
    APP_SAC_MENU_GET_CARD_READER_STATUS,
    APP_SAC_MENU_SET_TRANSPORT_PROTOCOL,
    APP_SAC_MENU_SET_MAX_MESSAGE_SIZE,
    APP_SAC_MENU_QUIT
};

tBSA_STATUS app_sac_disconnect(void);

/*******************************************************************************
**
** Function         app_sac_display_main_menu
**
** Description      This is the main menu
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_sac_display_main_menu(void)
{
    printf("\nBluetooth SAP Client Test Application\n");
    printf("    %d => Connect \n", APP_SAC_MENU_CONNECT);
    printf("    %d => Disconnect\n", APP_SAC_MENU_DISCONNECT);
    printf("    %d => Get ATR\n", APP_SAC_MENU_GET_ATR);
    printf("    %d => Transfer APDU\n", APP_SAC_MENU_TRANSFER_APDU);
    printf("    %d => Power SIM On\n", APP_SAC_MENU_POWER_SIM_ON);
    printf("    %d => Power SIM Off\n", APP_SAC_MENU_POWER_SIM_OFF);
    printf("    %d => Reset SIM\n", APP_SAC_MENU_RESET_SIM);
    printf("    %d => Get Card Reader Status\n", APP_SAC_MENU_GET_CARD_READER_STATUS);
    printf("    %d => Set Transport Protocol\n", APP_SAC_MENU_SET_TRANSPORT_PROTOCOL);
    printf("    %d => Set Max Message Size\n", APP_SAC_MENU_SET_MAX_MESSAGE_SIZE);
    printf("    %d => Quit\n", APP_SAC_MENU_QUIT);
}

/*******************************************************************************
**
** Function         app_read_file
**
** Description      Read data from input file
**
** Parameters       none
**
** Returns          Buffer contain file data
**
**
*******************************************************************************/
static UINT8 * app_read_file(char *p_name, int *p_data_len)
{
    int fd;
    struct stat file_stat;
    UINT8* p_buffer = NULL;

    fd = open(p_name, O_RDONLY);
    if(fd >= 0)
    {
        if (fstat(fd, &file_stat) == 0)
        {
            p_buffer = malloc(file_stat.st_size);

            if (p_buffer != NULL)
            {
                *p_data_len = read(fd, p_buffer, file_stat.st_size);
            }
        }

        close(fd);
    }
    else
    {
        printf("Error: could not open file '%s'\n", p_name);
    }

    return p_buffer;
}

/*******************************************************************************
**
** Function         app_save_file
**
** Description      Save data to file
**
** Parameters       none
**
** Returns          void
**
**
*******************************************************************************/
static void app_save_file(char *p_name, UINT8 *p_buffer, int data_len)
{
    int fd;

    fd = open(p_name, O_WRONLY | O_CREAT |O_TRUNC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if(fd >= 0)
    {
        if (write(fd, p_buffer, data_len) < data_len)
        {
            printf("Error: could not write all data to file %s", p_name);
        }

        close(fd);
    }
    else
    {
        printf("Error: could not create file '%s' for writing\n", p_name);
    }
}

/*******************************************************************************
**
** Function         app_sac_mgt_callback
**
** Description      This callback function is called in case of server
**                  disconnection (e.g. server crashes)
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static BOOLEAN app_sac_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_DISCONNECT_EVT:
        APP_DEBUG1("BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
        /* Connection with the Server lost => Just exit the application */
        exit(-1);
        break;

    default:
        break;
    }
    return FALSE;
}

/*******************************************************************************
**
** Function         app_sac_cback
**
** Description      Example of SAC callback function to handle SAC related events
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_sac_cback(tBSA_SAC_EVT event, tBSA_SAC_MSG *p_data)
{
    switch (event)
    {
    case BSA_SAC_ENABLE_EVT:
        printf("BSA_SAC_ENABLE_EVT Status %d\n", p_data->enable.status);
        break;
    case BSA_SAC_DISABLE_EVT:
        printf("BSA_SAC_DISABLE_EVT\n");
        break;
    case BSA_SAC_CONNECT_EVT:
        printf("BSA_SAC_CONNECT_EVT Status %d ConnStatus %d\n",
            p_data->connect.status, p_data->connect.conn_status);
        if (p_data->connect.status == BSA_SUCCESS &&
            p_data->connect.conn_status == BSA_SAC_CONN_OK)
        {
            printf("Successfully connected with peer device, max message size is %d\n",
                p_data->connect.max_msg_size);
            app_sac_cb.handle = p_data->connect.handle;
        }
        break;
    case BSA_SAC_DISCONNECT_EVT:
        printf("BSA_SAC_DISCONNECT_EVT\n");
        app_sac_cb.handle = BSA_SAC_INVALID_HANDLE;
        break;
    case BSA_SAC_TRANS_APDU_EVT:
        printf("BSA_SAC_TRANS_APDU_EVT Status %d Result %d\n",
            p_data->trans_apdu.status, p_data->trans_apdu.result_code);
        if (p_data->trans_apdu.status == BSA_SUCCESS &&
            p_data->trans_apdu.result_code == BSA_SAC_RESULT_OK)
        {
            printf("Successfully received %d bytes APDU and saved to file '%s'\n",
                p_data->trans_apdu.apdu_len, APDU_DATA_FILE_NAME);
            app_save_file(APDU_DATA_FILE_NAME, p_data->trans_apdu.p_apdu,
                p_data->trans_apdu.apdu_len);
        }
        break;
    case BSA_SAC_GET_ATR_EVT:
        printf("BSA_SAC_GET_ATR_EVT Status %d Result %d\n",
            p_data->get_atr.status, p_data->get_atr.result_code);
        if (p_data->get_atr.status == BSA_SUCCESS &&
            p_data->get_atr.result_code == BSA_SAC_RESULT_OK)
        {
            printf("Successfully received %d bytes ATR and saved to file '%s'\n",
                p_data->get_atr.atr_len, ATR_DATA_FILE_NAME);
            app_save_file(ATR_DATA_FILE_NAME, p_data->get_atr.p_atr,
                p_data->get_atr.atr_len);
        }
        break;
    case BSA_SAC_SET_SIM_EVT:
        printf("BSA_SAC_SET_SIM_EVT Status %d Result %d Type %d\n",
            p_data->set_sim.status, p_data->set_sim.result_code, p_data->set_sim.set_sim);
        break;
    case BSA_SAC_CARD_READER_STATUS_EVT:
        printf("BSA_SAC_CARD_READER_STATUS_EVT Status %d Result %d\n",
            p_data->reader_status.status, p_data->reader_status.result_code);
        printf("BSA_SAC_CARD_READER_STATUS_EVT Card Reader Status %d\n",
            p_data->reader_status.reader_status);
        break;
    case BSA_SAC_SET_PROTOCOL_EVT:
        printf("BSA_SAC_SET_PROTOCOL_EVT Status %d Result %d\n",
            p_data->set_protocol.status, p_data->set_protocol.result_code);
        break;
    case BSA_SAC_DISCONNECT_IND_EVT:
        printf("BSA_SAC_DISCONNECT_IND_EVT Type %d\n", p_data->disconnect_ind.disc_type);
        if (p_data->disconnect_ind.disc_type == BSA_SAC_DISC_GRACEFUL)
        {
            app_sac_disconnect();
        }
        else
        {
            app_sac_cb.handle = BSA_SAC_INVALID_HANDLE;
        }
        break;
    case BSA_SAC_STATUS_IND_EVT:
        printf("BSA_SAC_STATUS_IND_EVT Status %d\n", p_data->status_ind.server_status);
        break;
    case BSA_SAC_ERROR_RESP_EVT:
        printf("BSA_SAC_ERROR_RESP_EVT\n");
        app_sac_disconnect();
        break;
    default:
        fprintf(stderr, "app_sac_cback unknown event:%d\n", event);
        break;
    }
}


/*******************************************************************************
**
** Function         app_start_sac
**
** Description      Example of function to Enable SAC Client application
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_start_sac(void)
{
    tBSA_STATUS status;
    tBSA_SAC_ENABLE enable_param;

    printf("app_start_sac\n");

    /* Initialize the control structure */
    memset(&app_sac_cb, 0, sizeof(app_sac_cb));
    app_sac_cb.max_msg_size = DEFAULT_MESSAGE_SIZE;
    app_sac_cb.handle = BSA_SAC_INVALID_HANDLE;

    status = BSA_SacEnableInit(&enable_param);

    enable_param.p_cback = app_sac_cback;

    status = BSA_SacEnable(&enable_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_start_sac: Unable to enable SAC status:%d\n", status);
    }
    return status;
}
/*******************************************************************************
**
** Function         app_stop_sac
**
** Description      Example of function to stop and disable SAC client module
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_stop_sac(void)
{
    tBSA_STATUS status;

    tBSA_SAC_DISABLE disable_param;

    printf("app_stop_sac\n");

    status = BSA_SacDisableInit(&disable_param);
    status = BSA_SacDisable(&disable_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_stop_sac: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_connect
**
** Description      Example of function to open up SAP connection to peer device
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_connect(void)
{
    tBSA_STATUS status = 0;
    int device_index;
    tBSA_SAC_CONNECT param;

    printf("app_sac_connect\n");
    BSA_SacConnectInit(&param);

    printf("SAP Connect menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
    device_index = app_get_choice("Select source");
    switch (device_index)
    {
    case 0:
        /* Devices from XML databased */
        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();
        app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= APP_NUM_ELEMENTS(app_xml_remote_devices_db)) || (device_index < 0))
        {
            printf("Wrong Remote device index\n");
            return BSA_SUCCESS;
        }

        if (app_xml_remote_devices_db[device_index].in_use == FALSE)
        {
            printf("Wrong Remote device index\n");
            return BSA_SUCCESS;
        }
        bdcpy(param.bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
        break;
    case 1:
        /* Devices from Discovery */
        app_disc_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= APP_DISC_NB_DEVICES) || (device_index < 0))
        {
            printf("Wrong Remote device index\n");
            return BSA_SUCCESS;
        }
        if (app_discovery_cb.devs[device_index].in_use == FALSE)
        {
            printf("Wrong Remote device index\n");
            return BSA_SUCCESS;
        }
        bdcpy(param.bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
        break;
    default:
        printf("Wrong selection !!:\n");
        return BSA_SUCCESS;
    }

    param.sec_mask = DEFAULT_SEC_MASK;
    param.msg_size_max = app_sac_cb.max_msg_size;

    status = BSA_SacConnect(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sac_connect: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_disconnect
**
** Description      Example of function to end a SAP connection
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_disconnect(void)
{
    tBSA_STATUS status;
    tBSA_SAC_DISCONNECT param;

    printf("app_sac_disconnect\n");

    if (app_sac_cb.handle == BSA_SAC_INVALID_HANDLE)
    {
        printf("Not connected\n");
        return BSA_SUCCESS;
    }

    status = BSA_SacDisconnectInit(&param);

    param.handle = app_sac_cb.handle;

    status = BSA_SacDisconnect(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sac_disconnect: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_get_atr
**
** Description      Example of function to get ATR from SAP server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_get_atr(void)
{
    tBSA_STATUS status;
    tBSA_SAC_GET_ATR param;

    printf("app_sac_get_atr\n");

    if (app_sac_cb.handle == BSA_SAC_INVALID_HANDLE)
    {
        printf("Not connected\n");
        return BSA_SUCCESS;
    }

    status = BSA_SacGetATRInit(&param);

    param.handle = app_sac_cb.handle;

    status = BSA_SacGetATR(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sac_get_atr: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_transfer_apdu
**
** Description      Example of function to transfer APDU
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_transfer_apdu(void)
{
    tBSA_STATUS status;
    tBSA_SAC_TRANS_APDU param;
    char filename[256];
    int selection;
    int data_len;
    UINT8* p_buffer;

    printf("app_sac_transfer_apdu\n");

    if (app_sac_cb.handle == BSA_SAC_INVALID_HANDLE)
    {
        printf("Not connected\n");
        return BSA_SUCCESS;
    }

    status = BSA_SacTransferAPDUInit(&param);
    param.handle = app_sac_cb.handle;

    printf("SAP transfer APDU menu:\n");
    printf("    0 Read data from file\n");
    printf("    1 APDU sample 1 (select file)\n");
    printf("    2 APDU sample 2 (status)\n");
    selection = app_get_choice("Select");
    switch (selection)
    {
    case 0:
         if (app_get_string("Enter file name", filename, 256) <= 0)
         {
             printf("Invalid file name\n");
             return BSA_SUCCESS;
         }

         p_buffer = app_read_file(filename, &data_len);
         if (p_buffer == NULL)
         {
             printf("Failed to open file\n");
             return BSA_SUCCESS;
         }

        param.p_apdu = p_buffer;
        param.apdu_len = data_len;

        status = BSA_SacTransferAPDU(&param);
        free(p_buffer);
        break;
    case 1:
        param.p_apdu = apdu_sample1;
        param.apdu_len = sizeof(apdu_sample1);
        status = BSA_SacTransferAPDU(&param);
        break;
    case 2:
        param.p_apdu = apdu_sample2;
        param.apdu_len = sizeof(apdu_sample2);
        status = BSA_SacTransferAPDU(&param);
        break;
    default:
        printf("Wrong selection !!:\n");
        return BSA_SUCCESS;
    }


    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sac_transfer_apdu: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_set_sim
**
** Description      Example of function to set SIM status
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_set_sim(tBSA_SAC_SET_SIM_TYPE type)
{
    tBSA_STATUS status;
    tBSA_SAC_SET_SIM param;

    printf("app_sac_set_sim\n");

    if (app_sac_cb.handle == BSA_SAC_INVALID_HANDLE)
    {
        printf("Not connected\n");
        return BSA_SUCCESS;
    }

    status = BSA_SacSetSimInit(&param);

    param.handle = app_sac_cb.handle;
    param.set_sim = type;

    status = BSA_SacSetSim(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sac_set_sim: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_get_card_reader_status
**
** Description      Example of function to get card reader status
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_get_card_reader_status(void)
{
    tBSA_STATUS status;
    tBSA_SAC_GET_CARD_READER_STATUS param;

    printf("app_sac_get_card_reader_status\n");

    if (app_sac_cb.handle == BSA_SAC_INVALID_HANDLE)
    {
        printf("Not connected\n");
        return BSA_SUCCESS;
    }

    status = BSA_SacGetCardReaderStatusInit(&param);

    param.handle = app_sac_cb.handle;

    status = BSA_SacGetCardReaderStatus(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sac_get_card_reader_status: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_set_transport_protocol
**
** Description      Example of function to set transport protocol
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_set_transport_protocol(void)
{
    tBSA_STATUS status;
    tBSA_SAC_SET_PROTOCOL param;
    int protocol;

    printf("app_sac_set_transport_protocol\n");

    if (app_sac_cb.handle == BSA_SAC_INVALID_HANDLE)
    {
        printf("Not connected\n");
        return BSA_SUCCESS;
    }

    printf("    0 T=0\n");
    printf("    1 T=1\n");
    protocol = app_get_choice("Select protocol");
    if (protocol < 0 ||protocol > 1)
    {
        printf("Invalid protocol\n");
        return BSA_SUCCESS;
    }

    status = BSA_SacSetTransportProtocolInit(&param);

    param.handle = app_sac_cb.handle;
    param.protocol = (tBSA_SAC_TRANS_PROTOCOL)protocol;

    status = BSA_SacSetTransportProtocol(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sac_set_transport_protocol: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sac_set_max_message_size
**
** Description      Set max message size (to be used in Connect)
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sac_set_max_message_size(void)
{
    int max_msg_size;

    printf("app_sac_set_max_message_size\n");

    printf("Current max message size: %d\n", app_sac_cb.max_msg_size);
    max_msg_size = app_get_choice("Enter new max message size");

    app_sac_cb.max_msg_size = (UINT16)max_msg_size;
    printf("Max message size is set to: %d\n", app_sac_cb.max_msg_size);

    return BSA_SUCCESS;
}

/*******************************************************************************
**
** Function         main
**
** Description      This is the main function
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int main(int argc, char **argv)
{
    int status;
    int choice;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_sac_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Start SAC application */
    status = app_start_sac();
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "main: Unable to start SAC\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_sac_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
        case APP_SAC_MENU_NULL:
            break;
        case APP_SAC_MENU_CONNECT:
            app_sac_connect();
            break;
        case APP_SAC_MENU_DISCONNECT:
            app_sac_disconnect();
            break;
        case APP_SAC_MENU_GET_ATR:
            app_sac_get_atr();
            break;
        case APP_SAC_MENU_TRANSFER_APDU:
            app_sac_transfer_apdu();
            break;
        case APP_SAC_MENU_POWER_SIM_ON:
            app_sac_set_sim(BSA_SAC_SIM_POWER_ON);
            break;
        case APP_SAC_MENU_POWER_SIM_OFF:
            app_sac_set_sim(BSA_SAC_SIM_POWER_OFF);
            break;
        case APP_SAC_MENU_RESET_SIM:
            app_sac_set_sim(BSA_SAC_SIM_RESET);
            break;
        case APP_SAC_MENU_GET_CARD_READER_STATUS:
            app_sac_get_card_reader_status();
            break;
        case APP_SAC_MENU_SET_TRANSPORT_PROTOCOL:
            app_sac_set_transport_protocol();
            break;
        case APP_SAC_MENU_SET_MAX_MESSAGE_SIZE:
            app_sac_set_max_message_size();
            break;
        case APP_SAC_MENU_QUIT:
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_SAC_MENU_QUIT); /* While user don't exit application */

    /* Stop SAC application */
    app_stop_sac();

    /* Just to make sure we are getting the disable event before close the
    connection to the manager */
    sleep(2);

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}

