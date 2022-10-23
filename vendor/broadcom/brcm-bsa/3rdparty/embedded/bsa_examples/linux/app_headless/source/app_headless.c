/*****************************************************************************
 **
 **  Name:           app_headless.c
 **
 **  Description:    Bluetooth Headless functions
 **
 **  Copyright (c) 2011-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bsa_api.h"

#include "app_headless.h"
#include "app_utils.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_dm.h"
#include "app_tm_vsc.h"


#define HEADLESS_SET_SCAN_MODE  0x134
#define HEADLESS_DEV_LIST_VSC   0x36
#define HEADLESS_DEV_ADD_VSC    0x37
#define HEADLESS_DEV_DEL_VSC    0x39
#define HEADLESS_ENABLE_VSC     0x3B

/*******************************************************************************
 **
 ** Function         app_headless_send_add_device
 **
 ** Description      This function sends a VSC to add one Headless device
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_add_device(BD_ADDR bdaddr, UINT8 *p_class_of_device, LINK_KEY link_key)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p = &bsa_vsc.data[0];
    int i;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HEADLESS_DEV_ADD_VSC; /* Add HID Device VSC */
    bsa_vsc.length = BD_ADDR_LEN + LINK_KEY_LEN + DEV_CLASS_LEN;

    /* Write the BdAddr in HCI format (LSB first)*/
    BDADDR_TO_STREAM(p, bdaddr);

    /* Copy LinkKey in HCI format (LSB first) */
    for (i = 0 ; i < LINK_KEY_LEN ; i++)
    {
        *p++ = link_key[LINK_KEY_LEN -1 - i];
    }

    /* Copy COD in HCI format (LSB first)*/
    *p++ = p_class_of_device[2];
    *p++ = p_class_of_device[1];
    *p++ = p_class_of_device[0];

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send VSC status:%d", bsa_status);
        return(-1);
    }

    APP_DEBUG1("VSC Server status:%d", bsa_vsc.status);
    APP_DEBUG1("VSC opcode:0x%03X", bsa_vsc.opcode);
    APP_DEBUG1("VSC length:%d", bsa_vsc.length);
    APP_DEBUG1("VSC Chip/HCI status:0x%02X", bsa_vsc.data[0]);

    if (bsa_vsc.status != 0)
    {
        APP_ERROR0("Bad HCI Return code");
        return (-1);
    }

    return (0);
}

/*******************************************************************************
 **
 ** Function         app_headless_send_enable_uhe
 **
 ** Description      This function sends a VSC to Control Headless mode
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_enable_uhe(int enable)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HEADLESS_ENABLE_VSC; /* Enable USB HID Emulation VSC */
    bsa_vsc.length = 1;

    if (enable)
    {
        APP_DEBUG0("Enabling Headless mode");
        bsa_vsc.data[0] = 0x01;
    }
    else
    {
        APP_DEBUG0("Disabling Headless mode");
        bsa_vsc.data[0] = 0x00;
    }

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send VSC status:%d", bsa_status);
        return(-1);
    }

    APP_DEBUG1("VSC Server status:%d", bsa_vsc.status);
    APP_DEBUG1("VSC opcode:0x%03X", bsa_vsc.opcode);
    APP_DEBUG1("VSC length:%d", bsa_vsc.length);
    APP_DEBUG1("VSC Chip/HCI status:0x%02X", bsa_vsc.data[0]);

    if (bsa_vsc.status != 0)
    {
        APP_ERROR0("Bad HCI Return code");
        return (-1);
    }
    return (0);
}

/*******************************************************************************
 **
 ** Function         app_headless_send_del_device
 **
 ** Description      This function sends a VSC to delete one Headless device
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_del_device(BD_ADDR bdaddr)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p = &bsa_vsc.data[0];

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HEADLESS_DEV_DEL_VSC; /* Delete HID Device VSC */
    bsa_vsc.length = BD_ADDR_LEN;

    /* Write the BdAddr in HCI format (LSB first)*/
    BDADDR_TO_STREAM(p, bdaddr);

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send VSC status:%d", bsa_status);
        return(-1);
    }

    APP_DEBUG1("VSC Server status:%d", bsa_vsc.status);
    APP_DEBUG1("VSC opcode:0x%03X", bsa_vsc.opcode);
    APP_DEBUG1("VSC length:%d", bsa_vsc.length);
    APP_DEBUG1("VSC Chip/HCI status:0x%02X", bsa_vsc.data[0]);

    if (bsa_vsc.status != 0)
    {
        APP_ERROR0("Bad HCI Return code");
        return (-1);
    }

    return (0);
}

