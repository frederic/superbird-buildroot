/*****************************************************************************
 **
 **  Name:           app_avk_main.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "bsa_api.h"

#include "gki.h"
#include "uipc.h"

#include "app_avk.h"
#include "bsa_avk_api.h"
#include "app_xml_param.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_utils.h"

/* Menu items */
enum
{
    APP_AVK_MENU_DISCOVERY = 1,
    APP_AVK_MENU_REGISTER,
    APP_AVK_MENU_DEREGISTER,
    APP_AVK_MENU_OPEN,
    APP_AVK_MENU_CLOSE,
    APP_AVK_MENU_PLAY_START,
    APP_AVK_MENU_PLAY_STOP,
    APP_AVK_MENU_PLAY_PAUSE,
    APP_AVK_MENU_PLAY_NEXT_TRACK,
    APP_AVK_MENU_PLAY_PREVIOUS_TRACK,
    APP_AVK_MENU_RC_CMD,
    APP_AVK_MENU_GET_ELEMENT_ATTR,
    APP_AVK_MENU_GET_CAPABILITIES,
    APP_AVK_MENU_REGISTER_NOTIFICATION,
    APP_AVK_MENU_SEND_DELAY_RPT,
    APP_AVK_MENU_QUIT = 99
};


/*******************************************************************************
 **
 ** Function         app_avk_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_display_main_menu(void)
{
    APP_INFO0("\nBluetooth AVK Main menu:");

    APP_INFO1("\t%d  => Start Discovery", APP_AVK_MENU_DISCOVERY);
    APP_INFO1("\t%d  => AVK Register", APP_AVK_MENU_REGISTER);
    APP_INFO1("\t%d  => AVK DeRegister", APP_AVK_MENU_DEREGISTER);
    APP_INFO1("\t%d  => AVK Open", APP_AVK_MENU_OPEN);
    APP_INFO1("\t%d  => AVK Close", APP_AVK_MENU_CLOSE);
    APP_INFO1("\t%d  => AVK Start steam play", APP_AVK_MENU_PLAY_START);
    APP_INFO1("\t%d  => AVK Stop stream play", APP_AVK_MENU_PLAY_STOP);
    APP_INFO1("\t%d  => AVK Pause stream play", APP_AVK_MENU_PLAY_PAUSE);
    APP_INFO1("\t%d  => AVK Play next track", APP_AVK_MENU_PLAY_NEXT_TRACK);
    APP_INFO1("\t%d  => AVK Play previous track", APP_AVK_MENU_PLAY_PREVIOUS_TRACK);
    APP_INFO1("\t%d  => AVRC Command", APP_AVK_MENU_RC_CMD);
    APP_INFO1("\t%d  => AVK Send Get Element Attributes Command", APP_AVK_MENU_GET_ELEMENT_ATTR);
    APP_INFO1("\t%d  => AVK Get capabilities", APP_AVK_MENU_GET_CAPABILITIES);
    APP_INFO1("\t%d  => AVK Register Notification", APP_AVK_MENU_REGISTER_NOTIFICATION);
    APP_INFO1("\t%d  => AVK Send Delay Report", APP_AVK_MENU_SEND_DELAY_RPT);
    APP_INFO1("\t%d  => Quit", APP_AVK_MENU_QUIT);
}


/*******************************************************************************
 **
 ** Function         app_avk_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          TRUE if the event was completely handle, FALSE otherwise
 **
 *******************************************************************************/
static BOOLEAN app_avk_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
            app_avk_init(NULL);
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
 ** Description      This is the main function to connect to AVK. It is assumed that an other process handle security.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    int choice, avrcp_evt;
    int connection_index;
    UINT16 delay;
    tAPP_AVK_CONNECTION *connection = NULL;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_avk_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Init XML state machine */
    app_xml_init();

    app_avk_init(NULL);

    do
    {


        app_avk_display_main_menu();

        choice = app_get_choice("Select action");

        switch (choice)
        {

        case APP_AVK_MENU_DISCOVERY:
            /* Example to perform Device discovery (in blocking mode) */
            app_disc_start_regular(NULL);
            break;

        case APP_AVK_MENU_REGISTER:
            /* Example to register AVK connection end point*/
            app_avk_register();
            break;
        case APP_AVK_MENU_DEREGISTER:
            /* Example to de-register an AVK end point */
            app_avk_deregister();
            break;
        case APP_AVK_MENU_OPEN:
            /* Example to Open AVK connection (connect device)*/
            app_avk_open();
            break;

        case APP_AVK_MENU_CLOSE:
            /* Example to Close AVK connection (disconnect device)*/
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_close(connection->bda_connected);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_PLAY_START:
            /* Example to start stream play */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_play_start(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_PLAY_STOP:
            /* Example to stop stream play */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_play_stop(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;  

        case APP_AVK_MENU_PLAY_PAUSE:
            /* Example to pause stream play */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_play_pause(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_PLAY_NEXT_TRACK:
            /* Example to play next track */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
             app_avk_play_next_track(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_PLAY_PREVIOUS_TRACK:
            /* Example to play previous track */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_play_previous_track(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;  

        case APP_AVK_MENU_RC_CMD:
            /* Example to send AVRC cmd */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_rc_cmd(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;          

        case APP_AVK_MENU_GET_ELEMENT_ATTR:
            /* Example to send Vendor Dependent command */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_send_get_element_attributes_cmd(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_GET_CAPABILITIES:
            /* Example to send Vendor Dependent command */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            app_avk_send_get_capabilities(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_REGISTER_NOTIFICATION:
            /* Example to send Vendor Dependent command */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            {
            printf("Choose Event ID\n");
            printf("    1=play_status 2=track_change 5=play_pos 8=app_set\n");
            printf("    13=volume_changed\n");
            avrcp_evt = app_get_choice("\n");
            app_avk_send_register_notification(avrcp_evt, connection->rc_handle);
            }
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_SEND_DELAY_RPT:
            APP_INFO0("delay value(0.1ms unit)");
            delay = app_get_choice("\n");
            app_avk_send_delay_report(delay);
            break;

        case APP_AVK_MENU_QUIT:
            printf("main: Bye Bye\n");
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_AVK_MENU_QUIT); /* While user don't exit application */

    /* Terminate the profile */
    app_avk_end();


    /* Close BSA before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
