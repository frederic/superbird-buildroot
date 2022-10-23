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
#include "gamma_acamera_fsm.h"
#include "acamera_3aalg_preset.h"

#if defined( ISP_HAS_SBUF_FSM ) || defined( ISP_HAS_USER2KERNEL_FSM )
#include "sbuf.h"
#endif

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_GAMMA_ACAMERA
#endif

void gamma_acamera_fsm_clear( gamma_acamera_fsm_t *p_fsm )
{
    p_fsm->cur_using_stats_frame_id = 0;
    p_fsm->gamma_gain = 256;
    p_fsm->gamma_offset = 0;

    p_fsm->lib_handle = NULL;

    memset( &p_fsm->gamma_alg_obj, 0, sizeof( p_fsm->gamma_alg_obj ) );
}

void gamma_contrast_request_interrupt( gamma_acamera_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask )
{
    acamera_isp_interrupts_disable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
    p_fsm->mask.irq_mask |= mask;
    acamera_isp_interrupts_enable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
}

void gamma_acamera_fsm_init( void *fsm, fsm_init_param_t *init_param )
{
    gamma_acamera_fsm_t *p_fsm = (gamma_acamera_fsm_t *)fsm;
    p_fsm->cmn.p_fsm_mgr = init_param->p_fsm_mgr;
    p_fsm->cmn.isp_base = init_param->isp_base;
    p_fsm->p_fsm_mgr = init_param->p_fsm_mgr;

    gamma_acamera_fsm_clear( p_fsm );

    gamma_acamera_init( p_fsm );
}

void gamma_acamera_fsm_deinit( void *fsm )
{
    gamma_acamera_fsm_ptr_t p_fsm = (gamma_acamera_fsm_ptr_t)fsm;
    gamma_acamera_deinit( p_fsm );
}

int gamma_acamera_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size )
{
    int rc = 0;

#if defined( ISP_HAS_SBUF_FSM ) || defined( ISP_HAS_USER2KERNEL_FSM )
    gamma_acamera_fsm_t *p_fsm = (gamma_acamera_fsm_t *)fsm;
#endif

    switch ( param_id ) {
#if defined( ISP_HAS_SBUF_FSM ) || defined( ISP_HAS_USER2KERNEL_FSM )
    case FSM_PARAM_SET_GAMMA_STATS: {
        if ( !input || input_size != sizeof( sbuf_gamma_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        sbuf_gamma_t *p_sbuf_gamma = (sbuf_gamma_t *)input;

        p_fsm->fullhist_sum = p_sbuf_gamma->fullhist_sum;
        system_memcpy( p_fsm->fullhist, p_sbuf_gamma->stats_data, sizeof( p_fsm->fullhist ) );
        fsm_raise_event( p_fsm, event_id_gamma_stats_ready );

        break;
    }
#endif
    case FSM_PARAM_SET_GAMMA_FRAMEID:

        gamma_set_frameid( p_fsm, *(int32_t *)input );

        break;

    case FSM_PARAM_SET_GAMMA_PRESET:
        if ( !input || input_size != sizeof( isp_gamma_preset_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        isp_gamma_preset_t *p_new = (isp_gamma_preset_t *)input;

        gamma_prev_param( p_fsm, p_new );

        break;

    default:
        rc = -1;
        break;
    }

    return rc;
}


int gamma_acamera_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size )
{
    int rc = 0;
    gamma_acamera_fsm_t *p_fsm = (gamma_acamera_fsm_t *)fsm;

    switch ( param_id ) {
    case FSM_PARAM_GET_GAMMA_CONTRAST_RESULT: {
        if ( !output || output_size != sizeof( fsm_param_gamma_result_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        fsm_param_gamma_result_t *p_result = (fsm_param_gamma_result_t *)output;

        p_result->gamma_gain = p_fsm->gamma_gain;
        p_result->gamma_offset = p_fsm->gamma_offset;

        break;
    }

    default:
        rc = -1;
        break;
    }

    return rc;
}


uint8_t gamma_acamera_fsm_process_event( gamma_acamera_fsm_t *p_fsm, event_id_t event_id )
{
    uint8_t b_event_processed = 0;
    switch ( event_id ) {
    default:
        break;
    case event_id_gamma_stats_ready:
        gamma_acamera_process_stats( p_fsm );
        gamma_contrast_request_interrupt( p_fsm, ACAMERA_IRQ_MASK( ACAMERA_IRQ_ANTIFOG_HIST ) );
        b_event_processed = 1;
        break;
    }

    return b_event_processed;
}
