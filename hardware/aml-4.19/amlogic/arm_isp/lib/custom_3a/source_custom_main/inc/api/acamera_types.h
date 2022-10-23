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

#ifndef __ACAMERA__TYPES_H__
#define __ACAMERA__TYPES_H__

#include "acamera_firmware_config.h"

#if KERNEL_MODULE == 1
#include "linux/types.h"
#else
#include <stdint.h>
#endif

typedef enum dma_buf_state {
    dma_buf_empty = 0,
    dma_buf_busy,
    dma_buf_ready,
    dma_buf_purge
} dma_buf_state;

/**
 *   supported pipe types
 *   please note that dma_max is used
 *   only as a counter for supported pipes
 */
typedef enum dma_type {
    dma_fr = 0,
    dma_ds1,
    dma_ds2,
    dma_max
} dma_type;

typedef struct LookupTable {
    void *ptr;
#ifdef MATCHBIT
    uint32_t reserved;
#endif
    uint16_t rows;
    uint16_t cols;
    uint16_t width;
} LookupTable;

#endif /* __ACAMERA__TYPES_H__ */
