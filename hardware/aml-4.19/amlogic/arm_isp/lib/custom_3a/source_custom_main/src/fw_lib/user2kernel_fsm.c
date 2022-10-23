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

#include "acamera_fw.h"
#include "user2kernel_fsm.h"
#include "sbuf.h"
#include <stdlib.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_USER2KERNEL
#endif

extern int32_t user2kernel_get_total_gain_log2( int fw_id );
extern int32_t user2kernel_get_max_exposure_log2( int fw_id );
extern int32_t user2kernel_get_klens_status( int fw_id );
extern int32_t user2kernel_get_klens_param( int fw_id, lens_param_t *p_lens_param );
extern int32_t user2kernel_get_ksensor_info( int fw_id, struct sensor_info *p_sensor_info );

void user2kernel_fsm_clear( user2kernel_fsm_t *p_fsm )
{
    p_fsm->is_paused = 1;
}

void user2kernel_request_interrupt( user2kernel_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask )
{
    acamera_isp_interrupts_disable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
    p_fsm->mask.irq_mask |= mask;
    acamera_isp_interrupts_enable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
}


void user2kernel_fsm_init( void *fsm, fsm_init_param_t *init_param )
{
    int rc = 0;
    user2kernel_fsm_t *p_fsm = (user2kernel_fsm_t *)fsm;
    p_fsm->cmn.p_fsm_mgr = init_param->p_fsm_mgr;
    p_fsm->cmn.isp_base = init_param->isp_base;
    p_fsm->p_fsm_mgr = init_param->p_fsm_mgr;

    user2kernel_fsm_clear( p_fsm );

    rc = user2kernel_initialize( p_fsm );
    if ( rc ) {
        LOG( LOG_CRIT, "user2kernel_initialize failed, exit program, rc: %d.", rc );
        exit( rc );
    }

    user2kernel_reset( p_fsm );
}

int user2kernel_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size )
{
    int rc = 0;
    user2kernel_fsm_t *p_fsm = (user2kernel_fsm_t *)fsm;

    switch ( param_id ) {
    case FSM_PARAM_GET_TOTAL_GAIN_LOG2:
        if ( !output || output_size != sizeof( int32_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        *(int32_t *)output = user2kernel_get_total_gain_log2( p_fsm->cmn.ctx_id );

        break;

    case FSM_PARAM_GET_MAX_EXPOSURE_LOG2:
        if ( !output || output_size != sizeof( int32_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        *(int32_t *)output = user2kernel_get_max_exposure_log2( p_fsm->cmn.ctx_id );

        break;

    case FSM_PARAM_GET_KLENS_STATUS: {
        if ( !output || output_size != sizeof( int32_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        *(int32_t *)output = user2kernel_get_klens_status( p_fsm->cmn.ctx_id );

        break;
    }

    case FSM_PARAM_GET_KLENS_PARAM:
        if ( !output || output_size != sizeof( lens_param_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        rc = user2kernel_get_klens_param( p_fsm->cmn.ctx_id, (lens_param_t *)output );

        break;

    case FSM_PARAM_GET_KSENSOR_INFO:
        if ( !output || output_size != sizeof( struct sensor_info ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        rc = user2kernel_get_ksensor_info( p_fsm->cmn.ctx_id, (struct sensor_info *)output );

        break;

    default:
        rc = -1;
        break;
    }

    return rc;
}

uint8_t user2kernel_fsm_process_event( user2kernel_fsm_t *p_fsm, event_id_t event_id )
{
    uint8_t b_event_processed = 0;
    switch ( event_id ) {
    default:
        break;
    case event_id_sensor_not_ready:
        p_fsm->is_paused = 1;
        user2kernel_reset( p_fsm );
        b_event_processed = 1;
        break;

    case event_id_sensor_ready:
        p_fsm->is_paused = 0;
        b_event_processed = 1;
        break;

    case event_id_user_data_ready:
        user2kernel_process( p_fsm );
        b_event_processed = 1;
        break;
    }

    return b_event_processed;
}
