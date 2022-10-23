/*
 *
 * hci_cmd.h
 *
 *
 *
 * Copyright (C) 2012-2015 Broadcom Corporation.
 *
 *
 *
 * This software is licensed under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation (the "GPL"), and may
 * be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL for more details.
 *
 *
 * A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php
 * or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA
 *
 * HCI Command functions for BT controller Download tool
 *
 */

/* idempotency */
#ifndef HCI_CMD_H
#define HCI_CMD_H

/* self-sufficiency */
#include <stdint.h>

/* Maximum size of an HCI command (parameters) */
#define HCI_CMD_PARAM_MAX           255
#define HCI_EVT_PARAM_MAX           255

/* Write RAM size */
#define HCI_CMD_WRITE_RAM_HEADER_SIZE    (sizeof(uint32_t))
#define HCI_CMD_WRITE_RAM_SIZE_MAX   (HCI_CMD_PARAM_MAX - HCI_CMD_WRITE_RAM_HEADER_SIZE)

/* Read RAM size */
#define HCI_CMD_READ_RAM_HEADER_SIZE    (sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t))
#define HCI_CMD_READ_RAM_SIZE_MAX   (HCI_EVT_PARAM_MAX - HCI_CMD_READ_RAM_HEADER_SIZE)

#define BD_ADDR_LEN 6

/* HCI H4 Header definitions */
#define HCI_CMD_H4_CMD          0x01
#define HCI_CMD_H4_EVT          0x04

/* HCI Command header (H4 Header, OpCode and Length) */
#define HCI_CMD_HEADER          (sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t))

/* HCI Command Buffer size max */
#define HCI_CMD_SIZE_MAX        (HCI_CMD_HEADER + HCI_CMD_PARAM_MAX)

/* HCI Command Status definition */
#define HCI_CMD_STATUS_OK       0x00

/* HCI Event definition */
#define HCI_CMD_CMD_CPLT_EVT    0x0E

/*******************************************************************************
 **
 ** Function        hci_cmd_sw_reset
 **
 ** Description     Software reset of the BT chip
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_sw_reset(void);

/*******************************************************************************
 **
 ** Function        hci_cmd_read_fw_version
 **
 ** Description     Reads the FW version (based on HCI_ReadLocalVersion)
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_read_fw_version(void);

/*******************************************************************************
 **
 ** Function        hci_cmd_chip_erase
 **
 ** Description     Erase Flash/EEPROM
 **
 ** Parameters      fw_addr: FW address to erase (MSB must be FF for serial)
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_chip_erase(uint32_t fw_addr);

/*******************************************************************************
 **
 ** Function        hci_cmd_launch_ram
 **
 ** Description     Send a LaunchRAM VSC
 **
 ** Parameters      fw_addr: FW address to jump to
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_launch_ram(uint32_t fw_addr);

/*******************************************************************************
 **
 ** Function        hci_cmd_read_bd_addr
 **
 ** Description     Read the BdAddr
 **
 ** Parameters      bd_addr: pointer on BdAddr buffer
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_read_bd_addr(uint8_t *bd_addr);

/*******************************************************************************
 **
 ** Function        hci_cmd_dwnld_minidrv
 **
 ** Description     Send the Download Mini Driver Command
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_dwnld_minidrv(void);

/*******************************************************************************
 **
 ** Function        hci_cmd_upgrade_firmware
 **
 ** Description     upgrade firmware of the BT chip
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_upgrade_firmware(void);

/*******************************************************************************
 **
 ** Function        hci_cmd_write_ram
 **
 ** Description     Send the Write RAM Command
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_write_ram(uint32_t fw_addr, uint8_t *p_buf, uint8_t length);

/*******************************************************************************
 **
 ** Function        hci_cmd_read_ram
 **
 ** Description     Send the Read RAM Command
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_cmd_read_ram(uint32_t fw_addr, uint8_t *p_buf, uint8_t length);

#endif
