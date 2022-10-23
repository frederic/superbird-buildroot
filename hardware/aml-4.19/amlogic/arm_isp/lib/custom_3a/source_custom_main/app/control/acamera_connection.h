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

#if !defined( __ACAMERA_CONNECTION_H__ )
#define __ACAMERA_CONNECTION_H__

#include "acamera_control_config.h"

#define BUS_ERROR_RESET -1
#define BUS_ERROR_FATAL -2

void acamera_connection_init( void );
void acamera_connection_process( void );
void acamera_connection_destroy( void );

#endif /* __ACAMERA_CONNECTION_H__ */
