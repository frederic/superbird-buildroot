/****************************************************************************
*
*    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


/**
**  @file
**  gcoSURF object for user HAL layers.
**
*/

#include "gc_hal_user_precomp.h"

#define _GC_OBJ_ZONE    gcdZONE_SURFACE

/******************************************************************************\
**************************** gcoSURF API Support Code **************************
\******************************************************************************/

const gcsSAMPLES g_sampleInfos[] =
    {
        {1, 1, 1},  /* 0x msaa */
        {1, 1, 1},  /* 1x msaa */
        {2, 1, 2},  /* 2x msaa */
        {0, 0, 0},  /* 3x msaa: invalid */
        {2, 2, 4},  /* 4x msaa */
    };

#if gcdENABLE_3D
/*******************************************************************************
**
**  _LockAuxiliary
**
**  Lock auxiliary node (ts/hz/hz ts...) and make sure the lockCounts same
**  as main node.
**
*/
static gceSTATUS
_LockAuxiliaryNode(
    IN gcsSURF_NODE_PTR Node,
    IN gcsSURF_NODE_PTR Reference
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT i,j;
    gceHARDWARE_TYPE type;

    gcmHEADER_ARG("Node=0x%x Reference=0x%x", Node, Reference);

    gcmGETCURRENTHARDWARE(type);

    for (i = 0 ; i < gcvHARDWARE_NUM_TYPES; i++)
    {
        for (j = 0; j < gcvENGINE_GPU_ENGINE_COUNT; j++)
        {
            gcmASSERT(Node->lockCounts[i][j] <= Reference->lockCounts[i][j]);

            while (Node->lockCounts[i][j] < Reference->lockCounts[i][j])
            {
                gcoHAL_SetHardwareType(gcvNULL, (gceHARDWARE_TYPE)i);

                gcmONERROR(gcoHARDWARE_LockEx(Node, (gceENGINE)j, gcvNULL, gcvNULL));
            }
        }
    }

OnError:
    /* Restore type. */
    gcoHAL_SetHardwareType(gcvNULL, type);

    /* Return status. */
    gcmFOOTER();
    return status;
}
/*******************************************************************************
**
**  gcoSURF_LockTileStatus
**
**  Locked tile status buffer of surface
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_LockTileStatus(
    IN gcoSURF Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 address;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Lock the tile status buffer. */
        gcmONERROR(
            _LockAuxiliaryNode(&Surface->tileStatusNode,
                               &Surface->node));

        gcmGETHARDWAREADDRESS(Surface->tileStatusNode, address);

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Locked tile status 0x%x: physical=0x%08X logical=0x%x",
                      &Surface->tileStatusNode,
                      address,
                      Surface->tileStatusNode.logical);

        /* Only 1 address. */
        Surface->tileStatusNode.count = 1;

        /* Check if this is the first lock. */
        if (Surface->tileStatusFirstLock)
        {
            if (!(Surface->hints & gcvSURF_NO_VIDMEM))
            {
                /* Fill the tile status memory with the filler. */
                gcoOS_MemFill(Surface->tileStatusNode.logical,
                              (gctUINT8) Surface->tileStatusFiller,
                              Surface->tileStatusNode.size);

                /* Flush the node from cache. */
                gcmONERROR(
                    gcoSURF_NODE_Cache(&Surface->tileStatusNode,
                                     Surface->tileStatusNode.logical,
                                     Surface->tileStatusNode.size,
                                     gcvCACHE_FLUSH));

                gcmDUMP(gcvNULL, "#[info tile status buffer]");
                /* Dump the memory write. */
                gcmDUMP_BUFFER(gcvNULL,
                               gcvDUMP_BUFFER_MEMORY,
                               address,
                               Surface->tileStatusNode.logical,
                               0,
                               Surface->tileStatusNode.size);
            }

            /* No longer first lock. */
            Surface->tileStatusFirstLock = gcvFALSE;
        }
    }

    /* Lock the hierarchical Z tile status buffer. */
    if (Surface->hzTileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
       /* Lock the tile status buffer. */
        gcmONERROR(
            _LockAuxiliaryNode(&Surface->hzTileStatusNode,
                               &Surface->node));

        gcmGETHARDWAREADDRESS(Surface->hzTileStatusNode, address);

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Locked HZ tile status 0x%x: physical=0x%08X logical=0x%x",
                      &Surface->hzTileStatusNode,
                      address,
                      Surface->hzTileStatusNode.logical);

        /* Only 1 address. */
        Surface->hzTileStatusNode.count = 1;

        /* Check if this is the first lock. */
        if (Surface->hzTileStatusFirstLock)
        {
            /* Fill the tile status memory with the filler. */
            gcoOS_MemFill(Surface->hzTileStatusNode.logical,
                          (gctUINT8) Surface->hzTileStatusFiller,
                          Surface->hzTileStatusNode.size);

            /* Flush the node from cache. */
            gcmONERROR(
                gcoSURF_NODE_Cache(&Surface->hzTileStatusNode,
                                 Surface->hzTileStatusNode.logical,
                                 Surface->hzTileStatusNode.size,
                                 gcvCACHE_CLEAN));

            /* Dump the memory write. */
            gcmDUMP(gcvNULL, "#[info hzTile status buffer]");
            gcmDUMP_BUFFER(gcvNULL,
                           gcvDUMP_BUFFER_MEMORY,
                           address,
                           Surface->hzTileStatusNode.logical,
                           0,
                           Surface->hzTileStatusNode.size);

            /* No longer first lock. */
            Surface->hzTileStatusFirstLock = gcvFALSE;
        }
    }
OnError:
    gcmFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_LockHzBuffer
**
**  Locked HZ buffer buffer of surface
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_LockHzBuffer(
    IN gcoSURF Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 address;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->hzNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            _LockAuxiliaryNode(&Surface->hzNode, &Surface->node));

        gcmGETHARDWAREADDRESS(Surface->hzNode, address);

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Locked HZ surface 0x%x: physical=0x%08X logical=0x%x",
                      &Surface->hzNode,
                      address,
                      Surface->hzNode.logical);

        /* Only 1 address. */
        Surface->hzNode.count = 1;
    }
OnError:
    gcmFOOTER_NO();
    return status;

}

/*******************************************************************************
**
**  gcoSURF_AllocateTileStatus
**
**  Allocate tile status buffer for surface
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_AllocateTileStatus(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;
    gctSIZE_T bytes = 0;
    gctUINT alignment;
    gctBOOL tileStatusInVirtual;
    gctUINT32 allocFlags = gcvALLOC_FLAG_NONE;
    gctUINT i = 0;
    gctSIZE_T sliceBytes = 0;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* No tile status buffer allocated. */
    Surface->tileStatusNode.pool             = gcvPOOL_UNKNOWN;
    Surface->hzTileStatusNode.pool           = gcvPOOL_UNKNOWN;

    /* Set tile status disabled at the beginning to be consistent with POOL value */
    for (i = 0; i < Surface->requestD; i++)
    {
        Surface->tileStatusDisabled[i] = gcvTRUE;
    }

    for (i = 0; i < Surface->requestD; i++)
    {
        Surface->dirty[i] = gcvFALSE;
    }

    tileStatusInVirtual = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_MC20);

    /*   Verify if the type requires a tile status buffer:
    ** - do not turn on fast clear if the surface is virtual;
    */
    if ((Surface->node.pool == gcvPOOL_VIRTUAL) && !tileStatusInVirtual)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if ((Surface->type != gcvSURF_RENDER_TARGET) &&
        (Surface->type != gcvSURF_DEPTH) &&
        ((Surface->hints & gcvSURF_DEC) != gcvSURF_DEC))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if (Surface->hints & gcvSURF_NO_TILE_STATUS)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if ((Surface->formatInfo.fakedFormat &&
         !Surface->paddingFormat
        ) ||
        ((Surface->bitsPerPixel > 32) &&
         (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_64BPP_HW_CLEAR_SUPPORT) == gcvFALSE)
        )
       )
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Only BLT_ENGINE and HALTI5 can support multi-slice surface.
    ** For resolve, it will increase the TS buffer size for alignment.
    ** If layer rendering firstly, then per slice read or sample, the PE rending address for each slice will not match the TX sample address.
    */
    if (!(gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE) &&
        gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_HALTI5)) &&
        (Surface->requestD > 1)
        )
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }


    /* Set default fill color. */
    switch (Surface->format)
    {
    case gcvSURF_D16:
        Surface->clearValue[0] = 0xFFFFFFFF;
        for (i = 0; i < Surface->requestD; i++)
        {
            Surface->fcValueUpper[i]  =
            Surface->fcValue[i]       = 0xFFFFFFFF;
            gcmONERROR(gcoHARDWARE_HzClearValueControl(Surface->format,
                                                       Surface->fcValue[i],
                                                       &Surface->fcValueHz,
                                                       gcvNULL));
        }
        break;

    case gcvSURF_D24X8:
    case gcvSURF_D24S8:
        Surface->clearValue[0] = 0xFFFFFF00;
        for (i = 0; i < Surface->requestD; i++)
        {
            Surface->fcValueUpper[i]  =
            Surface->fcValue[i]       = 0xFFFFFF00;
            gcmONERROR(gcoHARDWARE_HzClearValueControl(Surface->format,
                                                       Surface->fcValue[i],
                                                       &Surface->fcValueHz,
                                                       gcvNULL));
        }
        break;

    case gcvSURF_S8:
    case gcvSURF_X24S8:
        Surface->clearValue[0] = 0x00000000;
        for (i = 0; i < Surface->requestD; i++)
        {
            Surface->fcValueUpper[i]  =
            Surface->fcValue[i]       = 0x00000000;
        }
        break;

    case gcvSURF_R8_1_X8R8G8B8:
    case gcvSURF_G8R8_1_X8R8G8B8:
        Surface->clearValue[0]      =
        Surface->clearValueUpper[0] = 0xFF000000;
        for (i = 0; i < Surface->requestD; i++)
        {
            Surface->fcValue[0]            =
            Surface->fcValueUpper[0]       = 0xFF000000;
        }
        break;

    default:
        Surface->clearValue[0]      =
        Surface->clearValueUpper[0] = 0x00000000;
        for (i = 0; i < Surface->requestD; i++)
        {
            Surface->fcValue[0]            =
            Surface->fcValueUpper[0]       = 0x00000000;
        }
        break;
    }

    if (Surface->isMsaa &&
        gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_VMSAA))
    {
        Surface->vMsaa = gcvTRUE;
    }
    else
    {
        Surface->vMsaa = gcvFALSE;
    }


    /* Query the linear size for the tile status buffer. */
    status = gcoHARDWARE_QueryTileStatus(gcvNULL,
                                         Surface,
                                         Surface->sliceSize,
                                         &sliceBytes,
                                         &alignment,
                                         &Surface->tileStatusFiller);

    /* Tile status supported? */
    if ((status == gcvSTATUS_NOT_SUPPORTED) || (0 == sliceBytes))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else if (gcmIS_ERROR(status))
    {
        gcmFOOTER_NO();
        return status;
    }

    /* Not support 2-layers fake format Ts enable, so no multiply layers.*/
    gcmASSERT(Surface->formatInfo.layers == 1);

    Surface->tileStatusSliceSize = (gctUINT)sliceBytes;
    bytes = sliceBytes * Surface->requestD;

    /*gcvFEATURE_MC_FCCACHE_BYTEMASK feature is fix for v621 HW cache overlap bug*/
    if ((gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_COMPRESSION_V4) || gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_COMPRESSION_DEC400))
    &&  !gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_MC_FCCACHE_BYTEMASK))
    {
        bytes += 128;
    }

    if (Surface->hints & gcvSURF_PROTECTED_CONTENT)
    {
        allocFlags |= gcvALLOC_FLAG_SECURITY;
    }

    if (Surface->hints & gcvSURF_DMABUF_EXPORTABLE)
    {
        allocFlags |= gcvALLOC_FLAG_DMABUF_EXPORTABLE;
    }

    /* Copy filler. */
    Surface->hzTileStatusFiller = Surface->tileStatusFiller;

    if (!(Surface->hints & gcvSURF_NO_VIDMEM))
    {
        /* Allocate the tile status buffer. */
        status = gcsSURF_NODE_Construct(
            &Surface->tileStatusNode,
            bytes,
            alignment,
            gcvSURF_TILE_STATUS,
            allocFlags,
            gcvPOOL_DEFAULT
            );

        if (gcmIS_ERROR(status))
        {
            /* Commit any command buffer and wait for idle hardware. */
            status = gcoHAL_Commit(gcvNULL, gcvTRUE);

            if (gcmIS_SUCCESS(status))
            {
                /* Try allocating again. */
                status = gcsSURF_NODE_Construct(
                    &Surface->tileStatusNode,
                    bytes,
                    alignment,
                    gcvSURF_TILE_STATUS,
                    allocFlags,
                    gcvPOOL_DEFAULT
                    );
            }
        }
    }

    if (gcmIS_SUCCESS(status))
    {
        /* When allocate successfully, set tile status is enabled for this surface by default.
        ** Logically, we should disable tile status buffer initially.
        ** But for MSAA, we always enable FC, otherwise it will hang up on hw.
        ** So for non-cleared we also need enable FC by default.
        */
        if (Surface->TSDirty)
        {
            Surface->tileStatusFiller = 0x0;
            Surface->TSDirty = gcvFALSE;
        }

        for (i = 0; i < Surface->requestD; i++)
        {
            Surface->tileStatusDisabled[i] = gcvFALSE;
        }

        /* Only set garbagePadded=0 if by default cleared tile status. */
        if (Surface->paddingFormat)
        {
            Surface->garbagePadded = gcvFALSE;
        }

        /*
        ** Get surface compression setting.
        */
        gcoHARDWARE_QueryCompression(gcvNULL,
                                     Surface,
                                     &Surface->compressed,
                                     &Surface->compressFormat,
                                     &Surface->compressDecFormat);
        Surface->tileStatusFirstLock = gcvTRUE;


        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Allocated tile status 0x%x: pool=%d size=%u",
                      &Surface->tileStatusNode,
                      Surface->tileStatusNode.pool,
                      Surface->tileStatusNode.size);

        /* Allocate tile status for hierarchical Z buffer. */
        if (Surface->hzNode.pool != gcvPOOL_UNKNOWN)
        {
            /* Query the linear size for the tile status buffer. */
            status = gcoHARDWARE_QueryHzTileStatus(gcvNULL,
                                                   Surface,
                                                   Surface->hzNode.size,
                                                   &bytes,
                                                   &alignment);

            /* Tile status supported? */
            if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                return gcvSTATUS_OK;
            }

            if (!(Surface->hints & gcvSURF_NO_VIDMEM))
            {
                status = gcsSURF_NODE_Construct(
                             &Surface->hzTileStatusNode,
                             bytes,
                             alignment,
                             gcvSURF_TILE_STATUS,
                             allocFlags,
                             gcvPOOL_DEFAULT
                             );
            }

            if (gcmIS_SUCCESS(status))
            {
                Surface->hzTileStatusFirstLock = gcvTRUE;

                gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                              "Allocated HZ tile status 0x%x: pool=%d size=%u",
                              &Surface->hzTileStatusNode,
                              Surface->hzTileStatusNode.pool,
                              Surface->hzTileStatusNode.size);
            }
        }
    }
    else
    {
        Surface->vMsaa = gcvFALSE;
    }

OnError:
    gcmFOOTER_NO();
    /* Return the status. */
    return status;
}

/* Append tile status for user pool render target. */
gceSTATUS
gcoSURF_AppendTileStatus(
    IN gcoSURF Surface
    )
{
#if gcdENABLE_3D
    gceSURF_TYPE hints;
    gceSTATUS status;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Temporilary remove NO_VIDMEM hint. */
    hints = Surface->hints;
    Surface->hints = (gceSURF_TYPE)(hints & ~gcvSURF_NO_VIDMEM);

    if ((Surface->type != gcvSURF_RENDER_TARGET) &&
        (Surface->type != gcvSURF_DEPTH))
    {
        /* Only render target and depth can not tile status. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Try to allocate tile status buffer. */
    gcmONERROR(gcoSURF_AllocateTileStatus(Surface));

    /* Lock Tile status buffer */
    gcmONERROR(gcoSURF_LockTileStatus(Surface));

OnError:
    /* Restore hints. */
    Surface->hints = hints;

    gcmFOOTER();
    return status;
#else

    return gcvSTATUS_OK;
#endif
}


/*******************************************************************************
**
**  gcoSURF_AllocateHzBuffer
**
**  Allocate HZ buffer for surface
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_AllocateHzBuffer(
    IN gcoSURF Surface
    )
{
    gcePOOL  pool;
    gceSTATUS status;
    gctUINT32 allocFlags = gcvALLOC_FLAG_NONE;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->hzNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    pool = Surface->node.pool;

    /* No Hierarchical Z buffer allocated. */
    Surface->hzNode.pool = gcvPOOL_UNKNOWN;

    Surface->hzDisabled = gcvTRUE;

    /* Can't support multi-slice surface*/
    if (Surface->requestD > 1)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if (Surface->hints & gcvSURF_PROTECTED_CONTENT)
    {
        allocFlags |= gcvALLOC_FLAG_SECURITY;
    }

    /* Check if this is a depth buffer and the GPU supports hierarchical Z. */
    if ((Surface->type == gcvSURF_DEPTH) &&
        (Surface->format != gcvSURF_S8) &&
        (Surface->format != gcvSURF_X24S8) &&
        (pool != gcvPOOL_USER) &&
        ((Surface->hints & gcvSURF_NO_VIDMEM) == 0) &&
        (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_HZ) == gcvSTATUS_TRUE))
    {
        gctSIZE_T bytes;
        gctUINT32 sizeAlignment = 32 * 32 * 4;

        gctSIZE_T unalignedBytes = (Surface->size + 63)/64 * 4;

        /* Compute the hierarchical Z buffer size.  Allocate enough for
        ** 16-bit min/max values. */
        bytes = gcmALIGN(unalignedBytes, sizeAlignment);

        /* Allocate the hierarchical Z buffer. */
        status = gcsSURF_NODE_Construct(
                    &Surface->hzNode,
                    bytes,
                    64,
                    gcvSURF_HIERARCHICAL_DEPTH,
                    allocFlags,
                    pool
                    );

        if (gcmIS_SUCCESS(status))
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                          "Allocated HZ surface 0x%x: pool=%d size=%u",
                          &Surface->hzNode,
                          Surface->hzNode.pool,
                          Surface->hzNode.size);
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

}
#endif

static gceSTATUS
_Lock(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gceHARDWARE_TYPE currentType = gcvHARDWARE_INVALID;
    gctUINT32 address;

    gcmGETCURRENTHARDWARE(currentType);

    {
        /* Lock the video memory. */
        gcmONERROR(
            gcoHARDWARE_Lock(&Surface->node,
                             &address,
                             gcvNULL));

        /* Lock for mulit-buffer surfaces. */
        if (Surface->node2.pool != gcvPOOL_UNKNOWN)
        {
            /* Lock video memory for node 2. */
            gcmONERROR(
                gcoHARDWARE_Lock(&Surface->node2,
                                 gcvNULL,
                                 gcvNULL));
        }

        if (Surface->node3.pool != gcvPOOL_UNKNOWN)
        {
            /* Lock video memory for node 3. */
            gcmONERROR(
                gcoHARDWARE_Lock(&Surface->node3,
                                 gcvNULL,
                                 gcvNULL));
        }
    }

    Surface->node.logicalBottom = Surface->node.logical + Surface->bottomBufferOffset;

    Surface->node.hardwareAddressesBottom[currentType] =
    Surface->node.physicalBottom =
        address + Surface->bottomBufferOffset;

    gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                  "Locked surface 0x%x: physical=0x%08X logical=0x%x",
                  &Surface->node,
                  address,
                  Surface->node.logical);

#if gcdENABLE_3D
    /* Lock the hierarchical Z node. */
    gcmONERROR(gcoSURF_LockHzBuffer(Surface));

    /* Lock the tile status buffer and hierarchical Z tile status buffer. */
    gcmONERROR(gcoSURF_LockTileStatus(Surface));

#endif /* gcdENABLE_3D */

    /* Update initial HW type the first time lock */
    if (Surface->initType == gcvHARDWARE_INVALID)
    {
        Surface->initType = currentType;
    }
    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_Unlock(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    {
        /* Unlock the surface. */
        gcmONERROR(
            gcoHARDWARE_Unlock(&Surface->node, Surface->type));

        if (Surface->node2.pool != gcvPOOL_UNKNOWN)
        {
            gcmONERROR(
                gcoHARDWARE_Unlock(&Surface->node2, Surface->type));
        }

        if (Surface->node3.pool != gcvPOOL_UNKNOWN)
        {
            gcmONERROR(
                gcoHARDWARE_Unlock(&Surface->node3, Surface->type));
        }
    }

    gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                  "Unlocked surface 0x%x",
                  &Surface->node);

#if gcdENABLE_3D
    /* Unlock the hierarchical Z buffer. */
    if (Surface->hzNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            gcoHARDWARE_Unlock(&Surface->hzNode,
                               gcvSURF_HIERARCHICAL_DEPTH));

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Unlocked HZ surface 0x%x",
                      &Surface->hzNode);
    }

    /* Unlock the tile status buffer. */
    if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            gcoHARDWARE_Unlock(&Surface->tileStatusNode,
                               gcvSURF_TILE_STATUS));

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Unlocked tile status 0x%x",
                      &Surface->hzNode);
    }

    /* Unlock the hierarchical tile status buffer. */
    if (Surface->hzTileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            gcoHARDWARE_Unlock(&Surface->hzTileStatusNode,
                               gcvSURF_TILE_STATUS));

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Unlocked HZ tile status 0x%x",
                      &Surface->hzNode);
    }
#endif /* gcdENABLE_3D */

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the error. */
    return status;
}

static gceSTATUS
_FreeSurface(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* We only manage valid and non-user pools. */
    if (Surface->node.pool != gcvPOOL_UNKNOWN)
    {
        gceHARDWARE_TYPE currentType = gcvHARDWARE_INVALID;
        gcmGETCURRENTHARDWARE(currentType);

        /* Unlock the video memory with initial HW type */
        if (currentType != Surface->initType)
        {
            gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, Surface->initType));
        }
        gcmONERROR(_Unlock(Surface));
        if (currentType != Surface->initType)
        {
            gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, currentType));
        }

        if (Surface->node.u.normal.node != 0)
        {
            {
                /* Free the video memory. */
                gcmONERROR(gcsSURF_NODE_Destroy(&Surface->node));
            }
        }

        if (Surface->node2.u.normal.node != 0)
        {
            {
                /* Free the video memory. */
                gcmONERROR(gcsSURF_NODE_Destroy(&Surface->node2));
            }
        }

        if (Surface->node3.u.normal.node != 0)
        {
            {
                /* Free the video memory. */
                gcmONERROR(gcsSURF_NODE_Destroy(&Surface->node3));
            }
        }

        /* Mark the memory as freed. */
        Surface->node.pool = gcvPOOL_UNKNOWN;

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Freed surface 0x%x",
                      &Surface->node);
    }

#if gcdENABLE_3D
    if (Surface->hzNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Free the hierarchical Z video memory. */
        gcmONERROR(
            gcsSURF_NODE_Destroy(&Surface->hzNode));

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Freed HZ surface 0x%x",
                      &Surface->hzNode);
    }

    if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Free the tile status memory. */
        gcmONERROR(
            gcsSURF_NODE_Destroy(&Surface->tileStatusNode));

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Freed tile status 0x%x",
                      &Surface->tileStatusNode);
    }

    if (Surface->hzTileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Free the hierarchical Z tile status memory. */
        gcmONERROR(
            gcsSURF_NODE_Destroy(&Surface->hzTileStatusNode));

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Freed HZ tile status 0x%x",
                      &Surface->hzTileStatusNode);
    }
#endif /* gcdENABLE_3D */

    if (Surface->shBuf != gcvNULL)
    {
        /* Destroy shared buffer. */
        gcmVERIFY_OK(
            gcoHAL_DestroyShBuffer(Surface->shBuf));

        /* Mark it as freed. */
        Surface->shBuf = gcvNULL;
    }

#if gcdENABLE_3D
    if (Surface->fcValue != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, Surface->fcValue);
    }

    if (Surface->fcValueUpper != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, Surface->fcValueUpper);
    }

    if (Surface->tileStatusDisabled != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, Surface->tileStatusDisabled);
    }

    if (Surface->dirty != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, Surface->dirty);
    }
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_UnwrapUserMemory(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* We only manage valid and non-user pools. */
    if ((Surface->node.pool != gcvPOOL_UNKNOWN) &&
        (Surface->node.u.normal.node != 0))
    {
        {
            /* Unlock the video memory. */
            gcmONERROR(_Unlock(Surface));

            /* Free the video memory. */
            gcmONERROR(
                gcoHARDWARE_ScheduleVideoMemory(Surface->node.u.normal.node));
        }

        Surface->node.u.normal.node = 0;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


#if gcdENABLE_3D
#if gcdENABLE_BUFFER_ALIGNMENT

#if gcdENABLE_BANK_ALIGNMENT

#if !gcdBANK_BIT_START
#error gcdBANK_BIT_START not defined.
#endif

#if !gcdBANK_BIT_END
#error gcdBANK_BIT_END not defined.
#endif
#endif

/*******************************************************************************
**  _GetBankOffsetBytes
**
**  Return the bytes needed to offset sub-buffers to different banks.
**
**  ARGUMENTS:
**
**      gceSURF_TYPE Type
**          Type of buffer.
**
**      gctUINT32 TopBufferSize
**          Size of the top buffer, need\ed to compute offset of the second buffer.
**
**  OUTPUT:
**
**      gctUINT32_PTR Bytes
**          Pointer to a variable that will receive the byte offset needed.
**
*/
gceSTATUS
_GetBankOffsetBytes(
    IN gcoSURF Surface,
    IN gceSURF_TYPE Type,
    IN gctUINT32 TopBufferSize,
    OUT gctUINT32_PTR Bytes
    )

{
    gctUINT32 baseOffset = 0;
    gctUINT32 offset     = 0;

#if gcdENABLE_BANK_ALIGNMENT
    gctUINT32 bank;
    /* To retrieve the bank. */
    static const gctUINT32 bankMask = (0xFFFFFFFF << gcdBANK_BIT_START)
                                    ^ (0xFFFFFFFF << (gcdBANK_BIT_END + 1));
#endif

    gcmHEADER_ARG("Type=%d TopBufferSize=%x Bytes=0x%x", Type, TopBufferSize, Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes != gcvNULL);

    switch(Type)
    {
    case gcvSURF_RENDER_TARGET:
        /* Put second buffer atleast 16KB away. */
        baseOffset = offset = (1 << 14);

#if gcdENABLE_BANK_ALIGNMENT
        TopBufferSize += (1 << 14);
        bank = (TopBufferSize & bankMask) >> (gcdBANK_BIT_START);

        /* Put second buffer (c1 or z1) 5 banks away. */
        if (bank <= 5)
        {
            offset += (5 - bank) << (gcdBANK_BIT_START);
        }
        else
        {
            offset += (8 + 5 - bank) << (gcdBANK_BIT_START);
        }
#if gcdBANK_CHANNEL_BIT
        /* Minimum 256 byte alignment needed for fast_msaa or small msaa. */
        if ((gcdBANK_CHANNEL_BIT > 7) ||
            ((gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_FAST_MSAA) != gcvSTATUS_TRUE) &&
             (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SMALL_MSAA) != gcvSTATUS_TRUE)))

        {
            /* Add a channel offset at the channel bit. */
            offset += (1 << gcdBANK_CHANNEL_BIT);
        }

#endif
#endif
        break;

    case gcvSURF_DEPTH:
        /* Put second buffer atleast 16KB away. */
        baseOffset = offset = (1 << 14);

#if gcdENABLE_BANK_ALIGNMENT
        TopBufferSize += (1 << 14);
        bank = (TopBufferSize & bankMask) >> (gcdBANK_BIT_START);

        /* Put second buffer (c1 or z1) 5 banks away. */
        if (bank <= 5)
        {
            offset += (5 - bank) << (gcdBANK_BIT_START);
        }
        else
        {
            offset += (8 + 5 - bank) << (gcdBANK_BIT_START);
        }

#if gcdBANK_CHANNEL_BIT
        /* Subtract the channel bit, as it's added by kernel side. */
        if (offset >= (1 << gcdBANK_CHANNEL_BIT))
        {
            /* Minimum 256 byte alignment needed for fast_msaa or small msaa. */
            if ((gcdBANK_CHANNEL_BIT > 7) ||
                ((gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_FAST_MSAA) != gcvSTATUS_TRUE) &&
                 (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SMALL_MSAA) != gcvSTATUS_TRUE)))
            {
                offset -= (1 << gcdBANK_CHANNEL_BIT);
            }
        }
#endif
#endif

        break;

    default:
        /* No alignment needed. */
        baseOffset = offset = 0;
    }

    *Bytes = offset;

    /* Avoid compiler warnings. */
#ifndef __clang__
    baseOffset = baseOffset;
#endif

    /* Only disable bottom-buffer-offset on android system. */
#if gcdPARTIAL_FAST_CLEAR && defined(ANDROID)
    if (!Surface->isMsaa)
    {
        /* In NOAA mode, disable extra bottom-buffer-offset if want
         * partial fast clear. 'baseOffset' is 16KB aligned, it can still
         * support. */
        *Bytes = baseOffset;
    }
#endif

    gcmFOOTER_ARG("*Bytes=0x%x", *Bytes);
    return gcvSTATUS_OK;
}

#endif
#endif

static void
_ComputeSurfacePlacement(
    gcoSURF Surface,
    gctBOOL calcStride
    )
{
    gctUINT32 blockSize;
    gcsSURF_FORMAT_INFO_PTR formatInfo = &Surface->formatInfo;
    blockSize = formatInfo->blockSize / formatInfo->layers;

    switch (Surface->format)
    {
    case gcvSURF_YV12:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        /*  WxH Y plane followed by (W/2)x(H/2) V and U planes. */
        Surface->uStride =
        Surface->vStride
#if defined(ANDROID)
            /*
             * Per google's requirement, we need u/v plane align to 16,
             * and there is no gap between YV plane
             */
            = (Surface->stride / 2 + 0xf) & ~0xf;
#else
            = (Surface->stride / 2);
        if(gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_2D_YUV420_OUTPUT_LINEAR))
        {
            /*If YUV420_OUTPUT_LINEAR feature is enabled,For YV12 format, Y need to 64 Bytes alignment,
                U/V need 32 Bytes alignment*/
            Surface->stride = gcmALIGN(Surface->stride, 64);
            Surface->uStride = gcmALIGN(Surface->uStride, 32);
            Surface->vStride = gcmALIGN(Surface->vStride, 32);
        }
#endif

        Surface->vOffset
            = Surface->stride * Surface->alignedH;

        Surface->uOffset
            = Surface->vOffset
            + Surface->vStride * Surface->alignedH / 2;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH / 2;
        break;
#if gcdVG_ONLY
    case gcvSURF_YV16:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        /*  WxH Y plane followed by (W/2)x(H/2) V and U planes. */
        Surface->uStride =
        Surface->vStride
            = (Surface->stride / 2);

        Surface->vOffset
            = Surface->stride * Surface->alignedH;

        Surface->uOffset
            = Surface->vOffset
            + Surface->vStride * Surface->alignedH;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH;
        break;
#endif

    case gcvSURF_I420:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        /*  WxH Y plane followed by (W/2)x(H/2) U and V planes. */
        Surface->uStride =
        Surface->vStride
#if defined(ANDROID)
            /*
             * Per google's requirement, we need u/v plane align to 16,
             * and there is no gap between YV plane
             */
            = (Surface->stride / 2 + 0xf) & ~0xf;
#else
            = (Surface->stride / 2);
        if(gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_2D_YUV420_OUTPUT_LINEAR))
        {
            /*If YUV420_OUTPUT_LINEAR feature is enabled,For I420 format, Y need to 64 Bytes alignment,
                 U/V need 32 Bytes alignment*/
            Surface->stride = gcmALIGN(Surface->stride, 64);
            Surface->uStride = gcmALIGN(Surface->uStride, 32);
            Surface->vStride = gcmALIGN(Surface->vStride, 32);
        }
#endif

        Surface->uOffset
            = Surface->stride * Surface->alignedH;

        Surface->vOffset
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH / 2;

        Surface->sliceSize
            = Surface->vOffset
            + Surface->vStride * Surface->alignedH / 2;
        break;

    case gcvSURF_NV12:
    case gcvSURF_NV21:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        if(gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_2D_YUV420_OUTPUT_LINEAR))
        {
            /*If YUV420_OUTPUT_LINEAR feature is enabled,For NV12/NV21 format ,
                 Y/UV need to 64 Bytes alignment*/
            Surface->stride = gcmALIGN(Surface->stride, 64);
        }
        /*  WxH Y plane followed by (W)x(H/2) interleaved U/V plane. */
        Surface->uStride =
        Surface->vStride = Surface->stride;

        Surface->uOffset =
        Surface->vOffset = Surface->stride * Surface->alignedH;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH / 2;
        break;

    case gcvSURF_NV16:
    case gcvSURF_NV61:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        if(gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_2D_YUV420_OUTPUT_LINEAR))
        {
            /*If YUV420_OUTPUT_LINEAR feature is enabled,For NV16/NV61 format,
                 Y/UV need to 64 Bytes alignment*/
            Surface->stride  = gcmALIGN(Surface->stride, 64);
        }
        /*  WxH Y plane followed by WxH interleaved U/V(V/U) plane. */
        Surface->uStride =
        Surface->vStride = Surface->stride;

        Surface->uOffset =
        Surface->vOffset = Surface->stride * Surface->alignedH;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH;
        break;

    case gcvSURF_YUY2:
    case gcvSURF_UYVY:
    case gcvSURF_YVYU:
    case gcvSURF_VYUY:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW * 2;
        }

        /*  WxH interleaved Y/U/Y/V plane. */
        Surface->uStride =
        Surface->vStride = Surface->stride;

        Surface->uOffset = Surface->vOffset = 0;

        Surface->sliceSize = Surface->stride * Surface->alignedH;
        break;

    case gcvSURF_YUV420_TILE_ST:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        /*  WxH Y plane followed by (W)x(H/2) interleaved U/V plane. */
        Surface->uStride =
        Surface->vStride = Surface->stride;

        Surface->uOffset =
        Surface->vOffset = Surface->stride * Surface->alignedH;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH / 2;
        break;

    case gcvSURF_YUV420_10_ST:
    case gcvSURF_YUV420_TILE_10_ST:
    case gcvSURF_NV12_10BIT:
    case gcvSURF_NV21_10BIT:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW * 10 / 8;
        }

        /*  WxH Y plane followed by WxH interleaved U/V(V/U) plane. */
        Surface->uStride =
        Surface->vStride = Surface->stride;

        Surface->uOffset =
        Surface->vOffset = Surface->stride * Surface->alignedH;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH / 2;
        break;

    case gcvSURF_NV16_10BIT:
    case gcvSURF_NV61_10BIT:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW * 10 / 8;
        }

        Surface->stride  =
        Surface->uStride =
        Surface->vStride = Surface->stride;

        Surface->uOffset =
        Surface->vOffset = Surface->stride * Surface->alignedH;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH;
        break;

#if gcdVG_ONLY
    case gcvSURF_ANV12:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        /*  NV12 + Alpha: 8bit alpha per pixel. */
        Surface->uStride =
        Surface->vStride =
        Surface->aStride = Surface->stride;

        Surface->uOffset =
        Surface->vOffset = Surface->stride * Surface->alignedH;

        Surface->aOffset
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH / 2;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH / 2
            + Surface->aStride * Surface->alignedH;
        break;

    case gcvSURF_ANV16:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW;
        }

        /* NV16 + Alpha: 8bit alpha per pixel. */
        Surface->uStride =
        Surface->vStride =
        Surface->aStride = Surface->stride;

        Surface->uOffset =
        Surface->vOffset = Surface->stride * Surface->alignedH;

        Surface->aOffset = Surface->uOffset
            + Surface->uStride * Surface->alignedH;

        Surface->sliceSize
            = Surface->uOffset
            + Surface->uStride * Surface->alignedH
            + Surface->aStride * Surface->alignedH;
        break;

    case gcvSURF_AUYVY:
    case gcvSURF_AYUY2:
        if (calcStride)
        {
            Surface->stride = Surface->alignedW * 2;
        }

        /*  WxH interleaved Y/U/Y/V plane. */
        Surface->uStride =
        Surface->vStride = Surface->stride;
        Surface->aStride = Surface->stride / 2;

        Surface->uOffset = Surface->vOffset = 0;
        Surface->aOffset = Surface->stride * Surface->alignedH;

        Surface->sliceSize
            = Surface->stride * Surface->alignedH
            + Surface->aStride * Surface->alignedH;
        break;

    case gcvSURF_INDEX1:    /* stride aligned by 8 bytes. */
        if (calcStride)
        {
            Surface->stride = gcmALIGN((Surface->alignedW / 8), 8);
        }

        Surface->uStride = Surface->vStride = 0;
        Surface->uOffset = Surface->vOffset = 0;

        Surface->sliceSize = Surface->stride * Surface->alignedH;
        break;

    case gcvSURF_INDEX2:    /* stride aligned by 8 bytes. */
        if (calcStride)
        {
            Surface->stride = gcmALIGN((Surface->alignedW / 4), 8);
        }

        Surface->uStride = Surface->vStride = 0;
        Surface->uOffset = Surface->vOffset = 0;

        Surface->sliceSize = Surface->stride * Surface->alignedH;
        break;

    case gcvSURF_INDEX4:    /* stride aligned by 8 bytes. */
        if (calcStride)
        {
            Surface->stride = gcmALIGN((Surface->alignedW / 2), 8);
        }

        Surface->uStride = Surface->vStride = 0;
        Surface->uOffset = Surface->vOffset = 0;

        Surface->sliceSize = Surface->stride * Surface->alignedH;
        break;

    case gcvSURF_INDEX8:    /* stride aligned by 8 bytes. */
        if (calcStride)
        {
            Surface->stride = gcmALIGN(Surface->alignedW, 16);
        }

        Surface->uStride = Surface->vStride = 0;
        Surface->uOffset = Surface->vOffset = 0;

        Surface->sliceSize = Surface->stride * Surface->alignedH;
        break;
#endif

    default:
        if (calcStride)
        {
            Surface->stride
                = (Surface->alignedW / formatInfo->blockWidth)
                * blockSize / 8;
        }

        Surface->uStride = Surface->vStride = 0;
        Surface->uOffset = Surface->vOffset = 0;

        Surface->sliceSize
            = Surface->stride
            * (Surface->alignedH / formatInfo->blockHeight);
        break;
    }

    /* HW need read 64bytes at least for YUV420 tex.*/
    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_YUV420_TILER)            &&
        (Surface->format >= gcvSURF_YUY2) && (Surface->format <= gcvSURF_AYUV) &&
        ((Surface->stride & (64 -1)) != 0))
    {
        Surface->sliceSize += 64;
    }
}

