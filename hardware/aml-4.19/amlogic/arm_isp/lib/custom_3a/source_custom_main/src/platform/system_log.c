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
#include "system_timer.h"
#include "system_log.h"

//debug log names for level
const char *const log_level_name[SYSTEM_LOG_LEVEL_MAX] = {"DEBUG", "INFO", "NOTICE", "WARNING", "ERR", "CRIT"};
//debug log names for modules
const char *const log_module_name[SYSTEM_LOG_MODULE_MAX] = FSM_NAMES;

const char *sys_time_log_cb( void )
{
    static char time_buf[32];
    struct timespec clk;
    clock_gettime( CLOCK_MONOTONIC, &clk );
    sprintf( time_buf, "[%5ld.%06ld]", clk.tv_sec, clk.tv_nsec / 1000 );
    return (const char *)time_buf;
}
