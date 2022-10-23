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

#if !defined( __AE_FSM_H__ )
#define __AE_FSM_H__


#include "acamera_firmware_config.h"
#include "ae_standard_api.h"

typedef struct _ae_acamera_fsm_t ae_acamera_fsm_t;
typedef struct _ae_acamera_fsm_t *ae_acamera_fsm_ptr_t;
typedef const struct _ae_acamera_fsm_t *ae_acamera_fsm_const_ptr_t;

#define ISP_HAS_AE_FSM 1

enum { AE_CLIPPING_ANTIFLICKER_N = 5 };

void ae_acamera_initialize( ae_acamera_fsm_ptr_t p_fsm );
void ae_acamera_deinitialize( ae_acamera_fsm_ptr_t p_fsm );
void ae_exposure_correction( ae_acamera_fsm_ptr_t p_fsm, int32_t corr );
void ae_set_frameid( ae_acamera_fsm_ptr_t p_fsm, uint32_t frame_id );
void ae_prev_param( ae_acamera_fsm_ptr_t p_fsm, isp_ae_preset_t * param );
void ae_calculate_target( ae_acamera_fsm_ptr_t p_fsm );
void ae_calculate_exposure( ae_acamera_fsm_ptr_t p_fsm );
void ae_roi_update( ae_acamera_fsm_ptr_t p_fsm );
void ae_process_stats( ae_acamera_fsm_ptr_t p_fsm );

struct _ae_acamera_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    uint32_t cur_using_stats_frame_id;

    int32_t exposure_log2;
    uint32_t exposure_ratio;
    uint32_t fullhist[ISP_FULL_HISTOGRAM_SIZE];
    uint32_t fullhist_sum;
    uint32_t ae_roi_api;
    uint32_t roi;
#if FW_ZONE_AE
    uint16_t hist4[ACAMERA_ISP_METERING_AEXP_NODES_USED_HORIZ_DEFAULT * ACAMERA_ISP_METERING_AEXP_NODES_USED_VERT_DEFAULT];
#endif

    void *lib_handle;

    ae_std_obj_t ae_alg_obj;
};


void ae_acamera_fsm_clear( ae_acamera_fsm_ptr_t p_fsm );

void ae_acamera_fsm_init( void *fsm, fsm_init_param_t *init_param );
void ae_acamera_fsm_deinit( void *fsm );
int ae_acamera_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size );
int ae_acamera_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );

uint8_t ae_acamera_fsm_process_event( ae_acamera_fsm_ptr_t p_fsm, event_id_t event_id );

void ae_acamera_fsm_process_interrupt( ae_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event );

void ae_acamera_request_interrupt( ae_acamera_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __AE_FSM_H__ */
