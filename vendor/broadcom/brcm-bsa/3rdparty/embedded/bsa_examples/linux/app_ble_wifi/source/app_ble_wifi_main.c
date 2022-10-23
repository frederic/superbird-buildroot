/*****************************************************************************
**
**  Name:           app_ble_wifi_main.c
**
**  Description:    Bluetooth BLE WiFi helper main application
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
#include "app_ble_wifi_init.h"
#include "app_ble_wifi_resp.h"

/*
 * Defines
 */


/* BLE WIFI menu items */
enum
{
    APP_BLE_WIFI_MENU_START_RESPONDER = 1,
    APP_BLE_WIFI_MENU_STOP_RESPONDER,

    APP_BLE_WIFI_MENU_SEARCH_RESPONDER,
    APP_BLE_WIFI_MENU_START_INITIATOR,
    APP_BLE_WIFI_MENU_REGISTER_NOTIFICATION,
    APP_BLE_WIFI_MENU_SEND_DATA,
    APP_BLE_WIFI_MENU_STOP_INITIATOR,
    APP_BLE_WIFI_MENU_QUIT = 99
};
/*
 * Global Variables
 */


/*
 * Local functions
 */
static void app_ble_wifi_menu(void);



/*******************************************************************************
 **
 ** Function         app_ble_wifi_display_menu
 **
 ** Description      This is the WIFI menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_wifi_display_menu (void)
{
    APP_INFO0("\t**** BLE WIFI Responder menu ***");
    APP_INFO1("\t%d => Start WIFI RESPONDER", APP_BLE_WIFI_MENU_START_RESPONDER);
    APP_INFO1("\t%d => Stop WIFI RESPONDER\n", APP_BLE_WIFI_MENU_STOP_RESPONDER);
    APP_INFO0("\t**** BLE WIFI Initiator menu ***");
    APP_INFO1("\t%d => Search WIFI RESPONDER", APP_BLE_WIFI_MENU_SEARCH_RESPONDER);
    APP_INFO1("\t%d => Start WIFI INITIATOR", APP_BLE_WIFI_MENU_START_INITIATOR);
    APP_INFO1("\t%d => Register notification", APP_BLE_WIFI_MENU_REGISTER_NOTIFICATION);
    APP_INFO1("\t%d => Send WIFI data to responder", APP_BLE_WIFI_MENU_SEND_DATA);
    APP_INFO1("\t%d => Stop WIFI INITIATOR", APP_BLE_WIFI_MENU_STOP_INITIATOR);
    APP_INFO1("\t%d => Exit", APP_BLE_WIFI_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_ble_wifi_menu
 **
 ** Description      WiFi helper sub menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_wifi_menu(void)
{
    int choice;

    do
    {
        app_ble_wifi_display_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_BLE_WIFI_MENU_START_RESPONDER:
            app_ble_wifi_resp_start();
            break;

        case APP_BLE_WIFI_MENU_STOP_RESPONDER:
            app_ble_wifi_resp_stop();
            break;

        case APP_BLE_WIFI_MENU_START_INITIATOR:
            app_ble_wifi_init_start();
            break;

        case APP_BLE_WIFI_MENU_REGISTER_NOTIFICATION:
            app_ble_wifi_init_register_notification();
            break;

        case APP_BLE_WIFI_MENU_SEND_DATA:
            app_ble_wifi_init_send_data();
            break;

        case APP_BLE_WIFI_MENU_SEARCH_RESPONDER:
            app_disc_start_ble_regular(app_ble_wifi_init_disc_cback);
            break;

        case APP_BLE_WIFI_MENU_STOP_INITIATOR:
            app_ble_wifi_init_stop();
            break;

        case APP_BLE_WIFI_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_BLE_WIFI_MENU_QUIT); /* While user don't exit sub-menu */
}

/*******************************************************************************
 **
 ** Function         app_ble_wifi_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_ble_wifi_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
    if (app_mgt_open(NULL, app_ble_wifi_mgt_callback) < 0)
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
    app_ble_wifi_menu();

    /* Exit BLE mode */
    app_ble_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    exit(0);
}

