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

#ifndef SYSTEM_LOG_H_INCLUDED
#define SYSTEM_LOG_H_INCLUDED

#include "acamera_types.h"
#include "acamera_firmware_config.h"
#include "system_timer.h"  //system_timer_timestamp
#include "system_stdlib.h" //system_memcpy
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

//defines for log level and system maximum value number for levels
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_NOTICE 2
#define LOG_WARNING 3
#define LOG_ERR 4
#define LOG_CRIT 5
#define LOG_NOTHING 6
#define SYSTEM_LOG_LEVEL_MAX LOG_NOTHING

//system maximum value number of modules
#define SYSTEM_LOG_MODULE_MAX LOG_MODULE_MAX

//initial values for logger log level and log module masks on real time logging, otherwise fixed
#define SYSTEM_LOG_LEVEL LOG_CRIT
#define SYSTEM_LOG_MASK FW_LOG_MASK


/*define for the logger for real time changing of level and module support
value of 1 means real time logging is enabled otherwise disabled on a compile level*/
#define SYSTEM_LOG_REAL_TIME FW_LOG_REAL_TIME

/*define for the logger to include src information like filenames and line number
value of 1 means filename, function and line number is logged otherwise disabled on a compile level*/
#define SYSTEM_LOG_HAS_SRC FW_LOG_HAS_SRC

/*define for the logger to enable logging from ISR
value of 1 means ISR logging is enabled otherwise disabled on the compile level*/
#define SYSTEM_LOG_FROM_ISR FW_LOG_FROM_ISR

//printf like functions used by the logger to log output
#define SYSTEM_VPRINTF vprintf
#define SYSTEM_PRINTF printf
#define SYSTEM_SNPRINTF snprintf
#define SYSTEM_VSPRINTF vsprintf

/*define for the logger to include time on the output log
value of 1 means timestamp is logged otherwise disabled on the compile level*/
#define SYSTEM_LOG_HAS_TIME FW_LOG_HAS_TIME

//callback for acamera_logger to log the time
#define SYSTEM_TIME_LOG_CB sys_time_log_cb
extern const char *sys_time_log_cb( void );

//debug log names for level and modules
extern const char *const log_level_name[SYSTEM_LOG_LEVEL_MAX];
extern const char *const log_module_name[SYSTEM_LOG_MODULE_MAX];


#endif // LOG_H_INCLUDED
