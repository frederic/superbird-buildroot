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
#include "iridix_acamera_fsm.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_IRIDIX_ACAMERA
#endif

void iridix_acamera_fsm_clear( iridix_acamera_fsm_t *p_fsm )
{
    p_fsm->strength_target = IRIDIX_STRENGTH_TARGET;
    p_fsm->strength_avg = IRIDIX_STRENGTH_TARGET * CALIBRATION_IRIDIX_AVG_COEF_INIT;
    p_fsm->dark_enh = 15000;

    p_fsm->iridix_contrast = 256;
    p_fsm->iridix_global_DG = 256;
    p_fsm->iridix_global_DG_prev = 256;

    p_fsm->frame_id_tracking = 0;

    p_fsm->lib_handle = NULL;

    memset( &p_fsm->iridix_alg_obj, 0, sizeof( p_fsm->iridix_alg_obj ) );
}

void iridix_acamera_request_interrupt( iridix_acamera_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask )
{
    acamera_isp_interrupts_disable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
    p_fsm->mask.irq_mask |= mask;
    acamera_isp_interrupts_enable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
}

void iridix_acamera_fsm_init( void *fsm, fsm_init_param_t *init_param )
{
    iridix_acamera_fsm_t *p_fsm = (iridix_acamera_fsm_t *)fsm;
    p_fsm->cmn.p_fsm_mgr = init_param->p_fsm_mgr;
    p_fsm->cmn.isp_base = init_param->isp_base;
    p_fsm->p_fsm_mgr = init_param->p_fsm_mgr;

    iridix_acamera_fsm_clear( p_fsm );

    iridix_initialize( p_fsm );
}

void iridix_acamera_fsm_deinit( void *fsm )
{
    iridix_acamera_fsm_t *p_fsm = (iridix_acamera_fsm_t *)fsm;

    iridix_deinitialize( p_fsm );
}

int iridix_acamera_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size )
{
    int rc = 0;
    iridix_acamera_fsm_t *p_fsm = (iridix_acamera_fsm_t *)fsm;

    switch ( param_id ) {
    case FSM_PARAM_SET_IRIDIX_INIT: {
        iridix_init_cali_lut( p_fsm );
        break;
    }
    case FSM_PARAM_SET_IRIDIX_FRAMEID:

        iridix_set_frameid( p_fsm, *(int32_t *)input );

        break;

    case FSM_PARAM_SET_IRIDIX_PRESET:
        if ( !input || input_size != sizeof( isp_iridix_preset_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        isp_iridix_preset_t *p_new = (isp_iridix_preset_t *)input;

        iridix_prev_param( p_fsm, p_new );

        break;

    default:
        rc = -1;
        break;
    }

    return rc;
}


int iridix_acamera_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size )
{
    int rc = 0;
    iridix_acamera_fsm_t *p_fsm = (iridix_acamera_fsm_t *)fsm;

    switch ( param_id ) {
    case FSM_PARAM_GET_IRIDIX_STRENGTH: {
        if ( !output || output_size != sizeof( uint16_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        *(uint16_t *)output = p_fsm->strength_target;

        break;
    }

    case FSM_PARAM_GET_IRIDIX_CONTRAST: {
        if ( !output || output_size != sizeof( uint32_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        *(uint32_t *)output = p_fsm->iridix_contrast;

        break;
    }

    case FSM_PARAM_GET_IRIDIX_DARK_ENH: {
        if ( !output || output_size != sizeof( uint16_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        *(uint16_t *)output = p_fsm->dark_enh;

        break;
    }

    case FSM_PARAM_GET_IRIDIX_GLOBAL_DG: {
        if ( !output || output_size != sizeof( uint16_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        *(uint16_t *)output = p_fsm->iridix_global_DG;

        break;
    }
    default:
        rc = -1;
        break;
    }

    return rc;
}


uint8_t iridix_acamera_fsm_process_event( iridix_acamera_fsm_t *p_fsm, event_id_t event_id )
{
    uint8_t b_event_processed = 0;
    switch ( event_id ) {
    default:
        break;
    case event_id_update_iridix:
        iridix_calculate( p_fsm );
        b_event_processed = 1;
        break;
    }

    return b_event_processed;
}
