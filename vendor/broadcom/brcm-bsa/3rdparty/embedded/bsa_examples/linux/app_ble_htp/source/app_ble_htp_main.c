/*****************************************************************************
**
**  Name:           app_ble_htp_main.c
**
**  Description:    Bluetooth BLE Health Thermometer Collector application
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
#include "app_ble_htp.h"

/*
 * Defines
 */

/* BLE HTP menu items */
enum
{
    APP_BLE_HTP_MENU_REGISTER = 1,
    APP_BLE_HTP_MENU_DISCOVERY,
    APP_BLE_MENU_CONFIG_BLE_BG_CONN,
    APP_BLE_HTP_MENU_OPEN,
    APP_BLE_HTP_MENU_SEARCH_HEALTH_THERMOMETER_SERVICE,
    APP_BLE_HTP_MENU_SEARCH_DEVICE_INFOR_SERVICE,
    APP_BLE_HTP_MENU_READ_TEMPERATURE_TYPE,
    APP_BLE_HTP_MENU_ENABLE_TEMPERATURE_MEASUREMENT,
    APP_BLE_HTP_MENU_DISBLE_TEMPERATURE_MEASUREMENT,
    APP_BLE_HTP_MENU_START_INTERMEDIATE_TEMPERATURE,
    APP_BLE_HTP_MENU_STOP_INTERMEDIATE_TEMPERATURE,
    APP_BLE_HTP_MENU_READ_MEASUREMENT_INTERVAL,
    APP_BLE_HTP_MENU_READ_MEASUREMENT_INTERVAL_RANGE,
    APP_BLE_HTP_MENU_WRITE_MEASUREMENT_INTERVAL,
    APP_BLE_HTP_MENU_ENABLE_MEASUREMENT_INTERVAL,
    APP_BLE_HTP_MENU_DISBLE_MEASUREMENT_INTERVAL,
    APP_BLE_HTP_MENU_DISPLAY_CLIENT,
    APP_BLE_HTP_MENU_QUIT = 99
};

/*
 * Global Variables
 */


/*
 * Local functions
 */
static void app_ble_htp_menu(void);