/*******************************************************************************
 **
 ** Function         app_headless_send_list_device
 **
 ** Description      This function sends a VSC to list Headless devices
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_send_list_device(tAPP_HEADLESS_DEV_LIST *p_dev_list, UINT8 table_size)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p = &bsa_vsc.data[0];
    int i;
    int nb_dev;
    UINT8 null_pin;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HEADLESS_DEV_LIST_VSC; /* List HID Devices VSC */
    bsa_vsc.length = 0;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send VSC status:%d", bsa_status);
        return(-1);
    }

    APP_DEBUG1("VSC Server status:%d", bsa_vsc.status);
    APP_DEBUG1("VSC opcode:0x%03X", bsa_vsc.opcode);
    APP_DEBUG1("VSC length:%d", bsa_vsc.length);
    APP_DEBUG1("VSC Chip/HCI status:0x%02X", bsa_vsc.data[0]);

    if (bsa_vsc.status != 0)
    {
        APP_ERROR0("Bad HCI Return code");
        return (-1);
    }

    p = &bsa_vsc.data[0];
    p++; /* Skip Status (already read) */

    /* Extract number of devices returned */
    STREAM_TO_UINT8(nb_dev, p);

    for (i = 0 ; (i < nb_dev) && (i < table_size) ; i++)
    {
        /* Read the BdAddr in HCI format (LSB first)*/
        STREAM_TO_BDADDR(p_dev_list[i].bdaddr, p);
        /* Read the COD in HCI format (LSB first)*/
        STREAM_TO_DEVCLASS(p_dev_list[i].class_of_device, p);
        /* Extract  Null+PIN code (ignored) */
        STREAM_TO_UINT8(null_pin, p);

        APP_DEBUG1("Headless dev[%i] BdAddr:%02X-%02X-%02X-%02X-%02X-%02X COD:%02X-%02X-%02X Null:%d",
                i,
                p_dev_list[i].bdaddr[0], p_dev_list[i].bdaddr[1],
                p_dev_list[i].bdaddr[2], p_dev_list[i].bdaddr[3],
                p_dev_list[i].bdaddr[4], p_dev_list[i].bdaddr[5],
                p_dev_list[i].class_of_device[0],
                p_dev_list[i].class_of_device[1],
                p_dev_list[i].class_of_device[2],
                null_pin);
    }

    if (nb_dev > table_size)
    {
        APP_ERROR1("Table too small nb:%d (received:%d)", table_size, nb_dev);
    }

    return (i);
}




/*******************************************************************************
 **
 ** Function         app_headless_scan_control
 **
 ** Description      Enable/Disable Headless Page Scan mode
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_headless_scan_control(void)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 page_scan, llr_scan;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HEADLESS_SET_SCAN_MODE; /* Enable USB HID Emulation VSC */
    bsa_vsc.length = 2;

    page_scan = app_get_choice("Set Headless Page Scan mode(0 = disable, 1 = enable)");
    if((page_scan != 0) && (page_scan != 1))
    {
        APP_ERROR1("Invalid Parameter:%d", page_scan);
        return(-1);
    }

    llr_scan = app_get_choice("Set Headless LLR Scan mode(0 = disable, 1 = enable)");
    if((llr_scan != 0) && (llr_scan != 1))
    {
        APP_ERROR1("Invalid Parameter:%d", llr_scan);
        return(-1);
    }

    bsa_vsc.data[0] = page_scan;
    bsa_vsc.data[1] = llr_scan;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send VSC status:%d", bsa_status);
        return(-1);
    }

    APP_DEBUG1("VSC Server status:%d", bsa_vsc.status);
    APP_DEBUG1("VSC opcode:0x%03X", bsa_vsc.opcode);
    APP_DEBUG1("VSC length:%d", bsa_vsc.length);
    APP_DEBUG1("VSC Chip/HCI status:0x%02X", bsa_vsc.data[0]);

    if (bsa_vsc.status != 0)
    {
        APP_ERROR0("Bad HCI Return code");
        return (-1);
    }
    return (0);
}
