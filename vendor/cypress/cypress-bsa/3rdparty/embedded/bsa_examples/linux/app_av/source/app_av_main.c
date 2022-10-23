/*****************************************************************************
 **
 **  Name:           app_av_main.c
 **
 **  Description:    Bluetooth Audio/Video Streaming application
 **
 **  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
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

#include "app_av.h"
#ifdef APP_AV_BCST_INCLUDED
#include "app_av_bcst.h"
#endif
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_xml_param.h"

/* local function */
static void app_av_display_notifications();
static void app_av_display_play_state();

/* Menu items */
enum
{
    APP_AV_MENU_ABORT_DISC = 1,
    APP_AV_MENU_DISCOVERY,
    APP_AV_MENU_DISPLAY_CONNECTIONS,
    APP_AV_MENU_REGISTER,
    APP_AV_MENU_DEREGISTER,
    APP_AV_MENU_OPEN,
    APP_AV_MENU_CLOSE,
    APP_AV_MENU_PLAY_AVK,
    APP_AV_MENU_PLAY_TONE,
    APP_AV_MENU_TOGGLE_TONE,
    APP_AV_MENU_PLAY_FILE,
    APP_AV_MENU_PLAY_LIST,
    APP_AV_MENU_PLAY_MIC,
    APP_AV_MENU_STOP,
    APP_AV_MENU_PAUSE,
    APP_AV_MENU_RESUME,
    APP_AV_MENU_INC_VOL,
    APP_AV_MENU_DEC_VOL,
    APP_AV_MENU_CLOSE_RC,
    APP_AV_MENU_ABSOLUTE_VOL,
    APP_AV_MENU_UIPC_CFG,
    APP_AV_MENU_CHANGE_CP,
    APP_AV_MENU_TEST_SEC_CODEC,
    APP_AV_MENU_SET_TONE_FREQ,
#if (defined(APP_AV_BITRATE_CONTROL_BY_USER) && (APP_AV_BITRATE_CONTROL_BY_USER == TRUE))
    APP_AV_MENU_SET_BUSY_LEVEL,
#endif
#ifdef APP_AV_BCST_INCLUDED
    APP_AV_BCST_MENU_REGISTER = 50,
    APP_AV_BCST_MENU_DEREGISTER,
    APP_AV_BCST_MENU_PLAY_TONE,
    APP_AV_BCST_MENU_PLAY_MIC,
#ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED
    APP_AV_BCST_MENU_PLAY_LOOPDRV,
#endif
    APP_AV_BCST_MENU_PLAY_MM,
    APP_AV_BCST_MENU_STOP,
    APP_AV_BCST_MENU_UIPC_CFG,
#endif
    APP_AV_MENU_SEND_META_RSP_TO_GET_ELEMENT_ATTR,
    APP_AV_MENU_SEND_META_RSP_TO_GET_PLAY_STATUS,
    APP_AV_MENU_SEND_RC_NOTIFICATIONS,
    APP_AV_MENU_CHANGE_AV_FILE,
    APP_AV_MENU_RESET_FOLDER_LOCATION,
    APP_AV_MENU_MAKE_NO_TRACK_SELECTED,
    APP_AV_MENU_QUIT = 99
};



