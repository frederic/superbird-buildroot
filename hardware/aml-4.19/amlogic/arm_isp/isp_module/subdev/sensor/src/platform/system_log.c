/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2011-2018 ARM or its affiliates
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2.
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "acamera_firmware_config.h"
#include "system_log.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"

//debug log names for level
const char *const log_level_name[SYSTEM_LOG_LEVEL_MAX] = {"DEBUG", "INFO", "NOTICE", "WARNING", "ERR", "CRIT"};
//debug log names for modules
const char *const log_module_name[SYSTEM_LOG_MODULE_MAX] = FSM_NAMES;

static ssize_t adap_dbg_read(
    struct device *dev,
    struct device_attribute *attr,
    char *buf)
{
	ssize_t ret = 0;

	uint32_t csi_err1 = mipi_csi_reg_rd_ext(MIPI_CSI_DATA_ERR1);
	uint32_t csi_err2 = mipi_csi_reg_rd_ext(MIPI_CSI_DATA_ERR2);
	uint32_t phy_lan0 = mipi_phy_reg_rd_ext(MIPI_PHY_DATA_LANE0_STS);
	uint32_t phy_lan1 = mipi_phy_reg_rd_ext(MIPI_PHY_DATA_LANE1_STS);
	uint32_t phy_lan2 = mipi_phy_reg_rd_ext(MIPI_PHY_DATA_LANE2_STS);
	uint32_t phy_lan3 = mipi_phy_reg_rd_ext(MIPI_PHY_DATA_LANE3_STS);
	uint32_t adapter_err = 0;
	uint32_t x_start_isp = 0;
	uint32_t x_start_mem = 0;
	uint32_t y_start_isp = 0;
	uint32_t y_start_mem = 0;

	mipi_adap_reg_rd_ext(MIPI_ADAPT_IRQ_PENDING0, ALIGN_IO, &adapter_err);
	mipi_adap_reg_rd_ext(CSI2_X_START_END_ISP, FRONTEND_IO, &x_start_isp);
	mipi_adap_reg_rd_ext(CSI2_X_START_END_MEM, FRONTEND_IO, &x_start_mem);
	mipi_adap_reg_rd_ext(CSI2_Y_START_END_ISP, FRONTEND_IO, &y_start_isp);
	mipi_adap_reg_rd_ext(CSI2_Y_START_END_MEM, FRONTEND_IO, &y_start_mem);

	ret = sprintf(buf, "\t\tcsi err1: %x\t\tcsi err2: %x\t\tadapter overflow:%x\n \
		x_start_isp:%x\t\t\t\tx_start_mem:%x\n  \
		y_start_isp:%x\t\t\t\ty_start_mem:%x\n  \
		lane0:%x\t\tlane1:%x\t\tlane2:%x\t\tlane3:%x\n",\
		csi_err1, csi_err2, adapter_err & (3 << 27), \
		x_start_isp, x_start_mem, \
		y_start_isp, y_start_mem, \
		phy_lan0, phy_lan1, phy_lan2, phy_lan3);

    return ret;
}

static ssize_t adap_dbg_write(
    struct device *dev, struct device_attribute *attr,
    char const *buf, size_t size)
{
    ssize_t ret = size;

    return ret;
}

static DEVICE_ATTR(adap_dbg, S_IRUGO | S_IWUSR, adap_dbg_read, adap_dbg_write);

void system_dbg_create( struct device *dev )
{
	device_create_file(dev, &dev_attr_adap_dbg);
}

void system_dbg_remove( struct device *dev )
{
	device_remove_file(dev, &dev_attr_adap_dbg);
}

