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
#include <time.h>
#include <unistd.h>

//================================================================================
// timer functions (for FPS calculation)
uint32_t system_timer_timestamp( void )
{
    struct timespec ts;
    clock_gettime( CLOCK_REALTIME, &ts );
    return ( uint32_t )( ts.tv_sec * 1000000 + ts.tv_nsec / 1000 );
}


void system_timer_init( void )
{
}


uint32_t system_timer_frequency( void )
{
    return 1000000;
}


int32_t system_timer_usleep( uint32_t usec )
{
    return usleep( usec );
}

//================================================================================
