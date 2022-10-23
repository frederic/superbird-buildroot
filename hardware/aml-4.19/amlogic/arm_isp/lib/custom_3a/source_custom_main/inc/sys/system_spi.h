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

#ifndef __SYSTEM_SPI_H__
#define __SYSTEM_SPI_H__

#include "acamera_types.h"


#define AUTO_CPHA_CPOL 0x4000
#define AUTO_SS_MSK 0x2000
#define IE_MSK 0x1000
#define LSB_MSK 0x0800
#define TX_NEG_MSK 0x0400
#define RX_NEG_MSK 0x0200
#define GO_BUSY_MSK 0x0100
#define CHAR_LEN_MSK 0x007F


/**
 *   Initialize spi connection
 *
 *   The function is called before any uart communication is run to
 *   initialise uart connection.
 *
 *   @return  fd - on success
 *           -1 - on error
 */
int32_t system_spi_init( void );


/**
 *   Read data
 *
 *   The function reads 32 bit data from spi channel
 *
 *   @param sel_mask
 *   @param control
 *   @param data
 *   @param data_size
 *
 */
uint32_t system_spi_rw32( uint32_t sel_mask, uint32_t control, uint32_t data, uint8_t data_size );


/**
 *   Read data
 *
 *   The function reads 32 bit data from spi channel
 *
 *   @param sel_mask
 *   @param control
 *   @param data
 *   @param data_size
 *
 */
uint32_t system_spi_rw48( uint32_t sel_mask, uint32_t control, uint32_t addr, uint8_t addr_size, uint32_t data, uint8_t data_size );


#endif /* __SYSTEM_SPI_H__ */
