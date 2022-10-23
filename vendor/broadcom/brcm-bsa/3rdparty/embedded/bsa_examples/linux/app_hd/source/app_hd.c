/*****************************************************************************
 **
 **  Name:           app_hd.c
 **
 **  Description:    Bluetooth HID Device application
 **
 **  Copyright (c) 2009-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
/* for *printf */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <termios.h>    /* termios, TCSANOW, ECHO, ICANON */
#include <unistd.h>     /* STDIN_FILENO */

#include "app_hd.h"

#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_link.h"
#include "app_hd_main.h"
#include "bsa_trace.h"

#if defined(APP_HD_AUDIO_STREAMING_INCLUDED) && (APP_HD_AUDIO_STREAMING_INCLUDED == TRUE)
#include "app_hd_as.h"
#endif

#ifndef APP_HD_SEC_DEFAULT
#define APP_HD_SEC_DEFAULT BSA_SEC_NONE
#endif


//#define APP_TRACE_NODEBUG
/*
 * Globals
 */

tAPP_HD_CB app_hd_cb;




/*******************************************************************************
 **
 ** Function         app_hd_cback
 **
 ** Description      Example of call back function of HID Device application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hd_cback(tBSA_HD_EVT event, tBSA_HD_MSG *p_data)
{
    /* If a specific callback function is defined */
    if (app_hd_cb.p_event_cback)
    {
        if (app_hd_cb.p_event_cback(event, p_data))
        {
            return;
        }
    }

    switch (event)
    {
    case BSA_HD_OPEN_EVT:
        APP_INFO1("BSA_HD_OPEN_EVT: status:%d", p_data->open.status);
        APP_INFO1("BdAddr :%02X-%02X-%02X-%02X-%02X-%02X",
                p_data->open.bd_addr[0], p_data->open.bd_addr[1], p_data->open.bd_addr[2],
                p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
        break;

    case BSA_HD_UNPLUG_EVT:
        APP_INFO0("BSA_HD_VC_UNPLUG_EVT is handled like a close event, redirecting");
    case BSA_HD_CLOSE_EVT:
        APP_INFO1("BSA_HD_CLOSE_EVT: status:%d", p_data->close.status);
        APP_INFO1("BdAddr :%02X-%02X-%02X-%02X-%02X-%02X",
                p_data->close.bd_addr[0], p_data->close.bd_addr[1], p_data->close.bd_addr[2],
                p_data->close.bd_addr[3], p_data->close.bd_addr[4], p_data->close.bd_addr[5]);
        app_hd_disable();
        break;

    case BSA_HD_DATA_EVT:
        APP_INFO1("BSA_HD_DATA_EVT: length:%d", p_data->data.len);
#if defined(APP_HD_AUDIO_STREAMING_INCLUDED) && (APP_HD_AUDIO_STREAMING_INCLUDED == TRUE)
        app_hd_as_audio_handler(&(p_data->data));
#endif
        break;

    case BSA_HD_DATC_EVT:
        APP_INFO1("BSA_HD_DATC_EVT: length:%d", p_data->data.len);
#if defined(APP_HD_AUDIO_STREAMING_INCLUDED) && (APP_HD_AUDIO_STREAMING_INCLUDED == TRUE)
        app_hd_as_audio_handler(&(p_data->data));
#endif
        break;

    default:
        APP_ERROR1("bad event:%d", event);
        break;
    }
}


