/*****************************************************************************
**
**  Name:           app_hh_main.c
**
**  Description:    Bluetooth HID Host main application
**
**  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
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

#include "bsa_api.h"

#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_utils.h"
#include "app_services.h"
#include "app_hh_main.h"
#include "app_hh.h"
#if defined(APP_HH_AUDIO_STREAMING_INCLUDED) && (APP_HH_AUDIO_STREAMING_INCLUDED == TRUE)
#include "app_hh_as.h"
#endif
/*#define HID_KB*/

#define APP_XML_REM_DEVICES_FILE_PATH "./bt_devices.xml"


/* Menu items */
enum
{
    /* HH Menu */
    APP_HH_MENU_DISC_ABORT = 1,
    APP_HH_MENU_COD_PERIF_DISC,
    APP_HH_MENU_COD_3DSG_DISC,
    APP_HH_MENU_BLE_DISC,
    APP_HH_MENU_LIMITED_DISC,
    APP_HH_MENU_CONNECT_REPORT,
    APP_HH_MENU_CONNECT_BOOT,
    APP_HH_MENU_DISCONNECT,
    APP_HH_MENU_GET_DSCP,
    APP_HH_MENU_SET_REPORT,
    APP_HH_MENU_GET_REPORT,
    APP_HH_MENU_SEND_CTRL_REQ,
    APP_HH_MENU_SEND_CTRL_DATA,
    APP_HH_MENU_SEND_CTRL_INT,
    APP_HH_MENU_SEND_VC_UNPLUG,
    APP_HH_MENU_SET_UCD_BRR_OLD,
    APP_HH_MENU_CFG_HID_UCD_NEW,
    APP_HH_MENU_START_3DSG,
    APP_HH_MENU_STOP_3DSG,
    APP_HH_MENU_REMOVE_DEV,
    APP_HH_MENU_ENABLE_MIP,
    APP_HH_MENU_DISABLE_MIP,
    APP_HH_MENU_SET_PROTO,
    APP_HH_MENU_GET_PROTO,
    APP_HH_MENU_SET_IDLE,
    APP_HH_MENU_GET_IDLE,
    APP_HH_MENU_SET_PRIO_AUDIO,
#if defined(APP_HH_OTA_FW_DL_INCLUDED) && (APP_HH_OTA_FW_DL_INCLUDED == TRUE)
    APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_EEPROM,
    APP_HH_MENU_FIRMWARE_UPDATE_FS_TO_EEPROM,
    APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_SFLASH,
    APP_HH_MENU_FIRMWARE_UPDATE_FS_TO_SFLASH,
    APP_HH_MENU_FIRMWARE_FASTBOOT_UPDATE_NOFS_TO_SFLASH,
    APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_DUALBOOT_RCU,
#endif
#if defined(APP_HH_AUDIO_STREAMING_INCLUDED) && (APP_HH_AUDIO_STREAMING_INCLUDED == TRUE)
    APP_HH_MENU_CONFIGURE_AUDIO_PACKET_INTERVAL,
    APP_HH_MENU_TOGGLE_TONE,
    APP_HH_MENU_SELECT_SOUND,
    APP_HH_MENU_START_STREAMING,
    APP_HH_MENU_STOP_AUDIO_STREAMING,
    APP_HH_MENU_SEND_NOTIF_AS_START_TO_RCU,
    APP_HH_MENU_SEND_NOTIF_AS_STOP_TO_RCU,
#endif
    APP_HH_MENU_QUIT = 99
};

/*
 * Local functions
 */


