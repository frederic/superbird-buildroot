/*
 *
 * btdl_hex.c
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

/* standard includes */
#include <stdio.h>
#include <stdlib.h>
/* for open */
#include <fcntl.h>
/* for read, write and close */
#include <unistd.h>

#include <string.h>
#include <stdbool.h>

#include "btdl_hex.h"

#include "btdl_utils.h"
#include "hci_cmd.h"
#include "hci_drv.h"

/* Item definitions for Static Section */
#define BTDL_HEX_SS_ITEM_IFPLL          0x01    /* IF_PLL Item */
#define BTDL_HEX_SS_ITEM_DS_VS          0x02    /* DataSection/VolatileSection item */
#define BTDL_HEX_SS_ITEM_IFPLL_LEN      0x08    /* IF_PLL Item length */
#define BTDL_HEX_SS_ITEM_BDADDR         0x40    /* BdAddr Item */
#define BTDL_HEX_SS_ITEM_BDADDR_LEN     0x06    /* BdAddr Item length */

/* Bulk send definitions */
#define ACL_CMD                 2
#define MAX_DL_FRAGMENT_SIZE    240

/*
 Global variables
*/
uint16_t DUMMY_ACL_CONNECTION_HANDLE = 0xBEEF;
extern tBTDL_CB btdl_cb;

/*
 * Local function
 */
static bool btdl_hex_is_static_section(tFLASH_DL_I32HEX *p_section);

/*******************************************************************************
 **
 ** Function        btdl_hex_read_hex_nibble
 **
 ** Description     Convert an ASCII char (representing an hexa number) to decimal
 **
 ** Parameters      c: ASCII input char
 **
 ** Returns         converted number (decimal)
 **
 *******************************************************************************/
uint8_t btdl_hex_read_hex_nibble(const uint8_t c)
{
    if ((c >='0') && (c <= '9'))
    {
        return c - '0';
    }
    else if ((c >='A') && (c <= 'F'))
    {
        return c - 'A' + 10;
    }
    else if ((c >='a') && (c <= 'f'))
    {
        return c - 'a' + 10;
    }
    else
    {
        puts("btdl_hex_read_hex_nibble: Error char not in supported range");
        return 0;
    }
}

/*******************************************************************************
 **
 ** Function        btdl_hex_read_hex_byte
 **
 ** Description     Convert two ASCII chars (representing an number) to decimal
 **
 ** Parameters      buf: pointer of the first of the two bytes
 **
 ** Returns         converted number (decimal)
 **
 *******************************************************************************/
