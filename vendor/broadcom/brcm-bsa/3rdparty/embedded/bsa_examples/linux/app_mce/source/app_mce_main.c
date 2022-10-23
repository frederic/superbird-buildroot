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

#include "bta_mce_api.h"

#include "bsa_api.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_utils.h"
#include "app_mce.h"


/* ui keypress definition */
enum
{
    APP_MCE_MENU_NULL,
    APP_MCE_MENU_OPEN,
    APP_MCE_MENU_MN_START,
    APP_MCE_MENU_MN_STOP,
    APP_MCE_MENU_NOTIF_REG,
    APP_MCE_MENU_FOLDER_LIST,

    /*  Left as excercise for user

    APP_MCE_MENU_SET_FILTER_NONE,
    APP_MCE_MENU_SET_FILTER_MAX_COUNT,
    APP_MCE_MENU_SET_FILTER_OFFSET,
    APP_MCE_MENU_SET_FILTER_TYPE,
    APP_MCE_MENU_SET_FILTER_PBEGIN,
    APP_MCE_MENU_SET_FILTER_PEND,
    APP_MCE_MENU_SET_FILTER_STS,
    APP_MCE_MENU_SET_FILTER_RCIPT,
    APP_MCE_MENU_SET_FILTER_ORIG,
    APP_MCE_MENU_SET_FILTER_PRIO,
    APP_MCE_MENU_SET_FILTER_PARAM_MASK,
    APP_MCE_MENU_SET_FILTER_SUB_LEN,
    */
    APP_MCE_MENU_MSG_LIST,
    APP_MCE_MENU_GET_MSG,
    APP_MCE_MENU_PUSH_MSG,
    APP_MCE_MENU_UPD_INBOX,
    APP_MCE_MENU_SET_MSG_STS,
    APP_MCE_MENU_SET_FOLDER,
    APP_MCE_MENU_SET_MMI_MSG_STATUS,
    APP_MCE_MENU_ABORT,
    APP_MCE_MENU_GET_MAS_INS_INFO,
    APP_MCE_MENU_GET_MAS_INSTANCES,
    APP_MCE_MENU_CLOSE,
    APP_MCE_MENU_DISC,
    APP_MCE_MENU_QUIT
};


/*******************************************************************************
**
** Function         app_mce_display_main_menu
**
** Description      This is the main menu
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static void app_mce_display_main_menu(void)
{
    printf("\nBluetooth MAP Client Test Application\n");
    printf("    %d => Open Connection \n", APP_MCE_MENU_OPEN);
    printf("    %d => Start Notif Server\n", APP_MCE_MENU_MN_START);
    printf("    %d => Stop Notif Server\n", APP_MCE_MENU_MN_STOP);
    printf("    %d => Register Notification\n", APP_MCE_MENU_NOTIF_REG);
    printf("    %d => Folder List\n", APP_MCE_MENU_FOLDER_LIST);

    /*  Left as excercise for user

    printf("    %d => Set Filter None\n", APP_MCE_MENU_SET_FILTER_NONE);
    printf("    %d => Set Filter Max Count\n", APP_MCE_MENU_SET_FILTER_MAX_COUNT);
    printf("    %d => Set Filter Offset\n", APP_MCE_MENU_SET_FILTER_OFFSET);
    printf("    %d => Set Filter Type\n", APP_MCE_MENU_SET_FILTER_TYPE);
    printf("    %d => Set Filter PBegin\n", APP_MCE_MENU_SET_FILTER_PBEGIN);
    printf("    %d => Set Filter PEnd\n", APP_MCE_MENU_SET_FILTER_PEND);
    printf("    %d => Set Filter Status\n", APP_MCE_MENU_SET_FILTER_STS);
    printf("    %d => Set Filter Receipt\n", APP_MCE_MENU_SET_FILTER_RCIPT);
    printf("    %d => Set Filter Origin\n", APP_MCE_MENU_SET_FILTER_ORIG);
    printf("    %d => Set Filter Priority\n", APP_MCE_MENU_SET_FILTER_PRIO);
    printf("    %d => Set Filter Param Mask\n", APP_MCE_MENU_SET_FILTER_PARAM_MASK);
    printf("    %d => Set Filter Sub Len\n", APP_MCE_MENU_SET_FILTER_SUB_LEN);
    */
    printf("    %d => Message Listing\n", APP_MCE_MENU_MSG_LIST);
    printf("    %d => Get Message\n", APP_MCE_MENU_GET_MSG);
    printf("    %d => Push Message\n", APP_MCE_MENU_PUSH_MSG);
    printf("    %d => Update Inbox\n", APP_MCE_MENU_UPD_INBOX);
    printf("    %d => Set Message Status\n", APP_MCE_MENU_SET_MSG_STS);
    printf("    %d => Set Folder\n", APP_MCE_MENU_SET_FOLDER);
    printf("    %d => Abort\n", APP_MCE_MENU_ABORT);
    printf("    %d => Get Server Instance Info\n", APP_MCE_MENU_GET_MAS_INS_INFO);
    printf("    %d => Get MAS Instances\n", APP_MCE_MENU_GET_MAS_INSTANCES);
    printf("    %d => Close Connection\n", APP_MCE_MENU_CLOSE);
    printf("    %d => Discovery\n", APP_MCE_MENU_DISC);
    printf("    %d => Quit\n", APP_MCE_MENU_QUIT);
}

