/*****************************************************************************
 **
 **  Name:           app_hh_otafwdl.h
 **
 **  Description:    Bluetooth HID Host OTA fw download header file
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_HH_OTAFWDL_H
#define APP_HH_OTAFWDL_H


/*
 * Definitions
 */

/* Firmware Upgrade */
#define APP_HH_OTAFWDL_RC_GETCFG_ID 0x30
#define APP_HH_OTAFWDL_RC_GETCFG_SIZE 3
#define APP_HH_OTAFWDL_RC_SETCFGIR_ID 0x31
#define APP_HH_OTAFWDL_RC_SETCFGGAIN_ID 0x32
#define APP_HH_OTAFWDL_RC_SETCFG_SIZE 2

#define APP_HH_OTAFWDL_EEPROM_BASE_ADDR 0xFF000000
#define APP_HH_OTAFWDL_TOUCHPAD_BASE_ADDR 0xFA000000
#define APP_HH_OTAFWDL_SS1_LOCATION 0x00000000
#define APP_HH_OTAFWDL_SS2_LOCATION 0x00000100
#define APP_HH_OTAFWDL_HIDFWU_UPPER_OFFSET 0x00008000
#define APP_HH_OTAFWDL_SS_LENGTH 0x30
#define APP_HH_OTAFWDL_SS_LOCATION_MAX 0x000000C0
#define APP_HH_OTAFWDL_VS_LOCATION 0x000001C0
#define APP_HH_OTAFWDL_VS_LENGTH 0x00000200
#define APP_HH_OTAFWDL_DS_LOCATION_MASK 0x00000FFF

/* Firmware Upgrade on Serial flash */
#ifndef APP_HH_OTAFWDL_SFLASH_BASE_ADDR
#define APP_HH_OTAFWDL_SFLASH_BASE_ADDR 0xF8000000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_DS1_LOCATION
#define APP_HH_OTAFWDL_SFLASH_DS1_LOCATION 0x00003000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_DS2_LOCATION
#define APP_HH_OTAFWDL_SFLASH_DS2_LOCATION 0x0000F000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_DS_LEN
#define APP_HH_OTAFWDL_SFLASH_DS_LEN 0x0000C000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_SS_LEN
#define APP_HH_OTAFWDL_SFLASH_SS_LEN 0x00001000
#endif

#ifndef APP_HH_OTAFWDL_SFLASH_DS1_LOCATION2
#define APP_HH_OTAFWDL_SFLASH_DS1_LOCATION2 0x00005000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_DS2_LOCATION2
#define APP_HH_OTAFWDL_SFLASH_DS2_LOCATION2 0x00012000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_DS_LEN2
#define APP_HH_OTAFWDL_SFLASH_DS_LEN2 0x0000D000
#endif

/* Firmware Upgrade on Serial flash for Fail Safe mode */
#define OFFSET_SIZE 4
#ifndef APP_HH_OTAFWDL_SFLASH_FAILSAFE_BASE_ADDR
#define APP_HH_OTAFWDL_SFLASH_FAILSAFE_BASE_ADDR 0x00001000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_FAILSAFE_LEN
#define APP_HH_OTAFWDL_SFLASH_FAILSAFE_LEN 0x00001000
#endif
#ifndef APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_OFFSET
#define APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_OFFSET 0x00000FF4
#endif
#define APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_LEN 8
#define APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_DS2_OFFSET_SIZE APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_LEN + OFFSET_SIZE
static const UINT8 FailSafeMagicNumberArray[APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_LEN] =
{
    0xAA,
    0x55,
    0xF0,
    0x0F,
    0x68,
    0xE5,
    0x97,
    0xD2
};

