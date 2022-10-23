/*
 *
 * btdl_main.c
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
 * Main function of the tool used to download a new content onto a BT controller
 *
 */

/* standard includes */
#include <stdio.h>
#include <stdlib.h>

/* for argument management */
#define _GNU_SOURCE
#include <getopt.h>

#include <string.h>
#include <unistd.h>

#include "btdl_utils.h"
#include "btdl_hex.h"
#include "hci_cmd.h"
#include "hci_drv.h"


/* Defines */

/* Chip Select values */
#define FLASH_DL_CS_RAM                 0x00        /* RAM Chip Select */
#define FLASH_DL_CS_P_FLASH             0xFC        /* Parallel Flash Chip Select */
#define FLASH_DL_CS_S_FLASH             0xFF        /* Serial Flash Chip Select */

/* Shared USB Address Define */
#define SHARED_USB_ADDRESS_CHIP_ERASE   0xffffffff  /* Shared USB Chip Erase Address */

/*
 * Global variables
 */
tBTDL_CB btdl_cb;

/*
 * Local functions
 */



/*******************************************************************************
 **
 ** Function        usage
 **
 ** Description     Print help information
 **
 ** Parameters      none
 **
 ** Returns         void
 **
 *******************************************************************************/
void usage(void)
{
    puts("Name:");
    puts("    btdl - download a new firmware into the BT controller device");
    puts("Usage:");
    puts("    btdl [parameters] [options]");
    puts("Description:");
    puts("    This application does the following operations:");
    puts("        1- read the current BD address of the device");
    puts("        2- reset the device");
    puts("        3- download the minidriver on the device");
    puts("        4- launch the minidriver");
    puts("        5- download the flash on the device");
    puts("        6- reboot the device");
    puts("        7- Re-write the BdAddr (or write a new one)");
    puts("    At the end of the flash download process, the driver is likely to");
    puts("    have restarted.");
    puts("Parameters:");
    puts("    --device DEVICE      BT USB device to download the firmware on");
    puts("    --minidrv DRV.hex    Minidriver to download on the chip to handle");
    puts("                         the flash update process");
    puts("    --image IMAGE.hex    Flash image to burn");
    puts("    --type [serial | parallel] Select Non Volatile Memory type");
    puts("Options:");
    puts("    --bdaddr BDADDR      BdAddr to write after download (format: 0123456789ab):");
    puts("    --verbose VERBOSE    indicate the verbose level:");
    puts("                            1 => Minimum (default)");
    puts("                            2 => Progress Information");
    puts("                            3 => Debug");
    puts("                            4 => Full Debug");
    puts("    --nodl               Just parse the files and print the BD address");
    puts("    --nochksum           Do not perform checksum when parsing files");
    puts("    --sharedusb          perform WiFi/BT shared USB FW Download");
    puts("    --failsafe           special FW Download method to 43569");
    puts("    --delayattheend DELAYMS      Amount of time to delay before terminating the program");
    puts("    --delaywrites   DELAYMS      Amount of time to delay between bulk writes");
    puts("    --addressbase   ADDRESS      Hexadecimal base value (without 0x) to add to RAM addresses");
    puts("    --noACLconnh                 DO NOT insert a dummy 2-byte connection handle and");
    puts("                                 2-byte payload length after the ACL type byte when building");
    puts("                                 bulk endpoint packets for dynamic firmware downloading");
    puts("                                 (by default, without this argument, a special ACL handle 0xBEEF is inserted)");
    puts("    --waitforkey                 A test option that waits for a keystroke AFTER opening a USB port");
    puts("                                 before starting to transfer bytes");
    puts("    --nodlack                    Do not wait for acknowledgement from the firmware that it is ready to");
    puts("                                 perform bluetooth firmware RAM download");
    puts("    --nominidrv                  Do not download Minidriver (required for some chips)");
}

