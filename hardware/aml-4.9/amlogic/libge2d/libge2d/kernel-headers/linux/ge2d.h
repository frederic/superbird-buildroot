/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef GE2D_H
#define GE2D_H

#define CONFIG_GE2D_SRC2
#define OPERATION_ADD           0    /* Cd = Cs*Fs+Cd*Fd */
#define OPERATION_SUB           1    /* Cd = Cs*Fs-Cd*Fd */
#define OPERATION_REVERSE_SUB   2    /* Cd = Cd*Fd-Cs*Fs */
#define OPERATION_MIN           3    /* Cd = Min(Cd*Fd,Cs*Fs) */
#define OPERATION_MAX           4    /* Cd = Max(Cd*Fd,Cs*Fs) */
#define OPERATION_LOGIC         5

#define COLOR_FACTOR_ZERO                     0
#define COLOR_FACTOR_ONE                      1
#define COLOR_FACTOR_SRC_COLOR                2
#define COLOR_FACTOR_ONE_MINUS_SRC_COLOR      3
#define COLOR_FACTOR_DST_COLOR                4
#define COLOR_FACTOR_ONE_MINUS_DST_COLOR      5
#define COLOR_FACTOR_SRC_ALPHA                6
#define COLOR_FACTOR_ONE_MINUS_SRC_ALPHA      7
#define COLOR_FACTOR_DST_ALPHA                8
#define COLOR_FACTOR_ONE_MINUS_DST_ALPHA      9
#define COLOR_FACTOR_CONST_COLOR              10
#define COLOR_FACTOR_ONE_MINUS_CONST_COLOR    11
#define COLOR_FACTOR_CONST_ALPHA              12
#define COLOR_FACTOR_ONE_MINUS_CONST_ALPHA    13
#define COLOR_FACTOR_SRC_ALPHA_SATURATE       14

#define ALPHA_FACTOR_ZERO                     0
#define ALPHA_FACTOR_ONE                      1
#define ALPHA_FACTOR_SRC_ALPHA                2
#define ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA      3
#define ALPHA_FACTOR_DST_ALPHA                4
#define ALPHA_FACTOR_ONE_MINUS_DST_ALPHA      5
#define ALPHA_FACTOR_CONST_ALPHA              6
#define ALPHA_FACTOR_ONE_MINUS_CONST_ALPHA    7

#define	 GE2D_GET_CAP               0x470b
#define  GE2D_BLEND_NOALPHA_NOBLOCK 0x470a
#define  GE2D_BLEND_NOALPHA         0x4709
#define  GE2D_STRETCHBLIT_NOALPHA   0x4702
#define  GE2D_BLIT_NOALPHA          0x4701
#define  GE2D_BLEND                 0x4700
#define  GE2D_BLIT                  0x46ff
#define  GE2D_STRETCHBLIT           0x46fe
#define  GE2D_FILLRECTANGLE         0x46fd
#define  GE2D_SET_COEF              0x46fb
#define  GE2D_ANTIFLICKER_ENABLE    0x46f8

typedef enum {
    OSD0_OSD0 = 0,
    OSD0_OSD1,
    OSD1_OSD1,
    OSD1_OSD0,
    ALLOC_OSD0,
    ALLOC_OSD1,
    ALLOC_ALLOC,
    TYPE_INVALID,
} ge2d_src_dst_t;

enum ge2d_src_canvas_type_e {
    CANVAS_OSD0 = 0,
    CANVAS_OSD1,
    CANVAS_ALLOC,
    CANVAS_TYPE_INVALID,
};

struct config_planes_s {
    unsigned long   addr;
    unsigned int    w;
    unsigned int    h;
};

struct src_key_ctrl_s {
    int key_enable;
    int key_color;
    int key_mask;
    int key_mode;
};

struct config_para_s {
    int  src_dst_type;
    int  alu_const_color;
    unsigned int src_format;
    unsigned int dst_format ; //add for src&dst all in user space.
    struct config_planes_s src_planes[4];
    struct config_planes_s dst_planes[4];
    struct src_key_ctrl_s  src_key;
};



struct rectangle_s {
    int x;   /* X coordinate of its top-left point */
    int y;   /* Y coordinate of its top-left point */
    int w;   /* width of it */
    int h;   /* height of it */
};

