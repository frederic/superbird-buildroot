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

#if !defined( __COLOR_MATRIX_FSM_H__ )
#define __COLOR_MATRIX_FSM_H__



typedef struct _color_matrix_fsm_t color_matrix_fsm_t;
typedef struct _color_matrix_fsm_t *color_matrix_fsm_ptr_t;
typedef const struct _color_matrix_fsm_t *color_matrix_fsm_const_ptr_t;

#ifndef AWB_LIGHT_SOURCE_D50
#define AWB_LIGHT_SOURCE_D50 0x03
#endif
uint16_t color_matrix_complement_to_direct( int16_t v );
int16_t color_matrix_direct_to_complement( uint16_t v );
void color_matrix_change_CCMs( color_matrix_fsm_ptr_t p_fsm );
void color_matrix_recalculate( color_matrix_fsm_ptr_t p_fsm );
void color_matrix_initialize( color_matrix_fsm_ptr_t p_fsm );
void color_matrix_update( color_matrix_fsm_ptr_t p_fsm );
void color_matrix_write( color_matrix_fsm_ptr_t p_fsm );
void color_matrix_shading_mesh_reload( color_matrix_fsm_ptr_t p_fsm );


struct _color_matrix_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    uint8_t color_matrix_enabled;
    uint8_t manual_saturation_enabled;
    uint8_t saturation_target;
    uint16_t color_wb_matrix[9];
    int16_t color_matrix[9];
    int16_t color_correction_matrix[9];
    int16_t color_saturation_matrix[9];
    uint8_t light_source;
    uint8_t light_source_previous;
    uint8_t light_source_ccm;
    uint8_t light_source_ccm_previous;
    uint8_t light_source_change_frames;
    uint8_t light_source_change_frames_left;
    int16_t color_matrix_A[9];
    int16_t color_matrix_D40[9];
    int16_t color_matrix_D50[9];
    int16_t color_matrix_one[9];
    int16_t shading_alpha;
    uint8_t shading_direction;
    uint8_t shading_source_previous;
    uint8_t manual_CCM;
    int16_t manual_color_matrix[9];
    int32_t temperature_threshold[8];
};


void color_matrix_fsm_clear( color_matrix_fsm_ptr_t p_fsm );

void color_matrix_fsm_init( void *fsm, fsm_init_param_t *init_param );
int color_matrix_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size );
int color_matrix_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );

uint8_t color_matrix_fsm_process_event( color_matrix_fsm_ptr_t p_fsm, event_id_t event_id );

void color_matrix_fsm_process_interrupt( color_matrix_fsm_const_ptr_t p_fsm, uint8_t irq_event );

void color_matrix_request_interrupt( color_matrix_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __COLOR_MATRIX_FSM_H__ */
