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

#ifndef __SYSTEM_SEMAPHORE_H__
#define __SYSTEM_SEMAPHORE_H__
//-----------------------------------------------------------------------------
#include "acamera_types.h"
//-----------------------------------------------------------------------------
typedef void *semaphore_t;

//-----------------------------------------------------------------------------

/**
 *   Initialize system semaphore
 *
 *   The function initializes a system dependent semaphore
 *
 *   @param void ** sem - the pointer to system semaphore will be returned
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int32_t system_semaphore_init( semaphore_t *sem );


/**
 *   Unlock the semaphore pointed to by sem
 *
 *   The function increments (unlocks) the semaphore.
 *   If the semaphore value becomes greater than zero, then
 *   another process which is blocked by system_semaphore_wait will be
 *   woken up
 *
 *   @param   sem - pointer to semaphore returned by system_semaphore_init
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int32_t system_semaphore_raise( semaphore_t sem );


/**
 *   Locks the semaphore pointed to by sem
 *
 *   The function decrements (locks) the semaphore.
 *   If the semaphore value is greater than zero, then
 *   the decrement proceeds, and the function returns.
 *
 *   @param   sem - pointer to semaphore returned by system_semaphore_init
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int32_t system_semaphore_wait( semaphore_t sem, uint32_t timeout_ms );


/**
 *   Destroy semaphore
 *
 *   The function destroys semaphore which was created by system_semaphore_init
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int32_t system_semaphore_destroy( semaphore_t sem );

//-----------------------------------------------------------------------------
#endif //__SYSTEM_SEMAPHORE_H__
