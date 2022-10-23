/*****************************************************************************
**
**  Name:           app_nsa_main.c
**
**  Description:    Bluetooth NSA Main application
**
**  Copyright (c) 2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdlib.h>

#include "app_nsa.h"
#include "app_mgt.h"
#include "app_utils.h"

/*
 * Defines
 */

/* NSA Menu items */
enum
{
    APP_NSA_MENU_ADD_IF = 1,
    APP_NSA_MENU_REMOVE_IF,
    APP_NSA_MENU_QUIT = 99
};


/*
 * Global Variables
 */

/*******************************************************************************
 **
 ** Function         app_display_nsa_menu
 **
 ** Description      This is the NSA menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_display_nsa_menu (void)
{
    APP_INFO0("Bluetooth NSA menu:");
    APP_INFO1("\t%d => Add NSA Interface", APP_NSA_MENU_ADD_IF);
    APP_INFO1("\t%d => Remove NSA Interface", APP_NSA_MENU_REMOVE_IF);
    APP_INFO1("\t%d => Exit Sub-Menu", APP_NSA_MENU_QUIT);
}


/*******************************************************************************
 **
 ** Function         app_nsa_menu
 **
 ** Description      Handle the NSA menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_nsa_menu(void)
{
    int choice;

    do
    {
        app_display_nsa_menu();

        choice = app_get_choice("Select action");

        switch(choice)
        {
        case APP_NSA_MENU_ADD_IF:
            app_nsa_add_if();
            break;

        case APP_NSA_MENU_REMOVE_IF:
            app_nsa_remove_if();
            break;

        case APP_NSA_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_NSA_MENU_QUIT); /* While user don't exit application */
}

/*******************************************************************************
 **
 ** Function         app_nsa_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_nsa_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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

    /* Initialize NSA application */
    status = app_nsa_init();
    if (status < 0)
    {
        APP_ERROR0("Couldn't Initialize NSA app, exiting");
        exit(-1);
    }

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_nsa_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* The main NSA loop */
    app_nsa_menu();

    app_nsa_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}

