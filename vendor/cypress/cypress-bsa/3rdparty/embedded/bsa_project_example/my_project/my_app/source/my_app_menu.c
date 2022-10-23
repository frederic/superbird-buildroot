/*****************************************************************************
**
**  Name:           my_app_menu.c
**
**  Description:    Bluetooth Composite Application Menu
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "my_app_menu.h"

/*
 * Definitions
 */

/* Main menu items */
enum
{
    APP_MGR_MENU = 1,
    APP_HH_MENU,
    APP_HD_MENU,
    APP_AV_MENU,
    APP_AVK_MENU,
    APP_HEADLESS_MENU,
    APP_TM_MENU,
    APP_BLE_MENU,
    APP_MENU_QUIT = 99
};

/* Manager Menu items */
enum
{
    APP_MGR_MENU_ABORT_DISC = 1,
    APP_MGR_MENU_DISCOVERY,
    APP_MGR_MENU_BOND,
    APP_MGR_MENU_SVC_DISCOVERY,
    APP_MGR_MENU_STOP_BT,
    APP_MGR_MENU_START_BT,
    APP_MGR_MENU_SET_AFH_CHANNELS,
    APP_MGR_MENU_SET_AUTH_RESPONSE,
    APP_MGR_MENU_READ_CONFIG,
    APP_MGR_MENU_SET_VISIBILITY,
    APP_MGR_MENU_MONITOR_RSSI,
    APP_MGR_MENU_QUIT = 99
};

/* HH Menu items */
enum
{
    APP_HH_MENU_ABORT_DISC = 1,
    APP_HH_MENU_COD_PERIF_DISC,
    APP_HH_MENU_LIMITED_DISC,
    APP_HH_MENU_BLE_DISC,
    APP_HH_MENU_CONNECT_REPORT,
    APP_HH_MENU_CONNECT_BOOT,
    APP_HH_MENU_DISCONNECT,
    APP_HH_MENU_GET_DSCP,
    APP_HH_MENU_DISPLAY_DEVICES,
    APP_HH_MENU_REMOVE_DEV,
    APP_HH_MENU_SEND_CTRL_DATA,
    APP_HH_MENU_SEND_CTRL_INT,
    APP_HH_MENU_QUIT = 99
};

/* AV menu items */
enum
{
    APP_AV_MENU_ABORT_DISC = 1,
    APP_AV_MENU_COD_AV_DISC,
    APP_AV_MENU_OPEN,
    APP_AV_MENU_CLOSE,
    APP_AV_MENU_PLAY_TONE,
    APP_AV_MENU_PLAY_FILE,
    APP_AV_MENU_PLAY_LIST,
    APP_AV_MENU_PLAY_MIC,
    APP_AV_MENU_STOP,
    APP_AV_MENU_PAUSE,
    APP_AV_MENU_RESUME,
    APP_AV_MENU_PAUSE_AND_RESUME,
    APP_AV_MENU_DISPLAY_CONNECTIONS,
    APP_AV_MENU_TOGGLE_TONE,
    APP_AV_MENU_UIPC_CFG,
    APP_AV_MENU_ABSOLUTE_VOL,
    APP_AV_MENU_GET_ELEMENT_ATTR,
    APP_AV_MENU_INC_VOL,
    APP_AV_MENU_DEC_VOL,
    APP_AV_MENU_SET_TONE_FREQ,
    APP_AV_MENU_QUIT = 99
};

/* AVK menu items */
enum
{
    APP_AVK_MENU_DISCOVERY = 1,
    APP_AVK_MENU_ENABLE,
    APP_AVK_MENU_DISABLE,
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
    APP_AVK_MENU_QUIT = 99
};

/* Headless menu items */
enum
{
    APP_HEADLESS_MENU_ADD = 1,
    APP_HEADLESS_MENU_DELETE,
    APP_HEADLESS_MENU_ENABLE,
    APP_HEADLESS_MENU_DISABLE,
    APP_HEADLESS_MENU_SET_HL_SCAN_MODE,
    APP_HEADLESS_MENU_SET_BLE_ADV_DATA,
    APP_HEADLESS_MENU_SET_BLE_ADV_INT,
    APP_HEADLESS_MENU_SET_SCAN_INT,
    APP_HEADLESS_MENU_QUIT = 99
};

/* TM menu items */
enum
{
    APP_TM_MENU_SV_MEM_USAGE = 1,
    APP_TM_MENU_CL_MEM_USAGE,
    APP_TM_MENU_TRACE_CONTROL,
    APP_TM_MENU_READ_VERSION,
    APP_TM_MENU_READ_CHIPID,
    APP_TM_MENU_READ_SERIAL_FLASH_BD_ADDR,
    APP_TM_MENU_VSE_SET_TX_CARRIER,
    APP_TM_MENU_ENB_TST_MODE,
    APP_TM_MENU_DIS_TST_MODE,
    APP_TM_MENU_DISCONNECT,
    APP_TM_MENU_READ_RAWRSSI,
    APP_TM_MENU_QUIT = 99
};

/* BLE Menu items */
enum
{
    APP_DTV_BLE_MENU_ABORT_DISC = 1,
    APP_DTV_BLE_MENU_DISCOVERY,
    APP_DTV_BLE_MENU_REMOVE,
    APP_DTV_BLE_MENU_CONFIG_BLE_BG_CONN,
    APP_DTV_BLE_MENU_CONFIG_BLE_SCAN_PARAM,
    APP_DTV_BLE_MENU_CONFIG_BLE_CONN_PARAM,
    APP_DTV_BLE_MENU_CONFIG_BLE_ADV_PARAM,
    APP_DTV_BLE_MENU_WAKE_ON_BLE,

