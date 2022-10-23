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
#include "acamera_math.h"
#include "acamera_logger.h"
#include "system_stdlib.h"
#include "acamera_ihist_stats_mem_config.h"
#include "acamera_command_api.h"
#include "gamma_acamera_fsm.h"
#include "gamma_acamera_core.h"

#include <dlfcn.h>

#include "acamera_fr_gamma_rgb_mem_config.h"

#if ISP_HAS_DS1
#include "acamera_ds1_gamma_rgb_mem_config.h"
#endif


#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_GAMMA_ACAMERA
#endif

void gamma_acamera_read_histogram( gamma_acamera_fsm_ptr_t p_fsm )
{
    int i;
    uint32_t sum = 0;

    fsm_param_mon_alg_flow_t gamma_flow;

    gamma_flow.frame_id_tracking = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
    p_fsm->cur_using_stats_frame_id = gamma_flow.frame_id_tracking;

    for ( i = 0; i < ISP_FULL_HISTOGRAM_SIZE; ++i ) {
        uint32_t v = acamera_ihist_stats_mem_array_data_read( p_fsm->cmn.isp_base, i );

        v <<= 8;

        p_fsm->fullhist[i] = v;
        sum += v;
    }
    p_fsm->fullhist_sum = sum;


    gamma_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
    gamma_flow.flow_state = MON_ALG_FLOW_STATE_INPUT_READY;
    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_GAMMA_FLOW, &gamma_flow, sizeof( gamma_flow ) );
    LOG( LOG_INFO, "Gamma flow: INPUT_READY: frame_id_tracking: %d, cur frame_id: %u.", gamma_flow.frame_id_tracking, gamma_flow.frame_id_current );
}

void gamma_acamera_update( gamma_acamera_fsm_ptr_t p_fsm )
{
    if ( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_auto_level == 0 ) {

        acamera_isp_fr_gamma_rgb_gain_r_write( p_fsm->cmn.isp_base, p_fsm->gamma_gain );
        acamera_isp_fr_gamma_rgb_gain_g_write( p_fsm->cmn.isp_base, p_fsm->gamma_gain );
        acamera_isp_fr_gamma_rgb_gain_b_write( p_fsm->cmn.isp_base, p_fsm->gamma_gain );

        acamera_isp_fr_gamma_rgb_offset_r_write( p_fsm->cmn.isp_base, p_fsm->gamma_offset );
        acamera_isp_fr_gamma_rgb_offset_g_write( p_fsm->cmn.isp_base, p_fsm->gamma_offset );
        acamera_isp_fr_gamma_rgb_offset_b_write( p_fsm->cmn.isp_base, p_fsm->gamma_offset );


#if ISP_HAS_DS1
        acamera_isp_ds1_gamma_rgb_gain_r_write( p_fsm->cmn.isp_base, p_fsm->gamma_gain );
        acamera_isp_ds1_gamma_rgb_gain_g_write( p_fsm->cmn.isp_base, p_fsm->gamma_gain );
        acamera_isp_ds1_gamma_rgb_gain_b_write( p_fsm->cmn.isp_base, p_fsm->gamma_gain );

        acamera_isp_ds1_gamma_rgb_offset_r_write( p_fsm->cmn.isp_base, p_fsm->gamma_offset );
        acamera_isp_ds1_gamma_rgb_offset_g_write( p_fsm->cmn.isp_base, p_fsm->gamma_offset );
        acamera_isp_ds1_gamma_rgb_offset_b_write( p_fsm->cmn.isp_base, p_fsm->gamma_offset );
#endif
    }

    // skip when frame_id is 0.
    if ( p_fsm->cur_using_stats_frame_id ) {
        fsm_param_mon_alg_flow_t gamma_flow;

        gamma_flow.frame_id_tracking = p_fsm->cur_using_stats_frame_id;
        gamma_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
        gamma_flow.flow_state = MON_ALG_FLOW_STATE_APPLIED;
        acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_GAMMA_FLOW, &gamma_flow, sizeof( gamma_flow ) );
        LOG( LOG_INFO, "GAMMA flow: APPLIED: frame_id_tracking: %d, cur frame_id: %u.", gamma_flow.frame_id_tracking, gamma_flow.frame_id_current );
    }
}

static void gamma_prepare_input( gamma_acamera_fsm_ptr_t p_fsm, gamma_custom_input_t *custom_input, gamma_acamera_input_t *acamera_input )
{
    system_memset( custom_input, 0, sizeof( *custom_input ) );
    system_memset( acamera_input, 0, sizeof( *acamera_input ) );

    // acamera_input
    acamera_input->cali_data.auto_level_ctrl = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AUTO_LEVEL_CONTROL );
}