/*******************************************************************************
 **
 ** Function        parse_help
 **
 ** Description     Parse help parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_help(char *optarg)
{
    usage();
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_nochksum
 **
 ** Description     Parse nochksum parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_nochksum(char *optarg)
{
    btdl_cb.nochksum = 1;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_nodl
 **
 ** Description     Parse nodl (No download) parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_nodl(char *optarg)
{
    btdl_cb.nodl = 1;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_verbose
 **
 ** Description     Parse verbose parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_verbose(char *optarg)
{
    int verbose = atoi(optarg);

    return btdl_set_verbose_level(verbose);
}

/*******************************************************************************
 **
 ** Function        parse_device
 **
 ** Description     Parse device parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_device(char *optarg)
{
    btdl_cb.device = optarg;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_minidrv
 **
 ** Description     Parse minidrv parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_minidrv(char *optarg)
{
    btdl_cb.minidriver = optarg;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_burnimage
 **
 ** Description     Parse burnimage parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_burnimage(char *optarg)
{
    btdl_cb.burnimage = optarg;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_bdaddr
 **
 ** Description     Parse bdaddr parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_bdaddr(char *optarg)
{
    btdl_cb.is_bd_addr_requested = 1;
    int tmp_bd_addr[6];

    memset(tmp_bd_addr, 0, sizeof(tmp_bd_addr));
    sscanf(optarg, "%02x%02x%02x%02x%02x%02x",
        &tmp_bd_addr[0], &tmp_bd_addr[1], &tmp_bd_addr[2],
        &tmp_bd_addr[3], &tmp_bd_addr[4], &tmp_bd_addr[5]);
    btdl_cb.bd_addr_requested[0] = (uint8_t)tmp_bd_addr[0];
    btdl_cb.bd_addr_requested[1] = (uint8_t)tmp_bd_addr[1];
    btdl_cb.bd_addr_requested[2] = (uint8_t)tmp_bd_addr[2];
    btdl_cb.bd_addr_requested[3] = (uint8_t)tmp_bd_addr[3];
    btdl_cb.bd_addr_requested[4] = (uint8_t)tmp_bd_addr[4];
    btdl_cb.bd_addr_requested[5] = (uint8_t)tmp_bd_addr[5];
    btdl_print(BTDL_INFO, "BdAddr Requested:%02X:%02X:%02X:%02X:%02X:%02X",
            btdl_cb.bd_addr_requested[0], btdl_cb.bd_addr_requested[1],
            btdl_cb.bd_addr_requested[2], btdl_cb.bd_addr_requested[3],
            btdl_cb.bd_addr_requested[4], btdl_cb.bd_addr_requested[5]);
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_delayend
 **
 ** Description     Parse delayed parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_delayend(char *optarg)
{
    int     n;

    if ((sscanf (optarg, "%d", &n) == 1) && (n >= 0))
    {
        btdl_cb.delay_at_the_end_ms = (unsigned long) n;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_delaywrites
 **
 ** Description     Parse delaywrites parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_delaywrites(char *optarg)
{
    int     n;

    if ((sscanf (optarg, "%d", &n) == 1) && (n >= 0))
    {
        btdl_cb.delay_between_bulk_writes_ms = (unsigned long) n;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_addressbase
 **
 ** Description     Parse addressbase parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_addressbase(char *optarg)
{
    unsigned    long    n;

    if (sscanf(optarg, "%lX", &n) == 1)
    {
        btdl_cb.AddressBase = n;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_noACLconnh
 **
 ** Description     Parse noACLconnh parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_noACLconnh (char *optarg)
{
    btdl_cb.bNeedDummyACLHandle = 0;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_waitforkey
 **
 ** Description     Parse waitforkey parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_waitforkey (char *optarg)
{
    btdl_cb.bWaitForKey = 1;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_nodlack
 **
 ** Description     Parse nodlack parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_nodlack (char *optarg)
{
    btdl_cb.bWaitForDLAck = 0;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_mem_type
 **
 ** Description     Parse Memory Type parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_mem_type(char *optarg)
{
    if (strcmp(optarg, "serial") == 0)
    {
        btdl_print(BTDL_INFO, "Serial Flash/Eeprom selected");
        btdl_cb.chip_select = FLASH_DL_CS_S_FLASH;
        btdl_cb.mem_type_param = 1;
    }
    else if (strcmp(optarg, "parallel") == 0)
    {
        btdl_print(BTDL_INFO, "Parallel Flash/Eeprom selected");
        btdl_cb.chip_select = FLASH_DL_CS_P_FLASH;
        btdl_cb.mem_type_param = 1;
    }
    else if (strcmp(optarg, "RAM") == 0)
    {
        btdl_print(BTDL_INFO, "RAM selected");
        btdl_cb.chip_select = FLASH_DL_CS_RAM;
        btdl_cb.mem_type_param = 1;
    }
    else
    {
        btdl_print(BTDL_ERROR, "Unknown Flash/Eeprom type:%d", optarg);
        return 1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_sharedusb
 **
 ** Description     parse sharedusb parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_sharedusb(char *optarg)
{
    btdl_cb.is_shared_usb = 1;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_failsafe
 **
 ** Description     parse failsafe parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_failsafe(char *optarg)
{
    btdl_cb.is_failsafe = 1;
    return 0;
}

/*******************************************************************************
 **
 ** Function        parse_nominidrv
 **
 ** Description     parse nominidrv parameter
 **
 ** Parameters      argument following the option
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_nominidrv(char *optarg)
{
    btdl_cb.nominidrv = 1;
    return 0;
}


/*******************************************************************************
 **
 ** Function        parse_cmd_line
 **
 ** Description     Main function used to parse the command line parameters
 **
 ** Parameters      argc: number of arguments
 **                 argv: the arguments
 **
 ** Returns         status
 **
 *******************************************************************************/