    APP_DTV_BLECL_MENU_REGISTER,
    APP_DTV_BLECL_MENU_DEREGISTER,
    APP_DTV_BLECL_MENU_OPEN,
    APP_DTV_BLECL_MENU_CLOSE,
    APP_DTV_BLECL_MENU_READ,
    APP_DTV_BLECL_MENU_WRITE,
    APP_DTV_BLECL_MENU_SERVICE_DISC,
    APP_DTV_BLECL_MENU_REG_FOR_NOTI,
    APP_DTV_BLECL_MENU_DEREG_FOR_NOTI,
    APP_DTV_BLECL_MENU_DISPLAY_CLIENT,
    APP_DTV_BLECL_MENU_SEARCH_DEVICE_INFORMATION_SERVICE,
    APP_DTV_BLECL_MENU_READ_MFR_NAME,
    APP_DTV_BLECL_MENU_READ_MODEL_NUMBER,
    APP_DTV_BLECL_MENU_READ_SERIAL_NUMBER,
    APP_DTV_BLECL_MENU_READ_HARDWARE_REVISION,
    APP_DTV_BLECL_MENU_READ_FIRMWARE_REVISION,
    APP_DTV_BLECL_MENU_READ_SOFTWARE_REVISION,
    APP_DTV_BLECL_MENU_READ_SYSTEM_ID,
    APP_DTV_BLECL_MENU_SEARCH_BATTERY_SERVICE,
    APP_DTV_BLECL_MENU_READ_BATTERY_LEVEL,

    APP_DTV_BLESE_MENU_REGISTER,
    APP_DTV_BLESE_MENU_DEREGISTER,
    APP_DTV_BLESE_MENU_CREATE_SERVICE,
    APP_DTV_BLESE_MENU_ADD_CHAR,
    APP_DTV_BLESE_MENU_START_SERVICE,
    APP_DTV_BLESE_MENU_CONFIG_BLE_ADV_DATA,
    APP_DTV_BLESE_MENU_DISPLAY_SERVER,

    APP_DTV_BLE_MENU_QUIT = 99
};


/*
 * Global Variables
 */
/*
 * Local functions
 */
int my_app_parse_help(char *optarg);
void my_app_print_usage(char **argv);
int my_app_parse_cmd_line(int argc, char **argv);


/*******************************************************************************
 **
 ** Function         my_app_display_manager_menu
 **
 ** Description      This is the Manager menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_display_manager_menu (void)
{
    APP_INFO0("\nBluetooth Manager Menu:");

    APP_INFO1("\t%d => Abort Discovery", APP_MGR_MENU_ABORT_DISC);
    APP_INFO1("\t%d => Discovery", APP_MGR_MENU_DISCOVERY);
    APP_INFO1("\t%d => Bonding", APP_MGR_MENU_BOND);
    APP_INFO1("\t%d => Service Discovery (all services)", APP_MGR_MENU_SVC_DISCOVERY);
    APP_INFO1("\t%d => Stop Bluetooth", APP_MGR_MENU_STOP_BT);
    APP_INFO1("\t%d => Restart Bluetooth", APP_MGR_MENU_START_BT);
    APP_INFO1("\t%d => Set AFH Configuration", APP_MGR_MENU_SET_AFH_CHANNELS);
    APP_INFO1("\t%d => Set Auth Response policy", APP_MGR_MENU_SET_AUTH_RESPONSE);
    APP_INFO1("\t%d => Read Device Configuration", APP_MGR_MENU_READ_CONFIG);
    APP_INFO1("\t%d => Set Device Visibility", APP_MGR_MENU_SET_VISIBILITY);
    APP_INFO1("\t%d => Periodic RSSI Monitor", APP_MGR_MENU_MONITOR_RSSI);
    APP_INFO1("\t%d => Exit Sub-Menu", APP_MGR_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         my_app_mgr_menu
 **
 ** Description      Handle the Manager menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_manager_menu(void)
{
    int choice, device_index;
    unsigned int afh_ch1,afh_ch2;
    BOOLEAN discoverable, connectable;

    do
    {
        my_app_display_manager_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_MGR_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_MGR_MENU_DISCOVERY:
            /* Example to perform Device discovery */
            app_disc_start_regular(NULL);
            break;

        case APP_MGR_MENU_BOND:
            /* Example to Bound  */
            APP_INFO0("Bluetooth Bond menu:");
            app_disc_display_devices();
            device_index = app_get_choice("Enter device number");

            if ((device_index < APP_DISC_NB_DEVICES) &&
                (app_discovery_cb.devs[device_index].in_use != FALSE))
            {
                app_sec_bond(app_discovery_cb.devs[device_index].device.bd_addr);
            }
            else
            {
                APP_ERROR1("bad device number:%d\n", device_index);
            }
            break;

        case APP_MGR_MENU_SVC_DISCOVERY:
            /* Example to perform Device Services discovery */
            app_disc_start_services(BSA_ALL_SERVICE_MASK);
            break;

        case APP_MGR_MENU_STOP_BT:
            /* Example of function to Stop the Local Bluetooth device */
            app_dm_set_local_bt_config(FALSE);
            break;

        case APP_MGR_MENU_START_BT:
            /* Example of function to Restart the Local Bluetooth device */
            app_dm_set_local_bt_config(TRUE);
            break;

        case APP_MGR_MENU_SET_AFH_CHANNELS:
            /* Choose first and last channels */
            APP_INFO0("    Set First AFH CHannel (79 = complete channel map):");
            afh_ch1 = app_get_choice("Select first channel to block");
            APP_INFO0("    Set Last AFH CHannel:");
            afh_ch2 = app_get_choice("Select last channel to block");
            app_dm_set_channel_map(afh_ch1, afh_ch2);
            break;

        case APP_MGR_MENU_READ_CONFIG:
            /* Read and print the current device configuration */
            app_dm_get_local_bt_config();
            break;

        case APP_MGR_MENU_SET_VISIBILITY:
            /* Choose discoverability*/
            choice = app_get_choice("Set Discoverability (0=Not Discoverable, 1=Discoverable)");
            if(choice > 0)
            {
                discoverable = TRUE;
                APP_INFO0("    Device will be discoverable (inquiry scan enabled)");
            }
            else
            {
                discoverable = FALSE;
                APP_INFO0("    Device will not be discoverable (inquiry scan disabled)");
            }

            /* Choose connectability */
            choice = app_get_choice("Set Connectability (0=Not Connectable, 1=Connectable)");
            if(choice > 0)
            {
                connectable = TRUE;
                APP_INFO0("    Device will be connectable (page scan enabled)");
            }
            else
            {
                connectable = FALSE;
                APP_INFO0("    Device will not be connectable (page scan disabled)");
            }

            app_dm_set_visibility(discoverable, connectable);
            break;

        case APP_MGR_MENU_SET_AUTH_RESPONSE:
            choice = app_get_choice("Set authorization response(0=PERM, 1=TEMP, 2=NOT AUTH):");
            app_sec_set_auth(choice);
            break;

        case APP_MGR_MENU_MONITOR_RSSI:
            choice = app_get_choice("Enable/Disable RSSI Monitoring (1 = enable, 0 = disable)");
            if(choice > 0)
            {
                choice = app_get_choice("Enter RSSI measurement period in seconds");
                APP_INFO0("Enabling RSSI Monitoring");
                app_dm_monitor_rssi(TRUE,choice);
            }
            else
            {
                APP_INFO0("Disabling RSSI Monitoring");
                app_dm_monitor_rssi(FALSE,0);

            }
            break;

        case APP_MGR_MENU_QUIT:
            APP_INFO0("Exit MGR SubMenu");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_MGR_MENU_QUIT); /* While user don't exit sub-menu */
}