static gceSTATUS
_AllocateSurface(
    IN gcoSURF Surface,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gctUINT Samples,
    IN gcePOOL Pool
    )
{
    gceSTATUS status;
    gceSURF_FORMAT format;
    gcsSURF_FORMAT_INFO_PTR formatInfo;
    /* Extra pages needed to offset sub-buffers to different banks. */
    gctUINT32 bankOffsetBytes = 0;
    gctUINT32 layers;

#if gcdENABLE_3D
    gctUINT i;
    gctUINT32 blockSize;
#endif


    format = (gceSURF_FORMAT) (Format & ~(gcvSURF_FORMAT_OCL | gcvSURF_FORMAT_PATCH_BORDER));
    gcmONERROR(gcoSURF_QueryFormat(format, &formatInfo));
    Surface->formatInfo = *formatInfo;
    layers = formatInfo->layers;
#if gcdENABLE_3D
    blockSize = formatInfo->blockSize / layers;
#endif

    {
        Surface->bitsPerPixel = formatInfo->bitsPerPixel;
    }

    if (Samples > 4 || Samples == 3)
    {
        /* Invalid multi-sample request. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }
    Surface->sampleInfo = g_sampleInfos[Samples];
    Surface->isMsaa     = (Surface->sampleInfo.product > 1);

    /* Set dimensions of surface. */
    Surface->requestW = Width;
    Surface->requestH = Height;
    Surface->requestD = Depth;
    Surface->allocedW = Width  * Surface->sampleInfo.x;
    Surface->allocedH = Height * Surface->sampleInfo.y;

#if gcdENABLE_3D
    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctUINT) * Surface->requestD, (gctPOINTER*)&Surface->fcValue));
    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctUINT) * Surface->requestD, (gctPOINTER*)&Surface->fcValueUpper));
    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctBOOL) * Surface->requestD, (gctPOINTER*)&Surface->tileStatusDisabled));
    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctBOOL) * Surface->requestD, (gctPOINTER*)&Surface->dirty));

    gcoOS_ZeroMemory(Surface->fcValue, gcmSIZEOF(gctUINT) * Surface->requestD);
    gcoOS_ZeroMemory(Surface->fcValueUpper, gcmSIZEOF(gctUINT) * Surface->requestD);
    gcoOS_ZeroMemory(Surface->tileStatusDisabled, gcmSIZEOF(gctBOOL) * Surface->requestD);
    gcoOS_ZeroMemory(Surface->dirty, gcmSIZEOF(gctBOOL) * Surface->requestD);
#endif
    /* Initialize rotation. */
    Surface->rotation    = gcvSURF_0_DEGREE;
#if gcdENABLE_3D
    Surface->orientation = gcvORIENTATION_TOP_BOTTOM;
    Surface->resolvable  = gcvTRUE;
#endif

    /* Obtain canonical type of surface. */
    Surface->type   = (gceSURF_TYPE) ((gctUINT32) Type & 0xFF);
    /* Get 'hints' of this surface. */
    Surface->hints  = (gceSURF_TYPE) ((gctUINT32) Type & ~0xFF);
    /* Append texture surface flag */
    Surface->hints |= (Surface->type == gcvSURF_TEXTURE) ? gcvSURF_CREATE_AS_TEXTURE : 0;
    /* Set format of surface. */
    Surface->format = format;
    Surface->tiling = (Surface->type == gcvSURF_TEXTURE)
                         ? (((Surface->hints & gcvSURF_LINEAR) == gcvSURF_LINEAR)
                            ? gcvLINEAR
                            : gcvTILED)
                         : gcvLINEAR
                         ;

    if (Surface->hints & gcvSURF_CACHE_MODE_128)
    {
        Surface->cacheMode  = gcvCACHE_128;
    }
    else
    {
        Surface->cacheMode  = gcvCACHE_256;
    }

    /* Set aligned surface size. */
    Surface->alignedW = Surface->allocedW;
    Surface->alignedH = Surface->allocedH;

#if gcdENABLE_3D
    for (i = 0; i < Surface->requestD; i++)
    {
        /* Tile status disabled currently. */
        Surface->tileStatusDisabled[i] = gcvTRUE;
    }

    /* Init superTiled info. */
    Surface->superTiled = gcvFALSE;
#endif

    /* User pool? */
    if (Pool == gcvPOOL_USER)
    {
        /* Init the node as the user allocated. */
        Surface->node.pool                    = gcvPOOL_USER;
        Surface->node.u.normal.node           = 0;

        /* Align the dimensions by the block size. */
        Surface->alignedW  = gcmALIGN_NP2(Surface->alignedW,
                                                   formatInfo->blockWidth);
        Surface->alignedH = gcmALIGN_NP2(Surface->alignedH,
                                                   formatInfo->blockHeight);

        /* Always single layer for user surface */
        gcmASSERT(layers == 1);

        /* Compute the surface placement parameters. */
        _ComputeSurfacePlacement(Surface, gcvTRUE);

        Surface->layerSize = Surface->sliceSize * Surface->requestD;

        Surface->size = Surface->layerSize * layers;

        if (Surface->type == gcvSURF_TEXTURE)
        {
            Surface->tiling = gcvTILED;
        }
    }

    /* No --> allocate video memory. */
    else
    {
        gctUINT32 extraSize = 0;
        {
            /* Align width and height to tiles. */
#if gcdENABLE_3D
            if (Surface->isMsaa &&
                ((Surface->type != gcvSURF_TEXTURE) ||
                 (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_MSAA_TEXTURE)))
               )
            {
                gcmONERROR(
                    gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                      Surface->type,
                                                      Surface->hints,
                                                      Format,
                                                      &Width,
                                                      &Height,
                                                      Depth,
                                                      &Surface->tiling,
                                                      &Surface->superTiled,
                                                      &Surface->hAlignment));

                Surface->alignedW = Width  * Surface->sampleInfo.x;
                Surface->alignedH = Height * Surface->sampleInfo.y;
            }
            else
            {
                gcmONERROR(
                    gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                      Surface->type,
                                                      Surface->hints,
                                                      Format,
                                                      &Surface->alignedW,
                                                      &Surface->alignedH,
                                                      Depth,
                                                      &Surface->tiling,
                                                      &Surface->superTiled,
                                                      &Surface->hAlignment));
            }
#else
            gcmONERROR(
                gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                  Surface->type,
                                                  Surface->hints,
                                                  Format,
                                                  &Surface->alignedW,
                                                  &Surface->alignedH,
                                                  Depth,
                                                  &Surface->tiling,
                                                  gcvNULL,
                                                  gcvNULL));
#endif /* gcdENABLE_3D */
        }

        /*
        ** We cannot use multi tiled/supertiled to create 3D surface, this surface cannot be recognized
        ** by texture unit now.
        */
        if ((Depth > 1) &&
            (Surface->tiling & gcvTILING_SPLIT_BUFFER))
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
        /* Determine bank offset bytes. */
#if gcdENABLE_3D
        /* If HW need to be programmed as multi pipe with 2 addresses,
         * bottom addresses need to calculated now, no matter the surface itself
         * was split or not.
         */
        if (Surface->tiling & gcvTILING_SPLIT_BUFFER)
        {
            gctUINT halfHeight = gcmALIGN(Surface->alignedH / 2,
                                          Surface->superTiled ? 64 : 4);

            gctUINT32 topBufferSize = ((Surface->alignedW/ formatInfo->blockWidth)
                                    *  (halfHeight/ formatInfo->blockHeight)
                                    *  blockSize) / 8;

#if gcdENABLE_BUFFER_ALIGNMENT
            if (Surface->tiling & gcvTILING_SPLIT_BUFFER)
            {
                gcmONERROR(
                    _GetBankOffsetBytes(Surface,
                                        Surface->type,
                                        topBufferSize,
                                        &bankOffsetBytes));
            }
#endif
            Surface->bottomBufferOffset = topBufferSize + bankOffsetBytes;
        }
        else
#endif
        {
            Surface->bottomBufferOffset = 0;
        }

        /* Compute the surface placement parameters. */
        _ComputeSurfacePlacement(Surface, gcvTRUE);

        /* Append bank offset bytes. */
        Surface->sliceSize += bankOffsetBytes;

        if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_128BTILE) &&
            (Surface->formatInfo.fmtClass != gcvFORMAT_CLASS_ASTC))
        {
            /* Align to 256B */
            Surface->sliceSize = gcmALIGN(Surface->sliceSize, 256);
        }

        Surface->layerSize = Surface->sliceSize * Surface->requestD;

        if (Format & gcvSURF_FORMAT_PATCH_BORDER)
        {
            gctUINT32 tileSize;
            gctUINT32 tileCountInOneLine;

            gcmASSERT(gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_BORDER_CLAMP));
            gcmASSERT(!gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_BORDER_CLAMP_FIX));
            switch (Surface->tiling)
            {
            case gcvSUPERTILED:
                tileSize = ((64 * 64 * Surface->formatInfo.bitsPerPixel) >> 3);
                tileCountInOneLine = (Surface->alignedW / 64);
                break;
            case gcvTILED:
                tileSize = 64;
                switch (Surface->formatInfo.bitsPerPixel)
                {
                case 8:
                case 16:
                    tileCountInOneLine = Surface->alignedW / 8;
                    break;
                case 32:
                case 64:
                    tileCountInOneLine = Surface->alignedW / 4;
                    break;
                default:
                    tileCountInOneLine = Surface->alignedW / 4;
                }
                break;
            case gcvLINEAR:
                tileSize = (Surface->formatInfo.bitsPerPixel >> 3);
                tileCountInOneLine = Surface->alignedW;
                break;
            default:
                gcmASSERT(0);
                tileSize = 64;
                tileCountInOneLine = Surface->alignedW;
                break;
            }
            if  (Type & gcvSURF_3D)
            {
                extraSize = Surface->sliceSize;
            }

            if (Surface->alignedW == Surface->allocedW)
            {
                extraSize += tileSize;
            }

            if (Surface->alignedH == Surface->allocedH)
            {
                extraSize += tileCountInOneLine * tileSize;
            }
        }
        else if ((Type == gcvSURF_TEXTURE) &&
                 !gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SH_IMAGE_LD_LAST_PIXEL_FIX) &&
                  gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_HALTI5) &&
                  (Surface->alignedH == Surface->allocedH) &&
                  (Surface->alignedW == Surface->allocedW))
        {
            extraSize = 15;
        }

        if (Surface->layerSize)
        {
            Surface->size = (Surface->layerSize + extraSize) * layers;
        }
    }

    if (Surface->formatInfo.fmtClass == gcvFORMAT_CLASS_ASTC)
    {
        if (Surface->alignedH & 0x3)
        {
            gctUINT32 alignedTileWidth = gcmALIGN(Surface->allocedW, 4);
            gctUINT32 alignedBlockWidth = gcmALIGN(alignedTileWidth, Surface->formatInfo.blockWidth);
            gctUINT32 extraSize = (alignedBlockWidth / Surface->formatInfo.blockWidth) * (Surface->formatInfo.blockSize / 8);
            Surface->size += extraSize;
        }
        else if (Surface->alignedW & 0x3)
        {
            Surface->size += 16;
        }

        Surface->size = gcmALIGN(Surface->size, 64);
    }

    if (!(Surface->hints & gcvSURF_NO_VIDMEM) && (Pool != gcvPOOL_USER))
    {
        gctUINT bytes = Surface->size;
        gctUINT alignment = 0;
        gctUINT32 allocFlags = gcvALLOC_FLAG_NONE;

#if gcdENABLE_3D
        if ((gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TILE_FILLER)) &&
            (gcoHARDWARE_QuerySurfaceRenderable(gcvNULL, Surface) == gcvSTATUS_OK) &&
            Depth == 1 )
        {
            if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_TILEFILLER_32TILE_ALIGNED))
            {
                /* 32 tile alignment. */
                bytes = gcmALIGN(bytes, 32 * (Surface->isMsaa ? 256 : 64));
            }
            else
            {
                /* 256 tile alignment for fast clear fill feature. */
                bytes = gcmALIGN(bytes, 256 * (Surface->isMsaa ? 256 : 64));
            }
        }

        if (Surface->formatInfo.fmtClass == gcvFORMAT_CLASS_ASTC)
        {
            /* for ASTC format, the address should be 16 Byte aligned. */
            alignment = 64;
        }
        else if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_128BTILE))
        {
            bytes = gcmALIGN(bytes,256);

            alignment = 256;
        }
        else if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_COLOR_COMPRESSION))
        {
            /* - Depth surfaces need 256 byte alignment.
             * - Color compression need 256 byte alignment(same in all versions?)
             *   Doing this generically.
             */
            alignment = 256;
        }
        else if (Surface->isMsaa)
        {
            /* 256 byte alignment for MSAA surface */
            alignment = 256;
        }
        else if (Surface->hints & gcvSURF_DEC)
        {
            /* 128 byte alignment for DEC format surface */
            alignment = 128;
        }
        else
#endif
        {
            /* alignment should be 16(pixels) * byte per pixels for tiled surface*/
            alignment = (formatInfo->bitsPerPixel >= 64) ? (4*4*formatInfo->bitsPerPixel/8) : 64;
        }


        if (Surface->hints & gcvSURF_PROTECTED_CONTENT)
        {
            allocFlags |= gcvALLOC_FLAG_SECURITY;
        }

        if (Surface->hints & gcvSURF_CONTIGUOUS)
        {
            allocFlags |= gcvALLOC_FLAG_CONTIGUOUS;
        }

        if (Surface->hints & gcvSURF_CACHEABLE)
        {
            allocFlags |= gcvALLOC_FLAG_CACHEABLE;
        }

        if (Surface->hints & gcvSURF_DMABUF_EXPORTABLE)
        {
            allocFlags |= gcvALLOC_FLAG_DMABUF_EXPORTABLE;
        }

        if (Format & gcvSURF_FORMAT_OCL)
        {
            bytes = gcmALIGN(bytes + 64,  64);
        }


        gcmONERROR(gcsSURF_NODE_Construct(
            &Surface->node,
            bytes,
            alignment,
            Surface->type,
            allocFlags,
            Pool
            ));

        gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                      "Allocated surface 0x%x: pool=%d size=%dx%dx%d bytes=%u",
                      &Surface->node,
                      Surface->node.pool,
                      Surface->alignedW,
                      Surface->alignedH,
                      Surface->requestD,
                      Surface->size);
    }

#if gcdENABLE_3D
    /* No Hierarchical Z buffer allocated. */
    gcmONERROR(gcoSURF_AllocateHzBuffer(Surface));

    if ((Pool != gcvPOOL_USER) &&
        ((Surface->type == gcvSURF_DEPTH) ||
         (Surface->type == gcvSURF_RENDER_TARGET)))
    {
        /* Allocate tile status buffer after HZ buffer
        ** b/c we need allocate HZ tile status if HZ exist.
        */
        gcmONERROR(gcoSURF_AllocateTileStatus(Surface));
    }

    Surface->hasStencilComponent = (format == gcvSURF_D24S8             ||
                                    format == gcvSURF_S8D32F            ||
                                    format == gcvSURF_S8D32F_2_A8R8G8B8 ||
                                    format == gcvSURF_S8D32F_1_G32R32F  ||
                                    format == gcvSURF_D24S8_1_A8R8G8B8  ||
                                    format == gcvSURF_S8                ||
                                    format == gcvSURF_X24S8);

    Surface->canDropStencilPlane = gcvTRUE;
#endif

    if (Type & gcvSURF_CACHEABLE)
    {
        gcmASSERT(Pool != gcvPOOL_USER);
        Surface->node.u.normal.cacheable = gcvTRUE;
    }
    else if (Pool != gcvPOOL_USER)
    {
        Surface->node.u.normal.cacheable = gcvFALSE;
    }

    if (Pool != gcvPOOL_USER)
    {
        if (!(Surface->hints & gcvSURF_NO_VIDMEM))
        {
            /* Lock the surface. */
            gcmONERROR(_Lock(Surface));
#if gcdDUMP
            {
                gctUINT32 address;
                gcmDUMP(gcvNULL, "#[info: initialize surface node memory at create time]");
                gcmGETHARDWAREADDRESS(Surface->node, address);
                gcmDUMP_BUFFER(gcvNULL,
                               gcvDUMP_BUFFER_MEMORY,
                               address,
                               Surface->node.logical,
                               0,
                               Surface->node.size);

                if (Surface->node2.pool != gcvPOOL_UNKNOWN)
                {
                    gcmDUMP(gcvNULL, "#[info node2 buffer]");
                    gcmGETHARDWAREADDRESS(Surface->node2, address);
                    gcmDUMP_BUFFER(gcvNULL,
                                   gcvDUMP_BUFFER_MEMORY,
                                   address,
                                   Surface->node2.logical,
                                   0,
                                   Surface->node2.size);

                }

                if (Surface->node3.pool != gcvPOOL_UNKNOWN)
                {
                    gcmDUMP(gcvNULL, "#[info node3 buffer]");
                    gcmGETHARDWAREADDRESS(Surface->node3, address);
                    gcmDUMP_BUFFER(gcvNULL,
                                   gcvDUMP_BUFFER_MEMORY,
                                   address,
                                   Surface->node3.logical,
                                   0,
                                   Surface->node3.size);

                }
            }

#endif

        }
    }

    Surface->pfGetAddr = gcoHARDWARE_GetProcCalcPixelAddr(gcvNULL, Surface);

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Free the memory allocated to the surface. */
    _FreeSurface(Surface);

    /* Return the status. */
    return status;
}


/******************************************************************************\
******************************** gcoSURF API Code *******************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoSURF_Construct
**
**  Create a new gcoSURF object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctUINT Width
**          Width of surface to create in pixels.
**
**      gctUINT Height
**          Height of surface to create in lines.
**
**      gctUINT Depth
**          Depth of surface to create in planes.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gceSURF_FORMAT Format
**          Format of surface to create.
**
**      gcePOOL Pool
**          Pool to allocate surface from.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to the variable that will hold the gcoSURF object pointer.
*/
gceSTATUS
gcoSURF_Construct(
    IN gcoHAL Hal,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gcePOOL Pool,
    OUT gcoSURF * Surface
    )
{
    gcoSURF surface = gcvNULL;
    gceSTATUS status;
    gceSURF_FORMAT format;

    gcmHEADER_ARG("Width=%u Height=%u Depth=%u Type=%d Format=%d Pool=%d",
                  Width, Height, Depth, Type, Format, Pool);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    /* Allocate the gcoSURF object. */
    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(struct _gcoSURF), (gctPOINTER*)&surface));
    gcoOS_ZeroMemory(surface, gcmSIZEOF(struct _gcoSURF));

    format = Format & ~(gcvSURF_FORMAT_OCL | gcvSURF_FORMAT_PATCH_BORDER);
    /* Initialize the gcoSURF object.*/
    surface->object.type        = gcvOBJ_SURF;
    surface->dither2D      = gcvFALSE;
    surface->deferDither3D = gcvFALSE;
    surface->paddingFormat = (format == gcvSURF_R8_1_X8R8G8B8 || format == gcvSURF_G8R8_1_X8R8G8B8 ||
                              format == gcvSURF_A8L8_1_A8R8G8B8)
                           ? gcvTRUE : gcvFALSE;
    surface->garbagePadded = gcvTRUE;

#if gcdENABLE_3D
    surface->colorType = gcvSURF_COLOR_UNKNOWN;
    surface->flags = gcvSURF_FLAG_NONE;
    surface->colorSpace = gcd_QUERY_COLOR_SPACE(format);
    surface->hzNode.pool = gcvPOOL_UNKNOWN;
    surface->tileStatusNode.pool = gcvPOOL_UNKNOWN;
    surface->hzTileStatusNode.pool = gcvPOOL_UNKNOWN;
#endif /* gcdENABLE_3D */

#if gcdENABLE_3D
    if (Type & gcvSURF_TILE_STATUS_DIRTY)
    {
        surface->TSDirty = gcvTRUE;
        Type &= ~gcvSURF_TILE_STATUS_DIRTY;
    }
#endif

    if (Depth < 1)
    {
        /* One plane. */
        Depth = 1;
    }

    /* Allocate surface. */
    gcmONERROR(
        _AllocateSurface(surface,
                         Width, Height, Depth,
                         Type,
                         Format,
                         1,
                         Pool));

    surface->refCount = 1;

    gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                  "Created gcoSURF 0x%x",
                  surface);

    /* Return pointer to the gcoSURF object. */
    *Surface = surface;

    /* Success. */
    gcmFOOTER_ARG("*Surface=0x%x", *Surface);
    return gcvSTATUS_OK;

OnError:
    /* Free the allocated memory. */
    if (surface != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, surface);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Destroy
**
**  Destroy an gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Destroy(
    IN gcoSURF Surface
    )
{
#if gcdENABLE_3D
    gcsTLS_PTR tls;
#endif

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Decrement surface reference count. */
    if (--Surface->refCount != 0)
    {
        /* Still references to this surface. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

#if gcdENABLE_3D

    if (gcmIS_ERROR(gcoOS_GetTLS(&tls)))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }


    /* Special case for 3D surfaces. */
    if (tls->engine3D != gcvNULL)
    {
        /* If this is a render target, unset it. */
        if ((Surface->type == gcvSURF_RENDER_TARGET)
        ||  (Surface->type == gcvSURF_TEXTURE)
        )
        {
            gctUINT32 rtIdx;

            for (rtIdx = 0; rtIdx < 4; ++rtIdx)
            {
                gcmVERIFY_OK(gco3D_UnsetTarget(tls->engine3D, rtIdx, Surface));
            }
        }

        /* If this is a depth buffer, unset it. */
        else if (Surface->type == gcvSURF_DEPTH)
        {
            gcmVERIFY_OK(gco3D_UnsetDepth(tls->engine3D, Surface));
        }
    }
#endif

#if gcdGC355_MEM_PRINT
    gcoOS_AddRecordAllocation(-(gctINT32)Surface->node.size);
#endif

    /* Free the video memory. */
    gcmVERIFY_OK(_FreeSurface(Surface));

    /* Mark gcoSURF object as unknown. */
    Surface->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcoSURF object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Surface));

    gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                  "Destroyed gcoSURF 0x%x",
                  Surface);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_QueryVidMemNode
