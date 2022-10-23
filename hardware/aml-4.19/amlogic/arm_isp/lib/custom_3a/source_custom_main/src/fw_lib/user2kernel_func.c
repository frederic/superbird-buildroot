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

#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include "acamera_firmware_config.h"
#include "acamera_command_api.h"
#include "system_timer.h"
#include "system_stdlib.h"
#include "acamera_fw.h"
#include "acamera.h"
#include "sbuf.h"
#include "user2kernel_fsm.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_USER2KERNEL
#endif

struct sbuf_map_context {
    user2kernel_fsm_ptr_t p_fsm;

    /* device */
    int fw_id;
    int fd_dev;
    int fd_opened;
    char dev_path[SBUF_DEV_PATH_LEN];

    /* fw_sbuf mapping */
    void *map_base;
    unsigned long map_len;
    struct fw_sbuf *fw_sbuf;
    uint32_t cur_wdr_mode;

    /* R/W to kernel-FW */

    // this array is for one frame, assume each item just has one valid sbuf_type,
    // the sbuf_type should be consumed before the next same sbuf_type comes
    pthread_mutex_t mutex;
    struct sbuf_idx_set idx_set_arr[SBUF_TYPE_MAX];
    // position in array for next read operation
    uint8_t read_pos;
    // position in array for next write operation
    uint8_t write_pos;

    unsigned int read_skipped;

    /* thread used to wait for new data from kernel-FW */
    pthread_t thread_u2k;
    int thread_u2k_stop;
};

static struct sbuf_map_context sbuf_map_contexts[FIRMWARE_CONTEXT_NUMBER];


int is_sbuf_idx_invalid( uint32_t buf_idx )
{
    return ( buf_idx >= SBUF_STATS_ARRAY_SIZE ) ? 1 : 0;
}

void *get_sbuf_addr( enum sbuf_type buf_type, uint32_t buf_idx, struct fw_sbuf *p_fw_sbuf )
{
    void *buf = NULL;

    if ( ( buf_type >= SBUF_TYPE_MAX ) ||
         ( buf_idx >= SBUF_STATS_ARRAY_SIZE ) ) {
        LOG( LOG_ERR, "get sbuf addr failed: Invalid param, buf_type: %u, buf_idx: %u.", buf_type, buf_idx );
        return NULL;
    }

    switch ( buf_type ) {
#if ( ISP_HAS_AE_BALANCED_FSM || ISP_HAS_AE_ACAMERA_FSM )
    case SBUF_TYPE_AE:
        buf = &( p_fw_sbuf->ae_sbuf[buf_idx] );
        break;
#endif

#if ( ISP_HAS_AWB_MESH_FSM || ISP_HAS_AWB_MESH_NBP_FSM || ISP_HAS_AWB_ACAMERA_FSM )
    case SBUF_TYPE_AWB:
        buf = &( p_fw_sbuf->awb_sbuf[buf_idx] );
        break;
#endif

#if ( ISP_HAS_AF_LMS_FSM || ISP_HAS_AF_ACAMERA_FSM )
    case SBUF_TYPE_AF:
        buf = &( p_fw_sbuf->af_sbuf[buf_idx] );
        break;
#endif

#if ISP_HAS_GAMMA_CONTRAST_FSM || ISP_HAS_GAMMA_ACAMERA_FSM
    case SBUF_TYPE_GAMMA:
        buf = &( p_fw_sbuf->gamma_sbuf[buf_idx] );
        break;
#endif

#if ISP_HAS_IRIDIX_HIST_FSM || ISP_HAS_IRIDIX8_FSM || ISP_HAS_IRIDIX_ACAMERA_FSM
    case SBUF_TYPE_IRIDIX:
        buf = &( p_fw_sbuf->iridix_sbuf[buf_idx] );
        break;
#endif

    default:
        LOG( LOG_ERR, "Error: Unsupported buf_tye: %u.", buf_type );
        break;
    }

    LOG( LOG_DEBUG, "system get sbuf addr OK, buf_idx: %u, buf_type: %u, buf_addr: %p.", buf_idx, buf_type, buf );

    return buf;
}


#if ISP_HAS_AE_BALANCED_FSM || ISP_HAS_AE_ACAMERA_FSM

static int update_stats_ae( user2kernel_fsm_ptr_t p_fsm, uint32_t ae_buf_idx )
{
    sbuf_ae_t *p_sbuf_ae;
    uint32_t len;

    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;

    if ( is_sbuf_idx_invalid( ae_buf_idx ) ) {
        LOG( LOG_INFO, "ae_buf_idx(%u) is invalid, return", ae_buf_idx );
        return 0;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    LOG( LOG_DEBUG, "AE +++, ae_buf_idx: %u.", ae_buf_idx );

    p_sbuf_ae = (sbuf_ae_t *)get_sbuf_addr( SBUF_TYPE_AE, ae_buf_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_ae ) {
        LOG( LOG_CRIT, "AE --- Fatal error: get ae sbuf failed, idx: %u, addr: %p.", ae_buf_idx, p_sbuf_ae );
        return -1;
    }

    LOG( LOG_DEBUG, "get ae sbuf OK, idx: %u, addr: %p, len: %u, histsum: %u.", ae_buf_idx, p_sbuf_ae, len, p_sbuf_ae->histogram_sum );

    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_AE_STATS, p_sbuf_ae, sizeof( sbuf_ae_t ) );

    LOG( LOG_DEBUG, "AE ---." );

    return 0;
}