#ifdef APP_HH_INCLUDED
/*******************************************************************************
 **
 ** Function         my_app_display_hh_menu
 **
 ** Description      This is the HH menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_display_hh_menu (void)
{
    APP_INFO0("\nBluetooth HID Host menu:");
    APP_INFO1("\t%d => Abort Discovery", APP_HH_MENU_ABORT_DISC);
    APP_INFO1("\t%d => COD Discovery (Peripheral Dev)", APP_HH_MENU_COD_PERIF_DISC);
#ifdef APP_BLE_INCLUDED
    APP_INFO1("\t%d => BLE Discovery", APP_HH_MENU_BLE_DISC);
#endif
    APP_INFO1("\t%d => Limited Discovery (LIAC)", APP_HH_MENU_LIMITED_DISC);
    APP_INFO1("\t%d => HID Connect (Report mode)", APP_HH_MENU_CONNECT_REPORT);
    APP_INFO1("\t%d => HID Disconnect", APP_HH_MENU_DISCONNECT);
    APP_INFO1("\t%d => HID Get DSCP Info", APP_HH_MENU_GET_DSCP);
    APP_INFO1("\t%d => HID Display connected devices", APP_HH_MENU_DISPLAY_DEVICES);
    APP_INFO1("\t%d => Remove HID Device", APP_HH_MENU_REMOVE_DEV);
    APP_INFO1("\t%d => Send Control Data", APP_HH_MENU_SEND_CTRL_DATA);
    APP_INFO1("\t%d => Send Interrupt Data", APP_HH_MENU_SEND_CTRL_INT);
    APP_INFO1("\t%d => Exit Sub-Menu", APP_HH_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         my_app_hh_menu
 **
 ** Description      Handle the HH menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_hh_menu(void)
{
    int choice;

    do
    {
        my_app_display_hh_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_HH_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_HH_MENU_COD_PERIF_DISC:
            /* Example to perform Peripheral Device discovery (in blocking mode) */
            app_disc_start_cod(0, BTM_COD_MAJOR_PERIPHERAL, 0, NULL);
            break;

#ifdef APP_BLE_INCLUDED
        case APP_HH_MENU_BLE_DISC:
            app_disc_start_ble_regular(NULL);
            break;
#endif

        case APP_HH_MENU_LIMITED_DISC:
            app_disc_start_limited();
            break;

        case APP_HH_MENU_CONNECT_REPORT:
            /* Example to connect HID Device in Standard Report mode*/
            app_hh_connect(BSA_HH_PROTO_RPT_MODE);
            break;

        case APP_HH_MENU_CONNECT_BOOT:
            /* Example to connect HID Device in BOOT mode*/
            app_hh_connect(BSA_HH_PROTO_BOOT_MODE);
            break;

        case APP_HH_MENU_DISCONNECT:
            /* Example to disconnect HID Device */
            app_hh_disconnect();
            break;

        case APP_HH_MENU_GET_DSCP:
            /* Example to get DSCP info from HID Device */
            app_hh_get_dscpinfo(BSA_HH_INVALID_HANDLE);
            break;

        case APP_HH_MENU_DISPLAY_DEVICES:
            app_hh_display_status();
            break;

        case APP_HH_MENU_REMOVE_DEV:
            /* Example to remove a device */
            app_hh_remove_dev();
            break;

        case APP_HH_MENU_SEND_CTRL_DATA:
            app_hh_send_ctrl_data();
            break;

        case APP_HH_MENU_SEND_CTRL_INT:
            app_hh_send_int_data();
            break;

        case APP_HH_MENU_QUIT:
            APP_INFO0("Exit HH SubMenu");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_HH_MENU_QUIT); /* While user don't exit sub-menu */
}
#endif /* APP_HH_INCLUDED */

