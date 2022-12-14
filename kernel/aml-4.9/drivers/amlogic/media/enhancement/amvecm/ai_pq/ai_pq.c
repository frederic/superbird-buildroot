/*
 * drivers/amlogic/media/enhancement/amvecm/ai_pq/ai_pq.c
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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/amlogic/media/amvecm/amvecm.h>
#include "ai_pq.h"
#include "../arch/vpp_regs.h"
#include "../dnlp_cal.h"
#include "../cm2_adj.h"

unsigned int aipq_debug;
module_param(aipq_debug, uint, 0664);
MODULE_PARM_DESC(aipq_debug, "\n aipq_debug\n");

#define pr_aipq_dbg(fmt, args...)\
	do {\
		if (aipq_debug)\
			pr_info("AIPQ: " fmt, ## args);\
	} while (0)\

#define DISABLE 0
#define ENABLE 1
struct single_scene_s detected_scenes[SCENE_MAX] = {
	{DISABLE, NULL},
	{DISABLE, NULL},
	{DISABLE, NULL},
	{DISABLE, NULL},
	{DISABLE, NULL},
	{DISABLE, NULL},
	{DISABLE, NULL},
};
EXPORT_SYMBOL(detected_scenes);

struct adap_param_setting_s adaptive_param;
struct adap_param_setting_s *adap_param;

/*base value : from db
 *reg value : last frame setting
 *offset : target offset for the scene
 *bld_speed used for value smooth change
 *bld_offset: change value frame base
 */
int smooth_process(int base_val, int reg_val, int offset, int bld_rs)
{
	int bld_offset = 0;
	int temp_offset = 0;

		/*base = current reg setting*/
	if (reg_val == (base_val + offset)) {
		bld_offset = 0;
		return bld_offset;
	}

	if (!offset) {
		/*offset = 0, reset to base value*/
		temp_offset = (base_val > reg_val) ?
			 (base_val - reg_val) :
			 (reg_val - base_val);
		if (bld_rs == 0)
			bld_offset = 1;
		else
			bld_offset =
			(temp_offset + ((1 << bld_rs) - 1)) >> bld_rs;
	} else {
		if (bld_rs == 0)
			bld_offset = 1;
		else
			bld_offset = (offset > 0)  ?
				((offset + ((1 << bld_rs) - 1)) >> bld_rs) :
				(((-offset) + ((1 << bld_rs) - 1)) >> bld_rs);
	}

	return bld_offset;
}

int blue_scene_process(int offset, int enable)
{
	int bld_rs = 4;
	static int reg_val;
	int base_val = 0;
	int bld_offset;
	static int first_frame = 1;

	if (!enable) {
		first_frame = 1;
		return -1;
	}

	if (first_frame == 1) {
		reg_val = base_val;
		first_frame = 0;
	}

	bld_offset = smooth_process(base_val, reg_val, offset, bld_rs);

	if (offset == 0)
		reg_val = (reg_val < base_val) ?
			((reg_val + bld_offset) >= base_val ?
			base_val : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < base_val ?
			base_val : (reg_val - bld_offset));
	else
		reg_val = (offset > 0) ?
			((reg_val + bld_offset) >= (base_val + offset) ?
			(base_val + offset) : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < (base_val + offset) ?
			(base_val + offset) : (reg_val - bld_offset));

	cm2_sat(ecm2colormd_cyan, reg_val, 0);
	cm2_curve_update_sat(ecm2colormd_cyan);
	cm2_sat(ecm2colormd_blue, reg_val, 0);
	cm2_curve_update_sat(ecm2colormd_blue);

	if (aipq_debug & (1 << BLUE_SCENE))
		pr_aipq_dbg(
			"%s, baseval: %d, regval: %d, offset: %d,bld_ofst: %d, bld_spd: %d\n",
			__func__, base_val, reg_val,
			offset, bld_offset, bld_rs);

	return 0;
}

