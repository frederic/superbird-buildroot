/*****************************************************************************
 **
 **  Name:           app_ag_main.c
 **
 **  Description:    Bluetooth AG application
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

#include "app_ag.h"

#include "bsa_api.h"

#include "gki.h"
#include "uipc.h"

#include "app_utils.h"
#include "app_mgt.h"
#include "app_xml_param.h"

#include "app_disc.h"

/* ui keypress definition */
enum
{
    APP_AG_KEY_DISC_HS = 1,
    APP_AG_KEY_ENABLE,
    APP_AG_KEY_DISABLE,
    APP_AG_KEY_REGISTER,
    APP_AG_KEY_DEREGISTER,
    APP_AG_KEY_OPEN,
    APP_AG_KEY_CLOSE,
    APP_AG_KEY_OPEN_AUDIO,
    APP_AG_KEY_CLOSE_AUDIO,
    APP_AG_KEY_RECORD,
    APP_AG_KEY_STOP_RECORD,
    APP_AG_KEY_PLAY,
    APP_AG_KEY_STOP_PLAY,
    APP_AG_KEY_GET_SCO_ROUTE,
    APP_AG_KEY_IN_CALL,
    APP_AG_KEY_PICKUP_CALL,
    APP_AG_KEY_HANGUP_CALL,
    APP_AG_KEY_QUIT = 99
};

