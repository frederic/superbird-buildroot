/*****************************************************************************
 **
 **  Name:           app_opc.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
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

#include "gki.h"
#include "uipc.h"
#include "bsa_api.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_opc.h"

#define APP_MY_VCARD_PATH "./my_vcard.vcf"
#define APP_YOUR_VCARD_PATH "./"

#define APP_XML_REM_DEVICES_FILE_PATH       "./bt_devices.xml"

/*
 * Globales Variables
 */
int progress_sum;


/*******************************************************************************
 **
 ** Function         app_opc_cback
 **
 ** Description      Example of OPC callback function
 **
 ** Parameters      event and message
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_opc_cback(tBSA_OPC_EVT event, tBSA_OPC_MSG *p_data)
{
    switch (event)
    {
        case BSA_OPC_OPEN_EVT: /* Connection Open (for info) */
            progress_sum = 0;
            printf("BSA_OPC_OPEN_EVT Status %d\n", p_data->status);
            break;

        case BSA_OPC_PROGRESS_EVT:
            progress_sum += p_data->prog.bytes;
            printf("BSA_OPC_PROGRESS_EVT %dkB of %dkB\n", (progress_sum >> 10),
                    ((int) p_data->prog.obj_size >> 10));
            break;

        case BSA_OPC_OBJECT_EVT:
            printf("BSA_OPC_OBJECT_EVT Object:%s status :%d\n",
                    p_data->object.name, p_data->object.status);
            break;

        case BSA_OPC_CLOSE_EVT:
            printf("BSA_OPC_CLOSE_EVT\n");
            break;

        case BSA_OPC_OBJECT_PSHD_EVT:
            printf("BSA_OPC_OBJECT_PSHD_EVT\n");
            break;

        default:
            fprintf(stderr, "app_opc_cback unknown event:%d\n", event);
            break;
    }
}

