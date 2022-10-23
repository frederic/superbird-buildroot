/*****************************************************************************
**
**  Name:           app_headless_main.c
**
**  Description:    Headless Main application
**
**  Copyright (c) 2011-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "app_headless_main.h"

/*
 * Defines
 */

/* The Reset TV_WAKEUP is not yet implemented in FW */
#define RESET_TV_WAKEUP_INCLUDED        FALSE

/* Headless menu items */
enum
{
    APP_HEADLESS_MENU_ADD = 1,
    APP_HEADLESS_MENU_DELETE,
    APP_HEADLESS_MENU_ENABLE,
    APP_HEADLESS_MENU_DISABLE,
    APP_HEADLESS_MENU_SET_HL_SCAN_MODE,
    APP_HEADLESS_MENU_ADD_TV_WAKEUP,
    APP_HEADLESS_MENU_REMOVE_TV_WAKEUP,
    APP_HEADLESS_MENU_RESET_TV_WAKEUP,
    APP_HEADLESS_MENU_ADD_BLE_HID_DEV,
    APP_HEADLESS_MENU_ADD_BLE_TV_WAKE_RECORDS,
    APP_HEADLESS_MENU_QUIT = 99
};

/*
 * Global Variables
 */

/*
 * Local Functions
 */
static void app_display_headless_menu (void);
static BOOLEAN app_headless_mgt_callback(tBSA_MGT_EVT event,
        tBSA_MGT_MSG *p_data);
static int app_headless_add(void);
static int app_headless_control(int enable);
static int app_headless_delete(void);
static int app_tvwakeup_dev_add();
static int app_tvwakeup_dev_remove(void);

/*******************************************************************************
 **
 ** Function         app_display_headless_menu
 **
 ** Description      This is the Headless menu
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_display_headless_menu (void)
{
    APP_INFO0("\nBluetooth Headless menu:");
    APP_INFO1("\t%d => Add Headless Device", APP_HEADLESS_MENU_ADD);
    APP_INFO1("\t%d => Remove Headless Device", APP_HEADLESS_MENU_DELETE);
    APP_INFO1("\t%d => Enable Headless Mode", APP_HEADLESS_MENU_ENABLE);
    APP_INFO1("\t%d => Disable Headless Mode", APP_HEADLESS_MENU_DISABLE);
    APP_INFO1("\t%d => Configure Headless Scan Mode",
            APP_HEADLESS_MENU_SET_HL_SCAN_MODE);
    APP_INFO1("\t%d => Add TV WakeUp Device",
            APP_HEADLESS_MENU_ADD_TV_WAKEUP);
    APP_INFO1("\t%d => Remove TV WakeUp Device",
            APP_HEADLESS_MENU_REMOVE_TV_WAKEUP);
#if defined(RESET_TV_WAKEUP_INCLUDED) && (RESET_TV_WAKEUP_INCLUDED == TRUE)
    APP_INFO1("\t%d => Reset TV WakeUp Device (clear all)",
            APP_HEADLESS_MENU_RESET_TV_WAKEUP);
#endif
    APP_INFO1("\t%d => Add BLE HID Device", APP_HEADLESS_MENU_ADD_BLE_HID_DEV);
    APP_INFO1("\t%d => Add BLE TV Wake Records", APP_HEADLESS_MENU_ADD_BLE_TV_WAKE_RECORDS);

    APP_INFO1("\t%d => Exit Sub-Menu", APP_HEADLESS_MENU_QUIT);
}


/*******************************************************************************
 **
 ** Function         app_headless_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          BOOLEAN
 **
 *******************************************************************************/
