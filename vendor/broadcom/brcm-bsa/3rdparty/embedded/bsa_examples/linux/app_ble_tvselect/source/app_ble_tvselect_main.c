/*****************************************************************************
**
**  Name:           app_ble_tvselect_main.c
**
**  Description:    Bluetooth BLE TVSELECT helper main application
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdlib.h>

#include "app_ble.h"
#include "app_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_dm.h"
#include "app_ble_client.h"
#include "app_ble_server.h"
#include "app_ble_tvselect_client.h"
#include "app_ble_tvselect_server.h"

/*
 * Defines
 */


/* BLE TVSELECT menu items */
enum
{
    APP_BLE_TVSELECT_MENU_START_SERVER = 1,
    APP_BLE_TVSELECT_MENU_CHANGE_METADATA_CHNAME,
    APP_BLE_TVSELECT_MENU_CHANGE_METADATA_CHICON,
    APP_BLE_TVSELECT_MENU_CHANGE_METADATA_MEDIANAME,
    APP_BLE_TVSELECT_MENU_CHANGE_METADATA_MEDIAICON,
    APP_BLE_TVSELECT_MENU_STOP_SERVER,

    APP_BLE_TVSELECT_MENU_SEARCH_SERVER,
    APP_BLE_TVSELECT_MENU_OPEN_SERVER,
    APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_VERSION,
    APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_STATE,
    APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_CHANNEL_NAME,
    APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_CHANNEL_ICON,
    APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_MEDIA_NAME,
    APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_MEDIA_ICON,
    APP_BLE_TVSELECT_MENU_CLOSE_SERVER,
    APP_BLE_TVSELECT_MENU_QUIT = 99
};

/*
 * Global Variables
 */

/*
 * Local functions
 */
static void app_ble_tvselect_menu(void);

/*******************************************************************************
 **
 ** Function         app_ble_tvselect_display_menu
 **
 ** Description      This is the TVSELECT  menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_tvselect_display_menu (void)
{
    APP_INFO0("\t**** BLE TVSELECT Server menu ***");
    APP_INFO1("\t%d => Start TVSELECT Server", APP_BLE_TVSELECT_MENU_START_SERVER);
    APP_INFO1("\t%d => Change Meta Data - Channel Name", APP_BLE_TVSELECT_MENU_CHANGE_METADATA_CHNAME);
    APP_INFO1("\t%d => Change Meta Data - Channel Icon", APP_BLE_TVSELECT_MENU_CHANGE_METADATA_CHICON);
    APP_INFO1("\t%d => Change Meta Data - Program Name", APP_BLE_TVSELECT_MENU_CHANGE_METADATA_MEDIANAME);
    APP_INFO1("\t%d => Change Meta Data - Program Icon", APP_BLE_TVSELECT_MENU_CHANGE_METADATA_MEDIAICON);
    APP_INFO1("\t%d => Stop TVSELECT Server\n", APP_BLE_TVSELECT_MENU_STOP_SERVER);
    APP_INFO0("\t**** BLE TVSELECT Client menu ***");
    APP_INFO1("\t%d => Search TVSELECT Server", APP_BLE_TVSELECT_MENU_SEARCH_SERVER);
    APP_INFO1("\t%d => Open TVSELECT Server", APP_BLE_TVSELECT_MENU_OPEN_SERVER);
    APP_INFO1("\t%d => Command CP_VERSION", APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_VERSION);
    APP_INFO1("\t%d => Command CP_STATE", APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_STATE);
    APP_INFO1("\t%d => Command CP_CHANNEL_NAME", APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_CHANNEL_NAME);
    APP_INFO1("\t%d => Command CP_CHANNEL_ICON", APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_CHANNEL_ICON);
    APP_INFO1("\t%d => Command CP_MEDIA_INFO", APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_MEDIA_NAME);
    APP_INFO1("\t%d => Command CP_MEDIA_ICON", APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_MEDIA_ICON);
    APP_INFO1("\t%d => Close TVSELECT Server", APP_BLE_TVSELECT_MENU_CLOSE_SERVER);
    APP_INFO1("\t%d => Exit", APP_BLE_TVSELECT_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_ble_tvselect_menu
 **
 ** Description      TVSELECT helper sub menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_tvselect_menu(void)
{
    int choice;

    do
    {
        app_ble_tvselect_display_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_BLE_TVSELECT_MENU_START_SERVER:
            app_ble_tvselect_server_start();
            break;

        case APP_BLE_TVSELECT_MENU_CHANGE_METADATA_CHNAME:
            app_ble_tvselect_server_change_metadata_chname();
            break;

        case APP_BLE_TVSELECT_MENU_CHANGE_METADATA_CHICON:
            app_ble_tvselect_server_change_metadata_chicon();
            break;

        case APP_BLE_TVSELECT_MENU_CHANGE_METADATA_MEDIANAME:
            app_ble_tvselect_server_change_metadata_medianame();
            break;

        case APP_BLE_TVSELECT_MENU_CHANGE_METADATA_MEDIAICON:
            app_ble_tvselect_server_change_metadata_mediaicon();
            break;

        case APP_BLE_TVSELECT_MENU_STOP_SERVER:
            app_ble_tvselect_server_stop();
            break;

        case APP_BLE_TVSELECT_MENU_SEARCH_SERVER:
            app_disc_start_ble_regular(app_ble_tvselect_client_disc_cback);
            break;

        case APP_BLE_TVSELECT_MENU_OPEN_SERVER:
            app_ble_tvselect_open_server();
            break;

        case APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_VERSION:
            app_ble_tvselect_send_command_cp_version();
            break;

        case APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_STATE:
            app_ble_tvselect_send_command_cp_state();
            break;

        case APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_CHANNEL_NAME:
            app_ble_tvselect_send_command_cp_channel_name();
            break;

        case APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_CHANNEL_ICON:
            app_ble_tvselect_send_command_cp_channel_icon();
            break;

        case APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_MEDIA_NAME:
            app_ble_tvselect_send_command_cp_media_name();
            break;

        case APP_BLE_TVSELECT_MENU_SEND_COMMAND_CP_MEDIA_ICON:
            app_ble_tvselect_send_command_cp_media_icon();
            break;

        case APP_BLE_TVSELECT_MENU_CLOSE_SERVER:
            app_ble_tvselect_close_server();
            break;

        case APP_BLE_TVSELECT_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_BLE_TVSELECT_MENU_QUIT); /* While user don't exit sub-menu */
}

/*******************************************************************************
 **
 ** Function         app_ble_tvselect_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_ble_tvselect_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
            app_ble_start();
        }
        break;

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
 ** Function        main
 **
 ** Description     This is the main function
 **
 ** Parameters      Program's arguments
 **
 ** Returns         status
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    int status;

    /* Initialize BLE application */
    status = app_ble_init();
    if (status < 0)
    {
        APP_ERROR0("Couldn't Initialize BLE app, exiting");
        exit(-1);
    }

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_ble_tvselect_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Start BLE application */
    status = app_ble_start();
    if (status < 0)
    {
        APP_ERROR0("Couldn't Start BLE app, exiting");
        return -1;
    }

    /* The main BLE loop */
    app_ble_tvselect_menu();

    /* Exit BLE mode */
    app_ble_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    exit(0);
}