**
**  Query the video memory node attributes of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gctUINT32 * Node
**          Pointer to a variable receiving the video memory node.
**
**      gcePOOL * Pool
**          Pointer to a variable receiving the pool the video memory node originated from.
**
**      gctUINT_PTR Bytes
**          Pointer to a variable receiving the video memory node size in bytes.
**
*/
gceSTATUS
gcoSURF_QueryVidMemNode(
    IN gcoSURF Surface,
    OUT gctUINT32 * Node,
    OUT gcePOOL * Pool,
    OUT gctSIZE_T_PTR Bytes,
    OUT gctUINT32 * TsNode,
    OUT gcePOOL * TsPool,
    OUT gctSIZE_T_PTR TsBytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Return the video memory attributes. */
    if (Node)
    {
        *Node = Surface->node.u.normal.node;
    }
    if (Pool)
    {
        *Pool = Surface->node.pool;
    }
    if (Bytes)
    {
        *Bytes = Surface->node.size;
    }
#if gcdENABLE_3D
    if (TsNode)
    {
        *TsNode = Surface->tileStatusNode.u.normal.node;
    }
    if (TsPool)
    {
        *TsPool = Surface->tileStatusNode.pool;
    }
    if (TsBytes)
    {
        *TsBytes = Surface->tileStatusNode.size;
    }
#endif

    /* Success. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoSURF_WrapSurface
**
**  Wrap gcoSURF_Object with known logica address (CPU) and physical address(GPU)
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object to be destroyed.
**
**      gctUINT Alignment
**          Alignment of each pixel row in bytes.
**
**      gctPOINTER Logical
**          Logical pointer to the user allocated surface or gcvNULL if no
**          logical pointer has been provided.
**
**      gctUINT32 Address
**          GPU address to the user allocated surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_WrapSurface(
    IN gcoSURF Surface,
    IN gctUINT Alignment,
    IN gctPOINTER Logical,
    IN gctUINT32 Address
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 address;

    gceHARDWARE_TYPE currentType = gcvHARDWARE_INVALID;

    gcmHEADER_ARG("Surface=0x%x Alignment=%u Logical=0x%x Address=%08x",
              Surface, Alignment, Logical, Address);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        /* Has to be user-allocated surface. */
        if (Surface->node.pool != gcvPOOL_USER)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        gcmGETCURRENTHARDWARE(currentType);

        /* Already mapped? */
        if (Surface->node.lockCounts[currentType][gcvENGINE_RENDER] > 0)
        {
            if ((Logical != gcvNULL) &&
                (Logical != Surface->node.logical))
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            gcmGETHARDWAREADDRESS(Surface->node, address);

            if ((Address != gcvINVALID_ADDRESS) &&
                (Address != address))
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            /* Success. */
            break;
        }

        /* Set new alignment. */
        if (Alignment != 0)
        {
            /* Compute the HAL required stride */
            _ComputeSurfacePlacement(Surface, gcvTRUE);

            /* Align the stride (Alignment can be not a power of number). */
            Surface->stride = gcmALIGN_NP2(Surface->stride, Alignment);

            /* Compute surface placement with user stride */
            _ComputeSurfacePlacement(Surface, gcvFALSE);

            Surface->layerSize = Surface->sliceSize * Surface->requestD;

            /* We won't map multi-layer surface. */
            gcmASSERT(Surface->formatInfo.layers == 1);

            /* Compute the new size. */
            Surface->size = Surface->layerSize * Surface->formatInfo.layers;
        }

        /* Validate the surface. */
        Surface->node.valid = gcvTRUE;

        /* Set the lock count. */
        Surface->node.lockCounts[currentType][gcvENGINE_RENDER]++;

        /* Set the node parameters. */
        Surface->node.u.normal.node = 0;

        Surface->node.logical                 = Logical;
        gcsSURF_NODE_SetHardwareAddress(&Surface->node, Address);
        Surface->node.count                   = 1;

        if (Address != gcvINVALID_ADDRESS)
        {
            Surface->node.u.wrapped.physical = Address;
        }
        else
        {
            Surface->node.u.wrapped.physical = gcvINVALID_PHYSICAL_ADDRESS;
        }
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoSURF_MapUserSurface
**
**  Store the logical and physical pointers to the user-allocated surface in
**  the gcoSURF object and map the pointers as necessary.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object to be destroyed.
**
**      gctUINT Alignment
**          Alignment of each pixel row in bytes.
**
**      gctPOINTER Logical
**          Logical pointer to the user allocated surface or gcvNULL if no
**          logical pointer has been provided.
**
**      gctPHYS_ADDR_T Physical
**          Physical pointer to the user allocated surface or gcvINVALID_PHYSICAL_ADDRESS if no
**          physical pointer has been provided.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_MapUserSurface(
    IN gcoSURF Surface,
    IN gctUINT Alignment,
    IN gctPOINTER Logical,
    IN gctPHYS_ADDR_T Physical
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gctPOINTER logical = gcvNULL;
    gceHARDWARE_TYPE currentType = gcvHARDWARE_INVALID;
    gctUINT32 address;
    gcsUSER_MEMORY_DESC desc;

    gcmHEADER_ARG("Surface=0x%x Alignment=%u Logical=0x%x Physical=%010llx",
              Surface, Alignment, Logical, Physical);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        /* Has to be user-allocated surface. */
        if (Surface->node.pool != gcvPOOL_USER)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        gcmGETCURRENTHARDWARE(currentType);

        /* Already mapped? */
        if (Surface->node.lockCounts[currentType][gcvENGINE_RENDER] > 0)
        {
            if ((Logical != gcvNULL) &&
                (Logical != Surface->node.logical))
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            gcmGETHARDWAREADDRESS(Surface->node, address);

            if ((Physical != gcvINVALID_PHYSICAL_ADDRESS) &&
                (Physical != address))
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            /* Success. */
            break;
        }

        /* Set new alignment. */
        if (Alignment != 0)
        {
            /* Compute the HAL required stride */
            _ComputeSurfacePlacement(Surface, gcvTRUE);

            /* Align the stride (Alignment can be not a power of number). */
            Surface->stride = gcmALIGN_NP2(Surface->stride, Alignment);

            /* Compute surface placement with user stride */
            _ComputeSurfacePlacement(Surface, gcvFALSE);

            Surface->layerSize = Surface->sliceSize * Surface->requestD;

            /* We won't map multi-layer surface. */
            gcmASSERT(Surface->formatInfo.layers == 1);

            /* Compute the new size. */
            Surface->size = Surface->layerSize * Surface->formatInfo.layers;
        }

        /* Map logical pointer if not specified. */
        if (Logical == gcvNULL)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }
        else
        {
            /* Set the logical pointer. */
            logical = Logical;
        }

        desc.physical = Physical;
        desc.logical = gcmPTR_TO_UINT64(Logical);
        desc.size = Surface->size;
        desc.flag = gcvALLOC_FLAG_USERMEMORY;

        gcmERR_BREAK(gcoHAL_WrapUserMemory(
            &desc,
            (gceVIDMEM_TYPE)Surface->type,
            &Surface->node.u.normal.node
            ));

        Surface->node.u.wrapped.physical = Physical;
        Surface->node.logical = logical;

        /* Because _FreeSurface() is used to free user surface too, we need to
         * reference this node for current hardware here, like
         * gcoSRUF_Construct() does.
         */
        gcmERR_BREAK(_Lock(Surface));
    }
    while (gcvFALSE);

    /* Roll back. */
    if (gcmIS_ERROR(status))
    {
        if (Surface->node.u.normal.node != 0)
        {
            gcmVERIFY_OK(gcoHAL_ReleaseVideoMemory(Surface->node.u.normal.node));
            Surface->node.u.normal.node = 0;
        }
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_IsValid
**
**  Verify whether the surface is a valid gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to a gcoSURF object.
**
**  RETURNS:
**
**      The return value of the function is set to gcvSTATUS_TRUE if the
**      surface is valid.
*/
gceSTATUS
gcoSURF_IsValid(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set return value. */
    status = ((Surface != gcvNULL)
        && (Surface->object.type != gcvOBJ_SURF))
        ? gcvSTATUS_FALSE
        : gcvSTATUS_TRUE;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoSURF_IsTileStatusSupported
**
**  Verify whether the tile status is supported by the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to a gcoSURF object.
**
**  RETURNS:
**
**      The return value of the function is set to gcvSTATUS_TRUE if the
**      tile status is supported.
*/
gceSTATUS
gcoSURF_IsTileStatusSupported(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set return value. */
    status = (Surface->tileStatusNode.pool == gcvPOOL_UNKNOWN)
        ? gcvSTATUS_NOT_SUPPORTED
        : gcvSTATUS_TRUE;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_IsTileStatusEnabled
**
**  Verify whether the tile status is enabled on this surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to a gcoSURF object.
**
**  RETURNS:
**
**      The return value of the function is set to gcvSTATUS_TRUE if the
**      tile status is enabled.
*/
gceSTATUS
gcoSURF_IsTileStatusEnabled(
    IN gcsSURF_VIEW *SurfView
    )
{
    gceSTATUS status = gcvSTATUS_FALSE;

    gcoSURF Surface = SurfView->surf;
    gctBOOL canTsEnabled = gcvTRUE;

    gcmHEADER_ARG("SurfView=0x%x", SurfView);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    gcmCanTileStatusEnabledForMultiSlice(SurfView, &canTsEnabled);

    /* Check whether the surface has enabled tile status. */
    if ((Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN) && canTsEnabled)
    {
        status = gcvSTATUS_TRUE;
    }
    else
    {
        status = gcvSTATUS_FALSE;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_IsCompressed
**
**  Verify whether the surface is compressed.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to a gcoSURF object.
**
**  RETURNS:
**
**      The return value of the function is set to gcvSTATUS_TRUE if the
**      tile status is supported.
*/
gceSTATUS
gcoSURF_IsCompressed(
    IN gcsSURF_VIEW *SurfView
    )
{
    gceSTATUS status;
    gcoSURF Surface = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x", SurfView);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurfView->numSlices == 1);

    /* Check whether the surface is compressed. */
    if ((Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN) &&
        (Surface->tileStatusDisabled[SurfView->firstSlice] == gcvFALSE) &&
        Surface->compressed)
    {
        status = gcvSTATUS_TRUE;
    }
    else
    {
        status = gcvSTATUS_FALSE;
    }

    /* Return status. */
    gcmFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_EnableTileStatusEx
**
**  Enable tile status for the specified surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctUINT RtIndex
**          Which RT slot will be bound for this surface
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_EnableTileStatusEx(
    IN gcsSURF_VIEW *surfView,
    IN gctUINT RtIndex
    )
{
    gceSTATUS status;
    gctUINT32 tileStatusAddress = 0;
    gcoSURF Surface = surfView->surf;

    gcmHEADER_ARG("surfView=0x%x RtIndex=%d", surfView, RtIndex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
        {
            gcmGETHARDWAREADDRESS(Surface->tileStatusNode, tileStatusAddress);
        }

        tileStatusAddress += surfView->firstSlice * Surface->tileStatusSliceSize;

        /* Enable tile status. */
        gcmERR_BREAK(
            gcoHARDWARE_EnableTileStatus(gcvNULL,
                                         surfView,
                                         tileStatusAddress,
                                         &Surface->hzTileStatusNode,
                                         RtIndex));

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_EnableTileStatus
**
**  Enable tile status for the specified surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_EnableTileStatus(
    IN gcsSURF_VIEW *surfView
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("surfView=0x%x", surfView);

    status = gcoSURF_EnableTileStatusEx(surfView, 0);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoSURF_DisableTileStatus
**
**  Disable tile status for the specified surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctBOOL Decompress
**          Set if the render target needs to decompressed by issuing a resolve
**          onto itself.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_DisableTileStatus(
    IN gcsSURF_VIEW *SurfView,
    IN gctBOOL Decompress
    )
{
    gceSTATUS status;
    gcoSURF Surface = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x", SurfView);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
        {
            /* Disable tile status. */
            gcmERR_BREAK(gcoHARDWARE_DisableTileStatus(gcvNULL, SurfView, Decompress));
        }

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoSURF_FlushTileStatus
**
**  Flush tile status for the specified surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctBOOL Decompress
**          Set if the render target needs to decompressed by issuing a resolve
**          onto itself.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_FlushTileStatus(
    IN gcsSURF_VIEW *SurfView,
    IN gctBOOL Decompress
    )
{
    gceSTATUS status;
    gcoSURF Surface = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x", SurfView);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Disable tile status. */
        gcmONERROR(gcoHARDWARE_FlushTileStatus(gcvNULL, SurfView, Decompress));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif /* gcdENABLE_3D */

gceSTATUS
gcoSURF_UpdateMetadata(
    IN gcoSURF Surface,
    IN gctINT TsFD
    )
{
    gcsHAL_INTERFACE iface = {0};

    iface.command = gcvHAL_SET_VIDEO_MEMORY_METADATA;
    iface.u.SetVidMemMetadata.node              = Surface->node.u.normal.node;
    iface.u.SetVidMemMetadata.readback          = 0;

#if gcdENABLE_3D
    {
        gctUINT32 compressFmt = 0;

        gcoHARDWARE_MapCompressionFormat(Surface->compressFormat, &compressFmt);

        iface.u.SetVidMemMetadata.ts_fd             = TsFD;
        iface.u.SetVidMemMetadata.fc_enabled        = !Surface->tileStatusDisabled[0];
        iface.u.SetVidMemMetadata.fc_value          = Surface->fcValue[0];
        iface.u.SetVidMemMetadata.fc_value_upper    = Surface->fcValueUpper[0];
        iface.u.SetVidMemMetadata.compressed        = Surface->compressed;
        iface.u.SetVidMemMetadata.compress_format   = compressFmt;
    }
#endif

    return gcoHAL_Call(gcvNULL, &iface);
}

/*******************************************************************************
**
**  gcoSURF_GetSize
**
**  Get the size of an gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT * Width
**          Pointer to variable that will receive the width of the gcoSURF
**          object.  If 'Width' is gcvNULL, no width information shall be returned.
**
**      gctUINT * Height
**          Pointer to variable that will receive the height of the gcoSURF
**          object.  If 'Height' is gcvNULL, no height information shall be returned.
**
**      gctUINT * Depth
**          Pointer to variable that will receive the depth of the gcoSURF
**          object.  If 'Depth' is gcvNULL, no depth information shall be returned.
*/
gceSTATUS
gcoSURF_GetSize(
    IN gcoSURF Surface,
    OUT gctUINT * Width,
    OUT gctUINT * Height,
    OUT gctUINT * Depth
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Width != gcvNULL)
    {
        /* Return the width. */
        *Width = Surface->requestW;
    }

    if (Height != gcvNULL)
    {
        /* Return the height. */
        *Height = Surface->requestH;
    }

    if (Depth != gcvNULL)
    {
        /* Return the depth. */
        *Depth = Surface->requestD;
    }

    /* Success. */
    gcmFOOTER_ARG("*Width=%u *Height=%u *Depth=%u",
                  (Width  == gcvNULL) ? 0 : *Width,
                  (Height == gcvNULL) ? 0 : *Height,
                  (Depth  == gcvNULL) ? 0 : *Depth);
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoSURF_GetInfo
**
**  Get information of an gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gceSURF_INFO_TYPE InfoType.
**          Information type need to be queried.
**
**  OUTPUT:
**
**      gctINT32 *Value
**          Pointer to variable that will receive return value.
**
*/
gceSTATUS
gcoSURF_GetInfo(
    IN gcoSURF Surface,
    IN gceSURF_INFO_TYPE InfoType,
    IN OUT gctINT32 *Value
    )
{
    gcmHEADER_ARG("Surface=0x%x InfoType=%d Value=0x%x", Surface, InfoType, Value);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Value)
    {
        switch (InfoType)
        {
        case gcvSURF_INFO_LAYERSIZE:
            *Value = Surface->layerSize;
            break;

        case gcvSURF_INFO_SLICESIZE:
            *Value = Surface->sliceSize;
            break;

        default:
            gcmPRINT("Invalid surface info type query");
            break;
        }
    }

    gcmFOOTER_ARG("*value=%d", gcmOPT_VALUE(Value));
    return gcvSTATUS_OK;

}

/*******************************************************************************
**
**  gcoSURF_GetAlignedSize
**
**  Get the aligned size of an gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT * Width
**          Pointer to variable that receives the aligned width of the gcoSURF
**          object.  If 'Width' is gcvNULL, no width information shall be returned.
**
**      gctUINT * Height
**          Pointer to variable that receives the aligned height of the gcoSURF
**          object.  If 'Height' is gcvNULL, no height information shall be
**          returned.
**
**      gctINT * Stride
**          Pointer to variable that receives the stride of the gcoSURF object.
**          If 'Stride' is gcvNULL, no stride information shall be returned.
*/
gceSTATUS
gcoSURF_GetAlignedSize(
    IN gcoSURF Surface,
    OUT gctUINT * Width,
    OUT gctUINT * Height,
    OUT gctINT * Stride
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Width != gcvNULL)
    {
        /* Return the aligned width. */
        *Width = Surface->alignedW;
    }

    if (Height != gcvNULL)
    {
        /* Return the aligned height. */
        *Height = Surface->alignedH;
    }

    if (Stride != gcvNULL)
    {
        /* Return the stride. */
        *Stride = Surface->stride;
    }

    /* Success. */
    gcmFOOTER_ARG("*Width=%u *Height=%u *Stride=%d",
                  (Width  == gcvNULL) ? 0 : *Width,
                  (Height == gcvNULL) ? 0 : *Height,
                  (Stride == gcvNULL) ? 0 : *Stride);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetAlignment
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceSURF_TYPE Type
**          Type of surface.
**
**      gceSURF_FORMAT Format
**          Format of surface.
**
**  OUTPUT:
**
**      gctUINT * addressAlignment
**          Pointer to the variable of address alignment.
**      gctUINT * xAlignmenet
**          Pointer to the variable of x Alignment.
**      gctUINT * yAlignment
**          Pointer to the variable of y Alignment.
*/
gceSTATUS
gcoSURF_GetAlignment(
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    OUT gctUINT * AddressAlignment,
    OUT gctUINT * XAlignment,
    OUT gctUINT * YAlignment
    )
{
    gceSTATUS status;
    gctUINT32 bitsPerPixel;
    gctUINT xAlign = (gcvSURF_TEXTURE == Type) ? 4 : 16;
    gctUINT yAlign = 4;

    gcmHEADER_ARG("Type=%d Format=%d", Type, Format);

    /* Compute alignment factors. */
    if (XAlignment != gcvNULL)
    {
        *XAlignment = xAlign;
    }

    if (YAlignment != gcvNULL)
    {
        *YAlignment = yAlign;
    }

    {
        /* Compute bits per pixel. */
        gcmONERROR(gcoHARDWARE_ConvertFormat(Format,
                                             &bitsPerPixel,
                                             gcvNULL));
    }

    if (AddressAlignment != gcvNULL)
    {
        *AddressAlignment = xAlign * yAlign * bitsPerPixel / 8;
    }

    gcmFOOTER_ARG("*XAlignment=0x%x  *YAlignment=0x%x *AddressAlignment=0x%x",
        gcmOPT_VALUE(XAlignment), gcmOPT_VALUE(YAlignment), gcmOPT_VALUE(AddressAlignment));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoSURF_AlignResolveRect (need to modify)
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceSURF_TYPE Type
**          Type of surface.
**
**      gceSURF_FORMAT Format
**          Format of surface.
**
**  OUTPUT:
**
**      gctUINT * addressAlignment
**          Pointer to the variable of address alignment.
**      gctUINT * xAlignmenet
**          Pointer to the variable of x Alignment.
**      gctUINT * yAlignment
**          Pointer to the variable of y Alignment.
*/
gceSTATUS
gcoSURF_AlignResolveRect(
    IN gcoSURF Surf,
    IN gcsPOINT_PTR RectOrigin,
    IN gcsPOINT_PTR RectSize,
    OUT gcsPOINT_PTR AlignedOrigin,
    OUT gcsPOINT_PTR AlignedSize
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surf=0x%x RectOrigin=0x%x RectSize=0x%x",
        Surf, RectOrigin, RectSize);

    status = gcoHARDWARE_AlignResolveRect(Surf, RectOrigin, RectSize,
                                         AlignedOrigin, AlignedSize);

    /* Success. */
    gcmFOOTER();

    return status;
}
#endif

/*******************************************************************************
**
**  gcoSURF_GetFormat
**
**  Get surface type and format.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gceSURF_TYPE * Type
**          Pointer to variable that receives the type of the gcoSURF object.
**          If 'Type' is gcvNULL, no type information shall be returned.
**
**      gceSURF_FORMAT * Format
**          Pointer to variable that receives the format of the gcoSURF object.
**          If 'Format' is gcvNULL, no format information shall be returned.
*/
gceSTATUS
gcoSURF_GetFormat(
    IN gcoSURF Surface,
    OUT gceSURF_TYPE * Type,
    OUT gceSURF_FORMAT * Format
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Type != gcvNULL)
    {
        /* Return the surface type. */
        *Type = Surface->type;
    }

    if (Format != gcvNULL)
    {
        /* Return the surface format. */
        *Format = Surface->format;
    }

    /* Success. */
    gcmFOOTER_ARG("*Type=%d *Format=%d",
                  (Type   == gcvNULL) ? 0 : *Type,
                  (Format == gcvNULL) ? 0 : *Format);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetFormatInfo
**
**  Get surface format information.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gcsSURF_FORMAT_INFO_PTR * formatInfo
**          Pointer to variable that receives the format informationof the gcoSURF object.
**          If 'formatInfo' is gcvNULL, no format information shall be returned.
*/
gceSTATUS
gcoSURF_GetFormatInfo(
    IN gcoSURF Surface,
    OUT gcsSURF_FORMAT_INFO_PTR * formatInfo
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (formatInfo != gcvNULL)
    {
        /* Return the surface format. */
        *formatInfo = &Surface->formatInfo;
    }

    /* Success. */
    gcmFOOTER_ARG("*Format=0x%x",
                  (formatInfo == gcvNULL) ? 0 : *formatInfo);
    return gcvSTATUS_OK;
}



/*******************************************************************************
**
**  gcoSURF_GetPackedFormat
**
**  Get surface packed format for multiple-layer surface
**  gcvSURF_A32B32G32R32UI_2 ->gcvSURF_A32B32G32R32UI
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**
**      gceSURF_FORMAT * Format
**          Pointer to variable that receives the format of the gcoSURF object.
**          If 'Format' is gcvNULL, no format information shall be returned.
*/
gceSTATUS
gcoSURF_GetPackedFormat(
    IN gcoSURF Surface,
    OUT gceSURF_FORMAT * Format
    )
{
    gceSURF_FORMAT format;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    switch (Surface->format)
    {
    case gcvSURF_X16R16G16B16_2_A8R8G8B8:
        format = gcvSURF_X16R16G16B16;
        break;

    case gcvSURF_A16R16G16B16_2_A8R8G8B8:
        format = gcvSURF_A16R16G16B16;
        break;

    case gcvSURF_A32R32G32B32_2_G32R32F:
    case gcvSURF_A32R32G32B32_4_A8R8G8B8:
        format = gcvSURF_A32R32G32B32;
        break;

    case gcvSURF_S8D32F_1_G32R32F:
    case gcvSURF_S8D32F_2_A8R8G8B8:
        format = gcvSURF_S8D32F;
        break;

    case gcvSURF_X16B16G16R16F_2_A8R8G8B8:
        format = gcvSURF_X16B16G16R16F;
        break;

    case gcvSURF_A16B16G16R16F_2_A8R8G8B8:
        format = gcvSURF_A16B16G16R16F;
        break;

    case gcvSURF_A16B16G16R16F_2_G16R16F:
        format = gcvSURF_A16B16G16R16F;
        break;

    case gcvSURF_G32R32F_2_A8R8G8B8:
        format = gcvSURF_G32R32F;
        break;

    case gcvSURF_X32B32G32R32F_2_G32R32F:
    case gcvSURF_X32B32G32R32F_4_A8R8G8B8:
        format = gcvSURF_X32B32G32R32F;
        break;

    case gcvSURF_A32B32G32R32F_2_G32R32F:
    case gcvSURF_A32B32G32R32F_4_A8R8G8B8:
        format = gcvSURF_A32B32G32R32F;
        break;

    case gcvSURF_R16F_1_A4R4G4B4:
        format = gcvSURF_R16F;
        break;

    case gcvSURF_G16R16F_1_A8R8G8B8:
        format = gcvSURF_G16R16F;
        break;

    case gcvSURF_B16G16R16F_2_A8R8G8B8:
        format = gcvSURF_B16G16R16F;
        break;

    case gcvSURF_R32F_1_A8R8G8B8:
        format = gcvSURF_R32F;
        break;

    case gcvSURF_B32G32R32F_3_A8R8G8B8:
        format = gcvSURF_B32G32R32F;
        break;

    case gcvSURF_B10G11R11F_1_A8R8G8B8:
        format = gcvSURF_B10G11R11F;
        break;

    case gcvSURF_G32R32I_2_A8R8G8B8:
        format = gcvSURF_G32R32I;
        break;

    case gcvSURF_G32R32UI_2_A8R8G8B8:
        format = gcvSURF_G32R32UI;
        break;

    case gcvSURF_X16B16G16R16I_2_A8R8G8B8:
        format = gcvSURF_X16B16G16R16I;
        break;

    case gcvSURF_A16B16G16R16I_2_A8R8G8B8:
        format = gcvSURF_A16B16G16R16I;
        break;

    case gcvSURF_X16B16G16R16UI_2_A8R8G8B8:
        format = gcvSURF_X16B16G16R16UI;
        break;

    case gcvSURF_A16B16G16R16UI_2_A8R8G8B8:
        format = gcvSURF_A16B16G16R16UI;
        break;

    case gcvSURF_X32B32G32R32I_2_G32R32I:
    case gcvSURF_X32B32G32R32I_3_A8R8G8B8:
        format = gcvSURF_X32B32G32R32I;
        break;

    case gcvSURF_A32B32G32R32I_2_G32R32I:
    case gcvSURF_A32B32G32R32I_4_A8R8G8B8:
        format = gcvSURF_A32B32G32R32I;
        break;

    case gcvSURF_X32B32G32R32UI_2_G32R32UI:
    case gcvSURF_X32B32G32R32UI_3_A8R8G8B8:
        format = gcvSURF_X32B32G32R32UI;
        break;

    case gcvSURF_A32B32G32R32UI_2_G32R32UI:
    case gcvSURF_A32B32G32R32UI_4_A8R8G8B8:
        format = gcvSURF_A32B32G32R32UI;
        break;

    case gcvSURF_A2B10G10R10UI_1_A8R8G8B8:
        format = gcvSURF_A2B10G10R10UI;
        break;

    case gcvSURF_A8B8G8R8I_1_A8R8G8B8:
        format = gcvSURF_A8B8G8R8I;
        break;

    case gcvSURF_A8B8G8R8UI_1_A8R8G8B8:
        format = gcvSURF_A8B8G8R8UI;
        break;

    case gcvSURF_R8I_1_A4R4G4B4:
        format = gcvSURF_R8I;
        break;

    case gcvSURF_R8UI_1_A4R4G4B4:
        format = gcvSURF_R8UI;
        break;

    case gcvSURF_R16I_1_A4R4G4B4:
        format = gcvSURF_R16I;
        break;

    case gcvSURF_R16UI_1_A4R4G4B4:
        format = gcvSURF_R16UI;
        break;

    case gcvSURF_R32I_1_A8R8G8B8:
        format = gcvSURF_R32I;
        break;

    case gcvSURF_R32UI_1_A8R8G8B8:
        format = gcvSURF_R32UI;
        break;

    case gcvSURF_X8R8I_1_A4R4G4B4:
        format = gcvSURF_X8R8I;
        break;

    case gcvSURF_X8R8UI_1_A4R4G4B4:
        format = gcvSURF_X8R8UI;
        break;

    case gcvSURF_G8R8I_1_A4R4G4B4:
        format = gcvSURF_G8R8I;
        break;

    case gcvSURF_G8R8UI_1_A4R4G4B4:
        format = gcvSURF_G8R8UI;
        break;

    case gcvSURF_X16R16I_1_A4R4G4B4:
        format = gcvSURF_X16R16I;
        break;

    case gcvSURF_X16R16UI_1_A4R4G4B4:
        format = gcvSURF_X16R16UI;
        break;

    case gcvSURF_G16R16I_1_A8R8G8B8:
        format = gcvSURF_G16R16I;
        break;

    case gcvSURF_G16R16UI_1_A8R8G8B8:
        format = gcvSURF_G16R16UI;
        break;
    case gcvSURF_X32R32I_1_A8R8G8B8:
        format = gcvSURF_X32R32I;
        break;

    case gcvSURF_X32R32UI_1_A8R8G8B8:
        format = gcvSURF_X32R32UI;
        break;

    case gcvSURF_X8G8R8I_1_A4R4G4B4:
        format = gcvSURF_X8G8R8I;
        break;

    case gcvSURF_X8G8R8UI_1_A4R4G4B4:
        format = gcvSURF_X8G8R8UI;
        break;

    case gcvSURF_B8G8R8I_1_A8R8G8B8:
        format = gcvSURF_B8G8R8I;
        break;

    case gcvSURF_B8G8R8UI_1_A8R8G8B8:
        format = gcvSURF_B8G8R8UI;
        break;

    case gcvSURF_B16G16R16I_2_A8R8G8B8:
        format = gcvSURF_B16G16R16I;
        break;

    case gcvSURF_B16G16R16UI_2_A8R8G8B8:
        format = gcvSURF_B16G16R16UI;
        break;

    case gcvSURF_B32G32R32I_3_A8R8G8B8:
        format = gcvSURF_B32G32R32I;
        break;

    case gcvSURF_B32G32R32UI_3_A8R8G8B8:
        format = gcvSURF_B32G32R32UI;
        break;

    default:
        format = Surface->format;
        break;
    }

    if (Format != gcvNULL)
    {
        /* Return the surface format. */
        *Format = format;
    }

    /* Success. */
    gcmFOOTER_ARG("*Format=%d",
                  (Format == gcvNULL) ? 0 : *Format);
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoSURF_GetTiling
**
**  Get surface tiling.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gceTILING * Tiling
**          Pointer to variable that receives the tiling of the gcoSURF object.
*/
gceSTATUS
gcoSURF_GetTiling(
    IN gcoSURF Surface,
    OUT gceTILING * Tiling
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Tiling != gcvNULL)
    {
        /* Return the surface tiling. */
        *Tiling = Surface->tiling;
    }

    /* Success. */
    gcmFOOTER_ARG("*Tiling=%d",
                  (Tiling == gcvNULL) ? 0 : *Tiling);
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoSURF_GetBottomBufferOffset
**
**  Get bottom buffer offset for split tiled surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT_PTR BottomBufferOffset
**          Pointer to variable that receives the offset value.
*/
gceSTATUS
gcoSURF_GetBottomBufferOffset(
    IN gcoSURF Surface,
    OUT gctUINT_PTR BottomBufferOffset
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (BottomBufferOffset != gcvNULL)
    {
        /* Return the surface tiling. */
        *BottomBufferOffset = Surface->bottomBufferOffset;
    }

    /* Success. */
    gcmFOOTER_ARG("*BottomBufferOffset=%d",
                  (BottomBufferOffset == gcvNULL) ? 0 : *BottomBufferOffset);
    return gcvSTATUS_OK;
}


#if gcdENABLE_3D
gceSTATUS
gcoSURF_GetFence(
    IN gcoSURF Surface,
    IN gceFENCE_TYPE Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Surface=0x%x Type=%d", Surface, Type);

    if (Surface)
    {
        status = gcsSURF_NODE_GetFence(&Surface->node,  gcvENGINE_RENDER, Type);
    }

    gcmFOOTER();
    return status;

}

gceSTATUS
gcoSURF_WaitFence(
    IN gcoSURF Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    if (Surface)
    {
        status = gcsSURF_NODE_WaitFence(&Surface->node,  gcvENGINE_CPU, gcvENGINE_RENDER, gcvFENCE_TYPE_ALL);
    }

    gcmFOOTER();
    return status;

}
#endif

/*******************************************************************************
**
**  gcoSURF_Lock
**
**  Lock the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Physical address array of the surface:
**          For YV12, Address[0] is for Y channel,
**                    Address[1] is for U channel and
**                    Address[2] is for V channel;
**          For I420, Address[0] is for Y channel,
**                    Address[1] is for U channel and
**                    Address[2] is for V channel;
**          For NV12, Address[0] is for Y channel and
**                    Address[1] is for UV channel;
**          For all other formats, only Address[0] is used to return the
**          physical address.
**
**      gctPOINTER * Memory
**          Logical address array of the surface:
**          For YV12, Memory[0] is for Y channel,
**                    Memory[1] is for V channel and
**                    Memory[2] is for U channel;
**          For I420, Memory[0] is for Y channel,
**                    Memory[1] is for U channel and
**                    Memory[2] is for V channel;
**          For NV12, Memory[0] is for Y channel and
**                    Memory[1] is for UV channel;
**          For all other formats, only Memory[0] is used to return the logical
**          address.
*/
gceSTATUS
gcoSURF_Lock(
    IN gcoSURF Surface,
    OPTIONAL OUT gctUINT32 * Address,
    OPTIONAL OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;
    gctUINT32 address = 0, address2 = 0, address3 = 0;
    gctUINT8_PTR logical = gcvNULL, logical2 = gcvNULL, logical3 = gcvNULL;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Lock the surface. */
    gcmONERROR(_Lock(Surface));

    gcmGETHARDWAREADDRESS(Surface->node, address);

    if (Surface->flags & gcvSURF_FLAG_MULTI_NODE)
    {
        /* Multiple nodes. */
        gcmGETHARDWAREADDRESS(Surface->node2, address2);
        gcmGETHARDWAREADDRESS(Surface->node3, address3);

        logical = Surface->node.logical;

        /* Calculate buffer address. */
        switch (Surface->format)
        {
        case gcvSURF_YV12:
        case gcvSURF_I420:
            Surface->node.count = 3;

            logical2 = Surface->node2.logical;

            logical3 = Surface->node3.logical;
            break;

        case gcvSURF_NV12:
        case gcvSURF_NV21:
        case gcvSURF_NV16:
        case gcvSURF_NV61:
        case gcvSURF_YUV420_10_ST:
        case gcvSURF_YUV420_TILE_ST:
        case gcvSURF_YUV420_TILE_10_ST:
        case gcvSURF_NV12_10BIT:
        case gcvSURF_NV21_10BIT:
        case gcvSURF_NV16_10BIT:
        case gcvSURF_NV61_10BIT:
            Surface->node.count = 2;

            logical2 = Surface->node2.logical;
            break;

        default:
            Surface->node.count = 1;
            break;
        }
    }
    else
    {
        /* Determine surface addresses. */
        logical = Surface->node.logical;

        switch (Surface->format)
        {
#if gcdVG_ONLY
        case gcvSURF_ANV16:
        case gcvSURF_ANV12:
            Surface->node.count = 3;

            logical2 = Surface->node.logicalBottom
                     + Surface->uOffset;

            address2 = Surface->node.physicalBottom
                     = address + Surface->uOffset;

            logical3 = Surface->node.logicalBottom
                     + Surface->aOffset;

            address3 = Surface->node.physicalBottom
                     = address + Surface->aOffset;
            break;

        case gcvSURF_AYUY2:
            Surface->node.count = 2;

            logical2 = Surface->node.logicalBottom
                     + Surface->aOffset;

            address2 = Surface->node.physicalBottom
                     = address + Surface->aOffset;
            break;
#endif
        case gcvSURF_I420:
            Surface->node.count = 3;

            logical2 = Surface->node.logical
                     + Surface->uOffset;

            address2 = Surface->node.physical2
                     = address + Surface->uOffset;

            logical3 = Surface->node.logical
                     + Surface->vOffset;

            address3 = Surface->node.physical3
                     = address + Surface->vOffset;
            break;

        case gcvSURF_YV12:
#if gcdVG_ONLY
        case gcvSURF_YV16:
#endif
            Surface->node.count = 3;

            logical2 = Surface->node.logical
                     + Surface->vOffset;

            address2 = Surface->node.physical2
                     = address + Surface->vOffset;

            logical3 = Surface->node.logical
                     + Surface->uOffset;

            address3 = Surface->node.physical3
                     = address + Surface->uOffset;
            break;

        case gcvSURF_NV12:
        case gcvSURF_NV21:
        case gcvSURF_NV16:
        case gcvSURF_NV61:
        case gcvSURF_YUV420_10_ST:
        case gcvSURF_YUV420_TILE_ST:
        case gcvSURF_YUV420_TILE_10_ST:
        case gcvSURF_NV12_10BIT:
        case gcvSURF_NV21_10BIT:
        case gcvSURF_NV16_10BIT:
        case gcvSURF_NV61_10BIT:
            Surface->node.count = 2;

            logical2 = Surface->node.logical
                     + Surface->uOffset;

            address2 = Surface->node.physical2
                     = address + Surface->uOffset;
            break;

        default:
            Surface->node.count = 1;
            break;
        }
    }

    /* Set result. */
    if (Address != gcvNULL)
    {
        switch (Surface->node.count)
        {
        case 3:
            Address[2] = address3;

            /* FALLTHROUGH */
        case 2:
            Address[1] = address2;

            /* FALLTHROUGH */
        case 1:
            Address[0] = address;

            /* FALLTHROUGH */
        default:
            break;
        }
    }

    if (Memory != gcvNULL)
    {
        switch (Surface->node.count)
        {
        case 3:
            Memory[2] = logical3;

            /* FALLTHROUGH */
        case 2:
            Memory[1] = logical2;

            /* FALLTHROUGH */
        case 1:
            Memory[0] = logical;

            /* FALLTHROUGH */
        default:
            break;
        }
    }

    /* Success. */
    gcmFOOTER_ARG("*Address=%08X *Memory=0x%x",
                  (Address == gcvNULL) ? 0 : *Address,
                  (Memory  == gcvNULL) ? gcvNULL : *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Unlock
**
**  Unlock the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctPOINTER Memory
**          Pointer to mapped memory.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Unlock(
    IN gcoSURF Surface,
    IN gctPOINTER Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x Memory=0x%x", Surface, Memory);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Unlock the surface. */
    gcmONERROR(_Unlock(Surface));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Fill
**
**  Fill surface with specified value.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gcsPOINT_PTR Origin
**          Pointer to the origin of the area to be filled.
**          Assumed to (0, 0) if gcvNULL is given.
**
**      gcsSIZE_PTR Size
**          Pointer to the size of the area to be filled.
**          Assumed to the size of the surface if gcvNULL is given.
**
**      gctUINT32 Value
**          Value to be set.
**
**      gctUINT32 Mask
**          Value mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Fill(
    IN gcoSURF Surface,
    IN gcsPOINT_PTR Origin,
    IN gcsSIZE_PTR Size,
    IN gctUINT32 Value,
    IN gctUINT32 Mask
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoSURF_Blend
**
**  Alpha blend two surfaces together.
**
**  INPUT:
**
**      gcoSURF SrcSurf
**          Pointer to the source gcoSURF object.
**
**      gcoSURF DstSurf
**          Pointer to the destination gcoSURF object.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin within the source.
**          If gcvNULL is specified, (0, 0) origin is assumed.
**
**      gcsPOINT_PTR DstOrigin
**          The origin within the destination.
**          If gcvNULL is specified, (0, 0) origin is assumed.
**
**      gcsSIZE_PTR Size
**          The size of the area to be blended.
**
**      gceSURF_BLEND_MODE Mode
**          One of the blending modes.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Blend(
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DstOrigin,
    IN gcsSIZE_PTR Size,
    IN gceSURF_BLEND_MODE Mode
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

#if gcdENABLE_3D
/******************************************************************************\
********************************* Support Code *********************************
\******************************************************************************/

/*******************************************************************************
**
**  _ConvertValue
**
**  Convert a value to a 32-bit color or depth value.
**
**  INPUT:
**
**      gceVALUE_TYPE ValueType
**          Type of value.
**
**      gcuVALUE Value
**          Value data.
**
**      gctUINT Bits
**          Number of bits used in output.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gctUINT32
**          Converted or casted value.
*/
extern gctFLOAT _LinearToNonLinearConv(gctFLOAT lFloat);

static gctUINT32
_ConvertValue(
    IN gceVALUE_TYPE ValueType,
    IN gcuVALUE Value,
    IN gctUINT Bits
    )
{
    /* Setup maximum value. */
    gctUINT uMaxValue = (Bits == 32) ? ~0 : ((1 << Bits) - 1);
    gctUINT32 tmpRet = 0;
    gcmASSERT(Bits <= 32);

    /*
    ** Data conversion clear path:
    ** Here we need handle clamp here coz client driver just pass clear value down.
    ** Now we plan to support INT/UINT RT, floating depth or floating RT later, we need set gcvVALUE_FLAG_NEED_CLAMP
    ** base on surface format.
    */
    switch (ValueType & ~gcvVALUE_FLAG_MASK)
    {
    case gcvVALUE_UINT:
        return ((Value.uintValue > uMaxValue) ? uMaxValue : Value.uintValue);

    case gcvVALUE_INT:
        {
            gctUINT32 mask = (Bits == 32) ? ~0 : ((1 << Bits) - 1);
            gctINT iMinValue = (Bits == 32)? (1 << (Bits-1))   :((~( 1 << (Bits -1))) + 1);
            gctINT iMaxValue = (Bits == 32)? (~(1 << (Bits-1))): ((1 << (Bits - 1)) - 1);
            return gcmCLAMP(Value.intValue, iMinValue, iMaxValue) & mask;
        }

    case gcvVALUE_FIXED:
        {
            gctFIXED_POINT tmpFixedValue = Value.fixedValue;
            if (ValueType & gcvVALUE_FLAG_UNSIGNED_DENORM)
            {
                tmpFixedValue = gcmFIXEDCLAMP_0_TO_1(tmpFixedValue);
                /* Convert fixed point (0.0 - 1.0) into color value. */
                return (gctUINT32) (((gctUINT64)uMaxValue * tmpFixedValue) >> 16);
            }
            else if (ValueType & gcvVALUE_FLAG_SIGNED_DENORM)
            {
                gcmASSERT(0);
                return 0;
            }
            else
            {
                gcmASSERT(0);
                return 0;
            }
        }
        break;

    case gcvVALUE_FLOAT:
        {
            gctFLOAT tmpFloat = Value.floatValue;
            gctFLOAT sFloat = tmpFloat;

            if (ValueType & gcvVALUE_FLAG_GAMMAR)
            {
                gcmASSERT ((ValueType & gcvVALUE_FLAG_FLOAT_TO_FLOAT16) == 0);

                sFloat = _LinearToNonLinearConv(tmpFloat);
            }

            if (ValueType & gcvVALUE_FLAG_FLOAT_TO_FLOAT16)
            {
                gcmASSERT ((ValueType & gcvVALUE_FLAG_GAMMAR) == 0);
                tmpRet = (gctUINT32)gcoMATH_FloatToFloat16(*(gctUINT32*)&tmpFloat);
                return tmpRet;
            }
            else if (ValueType & gcvVALUE_FLAG_UNSIGNED_DENORM)
            {
                sFloat = gcmFLOATCLAMP_0_TO_1(sFloat);
                /* Convert floating point (0.0 - 1.0) into color value. */
                sFloat = gcoMATH_Multiply(gcoMATH_UInt2Float(uMaxValue), sFloat);
                tmpRet = gcoMath_Float2UINT_STICKROUNDING(sFloat);
                return tmpRet > uMaxValue ? uMaxValue : tmpRet;
            }
            else if (ValueType & gcvVALUE_FLAG_SIGNED_DENORM)
            {
                gcmASSERT(0);
                return 0;
            }
            else
            {
                tmpRet = *(gctUINT32*)&sFloat;
                return tmpRet > uMaxValue ? uMaxValue : tmpRet;
            }
        }
        break;

    default:
        return 0;
        break;
    }
}
gctUINT32
_ByteMaskToBitMask(
    gctUINT32 ClearMask
    )
{
    gctUINT32 clearMask = 0;

    /* Byte mask to bit mask. */
    if (ClearMask & 0x1)
    {
        clearMask |= 0xFF;
    }

    if (ClearMask & 0x2)
    {
        clearMask |= (0xFF << 8);
    }

    if (ClearMask & 0x4)
    {
        clearMask |= (0xFF << 16);
    }

    if (ClearMask & 0x8)
    {
        clearMask |= (0xFF << 24);
    }

    return clearMask;
}

#define CHANNEL_BITMASK(x) ((mask##x & ((1 << width##x) - 1)) << start##x)

static gceSTATUS
_ComputeClear(
    IN gcoSURF Surface,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctUINT32 LayerIndex
    )
{
    gceSTATUS status;
    gctUINT32 maskRed, maskGreen, maskBlue, maskAlpha;
    gctUINT32 clearBitMask32, tempMask;
    gceVALUE_TYPE clearValueType;
    gcsSURF_FORMAT_INFO_PTR info = &Surface->formatInfo;
    gctUINT32 widthRed    = info->u.rgba.red.width;
    gctUINT32 widthGreen  = info->u.rgba.green.width;
    gctUINT32 widthBlue   = info->u.rgba.blue.width;
    gctUINT32 widthAlpha  = info->u.rgba.alpha.width;
    gctUINT32 startRed    = info->u.rgba.red.start & 0x1f;
    gctUINT32 startGreen  = info->u.rgba.green.start & 0x1f;
    gctUINT32 startBlue   = info->u.rgba.blue.start & 0x1f;
    gctUINT32 startAlpha  = info->u.rgba.alpha.start & 0x1f;

    gcmHEADER_ARG("Surface=0x%x ClearArgs=0x%x LayerIndex=0x%d", Surface, ClearArgs, LayerIndex);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(ClearArgs != gcvNULL);

    /* Test for clearing render target. */
    if (ClearArgs->flags & gcvCLEAR_COLOR)
    {
        Surface->clearMask[LayerIndex] = ((ClearArgs->colorMask & 0x1) << 2) | /* Red   */
                                          (ClearArgs->colorMask & 0x2)       | /* Green */
                                         ((ClearArgs->colorMask & 0x4) >> 2) | /* Blue  */
                                          (ClearArgs->colorMask & 0x8);        /* Alpha */
        maskRed   = (ClearArgs->colorMask & 0x1) ? 0xFFFFFFFF: 0;
        maskGreen = (ClearArgs->colorMask & 0x2) ? 0xFFFFFFFF: 0;
        maskBlue  = (ClearArgs->colorMask & 0x4) ? 0xFFFFFFFF: 0;
        maskAlpha = (ClearArgs->colorMask & 0x8) ? 0xFFFFFFFF: 0;

        clearBitMask32 =
            CHANNEL_BITMASK(Red) | CHANNEL_BITMASK(Green) | CHANNEL_BITMASK(Blue) | CHANNEL_BITMASK(Alpha);

        tempMask = (info->bitsPerPixel < 32 ) ?
                        ((gctUINT32)0xFFFFFFFF) >> (32 - info->bitsPerPixel) : 0xFFFFFFFF;

        clearBitMask32 &= tempMask;

        /* Dispatch on render target format. */
        switch (Surface->format)
        {
        case gcvSURF_X4R4G4B4: /* 12-bit RGB color without alpha channel. */
        case gcvSURF_A4R4G4B4: /* 12-bit RGB color with alpha channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red into 4-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 4) << 8)
                /* Convert green into 4-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 4) << 4)
                /* Convert blue into 4-bit. */
                | _ConvertValue(clearValueType,
                                ClearArgs->color.b, 4)
                /* Convert alpha into 4-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.a, 4) << 12);

            /* Expand 16-bit color into 32-bit color. */
            Surface->clearValue[0] |= Surface->clearValue[0] << 16;
            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
            Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
            break;

        case gcvSURF_X4B4G4R4: /* 12-bit RGB color without alpha channel. */
        case gcvSURF_A4B4G4R4: /* 12-bit RGB color with alpha channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red into 4-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.b, 4) << 8)
                /* Convert green into 4-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 4) << 4)
                /* Convert blue into 4-bit. */
                | _ConvertValue(clearValueType,
                                ClearArgs->color.r, 4)
                /* Convert alpha into 4-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.a, 4) << 12);

            /* Expand 16-bit color into 32-bit color. */
            Surface->clearValue[0] |= Surface->clearValue[0] << 16;
            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
            Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
            break;

        case gcvSURF_X1R5G5B5: /* 15-bit RGB color without alpha channel. */
        case gcvSURF_A1R5G5B5: /* 15-bit RGB color with alpha channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red into 5-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 5) << 10)
                /* Convert green into 5-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 5) << 5)
                /* Convert blue into 5-bit. */
                | _ConvertValue(clearValueType,
                                ClearArgs->color.b, 5)
                /* Convert alpha into 1-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.a, 1) << 15);

            /* Expand 16-bit color into 32-bit color. */
            Surface->clearValue[0] |= Surface->clearValue[0] << 16;
            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
            Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
            break;

        case gcvSURF_R5G6B5: /* 16-bit RGB color without alpha channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red into 5-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 5) << 11)
                /* Convert green into 6-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 6) << 5)
                /* Convert blue into 5-bit. */
                | _ConvertValue(clearValueType,
                                ClearArgs->color.b, 5);

            /* Expand 16-bit color into 32-bit color. */
            Surface->clearValue[0] |= Surface->clearValue[0] << 16;
            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
            Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
            break;

        case gcvSURF_B5G6R5: /* 16-bit RGB color without alpha channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red into 5-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.b, 5) << 11)
                /* Convert green into 6-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 6) << 5)
                /* Convert blue into 5-bit. */
                | _ConvertValue(clearValueType,
                                ClearArgs->color.r, 5);

            /* Expand 16-bit color into 32-bit color. */
            Surface->clearValue[0] |= Surface->clearValue[0] << 16;
            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
            Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
            break;

        case gcvSURF_YUY2:
            {
                gctUINT8 r, g, b;
                gctUINT8 y, u, v;
                gcmASSERT(LayerIndex == 0);
                /* Query YUY2 render target support. */
                if (gcoHAL_IsFeatureAvailable(gcvNULL,
                                              gcvFEATURE_YUY2_RENDER_TARGET)
                    != gcvSTATUS_TRUE)
                {
                    /* No, reject. */
                    gcmFOOTER_NO();
                    return gcvSTATUS_INVALID_ARGUMENT;
                }
                clearValueType = (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

                /* Get 8-bit RGB values. */
                r = (gctUINT8) _ConvertValue(clearValueType,
                                  ClearArgs->color.r, 8);

                g = (gctUINT8) _ConvertValue(clearValueType,
                                  ClearArgs->color.g, 8);

                b = (gctUINT8) _ConvertValue(clearValueType,
                                  ClearArgs->color.b, 8);

                /* Convert to YUV. */
                gcoHARDWARE_RGB2YUV(r, g, b, &y, &u, &v);

                /* Set the clear value. */
                Surface->clearValueUpper[0] =
                Surface->clearValue[0] =  ((gctUINT32) y)
                                     | (((gctUINT32) u) <<  8)
                                     | (((gctUINT32) y) << 16)
                                     | (((gctUINT32) v) << 24);
                Surface->clearBitMask[LayerIndex] = clearBitMask32;
                Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
            }
            break;

        case gcvSURF_X8R8G8B8: /* 24-bit RGB without alpha channel. */
        case gcvSURF_A8R8G8B8: /* 24-bit RGB with alpha channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red to 8-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 8) << 16)
                /* Convert green to 8-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 8) << 8)
                /* Convert blue to 8-bit. */
                |  _ConvertValue(clearValueType,
                                ClearArgs->color.b, 8)
                    /* Convert alpha to 8-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.a, 8) << 24);

            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
            break;

        case gcvSURF_X8B8G8R8: /* 24-bit RGB without alpha channel. */
        case gcvSURF_A8B8G8R8: /* 24-bit RGB with alpha channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red to 8-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.b, 8) << 16)
                /* Convert green to 8-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 8) << 8)
                /* Convert blue to 8-bit. */
                |  _ConvertValue(clearValueType,
                                ClearArgs->color.r, 8)
                    /* Convert alpha to 8-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.a, 8) << 24);

            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
            break;

        case gcvSURF_R8_1_X8R8G8B8: /* 32-bit R8 without bga channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red to 8-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 8) << 16)
                /* Convert green to 8-bit. */
                | 0xFF000000;

            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
            break;

        case gcvSURF_R8I_1_A4R4G4B4:  /* 32-bit R8I  */
        case gcvSURF_R8UI_1_A4R4G4B4: /* 32-bit R8UI */
            gcmASSERT(LayerIndex == 0);
            clearValueType = ClearArgs->color.valueType;;

            Surface->clearValue[0]
                = _ConvertValue(clearValueType, ClearArgs->color.r, 8);

            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
            break;

        case gcvSURF_G8R8_1_X8R8G8B8: /* 32-bit R8 without bga channel. */
            gcmASSERT(LayerIndex == 0);
            clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);

            Surface->clearValue[0]
                /* Convert red to 8-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 8) << 16)
                /* Convert green to 8-bit. */
                | (_ConvertValue(clearValueType,
                                 ClearArgs->color.g, 8) << 8)
                /* Convert green to 8-bit. */
                | 0xFF000000;

            Surface->clearValueUpper[0] = Surface->clearValue[0];
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
            break;

         case gcvSURF_G8R8:
             /* 16-bit RG color without alpha channel. */
             gcmASSERT(LayerIndex == 0);
             clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);
             Surface->clearValue[0]
                 /* Convert red to 8-bit. */
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.r, 8))
                 /* Convert green to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 8) << 8);

             /* Expand 16-bit color into 32-bit color. */
             Surface->clearValue[0] |= Surface->clearValue[0] << 16;
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_A8_SBGR8: /* 24-bit RGB with alpha channel. */
             gcmASSERT(LayerIndex == 0);
             clearValueType = (gceVALUE_TYPE)   (ClearArgs->color.valueType   |
                                                gcvVALUE_FLAG_UNSIGNED_DENORM |
                                                gcvVALUE_FLAG_GAMMAR);
             Surface->clearValue[0]
                /* Convert red to 8-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 8))
                 /* Convert green to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 8) << 8)
                 /* Convert blue to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.b, 8) << 16);

             clearValueType &= ~gcvVALUE_FLAG_GAMMAR;
             Surface->clearValue[0] |= (_ConvertValue(clearValueType,
                                                    ClearArgs->color.a, 8) << 24);
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;

        case gcvSURF_X8_SRGB8: /* 24-bit RGB without alpha channel. */
        case gcvSURF_A8_SRGB8: /* 24-bit RGB with alpha channel. */
             gcmASSERT(LayerIndex == 0);
             clearValueType = (gceVALUE_TYPE)   (ClearArgs->color.valueType   |
                                                gcvVALUE_FLAG_UNSIGNED_DENORM |
                                                gcvVALUE_FLAG_GAMMAR);

             Surface->clearValue[0]
                /* Convert red to 8-bit. */
                = (_ConvertValue(clearValueType,
                                 ClearArgs->color.r, 8) << 16)
                 /* Convert green to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 8) << 8)
                 /* Convert blue to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.b, 8));

             clearValueType &= ~gcvVALUE_FLAG_GAMMAR;
             Surface->clearValue[0] |= (_ConvertValue(clearValueType,
                                                    ClearArgs->color.a, 8) << 24);
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;


         case gcvSURF_X2R10G10B10:
         case gcvSURF_A2R10G10B10:
             gcmASSERT(LayerIndex == 0);
             clearValueType = (ClearArgs->color.valueType | gcvVALUE_FLAG_UNSIGNED_DENORM);
             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 /* Convert red to 10-bit. */
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.r, 10) << 20)
                 /* Convert green to 10-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 10) << 10)
                 /* Convert blue to 10-bit. */
                 | _ConvertValue(clearValueType,
                                 ClearArgs->color.b, 10)
                 /* Convert alpha to 2-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.a, 2) << 30);
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;

             break;

         case gcvSURF_X2B10G10R10:
         case gcvSURF_A2B10G10R10:
             gcmASSERT(LayerIndex == 0);
             clearValueType = (ClearArgs->color.valueType| gcvVALUE_FLAG_UNSIGNED_DENORM);
             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 /* Convert red to 10-bit. */
                 = _ConvertValue(clearValueType,
                                 ClearArgs->color.r, 10)
                 /* Convert green to 10-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 10) << 10)
                 /* Convert blue to 10-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.b, 10) << 20)
                 /* Convert alpha to 2-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.a, 2) << 30);
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;

             break;

          case gcvSURF_A8B12G12R12_2_A8R8G8B8:
             clearValueType = (ClearArgs->color.valueType| gcvVALUE_FLAG_UNSIGNED_DENORM);
             switch (LayerIndex)
             {
             case 0:
                 Surface->clearValueUpper[0] =
                 Surface->clearValue[0] =
                    /* Convert red upper 4 bits to 8-bit*/
                    ( ((_ConvertValue(clearValueType,
                                     ClearArgs->color.r, 12) & 0xf00) >> 4) << 16)
                    /* Convert green upper 4 bits to 8-bit*/
                    | (((_ConvertValue(clearValueType,
                                     ClearArgs->color.g, 12) & 0xf00) >> 4) << 8)
                    /* Convert blue upper 4 bits to 8-bit*/
                    | ((_ConvertValue(clearValueType,
                                    ClearArgs->color.b, 12) & 0xf00) >> 4)
                        /* Convert alpha to 8-bit. */
                    | (_ConvertValue(clearValueType,
                                     ClearArgs->color.a, 8) << 24);
                 break;

             case 1:
                 Surface->clearValueUpper[1] =
                 Surface->clearValue[1] =
                    /* Convert red lower 8 bits to 8-bit. */
                    ( (_ConvertValue(clearValueType,
                                     ClearArgs->color.r, 12) & 0xff) << 16)
                    /* Convert green lower 8 bits to 8-bit. */
                    | ((_ConvertValue(clearValueType,
                                     ClearArgs->color.g, 12) & 0xff) << 8)
                    /* Convert blue lower 8 bits to 8-bit. */
                    | (_ConvertValue(clearValueType,
                                    ClearArgs->color.b, 12) & 0xff)
                        /* Convert alpha to 8-bit. */
                    | (_ConvertValue(clearValueType,
                                     ClearArgs->color.a, 8) << 24);
                 break;

             default:
                 gcmASSERT(0);
                 break;
             }

             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;

         case gcvSURF_R5G5B5A1:
             gcmASSERT(LayerIndex == 0);
             clearValueType = (ClearArgs->color.valueType| gcvVALUE_FLAG_UNSIGNED_DENORM);
             Surface->clearValue[0]
                 /* Convert red into 5-bit. */
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.r, 5) << 11)
                 /* Convert green into 5-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 5) << 6)
                 /* Convert blue into 5-bit. */
                 | (_ConvertValue(clearValueType,
                                 ClearArgs->color.b, 5) << 1)
                 /* Convert alpha into 1-bit. */
                 | _ConvertValue(clearValueType,
                                  ClearArgs->color.a, 1);

             /* Expand 16-bit color into 32-bit color. */
             Surface->clearValue[0] |= Surface->clearValue[0] << 16;
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_A2B10G10R10UI:
         case gcvSURF_A2B10G10R10UI_1_A8R8G8B8:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 /* Convert red to 10-bit. */
                 = _ConvertValue(clearValueType,
                                 ClearArgs->color.r, 10)
                 /* Convert green to 10-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 10) << 10)
                 /* Convert blue to 10-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.b, 10) << 20)
                 /* Convert alpha to 2-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.a, 2) << 30);
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;

         case gcvSURF_A8B8G8R8I:
         case gcvSURF_A8B8G8R8UI:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 /* Convert red to 8-bit. */
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.r, 8))
                 /* Convert green to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 8) << 8)
                 /* Convert blue to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.b, 8) << 16)
                 /* Convert alpha to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.a, 8) << 24);
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;

         case gcvSURF_A8B8G8R8I_1_A8R8G8B8:
         case gcvSURF_A8B8G8R8UI_1_A8R8G8B8:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 /* Convert blue  to 8-bit. */
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.b, 8))
                 /* Convert green to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 8) << 8)
                 /* Convert red   to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.r, 8) << 16)
                 /* Convert alpha to 8-bit. */
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.a, 8) << 24);
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;

         case gcvSURF_R8I:
         case gcvSURF_R8UI:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValue[0] = _ConvertValue(clearValueType, ClearArgs->color.r, 8);
             Surface->clearValue[0] |= (Surface->clearValue[0] << 8);
             Surface->clearValue[0] |= (Surface->clearValue[0] << 16);
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 8;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_R8:
             gcmASSERT(LayerIndex == 0);
             clearValueType = (ClearArgs->color.valueType| gcvVALUE_FLAG_UNSIGNED_DENORM);

             Surface->clearValue[0] = _ConvertValue(clearValueType, ClearArgs->color.r, 8);
             Surface->clearValue[0] |= (Surface->clearValue[0] << 8);
             Surface->clearValue[0] |= (Surface->clearValue[0] << 16);
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 8;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_G8R8I:
         case gcvSURF_G8R8UI:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValue[0]
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.r, 8)
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 8) << 8));

             Surface->clearValue[0] |= (Surface->clearValue[0] << 16);
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];

             break;

         case gcvSURF_R16F:
             gcmASSERT(LayerIndex == 0);
             clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_FLOAT_TO_FLOAT16);

             Surface->clearValue[0] =
                 _ConvertValue(clearValueType, ClearArgs->color.r, 16);
             Surface->clearValue[0] |= Surface->clearValue[0] << 16;
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];

             break;

         case gcvSURF_B10G11R11F:
         case gcvSURF_B10G11R11F_1_A8R8G8B8:
             {
             gctUINT16 r,g,b;
             gcmASSERT(LayerIndex == 0);

             r = (gctUINT16)gcoMATH_FloatToFloat11(ClearArgs->color.r.uintValue);
             g = (gctUINT16)gcoMATH_FloatToFloat11(ClearArgs->color.g.uintValue);
             b = (gctUINT16)gcoMATH_FloatToFloat10(ClearArgs->color.b.uintValue);

             Surface->clearValue[0] = (b << 22) | (g << 11) | r;
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             }
             break;

         case gcvSURF_R16I:
         case gcvSURF_R16UI:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValue[0] =
                 _ConvertValue(clearValueType, ClearArgs->color.r, 16);
             Surface->clearValue[0] |= Surface->clearValue[0] << 16;
             Surface->clearValueUpper[0] = Surface->clearValue[0];
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMask[LayerIndex] |= Surface->clearBitMask[LayerIndex] << 16;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];

             break;

         case gcvSURF_G16R16F:
             gcmASSERT(LayerIndex == 0);
             clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_FLOAT_TO_FLOAT16);

             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 = (_ConvertValue(clearValueType, ClearArgs->color.r, 16)
                 | (_ConvertValue(clearValueType, ClearArgs->color.g, 16) << 16));
             Surface->clearBitMask[LayerIndex] = clearBitMask32;
             Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;

         case gcvSURF_G16R16I:
         case gcvSURF_G16R16UI:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 = (_ConvertValue(clearValueType, ClearArgs->color.r, 16)
                 | (_ConvertValue(clearValueType, ClearArgs->color.g, 16) << 16));
            Surface->clearBitMask[LayerIndex] = clearBitMask32;
            Surface->clearBitMaskUpper[LayerIndex] = clearBitMask32;
             break;

         case gcvSURF_A16B16G16R16F:
         case gcvSURF_X16B16G16R16F:
             gcmASSERT(LayerIndex == 0);
             clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_FLOAT_TO_FLOAT16);

             Surface->clearValue[0]
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.r, 16)
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.g, 16) << 16));
             Surface->clearValueUpper[0]
                 = (_ConvertValue(clearValueType,
                                  ClearArgs->color.b, 16)
                 | (_ConvertValue(clearValueType,
                                  ClearArgs->color.a, 16) << 16));

             Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Red) | CHANNEL_BITMASK(Green);
             Surface->clearBitMaskUpper[LayerIndex] = CHANNEL_BITMASK(Blue) | CHANNEL_BITMASK(Alpha);
              break;

         case gcvSURF_A16B16G16R16F_2_A8R8G8B8:
             clearValueType = (ClearArgs->color.valueType | gcvVALUE_FLAG_FLOAT_TO_FLOAT16);

             switch (LayerIndex)
             {
             case 0:
                 Surface->clearValueUpper[0] =
                 Surface->clearValue[0]
                     = (_ConvertValue(clearValueType, ClearArgs->color.r, 16)
                     | (_ConvertValue(clearValueType, ClearArgs->color.g, 16) << 16));

                 Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Red) | CHANNEL_BITMASK(Green);
                 break;
             case 1:
                 Surface->clearValueUpper[1] =
                 Surface->clearValue[1]
                     = (_ConvertValue(clearValueType, ClearArgs->color.b, 16)
                     | (_ConvertValue(clearValueType, ClearArgs->color.a, 16) << 16));

                 Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Blue) | CHANNEL_BITMASK(Alpha);
                 break;

             default:
                 gcmASSERT(0);
                 break;
             }

             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_A16B16G16R16F_2_G16R16F:
             clearValueType = (gceVALUE_TYPE) (ClearArgs->color.valueType | gcvVALUE_FLAG_FLOAT_TO_FLOAT16);

             switch (LayerIndex)
             {
             case 0:
                 Surface->clearValueUpper[0] =
                     Surface->clearValue[0]
                 = (_ConvertValue(clearValueType,
                     ClearArgs->color.r, 16)
                     | (_ConvertValue(clearValueType, ClearArgs->color.g, 16) << 16));
                 Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Red) | CHANNEL_BITMASK(Green);

                 break;
             case 1:
                 Surface->clearValueUpper[1] =
                     Surface->clearValue[1]
                 = (_ConvertValue(clearValueType, ClearArgs->color.b, 16)
                     | (_ConvertValue(clearValueType, ClearArgs->color.a, 16) << 16));
                 Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Blue) | CHANNEL_BITMASK(Alpha);

                 break;
             default:
                 gcmASSERT(0);
                 break;
             }

             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_A16B16G16R16I:
         case gcvSURF_X16B16G16R16I:
         case gcvSURF_A16B16G16R16UI:
         case gcvSURF_X16B16G16R16UI:
         case gcvSURF_A16B16G16R16I_1_G32R32F:
         case gcvSURF_A16B16G16R16UI_1_G32R32F:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValue[0]
                 = (_ConvertValue(clearValueType, ClearArgs->color.r, 16)
                 | (_ConvertValue(clearValueType, ClearArgs->color.g, 16) << 16));
             Surface->clearValueUpper[0]
                 = (_ConvertValue(clearValueType, ClearArgs->color.b, 16)
                 | (_ConvertValue(clearValueType, ClearArgs->color.a, 16) << 16));
             Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Red) | CHANNEL_BITMASK(Green);
             Surface->clearBitMaskUpper[LayerIndex] = CHANNEL_BITMASK(Blue) | CHANNEL_BITMASK(Alpha);

             break;

         case gcvSURF_A16B16G16R16I_2_A8R8G8B8:
         case gcvSURF_A16B16G16R16UI_2_A8R8G8B8:
             clearValueType = ClearArgs->color.valueType;

             switch (LayerIndex)
             {
             case 0:
                 Surface->clearValueUpper[0] =
                 Surface->clearValue[0]
                     = (_ConvertValue(clearValueType, ClearArgs->color.r, 16)
                     | (_ConvertValue(clearValueType, ClearArgs->color.g, 16) << 16));
                 Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Red) | CHANNEL_BITMASK(Green);

                 break;

             case 1:
                 Surface->clearValueUpper[1] =
                 Surface->clearValue[1]
                     = (_ConvertValue(clearValueType, ClearArgs->color.b, 16)
                     | (_ConvertValue(clearValueType, ClearArgs->color.a, 16) << 16));
                 Surface->clearBitMask[LayerIndex] = CHANNEL_BITMASK(Blue) | CHANNEL_BITMASK(Alpha);

                 break;

             default:
                 gcmASSERT(0);
                 break;
             }

             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_R32F:
         case gcvSURF_R32I:
         case gcvSURF_R32UI:
         case gcvSURF_R32UI_1_A8R8G8B8:
         case gcvSURF_R32I_1_A8R8G8B8:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValueUpper[0] =
             Surface->clearValue[0]
                 = (_ConvertValue(clearValueType, ClearArgs->color.r, 32));
             Surface->clearBitMask[LayerIndex] = maskRed;
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         case gcvSURF_G32R32F:
         case gcvSURF_G32R32I:
         case gcvSURF_G32R32UI:
             gcmASSERT(LayerIndex == 0);
             clearValueType = ClearArgs->color.valueType;

             Surface->clearValue[0]
                 = (_ConvertValue(clearValueType, ClearArgs->color.r, 32));
             Surface->clearValueUpper[0]
                 = (_ConvertValue(clearValueType, ClearArgs->color.g, 32));

             Surface->clearBitMask[LayerIndex] = maskRed;
             Surface->clearBitMaskUpper[LayerIndex] = maskGreen;
             break;

         case gcvSURF_A32B32G32R32F_2_G32R32F:
         case gcvSURF_X32B32G32R32F_2_G32R32F:
         case gcvSURF_A32B32G32R32I_2_G32R32I:
         case gcvSURF_X32B32G32R32I_2_G32R32I:
         case gcvSURF_A32B32G32R32UI_2_G32R32UI:
         case gcvSURF_X32B32G32R32UI_2_G32R32UI:
             clearValueType = ClearArgs->color.valueType;

             switch (LayerIndex)
             {
             case 0:
                 Surface->clearValue[0]
                     = (_ConvertValue(clearValueType, ClearArgs->color.r, 32));
                 Surface->clearValueUpper[0]
                     = (_ConvertValue(clearValueType, ClearArgs->color.g, 32));

                 Surface->clearBitMask[LayerIndex] = maskRed;
                 Surface->clearBitMaskUpper[LayerIndex] = maskGreen;
                 break;

             case 1:
                 Surface->clearValue[1]
                     = (_ConvertValue(clearValueType, ClearArgs->color.b, 32));
                 Surface->clearValueUpper[1]
                     = (_ConvertValue(clearValueType, ClearArgs->color.a, 32));

                 Surface->clearBitMask[LayerIndex] = maskBlue;
                 Surface->clearBitMaskUpper[LayerIndex] = maskAlpha;
                 break;
             default:
                 gcmASSERT(0);
                 break;
             }
             break;

         case gcvSURF_A32B32G32R32I_4_A8R8G8B8:
         case gcvSURF_A32B32G32R32UI_4_A8R8G8B8:
             clearValueType = ClearArgs->color.valueType;

             switch (LayerIndex)
             {
             case 0:
                 Surface->clearValueUpper[0] =
                 Surface->clearValue[0]
                     = (_ConvertValue(clearValueType, ClearArgs->color.r, 32));
                 Surface->clearBitMask[LayerIndex] = maskRed;
                 break;
             case 1:
                 Surface->clearValueUpper[1] =
                 Surface->clearValue[1]
                     = (_ConvertValue(clearValueType, ClearArgs->color.g, 32));
                 Surface->clearBitMask[LayerIndex] = maskGreen;
                 break;
             case 2:
                 Surface->clearValueUpper[2] =
                 Surface->clearValue[2]
                     = (_ConvertValue(clearValueType, ClearArgs->color.b, 32));
                Surface->clearBitMask[LayerIndex] = maskBlue;
                 break;
             case 3:
                 Surface->clearValueUpper[3] =
                 Surface->clearValue[3]
                     = (_ConvertValue(clearValueType, ClearArgs->color.a, 32));
                 Surface->clearBitMask[LayerIndex] = maskAlpha;
                 break;
             default:
                 gcmASSERT(0);
                 break;
             }
             Surface->clearBitMaskUpper[LayerIndex] = Surface->clearBitMask[LayerIndex];
             break;

         default:
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): Unknown format=%d",
                __FUNCTION__, __LINE__, Surface->format
                );

            /* Invalid surface format. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
         }
    }

    /* Test for clearing depth or stencil buffer. */
    if (ClearArgs->flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
    {
        gctBOOL  clearDepth   = (ClearArgs->flags & gcvCLEAR_DEPTH) && ClearArgs->depthMask;
        gctBOOL  clearStencil = (ClearArgs->flags & gcvCLEAR_STENCIL) && ClearArgs->stencilMask;
        gctUINT32 clearValue = 0;
        gctFLOAT tempValue = 0.0f;

        clearValueType = (gceVALUE_TYPE)(gcvVALUE_FLOAT | gcvVALUE_FLAG_UNSIGNED_DENORM);
        tempMask = 0;
        Surface->clearMask[0] = 0;

            /* Dispatch on depth format. */
        switch (Surface->format)
        {
        case gcvSURF_D16: /* 16-bit depth without stencil. */
            /* Convert depth value to 16-bit. */
            if (clearDepth)
            {
                tempValue = gcoMATH_Multiply(gcoMATH_UInt2Float((1 << 16) - 1), gcmFLOATCLAMP_0_TO_1(ClearArgs->depth.floatValue));
                /* This rounding mode need to be consistent with HW in RA & PE */
                clearValue = gcoMath_Float2UINT_STICKROUNDING(tempValue);
                clearValue |= (clearValue << 16);
                tempMask = 0xFFFFFFFF;
                Surface->clearMask[0] |= 0xF;
            }

            break;

        case gcvSURF_D24S8: /* 24-bit depth with 8-bit stencil. */
            /* Convert depth value to 24-bit. */
            if (clearDepth)
            {
                tempValue = gcoMATH_Multiply(gcoMATH_UInt2Float((1 << 24) - 1), gcmFLOATCLAMP_0_TO_1(ClearArgs->depth.floatValue));
                /* This rounding mode need to be consistent with HW in RA & PE */
                clearValue = (gcoMath_Float2UINT_STICKROUNDING(tempValue) << 8);
                tempMask = 0xFFFFFF00;
                Surface->clearMask[0] |= 0xE;
            }

            if (clearStencil)
            {
                clearValue &= ~0xFF;
                clearValue |= ClearArgs->stencil;
                tempMask |= ClearArgs->stencilMask;
                Surface->clearMask[0] |= 0x1;
            }

            break;

        case gcvSURF_D24X8: /* 24-bit depth with no stencil. */
            /* Convert depth value to 24-bit. */
            if (clearDepth)
            {
                tempValue = gcoMATH_Multiply(gcoMATH_UInt2Float((1 << 24) - 1), gcmFLOATCLAMP_0_TO_1(ClearArgs->depth.floatValue));
                /* This rounding mode need to be consistent with HW in RA & PE */
                clearValue = (gcoMath_Float2UINT_STICKROUNDING(tempValue) << 8);
                tempMask = 0xFFFFFFFF;
                Surface->clearMask[0] = 0xF;
            }

            break;

        case gcvSURF_S8:
        case gcvSURF_X24S8: /* 8-bit stencil with no depth. */
            if (clearStencil)
            {
                clearValue = ClearArgs->stencil;
                tempMask |= ClearArgs->stencilMask;
                Surface->clearMask[0] = 0xF;
            }

            clearValue  |= (clearValue << 8);
            clearValue  |= (clearValue << 16);
            tempMask    |= (tempMask << 8 );
            tempMask    |= (tempMask << 16 );

            break;

        default:
            /* Invalid depth buffer format. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        Surface->clearValue[0] = clearValue;
        Surface->clearValueUpper[0] = clearValue;
        Surface->clearBitMask[0] = tempMask;
        Surface->clearBitMaskUpper[0] = 0xFFFFFFFF;

        /* Compute HZ clear value. */
        gcmONERROR(gcoHARDWARE_HzClearValueControl(Surface->format,
                                                   Surface->clearValue[0],
                                                   &Surface->clearValueHz,
                                                   gcvNULL));

    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the error. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ClearRect(
    IN gcsSURF_VIEW *SurfView,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctUINT32 LayerIndex
    )
{
    gceSTATUS status;
    gctUINT32 address;
    gcsRECT_PTR rect = ClearArgs->clearRect;

    gcoSURF surf = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x ClearArgs=0x%x, LayerIndex=%d",
                   SurfView, ClearArgs, LayerIndex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(surf, gcvOBJ_SURF);
    gcmDEBUG_VERIFY_ARGUMENT(ClearArgs != gcvNULL);

    /* LayerIndex only apply for color buffer */
    gcmASSERT(LayerIndex == 0 || (ClearArgs->flags & gcvCLEAR_COLOR));

    gcmGETHARDWAREADDRESS(surf->node, address);
    gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                  "_ClearRect: Clearing surface 0x%x @ 0x%08X",
                  surf, address);

    if (!(ClearArgs->flags & gcvCLEAR_WITH_CPU_ONLY)
        && (surf->isMsaa)
        && ((gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_FAST_MSAA) == gcvTRUE) ||
            (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SMALL_MSAA) == gcvTRUE)))
    {
        gcmONERROR(gcvSTATUS_NOT_ALIGNED);
    }

    if (ClearArgs->flags & gcvCLEAR_WITH_GPU_ONLY)
    {
        gctUINT sizeX, sizeY;
        gctUINT originX, originY;

        /* Query resolve clear alignment for this surface. */
        gcmONERROR(
            gcoHARDWARE_GetSurfaceResolveAlignment(gcvNULL,
                                                   surf,
                                                   &originX,
                                                   &originY,
                                                   &sizeX,
                                                   &sizeY));

        if ((rect->left & (originX - 1)) ||
            (rect->top  & (originY - 1)) ||
            ((rect->right  * surf->sampleInfo.x < (gctINT)surf->requestW) &&
             ((rect->right  - rect->left) & (sizeX - 1))
            ) ||
            ((rect->bottom * surf->sampleInfo.y < (gctINT)surf->requestH) &&
             ((rect->bottom - rect->top ) & (sizeY - 1))
            )
           )
        {
            /* Quickly reject if not resolve aligned.
             * Avoid decompress and disable tile status below.
            */
            status = gcvSTATUS_NOT_ALIGNED;
            goto OnError;
        }
    }

    /* Flush the tile status and decompress the buffers. */
    gcmONERROR(gcoSURF_DisableTileStatus(SurfView, gcvTRUE));

    /* Compute clear values. */
    gcmONERROR(_ComputeClear(surf, ClearArgs, LayerIndex));

    if (surf->clearMask[LayerIndex])
    {
        gcsRECT msaaRect = *rect;
        /* Adjust the rect according to the msaa sample. */
        msaaRect.left   *= surf->sampleInfo.x;
        msaaRect.right  *= surf->sampleInfo.x;
        msaaRect.top    *= surf->sampleInfo.y;
        msaaRect.bottom *= surf->sampleInfo.y;

        /* Clamp the coordinates to surface dimensions. */
        msaaRect.left   = gcmMAX(msaaRect.left,   0);
        msaaRect.top    = gcmMAX(msaaRect.top,    0);
        msaaRect.right  = gcmMIN(msaaRect.right,  (gctINT32)surf->alignedW);
        msaaRect.bottom = gcmMIN(msaaRect.bottom, (gctINT32)surf->alignedH);

        status = gcvSTATUS_NOT_SUPPORTED;

        /* Try hardware clear if requested or default. */
        if (!(ClearArgs->flags & gcvCLEAR_WITH_CPU_ONLY) &&
            /* HW stencil clear only support byte masked. */
            ( !(ClearArgs->flags & gcvCLEAR_STENCIL)
            || (ClearArgs->stencilMask == 0xff)
            || (ClearArgs->stencilMask == 0x00))
           )
        {
            status = gcoHARDWARE_Clear(gcvNULL,
                                       SurfView,
                                       LayerIndex,
                                       &msaaRect,
                                       surf->clearValue[LayerIndex],
                                       surf->clearValueUpper[LayerIndex],
                                       surf->clearMask[LayerIndex]);
        }

        /* Try software clear if requested or default. */
        if (gcmIS_ERROR(status) && !(ClearArgs->flags & gcvCLEAR_WITH_GPU_ONLY))
        {
            gctUINT8 stencilWriteMask = (ClearArgs->flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
                                      ? ClearArgs->stencilMask
                                      : 0xFF;
            status = gcoHARDWARE_ClearSoftware(gcvNULL,
                                               SurfView,
                                               LayerIndex,
                                               &msaaRect,
                                               surf->clearValue[LayerIndex],
                                               surf->clearValueUpper[LayerIndex],
                                               surf->clearMask[LayerIndex],
                                               stencilWriteMask);
        }

        /* Break now if status is error. */
        if (status == gcvSTATUS_NOT_ALIGNED)
        {
            goto OnError;
        }
        gcmONERROR(status);

        /* Clear HZ if have */
        if ((ClearArgs->flags & gcvCLEAR_DEPTH) && (surf->hzNode.size > 0))
        {
            gcsRECT hzRect = {0};
            struct _gcoSURF hzSurf;
            gcsSURF_VIEW hzView = {gcvNULL, 0, 1};
            gctUINT width = 0, height = 0;
            gcsSURF_FORMAT_INFO_PTR fmtInfo = gcvNULL;

            /* Compute the hw specific clear window. */
            gcmONERROR(gcoHARDWARE_ComputeClearWindow(gcvNULL, surf->hzNode.size, &width, &height));

            gcoOS_ZeroMemory(&hzSurf, gcmSIZEOF(hzSurf));
            hzSurf.object.type     = gcvOBJ_SURF;
            hzSurf.requestW   = width;
            hzSurf.requestH   = height;
            hzSurf.requestD   = 1;
            hzSurf.allocedW   = width;
            hzSurf.allocedH   = height;
            hzSurf.alignedW   = width;
            hzSurf.alignedH   = height;
            hzSurf.sampleInfo = g_sampleInfos[1];
            hzSurf.isMsaa     = gcvFALSE;
            hzSurf.vMsaa     = gcvFALSE;
            hzSurf.format     = gcvSURF_A8R8G8B8;
            hzSurf.node       = surf->hzNode;
            hzSurf.stride     = width * 4;
            hzSurf.colorSpace = gcvSURF_COLOR_SPACE_LINEAR;

            gcoSURF_QueryFormat(gcvSURF_A8R8G8B8, &fmtInfo);
            hzSurf.formatInfo = *fmtInfo;
            hzSurf.bitsPerPixel = fmtInfo->bitsPerPixel;
            hzSurf.sliceSize = (gctUINT)surf->hzNode.size;
            hzSurf.layerSize = (gctUINT)surf->hzNode.size;

            /* If full clear, clear HZ to clear value*/
            if (rect->left == 0 && rect->right  >= (gctINT)surf->requestW &&
                rect->top == 0  && rect->bottom >= (gctINT)surf->requestH)
            {
                hzSurf.clearValueHz = surf->clearValueHz;
            }
            else
            {
                hzSurf.clearValueHz = 0xffffffff;
            }

            if (surf->tiling & gcvTILING_SPLIT_BUFFER)
            {
                hzSurf.tiling = gcvMULTI_TILED;
                hzSurf.bottomBufferOffset = (gctUINT32)surf->hzNode.size / 2;
            }
            else
            {
                hzSurf.tiling = gcvTILED;
            }
            hzSurf.pfGetAddr = gcoHARDWARE_GetProcCalcPixelAddr(gcvNULL, &hzSurf);

            hzRect.right = width;
            hzRect.bottom = height;

            hzView.surf = &hzSurf;
            /* Send clear command to hardware. */
            gcmONERROR(gcoHARDWARE_Clear(gcvNULL,
                                         &hzView,
                                         0,
                                         &hzRect,
                                         hzSurf.clearValueHz,
                                         hzSurf.clearValueHz,
                                         0xF));

            surf->hzDisabled = gcvFALSE;
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_DoClearTileStatus(
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 TileStatusAddress,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctINT32 LayerIndex
    )
{
    gceSTATUS status;
    gcoSURF Surface = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x TileStatusAddress=0x%08x ClearArgs=0x%x, LayerIndex=%d",
                  SurfView, TileStatusAddress, ClearArgs, LayerIndex);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Surface != 0);
    gcmDEBUG_VERIFY_ARGUMENT(TileStatusAddress != 0);

    /* Compute clear values. */
    gcmONERROR(
        _ComputeClear(Surface, ClearArgs, LayerIndex));

    gcmASSERT(LayerIndex == 0);

    /* Test for clearing render target. */
    if (ClearArgs->flags & gcvCLEAR_COLOR)
    {
        if (Surface->clearMask[0] != 0)
        {
            /* Always flush (and invalidate) the tile status cache. */
            gcmONERROR(
                gcoHARDWARE_FlushTileStatus(gcvNULL,
                                            SurfView,
                                            gcvFALSE));

            /* Send the clear command to the hardware. */
            status =
                gcoHARDWARE_ClearTileStatus(gcvNULL,
                                            SurfView,
                                            TileStatusAddress,
                                            0,
                                            gcvSURF_RENDER_TARGET,
                                            Surface->clearValue[0],
                                            Surface->clearValueUpper[0],
                                            Surface->clearMask[0]);

            if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                goto OnError;
            }

            gcmONERROR(status);
        }
        else
        {
            gcmFOOTER_ARG("%s", "gcvSTATUS_SKIP");
            return gcvSTATUS_SKIP;
        }
    }

    /* Test for clearing depth or stencil buffer. */
    if (ClearArgs->flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
    {
        if (Surface->clearMask[0] != 0)
        {
            status = gcvSTATUS_NOT_SUPPORTED;

            if (!(ClearArgs->flags & gcvCLEAR_STENCIL) || (ClearArgs->stencilMask == 0xff))
            {
                /* Always flush (and invalidate) the tile status cache. */
                gcmONERROR(
                    gcoHARDWARE_FlushTileStatus(gcvNULL,
                                                SurfView,
                                                gcvFALSE));

                /* Send clear command to hardware. */
                status = gcoHARDWARE_ClearTileStatus(gcvNULL,
                                                     SurfView,
                                                     TileStatusAddress,
                                                     0,
                                                     gcvSURF_DEPTH,
                                                     Surface->clearValue[0],
                                                     Surface->clearValueUpper[0],
                                                     Surface->clearMask[0]);
            }

            if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                goto OnError;
            }
            gcmONERROR(status);

            /* Send semaphore from RASTER to PIXEL. */
            gcmONERROR(
                gcoHARDWARE_Semaphore(gcvNULL,
                                      gcvWHERE_RASTER,
                                      gcvWHERE_PIXEL,
                                      gcvHOW_SEMAPHORE,
                                      gcvNULL));
        }
        else
        {
            gcmFOOTER_ARG("%s", "gcvSTATUS_SKIP");
            return gcvSTATUS_SKIP;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  _ClearHzTileStatus
**
**  Perform a hierarchical Z tile status clear with the current depth values.
**  Note that this function does not recompute the clear colors, since it must
**  be called after gco3D_ClearTileStatus has cleared the depth tile status.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcoSURF Surface
**          Pointer of the depth surface to clear.
**
**      gcsSURF_NODE_PTR TileStatusAddress
**          Pointer to the hierarhical Z tile status node toclear.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_ClearHzTileStatus(
    IN gcoSURF Surface,
    IN gcsSURF_NODE_PTR TileStatus
    )
{
    gceSTATUS status;
    gctUINT32 address;
    gcsSURF_VIEW surfView = {Surface, 0, 1};

    gcmHEADER_ARG("Surface=0x%x TileStatus=0x%x",
                   Surface, TileStatus);

    gcmGETHARDWAREADDRESS(*TileStatus, address);

    /* Send clear command to hardware. */
    gcmONERROR(
        gcoHARDWARE_ClearTileStatus(gcvNULL,
                                    &surfView,
                                    address,
                                    TileStatus->size,
                                    gcvSURF_HIERARCHICAL_DEPTH,
                                    Surface->clearValueHz,
                                    Surface->clearValueHz,
                                    0xF));

    /* Send semaphore from RASTER to PIXEL. */
    gcmONERROR(
        gcoHARDWARE_Semaphore(gcvNULL,
                              gcvWHERE_RASTER,
                              gcvWHERE_PIXEL,
                              gcvHOW_SEMAPHORE,
                              gcvNULL));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/* Attempt to clear using tile status. */
static gceSTATUS
_ClearTileStatus(
    IN gcsSURF_VIEW *SurfView,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctINT32 LayerIndex
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;
    gctUINT32 address;
    gcoSURF Surface = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x ClearArgs=0x%x LayerIndex=%d", SurfView, ClearArgs, LayerIndex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmDEBUG_VERIFY_ARGUMENT(ClearArgs != gcvNULL);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurfView->numSlices == 1);

    if (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        gctBOOL saved = Surface->tileStatusDisabled[SurfView->firstSlice];

        do
        {
            gcmGETHARDWAREADDRESS(Surface->tileStatusNode, address);

            gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                          "_ClearTileStatus: Clearing tile status 0x%x @ 0x%08X for"
                          "surface 0x%x",
                          Surface->tileStatusNode,
                          address,
                          Surface);
            /* Turn on the tile status on in the beginning,
            ** So the later invalidate ts cache will not be skipped. */
            Surface->tileStatusDisabled[SurfView->firstSlice] = gcvFALSE;

            /* Clear the tile status. */
            status = _DoClearTileStatus(SurfView,
                                        address,
                                        ClearArgs,
                                        LayerIndex);

            if (status == gcvSTATUS_SKIP)
            {
                /* Should restore the tile status when skip the clear. */
                Surface->tileStatusDisabled[SurfView->firstSlice] = saved;

                /* Nothing needed clearing, and no error has occurred. */
                status = gcvSTATUS_OK;
                break;
            }

            if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                break;
            }

            gcmERR_BREAK(status);

            if ((ClearArgs->flags & gcvCLEAR_DEPTH) &&
                (Surface->hzTileStatusNode.pool != gcvPOOL_UNKNOWN))
            {
                /* Clear the hierarchical Z tile status. */
                status = _ClearHzTileStatus(Surface, &Surface->hzTileStatusNode);

                if (status == gcvSTATUS_NOT_SUPPORTED)
                {
                    break;
                }

                gcmERR_BREAK(status);

                Surface->hzDisabled = gcvFALSE;
            }

            /* Reset the tile status. */
            gcmERR_BREAK(
                gcoSURF_EnableTileStatus(SurfView));
        }
        while (gcvFALSE);

        /* Restore if failed. */
        if (gcmIS_ERROR(status))
        {
            Surface->tileStatusDisabled[SurfView->firstSlice] = saved;
        }
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if gcdPARTIAL_FAST_CLEAR
static gceSTATUS
_ClearTileStatusWindowAligned(
    IN gcsSURF_VIEW *SurfView,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctINT32 LayerIndex,
    OUT gcsRECT_PTR AlignedRect
    )
{
    gceSTATUS status;
    gcoSURF Surface = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x ClearArgs=0x%x LayerIndex=%d",
                  SurfView, ClearArgs, LayerIndex);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(ClearArgs != 0);

    /* No MRT support when tile status enabled for now. */
    gcmASSERT(LayerIndex == 0);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurfView->numSlices == 1);

    /* Compute clear values. */
    gcmONERROR(_ComputeClear(Surface, ClearArgs, LayerIndex));

    /* Check clear masks. */
    if (Surface->clearMask[0] == 0)
    {
        /* Nothing to clear. */
        gcmFOOTER_ARG("%s", "gcvSTATUS_SKIP");
        return gcvSTATUS_SKIP;
    }

    /* Check clearValue changes. */
    if ((Surface->tileStatusDisabled[SurfView->firstSlice] == gcvFALSE)
    &&  ((Surface->fcValue[SurfView->firstSlice] != Surface->clearValue[0])
        || (Surface->fcValueUpper[SurfView->firstSlice] != Surface->clearValueUpper[0]))
    )
    {
        /*
         * Tile status is on but clear value changed.
         * Fail back to 3D draw clear or disable tile status and continue
         * partial fast clear.
         */

        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER();
        return status;
    }

    /* Flush the tile status cache. */
    gcmONERROR(gcoHARDWARE_FlushTileStatus(gcvNULL, SurfView, gcvFALSE));

    /* Test for clearing render target. */
    if (ClearArgs->flags & gcvCLEAR_COLOR)
    {
        /* Send the clear command to the hardware. */
        status =
            gcoHARDWARE_ClearTileStatusWindowAligned(gcvNULL,
                                                     SurfView,
                                                     gcvSURF_RENDER_TARGET,
                                                     Surface->clearValue[0],
                                                     Surface->clearValueUpper[0],
                                                     Surface->clearMask[0],
                                                     ClearArgs->clearRect,
                                                     AlignedRect);

        if (status == gcvSTATUS_NOT_SUPPORTED)
        {
            goto OnError;
        }

        gcmONERROR(status);
    }

    /* Test for clearing depth or stencil buffer. */
    if (ClearArgs->flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
    {
        if (Surface->hzNode.pool != gcvPOOL_UNKNOWN)
        {
            /* Can not support clear depth when HZ buffer exists. */
            status = gcvSTATUS_NOT_SUPPORTED;
            goto OnError;
        }

        /* Send the clear command to the hardware. */
        status =
            gcoHARDWARE_ClearTileStatusWindowAligned(gcvNULL,
                                                     SurfView,
                                                     gcvSURF_DEPTH,
                                                     Surface->clearValue[0],
                                                     Surface->clearValueUpper[0],
                                                     Surface->clearMask[0],
                                                     ClearArgs->clearRect,
                                                     AlignedRect);

        if (status == gcvSTATUS_NOT_SUPPORTED)
        {
            goto OnError;
        }

        gcmONERROR(status);

        gcmONERROR(
            gcoHARDWARE_Semaphore(gcvNULL,
                                  gcvWHERE_RASTER,
                                  gcvWHERE_PIXEL,
                                  gcvHOW_SEMAPHORE_STALL,
                                  gcvNULL));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_PartialFastClear(
    IN gcsSURF_VIEW *SurfView,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctINT32 LayerIndex
    )
{
    gceSTATUS status;
    gcsRECT alignedRect;
    gctBOOL saved;
    gcoSURF surf = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%x ClearArgs=0x%x LayerIndex=%d",
                  SurfView, ClearArgs, LayerIndex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(surf, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(ClearArgs != gcvNULL);

    if ((surf->tileStatusNode.pool == gcvPOOL_UNKNOWN) ||
        (surf->isMsaa))
    {
        /* No tile status buffer or AA. */
        status = gcvSTATUS_NOT_SUPPORTED;
        goto OnError;
    }

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurfView->numSlices == 1);

    /* Backup previous tile status is state. */
    saved = surf->tileStatusDisabled[SurfView->firstSlice];

    /* Do the partial fast clear. */
    status = _ClearTileStatusWindowAligned(SurfView,
                                           ClearArgs,
                                           LayerIndex,
                                           &alignedRect);

    if (gcmIS_SUCCESS(status))
    {
        gctINT i, count;
        gcsRECT rect[4];
        gcsRECT_PTR clearRect;
        gcsSURF_CLEAR_ARGS newArgs;
        gcsSURF_VIEW *rtView = (ClearArgs->flags & gcvCLEAR_COLOR) ? SurfView : gcvNULL;
        gcsSURF_VIEW *dsView = (ClearArgs->flags & gcvCLEAR_COLOR) ? gcvNULL  : SurfView;

        /* Tile status is enabled if success, turn it on. */
        surf->tileStatusDisabled[SurfView->firstSlice] = gcvFALSE;

        surf->dirty[SurfView->firstSlice] = gcvTRUE;

        /* Reset the tile status. */
        gcmONERROR(gcoSURF_EnableTileStatus(SurfView));

        if (saved)
        {
            /*
             * Invalidate tile status cache.
             * A case is that tile status is decompressed and disabled but still
             * cached. Tile status memory is changed in above clear, we need
             * invalidate tile status cache here.
             * Decompressing by resolve onto self will only read tile status,
             * hardware will drop tile status cache without write back in that
             * case. So here only 'invalidate' is done in 'flush'.
             */
            gcmONERROR(gcoSURF_FlushTileStatus(SurfView, gcvFALSE));
        }

        /* Get not cleared area count. */
        clearRect = ClearArgs->clearRect;
        count = 0;

        if (alignedRect.left > clearRect->left)
        {
            rect[count].left   = clearRect->left;
            rect[count].top    = alignedRect.top;
            rect[count].right  = alignedRect.left;
            rect[count].bottom = alignedRect.bottom;
            count++;
        }

        if (alignedRect.top > clearRect->top)
        {
            rect[count].left   = clearRect->left;
            rect[count].top    = clearRect->top;
            rect[count].right  = clearRect->right;
            rect[count].bottom = alignedRect.top;
            count++;
        }

        if (alignedRect.right < clearRect->right)
        {
            rect[count].left   = alignedRect.right;
            rect[count].top    = alignedRect.top;
            rect[count].right  = clearRect->right;
            rect[count].bottom = alignedRect.bottom;
            count++;
        }

        if (alignedRect.bottom < clearRect->bottom)
        {
            rect[count].left   = clearRect->left;
            rect[count].top    = alignedRect.bottom;
            rect[count].right  = clearRect->right;
            rect[count].bottom = clearRect->bottom;
            count++;
        }

        gcoOS_MemCopy(&newArgs, ClearArgs, gcmSIZEOF(gcsSURF_CLEAR_ARGS));

        for (i = 0; i < count; i++)
        {
            /* Call blit draw to clear. */
            newArgs.clearRect = &rect[i];
            gcmONERROR(gcoHARDWARE_DrawClear(rtView, dsView, &newArgs));
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

static gceSTATUS
_Clear(
    IN gcsSURF_VIEW *SurfView,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctINT32 LayerIndex,
    OUT gctBOOL *BlitDraw
    )
{
    gctBOOL fullSize = gcvFALSE;
    gctPOINTER surfAddr[3] = {gcvNULL};
    gceSTATUS status = gcvSTATUS_OK;

    gcoSURF surf = SurfView->surf;

    gceCHIPMODEL chipModel;
    gctUINT32 chipRevision;
    gcePATCH_ID patchID = gcvPATCH_INVALID;

    gcmHEADER_ARG("SurfView=0x%x ClearArg=0x%x LayerIndex=%d",
                   SurfView, ClearArgs, LayerIndex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(surf, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(ClearArgs != gcvNULL);

    gcoHAL_GetPatchID(gcvNULL, &patchID);
    gcoHAL_QueryChipIdentity(gcvNULL, &chipModel, &chipRevision, gcvNULL, gcvNULL);

    /* Lock the surface. */
    gcmONERROR(gcoSURF_Lock(surf, gcvNULL, surfAddr));

    if (ClearArgs->flags & gcvCLEAR_WITH_CPU_ONLY)
    {
        /* Software clear only. */
        status = _ClearRect(SurfView, ClearArgs, LayerIndex);
    }
    else
    {
        do
        {
            gceCLEAR savedFlags;
            gcsRECT_PTR rect = ClearArgs->clearRect;

            /* Test for entire surface clear. */
            if ((rect->left == 0) &&
                (rect->top == 0) &&
                (rect->right  >= (gctINT)surf->requestW) &&
                (rect->bottom >= (gctINT)surf->requestH))
            {
                fullSize = gcvTRUE;
                /* 1. Full fast clear when it is an entire surface clear. */
                status = _ClearTileStatus(SurfView, ClearArgs, LayerIndex);
            }
#if gcdPARTIAL_FAST_CLEAR
            else
            {
                /* 2. Partial fast clear + 3D draw clear.
                 * Clear tile status window and then draw clear for not aligned parts.
                 */
                status = _PartialFastClear(SurfView, ClearArgs, LayerIndex);
            }
#endif

            if (gcmIS_SUCCESS(status))
            {
                /* Done. */
                break;
            }

            /* 3. Try resolve clear if tile status is disabled.
             * resolve clear is better than below draw clear when no tile status.
            */
            savedFlags = ClearArgs->flags;
            ClearArgs->flags |= gcvCLEAR_WITH_GPU_ONLY;

            status = _ClearRect(SurfView, ClearArgs, LayerIndex);

            ClearArgs->flags = savedFlags;

            if (gcmIS_SUCCESS(status))
            {
                break;
            }

            if ((gcoSURF_IsRenderable(surf) == gcvSTATUS_OK) &&
                (!(patchID == gcvPATCH_SKIA_SKQP && chipModel == gcv600 && chipRevision == 0x4653)))
            {
                /* 4. Try 3D draw clear.
                 * Resolve clear will need to decompress and disable tile status
                 * before the actual clear. So 3D draw clear is better if tile
                 * status is currently enabled.
                 *
                 * Another thing is that when draw clear succeeds, all layers of the
                 * surface are cleared at the same time.
                 */

                gcsSURF_VIEW *rtView = (ClearArgs->flags & gcvCLEAR_COLOR) ? SurfView : gcvNULL;
                gcsSURF_VIEW *dsView = (ClearArgs->flags & gcvCLEAR_COLOR) ? gcvNULL  : SurfView;

                status = gcoHARDWARE_DrawClear(rtView, dsView, ClearArgs);

                if (gcmIS_SUCCESS(status))
                {
                    /* Report the flag to caller, which means all layers of the surface are cleared. */
                    *BlitDraw = gcvTRUE;
                    break;
                }
            }

            /* 5. Last, use software clear.
             *    If no GPU-ONLY requested, try software clear.
             */
            if (!(ClearArgs->flags & gcvCLEAR_WITH_GPU_ONLY))
            {
                savedFlags = ClearArgs->flags;
                ClearArgs->flags |= gcvCLEAR_WITH_CPU_ONLY;

                status = _ClearRect(SurfView, ClearArgs, LayerIndex);

                ClearArgs->flags = savedFlags;
            }
        }
        while (gcvFALSE);
    }

    if ((ClearArgs->flags & gcvCLEAR_STENCIL) && surf->hasStencilComponent)
    {
        surf->canDropStencilPlane = gcvFALSE;
    }

    if (gcmIS_SUCCESS(status) && fullSize)
    {
        /* Set garbagePadded=0 if full clear */
        if (surf->paddingFormat)
        {
            surf->garbagePadded = gcvFALSE;
        }

        if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_PE_DITHER_FIX) == gcvFALSE)
        {
            gctUINT8 clearMask = surf->clearMask[LayerIndex];

            /* Full mask clear can reset the deferDither3D flag */
            if ((clearMask == 0xF) ||
                (clearMask == 0x7 && (surf->format == gcvSURF_X8R8G8B8 || surf->format == gcvSURF_R5G6B5)) ||
                (clearMask == 0xE && surf->hasStencilComponent && surf->canDropStencilPlane)
               )
            {
                surf->deferDither3D = gcvFALSE;
            }
        }
    }

OnError:
    if (surfAddr[0])
    {
        gcmVERIFY_OK(gcoSURF_Unlock(surf, gcvNULL));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_3DBlitClearTileStatus(
    IN gcsSURF_VIEW *SurfView,
    IN gctBOOL ClearAsDirty
    )
{
    static gctBOOL sDirty = gcvFALSE;
    const gctSIZE_T cMaxWidth = 1 << 16;

    gcoSURF surf = SurfView->surf;
    gctSIZE_T bytes = 0;
    gctUINT32 fillColor = 0;
    gcsPOINT origin = {0};
    gcsPOINT rectSize;
    struct _gcoSURF tsSurf;
    gcsSURF_VIEW tsView;
    gcs3DBLIT_INFO clearInfo = {0};
    gcsSURF_FORMAT_INFO_PTR formatInfo;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("SurfView=0x%x ClearAsDirty=%d", SurfView, ClearAsDirty);

    /* Query the tile status size. */
    gcmONERROR(
        gcoHARDWARE_QueryTileStatus(gcvNULL,
                                    surf,
                                    surf->size,
                                    &bytes,
                                    gcvNULL,
                                    &fillColor));

    if (ClearAsDirty)
    {
        fillColor = 0;
    }

    /* Compute clear window for tile status */
    rectSize.x = (gctINT32)bytes >> 2;
    rectSize.y = 1;
    while ((rectSize.x >= (gctINT32)cMaxWidth) && ((rectSize.x & 0x1) == 0))
    {
        rectSize.x >>= 1;
        rectSize.y <<= 1;
    }

    if ((rectSize.x >= (gctINT32)cMaxWidth) || (rectSize.x * rectSize.y * 4 != (gctINT32)bytes))
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    gcmONERROR(gcoSURF_QueryFormat(gcvSURF_A8R8G8B8, &formatInfo));

    gcoOS_ZeroMemory(&tsSurf, sizeof(tsSurf));
    tsSurf.requestW = tsSurf.allocedW = tsSurf.alignedW = rectSize.x;
    tsSurf.requestH = tsSurf.allocedH = tsSurf.alignedH = rectSize.y;
    tsSurf.tiling = gcvLINEAR;
    tsSurf.sampleInfo = g_sampleInfos[1];
    tsSurf.isMsaa = gcvFALSE;
    tsSurf.format = formatInfo->format;
    tsSurf.formatInfo = *formatInfo;
    tsSurf.bitsPerPixel = tsSurf.formatInfo.bitsPerPixel;
    tsSurf.stride = tsSurf.alignedW * tsSurf.bitsPerPixel / 8;
    tsSurf.cacheMode = gcvCACHE_NONE;
    tsSurf.node = surf->tileStatusNode;
    tsSurf.dirty = &sDirty;

    tsView.surf = &tsSurf;
    tsView.firstSlice = 0;
    tsView.numSlices = 1;

    clearInfo.clearValue[0] = fillColor;  /* dirtied */
    clearInfo.clearBitMask  = _ByteMaskToBitMask(0xF);
    gcmGETHARDWAREADDRESS(surf->tileStatusNode, clearInfo.destAddress);
    clearInfo.destAddress += SurfView->firstSlice * surf->tileStatusSliceSize;

    gcmONERROR(gcoHARDWARE_3DBlitClear(gcvNULL, gcvENGINE_RENDER, &tsView, &clearInfo, &origin, &rectSize, gcvFALSE));

OnError:
    gcmFOOTER();
    return status;
}

static gceSTATUS
_3DBlitClearRect(
    IN gcsSURF_VIEW *SurfView,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs,
    IN gctUINT32 LayerIndex
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsPOINT origin, rectSize;
    gcs3DBLIT_INFO clearInfo = {0};
    gcs3DBLIT_INFO hzClearInfo = {0};
    gctBOOL previousFC = gcvFALSE;
    gctBOOL enableFC = gcvFALSE;    /* FALSE means don't change the previous FC status */
    gctBOOL dirtyTS = gcvFALSE;
    gcsRECT_PTR rect = ClearArgs->clearRect;

    gcoSURF surf = SurfView->surf;

    gctBOOL clearHZ = ((ClearArgs->flags & gcvCLEAR_DEPTH) && surf->hzNode.pool != gcvPOOL_UNKNOWN);

    gcmHEADER_ARG("SurfView=0x%x ClearArg=0x%x LayerIndex=%d",
                   SurfView, ClearArgs, LayerIndex);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurfView->numSlices == 1);

    origin.x = ClearArgs->clearRect->left;
    origin.y = ClearArgs->clearRect->top;
    rectSize.x = ClearArgs->clearRect->right - ClearArgs->clearRect->left;
    rectSize.y = ClearArgs->clearRect->bottom - ClearArgs->clearRect->top;

    if ((ClearArgs->flags & gcvCLEAR_STENCIL) && surf->hasStencilComponent)
    {
        surf->canDropStencilPlane = gcvFALSE;
    }

    /* Compute clear values. */
    gcmONERROR(_ComputeClear(surf, ClearArgs, LayerIndex));

    previousFC = !surf->tileStatusDisabled[SurfView->firstSlice];

    /* Prepare clearInfo. */
    clearInfo.clearValue[0] = surf->clearValue[LayerIndex];
    clearInfo.clearValue[1] = surf->clearValueUpper[LayerIndex];
    clearInfo.clearBitMask      = surf->clearBitMask[LayerIndex];
    clearInfo.clearBitMaskUpper = surf->clearBitMaskUpper[LayerIndex];
    gcmGETHARDWAREADDRESS(surf->node, clearInfo.destAddress);
    clearInfo.destAddress += LayerIndex * surf->layerSize
                          +  SurfView->firstSlice * surf->sliceSize;
    gcmGETHARDWAREADDRESS(surf->tileStatusNode, clearInfo.destTileStatusAddress);
    clearInfo.destTileStatusAddress += SurfView->firstSlice * surf->tileStatusSliceSize;
    clearInfo.origin = &origin;
    clearInfo.rect = &rectSize;

    if ((clearInfo.clearBitMask == 0)
    &&  (clearInfo.clearBitMaskUpper == 0)
    )
    {
        gcmFOOTER();
        return gcvSTATUS_OK;
    }

    /* Test for entire surface clear. */
    if ((rect->left == 0)  &&  (rect->top == 0)
    &&  (rect->right  >= (gctINT)surf->requestW)
    &&  (rect->bottom >= (gctINT)surf->requestH)
    &&  (clearInfo.clearBitMask == 0xFFFFFFFF)
    &&  (clearInfo.clearBitMaskUpper == 0xFFFFFFFF)
    &&  (clearInfo.destTileStatusAddress != 0)
    )
    {
        /* Can enable FC for full clear*/
        enableFC = gcvTRUE;
        clearInfo.fcClearValue[0] = clearInfo.clearValue[0];
        clearInfo.fcClearValue[1] = clearInfo.clearValue[1];
    }
    else
    {
        /* For partial full-mask clear, can eable fast clear if previously disabled */
        if (!previousFC
        &&  (clearInfo.clearBitMask == 0xFFFFFFFF)
        &&  (clearInfo.clearBitMaskUpper == 0xFFFFFFFF)
        &&  (clearInfo.destTileStatusAddress != 0)
        )
        {
            enableFC = gcvTRUE;
            dirtyTS  = gcvTRUE;
            clearInfo.fcClearValue[0] = clearInfo.clearValue[0];
            clearInfo.fcClearValue[1] = clearInfo.clearValue[1];
        }
        else
        {
            clearInfo.fcClearValue[0] = surf->fcValue[SurfView->firstSlice];
            clearInfo.fcClearValue[1] = surf->fcValueUpper[SurfView->firstSlice];
        }
    }

    if (clearHZ)
    {
       /* we don't expect to support HZ anymore in hardware with 3dblit for now */
        gcmASSERT(0);
        /* Prepare hzClearInfo. */
        hzClearInfo.clearValue[0] = surf->clearValueHz;
        hzClearInfo.clearValue[1] = surf->clearValueHz;
        hzClearInfo.fcClearValue[0] = hzClearInfo.clearValue[0];
        hzClearInfo.fcClearValue[1] = hzClearInfo.clearValue[1];
        hzClearInfo.clearBitMask    = _ByteMaskToBitMask(0xF);
        gcmGETHARDWAREADDRESS(surf->hzNode, hzClearInfo.destAddress);
        gcmGETHARDWAREADDRESS(surf->hzTileStatusNode, hzClearInfo.destTileStatusAddress);
        hzClearInfo.origin = &origin;
        hzClearInfo.rect = &rectSize;
    }

    if ((surf->bitsPerPixel == 8) &&
        (surf->isMsaa) &&
        !gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_8bit_256TILE_FC_FIX))
    {
        enableFC = gcvFALSE;
        dirtyTS  = gcvFALSE;

        /* Need to disable FC for this case */
        if (previousFC)
        {
            gcmONERROR(gcoSURF_DisableTileStatus(SurfView, gcvTRUE));
            previousFC = gcvFALSE;
        }
    }

    /* Flush the tile status cache. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, gcvNULL));
    gcmONERROR(gcoHARDWARE_FlushTileStatus(gcvNULL, SurfView, gcvFALSE));
    gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));

    /* Need to clear ts to all dirty for partial fast clear */
    if (dirtyTS)
    {
        if (gcmIS_ERROR(_3DBlitClearTileStatus(SurfView, gcvTRUE)))
        {
            enableFC = gcvFALSE;
        }
    }

    /* If previous fc was disabled, and this time not enable it */
    if (!previousFC && !enableFC)
    {
        clearInfo.destTileStatusAddress = 0;
        hzClearInfo.destTileStatusAddress = 0;
    }

    /* Clear. */
    if ((surf->bitsPerPixel >= 64) &&
        !gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_64bpp_MASKED_CLEAR_FIX) &&
        (surf->tileStatusNode.pool == gcvPOOL_UNKNOWN) &&
        (surf->clearBitMask[LayerIndex] != surf->clearBitMaskUpper[LayerIndex]))
    {
        gcmASSERT(!previousFC && !enableFC);

        status = gcoHARDWARE_ClearSoftware(gcvNULL,
                                           SurfView,
                                           LayerIndex,
                                           rect,
                                           surf->clearValue[LayerIndex],
                                           surf->clearValueUpper[LayerIndex],
                                           ClearArgs->colorMask,
                                           0);
    }
    else
    {
        gcmONERROR(gcoHARDWARE_3DBlitClear(gcvNULL, gcvENGINE_RENDER, SurfView, &clearInfo, &origin, &rectSize, gcvFALSE));
    }

    if (clearHZ)
    {
        /* Clear HZ. */
        gcmONERROR(gcoHARDWARE_3DBlitClear(gcvNULL, gcvENGINE_RENDER, SurfView, &hzClearInfo, &origin, &rectSize, gcvFALSE));
    }

    if (enableFC)
    {
        /* Record FC value. */
        surf->fcValue[SurfView->firstSlice] = clearInfo.fcClearValue[0];
        surf->fcValueUpper[SurfView->firstSlice] = clearInfo.fcClearValue[1];

        if (clearHZ && hzClearInfo.destTileStatusAddress)
        {
            /* Record HZ FC value. */
            surf->fcValueHz = hzClearInfo.clearValue[0];
        }

        /* Turn the tile status on again. */
        surf->tileStatusDisabled[SurfView->firstSlice] = gcvFALSE;

        /* Reset the tile status. */
        gcmONERROR(gcoSURF_EnableTileStatus(SurfView));
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_Clear(
    IN gcsSURF_VIEW *SurfView,
    IN gcsSURF_CLEAR_ARGS_PTR ClearArgs
    )
{
    gctUINT layerIndex;
    gcsRECT adjustRect;
    gcsSURF_CLEAR_ARGS newArgs;
    gcoSURF surf = SurfView->surf;
    gctPOINTER memory[3] = {gcvNULL, gcvNULL, gcvNULL};
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("SurfView=0x%x ClearArgs=0x%x", SurfView, ClearArgs);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(surf, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(ClearArgs != 0);

    gcoOS_MemCopy(&newArgs, ClearArgs, sizeof(gcsSURF_CLEAR_ARGS));
    if (ClearArgs->clearRect == gcvNULL)
    {
        /* Full screen. */
        adjustRect.left = 0;
        adjustRect.top  = 0;
        adjustRect.right  = surf->requestW;
        adjustRect.bottom = surf->requestH;
    }
    else
    {
        /* Intersect with surface size. */
        adjustRect.left   = gcmMAX(0, ClearArgs->clearRect->left);
        adjustRect.top    = gcmMAX(0, ClearArgs->clearRect->top);
        adjustRect.right  = gcmMIN((gctINT)surf->requestW, ClearArgs->clearRect->right);
        adjustRect.bottom = gcmMIN((gctINT)surf->requestH, ClearArgs->clearRect->bottom);

        /* Skip clear for invalid rect */
        if ((adjustRect.left >= adjustRect.right) || (adjustRect.top >= adjustRect.bottom))
        {
            goto OnError;
        }
    }
    newArgs.clearRect = &adjustRect;

    /* Lock the surface. */
    gcmONERROR(gcoSURF_Lock(surf, gcvNULL, memory));

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE))
    {
        gcsSURF_VIEW tmpView;

        if (ClearArgs->clearRect == gcvNULL)
        {
            surf->dither3D = gcvFALSE;
        }

        tmpView.surf = surf;
        tmpView.numSlices = 1;
        for (layerIndex = 0; layerIndex < surf->formatInfo.layers; layerIndex++)
        {
            if (ClearArgs->flags & gcvCLEAR_MULTI_SLICES)
            {
                gctUINT i;
                for (i = 0; i < surf->requestD; i++)
                {
                    tmpView.firstSlice = i;
                    gcmONERROR(_3DBlitClearRect(&tmpView, &newArgs, layerIndex));
                }
            }
            else
            {
                gcmONERROR(_3DBlitClearRect(SurfView, &newArgs, layerIndex));
            }
        }
    }
    else
    {
#if gcdENABLE_KERNEL_FENCE
        gcoHARDWARE_SetHWSlot(gcvNULL, gcvENGINE_RENDER, gcvHWSLOT_BLT_DST, surf->node.u.normal.node, 0);
#endif
        for (layerIndex = 0; layerIndex < surf->formatInfo.layers; layerIndex++)
        {
            gctBOOL blitDraw = gcvFALSE;

            if (ClearArgs->flags & gcvCLEAR_MULTI_SLICES)
            {
                gctUINT i;
                gctUINT sliceIndex = SurfView->firstSlice;

                for (i = 0; i < surf->requestD; i++)
                {
                    SurfView->firstSlice = i;
                    status = _Clear(SurfView, &newArgs, layerIndex, &blitDraw);

                    if (status == gcvSTATUS_NOT_ALIGNED)
                    {
                        SurfView->firstSlice = sliceIndex;
                        goto OnError;
                    }
                }

                SurfView->firstSlice = sliceIndex;
            }
            else
            {
                status = _Clear(SurfView, &newArgs, layerIndex, &blitDraw);

                if (status == gcvSTATUS_NOT_ALIGNED)
                {
                    goto OnError;
                }
            }

            gcmONERROR(status);

            if (blitDraw)
            {
                break;
            }
        }
    }

OnError:
    if (memory[0] != gcvNULL)
    {
        /* Unlock. */
        gcmVERIFY_OK(gcoSURF_Unlock(surf, memory[0]));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#define gcdCOLOR_SPACE_CONVERSION_NONE         0
#define gcdCOLOR_SPACE_CONVERSION_TO_LINEAR    1
#define gcdCOLOR_SPACE_CONVERSION_TO_NONLINEAR 2

gceSTATUS
gcoSURF_MixSurfacesCPU(
    IN gcoSURF TargetSurface,
    IN gctUINT TargetSliceIndex,
    IN gcoSURF *SourceSurface,
    IN gctUINT *SourceSliceIndices,
    IN gctFLOAT *Weights,
    IN gctINT Count
    )
{
    gceSTATUS status;
    gcoSURF srcSurf = gcvNULL, dstSurf = gcvNULL;
    gctPOINTER srcAddr[3] = {gcvNULL};
    gctPOINTER dstAddr[3] = {gcvNULL};
    _PFNreadPixel pfReadPixel = gcvNULL;
    _PFNwritePixel pfWritePixel = gcvNULL;
    gctUINT defaultSliceIndices[MAX_SURF_MIX_SRC_NUM] = { 0 };

    gctUINT i, j, k;
    gcsSURF_FORMAT_INFO *srcFmtInfo, *dstFmtInfo;
    gctPOINTER srcAddr_l[gcdMAX_SURF_LAYERS];
    gctPOINTER dstAddr_l[gcdMAX_SURF_LAYERS];
    gcsPIXEL internalSrc, internalDst;

    gcsSURF_VIEW srcView = {gcvNULL, 0, 1};
    gcsSURF_VIEW dstView = {TargetSurface, TargetSliceIndex, 1};

    dstSurf = TargetSurface;
    dstFmtInfo = &dstSurf->formatInfo;

    /* MSAA case not supported. */
    if (dstSurf->sampleInfo.product > 1)
    {
        return gcvSTATUS_NOT_SUPPORTED;
    }

    for (k = 0; k < (gctUINT)Count; ++k)
    {
        srcSurf = SourceSurface[k];
        srcFmtInfo = &SourceSurface[k]->formatInfo;

        /* Target and Source surfaces need to have same dimensions and format. */
        if ((dstSurf->requestW != srcSurf->requestW)
         || (dstSurf->requestH != srcSurf->requestH)
         || (dstSurf->format != srcSurf->format)
         || (dstSurf->type != srcSurf->type)
         || (dstSurf->tiling != srcSurf->tiling))
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        /* MSAA case not supported. */
        if (srcSurf->sampleInfo.product > 1)
        {
            return gcvSTATUS_NOT_SUPPORTED;
        }

        /*
        ** Integer format upload/blit, the data type must be totally matched.
        */
        if (((srcFmtInfo->fmtDataType == gcvFORMAT_DATATYPE_UNSIGNED_INTEGER) ||
             (srcFmtInfo->fmtDataType == gcvFORMAT_DATATYPE_SIGNED_INTEGER)) &&
             (srcFmtInfo->fmtDataType != dstFmtInfo->fmtDataType))
        {
            return gcvSTATUS_NOT_SUPPORTED;
        }

    }

    if ( SourceSliceIndices == gcvNULL )
    {
        SourceSliceIndices = defaultSliceIndices;
    }

    pfReadPixel  = gcoSURF_GetReadPixelFunc(srcSurf);
    pfWritePixel = gcoSURF_GetWritePixelFunc(dstSurf);

    if (!pfReadPixel || !pfWritePixel)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));
    gcmONERROR(gcoHARDWARE_Commit(gcvNULL));
    gcmONERROR(gcoHARDWARE_Stall(gcvNULL));

    gcmONERROR(gcoHARDWARE_DisableTileStatus(gcvNULL, &dstView, gcvTRUE));
    gcmONERROR(gcoSURF_Lock(dstSurf, gcvNULL, dstAddr));
    gcmONERROR(gcoSURF_NODE_Cache(&dstSurf->node,
                                  dstAddr[0],
                                  dstSurf->size,
                                  gcvCACHE_INVALIDATE));

    for (k = 0; k < (gctUINT)Count; ++k)
    {
        srcSurf = SourceSurface[k];
        srcFmtInfo = &SourceSurface[k]->formatInfo;

        /* set color space conversion flag */
        gcmASSERT(srcSurf->colorSpace == dstSurf->colorSpace);

        srcView.surf = srcSurf;
        srcView.firstSlice = SourceSliceIndices[k];
        srcView.numSlices = 1;

        /* Flush the GPU cache */
        gcmONERROR(gcoHARDWARE_FlushTileStatus(gcvNULL, &srcView, gcvTRUE));
        /* Lock the surfaces. */
        gcmONERROR(gcoSURF_Lock(srcSurf, gcvNULL, srcAddr));
        gcmONERROR(gcoSURF_NODE_Cache(&srcSurf->node,
                                      srcAddr[0],
                                      srcSurf->size,
                                      gcvCACHE_INVALIDATE));
    }

    for (j = 0; j < srcSurf->requestH; ++j)
    {
        for (i = 0; i < srcSurf->requestW; ++i)
        {
            internalDst.color.f.r = 0;
            internalDst.color.f.g = 0;
            internalDst.color.f.b = 0;
            internalDst.color.f.a = 0;
            internalDst.d = 0;
            internalDst.s = 0;

            for (k = 0; k < (gctUINT)Count; ++k)
            {
                gctFLOAT    factor = Weights[k];
                gctUINT     z = SourceSliceIndices[k];

                srcSurf = SourceSurface[k];
                srcFmtInfo = &SourceSurface[k]->formatInfo;

                internalSrc.color.f.r = 0;
                internalSrc.color.f.g = 0;
                internalSrc.color.f.b = 0;
                internalSrc.color.f.a = 0;
                internalSrc.d = 0;
                internalSrc.s = 0;

                srcSurf->pfGetAddr(srcSurf, (gctSIZE_T)i, (gctSIZE_T)j, (gctSIZE_T)z, srcAddr_l);
                pfReadPixel(srcAddr_l, &internalSrc);

                if (srcSurf->colorSpace == gcvSURF_COLOR_SPACE_NONLINEAR)
                {
                    gcoSURF_PixelToLinear(&internalSrc);
                }

                /* Mix the pixels. */
                internalDst.color.f.r += internalSrc.color.f.r * factor;
                internalDst.color.f.g += internalSrc.color.f.g * factor;
                internalDst.color.f.b += internalSrc.color.f.b * factor;
                internalDst.color.f.a += internalSrc.color.f.a * factor;
                internalDst.d += internalSrc.d * factor;
                internalDst.s += (gctUINT32)(internalSrc.s * factor);

            }

            if (srcSurf->colorSpace == gcvSURF_COLOR_SPACE_NONLINEAR)
            {
                gcoSURF_PixelToNonLinear(&internalDst);
            }

            dstSurf->pfGetAddr(dstSurf, (gctSIZE_T)i, (gctSIZE_T)j, (gctSIZE_T)TargetSliceIndex, dstAddr_l);
            pfWritePixel(&internalDst, dstAddr_l, 0);

        }
    }

    /* Dst surface was written by CPU and might be accessed by GPU later */
    gcmONERROR(gcoSURF_NODE_Cache(&dstSurf->node,
                                  dstAddr[0],
                                  dstSurf->size,
                                  gcvCACHE_CLEAN));

#if gcdDUMP
        gcmDUMP(gcvNULL, "#[info: verify mix surface source]");
        /* verify the source */
        gcmDUMP_BUFFER(gcvNULL,
                       gcvDUMP_BUFFER_VERIFY,
                       gcsSURF_NODE_GetHWAddress(&srcSurf->node),
                       srcSurf->node.logical,
                       0,
                       srcSurf->size);
        /* upload the destination */
        gcmDUMP_BUFFER(gcvNULL,
                       gcvDUMP_BUFFER_MEMORY,
                       gcsSURF_NODE_GetHWAddress(&dstSurf->node),
                       dstSurf->node.logical,
                       0,
                       dstSurf->size);
#endif

OnError:
    /* Unlock the surfaces. */
    gcoSURF_Unlock(dstSurf, gcvNULL);

    for ( k = 0; k < (gctUINT)Count; ++k )
    {
        srcSurf = SourceSurface[k];

        /* Lock the surfaces. */
        gcoSURF_Unlock(srcSurf, gcvNULL);
    }

    return gcvSTATUS_OK;
}

gceSTATUS
gcoSURF_Resample(
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf
    )
{
    gctUINT i;
    gcsSURF_RESOLVE_ARGS rlvArgs = {0};
    gcsSURF_VIEW srcView, dstView;
    gcoSURF tmpSurf = gcvNULL;
    gcsPOINT rectSize = {0, 0};
    gcsSAMPLES srcSampleInfo = {1, 1, 1};
    gcsSAMPLES dstSampleInfo = {1, 1, 1};
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("SrcSurf=0x%x DstSurf=0x%x", SrcSurf, DstSurf);

    /* Validate the surfaces. */
    gcmVERIFY_OBJECT(SrcSurf, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(DstSurf, gcvOBJ_SURF);

    /* Both surfaces have to be non-multisampled. */
    if (SrcSurf->isMsaa || DstSurf->isMsaa)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE))
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }


    /* Determine the samples and fill in coordinates. */
    if (SrcSurf->requestW == DstSurf->requestW)
    {
        srcSampleInfo.x = 1;
        dstSampleInfo.x = 1;
        rectSize.x      = SrcSurf->requestW;
    }
    else if (SrcSurf->requestW / 2 == DstSurf->requestW)
    {
        srcSampleInfo.x = 2;
        dstSampleInfo.x = 1;
        rectSize.x      = DstSurf->requestW;
    }
    else if (SrcSurf->requestW == DstSurf->requestW / 2)
    {
        srcSampleInfo.x = 1;
        dstSampleInfo.x = 2;
        rectSize.x      = SrcSurf->requestW;
    }
    else
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if (SrcSurf->requestH == DstSurf->requestH)
    {
        srcSampleInfo.y = 1;
        dstSampleInfo.y = 1;
        rectSize.y      = SrcSurf->requestH;
    }
    else if (SrcSurf->requestH / 2 == DstSurf->requestH)
    {
        srcSampleInfo.y = 2;
        dstSampleInfo.y = 1;
        rectSize.y      = DstSurf->requestH;
    }
    else if (SrcSurf->requestH == DstSurf->requestH / 2)
    {
        srcSampleInfo.y = 1;
        dstSampleInfo.y = 2;
        rectSize.y      = SrcSurf->requestH;
    }
    else
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }
    srcSampleInfo.product = srcSampleInfo.x * srcSampleInfo.y;
    dstSampleInfo.product = dstSampleInfo.x * dstSampleInfo.y;

    /* Overwrite surface samples.
    ** Note: isMsaa flag was still kept as 0, to indicate it's not real msaa buf and no need to set cache256 down
    */
    SrcSurf->sampleInfo = srcSampleInfo;
    DstSurf->sampleInfo = dstSampleInfo;

    srcView.surf = SrcSurf;
    srcView.numSlices = 1;
    dstView.surf = DstSurf;
    dstView.numSlices = 1;

    gcoOS_ZeroMemory(&rlvArgs, gcmSIZEOF(rlvArgs));
    rlvArgs.version = gcvHAL_ARG_VERSION_V2;
    rlvArgs.uArgs.v2.resample  = gcvTRUE;
    rlvArgs.uArgs.v2.rectSize  = rectSize;
    rlvArgs.uArgs.v2.numSlices = 1;
    /* For resample, we only try HW patch.*/
    rlvArgs.uArgs.v2.gpuOnly = gcvTRUE;

    /* 3D texture. */
    if (SrcSurf->requestD != DstSurf->requestD)
    {
        gcsSURF_VIEW tmpView;
        gcoSURF mixSrcSurfs[2];
        gctUINT mixSrcSliceIndices[2] = {0};
        gctFLOAT mixSrcWeights[2] = {0.5f, 0.5f};

        /* Need to be reducing depth, with resample. */
        if (SrcSurf->requestD < DstSurf->requestD)
        {
            gcmONERROR(gcvSTATUS_INVALID_REQUEST);
        }

        gcoSURF_Construct(gcvNULL,
                          DstSurf->requestW,
                          DstSurf->requestH,
                          1,
                          DstSurf->type,
                          DstSurf->format,
                          gcvPOOL_DEFAULT,
                          &tmpSurf);

        tmpView.surf = tmpSurf;
        tmpView.firstSlice = 0;
        tmpView.numSlices = 1;

        mixSrcSurfs[0] = DstSurf;
        mixSrcSurfs[1] = tmpSurf;

        /* Downsample 2 slices, and mix them together.
        ** Ignore the last Src slice, if the size is odd.
        */
        for (i = 0; i < DstSurf->requestD; ++i)
        {
            dstView.firstSlice = i;
            mixSrcSliceIndices[0] = dstView.firstSlice;

            srcView.firstSlice = 2 * i;
            gcmONERROR(gcoSURF_ResolveRect(&srcView, &dstView, &rlvArgs));

            srcView.firstSlice = 2 * i + 1;
            gcmONERROR(gcoSURF_ResolveRect(&srcView, &tmpView, &rlvArgs));

            gcmONERROR(gcoSURF_MixSurfacesCPU(DstSurf, dstView.firstSlice,
                                              mixSrcSurfs, mixSrcSliceIndices,
                                              mixSrcWeights, 2));
        }
    }
    else
    {
        for (i = 0; i < SrcSurf->requestD; ++i)
        {
            srcView.firstSlice = dstView.firstSlice = i;
            gcmONERROR(gcoSURF_ResolveRect(&srcView, &dstView, &rlvArgs));
        }
    }

OnError:
    /* Restore samples. */
    SrcSurf->sampleInfo = g_sampleInfos[1];
    DstSurf->sampleInfo = g_sampleInfos[1];

    if (tmpSurf != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Destroy(tmpSurf));
    }

    /* Fallback to CPU resampling */
    if (gcmIS_ERROR(status))
    {
        gcsSURF_BLIT_ARGS blitArgs;

#if !gcdDUMP
        gcePATCH_ID patchID = gcvPATCH_INVALID;
        gcoHAL_GetPatchID(gcvNULL, &patchID);
        if (patchID != gcvPATCH_GFXBENCH)
#endif
        {
            gcoOS_ZeroMemory(&blitArgs, sizeof(blitArgs));
            blitArgs.srcX               = 0;
            blitArgs.srcY               = 0;
            blitArgs.srcZ               = 0;
            blitArgs.srcWidth           = SrcSurf->requestW;
            blitArgs.srcHeight          = SrcSurf->requestH;
            blitArgs.srcDepth           = SrcSurf->requestD;
            blitArgs.srcSurface         = SrcSurf;
            blitArgs.dstX               = 0;
            blitArgs.dstY               = 0;
            blitArgs.dstZ               = 0;
            blitArgs.dstWidth           = DstSurf->requestW;
            blitArgs.dstHeight          = DstSurf->requestH;
            blitArgs.dstDepth           = DstSurf->requestD;
            blitArgs.dstSurface         = DstSurf;
            blitArgs.xReverse           = gcvFALSE;
            blitArgs.yReverse           = gcvFALSE;
            blitArgs.scissorTest        = gcvFALSE;
            blitArgs.srcNumSlice        = 1;
            blitArgs.dstNumSlice        = 1;
            status = gcoSURF_BlitCPU(&blitArgs);
        }
#if !gcdDUMP
        else
        {
            /* Skip SW GENERATION for low levels */
        }
#endif
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_GetResolveAlignment
**
**  Query the resolve alignment of the hardware.
**
**  INPUT:
**
**      gcoSURF Surface,
**          Pointer to an surface object.
**
**  OUTPUT:
**
**      gctUINT *originX,
**          X direction origin alignment
**
**      gctUINT *originY
**          Y direction origin alignemnt
**
**      gctUINT *sizeX,
**          X direction size alignment
**
**      gctUINT *sizeY
**          Y direction size alignemnt
**
*/
gceSTATUS
gcoSURF_GetResolveAlignment(
    IN gcoSURF Surface,
    OUT gctUINT *originX,
    OUT gctUINT *originY,
    OUT gctUINT *sizeX,
    OUT gctUINT *sizeY
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Query the tile sizes through gcoHARDWARE. */
    status = gcoHARDWARE_GetSurfaceResolveAlignment(gcvNULL, Surface, originX, originY, sizeX, sizeY);

    gcmFOOTER_ARG("status=%d *originX=%d *originY=%d *sizeX=%d *sizeY=%d",
                  status, gcmOPT_VALUE(originX), gcmOPT_VALUE(originY),
                  gcmOPT_VALUE(sizeX), gcmOPT_VALUE(sizeY));
    return status;
}


gceSTATUS gcoSURF_TranslateRotationRect(
    gcsSIZE_PTR rtSize,
    gceSURF_ROTATION rotation,
    gcsRECT * rect
    )
{
    gctFLOAT tx, ty, tz, tw;
    gctFLOAT tmp;

    tx  = (gctFLOAT)rect->left;
    ty  = (gctFLOAT)rect->top;

    tz  = (gctFLOAT)rect->right;
    tw  = (gctFLOAT)rect->bottom;

    /* 1, translate to rt center */
    tx = tx - (gctFLOAT)rtSize->width / 2.0f;
    ty = ty - (gctFLOAT)rtSize->height / 2.0f;

    tz = tz - (gctFLOAT)rtSize->width / 2.0f;
    tw = tw - (gctFLOAT)rtSize->height / 2.0f;

    /* cos? -sin?    90D  x = -y  180D x = -x 270D x = y
       sin? cos?          y = x        y = -y      y = -x */

    switch (rotation)
    {
        case gcvSURF_90_DEGREE:
            /* 2, rotate */
            tmp = tx;
            tx  = -ty;
            ty  = tmp;

            tmp = tz;
            tz  = -tw;
            tw  = tmp;
            /* 3, translate back  */
            tx = tx + (gctFLOAT)rtSize->height / 2.0f;
            ty = ty + (gctFLOAT)rtSize->width / 2.0f;

            tz = tz + (gctFLOAT)rtSize->height / 2.0f;
            tw = tw + (gctFLOAT)rtSize->width / 2.0f;

            /* Form the new (left,top) (right,bottom) */
            rect->left   = (gctINT32)tz;
            rect->top    = (gctINT32)ty;
            rect->right  = (gctINT32)tx;
            rect->bottom = (gctINT32)tw;
            break;

        case gcvSURF_270_DEGREE:
            /* 2, rotate */
            tmp = tx;
            tx  = ty;
            ty  = -tmp;

            tmp = tz;
            tz  = tw;
            tw  = -tmp;
            /* 3, translate back  */
            tx = tx + (gctFLOAT)rtSize->height / 2.0f;
            ty = ty + (gctFLOAT)rtSize->width / 2.0f;

            tz = tz + (gctFLOAT)rtSize->height / 2.0f;
            tw = tw + (gctFLOAT)rtSize->width / 2.0f;

            /* Form the new (left,top) (right,bottom) */
            rect->left   = (gctINT32)tx;
            rect->top    = (gctINT32)tw;
            rect->right  = (gctINT32)tz;
            rect->bottom = (gctINT32)ty;
            break;

         default :
            break;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_3DBlitBltRect(
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL dstEnableTS = gcvFALSE;

    gcmHEADER_ARG("SrcView=0x%x DstView=0x%x Args=0x%x", SrcView, DstView, Args);

    if (Args->version != gcvHAL_ARG_VERSION_V2)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    gcmONERROR(gcoHARDWARE_FlushTileStatus(gcvNULL, SrcView, gcvFALSE));

    gcmAnyTileStatusEnableForMultiSlice(DstView, &dstEnableTS);

    if (dstEnableTS)
    {
        gcmONERROR(gcoHARDWARE_DisableTileStatus(gcvNULL, DstView, gcvTRUE));
    }

    gcmONERROR(gcoHARDWARE_3DBlitBlt(gcvNULL, SrcView, DstView, Args, gcvFALSE));

OnError:
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoSURF_ResolveRect(
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args
    )
{
    gctPOINTER srcBase[3] = {gcvNULL};
    gctPOINTER dstBase[3] = {gcvNULL};
    gcoSURF srcSurf = SrcView->surf;
    gcoSURF dstSurf = DstView->surf;
    gcsPOINT_PTR srcOrigin, dstOrigin;
    gceSURF_FORMAT savedSrcFmt = gcvSURF_UNKNOWN;
    gceSURF_COLOR_SPACE savedSrcColorSpace = gcvSURF_COLOR_SPACE_UNKNOWN;
    gcsSURF_FORMAT_INFO savedSrcFmtInfo;
    gceSURF_FORMAT savedDstFmt = gcvSURF_UNKNOWN;
    gceSURF_COLOR_SPACE savedDstColorSpace = gcvSURF_COLOR_SPACE_UNKNOWN;
    gcsSURF_FORMAT_INFO savedDstFmtInfo;
    gcsSURF_RESOLVE_ARGS fullSizeArgs = {0};
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("SrcView=0x%x DstView=0x%x Args=0x%x", SrcView, DstView, Args);

    gcoOS_ZeroMemory(&savedDstFmtInfo, gcmSIZEOF(gcsSURF_FORMAT_INFO));
    gcoOS_ZeroMemory(&savedSrcFmtInfo, gcmSIZEOF(gcsSURF_FORMAT_INFO));

    /* default full size resolve */
    if (!Args)
    {
        fullSizeArgs.version = gcvHAL_ARG_VERSION_V2;
        fullSizeArgs.uArgs.v2.rectSize.x = srcSurf->requestW;
        fullSizeArgs.uArgs.v2.rectSize.y = srcSurf->requestH;
        fullSizeArgs.uArgs.v2.numSlices  = 1;
        Args = &fullSizeArgs;
    }

    if (Args->version != gcvHAL_ARG_VERSION_V2)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (Args->uArgs.v2.directCopy &&
        srcSurf->format != dstSurf->format &&
        !(Args->uArgs.v2.srcCompressed ||
        Args->uArgs.v2.dstCompressed))
    {
        if (Args->uArgs.v2.srcSwizzle &&
            !Args->uArgs.v2.dstSwizzle)
        {
            savedDstFmt = dstSurf->format;
            dstSurf->format = srcSurf->format;
            savedDstColorSpace = dstSurf->colorSpace;
            dstSurf->colorSpace = srcSurf->colorSpace;

            savedDstFmtInfo = dstSurf->formatInfo;
            dstSurf->formatInfo = srcSurf->formatInfo;
        }
        else
        {
            savedSrcFmt = srcSurf->format;
            srcSurf->format = dstSurf->format;
            savedSrcColorSpace = srcSurf->colorSpace;
            srcSurf->colorSpace = dstSurf->colorSpace;

            savedSrcFmtInfo = srcSurf->formatInfo;
            srcSurf->formatInfo = dstSurf->formatInfo;
        }
    }

    srcOrigin = &Args->uArgs.v2.srcOrigin;
    dstOrigin = &Args->uArgs.v2.dstOrigin;

    /* Lock the surfaces. */
    gcmONERROR(gcoSURF_Lock(srcSurf, gcvNULL, srcBase));
    gcmONERROR(gcoSURF_Lock(dstSurf, gcvNULL, dstBase));

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE))
    {
        gcmASSERT(!Args->uArgs.v2.resample);

        Args->uArgs.v2.engine = gcvENGINE_RENDER;

        if ((srcSurf->format == gcvSURF_YV12) ||
            (srcSurf->format == gcvSURF_I420) ||
            (srcSurf->format == gcvSURF_NV12) ||
            (srcSurf->format == gcvSURF_NV21))
        {
            if ((gcvLINEAR == srcSurf->tiling) &&
                (gcvLINEAR != dstSurf->tiling) &&
                (dstSurf->format == gcvSURF_YUY2))
            {
                status = gcoHARDWARE_3DBlit420Tiler(gcvNULL,
                                                    gcvENGINE_RENDER,
                                                    srcSurf,
                                                    dstSurf,
                                                    srcOrigin,
                                                    dstOrigin,
                                                    &Args->uArgs.v2.rectSize);
            }
            else
            {
                status = gcvSTATUS_NOT_SUPPORTED;
            }
        }
        else if (Args->uArgs.v2.srcCompressed ||
                 Args->uArgs.v2.dstCompressed ||
                 gcmIS_ERROR(_3DBlitBltRect(SrcView, DstView, Args)))
        {
            if(!Args->uArgs.v2.gpuOnly && !(Args->uArgs.v2.srcSwizzle^Args->uArgs.v2.dstSwizzle))
            {
                status = gcoSURF_CopyPixels(SrcView, DstView, Args);
            }
            else
            {
                status = gcvSTATUS_NOT_SUPPORTED;
            }
        }
    }
    else
    {
        gcsPOINT_PTR pAdjustRect;
        gcsSURF_RESOLVE_ARGS newArgs = {0};
        gctINT srcMaxW = srcSurf->alignedW - srcOrigin->x;
        gctINT srcMaxH = srcSurf->alignedH - srcOrigin->y;
        gctINT dstMaxW = dstSurf->alignedW - dstOrigin->x;
        gctINT dstMaxH = dstSurf->alignedH - dstOrigin->y;

        dstSurf->canDropStencilPlane = srcSurf->canDropStencilPlane;

        gcmONERROR(gcoHARDWARE_FlushTileStatus(gcvNULL, SrcView, gcvFALSE));

        if (srcSurf->type == gcvSURF_BITMAP)
        {
            /* Flush the CPU cache. Source would've been rendered by the CPU. */
            gcmONERROR(gcoSURF_NODE_Cache(
                &srcSurf->node,
                srcBase[0],
                srcSurf->size,
                gcvCACHE_CLEAN));
        }

        if (dstSurf->type == gcvSURF_BITMAP)
        {
            gcmONERROR(gcoSURF_NODE_Cache(
                &dstSurf->node,
                dstBase[0],
                dstSurf->size,
                gcvCACHE_FLUSH));
        }

        /* Make sure we don't go beyond the src/dst surface. */

        gcoOS_MemCopy(&newArgs, Args, sizeof(newArgs));
        pAdjustRect = &newArgs.uArgs.v2.rectSize;
        if (Args->uArgs.v2.resample)
        {
            gcmASSERT(srcOrigin->x == dstOrigin->x && srcOrigin->y == dstOrigin->y);

            /* Determine the resolve size. */
            if ((dstOrigin->x == 0) &&
                (pAdjustRect->x == (gctINT)dstSurf->requestW) &&
                (dstSurf->alignedW <= srcSurf->alignedW / srcSurf->sampleInfo.x))
            {
                /* take destination aligned size only if source is MSAA-required aligned accordingly. */
                pAdjustRect->x = dstSurf->alignedW;
            }
            else
            {
                pAdjustRect->x = gcmMIN(dstMaxW, (gctINT)(srcSurf->alignedW / srcSurf->sampleInfo.x));
            }

            /* Determine the resolve size. */
            if ((dstOrigin->y == 0) &&
                (pAdjustRect->y == (gctINT)dstSurf->requestH) &&
                (dstSurf->alignedH <= srcSurf->alignedH / srcSurf->sampleInfo.y))
            {
                /* take destination aligned size only if source is MSAA-required aligned accordingly. */
                pAdjustRect->y = dstSurf->alignedH;
            }
            else
            {
                pAdjustRect->y = gcmMIN(dstMaxH, (gctINT)(srcSurf->alignedH / srcSurf->sampleInfo.y));
            }

            gcmASSERT(pAdjustRect->x <= srcMaxW);
            gcmASSERT(pAdjustRect->y <= srcMaxH);
            gcmASSERT(pAdjustRect->x <= dstMaxW);
            gcmASSERT(pAdjustRect->y <= dstMaxH);
        }
        else
        {
            /* Determine the resolve size. */
            if ((dstOrigin->x == 0) && (pAdjustRect->x >= (gctINT)dstSurf->requestW))
            {
                /* Full width resolve, a special case. */
                pAdjustRect->x = dstSurf->alignedW;
            }

            if ((dstOrigin->y == 0) &&
                (pAdjustRect->y >= (gctINT)dstSurf->requestH))
            {
                /* Full height resolve, a special case. */
                pAdjustRect->y = dstSurf->alignedH;
            }

            pAdjustRect->x = gcmMIN(srcMaxW, pAdjustRect->x);
            pAdjustRect->y = gcmMIN(srcMaxH, pAdjustRect->y);
            pAdjustRect->x = gcmMIN(dstMaxW, pAdjustRect->x);
            pAdjustRect->y = gcmMIN(dstMaxH, pAdjustRect->y);
        }

        if (dstSurf->hzNode.valid)
        {
            /* Disable any HZ attached to destination. */
            dstSurf->hzDisabled = gcvTRUE;
        }

        /*
        ** 1, gcoHARDWARE_ResolveRect can't handle multi-layer src/dst.
        ** 2, Fake format, except padding channel ones.
        ** 3, gcoHARDWARE_ResolveRect can't handle non unsigned normalized src/dst
        ** For those cases, just fall back to generic copy pixels path.
        */
        if (((srcSurf->formatInfo.layers > 1)                                            ||
             (dstSurf->formatInfo.layers > 1)                                            ||
             (srcSurf->formatInfo.fakedFormat &&
             /* Faked format, but not paddingFormat with default value padded, go SW */
              !(srcSurf->paddingFormat && !srcSurf->garbagePadded)
             )                                                                           ||
             (dstSurf->formatInfo.fakedFormat && !dstSurf->paddingFormat)                ||
             (srcSurf->formatInfo.fmtDataType != gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED) ||
             (dstSurf->formatInfo.fmtDataType != gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED)
            )                                                                               &&
            ((srcSurf->format != gcvSURF_S8) || (dstSurf->format != gcvSURF_S8))
           )
        {
            if(!Args->uArgs.v2.gpuOnly)
            {
                gcmONERROR(gcoSURF_CopyPixels(SrcView, DstView, Args));
            }
            else
            {
                status = gcvSTATUS_NOT_SUPPORTED;
            }
        }
        /* Special case a resolve from the depth buffer with tile status. */
        else if ((srcSurf->type == gcvSURF_DEPTH)
        &&  (srcSurf->tileStatusNode.pool != gcvPOOL_UNKNOWN)
        )
        {
            /* Resolve a depth buffer. */
            if (gcmIS_ERROR(gcoHARDWARE_ResolveDepth(gcvNULL, SrcView, DstView, &newArgs)))
            {
                if(!Args->uArgs.v2.gpuOnly)
                {
                    gcmONERROR(gcoSURF_CopyPixels(SrcView, DstView, Args));
                }
                else
                {
                    status = gcvSTATUS_NOT_SUPPORTED;
                }
            }
        }
        else
        {
            /* Perform resolve. */
            if (gcmIS_ERROR(gcoHARDWARE_ResolveRect(gcvNULL, SrcView, DstView, &newArgs)))
            {
                if(!Args->uArgs.v2.gpuOnly)
                {
                    gcmONERROR(gcoSURF_CopyPixels(SrcView, DstView, Args));
                }
                else
                {
                    status = gcvSTATUS_NOT_SUPPORTED;
                }
            }
        }

        /* If dst surface was fully overwritten, reset the deferDither3D flag. */
        if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_PE_DITHER_FIX) == gcvFALSE &&
            dstOrigin->x == 0 && pAdjustRect->x >= (gctINT)dstSurf->requestW &&
            dstOrigin->y == 0 && pAdjustRect->y >= (gctINT)dstSurf->requestH)
        {
            dstSurf->deferDither3D = gcvFALSE;
        }
    }

OnError:
    /* Unlock the surfaces. */
    if (dstBase[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(dstSurf, dstBase[0]));
    }

    if (srcBase[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(srcSurf, srcBase[0]));
    }

    if (savedSrcFmt != gcvSURF_UNKNOWN)
    {
        srcSurf->format = savedSrcFmt;
        srcSurf->colorSpace = savedSrcColorSpace;
        srcSurf->formatInfo = savedSrcFmtInfo;
    }

    if (savedDstFmt != gcvSURF_UNKNOWN)
    {
        dstSurf->format = savedDstFmt;
        dstSurf->colorSpace = savedDstColorSpace;
        dstSurf->formatInfo = savedDstFmtInfo;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_Blit_v2(
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args
)
{
    gctUINT slice;
    gceSTATUS status = gcvSTATUS_OK;

    if (Args->version != gcvHAL_ARG_VERSION_V2)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    for (slice = 0; slice < Args->uArgs.v2.numSlices; ++slice)
    {
        if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE))
        {
        }
    }

OnError:
    return status;
}

#endif /* gcdENABLE_3D */


#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoSURF_CopyPixels
**
**  Copy a rectangular area from one surface to another with format conversion.
**
**  INPUT:
**
**      gcsSURF_VIEW *SrcView
**          Pointer to the source surface view.
**
**      gcsSURF_VIEW *DstView
**          Pointer to the destination surface.
**
**      gcsSURF_RESOLVE_ARGS *Args
**          Pointer to the resolve arguments.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_CopyPixels(
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args
    )
{
    gctPOINTER srcBase[3] = {gcvNULL};
    gctPOINTER dstBase[3] = {gcvNULL};
    gcoSURF srcSurf = SrcView->surf;
    gcoSURF dstSurf = DstView->surf;
    gcsPOINT_PTR srcOrigin = &Args->uArgs.v2.srcOrigin;
    gcsPOINT_PTR dstOrigin = &Args->uArgs.v2.dstOrigin;
    gcsPOINT rectSize = Args->uArgs.v2.rectSize;
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL cpuBlt = gcvFALSE;

    gcmHEADER_ARG("SrcView=0x%x DstView=0x%x Args=0x%x", SrcView, DstView, Args);

    if (Args->version != gcvHAL_ARG_VERSION_V2)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Lock the surfaces. */
    gcmONERROR(gcoSURF_Lock(srcSurf, gcvNULL, srcBase));
    gcmONERROR(gcoSURF_Lock(dstSurf, gcvNULL, dstBase));

    do
    {
        /* Limit copy rectangle not out of range */
        if (rectSize.x > (gctINT)srcSurf->allocedW - srcOrigin->x)
        {
            rectSize.x = (gctINT)srcSurf->allocedW - srcOrigin->x;
        }

        if (rectSize.y > (gctINT)srcSurf->allocedH - srcOrigin->y)
        {
            rectSize.y = (gctINT)srcSurf->allocedH - srcOrigin->y;
        }

        if (Args->uArgs.v2.srcCompressed && !Args->uArgs.v2.dstCompressed)
        {
            if (rectSize.x > ((gctINT)dstSurf->allocedW - dstOrigin->x) * (gctINT)srcSurf->formatInfo.blockWidth)
            {
                rectSize.x = ((gctINT)dstSurf->allocedW - dstOrigin->x) * (gctINT)srcSurf->formatInfo.blockWidth;
            }
            if (rectSize.y > ((gctINT)dstSurf->allocedH - dstOrigin->y) * (gctINT)srcSurf->formatInfo.blockHeight)
            {
                rectSize.y = ((gctINT)dstSurf->allocedH - dstOrigin->y) * (gctINT)srcSurf->formatInfo.blockHeight;
            }
        }
        else if (!Args->uArgs.v2.srcCompressed && Args->uArgs.v2.dstCompressed)
        {
            if (rectSize.x > ((gctINT)dstSurf->allocedW - dstOrigin->x) / (gctINT)dstSurf->formatInfo.blockWidth)
            {
                rectSize.x = ((gctINT)dstSurf->allocedW - dstOrigin->x) / (gctINT)dstSurf->formatInfo.blockWidth;
            }
            if (rectSize.y > ((gctINT)dstSurf->allocedH - dstOrigin->y) / (gctINT)dstSurf->formatInfo.blockHeight)
            {
                rectSize.y = ((gctINT)dstSurf->allocedH - dstOrigin->y) / (gctINT)dstSurf->formatInfo.blockHeight;
            }
        }
        else
        {
            if (rectSize.x > (gctINT)dstSurf->allocedW - dstOrigin->x)
            {
                rectSize.x = (gctINT)dstSurf->allocedW - dstOrigin->x;
            }
            if (rectSize.y > (gctINT)dstSurf->allocedH - dstOrigin->y)
            {
                rectSize.y = (gctINT)dstSurf->allocedH - dstOrigin->y;
            }
        }

        if (srcSurf->type == gcvSURF_BITMAP)
        {
            /* Flush the CPU cache. Source would've been rendered by the CPU. */
            gcmONERROR(gcoSURF_NODE_Cache(
                &srcSurf->node,
                srcBase[0],
                srcSurf->size,
                gcvCACHE_CLEAN));
        }

        if (dstSurf->type == gcvSURF_BITMAP)
        {
            gcmONERROR(gcoSURF_NODE_Cache(
                &dstSurf->node,
                dstBase[0],
                dstSurf->size,
                gcvCACHE_FLUSH));
        }

        /* Flush the surfaces. */
        gcmONERROR(gcoSURF_Flush(srcSurf));
        gcmONERROR(gcoSURF_Flush(dstSurf));

#if gcdENABLE_3D
        if (!srcSurf->isMsaa)
        {
            gcmONERROR(gcoSURF_DisableTileStatus(SrcView, gcvTRUE));
        }

        /* Disable the tile status for the destination. */
        gcmONERROR(gcoSURF_DisableTileStatus(DstView, gcvTRUE));
#endif /* gcdENABLE_3D */

        /* Only unsigned normalized data type and no space conversion needed go hardware copy pixels path.
        ** as this path can't handle other cases.
        */
        if (((srcSurf->formatInfo.fmtDataType == gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED) &&
            (dstSurf->formatInfo.fmtDataType == gcvFORMAT_DATATYPE_UNSIGNED_NORMALIZED) &&
            (!srcSurf->formatInfo.fakedFormat || (srcSurf->paddingFormat && !srcSurf->garbagePadded)) &&
            (!dstSurf->formatInfo.fakedFormat || dstSurf->paddingFormat) &&
            (srcSurf->colorSpace == dstSurf->colorSpace)) ||
            (Args->uArgs.v2.srcCompressed || Args->uArgs.v2.dstCompressed))
        {
            /* Read the pixel. */
            gcsSURF_RESOLVE_ARGS adjustArgs = {0};

            gcoOS_MemCopy(&adjustArgs, Args, gcmSIZEOF(adjustArgs));
            adjustArgs.uArgs.v2.rectSize = rectSize;
            if (gcmIS_ERROR(gcoHARDWARE_CopyPixels(gcvNULL, SrcView, DstView, &adjustArgs)))
            {
                cpuBlt = gcvTRUE;
            }
        }
        else
        {
            cpuBlt = gcvTRUE;
        }

#if gcdFPGA_BUILD
        {
            char* p = gcvNULL;
            gctSTRING pos = gcvNULL;

            gcoOS_GetEnv(gcvNULL, "VIV_SW_READPIXELS", &p);

            if (p)
            {
                gcoOS_StrStr(p, "1", &pos);
            }

            if (pos)
            {
                cpuBlt = gcvTRUE;
            }
        }
#endif

        if (cpuBlt)
        {
            gcsSURF_BLIT_ARGS arg;

            gcoOS_ZeroMemory(&arg, sizeof(arg));

            arg.srcSurface = srcSurf;
            arg.srcX       = srcOrigin->x;
            arg.srcY       = srcOrigin->y;
            arg.srcZ       = SrcView->firstSlice;
            arg.dstSurface = dstSurf;
            arg.dstX       = dstOrigin->x;
            arg.dstY       = dstOrigin->y;
            arg.dstZ       = DstView->firstSlice;
            arg.srcWidth   = arg.dstWidth  = rectSize.x;
            arg.srcHeight  = arg.dstHeight = rectSize.y;
            arg.srcDepth   = arg.dstDepth  = 1;
            arg.yReverse   = Args->uArgs.v2.yInverted;
            arg.srcNumSlice = SrcView->numSlices;
            arg.dstNumSlice = DstView->numSlices;
            gcmERR_BREAK(gcoSURF_BlitCPU(&arg));
        }

        if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_PE_DITHER_FIX) == gcvFALSE &&
            dstOrigin->x == 0 && rectSize.x >= (gctINT)dstSurf->requestW &&
            dstOrigin->y == 0 && rectSize.y >= (gctINT)dstSurf->requestH)
        {
            dstSurf->deferDither3D = gcvFALSE;
        }
    }
    while (gcvFALSE);

OnError:
    /* Unlock the surfaces. */
    if (dstBase[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(dstSurf, dstBase[0]));
    }

    if (srcBase[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(srcSurf, srcBase[0]));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

#endif /* gcdENABLE_3D */

gceSTATUS
gcoSURF_NODE_Cache(
    IN gcsSURF_NODE_PTR Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x, Operation=%d, Bytes=%u", Node, Operation, Bytes);

    if (Node->u.normal.cacheable == gcvFALSE)
    {
        gcmFOOTER();
        return gcvSTATUS_OK;
    }

    if (Node->pool == gcvPOOL_USER)
    {
        static gctBOOL printed;

        if (!printed)
        {
            gcmPRINT("NOTICE: Flush cache for USER_POOL memory!");
            printed = gcvTRUE;
        }
    }

    switch (Operation)
    {
        case gcvCACHE_CLEAN:
            gcmONERROR(gcoOS_CacheClean(gcvNULL, Node->u.normal.node, Logical, Bytes));
            break;

        case gcvCACHE_INVALIDATE:
            gcmONERROR(gcoOS_CacheInvalidate(gcvNULL, Node->u.normal.node, Logical, Bytes));
            break;

        case gcvCACHE_FLUSH:
            gcmONERROR(gcoOS_CacheFlush(gcvNULL, Node->u.normal.node, Logical, Bytes));
            break;

        default:
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            break;
    }

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_NODE_CPUCacheOperation
**
**  Perform the specified CPU cache operation on the surface node.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR Node
**          Pointer to the surface node.
**      gctPOINTER Logical
**          logical address to flush.
**      gctSIZE_T Bytes
**          bytes to flush.
**      gceSURF_CPU_CACHE_OP_TYPE Operation
**          Cache operation to be performed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_NODE_CPUCacheOperation(
    IN gcsSURF_NODE_PTR Node,
    IN gceSURF_TYPE Type,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Length,
    IN gceCACHEOPERATION Operation
    )
{

    gceSTATUS status;
    gctPOINTER memory;
    gctBOOL locked = gcvFALSE;

    gcmHEADER_ARG("Node=0x%x, Type=%u, Offset=%u, Length=%u, Operation=%d", Node, Type, Offset, Length, Operation);

    /* Lock the node. */
    gcmONERROR(gcoHARDWARE_Lock(Node, gcvNULL, &memory));
    locked = gcvTRUE;

    gcmONERROR(gcoSURF_NODE_Cache(Node,
                                  (gctUINT8_PTR)memory + Offset,
                                  Length,
                                  Operation));


    /* Unlock the node. */
    gcmONERROR(gcoHARDWARE_Unlock(Node, Type));
    locked = gcvFALSE;

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    if (locked)
    {
        gcmVERIFY_OK(gcoHARDWARE_Unlock(Node, Type));
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_LockNode(
                 IN gcsSURF_NODE_PTR Node,
                 OUT gctUINT32 * Address,
                 OUT gctPOINTER * Memory
                 )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x, Address=0x%x, Memory=0x%x", Node, Address, Memory);

    gcmONERROR(gcoHARDWARE_Lock(Node, Address, Memory));

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_UnLockNode(
                   IN gcsSURF_NODE_PTR Node,
                   IN gceSURF_TYPE Type
                   )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x, Type=%u", Node, Type);

    gcmONERROR(gcoHARDWARE_Unlock(Node, Type));

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_CPUCacheOperation
**
**  Perform the specified CPU cache operation on the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_CPU_CACHE_OP_TYPE Operation
**          Cache operation to be performed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_CPUCacheOperation(
    IN gcoSURF Surface,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER source[3] = {0};
    gctBOOL locked = gcvFALSE;

    gcmHEADER_ARG("Surface=0x%x, Operation=%d", Surface, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Lock the surfaces. */
    gcmONERROR(gcoSURF_Lock(Surface, gcvNULL, source));
    locked = gcvTRUE;

    gcmONERROR(gcoSURF_NODE_Cache(&Surface->node,
                                  source[0],
                                  Surface->node.size,
                                  Operation));

    /* Unlock the surfaces. */
    gcmONERROR(gcoSURF_Unlock(Surface, source[0]));
    locked = gcvFALSE;

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    if (locked)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(Surface, source[0]));
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Flush
**
**  Flush the caches to make sure the surface has all pixels.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Flush(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);


    /* Flush the current pipe. */
    status = gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL);
    gcmFOOTER();
    return status;
}

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoSURF_FillFromTile
**
**  Fill the surface from the information in the tile status buffer.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_FillFromTile(
    IN gcsSURF_VIEW *SurView
    )
{
    gceSTATUS status;
    gcoSURF Surface = SurView->surf;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurView->numSlices == 1);

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_TILE_FILLER)
    &&  (Surface->type == gcvSURF_RENDER_TARGET)
    &&  (!Surface->isMsaa)
    &&  (Surface->compressed == gcvFALSE)
    &&  (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    &&  (Surface->tileStatusDisabled[SurView->firstSlice] == gcvFALSE))
    {
        /*
         * Call underlying tile status disable to do FC fill:
         * 1. Flush pipe / tile status cache
         * 2. Decompress (Fill) tile status
         * 3. Set surface tileStatusDisabled to true.
         */
        gcmONERROR(
            gcoHARDWARE_DisableTileStatus(gcvNULL,
                                          SurView,
                                          gcvTRUE));
    }
    else if ((Surface->tileStatusNode.pool == gcvPOOL_UNKNOWN)
        || (Surface->tileStatusDisabled[SurView->firstSlice] == gcvTRUE))
    {
        /* Flush pipe cache. */
        gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));

        /*
         * No need to fill if tile status disabled.
         * Return OK here to tell the caller that FC fill is done, because
         * the caller(drivers) may be unable to know it.
         */
        status = gcvSTATUS_OK;
    }
    else
    {
        /* Set return value. */
        status = gcvSTATUS_NOT_SUPPORTED;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoSURF_SetSamples
**
**  Set the number of samples per pixel.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gctUINT Samples
**          Number of samples per pixel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetSamples(
    IN gcoSURF Surface,
    IN gctUINT Samples
    )
{
    gctUINT samples;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Surface=0x%x Samples=%u", Surface, Samples);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);


    /* Make sure this is not user-allocated surface. */
    if (Surface->node.pool == gcvPOOL_USER)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    samples = (Samples == 0) ? 1 : Samples;

    if (Surface->sampleInfo.product != samples)
    {
        gceSURF_TYPE type = Surface->type;

        type = (gceSURF_TYPE)(type | Surface->hints);

        /* Destroy existing surface memory. */
        gcmONERROR(_FreeSurface(Surface));

        /* Allocate new surface. */
        gcmONERROR(
            _AllocateSurface(Surface,
                             Surface->requestW,
                             Surface->requestH,
                             Surface->requestD,
                             type,
                             Surface->format,
                             samples,
                             gcvPOOL_DEFAULT));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_GetSamples
**
**  Get the number of samples per pixel.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gctUINT_PTR Samples
**          Pointer to variable receiving the number of samples per pixel.
**
*/
gceSTATUS
gcoSURF_GetSamples(
    IN gcoSURF Surface,
    OUT gctUINT_PTR Samples
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(Samples != gcvNULL);

    /* Return samples. */
    *Samples = Surface->sampleInfo.product;

    /* Success. */
    gcmFOOTER_ARG("*Samples=%u", *Samples);
    return gcvSTATUS_OK;
}
#endif

/*******************************************************************************
**
**  gcoSURF_SetResolvability
**
**  Set the resolvability of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gctBOOL Resolvable
**          gcvTRUE if the surface is resolvable or gcvFALSE if not.  This is
**          required for alignment purposes.
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_SetResolvability(
    IN gcoSURF Surface,
    IN gctBOOL Resolvable
    )
{
#if gcdENABLE_3D
    gcmHEADER_ARG("Surface=0x%x Resolvable=%d", Surface, Resolvable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set the resolvability. */
    Surface->resolvable = Resolvable;

    /* Success. */
    gcmFOOTER_NO();
#endif
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_SetOrientation
**
**  Set the orientation of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceORIENTATION Orientation
**          The requested surface orientation.  Orientation can be one of the
**          following values:
**
**              gcvORIENTATION_TOP_BOTTOM - Surface is from top to bottom.
**              gcvORIENTATION_BOTTOM_TOP - Surface is from bottom to top.
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_SetOrientation(
    IN gcoSURF Surface,
    IN gceORIENTATION Orientation
    )
{
    gcmHEADER_ARG("Surface=0x%x Orientation=%d", Surface, Orientation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set the orientation. */
    Surface->orientation = Orientation;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_QueryOrientation
**
**  Query the orientation of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gceORIENTATION * Orientation
**          Pointer to a variable receiving the surface orientation.
**
*/
gceSTATUS
gcoSURF_QueryOrientation(
    IN gcoSURF Surface,
    OUT gceORIENTATION * Orientation
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(Orientation != gcvNULL);

    /* Return the orientation. */
    *Orientation = Surface->orientation;

    /* Success. */
    gcmFOOTER_ARG("*Orientation=%d", *Orientation);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_QueryFlags
**
**  Query status of the flag.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to surface object.
**      gceSURF_FLAG Flag
**          Flag which is queried
**
**  OUTPUT:
**      None
**
*/
gceSTATUS
gcoSURF_QueryFlags(
    IN gcoSURF Surface,
    IN gceSURF_FLAG Flag
    )
{
    gceSTATUS status = gcvSTATUS_TRUE;
    gcmHEADER_ARG("Surface=0x%x Flag=0x%x", Surface, Flag);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->flags & Flag)
    {
        status = gcvSTATUS_TRUE;
    }
    else
    {
        status = gcvSTATUS_FALSE;
    }
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_QueryHints(
    IN gcoSURF Surface,
    IN gceSURF_TYPE Hints
    )
{
    gceSTATUS status = gcvSTATUS_TRUE;
    gcmHEADER_ARG("Surface=0x%x Hints=0x%x", Surface, Hints);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Surface->hints & Hints)
    {
        status = gcvSTATUS_TRUE;
    }
    else
    {
        status = gcvSTATUS_FALSE;
    }
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_QueryFormat
**
**  Return pixel format parameters.
**
**  INPUT:
**
**      gceSURF_FORMAT Format
**          API format.
**
**  OUTPUT:
**
**      gcsSURF_FORMAT_INFO_PTR * Info
**          Pointer to a variable that will hold the format description entry.
**          If the format in question is interleaved, two pointers will be
**          returned stored in an array fashion.
**
*/
gceSTATUS
gcoSURF_QueryFormat(
    IN gceSURF_FORMAT Format,
    OUT gcsSURF_FORMAT_INFO_PTR * Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Format=%d", Format);

    {
#if gcdENABLE_3D
        gcoHARDWARE hardware = gcvNULL;
        gcmGETHARDWARE(hardware);
        (void) hardware;
#endif
    }

    gcmONERROR(gcoHARDWARE_QueryFormat(Format, Info));

OnError:
    gcmFOOTER();
    return status;
}



/*******************************************************************************
**
**  gcoSURF_SetColorType
**
**  Set the color type of the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_COLOR_TYPE colorType
**          color type of the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetColorType(
    IN gcoSURF Surface,
    IN gceSURF_COLOR_TYPE ColorType
    )
{
    gcmHEADER_ARG("Surface=0x%x ColorType=%d", Surface, ColorType);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set the color type. */
    Surface->colorType = ColorType;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetColorType
**
**  Get the color type of the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gceSURF_COLOR_TYPE *colorType
**          pointer to the variable receiving color type of the surface.
**
*/
gceSTATUS
gcoSURF_GetColorType(
    IN gcoSURF Surface,
    OUT gceSURF_COLOR_TYPE *ColorType
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(ColorType != gcvNULL);

    /* Return the color type. */
    *ColorType = Surface->colorType;

    /* Success. */
    gcmFOOTER_ARG("*ColorType=%d", *ColorType);
    return gcvSTATUS_OK;
}




/*******************************************************************************
**
**  gcoSURF_SetColorSpace
**
**  Set the color type of the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_COLOR_SPACE ColorSpace
**          color space of the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetColorSpace(
    IN gcoSURF Surface,
    IN gceSURF_COLOR_SPACE ColorSpace
    )
{
    gcmHEADER_ARG("Surface=0x%x ColorSpace=%d", Surface, ColorSpace);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set the color space. */
    Surface->colorSpace = ColorSpace;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetColorSpace
**
**  Get the color space of the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gceSURF_COLOR_SPACE *ColorSpace
**          pointer to the variable receiving color space of the surface.
**
*/
gceSTATUS
gcoSURF_GetColorSpace(
    IN gcoSURF Surface,
    OUT gceSURF_COLOR_SPACE *ColorSpace
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(ColorSpace != gcvNULL);

    /* Return the color type. */
    *ColorSpace = Surface->colorSpace;

    /* Success. */
    gcmFOOTER_ARG("*ColorSpace=%d", *ColorSpace);
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoSURF_SetRotation
**
**  Set the surface ration angle.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_ROTATION Rotation
**          Rotation angle.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetRotation(
    IN gcoSURF Surface,
    IN gceSURF_ROTATION Rotation
    )
{
    gcmHEADER_ARG("Surface=0x%x Rotation=%d", Surface, Rotation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Support only 2D surfaces. */
    if (Surface->type != gcvSURF_BITMAP)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Set new rotation. */
    Surface->rotation = Rotation;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_SetDither
**
**  Set the surface dither flag.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_ROTATION Dither
**          dither enable or not.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetDither(
    IN gcoSURF Surface,
    IN gctBOOL Dither
    )
{
    gcmHEADER_ARG("Surface=0x%x Dither=%d", Surface, Dither);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Support only 2D surfaces. */
    if (Surface->type != gcvSURF_BITMAP)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Set new rotation. */
    Surface->dither2D = Dither;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#if gcdENABLE_3D
/*******************************************************************************
**
**  gcoSURF_Copy
**
**  Copy one tiled surface to another tiled surfaces.  This is used for handling
**  unaligned surfaces.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the gcoSURF object that describes the surface to copy
**          into.
**
**      gcoSURF Source
**          Pointer to the gcoSURF object that describes the source surface to
**          copy from.
**
**  OUTPUT:
**
**      Nothing.
**
*/
gceSTATUS
gcoSURF_Copy(
    IN gcoSURF Surface,
    IN gcoSURF Source
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT8_PTR source = gcvNULL, target = gcvNULL;


    gcmHEADER_ARG("Surface=0x%x Source=0x%x", Surface, Source);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(Source, gcvOBJ_SURF);


    if ((Surface->tiling != Source->tiling) ||
        ((Surface->tiling != gcvTILED) &&
         (Surface->tiling != gcvSUPERTILED)
        )
       )
    {
        /* Both surfaces need to the same tiled.
        ** only tile and supertile are supported.
        */
        gcmFOOTER();
        return gcvSTATUS_INVALID_REQUEST;
    }

    do
    {
        gctUINT y;
        gctUINT sourceOffset, targetOffset;
        gctINT height = 0;
        gctPOINTER pointer[3] = { gcvNULL };

        {
            /* Flush the pipe. */
            gcmERR_BREAK(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));

            /* Commit and stall the pipe. */
            gcmERR_BREAK(gcoHAL_Commit(gcvNULL, gcvTRUE));

            switch (Surface->tiling)
            {
            case gcvTILED:
                /* Get the tile height. */
                gcmERR_BREAK(gcoHARDWARE_QueryTileSize(gcvNULL, gcvNULL,
                                                       gcvNULL, &height,
                                                       gcvNULL));
                break;
            case gcvSUPERTILED:
                height = 64;
                break;
            default:
                gcmASSERT(0);
                height = 4;
                break;
            }
        }
        /* Lock the surfaces. */
        gcmERR_BREAK(gcoSURF_Lock(Source, gcvNULL, pointer));

        source = pointer[0];

        gcmERR_BREAK(gcoSURF_Lock(Surface, gcvNULL, pointer));

        target = pointer[0];

        /* Reset initial offsets. */
        sourceOffset = 0;
        targetOffset = 0;

        /* Loop target surface, one row of tiles at a time. */
        for (y = 0; y < Surface->alignedH; y += height)
        {
            /* Copy one row of tiles. */
            gcoOS_MemCopy(target + targetOffset,
                          source + sourceOffset,
                          Surface->stride * height);

            /* Move to next row of tiles. */
            sourceOffset += Source->stride  * height;
            targetOffset += Surface->stride * height;
        }
    }
    while (gcvFALSE);

    if (source != gcvNULL)
    {
        /* Unlock source surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(Source, source));
    }

    if (target != gcvNULL)
    {
        /* Unlock target surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(Surface, target));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

#if gcdENABLE_3D
gceSTATUS
gcoSURF_IsHWResolveable(
    IN gcoSURF SrcSurface,
    IN gcoSURF DstSurface,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DstOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gcsPOINT  rectSize;
    gctINT maxWidth;
    gctINT maxHeight;

    gcmHEADER_ARG("SrcSurface=0x%x DstSurface=0x%x SrcOrigin=0x%x "
                  "DstOrigin=0x%x RectSize=0x%x",
                  SrcSurface, DstSurface, SrcOrigin, DstOrigin, RectSize);

    if ((DstOrigin->x == 0) &&
        (DstOrigin->y == 0) &&
        (RectSize->x == (gctINT)DstSurface->requestW) &&
        (RectSize->y == (gctINT)DstSurface->requestH))
    {
        /* Full destination resolve, a special case. */
        rectSize.x = DstSurface->alignedW;
        rectSize.y = DstSurface->alignedH;
    }
    else
    {
        rectSize.x = RectSize->x;
        rectSize.y = RectSize->y;
    }

    /* Make sure we don't go beyond the source surface. */
    maxWidth  = SrcSurface->alignedW - SrcOrigin->x;
    maxHeight = SrcSurface->alignedH - SrcOrigin->y;

    rectSize.x = gcmMIN(maxWidth,  rectSize.x);
    rectSize.y = gcmMIN(maxHeight, rectSize.y);

    /* Make sure we don't go beyond the target surface. */
    maxWidth  = DstSurface->alignedW - DstOrigin->x;
    maxHeight = DstSurface->alignedH - DstOrigin->y;

    rectSize.x = gcmMIN(maxWidth,  rectSize.x);
    rectSize.y = gcmMIN(maxHeight, rectSize.y);

    if ((SrcSurface->type == gcvSURF_DEPTH)
    &&  (SrcSurface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
    )
    {
        status = gcvSTATUS_FALSE;
    }
    else
    {
         status = gcoHARDWARE_IsHWResolveable(SrcSurface,
                                              DstSurface,
                                              SrcOrigin,
                                              DstOrigin,
                                              &rectSize);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* gcdENABLE_3D */

/*******************************************************************************
**
**  gcoSURF_ConstructWrapper
**
**  Create a new gcoSURF wrapper object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to the variable that will hold the gcoSURF object pointer.
*/
gceSTATUS
gcoSURF_ConstructWrapper(
    IN gcoHAL Hal,
    OUT gcoSURF * Surface
    )
{
    gcoSURF surface = gcvNULL;
    gceSTATUS status;
    gctUINT i;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    do
    {
        /* Allocate the gcoSURF object. */
        gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(struct _gcoSURF), (gctPOINTER*)&surface));

        /* Reset the object. */
        gcoOS_ZeroMemory(surface, gcmSIZEOF(struct _gcoSURF));

        /* Initialize the gcoSURF object.*/
        surface->object.type = gcvOBJ_SURF;

#if gcdENABLE_3D
        /* 1 sample per pixel. */
        surface->sampleInfo = g_sampleInfos[1];
        surface->isMsaa     = gcvFALSE;
        surface->vMsaa     = gcvFALSE;
#endif /* gcdENABLE_3D */

        /* One plane. */
        surface->requestD = 1;

        /* Initialize the node. */
        surface->node.pool      = gcvPOOL_USER;
        surface->node.physical2 = ~0U;
        surface->node.physical3 = ~0U;
        surface->node.count     = 1;
        surface->refCount       = 1;

#if gcdENABLE_3D
        gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctUINT) * surface->requestD, (gctPOINTER*)&surface->fcValue));
        gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctUINT) * surface->requestD, (gctPOINTER*)&surface->fcValueUpper));
        gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctBOOL) * surface->requestD, (gctPOINTER*)&surface->tileStatusDisabled));
        gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gctBOOL) * surface->requestD, (gctPOINTER*)&surface->dirty));

        gcoOS_ZeroMemory(surface->fcValue, gcmSIZEOF(gctUINT) * surface->requestD);
        gcoOS_ZeroMemory(surface->fcValueUpper, gcmSIZEOF(gctUINT) * surface->requestD);
        gcoOS_ZeroMemory(surface->tileStatusDisabled, gcmSIZEOF(gctBOOL) * surface->requestD);
        gcoOS_ZeroMemory(surface->dirty, gcmSIZEOF(gctBOOL) * surface->requestD);
#endif

        surface->flags = gcvSURF_FLAG_NONE;

        for (i = 0; i < gcvHARDWARE_NUM_TYPES; i++)
        {
            surface->node.hardwareAddresses[i] = ~0U;
        }

        surface->pfGetAddr = gcoHARDWARE_GetProcCalcPixelAddr(gcvNULL, surface);

        /* Return pointer to the gcoSURF object. */
        *Surface = surface;

        /* Success. */
        gcmFOOTER_ARG("*Surface=0x%x", *Surface);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

OnError:

#if gcdENABLE_3D
    if (surface != gcvNULL)
    {
        if (surface->fcValue)
        {
            gcoOS_Free(gcvNULL, surface->fcValue);
            surface->fcValue = gcvNULL;
        }
        if (surface->fcValueUpper)
        {
            gcoOS_Free(gcvNULL, surface->fcValueUpper);
            surface->fcValueUpper = gcvNULL;
        }
        if (surface->tileStatusDisabled)
        {
            gcoOS_Free(gcvNULL, surface->tileStatusDisabled);
            surface->tileStatusDisabled = gcvNULL;
        }
        if (surface->dirty)
        {
            gcoOS_Free(gcvNULL, surface->dirty);
            surface->dirty = gcvNULL;
        }

        gcoOS_Free(gcvNULL, surface);
        surface = gcvNULL;
    }
#endif

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_SetFlags
**
**  Set status of the flag.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to surface object.
**      gceSURF_FLAG Flag
**          Surface Flag
**      gctBOOL Value
**          New value for this flag.
**
**  OUTPUT:
**      None
**
*/
gceSTATUS
gcoSURF_SetFlags(
    IN gcoSURF Surface,
    IN gceSURF_FLAG Flag,
    IN gctBOOL Value
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Surface=0x%x Flag=0x%x Value=0x%x", Surface, Flag, Value);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Value)
    {
        Surface->flags |= Flag;
    }
    else
    {
        Surface->flags &= ~Flag;
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_SetBuffer
**
**  Set the underlying buffer for the surface wrapper.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gceSURF_FORMAT Format
**          Format of surface to create.
**
**      gctINT Stride
**          Surface stride. Is set to ~0 the stride will be autocomputed.
**
**      gctPOINTER Logical
**          Logical pointer to the user allocated surface or gcvNULL if no
**          logical pointer has been provided.
**
**      gctPHYS_ADDR_T Physical
**          Physical address (NOT GPU address) of a contiguous buffer.
**          It should be gcvINVALID_PHYSICAL_ADDRESS for non-contiguous buffer or
**          buffer whose physical address is unknown.
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetBuffer(
    IN gcoSURF Surface,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gctUINT Stride,
    IN gctPOINTER Logical,
    IN gctPHYS_ADDR_T Physical
    )
{
    gceSTATUS status;
    gcsSURF_FORMAT_INFO_PTR fmtInfo;

    gcmHEADER_ARG("Surface=0x%x Type=%d Format=%d Stride=%u Logical=0x%x "
                  "Physical=0x%llx",
                  Surface, Type, Format, Stride, Logical, Physical);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Has to be user-allocated surface. */
    if (Surface->node.pool != gcvPOOL_USER)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Cancel existed wrapping. */
    gcmONERROR(_UnwrapUserMemory(Surface));

    /* Set surface parameters. */
    Surface->type   = (gceSURF_TYPE) ((gctUINT32) Type &  0xFF);
    Surface->hints  = (gceSURF_TYPE) ((gctUINT32) Type & ~0xFF);
    Surface->format = Format;
    Surface->stride = Stride;

    /* Set node pointers. */
    Surface->userLogical  = Logical;
    Surface->userPhysical = Physical;

    {
        /* Compute bits per pixel. */
        gcmONERROR(gcoHARDWARE_ConvertFormat(Format,
                                             (gctUINT32_PTR)&Surface->bitsPerPixel,
                                             gcvNULL));
    }

    /* Initialize Surface->formatInfo */
    gcmONERROR(gcoSURF_QueryFormat(Format, &fmtInfo));
    Surface->formatInfo = *fmtInfo;
    Surface->bitsPerPixel = fmtInfo->bitsPerPixel;

#if gcdENABLE_3D
    Surface->colorSpace = gcd_QUERY_COLOR_SPACE(Format);
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_SetVideoBuffer
**
**  Set the underlying video buffer for the surface wrapper.
**  The video plane addresses should be specified individually.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gceSURF_FORMAT Format
**          Format of surface to create.
**
**      gctINT Stride
**          Surface stride. Is set to ~0 the stride will be autocomputed.
**
**      gctPOINTER LogicalPlane1
**          Logical pointer to the first plane of the user allocated surface
**          or gcvNULL if no logical pointer has been provided.
**
**      gctUINT32 PhysicalPlane1
**          Physical pointer to the user allocated surface or ~0 if no
**          physical pointer has been provided.
**
**  OUTPUT:
**
**      Nothing.
*/

/*******************************************************************************
**
**  gcoSURF_SetWindow
**
**  Set the size of the surface in pixels and map the underlying buffer set by
**  gcoSURF_SetBuffer if necessary.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gctINT X, Y
**          The origin of the surface.
**
**      gctINT Width, Height
**          Size of the surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetWindow(
    IN gcoSURF Surface,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height
    )
{
    gceSTATUS status;
    gctUINT32 offset;
    gctUINT userStride;
    gcsUSER_MEMORY_DESC desc;

    gcmHEADER_ARG("Surface=0x%x X=%u Y=%u Width=%u Height=%u",
                  Surface, X, Y, Width, Height);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);


    /* Cancel existed wrapping. */
    gcmONERROR(_UnwrapUserMemory(Surface));

    /* Make sure at least one of the surface pointers is set. */
    if (Surface->userLogical == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_ADDRESS);
    }

    /* Set the size. */
    Surface->requestW = Width;
    Surface->requestH = Height;
    Surface->requestD = 1;
    Surface->allocedW = Width;
    Surface->allocedH = Height;
    Surface->alignedW = Width;
    Surface->alignedH = Height;

    /* Stride is the same as the width? */
    if (Surface->stride == ~0U)
    {
        /* Compute the stride. */
        Surface->stride = Width * Surface->bitsPerPixel / 8;
    }
    else
    {
        {
            /* Align the surface size. */
#if gcdENABLE_3D
            gcmONERROR(
                gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                  Surface->type,
                                                  Surface->hints,
                                                  Surface->format,
                                                  &Surface->alignedW,
                                                  &Surface->alignedH,
                                                  Surface->requestD,
                                                  &Surface->tiling,
                                                  &Surface->superTiled,
                                                  &Surface->hAlignment));
#else
            gcmONERROR(
                gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                  Surface->type,
                                                  Surface->hints,
                                                  Surface->format,
                                                  &Surface->alignedW,
                                                  &Surface->alignedH,
                                                  Surface->requestD,
                                                  &Surface->tiling,
                                                  gcvNULL,
                                                  gcvNULL));
#endif /* gcdENABLE_3D */
        }
    }

    userStride = Surface->stride;

    /* Compute the HAL required stride */
    _ComputeSurfacePlacement(Surface, gcvTRUE);
    if (Surface->type == gcvSURF_BITMAP)
    {
        /* For bitmap surface, user buffer stride may be larger than
        ** least stride required in HAL.
        */
        if (userStride < Surface->stride)
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }
    else
    {
        /* For Vivante internal surfaces types, user buffer placement
        ** must be the same as what defined in HAL, otherwise the user
        ** buffer is not compatible with HAL.
        */
        if (userStride != Surface->stride)
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    /* Compute surface placement with user stride */
    Surface->stride = userStride;
    _ComputeSurfacePlacement(Surface, gcvFALSE);

    Surface->layerSize = Surface->sliceSize * Surface->requestD;

    /* Always single layer for user surface */
    gcmASSERT(Surface->formatInfo.layers == 1);

    Surface->size = Surface->layerSize * Surface->formatInfo.layers;

    offset = Y * Surface->stride + X * Surface->bitsPerPixel / 8;

    /* Set user pool node size. */
    Surface->node.size = Surface->size;
    /* Need to map logical pointer? */
    Surface->node.logical = (gctUINT8_PTR)Surface->userLogical + offset;
    Surface->node.u.wrapped.physical = Surface->userPhysical + (gctUINT64)offset;

    desc.physical = Surface->node.u.wrapped.physical;
    desc.logical = gcmPTR_TO_UINT64(Surface->node.logical);
    desc.size = Surface->size;
    desc.flag = gcvALLOC_FLAG_USERMEMORY;

    gcmONERROR(gcoHAL_WrapUserMemory(&desc,
                                     (gceVIDMEM_TYPE)Surface->type,
                                     &Surface->node.u.normal.node));

    Surface->pfGetAddr = gcoHARDWARE_GetProcCalcPixelAddr(gcvNULL, Surface);

    /* Initial lock. */
    gcmONERROR(_Lock(Surface));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoSURF_SetImage
**
**  Set the size of the surface in pixels and map the underlying buffer set by
**  gcoSURF_SetBuffer if necessary.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gctINT X, Y
**          The origin of the surface.
**
**      gctINT Width, Height, Depth
**          Size of the surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetImage(
    IN gcoSURF Surface,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth
    )
{
    gceSTATUS status;
    gctUINT32 offset;
    gctUINT   userStride;
    gcsUSER_MEMORY_DESC desc;

    gcmHEADER_ARG("Surface=0x%x X=%u Y=%u Width=%u Height=%u",
                  Surface, X, Y, Width, Height);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);


    /* Cancel existed wrapping. */
    gcmONERROR(_UnwrapUserMemory(Surface));

    /* Make sure at least one of the surface pointers is set. */
    if (Surface->userLogical == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_ADDRESS);
    }

    /* Set the size. */
    Surface->requestW = Width;
    Surface->requestH = Height;
    Surface->requestD = Depth;
    Surface->allocedW = Width;
    Surface->allocedH = Height;
    Surface->alignedW = Width;
    Surface->alignedH = Height;

    /* Stride is the same as the width? */
    if (Surface->stride == ~0U)
    {
        /* Compute the stride. */
        Surface->stride = Width * Surface->bitsPerPixel / 8;
    }
    else
    {
        {
            /* Align the surface size. */
#if gcdENABLE_3D
            gcmONERROR(
                gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                  Surface->type,
                                                  Surface->hints,
                                                  Surface->format,
                                                  &Surface->alignedW,
                                                  &Surface->alignedH,
                                                  Surface->requestD,
                                                  &Surface->tiling,
                                                  &Surface->superTiled,
                                                  &Surface->hAlignment));
#else
            gcmONERROR(
                gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                  Surface->type,
                                                  0,
                                                  Surface->format,
                                                  &Surface->alignedW,
                                                  &Surface->alignedH,
                                                  Surface->requestD,
                                                  &Surface->tiling,
                                                  gcvNULL,
                                                  gcvNULL));
#endif /* gcdENABLE_3D */
        }
    }

    userStride = Surface->stride;

    /* Compute the HAL required stride */
    _ComputeSurfacePlacement(Surface, gcvTRUE);
    if (Surface->type == gcvSURF_BITMAP)
    {
        /* For bitmap surface, user buffer stride may be larger than
        ** least stride required in HAL.
        */
        if (userStride < Surface->stride)
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }
    else
    {
        /* For Vivante internal surfaces types, user buffer placement
        ** must be the same as what defined in HAL, otherwise the user
        ** buffer is not compatible with HAL.
        */
        if (userStride != Surface->stride)
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    /* Compute surface placement with user stride */
    Surface->stride = userStride;
    _ComputeSurfacePlacement(Surface, gcvFALSE);

    Surface->layerSize = Surface->sliceSize * Surface->requestD;

    /* Always single layer for user surface */
    gcmASSERT(Surface->formatInfo.layers == 1);

    Surface->size = Surface->layerSize * Surface->formatInfo.layers;

    offset = Y * Surface->stride + X * Surface->bitsPerPixel / 8;

    /* Set user pool node size. */
    Surface->node.size = Surface->size;
    Surface->node.logical = (gctUINT8_PTR)Surface->userLogical + offset;
    Surface->node.u.wrapped.physical = Surface->userPhysical + (gctUINT64)offset;

    desc.physical = Surface->node.u.wrapped.physical;
    desc.logical = gcmPTR_TO_UINT64(Surface->node.logical);
    desc.size = Surface->size;
    desc.flag = gcvALLOC_FLAG_USERMEMORY;

    gcmONERROR(gcoHAL_WrapUserMemory(&desc,
                                     (gceVIDMEM_TYPE)Surface->type,
                                     &Surface->node.u.normal.node));

    Surface->pfGetAddr = gcoHARDWARE_GetProcCalcPixelAddr(gcvNULL, Surface);

    /* Initial lock. */
    gcmONERROR(_Lock(Surface));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_SetAlignment
**
**  Set the alignment width/height of the surface and calculate stride/size.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gctINT Width, Height
**          Size of the surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetAlignment(
    IN gcoSURF Surface,
    IN gctUINT Width,
    IN gctUINT Height
    )
{
    gcmHEADER_ARG("Surface=0x%x Width=%u Height=%u", Surface, Width, Height);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    Surface->alignedW = Width;
    Surface->alignedH = Height;

    /* Compute the surface stride. */
    Surface->stride = Surface->alignedW
                           * Surface->bitsPerPixel / 8;

    /* Compute the surface size. */
    Surface->size
        = Surface->stride
        * Surface->alignedH;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_ReferenceSurface
**
**  Increase reference count of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_ReferenceSurface(
    IN gcoSURF Surface
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    Surface->refCount++;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoSURF_QueryReferenceCount
**
**  Query reference count of a surface
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      gctINT32 ReferenceCount
**          Reference count of a surface
*/
gceSTATUS
gcoSURF_QueryReferenceCount(
    IN gcoSURF Surface,
    OUT gctINT32 * ReferenceCount
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    *ReferenceCount = Surface->refCount;

    gcmFOOTER_ARG("*ReferenceCount=%d", *ReferenceCount);
    return gcvSTATUS_OK;
}

#if gcdENABLE_3D
gceSTATUS
gcoSURF_IsRenderable(
    IN gcoSURF Surface
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Check whether the surface is renderable. */
    status = gcoHARDWARE_QuerySurfaceRenderable(gcvNULL, Surface);

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_IsFormatRenderableAsRT(
    IN gcoSURF Surface
    )
{
    gceSTATUS               status;
    gceSURF_FORMAT          format;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Check whether the surface format is renderable when
       Tex bind to fbo. */
    format = Surface->format;

    status = (format >= 700) ? gcvSTATUS_FALSE
           : gcvSTATUS_TRUE;

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_Swap(
    IN gcoSURF Surface1,
    IN gcoSURF Surface2
    )
{
    struct _gcoSURF temp;

    gcmHEADER_ARG("Surface1=%p Surface2=%p", Surface1, Surface2);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface1, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(Surface2, gcvOBJ_SURF);

    /* Swap the surfaces. */
    temp      = *Surface1;
    *Surface1 = *Surface2;
    *Surface2 = temp;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#define gcdCOLOR_SPACE_CONVERSION_NONE         0
#define gcdCOLOR_SPACE_CONVERSION_TO_LINEAR    1
#define gcdCOLOR_SPACE_CONVERSION_TO_NONLINEAR 2

static
gceSTATUS
_AveragePixels(
    gcsPIXEL *pixels,
    gctINT pixelCount,
    gceFORMAT_DATATYPE inputFormat,
    gcsPIXEL *outPixel
    )
{
    gctINT i;
    gcsPIXEL mergePixel;

    gcoOS_ZeroMemory(&mergePixel, sizeof(gcsPIXEL));

    switch(inputFormat)
    {
    case gcvFORMAT_DATATYPE_UNSIGNED_INTEGER:
        for (i = 0; i < pixelCount; i++)
        {
            mergePixel.color.ui.r += pixels[i].color.ui.r;
            mergePixel.color.ui.g += pixels[i].color.ui.g;
            mergePixel.color.ui.b += pixels[i].color.ui.b;
            mergePixel.color.ui.a += pixels[i].color.ui.a;
            mergePixel.d += pixels[i].d;
            mergePixel.s += pixels[i].s;
        }

        mergePixel.color.ui.r /= pixelCount;
        mergePixel.color.ui.g /= pixelCount;
        mergePixel.color.ui.b /= pixelCount;
        mergePixel.color.ui.a /= pixelCount;
        mergePixel.d /= pixelCount;
        mergePixel.s /= pixelCount;
        break;

    case gcvFORMAT_DATATYPE_SIGNED_INTEGER:
        for (i = 0; i < pixelCount; i++)
        {
            mergePixel.color.i.r += pixels[i].color.i.r;
            mergePixel.color.i.g += pixels[i].color.i.g;
            mergePixel.color.i.b += pixels[i].color.i.b;
            mergePixel.color.i.a += pixels[i].color.i.a;
            mergePixel.d += pixels[i].d;
            mergePixel.s += pixels[i].s;
        }

        mergePixel.color.i.r /= pixelCount;
        mergePixel.color.i.g /= pixelCount;
        mergePixel.color.i.b /= pixelCount;
        mergePixel.color.i.a /= pixelCount;
        mergePixel.d /= pixelCount;
        mergePixel.s /= pixelCount;

        break;
    default:
        for (i = 0; i < pixelCount; i++)
        {
            mergePixel.color.f.r += pixels[i].color.f.r;
            mergePixel.color.f.g += pixels[i].color.f.g;
            mergePixel.color.f.b += pixels[i].color.f.b;
            mergePixel.color.f.a += pixels[i].color.f.a;
            mergePixel.d += pixels[i].d;
            mergePixel.s += pixels[i].s;
        }

        mergePixel.color.f.r /= pixelCount;
        mergePixel.color.f.g /= pixelCount;
        mergePixel.color.f.b /= pixelCount;
        mergePixel.color.f.a /= pixelCount;
        mergePixel.d /= pixelCount;
        mergePixel.s /= pixelCount;
        break;
    }

    *outPixel = mergePixel;

    return gcvSTATUS_OK;
}

gceSTATUS
gcoSURF_BlitCPU(
    IN gcsSURF_BLIT_ARGS* args
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF srcSurf = gcvNULL, dstSurf = gcvNULL;
    gctPOINTER srcAddr[3] = {gcvNULL};
    gctPOINTER dstAddr[3] = {gcvNULL};
    _PFNreadPixel pfReadPixel = gcvNULL;
    _PFNwritePixel pfWritePixel = gcvNULL;
    gctFLOAT xScale, yScale, zScale;
    gctINT iDst, jDst, kDst;
    gctINT colorSpaceConvert = gcdCOLOR_SPACE_CONVERSION_NONE;
    gcsSURF_BLIT_ARGS blitArgs;
    gctINT scissorHeight = 0;
    gctINT scissorWidth = 0;
    gctBOOL averagePixels = gcvTRUE;
    gcsSURF_FORMAT_INFO *srcFmtInfo, *dstFmtInfo;
    gcsSURF_VIEW srcView;
    gcsSURF_VIEW dstView;
    gceAPI currentApi;

    if (!args || !args->srcSurface || !args->dstSurface)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmONERROR(gcoHARDWARE_GetAPI(gcvNULL, &currentApi, gcvNULL));

    if(currentApi != gcvAPI_OPENGL)
    {
        if (args->srcSurface == args->dstSurface)
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }
    }

    srcView.surf = args->srcSurface;
    srcView.firstSlice = args->srcZ;
    srcView.numSlices = args->srcNumSlice;
    dstView.surf = args->dstSurface;
    dstView.firstSlice = args->dstZ;
    dstView.numSlices = args->dstNumSlice;

    gcoOS_MemCopy(&blitArgs, args, sizeof(gcsSURF_BLIT_ARGS));

    srcSurf = args->srcSurface;
    dstSurf = args->dstSurface;

    /* MSAA surface should multiple samples.*/
    blitArgs.srcWidth  *= srcSurf->sampleInfo.x;
    blitArgs.srcHeight *= srcSurf->sampleInfo.y;
    blitArgs.srcX      *= srcSurf->sampleInfo.x;
    blitArgs.srcY      *= srcSurf->sampleInfo.y;

    blitArgs.dstWidth  *= dstSurf->sampleInfo.x;
    blitArgs.dstHeight *= dstSurf->sampleInfo.y;
    blitArgs.dstX      *= dstSurf->sampleInfo.x;
    blitArgs.dstY      *= dstSurf->sampleInfo.y;

    if (blitArgs.scissorTest)
    {
        scissorHeight = blitArgs.scissor.bottom - blitArgs.scissor.top;
        scissorWidth  = blitArgs.scissor.right - blitArgs.scissor.left;
        blitArgs.scissor.top    *= dstSurf->sampleInfo.y;
        blitArgs.scissor.left   *= dstSurf->sampleInfo.x;
        blitArgs.scissor.bottom  = scissorHeight * dstSurf->sampleInfo.y + blitArgs.scissor.top;
        blitArgs.scissor.right   = scissorWidth * dstSurf->sampleInfo.x + blitArgs.scissor.left;
    }

    /* If either the src or dst rect are negative */
    if (0 > blitArgs.srcWidth || 0 > blitArgs.srcHeight || 0 > blitArgs.srcDepth ||
        0 > blitArgs.dstWidth || 0 > blitArgs.dstHeight || 0 > blitArgs.dstDepth)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* If either the src or dst rect are zero, skip the blit */
    if (0 == blitArgs.srcWidth || 0 == blitArgs.srcHeight || 0 == blitArgs.srcDepth ||
        0 == blitArgs.dstWidth || 0 == blitArgs.dstHeight || 0 == blitArgs.dstDepth)
    {
        return gcvSTATUS_OK;
    }

    srcFmtInfo = &srcSurf->formatInfo;
    dstFmtInfo = &dstSurf->formatInfo;

    /* If either the src or dst rect has no intersection with the surface, skip the blit. */
    if (blitArgs.srcX + blitArgs.srcWidth  <= 0 || blitArgs.srcX >= (gctINT)(srcSurf->alignedW) ||
        blitArgs.srcY + blitArgs.srcHeight <= 0 || blitArgs.srcY >= (gctINT)(srcSurf->alignedH) ||
        blitArgs.srcZ + blitArgs.srcDepth  <= 0 || blitArgs.srcZ >= (gctINT)(srcSurf->requestD) ||
        blitArgs.dstX + blitArgs.dstWidth  <= 0 || blitArgs.dstX >= (gctINT)(dstSurf->alignedW) ||
        blitArgs.dstY + blitArgs.dstHeight <= 0 || blitArgs.dstY >= (gctINT)(dstSurf->alignedH) ||
        blitArgs.dstZ + blitArgs.dstDepth  <= 0 || blitArgs.dstZ >= (gctINT)(dstSurf->requestD))
    {
        return gcvSTATUS_OK;
    }

    /* Propagate canDropStencil flag to the destination surface */
    dstSurf->canDropStencilPlane = srcSurf->canDropStencilPlane;

    if (dstSurf->hzNode.valid)
    {
        /* Disable any HZ attached to destination. */
        dstSurf->hzDisabled = gcvTRUE;
    }

    /*
    ** For integer format upload/blit, the data type must be totally matched.
    ** And we should not do conversion to float per spec, or precision will be lost.
    */
    if (((srcFmtInfo->fmtDataType == gcvFORMAT_DATATYPE_UNSIGNED_INTEGER) ||
         (srcFmtInfo->fmtDataType == gcvFORMAT_DATATYPE_SIGNED_INTEGER)) &&
         (srcFmtInfo->fmtDataType != dstFmtInfo->fmtDataType))
    {
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* For (sign)integer format, just pick one of them.
    */
    if ((srcFmtInfo->fmtDataType == gcvFORMAT_DATATYPE_UNSIGNED_INTEGER) ||
        (srcFmtInfo->fmtDataType == gcvFORMAT_DATATYPE_SIGNED_INTEGER))
    {
        averagePixels = gcvFALSE;
    }


    pfReadPixel  = gcoSURF_GetReadPixelFunc(srcSurf);
    pfWritePixel = gcoSURF_GetWritePixelFunc(dstSurf);

    /* set color space conversion flag */
    if (srcSurf->colorSpace != dstSurf->colorSpace)
    {
        gcmASSERT(dstSurf->colorSpace != gcvSURF_COLOR_SPACE_UNKNOWN);

        if (srcSurf->colorSpace == gcvSURF_COLOR_SPACE_LINEAR)
        {
            colorSpaceConvert = gcdCOLOR_SPACE_CONVERSION_TO_NONLINEAR;
        }
        else if (srcSurf->colorSpace == gcvSURF_COLOR_SPACE_NONLINEAR)
        {
            colorSpaceConvert = gcdCOLOR_SPACE_CONVERSION_TO_LINEAR;
        }
        else
        {
            /* color space should NOT be gcvSURF_COLOR_SPACE_UNKNOWN */
            gcmASSERT(0);
        }
    }

    if (!pfReadPixel || !pfWritePixel)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Flush the GPU cache */
    gcmONERROR(gcoHARDWARE_FlushTileStatus(gcvNULL, &srcView, gcvTRUE));
    gcmONERROR(gcoHARDWARE_DisableTileStatus(gcvNULL, &dstView, gcvTRUE));

    gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));
    gcmONERROR(gcoHARDWARE_Commit(gcvNULL));
    gcmONERROR(gcoHARDWARE_Stall(gcvNULL));

    /* Lock the surfaces. */
    gcmONERROR(gcoSURF_Lock(srcSurf, gcvNULL, srcAddr));
    gcmONERROR(gcoSURF_Lock(dstSurf, gcvNULL, dstAddr));

    /* Src surface might be written by GPU previously, CPU need to invalidate
    ** its cache before reading.
    ** Dst surface alo need invalidate CPU cache to guarantee CPU cache is coherent
    ** with memory, so it's correct to flush out after writing.
    */
    gcmONERROR(gcoSURF_NODE_Cache(&srcSurf->node,
                                  srcAddr[0],
                                  srcSurf->size,
                                  gcvCACHE_INVALIDATE));
    gcmONERROR(gcoSURF_NODE_Cache(&dstSurf->node,
                                  dstAddr[0],
                                  dstSurf->size,
                                  gcvCACHE_INVALIDATE));

    xScale = blitArgs.srcWidth  / (gctFLOAT)blitArgs.dstWidth;
    yScale = blitArgs.srcHeight / (gctFLOAT)blitArgs.dstHeight;
    zScale = blitArgs.srcDepth  / (gctFLOAT)blitArgs.dstDepth;

    for (kDst = blitArgs.dstZ; kDst < blitArgs.dstZ + blitArgs.dstDepth; ++kDst)
    {
        gctINT kSrc;
        gctINT paceI, paceJ, paceK;
        gctINT paceImax, paceJmax, paceKmax;

        if (kDst < 0 || kDst >= (gctINT)dstSurf->requestD)
        {
            continue;
        }

        kSrc = blitArgs.srcZ + (gctINT)((kDst - blitArgs.dstZ) * zScale);

        if (kSrc < 0 || kSrc >= (gctINT)srcSurf->requestD)
        {
            continue;
        }

        for (jDst = blitArgs.dstY; jDst < blitArgs.dstY + blitArgs.dstHeight; ++jDst)
        {
            gctINT jSrc;

            if (jDst < 0 || jDst >= (gctINT)dstSurf->alignedH)
            {
                continue;
            }

            /* scissor test */
            if (blitArgs.scissorTest &&
                (jDst < blitArgs.scissor.top || jDst >= blitArgs.scissor.bottom))
            {
                continue;
            }

            jSrc = (gctINT)(((jDst - blitArgs.dstY) * yScale) + 0.49);

            if (jSrc > blitArgs.srcHeight - 1)
            {
                jSrc = blitArgs.srcHeight - 1;
            }

            if (blitArgs.yReverse)
            {
                jSrc = blitArgs.srcHeight - 1 - jSrc;
            }

            jSrc += blitArgs.srcY;

            if (jSrc < 0 || jSrc >= (gctINT)srcSurf->alignedH)
            {
                continue;
            }

            for (iDst = blitArgs.dstX; iDst < blitArgs.dstX + blitArgs.dstWidth; ++iDst)
            {
                gcsPIXEL internal;
                gcsPIXEL samplePixels[32];
                gctUINT sampleCount = 0;
                gctPOINTER srcAddr_l[gcdMAX_SURF_LAYERS] = {gcvNULL};
                gctPOINTER dstAddr_l[gcdMAX_SURF_LAYERS] = {gcvNULL};
                gctINT iSrc;

                if (iDst < 0 || iDst >= (gctINT)dstSurf->alignedW)
                {
                    continue;
                }

                /* scissor test */
                if (blitArgs.scissorTest &&
                    (iDst < blitArgs.scissor.left || iDst >= blitArgs.scissor.right))
                {
                    continue;
                }

                iSrc = (gctINT)(((iDst - blitArgs.dstX) * xScale) + 0.49);

                if (iSrc > blitArgs.srcWidth - 1)
                {
                    iSrc = blitArgs.srcWidth - 1;
                }

                if (blitArgs.xReverse)
                {
                    iSrc = blitArgs.srcWidth - 1 - iSrc;
                }

                iSrc += blitArgs.srcX;

                if (iSrc < 0 || iSrc >= (gctINT)srcSurf->alignedW)
                {
                    continue;
                }

                paceImax = (gctINT)(xScale + 0.5f) > 1 ? (gctINT)(xScale + 0.5f) : 1;
                paceJmax = (gctINT)(yScale + 0.5f) > 1 ? (gctINT)(yScale + 0.5f) : 1;
                paceKmax = (gctINT)(zScale + 0.5f) > 1 ? (gctINT)(zScale + 0.5f) : 1;

                for (paceK = 0; paceK < paceKmax; paceK++)
                {
                    for (paceJ = 0; paceJ < paceJmax; paceJ++)
                    {
                        for (paceI = 0; paceI < paceImax; paceI++)
                        {
                            gctINT sampleI = iSrc + paceI * (blitArgs.xReverse ? -1 : 1);
                            gctINT sampleJ = jSrc + paceJ * (blitArgs.yReverse ? -1 : 1);
                            gctINT sampleK = kSrc + paceK;

                            sampleI = gcmCLAMP(sampleI, 0, (gctINT)(srcSurf->alignedW - 1));
                            sampleJ = gcmCLAMP(sampleJ, 0, (gctINT)(srcSurf->alignedH - 1));
                            sampleK = gcmCLAMP(sampleK, 0, (gctINT)(srcSurf->requestD - 1));

                            srcSurf->pfGetAddr(srcSurf, (gctSIZE_T)sampleI, (gctSIZE_T)sampleJ, (gctSIZE_T)sampleK, srcAddr_l);

                            pfReadPixel(srcAddr_l, &samplePixels[sampleCount]);

                            if (colorSpaceConvert == gcdCOLOR_SPACE_CONVERSION_TO_LINEAR)
                            {
                                gcoSURF_PixelToLinear(&samplePixels[sampleCount]);
                            }
                            else if (colorSpaceConvert == gcdCOLOR_SPACE_CONVERSION_TO_NONLINEAR)
                            {
                                gcoSURF_PixelToNonLinear(&samplePixels[sampleCount]);

                            }
                            if (blitArgs.needDecode)
                            {
                                gcoSURF_PixelToLinear(&samplePixels[sampleCount]);
                            }

                            sampleCount++;
                        }
                    }
                }

                if ((sampleCount > 1) && averagePixels)
                {
                    _AveragePixels(samplePixels, sampleCount, srcFmtInfo->fmtDataType, &internal);
                }
                else
                {
                    internal = samplePixels[0];
                }

                dstSurf->pfGetAddr(dstSurf, (gctSIZE_T)iDst, (gctSIZE_T)jDst, (gctSIZE_T)kDst, dstAddr_l);

                if (blitArgs.needDecode)
                {
                    gcoSURF_PixelToNonLinear(&internal);
                }

                pfWritePixel(&internal, dstAddr_l, args->flags);

            }
        }
    }

    /* Dst surface was written by CPU and might be accessed by GPU later */
    gcmONERROR(gcoSURF_NODE_Cache(&dstSurf->node,
                                  dstAddr[0],
                                  dstSurf->size,
                                  gcvCACHE_CLEAN));

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_PE_DITHER_FIX) == gcvFALSE &&
        args->flags == 0 && /* Full mask overwritten */
        args->dstX == 0 && args->dstWidth  >= (gctINT)dstSurf->requestW &&
        args->dstY == 0 && args->dstHeight >= (gctINT)dstSurf->requestH)
    {
        dstSurf->deferDither3D = gcvFALSE;
    }

    if (dstSurf->paddingFormat &&
        args->dstX == 0 && args->dstWidth  >= (gctINT)dstSurf->requestW &&
        args->dstY == 0 && args->dstHeight >= (gctINT)dstSurf->requestH)
    {
        dstSurf->garbagePadded = gcvFALSE;
    }

#if gcdDUMP

    if (gcvSURF_BITMAP != srcSurf->type)
    {
        gcmDUMP(gcvNULL, "#[info: verify BlitCPU source]");
        /* verify the source */
        gcmDUMP_BUFFER(gcvNULL,
                       gcvDUMP_BUFFER_VERIFY,
                       gcsSURF_NODE_GetHWAddress(&srcSurf->node),
                       srcSurf->node.logical,
                       0,
                       srcSurf->size);

    }
    /* upload the destination */
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_MEMORY,
                   gcsSURF_NODE_GetHWAddress(&dstSurf->node),
                   dstSurf->node.logical,
                   0,
                   dstSurf->size);
#endif

OnError:
    /* Unlock the surfaces. */
    if (srcAddr[0])
    {
        gcoSURF_Unlock(srcSurf, srcAddr[0]);
    }
    if (dstAddr[0])
    {
        gcoSURF_Unlock(dstSurf, dstAddr[0]);
    }

    return status;
}


gceSTATUS
gcoSURF_DrawBlit(
    gcsSURF_VIEW *SrcView,
    gcsSURF_VIEW *DstView,
    gscSURF_BLITDRAW_BLIT *Args
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("SrcView=0x%x DstView=0x%x Args=0x%x", SrcView, DstView, Args);

    status = gcoHARDWARE_DrawBlit(SrcView, DstView, Args);

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoSURF_Preserve(
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf,
    IN gcsRECT_PTR MaskRect
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsRECT rects[4];
    gctINT  count = 0;
    gctINT  width, height;
    gctUINT resolveAlignmentX = 0;
    gctUINT resolveAlignmentY = 0;
    gctUINT tileAlignmentX    = 0;
    gctUINT tileAlignmentY    = 0;
    gcsSURF_VIEW srcView = {SrcSurf, 0, 1};
    gcsSURF_VIEW dstView = {DstSurf, 0, 1};

    gcmHEADER_ARG("SrcSurf=0x%x DstSurf=0x%x MaskRect=0x%x",
                  SrcSurf, DstSurf, MaskRect);

    gcmASSERT(!(DstSurf->flags & gcvSURF_FLAG_CONTENT_UPDATED));

    /* Get surface size. */
    width  = DstSurf->requestW;
    height = DstSurf->requestH;

    if ((MaskRect != gcvNULL) &&
        (MaskRect->left   <= 0) &&
        (MaskRect->top    <= 0) &&
        (MaskRect->right  >= (gctINT) width) &&
        (MaskRect->bottom >= (gctINT) height))
    {
        gcmFOOTER_NO();
        /* Full screen clear. No copy. */
        return gcvSTATUS_OK;
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_BLT_ENGINE))
    {
        /* Pixel alignment in 3DBlit engine. */
        tileAlignmentX    = tileAlignmentY    =
        resolveAlignmentX = resolveAlignmentY = 1;
    }
    else
    {
        /* Query surface resolve alignment parameters. */
        gcmONERROR(
            gcoHARDWARE_GetSurfaceResolveAlignment(gcvNULL,
                                                   DstSurf,
                                                   &tileAlignmentX,
                                                   &tileAlignmentY,
                                                   &resolveAlignmentX,
                                                   &resolveAlignmentY));
    }

    if ((MaskRect == gcvNULL) ||
        (MaskRect->left == MaskRect->right) ||
        (MaskRect->top  == MaskRect->bottom))
    {
        /* Zarro clear rect, need copy full surface. */
        rects[0].left   = 0;
        rects[0].top    = 0;
        rects[0].right  = gcmALIGN(width,  resolveAlignmentX);
        rects[0].bottom = gcmALIGN(height, resolveAlignmentY);
        count = 1;
    }
    else
    {
        gcsRECT maskRect;

        if (DstSurf->flags & gcvSURF_FLAG_CONTENT_YINVERTED)
        {
            /* Y inverted content. */
            maskRect.left   = MaskRect->left;
            maskRect.top    = height - MaskRect->bottom;
            maskRect.right  = MaskRect->right;
            maskRect.bottom = height - MaskRect->top;
        }
        else
        {
            maskRect = *MaskRect;
        }

        /* Avoid right,bottom coordinate exceeding surface boundary. */
        if (tileAlignmentX < resolveAlignmentX)
        {
            tileAlignmentX = resolveAlignmentX;
        }

        if (tileAlignmentY < resolveAlignmentY)
        {
            tileAlignmentY = resolveAlignmentY;
        }

        /*
         *  +------------------------+
         *  |                        |
         *  |           r1           |
         *  |                        |
         *  |......+----------+......|
         *  |      |          |      |
         *  |  r0  |  mask    |  r2  |
         *  |      |   rect   |      |
         *  |......+----------+......|
         *  |                        |
         *  |           r3           |
         *  |                        |
         *  +------------------------+
         */

        /* Get real size of clear  */
        if (maskRect.left > 0)
        {
            rects[count].left   = 0;
            rects[count].top    = gcmALIGN_BASE(maskRect.top, tileAlignmentY);
            rects[count].right  = gcmALIGN(maskRect.left, resolveAlignmentX);
            rects[count].bottom = rects[count].top + gcmALIGN(maskRect.bottom - rects[count].top, resolveAlignmentY);
            count++;
        }

        if (maskRect.top > 0)
        {
            rects[count].left   = 0;
            rects[count].top    = 0;
            rects[count].right  = gcmALIGN(width, resolveAlignmentX);
            rects[count].bottom = gcmALIGN(maskRect.top, resolveAlignmentY);
            count++;
        }

        if (maskRect.right < width)
        {
            rects[count].left   = gcmALIGN_BASE(maskRect.right, tileAlignmentX);
            rects[count].top    = gcmALIGN_BASE(maskRect.top,   tileAlignmentY);
            rects[count].right  = rects[count].left + gcmALIGN(width - rects[count].left, resolveAlignmentX);
            rects[count].bottom = rects[count].top  + gcmALIGN(maskRect.bottom - rects[count].top, resolveAlignmentY);
            count++;
        }

        if (maskRect.bottom < height)
        {
            rects[count].left   = 0;
            rects[count].top    = gcmALIGN_BASE(maskRect.bottom, tileAlignmentY);
            rects[count].right  = gcmALIGN(width, resolveAlignmentX);
            rects[count].bottom = rects[count].top + gcmALIGN(height - rects[count].top, resolveAlignmentY);
            count++;
        }
    }

    /* Preserve calculated rects. */
    gcmONERROR(
        gcoHARDWARE_PreserveRects(gcvNULL,
                                  &srcView,
                                  &dstView,
                                  rects,
                                  count));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  depr_gcoSURF_Resolve
**
**  Resolve the surface to the frame buffer.  Resolve means that the frame is
**  finished and should be displayed into the frame buffer, either by copying
**  the data or by flipping to the surface, depending on the hardware's
**  capabilities.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DstSurface
**          Pointer to a gcoSURF object that represents the destination surface
**          to resolve into, or gcvNULL if the resolve surface is
**          a physical address.
**
**      gctUINT32 DstAddress
**          Physical address of the destination surface.
**
**      gctPOINTER DstBits
**          Logical address of the destination surface.
**
**      gctINT DstStride
**          Stride of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_TYPE DstType
**          Type of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DstWidth
**          Width of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DstHeight
**          Height of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
depr_gcoSURF_Resolve(
    IN gcoSURF SrcSurface,
    IN gcoSURF DstSurface,
    IN gctUINT32 DstAddress,
    IN gctPOINTER DstBits,
    IN gctINT DstStride,
    IN gceSURF_TYPE DstType,
    IN gceSURF_FORMAT DstFormat,
    IN gctUINT DstWidth,
    IN gctUINT DstHeight
    )
{
    gcsPOINT rectOrigin;
    gcsPOINT rectSize;
    gceSTATUS status;

    gcmHEADER_ARG("SrcSurface=0x%x DstSurface=0x%x DstAddress=%08x DstBits=0x%x "
              "DstStride=%d DstType=%d DstFormat=%d DstWidth=%u "
              "DstHeight=%u",
              SrcSurface, DstSurface, DstAddress, DstBits, DstStride,
              DstType, DstFormat, DstWidth, DstHeight);

    /* Validate the source surface. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);

    /* Fill in coordinates. */
    rectOrigin.x = 0;
    rectOrigin.y = 0;

    if (DstSurface == gcvNULL)
    {
        rectSize.x = DstWidth;
        rectSize.y = DstHeight;
    }
    else
    {
        rectSize.x = DstSurface->alignedW;
        rectSize.y = DstSurface->alignedH;
    }

    /* Call generic function. */
    status = depr_gcoSURF_ResolveRect(
        SrcSurface, DstSurface,
        DstAddress, DstBits, DstStride, DstType, DstFormat,
        DstWidth, DstHeight,
        &rectOrigin, &rectOrigin, &rectSize
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS
_WrapUserMemory(
    IN gctPOINTER Memory,
    IN gctSIZE_T Size,
    OUT gctPOINTER * Info,
    OUT gctUINT32_PTR Address
    )
{
    gceSTATUS status;
    gcsUSER_MEMORY_DESC desc;
    gctUINT32 node = 0;

    gcoOS_ZeroMemory(&desc, gcmSIZEOF(desc));

    desc.flag    = gcvALLOC_FLAG_USERMEMORY;
    desc.logical = gcmPTR_TO_UINT64(Memory);
    desc.size    = (gctUINT32)Size;
    desc.physical = ~0ULL;

    gcmONERROR(gcoHAL_WrapUserMemory(&desc, gcvVIDMEM_TYPE_BITMAP, &node));

    gcmONERROR(gcoHAL_LockVideoMemory(node, gcvFALSE, gcvENGINE_RENDER, Address, gcvNULL));

    *Info = (gctPOINTER)(gctUINTPTR_T)node;

    return gcvSTATUS_OK;

OnError:
    if (node)
    {
        gcmVERIFY_OK(gcoHAL_ReleaseVideoMemory(node));
    }

    return status;
}

/*******************************************************************************
**
**  depr_gcoSURF_ResolveRect
**
**  Resolve a rectangular area of a surface to another surface.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DstSurface
**          Pointer to a gcoSURF object that represents the destination surface
**          to resolve into, or gcvNULL if the resolve surface is
**          a physical address.
**
**      gctUINT32 DstAddress
**          Physical address of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gctPOINTER DstBits
**          Logical address of the destination surface.
**
**      gctINT DstStride
**          Stride of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_TYPE DstType
**          Type of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_FORMAT DstFormat
**          Format of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DstWidth
**          Width of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DstHeight
**          Height of the destination surface.
**          If 'DstSurface' is not gcvNULL, this parameter is ignored.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be resolved.
**
**      gcsPOINT_PTR DstOrigin
**          The origin of the destination area to be resolved.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
depr_gcoSURF_ResolveRect(
    IN gcoSURF SrcSurface,
    IN gcoSURF DstSurface,
    IN gctUINT32 DstAddress,
    IN gctPOINTER DstBits,
    IN gctINT DstStride,
    IN gceSURF_TYPE DstType,
    IN gceSURF_FORMAT DstFormat,
    IN gctUINT DstWidth,
    IN gctUINT DstHeight,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DstOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gctPOINTER destination[3] = {gcvNULL};
    gctPOINTER mapInfo = gcvNULL;
    gcoSURF dstSurf;
    struct _gcoSURF wrapSurf;
    gctUINT32 address;

    gcmHEADER_ARG("SrcSurface=0x%x DstSurface=0x%x DstAddress=%08x DstBits=0x%x "
              "DstStride=%d DstType=%d DstFormat=%d DstWidth=%u "
              "DstHeight=%u SrcOrigin=0x%x DstOrigin=0x%x RectSize=0x%x",
              SrcSurface, DstSurface, DstAddress, DstBits, DstStride,
              DstType, DstFormat, DstWidth, SrcOrigin, DstOrigin,
              RectSize);

    /* Validate the source surface. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);

    do
    {
        gcsSURF_VIEW srcView = {SrcSurface, 0, 1};
        gcsSURF_VIEW dstView = {DstSurface, 0, 1};
        gcsSURF_RESOLVE_ARGS rlvArgs = {0};

        /* Destination surface specified? */
        if (DstSurface != gcvNULL)
        {
            /* Set the pointer to the structure. */
            dstSurf = DstSurface;

            /* Lock the surface. */
            if (DstBits == gcvNULL)
            {
                gcmERR_BREAK(
                    gcoSURF_Lock(DstSurface,
                                 gcvNULL,
                                 destination));
            }
        }

        /* Create a surface wrapper. */
        else
        {
            gcoOS_ZeroMemory(&wrapSurf, gcmSIZEOF(wrapSurf));

            dstSurf = &wrapSurf;
            /* Fill the surface info structure. */
            dstSurf->type          = DstType;
            dstSurf->format        = DstFormat;
            dstSurf->requestW      = DstWidth;
            dstSurf->requestH      = DstHeight;
            dstSurf->requestD      = 1;
            dstSurf->allocedW      = DstWidth;
            dstSurf->allocedH      = DstHeight;
            dstSurf->alignedW      = DstWidth;
            dstSurf->alignedH      = DstHeight;
            dstSurf->rotation      = gcvSURF_0_DEGREE;
            dstSurf->orientation   = gcvORIENTATION_TOP_BOTTOM;
            dstSurf->stride        = DstStride;
            dstSurf->sliceSize     =
            dstSurf->layerSize     =
            dstSurf->size          = DstStride * DstHeight;
            dstSurf->node.valid    = gcvTRUE;
            dstSurf->node.pool     = gcvPOOL_UNKNOWN;
            gcsSURF_NODE_SetHardwareAddress(&(dstSurf->node), DstAddress);
            dstSurf->node.logical  = DstBits;
            dstSurf->sampleInfo    = g_sampleInfos[1];
            dstSurf->isMsaa        = gcvFALSE;
            dstSurf->colorSpace    = gcd_QUERY_COLOR_SPACE(DstFormat);

            gcmERR_BREAK(gcoHARDWARE_ConvertFormat(DstFormat, &dstSurf->bitsPerPixel, gcvNULL));
            gcmERR_BREAK(
                gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                                  DstType,
                                                  0,
                                                  DstFormat,
                                                  &dstSurf->alignedW,
                                                  &dstSurf->alignedH,
                                                  1,
                                                  &dstSurf->tiling,
                                                  &dstSurf->superTiled,
                                                  &dstSurf->hAlignment));

            /* Map the user memory. */
            if (DstBits != gcvNULL)
            {
                gcmERR_BREAK(
                    _WrapUserMemory(DstBits,
                                   dstSurf->size,
                                   &mapInfo,
                                   &address));

                gcsSURF_NODE_SetHardwareAddress(&dstSurf->node, address);
            }

            dstSurf->pfGetAddr = gcoHARDWARE_GetProcCalcPixelAddr(gcvNULL, dstSurf);
            dstView.surf = &wrapSurf;
        }

        rlvArgs.version = gcvHAL_ARG_VERSION_V2;
        rlvArgs.uArgs.v2.numSlices = 1;
        rlvArgs.uArgs.v2.srcOrigin = *SrcOrigin;
        rlvArgs.uArgs.v2.dstOrigin = *DstOrigin;

        /* Determine the resolve size. */
        if ((DstOrigin->x == 0)
        &&  (DstOrigin->y == 0)
        &&  (RectSize->x == (gctINT)dstSurf->requestW)
        &&  (RectSize->y == (gctINT)dstSurf->requestH)
        )
        {
            /* Full destination resolve, a special case. */
            rlvArgs.uArgs.v2.rectSize.x = dstSurf->alignedW;
            rlvArgs.uArgs.v2.rectSize.y = dstSurf->alignedH;
        }
        else
        {
            rlvArgs.uArgs.v2.rectSize.x = RectSize->x;
            rlvArgs.uArgs.v2.rectSize.y = RectSize->y;
        }

        /* Perform resolve. */
        gcmERR_BREAK(gcoHARDWARE_ResolveRect(gcvNULL, &srcView, &dstView, &rlvArgs));
    }
    while (gcvFALSE);

    /* Unlock the surface. */
    if ((destination[0] != gcvNULL) && DstSurface)
    {
        /* Unlock the resolve buffer. */
        gcmVERIFY_OK(
            gcoSURF_Unlock(DstSurface,
                           destination[0]));
    }

    /* Unmap the surface. */
    if (mapInfo != gcvNULL)
    {
        gcmGETHARDWAREADDRESS(dstSurf->node, address);

        /* Unmap the user memory. */
        gcmVERIFY_OK(
            gcoHARDWARE_ScheduleVideoMemory((gctUINT32)gcmPTR_TO_UINT64(mapInfo)));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif

gceSTATUS
gcoSURF_ResetSurWH(
    IN gcoSURF Surface,
    IN gctUINT oriw,
    IN gctUINT orih,
    IN gctUINT alignw,
    IN gctUINT alignh,
    IN gceSURF_FORMAT fmt
    )
{
    gceSTATUS status;
    Surface->requestW = oriw;
    Surface->requestH = orih;
    Surface->requestD = 1;
    Surface->allocedW = oriw;
    Surface->allocedH = orih;
    Surface->alignedW = alignw;
    Surface->alignedH = alignh;

    gcmONERROR(gcoHARDWARE_ConvertFormat(
                          fmt,
                          (gctUINT32_PTR)&Surface->bitsPerPixel,
                          gcvNULL));

    /* Compute surface placement parameters. */
    _ComputeSurfacePlacement(Surface, gcvTRUE);

    Surface->layerSize = Surface->sliceSize * Surface->requestD;

    gcmASSERT(Surface->formatInfo.layers == 1);

    Surface->size = Surface->layerSize * Surface->formatInfo.layers;

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;

}

/*******************************************************************************
**
**  gcoSURF_UpdateTimeStamp
**
**  Increase timestamp of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_UpdateTimeStamp(
    IN gcoSURF Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Surface=0x%X", Surface);
    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    /* Increase timestamp value. */
    Surface->timeStamp++;

    gcmFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_UpdateTimeStamp
**
**  Query timestamp of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT64 * TimeStamp
**          Pointer to hold the timestamp. Can not be null.
*/
gceSTATUS
gcoSURF_QueryTimeStamp(
    IN gcoSURF Surface,
    OUT gctUINT64 * TimeStamp
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Surface=0x%x", Surface);
    gcmVERIFY_ARGUMENT(Surface != gcvNULL);
    gcmVERIFY_ARGUMENT(TimeStamp != gcvNULL);

    /* Increase timestamp value. */
    *TimeStamp = Surface->timeStamp;

    gcmFOOTER_ARG("*TimeStamp=%lld", *TimeStamp);
    return status;
}

/*******************************************************************************
**
**  gcoSURF_AllocShBuffer
**
**  Allocate shared buffer (gctSHBUF) for this surface, so that its shared
**  states can be accessed across processes.
**
**  Shared buffer is freed when surface is destroyed.
**
**  Usage:
**  1. Process (A) constructed a surface (a) which is to be used by other
**     processes such as process (B).
**
**  2. Process (A) need alloc ShBuf (s) by gcoSURF_AllocShBuffer for surface
**     (a) if (a) need shared its states to other processes.
**
**  3. Process (B) need get surface node and other information of surface (a)
**     includes the ShBuf handle by some IPC method (such as android
**     Binder mechanism). So process (B) wrapps it as surface (b).
**
**  4. Process (B) need call gcoSURF_BindShBuffer on surface (b) with ShBuf (s)
**     to connect.
**
**  5. Processes can then call gcoSURF_PushSharedInfo/gcoSURF_PopSharedInfo to
**     shared states.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      gctSHBUF * ShBuf
**          Pointer to hold shared buffer handle.
*/
gceSTATUS
gcoSURF_AllocShBuffer(
    IN gcoSURF Surface,
    OUT gctSHBUF * ShBuf
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x ShBuf=%d",
                  Surface, (gctUINT32) (gctUINTPTR_T) ShBuf);

    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    if (Surface->shBuf == gcvNULL)
    {
        /* Create ShBuf. */
        gcmONERROR(
            gcoHAL_CreateShBuffer(sizeof (gcsSURF_SHARED_INFO),
                                  &Surface->shBuf));
    }

    /* Returns shared buffer handle. */
    *ShBuf = Surface->shBuf;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_BindShBuffer
**
**  Bind surface to a shared buffer. The share buffer should be allocated by
**  gcoSURF_AllocShBuffer.
**
**  See gcoSURF_AllocShBuffer.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      gctSHBUF ShBuf
**          Pointer shared buffer handle to connect to.
*/
gceSTATUS
gcoSURF_BindShBuffer(
    IN gcoSURF Surface,
    OUT gctSHBUF ShBuf
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x ShBuf=%d",
                  Surface, (gctUINT32) (gctUINTPTR_T) ShBuf);

    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    if (Surface->shBuf == gcvNULL)
    {
        /* Map/reference ShBuf. */
        gcmONERROR(gcoHAL_MapShBuffer(ShBuf));
        Surface->shBuf = ShBuf;
    }
    else
    {
        /* Already has a ShBuf. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_PushSharedInfo
**
**  Push surface shared states to shared buffer. Shared buffer should be
**  initialized before this function either by gcoSURF_AllocShBuffer or
**  gcoSURF_BindShBuffer. gcvSTATUS_NOT_SUPPORTED if not ever bound.
**
**  See gcoSURF_AllocShBuffer.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_PushSharedInfo(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;
    gcsSURF_SHARED_INFO info;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    if (Surface->shBuf == gcvNULL)
    {
        /* No shared buffer bound. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Gather states. */
    info.magic              = gcvSURF_SHARED_INFO_MAGIC;
    info.timeStamp          = Surface->timeStamp;

#if gcdENABLE_3D
    info.tileStatusDisabled = Surface->tileStatusDisabled[0];
    info.dirty              = Surface->dirty[0];
    info.fcValue            = Surface->fcValue[0];
    info.fcValueUpper       = Surface->fcValueUpper[0];
    info.compressed         = Surface->compressed;
#endif

    /* Put structure to shared buffer object. */
    gcmONERROR(
        gcoHAL_WriteShBuffer(Surface->shBuf,
                             &info,
                             sizeof (gcsSURF_SHARED_INFO)));

    /* Success. */
    gcmFOOTER_NO();
    return status;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_PopSharedInfo
**
**  Pop surface shared states from shared buffer. Shared buffer must be
**  initialized before this function either by gcoSURF_AllocShBuffer or
**  gcoSURF_BindShBuffer. gcvSTATUS_NOT_SUPPORTED if not ever bound.
**
**  Before sync shared states to this surface, timestamp is checked. If
**  timestamp is not newer than current, sync is discard and gcvSTATUS_SKIP
**  is returned.
**
**  See gcoSURF_AllocShBuffer.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_PopSharedInfo(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;
    gcsSURF_SHARED_INFO info;
    gctUINT32 size = sizeof (gcsSURF_SHARED_INFO);
    gctUINT32 bytesRead = 0;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    if (Surface->shBuf == gcvNULL)
    {
        /* No shared buffer bound. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Get structure from shared buffer object. */
    gcmONERROR(
        gcoHAL_ReadShBuffer(Surface->shBuf,
                            &info,
                            size,
                            &bytesRead));

    if (status == gcvSTATUS_SKIP)
    {
        /* No data in shared buffer. */
        goto OnError;
    }

    /* Check magic. */
    if ((info.magic != gcvSURF_SHARED_INFO_MAGIC) ||
        (bytesRead  != sizeof (gcsSURF_SHARED_INFO)))
    {
        /* Magic mismatch. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Check time stamp. */
    if (info.timeStamp <= Surface->timeStamp)
    {
        status = gcvSTATUS_SKIP;
        goto OnError;
    }

    /* Update surface states. */
    Surface->timeStamp          = info.timeStamp;

#if gcdENABLE_3D
    Surface->tileStatusDisabled[0] = info.tileStatusDisabled;
    Surface->dirty[0]              = info.dirty;
    Surface->fcValue[0]            = info.fcValue;
    Surface->fcValueUpper[0]       = info.fcValueUpper;
    Surface->compressed         = info.compressed;
#endif

    /* Success. */
    gcmFOOTER_NO();
    return status;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcsSURF_NODE_Construct(
    IN gcsSURF_NODE_PTR Node,
    IN gctSIZE_T Bytes,
    IN gctUINT Alignment,
    IN gceSURF_TYPE Type,
    IN gctUINT32 Flag,
    IN gcePOOL Pool
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface = {0};
    struct _gcsHAL_ALLOCATE_LINEAR_VIDEO_MEMORY * alvm
        = (struct _gcsHAL_ALLOCATE_LINEAR_VIDEO_MEMORY *) &iface.u;
    gctUINT i;
#if gcdENABLE_3D
    gctBOOL bForceVirtual = gcvFALSE;
    gceHARDWARE_TYPE type;
#endif

    /* Need refine this function to pass the internal and external SRAM index. */
    gctINT32 sRAMIndex = 0;
    gctINT32 extSRAMIndex = 0;

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_SPECIFY_POOL
    static gcePOOL poolPerType[gcvSURF_NUM_TYPES] =
    {
        /* gcvSURF_TYPE_UNKNOWN. */
        gcvPOOL_DEFAULT,
        /* gcvSURF_INDEX */
        gcvPOOL_DEFAULT,
        /* gcvSURF_VERTEX */
        gcvPOOL_DEFAULT,
        /* gcvSURF_TEXTURE */
        gcvPOOL_DEFAULT,
        /* gcvSURF_RENDER_TARGET */
        gcvPOOL_DEFAULT,
        /* gcvSURF_DEPTH */
        gcvPOOL_DEFAULT,
        /* gcvSURF_BITMAP */
        gcvPOOL_DEFAULT,
        /* gcvSURF_TILE_STATUS */
        gcvPOOL_DEFAULT,
        /* gcvSURF_IMAGE */
        gcvPOOL_DEFAULT,
        /* gcvSURF_MASK */
        gcvPOOL_DEFAULT,
        /* gcvSURF_SCISSOR */
        gcvPOOL_DEFAULT,
        /* gcvSURF_HIERARCHICAL_DEPTH */
        gcvPOOL_DEFAULT,
    };
#endif

    gcmHEADER_ARG("Node=%p, Bytes=%u, Alignement=%d, Type=%d, Flag=%d, Pool=%d",
                  Node, Bytes, Alignment, Type, Flag, Pool);

#ifdef LINUX
#ifndef ANDROID
#if gcdENABLE_3D
    gcePATCH_ID patchID = gcvPATCH_INVALID;
    gcoHAL_GetPatchID(gcvNULL, &patchID);

    if (gcdPROC_IS_WEBGL(patchID))
    {
        Flag |= gcvALLOC_FLAG_MEMLIMIT;
    }
#endif
#endif
#endif

    /* Reset CPU write flag, since there is flush when unlock in kernel.*/
    Node->bCPUWrite = gcvFALSE;

#if gcdENABLE_3D
    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &type));

    if (type == gcvHARDWARE_3D)
    {
        gcoHARDWARE_GetForceVirtual(gcvNULL, &bForceVirtual);

        if ((Type == gcvSURF_INDEX || Type == gcvSURF_VERTEX) &&
            !gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_MIXED_STREAMS) &&
            bForceVirtual)
        {
            Pool = gcvPOOL_VIRTUAL;
        }
    }
#endif

    iface.command   = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;

    alvm->bytes = Bytes;

    alvm->alignment = Alignment;
    alvm->type      = Type;
    alvm->pool      = Pool;
    alvm->flag      = Flag;

    alvm->sRAMIndex    = (Pool == gcvPOOL_INTERNAL_SRAM) ? sRAMIndex : -1;
    alvm->extSRAMIndex = (Pool == gcvPOOL_EXTERNAL_SRAM) ? extSRAMIndex : -1;

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_SPECIFY_POOL
    if (Pool == gcvPOOL_DEFAULT)
    {
        /* If pool is not specified, tune it to debug value now. */
        alvm->pool  = poolPerType[Type & 0xF];
    }
#endif

    gcoOS_ZeroMemory(Node, gcmSIZEOF(gcsSURF_NODE));

    if (alvm->bytes > 0)
    {
        gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

        Node->u.normal.node = alvm->node;
        Node->pool          = alvm->pool;
        Node->size          = (gctSIZE_T)alvm->bytes;
    }
    else
    {
        Node->u.normal.node = 0;
        Node->pool          = gcvPOOL_UNKNOWN;
        Node->size          = 0;
    }

    Node->physical2     = ~0U;
    Node->physical3     = ~0U;

    for (i = 0; i < gcvHARDWARE_NUM_TYPES; i++)
    {
        Node->hardwareAddresses[i] = ~0U;
    }

#if gcdGC355_MEM_PRINT
    gcoOS_AddRecordAllocation((gctINT32)Node->size);
#endif

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcsSURF_NODE_Destroy(
    IN gcsSURF_NODE_PTR Node
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Node=0x%x", Node);
#if gcdSYNC
    {
        gcsSYNC_CONTEXT_PTR ptr = Node->fenceCtx;

        while(ptr)
        {
            Node->fenceCtx = ptr->next;

            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL,ptr));

            ptr = Node->fenceCtx;
        }
    }
#endif

    status = gcoHARDWARE_ScheduleVideoMemory(Node->u.normal.node);

    /* Reset the node. */
    Node->pool  = gcvPOOL_UNKNOWN;
    Node->valid = gcvFALSE;

    gcmFOOTER();
    return status;
}

gceSTATUS
gcsSURF_NODE_Lock(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE engine,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    return gcoHARDWARE_LockEx(Node, engine, Address, Memory);
}

gceSTATUS
gcsSURF_NODE_Unlock(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE engine
    )
{
    return gcoHARDWARE_UnlockEx(Node, engine, gcvSURF_TYPE_UNKNOWN);
}

#if gcdENABLE_3D
gceSTATUS
gcsSURF_NODE_GetFence(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE engine,
    IN gceFENCE_TYPE Type
)
{
#if gcdSYNC
    if (!gcoHAL_GetOption(gcvNULL, gcvOPTION_KERNEL_FENCE))
    {
         if (Node)
         {
            gctBOOL fenceEnable;

            gcoHARDWARE_GetFenceEnabled(gcvNULL, &fenceEnable);

            if(fenceEnable)
            {
                gcoHARDWARE_GetFence(gcvNULL, &Node->fenceCtx, engine, Type);
                Node->fenceStatus = gcvFENCE_ENABLE;
            }
            else
            {
                Node->fenceStatus = gcvFENCE_GET;
            }
         }
    }
#endif
    return gcvSTATUS_OK;
}

gceSTATUS
gcsSURF_NODE_WaitFence(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE From,
    IN gceENGINE On,
    IN gceFENCE_TYPE Type
)
{
    gceSTATUS status = gcvSTATUS_OK;
#if gcdSYNC
    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_KERNEL_FENCE))
    {
        gcoHAL_WaitFence(Node->u.normal.node, gcvINFINITE);
    }
    else if (Node)
    {
        gctBOOL fenceEnable;

        gcoHARDWARE_GetFenceEnabled(gcvNULL, &fenceEnable);

        if(fenceEnable)
        {
            gcoHARDWARE_WaitFence(gcvNULL, Node->fenceCtx, From, On, Type);
        }
        else
        {
            if(Node->fenceStatus == gcvFENCE_GET)
            {
                Node->fenceStatus = gcvFENCE_ENABLE;
                gcoHARDWARE_SetFenceEnabled(gcvNULL, gcvTRUE);
                gcmONERROR(gcoHAL_Commit(gcvNULL, gcvTRUE));
            }
        }

    }
OnError:
#endif
    return status;
}

gceSTATUS
gcsSURF_NODE_IsFenceEnabled(
    IN gcsSURF_NODE_PTR Node
    )
{
#if gcdSYNC
    gceSTATUS status = gcvSTATUS_FALSE;

    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_KERNEL_FENCE))
    {
        return gcvSTATUS_TRUE;
    }
    else if (Node)
    {
        return (Node->fenceStatus != gcvFENCE_DISABLE) ?
                gcvSTATUS_TRUE : gcvSTATUS_FALSE;
    }
    return status;

#else
    return gcvSTATUS_FALSE;
#endif
}
#endif

gceSTATUS
gcsSURF_NODE_SetHardwareAddress(
    IN gcsSURF_NODE_PTR Node,
    IN gctUINT32 Address
    )
{
    gceHARDWARE_TYPE type;

    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &type));

    gcmASSERT(type != gcvHARDWARE_INVALID);

    Node->hardwareAddresses[type] = Address;

    return gcvSTATUS_OK;
}

