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

#include "acamera_firmware_api.h"
#include "acamera_fw.h"
#include "acamera_math.h"

#include "acamera_aexp_hist_stats_mem_config.h"
#include "acamera_metering_stats_mem_config.h"

#include "acamera_math.h"

#include "ae_acamera_fsm.h"
#include "cmos_fsm.h"
#include "ae_standard_api.h"

#include "ae_custom_core.h"
#include "ae_acamera_core.h"

#include <dlfcn.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_AE_ACAMERA
#endif

void ae_roi_update( ae_acamera_fsm_ptr_t p_fsm )
{
    uint16_t horz_zones = acamera_isp_metering_hist_aexp_nodes_used_horiz_read( p_fsm->cmn.isp_base );
    uint16_t vert_zones = acamera_isp_metering_hist_aexp_nodes_used_vert_read( p_fsm->cmn.isp_base );
    uint16_t x, y;

    uint16_t *ptr_ae_zone_whgh_h = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_ZONE_WGHT_HOR );
    uint16_t *ptr_ae_zone_whgh_v = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_ZONE_WGHT_VER );

    uint8_t x_start = ( uint8_t )( ( ( ( p_fsm->roi >> 24 ) & 0xFF ) * horz_zones + 128 ) >> 8 );
    uint8_t x_end = ( uint8_t )( ( ( ( p_fsm->roi >> 8 ) & 0xFF ) * horz_zones + 128 ) >> 8 );
    uint8_t y_start = ( uint8_t )( ( ( ( p_fsm->roi >> 16 ) & 0xFF ) * vert_zones + 128 ) >> 8 );
    uint8_t y_end = ( uint8_t )( ( ( ( p_fsm->roi >> 0 ) & 0xFF ) * vert_zones + 128 ) >> 8 );

    uint8_t zone_size_x = x_end - x_start;
    uint8_t zone_size_y = y_end - y_start;
    uint32_t middle_x = zone_size_x * 256 / 2;
    uint32_t middle_y = zone_size_y * 256 / 2;
    uint16_t scale_x = 0;
    uint16_t scale_y = 0;
    uint32_t ae_zone_wght_hor_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_ZONE_WGHT_HOR );
    uint32_t ae_zone_wght_ver_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_ZONE_WGHT_VER );

    if ( ae_zone_wght_hor_len ) {
        scale_x = ( horz_zones - 1 ) / ae_zone_wght_hor_len + 1;
    } else {
        LOG( LOG_ERR, "ae_zone_wght_hor_len is zero" );
        return;
    }
    if ( ae_zone_wght_ver_len ) {
        scale_y = ( vert_zones - 1 ) / ae_zone_wght_ver_len + 1;
    } else {
        LOG( LOG_ERR, "ae_zone_wght_ver_len is zero" );
        return;
    }

    uint16_t gaus_center_x = ( _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_ZONE_WGHT_HOR ) * 256 / 2 ) * scale_x;
    uint16_t gaus_center_y = ( _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_ZONE_WGHT_VER ) * 256 / 2 ) * scale_y;

    for ( y = 0; y < vert_zones; y++ ) {
        uint8_t ae_coeff = 0;
        for ( x = 0; x < horz_zones; x++ ) {
            if ( y >= y_start && y <= y_end &&
                 x >= x_start && x <= x_end ) {

                uint8_t index_y = ( y - y_start );
                uint8_t index_x = ( x - x_start );
                int32_t distance_x = ( index_x * 256 + 128 ) - middle_x;
                int32_t distance_y = ( index_y * 256 + 128 ) - middle_y;
                uint32_t coeff_x;
                uint32_t coeff_y;

                if ( ( x == x_end && x_start != x_end ) ||
                     ( y == y_end && y_start != y_end ) ) {
                    ae_coeff = 0;
                } else {
                    coeff_x = ( gaus_center_x + distance_x ) / 256;
                    if ( distance_x > 0 && ( distance_x & 0x80 ) )
                        coeff_x--;
                    coeff_y = ( gaus_center_y + distance_y ) / 256;
                    if ( distance_y > 0 && ( distance_y & 0x80 ) )
                        coeff_y--;

                    coeff_x = ptr_ae_zone_whgh_h[coeff_x / scale_x];
                    coeff_y = ptr_ae_zone_whgh_v[coeff_y / scale_y];

                    ae_coeff = ( coeff_x * coeff_y ) >> 4;
                    if ( ae_coeff > 1 )
                        ae_coeff--;
                }
            } else {
                ae_coeff = 0;
            }
            acamera_isp_metering_hist_aexp_zones_weight_write( p_fsm->cmn.isp_base, ISP_METERING_ZONES_MAX_H * y + x, ae_coeff );
        }
    }
}


