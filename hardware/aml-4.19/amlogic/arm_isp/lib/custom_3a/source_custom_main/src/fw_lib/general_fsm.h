//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
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

#if !defined( __GENERAL_FSM_H__ )
#define __GENERAL_FSM_H__



typedef struct _general_fsm_t general_fsm_t;
typedef struct _general_fsm_t *general_fsm_ptr_t;
typedef const struct _general_fsm_t *general_fsm_const_ptr_t;

#include "acamera_sbus_api.h"
#include "acamera_firmware_config.h"
void acamera_reload_isp_calibratons( general_fsm_ptr_t p_fsm );
void general_initialize( general_fsm_ptr_t p_fsm );
void general_frame_start( general_fsm_ptr_t p_fsm );
void general_frame_end( general_fsm_ptr_t p_fsm );
void general_set_wdr_mode( general_fsm_ptr_t p_fsm );
uint32_t general_calc_fe_lut_output( general_fsm_ptr_t p_fsm, uint16_t val );
uint32_t general_calc_fe_lut_input( general_fsm_ptr_t p_fsm, uint16_t val );


struct _general_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    uint32_t api_color_mode;
    uint32_t api_scene_mode;
    uint32_t api_reg_addr;
    uint16_t api_reg_size;
    uint16_t api_reg_source;
    uint16_t calibration_read_status;
    acamera_sbus_t isp_sbus;
    uint32_t wdr_mode;

#if ISP_WDR_SWITCH
    uint32_t wdr_mode_req;
    uint32_t wdr_auto_mode;
    uint32_t wdr_mode_frames;
    uint32_t cur_exp_number;
#endif
};


void general_fsm_clear( general_fsm_ptr_t p_fsm );
void general_fsm_init( void *fsm, fsm_init_param_t *init_param );
int general_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size );
int general_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );

uint8_t general_fsm_process_event( general_fsm_ptr_t p_fsm, event_id_t event_id );

void general_fsm_process_interrupt( general_fsm_const_ptr_t p_fsm, uint8_t irq_event );

void general_request_interrupt( general_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __GENERAL_FSM_H__ */
