/*****************************************************************************
**
**  Name:           hh.c
**
**  Description:    Bluetooth HH functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "hh.h"
#include "simple_app.h"
#include "bsa_api.h"
//
#include "bsa_trace.h"
#include "app_xml_utils.h"
#include "app_utils.h"

#if defined(BTHID_INCLUDED) && (BTHID_INCLUDED == TRUE)
#include "hh_bthid.h"
#endif

#ifndef HH_PROTO_MODE
#define HH_PROTO_MODE       BSA_HH_PROTO_RPT_MODE
/* #define HH_PROTO_MODE       BSA_HH_PROTO_BOOT_MODE */
#endif


/*
 * Global data
 */
static BOOLEAN          hh_open_rcv = FALSE;
static BOOLEAN          hh_close_rcv = FALSE;
static tBSA_STATUS      hh_open_status;
static UINT8            hh_uipc_channel;
static tBSA_HH_HANDLE   hh_handle;
static BOOLEAN          hh_dscpinfo_rcv = FALSE;

#if defined(BTHID_INCLUDED) && (BTHID_INCLUDED == TRUE)
static int bthid_fd = -1;
#endif


/*******************************************************************************
 **
 ** Function         hh_get_desc_filename
 **
 ** Description      Get the HID descriptor filename
 **
 ** Parameters       bd_addr: bd address of remote device
 **                  filename: filename string
 **
 ** Returns          void
 **
 *******************************************************************************/