static void update_result_ae( user2kernel_fsm_ptr_t p_fsm, uint8_t ae_idx )
{
    fsm_param_ae_info_t ae_info;

    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;
    sbuf_ae_t *p_sbuf_ae;

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( is_sbuf_idx_invalid( ae_idx ) ) {
        LOG( LOG_INFO, "ae_buf_idx(%u) is invalid, return", ae_idx );
        return;
    }

    p_sbuf_ae = (sbuf_ae_t *)get_sbuf_addr( SBUF_TYPE_AE, ae_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_ae ) {
        LOG( LOG_CRIT, "Fatal error: get ae sbuf failed, idx: %u.", ae_idx );
        return;
    }

    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_AE_INFO, NULL, 0, &ae_info, sizeof( ae_info ) );

    p_sbuf_ae->ae_exposure = ae_info.exposure_log2;
    p_sbuf_ae->ae_exposure_ratio = ae_info.exposure_ratio;

    LOG( LOG_DEBUG, "Updating AE: ctx: %d, AE exposure_log2: %d, exposure_ratio: %u.", fw_id, p_sbuf_ae->ae_exposure, p_sbuf_ae->ae_exposure_ratio );

    return;
}

#endif


#if ISP_HAS_AWB_MESH_NBP_FSM || ISP_HAS_AWB_ACAMERA_FSM
static int update_stats_awb( user2kernel_fsm_ptr_t p_fsm, uint32_t awb_buf_idx )
{
    sbuf_awb_t *p_sbuf_awb;

    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;

    if ( is_sbuf_idx_invalid( awb_buf_idx ) ) {
        LOG( LOG_INFO, "awb_buf_idx(%u) is invalid, return", awb_buf_idx );
        return 0;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    LOG( LOG_DEBUG, "AWB +++, awb_buf_idx: %u.", awb_buf_idx );

    p_sbuf_awb = (sbuf_awb_t *)get_sbuf_addr( SBUF_TYPE_AWB, awb_buf_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_awb ) {
        LOG( LOG_CRIT, "AWB --- Fatal error: get awb sbuf failed, idx: %u, addr: %p.", awb_buf_idx );
        return -1;
    }

    LOG( LOG_DEBUG, "get awb sbuf OK, idx: %u, addr: %p, curr_AWB_ZONES: %u.", awb_buf_idx, p_sbuf_awb, p_sbuf_awb->curr_AWB_ZONES );

    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_AWB_STATS, p_sbuf_awb, sizeof( sbuf_awb_t ) );

    LOG( LOG_DEBUG, "AWB ---." );

    return 0;
}

static void update_result_awb( user2kernel_fsm_ptr_t p_fsm, uint8_t awb_idx )
{
    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;
    sbuf_awb_t *p_sbuf_awb;

    fsm_param_awb_info_t wb_info;

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( is_sbuf_idx_invalid( awb_idx ) ) {
        LOG( LOG_INFO, "awb_buf_idx(%u) is invalid, return", awb_idx );
        return;
    }

    p_sbuf_awb = (sbuf_awb_t *)get_sbuf_addr( SBUF_TYPE_AWB, awb_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_awb ) {
        LOG( LOG_CRIT, "Fatal error: get awb sbuf failed, idx: %u.", awb_idx );
        return;
    }

    // update awb_red_gain & awb_blue_gain
    p_sbuf_awb->awb_red_gain = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_awb_red_gain;
    p_sbuf_awb->awb_blue_gain = ACAMERA_FSM2CTX_PTR( p_fsm )->stab.global_awb_blue_gain;

    // update wb_info
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_AWB_INFO, NULL, 0, &wb_info, sizeof( wb_info ) );
    p_sbuf_awb->p_high = wb_info.p_high;
    p_sbuf_awb->temperature_detected = wb_info.temperature_detected;
    p_sbuf_awb->light_source_candidate = wb_info.light_source_candidate;
    system_memcpy( p_sbuf_awb->awb_warming, wb_info.awb_warming, sizeof( p_sbuf_awb->awb_warming ) );

    status_info_param_t *p_status_info = (status_info_param_t *)_GET_UINT_PTR( ACAMERA_FSM2CTX_PTR( p_fsm ), CALIBRATION_STATUS_INFO );
    p_sbuf_awb->mix_light_contrast = p_status_info->awb_info.mix_light_contrast;

    LOG( LOG_DEBUG, "Updating AWB: ctx: %d, red_gain: %u, blue_gain: %u, p_high: %d, temperature_detected: %d, light_src_candidate: %d.",
         fw_id,
         p_sbuf_awb->awb_red_gain,
         p_sbuf_awb->awb_blue_gain,
         p_sbuf_awb->p_high,
         p_sbuf_awb->temperature_detected,
         p_sbuf_awb->light_source_candidate );

    return;
}
#endif

#if ( ISP_HAS_AF_LMS_FSM || ISP_HAS_AF_ACAMERA_FSM )
static int update_stats_af( user2kernel_fsm_ptr_t p_fsm, uint32_t af_buf_idx )
{
    sbuf_af_t *p_sbuf_af;

    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;

    if ( is_sbuf_idx_invalid( af_buf_idx ) ) {
        LOG( LOG_INFO, "af_buf_idx(%u) is invalid, return", af_buf_idx );
        return 0;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    LOG( LOG_DEBUG, "AF +++, af_buf_idx: %u.", af_buf_idx );

    p_sbuf_af = (sbuf_af_t *)get_sbuf_addr( SBUF_TYPE_AF, af_buf_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_af ) {
        LOG( LOG_CRIT, "AF --- Fatal error: get AF sbuf failed, idx: %u, addr: %p.", af_buf_idx );
        return -1;
    }

    LOG( LOG_DEBUG, "get af sbuf OK, idx: %u, addr: %p, frame_num: %u.", af_buf_idx, p_sbuf_af, p_sbuf_af->frame_num );

    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_AF_STATS, p_sbuf_af, sizeof( sbuf_af_t ) );

    // Used when ACAMERA_ISP_PROFILING is defined for user-FW.
    ACAMERA_FSM2CTX_PTR( p_fsm )
        ->frame++;
    LOG( LOG_DEBUG, "frame: %d.", ACAMERA_FSM2CTX_PTR( p_fsm )->frame );

    LOG( LOG_DEBUG, "AF ---." );

    return 0;
}

