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

#include "acamera_types.h"

#include "acamera_fw.h"
#include "acamera_metering_stats_mem_config.h"
#include "acamera_math.h"
#include "acamera_lens_api.h"
#include "system_stdlib.h"
#include "acamera_isp_core_nomem_settings.h"
#include "af_acamera_fsm.h"

//================================================================================
//#define TRACE
//#define TRACE_IQ
//#define TRACE_SCENE_CHANGE
//================================================================================

#if defined( ISP_METERING_OFFSET_AF )
#define ISP_METERING_OFFSET_AUTO_FOCUS ISP_METERING_OFFSET_AF
#elif defined( ISP_METERING_OFFSET_AF2W )
#define ISP_METERING_OFFSET_AUTO_FOCUS ISP_METERING_OFFSET_AF2W
#else
#error "The AF metering offset is not defined in acamera_metering_mem_config.h"
#endif

#include <dlfcn.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_AF_ACAMERA
#endif


static void af_acamera_read_stats_data( af_acamera_fsm_ptr_t p_fsm )
{
    uint8_t x, y;
    uint32_t( *stats )[2];

    p_fsm->zones_horiz = acamera_isp_metering_af_nodes_used_horiz_read( p_fsm->cmn.isp_base );
    p_fsm->zones_vert = acamera_isp_metering_af_nodes_used_vert_read( p_fsm->cmn.isp_base );
    stats = p_fsm->zone_raw_statistic;

    if ( !p_fsm->zones_horiz || !p_fsm->zones_vert ) {
        p_fsm->zones_horiz = ISP_DEFAULT_AF_ZONES_HOR;
        p_fsm->zones_vert = ISP_DEFAULT_AF_ZONES_VERT;
    }

    for ( y = 0; y < p_fsm->zones_vert; y++ ) {
        uint32_t inx = (uint32_t)y * p_fsm->zones_horiz;
        for ( x = 0; x < p_fsm->zones_horiz; x++ ) {
            uint32_t full_inx = inx + x;
            stats[full_inx][0] = acamera_metering_stats_mem_array_data_read( p_fsm->cmn.isp_base, ISP_METERING_OFFSET_AUTO_FOCUS + ( ( full_inx ) << 1 ) + 0 );
            stats[full_inx][1] = acamera_metering_stats_mem_array_data_read( p_fsm->cmn.isp_base, ISP_METERING_OFFSET_AUTO_FOCUS + ( ( full_inx ) << 1 ) + 1 );
        }
    }
}

//================================================================================
void af_acamera_fsm_process_interrupt( af_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event )
{
    if ( acamera_fsm_util_is_irq_event_ignored( (fsm_irq_mask_t *)( &p_fsm->mask ), irq_event ) )
        return;

    //check if lens was initialised properly
    if ( p_fsm->lens_driver_ok == 0 )
        return;

    switch ( irq_event ) {
    case ACAMERA_IRQ_AF2_STATS:
        af_acamera_read_stats_data( (af_acamera_fsm_ptr_t)p_fsm );
        fsm_raise_event( p_fsm, event_id_af_stats_ready );
        break;
    }
}
//================================================================================
void af_acamera_init( af_acamera_fsm_ptr_t p_fsm )
{
    af_lms_param_t *param = NULL;
    //check if lens was initialised properly
    if ( !p_fsm->lens_driver_ok ) {
        acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_KLENS_STATUS, NULL, 0, &p_fsm->lens_driver_ok, sizeof( p_fsm->lens_driver_ok ) );
        if ( p_fsm->lens_driver_ok ) {
            param = (af_lms_param_t *)_GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AF_LMS );
            acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_KLENS_PARAM, NULL, 0, &p_fsm->lens_param, sizeof( p_fsm->lens_param ) );
        }
    }

    if ( param ) {
        p_fsm->pos_min = param->pos_min;
        p_fsm->pos_max = param->pos_max;

        p_fsm->mask.repeat_irq_mask = ACAMERA_IRQ_MASK( ACAMERA_IRQ_AF2_STATS );
        af_acamera_request_interrupt( p_fsm, p_fsm->mask.repeat_irq_mask );

        p_fsm->lib_handle = dlopen( "libacamera_af_alg.so", RTLD_LAZY );
        LOG( LOG_INFO, "AF: try to open custom_alg library, return: %p.", p_fsm->lib_handle );
        if ( !p_fsm->lib_handle ) {
            p_fsm->lib_handle = dlopen( "./libacamera_alg_core.so", RTLD_LAZY );
            LOG( LOG_INFO, "AF: try to open acamera_alg_core library, return: %p.", p_fsm->lib_handle );
        }

        if ( p_fsm->lib_handle ) {
            /* init, deinit and proc are required in algorithm lib */
            p_fsm->af_alg_obj.init = dlsym( p_fsm->lib_handle, "af_acamera_core_init" );
            p_fsm->af_alg_obj.deinit = dlsym( p_fsm->lib_handle, "af_acamera_core_deinit" );
            p_fsm->af_alg_obj.proc = dlsym( p_fsm->lib_handle, "af_acamera_core_proc" );
            LOG( LOG_INFO, "AF: init: %p, deinit: %p, proc: %p .",
                        p_fsm->af_alg_obj.init, p_fsm->af_alg_obj.deinit, p_fsm->af_alg_obj.proc );
        } else {
            LOG( LOG_ERR, "Load failed, error: %s.", dlerror() );
        }

        if ( p_fsm->af_alg_obj.init ) {
            p_fsm->af_alg_obj.af_ctx = p_fsm->af_alg_obj.init( p_fsm->cmn.ctx_id );
            LOG( LOG_INFO, "Init AF core, ret: %p.", p_fsm->af_alg_obj.af_ctx );
        }
    }
}