/*******************************************************************************
 **
 ** Function         app_hh_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_display_main_menu(void)
{
    printf("\nBluetooth HID Host menu:\n");
    printf("    %d => Discovery Abort\n", APP_HH_MENU_DISC_ABORT);
    printf("    %d => COD Discovery (Peripheral Dev)\n", APP_HH_MENU_COD_PERIF_DISC);
    printf("    %d => COD Discovery (3DSG Dev)\n", APP_HH_MENU_COD_3DSG_DISC);
    printf("    %d => BLE Discovery\n", APP_HH_MENU_BLE_DISC);
    printf("    %d => Limited Discovery (LIAC)\n", APP_HH_MENU_LIMITED_DISC);
    printf("    %d => HID Connect (Report mode)\n", APP_HH_MENU_CONNECT_REPORT);
    printf("    %d => HID Connect (Boot mode)\n", APP_HH_MENU_CONNECT_BOOT);
    printf("    %d => HID Disconnect\n", APP_HH_MENU_DISCONNECT);
    printf("    %d => HID Get DSCP Info\n", APP_HH_MENU_GET_DSCP);
    printf("    %d => HID Set Report (num lock)\n", APP_HH_MENU_SET_REPORT);
    printf("    %d => HID Get Report \n", APP_HH_MENU_GET_REPORT);
    printf("    %d => Send HID Control\n", APP_HH_MENU_SEND_CTRL_REQ);
    printf("    %d => Send Set report to control channel\n", APP_HH_MENU_SEND_CTRL_DATA);
    printf("    %d => Send Set report to interrupt channel\n", APP_HH_MENU_SEND_CTRL_INT);
    printf("    %d => Send Virtual Cable Unplug\n", APP_HH_MENU_SEND_VC_UNPLUG);
    printf("    %d => HID Set UCD/BRR mode (Old remotes)\n", APP_HH_MENU_SET_UCD_BRR_OLD);
    printf("    %d => HID Configure UCD mode (New remotes)\n", APP_HH_MENU_CFG_HID_UCD_NEW);
    printf("    %d => HID Start 3DSG mode\n", APP_HH_MENU_START_3DSG);
    printf("    %d => HID Stop 3DSG mode\n", APP_HH_MENU_STOP_3DSG);
    printf("    %d => HID Remove Device\n", APP_HH_MENU_REMOVE_DEV);
    printf("    %d => Enable Multicast Individual Poll\n", APP_HH_MENU_ENABLE_MIP);
    printf("    %d => Disable Multicast Individual Poll\n", APP_HH_MENU_DISABLE_MIP);
    printf("    %d => HID Set Protocol \n", APP_HH_MENU_SET_PROTO);
    printf("    %d => HID Get Protocol \n", APP_HH_MENU_GET_PROTO);
    printf("    %d => HID Set Idle \n", APP_HH_MENU_SET_IDLE);
    printf("    %d => HID Get Idle \n", APP_HH_MENU_GET_IDLE);
    printf("    %d => HID Set Priority for Audio\n", APP_HH_MENU_SET_PRIO_AUDIO);
#if defined(APP_HH_OTA_FW_DL_INCLUDED) && (APP_HH_OTA_FW_DL_INCLUDED == TRUE)
    printf("    %d => Non fail-safe firmware update to eeprom (file = fw.hex)\n",
            APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_EEPROM);
    printf("    %d => Fail-safe firmware update to eeprom(file = fw.hex)\n",
            APP_HH_MENU_FIRMWARE_UPDATE_FS_TO_EEPROM);
    printf("    %d => Non Fail-safe firmware update to sflash(file = fw.hex)\n",
            APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_SFLASH);
    printf("    %d => Fail-safe firmware update to sflash(file = fw.hex)\n",
            APP_HH_MENU_FIRMWARE_UPDATE_FS_TO_SFLASH);
    printf("    %d => Non fail-safe fast boot firmware update to sflash(file = fw.hex fastboot = fastboot.txt)\n",
            APP_HH_MENU_FIRMWARE_FASTBOOT_UPDATE_NOFS_TO_SFLASH);
    printf("    %d => Download firmware to Dual Boot RCU\n",
            APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_DUALBOOT_RCU);
#endif
#if defined(APP_HH_AUDIO_STREAMING_INCLUDED) && (APP_HH_AUDIO_STREAMING_INCLUDED == TRUE)
    printf("    %d => Configure audio packet interval(20ms, 40ms)\n", APP_HH_MENU_CONFIGURE_AUDIO_PACKET_INTERVAL);
    printf("    %d => Toggle tone signal\n", APP_HH_MENU_TOGGLE_TONE);
    printf("    %d => Select audio source(tone or wav)\n", APP_HH_MENU_SELECT_SOUND);
    printf("    %d => Start audio streaming to RCU\n", APP_HH_MENU_START_STREAMING);
    printf("    %d => Stop audio streaming to RCU\n", APP_HH_MENU_STOP_AUDIO_STREAMING);
    printf("    %d => Send Notification of Start Streaming to RCU\n", APP_HH_MENU_SEND_NOTIF_AS_START_TO_RCU);
    printf("    %d => Send Notification of Stop Streaming to RCU\n", APP_HH_MENU_SEND_NOTIF_AS_STOP_TO_RCU);
#endif
    printf("    %d => Quit\n", APP_HH_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_hh_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_hh_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
            app_hh_init();
            app_hh_start();
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
 ** Function        app_dtv_hh_add_dev_cback
 **
 ** Description     Update the HhAddDev parameters for the application
 **
 ** Parameters      p_params: the HhAddDev function parameters (already pre-filled)
 **                 p_device: the XML contained information of the device
 **
 ** Returns         void
 **
 *******************************************************************************/
