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

#ifndef __ACAMERA_FIRMWARE_API_H__
#define __ACAMERA_FIRMWARE_API_H__

#include "acamera_types.h"
#include "acamera_sensor_api.h"
#include "acamera_lens_api.h"
#include "acamera_command_api.h"
#include "acamera_firmware_settings.h"

/**
 *   Initialize one instance of firmware context
 *
 *   The firmware can control several context at the same time. Each context must be initialized with its own
 *   set of setting and independently from all other contexts. A pointer will be returned on valid context on
 *   successful initialization.
 *
 *   @param  settings - a structure with setting for context to be initialized with
 *   @param  ctx_num - number of contexts to be initialized
 *
 *   @return 0 - success
 *          -1 - fail.
 */
int32_t acamera_init( acamera_settings *settings, uint32_t ctx_num );


/**
 *   Terminate the firmware
 *
 *   The function will close the firmware and free all previously allocated resources
 *
 *   @return 0 - success
 *          -1 - fail.
 */
int32_t acamera_terminate( void );


/**
 *   Process one instance of firmware context
 *
 *   The firmware can control several context at the same time. Each context must be given a CPU time to fulfil
 *   all tasks it has at the moment. This function has to be called for all contexts as frequent as possible to avoid
 *   delays.
 *
 *   @param  ctx - context pointer which was returned by acamera_init
 *
 *   @return 0 - success
 *          -1 - fail.
 */
int32_t acamera_process( void );


/**
 *   Process interrupts
 *
 *   This function must be called when any of interrupts happen. It will process interrupts properly to guarantee
 *   the correct firmware behaviour.
 *
 *   @return 0 - success
 *          -1 - fail.
 */
int32_t acamera_interrupt_handler( void );


#endif // __ACAMERA_FIRMWARE_API_H__
