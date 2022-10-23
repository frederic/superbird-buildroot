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
#include "acamera_metering_stats_mem_config.h"
#include "system_stdlib.h"
#include "awb_acamera_fsm.h"
#include "awb_standard_api.h"
#include "awb_acamera_core.h"

#include <dlfcn.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_AWB_ACAMERA
#endif

static __inline uint32_t acamera_awb_statistics_data_read( awb_acamera_fsm_t *p_fsm, uint32_t index_inp )
{
    return acamera_metering_stats_mem_array_data_read( p_fsm->cmn.isp_base, index_inp + ISP_METERING_OFFSET_AWB );
}

// Write matrix coefficients
static void awb_coeffs_write( const awb_acamera_fsm_t *p_fsm )
{
    acamera_isp_ccm_coefft_wb_r_write( p_fsm->cmn.isp_base, p_fsm->awb_warming[0] );
    acamera_isp_ccm_coefft_wb_g_write( p_fsm->cmn.isp_base, p_fsm->awb_warming[1] );
    acamera_isp_ccm_coefft_wb_b_write( p_fsm->cmn.isp_base, p_fsm->awb_warming[2] );
}

void awb_roi_update( awb_acamera_fsm_ptr_t p_fsm )
{
    uint16_t horz_zones = acamera_isp_metering_awb_nodes_used_horiz_read( p_fsm->cmn.isp_base );
    uint16_t vert_zones = acamera_isp_metering_awb_nodes_used_vert_read( p_fsm->cmn.isp_base );
    uint16_t x, y;

    uint16_t *ptr_awb_zone_whgh_h = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_ZONE_WGHT_HOR );
    uint16_t *ptr_awb_zone_whgh_v = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_ZONE_WGHT_VER );

    uint8_t x_start = ( uint8_t )( ( ( ( p_fsm->roi >> 24 ) & 0xFF ) * horz_zones + 128 ) >> 8 );
    uint8_t x_end = ( uint8_t )( ( ( ( p_fsm->roi >> 8 ) & 0xFF ) * horz_zones + 128 ) >> 8 );
    uint8_t y_start = ( uint8_t )( ( ( ( p_fsm->roi >> 16 ) & 0xFF ) * vert_zones + 128 ) >> 8 );
    uint8_t y_end = ( uint8_t )( ( ( ( p_fsm->roi >> 0 ) & 0xFF ) * vert_zones + 128 ) >> 8 );
    uint8_t zone_size_x = x_end - x_start;
    uint8_t zone_size_y = y_end - y_start;
    uint32_t middle_x = zone_size_x * 256 / 2;
    uint32_t middle_y = zone_size_y * 256 / 2;
    uint32_t len_zone_wght_hor = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_ZONE_WGHT_HOR );
    uint32_t len_zone_wght_ver = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_ZONE_WGHT_VER );
    uint16_t scale_x = ( horz_zones - 1 ) / ( len_zone_wght_hor > 0 ? len_zone_wght_hor : 1 ) + 1;
    uint16_t scale_y = ( vert_zones - 1 ) / ( len_zone_wght_ver > 0 ? len_zone_wght_ver : 1 ) + 1;

    uint16_t gaus_center_x = ( len_zone_wght_hor * 256 / 2 ) * scale_x;
    uint16_t gaus_center_y = ( len_zone_wght_ver * 256 / 2 ) * scale_y;

    for ( y = 0; y < vert_zones; y++ ) {
        uint8_t awb_coeff = 0;
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
                    awb_coeff = 0;
                } else {
                    coeff_x = ( gaus_center_x + distance_x ) / 256;
                    if ( distance_x > 0 && ( distance_x & 0x80 ) )
                        coeff_x--;
                    coeff_y = ( gaus_center_y + distance_y ) / 256;
                    if ( distance_y > 0 && ( distance_y & 0x80 ) )
                        coeff_y--;

                    coeff_x = ptr_awb_zone_whgh_h[coeff_x / scale_x];
                    coeff_y = ptr_awb_zone_whgh_v[coeff_y / scale_y];

                    awb_coeff = ( coeff_x * coeff_y ) >> 4;
                    if ( awb_coeff > 1 )
                        awb_coeff--;
                }
            } else {
                awb_coeff = 0;
            }
            acamera_isp_metering_awb_zones_weight_write( p_fsm->cmn.isp_base, y * vert_zones + x, awb_coeff );
        }
    }
}


