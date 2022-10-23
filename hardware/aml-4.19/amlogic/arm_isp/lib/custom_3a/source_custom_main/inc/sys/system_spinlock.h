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

#ifndef __SYSTEM_SPINLOCK_H__
#define __SYSTEM_SPINLOCK_H__
//-----------------------------------------------------------------------------
#include "acamera_types.h"
//-----------------------------------------------------------------------------
typedef void *sys_spinlock;

//-----------------------------------------------------------------------------

/**
 *   Initialize system spinlock
 *
 *   The function initializes a system dependent spinlock
 *
 *   @param void ** lock - the pointer to system spinlock will be returned
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int system_spinlock_init( sys_spinlock *lock );


/**
 *   Locks the spinlock pointed to by lock
 *
 *   @param   lock - pointer to spinlock returned by system_spinlock_init
 *
 *   @return  flags - used when unlock
 */
unsigned long system_spinlock_lock( sys_spinlock lock );


/**
 *   Unlock the spinlock pointed to by lock
 *
 *   @param   lock - pointer to spinlock returned by system_spinlock_init
 *
 *   @return  void
 */
void system_spinlock_unlock( sys_spinlock lock, unsigned long flags );


/**
 *   Destroy spinlock
 *
 *   The function destroys spinlock which was created by system_spinlock_init
 *
 *   @return  void
 */
void system_spinlock_destroy( sys_spinlock lock );

//-----------------------------------------------------------------------------
#endif //__SYSTEM_SPINLOCK_H__
