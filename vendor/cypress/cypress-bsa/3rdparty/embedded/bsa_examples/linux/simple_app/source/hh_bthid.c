/*****************************************************************************
**
**  Name:           hh_bthid.c
**
**  Description:    This file contains the functions implementation
**                  to pass HID reports/descriptor received from peer HID
**                  devices to the Linux kernel (via bthid module)
**
**  Copyright (c) 2010, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "bt_types.h"
#include "hh_bthid.h"

#include "bthid.h"

#ifndef HH_BTHID_PATH
#define HH_BTHID_PATH   "/dev/bthid"
#endif

/*******************************************************************************
**
** Function         hh_bthid_open
**
** Description      Open BTHID driver.
** Parameters       none
**
** Returns          BTHID File descriptor
**
*******************************************************************************/
int hh_bthid_open(void)
{
    int    bthid_fd = 0;

    printf("hh_bthid_open\n");

    /****************************************************
     * Before opening bthid device you need to make
     * sure the device entry is created under /dev/input
     * and that the bthid device driver is loaded.
     * To do so, you can run an external script to do this for you.
     * The contents of this script could be:
     * >mknod /dev/input/bthid c 10 224
     * >chmod 666 /dev/input/bthid
     * >insmod bthid.ko
     ****************************************************/

    /* Open bthid device.  Return if failed */
    if((bthid_fd = open(HH_BTHID_PATH, O_RDWR)) < 0)
    {
        printf("Failed to open %s : %d\n", HH_BTHID_PATH, errno);
        return -1;
    }

    return bthid_fd;
}

/*******************************************************************************
**
** Function         hh_bthid_close
**
** Description      This function is executed by app HH when connection is closed
**
** Parameters       BTHID file descriptor
**
** Returns          int
**
*******************************************************************************/
int hh_bthid_close(int bthid_fd)
{
    int status;

    printf("bthid_close\n");

    status = close(bthid_fd);

    /*******************************************
     * After closing bthid, normally it would
     * get unregistered from system.
     * If not you can run an external script to do this.
     * The contents of this script could be:
     * rmmod bthid
     * rm -Rf /dev/input/bthid
     *********************************************/

    return status;
}



/******************************************************************************
**
** Function:    hh_bthid_report_write
**
** Description: Send Bluetooth HID report to bthid driver for processing
**              bthid expects that application has sent HID DSCP to driver
**              this tells Linux system how to parse the reports
**              The bthid.ko driver will send the report to the Linux kernel
** Parameters   BTHID file descriptor
**              pointer on report to write
**              size of report to write
**
** Returns:     int
**
*******************************************************************************/
int hh_bthid_report_write(int fd, unsigned char *p_rpt, int size)
{
    ssize_t size_wrote;

    size_wrote = write(fd, p_rpt, (size_t)size);

    if (size_wrote < 0)
    {
        printf("unable to write HID report:%d\n", errno);
    }
    return size_wrote;
}

/******************************************************************************
**
** Function:    hh_bthid_desc_write
**
** Description: Send Bluetooth HID Descriptor to bthid driver for processing
** Parameters   BTHID file descriptor
**              pointer on descriptor to write
**              size of descriptor to write
** Returns:     int
**
*******************************************************************************/
int hh_bthid_desc_write(int fd, unsigned char *p_dscp, unsigned int size)
{
     BTHID_CONTROL ctrl;
     unsigned long arg;
     int status;

     if ((p_dscp == NULL) ||
         (size > sizeof(ctrl.data)))
     {
         printf("hh_bthid_desc_write: bad parameter\n");
     }
     /* Prepare ioctl structure */
     ctrl.size =  size;
     memcpy(&ctrl.data, p_dscp, ctrl.size);

     arg = (unsigned long )&ctrl;
     status = ioctl(fd, 1, arg);
     if (status < 0)
     {
         printf("hh_bthid_desc_write: fail to write HID descriptor fd=[%d] errno=[%d]\n",fd, errno);
         return status;
     }
     return status;
}

