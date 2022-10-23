/*****************************************************************************
**
**  Name:           app_ble_rscc_main.c
**
**  Description:    BLE Running Speed and Cadence Collector main application
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
#include "app_ble_rscc.h"

/*
 * Defines
 */


/* BLE RSCC menu items */
enum
{
    APP_BLE_RSCC_MENU_REGISTER = 1,
    APP_BLE_RSCC_MENU_DISCOVERY,
    APP_BLE_RSCC_MENU_CONFIG_BLE_BG_CONN,
    APP_BLE_RSCC_MENU_OPEN,
    APP_BLE_RSCC_MENU_SEARCH_RUNNING_SPEED_AND_CADENCE_SERVICE,
    APP_BLE_RSCC_MENU_SEARCH_DEVICE_INFORMATION_SERVICE,
    APP_BLE_RSCC_MENU_READ_RSCS_FEATURE,
    APP_BLE_RSCC_MENU_READ_RSCS_LOCATION,
    APP_BLE_RSCC_MENU_ENABLE_SC_CONTROL_POINT,
    APP_BLE_RSCC_MENU_DISABLE_SC_CONTROL_POINT,
    APP_BLE_RSCC_MENU_SET_CUMULATIVE_VALUE,
    APP_BLE_RSCC_MENU_START_SENSOR_CALIBRATION,
    APP_BLE_RSCC_MENU_START_MEASURE,
    APP_BLE_RSCC_MENU_STOP_MEASURE,
    APP_BLE_RSCC_MENU_DISPLAY_CLIENT,
    APP_BLE_RSCC_MENU_QUIT = 99
};
/*
 * Global Variables
 */


/*
 * Local functions
 */
static void app_ble_rscc_menu(void);


/*******************************************************************************
 **
 ** Function         app_ble_rscc_display_menu
 **
 ** Description      This is the RSCC menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_rscc_display_menu (void)
{
    APP_INFO0("\t**** BLE RSCC menu ***");
    APP_INFO1("\t%d => Register RSC Collector", APP_BLE_RSCC_MENU_REGISTER);
    APP_INFO1("\t%d => Discover RSC Sensor", APP_BLE_RSCC_MENU_DISCOVERY);
    APP_INFO1("\t%d => Configure BLE Background Connection Type", APP_BLE_RSCC_MENU_CONFIG_BLE_BG_CONN);
    APP_INFO1("\t%d => Connect RSC Sensor", APP_BLE_RSCC_MENU_OPEN);
    APP_INFO1("\t%d => Search Running Speed and Cadence Service", APP_BLE_RSCC_MENU_SEARCH_RUNNING_SPEED_AND_CADENCE_SERVICE);
    APP_INFO1("\t%d => Search Device Information Service", APP_BLE_RSCC_MENU_SEARCH_DEVICE_INFORMATION_SERVICE);
    APP_INFO1("\t%d => Read RSC Sensor Feature",APP_BLE_RSCC_MENU_READ_RSCS_FEATURE);
    APP_INFO1("\t%d => Read RSC Sensor Location", APP_BLE_RSCC_MENU_READ_RSCS_LOCATION);
    APP_INFO1("\t%d => Enable SC Control Point", APP_BLE_RSCC_MENU_ENABLE_SC_CONTROL_POINT);
    APP_INFO1("\t%d => Disable SC Control Point", APP_BLE_RSCC_MENU_DISABLE_SC_CONTROL_POINT);
    APP_INFO1("\t%d => Set Cumulative Value", APP_BLE_RSCC_MENU_SET_CUMULATIVE_VALUE);
    APP_INFO1("\t%d => Start Sensor Calibration", APP_BLE_RSCC_MENU_START_SENSOR_CALIBRATION);
    APP_INFO1("\t%d => Start Measure Data from Sensor", APP_BLE_RSCC_MENU_START_MEASURE);
    APP_INFO1("\t%d => Stop Measure Data from Sensor", APP_BLE_RSCC_MENU_STOP_MEASURE);
    APP_INFO1("\t%d => Show all attributes", APP_BLE_RSCC_MENU_DISPLAY_CLIENT);
    APP_INFO1("\t%d => Exit", APP_BLE_RSCC_MENU_QUIT);
}


/*******************************************************************************
 **
 ** Function         app_ble_rscc_menu
 **
 ** Description      RSC Collector menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_rscc_menu(void)
{
    int choice, type;

    do
    {
        app_ble_rscc_display_menu();

        choice = app_get_choice("Menu");

        switch(choice)
        {
        case APP_BLE_RSCC_MENU_REGISTER:
            app_ble_rscc_main();
            break;

        case APP_BLE_RSCC_MENU_DISCOVERY:
            app_disc_start_ble_regular(NULL);
            break;

        case APP_BLE_RSCC_MENU_CONFIG_BLE_BG_CONN:
            type = app_get_choice("Select conn type(0 = None, 1 = Auto)");
            if(type == 0 || type == 1)
            {
                app_dm_set_ble_bg_conn_type(type);
            }
            else
            {
                APP_ERROR1("Unknown type:%d", type);
            }
            break;

        case APP_BLE_RSCC_MENU_OPEN:
            app_ble_client_open();
            break;

        case APP_BLE_RSCC_MENU_SEARCH_RUNNING_SPEED_AND_CADENCE_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_RUNNING_SPEED_AND_CADENCE);
            break;

        case APP_BLE_RSCC_MENU_SEARCH_DEVICE_INFORMATION_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_DEVICE_INFORMATION);
            break;

        case APP_BLE_RSCC_MENU_READ_RSCS_FEATURE:
            app_ble_rscc_read_rscs_feature();
            break;

        case APP_BLE_RSCC_MENU_READ_RSCS_LOCATION:
            app_ble_rscc_read_rscs_location();
            break;

        case APP_BLE_RSCC_MENU_ENABLE_SC_CONTROL_POINT:
            app_ble_rscc_enable_sc_control_point(TRUE);
            break;

        case APP_BLE_RSCC_MENU_DISABLE_SC_CONTROL_POINT:
            app_ble_rscc_enable_sc_control_point(FALSE);
            break;

        case APP_BLE_RSCC_MENU_SET_CUMULATIVE_VALUE:
            app_ble_rscc_set_cumul_value();
            break;

        case APP_BLE_RSCC_MENU_START_SENSOR_CALIBRATION:
            app_ble_rscc_start_ss_cali();
            break;

        case APP_BLE_RSCC_MENU_START_MEASURE:
            app_ble_rscc_measure(TRUE);
            break;

        case APP_BLE_RSCC_MENU_STOP_MEASURE:
            app_ble_rscc_measure(FALSE);
            break;

        case APP_BLE_RSCC_MENU_DISPLAY_CLIENT:
            app_ble_client_display(1);
            break;

        case APP_BLE_RSCC_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_BLE_RSCC_MENU_QUIT); /* While user don't exit sub-menu */
}

/*******************************************************************************
 **
 ** Function         app_ble_rscc_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_ble_rscc_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
    status = app_mgt_open(NULL, app_ble_rscc_mgt_callback);
    if (status < 0)
    {
        APP_ERROR0("Unable to connect to server");
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
    app_ble_rscc_menu();

    /* Exit BLE mode */
    app_ble_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    exit(0);
}

