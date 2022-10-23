/*****************************************************************************
 **
 **  Name:           app_headless_ble.c
 **
 **  Description:    BLE Headless utility functions for applications
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include "app_headless_ble.h"

/*
 * Definitions
 */
/* Broadcom BLE VSCs controlling Headless */
#define BRCM_HCI_VSC_ADD_LE_HID_DEV         0x0124
#define BRCM_HCI_VSC_ADD_LE_TV_WAKE_RECORD  0x0193

#define APP_HEADLESS_BLE_RECORD_MAX         3   /* Maximum Record supported */



/*******************************************************************************
 **
 ** Function        app_headless_ble_hid_add_send
 **
 ** Description     Send a BLE HID Device Add command to the local BT Chip
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_headless_ble_hid_add_send(BD_ADDR bdaddr, tBLE_ADDR_TYPE bdaddr_type,
        UINT8 *p_class_of_device, UINT16 appearance, BT_OCTET16  ltk,
        BT_OCTET8 rand, UINT16 ediv)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p_data;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = BRCM_HCI_VSC_ADD_LE_HID_DEV;
    p_data = bsa_vsc.data;

    /* Write BdAddr parameter */
    BDADDR_TO_STREAM(p_data, bdaddr);

    /* Write BdAddr Type parameter */
    UINT8_TO_STREAM(p_data, bdaddr_type);

    /* Write Class Of Device parameter */
    DEVCLASS_TO_STREAM(p_data, p_class_of_device);

    /* Write Appearance Value parameter */
    UINT16_TO_STREAM(p_data, bdaddr_type);

    /* Write Ltk parameter */
    REVERSE_ARRAY_TO_STREAM(p_data, ltk, BT_OCTET16_LEN);

    /* Write Rand parameter */
    ARRAY_TO_STREAM(p_data, rand, BT_OCTET8_LEN);

    /* Write Ediv parameter */
    UINT16_TO_STREAM(p_data, ediv);

    bsa_vsc.length = p_data - bsa_vsc.data;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_headless_ble_hid_add_send BSA_TmVsc failed:%d", bsa_status);
        return(-1);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_headless_ble_add_tv_wake_records_send
 **
 ** Description     Send a BLE Add TV Wake records command to the local BT Chip
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_headless_ble_add_tv_wake_records_send(BD_ADDR bdaddr, UINT8 nb_records,
        tAPP_HEADLESS_BLE_RECORD *p_records)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p_data;
    int index;
    tAPP_HEADLESS_BLE_RECORD *p_rec = p_records;

    if ((nb_records == 0) ||
        (nb_records > APP_HEADLESS_BLE_RECORD_MAX))
    {
        APP_ERROR1("app_headless_ble_add_tv_wake_records_send bad Records number: %s",
                nb_records);
        return(-1);
    }

    if (p_rec == NULL)
    {
        APP_ERROR0("app_headless_ble_add_tv_wake_records_send No Records");
        return(-1);
    }

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = BRCM_HCI_VSC_ADD_LE_TV_WAKE_RECORD;
    p_data = bsa_vsc.data;

    /* Write BdAddr parameter */
    BDADDR_TO_STREAM(p_data, bdaddr);

    /* Write Nb Records parameter */
    UINT8_TO_STREAM(p_data, nb_records);

    for (index = 0 ; index < nb_records ; index++, p_rec++)
    {
        /* Write Record's length parameter */
        UINT8_TO_STREAM(p_data, p_rec->len);

        /* Write Record's Attribute Handle parameter */
        UINT16_TO_STREAM(p_data, p_rec->attribute_handle);

        /* Write Record's Data parameter */
        ARRAY_TO_STREAM(p_data, p_rec->data, p_rec->len);
    }

    bsa_vsc.length = p_data - bsa_vsc.data;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_headless_ble_hid_add_send BSA_TmVsc failed:%d", bsa_status);
        return(-1);
    }

    return 0;
}