// Initalisation code
void awb_init( awb_acamera_fsm_t *p_fsm )
{
    p_fsm->lib_handle = dlopen( "libacamera_awb_alg.so", RTLD_LAZY );
    LOG( LOG_INFO, "AWB: try to open custom_alg, return: %p.", p_fsm->lib_handle );
    if ( !p_fsm->lib_handle ) {
        p_fsm->lib_handle = dlopen( "./libacamera_alg_core.so", RTLD_LAZY );
        LOG( LOG_INFO, "AWB: try to open acamera_alg_core, return: %p.", p_fsm->lib_handle );
    }

    if ( p_fsm->lib_handle ) {
        /* init, deinit and proc are required in algorithm lib */
        /* frameid and prevparam are optional in algorithm lib */
        p_fsm->awb_alg_obj.init = dlsym( p_fsm->lib_handle, "awb_acamera_core_init" );
        p_fsm->awb_alg_obj.deinit = dlsym( p_fsm->lib_handle, "awb_acamera_core_deinit" );
        p_fsm->awb_alg_obj.proc = dlsym( p_fsm->lib_handle, "awb_acamera_core_proc" );
	p_fsm->awb_alg_obj.frameid = dlsym( p_fsm->lib_handle, "awb_acamera_core_frame_id" );
	p_fsm->awb_alg_obj.prevparam = dlsym( p_fsm->lib_handle, "awb_acamera_core_prev_param" );
        LOG( LOG_INFO, "AWB: init: %p, deinit: %p, proc: %p, frameid: %p, prevparam: %p",
		p_fsm->awb_alg_obj.init, p_fsm->awb_alg_obj.deinit,
		p_fsm->awb_alg_obj.proc, p_fsm->awb_alg_obj.frameid,
		p_fsm->awb_alg_obj.prevparam);
    }

    if ( p_fsm->awb_alg_obj.init ) {
        p_fsm->awb_alg_obj.awb_ctx = p_fsm->awb_alg_obj.init( p_fsm->cmn.ctx_id );
        LOG( LOG_INFO, "Init AWB core, ret: %p.", p_fsm->awb_alg_obj.awb_ctx );
    }

    // Initial AWB (rg,bg) is the identity
    p_fsm->rg_coef = 0x100;
    p_fsm->bg_coef = 0x100;

    // Set the default awb values
    if ( MAX_AWB_ZONES < acamera_isp_metering_awb_nodes_used_horiz_read( p_fsm->cmn.isp_base ) * acamera_isp_metering_awb_nodes_used_vert_read( p_fsm->cmn.isp_base ) ) {
        LOG( LOG_ERR, "MAX_AWB_ZONES is less than hardware reported zones" );
    }

    p_fsm->wb_log2[0] = 0;
    p_fsm->wb_log2[1] = 0;
    p_fsm->wb_log2[2] = 0;
    p_fsm->wb_log2[3] = 0;

    p_fsm->awb_warming[0] = (int32_t)_GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_WARMING_LS_D50 )[0];
    p_fsm->awb_warming[1] = (int32_t)_GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_WARMING_LS_D50 )[1];
    p_fsm->awb_warming[2] = (int32_t)_GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_WARMING_LS_D50 )[2];
    awb_coeffs_write( p_fsm );

    awb_roi_update( p_fsm );
}

void awb_deinit( awb_acamera_fsm_ptr_t p_fsm )
{
    if ( p_fsm->awb_alg_obj.deinit ) {
        p_fsm->awb_alg_obj.deinit( p_fsm->awb_alg_obj.awb_ctx );
    }

    if ( p_fsm->lib_handle ) {
        dlclose( p_fsm->lib_handle );
    }
}