#define APP_HH_OTAFWDL_HIDFWU_VERSION 0x01
#define APP_HH_OTAFWDL_HIDFWU_VERSION_SIZE 4
#define APP_HH_OTAFWDL_HIDFWU_ENABLE 0x70
#define APP_HH_OTAFWDL_HIDFWU_ENABLE_SIZE 1
#define APP_HH_OTAFWDL_HIDFWU_SETUP_READ 0x71
#define APP_HH_OTAFWDL_HIDFWU_SETUP_READ_SIZE 8
#define APP_HH_OTAFWDL_HIDFWU_READ 0x72
#define APP_HH_OTAFWDL_HIDFWU_READ_HEADER_SIZE 7
#define APP_HH_OTAFWDL_HIDFWU_ERASE 0x73
#define APP_HH_OTAFWDL_HIDFWU_ERASE_SIZE 8
#define APP_HH_OTAFWDL_HIDFWU_WRITE 0x74
#define APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(__l) (__l + 8)
#define APP_HH_OTAFWDL_HIDFWU_LAUNCH 0x75
#define APP_HH_OTAFWDL_HIDFWU_LAUNCH_SIZE 8
#define APP_HH_OTAFWDL_HIDFWU_CONFIG 0x76
#define APP_HH_OTAFWDL_HIDFWU_CONFIG_SIZE 1



/********************************************/
/* Definitions for Dual Boot RCU FW upgrade */
/********************************************/
/* Flash memory map */
#define APP_HH_OTAFWDL_DB_SF_BASE_ADDR       0xF8000000
#define APP_HH_OTAFWDL_DB_SF_SECTOR_SIZE     0x001000
/* Static section */
#define APP_HH_OTAFWDL_DB_SF_STATIC_ADDR     0x000000
#define APP_HH_OTAFWDL_DB_SF_STATIC_LEN      0x001000
/* Failsafe section */
#define APP_HH_OTAFWDL_DB_SF_FAILSAFE_ADDR   0x001000
#define APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN    0x001000
/* Static section 2*/
#define APP_HH_OTAFWDL_DB_SF_STATIC2_ADDR    0x001FF4
#define APP_HH_OTAFWDL_DB_SF_STATIC2_OFFSET  0x000FF4
#define APP_HH_OTAFWDL_DB_SF_STATIC2_LEN     0x00000C
/* Volatile section */
#define APP_HH_OTAFWDL_DB_SF_VOL_ADDR        0x002000
#define APP_HH_OTAFWDL_DB_SF_VOL_LEN         0x001000
/* Backup volatile section */
#define APP_HH_OTAFWDL_DB_SF_VOL_BK_ADDR     0x003000
#define APP_HH_OTAFWDL_DB_SF_VOL_BK_LEN      0x001000
/* Data section 1 for classic BT FW */
#define APP_HH_OTAFWDL_DB_SF_DS1_CL_ADDR     0x004000
#define APP_HH_OTAFWDL_DB_SF_DS1_CL_LEN      0x01C000
/* Customer section 1 */
#define APP_HH_OTAFWDL_DB_SF_CUST1_ADDR      0x020000
#define APP_HH_OTAFWDL_DB_SF_CUST1_LEN       0x004000
/* Data section 1 for BLE FW */
#define APP_HH_OTAFWDL_DB_SF_DS1_LE_ADDR     0x024000
#define APP_HH_OTAFWDL_DB_SF_DS1_LE_LEN      0x01C000
/* Customer section 2 */
#define APP_HH_OTAFWDL_DB_SF_CUST2_ADDR      0x040000
#define APP_HH_OTAFWDL_DB_SF_CUST2_LEN       0x004000
/* Data section 2 for classic BT FW */
#define APP_HH_OTAFWDL_DB_SF_DS2_CL_ADDR     0x044000
#define APP_HH_OTAFWDL_DB_SF_DS2_CL_LEN      0x01C000
/* Customer section 3 */
#define APP_HH_OTAFWDL_DB_SF_CUST3_ADDR      0x060000
#define APP_HH_OTAFWDL_DB_SF_CUST3_LEN       0x004000
/* Data section 2 for BLE FW */
#define APP_HH_OTAFWDL_DB_SF_DS2_LE_ADDR     0x064000
#define APP_HH_OTAFWDL_DB_SF_DS2_LE_LEN      0x01C000

#define APP_HH_OTAFWDL_DB_SIG_LEN      0xC