int green_scene_process(int offset, int enable)
{
	int bld_rs = 4;
	static int reg_val;
	int base_val = 0;
	int bld_offset;
	static int first_frame = 1;

	if (!enable) {
		first_frame = 1;
		return -1;
	}

	if (first_frame == 1) {
		reg_val = base_val;
		first_frame = 0;
	}

	bld_offset = smooth_process(base_val, reg_val, offset, bld_rs);

	if (offset == 0)
		reg_val = (reg_val < base_val) ?
			((reg_val + bld_offset) >= base_val ?
			base_val : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < base_val ?
			base_val : (reg_val - bld_offset));
	else
		reg_val = (offset > 0) ?
			((reg_val + bld_offset) >= (base_val + offset) ?
			(base_val + offset) : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < (base_val + offset) ?
			(base_val + offset) : (reg_val - bld_offset));

	cm2_sat(ecm2colormd_green, reg_val, 0);
	cm2_curve_update_sat(ecm2colormd_green);

	if (aipq_debug & (1 << GREEN_SCENE))
		pr_aipq_dbg(
			"%s, baseval: %d, regval: %d, offset: %d,bld_ofst: %d, bld_spd: %d\n",
			__func__, base_val, reg_val,
			offset, bld_offset, bld_rs);

	return 0;
}

int peaking_scene_process(int offset, int enable)
{
	int bld_rs = 4;
	static int reg_val[4];
	int base_val[4];
	int bld_offset[4];
	static int first_frame = 1;
	int i;
	/*sr0 hp,bp gain*/
	base_val[0] = adap_param->peaking_param.sr0_hp_final_gain;
	base_val[1] = adap_param->peaking_param.sr0_bp_final_gain;
	/*sr1 hp,bp gain*/
	base_val[2] = adap_param->peaking_param.sr1_hp_final_gain;
	base_val[3] = adap_param->peaking_param.sr1_bp_final_gain;
	adap_param->satur_param.offset = offset;

	if (!enable) {
		first_frame = 1;
		return -1;
	}

	if (first_frame == 1) {
		for (i = 0; i < 4; i++)
			reg_val[i] = base_val[i];
		first_frame = 0;
	}

	for (i = 0; i < 4; i++) {
		bld_offset[i] =
			smooth_process(base_val[i], reg_val[i], offset, bld_rs);
		if (offset == 0)
			reg_val[i] = (reg_val[i] < base_val[i]) ?
			((reg_val[i] + bld_offset[i]) >= base_val[i] ?
			(base_val[i] + offset) : (reg_val[i] + bld_offset[i])) :
			((reg_val[i] - bld_offset[i]) < base_val[i] ?
			base_val[i] : (reg_val[i] - bld_offset[i]));
		else
			reg_val[i] = (offset > 0) ?
			((reg_val[i] + bld_offset[i]) >=
			(base_val[i] + offset) ?
			(base_val[i] + offset) :
			(reg_val[i] + bld_offset[i])) :
			((reg_val[i] - bld_offset[i]) < (base_val[i] + offset) ?
			(base_val[i] + offset) : (reg_val[i] - bld_offset[i]));

		if (aipq_debug & (1 << PEAKING_SCENE))
			pr_aipq_dbg(
				"%s, i: %d, baseval: %d, regval: %d, offset: %d,bld_ofst: %d, bld_spd: %d\n",
				__func__, i, base_val[i], reg_val[i],
				offset, bld_offset[i], bld_rs);
	}

	VSYNC_WR_MPEG_REG_BITS(
		SRSHARP0_PK_FINALGAIN_HP_BP + sr_offset[0],
		reg_val[0] << 8 | reg_val[1],
		0, 16);
	VSYNC_WR_MPEG_REG_BITS(
		SRSHARP1_PK_FINALGAIN_HP_BP + sr_offset[1],
		reg_val[2] << 8 | reg_val[3],
		0, 16);

	return 0;
}