/*******************************************************************************
 **
 ** Function         app_ag_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_display_main_menu(void)
{
    printf("\nBluetooth Headset/HandsFree Main menu:\n");
    printf("    %d  => Discover Headset\n", APP_AG_KEY_DISC_HS);
    printf("    %d  => Enable \n", APP_AG_KEY_ENABLE);
    printf("    %d  => Disable \n", APP_AG_KEY_DISABLE);
    printf("    %d  => Register \n", APP_AG_KEY_REGISTER);
    printf("    %d  => Deregister \n", APP_AG_KEY_DEREGISTER);
    printf("    %d  => Connect \n", APP_AG_KEY_OPEN);
    printf("    %d  => Disconnect\n", APP_AG_KEY_CLOSE);
    printf("    %d  => Open audio \n", APP_AG_KEY_OPEN_AUDIO);
    printf("    %d  => Close audio \n", APP_AG_KEY_CLOSE_AUDIO);
    printf("    %d  => Record audio file\n", APP_AG_KEY_RECORD);
    printf("    %d  => Stop record audio file\n", APP_AG_KEY_STOP_RECORD);
    printf("    %d  => Play audio file\n", APP_AG_KEY_PLAY);
    printf("    %d  => Stop Playing audio file\n", APP_AG_KEY_STOP_PLAY);
    printf("    %d  => Display SCO route config\n", APP_AG_KEY_GET_SCO_ROUTE);
    printf("    %d  => Indicate incoming call\n", APP_AG_KEY_IN_CALL);
    printf("    %d  => PickUp call\n", APP_AG_KEY_PICKUP_CALL);
    printf("    %d  => HangUp call\n", APP_AG_KEY_HANGUP_CALL);
    printf("    %d  => Quit\n", APP_AG_KEY_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_ag_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_ag_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    int index;

    switch (event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable == FALSE)
        {
            APP_DEBUG0("Bluetooth Stopped");
            if(app_ag_cb.fd_sco_in_file >= 0)
            {
                /* if file was opened in previous test and if still opened, close it now */
                app_ag_close_rec_file();
                app_ag_cb.fd_sco_in_file = -1;
            }

            if(app_ag_cb.fd_sco_out_file >= 0)
            {
                /* if file was opened in previous test and if still opened, close it now */
                close(app_ag_cb.fd_sco_out_file);
                app_ag_cb.fd_sco_out_file = -1;
            }

            for(index=0; index<APP_AG_MAX_NUM_CONN; index++)
            {
                if((app_ag_cb.sco_route == BSA_SCO_ROUTE_HCI) &&
                   (app_ag_cb.uipc_channel[index] != UIPC_CH_ID_BAD))
                {
                    app_ag_cb.uipc_channel[index] = UIPC_CH_ID_BAD;
                }

                app_ag_cb.hndl[index] = BSA_AG_BAD_HANDLE;
            }
            app_ag_cb.voice_opened = FALSE;
            app_ag_cb.opened = FALSE;
        }
        else
        {
            APP_DEBUG0("Bluetooth restarted => enable and register application");
            app_ag_start(app_ag_cb.sco_route);
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
    if (app_mgt_open(NULL, app_ag_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }
    app_ag_init();
    app_ag_start(BSA_SCO_ROUTE_DEFAULT);

    do
    {
        app_ag_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
        case APP_AG_KEY_DISC_HS:
            /* Look for audio Devices (Headset/HandsFree) */
            app_disc_start_cod(BTM_COD_SERVICE_AUDIO, BTM_COD_MAJOR_AUDIO, 0, NULL);
            break;

        case APP_AG_KEY_ENABLE:
            app_ag_enable();
            break;

        case APP_AG_KEY_DISABLE:
            app_ag_disable();
            break;

        case APP_AG_KEY_REGISTER:
            app_ag_register(BSA_SCO_ROUTE_DEFAULT);
            break;

        case APP_AG_KEY_DEREGISTER:
            app_ag_deregister();
            break;

        case APP_AG_KEY_OPEN:
            app_ag_open(NULL);
            break;

        case APP_AG_KEY_CLOSE:
            app_ag_close();
            break;

        case APP_AG_KEY_OPEN_AUDIO:
            app_ag_open_audio();
            break;

        case APP_AG_KEY_CLOSE_AUDIO:
            app_ag_close_audio();
            break;

        case APP_AG_KEY_RECORD:
            /* example to record SCO IN channel to a file */
            app_ag_open_rec_file(APP_AG_SCO_IN_SOUND_FILE);
            break;

        case APP_AG_KEY_STOP_RECORD:
            /* example to record SCO IN channel to a file */
            app_ag_close_rec_file();
            break;

        case APP_AG_KEY_PLAY:
            /* example to play a file on SCO OUT channel */
            status = app_ag_play_file();
            break;

        case APP_AG_KEY_STOP_PLAY:
            status = app_ag_stop_play_file();
            break;

        case APP_AG_KEY_GET_SCO_ROUTE:
            printf("SCO route configuration value meaning:\n");
            printf("\t%d  => BSA_SCO_ROUTE_PCM\n", BSA_SCO_ROUTE_PCM);
            printf("\t%d  => BSA_SCO_ROUTE_HCI\n", BSA_SCO_ROUTE_HCI);
            printf("Configured SCO route in AG: %d\n", app_ag_cb.sco_route);
            break;

        case APP_AG_KEY_IN_CALL:
            if (app_ag_cb.opened)
            {
                printf("Ringing... (can pickup locally or wait for HF to answer)\n");
                tBSA_AG_RES bsa_ag_res;

                bsa_ag_res.hndl = BTA_AG_HANDLE_ALL;
                bsa_ag_res.result = BTA_AG_IN_CALL_RES;
                /* CLIP string */
                strncpy(bsa_ag_res.data.str, "\"BSA_CallYou\"",sizeof(bsa_ag_res.data.str));
                /* CLIP type: type of address octet in integer format:
                 * 145 when dialing string includes international access code character �+�
                 * otherwise 129
                 * 129 -> "0612345678"
                 * 145 -> "+33612345678"
                 */
                bsa_ag_res.data.num = 145;
                BSA_AgResult(&bsa_ag_res);
            }
            else
            {
                APP_ERROR0("No AG connection active");
            }
            break;

        case APP_AG_KEY_PICKUP_CALL:
            if (app_ag_cb.opened)
            {
                app_ag_pickupcall(app_ag_cb.hndl[0]);
            }
            else
            {
                APP_ERROR0("No AG connection active");
            }
            break;

        case APP_AG_KEY_HANGUP_CALL:
            if (app_ag_cb.opened)
            {
                app_ag_hangupcall(app_ag_cb.hndl[0]);
            }
            else
            {
                APP_ERROR0("No AG connection active");
            }
            break;

        case APP_AG_KEY_QUIT:
            printf("main: Bye Bye\n");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_AG_KEY_QUIT); /* While user don't exit application */

    app_ag_end();
    /*
     * Close BSA before exiting (to release resources)
     */
    app_mgt_close();

    return 0;
}