void ae_acamera_read_full_histogram_data( ae_acamera_fsm_ptr_t p_fsm )
{
    int i;
    int shift = 0;
    uint32_t sum = 0;
    fsm_param_mon_alg_flow_t ae_flow;

    ae_flow.frame_id_tracking = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
    p_fsm->cur_using_stats_frame_id = ae_flow.frame_id_tracking;
    for ( i = 0; i < ISP_FULL_HISTOGRAM_SIZE; ++i ) {
        uint32_t v = acamera_aexp_hist_stats_mem_array_data_read( p_fsm->cmn.isp_base, i );

        shift = ( v >> 12 ) & 0xF;
        v = v & 0xFFF;
        if ( shift ) {
            v = ( v | 0x1000 ) << ( shift - 1 );
        }
        p_fsm->fullhist[i] = v;
        sum += v;
    }
    p_fsm->fullhist_sum = sum;

    ae_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
    ae_flow.flow_state = MON_ALG_FLOW_STATE_INPUT_READY;
    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_AE_FLOW, &ae_flow, sizeof( ae_flow ) );
    LOG( LOG_INFO, "AE flow: INPUT_READY: frame_id_tracking: %d, cur frame_id: %u.", ae_flow.frame_id_tracking, ae_flow.frame_id_current );

#if FW_ZONE_AE
    int j;
    for ( i = 0; i < ACAMERA_ISP_METERING_HIST_AEXP_NODES_USED_VERT_DEFAULT; i++ ) {
        for ( j = 0; j < ACAMERA_ISP_METERING_HIST_AEXP_NODES_USED_HORIZ_DEFAULT; j++ ) {
            p_fsm->hist4[i * ACAMERA_ISP_METERING_HIST_AEXP_NODES_USED_HORIZ_DEFAULT + j] = ( uint16_t )( acamera_metering_stats_mem_array_data_read( p_fsm->cmn.isp_base, ( i * ACAMERA_ISP_METERING_HIST_AEXP_NODES_USED_HORIZ_DEFAULT + j ) * 2 + 1 ) >> 16 );
        }
    }
#endif
}

void ae_acamera_fsm_process_interrupt( ae_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event )
{
    if ( acamera_fsm_util_is_irq_event_ignored( (fsm_irq_mask_t *)( &p_fsm->mask ), irq_event ) )
        return;

    switch ( irq_event ) {
    case ACAMERA_IRQ_AE_STATS:
        ae_acamera_read_full_histogram_data( (ae_acamera_fsm_ptr_t)p_fsm );
        fsm_raise_event( p_fsm, event_id_ae_stats_ready );
        break;
    }
}

static inline uint32_t full_ratio_to_adjaced( int32_t sensor_exp_number, uint32_t ratio )
{
    switch ( sensor_exp_number ) {
    case 4:
        return acamera_math_exp2( acamera_log2_fixed_to_fixed( ratio, 6, 8 ) / 3, 8, 6 ) >> 6;
        break;
    case 3:
        return acamera_sqrt32( ratio >> 6 );
        break;
    default:
    case 2:
        return ratio >> 6;
        break;
    }
}