struct ge2d_para_s {
    unsigned int color ;
    struct rectangle_s src1_rect;
    struct rectangle_s src2_rect;
    struct rectangle_s dst_rect;
    int op;
};

struct src_dst_para_ex_s {
    int  canvas_index;
    int  top;
    int  left;
    int  width;
    int  height;
    int  format;
    int  mem_type;
    int  color;
    unsigned char x_rev;
    unsigned char y_rev;
    unsigned char fill_color_en;
    unsigned char fill_mode;
};

struct config_para_ex_s {
    struct src_dst_para_ex_s src_para;
    struct src_dst_para_ex_s src2_para;
    struct src_dst_para_ex_s dst_para;

    /* key mask */
    struct src_key_ctrl_s  src_key;
    struct src_key_ctrl_s  src2_key;

    int alu_const_color;
    unsigned src1_gb_alpha;
#ifdef CONFIG_GE2D_SRC2
	unsigned int src2_gb_alpha;
#endif
    unsigned op_mode;
    unsigned char bitmask_en;
    unsigned char bytemask_only;
    unsigned int  bitmask;
    unsigned char dst_xy_swap;

    /* scaler and phase releated */
    unsigned hf_init_phase;
    int hf_rpt_num;
    unsigned hsc_start_phase_step;
    int hsc_phase_slope;
    unsigned vf_init_phase;
    int vf_rpt_num;
    unsigned vsc_start_phase_step;
    int vsc_phase_slope;
    unsigned char src1_vsc_phase0_always_en;
    unsigned char src1_hsc_phase0_always_en;
    /* 1bit, 0: using minus, 1: using repeat data */
    unsigned char src1_hsc_rpt_ctrl;
    /* 1bit, 0: using minus  1: using repeat data */
    unsigned char src1_vsc_rpt_ctrl;

    /* canvas info */
    struct config_planes_s src_planes[4];
    struct config_planes_s src2_planes[4];
    struct config_planes_s dst_planes[4];
};

struct config_planes_ion_s {
	unsigned long addr;
	unsigned int w;
	unsigned int h;
	int shared_fd;
};

struct config_para_ex_ion_s_en {
	struct src_dst_para_ex_s src_para;
	struct src_dst_para_ex_s src2_para;
	struct src_dst_para_ex_s dst_para;

	/* key mask */
	struct src_key_ctrl_s  src_key;
	struct src_key_ctrl_s  src2_key;

	unsigned char src1_cmult_asel;
	unsigned char src2_cmult_asel;
#ifdef CONFIG_GE2D_SRC2
	unsigned char src2_cmult_ad;
#endif
	int alu_const_color;
	unsigned char src1_gb_alpha_en;
	unsigned src1_gb_alpha;
#ifdef CONFIG_GE2D_SRC2
	unsigned char src2_gb_alpha_en;
	unsigned int src2_gb_alpha;
#endif
	unsigned op_mode;
	unsigned char bitmask_en;
	unsigned char bytemask_only;
	unsigned int  bitmask;
	unsigned char dst_xy_swap;

	/* scaler and phase releated */
	unsigned hf_init_phase;
	int hf_rpt_num;
	unsigned hsc_start_phase_step;
	int hsc_phase_slope;
	unsigned vf_init_phase;
	int vf_rpt_num;
	unsigned vsc_start_phase_step;
	int vsc_phase_slope;
	unsigned char src1_vsc_phase0_always_en;
	unsigned char src1_hsc_phase0_always_en;
	/* 1bit, 0: using minus, 1: using repeat data */
	unsigned char src1_hsc_rpt_ctrl;
	/* 1bit, 0: using minus  1: using repeat data */
	unsigned char src1_vsc_rpt_ctrl;

	/* canvas info */
	struct config_planes_ion_s src_planes[4];
	struct config_planes_ion_s src2_planes[4];
	struct config_planes_ion_s dst_planes[4];
};

struct config_para_ex_ion_s {
	struct src_dst_para_ex_s src_para;
	struct src_dst_para_ex_s src2_para;
	struct src_dst_para_ex_s dst_para;

