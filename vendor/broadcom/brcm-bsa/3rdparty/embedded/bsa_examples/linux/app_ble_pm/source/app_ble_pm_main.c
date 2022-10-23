/*****************************************************************************
**
**  Name:           app_ble_pm_main.c
**
**  Description:    Bluetooth BLE Proximity Monitor Main application
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
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
#include "app_ble_pm.h"
#include "app_ble2_brcm.h"

/*
 * Defines
 */


/* BLE PM menu items */
enum
{
    APP_BLE_PM_MENU_REGISTER = 1,
    APP_BLE_PM_MENU_DISCOVERY,
    APP_BLE_MENU_CONFIG_BLE_BG_CONN,
    APP_BLE_PM_MENU_OPEN,
    APP_BLE_PM_MENU_SEARCH_LINK_LOSS_SERVICE,
    APP_BLE_PM_MENU_SEARCH_IMMEDIATE_ALERT_SERVICE,
    APP_BLE_PM_MENU_SEARCH_TX_POWER_MONITOR_SERVER,
    APP_BLE_PM_MENU_CONFIG_LINKLOSS_ALERT_LV,
    APP_BLE_PM_MENU_CONFIG_IMMEDIATE_ALERT_LV,
    APP_BLE_PM_MENU_READ_TX_POWER,
    APP_BLE_PM_MENU_DISPLAY_CLIENT,

#if defined(APP_BLE2_BRCM_INCLUDED) && (APP_BLE2_BRCM_INCLUDED == TRUE)
    APP_BLE_PM_MENU_LE2_CONTROL,
    APP_BLE_PM_MENU_LE2_STATUS,
#endif

    APP_BLE_PM_MENU_QUIT = 99
};
/*
 * Global Variables
 */


/*
 * Local functions
 */
static void app_ble_pm_menu(void);