gceSTATUS
gcsSURF_NODE_GetHardwareAddress(
    IN gcsSURF_NODE_PTR Node,
    OUT gctUINT32_PTR Physical,
    OUT gctUINT32_PTR Physical2,
    OUT gctUINT32_PTR Physical3,
    OUT gctUINT32_PTR PhysicalBottom
    )
{
    gceHARDWARE_TYPE type;

    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &type));

    gcmASSERT(type != gcvHARDWARE_INVALID);

    if (Physical != gcvNULL)
    {
        *Physical = Node->hardwareAddresses[type];
    }

    if (PhysicalBottom != gcvNULL)
    {
        *PhysicalBottom = Node->hardwareAddressesBottom[type];
    }

    return gcvSTATUS_OK;
}

gctUINT32
gcsSURF_NODE_GetHWAddress(
    IN gcsSURF_NODE_PTR Node
    )
{
    gceHARDWARE_TYPE type;

    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &type));

    gcmASSERT(type != gcvHARDWARE_INVALID);

    return Node->hardwareAddresses[type];
}

gceSTATUS
gcoSURF_WrapUserMemory(
    IN gcoHAL Hal,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Stride,
    IN gctUINT Depth,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Handle,
    IN gctUINT32 Flag,
    OUT gcoSURF * Surface
    )
{
    gceSTATUS status;
    gcoSURF surface = gcvNULL;
    gctUINT32 node;
    gceSURF_TYPE type;
    gcsUSER_MEMORY_DESC desc;

    gcmHEADER_ARG("Handle=%d, Flag=%d", Handle, Flag);

    /* Create a no video memory node surface. */
    type  = Type | gcvSURF_NO_VIDMEM;

    gcmONERROR(gcoSURF_Construct(
        gcvNULL, Width, Height, Depth, type, Format, gcvPOOL_VIRTUAL, &surface));

    /* Compute the HAL required stride */
    _ComputeSurfacePlacement(surface, gcvTRUE);
    if (surface->type == gcvSURF_BITMAP)
    {
        /* For bitmap surface, user buffer stride may be larger than
        ** least stride required in HAL.
        */
        if (Stride < surface->stride)
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }
    else
    {
        /* For Vivante internal surfaces types, user buffer placement
        ** must be the same as what defined in HAL, otherwise the user
        ** buffer is not compatible with HAL.
        */
        if (Stride != surface->stride)
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    /* Compute surface placement with user stride */
    surface->stride = Stride;
    _ComputeSurfacePlacement(surface, gcvFALSE);

    surface->layerSize = surface->sliceSize * surface->requestD;

    /* Always single layer for user surface */
    gcmASSERT(surface->formatInfo.layers == 1);

    surface->size = surface->layerSize * surface->formatInfo.layers;

    desc.flag = Flag;
    desc.handle = Handle;

    /* Wrap user memory to a video memory node. */
    gcmONERROR(gcoHAL_WrapUserMemory(&desc, (gceVIDMEM_TYPE)surface->type, &node));

    /* Import wrapped video memory node to the surface. */
    surface->node.u.normal.node = node;
    surface->node.pool          = gcvPOOL_VIRTUAL;
    surface->node.size          = surface->size;

    /* Initial lock. */
    gcmONERROR(_Lock(surface));

    /* Get a normal surface. */
    *Surface = surface;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (surface)
    {
        gcmVERIFY_OK(gcoSURF_Destroy(surface));
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSURF_WrapUserMultiBuffer(
    IN gcoHAL Hal,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gctUINT * Stride,
    IN gctUINT32 * Handle,
    IN gctUINT * BufferOffset,
    IN gctUINT32 Flag,
    OUT gcoSURF * Surface
    )
{
    gctUINT i;
    gceSTATUS status;
    gcoSURF surface = gcvNULL;
    gctUINT32 node[3] = {0, 0, 0};
    gctUINT nodeCount;
    gctUINT bitsPerPixel[3];

    gcmHEADER_ARG("Handle=%d, Flag=%d", Handle, Flag);

    if (Flag != gcvALLOC_FLAG_DMABUF)
    {
        /* Only for DMABUF for now. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Create a no video memory node surface. */
    Type = (gceSURF_TYPE) (Type | gcvSURF_NO_VIDMEM);

    if (Width > 0x2000 || Height > 0x2000)
    {
        /* Size should not be too large, use 8K x 8K as threshold. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    gcmONERROR(gcoSURF_Construct(
        gcvNULL, Width, Height, 1, Type, Format, gcvPOOL_VIRTUAL, &surface));

    surface->flags |= gcvSURF_FLAG_MULTI_NODE;
    surface->size   = 0;

    /* bytes per pixel of first plane. */
    switch (surface->format)
    {
    case gcvSURF_YV12:
    case gcvSURF_I420:
        bitsPerPixel[0] = 8;
        bitsPerPixel[1] = 4;
        bitsPerPixel[2] = 4;
        nodeCount = 3;
        break;

    case gcvSURF_NV12:
    case gcvSURF_NV21:
        bitsPerPixel[0] = 8;
        bitsPerPixel[1] = 8;
        nodeCount = 2;
        break;

    case gcvSURF_NV16:
    case gcvSURF_NV61:
        bitsPerPixel[0] = 8;
        bitsPerPixel[1] = 8;
        nodeCount = 2;
        break;

    default:
        nodeCount = 1;
        bitsPerPixel[0] = surface->bitsPerPixel;
        break;
    }

    /* Check parameters. */
    for (i = 0; i < nodeCount; i++)
    {
        /*
         * Checking for following items:
         * [sw] DMA buffer handle is file descriptor, should be GE 0.
         * [hw] stride must be 4 pixel aligned.
         * [sw] stride should not be too large (roughly use 16K pixel threshold).
         * [hw] buffer start address (offset) must be 64 byte aligned.
         * [sw] buffer offset should not be too large (roughly use 64M byte threshold)
         */
        if ((Handle[i] >= 0x80000000) ||
            (Stride[i] & (bitsPerPixel[i] * 4 / 8 - 1)) ||
            (Stride[i] > (bitsPerPixel[i] * 0x4000 / 8)) ||
            (BufferOffset[i] & 0x3F) ||
            (BufferOffset[i] > 0x4000000))
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
    }

    /* Compute the surface placement parameters. */
    switch (surface->format)
    {
    case gcvSURF_YV12:
        /*  WxH Y plane followed by (W/2)x(H/2) V and U planes. */
        surface->stride  = Stride[0];
        surface->uStride = Stride[2];
        surface->vStride = Stride[1];

        /* No offsets to first node. */
        surface->vOffset = surface->uOffset = 0;

        surface->sliceSize
            = surface->stride  * surface->alignedH
            + surface->uStride * surface->alignedH / 2;
        break;

    case gcvSURF_I420:
        /*  WxH Y plane followed by (W/2)x(H/2) U and V planes. */
        surface->stride  = Stride[0];
        surface->uStride = Stride[1];
        surface->vStride = Stride[2];

        /* No offsets to first node. */
        surface->uOffset = surface->vOffset = 0;

        surface->sliceSize
            = surface->stride  * surface->alignedH
            + surface->uStride * surface->alignedH / 2;
        break;

    case gcvSURF_NV12:
    case gcvSURF_NV21:
        /*  WxH Y plane followed by (W)x(H/2) interleaved U/V plane. */
        surface->stride  = Stride[0];
        surface->uStride =
        surface->vStride = Stride[1];

        /* No offsets to first node. */
        surface->uOffset = surface->vOffset = 0;

        surface->sliceSize
            = surface->stride  * surface->alignedH
            + surface->uStride * surface->alignedH / 2;
        break;

    case gcvSURF_NV16:
    case gcvSURF_NV61:
        /*  WxH Y plane followed by WxH interleaved U/V(V/U) plane. */
        surface->stride  = Stride[0];
        surface->uStride =
        surface->vStride = Stride[1];

        /* No offsets to first node. */
        surface->uOffset = surface->vOffset = 0;

        surface->sliceSize
            = surface->stride  * surface->alignedH
            + surface->uStride * surface->alignedH;
        break;

    case gcvSURF_YUY2:
    case gcvSURF_UYVY:
    case gcvSURF_YVYU:
    case gcvSURF_VYUY:
        /*  WxH interleaved Y/U/Y/V plane. */
        surface->stride  =
        surface->uStride =
        surface->vStride = Stride[0];

        /* No offsets to first node. */
        surface->uOffset = surface->vOffset = 0;

        surface->sliceSize
            = surface->stride * surface->alignedH;
        break;

    default:
        surface->stride = Stride[0];

        /* No u,v strides. */
        surface->uStride = surface->vStride = 0;

        /* No offsets to first node. */
        surface->uOffset = surface->vOffset = 0;

        surface->sliceSize
            = surface->stride * surface->alignedH;
        break;
    }

    surface->layerSize = surface->sliceSize * surface->requestD;

    /* Always single layer for user surface */
    gcmASSERT(surface->formatInfo.layers == 1);

    surface->size = surface->layerSize * surface->formatInfo.layers;

    /* TODO: Handle[3]/BufferOffset[3]/Stride[3]/BufferOffset[3], is for compression surface
    ** for our driver is tile status buffer. Need process those parameters */

    /* Wrap handles into Vivante HAL. */
    for (i = 0; i < nodeCount; i++)
    {
        gcsUSER_MEMORY_DESC desc;

        desc.handle = Handle[i];
        desc.flag = Flag;

        gcmONERROR(gcoHAL_WrapUserMemory(&desc, (gceVIDMEM_TYPE)surface->type, &node[i]));
    }

    /* Import wrapped video memory node to the surface. */
    switch (nodeCount)
    {
    case 3:
        surface->node3.u.normal.node = node[2];
        surface->node3.bufferOffset  = BufferOffset[2];
        surface->node3.pool = gcvPOOL_VIRTUAL;
        surface->node3.size = Stride[2] * Height + BufferOffset[2];
        /* fall through */
    case 2:
        surface->node2.u.normal.node = node[1];
        surface->node2.bufferOffset  = BufferOffset[1];
        surface->node2.pool = gcvPOOL_VIRTUAL;
        surface->node2.size = Stride[1] * Height + BufferOffset[1];
        /* fall through */
    default:
        surface->node.u.normal.node  = node[0];
        surface->node.bufferOffset   = BufferOffset[0];
        surface->node.pool  = gcvPOOL_VIRTUAL;
        surface->node.size  = Stride[0] * Height + BufferOffset[0];
        break;
    }

    /* Initial lock. */
    gcmONERROR(_Lock(surface));

    /* Get a normal surface. */
    *Surface = surface;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    for (i = 0; i < 3; i++)
    {
        if (node[i] > 0) {
            gcmVERIFY_OK(gcoHAL_ReleaseVideoMemory(node[i]));
        }
    }

    if (surface)
    {
        gcmVERIFY_OK(gcoSURF_Destroy(surface));
    }

    gcmFOOTER();
    return status;
}