static void update_result_af( user2kernel_fsm_ptr_t p_fsm, uint8_t af_idx )
{
    fsm_param_af_info_t af_info;

    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;
    sbuf_af_t *p_sbuf_af = NULL;

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( is_sbuf_idx_invalid( af_idx ) ) {
        LOG( LOG_INFO, "af_buf_idx(%u) is invalid, return", af_idx );
        return;
    }

    p_sbuf_af = (sbuf_af_t *)get_sbuf_addr( SBUF_TYPE_AF, af_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_af ) {
        LOG( LOG_CRIT, "Fatal error: get af sbuf failed, idx: %u.", af_idx );
        return;
    }

    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_AF_INFO, NULL, 0, &af_info, sizeof( af_info ) );

    p_sbuf_af->af_position = af_info.last_position;
    p_sbuf_af->frame_to_skip = af_info.skip_frame;
    p_sbuf_af->af_last_sharp = af_info.last_sharp;

    LOG( LOG_INFO, "Updating AF: ctx: %d, pos_manual: %u, last_sharp: %d.", fw_id, p_sbuf_af->af_position, p_sbuf_af->af_last_sharp );

    return;
}
#endif

#if ISP_HAS_GAMMA_CONTRAST_FSM || ISP_HAS_GAMMA_ACAMERA_FSM
static int update_stats_gamma( user2kernel_fsm_ptr_t p_fsm, uint32_t gamma_buf_idx )
{
    sbuf_gamma_t *p_sbuf_gamma;

    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;

    if ( is_sbuf_idx_invalid( gamma_buf_idx ) ) {
        LOG( LOG_INFO, "gamma_buf_idx(%u) is invalid, return", gamma_buf_idx );
        return 0;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    LOG( LOG_DEBUG, "GAMMA +++, gamma_buf_idx: %u.", gamma_buf_idx );

    p_sbuf_gamma = (sbuf_gamma_t *)get_sbuf_addr( SBUF_TYPE_GAMMA, gamma_buf_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_gamma ) {
        LOG( LOG_CRIT, "Gamma --- Fatal error: get Gamma sbuf failed, idx: %u, addr: %p.", gamma_buf_idx );
        return -1;
    }

    LOG( LOG_DEBUG, "get gamma sbuf OK, idx: %u, addr: %p, fullhist_sum: %u.", gamma_buf_idx, p_sbuf_gamma, p_sbuf_gamma->fullhist_sum );

    acamera_fsm_mgr_set_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_SET_GAMMA_STATS, p_sbuf_gamma, sizeof( sbuf_gamma_t ) );

    LOG( LOG_DEBUG, "GAMMA ---." );

    return 0;
}

static void update_result_gamma( user2kernel_fsm_ptr_t p_fsm, uint8_t gamma_idx )
{
    fsm_param_gamma_result_t gamma_result;

    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;
    sbuf_gamma_t *p_sbuf_gamma;

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( is_sbuf_idx_invalid( gamma_idx ) ) {
        LOG( LOG_INFO, "gamma_buf_idx(%u) is invalid, return", gamma_idx );
        return;
    }

    p_sbuf_gamma = (sbuf_gamma_t *)get_sbuf_addr( SBUF_TYPE_GAMMA, gamma_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_gamma ) {
        LOG( LOG_CRIT, "Fatal error: get gamma sbuf failed, idx: %u.", gamma_idx );
        return;
    }
    LOG( LOG_DEBUG, "Updating gamma lut: get gamma lut sbuf OK, idx: %u, addr: %p.", gamma_idx, p_sbuf_gamma );

    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_GAMMA_CONTRAST_RESULT, NULL, 0, &gamma_result, sizeof( fsm_param_gamma_result_t ) );

    p_sbuf_gamma->gamma_gain = gamma_result.gamma_gain;
    p_sbuf_gamma->gamma_offset = gamma_result.gamma_offset;

    LOG( LOG_DEBUG, "Updating gamma: ctx: %d, gamma gain: %u, offset: %u.",
         fw_id,
         p_sbuf_gamma->gamma_gain,
         p_sbuf_gamma->gamma_offset );

    return;
}
#endif

#if ISP_HAS_IRIDIX_HIST_FSM || ISP_HAS_IRIDIX8_FSM || ISP_HAS_IRIDIX_ACAMERA_FSM
static void update_result_iridix_hist( user2kernel_fsm_ptr_t p_fsm, uint8_t iridix_idx )
{
    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;
    sbuf_iridix_t *p_sbuf_iridix;
    uint16_t cur_iridix_strength = 0;
    uint32_t cur_iridix_contrast = 256;
    uint16_t cur_iridix_dark_enh = 0;
    uint16_t cur_iridix_global_DG = 0;


    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( is_sbuf_idx_invalid( iridix_idx ) ) {
        LOG( LOG_INFO, "iridix_buf_idx(%u) is invalid, return", iridix_idx );
        return;
    }

    p_sbuf_iridix = (sbuf_iridix_t *)get_sbuf_addr( SBUF_TYPE_IRIDIX, iridix_idx, p_ctx->fw_sbuf );
    if ( !p_sbuf_iridix ) {
        LOG( LOG_CRIT, "Fatal error: get iridix sbuf failed, idx: %u.", iridix_idx );
        return;
    }

    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_IRIDIX_STRENGTH, NULL, 0, &cur_iridix_strength, sizeof( cur_iridix_strength ) );