/*******************************************************************************
 **
 ** Function         app_av_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_av_display_main_menu(void)
{
    printf("\nBluetooth AV Main menu\n");
    printf("  AV Point To Point menu:\n");
    printf("    %d => Abort Discovery\n", APP_AV_MENU_ABORT_DISC);
    printf("    %d => Start Discovery\n", APP_AV_MENU_DISCOVERY);
    printf("    %d => Display local source points\n", APP_AV_MENU_DISPLAY_CONNECTIONS);
    printf("    %d => AV Register (Create local source point)\n", APP_AV_MENU_REGISTER);
    printf("    %d => AV DeRegister (Remove local source point)\n", APP_AV_MENU_DEREGISTER);
    printf("    %d => AV Open (Connect)\n", APP_AV_MENU_OPEN);
    printf("    %d => AV Close (Disconnect)\n", APP_AV_MENU_CLOSE);
    printf("    %d => AV Play AVK Stream\n", APP_AV_MENU_PLAY_AVK);
    printf("    %d => AV Play Tone\n", APP_AV_MENU_PLAY_TONE);
    printf("    %d => AV Toggle Tone\n", APP_AV_MENU_TOGGLE_TONE);
    printf("    %d => AV Play File\n", APP_AV_MENU_PLAY_FILE);
    printf("    %d => AV Start Playlist\n", APP_AV_MENU_PLAY_LIST);
    printf("    %d => AV Play Microphone\n", APP_AV_MENU_PLAY_MIC);
    printf("    %d => AV Stop\n", APP_AV_MENU_STOP);
    printf("    %d => AV Pause\n", APP_AV_MENU_PAUSE);
    printf("    %d => AV Resume\n", APP_AV_MENU_RESUME);
    printf("    %d => AV Send RC Command (Inc Volume)\n", APP_AV_MENU_INC_VOL);
    printf("    %d => AV Send RC Command (Dec Volume)\n", APP_AV_MENU_DEC_VOL);
    printf("    %d => AV Close RC\n", APP_AV_MENU_CLOSE_RC);
    printf("    %d => AV Send Absolute Vol RC Command\n", APP_AV_MENU_ABSOLUTE_VOL);
    printf("    %d => AV Configure UIPC\n", APP_AV_MENU_UIPC_CFG);
    printf("    %d => AV Change Content Protection (Currently:%s)\n", APP_AV_MENU_CHANGE_CP,
            app_av_get_current_cp_desc());
    printf("    %d => AV Test SEC codec\n", APP_AV_MENU_TEST_SEC_CODEC);
    printf("    %d => AV Set Tone sampling frequency\n", APP_AV_MENU_SET_TONE_FREQ);
#if (defined(APP_AV_BITRATE_CONTROL_BY_USER) && (APP_AV_BITRATE_CONTROL_BY_USER == TRUE))
    printf("    %d => AV Change busy level(1-5)\n",APP_AV_MENU_SET_BUSY_LEVEL);
#endif
#ifdef APP_AV_BCST_INCLUDED
    printf("  AV Broadcast menu:\n");
    printf("    %d => AV Broadcast Register (Create local stream)\n",
            APP_AV_BCST_MENU_REGISTER);
    printf("    %d => AV Broadcast Deregister (Remove local stream)\n",
            APP_AV_BCST_MENU_DEREGISTER);
    printf("    %d => AV Broadcast Play Tone \n", APP_AV_BCST_MENU_PLAY_TONE);
    printf("    %d => AV Broadcast Play Microphone \n", APP_AV_BCST_MENU_PLAY_MIC);
#ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED
    printf("    %d => AV Broadcast Play Loopback driver\n", APP_AV_BCST_MENU_PLAY_LOOPDRV);
#endif
    printf("    %d => AV Broadcast Play Kernel Audio (Tone, ALSA PCM)\n",
            APP_AV_BCST_MENU_PLAY_MM);
    printf("    %d => AV Broadcast Stop\n", APP_AV_BCST_MENU_STOP);
    printf("    %d => AV Broadcast Configure UIPC\n", APP_AV_BCST_MENU_UIPC_CFG);
#endif
    printf("    %d => AV Send Meta Response to Get Element Attributes Command\n",
            APP_AV_MENU_SEND_META_RSP_TO_GET_ELEMENT_ATTR);
    printf("    %d => AV Send Meta Response to Get Play Status Command\n",
            APP_AV_MENU_SEND_META_RSP_TO_GET_PLAY_STATUS);
    printf("    %d => AV Send Metadata Change Notifications\n",
            APP_AV_MENU_SEND_RC_NOTIFICATIONS);
    printf("    %d => Change AV file for PTS test\n",
            APP_AV_MENU_CHANGE_AV_FILE);
    printf("    %d => Reset folder location for PTS test\n",
            APP_AV_MENU_RESET_FOLDER_LOCATION);
    printf("    %d => Make no track selected\n", APP_AV_MENU_MAKE_NO_TRACK_SELECTED);
    printf("    %d => Quit\n", APP_AV_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_av_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          TRUE if the event was completely handle, FALSE otherwise
 **
 *******************************************************************************/