static void app_hh_main_add_dev_cback(tBSA_HH_ADD_DEV *p_params, tAPP_XML_REM_DEVICE *p_device)
{

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
    int choice;
    UINT8 handle;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_hh_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Init XML state machine */
    app_xml_init();

    /* Init HID Host Application */
    app_hh_init();

    app_hh_add_dev_cback_register(app_hh_main_add_dev_cback);
    app_hh_start();

    do
    {
        app_hh_display_main_menu();

        choice = app_get_choice("Select action");

        switch(choice)
        {
        case APP_HH_MENU_DISC_ABORT:
            /* Example to abort Discovery */
            app_disc_abort();
            break;

        case APP_HH_MENU_COD_PERIF_DISC:
            /* Example to perform Peripheral Device discovery (in blocking mode) */
            app_disc_start_cod(0, BTM_COD_MAJOR_PERIPHERAL, 0, NULL);
            break;

        case APP_HH_MENU_COD_3DSG_DISC:
            /* Example to perform 3DSG Device discovery (in blocking mode) */
            app_disc_start_cod(0, BTM_COD_MAJOR_WEARABLE , 0, NULL);
            break;

        case APP_HH_MENU_BLE_DISC:
            app_disc_start_ble_regular(NULL);
            break;

        case APP_HH_MENU_LIMITED_DISC:
            /* Example to perform Limited discovery (LIAC) */
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

        case APP_HH_MENU_SET_REPORT:
            /* Example to Send report */
            app_hh_set_report();
            break;

        case APP_HH_MENU_GET_REPORT:
            /* Example to Get report */
            app_hh_get_report();
            break;

        case APP_HH_MENU_SEND_CTRL_REQ:
            app_hh_send_hid_ctrl();
            break;

        case APP_HH_MENU_SEND_CTRL_DATA:
            app_hh_send_ctrl_data();
            break;

        case APP_HH_MENU_SEND_CTRL_INT:
            app_hh_send_int_data();
            break;

        case APP_HH_MENU_SEND_VC_UNPLUG:
            app_hh_send_vc_unplug();
            break;

        case APP_HH_MENU_SET_UCD_BRR_OLD:
            /* Example to Set RC in UCD mode */
            app_hh_set_ucd_brr_mode();
            break;

        case APP_HH_MENU_CFG_HID_UCD_NEW:
            app_hh_cfg_hid_ucd(BSA_HH_INVALID_HANDLE, 0, 0);
            break;

        case APP_HH_MENU_START_3DSG:
            /* Example to Start 3DSG mode for a Device  */
            app_hh_set_3dsg_mode(TRUE, -1, 16666);
            break;

        case APP_HH_MENU_STOP_3DSG:
            /* Example to Stop 3DSG mode for a Device */
            app_hh_set_3dsg_mode(FALSE, -1, 0);
            break;

        case APP_HH_MENU_REMOVE_DEV:
            /* Example to remove a device */
            app_hh_remove_dev();
            break;

        case APP_HH_MENU_ENABLE_MIP:
            /* Example to disconnect HID Device */
            app_hh_enable_mip();
            break;

        case APP_HH_MENU_DISABLE_MIP:
            /* Example to disconnect HID Device */
            app_hh_disable_mip();
            break;

        case APP_HH_MENU_SET_PROTO:
            /* Example to Set Protocol mode */
            app_hh_set_proto_mode();
            break;

        case APP_HH_MENU_GET_PROTO:
            /* Example to Get Protocol mode */
            app_hh_get_proto_mode();
            break;

        case APP_HH_MENU_SET_IDLE:
            /* Example to Set Idle */
            app_hh_set_idle();
            break;

        case APP_HH_MENU_GET_IDLE:
            /* Example to Get Idle */
            app_hh_get_idle();
            break;

        case APP_HH_MENU_SET_PRIO_AUDIO:
            /* Example to set priority of link for HID voice */
            app_hh_set_prio_audio();
            break;

#if defined(APP_HH_OTA_FW_DL_INCLUDED) && (APP_HH_OTA_FW_DL_INCLUDED == TRUE)
        case APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_EEPROM:
            app_hh_otafwdl_fwupdate_nofs();
            break;

        case APP_HH_MENU_FIRMWARE_UPDATE_FS_TO_EEPROM:
            app_hh_otafwdl_fwupdate_fs();
            break;

        case APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_SFLASH:
            app_hh_otafwdl_fwupdate_sflash_nofs();
            break;

        case APP_HH_MENU_FIRMWARE_UPDATE_FS_TO_SFLASH:
            app_hh_otafwdl_fwupdate_sflash_fs();
            break;

        case APP_HH_MENU_FIRMWARE_FASTBOOT_UPDATE_NOFS_TO_SFLASH:
            app_hh_otafwdl_fwupdate_fastboot_sflash();
            break;

        case APP_HH_MENU_FIRMWARE_UPDATE_NOFS_TO_DUALBOOT_RCU:
            app_hh_otafwdl_fwupdate_dualboot_rcu();
            break;
#endif

#if defined(APP_HH_AUDIO_STREAMING_INCLUDED) && (APP_HH_AUDIO_STREAMING_INCLUDED == TRUE)
        case APP_HH_MENU_CONFIGURE_AUDIO_PACKET_INTERVAL:
            app_hh_as_configure_audio_interval();
            break;

        case APP_HH_MENU_TOGGLE_TONE:
            app_hh_as_toggle_tone();
            break;

        case APP_HH_MENU_SELECT_SOUND:
            app_hh_as_select_sound();
            break;

        case APP_HH_MENU_START_STREAMING:
            app_hh_display_status();
            handle = app_get_choice("Enter HID Handle");
            app_hh_as_start_streaming(handle);
            break;

        case APP_HH_MENU_STOP_AUDIO_STREAMING:
            app_hh_display_status();
            handle = app_get_choice("Enter HID Handle");
            app_hh_as_stop_streaming(handle);
            break;

        case APP_HH_MENU_SEND_NOTIF_AS_START_TO_RCU:
            app_hh_display_status();
            handle = app_get_choice("Enter HID Handle");
            app_hh_as_notif_start_streaming(handle);
            break;

        case APP_HH_MENU_SEND_NOTIF_AS_STOP_TO_RCU:
            app_hh_display_status();
            handle = app_get_choice("Enter HID Handle");
            app_hh_as_notif_stop_streaming(handle);
            break;
#endif
        case APP_HH_MENU_QUIT:
            printf("main: Bye Bye\n");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_HH_MENU_QUIT); /* While user don't exit application */

    /* Exit HH */
    app_hh_exit();
    /*
     * Close BSA before exiting (to release resources)
     */
    app_mgt_close();

    return 0;

}