#ifdef APP_AV_INCLUDED
/*******************************************************************************
 **
 ** Function         my_app_display_av_menu
 **
 ** Description      This is the AV  menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_display_av_menu (void)
{
    APP_INFO0("\nBluetooth AV menu");
    APP_INFO0("  AV Point To Point menu:");
    APP_INFO1("\t%d => Abort Discovery", APP_AV_MENU_ABORT_DISC);
    APP_INFO1("\t%d => COD Discovery (Audio Dev)", APP_AV_MENU_COD_AV_DISC);
    APP_INFO1("\t%d => AV Open (Connect)", APP_AV_MENU_OPEN);
    APP_INFO1("\t%d => AV Close (Disconnect)", APP_AV_MENU_CLOSE);
    APP_INFO1("\t%d => AV Play Tone", APP_AV_MENU_PLAY_TONE);
    APP_INFO1("\t%d => AV Play File", APP_AV_MENU_PLAY_FILE);
    APP_INFO1("\t%d => AV Start Playlist", APP_AV_MENU_PLAY_LIST);
    APP_INFO1("\t%d => AV Play Microphone/LineIn", APP_AV_MENU_PLAY_MIC);
    APP_INFO1("\t%d => AV Stop", APP_AV_MENU_STOP);
    APP_INFO1("\t%d => AV Pause", APP_AV_MENU_PAUSE);
    APP_INFO1("\t%d => AV Resume", APP_AV_MENU_RESUME);
    APP_INFO1("\t%d => AV Pause & Resume", APP_AV_MENU_PAUSE_AND_RESUME);
    APP_INFO1("\t%d => Display local source points", APP_AV_MENU_DISPLAY_CONNECTIONS);
    APP_INFO1("\t%d => AV Toggle Tone", APP_AV_MENU_TOGGLE_TONE);
    APP_INFO1("\t%d => AV Configure UIPC", APP_AV_MENU_UIPC_CFG);
    APP_INFO1("\t%d => AV Send Absolute Vol RC Command", APP_AV_MENU_ABSOLUTE_VOL);
    APP_INFO1("\t%d => AV Send Get Element Attributes Command", APP_AV_MENU_GET_ELEMENT_ATTR);
    APP_INFO1("\t%d => AV Send RC Command (Inc Volume)", APP_AV_MENU_INC_VOL);
    APP_INFO1("\t%d => AV Send RC Command (Dec Volume)", APP_AV_MENU_DEC_VOL);
    APP_INFO1("\t%d => AV Set Tone Frequency", APP_AV_MENU_SET_TONE_FREQ);
    APP_INFO1("\t%d => Exit Sub-Menu", APP_AV_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         my_app_av_menu
 **
 ** Description      Handle the HH menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_av_menu(void)
{
    int choice, volume;

    do
    {
        my_app_display_av_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        /*
         * AV Point To point Menu
         */
        case APP_AV_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_AV_MENU_COD_AV_DISC:
            /* Example to perform Audio Device discovery (in blocking mode) */
            app_disc_start_cod(0, BTM_COD_MAJOR_AUDIO , 0, NULL);
            break;

        case APP_AV_MENU_OPEN:
            /* Example to Open AV connection (connect device)*/
            app_av_open(NULL);
            break;

        case APP_AV_MENU_CLOSE:
            /* Example to Close AV connection (disconnect device)*/
            app_av_close();
            break;

        case APP_AV_MENU_PLAY_TONE:
            /* Example to Play a tone */
            app_av_play_tone();
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
            app_av_play_mic();
            break;

        case APP_AV_MENU_STOP:
            /* Example to stop to play */
            app_av_stop();
            break;

        case APP_AV_MENU_PAUSE:
            /* Example to Pause play */
            app_av_pause();
            break;

        case APP_AV_MENU_RESUME:
            /* Example to Resume play */
            app_av_resume();
            break;

        case APP_AV_MENU_PAUSE_AND_RESUME:
            app_av_pause();
            GKI_delay(100);
            app_av_resume();
            break;

        case APP_AV_MENU_DISPLAY_CONNECTIONS:
            app_av_display_connections();
            break;

        case APP_AV_MENU_TOGGLE_TONE:
            /* Synchronization */
            app_av_toggle_tone();
            break;

        case APP_AV_MENU_UIPC_CFG:
            app_av_ask_uipc_config();
            break;

        case APP_AV_MENU_ABSOLUTE_VOL:
            app_av_display_connections();
            choice = app_get_choice("Enter index of the connection to send command to");
            volume = app_get_choice("Enter absolute volume");
            app_av_rc_send_absolute_volume_vd_command(choice, volume);
            break;

        case APP_AV_MENU_GET_ELEMENT_ATTR:
            app_av_display_connections();
            choice = app_get_choice("Enter index of the connection to send command to");
            app_av_rc_send_get_element_attributes_vd_command(choice);
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

        case APP_AV_MENU_SET_TONE_FREQ:
            /* Change the tone frequency */
            choice = app_get_choice("Enter new sampling frequency (44100 or 48000)");
            if (choice == 44100)
            {
                app_av_set_tone_sample_frequency(44100);
            }
            else if (choice == 48000)
            {
                app_av_set_tone_sample_frequency(48000);
            }
            else
            {
                APP_ERROR1("Bad Frequency=%d", choice);
            }
            break;

        case APP_AV_MENU_QUIT:
            APP_INFO0("Exit AV SubMenu");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_AV_MENU_QUIT); /* While user don't exit sub-menu */
}
#endif /* APP_AV_INCLUDED */

#ifdef APP_AVK_INCLUDED
/*******************************************************************************
 **
 ** Function         my_app_display_avk_menu
 **
 ** Description      This is the AVK  menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_display_avk_menu (void)
{
    APP_INFO0("\nBluetooth AVK Main menu:");
    APP_INFO1("\t%d  => Start Discovery", APP_AVK_MENU_DISCOVERY);
    APP_INFO1("\t%d  => AVK Enable", APP_AVK_MENU_ENABLE);
    APP_INFO1("\t%d  => AVK Disable", APP_AVK_MENU_DISABLE);
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
    APP_INFO1("\t%d  => Exit Sub-Menu", APP_AVK_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         my_app_avk_menu
 **
 ** Description      Handle the avk menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_avk_menu(void)
{
    int choice, avrcp_evt;
    tAPP_AVK_CONNECTION *connection = NULL;
    do
    {
        my_app_display_avk_menu();

        choice = app_get_choice("Sub Menu");

       connection =  app_avk_find_connection_by_index(0);

        switch(choice)
        {
        case APP_AVK_MENU_DISCOVERY:
            /* Example to perform Device discovery (in blocking mode) */
            app_disc_start_regular(NULL);
            break;

        case APP_AVK_MENU_ENABLE:
            /* Enable AVK */
            if (app_avk_init(NULL) < 0)
            {
                APP_ERROR0("Unable to start AVK");
            }
            break;

        case APP_AVK_MENU_DISABLE:
            /* Disable AVK */
            app_avk_end();
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
            app_avk_close(connection->bda_connected);
            break;

        case APP_AVK_MENU_PLAY_START:
            /* Example to start stream play */
            app_avk_play_start(connection->rc_handle);
            break;

        case APP_AVK_MENU_PLAY_STOP:
            /* Example to stop stream play */
            app_avk_play_stop(connection->rc_handle);
            break;

        case APP_AVK_MENU_PLAY_PAUSE:
            /* Example to pause stream play */
            app_avk_play_pause(connection->rc_handle);
            break;

        case APP_AVK_MENU_PLAY_NEXT_TRACK:
            /* Example to play next track */
            app_avk_play_next_track(connection->rc_handle);
            break;

        case APP_AVK_MENU_PLAY_PREVIOUS_TRACK:
            /* Example to play previous track */
            app_avk_play_previous_track(connection->rc_handle);
            break;

        case APP_AVK_MENU_RC_CMD:
            /* Example to send AVRC cmd */
            app_avk_rc_cmd(connection->rc_handle);
            break;

        case APP_AVK_MENU_GET_ELEMENT_ATTR:
            /* Example to send Vendor Dependent command */
            app_avk_send_get_element_attributes_cmd(connection->rc_handle);
            break;

        case APP_AVK_MENU_GET_CAPABILITIES:
            /* Example to send Vendor Dependent command */
            app_avk_send_get_capabilities(connection->rc_handle);
            break;

        case APP_AVK_MENU_REGISTER_NOTIFICATION:
            /* Example to send Vendor Dependent command */
            printf("Choose Event ID\n");
            printf("    1=play_status 2=track_change 5=play_pos 8=app_set\n");
            avrcp_evt = app_get_choice("\n");
            app_avk_send_register_notification(avrcp_evt,connection->rc_handle);
            break;
        case APP_AVK_MENU_QUIT:
            APP_INFO0("Exit AVK SubMenu");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_AV_MENU_QUIT); /* While user don't exit sub-menu */
}
#endif /* APP_AVK_INCLUDED */