int contrast_scene_process(int offset, int enable)
{
	int bld_rs = 0;
	static int reg_val;
	int base_val;
	int bld_offset;
	static int first_frame = 1;

	base_val = adap_param->dnlp_param.dnlp_final_gain;
	adap_param->dnlp_param.offset = offset;

	if (!enable) {
		first_frame = 1;
		return -1;
	}

	if (first_frame == 1) {
		reg_val = base_val;
		first_frame = 0;
	}

	bld_offset = smooth_process(base_val, reg_val, offset, bld_rs);

	if (bld_offset == 0)
		return 0;

	if (offset == 0)
		reg_val = (reg_val < base_val) ?
			((reg_val + bld_offset) >= base_val ?
			base_val : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < base_val ?
			base_val : (reg_val - bld_offset));
	else
		reg_val = (offset > 0) ?
			((reg_val + bld_offset) >= (base_val + offset) ?
			(base_val + offset) : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < (base_val + offset) ?
			(base_val + offset) : (reg_val - bld_offset));

	if (aipq_debug & (1 << DYNAMIC_CONTRAST_SCENE))
		pr_aipq_dbg(
			"%s, baseval: %d, regval: %d, offset: %d,bld_ofst: %d, bld_spd: %d\n",
			__func__, base_val, reg_val,
			offset, bld_offset, bld_rs);

	ai_dnlp_param_update(reg_val);

	return 0;
}

int skintone_scene_process(int offset, int enable)
{
	int bld_rs = 4;
	static int reg_val;
	int base_val = 0;
	int bld_offset;
	static int first_frame = 1;

	if (!enable) {
		first_frame = 1;
		return -1;
	}

	if (first_frame == 1) {
		reg_val = base_val;
		first_frame = 0;
	}

	bld_offset = smooth_process(base_val, reg_val, offset, bld_rs);

	if (offset == 0)
		reg_val = (reg_val < base_val) ?
			((reg_val + bld_offset) >= base_val ?
			base_val : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < base_val ?
			base_val : (reg_val - bld_offset));
	else
		reg_val = (offset > 0) ?
			((reg_val + bld_offset) >= (base_val + offset) ?
			(base_val + offset) : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < (base_val + offset) ?
			(base_val + offset) : (reg_val - bld_offset));

	cm2_sat(ecm2colormd_skin, reg_val, 0);
	cm2_curve_update_sat(ecm2colormd_skin);

	if (aipq_debug & (1 << SKIN_TONE_SCENE))
		pr_aipq_dbg(
			"%s, baseval: %d, regval: %d, offset: %d,bld_ofst: %d, bld_spd: %d\n",
			__func__, base_val, reg_val,
			offset, bld_offset, bld_rs);

	return 0;
}

int saturation_scene_process(int offset, int enable)
{
	int bld_rs = 4;
	static int reg_val;
	int base_val;
	int bld_offset;
	static int first_frame = 1;

	base_val = adap_param->satur_param.satur_hue_mab >> 16;
	adap_param->satur_param.offset = offset;

	if (!enable) {
		first_frame = 1;
		return -1;
	}

	if (first_frame == 1) {
		reg_val = base_val;
		first_frame = 0;
	}

	bld_offset = smooth_process(base_val, reg_val, offset, bld_rs);

	if (aipq_debug & (1 << SATURATION_SCENE))
		pr_aipq_dbg(
			"##%s, baseval: %d, regval: %d, offset: %d,bld_ofst: %d, bld_spd: %d\n",
			__func__, base_val, reg_val,
			offset, bld_offset, bld_rs);

	if (bld_offset == 0)
		return 0;

	if (offset == 0)
		reg_val = (reg_val < base_val) ?
			((reg_val + bld_offset) >= base_val ?
			base_val : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < base_val ?
			base_val : (reg_val - bld_offset));
	else
		reg_val = (offset > 0) ?
			((reg_val + bld_offset) >= (base_val + offset) ?
			(base_val + offset) : (reg_val + bld_offset)) :
			((reg_val - bld_offset) < (base_val + offset) ?
			(base_val + offset) : (reg_val - bld_offset));

	if (aipq_debug & (1 << SATURATION_SCENE))
		pr_aipq_dbg(
			"%s, baseval: %d, regval: %d, offset: %d,bld_ofst: %d, bld_spd: %d\n",
			__func__, base_val, reg_val,
			offset, bld_offset, bld_rs);

	amvecm_set_saturation_hue(reg_val << 16);
	return 0;
}

