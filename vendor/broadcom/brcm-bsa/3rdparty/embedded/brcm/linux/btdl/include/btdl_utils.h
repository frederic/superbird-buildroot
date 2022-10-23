/*
 *
 * btdl_utils.h
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
 * Utility functions
 *
 */

/* idempotency */
#ifndef BTDL_UTILS_H
#define BTDL_UTILS_H

/* self-sufficiency */
#include <stdint.h>

/*
 * Types
*/

/* this structure represents the local control block */
typedef struct
{
    char *device;
    char *minidriver;
    char *burnimage;
    int nodl;
    int nochksum;
    int is_bd_addr_requested;
    uint8_t bd_addr_requested[6];
    uint8_t chip_select;
    int mem_type_param;
    int is_shared_usb;
    int is_failsafe;
    int bDynamicFirmwareReload;
    int bNeedDummyACLHandle;
    int bWaitForKey;
    int bWaitForDLAck;
    unsigned long delay_between_bulk_writes_ms;
    unsigned long delay_at_the_end_ms;
    unsigned long AddressBase;
    int nominidrv;
} tBTDL_CB;

typedef enum
{
    BTDL_ERROR = 0,         /* Error message (cannot be filtered out) */
    BTDL_PRINT,             /* Minimum Verbose Level (default) */
    BTDL_INFO,              /* Limited Verbose Level */
    BTDL_DEBUG,             /* Debug Verbose Level */
    BTDL_FULL_DEBUG,        /* Full Debug Verbose Level */
} tBTDL_VERBOSE_LEVEL;


