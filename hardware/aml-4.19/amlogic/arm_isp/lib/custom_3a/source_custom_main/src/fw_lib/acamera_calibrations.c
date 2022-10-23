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

#include "acamera.h"
#include "acamera_fw.h"
#include "acamera_firmware_config.h"
#include "acamera_logger.h"
#include "system_timer.h"

static ACameraCalibrations *get_current_set( void *p_ctx )
{
    return &( (acamera_context_ptr_t)p_ctx )->acameraCalibrations;
}

LookupTable *_GET_LOOKUP_PTR( void *p_ctx, uint32_t idx )
{
    LookupTable *result = NULL;
    if ( idx < CALIBRATION_TOTAL_SIZE ) {
        result = get_current_set( p_ctx )->calibrations[idx];
    } else {
        result = NULL;
        LOG( LOG_CRIT, "Trying to access an isp lut with invalid index %d", (int)idx );
    }
    return result;
}


const void *_GET_LUT_PTR( void *p_ctx, uint32_t idx )
{
    const void *result = NULL;
    LookupTable *lut = _GET_LOOKUP_PTR( p_ctx, idx );
    if ( lut != NULL ) {
        result = lut->ptr;
    } else {
        while ( 1 ) {
            fsm_param_mon_err_head_t mon_err_head;
            mon_err_head.err_type = MON_TYPE_ERR_CALIBRATION_LUT_NULL;
            mon_err_head.err_param = idx;
            acamera_fsm_mgr_set_param( &( (acamera_context_ptr_t)p_ctx )->fsm_mgr, FSM_PARAM_SET_MON_ERROR_REPORT, &mon_err_head, sizeof( mon_err_head ) );
            LOG( LOG_CRIT, "LUT 0x%x is not initialized. Pointer is NULL. Going to the infinite loop", (int)idx );

            // sleep 3 second to avoid affect system performance badly.
            system_timer_usleep( 3 * 1000 * 1000 );
        };
    }
    return result;
}

// use fast version of lut access routines
uint8_t *_GET_UCHAR_PTR( void *p_ctx, uint32_t idx )
{
    uint8_t *result = (uint8_t *)_GET_LUT_PTR( p_ctx, idx );
    return result;
}

uint16_t *_GET_USHORT_PTR( void *p_ctx, uint32_t idx )
{
    uint16_t *result = (uint16_t *)_GET_LUT_PTR( p_ctx, idx );
    return result;
}

uint32_t *_GET_UINT_PTR( void *p_ctx, uint32_t idx )
{
    uint32_t *result = (uint32_t *)_GET_LUT_PTR( p_ctx, idx );
    return result;
}

modulation_entry_t *_GET_MOD_ENTRY16_PTR( void *p_ctx, uint32_t idx )
{
    modulation_entry_t *result = (modulation_entry_t *)_GET_LUT_PTR( p_ctx, idx );
    return result;
}

modulation_entry_32_t *_GET_MOD_ENTRY32_PTR( void *p_ctx, uint32_t idx )
{
    modulation_entry_32_t *result = (modulation_entry_32_t *)_GET_LUT_PTR( p_ctx, idx );
    return result;
}

uint32_t _GET_ROWS( void *p_ctx, uint32_t idx )
{
    uint32_t result = 0;
    LookupTable *lut = _GET_LOOKUP_PTR( p_ctx, idx );
    if ( lut != NULL ) {
        result = lut->rows;
    }
    return result;
}

uint32_t _GET_COLS( void *p_ctx, uint32_t idx )
{
    uint32_t result = 0;
    LookupTable *lut = _GET_LOOKUP_PTR( p_ctx, idx );
    if ( lut != NULL ) {
        result = lut->cols;
    }
    return result;
}

uint32_t _GET_LEN( void *p_ctx, uint32_t idx )
{
    uint32_t result = 0;
    LookupTable *lut = _GET_LOOKUP_PTR( p_ctx, idx );
    if ( lut != NULL ) {
        result = lut->cols * lut->rows;
    }
    return result;
}

uint32_t _GET_WIDTH( void *p_ctx, uint32_t idx )
{
    uint32_t result = 0;
    LookupTable *lut = _GET_LOOKUP_PTR( p_ctx, idx );
    if ( lut != NULL ) {
        result = lut->width;
    }
    return result;
}

uint32_t _GET_SIZE( void *p_ctx, uint32_t idx )
{
    uint32_t result = 0;
    LookupTable *lut = _GET_LOOKUP_PTR( p_ctx, idx );
    if ( lut != NULL ) {
        result = lut->cols * lut->rows * lut->width;
    }
    return result;
}