#if defined( ISP_HAS_IRIDIX8_FSM ) || defined( ISP_HAS_IRIDIX_ACAMERA_FSM )
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_IRIDIX_CONTRAST, NULL, 0, &cur_iridix_contrast, sizeof( cur_iridix_contrast ) );
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_IRIDIX_DARK_ENH, NULL, 0, &cur_iridix_dark_enh, sizeof( cur_iridix_dark_enh ) );
    acamera_fsm_mgr_get_param( p_fsm->cmn.p_fsm_mgr, FSM_PARAM_GET_IRIDIX_GLOBAL_DG, NULL, 0, &cur_iridix_global_DG, sizeof( cur_iridix_global_DG ) );
#endif

    p_sbuf_iridix->strength_target = cur_iridix_strength;
    p_sbuf_iridix->iridix_contrast = cur_iridix_contrast;
    p_sbuf_iridix->iridix_dark_enh = cur_iridix_dark_enh;
    p_sbuf_iridix->iridix_global_DG = cur_iridix_global_DG;

    LOG( LOG_INFO, "Updating iridix: ctx: %d, iridix_strength: %u,%u, %u,%u", p_sbuf_iridix->frame_id, p_sbuf_iridix->strength_target,cur_iridix_contrast, cur_iridix_dark_enh,cur_iridix_global_DG );

    return;
}
#endif

void user2kernel_fsm_process_interrupt( user2kernel_fsm_const_ptr_t p_fsm, uint8_t irq_event )
{

    switch ( irq_event ) {
    case ACAMERA_IRQ_FRAME_END:
        //user2kernel_update_stats(p_fsm);
        //fsm_raise_event(p_fsm, event_id_user_data_ready );
        LOG( LOG_ERR, "not expected INT" );
        break;
    }
}

static void update_stats( user2kernel_fsm_ptr_t p_fsm, struct sbuf_idx_set *p_set )
{
#if ISP_HAS_AE_BALANCED_FSM || ISP_HAS_AE_ACAMERA_FSM
    if ( p_set->ae_idx_valid ) {
        update_stats_ae( p_fsm, p_set->ae_idx );
#if ISP_HAS_IRIDIX_HIST_FSM || ISP_HAS_IRIDIX8_FSM || ISP_HAS_IRIDIX_ACAMERA_FSM
        // iridix algorithm depends on ae_stats data, so we just raise the event to
        // notify iridix to have a new calculation.
        if ( p_set->iridix_idx_valid ) {
            fsm_raise_event( p_fsm, event_id_update_iridix );
        }
#endif
    }
#endif

#if ISP_HAS_AWB_MESH_NBP_FSM || ISP_HAS_AWB_ACAMERA_FSM
    if ( p_set->awb_idx_valid ) {
        update_stats_awb( p_fsm, p_set->awb_idx );
    }
#endif

#if ( ISP_HAS_AF_LMS_FSM || ISP_HAS_AF_ACAMERA_FSM )
    if ( p_set->af_idx_valid ) {
        update_stats_af( p_fsm, p_set->af_idx );
    }
#endif

#if ISP_HAS_GAMMA_CONTRAST_FSM || ISP_HAS_GAMMA_ACAMERA_FSM
    if ( p_set->gamma_idx_valid ) {
        update_stats_gamma( p_fsm, p_set->gamma_idx );
    }
#endif

    // notify user2kernel FSM to switch to state and update result to kernel-FW
    fsm_raise_event( p_fsm, event_id_user_data_ready );
}