#ifdef APP_HEADLESS_INCLUDED
/*******************************************************************************
 **
 ** Function         app_display_headless_menu
 **
 ** Description      This is the Headless  menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_display_headless_menu (void)
{
    APP_INFO0("\nBluetooth Headless menu:");
    APP_INFO1("\t%d => Add Headless Device", APP_HEADLESS_MENU_ADD);
    APP_INFO1("\t%d => Remove Headless Device", APP_HEADLESS_MENU_DELETE);
    APP_INFO1("\t%d => Enable Headless Mode", APP_HEADLESS_MENU_ENABLE);
    APP_INFO1("\t%d => Disable Headless Mode", APP_HEADLESS_MENU_DISABLE);
    APP_INFO1("\t%d => Configure Headless Scan Mode", APP_HEADLESS_MENU_SET_HL_SCAN_MODE);
    APP_INFO1("\t%d => Configure BLE Advertising data at Headless mode", APP_HEADLESS_MENU_SET_BLE_ADV_DATA);
    APP_INFO1("\t%d => Configure BLE Adv Interval and enable", APP_HEADLESS_MENU_SET_BLE_ADV_INT);
    APP_INFO1("\t%d => Configure Scan interval at Headless mode", APP_HEADLESS_MENU_SET_SCAN_INT);
    APP_INFO1("\t%d => Exit Sub-Menu", APP_HEADLESS_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         my_app_headless_menu
 **
 ** Description      Handle the headless menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_headless_menu(void)
{
    int choice;

    do
    {
        my_app_display_headless_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_HEADLESS_MENU_ADD:
            app_headless_add();
            break;

        case APP_HEADLESS_MENU_DELETE:
            app_headless_delete();
            break;

        case APP_HEADLESS_MENU_ENABLE:
            app_headless_control(1);
            break;

        case APP_HEADLESS_MENU_DISABLE:
            app_headless_control(0);
            break;

        case APP_HEADLESS_MENU_SET_HL_SCAN_MODE:
            app_headless_scan_control();
            break;

        case APP_HEADLESS_MENU_SET_BLE_ADV_DATA:
            app_headless_set_ble_adv_data();
            break;

        case APP_HEADLESS_MENU_SET_BLE_ADV_INT:
            app_headless_configure_ble_adv();
            break;

        case APP_HEADLESS_MENU_SET_SCAN_INT:
            app_headless_scan_interval_control();
            break;

        case APP_HEADLESS_MENU_QUIT:
            APP_INFO0("Exit Headless SubMenu");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_HEADLESS_MENU_QUIT); /* While user don't exit sub-menu */
}
#endif /* APP_HEADLESS_INCLUDED */

