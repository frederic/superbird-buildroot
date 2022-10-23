//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#ifndef __ISP_CMN_CTRL_H__
#define __ISP_CMN_CTRL_H__

#define WDR_MODE_LINEAR                                   0x00000000
#define WDR_MODE_NATIVE                                   0x00000001
#define WDR_MODE_FS_LIN                                   0x00000002
#define WDR_MODE_COUNT                                    0x00000003

typedef enum dma_type {
    dma_fr = 0,
    dma_ds1,
    dma_ds2,
    dma_max
} dma_type;

typedef struct _aframe_t {
    uint32_t type ;        // frame type.
    uint32_t width ;       // frame width
    uint32_t height ;      // frame height
    unsigned long address ;     // start of memory block
    uint32_t line_offset ; // line offset for the frame
    uint32_t size ; // total size of the memory in bytes
    uint32_t status ;
} aframe_t ;

typedef struct _tframe_t {
    aframe_t primary;        //primary frames
    aframe_t secondary ;        //secondary frames
} tframe_t ;

typedef struct _fsm_param_ae_prst_ {
    int64_t integrator;
} ae_prst_param_t;

typedef struct _fsm_param_awb_prst_ {
    int32_t red_wb_log2;
    int32_t blue_wb_log2;
    uint32_t prst_cnt;
} awb_prst_param_t;

typedef struct _fsm_param_gamma_prst_ {
    uint32_t apply_gain;
    uint32_t auto_level_offset;
    uint32_t prst_cnt;
} gamma_prst_param_t;

typedef struct _fsm_param_iridix_prst_ {
    uint16_t strength_target;
    uint32_t iridix_contrast;
    uint16_t dark_enh;
    uint16_t iridix_global_DG;
    uint32_t prst_cnt;
} iridix_prst_param_t;

struct sensor_mode_info {
	uint32_t width;
	uint32_t height;
	uint32_t fps;
	uint32_t mode;
	uint32_t exps;
	uint32_t idx;
};

struct frame_info {
	uint32_t p_type;
	unsigned long idx;
	unsigned long addr;
	uint32_t fmt;
	uint32_t size;
};

struct alg_prst_param {
	uint32_t alg_prst_enable;
	ae_prst_param_t ae_prst;
	awb_prst_param_t awb_prst;
	gamma_prst_param_t gamma_prst;
	iridix_prst_param_t iridix_prst;
};

struct module_cfg_info {
	uint32_t p_type;
	uint32_t x_off;
	uint32_t y_off;
	uint32_t width;
	uint32_t height;
	uint32_t s_width;
	uint32_t s_height;
	uint32_t fmt;
	uint32_t planes;
	uint32_t auto_enable;
	tframe_t tfrm[2];
};

int32_t fw_alloc_path_buff(void *cfg_info);
int32_t fw_set_sensor_mode(void *mode_info);
int32_t fw_set_ds_scaler(void *cfg_info);
int32_t fw_set_path_crop(void *cfg_info);
int32_t fw_set_path_format(void *cfg_info);
void fw_set_path_buff(void *cfg_info);
void fw_sensor_stream_on(void);
void fw_sensor_stream_off(void);
int32_t fw_path_cfg(void *cfg_info);
void fw_auto_cap_cfg(void *cfg_info);
void fw_auto_cap_ping_pong(void *cfg_info);
void fw_auto_cap_start(void *cfg_info);
uint32_t fw_auto_cap_cnt(void *cfg_info);
void fw_auto_cap_stop(void *cfg_info);
void fw_auto_cap_frm_info(void *frm_info);
void fw_alg_prst_cfg(void *alg_info);
int32_t fw_init(void *p_set);
void fw_deinit(void);

#endif
