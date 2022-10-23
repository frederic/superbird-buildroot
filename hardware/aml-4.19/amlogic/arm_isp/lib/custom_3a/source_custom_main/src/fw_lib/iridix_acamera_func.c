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
#include "iridix_acamera_core.h"

#include <dlfcn.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_IRIDIX_ACAMERA
#endif


static void iridix_mointor_frame_end( iridix_acamera_fsm_ptr_t p_fsm )
{

    uint32_t irq_mask = acamera_isp_isp_global_interrupt_status_vector_read( p_fsm->cmn.isp_base );
    if ( irq_mask & 0x1 ) {
        fsm_param_mon_err_head_t mon_err;
        mon_err.err_type = MON_TYPE_ERR_IRIDIX_UPDATE_NOT_IN_VB;
        acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_ERROR_REPORT, &mon_err, sizeof( mon_err ) );
    }
}

void iridix_acamera_fsm_process_interrupt( iridix_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event )
{
    if ( acamera_fsm_util_is_irq_event_ignored( (fsm_irq_mask_t *)( &p_fsm->mask ), irq_event ) )
        return;

    // Get and process interrupts
    if ( irq_event == ACAMERA_IRQ_FRAME_END ) {

        int32_t diff = 256;
        uint8_t iridix_avg_coeff = _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_AVG_COEF )[0];

        int32_t diff_iridix_DG = 256;
        uint16_t iridix_global_DG = p_fsm->iridix_global_DG;

        if ( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_iridix == 0 ) {
            acamera_isp_iridix_gain_gain_write( p_fsm->cmn.isp_base, iridix_global_DG );
        }

        diff_iridix_DG = ( ( (int32_t)iridix_global_DG ) << 8 ) / ( int32_t )( p_fsm->iridix_global_DG_prev );
        ( (iridix_acamera_fsm_ptr_t)p_fsm )->iridix_global_DG_prev = iridix_global_DG; // already applyied gain


        iridix_avg_coeff = _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_AVG_COEF )[0];

        exposure_set_t exp_set_fr_next;
        exposure_set_t exp_set_fr_prev;
        int32_t frame_next = 1, frame_prev = 2;
// int32_t frame_next = 1, frame_prev = 0;
#if ISP_HAS_TEMPER == 3
        if ( !acamera_isp_temper_temper2_mode_read( p_fsm->cmn.isp_base ) ) { // temper 3 has 1 frame delay
            frame_next = 0;
            frame_prev = -1;
        }
#endif

        acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_FRAME_EXPOSURE_SET, &frame_next, sizeof( frame_next ), &exp_set_fr_next, sizeof( exp_set_fr_next ) );
        acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_FRAME_EXPOSURE_SET, &frame_prev, sizeof( frame_prev ), &exp_set_fr_prev, sizeof( exp_set_fr_prev ) );

        diff = acamera_math_exp2( exp_set_fr_prev.info.exposure_log2 - exp_set_fr_next.info.exposure_log2, LOG2_GAIN_SHIFT, 8 );
        // LOG(LOG_DEBUG,"diff1 %d %d ", (int)diff,(int)diff_iridix_DG);
        diff = ( diff * diff_iridix_DG ) >> 8;

        // LOG(LOG_DEBUG,"diff2 %d\n", (int)diff);
        if ( diff < 0 )
            diff = 256;
        if ( diff >= ( 1 << ACAMERA_ISP_IRIDIX_COLLECTION_CORRECTION_DATASIZE ) )
            diff = ( 1 << ACAMERA_ISP_IRIDIX_COLLECTION_CORRECTION_DATASIZE );
        //LOG(LOG_DEBUG,"%u %ld prev %ld\n", (unsigned int)diff, exp_set_fr1.info.exposure_log2, exp_set_fr0.info.exposure_log2);
        // LOG(LOG_DEBUG,"diff3 %d\n", (int)diff);

        acamera_isp_iridix_collection_correction_write( p_fsm->cmn.isp_base, diff );

        diff = 256; // this logic diff = 256 where strength is only updated creates a long delay.
        if ( diff == 256 ) {
            // time filter for iridix strength
            uint16_t iridix_strength = p_fsm->strength_target;
            if ( iridix_avg_coeff > 1 ) {
                ( (iridix_acamera_fsm_ptr_t)p_fsm )->strength_avg += p_fsm->strength_target - p_fsm->strength_avg / iridix_avg_coeff; // division by zero is checked
                iridix_strength = ( uint16_t )( p_fsm->strength_avg / iridix_avg_coeff );                                             // division by zero is checked
            }
            if ( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_iridix == 0 ) {
                acamera_isp_iridix_strength_inroi_write( p_fsm->cmn.isp_base, iridix_strength >> 6 );
            }

            if ( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_iridix == 0 ) {
                acamera_isp_iridix_dark_enh_write( p_fsm->cmn.isp_base, p_fsm->dark_enh );
            }
        }

        if ( p_fsm->frame_id_tracking ) {
            fsm_param_mon_alg_flow_t iridix_flow;

            iridix_flow.frame_id_tracking = p_fsm->frame_id_tracking;
            iridix_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &( (iridix_acamera_fsm_t *)p_fsm )->cmn );
            iridix_flow.flow_state = MON_ALG_FLOW_STATE_APPLIED;
            acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_IRIDIX_FLOW, &iridix_flow, sizeof( iridix_flow ) );
            LOG( LOG_INFO, "Iridix8 flow: APPLIED: frame_id_tracking: %d, cur frame_id: %u.", iridix_flow.frame_id_tracking, iridix_flow.frame_id_current );
            ( (iridix_acamera_fsm_ptr_t)p_fsm )->frame_id_tracking = 0;
        }

        iridix_mointor_frame_end( (iridix_acamera_fsm_ptr_t)p_fsm );
    }
}