/*******************************************************************************
 **
 ** Function         app_hd_start
 **
 ** Description      Start HID Device
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_start(void)
{
    int status;
    tBSA_HD_ENABLE enable_param;

    APP_INFO0("app_hd_start");

    BSA_HdEnableInit(&enable_param);
    enable_param.sec_mask = APP_HD_SEC_DEFAULT;
    app_hd_cb.sec_mask_in = enable_param.sec_mask;

    enable_param.p_cback = app_hd_cback;

    status = BSA_HdEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable HID Device status:%d", status);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hd_listen
 **
 ** Description      Listen HID Host
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_listen(void)
{
    int status;
    tBSA_HD_ENABLE enable_param;

    APP_INFO0("app_hd_start");

    BSA_HdEnableInit(&enable_param);
    enable_param.sec_mask = APP_HD_SEC_DEFAULT;
    app_hd_cb.sec_mask_in = enable_param.sec_mask;

    enable_param.bd_addr[0] = 0xff;
    enable_param.bd_addr[1] = 0xff;
    enable_param.bd_addr[2] = 0xff;
    enable_param.bd_addr[3] = 0xff;
    enable_param.bd_addr[4] = 0xff;
    enable_param.bd_addr[5] = 0xff;

    enable_param.p_cback = app_hd_cback;

    status = BSA_HdEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable HID Device status:%d", status);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hd_connect
 **
 ** Description      Example of function to connect to HID host
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_connect(void)
{
    int status = -1;
    tBSA_STATUS bsastatus;
    int device_index;
    BOOLEAN connect = FALSE;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_HD_OPEN hd_open_param;
    int choice;
    tBSA_HD_ENABLE enable_param;

    APP_INFO0("Bluetooth HID connect menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    device_index = app_get_choice("Select source");

    /* Devices from XML database */
    if (device_index == 0)
    {
        /* Read the XML file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
            p_name = app_xml_remote_devices_db[device_index].name;
            connect = TRUE;
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_disc_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_DISC_NB_DEVICES) &&
            (app_discovery_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
            p_name = app_discovery_cb.devs[device_index].device.name;
            connect = TRUE;
        }
    }
    else
    {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
        return -1;
    }

    if (!connect)
    {
        APP_ERROR1("Bad Device index:%d", device_index);
        return -1;
    }

    BSA_HdEnableInit(&enable_param);
    enable_param.sec_mask = APP_HD_SEC_DEFAULT;
    app_hd_cb.sec_mask_in = enable_param.sec_mask;

    enable_param.p_cback = app_hd_cback;
    bdcpy(enable_param.bd_addr, bd_addr);

    status = BSA_HdEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable HID Device status:%d", status);
        return -1;
    }


    BSA_HdOpenInit(&hd_open_param);

    APP_INFO0("Security?");
    APP_INFO0("   NONE: 0");
    APP_INFO0("   Authentication: 1");
    APP_INFO0("   Encryption+Authentication: 3");
    APP_INFO0("   Authorization: 4");
    APP_INFO0("   Encryption+Authentication+Authorization: 7");

    choice = app_get_choice("Enter Security mask");
    if ((choice & ~7) == 0)
    {
        hd_open_param.sec_mask = BTA_SEC_NONE;
        if (choice & 1)
        {
            hd_open_param.sec_mask |= BSA_SEC_AUTHENTICATION;
        }
        if (choice & 2)
        {
            hd_open_param.sec_mask |= BSA_SEC_ENCRYPTION;
        }
        if (choice & 4)
        {
            hd_open_param.sec_mask |= BSA_SEC_AUTHORIZATION;
        }
    }
    else
    {
        APP_ERROR1("Bad Security Mask selected :%d", choice);
        return -1;
    }
    app_hd_cb.sec_mask_out = hd_open_param.sec_mask;

    bsastatus = BSA_HdOpen(&hd_open_param);
    if (bsastatus != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HdOpen failed: %d", bsastatus);
    }
    else
    {
        APP_DEBUG1("Connecting to HID host:%s", p_name);
    }

    return status;
}


/*******************************************************************************
 **
 ** Function         app_hd_disconnect
 **
 ** Description      Example of HD Close
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_disconnect(void)
{
    int status;
    tBSA_HD_CLOSE hd_close_param;

    APP_INFO0("Bluetooth HID Disconnect");

    BSA_HdCloseInit(&hd_close_param);
    status = BSA_HdClose(&hd_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to disconnect HID host status:%d", status);
        return -1;
    }
    else
    {
        APP_DEBUG0("Disconnecting from HID host...");
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function        app_hd_send_regular_key
 **
 ** Description     Example of HD Send Regular key
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_regular_key(void)
{
    int status = BSA_SUCCESS;
    tBSA_HD_SEND hd_send_param;
    int c;
    static struct termios oldt, newt;

    APP_INFO0("app_hd_send_regular_key");

    BSA_HdSendInit(&hd_send_param);
    hd_send_param.key_type = BSA_HD_REGULAR_KEY;
    hd_send_param.param.reg_key.auto_release = TRUE;
    hd_send_param.param.reg_key.key_code = 0;
    hd_send_param.param.reg_key.modifier = 0;

    tcgetattr( STDIN_FILENO, &oldt);    /* save current settings */
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    printf("Enter keys to send, ESC key to stop...\n");
    for (;;)
    {
        if (27 == (c =getchar()))
            break;

        hd_send_param.param.reg_key.key_code = (c-'a')+4;
        status = BSA_HdSend(&hd_send_param);

        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_HdSend fail status:%d", status);
            break;
        }
    }

    /*restore the old settings*/
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

    return status;
}


