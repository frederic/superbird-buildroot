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

#ifndef __SYSTEM_TIMER_H__
#define __SYSTEM_TIMER_H__

#include "acamera_types.h"


/**
 *   Initialize system timer
 *
 *   The function system_timer_init() can be called at application level to
 *   initialise the timestamp facility.
 *
 *   @return  0 - on success
 *           -1 - on error
 */
void system_timer_init( void );


/**
 *   Return the current timer value
 *
 *   The function must return the current timer value.
 *
 *   @return  current timestamp
 */
uint32_t system_timer_timestamp( void );


/**
 *   Return the system timer frequency
 *
 *   This function must return a number of ticks per second for a system timer
 *   For example in the case if the timer uses microseconds this function should return
 *   1000000
 *
 *   @return positive number - timer frequency
 *           0 - on error
 */
uint32_t system_timer_frequency( void );


/**
 *   Usleep implementation for the current platform
 *
 *   This function must suspend execution of the calling thread
 *   by specified number of microseconds. The implementation of this function
 *   depends on the current platfrom.
 *
 *   @param   usec - time to sleep in microseconds
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int32_t system_timer_usleep( uint32_t usec );


#endif /* __SYSTEM_TIMER_H__ */