	/* key mask */
	struct src_key_ctrl_s  src_key;
	struct src_key_ctrl_s  src2_key;

	unsigned char src1_cmult_asel;
	unsigned char src2_cmult_asel;
	unsigned char src2_cmult_ad;

	int alu_const_color;
	unsigned char src1_gb_alpha_en;
	unsigned src1_gb_alpha;
	unsigned char src2_gb_alpha_en;
	unsigned int src2_gb_alpha;
	unsigned op_mode;
	unsigned char bitmask_en;
	unsigned char bytemask_only;
	unsigned int  bitmask;
	unsigned char dst_xy_swap;

	/* scaler and phase releated */
	unsigned hf_init_phase;
	int hf_rpt_num;
	unsigned hsc_start_phase_step;
	int hsc_phase_slope;
	unsigned vf_init_phase;
	int vf_rpt_num;
	unsigned vsc_start_phase_step;
	int vsc_phase_slope;
	unsigned char src1_vsc_phase0_always_en;
	unsigned char src1_hsc_phase0_always_en;
	/* 1bit, 0: using minus, 1: using repeat data */
	unsigned char src1_hsc_rpt_ctrl;
	/* 1bit, 0: using minus  1: using repeat data */
	unsigned char src1_vsc_rpt_ctrl;

	/* canvas info */
	struct config_planes_ion_s src_planes[4];
	struct config_planes_ion_s src2_planes[4];
	struct config_planes_ion_s dst_planes[4];
};

struct config_para_ex_memtype_s {
	int ge2d_magic;
	struct config_para_ex_ion_s _ge2d_config_ex;
	/* memtype*/
	unsigned int src1_mem_alloc_type;
	unsigned int src2_mem_alloc_type;
	unsigned int dst_mem_alloc_type;
};

struct config_ge2d_para_ex_s {
	union {
		struct config_para_ex_ion_s para_config_ion;
		struct config_para_ex_memtype_s para_config_memtype;
	};
};

struct ge2d_dmabuf_req_s {
	int index;
	unsigned int len;
	unsigned int dma_dir;
};

struct ge2d_dmabuf_exp_s {
	int index;
	unsigned int flags;
	int fd;
};

#define GE2D_ENDIAN_SHIFT       24
#define GE2D_ENDIAN_MASK        (0x1 << GE2D_ENDIAN_SHIFT)
#define GE2D_BIG_ENDIAN	        (0 << GE2D_ENDIAN_SHIFT)
#define GE2D_LITTLE_ENDIAN      (1 << GE2D_ENDIAN_SHIFT)

#define GE2D_COLOR_MAP_SHIFT    20
#define GE2D_COLOR_MAP_MASK     (0xf << GE2D_COLOR_MAP_SHIFT)
/* nv12 &nv21, only works on m6*/
#define GE2D_COLOR_MAP_NV12		(15 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_NV21		(14 << GE2D_COLOR_MAP_SHIFT)

/* deep color, only works after TXL */
#define GE2D_COLOR_MAP_10BIT_YUV444		(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_10BIT_VUY444		(5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_10BIT_YUV422		(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_10BIT_YVU422		(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_12BIT_YUV422		(8 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_12BIT_YVU422		(9 << GE2D_COLOR_MAP_SHIFT)

/* 16 bit */
#define GE2D_COLOR_MAP_YUV422   (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB655   (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV655   (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB844   (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV844   (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6442 (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6442 (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4444 (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4444 (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB565   (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV565   (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB4444 (6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV4444 (6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB1555 (7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV1555 (7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4642 (8 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4642 (8 << GE2D_COLOR_MAP_SHIFT)
/* 24 bit */
#define GE2D_COLOR_MAP_RGB888   (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV444   (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA5658 (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA5658 (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8565 (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8565 (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6666 (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6666 (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB6666 (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV6666 (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGR888   (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUY888   (5 << GE2D_COLOR_MAP_SHIFT)
/* 32 bit */
#define GE2D_COLOR_MAP_RGBA8888 (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA8888 (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8888 (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8888 (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ABGR8888 (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AVUY8888 (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGRA8888 (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUYA8888 (3 << GE2D_COLOR_MAP_SHIFT)
/*
 *format code is defined as:
 *[18] : 1-deep color mode(10/12 bit), 0-8bit mode
 *[17] : 1-YUV color space, 0-RGB color space
 *[16] : compress_range
 *[9:8]: format
 *[7:6]: 8bit_mode_sel
 *[5]  : LUT_EN
 *[4:3]: PIC_STRUCT
 *[2]  : SEP_EN
 *[1:0]: X_YC_RATIO, SRC1_Y_YC_RATIO
 according datasheet--
  16: 16-bit mode
  24: 24-bit mode
  32: 32-bit mode
 */
