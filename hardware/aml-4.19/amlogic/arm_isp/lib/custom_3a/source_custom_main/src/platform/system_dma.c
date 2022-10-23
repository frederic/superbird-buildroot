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

#include "acamera_types.h"
#include "acamera_logger.h"
#include "system_dma.h"


int32_t system_dma_init( void **ctx )
{
    return 0;
}


int32_t system_dma_destroy( void *ctx )
{
    return 0;
}

int32_t system_dma_copy_device_to_memory( void *ctx, void *dst_mem, uint32_t dev_phy_addr, uint32_t size_to_copy )
{
    return 0;
}


int32_t system_dma_copy_memory_to_device( void *ctx, void *src_mem, uint32_t dev_phy_addr, uint32_t size_to_copy )
{
    return 0;
}


int32_t system_dma_copy_device_to_memory_async( void *ctx, void *dst_mem, uint32_t dev_phy_addr, uint32_t size_to_copy, dma_completion_callback complete_func, void *arg )
{
    return 0;
}


int32_t system_dma_copy_memory_to_device_async( void *ctx, void *src_mem, uint32_t dev_phy_addr, uint32_t size_to_copy, dma_completion_callback complete_func, void *arg )
{
    return 0;
}

int32_t system_dma_sg_device_setup( void *ctx, int32_t buff_loc, dma_addr_pair_t *device_addr_pair, int32_t addr_pairs )
{
    return 0;
}

int32_t system_dma_sg_fwmem_setup( void *ctx, int32_t buff_loc, fwmem_addr_pair_t *fwmem_pair, int32_t addr_pairs )
{
    return 0;
}

void system_dma_unmap_sg( void *ctx )
{
}

int32_t system_dma_copy_sg( void *ctx, int32_t buff_loc, uint32_t direction, dma_completion_callback complete_func /*, void *arg*/ )
{
    return 0;
}

static int32_t system_dma_copy( void *ctx, void *sys_mem, uint32_t dev_phy_addr, uint32_t size_to_copy, uint32_t direction, dma_completion_callback complete_func, void *arg )
{
    return 0;
}