// Read the statistics from hardware
void awb_read_statistics( awb_acamera_fsm_t *p_fsm )
{
    // Only selected number of zones will contribute
    uint16_t _i;

    fsm_param_mon_alg_flow_t awb_flow;

    awb_flow.frame_id_tracking = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );

    if ( p_fsm->cur_using_stats_frame_id ) {
        LOG( LOG_INFO, "AWB: overwrite: prev frame_id: %d, cur: %d.", p_fsm->cur_using_stats_frame_id, awb_flow.frame_id_tracking );
    }

    p_fsm->cur_using_stats_frame_id = awb_flow.frame_id_tracking;

    p_fsm->curr_AWB_ZONES = acamera_isp_metering_awb_nodes_used_horiz_read( p_fsm->cmn.isp_base ) *
                            acamera_isp_metering_awb_nodes_used_vert_read( p_fsm->cmn.isp_base );

    if ( p_fsm->curr_AWB_ZONES > MAX_AWB_ZONES ) {
        LOG( LOG_ERR, "AVOIDED ARRAY ACCESS BEYOND LIMITS" );
        return;
    }

    // Read out the per zone statistics
    for ( _i = 0; _i < p_fsm->curr_AWB_ZONES; ++_i ) {
        uint32_t _metering_lut_entry;
        uint16_t irg, ibg;

        _metering_lut_entry = acamera_awb_statistics_data_read( p_fsm, _i * 2 );
        //What we get from HW is G/R.
        //It is also programmable in the latest HW.AWB_STATS_MODE=0-->G/R and AWB_STATS_MODE=1-->R/G
        //rg_coef is actually R_gain appiled to R Pixels.Since we get (G*G_gain)/(R*R_gain) from HW,we multiply by the gain rg_coef to negate its effect.
        irg = ( _metering_lut_entry & 0xfff );
        ibg = ( ( _metering_lut_entry >> 16 ) & 0xfff );

        irg = ( irg * ( p_fsm->rg_coef ) ) >> 8;
        ibg = ( ibg * ( p_fsm->bg_coef ) ) >> 8;
        irg = ( irg == 0 ) ? 1 : irg;
        ibg = ( ibg == 0 ) ? 1 : ibg;
        p_fsm->awb_zones[_i].rg = U16_MAX / irg;
        p_fsm->awb_zones[_i].bg = U16_MAX / ibg;
        p_fsm->awb_zones[_i].sum = acamera_awb_statistics_data_read( p_fsm, _i * 2 + 1 );
    }

    awb_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
    awb_flow.flow_state = MON_ALG_FLOW_STATE_INPUT_READY;
    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_AWB_FLOW, &awb_flow, sizeof( awb_flow ) );
}

//    For CCM switching
void awb_process_light_source( awb_acamera_fsm_t *p_fsm )
{
    int32_t total_gain = 0;
    int high_gain = 0;

    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CMOS_TOTAL_GAIN, NULL, 0, &total_gain, sizeof( total_gain ) );

    high_gain = ( total_gain >> ( LOG2_GAIN_SHIFT - 8 ) ) >= _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CCM_ONE_GAIN_THRESHOLD )[0];

    fsm_param_ccm_info_t ccm_info;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CCM_INFO, NULL, 0, &ccm_info, sizeof( ccm_info ) );

    if ( p_fsm->light_source_detected == p_fsm->light_source_candidate ) {
        if ( ( p_fsm->light_source_candidate != ccm_info.light_source ) || ( high_gain && ccm_info.light_source_ccm != AWB_LIGHT_SOURCE_UNKNOWN ) || ( !high_gain && ccm_info.light_source_ccm == AWB_LIGHT_SOURCE_UNKNOWN ) ) {
            ++p_fsm->detect_light_source_frames_count;
            if ( p_fsm->detect_light_source_frames_count >= p_fsm->switch_light_source_detect_frames_quantity && !ccm_info.light_source_change_frames_left ) {
                ccm_info.light_source_previous = ccm_info.light_source;
                ccm_info.light_source = p_fsm->light_source_candidate;
                ccm_info.light_source_ccm_previous = ccm_info.light_source_ccm;
                ccm_info.light_source_ccm = high_gain ? AWB_LIGHT_SOURCE_UNKNOWN : p_fsm->light_source_candidate; // for low light set ccm = I
                ccm_info.light_source_change_frames = p_fsm->switch_light_source_change_frames_quantity;
                ccm_info.light_source_change_frames_left = p_fsm->switch_light_source_change_frames_quantity;
                acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_CCM_INFO, &ccm_info, sizeof( ccm_info ) );

                // These are rarer so can print wherever they are fired (i.e. not dependent on ittcount)
                LOG( LOG_DEBUG, "Light source is changed\n" );
                if ( ccm_info.light_source == AWB_LIGHT_SOURCE_A )
                    LOG( LOG_DEBUG, "AWB_LIGHT_SOURCE_A\n" );
                if ( ccm_info.light_source == AWB_LIGHT_SOURCE_D40 )
                    LOG( LOG_DEBUG, "AWB_LIGHT_SOURCE_D40\n" );
                if ( ccm_info.light_source == AWB_LIGHT_SOURCE_D50 )
                    LOG( LOG_DEBUG, "AWB_LIGHT_SOURCE_D50\n" );
            }
        }
    } else {
        p_fsm->detect_light_source_frames_count = 0;
    }
    p_fsm->light_source_detected = p_fsm->light_source_candidate;
}

