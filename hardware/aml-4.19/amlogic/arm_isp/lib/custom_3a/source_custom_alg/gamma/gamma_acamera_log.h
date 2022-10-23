//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] Amlogic Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#ifndef __GAMMA_ACAMERA_LOG_H__
#define __GAMMA_ACAMERA_LOG_H__

#include <stdio.h>

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_NOTICE 2
#define LOG_WARNING 3
#define LOG_ERR 4
#define LOG_CRIT 5
#define LOG_NOTHING 6
#define SYSTEM_LOG_LEVEL_MAX LOG_NOTHING

#define SYSTEM_LOG_LEVEL LOG_ERR

#define LOG_ON(level) \
	((level >= SYSTEM_LOG_LEVEL))

#define LOG_WRITE( level, format, ... )               \
	do {                                                     \
		if (LOG_ON( level))           \
			printf( "%s:%5d " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__ ); \
	} while (0)

#define LOG(level, ... ) LOG_WRITE(level, __VA_ARGS__)

#endif
