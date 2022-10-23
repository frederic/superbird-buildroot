/*****************************************************************************
**
**  Name:           app_hh_otafwdl.c
**
**  Description:    Bluetooth HH OTA fw download application
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* for usleep */
#include <unistd.h>

#include "bsa_api.h"

#include "app_utils.h"
#include "app_disc.h"
#include "app_hh.h"
#include "app_xml_param.h"
#include "app_hh.h"
#include "app_link.h"
#include "app_thread.h"
#include "app_hh_otafwdl.h"

#ifndef APP_HH_OTAFWDL_DEBUG
#define APP_HH_OTAFWDL_DEBUG TRUE
#endif

static UINT8 app_hh_otafwdl_db_sig_bt1[APP_HH_OTAFWDL_DB_SIG_LEN] =
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static UINT8 app_hh_otafwdl_db_sig_bt2[APP_HH_OTAFWDL_DB_SIG_LEN] =
    {0xAA, 0x55, 0xF0, 0x0F, 0x68, 0xE5, 0x97, 0xD2, 0x00, 0x40, 0x04, 0x00};

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
UINT8 app_hh_otafwdl_ota_cksum(int len, UINT8 *p_data)
{
    UINT8 sum = 0;
    int index;

    for (index = 0; index < len; index++)
    {
        sum += p_data[index];
    }

    /* return the 2's complement of the sum */
    return ~(sum-1);
}

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
int app_hh_otafwdl_ota_setreport(tBSA_HH_HANDLE handle, UINT16 len, UINT8 *p_data)
{
    tBSA_STATUS status;
    tBSA_HH_SET hh_set_param;
    UINT32 timeout = APP_HH_OTAFWDL_HIDFWU_TIMEOUT;

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = BSA_HH_RPTT_FEATURE;
    hh_set_param.param.set_report.report.length = len;
    memcpy(hh_set_param.param.set_report.report.data, p_data, len);

    app_hh_cb.ota_fw_dl.handle = handle;
    app_hh_cb.ota_fw_dl.waiting = TRUE;

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet failed:%d", status);
        app_hh_cb.ota_fw_dl.waiting = FALSE;
        return -1;
    }
    /* Wait unless it is a launch request */
    if (p_data[0] != APP_HH_OTAFWDL_HIDFWU_LAUNCH)
    {
        while (app_hh_cb.ota_fw_dl.waiting && timeout)
        {
            usleep(1000);
            timeout--;
        }
        app_hh_cb.ota_fw_dl.waiting = FALSE;
        if ((app_hh_cb.ota_fw_dl.status != BSA_SUCCESS) || (timeout == 0))
        {
            APP_ERROR1("Set report failed:status=%d, timeout=%d", app_hh_cb.ota_fw_dl.status, timeout);
            return -1;
        }
    }
    else
    {
        app_hh_cb.ota_fw_dl.waiting = FALSE;
    }
    return 0;
}

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
int app_hh_otafwdl_ota_getreport(tBSA_HH_HANDLE handle, UINT8 report_id, UINT16 len, UINT8 *p_buf)
{
    int status;
    tBSA_HH_GET hh_get_param;
    UINT32 timeout = APP_HH_OTAFWDL_HIDFWU_TIMEOUT;

    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_REPORT_NO_SIZE_REQ;
    hh_get_param.handle = handle;
    hh_get_param.param.get_report.report_type = BSA_HH_RPTT_FEATURE;
    hh_get_param.param.get_report.report_id = report_id;

    app_hh_cb.ota_fw_dl.handle = handle;
    app_hh_cb.ota_fw_dl.p_buf = p_buf;
    app_hh_cb.ota_fw_dl.len = len;
    app_hh_cb.ota_fw_dl.waiting = TRUE;

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhGet failed:%d", status);
        app_hh_cb.ota_fw_dl.waiting = FALSE;
        return -1;
    }
    while (app_hh_cb.ota_fw_dl.waiting && timeout)
    {
        usleep(1000);
        timeout--;
    }
    app_hh_cb.ota_fw_dl.waiting = FALSE;
    if ((app_hh_cb.ota_fw_dl.status != BSA_SUCCESS) || (timeout == 0))
    {
        APP_ERROR1("Get report failed:status=%d, timeout=%d", app_hh_cb.ota_fw_dl.status, timeout);
        return -1;
    }

    return app_hh_cb.ota_fw_dl.len;
}

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
int app_hh_otafwdl_ota_getversion(tBSA_HH_HANDLE handle)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_VERSION_SIZE];
    UINT16 fw_version;
    int length;

    length = app_hh_otafwdl_ota_getreport(handle, APP_HH_OTAFWDL_HIDFWU_VERSION,
            sizeof(report), report);
    if (length < 2)
    {
        APP_ERROR1("Version report format is not supported, %d bytes", length);
        return -1;
    }

    fw_version = (report[0] << 8) + report[1];

    APP_INFO1("Firmware version number : %d", fw_version);
    APP_INFO1("TouchPad version number : %d.%d", report[2], report[3]);

    return 0;
}

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
int app_hh_otafwdl_ota_readmem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_SETUP_READ_SIZE], *p_data;
    UINT16 rlen, index;

    APP_DEBUG1("app_hh_otafwdl_ota_readmem addr:0x%x ,len:%d"
                , addr, len);

    /* Limit the maximum size of the read length */
    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_SETUP_READ);
    UINT32_TO_STREAM(p_data, addr);
    UINT16_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_READSIZE);
    UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(7, report));
    if (app_hh_otafwdl_ota_setreport(handle, sizeof(report), report))
    {
        return -1;
    }
    index = 0;
    while (1)
    {
        /* Remaining length to read */
        rlen = len - index;
        /* Reached end? */
        if (rlen == 0)
        {
            break;
        }
        /* Is remaining length larger than configured read size? */
        else if (rlen > APP_HH_OTAFWDL_HIDFWU_READSIZE)
        {
            rlen = APP_HH_OTAFWDL_HIDFWU_READSIZE;
        }
        if (app_hh_otafwdl_ota_getreport(handle, APP_HH_OTAFWDL_HIDFWU_READ, rlen, &p_buf[index]) != rlen)
        {
            return -1;
        }
        /* Increment index */
        index += rlen;
    }
    return 0;
}

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
int app_hh_otafwdl_ota_writemem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(APP_HH_OTAFWDL_HEX_RECORD_SIZE)], *p_data;
    UINT16 wlen;

    /* Check the user is not trying to write too large */
    if ((UINT16)(APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(len)) > sizeof(report))
    {
        APP_DEBUG1("APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(len):0x%x ,sizeof(report):0x%x",
                  APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(len), sizeof(report));
        APP_ERROR1("Record too large to be written(0x%x), is this an Intel Hex file record??", len);
        return -1;
    }
    APP_DEBUG1("app_hh_otafwdl_ota_writemem addr:0x%x ,len:%d, writesize:%d"
                , addr, len, app_hh_cb.ota_fw_dl.otafwu_writesize);

    while (len)
    {
        /* Make sure that the length does not override MTU */
        wlen = len;
        if (wlen > app_hh_cb.ota_fw_dl.otafwu_writesize)
        {
            wlen = app_hh_cb.ota_fw_dl.otafwu_writesize;
        }

        /* Write the next chunk */
        p_data = report;
        UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_WRITE);
        UINT32_TO_STREAM(p_data, addr);
        UINT16_TO_STREAM(p_data, wlen);
        memcpy(p_data, p_buf, wlen);
        p_data += wlen;
        UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(wlen + 7, report));

        if (app_hh_otafwdl_ota_setreport(handle, APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(wlen), report))
        {
            return -1;
        }
        /* Increment the pointers and counters */
        p_buf += wlen;
        addr += wlen;
        len -= wlen;
    }
    return 0;
}

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
int app_hh_otafwdl_ota_erasemem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_ERASE_SIZE], *p_data;

    APP_DEBUG1("app_hh_otafwdl_ota_erasemem addr:0x%x ,len:0x%x", addr, len);

    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_ERASE);
    UINT32_TO_STREAM(p_data, addr);
    UINT16_TO_STREAM(p_data, len);
    UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(7, report));

    if (app_hh_otafwdl_ota_setreport(handle, sizeof(report), report))
    {
        return -1;
    }
    return 0;
}

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
int app_hh_otafwdl_ota_fuenable(tBSA_HH_HANDLE handle)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_ENABLE_SIZE], *p_data;

    APP_DEBUG0("app_hh_otafwdl_ota_fuenable");
    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_ENABLE);
    return app_hh_otafwdl_ota_setreport(handle, sizeof(report), report);
}



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
int app_hh_otafwdl_ota_launch(tBSA_HH_HANDLE handle, UINT32 addr)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_LAUNCH_SIZE], *p_data;

    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_LAUNCH);
    UINT32_TO_STREAM(p_data, addr);
    UINT16_TO_STREAM(p_data, 0);
    UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(7, report));
    return app_hh_otafwdl_ota_setreport(handle, sizeof(report), report);
}


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
int app_hh_otafwdl_ota_config(tBSA_HH_HANDLE handle)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_CONFIG_SIZE], *p_data;

    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_CONFIG);
    return app_hh_otafwdl_ota_setreport(handle, sizeof(report), report);
}

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
int app_hh_otafwdl_fwversion(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ss_location;
    UINT32 ds_location, vs_location;
    UINT16 len, vs_length;
    UINT8 ssbuf[APP_HH_OTAFWDL_SS_LENGTH], *p_data, eeprombuf[APP_HH_OTAFWDL_SS_LENGTH];

    app_hh_display_status();
    handle = app_get_choice("Enter HID Handle");

    if (app_hh_otafwdl_ota_getversion(handle))
    {
        return -1;
    }

    /* Search for static section in EEPROM */
    ss_location = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS1_LOCATION;
    if (app_hh_otafwdl_ota_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
    {
        return -1;
    }

    /* Check if it contains the SS identifier 01 08 00 */
    if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
    {
        /* Search for static section in EEPROM */
        ss_location = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS2_LOCATION;
        if (app_hh_otafwdl_ota_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
        {
            return -1;
        }
        /* Check if it contains the SS identifier 01 08 00 */
        if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
        {
            APP_INFO0("SS location not found");
            return -1;
        }
    }
    APP_INFO1("SS location = x%08X", ss_location);

    /* Get the DS VS OFFSETs (config entry at 0x1b in SS: id=2) */
    if (ssbuf[0x1b] != 2)
    {
        APP_ERROR0("DS/VS offsets not found in SS");
        return -1;
    }
    p_data = &ssbuf[0x1c];
    STREAM_TO_UINT16(len, p_data);
    if (len != 10)
    {
        APP_ERROR0("DS/VS offsets length not correct in SS");
        return -1;
    }
    STREAM_TO_UINT32(ds_location, p_data);
    STREAM_TO_UINT32(vs_location, p_data);
    STREAM_TO_UINT16(vs_length, p_data);

    APP_INFO1("DS location = x%08X - VS location = x%08X - VS length = x%04X", ds_location, vs_location, vs_length);

    /* Read at offset 0 and 0x8000 and compare (if identical it is 256kb EEPROM) */
    if (app_hh_otafwdl_ota_readmem(handle, APP_HH_OTAFWDL_EEPROM_BASE_ADDR, sizeof(ssbuf), ssbuf))
    {
        return -1;
    }
    if (app_hh_otafwdl_ota_readmem(handle, APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_HIDFWU_UPPER_OFFSET,
            sizeof(eeprombuf), eeprombuf))
    {
        return -1;
    }
    if (memcmp(eeprombuf, ssbuf, sizeof(eeprombuf)) == 0)
    {
        APP_INFO0("EEPROM is 256kb");
    }
    else
    {
        APP_INFO0("EEPROM is 512kb");
    }

    return 0;
}

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
int app_hh_otafwdl_fwupdate_start(tBSA_HH_HANDLE *p_handle)
{
    UINT32 timeout = APP_HH_OTAFWDL_HIDFWU_TIMEOUT;
    tAPP_HH_DEVICE *p_dev;
    int choice;
    tBSA_STATUS bsastatus;
    tBSA_HH_OPEN hh_open_param;

    APP_INFO0("Bluetooth HID Firmware upgrade");

    app_hh_display_status();
    choice = app_get_choice("Put the device in pairing mode, then select its HID handle in table");
    p_dev = app_hh_pdev_find_by_handle((tBSA_HH_HANDLE)choice);
    if (p_dev == NULL)
    {
        APP_ERROR0("Device index does not exist");
        return -1;
    }
    /* Save the HID handle */
    *p_handle = p_dev->handle;

    BSA_HhOpenInit(&hh_open_param);
    bdcpy(hh_open_param.bd_addr, p_dev->bd_addr);
    hh_open_param.mode = BSA_HH_PROTO_RPT_MODE;
    hh_open_param.sec_mask = BTA_SEC_NONE;

retry:
    app_hh_cb.ota_fw_dl.handle = p_dev->handle;
    app_hh_cb.ota_fw_dl.waiting = TRUE;

    bsastatus = BSA_HhOpen(&hh_open_param);
    if (bsastatus != BSA_SUCCESS)
    {
        app_hh_cb.ota_fw_dl.waiting = FALSE;
        choice = app_get_choice("Device seems already connected, retry [Y/n]");
        if (choice == 0)
        {
            goto retry;
        }
        return -1;
    }

    while (app_hh_cb.ota_fw_dl.waiting && timeout)
    {
        usleep(1000);
        timeout--;
    }
    app_hh_cb.ota_fw_dl.waiting = FALSE;

    if ((app_hh_cb.ota_fw_dl.status != BSA_SUCCESS) || (timeout == 0))
    {
        APP_INFO0("FIRMWARE UPDATE FAILED: device not in pairing mode (not reachable)");
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_burn
 **
 ** Description     Burn an image in specified DS location
 **
 ** Parameters      handle: HID handle
 **                 ds_new: ds location to burn content at
 **
 ** Returns  app_hh_cb.ota_fw_dlatus: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_burn(tBSA_HH_HANDLE handle, UINT32 ds_new)
{
    FILE *fp;
    UINT32 ds_file;
    UINT16 len, offset;
    UINT8 linebuf[APP_HH_OTAFWDL_READBUF_SIZE], readbuf[APP_HH_OTAFWDL_HEX_RECORD_SIZE], type;
    BOOLEAN successful;

    /* Open the file */
    fp = fopen("fw.hex", "r");
    if (fp == NULL)
    {
        APP_ERROR0("Failed opening file fw.hex");
        return -1;
    }

    APP_INFO0("Writing");

    /* Mark file DS offset as not initialized */
    ds_file = 0xFFFFFFFF;

    /* Read all the records in the firmware file */
    successful = FALSE;
    while (TRUE)
    {
        len = sizeof(linebuf);
        if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
        {
            /* Not successful, should finish on a type 1 record */
            break;
        }

        if (type == 0) /* DATA record */
        {
            /* if we have not reached the file DS section yet */
            if (ds_file == 0xFFFFFFFF)
            {
                /* if this is still a static section record */
                if (offset < APP_HH_OTAFWDL_SS_LOCATION_MAX)
                {
                    APP_DEBUG1("Pre-DS record: 0x%04X", offset);
                    continue;
                }
                /* first DS record found, save the base address */
                ds_file = offset;
            }
            /* translate the record to the new DS base address */
            if (app_hh_otafwdl_ota_writemem(handle, APP_HH_OTAFWDL_EEPROM_BASE_ADDR - ds_file + ds_new + offset, len, linebuf))
            {
                break;
            }

            printf(".");
            fflush(stdout);
        }
        else if (type == 1) /* END record */
        {
            successful = TRUE;
            break;
        }
    }

    if (successful)
    {
        APP_INFO0("\nVerifying");

        /* Seek to the beginning of the file */
        fseek(fp, 0, SEEK_SET);

        successful = FALSE;
        while (TRUE)
        {
            len = sizeof(linebuf);
            if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
            {
                /* Not successful, should finish on a type 1 record */
                break;
            }

            if (type == 0)
            {
                /* do not check the pre-DS records */
                if (offset < (ds_new & APP_HH_OTAFWDL_DS_LOCATION_MASK))
                {
                    APP_DEBUG1("Pre-DS record: 0x%04X", offset);
                    continue;
                }

                if (len > sizeof(readbuf))
                {
                    APP_ERROR0("Buffer not large enough to read back");
                    break;
                }
                if (app_hh_otafwdl_ota_readmem(handle, APP_HH_OTAFWDL_EEPROM_BASE_ADDR - ds_file + ds_new + offset, len, readbuf))
                {
                    break;
                }
                /* scru_dump_hex(readbuf, "read", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG); */
                /* Check if the data matches */
                if (memcmp(readbuf, linebuf, len))
                {
                    APP_ERROR0("Failure when reading back data");
                    break;
                }
                printf("R");
                fflush(stdout);
            }
            else if (type == 1)
            {
                successful = TRUE;
                break;
            }
        }
    }
    fclose(fp);

    if (successful)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}


/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_fwupdate_sflash_burn
 **
 ** Description     Burn an image in specified DS location
 **
 ** Parameters      handle: HID handle
 **                 ds_new: ds location to burn content at
 **
 ** Returns  app_hh_cb.ota_fw_dlatus: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_sflash_burn(tBSA_HH_HANDLE handle, UINT32 ds_new)
{
    FILE *fp;
    UINT32 ds_file, ext_addr = 0;
    UINT16 len, offset, ds_len;
    UINT8 linebuf[APP_HH_OTAFWDL_READBUF_SIZE], readbuf[APP_HH_OTAFWDL_HEX_RECORD_SIZE], type;
    BOOLEAN successful;

    /* Open the file */
    fp = fopen("fw.hex", "r");
    if (fp == NULL)
    {
        APP_ERROR0("Failed opening file fw.hex");
        return -1;
    }

    APP_INFO1("Erasing Addr:0x%x Len:0x%x",
            APP_HH_OTAFWDL_SFLASH_BASE_ADDR + ds_new,
            APP_HH_OTAFWDL_SFLASH_DS_LEN);

    /* Find DS Length */
    if ((ds_new == APP_HH_OTAFWDL_SFLASH_DS1_LOCATION)||(ds_new == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION))
    {
        ds_len = APP_HH_OTAFWDL_SFLASH_DS_LEN;
    }
    else if ((ds_new == APP_HH_OTAFWDL_SFLASH_DS1_LOCATION2)||(ds_new == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION2))
    {
        ds_len = APP_HH_OTAFWDL_SFLASH_DS_LEN2;
    }
    else
    {
        APP_ERROR1("Failed to decide ds len 0x%x", ds_new);
        fclose(fp);
        return -1;
    }
    /* erase first DS area */
    if (app_hh_otafwdl_ota_erasemem(handle, APP_HH_OTAFWDL_SFLASH_BASE_ADDR + ds_new, ds_len))
    {
        fclose(fp);
        return -1;
    }

    APP_INFO0("Writing");

    /* Mark file DS offset as not initialized */
    ds_file = 0xFFFFFFFF;

    /* Read all the records in the firmware file */
    successful = FALSE;
    while (TRUE)
    {
        len = sizeof(linebuf);
        if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
        {
            /* Not successful, should finish on a type 1 record */
            break;
        }

        if (type == 0) /* DATA record */
        {
            /* if we have not reached the file DS section yet */
            if (ds_file == 0xFFFFFFFF)
            {
                /* if this is still a static section record */
                if (offset + ext_addr < APP_HH_OTAFWDL_SS_LOCATION_MAX)
                {
                    APP_DEBUG1("Pre-DS record: 0x%04X", offset);
                    continue;
                }
                /* first DS record found, save the base address */
                ds_file = offset;
            }
            APP_DEBUG1("addr:0x%x",
                APP_HH_OTAFWDL_SFLASH_BASE_ADDR - ds_file + ds_new + offset + ext_addr);
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
            scru_dump_hex(linebuf, "write", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
            /* translate the record to the new DS base address */
            if (app_hh_otafwdl_ota_writemem(handle,
                APP_HH_OTAFWDL_SFLASH_BASE_ADDR - ds_file + ds_new + offset + ext_addr,
                len, linebuf))
            {
                break;
            }

            printf(".");
            fflush(stdout);
        }
        else if (type == 1) /* END record */
        {
            successful = TRUE;
            break;
        }
        else if (type == 4) /* Extended Linear Address */
        {
            ext_addr = (linebuf[0] << 8) | linebuf[1];
            ext_addr = ext_addr << 16;
            APP_DEBUG1("Extended Linear Address:0x%x", ext_addr);
        }
    }

    if (successful)
    {
        APP_INFO0("\nVerifying");

        /* Seek to the beginning of the file */
        fseek(fp, 0, SEEK_SET);
        ext_addr = 0;

        successful = FALSE;
        while (TRUE)
        {
            len = sizeof(linebuf);
            if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
            {
                /* Not successful, should finish on a type 1 record */
                break;
            }
            APP_DEBUG1("app_hex_read type:0x%x offset:0x%x len:0x%x",
                type, offset, len);

            if (type == 0)
            {
                /* do not check the pre-DS records */
                if (offset + ext_addr < APP_HH_OTAFWDL_SS_LOCATION_MAX)
                {
                    APP_DEBUG1("Pre-DS record: 0x%04X", offset + ext_addr);
                    continue;
                }

                if (len > sizeof(readbuf))
                {
                    APP_ERROR0("Buffer not large enough to read back");
                    break;
                }
                APP_DEBUG1("read mem:0x%x",
                    APP_HH_OTAFWDL_SFLASH_BASE_ADDR + ds_new + offset - ds_file + ext_addr);

                if (app_hh_otafwdl_ota_readmem(handle,
                    APP_HH_OTAFWDL_SFLASH_BASE_ADDR + ds_new + offset - ds_file + ext_addr,
                    len, readbuf))
                {
                    break;
                }
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
                scru_dump_hex(readbuf, "read", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
                scru_dump_hex(linebuf, "line", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
                /* Check if the data matches */
                if (memcmp(readbuf, linebuf, len))
                {
                    APP_ERROR0("Failure when reading back data");
                    break;
                }
                printf("R");
                fflush(stdout);
            }
            else if (type == 1)
            {
                successful = TRUE;
                break;
            }
            else if (type == 4) /* Extended Linear Address */
            {
                ext_addr = (linebuf[0] << 8) | linebuf[1];
                ext_addr = ext_addr << 16;
                APP_DEBUG1("Extended Linear Address:0x%x", ext_addr);
            }
        }
    }
    fclose(fp);

    if (successful)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_mcu_setreport
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
int app_hh_otafwdl_mcu_setreport(tBSA_HH_HANDLE handle, UINT16 len, UINT8 *p_data)
{
    tBSA_STATUS status;
    tBSA_HH_SET hh_set_param;
    UINT32 timeout = APP_HH_OTAFWDL_HIDFWU_TIMEOUT;

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = BSA_HH_RPTT_FEATURE;
    hh_set_param.param.set_report.report.length = len;
    memcpy(hh_set_param.param.set_report.report.data, p_data, len);

    app_hh_cb.ota_fw_dl.handle = handle;
    app_hh_cb.ota_fw_dl.waiting = TRUE;

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet failed:%d", status);
        app_hh_cb.ota_fw_dl.waiting = FALSE;
        return -1;
    }
    /* Wait unless it is a launch request */
    if (p_data[0] != APP_HH_OTAFWDL_HIDMCUU_LAUNCH)
    {
        while (app_hh_cb.ota_fw_dl.waiting && timeout)
        {
            usleep(1000);
            timeout--;
        }
        app_hh_cb.ota_fw_dl.waiting = FALSE;
        if ((app_hh_cb.ota_fw_dl.status != BSA_SUCCESS) || (timeout == 0))
        {
            APP_ERROR1("Set report failed:status=%d, timeout=%d", app_hh_cb.ota_fw_dl.status, timeout);
            return -1;
        }
    }
    else
    {
        app_hh_cb.ota_fw_dl.waiting = FALSE;
    }
    return 0;
}

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
int app_hh_otafwdl_mcu_readmem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_SETUP_READ_SIZE], *p_data;
    UINT16 rlen, index;

    /* Limit the maximum size of the read length */
    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDMCUU_SETUP_READ);
    UINT32_TO_STREAM(p_data, addr);
    UINT16_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_READSIZE);
    UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(7, report));
    if (app_hh_otafwdl_ota_setreport(handle, sizeof(report), report))
    {
        return -1;
    }
    index = 0;
    while (1)
    {
        /* Remaining length to read */
        rlen = len - index;
        /* Reached end? */
        if (rlen == 0)
        {
            break;
        }
        /* Is remaining length larger than configured read size? */
        else if (rlen > APP_HH_OTAFWDL_HIDFWU_READSIZE)
        {
            rlen = APP_HH_OTAFWDL_HIDFWU_READSIZE;
        }
        if (app_hh_otafwdl_ota_getreport(handle, APP_HH_OTAFWDL_HIDMCUU_READ, rlen, &p_buf[index]))
        {
            return -1;
        }
        /* Increment index */
        index += rlen;
    }
    return 0;
}

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
int app_hh_otafwdl_mcu_writemem(tBSA_HH_HANDLE handle, UINT32 addr, UINT16 len, UINT8 *p_buf)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(APP_HH_OTAFWDL_HEX_RECORD_SIZE)], *p_data;
    UINT16 wlen;

    /* Check the user is not trying to write too large */
    if ((UINT16)(APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(len)) > sizeof(report))
    {
        APP_ERROR0("Record too large to be written, is this an Intel Hex file record??");
        return -1;
    }
    APP_DEBUG1("app_hh_otafwdl_ota_writemem addr:0x%x ,len:%d, writesize:%d"
                , addr, len, app_hh_cb.ota_fw_dl.otafwu_writesize);

    while (len)
    {
        /* Make sure that the length does not override MTU */
        wlen = len;
        if (wlen > app_hh_cb.ota_fw_dl.otafwu_writesize)
        {
            wlen = app_hh_cb.ota_fw_dl.otafwu_writesize;
        }

        /* Write the next chunk */
        p_data = report;
        UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDMCUU_WRITE);
        UINT32_TO_STREAM(p_data, addr);
        UINT16_TO_STREAM(p_data, wlen);
        memcpy(p_data, p_buf, wlen);
        p_data += wlen;
        UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(wlen + 7, report));

        if (app_hh_otafwdl_ota_setreport(handle, APP_HH_OTAFWDL_HIDFWU_WRITE_SIZE(wlen), report))
        {
            return -1;
        }
        /* Increment the pointers and counters */
        p_buf += wlen;
        addr += wlen;
        len -= wlen;
    }
    return 0;
}

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
int app_hh_otafwdl_mcu_fuenable(tBSA_HH_HANDLE handle)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_ENABLE_SIZE], *p_data;

    APP_DEBUG0("app_hh_otafwdl_mcu_fuenable");
    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDMCUU_ENABLE);
    return app_hh_otafwdl_ota_setreport(handle, sizeof(report), report);
}

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
int app_hh_otafwdl_mcu_launch(tBSA_HH_HANDLE handle, UINT32 addr, UINT32 start_addr, UINT32 len)
{
    UINT8 report[APP_HH_OTAFWDL_HIDMCUU_LAUNCH_SIZE], *p_data;

    p_data = report;
    UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDMCUU_LAUNCH);
    UINT32_TO_STREAM(p_data, addr);
    UINT16_TO_STREAM(p_data, 8);
    UINT32_TO_STREAM(p_data, start_addr);
    UINT32_TO_STREAM(p_data, len);
    UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(15, report));
    return app_hh_otafwdl_mcu_setreport(handle, sizeof(report), report);
}


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
int app_hh_otafwdl_mcuupdate_burn(tBSA_HH_HANDLE handle, UINT32 ds_new)
{
    FILE *fp;
    UINT32 ds_file;
    UINT16 len, offset;
    UINT8 linebuf[APP_HH_OTAFWDL_READBUF_SIZE], readbuf[APP_HH_OTAFWDL_HEX_RECORD_SIZE], type;
    BOOLEAN successful;
    UINT32 total_len = 0;

    /* Open the file */
    fp = fopen("mcu.hex", "r");
    if (fp == NULL)
    {
        APP_ERROR0("Failed opening file mcu.hex");
        return -1;
    }

    APP_INFO0("Writing");

    /* Mark file DS offset as not initialized */
    ds_file = 0xFFFFFFFF;

    /* Read all the records in the firmware file */
    successful = FALSE;
    while (TRUE)
    {
        len = sizeof(linebuf);
        if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
        {
            /* Not successful, should finish on a type 1 record */
            break;
        }

        if (type == 0) /* DATA record */
        {
            if (app_hh_otafwdl_mcu_writemem(handle, APP_HH_OTAFWDL_EEPROM_BASE_ADDR - ds_file + ds_new + offset, len, linebuf))
            {
                break;
            }
            total_len = total_len + len;
            printf(".");
            fflush(stdout);
        }
        else if (type == 1) /* END record */
        {
            successful = TRUE;
            break;
        }
    }

    if (successful)
    {
        APP_INFO0("\nVerifying");

        /* Seek to the beginning of the file */
        fseek(fp, 0, SEEK_SET);

        successful = FALSE;
        while (TRUE)
        {
            len = sizeof(linebuf);
            if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
            {
                /* Not successful, should finish on a type 1 record */
                break;
            }

            if (type == 0)
            {
                if (len > sizeof(readbuf))
                {
                    APP_ERROR0("Buffer not large enough to read back");
                    break;
                }
                if (app_hh_otafwdl_mcu_readmem(handle, APP_HH_OTAFWDL_EEPROM_BASE_ADDR - ds_file + ds_new + offset, len, readbuf))
                {
                    break;
                }
                /* scru_dump_hex(readbuf, "read", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG); */
                /* Check if the data matches */
                if (memcmp(readbuf, linebuf, len))
                {
                    APP_ERROR0("Failure when reading back data");
                    break;
                }
                printf("R");
                fflush(stdout);
            }
            else if (type == 1)
            {
                successful = TRUE;
                break;
            }
        }
    }
    fclose(fp);

    if (successful)
    {
        return total_len;
    }
    else
    {
        return -1;
    }
}

