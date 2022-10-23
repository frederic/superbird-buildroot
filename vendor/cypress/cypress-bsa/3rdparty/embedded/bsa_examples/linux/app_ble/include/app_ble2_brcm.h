/*****************************************************************************
 **
 **  Name:           app_ble2_brcm.h
 **
 **  Description:    Bluetooth BLE2 Broadcom Proprietary application
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#ifndef APP_BLE2_BRCM_H
#define APP_BLE2_BRCM_H

#include "bsa_api.h"
#include "stdint.h"

/*
 * Definitions
 */
/* LE2 Broadcom Configuration Flags */
#define LE2_CFG_FLG_ADV_ENABLE              0x01    /* Advertising Enabled */
#define LE2_CFG_FLG_SCAN_ENABLE             0x02    /* Scan Enabled */
#define LE2_CFG_FLG_CON_ENABLE              0x04    /* Connection Enabled */
#define LE2_CFG_FLG_ACCESS_CODE_DEFAULT     0x71764129

#define LE2_EXT_PKT_LEN_MODE_0              0
#define LE2_EXT_PKT_LEN_MODE_0_MAX_LEN      27
#define LE2_EXT_PKT_LEN_MODE_1              1
#define LE2_EXT_PKT_LEN_MODE_1_MAX_LEN      123
#define LE2_EXT_PKT_LEN_MODE_2              2
#define LE2_EXT_PKT_LEN_MODE_2_MAX_LEN      123
#define LE2_EXT_PKT_LEN_MODE_3              3
#define LE2_EXT_PKT_LEN_MODE_3_MAX_LEN      251

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_init
 **
 ** Description     This is the BLE2 Broadcom init function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_init(void);

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
        uint32_t msb_access_code);

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
        uint32_t *p_msb_access_code);

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_ext_packet_len_write
 **
 ** Description     Write BLE2 Broadcom Extended Packet Length Flags
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_ext_packet_len_write(uint8_t mode, uint8_t max_packet_len);

/*******************************************************************************
 **
 ** Function        app_ble2_brcm_ext_packet_len_read
 **
 ** Description     Read BLE2 Broadcom Extended Packet Length Flags
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble2_brcm_ext_packet_len_read(uint8_t *p_mode, uint8_t *p_max_packet_len);

#endif /* APP_BLE2_BRCM_H */
