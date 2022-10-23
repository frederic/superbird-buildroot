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

#ifndef __ACAMERA_DS1_SCALER_VFILT_COEFMEM_CONFIG_H__
#define __ACAMERA_DS1_SCALER_VFILT_COEFMEM_CONFIG_H__


#include "system_sw_io.h"

#include "system_hw_io.h"

// ------------------------------------------------------------------------------ //
// Instance 'ds_scaler_vfilt_coefmem' of module 'ds_scaler_vfilt_coefmem'
// ------------------------------------------------------------------------------ //

#define ACAMERA_DS1_SCALER_VFILT_COEFMEM_BASE_ADDR (0x1ca8L)
#define ACAMERA_DS1_SCALER_VFILT_COEFMEM_SIZE (0x800)

#define ACAMERA_DS1_SCALER_VFILT_COEFMEM_ARRAY_DATA_DEFAULT (0x0)
#define ACAMERA_DS1_SCALER_VFILT_COEFMEM_ARRAY_DATA_DATASIZE (32)
#define ACAMERA_DS1_SCALER_VFILT_COEFMEM_ARRAY_DATA_OFFSET (0x1ca8L)

// args: index (0-511), data (32-bit)
static __inline void acamera_ds1_scaler_vfilt_coefmem_array_data_write( uintptr_t base, uint32_t index, uint32_t data) {
    system_hw_write_32(0x1ca8L + (index << 2), data);
}
static __inline uint32_t acamera_ds1_scaler_vfilt_coefmem_array_data_read( uintptr_t base, uint32_t index) {
    return system_hw_read_32(0x1ca8L + (index << 2));
}
// ------------------------------------------------------------------------------ //
#endif //__ACAMERA_DS1_SCALER_VFILT_COEFMEM_CONFIG_H__
