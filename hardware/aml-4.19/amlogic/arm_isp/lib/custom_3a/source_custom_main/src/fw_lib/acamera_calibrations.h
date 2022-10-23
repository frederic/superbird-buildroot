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

#ifndef __ACAMERA_CALIBRATIONS_H__
#define __ACAMERA_CALIBRATIONS_H__

#include "acamera_types.h"
#include "acamera_math.h"
#include "acamera_firmware_api.h"


#define CALIBRATION_BASE_SET 0
#define CALIBRATION_PREVIEW_SET 1


LookupTable *_GET_LOOKUP_PTR( void *p_ctx, uint32_t idx );

const void *_GET_LUT_PTR( void *p_ctx, uint32_t idx );

uint8_t *_GET_UCHAR_PTR( void *p_ctx, uint32_t idx );

uint16_t *_GET_USHORT_PTR( void *p_ctx, uint32_t idx );

uint32_t *_GET_UINT_PTR( void *p_ctx, uint32_t idx );

modulation_entry_t *_GET_MOD_ENTRY16_PTR( void *p_ctx, uint32_t idx );

modulation_entry_32_t *_GET_MOD_ENTRY32_PTR( void *p_ctx, uint32_t idx );

uint32_t _GET_ROWS( void *p_ctx, uint32_t idx );

uint32_t _GET_COLS( void *p_ctx, uint32_t idx );

uint32_t _GET_LEN( void *p_ctx, uint32_t idx );

uint32_t _GET_WIDTH( void *p_ctx, uint32_t idx );

uint32_t _GET_SIZE( void *p_ctx, uint32_t idx );


#endif // __ACAMERA_CALIBRATIONS_H__