/*******************************************************************************
 **
 ** Function         app_hh_otafwdl_cback
 **
 ** Description      Example of callback function for OTA fw download
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_hh_otafwdl_cback(tBSA_HH_EVT event, tBSA_HH_MSG *p_data)
{

    BOOLEAN rv = FALSE;

    switch (event)
    {
    case BSA_HH_OPEN_EVT:
        if (app_hh_cb.ota_fw_dl.waiting && (p_data->open.handle == app_hh_cb.ota_fw_dl.handle))
        {
            app_hh_cb.ota_fw_dl.status = p_data->open.status;
            app_hh_cb.ota_fw_dl.waiting = FALSE;
        }
        break;

    /* coverity[MISSING BREAK] False-positive: Unplug event is handled as a close event */
    case BSA_HH_VC_UNPLUG_EVT:
    case BSA_HH_CLOSE_EVT:
        break;

    case BSA_HH_GET_REPORT_EVT:
        if (app_hh_cb.ota_fw_dl.waiting && (p_data->get_report.handle == app_hh_cb.ota_fw_dl.handle))
        {
            app_hh_cb.ota_fw_dl.status = p_data->get_report.status;
            if ((app_hh_cb.ota_fw_dl.status == BSA_SUCCESS) &&
                (p_data->get_report.report.length > 0))
            {
                /* Memory read report */
                if ((p_data->get_report.report.data[0] == APP_HH_OTAFWDL_HIDFWU_READ) ||
                    (p_data->get_report.report.data[0] == APP_HH_OTAFWDL_HIDMCUU_READ))
                {
                    /* Check length of memory data matches */
                    if (p_data->get_report.report.length !=
                        (APP_HH_OTAFWDL_HIDFWU_READSIZE + APP_HH_OTAFWDL_HIDFWU_READ_HEADER_SIZE + 1))
                    {
                        app_hh_cb.ota_fw_dl.status = BSA_ERROR_CLI_BAD_RSP_SIZE;
                    }
                    else
                    {
                        /* Check checksum is ok */
                        if (!app_hh_otafwdl_ota_cksum(p_data->get_report.report.length,
                            p_data->get_report.report.data))
                        {
                            memcpy(app_hh_cb.ota_fw_dl.p_buf,
                                &p_data->get_report.report.data[APP_HH_OTAFWDL_HIDFWU_READ_HEADER_SIZE], app_hh_cb.ota_fw_dl.len);
                        }
                        else
                        {
                            app_hh_cb.ota_fw_dl.status = BSA_ERROR_CLI_BAD_MSG;
                        }
                    }
                }
                /* Other reports */
                else
                {
                    /* Check the received data is smaller than max length */
                    if (app_hh_cb.ota_fw_dl.len < p_data->get_report.report.length)
                    {
                        APP_INFO1("HID report returned larger than expected maximum size: %u > %u",
                                p_data->get_report.report.length, app_hh_cb.ota_fw_dl.len);
                    }
                    else
                    {
                        app_hh_cb.ota_fw_dl.len = p_data->get_report.report.length;
                    }
                    memcpy(app_hh_cb.ota_fw_dl.p_buf, p_data->get_report.report.data, app_hh_cb.ota_fw_dl.len);
                }
            }
            app_hh_cb.ota_fw_dl.waiting = FALSE;
            rv = TRUE;
        }
        break;

    case BSA_HH_SET_REPORT_EVT:
        /* If waiting this handle waits for an event, unlock it */
        if (app_hh_cb.ota_fw_dl.waiting && (p_data->set_report.handle == app_hh_cb.ota_fw_dl.handle))
        {
            app_hh_cb.ota_fw_dl.status = p_data->set_report.status;
            app_hh_cb.ota_fw_dl.waiting = FALSE;
            rv = TRUE;
        }
        break;

    case BSA_HH_GET_PROTO_EVT:
    case BSA_HH_SET_PROTO_EVT:
    case BSA_HH_GET_IDLE_EVT:
    case BSA_HH_SET_IDLE_EVT:
    case BSA_HH_MIP_START_EVT:
    case BSA_HH_MIP_STOP_EVT:
        break;

    default:
        APP_ERROR1("bad event:%d", event);
        break;
    }

    return rv;
}


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
int app_hh_otafwdl_fwupdate_nofs(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ds_location, vs_location;
    UINT16 len, vs_length;
    UINT8 ssbuf[APP_HH_OTAFWDL_SS_LENGTH], *p_data;
    int burn_status;

    app_hh_event_cback_register(app_hh_otafwdl_cback);

    /* Start the OTAFWU */
    if (app_hh_otafwdl_fwupdate_start(&handle))
    {
        return -1;
    }

    /* Search for static section in EEPROM */
    if (app_hh_otafwdl_ota_readmem(handle, APP_HH_OTAFWDL_EEPROM_BASE_ADDR, sizeof(ssbuf), ssbuf))
    {
        return -1;
    }

    /* Check if it contains the SS identifier 01 08 00 */
    if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
    {
        APP_ERROR0("Static section not found at beginning of EEPROM");
        return -1;
    }

    /* scru_dump_hex(ssbuf, "Static section", 48, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG); */

    /* Get the DS VS OFFSETs (config entry at 0x1b in SS: id=2) */
    if (ssbuf[0x1b] != 2)
    {
        APP_ERROR0("DS/VS offsets not found in SS");
        return -1;
    }
    p_data = &ssbuf[0x1c];
    STREAM_TO_UINT16(len, p_data);
    if (len != 10)
    {
        APP_ERROR0("DS/VS offsets length not correct in SS");
        return -1;
    }
    STREAM_TO_UINT32(ds_location, p_data);
    STREAM_TO_UINT32(vs_location, p_data);
    STREAM_TO_UINT16(vs_length, p_data);

    APP_DEBUG1("DS location = x%08X - VS location = x%08X - VS length = x%04X", ds_location, vs_location, vs_length);

    /* Check locations are supported */
    if ((vs_location != APP_HH_OTAFWDL_VS_LOCATION) || (vs_length != APP_HH_OTAFWDL_VS_LENGTH))
    {
        APP_ERROR0("EEPROM layout not supported");
        return -1;
    }

    /* Enable the firmware update on the remote (result no checked because we must launch anyway) */
    app_hh_otafwdl_ota_fuenable(handle);

    burn_status = app_hh_otafwdl_fwupdate_burn(handle, ds_location);

    /* Exit OTA mode */
    app_hh_otafwdl_ota_launch(handle, 0);

    if (burn_status == 0)
    {
        APP_INFO0("Finished updating firmware successfully");
    }
    else
    {
        APP_INFO0("Finished updating firmware on ERROR");
        return -1;
    }

    return 0;
}

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
int app_hh_otafwdl_fwupdate_fs(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ss_location, ss_new;
    UINT32 ds_location, ds_new, vs_location;
    UINT16 len, vs_length;
    UINT8 ssbuf[APP_HH_OTAFWDL_SS_LENGTH], linebuf[APP_HH_OTAFWDL_READBUF_SIZE], *p_data;
    BOOLEAN successful;
    int burn_status;

    app_hh_event_cback_register(app_hh_otafwdl_cback);

    /* Start the OTAFWU */
    if (app_hh_otafwdl_fwupdate_start(&handle))
    {
        return -1;
    }

    /* Search for static section in EEPROM */
    ss_location = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS1_LOCATION;
    ss_new = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS2_LOCATION;
    if (app_hh_otafwdl_ota_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
    {
        return -1;
    }

    /* Check if it contains the SS identifier 01 08 00 */
    if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
    {
        /* Search for static section in EEPROM */
        ss_location = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS2_LOCATION;
        ss_new = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS1_LOCATION;
        if (app_hh_otafwdl_ota_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
        {
            return -1;
        }
        /* Check if it contains the SS identifier 01 08 00 */
        if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
        {
            APP_ERROR0("SS location not found");
            return -1;
        }
    }
    APP_DEBUG1("SS location = x%08X", ss_location);

    /* scru_dump_hex(ssbuf, "Static section", 48, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG); */

    /* Get the DS VS OFFSETs (config entry at 0x1b in SS: id=2) */
    if (ssbuf[0x1b] != 2)
    {
        APP_ERROR0("DS/VS offsets not found in SS");
        return -1;
    }
    p_data = &ssbuf[0x1c];
    STREAM_TO_UINT16(len, p_data);
    if (len != 10)
    {
        APP_ERROR0("DS/VS offsets length not correct in SS");
        return -1;
    }
    STREAM_TO_UINT32(ds_location, p_data);
    STREAM_TO_UINT32(vs_location, p_data);
    STREAM_TO_UINT16(vs_length, p_data);

    APP_DEBUG1("DS location = x%08X - VS location = x%08X - VS length = x%04X", ds_location, vs_location, vs_length);

    /* Toggle between upper and lower DS sections */
    if (ds_location < APP_HH_OTAFWDL_HIDFWU_UPPER_OFFSET)
    {
        ds_new = ds_location + APP_HH_OTAFWDL_HIDFWU_UPPER_OFFSET;
    }
    else
    {
        ds_new = ds_location & APP_HH_OTAFWDL_DS_LOCATION_MASK;
    }

    /* Enable the firmware update on the remote (result no checked because we must launch anyway) */
    app_hh_otafwdl_ota_fuenable(handle);

    burn_status = app_hh_otafwdl_fwupdate_burn(handle, ds_new);

    while (TRUE)
    {
        successful = FALSE;

        /* If burn failed, exit */
        if (burn_status != 0)
        {
            break;
        }

        /* Update SS with new DS location */
        p_data = &ssbuf[0x1e];
        UINT32_TO_STREAM(p_data, ds_new);

        /* Invalidate SS until written correctly */
        ssbuf[0] = 0;
        if (app_hh_otafwdl_ota_writemem(handle, ss_new, sizeof(ssbuf), ssbuf))
        {
            break;
        }
        /* Verify SS just written */
        if (app_hh_otafwdl_ota_readmem(handle, ss_new, sizeof(ssbuf), linebuf))
        {
            break;
        }
        if (memcmp(ssbuf, linebuf, sizeof(ssbuf)))
        {
            APP_ERROR0("Failed reading back new Static section");
            break;
        }

        /* Mark new SS valid */
        ssbuf[0] = 1;
        if (app_hh_otafwdl_ota_writemem(handle, ss_new, 1, ssbuf))
        {
            break;
        }

        /* Invalidate old SS */
        ssbuf[0] = 0;
        if (app_hh_otafwdl_ota_writemem(handle, ss_location, 1, ssbuf))
        {
            break;
        }
        successful = TRUE;
        break;
    }

    /* Exit OTA mode */
    app_hh_otafwdl_ota_launch(handle, 0);

    if (successful)
    {
        APP_INFO0("Finished updating firmware successfully");
    }
    else
    {
        APP_INFO0("Finished updating firmware on ERROR");
        return -1;
    }

    return 0;
}


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
int app_hh_otafwdl_mcuupdate_fs(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ss_location;
    UINT32 ds_location, ds_new, vs_location;
    UINT16 len, vs_length;
    UINT8 ssbuf[APP_HH_OTAFWDL_SS_LENGTH], *p_data;
    BOOLEAN successful;
    int burned_len;

    app_hh_event_cback_register(app_hh_otafwdl_cback);

    /* Start the OTAFWU */
    if (app_hh_otafwdl_fwupdate_start(&handle))
    {
        return -1;
    }

    /* Search for static section in EEPROM */
    ss_location = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS1_LOCATION;
    if (app_hh_otafwdl_mcu_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
    {
        return -1;
    }

    /* Check if it contains the SS identifier 01 08 00 */
    if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
    {
        /* Search for static section in EEPROM */
        ss_location = APP_HH_OTAFWDL_EEPROM_BASE_ADDR + APP_HH_OTAFWDL_SS2_LOCATION;
        if (app_hh_otafwdl_mcu_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
        {
            return -1;
        }
        /* Check if it contains the SS identifier 01 08 00 */
        if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
        {
            APP_ERROR0("SS location not found");
            return -1;
        }
    }
    APP_DEBUG1("SS location = x%08X", ss_location);

    /* Get the DS VS OFFSETs (config entry at 0x1b in SS: id=2) */
    if (ssbuf[0x1b] != 2)
    {
        APP_ERROR0("DS/VS offsets not found in SS");
        return -1;
    }
    p_data = &ssbuf[0x1c];
    STREAM_TO_UINT16(len, p_data);
    if (len != 10)
    {
        APP_ERROR0("DS/VS offsets length not correct in SS");
        return -1;
    }
    STREAM_TO_UINT32(ds_location, p_data);
    STREAM_TO_UINT32(vs_location, p_data);
    STREAM_TO_UINT16(vs_length, p_data);

    APP_DEBUG1("DS location = x%08X - VS location = x%08X - VS length = x%04X", ds_location, vs_location, vs_length);

    /* Toggle between upper and lower DS sections */
    if (ds_location < APP_HH_OTAFWDL_HIDFWU_UPPER_OFFSET)
    {
        ds_new = ds_location + APP_HH_OTAFWDL_HIDFWU_UPPER_OFFSET;
    }
    else
    {
        ds_new = ds_location & APP_HH_OTAFWDL_DS_LOCATION_MASK;
    }

    /* Enable the firmware update on the remote (result no checked because we must launch anyway) */
    app_hh_otafwdl_mcu_fuenable(handle);
    burned_len = app_hh_otafwdl_mcuupdate_burn(handle, ds_new);

    while (TRUE)
    {
        successful = FALSE;

        /* If burn failed, exit */
        if (burned_len < 0)
        {
            break;
        }
        successful = TRUE;
        break;

    }

    /* Exit OTA mode */
    APP_INFO1("call app_hh_otafwdl_mcu_launch ds_new:0x%x, bsa_status:%d",
            ds_new, burned_len);
    app_hh_otafwdl_mcu_launch(handle, 0, ds_new, burned_len);

    if (successful)
    {
        APP_INFO0("Finished updating firmware successfully");
    }
    else
    {
        APP_INFO0("Finished updating firmware on ERROR");
        return -1;
    }

    return 0;
}



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
 **    0x0000   +--------------+  SS
 **             |              |
 **             | ............ |
 **    0x3000   +--------------+  DS1
 **             |              |
 **             |  .........   |
 **    0xF000   +--------------+  DS2
 **             |              |
 **             |     ..       |
 *******************************************************************************/
int app_hh_otafwdl_fwupdate_sflash_nofs(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ss_location;
    UINT32 ds_location, ds_new, vs_location;
    UINT16 len, vs_length;
    UINT8 ssbuf[APP_HH_OTAFWDL_SS_LENGTH], *linebuf, *p_data, *ss_backup_buf;
    BOOLEAN successful;
    int burn_status;
    UINT16 ss_length, ss_address;

    app_hh_event_cback_register(app_hh_otafwdl_cback);

    /* Start the OTAFWU */
    if (app_hh_otafwdl_fwupdate_start(&handle))
    {
        return -1;
    }

    /* Search for static section in SFLASH */
    ss_location = APP_HH_OTAFWDL_SFLASH_BASE_ADDR;
    if (app_hh_otafwdl_ota_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
    {
        return -1;
    }

    /* Check if it contains the SS identifier 01 08 00 */
    if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
    {
        APP_ERROR0("SS location not found");
        return -1;
    }
    APP_DEBUG1("SS location = x%08X", ss_location);

#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
    scru_dump_hex(ssbuf, "Static section", 48, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
    /* Get the DS VS OFFSETs (config entry at 0x1b in SS: id=2) */
    if (ssbuf[0x1b] != 2)
    {
        APP_ERROR0("DS/VS offsets not found in SS");
        return -1;
    }
    p_data = &ssbuf[0x1c];
    STREAM_TO_UINT16(len, p_data);
    if (len != 10)
    {
        APP_ERROR0("DS/VS offsets length not correct in SS");
        return -1;
    }
    STREAM_TO_UINT32(ds_location, p_data);
    STREAM_TO_UINT32(vs_location, p_data);
    STREAM_TO_UINT16(vs_length, p_data);

    APP_DEBUG1("DS location = x%08X - VS location = x%08X - VS length = x%04X", ds_location, vs_location, vs_length);

    /* Toggle between upper and lower DS sections */
    if (ds_location == APP_HH_OTAFWDL_SFLASH_DS1_LOCATION)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS2_LOCATION;
    }
    else if(ds_location == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS1_LOCATION;
    }
    else if(ds_location == APP_HH_OTAFWDL_SFLASH_DS1_LOCATION2)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS2_LOCATION2;
    }
    else if(ds_location == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION2)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS1_LOCATION2;
    }
    else
    {
        APP_ERROR1("DS address was wrong!! 0x%x", ds_location);
        return -1;
    }

    /* Enable the firmware update on the remote (result no checked because we must launch anyway) */
    app_hh_otafwdl_ota_fuenable(handle);

    burn_status = app_hh_otafwdl_fwupdate_sflash_burn(handle, ds_new);
    APP_DEBUG1("app_hh_otafwdl_fwupdate_sflash_nofs burn_status:%d", burn_status);

    ss_backup_buf = malloc(APP_HH_OTAFWDL_SFLASH_SS_LEN);
    if (ss_backup_buf == NULL)
    {
        APP_ERROR0("memory alloc error!");
        return -1;
    }
    linebuf = malloc(APP_HH_OTAFWDL_SFLASH_SS_LEN);
    if (linebuf == NULL)
    {
        APP_ERROR0("memory alloc error!");
        free(ss_backup_buf);
        return -1;
    }

    APP_DEBUG0("app_hh_otafwdl_fwupdate_sflash_nofs update SS sector");

    while (TRUE)
    {
        successful = FALSE;

        /* If burn failed, exit */
        if (burn_status != 0)
        {
            break;
        }

        /* read SS */
        APP_DEBUG1("app_hh_otafwdl_fwupdate_sflash_nofs read SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        if (app_hh_otafwdl_ota_readmem(handle, ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN, ss_backup_buf))
        {
            break;
        }

        /* Update SS with new DS location */
        p_data = ss_backup_buf+0x1e;
        UINT32_TO_STREAM(p_data, ds_new);

        /* erase SS */
        APP_DEBUG1("app_hh_otafwdl_fwupdate_sflash_nofs erase SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        app_hh_otafwdl_ota_erasemem(handle, ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);

        *ss_backup_buf = 1;
        ss_length = APP_HH_OTAFWDL_SFLASH_SS_LEN;
        ss_address = 0;
        APP_DEBUG1("write modified SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        while (ss_length > 0)
        {
            if (ss_length < APP_HH_OTAFWDL_HEX_RECORD_SIZE)
            {
                if (app_hh_otafwdl_ota_writemem(handle, ss_location + ss_address, ss_length, ss_backup_buf+ss_address))
                {
                    break;
                }
                ss_length = 0;
            }
            else
            {
                if (app_hh_otafwdl_ota_writemem(handle, ss_location + ss_address, APP_HH_OTAFWDL_HEX_RECORD_SIZE, ss_backup_buf+ss_address))
                {
                    break;
                }
                ss_length -= APP_HH_OTAFWDL_HEX_RECORD_SIZE;
                ss_address += APP_HH_OTAFWDL_HEX_RECORD_SIZE;
            }
        }
        APP_DEBUG1("verify SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        /* Verify SS just written */
        if (app_hh_otafwdl_ota_readmem(handle, ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN, linebuf))
        {
            break;
        }
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
        scru_dump_hex(ssbuf, "write", sizeof(ssbuf), TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
        scru_dump_hex(linebuf, "read", sizeof(ssbuf), TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
        if (memcmp(ss_backup_buf, linebuf, APP_HH_OTAFWDL_SFLASH_SS_LEN))
        {
            APP_ERROR0("Failed reading back new Static section");
            break;
        }

        successful = TRUE;
        break;
    }

    free(ss_backup_buf);
    free(linebuf);

    /* Exit OTA mode */
    app_hh_otafwdl_ota_launch(handle, 0);

    if (successful)
    {
        APP_INFO0("Finished updating firmware successfully");
    }
    else
    {
        APP_INFO0("Finished updating firmware on ERROR");
        return -1;
    }

    return 0;
}

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
int app_hh_otafwdl_fwupdate_sflash_fs(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ds_location = 0, ds_new;
    int burn_status;

    app_hh_event_cback_register(app_hh_otafwdl_cback);

    /* Start the OTAFWU */
    if (app_hh_otafwdl_fwupdate_start(&handle))
    {
        return -1;
    }

    /* Search for Magic number in Failsafe Section of SFLASH */
    if (app_hh_otafwdl_fwupdate_search_alternate(handle, &ds_location))
    {
        APP_ERROR0("=> Search magic number error!!!");
        app_hh_otafwdl_ota_launch(handle, 0);
        return -1;
    }

    /* Toggle between upper and lower DS sections */
    if (ds_location == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS1_LOCATION;
    }
    else
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS2_LOCATION;
    }

    /* Enable the firmware update on the remote (result no checked because we must launch anyway) */
    app_hh_otafwdl_ota_fuenable(handle);

    burn_status = app_hh_otafwdl_fwupdate_sflash_burn(handle, ds_new);
    APP_DEBUG1("app_hh_otafwdl_fwupdate_sflash_nofs burn_status:%d", burn_status);

    /* If burn failed, exit */
    if (burn_status != 0)
    {
        APP_ERROR0("=> Burn failed!!!");
        app_hh_otafwdl_ota_launch(handle, 0);
        return -1;
    }

    /* erase Failsafe Section */
    APP_DEBUG1("app_hh_otafwdl_fwupdate_sflash_fs erase Failsafe address:0x%x length:0x%x", APP_HH_OTAFWDL_SFLASH_BASE_ADDR + APP_HH_OTAFWDL_SFLASH_FAILSAFE_BASE_ADDR, APP_HH_OTAFWDL_SFLASH_FAILSAFE_LEN);
    if (app_hh_otafwdl_ota_erasemem(handle, APP_HH_OTAFWDL_SFLASH_BASE_ADDR + APP_HH_OTAFWDL_SFLASH_FAILSAFE_BASE_ADDR, APP_HH_OTAFWDL_SFLASH_FAILSAFE_LEN))
    {
        APP_ERROR0("=> Erase Failsafe Section failed!!!");
        app_hh_otafwdl_ota_launch(handle, 0);
        return -1;
    }

    if (ds_new == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION)
    {
        if (app_hh_otafwdl_fwupdate_write_alternate(handle, ds_new))
        {
            APP_ERROR0("=> Write alternate failed!!!");
            app_hh_otafwdl_ota_launch(handle, 0);
            return -1;
        }
    }

    /* Exit OTA mode */
    app_hh_otafwdl_ota_launch(handle, 0);

    APP_INFO0("\nFinished updating firmware successfully");

    return 0;
}

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
int app_hh_otafwdl_fwupdate_search_alternate(tBSA_HH_HANDLE handle, UINT32 *ds_location)
{
    UINT8 alt_buf[APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_DS2_OFFSET_SIZE], *p_data;
    UINT32 mn_location;

    /* Search for Alternate DS in SFLASH */
    APP_INFO0("=> Search for Magic number in Failsafe Section of SFLASH");

    memset(&alt_buf, 0, sizeof(alt_buf));

    mn_location = APP_HH_OTAFWDL_SFLASH_BASE_ADDR + APP_HH_OTAFWDL_SFLASH_FAILSAFE_BASE_ADDR + APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_OFFSET;
    if (app_hh_otafwdl_ota_readmem(handle, mn_location, APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_DS2_OFFSET_SIZE, alt_buf))
    {
        return -1;
    }

    /* Check if it contains the Magic number */
    if (memcmp(FailSafeMagicNumberArray, alt_buf, APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_LEN) != 0)
    {
        APP_INFO0("=> There are not exist Magic number");

#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
        APP_INFO1("=> Mem Magic number=0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
            alt_buf[0],alt_buf[1],alt_buf[2],alt_buf[3],alt_buf[4],alt_buf[5],alt_buf[6],alt_buf[7]);
#endif
    }
    else
    {
        APP_INFO0("=> Magic number exist on Failsafe Section!!!");

#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
        APP_INFO1("=> Mem Magic number=0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
            alt_buf[0],alt_buf[1],alt_buf[2],alt_buf[3],alt_buf[4],alt_buf[5],alt_buf[6],alt_buf[7]);
#endif

        p_data = &alt_buf[APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_LEN];
        STREAM_TO_UINT32(*ds_location, p_data);
        APP_INFO1("=> Alternate DS location=0x%X!!!", *ds_location);
    }

    return 0;
}

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
int app_hh_otafwdl_fwupdate_write_alternate(tBSA_HH_HANDLE handle, UINT32 ds_location)
{
    UINT8 alt_buf[APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_DS2_OFFSET_SIZE], *p_data;
    UINT32 mn_location;

    /* Search for Alternate DS in SFLASH */
    APP_INFO0("\n=> Write Magic number and DS2 offset in Failsafe Section of SFLASH");

    memset(&alt_buf, 0, sizeof(alt_buf));
    memcpy (alt_buf, FailSafeMagicNumberArray, APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_LEN);
    p_data = &alt_buf[APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_LEN];
    UINT32_TO_STREAM(p_data, ds_location);

    mn_location = APP_HH_OTAFWDL_SFLASH_BASE_ADDR + APP_HH_OTAFWDL_SFLASH_FAILSAFE_BASE_ADDR + APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_OFFSET;
    if (app_hh_otafwdl_ota_writemem(handle, mn_location, APP_HH_OTAFWDL_SFLASH_MAGIC_NUMBER_DS2_OFFSET_SIZE, alt_buf))
    {
        return -1;
    }

    return 0;
}


/******************************************************************************
**
** Function         app_hh_otafwdl_get_remote_fastboot_size
**
** Description      to calculate the remote controller fastboot size
**
** Parameters       buf:contain the remote SS data
**
** Returns          remote fastboot size
****************************************************************************/
int app_hh_otafwdl_get_remote_fastboot_size(UINT8 *buf)
{
    UINT32 idx = APP_HH_OTAFWDL_FASTBOOT_SS_LENGTH-1;

    while (buf[--idx] == 0xff);

    return (idx+1-APP_HH_OTAFWDL_FASTBOOT_SS_SIZE);
}

/******************************************************************************
**
** Function         app_hh_otafwdl_copy_fastboot
**
** Description      copy fastboot data to buf
**
** Parameters       buf:store fastboot data  size:return the fastboot size
**
** Returns          status: 1 if success / 0 otherwise
****************************************************************************/
int app_hh_otafwdl_copy_fastboot(UINT8 *buf,UINT16 *size)
{
    FILE *fp;
    int idx=0;
    *size = 0;
    fp = fopen("fastboot.txt","r");
    if (fp == NULL)
    {
        APP_ERROR0("fastboot file is not exist");
        return 0;
    }
    while ((fscanf(fp,"%x",&buf[idx++])) != EOF)
        (*size)++;

    fclose(fp);
    return 1;
}

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
int app_hh_otafwdl_fwupdate_fastboot_sflash(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ss_location;
    UINT32 ds_location, ds_new, vs_location,ds_offset;
    UINT16 vs_length, local_fastboot_size, remote_fastboot_size;
    UINT8 ssbuf[APP_HH_OTAFWDL_FASTBOOT_SS_LENGTH], *linebuf, *p_data, *ss_backup_buf,fast_boot_data[APP_HH_OTAFWDL_FASTBOOT_MAX_SIZE];
    BOOLEAN successful, remote_contain_fastboot, local_update_fastboot;
    int burn_status;
    UINT16 ss_length, ss_address;
    app_hh_event_cback_register(app_hh_otafwdl_cback);
    /* Start the OTAFWU */

    if (app_hh_otafwdl_fwupdate_start(&handle))
    {
        return -1;
    }

    /* Search for static section in SFLASH */
    ss_location = APP_HH_OTAFWDL_SFLASH_BASE_ADDR;
    if (app_hh_otafwdl_ota_readmem(handle, ss_location, sizeof(ssbuf), ssbuf))
    {
        return -1;
    }

    /* Check if it contains the SS identifier 01 08 00 */
    if (memcmp("\x01\x08\x00", ssbuf, 3) != 0)
    {
        APP_ERROR0("SS location not found");
        return -1;
    }
    APP_DEBUG1("SS location = x%08X", ss_location);
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
    scru_dump_hex(ssbuf, "Static section", 48, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
    local_update_fastboot = app_hh_otafwdl_copy_fastboot(&fast_boot_data[0],&local_fastboot_size);
    if (local_update_fastboot)
    {
        APP_DEBUG1("local fast boot size 0x%x",local_fastboot_size);
    }
    /* 0x0A is the fastboot identifier*/
    if (ssbuf[APP_HH_OTAFWDL_FASTBOOT_OFFSET] == 0x0A)
    {
        remote_contain_fastboot = TRUE;
        remote_fastboot_size = app_hh_otafwdl_get_remote_fastboot_size(ssbuf);
        ds_offset = APP_HH_OTAFWDL_DSLOCATION_OFFSET + remote_fastboot_size;
        APP_DEBUG1("Remote contain fast boot size 0x%x",remote_fastboot_size);
    }
    else
    {
        remote_contain_fastboot = FALSE;
        remote_fastboot_size = 0;
        ds_offset = APP_HH_OTAFWDL_DSLOCATION_OFFSET;
        APP_DEBUG0("Remote contain no fast boot update  size fast boot");
    }
    p_data = &ssbuf[ds_offset];

    STREAM_TO_UINT32(ds_location, p_data);
    STREAM_TO_UINT32(vs_location, p_data);
    STREAM_TO_UINT16(vs_length, p_data);

    APP_DEBUG1("DS location = x%08X - VS location = x%08X - VS length = x%04X", ds_location, vs_location, vs_length);

    /* Toggle between upper and lower DS sections */
    if (ds_location == APP_HH_OTAFWDL_SFLASH_DS1_LOCATION)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS2_LOCATION;
    }
    else if(ds_location == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS1_LOCATION;
    }
    else if(ds_location == APP_HH_OTAFWDL_SFLASH_DS1_LOCATION2)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS2_LOCATION2;
    }
    else if(ds_location == APP_HH_OTAFWDL_SFLASH_DS2_LOCATION2)
    {
        ds_new = APP_HH_OTAFWDL_SFLASH_DS1_LOCATION2;
    }
    else
    {
        APP_ERROR1("DS address was wrong!! 0x%x", ds_location);
        return -1;
    }
    /* Enable the firmware update on the remote (result no checked because we must launch anyway) */
    app_hh_otafwdl_ota_fuenable(handle);

    burn_status = app_hh_otafwdl_fwupdate_sflash_burn(handle, ds_new);
    APP_DEBUG1("app_hh_otafwdl_fwupdate_fastboot_sflash burn_status:%d", burn_status);

    ss_backup_buf = malloc(APP_HH_OTAFWDL_SFLASH_SS_LEN);
    if (ss_backup_buf == NULL)
    {
        APP_ERROR0("memory alloc error!");
        return -1;
    }
    linebuf = malloc(APP_HH_OTAFWDL_SFLASH_SS_LEN);
    if (linebuf == NULL)
    {
        APP_ERROR0("memory alloc error!");
        free(ss_backup_buf);
        return -1;
    }

    APP_DEBUG0("app_hh_otafwdl_fwupdate_fastboot_sflash update SS sector");

    while (TRUE)
    {
        successful = FALSE;

        /* If burn failed, exit */
        if (burn_status != 0)
        {
            break;
        }

        /* read SS */
        APP_DEBUG1("app_hh_otafwdl_fwupdate_fastboot_sflash read SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        if (app_hh_otafwdl_ota_readmem(handle, ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN, ss_backup_buf))
        {
            break;
        }

        if (remote_contain_fastboot == TRUE && local_update_fastboot == FALSE)
        {
                p_data = ss_backup_buf+APP_HH_OTAFWDL_DSLOCATION_OFFSET + remote_fastboot_size;
                UINT32_TO_STREAM(p_data, ds_new);
        }
        else if (remote_contain_fastboot == TRUE && local_update_fastboot ==TRUE)
        {
            /* copy the remainder data in SS after the FASTBOOT_OFFSET */
            memmove(ss_backup_buf+APP_HH_OTAFWDL_FASTBOOT_OFFSET+local_fastboot_size,ss_backup_buf+APP_HH_OTAFWDL_FASTBOOT_OFFSET+remote_fastboot_size,APP_HH_OTAFWDL_FASTBOOT_SS_SIZE - APP_HH_OTAFWDL_FASTBOOT_OFFSET);
            memcpy(ss_backup_buf+APP_HH_OTAFWDL_FASTBOOT_OFFSET,fast_boot_data,local_fastboot_size);
            p_data = ss_backup_buf+APP_HH_OTAFWDL_DSLOCATION_OFFSET + local_fastboot_size;
            UINT32_TO_STREAM(p_data, ds_new);
        }
        else if (remote_contain_fastboot == FALSE && local_update_fastboot == TRUE)
        {
            /* copy the remainder data in SS after the FASTBOOT_OFFSET */
            memmove(ss_backup_buf+APP_HH_OTAFWDL_FASTBOOT_OFFSET+local_fastboot_size,ss_backup_buf+APP_HH_OTAFWDL_FASTBOOT_OFFSET+remote_fastboot_size,APP_HH_OTAFWDL_FASTBOOT_SS_SIZE - APP_HH_OTAFWDL_FASTBOOT_OFFSET);
            memcpy(ss_backup_buf+APP_HH_OTAFWDL_FASTBOOT_OFFSET,fast_boot_data,local_fastboot_size);
            p_data = ss_backup_buf+APP_HH_OTAFWDL_DSLOCATION_OFFSET + local_fastboot_size;
            UINT32_TO_STREAM(p_data, ds_new);
        }
        else if (remote_contain_fastboot == FALSE && local_update_fastboot == FALSE)
        {
             p_data = ss_backup_buf+APP_HH_OTAFWDL_DSLOCATION_OFFSET;
             UINT32_TO_STREAM(p_data, ds_new);
        }

        /* erase SS */
        APP_DEBUG1("app_hh_otafwdl_fwupdate_fastboot_sflash erase SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        app_hh_otafwdl_ota_erasemem(handle, ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);

        *ss_backup_buf = 1;
        ss_length = APP_HH_OTAFWDL_SFLASH_SS_LEN;
        ss_address = 0;
        APP_DEBUG1("write modified SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        while (ss_length > 0)
        {
            if (ss_length < APP_HH_OTAFWDL_HEX_RECORD_SIZE)
            {
                if (app_hh_otafwdl_ota_writemem(handle, ss_location + ss_address, ss_length, ss_backup_buf+ss_address))
                {
                    break;
                }
                ss_length = 0;
            }
            else
            {
                if (app_hh_otafwdl_ota_writemem(handle, ss_location + ss_address, APP_HH_OTAFWDL_HEX_RECORD_SIZE, ss_backup_buf+ss_address))
                {
                    break;
                }
                ss_length -= APP_HH_OTAFWDL_HEX_RECORD_SIZE;
                ss_address += APP_HH_OTAFWDL_HEX_RECORD_SIZE;
            }
        }
        APP_DEBUG1("verify SS address:0x%x length:0x%x", ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN);
        /* Verify SS just written */
        if (app_hh_otafwdl_ota_readmem(handle, ss_location, APP_HH_OTAFWDL_SFLASH_SS_LEN, linebuf))
        {
            break;
        }
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
        scru_dump_hex(ss_backup_buf, "write", sizeof(ssbuf), TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
        scru_dump_hex(linebuf, "read", sizeof(ssbuf), TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
        if (memcmp(ss_backup_buf, linebuf, APP_HH_OTAFWDL_SFLASH_SS_LEN))
        {
            APP_ERROR0("Failed reading back new Static section");
            break;
        }

        successful = TRUE;
        break;
    }

    free(ss_backup_buf);
    free(linebuf);

    /* Exit OTA mode */
    app_hh_otafwdl_ota_launch(handle, 0);

    if (successful)
    {
        APP_INFO0("Finished updating firmware successfully");
    }
    else
    {
        APP_INFO0("Finished updating firmware on ERROR");
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_ota_erasemem_len32
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
int app_hh_otafwdl_ota_erasemem_len32(tBSA_HH_HANDLE handle, UINT32 addr, UINT32 len)
{
    UINT8 report[APP_HH_OTAFWDL_HIDFWU_ERASE_SIZE], *p_data;
    UINT16 index = 0;

    while(len > 0)
    {
        if(len > 0x10000)
        {
            len -= 0x10000;
            p_data = report;
            UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_ERASE);
            UINT32_TO_STREAM(p_data, addr + (index * 0x10000));
            UINT16_TO_STREAM(p_data, 0xFFFF);
            UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(7, report));
            APP_DEBUG1("app_hh_otafwdl_ota_erasemem addr:0x%x ,len:0x%x", addr + (index * 0x10000), 0xFFFF);
            index++;
        }
        else
        {
            p_data = report;
            UINT8_TO_STREAM(p_data, APP_HH_OTAFWDL_HIDFWU_ERASE);
            UINT32_TO_STREAM(p_data, addr + (index * 0x10000));
            UINT16_TO_STREAM(p_data, len);
            UINT8_TO_STREAM(p_data, app_hh_otafwdl_ota_cksum(7, report));
            APP_DEBUG1("app_hh_otafwdl_ota_erasemem addr:0x%x ,len:0x%x", addr + (index * 0x10000), len);
            len = 0;
        }

        if (app_hh_otafwdl_ota_setreport(handle, sizeof(report), report))
        {
            return -1;
        }
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function        app_hh_otafwdl_dualboot_burn
 **
 ** Description     Burn an image in specified DS location
 **
 ** Parameters      handle: HID handle
 **                 ds_new: ds location to burn content at
 **
 ** Returns  app_hh_cb.ota_fw_dlatus: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_otafwdl_dualboot_burn(tBSA_HH_HANDLE handle, UINT32 ds_new)
{
    FILE *fp;
    UINT32 ds_file, ext_addr = 0, ds_len;
    UINT16 len, offset;
    UINT8 linebuf[APP_HH_OTAFWDL_READBUF_SIZE], readbuf[APP_HH_OTAFWDL_HEX_RECORD_SIZE], type;
    BOOLEAN successful;

    /* Open the file */
    fp = fopen("fw.hex", "r");
    if (fp == NULL)
    {
        APP_ERROR0("Failed opening file fw.hex");
        return -1;
    }

    /* Find DS Length */
    if (ds_new == APP_HH_OTAFWDL_DB_SF_DS1_CL_ADDR)
    {
        ds_len = APP_HH_OTAFWDL_DB_SF_DS1_CL_LEN
                + APP_HH_OTAFWDL_DB_SF_DS1_LE_LEN
                + APP_HH_OTAFWDL_DB_SF_CUST1_LEN;
    }
    else if (ds_new == APP_HH_OTAFWDL_DB_SF_DS2_CL_ADDR)
    {
        ds_len = APP_HH_OTAFWDL_DB_SF_DS2_CL_LEN
                + APP_HH_OTAFWDL_DB_SF_DS2_LE_LEN
                + APP_HH_OTAFWDL_DB_SF_CUST2_LEN;
    }
    else
    {
        APP_ERROR1("Failed to decide. invalid address 0x%x", ds_new);
        fclose(fp);
        return -1;
    }

    APP_INFO1("Erasing Addr:0x%x, len:0x%x",
        APP_HH_OTAFWDL_DB_SF_BASE_ADDR + ds_new, ds_len);

    /* erase first DS area */
    if (app_hh_otafwdl_ota_erasemem_len32(handle,
            APP_HH_OTAFWDL_DB_SF_BASE_ADDR + ds_new,
            ds_len))
    {
        fclose(fp);
        return -1;
    }

    APP_INFO0("Writing");

    /* Mark file DS offset as not initialized */
    ds_file = 0xFFFFFFFF;

    /* Read all the records in the firmware file */
    successful = FALSE;
    while (TRUE)
    {
        len = sizeof(linebuf);
        if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
        {
            /* Not successful, should finish on a type 1 record */
            break;
        }

        if (type == 0) /* DATA record */
        {
            /* if we have not reached the file DS section yet */
            if (ds_file == 0xFFFFFFFF)
            {
                /* if this is still a static section record */
                if (offset + ext_addr < APP_HH_OTAFWDL_SS_LOCATION_MAX)
                {
                    APP_DEBUG1("Pre-DS record: 0x%04X", offset);
                    continue;
                }
                /* first DS record found, save the base address */
                ds_file = offset;
            }
            APP_DEBUG1("ds_new:0x%x, offset:0x%x, ext_addr:0x%x, ds_file:0x%x",
                ds_new, offset, ext_addr, ds_file);
            APP_DEBUG1("addr:0x%x", ds_new + offset + ext_addr - ds_file);
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
            scru_dump_hex(linebuf, "write", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
            /* translate the record to the new DS base address */
            if (app_hh_otafwdl_ota_writemem(handle,
                APP_HH_OTAFWDL_DB_SF_BASE_ADDR + ds_new + offset + ext_addr - ds_file,
                len,
                linebuf))
            {
                break;
            }

            printf(".");
            fflush(stdout);
        }
        else if (type == 1) /* END record */
        {
            successful = TRUE;
            break;
        }
        else if (type == 4) /* Extended Linear Address */
        {
            ext_addr = (linebuf[0] << 8) | linebuf[1];
            ext_addr = ext_addr << 16;
            APP_DEBUG1("Extended Linear Address:0x%x", ext_addr);
        }
    }

    if (successful)
    {
        APP_INFO0("\nVerifying");

        /* Seek to the beginning of the file */
        fseek(fp, 0, SEEK_SET);
        ext_addr = 0;

        successful = FALSE;
        while (TRUE)
        {
            len = sizeof(linebuf);
            if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
            {
                /* Not successful, should finish on a type 1 record */
                break;
            }
            APP_DEBUG1("app_hex_read type:0x%x offset:0x%x len:0x%x",
                type, offset, len);

            if (type == 0)
            {
                /* do not check the pre-DS records */
                if (offset + ext_addr < APP_HH_OTAFWDL_SS_LOCATION_MAX)
                {
                    APP_DEBUG1("Pre-DS record: 0x%04X", offset + ext_addr);
                    continue;
                }

                if (len > sizeof(readbuf))
                {
                    APP_ERROR0("Buffer not large enough to read back");
                    break;
                }
                APP_DEBUG1("read mem:0x%x", ds_new + offset - ds_file + ext_addr);

                if (app_hh_otafwdl_ota_readmem(handle,
                    APP_HH_OTAFWDL_DB_SF_BASE_ADDR + ds_new + offset - ds_file + ext_addr,
                    len, readbuf))
                {
                    break;
                }
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
                scru_dump_hex(readbuf, "read", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
                scru_dump_hex(linebuf, "line", len, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
                /* Check if the data matches */
                if (memcmp(readbuf, linebuf, len))
                {
                    APP_ERROR0("Failure when reading back data");
                    break;
                }
                printf("R");
                fflush(stdout);
            }
            else if (type == 1)
            {
                successful = TRUE;
                break;
            }
            else if (type == 4) /* Extended Linear Address */
            {
                ext_addr = (linebuf[0] << 8) | linebuf[1];
                ext_addr = ext_addr << 16;
                APP_DEBUG1("Extended Linear Address:0x%x", ext_addr);
            }
        }
    }
    fclose(fp);

    if (successful)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

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
int app_hh_otafwdl_fwupdate_dualboot_rcu(void)
{
    tBSA_HH_HANDLE handle;
    UINT32 ss_location, ds_new;
    UINT8 sig[APP_HH_OTAFWDL_DB_SF_STATIC2_LEN], *ss_backup_buf, *read_buf;
    int burn_status;
    UINT16 ss_length, ss_address;

    app_hh_event_cback_register(app_hh_otafwdl_cback);

    /* 0. Connect HID */
    if (app_hh_otafwdl_fwupdate_start(&handle))
    {
        return -1;
    }

    /* 1. Enable the firmware update on the remote (result no checked because we must launch anyway) */
    app_hh_otafwdl_ota_fuenable(handle);

    /* 2. Check current signature status and DS offset */
    if (app_hh_otafwdl_ota_readmem(
            handle,
            APP_HH_OTAFWDL_DB_SF_BASE_ADDR + APP_HH_OTAFWDL_DB_SF_STATIC2_ADDR,
            sizeof(sig),
            sig))
    {
        return -1;
    }
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
    scru_dump_hex(sig, "Signature", sizeof(sig), TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif

    if (!memcmp(sig, app_hh_otafwdl_db_sig_bt1, APP_HH_OTAFWDL_DB_SF_STATIC2_LEN))
    {
        ds_new = APP_HH_OTAFWDL_DB_SF_DS2_CL_ADDR;
    }
    else if(!memcmp(sig, app_hh_otafwdl_db_sig_bt2, APP_HH_OTAFWDL_DB_SF_STATIC2_LEN))
    {
        ds_new = APP_HH_OTAFWDL_DB_SF_DS1_CL_ADDR;
    }
    else
    {
        APP_ERROR0("signature value was wrong!!");
        scru_dump_hex(sig, "Signature", sizeof(sig), TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
        ds_new = APP_HH_OTAFWDL_DB_SF_DS2_CL_ADDR;
    }

    APP_DEBUG1("New DS address is 0x%x", ds_new);

    /* 3. Program primary DS */
    burn_status = app_hh_otafwdl_dualboot_burn(handle, ds_new);
    if (burn_status)
    {
        APP_ERROR1("Burn new image failed status:%d", burn_status);
        return -1;
    }
    APP_DEBUG1("app_hh_otafwdl_fwupdate_fastboot_sflash burn_status:%d", burn_status);

    /* 4. Update Static Section 2 */
    ss_backup_buf = malloc(APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN);
    if (ss_backup_buf == NULL)
    {
        APP_ERROR0("memory alloc error!");
        return -1;
    }

    app_hh_otafwdl_ota_readmem(handle,
            APP_HH_OTAFWDL_DB_SF_BASE_ADDR + APP_HH_OTAFWDL_DB_SF_FAILSAFE_ADDR,
            APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN,
            ss_backup_buf);

    if (ds_new == APP_HH_OTAFWDL_DB_SF_DS1_CL_ADDR)
    {
        memcpy(ss_backup_buf + APP_HH_OTAFWDL_DB_SF_STATIC2_OFFSET,
                app_hh_otafwdl_db_sig_bt1, APP_HH_OTAFWDL_DB_SIG_LEN);
    }
    else if (ds_new == APP_HH_OTAFWDL_DB_SF_DS2_CL_ADDR)
    {
        memcpy(ss_backup_buf + APP_HH_OTAFWDL_DB_SF_STATIC2_OFFSET,
                app_hh_otafwdl_db_sig_bt2, APP_HH_OTAFWDL_DB_SIG_LEN);
    }

    APP_DEBUG1("erase SS2 address:0x%x length:0x%x",
            APP_HH_OTAFWDL_DB_SF_FAILSAFE_ADDR, APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN);
    app_hh_otafwdl_ota_erasemem(handle,
            APP_HH_OTAFWDL_DB_SF_FAILSAFE_ADDR,
            APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN);

    ss_location = APP_HH_OTAFWDL_DB_SF_BASE_ADDR + APP_HH_OTAFWDL_DB_SF_FAILSAFE_ADDR;
    ss_length = APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN;
    ss_address = 0;
    APP_DEBUG1("write modified SS address:0x%x length:0x%x", ss_location, ss_length);
    while (ss_length > 0)
    {
        if (ss_length < APP_HH_OTAFWDL_HEX_RECORD_SIZE)
        {
            if (app_hh_otafwdl_ota_writemem(
                    handle,
                    ss_location + ss_address,
                    ss_length,
                    ss_backup_buf + ss_address))
            {
                break;
            }
            ss_length = 0;
        }
        else
        {
            if (app_hh_otafwdl_ota_writemem(
                    handle,
                    ss_location + ss_address,
                    APP_HH_OTAFWDL_HEX_RECORD_SIZE,
                    ss_backup_buf + ss_address))
            {
                break;
            }
            ss_length -= APP_HH_OTAFWDL_HEX_RECORD_SIZE;
            ss_address += APP_HH_OTAFWDL_HEX_RECORD_SIZE;
        }
    }

    read_buf = malloc(APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN);

    APP_DEBUG1("verify SS address:0x%x length:0x%x",
            ss_location, APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN);
    /* Verify SS just written */
    if (app_hh_otafwdl_ota_readmem(
            handle,
            ss_location,
            APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN,
            read_buf))
    {
        APP_ERROR0("Failed reading back new Static section");
        return -1;
    }
#if (defined(APP_HH_OTAFWDL_DEBUG) && (APP_HH_OTAFWDL_DEBUG == TRUE))
    scru_dump_hex(ss_backup_buf, "write", APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN,
            TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
    scru_dump_hex(read_buf, "read", APP_HH_OTAFWDL_DB_SF_FAILSAFE_LEN,
            TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
    if (memcmp(ss_backup_buf, read_buf, APP_HH_OTAFWDL_SFLASH_SS_LEN))
    {
        APP_ERROR0("SS Verification failed");
        return -1;
    }

    free(ss_backup_buf);
    free(read_buf);

    /* Exit OTA mode */
    app_hh_otafwdl_ota_launch(handle, 0);

    APP_INFO0("Finished updating firmware successfully");
    return 0;
}
