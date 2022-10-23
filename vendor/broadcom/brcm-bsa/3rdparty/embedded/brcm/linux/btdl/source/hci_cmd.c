/*
 *
 * hci_cmd.c
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

/* for NULL */
#include <stddef.h>
#include <unistd.h>

#include "hci_cmd.h"

#include "hci_drv.h"
#include "btdl_utils.h"
#include "btdl_hex.h"

/*
 *  Definitions for HCI groups
 */
#define HCI_GRP_LINK_CONTROL_CMDS       (0x01 << 10)            /* 0x0400 */
#define HCI_GRP_LINK_POLICY_CMDS        (0x02 << 10)            /* 0x0800 */
#define HCI_GRP_HOST_CONT_BASEBAND_CMDS (0x03 << 10)            /* 0x0C00 */
#define HCI_GRP_INFORMATIONAL_PARAMS    (0x04 << 10)            /* 0x1000 */
#define HCI_GRP_STATUS_PARAMS           (0x05 << 10)            /* 0x1400 */
#define HCI_GRP_TESTING_CMDS            (0x06 << 10)            /* 0x1800 */
#define HCI_GRP_VENDOR_SPECIFIC         (0x3F << 10)            /* 0xFC00 */

/*
 * Some HCI OpCode definitions
 */
#define HCI_RESET                       (0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)

#define HCI_READ_LOCAL_VERSION          (0x0001 | HCI_GRP_INFORMATIONAL_PARAMS)
#define HCI_READ_BD_ADDR                (0x0009 | HCI_GRP_INFORMATIONAL_PARAMS)

#define HCI_WRITE_BDADDR                (0x0001 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_FACTORY_COMMIT_BDADDR       (0x0010 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DWNLD_MINIDRV               (0x002E | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_WRITE_RAM                   (0x004C | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_READ_RAM                    (0x004D | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_LAUNCH_RAM                  (0x004E | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_ENABLE_BTW                  (0x0053 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_UPGRADE_FIRMWARE            (0x0122 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_CHIP_ERASE                  (0x03CE | HCI_GRP_VENDOR_SPECIFIC)

/*******************************************************************************
 **
 ** Function        hci_cmd_send_receive
 **
 ** Description     High level function which send an HCI command and wait for the
 **                 answer from the chip
 **
 ** Parameters      opcode: HCI OpCode
 **                 params: pointer on parameters
 **                 params_size: size of the buffer to send
 **                 ret_param_max: maximum size of the received parameters
 **
 ** Returns         size of the returned parameters (<0 if error)
 **
 *******************************************************************************/