static void ae_prepare_input( ae_acamera_fsm_ptr_t p_fsm, ae_custom_input_t *custom_input, ae_acamera_input_t *acamera_input )
{
    system_memset( custom_input, 0, sizeof( *custom_input ) );
    system_memset( acamera_input, 0, sizeof( *acamera_input ) );

#if FW_CUSTOM_AE_ALG
    // TODO: prepare custom input
    ae_balanced_param_t *param = (ae_balanced_param_t *)_GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_CONTROL );
    custom_input->target_point = param->target_point;
    custom_input->coeff = param->pi_coeff;

    custom_input->global_exposure = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_exposure;
    custom_input->global_manual_exposure = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_exposure;

    int32_t max_exposure_log2 = 0;
    int32_t type = CMOS_MAX_EXPOSURE_LOG2;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CMOS_EXPOSURE_LOG2, &type, sizeof( type ), &max_exposure_log2, sizeof( max_exposure_log2 ) );
    custom_input->max_exposure_log2 = max_exposure_log2;

    fsm_param_awb_zone_stats_info_t awb_zone_stats;
    awb_zone_stats.zone_stats = NULL;
    awb_zone_stats.zone_size = 0;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_AWB_ZONE_STATS_INFO, NULL, 0, &awb_zone_stats, sizeof( awb_zone_stats ) );
    custom_input->awb_zone_stats = awb_zone_stats.zone_stats;
    custom_input->awb_zone_size = awb_zone_stats.zone_size;
#else
    // acamera_input
    acamera_input->ae_ctrl = (ae_balanced_param_t *)_GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_CONTROL );

    fsm_param_sensor_info_t sensor_info;
    sensor_info.sensor_exp_number = 1;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_SENSOR_INFO, NULL, 0, &sensor_info, sizeof( sensor_info ) );
    acamera_input->misc_info.sensor_exp_number = sensor_info.sensor_exp_number;

    int32_t total_gain = 0;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CMOS_TOTAL_GAIN, NULL, 0, &total_gain, sizeof( total_gain ) );
    acamera_input->misc_info.total_gain = total_gain;

    int32_t max_exposure_log2 = 0;
    int32_t type = CMOS_MAX_EXPOSURE_LOG2;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CMOS_EXPOSURE_LOG2, &type, sizeof( type ), &max_exposure_log2, sizeof( max_exposure_log2 ) );
    acamera_input->misc_info.max_exposure_log2 = max_exposure_log2;

    cmos_control_param_t *param_cmos = (cmos_control_param_t *)_GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CMOS_CONTROL );
    acamera_input->misc_info.global_max_exposure_ratio = param_cmos->global_max_exposure_ratio;

    uint32_t iridix_contrast = 256;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_IRIDIX_CONTRAST, NULL, 0, &iridix_contrast, sizeof( iridix_contrast ) );
    acamera_input->misc_info.iridix_contrast = iridix_contrast;

    acamera_input->misc_info.global_exposure = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_exposure;
    acamera_input->misc_info.global_ae_compensation = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_ae_compensation;
    acamera_input->misc_info.global_manual_exposure = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_exposure;
    acamera_input->misc_info.global_manual_exposure_ratio = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_exposure_ratio;
    acamera_input->misc_info.global_exposure_ratio = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_exposure_ratio;

    acamera_input->cali_data.ae_corr_lut = _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_CORRECTION );
    acamera_input->cali_data.ae_corr_lut_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_CORRECTION );
    acamera_input->cali_data.ae_exp_corr_lut = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_EXPOSURE_CORRECTION );
    acamera_input->cali_data.ae_exp_corr_lut_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_EXPOSURE_CORRECTION );
    acamera_input->cali_data.ae_hdr_target = _GET_MOD_ENTRY16_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_CONTROL_HDR_TARGET );
    acamera_input->cali_data.ae_hdr_target_len = _GET_ROWS( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AE_CONTROL_HDR_TARGET );
    acamera_input->cali_data.ae_exp_ratio_adjustment = _GET_MOD_ENTRY16_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_EXPOSURE_RATIO_ADJUSTMENT );
    acamera_input->cali_data.ae_exp_ratio_adjustment_len = _GET_ROWS( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_EXPOSURE_RATIO_ADJUSTMENT );
#endif
}