#define GE2D_FORMAT_MASK                0x0ffff
#define GE2D_BPP_MASK                   0x00300
#define GE2D_BPP_8BIT                   0x00000
#define GE2D_BPP_16BIT                  0x00100
#define GE2D_BPP_24BIT                  0x00200
#define GE2D_BPP_32BIT                  0x00300
#define GE2D_FORMAT_DEEP_COLOR   0x40000
#define GE2D_FORMAT_YUV                 0x20000
#define GE2D_FORMAT_COMP_RANGE          0x10000
/*bit8(2)  format   bi6(2) mode_8b_sel  bit5(1)lut_en   bit2 sep_en*/
/*M  separate block S one block.*/

#define GE2D_FMT_S8_Y           0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_CB          0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_CR          0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_R           0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_G           0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_B           0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_A           0x000c0 /* 00_11_0_00_0_00 */
#define GE2D_FMT_S8_LUT         0x00020 /* 00_00_1_00_0_00 */
#define GE2D_FMT_S16_YUV422     0x20102 /* 01_00_0_00_0_00 */
#define GE2D_FMT_S16_RGB        (GE2D_LITTLE_ENDIAN|0x00100) /* 01_00_0_00_0_00 */
#define GE2D_FMT_S24_YUV444     0x20200 /* 10_00_0_00_0_00 */
#define GE2D_FMT_S24_RGB        (GE2D_LITTLE_ENDIAN|0x00200) /* 10_00_0_00_0_00 */
#define GE2D_FMT_S32_YUVA444    0x20300 /* 11_00_0_00_0_00 */
#define GE2D_FMT_S32_RGBA       (GE2D_LITTLE_ENDIAN|0x00300) /* 11_00_0_00_0_00 */
#define GE2D_FMT_M24_YUV420     0x20007 /* 00_00_0_00_1_11 */
#define GE2D_FMT_M24_YUV422     0x20006 /* 00_00_0_00_1_10 */
#define GE2D_FMT_M24_YUV444     0x20004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_RGB        0x00004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_YUV420T    0x20017 /* 00_00_0_10_1_11 */
#define GE2D_FMT_M24_YUV420B    0x2001f /* 00_00_0_11_1_11 */
#define GE2D_FMT_M24_YUV420SP		0x20207
#define GE2D_FMT_M24_YUV422SP		0x20206
/* 01_00_0_00_1_11 nv12 &nv21, only works on m6. */
#define GE2D_FMT_M24_YUV420SPT		0x20217
/* 01_00_0_00_1_11 nv12 &nv21, only works on m6. */
#define GE2D_FMT_M24_YUV420SPB		0x2021f
#define GE2D_FMT_S16_YUV422T    0x20110 /* 01_00_0_10_0_00 */
#define GE2D_FMT_S16_YUV422B    0x20138 /* 01_00_0_11_0_00 */
#define GE2D_FMT_S24_YUV444T    0x20210 /* 10_00_0_10_0_00 */
#define GE2D_FMT_S24_YUV444B    0x20218 /* 10_00_0_11_0_00 */
/* only works after TXL and for src1. */
#define GE2D_FMT_S24_10BIT_YUV444		0x60200
#define GE2D_FMT_S24_10BIT_YUV444T		0x60210
#define GE2D_FMT_S24_10BIT_YUV444B		0x60218

#define GE2D_FMT_S16_10BIT_YUV422		0x60102
#define GE2D_FMT_S16_10BIT_YUV422T		0x60112
#define GE2D_FMT_S16_10BIT_YUV422B		0x6011a

