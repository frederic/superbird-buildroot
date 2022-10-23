/*
 *
 * hci_drv.c
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
 * HCI Driver functions for Firmware Download tool
 *
 */

/* standard includes */
#include <stdio.h>

/* For string manipulation */
#include <string.h>

/* For file operations */
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>

#include "hci_drv.h"
#include "btdl_utils.h"

/* Definitions */
#ifndef HCI_DRV_OPEN_RETRY_MAX
#define HCI_DRV_OPEN_RETRY_MAX 3
#endif

#define HCI_DRV_FD_BAD (-1)

typedef struct
{
    int fd;
} tHCI_DRV_CB;

/* Global */
tHCI_DRV_CB hci_drv_cb;

/* Local functions */

/*******************************************************************************
 **
 ** Function        hci_drv_init
 **
 ** Description     Init the HCI driver
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_drv_init(void)
{
    memset(&hci_drv_cb, 0, sizeof(hci_drv_cb));
    hci_drv_cb.fd = HCI_DRV_FD_BAD;
    return 0;
}

/*******************************************************************************
 **
 ** Function        hci_drv_open
 **
 ** Description     Open the HCI driver
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_drv_open(const char *device)
{
    int status = 0;
    char procname[256];
    int fd;

    btdl_print(BTDL_DEBUG, "Opening HCI Port %s", device);

    if (strstr(device, "btusb"))
    {
        btdl_print(BTDL_DEBUG, "hci_drv_open BTUSB port");

        /* Create the /proc/driver/ entry name */
        strncpy(procname, "/proc/driver/btusb", sizeof(procname));
        procname[sizeof(procname) - 1] = '\0';
        strncat(procname, strrchr(device, '/'), sizeof(procname) - 1 - strlen(procname));

        /* Wait until the file in /proc/driver/ is created */
        status = 0;
        btdl_print(BTDL_DEBUG, "trying to access file %s", procname);
        while (access(procname, R_OK) < 0)
        {
            status++;
            if (status > HCI_DRV_OPEN_RETRY_MAX)
            {
                return -1;
            }
            btdl_sleep(1);
            btdl_print(BTDL_DEBUG, "retrying to access file %s", procname);
        }

        /* Wait until the device (/dev/btusbx) can be access in R/W mode */
        /* This may be needed if udev is used */
        status = 0;
        btdl_print(BTDL_DEBUG, "trying to access file %s", device);
        while (access(device, R_OK | W_OK) < 0)
        {
            status++;
            if (status > HCI_DRV_OPEN_RETRY_MAX)
            {
                return -1;
            }
            btdl_sleep(1);
            btdl_print(BTDL_DEBUG, "retrying to access file %s", device);
        }
    }

    /* Open the BT controller device */
    btdl_print(BTDL_DEBUG, "Opening %s", device);
    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        btdl_print(BTDL_ERROR, "open(%s) failed", device);
        perror("open");
        return -1;
    }

    hci_drv_cb.fd = fd;

    return 0;
}

/*******************************************************************************
 **
 ** Function        hci_drv_close
 **
 ** Description     Close the HCI (btusb driver)
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int hci_drv_close(void)
{
    btdl_print(BTDL_DEBUG, "Closing HCI");
    if (hci_drv_cb.fd != HCI_DRV_FD_BAD)
    {
        close(hci_drv_cb.fd);
        hci_drv_cb.fd = HCI_DRV_FD_BAD;
        return 0;
    }
    else
    {
        btdl_print(BTDL_ERROR, "hci_drv_close driver was not opened");
    }
    return -1;
}


/*******************************************************************************
 **
 ** Function        hci_drv_write
 **
 ** Description     Write an HCI packet
 **
 ** Parameters      buf: buffer to write the read event
 **                 size: size of the buffer
 **
 ** Returns         Number of bytes written. -1 in case of error
 **
 *******************************************************************************/
int hci_drv_write(const uint8_t *buf, int size)
{
    int ret, sent;

    if (hci_drv_cb.fd == HCI_DRV_FD_BAD)
    {
        btdl_print(BTDL_ERROR, "hci_drv_write HCI Driver not opened");
        return -1;
    }

    /* debug */
    btdl_print(BTDL_DEBUG, ">>> HCI Tx Buffer:");
    btdl_dump(BTDL_DEBUG, ">>> ", buf, size);

    /* loop on write until all the chars are sent out */
    sent = 0;
    while (sent != size)
    {
        ret = write(hci_drv_cb.fd, &buf[sent], size-sent);
        if (ret < 0)
        {
            btdl_print(BTDL_ERROR, "write failed");
            perror("write");
            return -1;
        }
        else
        {
            sent += ret;
        }
    }
    return sent;
}

/*******************************************************************************
 **
 ** Function        hci_drv_read
 **
 ** Description     Read an HCI packet
 **
 ** Parameters      buf: buffer to store the read event
 **                 maxsize: size of the buffer
 **
 ** Returns         number of byte read
 **
 *******************************************************************************/
int hci_drv_read(uint8_t *buf, int maxsize)
{
    fd_set input;
    struct timeval timeout;
    int n, ret;

    if (hci_drv_cb.fd == HCI_DRV_FD_BAD)
    {
        btdl_print(BTDL_ERROR, "hci_drv_read HCI Driver not opened");
        return -1;
    }

    /* input set (only fd) */
    FD_ZERO(&input);
    FD_SET(hci_drv_cb.fd, &input);

    /* timeout = 5 seconds */
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    /* wait for an event (first parameter is highest id) */
    n = select(hci_drv_cb.fd + 1, &input, NULL, NULL, &timeout);

    /* by default, there are no chars returned */
    ret = 0;

    /* check if there was an error */
    if (n < 0)
    {
        btdl_print(BTDL_ERROR, "select failed");
        perror("select");
    }
    /* check if there was a timeout */
    else if (n == 0)
    {
        btdl_print(BTDL_ERROR, "select timeout");
    }
    else
    {
        /* check if input is from fid */
        if (FD_ISSET(hci_drv_cb.fd, &input))
        {
            ret = read(hci_drv_cb.fd, buf, maxsize);

            btdl_print(BTDL_DEBUG, "<<< HCI Rx buffer:");
            if (ret > 0)
            {
                btdl_dump(BTDL_DEBUG, "<<< ", buf, ret);
            }
        }
        else
        {
            btdl_print(BTDL_ERROR, "select not raised for correct file descriptor");
        }
    }
    return ret;
}

