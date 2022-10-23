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

#ifndef __ACAMERA_H__
#define __ACAMERA_H__


#include "acamera_types.h"
#include "acamera_firmware_config.h"
#include "acamera_firmware_api.h"
#include "acamera_logger.h"
#include "system_interrupts.h"
#include "system_semaphore.h"
#include "acamera_math.h"


typedef void ( *buffer_callback_t )( void *ctx_param, tframe_t *tframe, const metadata_t *metadata );

int32_t acamera_get_context_number( void );

void acamera_update_cur_settings_to_isp( void );

#endif /* __ACAMERA_H__ */
