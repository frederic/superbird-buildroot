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

#if !defined( __IRIDIX_FSM_H__ )
#define __IRIDIX_FSM_H__

#include "iridix_standard_api.h"

typedef struct _iridix_acamera_fsm_t iridix_acamera_fsm_t;
typedef struct _iridix_acamera_fsm_t *iridix_acamera_fsm_ptr_t;
typedef const struct _iridix_acamera_fsm_t *iridix_acamera_fsm_const_ptr_t;

void iridix_initialize( iridix_acamera_fsm_ptr_t p_fsm );
void iridix_deinitialize( iridix_acamera_fsm_ptr_t p_fsm );
void iridix_init_cali_lut( iridix_acamera_fsm_ptr_t p_fsm );
void iridix_calculate( iridix_acamera_fsm_ptr_t p_fsm );
void iridix_set_frameid( iridix_acamera_fsm_ptr_t p_fsm, uint32_t frame_id );
void iridix_prev_param( iridix_acamera_fsm_ptr_t p_fsm, isp_iridix_preset_t * param );

struct _iridix_acamera_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    uint16_t strength_target;
    uint32_t strength_avg;
    uint16_t dark_enh;

    uint32_t iridix_contrast;
    uint16_t iridix_global_DG;
    uint16_t iridix_global_DG_prev;

    uint32_t frame_id_tracking;

    void *lib_handle;

    iridix_std_obj_t iridix_alg_obj;
};


void iridix_acamera_fsm_clear( iridix_acamera_fsm_ptr_t p_fsm );
void iridix_acamera_fsm_init( void *fsm, fsm_init_param_t *init_param );
void iridix_acamera_fsm_deinit( void *fsm );
int iridix_acamera_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size );
int iridix_acamera_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );
uint8_t iridix_acamera_fsm_process_event( iridix_acamera_fsm_ptr_t p_fsm, event_id_t event_id );
void iridix_acamera_fsm_process_interrupt( iridix_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event );
void iridix_acamera_request_interrupt( iridix_acamera_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __IRIDIX_FSM_H__ */