int parse_cmd_line(int argc, char **argv)
{
    int c;
    int status;

    typedef int (*PFI)();

    PFI parse_param[] =
    {
        parse_help,
        parse_device,
        parse_minidrv,
        parse_burnimage,
        parse_bdaddr,
        parse_nodl,
        parse_nochksum,
        parse_verbose,
        parse_mem_type,
        parse_sharedusb,
        parse_failsafe,
        parse_delayend,
        parse_delaywrites,
        parse_addressbase,
        parse_noACLconnh,
        parse_waitforkey,
        parse_nodlack,
        parse_nominidrv
    };

    while (1)
    {
        int option_index = 0;

        const struct option long_options[] =
        {
         {"help", 0, 0, 0},     /* Help => no parameter */
         {"device", 1, 0, 0},   /* device  => 1 parameter */
         {"minidrv", 1, 0, 0},  /* Minidriver => 1 parameter */
         {"image", 1, 0, 0},    /* Image => 1 parameter */
         {"bdaddr", 1, 0, 0},   /* BdAddr parameter => 1 parameter */
         {"nodl", 0, 0, 0},     /* No Download => 0 parameter */
         {"nochksum", 0, 0, 0}, /* No Ckecksum => 0 parameter */
         {"verbose", 1, 0, 0},  /* Verbose => 1 parameter */
         {"type", 1, 0, 0},     /* Memory Type => 1 parameter */
         {"sharedusb", 0, 0, 0}, /* Is this shared USB => 0 parameter */
         {"failsafe", 0, 0, 0}, /* failsafe FW update => 0 parameter */
         {"delayattheend", 1, 0, 0},/*  => 1 parameter */
         {"delaywrites", 1, 0, 0},  /*  => 1 parameter */
         {"addressbase", 1, 0, 0},  /*  => 1 parameter */
         {"noACLconnh", 0, 0, 0},   /* No ACL connection handle => 0 parameter */
         {"waitforkey", 0, 0, 0},   /* Wait for keypress after opening USB  => 0 parameter */
         {"nodlack", 0, 0, 0},      /* Do not wait for start download ack => 0 parameter */
         {"nominidrv", 0, 0, 0},    /* NoMinidriver => 0 parameter */
         {0, 0, 0, 0}
        };

        c = getopt_long_only(argc, argv, "", long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 0:
            if (optarg)
            {
                btdl_print(BTDL_INFO, "Option [%s] with arg [%s]",
                        long_options[option_index].name, optarg);
            }
            else
            {
                btdl_print(BTDL_INFO, "Option [%s]", long_options[option_index].name);
            }

            /* Call the function in charge or this option */
            status = (*parse_param[option_index])(optarg);
            if (status < 0)
            {
                return status;
            }
        break;

        default:
            return -1;
            break;
        }
    }

    if (optind < argc)
    {
        if (optind < argc)
        {
            btdl_print(BTDL_ERROR, "Unknown parameter %s", argv[optind]);
            return -1;
        }
        btdl_print(BTDL_INFO, "\n");
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        btdl_hw_reset
 **
 ** Description     Hardware reset of the BT chip (LaunchRam, reopen HCI, SW reset)
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int btdl_hw_reset(char *device)
{
    int status = 0;

    btdl_print(BTDL_DEBUG, "Reseting BT chip");

    /* restart from 0 to reboot */
    status = hci_cmd_launch_ram(0x00000000);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while resetting the BT controller status:%d", status);
        return -1;
    }

    status = hci_drv_close();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while closing HCI", status);
        return -1;
    }

    /* Wait 1 sec */
    btdl_sleep(1);

    status = hci_drv_open(device);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while opening HCI", status);
        return -1;
    }

    /* send an HCI_Reset */
    status = hci_cmd_sw_reset();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error hci_cmd_sw_reset status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        btdl_init
 **
 ** Description     Initialization function
 **
 ** Parameters      none
 **
 ** Returns         none
 **
 *******************************************************************************/
