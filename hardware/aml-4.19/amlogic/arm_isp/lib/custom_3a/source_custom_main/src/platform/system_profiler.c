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

#include "acamera_firmware_config.h"


#if defined( ACAMERA_ISP_PROFILING ) && ( ACAMERA_ISP_PROFILING == 1 )
#include "acamera_profiler.h"
//#include "system_headers.h"
#include <time.h>
#include "acamera_fw.h"
//=============Controls===========================================================

//================================================================================
uint64_t time_diff( struct timespec start, struct timespec end )
{
    struct timespec temp;
    if ( ( end.tv_nsec - start.tv_nsec ) < 0 ) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return (uint64_t)temp.tv_sec * 1000000000 + temp.tv_nsec;
}
//=============Profiler functions (calling order:  profiler.scxml)================

//-------------Platform depended functions----------------------------------------
int32_t cpu_get_freq( void )
{
    return 1000000000;
}

static struct timespec start_clk;
static struct timespec start_isr_clk;

void cpu_start_clocks( void )
{
    clock_gettime( CLOCK_REALTIME, &start_clk );
}

void cpu_start_isr_clocks( void )
{
    clock_gettime( CLOCK_THREAD_CPUTIME_ID, &start_isr_clk );
}

uint64_t cpu_stop_clocks( void )
{
    struct timespec end_clk;
    clock_gettime( CLOCK_REALTIME, &end_clk );
    return time_diff( start_clk, end_clk );
}

uint64_t cpu_stop_isr_clocks( void )
{
    struct timespec end_clk;
    clock_gettime( CLOCK_THREAD_CPUTIME_ID, &end_clk );
    return time_diff( start_isr_clk, end_clk );
}
void cpu_init_profiler( void )
{
}
//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------------
//================================================================================
#endif //ACAMERA_ISP_PROFILING
