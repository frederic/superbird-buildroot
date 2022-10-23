/*****************************************************************************
 **
 **  Name:           app_pbs_main.c
 **
 **  Description:    Bluetooth PBAP application menu for BSA
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
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

#include "app_utils.h"
#include "app_mgt.h"
#include "app_xml_param.h"

#include "app_pbs.h"

/*
 * Defines
 */
enum
{
    APP_PBS_ENABLE_ACCESS  = 1,
    APP_PBS_DISABLE_ACCESS,
    APP_PBS_QUIT = 99
};

/*
 * Global Variables
 */

/*******************************************************************************
 **
 ** Function         app_pbs_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_pbs_display_main_menu(void)
{
    printf("\nBluetooth PhoneBook Server Main menu:\n");
    printf("    %d => Enable access\n",APP_PBS_ENABLE_ACCESS);
    printf("    %d => Disable access\n",APP_PBS_DISABLE_ACCESS);
    printf("    %d => Quit\n",APP_PBS_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_pbs_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_pbs_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
 ** Function         main
 **
 ** Description      This is the main function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    tBSA_STATUS status;
    int choice;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_pbs_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Example of function to start PBS application */
    status = app_start_pbs();
    if (status != BSA_SUCCESS)
    {
        printf("main: Unable to start PBS\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_pbs_display_main_menu();

        choice = app_get_choice("Select action");

        switch (choice)
        {
            case APP_PBS_ENABLE_ACCESS:
                printf("enable access");
                pbap_access_flag = BSA_PBS_ACCESS_ALLOW;
                break;
            case APP_PBS_DISABLE_ACCESS:
                printf("disable access");
                pbap_access_flag = BSA_PBS_ACCESS_FORBID;
                break;
            case APP_PBS_QUIT:
                printf("main: Bye Bye\n");
                break;

            default:
                printf("main: Unknown choice:%d\n", choice);
                break;
        }
    }
    while (choice != APP_PBS_QUIT); /* While user don't exit application */

    /* Example of function to stop PBS application */
    app_stop_pbs();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
