/*
 * Broadcom Host Remote Download Utility
 *
 * Linux USB OSL routines.
 *
 * Copyright (C) 2014, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: usb_linux.c 404499 2013-05-28 01:06:37Z $
 */

#include <stdio.h>
#include <string.h>
#include <usb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <typedefs.h>
#include <usbrdl.h>
#include <usbstd.h>
#include "usb_osl.h"
#include <bcmutils.h>

struct usbinfo {
	struct bcm_device_id *devtable;
	int bulkoutpipe;
	int ctrlinpipe;
	usb_dev_handle *dev;
	int int_wlan;
};

#define GET_UPPER_16(u32val) ((uint16)((0xFFFF0000 & u32val) >> 16))
#define GET_LOWER_16(u32val) ((uint16)(0x0000FFFF & u32val))

struct usb_device *
find_my_device(usbinfo_t *info, struct bcm_device_id **bd)
{
	struct usb_bus *bus;
	struct usb_device *device;
	struct bcm_device_id *bcmdev;

	usb_init();
	/* Enumerate all busses */
	usb_find_busses();

	/* Enumerate all devices */
	usb_find_devices();

	/* Find the right Vendor/Product pair */
	for (bus = usb_busses; bus; bus = bus->next)
		for (device = bus->devices; device; device = device->next) {
			fprintf(stderr, "Vendor 0x%x ID 0x%x\n", device->descriptor.idVendor,
			       device->descriptor.idProduct);
			for (bcmdev = info->devtable; bcmdev->name; bcmdev++)
				if ((device->descriptor.idVendor == bcmdev->vend) &&
				    (device->descriptor.idProduct == bcmdev->prod)) {
					*bd = bcmdev;
					return device;
				}
		}
	return NULL;
}

usbinfo_t *
usbdev_find(struct bcm_device_id *devtable, struct bcm_device_id **bcmdev)
{
	struct usb_device *device;
	usbinfo_t *info;

	if ((info = malloc(sizeof(usbinfo_t))) == NULL)
		return NULL;

	info->devtable = devtable;

	device = find_my_device(info, bcmdev);
	if (device == NULL) {
		*bcmdev = NULL;
		fprintf(stderr, "No devices found\n");
		goto err;
	}

	return info;
err:
	free(info);
	return NULL;
}

usbinfo_t *
usbdev_init(struct bcm_device_id *devtable, struct bcm_device_id **bcmdev)
{
	struct usb_device *device;
	int status;
	usb_dev_handle *dev;
	struct usb_config_descriptor *config;
	struct usb_interface_descriptor *altsetting;
	struct usb_endpoint_descriptor *endpoint = NULL;
	int i, j, num_eps;
	usbinfo_t *info;

	if ((info = malloc(sizeof(usbinfo_t))) == NULL)
		return NULL;

	info->devtable = devtable;

	/* Initialize USB subsystem */
	//usb_init();

	device = find_my_device(info, bcmdev);
	if (device == NULL) {
		*bcmdev = NULL;
		fprintf(stderr, "No devices found\n");
		goto err;
	}

	usb_set_debug(0xff);

	/* Open the USB device */
	if (!(dev = usb_open(device))) {
		fprintf(stderr, "Failed to open device\n");
		goto err;
	}

	info->dev = dev;

	/* Set default configuration:
	 * for a multi-interface device the default cofiguration
	 * may already be set by another driver (e.g. BT)
	 */
	if (!device->config->bConfigurationValue) {
		if ((status = usb_set_configuration(dev, 1))) {
			fprintf(stderr, "Failed to set configuartion\n");
			goto err;
		}
	}

	config = device->config;
	if (!config)
		goto err;

	/* find WLAN interface */
	for (i = 0; i < config->bNumInterfaces; i++) {
		if (!(altsetting = config->interface[i].altsetting))
			continue;
		if (!(endpoint = altsetting->endpoint))
			continue;
		if (!(num_eps = altsetting->bNumEndpoints))
			continue;

		if (altsetting->bInterfaceClass != UICLASS_VENDOR)
			continue;

		/* Check data endpoints */
		for (j = 0; j < num_eps; j++) {
			if (UE_GET_XFERTYPE(endpoint[j].bmAttributes) != UE_BULK)
				continue;
			if (!((UE_GET_DIR(endpoint[j].bEndpointAddress) == UE_DIR_IN))) {
				info->bulkoutpipe = UE_GET_ADDR(endpoint[j].bEndpointAddress);
				break;
			}
		}
		if (j < num_eps)
			break;
	}

	if (i == config->bNumInterfaces)
		goto err;

	info->int_wlan = i;
	info->ctrlinpipe = USB_CTRL_IN;

	if (info->bulkoutpipe == 0) {
		fprintf(stderr, "bcmdl: could not find bulk out ep on interface %d\n", info->int_wlan);
		goto err;
	}

	/*
	 * Try to claim the WLAN interface; disregard status since this
	 * call will fail on some versions of Linux
	 */
	fprintf(stderr, "claiming interface %d\n", info->int_wlan);
	usb_claim_interface(dev, info->int_wlan);

	return info;
err:
	free(info);
	return NULL;
}

int
usbdev_deinit(usbinfo_t *info)
{
	int status;

	/* Release the interface */
	if ((status = usb_release_interface(info->dev, info->int_wlan)))
		fprintf(stderr, "Failed to release device %d\n", status);

	/* Close the USB device */
	if (usb_close(info->dev))
		fprintf(stderr, "failed to close device\n");

	free(info);
	return 0;
}

int
usbdev_bulk_write(usbinfo_t *info, void *data, int len, int timeout)
{
	return usb_bulk_write(info->dev, info->bulkoutpipe, (void *)data, len, timeout);

}

int
usbdev_control_read(usbinfo_t *info, int request, int value, int index,
                    void *data, int size, bool interface, int timeout)

{
	if (interface)
		return usb_control_msg(info->dev, info->ctrlinpipe, request, value, index,
		                       (char*)data, size, timeout);
	else
		return usb_control_msg(info->dev, 0xc0, request, value, index,
		                       (char*)data, size, timeout);
}

/* special fun to talk to 4319 cpuless usb device */
int
usbdev_control_write(usbinfo_t *info, int request, int value, int index,
                     void *data, int size, bool interface, int timeout)

{
	if (interface)
		return usb_control_msg(info->dev, 0x41, request, value, index,
		                       (char*)data, size, timeout);
	else
		return usb_control_msg(info->dev, 0x40, request, value, index,
		                       (char*)data, size, timeout);
}

/* To reset the USB device. Used for HSIC hot plugging */
int usbdev_reset(usbinfo_t *info)
{
	return usb_reset(info->dev);
}