uint8_t btdl_hex_read_hex_byte(const uint8_t *buf)
{
    uint8_t result;

    result = btdl_hex_read_hex_nibble(buf[0]) << 4;
    result += btdl_hex_read_hex_nibble(buf[1]);
    return result;
}



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
void btdl_hex_free_i32hex(tFLASH_DL_I32HEX *p_head)
{
    tFLASH_DL_I32HEX *p_current, *p_next;

    /* free all the allocated blocks */
    p_current = p_head;
    while (p_current != NULL)
    {
        /* if the current contiguous allocation */
        if (p_current->data != NULL)
        {
            free(p_current->data);
        }
        p_next = p_current->next;
        free(p_current);
        p_current = p_next;
    }
}

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
tFLASH_DL_I32HEX *btdl_hex_parse_i32hexfile(const char *fname, int nochksum)
{
    int fid, linenum, ret, i;
    uint32_t baseaddr = 0;
    uint8_t c, buf[8];
    uint8_t *tmpbuf = NULL;
    struct i32hex *current = NULL;
    struct i32hex *head = NULL;

    btdl_print(BTDL_DEBUG, "Parsing %s file", fname);

    fid = open(fname, O_RDONLY);
    if (fid < 0)
    {
        btdl_print(BTDL_ERROR, "open(%s) failed", fname);
        perror("open");
        return NULL;
    }

    /* initialize the variables */
    linenum = 1;
    do
    {
        int eol_char = 0;
        uint8_t chksum, len, type;
        uint16_t addr;

        /* chomp data until the next colon */
        do
        {
            /* read 1 char */
            ret = read(fid, &c, 1);
            if (ret == 0)
            {
                /* end of file reached */
                goto btdl_hex_parse_i32hexfile_eof;
            }
            else if (ret < 0)
            {
                puts("read failed while looking for colon");
                goto btdl_hex_parse_i32hexfile_error;
            }
            if (strchr("$: \t\r\n", c) == NULL)
            {
                btdl_print(BTDL_ERROR, "Unsupported char at line %d", linenum);
                goto btdl_hex_parse_i32hexfile_error;
            }
            else
            {
                if (c == '\r')
                {
                    eol_char = 1;
                    linenum++;
                }
                else if (c == '\n')
                {
                    if (!eol_char)
                    {
                        linenum++;
                    }
                    eol_char = 0;
                }
                else
                {
                    eol_char = 0;
                }
            }
        } while (c != ':');

        /* read the header */
        ret = read(fid, buf, 8);
        if (ret <= 0)
        {
            puts("read failed while reading header");
            goto btdl_hex_parse_i32hexfile_error;
        }
        len = btdl_hex_read_hex_byte(buf);
        addr = (btdl_hex_read_hex_byte(&buf[2]) << 8) + btdl_hex_read_hex_byte(&buf[4]);
        type = btdl_hex_read_hex_byte(&buf[6]);

        /* initialize the checksum */
        chksum  = btdl_hex_read_hex_byte(buf);
        chksum += btdl_hex_read_hex_byte(buf+2);
        chksum += btdl_hex_read_hex_byte(buf+4);
        chksum += btdl_hex_read_hex_byte(buf+6);

        /* only allocate a buffer if the length is != 0 */
        if (len != 0)
        {
            /* allocate the right number of bytes for the current record */
            tmpbuf = malloc(len);
            if (tmpbuf == NULL)
            {
                puts("failed to allocate temporary buffer");
                goto btdl_hex_parse_i32hexfile_error;
            }
            /* read the data */
            for (i = 0; i < len; i++)
            {
                /* read the next byte */
                ret = read(fid, buf, 2);
                if (ret <= 0)
                {
                    puts("read failed while reading data");
                    goto btdl_hex_parse_i32hexfile_error;
                }
                tmpbuf[i] = btdl_hex_read_hex_byte(buf);
                chksum += btdl_hex_read_hex_byte(buf);
            }
        }
        /* read the checksum */
        ret = read(fid, buf, 2);
        if (ret <= 0)
        {
            puts("read failed while reading checksum");
            goto btdl_hex_parse_i32hexfile_error;
        }
        chksum += btdl_hex_read_hex_byte(buf);
        if ((!nochksum) && chksum)
        {
            btdl_print(BTDL_ERROR, "checksum error at line %d", linenum);
            goto btdl_hex_parse_i32hexfile_error;
        }

        switch(type)
        {
        case 0:
            /* if the length is equal to zero, do not do anything */
            if (len != 0)
            {
                /* check if this is the first element allocated */
                if (current == NULL)
                {
                    current = malloc(sizeof(*current));
                    if (current == NULL)
                    {
                        puts("failed allocating first buffer");
                        goto btdl_hex_parse_i32hexfile_error;
                    }
                    current->addr = baseaddr + addr;
                    current->length = len;
                    current->data = tmpbuf;
                    current->next = NULL;
                    current->linenum = linenum;

                    /* save the head of list */
                    head = current;
                }
                else
                {
                    /* check if the record is right after last record (to just append) */
                    if ((current->addr + current->length) == (baseaddr + addr))
                    {
                        uint8_t *newbuf;
                        newbuf = malloc(current->length + len);
                        if (newbuf == NULL)
                        {
                            puts("failed allocating new buffer");
                            goto btdl_hex_parse_i32hexfile_error;
                        }
                        memcpy(newbuf, current->data, current->length);
                        memcpy(&newbuf[current->length], tmpbuf, len);
                        /* free the previously allocated chunk */
                        free(current->data);
                        current->data = newbuf;
                        current->length += len;
                        /* free the data allocated for the current record */
                        free(tmpbuf);
                    }
                    else
                    {
                        /* allocate a new memory area */
                        current->next = malloc(sizeof(*current));
                        if (current->next == NULL)
                        {
                            puts("failed allocating new record");
                            goto btdl_hex_parse_i32hexfile_error;
                        }
                        current = current->next;
                        current->addr = baseaddr + addr;
                        current->length = len;
                        current->data = tmpbuf;
                        current->next = NULL;
                        current->linenum = linenum;
                    }
                }
                /* allocated temporary buffer has been used */
                tmpbuf = NULL;
            }
            /* check if the address is located after the previous allocation */
            break;
        case 1:
            if (tmpbuf != NULL)
            {
                free(tmpbuf);
                tmpbuf = NULL;
            }
            /* end of file record, simply exit */
            goto btdl_hex_parse_i32hexfile_eof;
            break;
        case 2:
            if (tmpbuf != NULL)
            {
                free(tmpbuf);
                tmpbuf = NULL;
            }
            /* not supported */
            puts("Unsupported type 2 record");
            goto btdl_hex_parse_i32hexfile_error;
            break;
        case 3:
            if (tmpbuf != NULL)
            {
                free(tmpbuf);
                tmpbuf = NULL;
            }
            /* not supported */
            puts("Unsupported type 3 record");
            goto btdl_hex_parse_i32hexfile_error;
            break;
        case 4:
            if (len == 2)
            {
                baseaddr = (tmpbuf[0] << 24) + (tmpbuf[1] << 16);
            }
            if (tmpbuf != NULL)
            {
                free(tmpbuf);
                tmpbuf = NULL;
            }
            if (len != 2)
            {
                puts("type 4 does not have appropriate number of bytes");
                goto btdl_hex_parse_i32hexfile_error;
            }
            break;
        case 5:
            if (tmpbuf != NULL)
            {
                free(tmpbuf);
                tmpbuf = NULL;
            }
            /* not supported */
            puts("Unsupported type 5 record");
            goto btdl_hex_parse_i32hexfile_error;
            break;
        default:
            if (tmpbuf != NULL)
            {
                free(tmpbuf);
                tmpbuf = NULL;
            }
            btdl_print(BTDL_ERROR, "Unknown type at line %d", linenum);
            goto btdl_hex_parse_i32hexfile_error;
            break;
        }
    } while (1);

btdl_hex_parse_i32hexfile_eof:
    close(fid);
    /* debug */
    if (btdl_get_verbose_level() == BTDL_FULL_DEBUG)
    {
        current = head;
        i = 0;
        while (current != NULL)
        {
            btdl_print(BTDL_FULL_DEBUG, "Segment %d: @0x%08X (line %d)", i++, current->addr, current->linenum);
            btdl_dump_baseaddr(BTDL_FULL_DEBUG, NULL, current->data, current->length, current->addr);
            current = current->next;
        }
    }
    return head;

btdl_hex_parse_i32hexfile_error:
    /* close the file */
    close(fid);

    /* free the current record if allocated */
    if (tmpbuf != NULL)
    {
        free(tmpbuf);
        tmpbuf = NULL;
    }

    /* in case of error, free all the allocated buffers */
    btdl_hex_free_i32hex(head);
    return NULL;
}

