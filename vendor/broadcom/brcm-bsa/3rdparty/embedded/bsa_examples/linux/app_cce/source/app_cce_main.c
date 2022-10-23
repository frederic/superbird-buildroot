/*****************************************************************************
**
**  Name:           app_cce_main.c
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

#include "bsa_api.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_utils.h"
#include "app_cce.h"


/* ui keypress definition */
enum
{
    APP_CCE_MENU_NULL,
    APP_CCE_MENU_GET_ACCTS,
    APP_CCE_MENU_OPEN,
    APP_CCE_MENU_OBJ_LIST,
    APP_CCE_MENU_GET_OBJ,
    APP_CCE_MENU_SET_STATUS,
    APP_CCE_MENU_PUSH_OBJ,
    APP_CCE_MENU_FWD_OBJ,
    APP_CCE_MENU_SYNC_ACCT,
    APP_CCE_MENU_GET_ACCT_INFO,
    APP_CCE_MENU_CNS_START,
    APP_CCE_MENU_CNS_STOP,
    APP_CCE_MENU_NOTIF_REG,
    APP_CCE_MENU_CLOSE,
    APP_CCE_MENU_DISC,
    APP_CCE_MENU_QUIT = 99
};


/*******************************************************************************
**
** Function         app_cce_display_main_menu
**
** Description      This is the main menu
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_cce_display_main_menu(void)
{
    printf("\nBluetooth CTN Client Test Application\n");
    printf("    %d => Get Server Accounts\n", APP_CCE_MENU_GET_ACCTS);
    printf("    %d => Open Connection \n", APP_CCE_MENU_OPEN);
    printf("    %d => Get Object Listing\n", APP_CCE_MENU_OBJ_LIST);
    printf("    %d => Get Object\n", APP_CCE_MENU_GET_OBJ);
    printf("    %d => Set Object Status\n", APP_CCE_MENU_SET_STATUS);
    printf("    %d => Push Object\n", APP_CCE_MENU_PUSH_OBJ);
    printf("    %d => Forward Object\n", APP_CCE_MENU_FWD_OBJ);
    printf("    %d => Sync Account\n", APP_CCE_MENU_SYNC_ACCT);
    printf("    %d => Get Server Account Info\n", APP_CCE_MENU_GET_ACCT_INFO);
    printf("    %d => Start Notification Server\n", APP_CCE_MENU_CNS_START);
    printf("    %d => Stop Notification Server\n", APP_CCE_MENU_CNS_STOP);
    printf("    %d => Notification Registration\n", APP_CCE_MENU_NOTIF_REG);
    printf("    %d => Close Connection\n", APP_CCE_MENU_CLOSE);
    printf("    %d => Start Discovery\n", APP_CCE_MENU_DISC);
    printf("    %d => Quit\n", APP_CCE_MENU_QUIT);
}

/*******************************************************************************
**
** Function         app_cce_mgt_callback
**
** Description      This callback function is called in case of server
**                  disconnection (e.g. server crashes)
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static BOOLEAN app_cce_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
** Function         app_cce_menu_select_device
**
** Description      Menu to select a device
**
** Parameters
**
** Returns          BOOLEAN
**
*******************************************************************************/
static BOOLEAN app_cce_menu_select_device(BD_ADDR bda)
{
    int device_index;

    printf("Bluetooth CCE menu:\n");
    printf("  0 Device from XML database (already paired)\n");
    printf("  1 Device found in last discovery\n");
    device_index = app_get_choice("Select source");
    switch (device_index)
    {
    case 0 :
        /* Devices from XML databased */
        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();
        app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= APP_NUM_ELEMENTS(app_xml_remote_devices_db)) || (device_index < 0))
        {
            printf("Wrong Remote device index\n");
            return FALSE;
        }

        if (app_xml_remote_devices_db[device_index].in_use == FALSE)
        {
            printf("Wrong Remote device index\n");
            return FALSE;
        }
        bdcpy(bda, app_xml_remote_devices_db[device_index].bd_addr);
        break;
    case 1 :
        /* Devices from Discovery */
        app_disc_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= APP_DISC_NB_DEVICES) || (device_index < 0))
        {
            printf("Wrong Remote device index\n");
            return FALSE;
        }
        if (app_discovery_cb.devs[device_index].in_use == FALSE)
        {
            printf("Wrong Remote device index\n");
            return FALSE;
        }
        bdcpy(bda, app_discovery_cb.devs[device_index].device.bd_addr);
        break;
    default:
        printf("Wrong selection !!:\n");
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         app_cce_menu_select_connection
**
** Description      Menu to select a connection
**
** Parameters
**
** Returns          BOOLEAN
**
*******************************************************************************/
static BOOLEAN app_cce_menu_select_connection(UINT8 *p_cce_handle)
{
    UINT8 num, i, index;
    tAPP_CCE_CONN *p_conn;
    BOOLEAN ret = FALSE;

    p_conn = app_cce_get_connections(&num);

    if (num == 0 || p_conn == NULL)
    {
        printf("Please open a connection first\n");
        return FALSE;
    }

    index = 0;
    if (num > 1)
    {
        printf("CTN connections:\n");
        for (i = 0; i < num; i++)
        {
            printf("  %d Device:%s BD Address:%02x:%02x:%02x:%02x:%02x:%02x Instance:%d\n",
                i, p_conn[i].dev_name, p_conn[i].bd_addr[0], p_conn[i].bd_addr[1], p_conn[i].bd_addr[2],
                p_conn[i].bd_addr[3], p_conn[i].bd_addr[4], p_conn[i].bd_addr[5], p_conn[i].instance_id);
        }
        index = app_get_choice("Select a connection");
    }

    if (index < num)
    {
        *p_cce_handle = p_conn[index].cce_handle;
        ret = TRUE;
    }

    return ret;
}