void iridix_init_cali_lut( iridix_acamera_fsm_ptr_t p_fsm )
{
    uint16_t i;
    //put initialization here
    p_fsm->strength_avg = IRIDIX_STRENGTH_TARGET * _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_AVG_COEF )[0];

    // Initialize parameters
    if ( _GET_WIDTH( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_ASYMMETRY ) == sizeof( int32_t ) ) {
        // 32 bit tables
        for ( i = 0; i < _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_ASYMMETRY ); i++ ) {
            uint32_t val = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_ASYMMETRY )[i];
            acamera_isp_iridix_lut_asymmetry_lut_write( p_fsm->cmn.isp_base, i, val );
        }
    } else {
        // 16 bit tables
        for ( i = 0; i < _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_ASYMMETRY ); i++ ) {
            acamera_isp_iridix_lut_asymmetry_lut_write( p_fsm->cmn.isp_base, i, _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_ASYMMETRY )[i] );
        }
    }
}

void iridix_initialize( iridix_acamera_fsm_ptr_t p_fsm )
{
    p_fsm->lib_handle = dlopen( "libacamera_iridix_alg.so", RTLD_LAZY );
    LOG( LOG_INFO, "Iridix: try to open custom_alg, return: %p.", p_fsm->lib_handle );
    if ( !p_fsm->lib_handle ) {
        p_fsm->lib_handle = dlopen( "./libacamera_alg_core.so", RTLD_LAZY );
        LOG( LOG_INFO, "Iridix: try to open acamera_alg_core, return: %p.", p_fsm->lib_handle );
    }

    if ( p_fsm->lib_handle ) {
        /* init, deinit and proc are required in algorithm lib */
        /* frameid and prevparam are optional in algorithm lib */
        p_fsm->iridix_alg_obj.init = dlsym( p_fsm->lib_handle, "iridix_acamera_core_init" );
        p_fsm->iridix_alg_obj.deinit = dlsym( p_fsm->lib_handle, "iridix_acamera_core_deinit" );
        p_fsm->iridix_alg_obj.proc = dlsym( p_fsm->lib_handle, "iridix_acamera_core_proc" );
        p_fsm->iridix_alg_obj.frameid = dlsym( p_fsm->lib_handle, "iridix_acamera_core_frame_id" );
        p_fsm->iridix_alg_obj.prevparam = dlsym( p_fsm->lib_handle, "iridix_acamera_core_prev_param" );
        LOG( LOG_INFO, "Iridix: init: %p, deinit: %p, proc: %p, frameid: %p, prevparam: %p",
                    p_fsm->iridix_alg_obj.init, p_fsm->iridix_alg_obj.deinit,
                    p_fsm->iridix_alg_obj.proc,  p_fsm->iridix_alg_obj.frameid,
                    p_fsm->iridix_alg_obj.prevparam);
    }

    if ( p_fsm->iridix_alg_obj.init ) {
        p_fsm->iridix_alg_obj.iridix_ctx = p_fsm->iridix_alg_obj.init( p_fsm->cmn.ctx_id );
        LOG( LOG_INFO, "Init Iridix core, ret: %p.", p_fsm->iridix_alg_obj.iridix_ctx );
    }

    ACAMERA_FSM2CTX_PTR( p_fsm )
        ->stab.global_iridix_strength_target = ( p_fsm->strength_target );
    ACAMERA_FSM2CTX_PTR( p_fsm )
        ->stab.global_maximum_iridix_strength = ( _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_STRENGTH_MAXIMUM )[0] );

    iridix_init_cali_lut( p_fsm );

    p_fsm->mask.repeat_irq_mask = ACAMERA_IRQ_MASK( ACAMERA_IRQ_FRAME_END );
    iridix_acamera_request_interrupt( p_fsm, p_fsm->mask.repeat_irq_mask );
}