static void *thread_func_user2kernel( void *param )
{
    int rc;
    struct sbuf_idx_set tmp_set;
    uint32_t len_to_read = sizeof( struct sbuf_idx_set );
    struct sbuf_map_context *p_ctx = (struct sbuf_map_context *)param;

    LOG( LOG_INFO, "ctx: %d, Enter +++", p_ctx->fw_id );

    while ( !p_ctx->thread_u2k_stop ) {
        struct sbuf_idx_set *p_cur_set = NULL;
        uint8_t cur_read_pos = SBUF_TYPE_MAX;
        uint8_t cur_write_pos = SBUF_TYPE_MAX;

        // reset before read
        memset( &tmp_set, 0, len_to_read );

        // check whether it's paused before read
        if ( p_ctx->p_fsm->is_paused ) {
            LOG( LOG_INFO, "paused, sleep and continue" );
            system_timer_usleep( 100 * 1000 ); // sleep 100 ms
            continue;
        }

        // use a tmp_set to read from kernel-FW in case we read out no valid item,
        // and we just to continue read but don't need to manage the read_pos and
        // write_pos in p_ctx idx_set_arr[].
        rc = read( p_ctx->fd_dev, &tmp_set, len_to_read );
        if ( rc != len_to_read ) {
            LOG( LOG_ERR, "ctx: %d, read failed, rc: %d, expected len: %d", p_ctx->fw_id, rc, len_to_read );
            system_timer_usleep( 10 * 1000 ); // sleep 1 ms
            continue;
        }

        // we can't do infinite wait reading because we need to break this while loop
        // when required to exit thread.
        if ( !is_idx_set_has_valid_item( &tmp_set ) ) {
            LOG( LOG_INFO, "ctx: %d, no valid idx_set item read out, continue", p_ctx->fw_id );
            continue;
        }

        // check whether it's paused, if it's, we just need to return the sbuf and continue
        if ( p_ctx->p_fsm->is_paused ) {
            rc = write( p_ctx->fd_dev, &tmp_set, len_to_read );
            LOG( LOG_INFO, "paused, return idx_set and continue, rc: %d.", rc );
            p_ctx->read_skipped++;
            continue;
        }

        pthread_mutex_lock( &p_ctx->mutex );
        // check whether we'll overwrite the write_pos item.
        if ( p_ctx->read_pos == p_ctx->write_pos ) {
            LOG( LOG_ERR, "Error: ctx: %d, read_pos(%u) catch up write_pos(%u), discard data.", p_ctx->fw_id, p_ctx->read_pos, p_ctx->write_pos );
            cur_read_pos = SBUF_TYPE_MAX;
        } else {
            cur_read_pos = p_ctx->read_pos++;
            // wrap to the beginning of the array to prepare next read operation.
            if ( p_ctx->read_pos >= SBUF_TYPE_MAX ) {
                p_ctx->read_pos = 0;
            }

            // copy the tmp_set to the destination
            p_cur_set = &p_ctx->idx_set_arr[cur_read_pos];
            memcpy( p_cur_set, &tmp_set, len_to_read );

            // if write_pos is not used, use it
            if ( SBUF_TYPE_MAX == p_ctx->write_pos ) {
                p_ctx->write_pos = cur_read_pos;
            }

            cur_write_pos = p_ctx->write_pos;
        }
        LOG( LOG_DEBUG, "ctx: %d, cur_read_pos: %u, cur_write_pos: %u, read_pos: %u, write_pos: %u", p_ctx->fw_id, cur_read_pos, cur_write_pos, p_ctx->read_pos, p_ctx->write_pos );
        pthread_mutex_unlock( &p_ctx->mutex );

        if ( SBUF_TYPE_MAX == cur_read_pos ) {
            // the changed_flag in the valid sbuf item should be 0 since no one set it,
            // so this write operation just return sbuf to kernel-FW and doesn't change
            // any ISP or sensor settings.
            rc = write( p_ctx->fd_dev, &tmp_set, len_to_read );
            p_ctx->read_skipped++;
            LOG( LOG_ERR, "ctx: %d, no space to save, skip idx_set, count: %d, rc: %d, continue", p_ctx->fw_id, p_ctx->read_skipped, rc );
            continue;
        }

        LOG( LOG_INFO, "ctx: %d, read idx_set: %u(%u)-%u(%u)-%u(%u)-%u(%u)-%u(%u).",
             p_ctx->fw_id,
             p_cur_set->ae_idx_valid, p_cur_set->ae_idx,
             p_cur_set->awb_idx_valid, p_cur_set->awb_idx,
             p_cur_set->af_idx_valid, p_cur_set->af_idx,
             p_cur_set->gamma_idx_valid, p_cur_set->gamma_idx,
             p_cur_set->iridix_idx_valid, p_cur_set->iridix_idx );

        // only call update_stats() function when only 1 item in array to be handled.
        if ( cur_write_pos == cur_read_pos ) {
            update_stats( p_ctx->p_fsm, p_cur_set );
        }
    }

    LOG( LOG_INFO, "ctx: %d, Exit ---", p_ctx->fw_id );

    return NULL;
}

int user2kernel_initialize( user2kernel_fsm_ptr_t p_fsm )
{
    int32_t rc = 0;
    uint32_t fw_id = p_fsm->cmn.ctx_id;
    struct sbuf_map_context *p_ctx = NULL;

    LOG( LOG_INFO, "init user2kernel FSM for fw_id: %d.", fw_id );

    if ( fw_id >= acamera_get_context_number() ) {
        LOG( LOG_CRIT, "Fatal error: Invalid FW context ID: %d, max is: %d", fw_id, acamera_get_context_number() - 1 );
        return -1;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );
    memset( p_ctx, 0, sizeof( struct sbuf_map_context ) );
    p_ctx->fw_id = fw_id;
    p_ctx->cur_wdr_mode = 0xFFFF; // Invalid wdr_mode
    snprintf( p_ctx->dev_path, SBUF_DEV_PATH_LEN, SBUF_DEV_PATH_FORMAT, fw_id );
    p_ctx->fd_dev = open( p_ctx->dev_path, O_RDWR | O_SYNC );
    if ( p_ctx->fd_dev != -1 ) {
        LOG( LOG_CRIT, "ISP device successfully opened %s p_ctx->fd_dev:0x%x", p_ctx->dev_path, p_ctx->fd_dev );
    } else {
        LOG( LOG_CRIT, "Failed to open device %s", p_ctx->dev_path );
        return -2;
    }

    p_ctx->map_len = sizeof( struct fw_sbuf );
    LOG( LOG_INFO, "mmap request size: %lu.", p_ctx->map_len );
    p_ctx->map_base = mmap( NULL, p_ctx->map_len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, p_ctx->fd_dev, 0 );
    if ( p_ctx->map_base == (void *)MAP_FAILED ) {
        LOG( LOG_CRIT, "sbuf map failed, err: %s.", strerror( errno ) );
        close( p_ctx->fd_dev );
        p_ctx->fd_dev = -1;
        return -3;
    } else {
        LOG( LOG_INFO, "sbuf map OK, base: %p.", p_ctx->map_base );
        p_ctx->fw_sbuf = (struct fw_sbuf *)p_ctx->map_base;
        p_ctx->fd_opened = 1;
    }

    pthread_mutex_init( &p_ctx->mutex, NULL );

    pthread_mutex_lock( &p_ctx->mutex );
    p_ctx->read_pos = 0;
    p_ctx->write_pos = SBUF_TYPE_MAX; // Invalid write position
    p_ctx->p_fsm = p_fsm;
    p_ctx->thread_u2k_stop = 0;
    pthread_mutex_unlock( &p_ctx->mutex );

    rc = pthread_create( &p_ctx->thread_u2k, NULL, thread_func_user2kernel, p_ctx );
    if ( rc ) {
        LOG( LOG_ERR, "create thread failed, err: %s.", strerror( rc ) );
        close( p_ctx->fd_dev );
        p_ctx->fd_dev = -1;
        return -4;
    }

    p_fsm->mask.repeat_irq_mask = ACAMERA_IRQ_MASK( ACAMERA_IRQ_FRAME_END );
    user2kernel_request_interrupt( p_fsm, p_fsm->mask.repeat_irq_mask );

    return 0;
}

