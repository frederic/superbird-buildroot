/*
 * drivers/amlogic/media/deinterlace/deinterlace_hw.h
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

#ifndef _DI_HW_H
#define _DI_HW_H
#include <linux/amlogic/media/amvecm/amvecm.h>
#include "di_pqa.h"
#include "nr_drv.h"

/* if post size < 80, filter of ei can't work */
#define MIN_POST_WIDTH  80
#define MIN_BLEND_WIDTH  27

#define	SKIP_CTRE_NUM	13

struct DI_MIF_s {
	unsigned short	luma_x_start0;
	unsigned short	luma_x_end0;
	unsigned short	luma_y_start0;
	unsigned short	luma_y_end0;
	unsigned short	chroma_x_start0;
	unsigned short	chroma_x_end0;
	unsigned short	chroma_y_start0;
	unsigned short	chroma_y_end0;
	unsigned int	nocompress;
	unsigned		set_separate_en:2;
	unsigned		src_field_mode:1;
	unsigned		src_prog:1;
	unsigned		video_mode:1;
	unsigned		output_field_num:1;
	unsigned		bit_mode:2;
	/*
	 * unsigned		burst_size_y:2; set 3 as default
	 * unsigned		burst_size_cb:2;set 1 as default
	 * unsigned		burst_size_cr:2;set 1 as default
	 */
	unsigned		canvas0_addr0:8;
	unsigned		canvas0_addr1:8;
	unsigned		canvas0_addr2:8;
	/* canvas_w: for input not 64 align*/
	unsigned int	canvas_w;
};

struct DI_SIM_MIF_s {
	unsigned short	start_x;
	unsigned short	end_x;
	unsigned short	start_y;
	unsigned short	end_y;
	unsigned short	canvas_num;
	unsigned short	bit_mode;
};

struct DI_MC_MIF_s {
	unsigned short start_x;
	unsigned short start_y;
	unsigned short end_y;
	unsigned short size_x;
	unsigned short size_y;
	unsigned short canvas_num;
	unsigned short blend_en;
	unsigned short vecrd_offset;
};

enum gate_mode_e {
	GATE_AUTO,
	GATE_ON,
	GATE_OFF,
};

struct mcinfo_lmv_s {
	unsigned char lock_flag;
	char			lmv;
	unsigned short lock_cnt;
};

struct di_pq_parm_s {
	struct am_pq_parm_s pq_parm;
	struct am_reg_s *regs;
	struct list_head list;
};

extern u32 afbc_disable_flag;
void read_pulldown_info(unsigned int *glb_frm_mot_num,
	unsigned int *glb_fid_mot_num);
unsigned int di_rd_mcdi_fldcnt(void);
void read_new_pulldown_info(struct FlmModReg_t *pFMRegp);
void pulldown_info_clear_g12a(void);
void combing_pd22_window_config(unsigned int width, unsigned int height);
void di_hw_init(bool pulldown_en, bool mc_enable);
void di_hw_uninit(void);
void enable_di_pre_aml(
	struct DI_MIF_s		   *di_inp_mif,
	struct DI_MIF_s		   *di_mem_mif,
	struct DI_MIF_s		   *di_chan2_mif,
	struct DI_SIM_MIF_s    *di_nrwr_mif,
	struct DI_SIM_MIF_s    *di_mtnwr_mif,
	struct DI_SIM_MIF_s    *di_contp2rd_mif,
	struct DI_SIM_MIF_s    *di_contprd_mif,
	struct DI_SIM_MIF_s    *di_contwr_mif,
	unsigned char madi_en, unsigned char pre_field_num,
	unsigned char pre_vdin_link);

void mc_pre_mv_irq(void);
void enable_mc_di_pre(struct DI_MC_MIF_s *di_mcinford_mif,
	struct DI_MC_MIF_s *di_mcinfowr_mif,
	struct DI_MC_MIF_s *di_mcvecwr_mif,
	unsigned char mcdi_en);
void enable_mc_di_pre_g12(struct DI_MC_MIF_s *di_mcinford_mif,
	struct DI_MC_MIF_s *di_mcinfowr_mif,
	struct DI_MC_MIF_s *di_mcvecwr_mif,
	unsigned char mcdi_en);

void enable_mc_di_post(struct DI_MC_MIF_s *di_mcvecrd_mif,
	int urgent, bool reverse, int invert_mv);
void enable_mc_di_post_g12(struct DI_MC_MIF_s *di_mcvecrd_mif,
	int urgent, bool reverse, int invert_mv);

void disable_post_deinterlace_2(void);
void initial_di_post_2(int hsize_post, int vsize_post,
	int hold_line, bool write_en);
void enable_di_post_2(
	struct DI_MIF_s		*di_buf0_mif,
	struct DI_MIF_s		*di_buf1_mif,
	struct DI_MIF_s		*di_buf2_mif,
	struct DI_SIM_MIF_s	*di_diwr_mif,
	struct DI_SIM_MIF_s	*di_mtnprd_mif,
	int ei_en, int blend_en, int blend_mtn_en, int blend_mode,
	int di_vpp_en, int di_ddr_en,
	int post_field_num, int hold_line, int urgent,
	int invert_mv, int vskip_cnt
);
void di_post_switch_buffer(
	struct DI_MIF_s		*di_buf0_mif,
	struct DI_MIF_s		*di_buf1_mif,
	struct DI_MIF_s		*di_buf2_mif,
	struct DI_SIM_MIF_s	*di_diwr_mif,
	struct DI_SIM_MIF_s	*di_mtnprd_mif,
	struct DI_MC_MIF_s		*di_mcvecrd_mif,
	int ei_en, int blend_en, int blend_mtn_en, int blend_mode,
	int di_vpp_en, int di_ddr_en,
	int post_field_num, int hold_line, int urgent,
	int invert_mv, bool pd_en, bool mc_enable,
	int vskip_cnt
);
void di_post_read_reverse_irq(bool reverse,
	unsigned char mc_pre_flag, bool mc_enable);