// Perform normalisation
void awb_normalise( awb_acamera_fsm_t *p_fsm )
{
    int32_t wb[4];

    wb[0] = acamera_log2_fixed_to_fixed( _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_STATIC_WB )[0], 8, LOG2_GAIN_SHIFT );
    wb[1] = acamera_log2_fixed_to_fixed( _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_STATIC_WB )[1], 8, LOG2_GAIN_SHIFT );
    wb[2] = acamera_log2_fixed_to_fixed( _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_STATIC_WB )[2], 8, LOG2_GAIN_SHIFT );
    wb[3] = acamera_log2_fixed_to_fixed( _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_STATIC_WB )[3], 8, LOG2_GAIN_SHIFT );

    if ( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_awb ) {
        wb[0] += acamera_log2_fixed_to_fixed( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_awb_red_gain, 8, LOG2_GAIN_SHIFT );
        wb[3] += acamera_log2_fixed_to_fixed( ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_awb_blue_gain, 8, LOG2_GAIN_SHIFT );
    } else {
        wb[0] += acamera_log2_fixed_to_fixed( p_fsm->rg_coef, 8, LOG2_GAIN_SHIFT );
        wb[3] += acamera_log2_fixed_to_fixed( p_fsm->bg_coef, 8, LOG2_GAIN_SHIFT );
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_awb_red_gain = p_fsm->rg_coef;
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_awb_blue_gain = p_fsm->bg_coef;
    }


    int i;
    int32_t min_wb = wb[0];
    for ( i = 1; i < 4; ++i ) {
        int32_t _wb = wb[i];
        if ( min_wb > _wb ) {
            min_wb = _wb;
        }
    }

    fsm_param_sensor_info_t sensor_info;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_SENSOR_INFO, NULL, 0, &sensor_info, sizeof( sensor_info ) );

    int32_t diff = ( ISP_INPUT_BITS << LOG2_GAIN_SHIFT ) - acamera_log2_fixed_to_fixed( ( 1 << ISP_INPUT_BITS ) - sensor_info.black_level, 0, LOG2_GAIN_SHIFT ) - min_wb;
    for ( i = 0; i < 4; ++i ) {
        int32_t _wb = wb[i] + diff;
        p_fsm->wb_log2[i] = _wb;
    }

    // skip when frame_id is 0.
    if ( p_fsm->cur_using_stats_frame_id ) {
        fsm_param_mon_alg_flow_t awb_flow;

        p_fsm->cur_result_gain_frame_id = p_fsm->cur_using_stats_frame_id;

        // prepare it for next time
        p_fsm->cur_using_stats_frame_id = 0;

        awb_flow.frame_id_tracking = p_fsm->cur_result_gain_frame_id;
        awb_flow.frame_id_current = acamera_fsm_util_get_cur_frame_id( &p_fsm->cmn );
        awb_flow.flow_state = MON_ALG_FLOW_STATE_OUTPUT_READY;
        acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_MON_AWB_FLOW, &awb_flow, sizeof( awb_flow ) );
    }
}