/*******************************************************************************
 **
 ** Function         my_app_display_tm_menu
 **
 ** Description      This is the TM menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_display_tm_menu (void)
{
    APP_INFO0("\nBluetooth Test Module menu:");
    APP_INFO1("\t%d => Get Server Memory Usage", APP_TM_MENU_SV_MEM_USAGE);
    APP_INFO1("\t%d => Get Client Memory Usage", APP_TM_MENU_CL_MEM_USAGE);
    APP_INFO1("\t%d => Control trace level(0 ~ 5)", APP_TM_MENU_TRACE_CONTROL);
    APP_INFO1("\t%d => Read Version", APP_TM_MENU_READ_VERSION);
    APP_INFO1("\t%d => Read Chip ID", APP_TM_MENU_READ_CHIPID);
    APP_INFO1("\t%d => Read BD address from Serial Flash", APP_TM_MENU_READ_SERIAL_FLASH_BD_ADDR);
    APP_INFO1("\t%d => Set Tx Carrier Frequency", APP_TM_MENU_VSE_SET_TX_CARRIER);
    APP_INFO1("\t%d => Enable Test Mode", APP_TM_MENU_ENB_TST_MODE);
    APP_INFO1("\t%d => Disable Test Mode", APP_TM_MENU_DIS_TST_MODE);
    APP_INFO1("\t%d => Disconnect connection", APP_TM_MENU_DISCONNECT);
    APP_INFO1("\t%d => Read Raw RSSI", APP_TM_MENU_READ_RAWRSSI);
    APP_INFO1("\t%d => Exit Sub-Menu", APP_TM_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         my_app_tm_menu
 **
 ** Description      Handle the TM menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_tm_menu(void)
{
    int choice;
    int level;
    BD_ADDR bd_addr;

    do
    {
        my_app_display_tm_menu();

        choice = app_get_choice("Sub Menu");

        switch(choice)
        {
        case APP_TM_MENU_SV_MEM_USAGE:
            app_tm_get_mem_usage(BSA_TM_SERVER);
            break;

        case APP_TM_MENU_CL_MEM_USAGE:
            app_tm_get_mem_usage(BSA_TM_CLIENT);
            break;

        case APP_TM_MENU_TRACE_CONTROL:
            APP_INFO0("Trace level supported:");
            APP_INFO0("\t0: None");
            APP_INFO0("\t1: Error");
            APP_INFO0("\t2: Warning");
            APP_INFO0("\t3: API");
            APP_INFO0("\t4: Event");
            APP_INFO0("\t5: Debug (full)");
            level = app_get_choice("level");
            app_tm_set_trace(level);
            break;

        case APP_TM_MENU_READ_VERSION:
            app_tm_read_version();
            break;

        case APP_TM_MENU_READ_CHIPID:
            app_dm_get_chip_id();
            break;

        case APP_TM_MENU_READ_SERIAL_FLASH_BD_ADDR:
            app_tm_read_serial_flash_bd_addr(bd_addr);
            break;

        case APP_TM_MENU_VSE_SET_TX_CARRIER:
            app_tm_set_tx_carrier_frequency_test();
            break;

        case APP_TM_MENU_ENB_TST_MODE:
            app_tm_set_test_mode(TRUE);
            break;

        case APP_TM_MENU_DIS_TST_MODE:
            app_tm_set_test_mode(FALSE);
            break;

        case APP_TM_MENU_DISCONNECT:
             app_tm_disconnect(NULL);
             break;

        case APP_TM_MENU_READ_RAWRSSI:
             app_tm_vsc_read_raw_rssi(NULL);
             break;

        case APP_TM_MENU_QUIT:
            APP_INFO0("Exit TM SubMenu");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_TM_MENU_QUIT); /* While user don't exit sub-menu */
}

#ifdef APP_BLE_INCLUDED
/*******************************************************************************
 **
 ** Function         my_app_display_ble_menu
 **
 ** Description      This is the BLE menu
 **
 ** Returns          void
 **
 *******************************************************************************/
static void my_app_display_ble_menu (void)
{
    APP_INFO1("\t%d => Abort Discovery", APP_DTV_BLE_MENU_ABORT_DISC);
    APP_INFO1("\t%d => Start BLE Discovery", APP_DTV_BLE_MENU_DISCOVERY);
    APP_INFO1("\t%d => Remove BLE device", APP_DTV_BLE_MENU_REMOVE);
    APP_INFO1("\t%d => Configure BLE Background Connection Type", APP_DTV_BLE_MENU_CONFIG_BLE_BG_CONN);
    APP_INFO1("\t%d => Configure BLE Scan Parameter",APP_DTV_BLE_MENU_CONFIG_BLE_SCAN_PARAM);
    APP_INFO1("\t%d => Configure BLE Connection Parameter",APP_DTV_BLE_MENU_CONFIG_BLE_CONN_PARAM);
    APP_INFO1("\t%d => Configure BLE Advertisement Parameters",APP_DTV_BLE_MENU_CONFIG_BLE_ADV_PARAM);
    APP_INFO1("\t%d => Configure for Wake on BLE",APP_DTV_BLE_MENU_WAKE_ON_BLE);

    APP_INFO0("\t**** Bluetooth Low Energy Client menu ****");
    APP_INFO1("\t%d => Register client app", APP_DTV_BLECL_MENU_REGISTER);
    APP_INFO1("\t%d => Deregister Client app", APP_DTV_BLECL_MENU_DEREGISTER);
    APP_INFO1("\t%d => Connect to Server", APP_DTV_BLECL_MENU_OPEN);
    APP_INFO1("\t%d => Close Connection", APP_DTV_BLECL_MENU_CLOSE);
    APP_INFO1("\t%d => Read information", APP_DTV_BLECL_MENU_READ);
    APP_INFO1("\t%d => Write information", APP_DTV_BLECL_MENU_WRITE);
    APP_INFO1("\t%d => Service Discovery", APP_DTV_BLECL_MENU_SERVICE_DISC);
    APP_INFO1("\t%d => Register Notification", APP_DTV_BLECL_MENU_REG_FOR_NOTI);
    APP_INFO1("\t%d => Deregister Notification", APP_DTV_BLECL_MENU_DEREG_FOR_NOTI);
    APP_INFO1("\t%d => Display Clients", APP_DTV_BLECL_MENU_DISPLAY_CLIENT);
    APP_INFO1("\t%d => Search Device Information Service", APP_DTV_BLECL_MENU_SEARCH_DEVICE_INFORMATION_SERVICE);
    APP_INFO1("\t%d => Read Manufacturer Name", APP_DTV_BLECL_MENU_READ_MFR_NAME);
    APP_INFO1("\t%d => Read Model Number", APP_DTV_BLECL_MENU_READ_MODEL_NUMBER);
    APP_INFO1("\t%d => Read Serial Number", APP_DTV_BLECL_MENU_READ_SERIAL_NUMBER);
    APP_INFO1("\t%d => Read Hardware Revision", APP_DTV_BLECL_MENU_READ_HARDWARE_REVISION);
    APP_INFO1("\t%d => Read Firmware Revision", APP_DTV_BLECL_MENU_READ_FIRMWARE_REVISION);
    APP_INFO1("\t%d => Read Software Revision", APP_DTV_BLECL_MENU_READ_SOFTWARE_REVISION);
    APP_INFO1("\t%d => Read System ID", APP_DTV_BLECL_MENU_READ_SYSTEM_ID);
    APP_INFO1("\t%d => Search Battery Service", APP_DTV_BLECL_MENU_SEARCH_BATTERY_SERVICE);
    APP_INFO1("\t%d => Read Battery Level", APP_DTV_BLECL_MENU_READ_BATTERY_LEVEL);

    APP_INFO0("\t**** Bluetooth Low Energy Server menu ****");
    APP_INFO1("\t%d => Register server app", APP_DTV_BLESE_MENU_REGISTER);
    APP_INFO1("\t%d => Deregister server app", APP_DTV_BLESE_MENU_DEREGISTER);
    APP_INFO1("\t%d => Create service", APP_DTV_BLESE_MENU_CREATE_SERVICE);
    APP_INFO1("\t%d => Add character", APP_DTV_BLESE_MENU_ADD_CHAR);
    APP_INFO1("\t%d => Start service", APP_DTV_BLESE_MENU_START_SERVICE);
    APP_INFO1("\t%d => Configure BLE Advertisement data",APP_DTV_BLESE_MENU_CONFIG_BLE_ADV_DATA);
    APP_INFO1("\t%d => Display Servers", APP_DTV_BLESE_MENU_DISPLAY_SERVER);

    APP_INFO1("\t%d => QUIT", APP_DTV_BLE_MENU_QUIT);
}


