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

#if !defined( __USER2KERNEL_FSM_H__ )
#define __USER2KERNEL_FSM_H__



typedef struct _user2kernel_fsm_t user2kernel_fsm_t;
typedef struct _user2kernel_fsm_t *user2kernel_fsm_ptr_t;
typedef const struct _user2kernel_fsm_t *user2kernel_fsm_const_ptr_t;

#define ISP_HAS_USER2KERNEL_FSM 1
int user2kernel_initialize( user2kernel_fsm_ptr_t p_fsm );
void user2kernel_deinit( user2kernel_fsm_ptr_t p_fsm );
void user2kernel_process( user2kernel_fsm_ptr_t p_fsm );
void user2kernel_reset( user2kernel_fsm_ptr_t p_fsm );
uint8_t user2kernel_get_calibrations( uint32_t ctx_id, void *sensor_arg, ACameraCalibrations *c );

struct _user2kernel_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    uint8_t is_paused;
};


void user2kernel_fsm_clear( user2kernel_fsm_ptr_t p_fsm );

void user2kernel_fsm_init( void *fsm, fsm_init_param_t *init_param );
int user2kernel_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );

uint8_t user2kernel_fsm_process_event( user2kernel_fsm_ptr_t p_fsm, event_id_t event_id );

void user2kernel_fsm_process_interrupt( user2kernel_fsm_const_ptr_t p_fsm, uint8_t irq_event );

void user2kernel_request_interrupt( user2kernel_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __USER2KERNEL_FSM_H__ */