/********************************************************************************
** Macros to get and put bytes to and from a stream (Little Endian format).
*/
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define UINT24_TO_STREAM(p, u24) {*(p)++ = (uint8_t)(u24); *(p)++ = (uint8_t)((u24) >> 8); *(p)++ = (uint8_t)((u24) >> 16);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define INT8_TO_STREAM(p, u8)    {*(p)++ = (int8_t)(u8);}
#define ARRAY32_TO_STREAM(p, a)  {register int ijk; for (ijk = 0; ijk < 32;           ijk++) *(p)++ = (uint8_t) a[31 - ijk];}
#define ARRAY16_TO_STREAM(p, a)  {register int ijk; for (ijk = 0; ijk < 16;           ijk++) *(p)++ = (uint8_t) a[15 - ijk];}
#define ARRAY8_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < 8;            ijk++) *(p)++ = (uint8_t) a[7 - ijk];}
#define BDADDR_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk];}
#define LAP_TO_STREAM(p, a)      {register int ijk; for (ijk = 0; ijk < LAP_LEN;      ijk++) *(p)++ = (uint8_t) a[LAP_LEN - 1 - ijk];}
#define DEVCLASS_TO_STREAM(p, a) {register int ijk; for (ijk = 0; ijk < DEV_CLASS_LEN;ijk++) *(p)++ = (uint8_t) a[DEV_CLASS_LEN - 1 - ijk];}
#define ARRAY_TO_STREAM(p, a, len) {register int ijk; for (ijk = 0; ijk < len;        ijk++) *(p)++ = (uint8_t) a[ijk];}
#define REVERSE_ARRAY_TO_STREAM(p, a, len)  {register int ijk; for (ijk = 0; ijk < len; ijk++) *(p)++ = (uint8_t) a[len - 1 - ijk];}

#define STREAM_TO_UINT8(u8, p)   {u8 = (uint8_t)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)((uint16_t)*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT24(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) ); (p) += 3;}
#define STREAM_TO_UINT32(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); (p) += 4;}
#define STREAM_TO_BDADDR(a, p)   {register int ijk; register uint8_t *pbda = (uint8_t *)a + BD_ADDR_LEN - 1; for (ijk = 0; ijk < BD_ADDR_LEN; ijk++) *pbda-- = *p++;}
#define STREAM_TO_ARRAY32(a, p)  {register int ijk; register uint8_t *_pa = (uint8_t *)a + 31; for (ijk = 0; ijk < 32; ijk++) *_pa-- = *p++;}
#define STREAM_TO_ARRAY16(a, p)  {register int ijk; register uint8_t *_pa = (uint8_t *)a + 15; for (ijk = 0; ijk < 16; ijk++) *_pa-- = *p++;}
#define STREAM_TO_ARRAY8(a, p)   {register int ijk; register uint8_t *_pa = (uint8_t *)a + 7; for (ijk = 0; ijk < 8; ijk++) *_pa-- = *p++;}
#define STREAM_TO_DEVCLASS(a, p) {register int ijk; register uint8_t *_pa = (uint8_t *)a + DEV_CLASS_LEN - 1; for (ijk = 0; ijk < DEV_CLASS_LEN; ijk++) *_pa-- = *p++;}
#define STREAM_TO_LAP(a, p)      {register int ijk; register uint8_t *plap = (uint8_t *)a + LAP_LEN - 1; for (ijk = 0; ijk < LAP_LEN; ijk++) *plap-- = *p++;}
#define STREAM_TO_ARRAY(a, p, len) {register int ijk; for (ijk = 0; ijk < len; ijk++) ((uint8_t *) a)[ijk] = *p++;}
#define REVERSE_STREAM_TO_ARRAY(a, p, len) {register int ijk; register UINT8 *_pa = (uint8_t *)a + len - 1; for (ijk = 0; ijk < len; ijk++) *_pa-- = *p++;}


/*******************************************************************************
 **
 ** Function        btdl_utils_init
 **
 ** Description     General utilities initialization
 **
 ** Parameters      none
 **
 ** Returns         void
 **
 *******************************************************************************/
void btdl_utils_init(void);

/*******************************************************************************
 **
 ** Function        btdl_get_verbose_level
 **
 ** Description     Get the Verbose Level
 **
 ** Parameters      verbose level
 **
 ** Returns         status
 *******************************************************************************/
tBTDL_VERBOSE_LEVEL btdl_get_verbose_level(void);

/*******************************************************************************
 **
 ** Function        btdl_set_verbose_level
 **
 ** Description     Set the Verbose Level
 **
 ** Parameters      verbose level
 **
 ** Returns         status
 *******************************************************************************/
int btdl_set_verbose_level(tBTDL_VERBOSE_LEVEL verbose_level);

/*******************************************************************************
 **
 ** Function        btdl_print
 **
 ** Description     Print a message
 **
 ** Parameters      message level
 **                 Print format (same as printf)
 **
 ** Returns         void
 *******************************************************************************/
void btdl_print(tBTDL_VERBOSE_LEVEL level, const char *format, ...);

/*******************************************************************************
 **
 ** Function        btdl_dump_baseaddr
 **
 ** Description     Dump memory
 **
 ** Parameters      prefix: prefix string
 **                 buf: pointer on buffer to dump
 **                 size: size of the the buffer to dump (in byte)
 **                 baseaddr:base address
 **
 ** Returns         void
 *******************************************************************************/
void btdl_dump_baseaddr(tBTDL_VERBOSE_LEVEL message_level, const char *prefix, const uint8_t *buf, int size, uint32_t baseaddr);

/*******************************************************************************
 **
 ** Function        btdl_dump
 **
 ** Description     Dump memory
 **
 ** Parameters      prefix: prefix string
 **                 buf: pointer on buffer to dump
 **                 size: size of the the buffer to dump (in byte)
 **
 ** Returns         void
 **
 ** Note that this function calls btdl_dump_baseaddr with baseaddr set to 0
 *******************************************************************************/
void btdl_dump(tBTDL_VERBOSE_LEVEL message_level, const char *prefix, const uint8_t *buf, int size);

/*******************************************************************************
 **
 ** Function        btdl_sleep
 **
 ** Description     Sleep requested number of secondes
 **
 ** Parameters      duration: number of secondes to sleep
 **
 ** Returns         void
 **
 *******************************************************************************/
void btdl_sleep(int duration);

#endif /* BTDL_UTILS_H_ */