// Handle the hardware interrupt
void awb_acamera_fsm_process_interrupt( const awb_acamera_fsm_t *p_fsm, uint8_t irq_event )
{
    if ( acamera_fsm_util_is_irq_event_ignored( (fsm_irq_mask_t *)( &p_fsm->mask ), irq_event ) )
        return;

    switch ( irq_event ) {
    case ACAMERA_IRQ_AWB_STATS:
        awb_read_statistics( (awb_acamera_fsm_t *)p_fsm ); // we know what we are doing
        fsm_raise_event( p_fsm, event_id_awb_stats_ready );
        break;
    case ACAMERA_IRQ_FRAME_END:
        awb_coeffs_write( p_fsm );
        break;
    }
}

static void awb_prepare_input( awb_acamera_fsm_ptr_t p_fsm, awb_custom_input_t *custom_input, awb_acamera_input_t *acamera_input )
{
    system_memset( custom_input, 0, sizeof( *custom_input ) );
    system_memset( acamera_input, 0, sizeof( *acamera_input ) );

    // prepare acamera input data
    int32_t total_gain = 0;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CMOS_TOTAL_GAIN, NULL, 0, &total_gain, sizeof( total_gain ) );
    acamera_input->misc_info.log2_gain = total_gain >> ( LOG2_GAIN_SHIFT - 8 );

    int32_t cur_exposure_log2 = 0;
    int32_t type = CMOS_CURRENT_EXPOSURE_LOG2;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_CMOS_EXPOSURE_LOG2, &type, sizeof( type ), &cur_exposure_log2, sizeof( cur_exposure_log2 ) );
    acamera_input->misc_info.cur_exposure_log2 = cur_exposure_log2;

    uint32_t iridix_contrast = 256;
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_IRIDIX_CONTRAST, NULL, 0, &iridix_contrast, sizeof( iridix_contrast ) );
    acamera_input->misc_info.iridix_contrast = iridix_contrast;
    // update status info
    status_info_param_t *p_status_info = (status_info_param_t *)_GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_STATUS_INFO );
    p_status_info->awb_info.mix_light_contrast = iridix_contrast;


    acamera_input->misc_info.global_manual_awb = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_awb;
    acamera_input->misc_info.global_awb_red_gain = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_awb_red_gain;
    acamera_input->misc_info.global_awb_blue_gain = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_awb_blue_gain;

    acamera_input->cali_data.cali_light_src = (calibration_light_src_t)_GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_LIGHT_SRC );
    acamera_input->cali_data.cali_light_src_len = _GET_ROWS( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_LIGHT_SRC );
    acamera_input->cali_data.cali_evtolux_ev_lut = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_EVTOLUX_EV_LUT );
    acamera_input->cali_data.cali_evtolux_ev_lut_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_EVTOLUX_EV_LUT );
    acamera_input->cali_data.cali_evtolux_lux_lut = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_EVTOLUX_LUX_LUT );
    acamera_input->cali_data.cali_evtolux_lux_lut_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_EVTOLUX_LUX_LUT );
    acamera_input->cali_data.cali_awb_avg_coef = _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_AVG_COEF );
    acamera_input->cali_data.cali_awb_avg_coef_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_AVG_COEF );
    acamera_input->cali_data.cali_rg_pos = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_RG_POS );
    acamera_input->cali_data.cali_rg_pos_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_RG_POS );
    acamera_input->cali_data.cali_bg_pos = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_BG_POS );
    acamera_input->cali_data.cali_bg_pos_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_BG_POS );
    acamera_input->cali_data.cali_color_temp = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_COLOR_TEMP );
    acamera_input->cali_data.cali_color_temp_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_COLOR_TEMP );
    acamera_input->cali_data.cali_ct_rg_pos_calc = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CT_RG_POS_CALC );
    acamera_input->cali_data.cali_ct_rg_pos_calc_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CT_RG_POS_CALC );
    acamera_input->cali_data.cali_ct_bg_pos_calc = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CT_BG_POS_CALC );
    acamera_input->cali_data.cali_ct_bg_pos_calc_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CT_BG_POS_CALC );

    acamera_input->cali_data.cali_awb_bg_max_gain = _GET_MOD_ENTRY16_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_BG_MAX_GAIN );
    acamera_input->cali_data.cali_awb_bg_max_gain_len = _GET_ROWS( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_BG_MAX_GAIN );

    acamera_input->cali_data.cali_mesh_ls_weight = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_MESH_LS_WEIGHT );
    acamera_input->cali_data.cali_mesh_rgbg_weight = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_MESH_RGBG_WEIGHT );
    acamera_input->cali_data.cali_evtolux_probability_enable = _GET_UCHAR_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_EVTOLUX_PROBABILITY_ENABLE );
    acamera_input->cali_data.cali_awb_mix_light_param = _GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_MIX_LIGHT_PARAMETERS );
    acamera_input->cali_data.cali_ct65pos = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CT65POS );
    acamera_input->cali_data.cali_ct40pos = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CT40POS );
    acamera_input->cali_data.cali_ct30pos = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_CT30POS );
    acamera_input->cali_data.cali_sky_lux_th = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_SKY_LUX_TH );
    acamera_input->cali_data.cali_wb_strength = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_WB_STRENGTH );
    acamera_input->cali_data.cali_mesh_color_temperature = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_MESH_COLOR_TEMPERATURE );
    acamera_input->cali_data.cali_awb_warming_ls_a = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_WARMING_LS_A );
    acamera_input->cali_data.cali_awb_warming_ls_d75 = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_WARMING_LS_D75 );
    acamera_input->cali_data.cali_awb_warming_ls_d50 = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_WARMING_LS_D50 );
    acamera_input->cali_data.cali_awb_colour_preference = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), AWB_COLOUR_PREFERENCE );
}