void iridix_deinitialize( iridix_acamera_fsm_ptr_t p_fsm )
{
    if ( p_fsm->iridix_alg_obj.deinit ) {
        p_fsm->iridix_alg_obj.deinit( p_fsm->iridix_alg_obj.iridix_ctx );
    }

    if ( p_fsm->lib_handle ) {
        dlclose( p_fsm->lib_handle );
    }
}

static void iridix_prepare_input( iridix_acamera_fsm_ptr_t p_fsm, iridix_custom_input_t *custom_input, iridix_acamera_input_t *acamera_input )
{
    system_memset( custom_input, 0, sizeof( *custom_input ) );
    system_memset( acamera_input, 0, sizeof( *acamera_input ) );

    // prepare acamera input data
    int32_t cur_exposure_log2 = 0;
    int32_t type = CMOS_CURRENT_EXPOSURE_LOG2;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CMOS_EXPOSURE_LOG2, &type, sizeof( type ), &cur_exposure_log2, sizeof( cur_exposure_log2 ) );
    acamera_input->misc_info.cur_exposure_log2 = cur_exposure_log2;

    // update max strength from calibration data
    ACAMERA_FSM2CTX_PTR( p_fsm )
        ->stab.global_maximum_iridix_strength = ( _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_STRENGTH_MAXIMUM )[0] );

    acamera_input->misc_info.global_manual_iridix = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_iridix;
    acamera_input->misc_info.global_iridix_strength_target = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_iridix_strength_target;
    acamera_input->misc_info.global_minimum_iridix_strength = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_minimum_iridix_strength;
    acamera_input->misc_info.global_maximum_iridix_strength = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_maximum_iridix_strength;

    acamera_input->cali_data.cali_ae_ctrl = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_CONTROL );
    acamera_input->cali_data.cali_iridix_avg_coef = _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_AVG_COEF );
    acamera_input->cali_data.cali_iridix_strength_dk_enh_ctrl = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX8_STRENGTH_DK_ENH_CONTROL );
    acamera_input->cali_data.cali_iridix_ev_lim_no_str = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_EV_LIM_NO_STR );
    acamera_input->cali_data.cali_iridix_ev_lim_full_str = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_EV_LIM_FULL_STR );
    acamera_input->cali_data.cali_iridix_min_max_str = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_IRIDIX_MIN_MAX_STR );
}

