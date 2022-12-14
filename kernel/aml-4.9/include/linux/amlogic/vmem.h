/*
 * include/linux/amlogic/vmem.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __LINUX_VMEM_H
#define __LINUX_VMEM_H

struct vmem_controller;
struct vmem_device;

struct vmem_controller {
	struct device dev;
	struct list_head list;
	struct list_head device_list;
	int bus_num;
	int (*setup)(struct vmem_device *vmemdev);
	int (*cleanup)(struct vmem_device *vmemdev);
};

struct vmem_device {
	struct device dev;
	struct vmem_controller *vmemctlr;
	struct list_head list;
	u8 dev_num;
	u8 *mem;
	int size;
	int (*cmd_notify)(u8 cmd_val, int offset, int size);
	int (*data_notify)(u8 cmd_val, int offset, int size);
};

extern struct vmem_controller *vmem_get_controller(int bus_num);
extern struct vmem_controller *
vmem_create_controller(struct device *dev, int size);
extern void vmem_destroy_controller(struct vmem_controller *vmemctlr);

extern struct vmem_device *
vmem_get_device(struct vmem_controller *vmemctlr, int dev_num);
extern struct vmem_device *
vmem_create_device(struct vmem_controller *vmemctlr, int dev_num, int mem_size);
extern void vmem_destroy_device(struct vmem_device *vmemdev);

extern int vmem_setup(struct vmem_device *vmemdev);
extern int vmem_cleanup(struct vmem_device *vmemdev);

#endif /* __LINUX_VMEM_H */