void user2kernel_deinit( user2kernel_fsm_ptr_t p_fsm )
{
    uint32_t fw_id = p_fsm->cmn.ctx_id;
    struct sbuf_map_context *p_ctx = NULL;
    LOG( LOG_INFO, "deinit user2kernel FSM for fw_id: %d.", fw_id );

    if ( fw_id >= acamera_get_context_number() ) {
        LOG( LOG_CRIT, "Fatal error: Invalid FW context ID: %d, max is: %d", fw_id, acamera_get_context_number() - 1 );
        return;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );
    //pthread_mutex_lock( &p_ctx->mutex );
    p_ctx->thread_u2k_stop = 1;
    pthread_join( p_ctx->thread_u2k, NULL );
    pthread_mutex_destroy( &p_ctx->mutex );
    if ( p_ctx->fd_dev != -1 ) {
        munmap( p_ctx->map_base, p_ctx->map_len );
        close( p_ctx->fd_dev );
    }
}

void user2kernel_reset( user2kernel_fsm_ptr_t p_fsm )
{
    int rc;
    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;
    uint32_t len_to_write = sizeof( struct sbuf_idx_set );
    uint8_t cur_write_pos = SBUF_TYPE_MAX;
    struct sbuf_idx_set tmp_set;

    p_ctx = &( sbuf_map_contexts[fw_id] );

    LOG( LOG_INFO, "fw_id: %u, p_ctx: %p", fw_id, p_ctx );

    if ( !p_ctx->fd_opened ) {
        LOG( LOG_ERR, "Failed to update the Kernel FW 3A data. Connection channel has not been established" );
        return;
    }

    // get the position for write() function.
    pthread_mutex_lock( &p_ctx->mutex );
    while ( p_ctx->write_pos < SBUF_TYPE_MAX ) {
        LOG( LOG_INFO, "read_pos: %u, write_pos: %u", p_ctx->read_pos, p_ctx->write_pos );
        cur_write_pos = p_ctx->write_pos++;

        // wrap to the beginning if needed.
        if ( p_ctx->write_pos == SBUF_TYPE_MAX ) {
            p_ctx->write_pos = 0;
        }

        // if the two position are the same, that means there is
        // no more items need to be handled.
        if ( p_ctx->write_pos == p_ctx->read_pos ) {
            p_ctx->write_pos = SBUF_TYPE_MAX;
        }

        memcpy( &tmp_set, &p_ctx->idx_set_arr[cur_write_pos], sizeof( tmp_set ) );

        // reset it after used.
        memset( &p_ctx->idx_set_arr[cur_write_pos], 0, sizeof( tmp_set ) );

        LOG( LOG_INFO, "write idx_set: %u(%u)-%u(%u)-%u(%u)-%u(%u)-%u(%u).",
             tmp_set.ae_idx_valid, tmp_set.ae_idx,
             tmp_set.awb_idx_valid, tmp_set.awb_idx,
             tmp_set.af_idx_valid, tmp_set.af_idx,
             tmp_set.gamma_idx_valid, tmp_set.gamma_idx,
             tmp_set.iridix_idx_valid, tmp_set.iridix_idx );

        // the changed_flag in the valid sbuf item should be 0 since no one set it,
        // so this write operation just return sbuf to kernel-FW and doesn't change
        // any ISP or sensor settings.
        rc = write( p_ctx->fd_dev, &tmp_set, len_to_write );
        if ( rc != len_to_write ) {
            LOG( LOG_ERR, "Error: write failed, rc: %d, expcted: %u", rc, len_to_write );
        }
    }
    pthread_mutex_unlock( &p_ctx->mutex );

    return;
}