/*******************************************************************************
**
** Function         app_cce_menu_get_obj_type
**
** Description      Menu to choose an object type
**
** Parameters
**
** Returns          BOOLEAN
**
*******************************************************************************/
static BOOLEAN app_cce_menu_get_obj_type(UINT8 *p_obj_type)
{
    BOOLEAN ret = FALSE;

    *p_obj_type = (UINT8)app_get_choice("Object types:\n  0 Calendar\n  1 Tasks\n  2 Notes\nSelect");

    if (*p_obj_type <= 2)
        ret = TRUE;

    return ret;
}

/*******************************************************************************
**
** Function         app_cce_menu_get_accounts
**
** Description      Get server accounts for the specified BD address
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_get_accounts()
{
    BD_ADDR  bda;

    if (!app_cce_menu_select_device(bda))
        return BSA_ERROR_CLI_BAD_PARAM;

    return app_cce_get_accounts(bda);
}

/*******************************************************************************
**
** Function         app_cce_menu_open
**
** Description      Example of function to open up CTN connection to peer device
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_open(void)
{
    BD_ADDR bda;
    UINT8 instance_id;

    if (!app_cce_menu_select_device(bda))
        return BSA_ERROR_CLI_BAD_PARAM;

    instance_id = app_get_choice("Enter Instance ID");

    return app_cce_open(bda, instance_id);
}

/*******************************************************************************
**
** Function         app_cce_menu_close
**
** Description      Example of function to disconnect current connection
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_close(void)
{
    UINT8 cce_handle;

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    return app_cce_close(cce_handle);
}

/*******************************************************************************
**
** Function         app_cce_menu_get_obj_list
**
** Description      Get object listing for the specified account ID
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_get_obj_list()
{
    UINT8 cce_handle;
    UINT8 obj_type;

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    if (!app_cce_menu_get_obj_type(&obj_type))
        return BSA_ERROR_CLI_BAD_PARAM;

    return app_cce_get_obj_list(cce_handle, obj_type);
}

/*******************************************************************************
**
** Function         app_cce_menu_get_obj
**
** Description      Get the object for specified handle from CTN server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_get_obj()
{
    UINT8 cce_handle;
    char obj_handle[20] = {0};
    int recurrent;

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    if (app_get_string("Enter Object Handle", obj_handle, sizeof(obj_handle)) < 0)
    {
        APP_ERROR0("app_get_string failed");
        return BSA_ERROR_CLI_BAD_PARAM;
    }

    recurrent = app_get_choice("Recurrent? Yes (1) / No (0)");

    return app_cce_get_obj(cce_handle, obj_handle, (BOOLEAN)recurrent);
}

/*******************************************************************************
**
** Function         app_cce_menu_set_status
**
** Description      Set the status of an object
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_set_status()
{
    UINT8 cce_handle;
    char obj_handle[20] = {0};
    UINT8 indicator;
    UINT8 value;
    UINT32 postpone_val;

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    if (app_get_string("Enter Object Handle", obj_handle, sizeof(obj_handle)) < 0)
    {
        APP_ERROR0("app_get_string failed");
        return BSA_ERROR_CLI_BAD_PARAM;
    }

    indicator = (UINT8)app_get_choice("Status types:\n  0 Participant\n  1 Alarm\n  2 Send\n  3 Delete\nSelect");

    switch (indicator)
    {
    case 0:
        value = (UINT8)app_get_choice("Status values:\n  0 Tentative\n  1 Needs action\n  2 Accepted\n  3 Declined\n  4 Delegated\n  5 Completed\n  6 In progress\nSelect");
        value += 3;
        if (value > 9)
            return BSA_ERROR_CLI_BAD_PARAM;
        break;
    case 1:
        value = (UINT8)app_get_choice("Status values:\n  0 Deactivate\n  1 Activate\n  2 Postpone\nSelect");
        if (value > 2)
            return BSA_ERROR_CLI_BAD_PARAM;
        break;
    case 2:
        value = (UINT8)app_get_choice("Status values:\n  0 Do not send\n  1 Send\nSelect");
        if (value > 1)
            return BSA_ERROR_CLI_BAD_PARAM;
        break;
    case 3:
        value = (UINT8)app_get_choice("Status values:\n  0 Not deleted\n  1 Deleted\nSelect");
        if (value > 1)
            return BSA_ERROR_CLI_BAD_PARAM;
        break;
    default:
        return BSA_ERROR_CLI_BAD_PARAM;
    }

    if (value == 2)
        postpone_val = app_get_choice("Postpone time (min)");
    else
        postpone_val = 0;

    return app_cce_set_obj_status(cce_handle, obj_handle, indicator, value, postpone_val);
}

/*******************************************************************************
**
** Function         app_cce_menu_push_obj
**
** Description      Example of function to push an object to CTN server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_push_obj()
{
    UINT8 cce_handle;
    UINT8 obj_type;
    char file[260];

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    if (!app_cce_menu_get_obj_type(&obj_type))
        return BSA_ERROR_CLI_BAD_PARAM;

    if (app_get_string("Enter File Name", file, sizeof(file)) < 0)
    {
        APP_ERROR0("app_get_string failed");
        return BSA_ERROR_CLI_BAD_PARAM;
    }
    return app_cce_push_obj(cce_handle, obj_type, TRUE, file);
}

/*******************************************************************************
**
** Function         app_cce_menu_forward_obj
**
** Description      Forward an object to one or more additional recipients
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_forward_obj()
{
    UINT8 cce_handle;
    char obj_handle[20] = {0};
    char recipients[256];

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    if (app_get_string("Enter Object Handle", obj_handle, sizeof(obj_handle)) < 0)
    {
        APP_ERROR0("app_get_string failed");
        return BSA_ERROR_CLI_BAD_PARAM;
    }

    if (app_get_string("Enter Recipients", recipients, sizeof(recipients)) < 0)
    {
        APP_ERROR0("app_get_string failed");
        return BSA_ERROR_CLI_BAD_PARAM;
    }


    return app_cce_forward_obj(cce_handle, obj_handle, recipients);
}

/*******************************************************************************
**
** Function         app_cce_menu_get_account_info
**
** Description      Get account info for the specified CAS instance id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_get_account_info()
{
    UINT8 cce_handle;
    int instance_id;

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    instance_id = app_get_choice("Enter Instance ID");

    return app_cce_get_account_info(cce_handle, instance_id);
}

/*******************************************************************************
**
** Function         app_cce_menu_sync_account
**
** Description      Synchronize CAS account with an external server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_sync_account()
{
    UINT8 cce_handle;

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    return app_cce_sync_account(cce_handle);
}

/*******************************************************************************
**
** Function         app_cce_menu_notif_reg
**
** Description      Set the Message Notification status to On or OFF on the CTN server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_cce_menu_notif_reg(void)
{
    UINT8 cce_handle;
    BOOLEAN notif_on;
    BOOLEAN alarm_on;

    if (!app_cce_menu_select_connection(&cce_handle))
        return BSA_ERROR_CLI_BAD_PARAM;

    notif_on = (BOOLEAN)app_get_choice("Set Notification  On (1) / Off (0)");
    alarm_on = (BOOLEAN)app_get_choice("Set Alarm  On (1) / Off (0)");

    return app_cce_notif_reg(cce_handle, notif_on, alarm_on);
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
    if (app_mgt_open(NULL, app_cce_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Example of function to start CCE application */
    status = app_cce_enable(NULL);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "main: Unable to start CCE\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_cce_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
        case APP_CCE_MENU_NULL:
            break;
        case APP_CCE_MENU_GET_ACCTS:
            app_cce_menu_get_accounts();
            break;
        case APP_CCE_MENU_OPEN:
            app_cce_menu_open();
            break;
        case APP_CCE_MENU_OBJ_LIST:
            app_cce_menu_get_obj_list();
            break;
        case APP_CCE_MENU_GET_OBJ:
            app_cce_menu_get_obj();
            break;
        case APP_CCE_MENU_SET_STATUS:
            app_cce_menu_set_status();
            break;
        case APP_CCE_MENU_PUSH_OBJ:
            app_cce_menu_push_obj();
            break;
        case APP_CCE_MENU_FWD_OBJ:
            app_cce_menu_forward_obj();
            break;
        case APP_CCE_MENU_SYNC_ACCT:
            app_cce_menu_sync_account();
            break;
        case APP_CCE_MENU_GET_ACCT_INFO:
            app_cce_menu_get_account_info();
            break;
        case APP_CCE_MENU_CNS_START:
            app_cce_cns_start("CTN-Notif-Svc");
            break;
        case APP_CCE_MENU_CNS_STOP:
            app_cce_cns_stop();
            break;
        case APP_CCE_MENU_NOTIF_REG:
            app_cce_menu_notif_reg();
            break;
        case APP_CCE_MENU_CLOSE:
            app_cce_menu_close();
            break;
        case APP_CCE_MENU_DISC:
            app_disc_start_regular(NULL);
            break;
        case APP_CCE_MENU_QUIT:
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_CCE_MENU_QUIT); /* While user don't exit application */

    /* example to stop CCE service */
    app_cce_disable();

    /* Just to make sure we are getting the disable event before close the
    connection to the manager */
    sleep(2);

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