/*******************************************************************************
 **
 ** Function         app_ble_pm_display_menu
 **
 ** Description      This is the PM menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_pm_display_menu (void)
{
    APP_INFO0("\t**** BLE PM menu ***");
    APP_INFO1("\t%d => Register Proximity Monitor", APP_BLE_PM_MENU_REGISTER);
    APP_INFO1("\t%d => Discover Proximity Reporter", APP_BLE_PM_MENU_DISCOVERY);
    APP_INFO1("\t%d => Configure BLE Background Connection Type", APP_BLE_MENU_CONFIG_BLE_BG_CONN);
    APP_INFO1("\t%d => Connect Proximity Reporter", APP_BLE_PM_MENU_OPEN);
    APP_INFO1("\t%d => Search Link Loss Service", APP_BLE_PM_MENU_SEARCH_LINK_LOSS_SERVICE);
    APP_INFO1("\t%d => Search Immediate Alert Service", APP_BLE_PM_MENU_SEARCH_IMMEDIATE_ALERT_SERVICE);
    APP_INFO1("\t%d => Search TX Power Monitor Service", APP_BLE_PM_MENU_SEARCH_TX_POWER_MONITOR_SERVER);
    APP_INFO1("\t%d => Configure Link Loss Alert Level", APP_BLE_PM_MENU_CONFIG_LINKLOSS_ALERT_LV);
    APP_INFO1("\t%d => Configure Immediate Alert Level", APP_BLE_PM_MENU_CONFIG_IMMEDIATE_ALERT_LV);
    APP_INFO1("\t%d => Read TX Power(-100dBm ~ +20dBm)", APP_BLE_PM_MENU_READ_TX_POWER);
    APP_INFO1("\t%d => Show all attributes", APP_BLE_PM_MENU_DISPLAY_CLIENT);

#if defined(APP_BLE2_BRCM_INCLUDED) && (APP_BLE2_BRCM_INCLUDED == TRUE)
    APP_INFO0("\t**** Bluetooth Low Energy 2MBPS menu ****");
    APP_INFO1("\t%d => BLE2 Control", APP_BLE_PM_MENU_LE2_CONTROL);
    APP_INFO1("\t%d => BLE2 Status", APP_BLE_PM_MENU_LE2_STATUS);
#endif

    APP_INFO1("\t%d => Exit", APP_BLE_PM_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_ble_pm_menu
 **
 ** Description      Proximity Monitor sub menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_pm_menu(void)
{
    int choice, type;

    do
    {
        app_ble_pm_display_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_BLE_PM_MENU_REGISTER:
            app_ble_pm_main();
            break;

        case APP_BLE_PM_MENU_DISCOVERY:
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

        case APP_BLE_PM_MENU_OPEN:
            app_ble_client_open();
            break;

        case APP_BLE_PM_MENU_SEARCH_LINK_LOSS_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_LINKLOSS);
            break;

        case APP_BLE_PM_MENU_SEARCH_IMMEDIATE_ALERT_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_IMMEDIATE_ALERT);
            break;

        case APP_BLE_PM_MENU_SEARCH_TX_POWER_MONITOR_SERVER:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_TX_POWER);
            break;

        case APP_BLE_PM_MENU_CONFIG_LINKLOSS_ALERT_LV:
            app_ble_pm_write_linkloss_alert_level();
            break;

        case APP_BLE_PM_MENU_CONFIG_IMMEDIATE_ALERT_LV:
            app_ble_pm_write_immediate_alert_level();
            break;

        case APP_BLE_PM_MENU_READ_TX_POWER:
            app_ble_pm_read_tx_power();
            break;

        case APP_BLE_PM_MENU_DISPLAY_CLIENT:
            app_ble_client_display(1);
            break;

#if defined(APP_BLE2_BRCM_INCLUDED) && (APP_BLE2_BRCM_INCLUDED == TRUE)
        case APP_BLE_PM_MENU_LE2_CONTROL:
            choice = app_get_choice("BLE2 Enable/Disable (0 = Disable, 1 = Enable)");
            if(choice == 0)
            {
                APP_INFO0("Disabling BLE2");
                /* Disable BLE2 */
                app_ble2_brcm_config_flags_write(0, 0, 0);
            }
            else
            {
                APP_INFO0("Enabling BLE2");
                /* Enable LE2 (Advertising, Scanning and Connection) */
                app_ble2_brcm_config_flags_write(LE2_CFG_FLG_ADV_ENABLE |
                        LE2_CFG_FLG_SCAN_ENABLE | LE2_CFG_FLG_CON_ENABLE,
                        LE2_CFG_FLG_ACCESS_CODE_DEFAULT,
                        LE2_CFG_FLG_ACCESS_CODE_DEFAULT);
                /* Configure Extended Packet Length mode 3 (251 bytes) */
                app_ble2_brcm_ext_packet_len_write(
                        LE2_EXT_PKT_LEN_MODE_3, LE2_EXT_PKT_LEN_MODE_3_MAX_LEN);
            }
            break;

        case APP_BLE_PM_MENU_LE2_STATUS:
            {
                uint8_t flags, mode, packet_len;
                uint32_t lsb_access_code, msb_access_code;

                if (app_ble2_brcm_config_flags_read(&flags, &lsb_access_code,
                        &msb_access_code) < 0)
                {
                    break;
                }
                if (flags)
                {
                    APP_INFO1("BLE2 Enabled. Advertising:%d Scanning:%d Connection:%d",
                            flags & LE2_CFG_FLG_ADV_ENABLE?1:0,
                            flags & LE2_CFG_FLG_SCAN_ENABLE?1:0,
                            flags & LE2_CFG_FLG_CON_ENABLE?1:0);
                    APP_INFO1("BLE2 Access code LSB:0x%08X MSB:0x%08X",
                            lsb_access_code, msb_access_code);

                    if (app_ble2_brcm_ext_packet_len_read(&mode, &packet_len) < 0)
                    {
                        break;
                    }
                    APP_INFO1("BLE2 Mode:%d PacketLen:%d", mode, packet_len);
                }
                else
                {
                    APP_INFO0("BLE2 Disabled");
                }
            }
            break;
#endif

        case APP_BLE_PM_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_BLE_PM_MENU_QUIT); /* While user don't exit sub-menu */
}

/*******************************************************************************
 **
 ** Function         app_ble_pm_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_ble_pm_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
    if (app_mgt_open(NULL, app_ble_pm_mgt_callback) < 0)
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
    app_ble_pm_menu();

    /* Exit BLE mode */
    app_ble_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    exit(0);
}