int noise_scene_process(int offset, int enable)
{
	return 0;
}

int adaptive_param_init(void)
{
	adap_param = &adaptive_param;
	/*dnlp parameters*/
	adap_param->dnlp_param.offset = 0;
	adap_param->dnlp_param.dnlp_final_gain = 8;
	/*saturation parameters*/
	adap_param->satur_param.offset = 0;
	adap_param->satur_param.satur_hue_mab = 0x1000000;
	/*peaking parameters*/
	adap_param->peaking_param.offset = 0;
	adap_param->peaking_param.sr0_hp_final_gain = 0;
	adap_param->peaking_param.sr0_bp_final_gain = 0;
	adap_param->peaking_param.sr1_hp_final_gain = 0;
	adap_param->peaking_param.sr1_bp_final_gain = 0;

	/*blue/green/skintone in cm,default def_sat_via_hs[3][32]*/
	return 0;
}

/*temporary solution for base param get from db*/
int aipq_base_dnlp_param(unsigned int final_gain)
{
	adap_param->dnlp_param.dnlp_final_gain = final_gain;
	return 0;
}

/*temporary solution for base param get from db*/
int aipq_base_peaking_param(
	unsigned int reg,
	unsigned int mask,
	unsigned int value)
{
	if (
	    (reg == SRSHARP0_PK_FINALGAIN_HP_BP + sr_offset[0]) &&
	    ((mask & 0xffff) == 0xffff)) {
		adap_param->peaking_param.sr0_hp_final_gain =
			(value >> 8) & 0xff;
		adap_param->peaking_param.sr0_bp_final_gain =
			value & 0xff;
	}

	if (
	    (reg == SRSHARP1_PK_FINALGAIN_HP_BP + sr_offset[1]) &&
	    ((mask & 0xffff) == 0xffff)) {
		adap_param->peaking_param.sr1_hp_final_gain =
			(value >> 8) & 0xff;
		adap_param->peaking_param.sr1_bp_final_gain =
			value & 0xff;
	}
	return 0;
}

/*temporary solution for base param get from db*/
int aipq_base_satur_param(int value)
{
	adap_param->satur_param.satur_hue_mab = value;
	return 0;
}

int ai_detect_scene_init(void)
{
	enum detect_scene_e det_scs;

	for (det_scs = BLUE_SCENE; det_scs <= SCENE_MAX; det_scs++) {
		switch (det_scs) {
		case BLUE_SCENE:
			detected_scenes[det_scs].enable = ENABLE;
			detected_scenes[det_scs].func = blue_scene_process;
			break;
		case GREEN_SCENE:
			detected_scenes[det_scs].enable = ENABLE;
			detected_scenes[det_scs].func = green_scene_process;
			break;
		case SKIN_TONE_SCENE:
			detected_scenes[det_scs].enable = ENABLE;
			detected_scenes[det_scs].func = skintone_scene_process;
			break;
		case PEAKING_SCENE:
			detected_scenes[det_scs].enable = ENABLE;
			detected_scenes[det_scs].func = peaking_scene_process;
			break;
		case SATURATION_SCENE:
			detected_scenes[det_scs].enable = ENABLE;
			detected_scenes[det_scs].func =
				saturation_scene_process;
			break;
		case DYNAMIC_CONTRAST_SCENE:
			detected_scenes[det_scs].enable = ENABLE;
			detected_scenes[det_scs].func = contrast_scene_process;
			break;
		case NOISE_SCENE:
			detected_scenes[det_scs].enable = ENABLE;
			detected_scenes[det_scs].func = noise_scene_process;
			break;
		default:
			break;
		}
	}

	return 0;
}