void ae_process_stats( ae_acamera_fsm_ptr_t p_fsm )
{
    if ( !p_fsm->ae_alg_obj.proc ) {
        LOG( LOG_ERR, "AE: can't process stats since function is NULL." );
        return;
    }

    ae_stats_data_t stats;
    stats.fullhist = p_fsm->fullhist;
    stats.fullhist_size = array_size( p_fsm->fullhist );
    stats.fullhist_sum = p_fsm->fullhist_sum;
#if FW_ZONE_AE
    stats.zone_hist = p_fsm->hist4;
    stats.zone_hist_size = array_size( p_fsm->hist4 );
#endif

    ae_acamera_input_t acamera_ae_input;
    ae_custom_input_t custom_ae_input;
    ae_input_data_t ae_input;

    ae_prepare_input( p_fsm, &custom_ae_input, &acamera_ae_input );
    ae_input.custom_input = &custom_ae_input;
    ae_input.acamera_input = &acamera_ae_input;

    ae_output_data_t ae_output;
    ae_acamera_output_t acamera_ae_output;
    ae_custom_output_t custom_ae_output;
    system_memset( &acamera_ae_output, 0, sizeof( acamera_ae_output ) );
    ae_output.acamera_output = &acamera_ae_output;
    ae_output.custom_output = &custom_ae_output;

    // call the core algorithm to calculate target exposure and exposure_ratio
    if ( p_fsm->ae_alg_obj.proc( p_fsm->ae_alg_obj.ae_ctx, &stats, &ae_input, &ae_output ) != 0 ) {
        LOG( LOG_ERR, "AE algorithm process failed." );
        return;
    }

#if FW_CUSTOM_AE_ALG
    p_fsm->exposure_log2 = custom_ae_output.exposure_log2;
    p_fsm->exposure_ratio = custom_ae_output.exposure_ratio;
#else
    p_fsm->exposure_log2 = acamera_ae_output.exposure_log2;
    p_fsm->exposure_ratio = acamera_ae_output.exposure_ratio;
#endif

    if ( !ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_exposure ) {
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_exposure = ( acamera_math_exp2( p_fsm->exposure_log2, LOG2_GAIN_SHIFT, 6 ) );
    }

    if ( !ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_exposure_ratio ) {
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_exposure_ratio = full_ratio_to_adjaced( acamera_ae_input.misc_info.sensor_exp_number, p_fsm->exposure_ratio );
    }

    fsm_param_exposure_target_t exp_target;
    exp_target.exposure_log2 = p_fsm->exposure_log2;
    exp_target.exposure_ratio = p_fsm->exposure_ratio;
    exp_target.frame_id_tracking = p_fsm->cur_using_stats_frame_id;

    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_EXPOSURE_TARGET, &exp_target, sizeof( exp_target ) );

    // skip when frame_id is 0.
    if ( p_fsm->cur_using_stats_frame_id ) {
        fsm_param_mon_alg_flow_t ae_flow;
        ae_flow.frame_id_tracking = p_fsm->cur_using_stats_frame_id;
        ae_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
        ae_flow.flow_state = MON_ALG_FLOW_STATE_OUTPUT_READY;
        acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_AE_FLOW, &ae_flow, sizeof( ae_flow ) );
        LOG( LOG_INFO, "AE flow: OUTPUT_READY: frame_id_tracking: %d, cur frame_id: %u.", ae_flow.frame_id_tracking, ae_flow.frame_id_current );
    }
}

