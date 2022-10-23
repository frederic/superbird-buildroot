/*
 *
 * hci_drv.h
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

/* idempotency */
#ifndef HCI_DRV_H
#define HCI_DRV_H

/* self-sufficiency */
#include <stdint.h>

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
int hci_drv_init(void);

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
int hci_drv_open(const char *device);

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
int hci_drv_close(void);

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
int hci_drv_write(const uint8_t *buf, int size);

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
int hci_drv_read(uint8_t *buf, int maxsize);


#endif /* HCI_DRV */