/*******************************************************************************
 **
 ** Function        btdl_hex_extract_static_section
 **
 ** Description     Search and extract (unlink) the Static Section from
 **                 an Hex chained list
 **
 ** Parameters      pp_head: IN/OUT head of the chained list
 **
 ** Returns         pointer on the extracted section
 **
 *******************************************************************************/
struct i32hex *btdl_hex_extract_static_section(tFLASH_DL_I32HEX **pp_head)
{
    struct i32hex *p_previous = NULL;
    struct i32hex *p_current;
    struct i32hex *p_extract;

    btdl_print(BTDL_DEBUG, "Extracting Static Section");

    if ((pp_head == NULL) || (*pp_head == NULL))
    {
        btdl_print(BTDL_ERROR,  "btdl_hex_extract_section bad header pointer");
        return NULL;
    }
    p_current = *pp_head;

    /* For every Section */
    while (p_current != NULL)
    {
        /* If this section is a Static Section */
        if (btdl_hex_is_static_section(p_current))
        {
            /* Save reference to the section to extract */
            p_extract = p_current;
            /* If this is not the first section => link the previous to the next */
            if (p_previous)
            {
                p_previous->next = p_current->next;
            }
            /* If this is the first section => update head */
            else
            {
                *pp_head = p_current->next;
            }
            /* The extracted section contains only one section */
            p_extract->next = NULL;
            btdl_print(BTDL_DEBUG, "Static Section extracted addr=0x%08X", p_extract->addr);
            return p_extract;
        }

        /* current section become previous section */
        p_previous = p_current;

        /* move to the next memory area */
        p_current = p_current->next;
    }
    btdl_print(BTDL_ERROR,  "Section Not found");
    return NULL;
}

/*******************************************************************************
 **
 ** Function        btdl_hex_is_static_section
 **
 ** Description     Check if a section is a Static Section (look if SS Items present)
 **
 ** Parameters      p_section: Hex section
 **
 ** Returns         true if this section is a Static Section, false otherwise.
 **
 *******************************************************************************/
