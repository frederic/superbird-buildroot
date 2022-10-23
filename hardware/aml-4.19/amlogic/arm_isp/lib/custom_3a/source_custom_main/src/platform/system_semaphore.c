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
#include "system_semaphore.h"
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

int32_t system_semaphore_init( semaphore_t *sem )
{
    sem_t *sys_sem = malloc( sizeof( sem_t ) );
    *sem = sys_sem;
    sem_init( sys_sem, 0, 1 );
    return 0;
}

int32_t system_semaphore_raise( semaphore_t sem )
{
    sem_t *sys_sem = (sem_t *)sem;
    sem_post( sys_sem );
    return 0;
}

int32_t system_semaphore_wait( semaphore_t sem, uint32_t timeout_ms )
{
    sem_t *sys_sem = (sem_t *)sem;

    if ( timeout_ms ) {
        struct timespec ts;
        clock_gettime( CLOCK_REALTIME, &ts );
        ts.tv_sec += timeout_ms / 1000;
        timeout_ms = timeout_ms % 1000;
        ts.tv_nsec += timeout_ms * 1000000;
        if ( ts.tv_nsec >= 1000000000 ) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        return sem_timedwait( sys_sem, &ts ); // wait semaphore with timeout and return result
    } else {
        return sem_wait( sys_sem );
    }
}

int32_t system_semaphore_destroy( semaphore_t sem )
{
    sem_t *sys_sem = (sem_t *)sem;
    sem_destroy( sys_sem );
    free( sys_sem );
    return 0;
}