/*******************************************************************************
 **
 ** Function        app_hd_send_special_key
 **
 ** Description     Example of HD Send Special key
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_special_key(void)
{
    int status;
    tBSA_HD_SEND hd_send_param;

    APP_INFO0("app_hd_send_special_key");

    BSA_HdSendInit(&hd_send_param);
    hd_send_param.key_type = BSA_HD_SPECIAL_KEY;
    hd_send_param.param.sp_key.auto_release = TRUE;
    hd_send_param.param.sp_key.key_len = 1;

    status = BSA_HdSend(&hd_send_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HdSend fail status:%d", status);
        return -1;
    }
    return status;
}




/*******************************************************************************
 **
 ** Function        app_hd_send_mouse_data
 **
 ** Description     Example of HD Send mouse data
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_mouse_data(void)
{
    int status;
    tBSA_HD_SEND hd_send_param;

    APP_INFO0("app_hd_send_mouse_data");

    BSA_HdSendInit(&hd_send_param);
    hd_send_param.key_type = BSA_HD_MOUSE_DATA;
    hd_send_param.param.mouse.delta_wheel = 0;
    hd_send_param.param.mouse.delta_x = 0;
    hd_send_param.param.mouse.delta_y = 0;
    hd_send_param.param.mouse.is_left = 0;
    hd_send_param.param.mouse.is_middle = 0;
    hd_send_param.param.mouse.is_right = 0;

    status = BSA_HdSend(&hd_send_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HdSend fail status:%d", status);
        return -1;
    }

    return status;

}


/*******************************************************************************
 **
 ** Function        app_hd_send_customer_data
 **
 ** Description     Example of HD Send customer data
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_customer_data(void)
{
    int status, index;
    tBSA_HD_SEND hd_send_param;

    APP_INFO0("app_hd_send_customer_data");

    BSA_HdSendInit(&hd_send_param);
    hd_send_param.key_type = BSA_HD_CUSTOMER_DATA;
    hd_send_param.param.customer.data_len = 100;
    for (index = 0 ; index < hd_send_param.param.customer.data_len ; index++)
    {
        hd_send_param.param.customer.data[index] = index;
    }

    status = BSA_HdSend(&hd_send_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HdSend fail status:%d", status);
        return -1;
    }
    return status;
}


/*******************************************************************************
 **
 ** Function         app_hd_disable
 **
 ** Description      Disable HID Device
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_disable(void)
{
    int status;

    tBSA_HD_DISABLE disable_param;

    APP_INFO0("app_hd_disable");

    BSA_HdDisableInit(&disable_param);

    status = BSA_HdDisable(&disable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to disable HID Device status:%d", status);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hd_init
 **
 ** Description      Initialize HD
 **
 ** Parameters       boot: indicate if the management path must be created
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_init(void)
{
    /* Reset HD Control Block */
    memset(&app_hd_cb, 0, sizeof(app_hd_cb));

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hd_exit
 **
 ** Description      Exit HD application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_exit(void)
{
    tBSA_HD_DISABLE dis_param;

    BSA_HdDisableInit(&dis_param);
    BSA_HdDisable(&dis_param);

    return 0;
}

/*******************************************************************************
** Function         app_hd_set_cod
**
** Description      Sets the COD to HID device
**
** Parameters
**
** Returns          status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_hd_set_cod()
{
    DEV_CLASS cod = { 0x00, 0x05, 0xC0 };

    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        APP_ERROR0("BSA_DmSetConfigInit failed");
        return -1;
    }

    memcpy(bsa_dm_set_config.class_of_device, cod, sizeof(DEV_CLASS));
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_DEV_CLASS_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        APP_ERROR0("BSA_DmSetConfig failed");
        return -1;
    }

    return 0;
}