/*******************************************************************************
 **
 ** Function         my_app_ble_menu
 **
 ** Description      Handle the BLE menu
 **
 ** Returns          void
 **
 *******************************************************************************/
static void my_app_ble_menu(void)
{
    int choice, type;
    UINT16 ble_scan_interval, ble_scan_window, ble_scan_mode;
    tBSA_DM_BLE_CONN_PARAM conn_param;

    do
    {
        my_app_display_ble_menu();

        choice = app_get_choice("Select action");

        switch(choice)
        {
        case APP_DTV_BLE_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_DTV_BLE_MENU_DISCOVERY:
            app_disc_start_ble_regular(NULL);
            break;

        case APP_DTV_BLE_MENU_REMOVE:
            app_ble_client_unpair();
            break;

        case APP_DTV_BLE_MENU_CONFIG_BLE_BG_CONN:
            type = app_get_choice("Select conn type(0 = None, 1 = Auto)");
            if(type == 0 || type == 1)
            {
                app_dm_set_ble_bg_conn_type(type);
            }
            else
            {
                APP_ERROR1("Unknown type:%d", type);
            }
            break;

        case APP_DTV_BLE_MENU_CONFIG_BLE_SCAN_PARAM:
            ble_scan_interval = app_get_choice("BLE scan interval(N x 625us)");
            ble_scan_window = app_get_choice("BLE scan window(N x 625us)");
            ble_scan_mode = app_get_choice("BLE scan mode(0:Passive, 1:Active)");
            app_dm_set_ble_scan_param(ble_scan_interval, ble_scan_window, ble_scan_mode);
            break;

        case APP_DTV_BLE_MENU_CONFIG_BLE_CONN_PARAM:
            conn_param.min_conn_int = app_get_choice("min_conn_int(N x 1.25 msec)");
            conn_param.max_conn_int = app_get_choice("max_conn_int(N x 1.25 msec)");
            conn_param.slave_latency = app_get_choice("slave_latency");
            conn_param.supervision_tout = app_get_choice("supervision_tout(N x 10 msec)");
            APP_INFO0("Enter the BD address to configure Conn Param (AA.BB.CC.DD.EE.FF): ");
            if (scanf("%hhx.%hhx.%hhx.%hhx.%hhx.%hhx",
                &conn_param.bd_addr[0], &conn_param.bd_addr[1],
                &conn_param.bd_addr[2], &conn_param.bd_addr[3],
                &conn_param.bd_addr[4], &conn_param.bd_addr[5]) != 6)
            {
                APP_ERROR0("BD address not entered correctly");
                break;
            }
            app_dm_set_ble_conn_param(&conn_param);
            break;

        case APP_DTV_BLE_MENU_WAKE_ON_BLE:
            app_ble_wake_configure();
            break;

        case APP_DTV_BLECL_MENU_REGISTER:
            app_ble_client_register(0xFF);
            break;

        case APP_DTV_BLECL_MENU_OPEN:
            app_ble_client_open();
            break;

        case APP_DTV_BLECL_MENU_SERVICE_DISC:
            app_ble_client_service_search();
            break;

        case APP_DTV_BLECL_MENU_READ:
            app_ble_client_read();
            break;

        case APP_DTV_BLECL_MENU_WRITE:
            app_ble_client_write();
            break;

        case APP_DTV_BLECL_MENU_REG_FOR_NOTI:
            app_ble_client_register_notification();
            break;

        case APP_DTV_BLECL_MENU_CLOSE:
            app_ble_client_close();
            break;

        case APP_DTV_BLECL_MENU_DEREGISTER:
            app_ble_client_deregister();
            break;

        case APP_DTV_BLECL_MENU_DEREG_FOR_NOTI:
            app_ble_client_deregister_notification();
            break;

        case APP_DTV_BLECL_MENU_DISPLAY_CLIENT:
            app_ble_client_display(1);
            break;

        case APP_DTV_BLECL_MENU_SEARCH_DEVICE_INFORMATION_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_DEVICE_INFORMATION);
            break;

        case APP_DTV_BLECL_MENU_READ_MFR_NAME:
            app_ble_client_read_mfr_name();
            break;

        case APP_DTV_BLECL_MENU_READ_MODEL_NUMBER:
            app_ble_client_read_model_number();
            break;

        case APP_DTV_BLECL_MENU_READ_SERIAL_NUMBER:
            app_ble_client_read_serial_number();
            break;

        case APP_DTV_BLECL_MENU_READ_HARDWARE_REVISION:
            app_ble_client_read_hardware_revision();
            break;

        case APP_DTV_BLECL_MENU_READ_FIRMWARE_REVISION:
            app_ble_client_read_firmware_revision();
            break;

        case APP_DTV_BLECL_MENU_READ_SOFTWARE_REVISION:
            app_ble_client_read_software_revision();
            break;

        case APP_DTV_BLECL_MENU_READ_SYSTEM_ID:
            app_ble_client_read_system_id();
            break;

        case APP_DTV_BLECL_MENU_SEARCH_BATTERY_SERVICE:
            app_ble_client_service_search_execute(BSA_BLE_UUID_SERVCLASS_BATTERY_SERVICE);
            break;

        case APP_DTV_BLECL_MENU_READ_BATTERY_LEVEL:
            app_ble_client_read_battery_level();
            break;

        case APP_DTV_BLESE_MENU_REGISTER:
            app_ble_server_register(0xFF, NULL);
            break;

        case APP_DTV_BLESE_MENU_DEREGISTER:
            app_ble_server_deregister();
            break;

        case APP_DTV_BLESE_MENU_CREATE_SERVICE:
            app_ble_server_create_service();
            break;

        case APP_DTV_BLESE_MENU_ADD_CHAR:
            app_ble_server_add_char();
            break;

        case APP_DTV_BLESE_MENU_START_SERVICE:
            app_ble_server_start_service();
            break;

        case APP_DTV_BLESE_MENU_DISPLAY_SERVER:
            app_ble_server_display();
            break;

        case APP_DTV_BLE_MENU_QUIT:
            APP_INFO0("Quit");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_DTV_BLE_MENU_QUIT); /* While user don't exit application */
}
#endif


