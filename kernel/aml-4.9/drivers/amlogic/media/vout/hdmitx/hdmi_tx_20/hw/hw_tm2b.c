/*
 * drivers/amlogic/media/vout/hdmitx/hdmi_tx_20/hw/hw_tm2b.c
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

#include <linux/printk.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#include "common.h"
#include "mach_reg.h"

void set_tm2b_phy_para(unsigned int mode)
{
	switch (mode) {
	case HDMI_PHYPARA_6G: /* 5.94/4.5/3.7Gbps */
	case HDMI_PHYPARA_4p5G:
	case HDMI_PHYPARA_3p7G:
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL0, 0x37EB65c4);
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL5, 0x0000080b);
		break;
	case HDMI_PHYPARA_3G: /* 2.97Gbps */
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL0, 0x33eb62a3);
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL5, 0x00000003);
		break;
	case HDMI_PHYPARA_270M: /* 270Mbps */
	case HDMI_PHYPARA_DEF: /* 1.485Gbps, and others */
	default:
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL0, 0x33eb5252);
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		hd_write_reg(P_TM2_HHI_HDMI_PHY_CNTL5, 0x00000003);
		break;
	}
}

void set_tm2b_phy_reset(int chip_type)
{
/* P_HHI_HDMI_PHY_CNTL1 bit[1]: enable clock	bit[0]: soft reset */
#define RESET_HDMI_PHY() \
do { \
	hd_set_reg_bits(P_TM2_HHI_HDMI_PHY_CNTL1, 0xf, 0, 4); \
	mdelay(2); \
	hd_set_reg_bits(P_TM2_HHI_HDMI_PHY_CNTL1, 0xe, 0, 4); \
	mdelay(2); \
} while (0)
	hd_set_reg_bits(P_TM2_HHI_HDMI_PHY_CNTL1, 0x0390, 16, 16);
	hd_set_reg_bits(P_TM2_HHI_HDMI_PHY_CNTL1, 0x1, 17, 1);
	hd_set_reg_bits(P_TM2_HHI_HDMI_PHY_CNTL1, 0x0, 17, 1);
	hd_set_reg_bits(P_TM2_HHI_HDMI_PHY_CNTL1, 0x0, 0, 4);
	msleep(20);
	RESET_HDMI_PHY();
	RESET_HDMI_PHY();
	RESET_HDMI_PHY();
#undef RESET_HDMI_PHY
}
