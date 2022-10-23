/*
 *
 * btdl_hex.h
 *
 *
 *
 * Copyright (C) 2012-2014 Broadcom Corporation.
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
 * Utility functions to manipulate INTEL hex files
 *
 */

/* idempotency */
#ifndef BTDL_HEX_H
#define BTDL_HEX_H

/* self-sufficiency */
#include <stdint.h>

/* this structure represents a contiguous memory area */
typedef struct i32hex
{
    /* start address of the area */
    uint32_t addr;
    /* length of the area */
    uint32_t length;
    /* area content */
    uint8_t *data;
    /* pointer to the next contiguous area */
    struct i32hex *next;
    /* debug info */
    int linenum;
} tFLASH_DL_I32HEX;


/*******************************************************************************
 **
 ** Function        btdl_hex_parse_i32hexfile
 **
 ** Description     Read a .hex file and create corresponding Hex chained list
 **
 ** Parameters      fname: filename
 **                 nochksum: indicates if CheckSum verification must be ignored
 **
 ** Returns         Pointer on created chained list
 **
 *******************************************************************************/
tFLASH_DL_I32HEX *btdl_hex_parse_i32hexfile(const char *fname, int nochksum);

/*******************************************************************************
 **
 ** Function        btdl_hex_extract_section
 **
 ** Description     Search and extract (unlink) the Static Section from
 **                 an Hex chained list
 **
 ** Parameters      pp_head: IN/OUT head of the chained list
 **
 ** Returns         pointer on the extracted section
 **
 *******************************************************************************/
struct i32hex *btdl_hex_extract_static_section(tFLASH_DL_I32HEX **pp_head);

/*******************************************************************************
 **
 ** Function        btdl_hex_free_i32hex
 **
 ** Description     Free an Hex chained list
 **
 ** Parameters      head: head of the chained list
 **
 ** Returns         void
 **
 *******************************************************************************/
void btdl_hex_free_i32hex(tFLASH_DL_I32HEX *p_head);

/*******************************************************************************
 **
 ** Function        btdl_hex_overwrite_bdaddr
 **
 ** Description     Overwrite the BdAddr in the BdAddr item of a Static Section
 **                 pointed by Hex chained list
 **
 ** Parameters      p_section: Hex section
 **                 p_bd_addr: BdAddr to write
 **
 ** Returns         Status
 **
 *******************************************************************************/
int btdl_hex_overwrite_bdaddr(tFLASH_DL_I32HEX *p_section, uint8_t *p_bd_addr);

/*******************************************************************************
 **
 ** Function        btdl_hex_check
 **
 ** Description     Read the FW and check that it match the Hex downloaded
 **
 **
 ** Parameters      head: Hex head
 **                 chip_select: indicates FW location:
 **                        0x00  => RAM/Parallel Flash
 **                        0xFF  => Serial Flash/EEPROM
 **
 ** Returns         Status
 **
 *******************************************************************************/
int btdl_hex_check(tFLASH_DL_I32HEX *head, uint8_t chip_select);

/*******************************************************************************
 **
 ** Function        btdl_hex_download
 **
 ** Description     Download an Hex chained list
 **
 ** Parameters      head: hea of the Hex chained list
 **                 chip_select: indicates FW location:
 **                        0x00  => RAM/Parallel Flash
 **                        0xFF  => Serial Flash/EEPROM
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hex_download(tFLASH_DL_I32HEX *head, uint8_t chip_select);

/*******************************************************************************
 **
 ** Function        btdl_hex_firmware_reload
 **
 ** Description     Firmware reload
 **
 ** Parameters      pointer to burn image`
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hex_firmware_reload(tFLASH_DL_I32HEX *p_burn_image);

#endif
