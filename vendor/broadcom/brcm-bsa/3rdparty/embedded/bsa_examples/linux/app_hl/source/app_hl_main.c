/*****************************************************************************
**
**  Name:           app_hl_main.c
**
**  Description:    Bluetooth Health main application
**
**  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
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

#include "bsa_api.h"

#include "app_xml_param.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_services.h"
#include "app_hl_main.h"
#include "app_hl.h"


#define APP_XML_REM_DEVICES_FILE_PATH       "./bt_devices.xml"

#define APP_HL_TEST_FILE  "test_files/hl/tx_test_file.txt"


/* Menu items */
enum
{
    /* HL Menu */
    APP_HL_MENU_ABORT_DISC = 1,
    APP_HL_MENU_DISC,
    APP_HL_MENU_COD_DISC,
    APP_HL_MENU_SDP_DISC,
    APP_HL_MENU_SDP_QUERY,
    APP_HL_MENU_OPEN,
    APP_HL_MENU_CLOSE,
    APP_HL_MENU_RECONNECT,
    APP_HL_MENU_SEND_DATA,
    APP_HL_MENU_SEND_FILE,
    APP_HL_MENU_DISPLAY_CON,

    APP_HL_MENU_QUIT = 99
};
/*
 * Globales Variables
 */

/*
 * Local functions
 */


/*******************************************************************************
 **
 ** Function         app_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_display_main_menu (void)
{
    printf("\nBluetooth Health menu:\n");
    printf("\t%d => Discovery Abort\n", APP_HL_MENU_ABORT_DISC);
    printf("\t%d => Device Discovery\n", APP_HL_MENU_DISC);
    printf("\t%d => Device COD Discovery (Health)\n", APP_HL_MENU_COD_DISC);
    printf("\t%d => Device SDP Discovery (Health)\n", APP_HL_MENU_SDP_DISC);
    printf("\t%d => SDP Query (SDP)\n", APP_HL_MENU_SDP_QUERY);
    printf("\t%d => Open HL Connection\n", APP_HL_MENU_OPEN);
    printf("\t%d => Close HL Connection\n", APP_HL_MENU_CLOSE);
    printf("\t%d => Reconnect Connection\n", APP_HL_MENU_RECONNECT);
    printf("\t%d => Send Test Data\n", APP_HL_MENU_SEND_DATA);
    printf("\t%d => Send Test File\n", APP_HL_MENU_SEND_FILE);
    printf("\t%d => Display Health Connections\n", APP_HL_MENU_DISPLAY_CON);

    printf("\t%d => Quit\n", APP_HL_MENU_QUIT);
}


/*******************************************************************************
 **
 ** Function         app_hl_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_hl_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
            app_hl_init(FALSE);
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
 ** Function         main
 **
 ** Description      This is the main function
 **
 ** Returns          void
 **
 *******************************************************************************/
int main (int argc, char **argv)
{
    int choice;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_hl_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Init XML state machine */
    app_xml_init();

    if (app_hl_init(TRUE) < 0)
    {
        APP_ERROR0("Couldn't Initialize Health, exiting");
        exit(-1);
    }

    do
    {
        /* Display HL Menu */
        app_display_main_menu();

        /* Get user choice */
        choice = app_get_choice("Select action");

        switch(choice)
        {
        case APP_HL_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_HL_MENU_DISC:
            /* Example to perform Device discovery */
            app_disc_start_regular(NULL);
            break;

        case APP_HL_MENU_COD_DISC:
            /* Example to perform Health Peripheral Device discovery */
            app_disc_start_cod(0, BTM_COD_MAJOR_HEALTH, 0, NULL);
            break;

        case APP_HL_MENU_SDP_DISC:
            /* Example to perform Health SDP discovery */
            app_disc_start_services(BTA_HL_SERVICE_MASK);
            break;

        case APP_HL_MENU_SDP_QUERY:
            /* Example to perform SDP Query on a peer device */
            app_hl_sdp_query();
            break;

        case APP_HL_MENU_OPEN:
            app_hl_open();
            break;

        case APP_HL_MENU_CLOSE:
            app_hl_close();
            break;

        case APP_HL_MENU_RECONNECT:
            app_hl_reconnect();
            break;

        case APP_HL_MENU_SEND_DATA:
            /* Example to send test data to a peer device */
            app_hl_send_data();
            break;

        case APP_HL_MENU_SEND_FILE:
            /* Example to send test file to a peer device */
            app_hl_send_file(APP_HL_TEST_FILE);
            break;

        case APP_HL_MENU_DISPLAY_CON:
            app_hl_con_display();
            break;

        case APP_HL_MENU_QUIT:
            printf("main: Bye Bye\n");
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_HL_MENU_QUIT); /* While user don't exit application */

    /* Exit HH */
    app_hl_exit();

    /* Close the BSA connection */
    app_mgt_close();


    exit(0);
}
