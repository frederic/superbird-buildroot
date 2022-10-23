/*****************************************************************************
**
**  Name:           app_ble_eddystone_main.c
**
**  Description:    Bluetooth BLE eddystone advertiser application
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
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
#include "app_ble_eddystone.h"

/*
 * Defines
 */

/* BLE EDDYSTONE menu items */
enum
{
    APP_BLE_EDDYSTONE_START_EDDYSTONE_UID_ADV = 1,
    APP_BLE_EDDYSTONE_START_EDDYSTONE_URL_ADV,
    APP_BLE_EDDYSTONE_START_EDDYSTONE_TLM_ADV,
    APP_BLE_EDDYSTONE_STOP_EDDYSTONE_ADV,
    APP_BLE_EDDYSTONE_START_EDDYSTONE_UID_SERVICE,
    APP_BLE_EDDYSTONE_STOP_EDDYSTONE_UID_SERVICE,
    APP_BLE_EDDYSTONE_DISCOVER_EDDYSTONE_SERVER,
    APP_BLE_EDDYSTONE_START_LE_CLIENT,
    APP_BLE_EDDYSTONE_SEND_DATA2_EDDYSTONE_SERVER,
    APP_BLE_EDDYSTONE_STOP_LE_CLIENT,
    APP_BLE_EDDYSTONE_MENU_QUIT = 99
};

/*
 * Global Variables
 */


/*
 * Local functions
 */
static void app_ble_eddystone_menu(void);



/*******************************************************************************
 **
 ** Function         app_ble_eddystone_display_menu
 **
 ** Description      This is the EDDYSTON menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_eddystone_display_menu (void)
{
    APP_INFO0("\t**** BLE EDDYSTONE Server menu ***\n");

    APP_INFO1("\t%d => Start EDDYSTONE UID Advertisement", APP_BLE_EDDYSTONE_START_EDDYSTONE_UID_ADV);
    APP_INFO1("\t%d => Start EDDYSTONE URL Advertisement", APP_BLE_EDDYSTONE_START_EDDYSTONE_URL_ADV);
    APP_INFO1("\t%d => Start EDDYSTONE TLM Advertisement\n", APP_BLE_EDDYSTONE_START_EDDYSTONE_TLM_ADV);

    APP_INFO1("\t%d => Stop EDDYSTONE Advertisement\n", APP_BLE_EDDYSTONE_STOP_EDDYSTONE_ADV);

    APP_INFO1("\t%d => Start EDDYSTONE UID Service", APP_BLE_EDDYSTONE_START_EDDYSTONE_UID_SERVICE);
    APP_INFO1("\t%d => Stop EDDYSTONE UID Service\n", APP_BLE_EDDYSTONE_STOP_EDDYSTONE_UID_SERVICE);

#ifdef APP_BLE_EDDYSTONE_CLIENT_MENU /* TODO, not needed yet */
    APP_INFO0("\t**** BLE EDDYSTONE Client menu ***\n");

    APP_INFO1("\t%d => Discover EDDYSTONE Server", APP_BLE_EDDYSTONE_DISCOVER_EDDYSTONE_SERVER);
    APP_INFO1("\t%d => Start LE client", APP_BLE_EDDYSTONE_START_LE_CLIENT);
    APP_INFO1("\t%d => Send data to EDDYSTONE Server", APP_BLE_EDDYSTONE_SEND_DATA2_EDDYSTONE_SERVER);
    APP_INFO1("\t%d => Stop LE client\n", APP_BLE_EDDYSTONE_STOP_LE_CLIENT);
#endif

    APP_INFO1("\t%d => Quit", APP_BLE_EDDYSTONE_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_ble_eddystone_menu
 **
 ** Description      EDDYSTONE advertiser applicatoin sub menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_eddystone_menu(void)
{
    int choice;

    app_ble_eddystone_init();

    do
    {
        app_ble_eddystone_display_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_BLE_EDDYSTONE_START_EDDYSTONE_UID_ADV:
            app_ble_eddystone_start_eddystone_uid_adv();
            break;

        case APP_BLE_EDDYSTONE_START_EDDYSTONE_URL_ADV:
            app_ble_eddystone_start_eddystone_url_adv();
            break;

        case APP_BLE_EDDYSTONE_START_EDDYSTONE_TLM_ADV:
            app_ble_eddystone_start_eddystone_tlm_adv();
            break;

        case APP_BLE_EDDYSTONE_STOP_EDDYSTONE_ADV:
            app_ble_eddystone_stop_eddystone_adv();
            break;

        case APP_BLE_EDDYSTONE_START_EDDYSTONE_UID_SERVICE:
            app_ble_eddystone_start_eddystone_uid_service();
            break;

        case APP_BLE_EDDYSTONE_STOP_EDDYSTONE_UID_SERVICE:
            app_ble_eddystone_stop_eddystone_uid_service();
            break;

        case APP_BLE_EDDYSTONE_DISCOVER_EDDYSTONE_SERVER:
            app_disc_start_ble_regular(app_ble_eddystone_init_disc_cback);
            break;

        case APP_BLE_EDDYSTONE_START_LE_CLIENT:
            app_ble_eddystone_start_le_client();
            break;

       case APP_BLE_EDDYSTONE_SEND_DATA2_EDDYSTONE_SERVER:
            app_ble_eddystone_send_data_to_eddystone_server();
            break;

        case APP_BLE_EDDYSTONE_STOP_LE_CLIENT:
            app_ble_eddystone_stop_le_client();
            break;

        case APP_BLE_EDDYSTONE_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_BLE_EDDYSTONE_MENU_QUIT); /* While user don't exit sub-menu */
}

/*******************************************************************************
 **
 ** Function         app_ble_eddystone_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_ble_eddystone_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
        APP_DEBUG1("app_management_callback BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
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

    app_mgt_init();
    /* Open connection to BSA Server */
    status = app_mgt_open(NULL, app_ble_eddystone_mgt_callback);
    if (status < 0)
    {
        APP_ERROR0("Unable to Connect to server");
        return -1;
    }

    /* Start BLE application */
    status = app_ble_start();
    if (status < 0)
    {
        APP_ERROR0("Couldn't Start BLE app, exiting");
        exit(-1);
    }

    /* The main BLE loop */
    app_ble_eddystone_menu();

    /* Exit BLE mode */
    app_ble_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
