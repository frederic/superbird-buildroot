/*****************************************************************************
 **
 **  Name:           app_fts_main.c
 **
 **  Description:    Bluetooth File Transfer Server Menu application
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

#include "app_fts.h"

#include "app_mgt.h"
#include "app_utils.h"
#include "app_xml_param.h"

/*
 * Defines
 */
enum
{
    APP_FTS_ENABLE_AUTO_ACCEPT  = 1,
    APP_FTS_DISABLE_AUTO_ACCEPT,
    APP_FTS_QUIT = 99
};

/*
 * Global Variables
 */


/*******************************************************************************
 **
 ** Function         app_fts_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_fts_display_main_menu(void)
{
    printf("\nBluetooth File Transfer Server Main menu:\n");
    printf("\t%d => Enable auto access\n", APP_FTS_ENABLE_AUTO_ACCEPT);
    printf("\t%d => Disable auto access\n", APP_FTS_DISABLE_AUTO_ACCEPT);
    printf("\t%d => Quit\n", APP_FTS_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_fts_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_fts_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
    int status;
    int choice;
    int opt;
    char* p_opt_value = NULL;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_fts_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Interpret the command line arguments */
    while((opt = getopt (argc, argv, "d:")) != -1)
    {
        switch (opt)
        {
            /* -d for root directory */
            case 'd':
                p_opt_value = optarg;
                break;

            default :
                printf("main: Unknown argument\n");
                break;
        }
    }

    printf("FTS Root path: %s\n",p_opt_value);

    /* Example of function to start FTS application */
    status = app_start_fts(p_opt_value);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "main: Unable to start FTS\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_fts_display_main_menu();

        choice = app_get_choice("Select action");

        switch (choice)
        {
        case APP_FTS_ENABLE_AUTO_ACCEPT:
            app_fts_auto_access(TRUE);
            break;
        case APP_FTS_DISABLE_AUTO_ACCEPT:
            app_fts_auto_access(FALSE);
            break;
        case APP_FTS_QUIT:
            printf("main: Bye Bye\n");
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_FTS_QUIT); /* While user don't exit application */

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
