/*
 *
 * btdl_utils.c
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

/* For printf */
#include <stdio.h>

/* For sleep */
#include <unistd.h>

/* For varargs */
#include <stdarg.h>

#include "btdl_utils.h"

typedef struct
{
    tBTDL_VERBOSE_LEVEL verbose_level;
} tBTDL_UTILS_CB;

/* Globals */

tBTDL_UTILS_CB btdl_utils_cb;

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
void btdl_utils_init(void)
{
    btdl_utils_cb.verbose_level = BTDL_PRINT;
}

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
void btdl_sleep(int duration)
{
    btdl_print(BTDL_DEBUG, "Sleep for %d seconds...\n", duration);
    sleep(duration);
}

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
int btdl_set_verbose_level(tBTDL_VERBOSE_LEVEL verbose_level)
{
    if (verbose_level <= BTDL_FULL_DEBUG)
    {
        btdl_utils_cb.verbose_level = verbose_level;
        return 0;
    }
    else
    {
        btdl_print(BTDL_ERROR, "Bad verbose level:%d", verbose_level);
        return -1;
    }
}

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
tBTDL_VERBOSE_LEVEL btdl_get_verbose_level(void)
{
    return btdl_utils_cb.verbose_level;
}

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
void btdl_print(tBTDL_VERBOSE_LEVEL message_level, const char *format, ...)
{
    va_list ap;

    /* Print message if it's an error message or if its below verbose level */
    if (message_level <= btdl_utils_cb.verbose_level)
    {
        /* Error messages have a prefix */
        if (message_level == BTDL_ERROR)
        {
            printf("ERROR: ");
        }
        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);
        printf("\n");
    }
}

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
void btdl_dump_baseaddr(tBTDL_VERBOSE_LEVEL message_level, const char *prefix, const uint8_t *buf, int size, uint32_t baseaddr)
{
    int i;

    /* Print message if it's an error message or if its below verbose level */
    if (message_level <= btdl_utils_cb.verbose_level)
    {
        for (i = 0; i < size; i++)
        {
            if ((i%16) == 0)
            {
                if (prefix)
                    printf("%s0x%08X :", prefix, baseaddr+i);
                else
                    printf("0x%08X :", baseaddr+i);
            }
            printf(" %02X", buf[i]);
            if ((i%16) == 15)
            {
                fputs("\n", stdout);
            }
        }
        fputs("\n", stdout);
    }
}

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
void btdl_dump(tBTDL_VERBOSE_LEVEL message_level, const char *prefix, const uint8_t *buf, int size)
{
    /* Print message if it's an error message or if its below verbose level */
    if (message_level <= btdl_utils_cb.verbose_level)
    {
        btdl_dump_baseaddr(message_level, prefix, buf, size, 0);
    }
}