/* MCU upgrade */
#define APP_HH_OTAFWDL_HIDMCUU_ENABLE 0x80
#define APP_HH_OTAFWDL_HIDMCUU_SETUP_READ 0x81
#define APP_HH_OTAFWDL_HIDMCUU_READ 0x82
#define APP_HH_OTAFWDL_HIDMCUU_ERASE 0x83
#define APP_HH_OTAFWDL_HIDMCUU_WRITE 0x84
#define APP_HH_OTAFWDL_HIDMCUU_LAUNCH 0x85
#define APP_HH_OTAFWDL_HIDMCUU_LAUNCH_SIZE 16

#define APP_HH_OTAFWDL_HIDFWU_DEFAULT_WRITESIZE 256
#define APP_HH_OTAFWDL_HIDFWU_TIMEOUT 4000

#define APP_HH_OTAFWDL_HEX_RECORD_SIZE 255
#define APP_HH_OTAFWDL_READBUF_SIZE 1024

/* configuration values */
/*   + OTA read size */
#define APP_HH_OTAFWDL_HIDFWU_READSIZE      256
#define APP_HH_OTAFWDL_FASTBOOT_SS_LENGTH   0x1000
#define APP_HH_OTAFWDL_FASTBOOT_OFFSET     0x1B
#define APP_HH_OTAFWDL_DSLOCATION_OFFSET    0x1e
#define APP_HH_OTAFWDL_FASTBOOT_MAX_SIZE   0x800
#define APP_HH_OTAFWDL_FASTBOOT_SS_SIZE    0x28

typedef struct
{
    UINT16 otafwu_writesize;
    volatile BOOLEAN waiting;  /* Indicate that the module is waiting for an OTAFWU response */
    tBSA_HH_HANDLE handle;
    tBSA_STATUS status;
    UINT8 *p_buf;
    UINT16 len;
} tAPP_HH_OTAFWDL;


/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_cksum
 **
 ** Description     Compute OTA checksum on data
 **
 ** Parameters      len: length of buffer
 **                 p_data: buffer containing the data to compute checksum of
 **
 ** Returns         2's complement of the byte sum
 **
 *******************************************************************************/
