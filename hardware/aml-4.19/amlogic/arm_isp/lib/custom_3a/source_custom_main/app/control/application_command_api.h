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

#ifndef __APPLICATION_COMMAND_API_H__
#define __APPLICATION_COMMAND_API_H__

#include "acamera_types.h"

uint8_t application_command( uint8_t command_type, uint8_t command, uint32_t value, uint8_t direction, uint32_t *ret_value );

uint8_t application_api_calibration( uint8_t type, uint8_t id, uint8_t direction, void *data, uint32_t data_size, uint32_t *ret_value );


#endif // __APPLICATION_COMMAND_API_H__
