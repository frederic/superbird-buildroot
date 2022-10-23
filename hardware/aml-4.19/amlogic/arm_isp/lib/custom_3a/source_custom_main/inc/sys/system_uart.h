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

#ifndef __SYSTEM_UART_H__
#define __SYSTEM_UART_H__

#include "acamera_types.h"


/**
 *   Initialize uart connection
 *
 *   The function is called before any uart communication is run to
 *   initialise uart connection.
 *
 *   @return  fd - on success
 *           -1 - on error
 */
int32_t system_uart_init( void );


/**
 *   Write data with uart connection
 *
 *   The function writes specified amount of data
 *   to a given uart descriptor fd
 *
 *   @param   fd  - uard descriptor which was returned by system_uart_init
 *            p   - pointer to a data to be written
 *            len - amount of bytes to write
 *
 *   @return  >=0 - amount of written bytes on success
 *           -1 - on error
 */
int32_t system_uart_write( int32_t fd, const uint8_t *p, int32_t len );


/**
 *   Read data from uart connection
 *
 *   The function reads specified amount of data
 *   from a given uart descriptor fd
 *
 *   @param   fd  - uard descriptor which was returned by system_uart_init
 *            p   - pointer to a memory block to save input data
 *            len - amount of bytes to read
 *
 *   @return  >=0 - amount of read bytes on success
 *           -1 - on error
 */
int32_t system_uart_read( int32_t fd, uint8_t *p, int32_t len );

#endif // __SYSTEM_UART_H__