void af_acamera_update( af_acamera_fsm_ptr_t p_fsm )
{
    p_fsm->last_position = p_fsm->new_lens_pos;
    p_fsm->last_sharp_done = p_fsm->new_sharp_val;
}

static void af_acamera_prepare_input( af_acamera_fsm_ptr_t p_fsm, af_custom_input_t *custom_input, af_acamera_input_t *acamera_input )
{
    system_memset( custom_input, 0, sizeof( *custom_input ) );
    system_memset( acamera_input, 0, sizeof( *acamera_input ) );

    // acamera_input
    int16_t accel_angle = 0;

#if defined( FSM_PARAM_GET_ACCEL_DATA )
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_ACCEL_DATA, NULL, 0, &accel_angle, sizeof( accel_angle ) );
#endif
    acamera_input->misc_info.accel_angle = accel_angle;
    acamera_input->misc_info.lens_min_step = p_fsm->lens_param.min_step;

    acamera_input->af_info.refocus_required = p_fsm->refocus_required;
    // reset to 0 after passed to core algorithm
    p_fsm->refocus_required = 0;
    acamera_input->af_info.roi = p_fsm->roi;
    acamera_input->af_info.af_mode = p_fsm->mode;
    acamera_input->af_info.zones_horiz = p_fsm->zones_horiz;
    acamera_input->af_info.zones_vert = p_fsm->zones_vert;
    acamera_input->af_info.af_pos_manual = p_fsm->pos_manual;

    acamera_input->cali_data.af_param = (af_lms_param_t *)_GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AF_LMS );
    acamera_input->cali_data.af_zone_whgh_h = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AF_ZONE_WGHT_HOR );
    acamera_input->cali_data.af_zone_whgh_h_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AF_ZONE_WGHT_HOR );
    acamera_input->cali_data.af_zone_whgh_v = _GET_USHORT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AF_ZONE_WGHT_VER );
    acamera_input->cali_data.af_zone_whgh_v_len = _GET_LEN( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AF_ZONE_WGHT_VER );
}

void af_acamera_process_stats( af_acamera_fsm_ptr_t p_fsm )
{
    if ( !p_fsm->af_alg_obj.proc ) {
        LOG( LOG_ERR, "AF: can't process stats since function is NULL." );
        return;
    }

    af_stats_data_t stats;
    stats.zones_stats = (uint32_t *)p_fsm->zone_raw_statistic;
    stats.zones_size = array_size( p_fsm->zone_raw_statistic );

    af_acamera_input_t acamera_af_input;
    af_custom_input_t custom_af_input;
    af_input_data_t af_input;

    af_acamera_prepare_input( p_fsm, &custom_af_input, &acamera_af_input );
    af_input.custom_input = &custom_af_input;
    af_input.acamera_input = &acamera_af_input;

    af_output_data_t af_output;
    af_acamera_output_t acamera_af_output;
    system_memset( &acamera_af_output, 0, sizeof( acamera_af_output ) );
    af_output.acamera_output = &acamera_af_output;
    af_output.custom_output = NULL;

    // call the core algorithm to calculate target exposure and exposure_ratio
    if ( p_fsm->af_alg_obj.proc( p_fsm->af_alg_obj.af_ctx, &stats, &af_input, &af_output ) != 0 ) {
        LOG( LOG_ERR, "AE algorithm process failed." );
        return;
    }

    p_fsm->new_lens_pos = acamera_af_output.af_lens_pos;
    p_fsm->new_sharp_val = acamera_af_output.af_sharp_val;

    LOG( LOG_DEBUG, "mode: %d, new_lens_pos: %d, new_sharp_val: %d.", p_fsm->mode, p_fsm->new_lens_pos, p_fsm->new_sharp_val );
}

void af_acamera_deinit( af_acamera_fsm_ptr_t p_fsm )
{
    if ( ACAMERA_FSM2CTX_PTR( p_fsm )->settings.lens_deinit )
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->settings.lens_deinit( p_fsm->lens_ctx );

    if ( p_fsm->af_alg_obj.deinit ) {
        p_fsm->af_alg_obj.deinit( p_fsm->af_alg_obj.af_ctx );
    }
    if ( p_fsm->lib_handle ) {
        dlclose( p_fsm->lib_handle );
    }
}
