/*****************************************************************************
**
**  Name:           app_ble2_brcm.c
**
**  Description:    Bluetooth BLE2 Broadcom Proprietary application
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "app_ble2_brcm.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_dm.h"
#include "app_ble_client_db.h"
#include "app_ble_client.h"
#include "app_ble_server.h"

#if defined(APP_BLE2_BRCM_INCLUDED) && (APP_BLE2_BRCM_INCLUDED == TRUE)

/*
 * Definitions
 */
#define HCI_OPCODE_LE_META2_VSC                 0x123

#define LE_META2_EXT_OP_LE2_CFG_FLG_WRITE       0x01
#define LE_META2_EXT_OP_LE2_CFG_FLG_READ        0x02
#define LE_META2_EXT_OP_LE2_EXT_PKT_LEN_WRITE   0x03
#define LE_META2_EXT_OP_LE2_EXT_PKT_LEN_READ    0x04

typedef struct
{
    int dummy;
} tAPP_BLE2_BRCM_CB;

/*
 * Global Variables
 */
tAPP_BLE2_BRCM_CB app_ble2_brcm_cb;

/*
 * Local functions
 */

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_init
 **
 ** Description     This is the BLE2 Broadcom init function
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_init(void)
{
    memset(&app_ble2_brcm_cb, 0, sizeof(app_ble2_brcm_cb));

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_config_flags_write
 **
 ** Description     Write BLE2 Broadcom configuration Flags
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_config_flags_write(uint8_t flags, uint32_t lsb_access_code,
        uint32_t msb_access_code)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    uint8_t *p_data;

    /* Prepare VSC request */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HCI_OPCODE_LE_META2_VSC;

    p_data = bsa_vsc.data;
    UINT8_TO_STREAM(p_data, LE_META2_EXT_OP_LE2_CFG_FLG_WRITE);
    UINT8_TO_STREAM(p_data, flags);
    UINT32_TO_STREAM(p_data, lsb_access_code);
    UINT32_TO_STREAM(p_data, msb_access_code);

    bsa_vsc.length = p_data - bsa_vsc.data;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble2_brcm_config_flags_write BSA_TmVsc failed:%d", bsa_status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_config_flags_read
 **
 ** Description     Read BLE2 Broadcom configuration Flags
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_config_flags_read(uint8_t *p_flags, uint32_t *p_lsb_access_code,
        uint32_t *p_msb_access_code)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    uint8_t *p_data;
    uint8_t sub_opcode;

    /* Prepare VSC request */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HCI_OPCODE_LE_META2_VSC;

    p_data = bsa_vsc.data;
    UINT8_TO_STREAM(p_data, LE_META2_EXT_OP_LE2_CFG_FLG_READ);

    bsa_vsc.length = p_data - bsa_vsc.data;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble2_brcm_config_flags_read BSA_TmVsc failed:%d", bsa_status);
        return -1;
    }

    /* Extract Sub-Opcode */
    STREAM_TO_UINT8(sub_opcode, p_data);
    if (sub_opcode != LE_META2_EXT_OP_LE2_CFG_FLG_READ)
    {
        APP_ERROR1("app_ble2_brcm_config_flags_read Wrong Sub-Opcode:%d", sub_opcode);
        return -1;
    }

    /* Extract BLE2 flags and access code */
    STREAM_TO_UINT8(*p_flags, p_data);
    STREAM_TO_UINT32(*p_lsb_access_code, p_data);
    STREAM_TO_UINT32(*p_msb_access_code, p_data);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_ext_packet_len_write
 **
 ** Description     Write BLE2 Broadcom Extended Packet Length Flags
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_ext_packet_len_write(uint8_t mode, uint8_t max_packet_len)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    uint8_t *p_data;

    /* Prepare VSC request */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HCI_OPCODE_LE_META2_VSC;

    p_data = bsa_vsc.data;
    UINT8_TO_STREAM(p_data, LE_META2_EXT_OP_LE2_EXT_PKT_LEN_WRITE);
    UINT8_TO_STREAM(p_data, mode);
    UINT8_TO_STREAM(p_data, max_packet_len);

    bsa_vsc.length = p_data - bsa_vsc.data;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble2_brcm_ext_packet_len_write BSA_TmVsc failed:%d", bsa_status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_ext_packet_len_read
 **
 ** Description     Read BLE2 Broadcom Extended Packet Length Flags
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_ext_packet_len_read(uint8_t *p_mode, uint8_t *p_max_packet_len)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    uint8_t *p_data;
    uint8_t sub_opcode;

    /* Prepare VSC request */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = HCI_OPCODE_LE_META2_VSC;

    p_data = bsa_vsc.data;
    UINT8_TO_STREAM(p_data, LE_META2_EXT_OP_LE2_EXT_PKT_LEN_READ);

    bsa_vsc.length = p_data - bsa_vsc.data;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_ble2_brcm_ext_packet_len_read BSA_TmVsc failed:%d", bsa_status);
        return -1;
    }

    /* Extract Sub-Opcode */
    STREAM_TO_UINT8(sub_opcode, p_data);
    if (sub_opcode != LE_META2_EXT_OP_LE2_EXT_PKT_LEN_READ)
    {
        APP_ERROR1("app_ble2_brcm_ext_packet_len_read Wrong Sub-Opcode:%d", sub_opcode);
        return -1;
    }

    /* Extract BLE2 flags and access code */
    STREAM_TO_UINT8(*p_mode, p_data);
    STREAM_TO_UINT8(*p_max_packet_len, p_data);

    return 0;
}

#endif