/*******************************************************************************
**
** Function         app_mce_mgt_callback
**
** Description      This callback function is called in case of server
**                  disconnection (e.g. server crashes)
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static BOOLEAN app_mce_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
** Function         app_mce_menu_select_device
**
** Description      Menu to select a device
**
** Parameters
**
** Returns          BOOLEAN
**
*******************************************************************************/
static BOOLEAN app_mce_menu_select_device(BD_ADDR bda)
{
    int device_index;

    printf("Bluetooth MCE menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
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
** Function         app_mce_menu_open
**
** Description      Example of function to open up MCE connection to peer device
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_open(void)
{
    BD_ADDR bda;
    UINT8 instance_id;

    if (!app_mce_menu_select_device(bda))
        return BSA_ERROR_MCE_FAIL;

    instance_id = app_get_choice("Enter MAS Instance ID");

    return app_mce_open(bda, instance_id);
}

/*******************************************************************************
**
** Function         app_mce_menu_close
**
** Description      Example of function to disconnect current connection
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_close(void)
{
    UINT8 instance_id;

    instance_id = app_get_choice("Enter MAS Instance ID");

    return app_mce_close(instance_id);
}

/*******************************************************************************
**
** Function         app_mce_menu_abort
**
** Description      Example of function to abort current operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_abort(void)
{
    UINT8  instance_id;

    instance_id = app_get_choice("Enter MAS Instance ID");

    return app_mce_abort(instance_id);
}

/*******************************************************************************
**
** Function         app_mce_menu_get_folder_list
**
** Description      Get Folder listing for the specified MAS instance id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_get_folder_list()
{
    UINT16 max_list_count = 100;
    UINT16 start_offset = 0;
    UINT8  instance_id;

    instance_id = app_get_choice("Enter MAS Instance ID");

    return app_mce_get_folder_list(instance_id, max_list_count, start_offset);
}

/*******************************************************************************
**
** Function         app_mce_menu_get_msg
**
** Description      Get the message for specified handle from MAS server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_get_msg()
{
    UINT8  instance_id;
    int len;
    char handlestr[20] = {0};
    BOOLEAN attachment;

    instance_id = app_get_choice("Enter MAS Instance ID");

    len = app_get_string("Enter Handle string", handlestr, sizeof(handlestr));
    if (len < 0)
    {
        APP_ERROR0("app_get_string failed");
        return -1;
    }

    attachment = app_get_choice("Attachment Yes (1) / No (0)");

    return app_mce_get_msg(instance_id, handlestr, attachment);
}

/*******************************************************************************
**
** Function         app_mce_menu_get_msg_list
**
** Description      Get message listing from the specified MAS server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_get_msg_list()
{
    UINT8  instance_id;
    char folder[BSA_MCE_ROOT_PATH_LEN_MAX] = {0};
    tBSA_MCE_MSG_LIST_FILTER_PARAM filter_param;
    int dir_len;

    instance_id = app_get_choice("Enter MAS Instance ID");

    dir_len = app_get_string("Enter directory name", folder, sizeof(folder));
    if (dir_len < 0)
    {
        APP_ERROR0("app_get_string failed");
        return -1;
    }

    memset(&filter_param, 0, sizeof(tBSA_MCE_MSG_LIST_FILTER_PARAM));
    filter_param.max_list_cnt = 65535;
    filter_param.read_status = BSA_MCE_READ_STATUS_NO_FILTERING;

    return app_mce_get_msg_list(instance_id, folder, &filter_param);
}

/*******************************************************************************
**
** Function         app_mce_menu_get_mas_ins_info
**
** Description      Get MAS info for the specified MAS instance id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_get_mas_ins_info()
{
    int instance_id;

    instance_id = app_get_choice("Enter MAS Instance ID");

    return app_mce_get_mas_ins_info(instance_id);
}

/*******************************************************************************
**
** Function         app_mce_menu_get_mas_instances
**
** Description      Get MAS instances for the specified BD address
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_get_mas_instances()
{
    BD_ADDR  bda;

    if (!app_mce_menu_select_device(bda))
        return BSA_ERROR_MCE_FAIL;

    return app_mce_get_mas_instances(bda);
}

/*******************************************************************************
**
** Function         app_mce_menu_notif_reg
**
** Description      Set the Message Notification status to On or OFF on the MSE.
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_notif_reg(void)
{
    int instance_id;
    int choice;

    instance_id = app_get_choice("Enter MAS Instance ID");

    APP_INFO0("Set Notif State:");
    APP_INFO0("    0 BSA_MCE_NOTIF_OFF");
    APP_INFO0("    1 BSA_MCE_NOTIF_ON");
    choice = app_get_choice("Register Notification 0 / 1 ?");

    return app_mce_notif_reg(instance_id, (BOOLEAN)choice);
}

/*******************************************************************************
**
** Function         app_mce_menu_set_msg_sts
**
** Description      Example of function to perform a set message status operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_set_msg_sts()
{
    int instance_id;
    char handlestr[20] = {0};
    int len;
    int status_indicator;
    int status_value;

    instance_id = app_get_choice("Enter MAS Instance ID");

    len = app_get_string("Enter Handle string", handlestr, sizeof(handlestr));
    if (len < 0)
    {
        APP_ERROR0("app_get_string failed");
        return -1;
    }

    APP_INFO0("Select Status Indicator:");
    APP_INFO0("    0 BSA_MCE_STS_INDTR_READ");
    APP_INFO0("    1 BSA_MCE_STS_INDTR_DELETE");
    status_indicator = app_get_choice("Status Indiator:");

    APP_INFO0("Select Status Value:");
    APP_INFO0("    0 BSA_MCE_STS_VALUE_NO");
    APP_INFO0("    1 BSA_MCE_STS_VALUE_YES");
    status_value = app_get_choice("Status Indiator:");

    return app_mce_set_msg_sts(instance_id, handlestr, status_indicator, status_value);
}

/*******************************************************************************
**
** Function         app_mce_menu_set_folder
**
** Description      Example of function to perform a set folder operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_set_folder()
{
    int instance_id;
    char string_dir[20] = {0};
    int dir_len;
    int choice;
    int dir_flag;

    instance_id = app_get_choice("Enter MAS Instance ID");

    dir_len = app_get_string("Enter directory name", string_dir, sizeof(string_dir));
    if (dir_len < 0)
    {
        APP_ERROR0("app_get_string failed");
        return -1;
    }

    APP_INFO0("Select Direction:");
    APP_INFO0("    0 Root or Down One Level");
    APP_INFO0("    1 Up One Level");
    choice = app_get_choice("Direction:");
    if (choice < 0 || choice > 1)
    {
        APP_ERROR0("Wrong choice");
        return -1;
    }
    dir_flag = choice ? BSA_MCE_DIR_NAV_UP_ONE_LVL : BSA_MCE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL;

    return app_mce_set_folder(instance_id, string_dir, dir_flag);
}

/*******************************************************************************
**
** Function         app_mce_menu_upd_inbox
**
** Description      Example of function to send update inbox request to MAS
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_upd_inbox(void)
{
    int instance_id;

    instance_id = app_get_choice("Enter MAS Instance ID");

    return app_mce_upd_inbox(instance_id);
}

/*******************************************************************************
**
** Function         app_mce_menu_push_msg
**
** Description      Example of function to push a message to MAS
**
** Parameters
**
** Returns          void
**
**            Left as an exercise for user to implement.
**            Please refer to:
**            1) BSA Programmers Guide for sequence of operations and,
**            2) QT Sample application demonstrating the MAP Push message functionality.
**
**              SAMPLE CODE FOR PUSH MESSAGE IMPLEMENTATION
**
*******************************************************************************/
static tBSA_STATUS app_mce_menu_push_msg(void)
{
    int instance_id;
    char folder[BSA_MCE_MAX_VALUE_LEN] = {0};
    int len = 0;

    instance_id = app_get_choice("Enter MAS Instance ID");

    len = app_get_string("Enter folder", folder, sizeof(folder));
    if (len < 0)
    {
        APP_ERROR0("app_get_string failed");
        return -1;
    }

    return app_mce_push_msg(instance_id, folder);
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
    if (app_mgt_open(NULL, app_mce_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Example of function to start MCE application */
    status = app_mce_enable(NULL);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "main: Unable to start MCE\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_mce_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
        case APP_MCE_MENU_NULL:
            break;
        case APP_MCE_MENU_OPEN:
            app_mce_menu_open();
            break;
        case APP_MCE_MENU_MN_START:
            app_mce_mn_start();
            break;
        case APP_MCE_MENU_MN_STOP:
            app_mce_mn_stop();
            break;
        case APP_MCE_MENU_NOTIF_REG:
            app_mce_menu_notif_reg();
            break;

            /* Get operations */
        case APP_MCE_MENU_FOLDER_LIST:
            app_mce_menu_get_folder_list();
            break;
        case APP_MCE_MENU_MSG_LIST:
            app_mce_menu_get_msg_list();
            break;
        case APP_MCE_MENU_GET_MSG:
            app_mce_menu_get_msg();
            break;
        case APP_MCE_MENU_GET_MAS_INS_INFO:
            app_mce_menu_get_mas_ins_info();
            break;
        case APP_MCE_MENU_GET_MAS_INSTANCES:
            app_mce_menu_get_mas_instances();
            break;

            /* Set operations */
            /*  Left as excercise for user

            case APP_MCE_MENU_SET_FILTER_NONE:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_NONE);
            break;
            case APP_MCE_MENU_SET_FILTER_MAX_COUNT:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_MAX_COUNT);
            break;
            case APP_MCE_MENU_SET_FILTER_OFFSET:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_OFFSET);
            break;
            case APP_MCE_MENU_SET_FILTER_TYPE:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_TYPE);
            break;
            case APP_MCE_MENU_SET_FILTER_PBEGIN:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_PBEGIN);
            break;
            case APP_MCE_MENU_SET_FILTER_PEND:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_PEND);
            break;
            case APP_MCE_MENU_SET_FILTER_STS:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_STS);
            break;
            case APP_MCE_MENU_SET_FILTER_RCIPT:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_RCIPT);
            break;
            case APP_MCE_MENU_SET_FILTER_ORIG:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_ORIG);
            break;
            case APP_MCE_MENU_SET_FILTER_PRIO:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_PRIO);
            break;
            case APP_MCE_MENU_SET_FILTER_PARAM_MASK:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_PARAM_MASK);
            break;
            case APP_MCE_MENU_SET_FILTER_SUB_LEN:
            app_mce_set_filter(APP_MCE_MENU_SET_FILTER_SUB_LEN);
            break;

            */
        case APP_MCE_MENU_SET_MSG_STS:
            app_mce_menu_set_msg_sts();
            break;
        case APP_MCE_MENU_SET_FOLDER:
            app_mce_menu_set_folder();
            break;
        case APP_MCE_MENU_PUSH_MSG:
            app_mce_menu_push_msg();
            break;
        case APP_MCE_MENU_UPD_INBOX:
            app_mce_menu_upd_inbox();
            break;
        case APP_MCE_MENU_ABORT:
            app_mce_menu_abort();
            break;
        case APP_MCE_MENU_CLOSE:
            app_mce_menu_close();
            break;
        case APP_MCE_MENU_DISC:
            /* Example to perform Device discovery (in blocking mode) */
            app_disc_start_regular(NULL);
            break;
        case APP_MCE_MENU_QUIT:
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_MCE_MENU_QUIT); /* While user don't exit application */

    /* example to stop MCE service */
    app_mce_disable();

    /* Just to make sure we are getting the disable event before close the
    connection to the manager */
    sleep(2);

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
