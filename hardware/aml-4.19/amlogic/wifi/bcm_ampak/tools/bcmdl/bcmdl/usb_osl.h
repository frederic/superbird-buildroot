/*
 * Broadcom USB remote download OSL interface routines
 *
 * Copyright (C) 2014, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: usb_osl.h 484281 2014-06-12 22:42:26Z $
 */
#ifndef __usb_osl_h_
#define __usb_osl_h_
#if defined(__FreeBSD__)
#include <stdbool.h>
#endif

typedef struct usbinfo usbinfo_t;

/* OSL common routines.
 * Implement these routines when porting to another OS
 */
extern usbinfo_t* usbdev_init(struct bcm_device_id *devtable, struct bcm_device_id **bcmdev);
extern int usbdev_deinit(usbinfo_t *info);
extern int usbdev_bulk_write(usbinfo_t *info, void *data, int len, int timeout);
extern int usbdev_control_read(usbinfo_t *info, int request, int value, int index,
                               void *data, int size, bool interface, int timeout);
extern int usbdev_control_write(usbinfo_t *info, int request, int value, int index,
                                void *data, int size, bool interface, int timeout);
extern int usbdev_reset(usbinfo_t *info);
#endif  /* __usb_osl_h_ */
