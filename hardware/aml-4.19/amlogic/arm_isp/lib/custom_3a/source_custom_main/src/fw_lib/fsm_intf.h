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

#if !defined( __FSM_OPERATIONS_H__ )
#define __FSM_OPERATIONS_H__

typedef struct _fsm_init_param_t_ {
    void *p_fsm_mgr;
    uintptr_t isp_base;

} fsm_init_param_t;

typedef void ( *FUN_PTR_INIT )( void *fsm, fsm_init_param_t *init_param );
typedef void ( *FUN_PTR_DEINIT )( void *fsm );

typedef int ( *FUN_PTR_RUN )( void *fsm );
typedef int ( *FUN_PTR_SET_PARAM )( void *p_fsm, uint32_t param_id, void *input, uint32_t input_size );
typedef int ( *FUN_PTR_GET_PARAM )( void *p_fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );

typedef int ( *FUN_PTR_PROC_EVENT )( void *fsm, event_id_t event_id );
typedef void ( *FUN_PTR_PROC_INT )( void *fsm, uint8_t irq_event );


typedef struct _fsm_ops_t_ {

    FUN_PTR_INIT init;
    FUN_PTR_DEINIT deinit;

    FUN_PTR_RUN run;
    FUN_PTR_SET_PARAM set_param;
    FUN_PTR_GET_PARAM get_param;

    FUN_PTR_PROC_EVENT proc_event;
    FUN_PTR_PROC_INT proc_interrupt;
} fsm_ops_t;

typedef struct _fsm_common_t_ {
    void *p_fsm;

    void *p_fsm_mgr;
    uintptr_t isp_base;
    uint8_t ctx_id;

    fsm_ops_t ops;
} fsm_common_t;

typedef fsm_common_t *( *FUN_PTR_GET_FSM_COMMON )( uint8_t ctx_id );

#endif /* __FSM_OPERATIONS_H__ */
