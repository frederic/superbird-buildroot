// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
//#define DEBUG
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include <linux/amlogic/power_domain.h>
#include <dt-bindings/power/amlogic,pd.h>
#include "hifi4dsp_priv.h"
#include "hifi4dsp_firmware.h"
#include "hifi4dsp_dsp.h"
#include <linux/amlogic/scpi_protocol.h>

/*DSP TOP*/
#define REG_DSP_CFG0			(0x0)
#define REG_DSP_CFG1			(0x4)
#define REG_DSP_CFG2			(0x8)
#define REG_DSP_RESET_VEC		(0x004 << 2)

static inline void soc_dsp_top_reg_dump(char *name,
					void __iomem *reg_base,
					u32 reg_offset)
{
	pr_info("%s (%lx) = 0x%x\n", name,
		(unsigned long)(reg_base + reg_offset),
		readl(reg_base + reg_offset));
}

void soc_dsp_regs_iounmem(void)
{
	iounmap(g_regbases.dspa_addr);
	iounmap(g_regbases.dspb_addr);

	pr_debug("%s done\n", __func__);
}

static inline void __iomem *get_dsp_addr(int dsp_id)
{
	if (dsp_id == 1)
		return g_regbases.dspb_addr;
	else
		return g_regbases.dspa_addr;
}

static void start_dsp(u32 dsp_id, u32 reset_addr)
{
	u32 StatVectorSel;
	u32 strobe = 1;
	u32 tmp;
	u32 read;
	void __iomem *reg;

	reg = get_dsp_addr(dsp_id);
	StatVectorSel = (reset_addr != 0xfffa0000);

	tmp = 0x1 |  StatVectorSel << 1 | strobe << 2;
	scpi_init_dsp_cfg0(dsp_id, reset_addr, tmp);
	read = readl(reg + REG_DSP_CFG0);
	pr_debug("REG_DSP_CFG0 read=0x%x\n", read);

	if (dsp_id == 0) {
		read  = (read & ~(0xffff << 0)) | (0x2018 << 0) | (1 << 29);
		read = read & ~(1 << 31);         //dreset assert
		read = read & ~(1 << 30);     //Breset
	} else {
		read  = (read & ~(0xffff << 0)) | (0x2019 << 0) | (1 << 29);
		read = read & ~(1 << 31);         //dreset assert
		read = read & ~(1 << 30);     //Breset
	}
	pr_info("REG_DSP_CFG0 read=0x%x\n", readl(reg + REG_DSP_CFG0));
	writel(read, reg + REG_DSP_CFG0);
	soc_dsp_top_reg_dump("REG_DSP_CFG0", reg, REG_DSP_CFG0);

	pr_debug("%s\n", __func__);
}

static void soc_dsp_power_switch(int dsp_id, int pwr_cntl)
{
	if (!dsp_id)
		power_domain_switch(PM_DSPA, pwr_cntl);
	else if (dsp_id == 1)
		power_domain_switch(PM_DSPB, pwr_cntl);
}

void soc_dsp_top_regs_dump(int dsp_id)
{
	void __iomem *reg;

	pr_debug("%s\n", __func__);

	reg = get_dsp_addr(dsp_id);
	pr_debug("%s base=%lx\n", __func__, (unsigned long)reg);

	soc_dsp_top_reg_dump("REG_DSP_CFG0", reg, REG_DSP_CFG0);
	soc_dsp_top_reg_dump("REG_DSP_CFG1", reg, REG_DSP_CFG1);
	soc_dsp_top_reg_dump("REG_DSP_CFG2", reg, REG_DSP_CFG2);
	soc_dsp_top_reg_dump("REG_DSP_RESET_VEC", reg, REG_DSP_RESET_VEC);
}

static void soc_dsp_set_clk(int dsp_id, int freq_sel)
{
	//clk_util_set_dsp_clk(dsp_id, freq_sel);
}

void soc_dsp_hw_init(int dsp_id, int freq_sel)
{
	soc_dsp_set_clk(dsp_id, freq_sel);
	soc_dsp_power_switch(dsp_id, PWR_ON);

	pr_debug("%s done\n", __func__);
}

void soc_dsp_start(int dsp_id, int reset_addr)
{
	start_dsp(dsp_id, reset_addr);
}

void soc_dsp_bootup(int dsp_id, u32 reset_addr, int freq_sel)
{
	pr_debug("%s dsp_id=%d, address=0x%x\n",
		 __func__, dsp_id, reset_addr);

	soc_dsp_power_switch(dsp_id, PWR_ON);
	soc_dsp_start(dsp_id, reset_addr);

	pr_debug("\n after boot: reg state:\n");
	soc_dsp_top_regs_dump(dsp_id);
}

void soc_dsp_poweroff(int dsp_id)
{
	soc_dsp_power_switch(dsp_id,  PWR_OFF);
}

