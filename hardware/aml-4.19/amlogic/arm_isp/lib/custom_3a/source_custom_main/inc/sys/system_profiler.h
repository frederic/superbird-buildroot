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

#ifndef __SYSTEM_PROFILER_H__
#define __SYSTEM_PROFILER_H__

#include "acamera_types.h"

/**
 *   Return system CPU frequency
 *
 *   @return  frequency - on success
 *           -1 - on error
 */
int32_t cpu_get_freq( void );


/**
 *   Initialize system clock
 *
 *
 *   @return  none
 */
void cpu_start_clocks( void );


/**
 *   Stop system clock
 *
 *   The function returns the time difference between the current time and
 *   previous call of cpu_start_clocks
 *
 *
 *   @return  time - on success
 *           -1 - on error
 */
uint64_t cpu_stop_clocks( void );


/**
 *   Initialize profiler
 *
 *   @return  none
 */
void cpu_init_profiler( void );

uint32_t acamera_isp_io_get_counter( void );

#endif /* __SYSTEM_PROFILER_H__ */