#define GE2D_FMT_S16_12BIT_YUV422		0x60102
#define GE2D_FMT_S16_12BIT_YUV422T		0x60112
#define GE2D_FMT_S16_12BIT_YUV422B		0x6011a

/* back compatible defines */
#define GE2D_FORMAT_S8_Y           (GE2D_FORMAT_YUV|GE2D_FMT_S8_Y)
#define GE2D_FORMAT_S8_CB          (GE2D_FORMAT_YUV|GE2D_FMT_S8_CB)
#define GE2D_FORMAT_S8_CR          (GE2D_FORMAT_YUV|GE2D_FMT_S8_CR)
#define GE2D_FORMAT_S8_R            (GE2D_FMT_S8_R)
#define GE2D_FORMAT_S8_G            (GE2D_FMT_S8_G)
#define GE2D_FORMAT_S8_B            (GE2D_FMT_S8_B)
#define GE2D_FORMAT_S8_A            (GE2D_FMT_S8_A)
#define GE2D_FORMAT_S8_LUT          (GE2D_FMT_S8_LUT)
/* nv12 &nv21, only works on m6. */
#define GE2D_FORMAT_M24_NV12  (GE2D_FMT_M24_YUV420SP | GE2D_COLOR_MAP_NV12)
#define GE2D_FORMAT_M24_NV12T (GE2D_FMT_M24_YUV420SPT | GE2D_COLOR_MAP_NV12)
#define GE2D_FORMAT_M24_NV12B (GE2D_FMT_M24_YUV420SPB | GE2D_COLOR_MAP_NV12)
#define GE2D_FORMAT_M24_NV21        (GE2D_FMT_M24_YUV420SP | GE2D_COLOR_MAP_NV21)
#define GE2D_FORMAT_M24_NV21T (GE2D_FMT_M24_YUV420SPT | GE2D_COLOR_MAP_NV21)
#define GE2D_FORMAT_M24_NV21B (GE2D_FMT_M24_YUV420SPB | GE2D_COLOR_MAP_NV21)
#define GE2D_FORMAT_S12_RGB_655 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGB655)
#define GE2D_FORMAT_S16_YUV422      (GE2D_FMT_S16_YUV422    | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_RGB_655     (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_RGB655)
#define GE2D_FORMAT_S24_YUV444      (GE2D_FMT_S24_YUV444    | GE2D_COLOR_MAP_YUV444)
#define GE2D_FORMAT_S24_RGB	        (GE2D_FMT_S24_RGB       | GE2D_COLOR_MAP_RGB888)
#define GE2D_FORMAT_S32_YUVA444     (GE2D_FMT_S32_YUVA444   | GE2D_COLOR_MAP_YUVA4444)


#define GE2D_FORMAT_M24_YUV420      (GE2D_FMT_M24_YUV420)
#define GE2D_FORMAT_M24_YUV422      (GE2D_FMT_M24_YUV422)
#define GE2D_FORMAT_M24_YUV444      (GE2D_FMT_M24_YUV444)
#define GE2D_FORMAT_M24_RGB         (GE2D_FMT_M24_RGB)
#define GE2D_FORMAT_M24_YUV420T     (GE2D_FMT_M24_YUV420T)
#define GE2D_FORMAT_M24_YUV420B     (GE2D_FMT_M24_YUV420B)
#define GE2D_FORMAT_M24_YUV422SP    (GE2D_FMT_M24_YUV422SP | GE2D_COLOR_MAP_NV12)
#define GE2D_FORMAT_S16_YUV422T     (GE2D_FMT_S16_YUV422T | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_YUV422B     (GE2D_FMT_S16_YUV422B | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S24_YUV444T     (GE2D_FMT_S24_YUV444T | GE2D_COLOR_MAP_YUV444)
#define GE2D_FORMAT_S24_YUV444B     (GE2D_FMT_S24_YUV444B | GE2D_COLOR_MAP_YUV444)
/*16 bit*/
#define GE2D_FORMAT_S16_RGB_565     (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_RGB565)
#define GE2D_FORMAT_S16_RGB_844     (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_RGB844)
#define GE2D_FORMAT_S16_RGBA_6442   (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_RGBA6442)
#define GE2D_FORMAT_S16_RGBA_4444   (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_RGBA4444)
#define GE2D_FORMAT_S16_ARGB_4444   (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_ARGB4444)
#define GE2D_FORMAT_S16_ARGB_1555   (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_ARGB1555)
#define GE2D_FORMAT_S16_RGBA_4642   (GE2D_FMT_S16_RGB       | GE2D_COLOR_MAP_RGBA4642)
/*24 bit*/
#define GE2D_FORMAT_S24_RGBA_5658 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGBA5658)
#define GE2D_FORMAT_S24_ARGB_8565 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_ARGB8565)
#define GE2D_FORMAT_S24_RGBA_6666 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGBA6666)
#define GE2D_FORMAT_S24_ARGB_6666 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_ARGB6666)
#define GE2D_FORMAT_S24_BGR (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_BGR888)
/*32 bit*/
#define GE2D_FORMAT_S32_RGBA        (GE2D_FMT_S32_RGBA      | GE2D_COLOR_MAP_RGBA8888)
#define GE2D_FORMAT_S32_ARGB        (GE2D_FMT_S32_RGBA      | GE2D_COLOR_MAP_ARGB8888)
#define GE2D_FORMAT_S32_ABGR        (GE2D_FMT_S32_RGBA      | GE2D_COLOR_MAP_ABGR8888)
#define GE2D_FORMAT_S32_BGRA        (GE2D_FMT_S32_RGBA      | GE2D_COLOR_MAP_BGRA8888)

