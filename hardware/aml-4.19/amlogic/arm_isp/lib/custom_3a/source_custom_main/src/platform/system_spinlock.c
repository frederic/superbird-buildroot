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

#include "acamera_types.h"
#include "system_spinlock.h"
#include <stdlib.h>
#include <pthread.h>


int system_spinlock_init( sys_spinlock *lock )
{
    pthread_mutex_t *slock = malloc( sizeof( pthread_mutex_t ) );

    if ( slock ) {
        *lock = (void *)slock;
        pthread_mutex_init( slock, 0 );
    }

    return slock ? 0 : -1;
}

unsigned long system_spinlock_lock( sys_spinlock lock )
{
    unsigned long flags = 0;
    pthread_mutex_t *slock = (pthread_mutex_t *)lock;

    pthread_mutex_lock( slock );

    return flags;
}

void system_spinlock_unlock( sys_spinlock lock, unsigned long flags )
{
    pthread_mutex_t *slock = (pthread_mutex_t *)lock;

    pthread_mutex_unlock( slock );
}

void system_spinlock_destroy( sys_spinlock lock )
{
    if ( lock )
        free( lock );
}