int hci_cmd_send_receive(uint16_t opcode, const uint8_t *params,
        uint8_t params_size, uint8_t *p_ret_param, uint8_t ret_param_max)
{
    uint8_t buf[HCI_CMD_SIZE_MAX];
    uint8_t *p = buf;
    int length;
    int rv = -1;
    uint8_t hci_h4_hdr, hci_evt, hci_num, hci_len, hci_status;
    uint16_t hci_opcode;

    /* build the command header */
    UINT8_TO_STREAM(p, HCI_CMD_H4_CMD);
    UINT16_TO_STREAM(p, opcode);
    UINT8_TO_STREAM(p, params_size);
    ARRAY_TO_STREAM(p, params, params_size);

    /* Send the command */
    rv = hci_drv_write(buf, params_size + HCI_CMD_HEADER);
    if (rv < 0)
    {
        return -1;
    }

    /* Wait for acknowledgment */
    p = buf;
    rv = hci_drv_read(buf, sizeof(buf));

    if (rv > 5)
    {
        /* Extract H4 Header */
        STREAM_TO_UINT8(hci_h4_hdr, p);
        /* Extract HCI Event code */
        STREAM_TO_UINT8(hci_evt, p);
        /* Extract HCI Length */
        STREAM_TO_UINT8(hci_len, p);
        /* Extract HCI number of Command Packet (unused) */
        STREAM_TO_UINT8(hci_num, p);
        (void)hci_num;
        /* Extract HCI Opcode Packet */
        STREAM_TO_UINT16(hci_opcode, p);

        if ((hci_h4_hdr == HCI_CMD_H4_EVT) &&       /* HCI-UART Event Indication */
            (hci_evt    == HCI_CMD_CMD_CPLT_EVT) && /* HCI Command Complete Event */
            (hci_opcode == opcode))                 /* Check OpCode */
        {
            /* Compute the length of the HCI Parameters (skip NumCmd and Opcode) */
            length = hci_len - sizeof(uint8_t) - sizeof(uint16_t);

            /* Extract HCI Status */
            STREAM_TO_UINT8(hci_status, p);
            length--;

            /* Check the HCI Status */
            if (hci_status == HCI_CMD_STATUS_OK)
            {
                /* The function will return the size of the parameters */
                rv = length;
            }
            else
            {
                /* The HCI error is positive. Let's make it negative (unix like) */
                rv = 0 - 100 - hci_status;
            }
            /* Copy parameters if caller provided a buffer */
            if (p_ret_param != NULL)
            {
                /* Check if the return buffer is big enough to contain parameters */
                if (length > ret_param_max)
                {
                    length = ret_param_max;
                    btdl_print(BTDL_ERROR, "Response buffer not large enough to hold data, cutting");
                }
                STREAM_TO_ARRAY(p_ret_param, p, length);
            }
        } /* HCI decoding*/
        else
        {
            /* We received a wrong Event */
            rv = -1;
        }
    } /* HCI Read ok */
    else
    {
        /* We received a wrong Event */
        rv = -1;
    }
    return rv;
}

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
int hci_cmd_sw_reset(void)
{
    int status = 0;
    uint8_t buf[256];

    btdl_print(BTDL_DEBUG, "sw_reset");

    /* Disable UHE */
    btdl_print(BTDL_INFO, "Enable all HCI commands (disable UHE BTW specific mode)");
    buf[0] = 0;

    /* Exit UHE mode (this may fail if FW does not support this command) */
    hci_cmd_send_receive(HCI_ENABLE_BTW, buf, 1, NULL, 0);

    /* Send an HCI_Reset */
    btdl_print(BTDL_INFO, "Send HCI_Reset");
    status = hci_cmd_send_receive(HCI_RESET, NULL, 0, NULL, 0);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while resetting the BT controller status:%d", status);
        return -1;
    }
    return 0;
}

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
int hci_cmd_upgrade_firmware(void)
{
    int status = 0;

    btdl_print(BTDL_DEBUG, "Upgrade_firmware");

    /* Send an HCI_Reset */
    btdl_print(BTDL_INFO, "Send HCI_UPGRADE_FIRMWARE");
    status = hci_cmd_send_receive(HCI_UPGRADE_FIRMWARE, NULL, 0, NULL, 0);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while resetting the BT controller status:%d", status);
        return -1;
    }
    return 0;
}

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
int hci_cmd_read_fw_version(void)
{
    uint8_t buf[200];
    uint8_t *p = buf;
    int status;
    uint16_t hci_version;
    uint16_t lmp_subversion;
    uint8_t major, minor, build;
    uint16_t config;
    uint8_t u8;
    uint16_t u16;

    btdl_print(BTDL_INFO, "Reading the FW version");

    /* Send the VSC */
    status = hci_cmd_send_receive(HCI_READ_LOCAL_VERSION, NULL, 0, buf, sizeof(buf));
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive return:%d", status);
        return -1;
    }

    /* Read HCI Version (unused) */
    STREAM_TO_UINT8(u8, p);
    (void)u8;

    /* Read HCI Revision */
    STREAM_TO_UINT16(hci_version, p);

    /* Read LMP Version (unused) */
    STREAM_TO_UINT8(u8, p);
    (void)u8;

    /* Read Manufacturer code/name (unused) */
    STREAM_TO_UINT16(u16, p);
    (void)u16;

    STREAM_TO_UINT16(lmp_subversion, p);

    btdl_print(BTDL_DEBUG, "hci_version:0x%04X lmp_subversion:0x%04X", hci_version, lmp_subversion);

    /* compute clear version number */
    major = (lmp_subversion >> 13) & 0x07;
    minor = (lmp_subversion >> 8)  & 0x1F;
    build = (lmp_subversion >> 0)  & 0xFF;
    config = hci_version & 0x0FFF;

    btdl_print(BTDL_PRINT, "FW Version: %d.%d.%d.%d", major, minor, build, config);
    return 0;
}


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
int hci_cmd_chip_erase(uint32_t fw_addr)
{
    uint8_t buf[200];
    uint8_t *p = buf;
    int status;

    btdl_print(BTDL_INFO, "Erasing Chip FW_Addr:%08X", fw_addr);

    /* Write the FW Address in the message */
    UINT32_TO_STREAM(p, fw_addr);

    /* Send the VSC */
    status = hci_cmd_send_receive(HCI_CHIP_ERASE, buf, 4, NULL, 0);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive return:%d", status);
        return -1;
    }

    btdl_print(BTDL_DEBUG, "Chip Erase done");
    return 0;
}

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
int hci_cmd_launch_ram(uint32_t fw_addr)
{
    uint8_t buf[200];
    uint8_t *p = buf;
    int status;

    btdl_print(BTDL_INFO, "Launching Ram FW_Addr:%08X", fw_addr);

    /* Write the FW Address in the message */
    UINT32_TO_STREAM(p, fw_addr);

    /* Send the VSC */
    status = hci_cmd_send_receive(HCI_LAUNCH_RAM, buf, 4, buf, sizeof(buf));
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive return:%d", status);
        return -1;
    }

    btdl_print(BTDL_DEBUG, "Launch Ram Done");
    return 0;
}

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
int hci_cmd_read_bd_addr(uint8_t *bd_addr)
{
    int status = 0;
    uint8_t buf[256];
    uint8_t *p = buf;

    btdl_print(BTDL_INFO, "Reading BdAddr");

    if (bd_addr == NULL)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_read_bd_addr null pointer");
        return -1;
    }

    /* Read the current BD address */
    status = hci_cmd_send_receive(HCI_READ_BD_ADDR, NULL, 0, buf, BD_ADDR_LEN);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive return:%d", status);
        return -1;
    }

    STREAM_TO_BDADDR(bd_addr, p);

    btdl_print(BTDL_PRINT, "Current Module's BdAddr: %02X:%02X:%02X:%02X:%02X:%02X",
        bd_addr[0], bd_addr[1], bd_addr[2],
        bd_addr[3], bd_addr[4], bd_addr[5]);

    return 0;
}

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
int hci_cmd_dwnld_minidrv(void)
{
    int status = 0;

    btdl_print(BTDL_INFO, "Send Download Mini Driver");

    status = hci_cmd_send_receive(HCI_DWNLD_MINIDRV, NULL, 0, NULL, 0);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive return:%d", status);
        return -1;
    }

    btdl_print(BTDL_DEBUG, "Send Download Mini Driver Done");

    return status;
}

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
int hci_cmd_write_ram(uint32_t fw_addr, uint8_t *p_buf, uint8_t length)
{
    int status = 0;
    uint8_t buf[HCI_CMD_SIZE_MAX];
    uint8_t *p = buf;

    btdl_print(BTDL_DEBUG, "Write RAM Addr:%08X len:%d", fw_addr, length);

    UINT32_TO_STREAM(p, fw_addr);
    ARRAY_TO_STREAM(p, p_buf, length);

    status = hci_cmd_send_receive(HCI_WRITE_RAM, buf, length + 4, NULL, 0);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive return:%d", status);
        return -1;
    }

    btdl_print(BTDL_DEBUG, "Write RAM Done");

    return status;
}

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
int hci_cmd_read_ram(uint32_t fw_addr, uint8_t *p_buf, uint8_t length)
{
    int status = 0;
    uint8_t buf[HCI_CMD_SIZE_MAX];
    uint8_t *p = buf;

    if (length > HCI_CMD_READ_RAM_SIZE_MAX)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive length too big:%d", length);
        return -1;
    }

    btdl_print(BTDL_DEBUG, "Read RAM fw_addr:%08X len:%d", fw_addr, length);

    UINT32_TO_STREAM(p, fw_addr);
    UINT8_TO_STREAM(p, length);

    status = hci_cmd_send_receive(HCI_READ_RAM, buf, 5, p_buf, length);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "hci_cmd_send_receive return:%d", status);
        return -1;
    }

    btdl_print(BTDL_DEBUG, "Read RAM Done");

    return status;
}


