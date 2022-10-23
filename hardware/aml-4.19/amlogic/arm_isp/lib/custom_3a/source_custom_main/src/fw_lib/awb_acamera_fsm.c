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
#include "awb_acamera_fsm.h"
#include "awb_acamera_core.h"
#include "acamera_3aalg_preset.h"

#if defined( ISP_HAS_SBUF_FSM ) || defined( ISP_HAS_USER2KERNEL_FSM )
#include "sbuf.h"
#endif

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_AWB_ACAMERA
#endif

void awb_acamera_fsm_clear( awb_acamera_fsm_t *p_fsm )
{
    p_fsm->curr_AWB_ZONES = MAX_AWB_ZONES;
    p_fsm->avg_GR = 128;
    p_fsm->avg_GB = 128;
    p_fsm->temperature_detected = 5000;
    p_fsm->light_source_detected = AWB_LIGHT_SOURCE_D50;
    p_fsm->detect_light_source_frames_count = 0;
    p_fsm->roi = 65535;
    p_fsm->switch_light_source_detect_frames_quantity = AWB_DLS_SWITCH_LIGHT_SOURCE_DETECT_FRAMES_QUANTITY;
    p_fsm->switch_light_source_change_frames_quantity = AWB_DLS_SWITCH_LIGHT_SOURCE_CHANGE_FRAMES_QUANTITY;
    p_fsm->is_ready = 1;

    p_fsm->cur_using_stats_frame_id = 0;
    p_fsm->cur_result_gain_frame_id = 0;

    p_fsm->lib_handle = NULL;

    memset( &p_fsm->awb_alg_obj, 0, sizeof( p_fsm->awb_alg_obj ) );
}

void awb_acamera_fsm_request_interrupt( awb_acamera_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask )
{
    acamera_isp_interrupts_disable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
    p_fsm->mask.irq_mask |= mask;
    acamera_isp_interrupts_enable( ACAMERA_FSM2MGR_PTR( p_fsm ) );
}


#if AWB_MODE_ID
static uint32_t get_awb_idx( uint32_t value )
{
    uint32_t res = 0;
    // acamera awb scenes are always located in a certain order
    // this order is
    // {AWB_DAY_LIGHT}
    // {AWB_CLOUDY}
    // {AWB_INCANDESCENT}
    // {AWB_FLOURESCENT}
    // {AWB_TWILIGHT}
    // {AWB_SHADE}
    // {AWB_WARM_FLOURESCENT}
    switch ( value ) {
    case AWB_DAY_LIGHT:
        res = 0;
        break;
    case AWB_CLOUDY:
        res = 1;
        break;
    case AWB_INCANDESCENT:
        res = 2;
        break;
    case AWB_FLOURESCENT:
        res = 3;
        break;
    case AWB_TWILIGHT:
        res = 4;
        break;
    case AWB_SHADE:
        res = 5;
        break;
    case AWB_WARM_FLOURESCENT:
        res = 6;
        break;
    default:
        LOG( LOG_ERR, "Invalid AWB scene index. Please check your calibrations" );
        break;
    }
    return res;
}
#endif

static int AWB_set_mode( awb_acamera_fsm_ptr_t p_fsm, uint32_t mode )
{
    int rc = 0;

#if AWB_MODE_ID
    switch ( mode ) {
    case AWB_MANUAL:
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_manual_awb = ( 1 );
        p_fsm->mode = mode;
        break;

    case AWB_AUTO:
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_manual_awb = ( 0 );
        p_fsm->mode = mode;
        break;

    case AWB_DAY_LIGHT:
    case AWB_CLOUDY:
    case AWB_INCANDESCENT:
    case AWB_FLOURESCENT:
    case AWB_TWILIGHT:
    case AWB_SHADE:
    case AWB_WARM_FLOURESCENT: {
        modulation_entry_t *_calibration_awb_scene_presets = _GET_MOD_ENTRY16_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_AWB_SCENE_PRESETS );
        uint32_t i = get_awb_idx( mode );
        p_fsm->mode = mode;
        // Then set the red and blue values
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_manual_awb = ( 1 );
        uint32_t cr = _calibration_awb_scene_presets[i].x;
        uint32_t cb = _calibration_awb_scene_presets[i].y;
        // Change 9 bit into 7 bit
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_awb_red_gain = cr;
        ACAMERA_FSM2CTX_PTR( p_fsm )
            ->stab.global_awb_blue_gain = cb;
        break;
    }
    default:
        rc = -1;
        break;
    }
#endif

    return rc;
}

void awb_acamera_fsm_init( void *fsm, fsm_init_param_t *init_param )
{
    awb_acamera_fsm_t *p_fsm = (awb_acamera_fsm_t *)fsm;
    p_fsm->cmn.p_fsm_mgr = init_param->p_fsm_mgr;
    p_fsm->cmn.isp_base = init_param->isp_base;
    p_fsm->p_fsm_mgr = init_param->p_fsm_mgr;

    awb_acamera_fsm_clear( p_fsm );

    awb_init( p_fsm );
    awb_acamera_fsm_request_interrupt( p_fsm, ACAMERA_IRQ_MASK( ACAMERA_IRQ_AWB_STATS ) );
}

void awb_acamera_fsm_deinit( void *fsm )
{
    awb_acamera_fsm_t *p_fsm = (awb_acamera_fsm_t *)fsm;
    awb_deinit( p_fsm );
}