void awb_process_stats( awb_acamera_fsm_ptr_t p_fsm )
{
    if ( !p_fsm->awb_alg_obj.proc ) {
        LOG( LOG_ERR, "AWB: can't process stats since function is NULL." );
        return;
    }

    awb_stats_data_t stats;
    stats.awb_zones = p_fsm->awb_zones;
    stats.zones_size = p_fsm->curr_AWB_ZONES;

    awb_acamera_input_t acamera_awb_input;
    awb_custom_input_t custom_awb_input;
    awb_input_data_t awb_input;

    awb_prepare_input( p_fsm, &custom_awb_input, &acamera_awb_input );
    awb_input.custom_input = &custom_awb_input;
    awb_input.acamera_input = &acamera_awb_input;

    awb_output_data_t awb_output;
    awb_acamera_output_t acamera_awb_output;
    awb_output.acamera_output = &acamera_awb_output;
    awb_output.custom_output = NULL;

    // call the core algorithm to calculate target exposure and exposure_ratio
    if ( p_fsm->awb_alg_obj.proc( p_fsm->awb_alg_obj.awb_ctx, &stats, &awb_input, &awb_output ) != 0 ) {
        LOG( LOG_ERR, "AWB algorithm process failed." );
        return;
    }

    p_fsm->rg_coef = acamera_awb_output.rg_coef;
    p_fsm->bg_coef = acamera_awb_output.bg_coef;
    p_fsm->temperature_detected = acamera_awb_output.temperature_detected;
    p_fsm->p_high = acamera_awb_output.p_high;
    p_fsm->light_source_candidate = acamera_awb_output.light_source_candidate;
    system_memcpy( p_fsm->awb_warming, acamera_awb_output.awb_warming, sizeof( p_fsm->awb_warming ) );

    LOG( LOG_INFO, "Out: rg: %d, bg: %d, temp: %d.", p_fsm->rg_coef, p_fsm->bg_coef, p_fsm->temperature_detected );
}

void awb_set_frameid( awb_acamera_fsm_ptr_t p_fsm, uint32_t frame_id )
{
	if ( p_fsm->awb_alg_obj.frameid ) {
        p_fsm->awb_alg_obj.frameid( p_fsm->awb_alg_obj.awb_ctx, frame_id );
    }
}

void awb_prev_param( awb_acamera_fsm_ptr_t p_fsm, isp_awb_preset_t * param )
{
	if ( p_fsm->awb_alg_obj.prevparam ) {
		p_fsm->awb_alg_obj.prevparam( p_fsm->awb_alg_obj.awb_ctx, param );
	}
}