void user2kernel_process( user2kernel_fsm_ptr_t p_fsm )
{
    int rc;
    struct sbuf_map_context *p_ctx = NULL;
    int fw_id = p_fsm->cmn.ctx_id;
    uint32_t len_to_write = sizeof( struct sbuf_idx_set );
    uint8_t cur_write_pos = SBUF_TYPE_MAX;
    uint8_t next_write_pos = SBUF_TYPE_MAX;
    struct sbuf_idx_set tmp_set;
    uint8_t one_more_item = 0;

    p_ctx = &( sbuf_map_contexts[fw_id] );

    LOG( LOG_DEBUG, "fw_id: %u, p_ctx: %p", fw_id, p_ctx );

    if ( !p_ctx->fd_opened ) {
        LOG( LOG_ERR, "Failed to update the Kernel FW 3A data. Connection channel has not been established" );
        return;
    }

    // get the position for write() function.
    pthread_mutex_lock( &p_ctx->mutex );
    if ( p_ctx->write_pos < SBUF_TYPE_MAX ) {
        LOG( LOG_DEBUG, "read_pos: %u, write_pos: %u", p_ctx->read_pos, p_ctx->write_pos );
        cur_write_pos = p_ctx->write_pos++;

        // wrap to the beginning if needed.
        if ( p_ctx->write_pos == SBUF_TYPE_MAX ) {
            p_ctx->write_pos = 0;
        }

        // if the two position are the same, that means there is
        // no more items need to be handled.
        if ( p_ctx->write_pos == p_ctx->read_pos ) {
            p_ctx->write_pos = SBUF_TYPE_MAX;
        }

        memcpy( &tmp_set, &p_ctx->idx_set_arr[cur_write_pos], sizeof( tmp_set ) );

        // reset it after used.
        memset( &p_ctx->idx_set_arr[cur_write_pos], 0, sizeof( tmp_set ) );

        next_write_pos = p_ctx->write_pos;
    }
    pthread_mutex_unlock( &p_ctx->mutex );

    if ( SBUF_TYPE_MAX == cur_write_pos ) {
        LOG( LOG_ERR, "Error: no item to handle, not expected." );
        return;
    }

#if ISP_HAS_AE_BALANCED_FSM || ISP_HAS_AE_ACAMERA_FSM
    if ( tmp_set.ae_idx_valid ) {
        update_result_ae( p_fsm, tmp_set.ae_idx );
#if ISP_HAS_IRIDIX_HIST_FSM || ISP_HAS_IRIDIX8_FSM || ISP_HAS_IRIDIX_ACAMERA_FSM
        if ( tmp_set.iridix_idx_valid ) {
            update_result_iridix_hist( p_fsm, tmp_set.iridix_idx );
        }
#endif
    }
#endif

#if ISP_HAS_AWB_MESH_NBP_FSM || ISP_HAS_AWB_ACAMERA_FSM
    if ( tmp_set.awb_idx_valid ) {
        update_result_awb( p_fsm, tmp_set.awb_idx );
    }
#endif

#if ( ISP_HAS_AF_LMS_FSM || ISP_HAS_AF_ACAMERA_FSM )
    if ( tmp_set.af_idx_valid ) {
        update_result_af( p_fsm, tmp_set.af_idx );
    }
#endif

#if ISP_HAS_GAMMA_CONTRAST_FSM || ISP_HAS_GAMMA_ACAMERA_FSM
    if ( tmp_set.gamma_idx_valid ) {
        update_result_gamma( p_fsm, tmp_set.gamma_idx );
    }
#endif

    LOG( LOG_INFO, "ctx: %d, write idx_set: %u(%u)-%u(%u)-%u(%u)-%u(%u)-%u(%u).",
         p_ctx->fw_id,
         tmp_set.ae_idx_valid, tmp_set.ae_idx,
         tmp_set.awb_idx_valid, tmp_set.awb_idx,
         tmp_set.af_idx_valid, tmp_set.af_idx,
         tmp_set.gamma_idx_valid, tmp_set.gamma_idx,
         tmp_set.iridix_idx_valid, tmp_set.iridix_idx );

    /* finally we write to kernel-FW at once */
    rc = write( p_ctx->fd_dev, &tmp_set, len_to_write );
    if ( rc != len_to_write ) {
        LOG( LOG_ERR, "Error: write failed, rc: %d, expcted: %u", rc, len_to_write );
    }

    // try to handle one more item if there is one.
    memset( &tmp_set, 0, sizeof( tmp_set ) );

    if ( next_write_pos != SBUF_TYPE_MAX ) {
        pthread_mutex_lock( &p_ctx->mutex );
        if ( p_ctx->write_pos != SBUF_TYPE_MAX ) {
            LOG( LOG_DEBUG, "read_pos: %u, write_pos: %u", p_ctx->read_pos, p_ctx->write_pos );

            // here we don't need to upate write_pos, because it'll be updated when it
            // going to real write this tmp_set into kernel-FW, which happened in next
            // round function call of this function.
            memcpy( &tmp_set, &p_ctx->idx_set_arr[p_ctx->write_pos], sizeof( tmp_set ) );
            one_more_item = 1;

            LOG( LOG_INFO, "one more item, idx_set: %u(%u)-%u(%u)-%u(%u)-%u(%u)-%u(%u).",
                 tmp_set.ae_idx_valid, tmp_set.ae_idx,
                 tmp_set.awb_idx_valid, tmp_set.awb_idx,
                 tmp_set.af_idx_valid, tmp_set.af_idx,
                 tmp_set.gamma_idx_valid, tmp_set.gamma_idx,
                 tmp_set.iridix_idx_valid, tmp_set.iridix_idx );
        }
        pthread_mutex_unlock( &p_ctx->mutex );

        // re-trigger the next time's update process
        if ( one_more_item ) {
            update_stats( p_fsm, &tmp_set );
        }
    }
}