/*******************************************************************************
 **
 ** Function         my_app_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_display_main_menu (void)
{
    APP_INFO0("\nBluetooth Main Menu:");
    APP_INFO1("\t%d => Manager sub-menu", APP_MGR_MENU);
#ifdef APP_HH_INCLUDED
    APP_INFO1("\t%d => HID Host sub-menu", APP_HH_MENU);
#endif
#ifdef APP_AV_INCLUDED
    APP_INFO1("\t%d => AV sub-menu", APP_AV_MENU);
#endif
#ifdef APP_AVK_INCLUDED
    APP_INFO1("\t%d => AV Sink sub-menu", APP_AVK_MENU);
#endif
#ifdef APP_HEADLESS_INCLUDED
    APP_INFO1("\t%d => Headless sub-menu", APP_HEADLESS_MENU);
#endif
    APP_INFO1("\t%d => Test module sub-menu", APP_TM_MENU);
#ifdef APP_BLE_INCLUDED
    APP_INFO1("\t%d => BLE sub-menu", APP_BLE_MENU);
#endif
    APP_INFO1("\t%d => Quit", APP_MENU_QUIT);
};

/*******************************************************************************
 **
 ** Function         my_app_main_menu
 **
 ** Description      Handle the Main menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_main_menu(void)
{
    int choice;

    do
    {
        /* Display the main menu the first time */
        my_app_display_main_menu();

        choice = app_get_choice("Menu");

        switch(choice)
        {
        case APP_MGR_MENU:
            my_app_manager_menu();
            break;

#ifdef APP_HH_INCLUDED
        case APP_HH_MENU:
            my_app_hh_menu();
            break;
#endif

#ifdef APP_AV_INCLUDED
        case APP_AV_MENU:
            my_app_av_menu();
            break;
#endif

#ifdef APP_AVK_INCLUDED
        case APP_AVK_MENU:
            my_app_avk_menu();
            break;
#endif

#ifdef APP_HEADLESS_INCLUDED
        case APP_HEADLESS_MENU:
            my_app_headless_menu();
            break;
#endif

        case APP_TM_MENU:
            my_app_tm_menu();
            break;

#ifdef APP_BLE_INCLUDED
        case APP_BLE_MENU:
            my_app_ble_menu();
            break;
#endif

        case APP_MENU_QUIT:
            APP_INFO0("Bye Bye");
            break;

        default:
            my_app_display_main_menu();
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_MGR_MENU_QUIT); /* While user don't exit application */
}

/*******************************************************************************
 **
 ** Function         my_app_parse_help
 **
 ** Description      This function parse help command
 **
 ** Returns          int
 **
 *******************************************************************************/
int my_app_parse_help(char *optarg)
{
     return -1;
}

/*******************************************************************************
 **
 ** Function         my_app_print_usage
 **
 ** Description      This function displays command line usage
 **
 ** Returns          void
 **
 *******************************************************************************/
void my_app_print_usage(char **argv)
{
    char    *p_cmd;

    p_cmd = argv[0] + strlen(argv[0]) - 2;
    while ((p_cmd > argv[0]) &&
           (*p_cmd != '/'))
        p_cmd--;
    if (*p_cmd == '/')
        p_cmd++;

    printf("Usage: %s [OPTION]...\n", p_cmd);
    printf("  --help                     print this help\n");
}

/*******************************************************************************
 **
 ** Function         my_app_parse_cmd_line
 **
 ** Description      This function parse the complete command line
 **
 ** Returns          status
 **
 *******************************************************************************/
int my_app_parse_cmd_line(int argc, char **argv)
{
    int c;

    typedef int (*PFI)();

    PFI parse_param[] =
    {
        my_app_parse_help,
    };

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 0},            /* 0 Help => no parameter */
            {"server1", required_argument, 0, 0},   /* Server1 path => 1 parameter required */
            {NULL, 0, NULL, 0}
        };

        c = getopt_long_only(argc, argv, "", long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 0:
            printf ("Option [%s]", long_options[option_index].name);
            if (optarg)
            {
                printf (" with argument [%s]", optarg);
            }
            printf ("\n");

            if ((*parse_param[option_index])(optarg) < 0)
            {
                return -1;
            }
        break;

        case '?':
        default:
            return -1;
        }
    }

    if (optind < argc)
    {
        if (optind < argc)
        {
            printf ("Unknown parameter %s\n", argv[optind]);
            my_app_print_usage(argv);
            return -1;
        }
        printf ("\n");
    }
    return 0;
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

    /* Parse the command line parameters */
    if (my_app_parse_cmd_line(argc, argv) < 0)
    {
        my_app_print_usage(argv);
        return -1;
    }

    /* Initialize application */
    if (my_app_init() < 0)
    {
        APP_ERROR0("Unable to initialize application");
        return -1;
    }

    /* Open connection to BSA Server */
    status = app_mgt_open(NULL, my_app_mgt_callback);
    if (status < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Start application */
    status = my_app_start();
    if (status < 0)
    {
        APP_ERROR0("Couldn't initialize application");
        return -1;
    }

    /* Display versions */
    app_tm_read_version();

    /* The main loop */
    my_app_main_menu();

    /* Exit application (stop profiles) */
    my_app_exit();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}

