/*****************************************************************************
 **
 **  Name:           app_pan_main.c
 **
 **  Description:    Bluetooth PAN main application
 **
 **  Copyright (c) 2011-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>

#include "app_pan.h"

#include "bsa_api.h"
#include "app_mgt.h"
#include "app_utils.h"
#include "app_disc.h"
#include "app_xml_param.h"

/*
 * Global Variables
 */

/* UI keypress definition */
/* Menu items */
enum
{
    APP_PAN_MENU_DISC_ABORT  = 1,
    APP_PAN_MENU_DISC,
    APP_PAN_MENU_DISC_PAN,
    APP_PAN_MENU_ENABLE,
    APP_PAN_MENU_DISABLE,
    APP_PAN_MENU_SET_ROLE,
    APP_PAN_MENU_OPEN,
    APP_PAN_MENU_CLOSE,
    APP_PAN_MENU_SET_PFILTER,
    APP_PAN_MENU_SET_MFILTER,
    APP_PAN_MENU_DISPLAY_CON,
    APP_PAN_MENU_QUIT = 99
};

/*
 * Local functions
 */
static void app_pan_display_main_menu(void);
static BOOLEAN app_pan_management_callback(tBSA_MGT_EVT event,
        tBSA_MGT_MSG *p_data);


/******************************************************************************
 **
 ** Function         app_pan_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
static void app_pan_display_main_menu(void)
{
    APP_INFO0("\nBluetooth PAN Main menu:");
    APP_INFO1("    %d => Abort Service Discovery", APP_PAN_MENU_DISC_ABORT);
    APP_INFO1("    %d => Services Discovery", APP_PAN_MENU_DISC);
    APP_INFO1("    %d => Services Discovery (PAN)", APP_PAN_MENU_DISC_PAN);
    APP_INFO1("    %d => PAN Enable", APP_PAN_MENU_ENABLE);
    APP_INFO1("    %d => PAN Disable", APP_PAN_MENU_DISABLE);
    APP_INFO1("    %d => PAN Set Role", APP_PAN_MENU_SET_ROLE);
    APP_INFO1("    %d => PAN Connect", APP_PAN_MENU_OPEN);
    APP_INFO1("    %d => PAN Disconnect", APP_PAN_MENU_CLOSE);
    APP_INFO1("    %d => PAN Set Protocol Filter", APP_PAN_MENU_SET_PFILTER);
    APP_INFO1("    %d => PAN Set Multicast Address Filter",
            APP_PAN_MENU_SET_MFILTER);
    APP_INFO1("    %d => Display Connection List", APP_PAN_MENU_DISPLAY_CON);
    APP_INFO1("    %d => Quit", APP_PAN_MENU_QUIT);
}

/******************************************************************************
 **
 ** Function         app_pan_management_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          exit flag
 **
 ******************************************************************************/
static BOOLEAN app_pan_management_callback(tBSA_MGT_EVT event,
        tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_INFO0("app_management_callback BSA_MGT_STATUS_EVT");
        if (p_data->status.enable == FALSE)
        {
            APP_INFO0("\tBluetooth Stopped");
        }
        else
        {
            /* Re-Init PAN Application */
            APP_INFO0("\tBluetooth restarted => re-initialize the application");
            if (app_pan_init() < 0)
            {
                APP_ERROR0("Unable to init PAN");
            }
        }

        break;

    case BSA_MGT_DISCONNECT_EVT:
        APP_INFO1("app_management_callback BSA_MGT_DISCONNECT_EVT reason:%d",
                p_data->disconnect.reason);

        /* Connection with the Server lost => Just exit the application */
        exit(-1);

        break;

    default:
        APP_INFO1("app_management_callback unknown event", event);

        break;
    }

    return FALSE;
}

/******************************************************************************
 **
 ** Function         main
 **
 ** Description      This is the main function
 **
 ** Parameters
 **
 ** Returns          void
 **
 ******************************************************************************/
int main(int argc, char **argv)
{
    int choice;

    app_mgt_init();

    /* open BSA connection */
    if (app_mgt_open(NULL, app_pan_management_callback))
    {
        APP_ERROR0("Couldn't open successfully, exiting");

        exit(-1);
    }

    /* Init XML state machine */
    app_xml_init();

    /* Initialize PAN application */
    if (app_pan_init() < 0)
    {
        APP_ERROR0("Unable to init PAN");

        return -1;
    }

    APP_INFO0("PAN Application is in Single Connection mode");

    do
    {
        app_pan_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
        case APP_PAN_MENU_DISC_ABORT:
            app_disc_abort();

            break;

        case APP_PAN_MENU_DISC:
            app_disc_start_services(0);

            break;

        case APP_PAN_MENU_DISC_PAN:
            app_disc_start_services(BSA_PANU_SERVICE_MASK |
                    BSA_NAP_SERVICE_MASK | BSA_GN_SERVICE_MASK);

            break;

        case APP_PAN_MENU_ENABLE:
            app_pan_start();

            break;

        case APP_PAN_MENU_DISABLE:
            app_pan_stop();

            break;

        case APP_PAN_MENU_SET_ROLE:
            app_pan_set_role();

            break;

        case APP_PAN_MENU_OPEN:
            app_pan_open();

            break;

        case APP_PAN_MENU_CLOSE:
            app_pan_close();

            break;

        case APP_PAN_MENU_SET_PFILTER:
            app_pan_set_pfilter();

            break;

        case APP_PAN_MENU_SET_MFILTER:
            app_pan_set_mfilter();

            break;

        case APP_PAN_MENU_DISPLAY_CON:
            app_pan_con_display();

            break;

        case APP_PAN_MENU_QUIT:
            break;

        default:
            APP_ERROR1("main: Unknown choice:%d", choice);

            break;
        }
    } while (choice != APP_PAN_MENU_QUIT);
    /* While user don't exit application */

    /* Close BSA before exiting (to release resources) */
    app_mgt_close();

    exit(0);
}