void btdl_init(void)
{
    /* clear the control block */
    memset(&btdl_cb, 0, sizeof(btdl_cb));

    hci_drv_init();
    btdl_utils_init();

    /* cUSB RAM download init */
    btdl_cb.bNeedDummyACLHandle = 1;
    btdl_cb.bWaitForDLAck = 1;
    btdl_cb.delay_between_bulk_writes_ms = 3;
    btdl_cb.delay_at_the_end_ms = 100;
}

/*******************************************************************************
 **
 ** Function        btdl_fw_failsafe_upgrade
 **
 ** Description     failsafe firmware upgrade function
 **
 ** Parameters      none
 **
 ** Returns         none
 **
 *******************************************************************************/
int btdl_fw_failsafe_upgrade(void)
{
    int status = 0;
    tFLASH_DL_I32HEX *p_burn_img = NULL;

    if ((!btdl_cb.nodl) && (!btdl_cb.burnimage))
    {
        btdl_print(BTDL_ERROR, "image file not specified");
        status = -1;
        usage();
        goto btdl_fw_failsafe_upgrade_exit;
    }

    /* parse the flash content hex file */
    if (btdl_cb.burnimage != NULL)
    {
        p_burn_img = btdl_hex_parse_i32hexfile(btdl_cb.burnimage, btdl_cb.nochksum);
        if (p_burn_img == NULL)
        {
            btdl_print(BTDL_ERROR, "Could not parse the burnimage file");
            status = -1;
            goto btdl_fw_failsafe_upgrade_exit;
        }
    }

    /* Open the HCI */
    status = hci_drv_open(btdl_cb.device);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while opening HCI", status);
        goto btdl_fw_failsafe_upgrade_exit;
    }

    /* SW Reset (EnableBTW and HCI_Reset) */
    status = hci_cmd_sw_reset();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while SW Reset status:%d", status);
        goto btdl_fw_failsafe_upgrade_exit;
    }

    /* Send Upgrade firmware command */
    status = hci_cmd_upgrade_firmware();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while Upgrade firmware status:%d", status);
        goto btdl_fw_failsafe_upgrade_exit;
    }
    btdl_sleep(3);

    /* Download the Burnimage */
    btdl_print(BTDL_PRINT, "Downloading the Data Section...");
    status = btdl_hex_download(p_burn_img, btdl_cb.chip_select);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error Image Download status :%d", status);
        goto btdl_fw_failsafe_upgrade_exit;
    }

    btdl_print(BTDL_PRINT, "Flash download completed");

    /* launch ram */
    status = hci_cmd_launch_ram(0);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error LaunchRAM (0x%08X) status :%d", 0, status);
        goto btdl_fw_failsafe_upgrade_exit;
    }

    btdl_print(BTDL_PRINT, "Failsafe upgrade Done.");