static bool btdl_hex_is_static_section(tFLASH_DL_I32HEX *p_section)
{
    uint8_t *p_data;
    uint32_t length;
    bool bdaddr_item = false;

    if (p_section == NULL)
    {
        btdl_print(BTDL_ERROR, "NULL section pointer");
        return false;
    }

    length = p_section->length;
    p_data = p_section->data;

    if (p_data == NULL)
    {
        btdl_print(BTDL_ERROR, "No data");
        return false;
    }
    if (length == 0)
    {
        btdl_print(BTDL_ERROR, "Null length");
        return false;
    }

    /* Sanity: check if the first item is IF_PLL */
    if ((p_data[0] != BTDL_HEX_SS_ITEM_IFPLL) ||        /* Item Id must be 0x01 */
        (p_data[1] != BTDL_HEX_SS_ITEM_IFPLL_LEN) ||    /* Item Length must be 0x08 */
        ((p_data[1] + 3) > length))                     /* buffer too small */
    {
        btdl_print(BTDL_DEBUG, "IF_PLL item not found => this is not a Static Section");
        return false;
    }

    /* Jump item_id, length, status and data */
    length -= p_data[1] + 3;
    p_data += p_data[1] + 3;

    while(length)
    {
        switch(p_data[0])
        {
        case BTDL_HEX_SS_ITEM_BDADDR:
            if (p_data[1] == BTDL_HEX_SS_ITEM_BDADDR_LEN)
            {
                btdl_print(BTDL_DEBUG, "BdAddr item found");
                bdaddr_item = true;
            }
            else
            {
                btdl_print(BTDL_ERROR, "Bad BdAddr length=%d => this is not a Static Section", p_data[1]);
                return false;
            }
            break;
        case BTDL_HEX_SS_ITEM_DS_VS:
            btdl_print(BTDL_DEBUG, "DS/VS item found");
            break;
        default:
            btdl_print(BTDL_DEBUG, "Unknown idem=0x%x", p_data[0]);
            break;
        }

        if ((p_data[1] + 3) > length)
        {
            btdl_print(BTDL_ERROR, "Bad item length=%d => this is not a Static Section", p_data[1]);
            return false;
        }
        /* Jump item_id, length, status and data */
        length -= p_data[1] + 3;
        p_data += p_data[1] + 3;
    }

    if (bdaddr_item)
    {
        return true;
    }
    return false;
}

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
int btdl_hex_overwrite_bdaddr(tFLASH_DL_I32HEX *p_section, uint8_t *p_bd_addr)
{
    int length;
    uint8_t *p_data, *p;

    btdl_print(BTDL_DEBUG, "Overwriting BdAddr (in SS) with: %02X:%02X:%02X:%02X:%02X:%02X",
            p_bd_addr[0], p_bd_addr[1], p_bd_addr[2],
            p_bd_addr[3], p_bd_addr[4], p_bd_addr[5]);

    if (p_section == NULL)
    {
        btdl_print(BTDL_ERROR, "This is not a Static Section");
        return -1;
    }

    length = p_section->length;
    p_data = p_section->data;

    /* Sanity: check that it's a Static Section (must start with IF_PLL tuple) */
    if ((p_data[0] != BTDL_HEX_SS_ITEM_IFPLL) ||  /* Item Id must be 0x01 */
        (p_data[1] != BTDL_HEX_SS_ITEM_IFPLL_LEN))    /* Length must be 0x08 */
    {
        btdl_print(BTDL_ERROR, "IF_PLL item not found => this is not a Static Section");
        return -1;
    }

    while(length)
    {
        /* If this item is a BdAddr and length is 6 bytes */
        if ((p_data[0] == BTDL_HEX_SS_ITEM_BDADDR) &&
            (p_data[1] == BTDL_HEX_SS_ITEM_BDADDR_LEN))
        {
            /* Update BdAddr (swap bytes) */
            p = &p_data[3];
            BDADDR_TO_STREAM(p, p_bd_addr);
            return 0;
        }
        /* If it's not a BdAddt item, jump to next one */
        /* jump item_id, length, status and data */
        length -= p_data[1] + 3;
        p_data += p_data[1] + 3;
    }
    btdl_print(BTDL_ERROR, "BdAddr item not found");
    return -1;
}

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
int btdl_hex_check(tFLASH_DL_I32HEX *head, uint8_t chip_select)
{
    int rv;
    struct i32hex *current = head;
    uint8_t buf[HCI_CMD_READ_RAM_SIZE_MAX];

    /* loop while there are flash content */
    while (current != NULL)
    {
        uint32_t addr;
        uint32_t remaining;

        addr = current->addr;
        remaining = current->length;

        while (remaining > 0)
        {
            uint8_t len;

            /* check if we can request a full buffer */
            if (remaining > HCI_CMD_READ_RAM_SIZE_MAX)
            {
                len = HCI_CMD_READ_RAM_SIZE_MAX;
            }
            else
            {
                len = remaining;
            }

            /* read ram and wait for confirmation */
            rv = hci_cmd_read_ram(addr | (chip_select << 24), buf, len);

            /* check the response length (HCI command status is counted in) */
            if (rv != len)
            {
                btdl_print(BTDL_ERROR,  "HCI response length wrong (%d != %d)", rv-1, len);
                return -1;
            }
            /* compare the data returned by the chip (after status byte) */
            if (memcmp(buf, &current->data[addr - current->addr], len) != 0)
            {
                btdl_print(BTDL_ERROR,  "Flash verify failed");
                return -1;
            }

            /* decrement the remaining */
            remaining -= len;
            addr += len;
        }

        /* move to the next memory area */
        current = current->next;
    }
    return 0;
}

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
int btdl_hex_download(tFLASH_DL_I32HEX *head, uint8_t chip_select)
{
    int rv;
    struct i32hex *current = head;
    uint8_t buf[256];

    while (current != NULL)
    {
        uint32_t addr;
        uint32_t remaining;
        addr = current->addr;
        remaining = current->length;

        while (remaining > 0)
        {
            uint8_t len;

            /* Check if we can transmit a full buffer */
            if (remaining > HCI_CMD_WRITE_RAM_SIZE_MAX)
            {
                len = HCI_CMD_WRITE_RAM_SIZE_MAX;
            }
            else
            {
                len = remaining;
            }
            memcpy(buf, &current->data[addr - current->addr], len);

            /* write ram and wait for confirmation */
            rv = hci_cmd_write_ram(addr | (chip_select << 24), buf, len);
            if (rv < 0)
            {
                btdl_print(BTDL_ERROR,  "HCI Command failed");
                return -1;
            }
            /* decrement the remaining */
            remaining -= len;
            addr += len;
        }

        /* move to the next memory area */
        current = current->next;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        btdl_hex_bulk_cmd_send
 **
 ** Description     Send bulk Command
 **
 ** Parameters      params, params_size
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hex_bulk_cmd_send (const uint8_t *params, uint32_t params_size)
{
    uint8_t     buf[0x2000];
    uint8_t *   p = buf;
    int         rv;
    int         nExtraSize = 0;

    /* build the command header */
    UINT8_TO_STREAM(p, ACL_CMD);

    if (btdl_cb.bNeedDummyACLHandle)
    {
        /* Add dummy ACL connection handle and payload length */
        UINT16_TO_STREAM(p, DUMMY_ACL_CONNECTION_HANDLE);
        UINT16_TO_STREAM(p, (uint16_t) params_size);
        nExtraSize = 4;
    }

    ARRAY_TO_STREAM(p, params, params_size);

    /* Send the command */
    rv = hci_drv_write(buf, params_size + 1 + nExtraSize);

    return rv;
}

/*******************************************************************************
 **
 ** Function        btdl_hex_wait_for_download_ack
 **
 ** Description     Wait for download acknowledgement
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hex_wait_for_download_ack()
{
    /*
        Waiting for byte sequence
        04 0E 08 01 14 0C 00 42 54 44 4C
        from the firmware

     */ 
    uint8_t buf[HCI_CMD_SIZE_MAX];
    uint8_t *p = buf;
    int length;
    int rv = -1;
    uint8_t hci_h4_hdr, hci_evt, hci_num, hci_len, hci_status;
    uint16_t hci_opcode;
    uint16_t EXPECTED_OPCODE = 0x0C14;
    uint8_t c;
    int     j;
    const char* EXPECTED_CHARS = "BTDL";

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
            (hci_opcode == EXPECTED_OPCODE))                 /* Check OpCode */
        {
            /* Compute the length of the HCI Parameters (skip NumCmd and Opcode) */
            length = hci_len - sizeof(uint8_t) - sizeof(uint16_t);

            /* Extract HCI Status */
            STREAM_TO_UINT8(hci_status, p);
            length--;

            /* Check the HCI Status */
            if (hci_status == HCI_CMD_STATUS_OK)
            {
                if (length == 4)
                {
                    /* Assume correct values (4 letters) */
                    rv = 4;

                    /* The return data hex byte sequence should be 42 54 44 4C */
                    /* (ASCII codes of letters B T D L) */

                    for (j = 0; (j < 4) && (rv > 0); j++)
                    {
                        /* Extract next char */
                       STREAM_TO_UINT8(c, p);
                        if (c != EXPECTED_CHARS [j])
                            rv = -999;
                    }
                }
            }
            else
            {
                /* The HCI error is positive. Let's make it negative (unix like) */
                rv = 0 - 100 - hci_status;
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
 ** Function        btdl_hex_bulk_send_start_stop_dl
 **
 ** Description     Bulk start or stop download
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hex_bulk_send_start_stop_dl (int bStart)
{
    uint8_t buf[200];
    uint8_t *p = buf;
    int status;
    uint32_t dwSignal = bStart ? 0xFFFFFFFE : 0xFFFFFFFF;

    if (!bStart)
        btdl_print(BTDL_PRINT, "btdl_hex_bulk_send_start_stop_dl to STOP");
    else
        btdl_print(BTDL_PRINT, "btdl_hex_bulk_send_start_stop_dl to START");

    UINT32_TO_STREAM(p, dwSignal);

    status = btdl_hex_bulk_cmd_send (buf, 4);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "btdl_hex_bulk_send_start_stop_dl return:%d", status);
        return status;
    }

    if (bStart)
    {
        if (!btdl_cb.bWaitForDLAck)
            usleep (200000); /* Sleep for 0.2 seconds */
        else
        {
            status = btdl_hex_wait_for_download_ack();
            if (status < 0)
            {
                btdl_print(BTDL_ERROR, "btdl_hex_bulk_send_start_stop_dl failed to receive start ack   return:%d", status);
                return status;
            }
        }
    }

    btdl_print(BTDL_DEBUG, "btdl_hex_bulk_send_start_stop_dl Done");
    return 0;
}

/*******************************************************************************
 **
 ** Function        btdl_hex_bulk_download
 **
 ** Description     Bulk hex firmware download
 **
 ** Parameters      pointer to burn image
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hex_bulk_download(tFLASH_DL_I32HEX *head)
{
    struct i32hex *current = head;
    uint8_t buf[8 + MAX_DL_FRAGMENT_SIZE];
    uint8_t * p;
    uint32_t addr;
    uint32_t remaining;
    uint16_t len;
    int status;

    btdl_print(BTDL_PRINT, "btdl_hex_bulk_download");

    while (current != NULL)
    {
        addr = current->addr;
        remaining = current->length;

        while (remaining > 0)
        {
            if (btdl_cb.delay_between_bulk_writes_ms)
                usleep (btdl_cb.delay_between_bulk_writes_ms * 1000);

            /* Check if we can transmit a full buffer */
            if (remaining > MAX_DL_FRAGMENT_SIZE)
            {
                len = MAX_DL_FRAGMENT_SIZE;
            }
            else
            {
                len = remaining;
            }

            btdl_print (BTDL_PRINT, "About to download %d bytes starting at 0x%08X",
                        len, addr);

            p = buf;

            UINT32_TO_STREAM(p, addr + btdl_cb.AddressBase);
            UINT16_TO_STREAM(p, len);
            //ARRAY_TO_STREAM(p, (uint8_t*)(current->data + (addr - current->addr)), len);
            ARRAY_TO_STREAM(p, (uint8_t *)(current->data + (addr - current->addr)), len);


            status = btdl_hex_bulk_cmd_send (buf, 6 +  (uint32_t) len);

            btdl_print (BTDL_PRINT, "btdl_hex_bulk_cmd_send returned status=%d", status);

            if (status < 0)
                return status;

            /* decrement the remaining */
            remaining -= len;
            addr += len;
        }

        /* move to the next memory area */
        current = current->next;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        btdl_hex_firmware_reload
 **
 ** Description     Firmware reload
 **
 ** Parameters      pointer to burn image
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hex_firmware_reload(tFLASH_DL_I32HEX *p_burn_img)
{
    int status;

    /* First send 0xFFFFFFFF over bulk endpoint */
    status = btdl_hex_bulk_send_start_stop_dl (1);
    if (status < 0)
        return status;

    status = btdl_hex_bulk_download (p_burn_img);
    if (status < 0)
        return status;

    status = btdl_hex_bulk_send_start_stop_dl (0);
    if (status < 0)
        return status;

    return 0;
}