void gamma_acamera_process_stats( gamma_acamera_fsm_ptr_t p_fsm )
{
    if ( !p_fsm->gamma_alg_obj.proc ) {
        LOG( LOG_ERR, "Gamma: can't process stats since function is NULL." );
        return;
    }

    gamma_stats_data_t stats;
    stats.fullhist = p_fsm->fullhist;
    stats.fullhist_size = array_size( p_fsm->fullhist );

    gamma_acamera_input_t acamera_gamma_input;
    gamma_custom_input_t custom_gamma_input;
    gamma_input_data_t gamma_input;

    gamma_prepare_input( p_fsm, &custom_gamma_input, &acamera_gamma_input );
    gamma_input.custom_input = &custom_gamma_input;
    gamma_input.acamera_input = &acamera_gamma_input;

    gamma_output_data_t gamma_output;
    gamma_acamera_output_t acamera_gamma_output;
    system_memset( &acamera_gamma_output, 0, sizeof( acamera_gamma_output ) );
    gamma_output.acamera_output = &acamera_gamma_output;
    gamma_output.custom_output = NULL;

    // call the core algorithm to calculate target exposure and exposure_ratio
    if ( p_fsm->gamma_alg_obj.proc( p_fsm->gamma_alg_obj.gamma_ctx, &stats, &gamma_input, &gamma_output ) != 0 ) {
        LOG( LOG_ERR, "Gamma algorithm process failed." );
        return;
    }

    p_fsm->gamma_gain = acamera_gamma_output.gamma_gain;
    p_fsm->gamma_offset = acamera_gamma_output.gamma_offset;

    if ( p_fsm->cur_using_stats_frame_id ) {
        fsm_param_mon_alg_flow_t gamma_flow;

        gamma_flow.frame_id_tracking = p_fsm->cur_using_stats_frame_id;
        gamma_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
        gamma_flow.flow_state = MON_ALG_FLOW_STATE_OUTPUT_READY;
        acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_GAMMA_FLOW, &gamma_flow, sizeof( gamma_flow ) );
        LOG( LOG_INFO, "GAMMA flow: OUTPUT_READY: frame_id_tracking: %d, cur frame_id: %u.", gamma_flow.frame_id_tracking, gamma_flow.frame_id_current );
    }

    gamma_acamera_update( p_fsm );
}

void gamma_acamera_init( gamma_acamera_fsm_ptr_t p_fsm )
{
    gamma_contrast_request_interrupt( p_fsm, ACAMERA_IRQ_MASK( ACAMERA_IRQ_ANTIFOG_HIST ) );

    p_fsm->lib_handle = dlopen( "libacamera_gamma_alg.so", RTLD_LAZY );
    LOG( LOG_INFO, "Gamma: try to open custom_alg library, return: %p.", p_fsm->lib_handle );
    if ( !p_fsm->lib_handle ) {
        p_fsm->lib_handle = dlopen( "./libacamera_alg_core.so", RTLD_LAZY );
        LOG( LOG_INFO, "Gamma: try to open acamera_alg_core library, return: %p.", p_fsm->lib_handle );
    }

    if ( p_fsm->lib_handle ) {
        /* init, deinit and proc are required in algorithm lib */
        /* frameid and prevparam are optional in algorithm lib */
        p_fsm->gamma_alg_obj.init = dlsym( p_fsm->lib_handle, "gamma_acamera_core_init" );
        p_fsm->gamma_alg_obj.deinit = dlsym( p_fsm->lib_handle, "gamma_acamera_core_deinit" );
        p_fsm->gamma_alg_obj.proc = dlsym( p_fsm->lib_handle, "gamma_acamera_core_proc" );
	p_fsm->gamma_alg_obj.frameid = dlsym( p_fsm->lib_handle, "gamma_acamera_core_frame_id" );
	p_fsm->gamma_alg_obj.prevparam = dlsym( p_fsm->lib_handle, "gamma_acamera_core_prev_param" );
        LOG( LOG_INFO, "Gamma: init: %p, deinit: %p, proc: %p, frameid: %p, prevparam: %p",
                    p_fsm->gamma_alg_obj.init, p_fsm->gamma_alg_obj.deinit,
                    p_fsm->gamma_alg_obj.proc, p_fsm->gamma_alg_obj.frameid,
                    p_fsm->gamma_alg_obj.prevparam);
    } else {
        LOG( LOG_ERR, "Load shared lib failed, error: %s.", dlerror() );
    }

    if ( p_fsm->gamma_alg_obj.init ) {
        p_fsm->gamma_alg_obj.gamma_ctx = p_fsm->gamma_alg_obj.init( p_fsm->cmn.ctx_id );
        LOG( LOG_INFO, "Init Gamma core, ret: %p.", p_fsm->gamma_alg_obj.gamma_ctx );
    }
}

void gamma_acamera_fsm_process_interrupt( gamma_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event )
{
    if ( acamera_fsm_util_is_irq_event_ignored( (fsm_irq_mask_t *)( &p_fsm->mask ), irq_event ) )
        return;

    if ( irq_event == ACAMERA_IRQ_ANTIFOG_HIST ) {
        if ( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_auto_level == 0 ) {
            gamma_acamera_read_histogram( (gamma_acamera_fsm_ptr_t)p_fsm );
            fsm_raise_event( p_fsm, event_id_gamma_stats_ready );
        }
    }
    return;
}

void gamma_acamera_deinit( gamma_acamera_fsm_ptr_t p_fsm )
{
    if ( p_fsm->gamma_alg_obj.deinit ) {
        p_fsm->gamma_alg_obj.deinit( p_fsm->gamma_alg_obj.gamma_ctx );
    }

    if ( p_fsm->lib_handle ) {
        dlclose( p_fsm->lib_handle );
    }
}

void gamma_set_frameid( gamma_acamera_fsm_ptr_t p_fsm, uint32_t frame_id )
{
	if ( p_fsm->gamma_alg_obj.frameid ) {
        p_fsm->gamma_alg_obj.frameid( p_fsm->gamma_alg_obj.gamma_ctx, frame_id );
    }
}

void gamma_prev_param( gamma_acamera_fsm_ptr_t p_fsm, isp_gamma_preset_t * param )
{
	if ( p_fsm->gamma_alg_obj.prevparam ) {
		p_fsm->gamma_alg_obj.prevparam( p_fsm->gamma_alg_obj.gamma_ctx, param );
	}
}