void di_hdr2_hist_init(void);
void di_top_gate_control(bool top_en, bool mc_en);
void di_pre_gate_control(bool enable, bool mc_enable);
void di_post_gate_control(bool gate);
void diwr_set_power_control(unsigned char enable);
void di_hw_disable(bool mc_enable);
void enable_di_pre_mif(bool enable, bool mc_enable);
void enable_di_post_mif(enum gate_mode_e mode);
void di_hw_uninit(void);
void combing_pd22_window_config(unsigned int width, unsigned int height);
void calc_lmv_init(void);
void calc_lmv_base_mcinfo(unsigned int vf_height, unsigned short *mcinfo_vadr);
void init_field_mode(unsigned short height);
void film_mode_win_config(unsigned int width, unsigned int height);
void pulldown_vof_win_config(struct pulldown_detected_s *wins);
void di_load_regs(struct di_pq_parm_s *di_pq_ptr);
void pre_frame_reset_g12(unsigned char madi_en, unsigned char mcdi_en);
void pre_frame_reset(void);
void di_hpre_gl_sw(bool on);
void di_interrupt_ctrl(unsigned char ma_en,
	unsigned char det3d_en, unsigned char nrds_en,
	unsigned char post_wr, unsigned char mc_en);
void di_txl_patch_prog(int prog_flg, unsigned int cnt, bool mc_en);
//extern void afbc_power_sw(bool on);
/*extern void afbc_sw_trig(bool  on);*/
extern void dump_vd2_afbc(void);

extern u8 *di_vmap(ulong addr, u32 size, bool *bflg);
extern void di_unmap_phyaddr(u8 *vaddr);
extern int di_print(const char *fmt, ...);

#define DI_MC_SW_OTHER	(1<<0)
#define DI_MC_SW_REG	(1<<1)
//#define DI_MC_SW_POST	(1<<2)
#define DI_MC_SW_IC	(1<<2)

#define DI_MC_SW_ON_MASK	(DI_MC_SW_REG | DI_MC_SW_OTHER | DI_MC_SW_IC)

extern void di_patch_post_update_mc(void);
extern void di_patch_post_update_mc_sw(unsigned int cmd, bool on);

struct reg_acc {
	void (*wr)(unsigned int adr, unsigned int val);
	unsigned int (*rd)(unsigned int adr);
	unsigned int (*bwr)(unsigned int adr, unsigned int val,
			    unsigned int start, unsigned int len);
	unsigned int (*brd)(unsigned int adr, unsigned int start,
			    unsigned int len);

};
extern void di_rst_protect(bool on);
extern void di_pre_nr_wr_done_sel(bool on);
extern void di_arb_sw(bool on);
extern enum eAFBC_DEC afbc_get_decnub(void);

/*also see: dbg_mode_name*/
enum eDI_DBG_MOD {
	eDI_DBG_MOD_REGB,	//0
	eDI_DBG_MOD_REGE,	//1
	eDI_DBG_MOD_UNREGB,	//2
	eDI_DBG_MOD_UNREGE,	// 3
	eDI_DBG_MOD_PRE_SETB,	// 4
	eDI_DBG_MOD_PRE_SETE,	// 5
	eDI_DBG_MOD_PRE_DONEB,	// 6
	eDI_DBG_MOD_PRE_DONEE,	// 7
	eDI_DBG_MOD_POST_SETB,	// 8
	eDI_DBG_MOD_POST_SETE,	// 9
	eDI_DBG_MOD_POST_IRQB,	// a
	eDI_DBG_MOD_POST_IRQE,	// b
	eDI_DBG_MOD_POST_DB,	// c
	eDI_DBG_MOD_POST_DE,	// d
	eDI_DBG_MOD_POST_CH_CHG,	// e
	eDI_DBG_MOD_POST_TIMEOUT,	// F

	eDI_DBG_MOD_RVB,	//10
	eDI_DBG_MOD_RVE,	//11

	eDI_DBG_MOD_POST_RESIZE, //0x12

	//---add for debug tl1
	eDI_DBG_MOD_PQB,	//0x13
	eDI_DBG_MOD_PQE,	//0x14
	eDI_DBG_MOD_OREGB,	//0x15
	eDI_DBG_MOD_OREGE,	//0x16
	eDI_DBG_MOD_OUNREGB,	//0x17
	eDI_DBG_MOD_OUNREGE,	//0x18
	eDI_DBG_MOD_PRE_IRQB,	//0x19
	eDI_DBG_MOD_PRE_IRQE,	//0x1a
	eDI_DBG_MOD_PRE_TIMEOUT,	//0x1b
	eDI_DBG_MOD_END,

};

unsigned int get_reg_bits(unsigned int val, unsigned int bstart,
			  unsigned int bw);

extern void ddbg_mod_save(unsigned int mod, unsigned int ch, unsigned int cnt);

#endif