/* format added in TXL */
#define GE2D_FORMAT_S24_10BIT_YUV444 \
	(GE2D_FMT_S24_10BIT_YUV444 | GE2D_COLOR_MAP_10BIT_YUV444)

#define GE2D_FORMAT_S24_10BIT_VUY444 \
	(GE2D_FMT_S24_10BIT_YUV444 | GE2D_COLOR_MAP_10BIT_VUY444)

#define GE2D_FORMAT_S16_10BIT_YUV422 \
	(GE2D_FMT_S16_10BIT_YUV422 | GE2D_COLOR_MAP_10BIT_YUV422)

#define GE2D_FORMAT_S16_10BIT_YVU422 \
	(GE2D_FMT_S16_10BIT_YUV422 | GE2D_COLOR_MAP_10BIT_YVU422)

#define GE2D_FORMAT_S16_12BIT_YUV422 \
	(GE2D_FMT_S16_12BIT_YUV422 | GE2D_COLOR_MAP_12BIT_YUV422)

#define GE2D_FORMAT_S16_12BIT_YVU422 \
	(GE2D_FMT_S16_12BIT_YUV422 | GE2D_COLOR_MAP_12BIT_YVU422)

#define GE2D_IOC_MAGIC  'G'

#define GE2D_CONFIG     _IOW(GE2D_IOC_MAGIC, 0x00, struct config_para_s)
#define GE2D_CONFIG_EX  _IOW(GE2D_IOC_MAGIC, 0x01, struct config_para_ex_s)
#define GE2D_SRCCOLORKEY    _IOW(GE2D_IOC_MAGIC, 0x02, struct config_para_s)
#define GE2D_CONFIG_EX_ION	 _IOW(GE2D_IOC_MAGIC, 0x03,  struct config_para_ex_ion_s)
#define GE2D_CONFIG_EX_ION_EN	 _IOW(GE2D_IOC_MAGIC, 0x03,  struct config_para_ex_ion_s_en)

#define GE2D_REQUEST_BUFF _IOW(GE2D_IOC_MAGIC, 0x04, struct ge2d_dmabuf_req_s)
#define GE2D_EXP_BUFF _IOW(GE2D_IOC_MAGIC, 0x05, struct ge2d_dmabuf_exp_s)
#define GE2D_FREE_BUFF _IOW(GE2D_IOC_MAGIC, 0x06, int)

#define GE2D_CONFIG_EX_MEM   \
     _IOW(GE2D_IOC_MAGIC, 0x07,  struct config_ge2d_para_ex_s)

#define GE2D_SYNC_DEVICE _IOW(GE2D_IOC_MAGIC, 0x08, int)
#define GE2D_SYNC_CPU _IOW(GE2D_IOC_MAGIC, 0x09, int)

#endif /* GE2D_H */
