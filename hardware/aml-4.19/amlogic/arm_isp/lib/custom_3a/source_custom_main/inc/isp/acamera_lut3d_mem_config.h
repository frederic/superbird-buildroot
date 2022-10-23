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

#ifndef __ACAMERA_LUT3D_MEM_CONFIG_H__
#define __ACAMERA_LUT3D_MEM_CONFIG_H__


#include "system_sw_io.h"

#include "system_hw_io.h"

// ------------------------------------------------------------------------------ //
// Instance 'lut3d_mem' of module 'lut3d_mem'
// ------------------------------------------------------------------------------ //

#define ACAMERA_LUT3D_MEM_BASE_ADDR (0x508L)
#define ACAMERA_LUT3D_MEM_SIZE (0x1000)

#define ACAMERA_LUT3D_MEM_ARRAY_DATA_DEFAULT (0x0)
#define ACAMERA_LUT3D_MEM_ARRAY_DATA_DATASIZE (32)
#define ACAMERA_LUT3D_MEM_ARRAY_DATA_OFFSET (0x508L)

// args: index (0-999), data (32-bit)
static __inline void acamera_lut3d_mem_array_data_write( uintptr_t base, uint32_t index, uint32_t data) {
    system_hw_write_32(0x508L + (index << 2), data);
}
static __inline uint32_t acamera_lut3d_mem_array_data_read( uintptr_t base, uint32_t index) {
    return system_hw_read_32(0x508L + (index << 2));
}
// ------------------------------------------------------------------------------ //
#endif //__ACAMERA_LUT3D_MEM_CONFIG_H__