/*******************************************************************************
 **
 ** Function         app_opc_start
 **
 ** Description      Example of function to start OPP Client application
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_start(void)
{
    tBSA_STATUS status = BSA_SUCCESS;

    tBSA_OPC_ENABLE enable_param;

    printf("app_opc_start\n");

    status = BSA_OpcEnableInit(&enable_param);

    enable_param.sec_mask = BSA_SEC_NONE;

    enable_param.p_cback = app_opc_cback;

    status = BSA_OpcEnable(&enable_param);
    printf("app_opc_start Status: %d \n", status);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_opc_start: Unable to enable OPC status:%d\n",
                status);
    }
    return status;
}
/*******************************************************************************
 **
 ** Function         app_opc_stop
 **
 ** Description      Example of function to sttop OPP Server application
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_stop(void)
{
    tBSA_STATUS status;

    tBSA_OPC_DISABLE disable_param;

    printf("app_opc_stop\n");

    status = BSA_OpcDisableInit(&disable_param);

    status = BSA_OpcDisable(&disable_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_opc_stop: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_opc_disconnect
 **
 ** Description      Example of function to disconnect current device
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_disconnect(void)
{
    tBSA_STATUS status;

    tBSA_OPC_CLOSE close_param;

    printf("app_opc_disconnect\n");

    status = BSA_OpcCloseInit(&close_param);

    status = BSA_OpcClose(&close_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_opc_disconnect: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_opc_push_vc
 **
 ** Description      Example of function to push a v card to current peer device
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_push_vc(void)
{
    tBSA_STATUS status = 0;
    int device_index;
    tBSA_OPC_PUSH param;

    printf("app_opc_push_vc\n");
    status = BSA_OpcPushInit(&param);

    printf("Bluetooth OPC menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
    device_index = app_get_choice("Select source");

    switch (device_index)
    {
        case 0:
            /* Devices from XML database */
            /* Read the Remote device XML file to have a fresh view */
            app_read_xml_remote_devices();

            app_xml_display_devices(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db));
            device_index = app_get_choice("Select device");
            if ((device_index >= APP_NUM_ELEMENTS(app_xml_remote_devices_db)) ||
                (device_index < 0))
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }

            if (app_xml_remote_devices_db[device_index].in_use == FALSE)
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }
            bdcpy(param.bd_addr,
                    app_xml_remote_devices_db[device_index].bd_addr);
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

    param.format = BTA_OP_VCARD21_FMT;
    strncpy(param.send_path, APP_MY_VCARD_PATH, sizeof(param.send_path) - 1);
    status = BSA_OpcPush(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_OpcPush failed (%d)", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_opc_push_file
 **
 ** Description      Example of function to push a file to current peer device
 **
 ** Parameters      file name
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_push_file(char * p_file_name)
{
    tBSA_STATUS status = 0;
    int device_index;
    tBSA_OPC_PUSH param;

    printf("app_opc_push_file\n");
    status = BSA_OpcPushInit(&param);

    printf("Bluetooth OPC menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
    device_index = app_get_choice("Select source");

    switch (device_index)
    {
        case 0:
            /* Devices from XML database */
            /* Read the Remote device XML file to have a fresh view */
            app_read_xml_remote_devices();
            app_xml_display_devices(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db));
            device_index = app_get_choice("Select device");
            if ((device_index >= APP_NUM_ELEMENTS(app_xml_remote_devices_db)) ||
                (device_index < 0))
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }

            if (app_xml_remote_devices_db[device_index].in_use == FALSE)
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }
            bdcpy(param.bd_addr,
                    app_xml_remote_devices_db[device_index].bd_addr);
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

    param.format = BTA_OP_OTHER_FMT;
    strncpy(param.send_path, p_file_name, sizeof(param.send_path) - 1);
    param.send_path[BSA_OP_OBJECT_NAME_LEN_MAX - 1] = 0;

    status = BSA_OpcPush(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_opc_push_file: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_opc_exchange_vc
 **
 ** Description      Example of function to exhcange a vcard with current peer device
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_exchange_vc(void)
{
    tBSA_STATUS status = 0;
    int device_index;
    tBSA_OPC_EXCH_CARD param;

    printf("app_opc_exchange_vc\n");
    status = BSA_OpcExchCardInit(&param);

    printf("Bluetooth OPC menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
    device_index = app_get_choice("Select source");

    switch (device_index)
    {
        case 0:
            /* Devices from XML database */
            /* Read the Remote device XML file to have a fresh view */
            app_read_xml_remote_devices();
            app_xml_display_devices(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db));
            device_index = app_get_choice("Select device");
            if ((device_index >= APP_NUM_ELEMENTS(app_xml_remote_devices_db)) ||
                (device_index < 0) )
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }

            if (app_xml_remote_devices_db[device_index].in_use == FALSE)
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }
            bdcpy(param.bd_addr,
                    app_xml_remote_devices_db[device_index].bd_addr);
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

    strncpy(param.send_path, APP_MY_VCARD_PATH, sizeof(param.send_path) - 1);
    strncpy(param.recv_path, APP_YOUR_VCARD_PATH, sizeof(param.recv_path) - 1);

    status = BSA_OpcExchCard(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_opc_exchange_vc: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_opc_pull_vc
 **
 ** Description      Example of function to exchange a vcard with current peer device
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_pull_vc(void)
{
    tBSA_STATUS status = 0;
    int device_index;
    tBSA_OPC_PULL_CARD param;

    printf("app_opc_pull_vc\n");
    status = BSA_OpcPullCardInit(&param);

    printf("Bluetooth OPC menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
    device_index = app_get_choice("Select source");

    switch (device_index)
    {
        case 0:
            /* Devices from XML database */
            /* Read the Remote device XML file to have a fresh view */
            app_read_xml_remote_devices();
            app_xml_display_devices(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db));
            device_index = app_get_choice("Select device");
            if ((device_index >= APP_NUM_ELEMENTS(app_xml_remote_devices_db)) ||
                (device_index < 0))
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }

            if (app_xml_remote_devices_db[device_index].in_use == FALSE)
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }
            bdcpy(param.bd_addr,
                    app_xml_remote_devices_db[device_index].bd_addr);
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

    strncpy(param.recv_path, APP_YOUR_VCARD_PATH, sizeof(param.recv_path)- 1);

    status = BSA_OpcPullCard(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_opc_pull_vc: Error:%d\n", status);
    }
    return status;
}



