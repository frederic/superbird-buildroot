/*
 * drivers/amlogic/media/vout/cvbs/cvbs_out.h
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

#ifndef _CVBS_OUT_H_
#define _CVBS_OUT_H_

/* Standard Linux Headers */
#include <linux/workqueue.h>

/* Amlogic Headers */
#include <linux/amlogic/media/vout/vout_notify.h>
#include "cvbs_mode.h"

#define CVBSOUT_VER "Ref.2018/11/07"

#define CVBS_CLASS_NAME	"cvbs"
#define CVBS_NAME	"cvbs"
#define	MAX_NUMBER_PARA  10

#define _TM_V 'V'

#define VOUT_IOC_CC_OPEN           _IO(_TM_V, 0x01)
#define VOUT_IOC_CC_CLOSE          _IO(_TM_V, 0x02)
#define VOUT_IOC_CC_DATA           _IOW(_TM_V, 0x03, struct vout_CCparm_s)

struct reg_s {
	unsigned int reg;
	unsigned int val;
};

enum cvbs_cpu_type {
	CVBS_CPU_TYPE_GXL    = 0,
	CVBS_CPU_TYPE_GXM    = 1,
	CVBS_CPU_TYPE_TXLX   = 2,
	CVBS_CPU_TYPE_G12A   = 3,
	CVBS_CPU_TYPE_G12B   = 4,
	CVBS_CPU_TYPE_TL1    = 5,
	CVBS_CPU_TYPE_SM1    = 6,
	CVBS_CPU_TYPE_TM2    = 7,
};

struct meson_cvbsout_data {
	unsigned int vdac_vref_adj;
	unsigned int vdac_gsw;
	enum cvbs_cpu_type cpu_id;
	const char *name;
};

#define CVBS_PERFORMANCE_CNT_MAX    20
struct performance_config_s {
	unsigned int reg_cnt;
	struct reg_s *reg_table;
};

/* cvbs driver flag */
#define CVBS_FLAG_EN_ENCI   BIT(0)
#define CVBS_FLAG_EN_VDAC   BIT(1)

struct cvbs_drv_s {
	struct vinfo_s *vinfo;
	struct cdev   *cdev;
	dev_t         devno;
	struct class  *base_class;
	struct device *dev;
	struct meson_cvbsout_data *cvbs_data;
	struct performance_config_s perf_conf_pal;
	struct performance_config_s perf_conf_ntsc;
	struct delayed_work vdac_dwork;
	unsigned int flag;

	/* clktree */
	unsigned int clk_gate_state;
	struct clk *venci_top_gate;
	struct clk *venci_0_gate;
	struct clk *venci_1_gate;
	struct clk *vdac_clk_gate;
};

static  DEFINE_MUTEX(cvbs_mutex);

struct vout_CCparm_s {
	unsigned int type;
	unsigned char data1;
	unsigned char data2;
};

struct cvbsregs_set_t {
	enum cvbs_mode_e cvbsmode;
	const struct reg_s *enc_reg_setting;
};

extern void amvecm_clip_range_limit(bool limit_en);

#endif