void ae_acamera_initialize( ae_acamera_fsm_ptr_t p_fsm )
{
    p_fsm->lib_handle = dlopen( "libacamera_ae_alg.so", RTLD_LAZY );
    LOG( LOG_INFO, "AE: try to open custom_alg library, return: %p.", p_fsm->lib_handle );
    if ( !p_fsm->lib_handle ) {
        p_fsm->lib_handle = dlopen( "./libacamera_alg_core.so", RTLD_LAZY );
        LOG( LOG_INFO, "AE: try to open acamera_alg_core library, return: %p.", p_fsm->lib_handle );
    }

    if ( p_fsm->lib_handle ) {
        /* init, deinit and proc are required in algorithm lib */
        /* frameid, exposure and prevparam are optional in algorithm lib */
        p_fsm->ae_alg_obj.init = dlsym( p_fsm->lib_handle, "ae_acamera_core_init" );
        p_fsm->ae_alg_obj.deinit = dlsym( p_fsm->lib_handle, "ae_acamera_core_deinit" );
        p_fsm->ae_alg_obj.proc = dlsym( p_fsm->lib_handle, "ae_acamera_core_proc" );
        p_fsm->ae_alg_obj.frameid = dlsym( p_fsm->lib_handle, "ae_acamera_core_frame_id" );
        p_fsm->ae_alg_obj.exposure = dlsym( p_fsm->lib_handle, "ae_acamera_core_expo_correction" );
        p_fsm->ae_alg_obj.prevparam = dlsym( p_fsm->lib_handle, "ae_acamera_core_prev_param" );
        LOG( LOG_INFO, "AE: init: %p, deinit: %p, proc: %p, frameid: %p, exposure: %p, prevparam: %p",
		p_fsm->ae_alg_obj.init, p_fsm->ae_alg_obj.deinit, p_fsm->ae_alg_obj.proc,
		p_fsm->ae_alg_obj.frameid, p_fsm->ae_alg_obj.exposure, p_fsm->ae_alg_obj.prevparam);
    }

    if ( p_fsm->ae_alg_obj.init ) {
        p_fsm->ae_alg_obj.ae_ctx = p_fsm->ae_alg_obj.init( p_fsm->cmn.ctx_id );
        LOG( LOG_INFO, "Init AE core, ret: %p.", p_fsm->ae_alg_obj.ae_ctx );
    }

    acamera_isp_metering_aexp_hist_thresh_0_1_write( p_fsm->cmn.isp_base, 0 );
    acamera_isp_metering_aexp_hist_thresh_1_2_write( p_fsm->cmn.isp_base, 0 );
    acamera_isp_metering_aexp_hist_thresh_3_4_write( p_fsm->cmn.isp_base, 0 );
    acamera_isp_metering_aexp_hist_thresh_4_5_write( p_fsm->cmn.isp_base, 224 );

    int i, j;
    for ( i = 0; i < ACAMERA_ISP_METERING_HIST_AEXP_NODES_USED_VERT_DEFAULT; i++ ) {
        for ( j = 0; j < ACAMERA_ISP_METERING_HIST_AEXP_NODES_USED_HORIZ_DEFAULT; j++ ) {
            acamera_isp_metering_hist_aexp_zones_weight_write( p_fsm->cmn.isp_base, ISP_METERING_ZONES_MAX_H * i + j, 15 );
        }
    }

    ae_roi_update( p_fsm );
}

void ae_acamera_deinitialize( ae_acamera_fsm_ptr_t p_fsm )
{
    if ( p_fsm->ae_alg_obj.deinit ) {
        p_fsm->ae_alg_obj.deinit( p_fsm->ae_alg_obj.ae_ctx );
    }

    if ( p_fsm->lib_handle ) {
        dlclose( p_fsm->lib_handle );
    }
}

void ae_exposure_correction( ae_acamera_fsm_ptr_t p_fsm, int32_t corr )
{
	if ( p_fsm->ae_alg_obj.exposure ) {
        p_fsm->ae_alg_obj.exposure( p_fsm->ae_alg_obj.ae_ctx, corr );
    }
}

void ae_set_frameid( ae_acamera_fsm_ptr_t p_fsm, uint32_t frame_id )
{
	if ( p_fsm->ae_alg_obj.frameid ) {
        p_fsm->ae_alg_obj.frameid( p_fsm->ae_alg_obj.ae_ctx, frame_id );
    }
}

void ae_prev_param( ae_acamera_fsm_ptr_t p_fsm, isp_ae_preset_t * param )
{
    if ( p_fsm->ae_alg_obj.prevparam ) {
        p_fsm->ae_alg_obj.prevparam( p_fsm->ae_alg_obj.ae_ctx, param );
    }
}