/*******************************************************************************
 **
 ** Function         app_ble_htp_display_menu
 **
 ** Description      This is the HTP menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_htp_display_menu (void)
{
    APP_INFO0("\t**** BLE HTP menu ***");
    APP_INFO1("\t%d => Register Health Thermometer", APP_BLE_HTP_MENU_REGISTER);
    APP_INFO1("\t%d => Discover Health Thermometer Sensor", APP_BLE_HTP_MENU_DISCOVERY);
    APP_INFO1("\t%d => Configure BLE Background Connection Type", APP_BLE_MENU_CONFIG_BLE_BG_CONN);
    APP_INFO1("\t%d => Connect Thermometer Sensor", APP_BLE_HTP_MENU_OPEN);
    APP_INFO1("\t%d => Search Theremometer Service",APP_BLE_HTP_MENU_SEARCH_HEALTH_THERMOMETER_SERVICE);
    APP_INFO1("\t%d => Search Device Information Service",APP_BLE_HTP_MENU_SEARCH_DEVICE_INFOR_SERVICE);
    APP_INFO1("\t%d => Read type of Temperature Measurement",APP_BLE_HTP_MENU_READ_TEMPERATURE_TYPE);
    APP_INFO1("\t%d => Enable Temperature measurement",APP_BLE_HTP_MENU_ENABLE_TEMPERATURE_MEASUREMENT);
    APP_INFO1("\t%d => Disable Temperature measurement",APP_BLE_HTP_MENU_DISBLE_TEMPERATURE_MEASUREMENT);
    APP_INFO1("\t%d => Start Intermediate temperature measurement",APP_BLE_HTP_MENU_START_INTERMEDIATE_TEMPERATURE);
    APP_INFO1("\t%d => Stop Intermediate temperature measurement",APP_BLE_HTP_MENU_STOP_INTERMEDIATE_TEMPERATURE);
    APP_INFO1("\t%d => Read Measurement Interval", APP_BLE_HTP_MENU_READ_MEASUREMENT_INTERVAL);
    APP_INFO1("\t%d => Read valid range for temperature measurement interval", APP_BLE_HTP_MENU_READ_MEASUREMENT_INTERVAL_RANGE);
    APP_INFO1("\t%d => Write Measurement Interval",APP_BLE_HTP_MENU_WRITE_MEASUREMENT_INTERVAL);
    APP_INFO1("\t%d => Enable temperature measurement with interval",APP_BLE_HTP_MENU_ENABLE_MEASUREMENT_INTERVAL);
    APP_INFO1("\t%d => Disable temperature measurement with interval",APP_BLE_HTP_MENU_DISBLE_MEASUREMENT_INTERVAL);
    APP_INFO1("\t%d => Show all attributes", APP_BLE_HTP_MENU_DISPLAY_CLIENT);
    APP_INFO1("\t%d => Quit", APP_BLE_HTP_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_ble_htp_menu
 **
 ** Description      Health Thermometer Collector sub menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_htp_menu(void)
{
    int choice, type;

    do
    {
        app_ble_htp_display_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_BLE_HTP_MENU_REGISTER:
            app_ble_htp_main();
            break;

        case APP_BLE_HTP_MENU_DISCOVERY:
            app_disc_start_ble_regular(NULL);
            break;

        case APP_BLE_MENU_CONFIG_BLE_BG_CONN:
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

        case APP_BLE_HTP_MENU_OPEN:
            app_ble_client_open();
            break;

        case APP_BLE_HTP_MENU_SEARCH_HEALTH_THERMOMETER_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER);
            break;

        case APP_BLE_HTP_MENU_SEARCH_DEVICE_INFOR_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_DEVICE_INFORMATION);
            break;

        case APP_BLE_HTP_MENU_READ_TEMPERATURE_TYPE:
            app_ble_htp_read_temperature_type();
            break;

        case APP_BLE_HTP_MENU_ENABLE_TEMPERATURE_MEASUREMENT:
            app_ble_htp_enable_temperature_measurement(TRUE);
            break;

        case APP_BLE_HTP_MENU_DISBLE_TEMPERATURE_MEASUREMENT:
            app_ble_htp_enable_temperature_measurement(FALSE);
            break;

        case APP_BLE_HTP_MENU_START_INTERMEDIATE_TEMPERATURE:
            app_ble_htp_intermediate_temperature(TRUE);
            break;

        case APP_BLE_HTP_MENU_STOP_INTERMEDIATE_TEMPERATURE:
            app_ble_htp_intermediate_temperature(FALSE);
            break;

        case APP_BLE_HTP_MENU_READ_MEASUREMENT_INTERVAL:
            app_ble_htp_read_measurement_interval();
            break;

        case APP_BLE_HTP_MENU_READ_MEASUREMENT_INTERVAL_RANGE:
            app_ble_htp_read_measurement_interval_range();
            break;

        case APP_BLE_HTP_MENU_WRITE_MEASUREMENT_INTERVAL:
            app_ble_htp_write_measurement_interval();
            break;

        case APP_BLE_HTP_MENU_ENABLE_MEASUREMENT_INTERVAL:
            app_ble_htp_enable_interval_measurement(TRUE);
            break;

        case APP_BLE_HTP_MENU_DISBLE_MEASUREMENT_INTERVAL:
             app_ble_htp_enable_interval_measurement(FALSE);
            break;

        case APP_BLE_HTP_MENU_DISPLAY_CLIENT:
            app_ble_client_display(1);
            break;

        case APP_BLE_HTP_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_BLE_HTP_MENU_QUIT); /* While user don't exit sub-menu */
}

/*******************************************************************************
 **
 ** Function         app_ble_htp_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_ble_htp_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
    status = app_mgt_open(NULL, app_ble_htp_mgt_callback);
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
    app_ble_htp_menu();

    /* Exit BLE mode */
    app_ble_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