btdl_fw_failsafe_upgrade_exit:
    btdl_print(BTDL_DEBUG, "Exiting status:%d", status);
    /* free all the allocated elements */
    btdl_hex_free_i32hex(p_burn_img);

    return status;
}

/*******************************************************************************
 **
 ** Function        main
 **
 ** Description     Main function
 **
 ** Parameters      argc: number of arguments
 **                 argv: the arguments
 **
 ** Returns         status
 **
 *******************************************************************************/
int main(int argc, char* argv[])
{
    int status = 0;
    tFLASH_DL_I32HEX *p_minidrv = NULL;
    tFLASH_DL_I32HEX *p_burn_img = NULL;
    tFLASH_DL_I32HEX *p_static_section = NULL;
    uint8_t bd_addr_old[6];
    uint8_t bd_addr_new[6];

    btdl_init();

    btdl_set_verbose_level(BTDL_PRINT);

    /* parse the command line parameters */
    status = parse_cmd_line(argc, argv);
    if (status < 0)
    {
        usage();
        status = -1;
        goto main_exit;
    }

    /* sanity check */
    if (btdl_cb.device == NULL)
    {
        btdl_print(BTDL_ERROR, "Device not specified");
        status = -1;
        usage();
        goto main_exit;
    }

    if (btdl_cb.mem_type_param == 0)
    {
        btdl_print(BTDL_ERROR, "Memory Type not specified");
        status = -1;
        usage();
        goto main_exit;
    }

    if (btdl_cb.is_failsafe)
    {
        status = btdl_fw_failsafe_upgrade();
        if (status < 0)
        {
            btdl_print(BTDL_ERROR, "fail safe firmware upgrade error");
        }
        goto main_exit;
    }

    if (btdl_cb.nominidrv)
    {
        btdl_cb.minidriver = NULL;
    }

    if (btdl_cb.chip_select == FLASH_DL_CS_RAM)
    {
        /* New option to reload BT firmware on the fly into RAM -- no minidriver needed */
        /* MUST SPECIFY a hex file! */
        if (btdl_cb.burnimage == NULL)
        {
            btdl_print(BTDL_ERROR, "Intelhex burn image must be specified for RAM firmware reload");
            status = -1;
            usage();
            goto main_exit;
        }
        else
            btdl_cb.bDynamicFirmwareReload = 1;
    }
    else
    {
        if ((!btdl_cb.nodl) &&
            (!btdl_cb.nominidrv) &&
            ((btdl_cb.is_shared_usb && (btdl_cb.minidriver || !btdl_cb.burnimage)) ||
             (!btdl_cb.is_shared_usb && (!btdl_cb.minidriver || !btdl_cb.burnimage))))
        {
            btdl_print(BTDL_ERROR, "minidriver or image files not specified");
            status = -1;
            usage();
            goto main_exit;
        }

        /* parse the minidriver hex file */
        if (btdl_cb.minidriver != NULL)
        {
            p_minidrv = btdl_hex_parse_i32hexfile(btdl_cb.minidriver, btdl_cb.nochksum);
            if (p_minidrv == NULL)
            {
                btdl_print(BTDL_ERROR, "Could not parse the minidriver file");
                status = -1;
                goto main_exit;
            }
        }
    }

    /* parse the flash content hex file */
    if (btdl_cb.burnimage != NULL)
    {
        p_burn_img = btdl_hex_parse_i32hexfile(btdl_cb.burnimage, btdl_cb.nochksum);
        if (p_burn_img == NULL)
        {
            btdl_print(BTDL_ERROR, "Could not parse the burnimage file");
            status = -1;
            goto main_exit;
        }
    }

    if (btdl_cb.bDynamicFirmwareReload)
    {
        p_static_section = NULL;
    }
    else
    {
        p_static_section = btdl_hex_extract_static_section(&p_burn_img);
        if (p_static_section == NULL)
        {
            btdl_print(BTDL_ERROR, "Could not extract Static Section from the burnimage");
            status = -1;
            goto main_exit;
        }

    }

    /* Open the HCI */
    status = hci_drv_open(btdl_cb.device);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while opening HCI", status);
        return -1;
    }

    if (btdl_cb.bWaitForKey)
    {
        printf ("The USB port has been opened. Please press a key to continue...\n");
        getchar ();
    }

    if (btdl_cb.bDynamicFirmwareReload)
    {
        status = btdl_hex_firmware_reload (p_burn_img);
        goto main_exit;
    }

    /* SW Reset (EnableBTW and HCI_Reset) */
    status = hci_cmd_sw_reset();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while SW Reset status:%d", status);
        goto main_exit;
    }

    /* read the current BD address */
    status = hci_cmd_read_bd_addr(bd_addr_old);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while reading current BdAddr status:%d", status);
        goto main_exit;
    }

    /* Overwrite the BdAddr in the Static Section */
    if (btdl_cb.is_bd_addr_requested)
    {
        btdl_print(BTDL_PRINT, "Changing BdAddr to: %02X:%02X:%02X:%02X:%02X:%02X",
                btdl_cb.bd_addr_requested[0],
                btdl_cb.bd_addr_requested[1],
                btdl_cb.bd_addr_requested[2],
                btdl_cb.bd_addr_requested[3],
                btdl_cb.bd_addr_requested[4],
                btdl_cb.bd_addr_requested[5]);
        status = btdl_hex_overwrite_bdaddr(p_static_section, btdl_cb.bd_addr_requested);
    }
    else
    {
        btdl_print(BTDL_PRINT, "Restoring BdAddr: %02X:%02X:%02X:%02X:%02X:%02X",
            bd_addr_old[0], bd_addr_old[1], bd_addr_old[2],
            bd_addr_old[3], bd_addr_old[4], bd_addr_old[5]);
        status = btdl_hex_overwrite_bdaddr(p_static_section, bd_addr_old);
    }
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while overwriting BdAddr in Static Section status:%d", status);
        goto main_exit;
    }

    /* Read FW Version */
    status = hci_cmd_read_fw_version();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while reading current version:%d", status);
        goto main_exit;
    }

    if (btdl_cb.nodl)
    {
        btdl_print(BTDL_PRINT, "No download requested");
        status = 0;
        goto main_exit;
    }

    if (btdl_cb.is_shared_usb)
    {
        /* Erase the Serial flash */
        uint8_t buf[2];
        buf[0]=0;
        buf[1]=0;
        hci_cmd_write_ram(SHARED_USB_ADDRESS_CHIP_ERASE, buf, 2);
        btdl_sleep(1);
    }
    else
    {
        /* Send the download minidriver command and wait for confirmation */
        btdl_print(BTDL_DEBUG, "Send Download Mini Driver command");
        status = hci_cmd_dwnld_minidrv();
        if (status < 0)
        {
            btdl_print(BTDL_ERROR, "Error while sending Download minidriver command:%d", status);
            goto main_exit;
        }

        /* wait for the download minidriver switch to complete */
        btdl_sleep(1);

        /* Some Devices do not need minidriver to be downloaded */
        if (btdl_cb.minidriver)
        {
            /* download the minidriver */
            btdl_print(BTDL_PRINT, "Downloading Mini Driver in RAM...");
            status = btdl_hex_download(p_minidrv, FLASH_DL_CS_RAM);
            if (status < 0)
            {
                btdl_print(BTDL_ERROR, "Error while Downloading minidriver status:%d", status);
                goto main_exit;
            }

            /* launch the minidriver */
            status = hci_cmd_launch_ram(p_minidrv->addr);
            if (status < 0)
            {
                btdl_print(BTDL_ERROR, "Error LaunchRAM (0x%08X) status :%d", p_minidrv->addr, status);
                goto main_exit;
            }

            /* wait for restart to complete */
            btdl_sleep(1);
        }

        /* Erase the Serial flash */
        btdl_print(BTDL_PRINT, "Erasing the module's flash...");
        status = hci_cmd_chip_erase(0x00000000 | (btdl_cb.chip_select << 24));
        if (status < 0)
        {
            btdl_print(BTDL_ERROR, "Error EraseFlash status :%d", status);
            goto main_exit;
        }
    }

    /* Download the Burnimage */
    btdl_print(BTDL_PRINT, "Downloading the Data Section...");
    status = btdl_hex_download(p_burn_img, btdl_cb.chip_select);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error Image Download status :%d", status);
        goto main_exit;
    }

    /* Download the Static Section */
    btdl_print(BTDL_PRINT, "Downloading the Static Section...");
    status = btdl_hex_download(p_static_section, btdl_cb.chip_select);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error Static Section Download status :%d", status);
        goto main_exit;
    }

    btdl_print(BTDL_PRINT, "Flash download completed");

    btdl_print(BTDL_PRINT, "Verifying the flash content...");
    /* verifying burn (Data Section) operation */
    status = btdl_hex_check(p_burn_img, btdl_cb.chip_select);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Flash verification Data Section failed");
        goto main_exit;
    }
    else
    {
        btdl_print(BTDL_PRINT, "Flash verification Data Section successful");
    }

    /* verifying burn (Static Section) operation */
    status = btdl_hex_check(p_static_section, btdl_cb.chip_select);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Flash verification Static Section failed");
        goto main_exit;
    }
    else
    {
        btdl_print(BTDL_PRINT, "Flash verification Static Section successful");
    }

    if (btdl_cb.is_shared_usb)
    {
        btdl_print(BTDL_PRINT, "Done.");
        btdl_print(BTDL_PRINT, "Finished Combo Coex shared usb doesnot support hw reset yet!!");
        goto main_exit;
    }

    btdl_sleep(1);

    /* Reset the chip (use the new FW) */
    status = btdl_hw_reset(btdl_cb.device);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while Reseting the chip:%d", status);
        goto main_exit;
    }

    /* Read new FW version */
    status = hci_cmd_read_fw_version();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while reading current version:%d", status);
        goto main_exit;
    }

    /* Read BdAddr (which was set in the downloaded FW) */
    status = hci_cmd_read_bd_addr(bd_addr_new);
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while reading current BdAddr status:%d", status);
        goto main_exit;
    }

    /* send an HCI_Reset */
    btdl_print(BTDL_INFO, "Send HCI_Reset");
    /* Reset the chip (use the new FW) */
    status = hci_cmd_sw_reset();
    if (status < 0)
    {
        btdl_print(BTDL_ERROR, "Error while SW Reset status:%d", status);
        goto main_exit;
    }

    btdl_print(BTDL_PRINT, "Done.");

main_exit:
    if (btdl_cb.delay_at_the_end_ms)
    {
        btdl_print(BTDL_PRINT, "Delaying by %d ms", btdl_cb.delay_at_the_end_ms);
        usleep (btdl_cb.delay_at_the_end_ms * 1000);
    }
    btdl_print(BTDL_DEBUG, "Exiting status:%d", status);
    /* free all the allocated elements */
    btdl_hex_free_i32hex(p_minidrv);
    btdl_hex_free_i32hex(p_burn_img);

    hci_drv_close();

    return status;
}