void hh_get_desc_filename(BD_ADDR bd_addr, char *filename)
{
    sprintf(filename, "hh-desc-%02X-%02X-%02X-%02X-%02X-%02X.bin", bd_addr[0],
            bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
}

/*******************************************************************************
 **
 ** Function         hh_write_desc_data
 **
 ** Description      This function is used to write HID Descriptor data in a file
 **
 ** Parameters       bd_addr: bd address of remote device
 **                  p_desc_data: descriptor data
 **                  desc_len: descriptor len
 **
 ** Returns          int
 **
 *******************************************************************************/
int hh_write_desc_data(BD_ADDR bd_addr, UINT8 *p_desc_data, UINT16 desc_len)
{
    char hid_desc_filename[200];
    int fd;
    ssize_t nb_wrote;
    int status = -1;

    /* Build HID desc filename based on BdAddr of peer device */
    hh_get_desc_filename(bd_addr, hid_desc_filename);

    /* Open the file in Write & Create mode */
    fd = open(hid_desc_filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd >= 0)
    {
        printf("Write HID DESC Data in %s file\n", hid_desc_filename);
        nb_wrote = write(fd, p_desc_data, desc_len);
        if (nb_wrote < 0)
        {
            printf("cannot write data in file:%d\n", errno);
        }
        else if (nb_wrote != desc_len)
        {
            printf("bad size wrote:%d (expect:%d):%d\n",(int)nb_wrote, desc_len, errno);
        }
        else
        {
            status = 0; /* Write OK */
        }
        close(fd);
    }
    else
    {
        printf("cannot open/create HID desc file: %d\n", errno);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         hh_read_desc_data
 **
 ** Description      This function is used to read HID Desc data from a file
 **
 ** Parameters       bd_addr: bd address of remote device
 **                  p_desc_buf: descriptor buffer
 **                  desc_buf_size: descriptor buffer len
 ** Returns          int
 **
 *******************************************************************************/
int hh_read_desc_data(BD_ADDR bd_addr, UINT8 *p_desc_buf, UINT16 desc_buf_size)
{
    char hid_desc_filename[200];
    int fd;
    ssize_t nb_read;
    int ret_code = -1;;
    struct stat file_stat;

    /* Build Desc filename based on BdAddr of peer device */
    hh_get_desc_filename(bd_addr, hid_desc_filename);

    printf("Try to open desc file [%s]\n",hid_desc_filename);

    /* Open the file in Read only mode */
    fd = open(hid_desc_filename, O_RDONLY);
    if (fd >= 0)
    {
        /* If buffer pointer is null, this function is only used to check if
         * the HID desc file exist */
        if ((p_desc_buf != NULL) && (desc_buf_size != 0))
        {
            /* Get the size of the file */
            ret_code = fstat(fd, &file_stat);

            /* check the size of the file */
            if ((ret_code == 0) &&
                (file_stat.st_size > 0) &&
                (file_stat.st_size <= desc_buf_size))
            {
                printf("HID descriptor file found for this device (%d bytes)\n",
                        (int)file_stat.st_size);
                /* Read file */
                nb_read = read(fd, p_desc_buf, file_stat.st_size);
                if (nb_read < 0)
                {
                    printf("cannot read data from file: %d\n", errno);
                }
                else if (nb_read != file_stat.st_size)
                {
                    printf("bad size read:%d (expect:%d): %d\n",
                            (int)nb_read, (int)file_stat.st_size, errno);
                }
                else
                {
                    /* Read OK => Return file size */
                    ret_code = (int)file_stat.st_size;
                }
            }
            else
            {
                printf("bad file size :%d (max:%d): %d\n",
                        (int)file_stat.st_size, desc_buf_size, errno);
            }
        }
        else
        {
            ret_code = 0; /* OK, file exist */
        }
        close(fd);
    }
    else
    {
        printf("cannot open HID desc file: %d\n", errno);
    }

    return ret_code;
}


/*******************************************************************************
 **
 ** Function         hh_cback
 **
 ** Description      This function is the hh events callback
 **
 ** Parameters       event: hh event
 **                  p_data: hh event data pointer
 ** Returns          void
 **
 *******************************************************************************/
void hh_cback (tBSA_HH_EVT event, tBSA_HH_MSG *p_data)
{
    int hid_desc_size;
    UINT8 dev_dscp_data[BSA_HH_DSCPINFO_SIZE_MAX];
    tBSA_HH_GET         get_dscpinfo;
    tBSA_STATUS bsa_status;

    switch (event)
    {
    case BSA_HH_OPEN_EVT:               /* Connection Open*/
        printf("BSA_HH_OPEN_EVT event received\n");
        printf("\tStatus:%d", p_data->open.status);
        if (p_data->open.status == BSA_SUCCESS)
        {
            printf("=> SUCCESS\n");
            /* Read the Remote device xml file to have a fresh view */
            app_read_xml_remote_devices();

            app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->open.bd_addr,
                        BSA_HID_SERVICE_MASK);

            /* Update database => write on disk */
            app_write_xml_remote_devices();

#if defined(BTHID_INCLUDED) && (BTHID_INCLUDED == TRUE)
            bthid_fd = -1;
            printf("OPEN BT HID driver \n");
            /* Open BTHID driver */
            bthid_fd = hh_bthid_open();

#endif
            hid_desc_size = hh_read_desc_data(p_data->open.bd_addr,
                                              dev_dscp_data, sizeof(dev_dscp_data));
            if (hid_desc_size < 0)
            {
                printf("No HID descriptor file found for this device => Start HID Desc Read\n");
                hh_dscpinfo_rcv = FALSE;
                BSA_HhGetInit(&get_dscpinfo);
                get_dscpinfo.request = BSA_HH_DSCP_REQ;
                get_dscpinfo.handle = p_data->open.handle;
                bsa_status = BSA_HhGet(&get_dscpinfo);

                if(bsa_status != BSA_SUCCESS)
                {
                    fprintf(stderr, "hh_open: BSA_HhGet: fail status:%d\n", bsa_status);
                    /* Set hh_dscpinfo_rcv to TRUE to unlock the while in main function */
                    hh_dscpinfo_rcv = TRUE;
                }
            }
            else
            {
#if defined(BTHID_INCLUDED) && (BTHID_INCLUDED == TRUE)
                /* Send HID descriptor to the kernel (via bthid module) */
                hh_bthid_desc_write(bthid_fd, dev_dscp_data,(unsigned int) hid_desc_size);
#endif
                hh_dscpinfo_rcv = TRUE;
            }
        }
        else
        {
            printf("=> FAIL\n");
        }

        hh_open_status = p_data->open.status;
        hh_open_rcv = TRUE;
        hh_uipc_channel = p_data->open.uipc_channel;
        hh_handle = p_data->open.handle;

        printf("\tHandle:%d\n", p_data->open.handle);
        printf("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
                p_data->open.bd_addr[0],
                p_data->open.bd_addr[1],
                p_data->open.bd_addr[2],
                p_data->open.bd_addr[3],
                p_data->open.bd_addr[4],
                p_data->open.bd_addr[5]);
        break;

    case BSA_HH_CLOSE_EVT:              /* Connection Closed */
        printf("BSA_HH_CLOSE_EVT event received\n");
        printf("\tStatus:%d\n", p_data->close.status);
        printf("\tHandle:%d\n", p_data->close.handle);
        printf("Close UIPC HH channel\n");
#if defined(BTHID_INCLUDED) && (BTHID_INCLUDED == TRUE)
        hh_bthid_close(bthid_fd);
        bthid_fd = -1;
#endif
        hh_close_rcv = TRUE;
        break;

    case BSA_HH_GET_DSCPINFO_EVT:               /* DSCPINFO  */
        printf("BSA_HH_GET_DSCPINFO_EVT event received\n");
        printf("\tStatus:%d\n", p_data->dscpinfo.status);
        printf("\tHandle:%d\n", p_data->dscpinfo.handle);

        if (p_data->dscpinfo.status == BSA_SUCCESS)
        {
            printf("DscpInfo (len:%d):\n", p_data->dscpinfo.dscpdata.length);
            scru_dump_hex(p_data->dscpinfo.dscpdata.data, NULL,
                p_data->dscpinfo.dscpdata.length,
                TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);

            hh_write_desc_data(simple_app.device_bd_addr,p_data->dscpinfo.dscpdata.data,
                    p_data->dscpinfo.dscpdata.length);
#if defined(BTHID_INCLUDED) && (BTHID_INCLUDED == TRUE)
            /* Send HID descriptor to the kernel (via bthid module) */
            hh_bthid_desc_write(bthid_fd, p_data->dscpinfo.dscpdata.data,
                    p_data->dscpinfo.dscpdata.length);
#endif
        }
        hh_dscpinfo_rcv = TRUE;
        break;

    default:
        printf("hh_cback bad event:%d\n", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         hh_uipc_cback
 **
 ** Description      This function is hh uipc channel callback
 **
 ** Parameters       p_msg: pointer on UIPC report
 ** Returns          void
 **
 *******************************************************************************/
static void hh_uipc_cback(BT_HDR *p_msg)
{
    tBSA_HH_UIPC_REPORT  *p_uipc_report = (tBSA_HH_UIPC_REPORT *)p_msg;

#if defined(BTHID_INCLUDED) && (BTHID_INCLUDED == TRUE)
    if (bthid_fd != -1)
    {
        /* printf("index=%d/hh_device_cb[index].bthid_fd=%d\n",index,hh_device_cb[index].bthid_fd ); */
        hh_bthid_report_write(bthid_fd,
                p_uipc_report->report.report_data.data,
                p_uipc_report->report.report_data.length);
    }
    else
    {
        printf("hh_uipc_cback bad Handle:%d\n", p_uipc_report->report.handle);
        /*printf("hh_uipc_cback : index=%d / bthid_fd = %d",index,hh_device_cb[index].bthid_fd);*/
    }
#else
    printf("app_hh_uipc_cback called Handle:%d\n", p_uipc_report->report.handle);
    printf("\tMode:%d SubClass:0x%x CtryCode:%d\n", p_uipc_report->report.mode,
            p_uipc_report->report.sub_class, p_uipc_report->report.ctry_code);
    printf("\tReport:");
    scru_dump_hex(p_uipc_report->report.report_data.data, "\tReport:",
            p_uipc_report->report.report_data.length,
            TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
#endif
    GKI_freebuf(p_msg);
}


/*******************************************************************************
 **
 ** Function         hh_open
 **
 ** Description      This function request HH connection
 **
 ** Parameters       none
 ** Returns          int
 **
 *******************************************************************************/
int hh_open(void)
{
    int                 status;
    tBSA_HH_ENABLE      bsa_hh_enable_param;
    tBSA_HH_OPEN        bsa_hh_open_param;
    tBSA_HH_CLOSE       bsa_hh_close_param;

    printf("hh_open\n");

    /*
     * Enable HH
     */
    BSA_HhEnableInit (&bsa_hh_enable_param);
    bsa_hh_enable_param.sec_mask = BSA_SEC_NONE;
    bsa_hh_enable_param.p_cback = hh_cback;
    status = BSA_HhEnable(&bsa_hh_enable_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "hh_open: Unable to enable HID Host status:%d\n", status);
        return status;
    }

    /*
     * Request HH connection
     */
    BSA_HhOpenInit(&bsa_hh_open_param);
    bsa_hh_open_param.mode = HH_PROTO_MODE;
    bdcpy(bsa_hh_open_param.bd_addr, simple_app.device_bd_addr);
    bsa_hh_open_param.sec_mask = BSA_SEC_NONE;
    status = BSA_HhOpen(&bsa_hh_open_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "hh_open: Unable to connect HID device status:%d\n", status);
        return status;
    }

    while(hh_open_rcv == FALSE);

    if (hh_open_status != BSA_SUCCESS)
    {
        fprintf(stderr, "hh_open: Unable to connect HID device status:%d\n", hh_open_status);
        return status;
    }

    /*
     * Open UIPC Channel to receive reports
     */
    UIPC_Open(hh_uipc_channel, hh_uipc_cback);

    /*
     * The HID Device is now connected.
     * Loop until user press a key
     */
    app_get_choice("Press enter to close HH connection");

    /*
     * close UIPC Channel
     */
    UIPC_Close(hh_uipc_channel);

    /*
     * close HH connection
     */
    BSA_HhCloseInit(&bsa_hh_close_param);
    bsa_hh_close_param.handle = hh_handle;
    status = BSA_HhClose(&bsa_hh_close_param);

    return status;
}

/*******************************************************************************
 **
 ** Function         hh_wopen
 **
 ** Description      This function request HH to wait an incoming connection
 **
 ** Parameters       none
 ** Returns          int
 **
 *******************************************************************************/
int hh_wopen(void)
{
    int                 status;
    tBSA_HH_ADD_DEV     bsa_hh_add_dev_param;
    tBSA_HH_ENABLE      bsa_hh_enable_param;
    tBSA_HH_CLOSE       bsa_hh_close_param;

    printf("hh_wopen\n");

    /*
     * Enable HH
     */
    BSA_HhEnableInit (&bsa_hh_enable_param);
    bsa_hh_enable_param.sec_mask = BSA_SEC_NONE;
    bsa_hh_enable_param.p_cback = hh_cback;
    status = BSA_HhEnable(&bsa_hh_enable_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "hh_open: Unable to enable HID Host status:%d\n", status);
        return status;
    }

    BSA_HhAddDevInit(&bsa_hh_add_dev_param);
    bdcpy(bsa_hh_add_dev_param.bd_addr, simple_app.device_bd_addr);
    bsa_hh_add_dev_param.attr_mask = 0;
    bsa_hh_add_dev_param.sub_class = 0x80; /* sub-class mouse */
    bsa_hh_add_dev_param.dscp_data.length = 0;

    BSA_HhAddDev(&bsa_hh_add_dev_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "hh_wopen: Unable to add HID device:%d\n", status);
        return status;
    }

    printf("Waiting for HID device to initiate connection\n");
    while(hh_open_rcv == FALSE);

    if (hh_open_status != BSA_SUCCESS)
    {
        fprintf(stderr, "hh_open: Unable to connect HID device status:%d\n", hh_open_status);
        return status;
    }

    /*
     * Open UIPC Channel to receive reports
     */
    UIPC_Open(hh_uipc_channel, hh_uipc_cback);

    /*
     * In this example, we don't request HID Descriptor because we where
     * supposed to have it stored in NVRAM
     */

    /*
     * The HID Device is now connected.
     * Loop until user press a key
     */
    app_get_choice("Press enter to close HH connection");

    /*
     * close UIPC Channel
     */
    UIPC_Close(hh_uipc_channel);

    /*
     * close HH connection
     */
    BSA_HhCloseInit(&bsa_hh_close_param);
    bsa_hh_close_param.handle = hh_handle;
    status = BSA_HhClose(&bsa_hh_close_param);
    while(hh_close_rcv == FALSE);

    return status;
}

