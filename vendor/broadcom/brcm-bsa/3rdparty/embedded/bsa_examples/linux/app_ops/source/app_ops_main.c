/*****************************************************************************
 **
 **  Name:           app_ops_main.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>


#include "app_ops.h"

#include "app_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_xml_utils.h"

/*******************************************************************************
 **
 ** Function         app_ops_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters      event and message
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_ops_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
 ** Function         app_ops_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters      void
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ops_display_main_menu(void)
{
    printf("\nBluetooth Object Push Server Running\n");
    printf("    1 => enable auto accept\n");
    printf("    2 => disconnect device\n");
    printf("    9 => quit\n");
}

/*******************************************************************************
 **
 ** Function         main
 **
 ** Description      This is the main function
 **
 ** Parameters      argc and argv
 **
 ** Returns          int
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    int status;
    int choice;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_ops_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Example of function to start OPS application */
    status = app_start_ops();
    if (status != BSA_SUCCESS)
    {
        printf("main: Unable to start OPS\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_ops_display_main_menu();

        choice = app_get_choice("Select action");

        switch (choice)
        {

            case 1:
                app_ops_auto_accept();
                break;

            case 2:
                app_ops_disconnect();
                break;

            case 9:
                break;

            default:
                printf("main: Unknown choice:%d\n", choice);
                break;
        }
    }
    while (choice != 9); /* While user don't exit application */

    /* example to stop OPC service */
    app_stop_ops();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}


