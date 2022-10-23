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

#if !defined( __AWB_ACAMERA_FSM_H__ )
#define __AWB_ACAMERA_FSM_H__

#include "acamera_isp_core_nomem_settings.h"
#include "awb_standard_api.h"

typedef struct _awb_acamera_fsm_t awb_acamera_fsm_t;
typedef struct _awb_acamera_fsm_t *awb_acamera_fsm_ptr_t;
typedef const struct _awb_acamera_fsm_t *awb_acamera_fsm_const_ptr_t;


#define ISP_HAS_AWB_FSM 1

#define MAX_AWB_ZONES ( ISP_METERING_ZONES_MAX_V * ISP_METERING_ZONES_MAX_H )
#define AWB_DLS_SWITCH_LIGHT_SOURCE_DETECT_FRAMES_QUANTITY 15
#define AWB_DLS_SWITCH_LIGHT_SOURCE_CHANGE_FRAMES_QUANTITY 35


void awb_init( awb_acamera_fsm_ptr_t p_fsm );
void awb_deinit( awb_acamera_fsm_ptr_t p_fsm );
void awb_process_light_source( awb_acamera_fsm_ptr_t p_fsm );
void awb_normalise( awb_acamera_fsm_ptr_t p_fsm );
void awb_process_stats( awb_acamera_fsm_ptr_t p_fsm );
void awb_set_frameid( awb_acamera_fsm_ptr_t p_fsm, uint32_t frame_id );
void awb_prev_param( awb_acamera_fsm_ptr_t p_fsm, isp_awb_preset_t * param );

struct _awb_acamera_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    uint16_t curr_AWB_ZONES;
    uint16_t rg_coef;
    uint16_t bg_coef;
    uint8_t p_high;
    uint32_t avg_GR;
    uint32_t avg_GB;
    awb_zone_t awb_zones[MAX_AWB_ZONES];
    int32_t wb_log2[4];
    int32_t temperature_detected;
    uint8_t light_source_detected;
    uint8_t light_source_candidate;
    uint8_t detect_light_source_frames_count;
    uint32_t mode;
    uint32_t roi;

    int32_t awb_warming[3];
    uint32_t switch_light_source_detect_frames_quantity;
    uint32_t switch_light_source_change_frames_quantity;
    uint8_t is_ready;

    uint32_t cur_using_stats_frame_id;
    uint32_t cur_result_gain_frame_id;

    void *lib_handle;

    awb_std_obj_t awb_alg_obj;
};


void awb_acamera_fsm_clear( awb_acamera_fsm_ptr_t p_fsm );
void awb_acamera_fsm_init( void *fsm, fsm_init_param_t *init_param );
void awb_acamera_fsm_deinit( void *fsm );
int awb_acamera_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size );
int awb_acamera_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );
uint8_t awb_acamera_fsm_process_event( awb_acamera_fsm_ptr_t p_fsm, event_id_t event_id );
void awb_acamera_fsm_process_interrupt( awb_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event );
void awb_acamera_fsm_request_interrupt( awb_acamera_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __AWB_ACAMERA_FSM_H__ */