static BOOLEAN app_av_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    UINT8 cnt;

    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
            app_av_init(FALSE);

            for (cnt = 0; cnt < APP_AV_MAX_CONNECTIONS; cnt++)
            {
                app_av_register();
            }
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
 ** Description      This is the main function to connect to AV. It is assumed
 **                  that an other process handle security.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    int choice, volume;
    UINT16 tone_sampling_freq = 48000;
    tBSA_AVK_RELAY_AUDIO relay_param;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_av_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Init XML state machine */
    app_xml_init();

    app_av_init(TRUE);

    for (choice = 0; choice < APP_AV_MAX_CONNECTIONS; choice++)
    {
        app_av_register();
    }

    do
    {
        app_av_display_main_menu();

        choice = app_get_choice("Select action");

        switch (choice)
        {
        /*
         * AV Point To point Menu
         */
        case APP_AV_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_AV_MENU_DISCOVERY:
            /* Example to perform Device discovery (in blocking mode) */
            app_disc_start_regular(NULL);
            break;

        case APP_AV_MENU_DISPLAY_CONNECTIONS:
            app_av_display_connections();
            break;

        case APP_AV_MENU_REGISTER:
            /* Example to Register an AV stream */
            app_av_register();
            break;

        case APP_AV_MENU_DEREGISTER:
            app_av_display_connections();
            choice = app_get_choice("Enter the index of the connection to deregister");
            /* Example to DeRegister an AV stream */
            app_av_deregister(choice);
            break;

        case APP_AV_MENU_OPEN:
            /* Example to Open AV connection (connect device) */
            app_av_open(NULL);
            break;

        case APP_AV_MENU_CLOSE:
            /* Example to Close AV connection (disconnect device) */
            app_av_close();
            break;

        case APP_AV_MENU_SET_TONE_FREQ:
            /* Change the tone sampling frequency */
            choice = app_get_choice("Enter new sampling frequency (0 to toggle)");
            if (choice > 0)
            {
                tone_sampling_freq = (UINT16) choice;
            }
            else
            {
                if (tone_sampling_freq == 48000)
                {
                    tone_sampling_freq = 44100;
                }
                else
                {
                    tone_sampling_freq = 48000;
                }
            }
            app_av_set_tone_sample_frequency(tone_sampling_freq);
            break;

#if (defined(BSA_AVK_AV_AUDIO_RELAY) && (BSA_AVK_AV_AUDIO_RELAY==TRUE))
        case APP_AV_MENU_PLAY_AVK:

            APP_DEBUG0("This option relays incoming audio stream from AVK conn to AV conn.");
            APP_DEBUG0("Please make sure that app_avk is running and has an existing AVK connection first");
            APP_DEBUG0("or use BSA Qt Sample Application");

            BSA_AvkRelayAudioInit(&relay_param);
            relay_param.audio_relay = TRUE;
            BSA_AvkRelayAudio(&relay_param);

            app_av_play_from_avk();

            break;
#endif

        case APP_AV_MENU_PLAY_TONE:

#if (defined(BSA_AVK_AV_AUDIO_RELAY) && (BSA_AVK_AV_AUDIO_RELAY==TRUE))

            BSA_AvkRelayAudioInit(&relay_param);
            relay_param.audio_relay = FALSE;
            BSA_AvkRelayAudio(&relay_param);
#endif
            /* Example to Play a tone */
            app_av_play_tone();
            break;

        case APP_AV_MENU_TOGGLE_TONE:
            /* Synchronization */
            app_av_toggle_tone();
            break;

        case APP_AV_MENU_PLAY_FILE:
            /* Example to Play a file */
            app_av_play_file();
            break;

        case APP_AV_MENU_PLAY_LIST:
            /* Example to start to play a playlist */
            app_av_play_playlist(APP_AV_START);
            break;

        case APP_AV_MENU_PLAY_MIC:
            /* Example to start to play a microphone input */
            app_av_play_mic();
            break;

        case APP_AV_MENU_STOP:
            app_av_stop();
            break;

        case APP_AV_MENU_PAUSE:
            app_av_pause();
            break;

        case APP_AV_MENU_RESUME:
            app_av_resume();
            break;

        case APP_AV_MENU_INC_VOL:
            app_av_display_connections();
            choice = app_get_choice("Enter index of the connection to send command to");
            app_av_rc_send_command(choice, BSA_AV_RC_VOL_UP);
            break;

        case APP_AV_MENU_DEC_VOL:
            app_av_display_connections();
            choice = app_get_choice("Enter index of the connection to send command to");
            app_av_rc_send_command(choice, BSA_AV_RC_VOL_DOWN);
            break;

        case APP_AV_MENU_CLOSE_RC:
            app_av_display_connections();
            choice = app_get_choice("Enter the connection index to send command to");
            app_av_rc_close(choice);
            break;

        case APP_AV_MENU_ABSOLUTE_VOL:
            app_av_display_connections();
            choice = app_get_choice("Enter index of the connection to send command to");
            volume = app_get_choice("Enter absolute volume");
            app_av_rc_send_absolute_volume_vd_command(choice, volume);
            break;

        case APP_AV_MENU_UIPC_CFG:
            app_av_ask_uipc_config();
            break;

        case APP_AV_MENU_CHANGE_CP:
            app_av_change_cp();
            break;

        case APP_AV_MENU_TEST_SEC_CODEC:
            /* Test SEC codec */
            app_av_test_sec_codec(FALSE);
            break;

#if (defined(APP_AV_BITRATE_CONTROL_BY_USER) && (APP_AV_BITRATE_CONTROL_BY_USER == TRUE))
        case APP_AV_MENU_SET_BUSY_LEVEL:
            choice = app_get_choice("Select busy level(1~5)");
            app_av_set_busy_level(choice);
            break;
#endif

        /*
         * AV Broadcast Menu
         */
#ifdef APP_AV_BCST_INCLUDED
        case APP_AV_BCST_MENU_REGISTER:
            app_av_bcst_register();
            break;

        case APP_AV_BCST_MENU_DEREGISTER:
            app_av_bcst_deregister();
            break;

        case APP_AV_BCST_MENU_PLAY_TONE:
            app_av_bcst_play_tone();
            break;

        case APP_AV_BCST_MENU_PLAY_MIC:
            app_av_bcst_play_mic();
            break;

#ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED
        case APP_AV_BCST_MENU_PLAY_LOOPDRV:
            app_av_bcst_play_loopback_input();
            break;
#endif
        case APP_AV_BCST_MENU_PLAY_MM:
            app_av_bcst_play_mm();
            break;

        case APP_AV_BCST_MENU_STOP:
            app_av_bcst_stop();
            break;

        case APP_AV_BCST_MENU_UIPC_CFG:
            app_av_bcst_ask_uipc_config();
            break;
#endif /* APP_AV_BCST_INCLUDED */

        case APP_AV_MENU_SEND_META_RSP_TO_GET_ELEMENT_ATTR:
            app_av_display_connections();
            choice = app_get_choice("Enter index of the connection to send command to");
            app_av_rc_send_get_element_attributes_meta_response(choice);
            break;

        case APP_AV_MENU_SEND_META_RSP_TO_GET_PLAY_STATUS:
            app_av_display_connections();
            choice = app_get_choice("Enter index of the connection to send command to");
            app_av_rc_send_play_status_meta_response(choice);
            break;

        case APP_AV_MENU_SEND_RC_NOTIFICATIONS:
            app_av_display_notifications();
            choice = app_get_choice("Enter type of notification to send");
            switch(choice)
            {
            case BSA_AVRC_EVT_PLAY_STATUS_CHANGE:
                app_av_display_play_state();
                choice = app_get_choice("Enter type of play state change");
                app_av_rc_change_play_status(choice);
                break;

            case BSA_AVRC_EVT_TRACK_CHANGE:
                app_av_rc_change_track();
                break;

           case BSA_AVRC_EVT_ADDR_PLAYER_CHANGE:
                choice = app_get_choice("Enter addressed player ID (1, 2 or 3)");
                app_av_rc_addressed_player_change(choice);
                break;

            case BSA_AVRC_EVT_APP_SETTING_CHANGE:
                choice = app_get_choice("Change setting type - 1:equilizer or 2:repeat or 3:shuffle");
                int val = app_get_choice("Enter setting value -1 or 2");
                app_av_rc_settings_change(choice, val);
                break;

            case BSA_AVRC_EVT_TRACK_REACHED_END:
            case BSA_AVRC_EVT_TRACK_REACHED_START:
            case BSA_AVRC_EVT_PLAY_POS_CHANGED:
            case BSA_AVRC_EVT_BATTERY_STATUS_CHANGE:
            case BSA_AVRC_EVT_SYSTEM_STATUS_CHANGE:
            case BSA_AVRC_EVT_NOW_PLAYING_CHANGE:
            case BSA_AVRC_EVT_AVAL_PLAYERS_CHANGE:
            case BSA_AVRC_EVT_UIDS_CHANGE:
            case BSA_AVRC_EVT_VOLUME_CHANGE:
                app_av_rc_complete_notification(choice);
                break;

            default:
                APP_ERROR1("Unknown choice:%d", choice);
                break;
            }

            break;

        case APP_AV_MENU_CHANGE_AV_FILE:
            app_av_change_song(TRUE);
            break;

        case APP_AV_MENU_RESET_FOLDER_LOCATION:
            app_av_reset_folder_info();
            break;

        case APP_AV_MENU_MAKE_NO_TRACK_SELECTED:
            app_av_make_no_track_selected();
            break;

        case APP_AV_MENU_QUIT:
            printf("main: Bye Bye\n");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_AV_MENU_QUIT); /* While user don't exit application */

    /* Terminate the profile */
    app_av_end();

    /* Close the BSA connection */
    app_mgt_close();

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_display_notifications
 **
 ** Description      Display choices for AVRC notifications that can be sent to AVRCP controller
 **
 **
 *******************************************************************************/
static void app_av_display_notifications()
{
    APP_INFO1("    BSA_AVRC_EVT_PLAY_STATUS_CHANGE: %d",   BSA_AVRC_EVT_PLAY_STATUS_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_TRACK_CHANGE: %d",         BSA_AVRC_EVT_TRACK_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_TRACK_REACHED_END: %d",    BSA_AVRC_EVT_TRACK_REACHED_END);
    APP_INFO1("    BSA_AVRC_EVT_TRACK_REACHED_START: %d",  BSA_AVRC_EVT_TRACK_REACHED_START);
    APP_INFO1("    BSA_AVRC_EVT_BATTERY_STATUS_CHANGE: %d",BSA_AVRC_EVT_BATTERY_STATUS_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_SYSTEM_STATUS_CHANGE: %d", BSA_AVRC_EVT_SYSTEM_STATUS_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_APP_SETTING_CHANGE: %d",   BSA_AVRC_EVT_APP_SETTING_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_NOW_PLAYING_CHANGE: %d",   BSA_AVRC_EVT_NOW_PLAYING_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_AVAL_PLAYERS_CHANGE: %d",  BSA_AVRC_EVT_AVAL_PLAYERS_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_ADDR_PLAYER_CHANGE: %d",   BSA_AVRC_EVT_ADDR_PLAYER_CHANGE);
    APP_INFO1("    BSA_AVRC_EVT_UIDS_CHANGE: %d",          BSA_AVRC_EVT_UIDS_CHANGE);
}

static void app_av_display_play_state()
{
    APP_INFO1("    BSA_AVRC_PLAYSTATE_STOPPED: %d",   BSA_AVRC_PLAYSTATE_STOPPED);
    APP_INFO1("    BSA_AVRC_PLAYSTATE_PLAYING: %d",   BSA_AVRC_PLAYSTATE_PLAYING);
    APP_INFO1("    BSA_AVRC_PLAYSTATE_PAUSED: %d",    BSA_AVRC_PLAYSTATE_PAUSED);
    APP_INFO1("    BSA_AVRC_PLAYSTATE_FWD_SEEK: %d",  BSA_AVRC_PLAYSTATE_FWD_SEEK);
    APP_INFO1("    BSA_AVRC_PLAYSTATE_REV_SEEK: %d",  BSA_AVRC_PLAYSTATE_REV_SEEK);
}