int awb_acamera_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size )
{
    int rc = 0;
    awb_acamera_fsm_t *p_fsm = (awb_acamera_fsm_t *)fsm;

    switch ( param_id ) {
    case FSM_PARAM_GET_AWB_INFO: {
        if ( !output || output_size != sizeof( fsm_param_awb_info_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        fsm_param_awb_info_t *p_info = (fsm_param_awb_info_t *)output;

        system_memcpy( p_info->wb_log2, p_fsm->wb_log2, sizeof( p_info->wb_log2 ) );
        system_memcpy( p_info->awb_warming, p_fsm->awb_warming, sizeof( p_info->awb_warming ) );

        p_info->temperature_detected = p_fsm->temperature_detected;
        p_info->p_high = p_fsm->p_high;
        p_info->light_source_candidate = p_fsm->light_source_candidate;
        p_info->avg_GR = p_fsm->avg_GR;
        p_info->avg_GB = p_fsm->avg_GB;
        p_info->rg_coef = p_fsm->rg_coef;
        p_info->bg_coef = p_fsm->bg_coef;
        p_info->result_gain_frame_id = p_fsm->cur_result_gain_frame_id;

        break;
    }

    case FSM_PARAM_GET_AWB_MODE:
        if ( !output || output_size != sizeof( uint32_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

#if AWB_MODE_ID
        if ( !ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_manual_awb ) {
            *(uint32_t *)output = AWB_AUTO;
        } else {
            *(uint32_t *)output = p_fsm->mode;
        }
#endif

        break;

    case FSM_PARAM_GET_AWB_ZONE_STATS_INFO: {
        if ( !output || output_size != sizeof( fsm_param_awb_zone_stats_info_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        fsm_param_awb_zone_stats_info_t *p_info = (fsm_param_awb_zone_stats_info_t *)output;
        p_info->zone_stats = (uint32_t *)p_fsm->awb_zones;
        p_info->zone_size = sizeof( p_fsm->awb_zones );

        break;
    }

    default:
        rc = -1;
        break;
    }

    return rc;
}

int awb_acamera_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size )
{
    int rc = 0;
    awb_acamera_fsm_t *p_fsm = (awb_acamera_fsm_t *)fsm;

    switch ( param_id ) {
#if defined( ISP_HAS_SBUF_FSM ) || defined( ISP_HAS_USER2KERNEL_FSM )
    case FSM_PARAM_SET_AWB_STATS: {
        if ( !input || input_size != sizeof( sbuf_awb_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        sbuf_awb_t *p_sbuf_awb = (sbuf_awb_t *)input;

        p_fsm->curr_AWB_ZONES = p_sbuf_awb->curr_AWB_ZONES;
        system_memcpy( p_fsm->awb_zones, p_sbuf_awb->stats_data, sizeof( p_fsm->awb_zones ) );
        fsm_raise_event( p_fsm, event_id_awb_stats_ready );

        // AWB algorithm needs frame_end event to back into ready state after calculation
        fsm_raise_event( p_fsm, event_id_frame_end );

        break;
    }
#endif

    case FSM_PARAM_SET_AWB_MODE:
        if ( !input || input_size != sizeof( uint32_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        rc = AWB_set_mode( p_fsm, *(uint32_t *)input );

        break;

    case FSM_PARAM_SET_AWB_INFO: {
        if ( !input || input_size != sizeof( fsm_param_awb_info_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        fsm_param_awb_info_t *p_info = (fsm_param_awb_info_t *)input;

        if ( p_info->flag & AWB_INFO_TEMPERATURE_DETECTED ) {
            p_fsm->temperature_detected = p_info->temperature_detected;
        }

        if ( p_info->flag & AWB_INFO_P_HIGH ) {
            p_fsm->p_high = p_info->p_high;
        }

        if ( p_info->flag & AWB_INFO_LIGHT_SOURCE_CANDIDATE ) {
            p_fsm->light_source_candidate = p_info->light_source_candidate;
        }

        break;
    }

    case FSM_PARAM_SET_AWB_FRAMEID:

        awb_set_frameid( p_fsm, *(int32_t *)input );

        break;

    case FSM_PARAM_SET_AWB_PRESET:
        if ( !input || input_size != sizeof( isp_awb_preset_t ) ) {
            LOG( LOG_ERR, "Invalid param, param_id: %d.", param_id );
            rc = -1;
            break;
        }

        isp_awb_preset_t *p_new = (isp_awb_preset_t *)input;

		awb_prev_param( p_fsm, p_new );

        break;

    default:
        rc = -1;
        break;
    }

    return rc;
}

uint8_t awb_acamera_fsm_process_event( awb_acamera_fsm_t *p_fsm, event_id_t event_id )
{
    uint8_t b_event_processed = 0;
    switch ( event_id ) {
    default:
        break;
    case event_id_frame_end:
        if ( !p_fsm->is_ready ) {
            awb_acamera_fsm_request_interrupt( p_fsm, ACAMERA_IRQ_MASK( ACAMERA_IRQ_AWB_STATS ) );
            b_event_processed = 1;

            p_fsm->is_ready = 1;
        }
        break;
    case event_id_awb_stats_ready:
        if ( p_fsm->is_ready ) {
            awb_process_stats( p_fsm );
            awb_process_light_source( p_fsm );
            awb_normalise( p_fsm );
            awb_acamera_fsm_request_interrupt( p_fsm, ACAMERA_IRQ_MASK( ACAMERA_IRQ_FRAME_END ) );
            b_event_processed = 1;

            p_fsm->is_ready = 0;
        }
        break;
    }

    return b_event_processed;
}