void iridix_prepare_stats( iridix_acamera_fsm_t *p_fsm, iridix_stats_data_t *p_stats )
{
    fsm_param_ae_hist_info_t ae_hist_info;

    // init to NULL in case no AE configured and caused wrong access via wild pointer
    ae_hist_info.fullhist = NULL;

    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_AE_HIST_INFO, NULL, 0, &ae_hist_info, sizeof( ae_hist_info ) );

    if ( ae_hist_info.frame_id ) {
        fsm_param_mon_alg_flow_t iridix_flow;

        iridix_flow.frame_id_tracking = ae_hist_info.frame_id;
        iridix_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
        iridix_flow.flow_state = MON_ALG_FLOW_STATE_INPUT_READY;

        // check whether previous frame_id_tracking finished.
        if ( p_fsm->frame_id_tracking ) {
            LOG( LOG_INFO, "Iridix flow: Overwrite: prev frame_id_tracking: %d, new: %u, cur: %d.", p_fsm->frame_id_tracking, iridix_flow.frame_id_tracking, iridix_flow.frame_id_current );
        }

        p_fsm->frame_id_tracking = iridix_flow.frame_id_tracking;
        acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_IRIDIX_FLOW, &iridix_flow, sizeof( iridix_flow ) );
        LOG( LOG_INFO, "Iridix flow: INPUT_READY: frame_id_tracking: %d, cur frame_id: %u.", iridix_flow.frame_id_tracking, iridix_flow.frame_id_current );
    }

    p_stats->fullhist = ae_hist_info.fullhist;
    p_stats->fullhist_size = ae_hist_info.fullhist_size;
    p_stats->fullhist_sum = ae_hist_info.fullhist_sum;
}

void iridix_calculate( iridix_acamera_fsm_t *p_fsm )
{
    if ( !p_fsm->iridix_alg_obj.proc ) {
        LOG( LOG_ERR, "Iridix: can't calculate since proc func is NULL." );
        return;
    }

    // Input
    iridix_acamera_input_t acamera_iridix_input;
    iridix_custom_input_t custom_iridix_input;
    iridix_input_data_t iridix_input;

    iridix_prepare_input( p_fsm, &custom_iridix_input, &acamera_iridix_input );
    iridix_input.custom_input = &custom_iridix_input;
    iridix_input.acamera_input = &acamera_iridix_input;

    // Output
    iridix_output_data_t iridix_output;
    iridix_acamera_output_t acamera_iridix_output;
    iridix_output.acamera_output = &acamera_iridix_output;
    iridix_output.custom_output = NULL;

    // statistics data
    iridix_stats_data_t stats;
    iridix_prepare_stats( p_fsm, &stats );

    // call the core algorithm to calculate target exposure and exposure_ratio
    if ( p_fsm->iridix_alg_obj.proc( p_fsm->iridix_alg_obj.iridix_ctx, &stats, &iridix_input, &iridix_output ) != 0 ) {
        LOG( LOG_ERR, "Iridix algorithm process failed." );
        return;
    }

    p_fsm->strength_target = acamera_iridix_output.strength_target;
    p_fsm->dark_enh = acamera_iridix_output.dark_enh;
    p_fsm->iridix_global_DG = acamera_iridix_output.iridix_global_DG;
    p_fsm->iridix_contrast = acamera_iridix_output.iridix_contrast;

    ACAMERA_FSM2CTX_PTR( p_fsm )
        ->stab.global_iridix_strength_target = p_fsm->strength_target >> 8;

    if ( p_fsm->frame_id_tracking ) {
        fsm_param_mon_alg_flow_t iridix_flow;

        iridix_flow.frame_id_tracking = p_fsm->frame_id_tracking;
        iridix_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
        iridix_flow.flow_state = MON_ALG_FLOW_STATE_OUTPUT_READY;
        acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_IRIDIX_FLOW, &iridix_flow, sizeof( iridix_flow ) );
        LOG( LOG_INFO, "Iridix flow: OUTPUT_READY: frame_id_tracking: %d, cur frame_id: %u.", iridix_flow.frame_id_tracking, iridix_flow.frame_id_current );
    }

    LOG( LOG_INFO, "Out: strength_target: %d, dark_enh: %d, iridix_global_DG: %d, iridix_contrast: %d.", p_fsm->strength_target, p_fsm->dark_enh, p_fsm->iridix_global_DG, p_fsm->iridix_contrast );
}

void iridix_set_frameid( iridix_acamera_fsm_ptr_t p_fsm, uint32_t frame_id )
{
	if ( p_fsm->iridix_alg_obj.frameid ) {
        p_fsm->iridix_alg_obj.frameid( p_fsm->iridix_alg_obj.iridix_ctx, frame_id );
    }
}

void iridix_prev_param( iridix_acamera_fsm_ptr_t p_fsm, isp_iridix_preset_t * param )
{
	if ( p_fsm->iridix_alg_obj.prevparam ) {
		p_fsm->iridix_alg_obj.prevparam( p_fsm->iridix_alg_obj.iridix_ctx, param );
	}
}

