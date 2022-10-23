/*****************************************************************************
 **
 **  Name:           app_opc_main.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>


#include "app_opc.h"

#include "app_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_xml_utils.h"

#define APP_TEST_FILE_PATH "./test_file.txt"

/* ui keypress definition */
#define APP_OPC_KEY_PUSH        1
#define APP_OPC_KEY_EXCH        2
#define APP_OPC_KEY_PULL        3
#define APP_OPC_KEY_PUSH_FILE   4
#define APP_OPC_KEY_CLOSE       5
#define APP_OPC_KEY_DISC        6
#define APP_OPC_KEY_QUIT        9

/*******************************************************************************
 **
 ** Function         app_opc_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters      event and message
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_opc_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
 ** Function         app_opc_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters      void
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_opc_display_main_menu(void)
{
    printf("\nBluetooth Object Push Server Running\n");
    printf("    %d => push vcard\n", APP_OPC_KEY_PUSH);
    printf("    %d => exchange vcard\n", APP_OPC_KEY_EXCH);
    printf("    %d => pull vcard\n", APP_OPC_KEY_PULL);
    printf("    %d => push %s\n", APP_OPC_KEY_PUSH_FILE, APP_TEST_FILE_PATH);
    printf("    %d => close\n", APP_OPC_KEY_CLOSE);
    printf("    %d => discovery\n", APP_OPC_KEY_DISC);
    printf("    %d => quit\n", APP_OPC_KEY_QUIT);
}

/*******************************************************************************
 **
 ** Function         main
 **
 ** Description      This is the main function
 **
 ** Parameters      argc and argv
 **
 ** Returns          void
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    tBSA_STATUS status;
    int choice;

    /* Example of function to read the data-base of remote devices */
    if (app_read_xml_remote_devices() < 0)
    {
        printf("No remote device database found\n");
    }

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_opc_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Example of function to start OPC application */
    status = app_opc_start();
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "main: Unable to start OPC\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_opc_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
            case APP_OPC_KEY_PUSH:
                app_opc_push_vc();
                break;

            case APP_OPC_KEY_EXCH:
                app_opc_exchange_vc();
                break;

            case APP_OPC_KEY_PULL:
                app_opc_pull_vc();
                break;

            case APP_OPC_KEY_PUSH_FILE:
                app_opc_push_file(APP_TEST_FILE_PATH);
                break;

            case APP_OPC_KEY_CLOSE:
                app_opc_disconnect();
                break;

            case APP_OPC_KEY_DISC:
                /* Example to perform Device discovery (in blocking mode) */
                app_disc_start_regular(NULL);
                break;

            case APP_OPC_KEY_QUIT:
                break;

            default:
                printf("main: Unknown choice:%d\n", choice);
                break;
        }
    }
    while (choice != APP_OPC_KEY_QUIT); /* While user don't exit application */

    /* example to stop OPC service */
    app_opc_stop();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}