BOOLEAN app_headless_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth Restarted");
        }
        else
        {
            APP_DEBUG0("Bluetooth Disabled");
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
 ** Function         app_headless_add
 **
 ** Description
 **
 ** Returns          int
 **
 *******************************************************************************/
static int app_headless_add(void)
{
    int index;

    /* Read the xml file which contains all the bonded devices */
    app_read_xml_remote_devices();

    app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    index = app_get_choice("Enter device Index to Add");

    if (app_xml_is_device_index_valid(index))
    {
        APP_DEBUG1("Adding :%s", app_xml_remote_devices_db[index].name);
    }
    else
    {
        APP_ERROR1("Invalid Device index:%d", index);
        return -1;
    }

    return app_headless_send_add_device(
            app_xml_remote_devices_db[index].bd_addr,
            &app_xml_remote_devices_db[index].class_of_device[0],
            app_xml_remote_devices_db[index].link_key);
}

/*******************************************************************************
 **
 ** Function         app_headless_control
 **
 ** Description
 **
 ** Returns          int
 **
 *******************************************************************************/
static int app_headless_control(int enable)
{
    int status;
    tBSA_MGT_KILL_SERVER bsa_kill_param;

    if (enable)
    {
        APP_DEBUG0("Stopping Bluetooth");
        app_dm_set_local_bt_config(FALSE);

        APP_DEBUG0("Enable Headless Mode");
        status = app_headless_send_enable_uhe(1);
        if (status < 0)
        {
            APP_ERROR0("Not able to enable Headless mode");
            app_dm_set_local_bt_config(TRUE);
            return status;
        }

        APP_DEBUG0("Killing BSA Server");
        BSA_MgtKillServerInit(&bsa_kill_param);
        BSA_MgtKillServer(&bsa_kill_param);
    }
    else
    {
        APP_DEBUG0("Disable Headless Mode");
        status = app_headless_send_enable_uhe(0);
        if (status < 0)
        {
            APP_ERROR0("Not able to disable Headless mode");
            app_dm_set_local_bt_config(TRUE);
            return status;
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_headless_delete
 **
 ** Description
 **
 ** Returns          int
 **
 *******************************************************************************/
static int app_headless_delete(void)
{
    int rv;
    tAPP_HEADLESS_DEV_LIST headless_dev_tab[APP_HEADLESS_DEV_NB];
    UINT8 i;

    rv = app_headless_send_list_device(headless_dev_tab, APP_HEADLESS_DEV_NB);
    if (rv < 0)
    {
        APP_ERROR0("Headless list failed");
        return -1;
    }

    APP_DEBUG1("%d Headless Device(s) found:", rv);
    for (i = 0 ; i < rv ; i++)
    {
        APP_DEBUG1("Headless dev[%i] BdAddr:%02X-%02X-%02X-%02X-%02X-%02X",
                i,
                headless_dev_tab[i].bdaddr[0], headless_dev_tab[i].bdaddr[1],
                headless_dev_tab[i].bdaddr[2], headless_dev_tab[i].bdaddr[3],
                headless_dev_tab[i].bdaddr[4], headless_dev_tab[i].bdaddr[5]);
    }

    i = app_get_choice("Enter device Index to Delete");

    if (i < rv)
    {
        APP_DEBUG1("Removing Headless device[%i]", i);
        rv = app_headless_send_del_device(headless_dev_tab[i].bdaddr);
    }
    else
    {
        APP_ERROR1("Headless device cannot be removed status:%d", rv);
        rv = -1;
    }
    return rv;
}

/*******************************************************************************
 **
 ** Function         app_tvwakeup_dev_add
 **
 ** Description      Add a TV WakeUp  Device
 **
 ** Returns          status
 **
 *******************************************************************************/
static int app_tvwakeup_dev_add(void)
{
    int index;
    tAPP_TVWAKEUP_PROFILE tvwakeup_profile[1];

    /* Read the xml file which contains all the bonded devices */
    app_read_xml_remote_devices();

    app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    index = app_get_choice("Enter device Index to Add");

    if (app_xml_is_device_index_valid(index))
    {
        APP_DEBUG1("Adding :%s", app_xml_remote_devices_db[index].name);
    }
    else
    {
        APP_ERROR1("Invalid Device index:%d", index);
        return -1;
    }

    tvwakeup_profile[0].id = APP_TVWAKEUP_PROFILE_ID_HID;
    tvwakeup_profile[0].records_nb = 1;
    tvwakeup_profile[0].records[0].len = 6;
    tvwakeup_profile[0].records[0].data[0] = 0x02;
    tvwakeup_profile[0].records[0].data[1] = 0x02;
    tvwakeup_profile[0].records[0].data[2] = 0x00;
    tvwakeup_profile[0].records[0].data[3] = 0x20;
    tvwakeup_profile[0].records[0].data[4] = 0x00;
    tvwakeup_profile[0].records[0].data[5] = 0x00;

    /* Format and Send the VSC */
    return app_tvwakeup_add_send(
            app_xml_remote_devices_db[index].bd_addr,
            app_xml_remote_devices_db[index].link_key,
            &app_xml_remote_devices_db[index].class_of_device[0],
            &tvwakeup_profile[0], 1);
}

/*******************************************************************************
 **
 ** Function         app_tvwakeup_dev_remove
 **
 ** Description      Remove a TV WakeUp Device
 **
 ** Returns          status
 **
 *******************************************************************************/
static int app_tvwakeup_dev_remove(void)
{
    int index;

    /* Read the xml file which contains all the bonded devices */
    app_read_xml_remote_devices();

    app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    index = app_get_choice("Enter device Index to Add");

    if (app_xml_is_device_index_valid(index))
    {
        APP_DEBUG1("Adding :%s", app_xml_remote_devices_db[index].name);
    }
    else
    {
        APP_ERROR1("Invalid Device index:%d", index);
        return -1;
    }

    /* Format and Send the VSC */
    return app_tvwakeup_remove_send(app_xml_remote_devices_db[index].bd_addr);
}

/*******************************************************************************
 **
 ** Function         app_headless_ble_hid_add
 **
 ** Description      Add a BLE HID Headless Device
 **
 ** Returns          status
 **
 *******************************************************************************/
static int app_headless_ble_hid_add(void)
{
    int index;
    tAPP_XML_REM_DEVICE *p_xml_dev;

    /* Read the xml file which contains all the bonded devices */
    app_read_xml_remote_devices();

    app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    index = app_get_choice("Enter device Index to Add");

    if (app_xml_is_device_index_valid(index))
    {
        APP_DEBUG1("Adding :%s", app_xml_remote_devices_db[index].name);
    }
    else
    {
        APP_ERROR1("Invalid Device index:%d", index);
        return -1;
    }

    p_xml_dev = &app_xml_remote_devices_db[index];

    /* Format and Send the VSC */
    return app_headless_ble_hid_add_send(
            p_xml_dev->bd_addr,
            p_xml_dev->ble_addr_type,
            p_xml_dev->class_of_device,
            0, /* TODO: find appearance */
            p_xml_dev->penc_ltk,
            p_xml_dev->penc_rand,
            p_xml_dev->penc_ediv);
}

/*******************************************************************************
 **
 ** Function         app_headless_ble_tv_wake_records_add
 **
 ** Description      Add a BLE TV Wake Record
 **
 ** Returns          status
 **
 *******************************************************************************/
static int app_headless_ble_tv_wake_records_add(void)
{
    int index;
    tAPP_XML_REM_DEVICE *p_xml_dev;
    tAPP_HEADLESS_BLE_RECORD tv_wake_record;

    /* Read the xml file which contains all the bonded devices */
    app_read_xml_remote_devices();

    app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    index = app_get_choice("Enter device Index");

    if (app_xml_is_device_index_valid(index))
    {
        APP_DEBUG1("Adding :%s", app_xml_remote_devices_db[index].name);
    }
    else
    {
        APP_ERROR1("Invalid Device index:%d", index);
        return -1;
    }

    p_xml_dev = &app_xml_remote_devices_db[index];

    APP_DEBUG0("Hardcoding Button '0' for BLE TV WakeUp. It can be changed in file:");
    APP_DEBUG1("%s", __FILE__);

    /* Attribute Handle (Report Id)*/
    tv_wake_record.attribute_handle = 0x01;

    /* Hardcode the data length */
    tv_wake_record.len = 8;

    memset(tv_wake_record.data, 0, sizeof(tv_wake_record.data));
    tv_wake_record.data[2] = 0x27; /* Hardcode the '0' button */

    /* Format and Send the VSC */
    return app_headless_ble_add_tv_wake_records_send(p_xml_dev->bd_addr, 1,
            &tv_wake_record);
}

/*******************************************************************************
 **
 ** Function         app_headless_menu
 **
 ** Description      Handle the headless menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_headless_menu(void)
{
    int choice;

    do
    {
        /* Print the Headless menu */
        app_display_headless_menu();

        /* Wait for User's input */
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

        case APP_HEADLESS_MENU_ADD_TV_WAKEUP:
            app_tvwakeup_dev_add();
            break;

        case APP_HEADLESS_MENU_REMOVE_TV_WAKEUP:
            app_tvwakeup_dev_remove();
            break;

#if defined(RESET_TV_WAKEUP_INCLUDED) && (RESET_TV_WAKEUP_INCLUDED == TRUE)
        case APP_HEADLESS_MENU_RESET_TV_WAKEUP:
            app_tvwakeup_clear_send();
            break;
#endif

        case APP_HEADLESS_MENU_ADD_BLE_HID_DEV:
            app_headless_ble_hid_add();
            break;

        case APP_HEADLESS_MENU_ADD_BLE_TV_WAKE_RECORDS:
            app_headless_ble_tv_wake_records_add();
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
    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_headless_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* The main Headless loop */
    app_headless_menu();

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}

