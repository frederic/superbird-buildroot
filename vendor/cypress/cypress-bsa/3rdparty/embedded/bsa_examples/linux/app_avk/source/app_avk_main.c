/*****************************************************************************
 **
 **  Name:           app_avk_main.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 **  Copyright (C) 2017 Cypress Semiconductor Corporation
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
    APP_AVK_MENU_RC_OPEN,
    APP_AVK_MENU_RC_CLOSE,
    APP_AVK_MENU_RC_CMD,
    APP_AVK_MENU_GET_ELEMENT_ATTR,
    APP_AVK_MENU_GET_CAPABILITIES,
    APP_AVK_MENU_REGISTER_NOTIFICATION,
    APP_AVK_MENU_REGISTER_NOTIFICATION_RESPONSE,
    APP_AVK_MENU_SEND_DELAY_RPT,
    APP_AVK_MENU_SEND_ABORT_REQ,
    APP_AVK_MENU_GET_PLAY_STATUS,
    APP_AVK_MENU_GET_ELEMENT_ATTRIBUTES,
    APP_AVK_MENU_SET_BROWSED_PLAYER,
    APP_AVK_MENU_SET_ADDRESSED_PLAYER,
    APP_AVK_MENU_GET_FOLDER_ITEMS,
    APP_AVK_MENU_CHANGE_PATH,
    APP_AVK_MENU_GET_ITEM_ATTRIBUTES,
    APP_AVK_MENU_PLAY_ITEM,
    APP_AVK_MENU_ADD_TO_NOW_PLAYING,
    APP_AVK_MENU_LIST_PLAYER_APP_SET_ATTR,
    APP_AVK_MENU_LIST_PLAYER_APP_SET_VALUE,
    APP_AVK_MENU_SET_ABSOLUTE_VOLUME,
    APP_AVK_MENU_SELECT_STREAMING_DEVICE,
#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
    APP_AVK_MENU_GET_COVER_ART_PROPERTY,
    APP_AVK_MENU_GET_COVER_ART_THUMBNAIL,
    APP_AVK_MENU_GET_COVER_ART_IMAGE,
    APP_AVK_MENU_COVERT_ART_ABORT,
#endif
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
    APP_INFO1("\t%d  => AVRC Open", APP_AVK_MENU_RC_OPEN);
    APP_INFO1("\t%d  => AVRC Close", APP_AVK_MENU_RC_CLOSE);
    APP_INFO1("\t%d  => AVRC Command", APP_AVK_MENU_RC_CMD);
    APP_INFO1("\t%d  => AVK Send Get Element Attributes Command", APP_AVK_MENU_GET_ELEMENT_ATTR);
    APP_INFO1("\t%d  => AVK Get capabilities", APP_AVK_MENU_GET_CAPABILITIES);
    APP_INFO1("\t%d  => AVK Register Notification", APP_AVK_MENU_REGISTER_NOTIFICATION);
    APP_INFO1("\t%d  => AVK Register Notification Response", APP_AVK_MENU_REGISTER_NOTIFICATION_RESPONSE);
    APP_INFO1("\t%d  => AVK Send Delay Report", APP_AVK_MENU_SEND_DELAY_RPT);
    APP_INFO1("\t%d  => AVK Send Abort Request", APP_AVK_MENU_SEND_ABORT_REQ);
    APP_INFO1("\t%d  => AVK Send Get Play Status Command", APP_AVK_MENU_GET_PLAY_STATUS);
    APP_INFO1("\t%d  => AVK Send Get Element Attributes Command", APP_AVK_MENU_GET_ELEMENT_ATTRIBUTES);
    APP_INFO1("\t%d  => AVK Send Set Browsed Player Command", APP_AVK_MENU_SET_BROWSED_PLAYER);
    APP_INFO1("\t%d  => AVK Send Set Addressed Player Command", APP_AVK_MENU_SET_ADDRESSED_PLAYER);
    APP_INFO1("\t%d  => AVK Send Get Folder Items Command", APP_AVK_MENU_GET_FOLDER_ITEMS);
    APP_INFO1("\t%d  => AVK Send Change Path Command", APP_AVK_MENU_CHANGE_PATH);
    APP_INFO1("\t%d  => AVK Send Get Item Attribute Command", APP_AVK_MENU_GET_ITEM_ATTRIBUTES);
    APP_INFO1("\t%d  => AVK Send Play Item Command", APP_AVK_MENU_PLAY_ITEM);
    APP_INFO1("\t%d  => AVK Send Add to Now Playing Command", APP_AVK_MENU_ADD_TO_NOW_PLAYING);
    APP_INFO1("\t%d  => AVK Send List Player application setting Attributes Command",
               APP_AVK_MENU_LIST_PLAYER_APP_SET_ATTR);
    APP_INFO1("\t%d  => AVK Send List Player application setting Values Command",
               APP_AVK_MENU_LIST_PLAYER_APP_SET_VALUE);
    APP_INFO1("\t%d  => AVK Send Absolute Volume Command", APP_AVK_MENU_SET_ABSOLUTE_VOLUME);
    APP_INFO1("\t%d  => AVK Select Streaming Device", APP_AVK_MENU_SELECT_STREAMING_DEVICE);
#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
    APP_INFO1("\t%d  => AVK Get Cover Art Property", APP_AVK_MENU_GET_COVER_ART_PROPERTY);
    APP_INFO1("\t%d  => AVK Get Cover Art Thumbnail", APP_AVK_MENU_GET_COVER_ART_THUMBNAIL);
    APP_INFO1("\t%d  => AVK Get Cover Art Image", APP_AVK_MENU_GET_COVER_ART_IMAGE);
    APP_INFO1("\t%d  => AVK Cover Art Abort", APP_AVK_MENU_COVERT_ART_ABORT);
#endif
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
    int connection_index, i;
    UINT32 first_param, second_param, third_param;
    tAVRC_UID uid;
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
    app_avk_register();

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

        case APP_AVK_MENU_RC_OPEN:
            /* Example to Open RC connection (connect device)*/
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection && connection->is_open && !connection->is_rc_open)
                app_avk_open_rc(connection->bda_connected);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_RC_CLOSE:
            /* Example to Close RC connection (disconnect device)*/
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection && connection->is_rc_open)
                app_avk_close_rc(connection->rc_handle);
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

        case APP_AVK_MENU_REGISTER_NOTIFICATION_RESPONSE:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            {
                printf("Choose Event ID\n");
                printf("    13=volume_changed\n");
                avrcp_evt = app_get_choice("\n");
                printf("Choose Volume\n");
                first_param = app_get_choice("\n");
                printf("Choose label\n");
                second_param = app_get_choice("\n");
                app_avk_reg_notfn_rsp(first_param, connection->rc_handle,
                                second_param, avrcp_evt, BSA_AVK_RSP_CHANGED);
            }
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

        case APP_AVK_MENU_SEND_DELAY_RPT:
            /* Example to send delay report */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            {
                APP_INFO0("delay value(0.1ms unit)");
                delay = app_get_choice("\n");
                app_avk_send_delay_report(connection->bda_connected, delay);
            }
            break;

        case APP_AVK_MENU_SEND_ABORT_REQ:
            /* Example to send an abort request */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
                app_avk_send_abort_req(connection->bda_connected);
            break;

        case APP_AVK_MENU_GET_PLAY_STATUS:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            app_avk_rc_get_play_status_command(connection->rc_handle);
            break;

        case APP_AVK_MENU_GET_ELEMENT_ATTRIBUTES:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            app_avk_rc_get_element_attr_command(connection->rc_handle);
            break;

        case APP_AVK_MENU_SET_BROWSED_PLAYER:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose player_id\n");
            first_param = app_get_choice("\n");
            app_avk_rc_set_browsed_player_command(first_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_SET_ADDRESSED_PLAYER:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose player_id\n");
            first_param = app_get_choice("\n");
            app_avk_rc_set_addressed_player_command(first_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_GET_FOLDER_ITEMS:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose scope(0:Player list, 1:File system, 2:Search, 3:Now playing)\n");
            first_param = app_get_choice("\n");
            printf("Choose start item\n");
            second_param = app_get_choice("\n");
            printf("Choose end item\n");
            third_param = app_get_choice("\n");
            app_avk_rc_get_folder_items(first_param, second_param, third_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_CHANGE_PATH:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose uid_counter\n");
            first_param = app_get_choice("\n");
            printf("Choose direction(0:Up, 1:Down)\n");
            second_param = app_get_choice("\n");
            for (i = 0 ; i < AVRC_UID_SIZE ; i++)
            {
                printf("%d byte of UID\n", i+1);
                uid[i] = app_get_choice("\n");
            }
            app_avk_rc_change_path_command(first_param, second_param, uid, connection->rc_handle);
            break;

        case APP_AVK_MENU_GET_ITEM_ATTRIBUTES:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose scope(0:Player list, 1:File system, 2:Search, 3:Now playing)\n");
            first_param = app_get_choice("\n");
            for (i = 0 ; i < AVRC_UID_SIZE ; i++)
            {
                printf("%d byte of UID\n", i+1);
                uid[i] = app_get_choice("\n");
            }
            printf("Choose uid_counter\n");
            third_param = app_get_choice("\n");
            app_avk_rc_get_items_attr(first_param, uid, third_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_PLAY_ITEM:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose scope(0:Player list, 1:File system, 2:Search, 3:Now playing)\n");
            first_param = app_get_choice("\n");
            for (i = 0 ; i < AVRC_UID_SIZE ; i++)
            {
                printf("%d byte of UID\n", i+1);
                uid[i] = app_get_choice("\n");
            }
            printf("Choose uid_counter\n");
            third_param = app_get_choice("\n");
            app_avk_rc_play_item(first_param, uid, third_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_ADD_TO_NOW_PLAYING:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose scope(0:Player list, 1:File system, 2:Search, 3:Now playing)\n");
            first_param = app_get_choice("\n");
            for (i = 0 ; i < AVRC_UID_SIZE ; i++)
            {
                printf("%d byte of UID\n", i+1);
                uid[i] = app_get_choice("\n");
            }
            printf("Choose uid_counter\n");
            third_param = app_get_choice("\n");
            app_avk_rc_add_to_play(first_param, uid, third_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_LIST_PLAYER_APP_SET_ATTR:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            app_avk_rc_list_player_attr_command(connection->rc_handle);
            break;

        case APP_AVK_MENU_LIST_PLAYER_APP_SET_VALUE:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose attr\n");
            first_param = app_get_choice("\n");
            app_avk_rc_list_player_attr_value_command(first_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_SET_ABSOLUTE_VOLUME:
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            printf("Choose volume(0 ~ 127)\n");
            first_param = app_get_choice("\n");
            app_avk_send_set_absolute_volume_cmd(first_param, connection->rc_handle);
            break;

        case APP_AVK_MENU_SELECT_STREAMING_DEVICE:
            /* Example to select streaming device */
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
                app_avk_select_streaming_device(connection->bda_connected);
            else
                printf("Unknown choice:%d\n", connection_index);
            break;

#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
        case APP_AVK_MENU_GET_COVER_ART_PROPERTY:
        {
            char input[10];
            tBSA_AVK_RC_CAC_IMG_HDL img_handle;
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            {
                memset(input, 0, sizeof(input));
                if(app_get_string("Enter image handle(7 Digit): ", input, sizeof(input)) == 7)
                {
                    memcpy(img_handle, input, sizeof(tBSA_AVK_RC_CAC_IMG_HDL));
                    app_avk_cac_get_prop(img_handle, connection->rc_handle);
                }
                else
                    printf("Invalid Value: %s\n", input);
            }
            else
                printf("Unknown choice:%d\n", connection_index);
        }
        break;

        case APP_AVK_MENU_GET_COVER_ART_THUMBNAIL:
        {
            char input[10];
            char *path = "./thumb.jpg";
            tBSA_AVK_RC_CAC_IMG_HDL img_handle;
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            {
                memset(input, 0, sizeof(input));
                if(app_get_string("Enter image handle(7 Digit): ", input, sizeof(input)) == 7)
                {
                    memcpy(img_handle, input, sizeof(tBSA_AVK_RC_CAC_IMG_HDL));
                    app_avk_cac_get_thumb(img_handle, strlen(path), (UINT8*)path, connection->rc_handle);
                }
                else
                    printf("Invalid Value: %s\n", input);
            }
            else
                printf("Unknown choice:%d\n", connection_index);
        }
        break;

        case APP_AVK_MENU_GET_COVER_ART_IMAGE:
        {
            char input[10];
            char *path = "./image.jpg";
            char *encoding = "JPEG";
            tBSA_AVK_RC_CAC_IMG_HDL img_handle;
            tBSA_AVK_RC_CAC_IMAGE_DESC image_desc;
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
            {
                memset(input, 0, sizeof(input));
                if(app_get_string("Enter image handle(7 Digit): ", input, sizeof(input)) == 7)
                {
                    memcpy(img_handle, input, sizeof(tBSA_AVK_RC_CAC_IMG_HDL));
                    memset(image_desc.encoding, 0, sizeof(tBIP_IMG_ENC_STR));
                    memcpy(image_desc.encoding, encoding, strlen(encoding));
                    image_desc.size = 0;
                    image_desc.transform = BIP_TRANS_STRETCH;
                    /* if all of the pixel values are set to zero, */
                    /* remote will return the native version of the image */
                    printf("Enter pixel size width\n");
                    image_desc.pixel.w = app_get_choice("\n");
                    printf("Enter pixel size height\n");
                    image_desc.pixel.h = app_get_choice("\n");
                    printf("Enter pixel range width\n");
                    image_desc.pixel.w2= app_get_choice("\n");
                    printf("Enter pixel range height\n");
                    image_desc.pixel.h2 = app_get_choice("\n");
                    app_avk_cac_get_img(img_handle, &image_desc, strlen(path),
                        (UINT8*)path, connection->rc_handle);
                }
                else
                    printf("Invalid Value: %s\n", input);
            }
            else
                printf("Unknown choice:%d\n", connection_index);
        }
        break;

        case APP_AVK_MENU_COVERT_ART_ABORT:
        {
            printf("Choose connection index\n");
            app_avk_display_connections();
            connection_index = app_get_choice("\n");
            connection = app_avk_find_connection_by_index(connection_index);
            if(connection)
                app_avk_cac_abort(connection->rc_handle);
            else
                printf("Unknown choice:%d\n", connection_index);
        }
        break;
#endif

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