UINT8 app_hh_otafwdl_ota_cksum(int len, UINT8 *p_data);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_setreport
 **
 ** Description     Send OTA set report
 **
 ** Parameters      handle: device HID handle
 **                 len: length of report to set (includes report ID)
 **                 p_data: report
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_setreport(tBSA_HH_HANDLE handle, UINT16 len, UINT8 *p_data);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_getreport
 **
 ** Description     Send OTA get report
 **
 ** Parameters      handle: device HID handle
 **                 report_id: ID of the report to get
 **                 len: maximum length of report to get
 **                 p_buf: buffer allocated to receive the data
 **
 ** Returns         status: length of the report data if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_getreport(tBSA_HH_HANDLE handle, UINT8 report_id, UINT16 len, UINT8 *p_buf);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_getversion
 **
 ** Description     Get HID device version numbers
 **
 ** Parameters      handle: device HID handle
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_getversion(tBSA_HH_HANDLE handle);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_readmem
 **
 ** Description     Read memory on remote device
 **
 ** Parameters      handle: device HID handle
 **                 addr: address to read at
 **                 len: length of data to read
 **                 p_buf: buffer allocated to receive the data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_readmem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_writemem
 **
 ** Description     Write memory on remote device
 **
 ** Parameters      handle: device HID handle
 **                 addr: address to write at
 **                 len: length of data to write
 **                 p_buf: buffer containing the data to write
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_writemem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_erasemem
 **
 ** Description     Erase memory on remote device
 **
 ** Parameters      handle: device HID handle
 **                 addr: address to erase at
 **                 len: length of data to erase
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_erasemem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_fuenable
 **
 ** Description     Enable firmware update on remote device
 **
 ** Parameters      handle: device HID handle
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_fuenable(tBSA_HH_HANDLE handle);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_launch
 **
 ** Description     Launch firmware on remote device
 **
 ** Parameters      handle: device HID handle
 **                 addr: launch address
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_launch(tBSA_HH_HANDLE handle, UINT32 addr);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_config
 **
 ** Description     Enable configuration on remote device
 **
 ** Parameters      handle: device HID handle
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_ota_config(tBSA_HH_HANDLE handle);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwversion
 **
 ** Description     Retrieve the version number of a remote HID device
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwversion(void);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_start
 **
 ** Description     Start the over the air firmware upgrade
 **
 ** Parameters      p_handle: returned handle of the connection if successful
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_start(tBSA_HH_HANDLE *p_handle);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_burn
 **
 ** Description     Burn an image in specified DS location
 **
 ** Parameters      handle: HID handle
 **                 ds_new: ds location to burn content at
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_burn(tBSA_HH_HANDLE handle, UINT32 ds_new);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_nofs
 **
 ** Description     Update the firmware of a HID device that does not have
 **                 failure protection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_nofs(void);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_fs
 **
 ** Description     Update the firmware of a HID device that has failure protection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_fs(void);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_mcu_readmem
 **
 ** Description     Read memory on remote device
 **
 ** Parameters      handle: device HID handle
 **                 addr: address to read at
 **                 len: length of data to read
 **                 p_buf: buffer allocated to receive the data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_mcu_readmem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_mcu_writemem
 **
 ** Description     Write memory on remote device
 **
 ** Parameters      handle: device HID handle
 **                 addr: address to write at
 **                 len: length of data to write
 **                 p_buf: buffer containing the data to write
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_mcu_writemem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_mcu_fuenable
 **
 ** Description     Enable MCU update on remote device
 **
 ** Parameters      handle: device HID handle
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_mcu_fuenable(tBSA_HH_HANDLE handle);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_mcu_launch
 **
 ** Description     Launch MCU on remote device
 **
 ** Parameters      handle: device HID handle
 **                 addr: launch address
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_mcu_launch(tBSA_HH_HANDLE handle, UINT32 addr, UINT32 start_addr, UINT32 len);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_mcuupdate_burn
 **
 ** Description     Burn a MCU image in specified DS location
 **
 ** Parameters      handle: HID handle
 **                 ds_new: ds location to burn content at
 **
 ** Returns  length of mcu image  / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_mcuupdate_burn(tBSA_HH_HANDLE handle, UINT32 ds_new);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_mcuupdate_fs
 **
 ** Description     Update the MCU binary of a HID device that has failure protection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_mcuupdate_fs(void);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_sflash_nofs
 **
 ** Description     Update the firmware of a HID device serial flash
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_sflash_nofs(void);

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_sflash_fs
 **
 ** Description     Update the firmware of a HID device serial flash that has failure protection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_sflash_fs(void);

/*******************************************************************************
**
** Function        app_hh_otafwdl_fwupdate_search_alternate
**
** Description     Search for Magic number in Failsafe Section of SFLASH
**
** Parameters      handle : HID handle
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_hh_otafwdl_fwupdate_search_alternate(tBSA_HH_HANDLE handle, UINT32 *ds_location);

/*******************************************************************************
**
** Function        app_hh_otafwdl_fwupdate_write_alternate
**
** Description     Write Magic number and DS2 offset in Failsafe Section of SFLASH
**
** Parameters      handle : HID handle
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_hh_otafwdl_fwupdate_write_alternate(tBSA_HH_HANDLE handle, UINT32 ds_location);

/*******************************************************************************
**
** Function        app_hh_otafwdl_fwupdate_fastboot_sflash
**
** Description     Update the firmware of a HID device serial flash with fastboot
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
**    0x0000   +--------------+  SS
**             |              |
**             | ............ |
**    0x1b     + ............ +  fastboot offset
**             |              |
**             |              |
**0x1b +fbsize +--------------+  DS type
**             |              |
**             |              |
**             |              |
**             |     ..       |
*******************************************************************************/
int app_hh_otafwdl_fwupdate_fastboot_sflash(void);

/*******************************************************************************
**
** Function        app_hh_otafwdl_fwupdate_dualboot_rcu
**
** Description     Update the firmware to Dual Boot RCU
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_hh_otafwdl_fwupdate_dualboot_rcu(void);
#endif