int32_t user2kernel_get_klens_status( int fw_id )
{
    int32_t rc = 0;
    struct sbuf_map_context *p_ctx = NULL;

    if ( fw_id >= acamera_get_context_number() ) {
        LOG( LOG_ERR, "Error: Invalid fw_id: %d, max is: %d.", fw_id, acamera_get_context_number() - 1 );
        return rc;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( !p_ctx->fd_opened ) {
        LOG( LOG_ERR, "Error: Connection channel has not been established" );
        return rc;
    }

    rc = p_ctx->fw_sbuf->kf_info.af_info.lens_driver_ok;
    LOG( LOG_INFO, "fw_id: %d, af_info lens_driver_ok: %d.", fw_id, rc );

    return rc;
}

int32_t user2kernel_get_klens_param( int fw_id, lens_param_t *p_lens_param )
{
    int32_t rc = 0;
    struct sbuf_map_context *p_ctx = NULL;

    if ( fw_id >= acamera_get_context_number() ) {
        LOG( LOG_ERR, "Error: Invalid fw_id: %d, max is: %d.", fw_id, acamera_get_context_number() - 1 );
        return rc;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( !p_ctx->fd_opened ) {
        LOG( LOG_ERR, "Error: Connection channel has not been established" );
        return rc;
    }

    *p_lens_param = p_ctx->fw_sbuf->kf_info.af_info.lens_param;
    LOG( LOG_INFO, "fw_id: %d, af_info lens_type: %d, lens_min_step: %d.", fw_id, p_lens_param->lens_type, p_lens_param->min_step );

    return rc;
}

int32_t user2kernel_get_ksensor_info( int fw_id, struct sensor_info *p_sensor_info )
{
    int32_t rc = 0;
    struct sbuf_map_context *p_ctx = NULL;

    if ( fw_id >= acamera_get_context_number() ) {
        LOG( LOG_ERR, "Error: Invalid fw_id: %d, max is: %d.", fw_id, acamera_get_context_number() - 1 );
        return rc;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( !p_ctx->fd_opened ) {
        LOG( LOG_ERR, "Error: Connection channel has not been established" );
        return rc;
    }

    *p_sensor_info = p_ctx->fw_sbuf->kf_info.sensor_info;
    LOG( LOG_INFO, "fw_id: %d, sensor_info modes_num: %d.", fw_id, p_sensor_info->modes_num );

    return rc;
}

int32_t user2kernel_get_max_exposure_log2( int fw_id )
{
    int32_t rc = 0;
    struct sbuf_map_context *p_ctx = NULL;

    if ( fw_id >= acamera_get_context_number() ) {
        LOG( LOG_ERR, "Error: Invalid fw_id: %d, max is: %d.", fw_id, acamera_get_context_number() - 1 );
        return rc;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( !p_ctx->fd_opened ) {
        LOG( LOG_ERR, "Error: Connection channel has not been established" );
        return rc;
    }

    rc = p_ctx->fw_sbuf->kf_info.cmos_info.max_exposure_log2;
    LOG( LOG_INFO, "fw_id: %d, cmos_info max_exposure_log2: %d.", fw_id, rc );

    return rc;
}

int32_t user2kernel_get_total_gain_log2( int fw_id )
{
    int32_t rc = 0;
    struct sbuf_map_context *p_ctx = NULL;

    if ( fw_id >= acamera_get_context_number() ) {
        LOG( LOG_ERR, "Error: Invalid fw_id: %d, max is: %d.", fw_id, acamera_get_context_number() - 1 );
        return rc;
    }

    p_ctx = &( sbuf_map_contexts[fw_id] );

    if ( !p_ctx->fd_opened ) {
        LOG( LOG_ERR, "Error: Connection channel has not been established" );
        return rc;
    }

    rc = p_ctx->fw_sbuf->kf_info.cmos_info.total_gain_log2;
    LOG( LOG_INFO, "fw_id: %d, cmos_info total_gain_log2: %d.", fw_id, rc );

    return rc;
}

uint8_t user2kernel_get_calibrations( uint32_t ctx_id, void *sensor_arg, ACameraCalibrations *c )
{
    uint32_t idx;
    uint32_t data_offset = 0;
    struct sbuf_map_context *p_ctx = NULL;

    LOG( LOG_ERR, "+++ get_calibration for ctx_id: %d.", ctx_id );

    if ( ctx_id >= FIRMWARE_CONTEXT_NUMBER ) {
        LOG( LOG_ERR, "Wrong ctx_id: %d to get_calibration, max: %d.", ctx_id, FIRMWARE_CONTEXT_NUMBER - 1 );
        return -1;
    }

    if ( !sensor_arg ) {
        LOG( LOG_ERR, "calibration sensor_arg is NULL" );
        return -2;
    }

    p_ctx = &( sbuf_map_contexts[ctx_id] );

    int32_t new_wdr_mode = ( (sensor_mode_t *)sensor_arg )->wdr_mode;

    if ( new_wdr_mode != p_ctx->cur_wdr_mode ) {
        uint32_t cnt = 0;
        // we need to wait KF to update the is_fetched to 0.
        while ( p_ctx->fw_sbuf->kf_info.cali_info.is_fetched ) {
            cnt++;
            LOG( LOG_INFO, "wait KF to update calibrations, cnt: %d.", cnt );

            system_timer_usleep( 10 * 1000 );

            // 3 seconds timeout
            if ( cnt >= 300 ) {
                LOG( LOG_ERR, "Timeout to wait for KF to update calibrations." );
                return -3;
            }
        }

        LOG( LOG_ERR, "update preset from '%d' to '%d'", p_ctx->cur_wdr_mode, new_wdr_mode );

        p_ctx->cur_wdr_mode = new_wdr_mode;
    }

    uint8_t *cali_base = (uint8_t *)&p_ctx->fw_sbuf->kf_info.cali_info.cali_data;
    LookupTable *cali_lut_base = (LookupTable *)cali_base;
    uint8_t *cali_data_base = (uint8_t *)cali_base + sizeof( struct sbuf_lookup_table ) * CALIBRATION_TOTAL_SIZE;
    LOG( LOG_ERR, "p_ctx: %p, cali_base: %p, cali_data_base: %p, CALIBRATION_TOTAL_SIZE: %d.", p_ctx, cali_base, cali_data_base, CALIBRATION_TOTAL_SIZE );

    for ( idx = 0; idx < CALIBRATION_TOTAL_SIZE; idx++ ) {

        cali_lut_base[idx].ptr = cali_data_base + data_offset;

        data_offset += cali_lut_base[idx].rows * cali_lut_base[idx].cols * cali_lut_base[idx].width;

        c->calibrations[idx] = &cali_lut_base[idx];

        if ( idx <= 5 || idx >= ( CALIBRATION_TOTAL_SIZE - 5 ) ) {
            LOG( LOG_ERR, "cali_lut_base[%d].ptr: %p, rows: %d, cols: %d, width: %d.",
                 idx,
                 c->calibrations[idx]->ptr,
                 c->calibrations[idx]->rows,
                 c->calibrations[idx]->cols,
                 c->calibrations[idx]->width );
        }
    }

    p_ctx->fw_sbuf->kf_info.cali_info.is_fetched = 1;

    LOG( LOG_ERR, "--- get_calibration done for ctx_id: %d", ctx_id );

    return 0;
}
