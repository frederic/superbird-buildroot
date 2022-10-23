/*****************************************************************************
 **
 **  Name:           app_switch_main.c
 **
 **  Description:    Switching from AV/AG to AVK/HF. This sample is provided to
 **                  to overview of sequence of operations needed to switch
 **                  between AV/AG to AVK/HF and vice-versa. Please refer to
 **                  API documentation and sample applications (app_hs, app_ag,
 **                  app_av and app_avk) for implementation details.
 **
 **
 **  Copyright (c) 2009-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
/* for exit */
#include <stdlib.h>
/* for sleep */
#include <unistd.h>
/* for strcpy */
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "app_switch.h"
#include "bsa_api.h"

#include "gki.h"
#include "uipc.h"

#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_xml_param.h"

#if (defined(BSA_HS_INCLUDED) && BSA_HS_INCLUDED==TRUE)
#include "app_hs.h"
#endif
#if (defined(BSA_AVK_INCLUDED) && BSA_AVK_INCLUDED==TRUE)
#include "app_avk.h"
#endif

#include "app_ag.h"
extern tAPP_AG_CB app_ag_cb;

/* Menu items */
enum
{
    APP_SWITCH_MENU_ABORT_DISC = 1,
    APP_SWITCH_MENU_DISCOVERY,
    APP_SWITCH_MENU_AV_AG_ENABLE,
    APP_SWITCH_MENU_AV_AG_CONNECT,
    APP_SWITCH_MENU_AVK_HF_ENABLE,
    APP_SWITCH_MENU_AVK_HF_CONNECT,
    APP_SWITCH_MENU_QUIT = 99
};


/*******************************************************************************
 **
 ** Function         app_switch_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_display_main_menu(void)
{
    APP_INFO0("\nBluetooth AV & AG TO AVK & HF Switch App Main menu");
    APP_INFO1("    %d => Abort Discovery", APP_SWITCH_MENU_ABORT_DISC);
    APP_INFO1("    %d => Start Discovery", APP_SWITCH_MENU_DISCOVERY);
    APP_INFO1("    %d => Switch to AG-AV Source Role", APP_SWITCH_MENU_AV_AG_ENABLE);
    APP_INFO1("    %d => Connect to Headset (AVK+HF)", APP_SWITCH_MENU_AV_AG_CONNECT);
    APP_INFO1("    %d => Switch to Headset Mode (AVK+HF)", APP_SWITCH_MENU_AVK_HF_ENABLE);
    APP_INFO1("    %d => Connect to AG & Av Source", APP_SWITCH_MENU_AVK_HF_CONNECT);
    APP_INFO1("    %d => Quit", APP_SWITCH_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_switch_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          TRUE if the event was completely handle, FALSE otherwise
 **
 *******************************************************************************/
static BOOLEAN app_switch_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
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
 ** Description      This is the main function to connect . It is assumed
 **                  that an other process handle security.
 **
 ** Parameters
 **
 ** Returns          status
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    int choice ;
    BOOLEAN bOnStartup = TRUE;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_switch_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Init XML state machine */
    app_xml_init();

    /*  Initially enter HS - AVK Role */
   app_hs_init();
   app_hs_enable();
   app_hs_register();
   app_avk_init(NULL);
   app_avk_register();
   app_switch_cb.current_role = APP_AVK_HF_ROLE;
   app_ag_cb.sco_route=BSA_SCO_ROUTE_HCI;

   APP_INFO0("Current Role: HS - AVK");

    do
    {
        app_switch_display_main_menu();

        choice = app_get_choice("Select action");

        switch (choice)
        {
        /*
         * AV Point To point Menu
         */
        case APP_SWITCH_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_SWITCH_MENU_DISCOVERY:
            /* Example to perform Device discovery (in blocking mode) */
            app_disc_start_regular(NULL);
            break;

        case APP_SWITCH_MENU_AV_AG_ENABLE:
            if( app_switch_cb.current_role == APP_AV_AG_ROLE )
            {
                APP_INFO0("Already AG-AV Source Role");
                break;
            }

            app_switch_av_ag_enable(bOnStartup);
            bOnStartup = FALSE;
            APP_INFO0("Current Role: HF-AG - AV");
            break;

        case APP_SWITCH_MENU_AV_AG_CONNECT:
            app_switch_av_ag_connect();
            break;

        case APP_SWITCH_MENU_AVK_HF_ENABLE:
            if( app_switch_cb.current_role == APP_AVK_HF_ROLE )
            {
                APP_INFO0("Already Headset Mode");
                break;
            }

            app_switch_avk_hf_enable();
            APP_INFO0("Current Role: HS - AVK");
            break;

        case APP_SWITCH_MENU_AVK_HF_CONNECT:
            app_switch_avk_hf_connect();
            break;

            /* Quit Menu */
        case APP_SWITCH_MENU_QUIT:
            APP_INFO0("main: Bye Bye");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_SWITCH_MENU_QUIT); /* While user don't exit application */

    /* Close the BSA connection */
    app_mgt_close();

    return 0;
}
