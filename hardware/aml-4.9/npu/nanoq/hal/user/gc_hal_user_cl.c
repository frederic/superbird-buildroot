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


#include "gc_hal_user_precomp.h"
#if gcdENABLE_3D
#include "gc_hal_cl.h"

#define _GC_OBJ_ZONE            gcdZONE_CL

/******************************************************************************\
|******************************* gcoCL API Code *******************************|
\******************************************************************************/

/*******************************************************************************
**
**  gcoCL_InitializeHardware
**
**  Initialize hardware.  This is required for each thread.
**
**  INPUT:
**
**      Nothing
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoCL_InitializeHardware()
{
    gceSTATUS status;
    gceAPI currentApi;

    gcmHEADER();

    /* Set the hardware type. */
    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_MCFE))
    {
        gcoHARDWARE_SelectChannel(gcvNULL, 0, 1);
    }

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, gcvNULL));

    /* Get Current API. */
    gcmVERIFY_OK(gcoHARDWARE_GetAPI(gcvNULL, &currentApi, gcvNULL));

    if (currentApi == 0)
    {
        /* Set HAL API to OpenCL only when there is API is not set. */
        gcmVERIFY_OK(gcoHARDWARE_SetAPI(gcvNULL, gcvAPI_OPENCL));
    }

    if (!gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_PIPE_CL))
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Set rounding mode */
    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SHADER_HAS_RTNE))
    {
        gcmVERIFY_OK(gcoHARDWARE_SetRTNERounding(gcvNULL, gcvTRUE));
    }

    gcmONERROR(gcoCLHardware_Construct());

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}



/**********************************************************************
**
**  gcoCL_SetHardware
**
**  Set the gcoHARDWARE object for current thread., CL used its own hardware, Need Save'
**  Current Thread's information.
**
**  INPUT:
**
**      the new hardware.
**
**  OUTPUT:
**
**      current thread's hw, hwType, coreIndex.
*/
gceSTATUS
gcoCL_SetHardware(
    IN gcoHARDWARE hw,
    OUT gcoHARDWARE *savedHW,
    OUT gceHARDWARE_TYPE *savedType,
    OUT gctUINT32 *savedCoreIndex
)
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 coreIndex = 0;
    gcmHEADER();
    gcmVERIFY_ARGUMENT(savedHW != gcvNULL);
    gcoHAL_GetCurrentCoreIndex(gcvNULL, savedCoreIndex);
    gcmONERROR(gcoHARDWARE_Get3DHardware(savedHW));
    gcmONERROR(gcoHAL_GetHardwareType(gcvNULL, savedType));
    gcmONERROR(gcoHARDWARE_Set3DHardware(hw));
    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));
    if(hw)
    {
        gcoHARDWARE_QueryCoreIndex(hw,0,&coreIndex);
        gcoHAL_SetCoreIndex(gcvNULL, coreIndex);
    }
OnError:
    gcmFOOTER();
    return status;

}


    gceSTATUS
gcoCL_RestoreContext(
     gcoHARDWARE preHW,
     gceHARDWARE_TYPE preType,
     gctUINT32 preCoreIndex
    )
    {
        gceSTATUS status = gcvSTATUS_OK;
        gcmHEADER();

        gcmONERROR(gcoHARDWARE_Set3DHardware(preHW));
        gcoHAL_SetCoreIndex(gcvNULL, preCoreIndex);
        gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, preType));

OnError:
        gcmFOOTER();
        return status;
    }

/******************************************************************************\
|****************************** MEMORY MANAGEMENT *****************************|
\******************************************************************************/

/*******************************************************************************
**
**  gcoCL_AllocateMemory
**
**  Allocate contiguous memory from the kernel.
**
**  INPUT:
**
**      gctUINT * Bytes
**          Pointer to the number of bytes to allocate.
**
**  OUTPUT:
**
**      gctUINT * Bytes
**          Pointer to a variable that will receive the aligned number of bytes
**          allocated.
**
**      gctUINT32_PTR Physical
**          Pointer to a variable that will receive the gpu virtual address of
**          the allocated memory, might be same as gpu physical address for flat
**          mapping case etc.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
**
**      gcsSURF_NODE_PTR  * Node
**          Pointer to a variable that will receive the gcsSURF_NODE structure
**          pointer that describes the video memory to lock.
*/
gceSTATUS
gcoCL_AllocateMemory(
    IN OUT gctUINT *        Bytes,
    OUT gctUINT32_PTR       Physical,
    OUT gctPOINTER *        Logical,
    OUT gcsSURF_NODE_PTR *  Node,
    IN  gceSURF_TYPE        Type,
    IN  gctUINT32           Flag
    )
{
    gceSTATUS status;
    gctUINT bytes;
    gcsSURF_NODE_PTR node = gcvNULL;
    gctUINT alignBytes = 256; /* enlarge the alignment size to 256 to fit the requirement of instruction memory */
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("*Bytes=%lu", *Bytes);

    /* Allocate extra 64 bytes to avoid cache overflow */
    bytes = gcmALIGN(*Bytes + 64, 64);

    /* Allocate node. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(gcsSURF_NODE),
                              &pointer));

    node = pointer;
    /* for CL FP long16/ulong16 need 128 bytes alignment */

    gcmONERROR(gcsSURF_NODE_Construct(
        node,
        bytes,
        alignBytes,
        Type,
        Flag == 0 ? gcvALLOC_FLAG_NONE:Flag,
        gcvPOOL_DEFAULT
        ));

    /* Lock the cl buffer. */
    gcmONERROR(gcoHARDWARE_Lock(node,
                                Physical,
                                Logical));

    gcmDUMP(gcvNULL, "#[info: initialize OCL node memory at create time]");
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_MEMORY,
                   *Physical,
                   *Logical,
                   0,
                   node->size);
    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
    {
        gcmONERROR(gcoHARDWARE_LockEx(node, gcvENGINE_BLT,
                                      gcvNULL, gcvNULL));
    }

    /* Return allocated number of bytes. */
    *Bytes = bytes;

    /* Return node. */
    *Node = node;

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%lu *Physical=0x%x *Logical=0x%x *Node=0x%x",
                  *Bytes, *Physical, *Logical, *Node);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    if(node != gcvNULL)
    {
        gcoOS_Free(gcvNULL, node);
        node = gcvNULL;
    }
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_FreeMemory
**
**  Free contiguous memeory to the kernel.
**
**  INPUT:
**
**      gctUINT32 Physical
**          The gpu virtual address of the allocated pages.
**
**      gctPOINTER Logical
**          The logical address of the allocation.
**
**      gctUINT Bytes
**          Number of bytes allocated.
**
**      gcsSURF_NODE_PTR  Node
**          Pointer to a gcsSURF_NODE structure
**          that describes the video memory to unlock.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoCL_FreeMemory(
    IN gctUINT32            Physical,
    IN gctPOINTER           Logical,
    IN gctUINT              Bytes,
    IN gcsSURF_NODE_PTR     Node,
    IN gceSURF_TYPE         Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Physical=0x%x Logical=0x%x Bytes=%u Node=0x%x",
                  Physical, Logical, Bytes, Node);

    /* Do we have a buffer allocated? */
    if (Node && Node->pool != gcvPOOL_UNKNOWN)
    {
        /* Unlock the index buffer. */
        gcmONERROR(gcoHARDWARE_Unlock(Node,
                                      Type));

        if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
        {
            gcmONERROR(gcoHARDWARE_UnlockEx(Node, gcvENGINE_BLT, Type));
        }

        /* Create an event to free the video memory. */
        gcmONERROR(gcsSURF_NODE_Destroy(Node));

        /* Free node. */
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Node));
    }

OnError:

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_WrapUserMemory
**
*/
gceSTATUS
gcoCL_WrapUserMemory(
    IN gctPOINTER           Ptr,
    IN gctUINT              Bytes,
    IN gctBOOL              VIVUnCached,
    OUT gctUINT32_PTR       Physical,
    OUT gcsSURF_NODE_PTR *  Node
    )
{
    gctUINT32 node = 0;
    gctUINT i;
    gceSTATUS status;
    gcsUSER_MEMORY_DESC desc;
    gctPOINTER pointer = gcvNULL;
    gcsSURF_NODE_PTR surf = gcvNULL;
    gctUINT32 physical;

    /* Allocate node. */
    gcoOS_ZeroMemory(&desc, gcmSIZEOF(desc));
    desc.flag     = gcvALLOC_FLAG_USERMEMORY;
    desc.logical  = gcmPTR_TO_UINT64(Ptr);
    desc.physical = gcvINVALID_PHYSICAL_ADDRESS;
    desc.size     = Bytes;

    gcmONERROR(gcoHAL_WrapUserMemory(&desc, gcvVIDMEM_TYPE_BITMAP, &node));

    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(gcsSURF_NODE),
                              &pointer));
    gcoOS_ZeroMemory(pointer, gcmSIZEOF(gcsSURF_NODE));

    surf = pointer;

    surf->u.normal.node = node;
    surf->size          = Bytes;

    if (VIVUnCached)
    {
        surf->pool          = gcvPOOL_USER;
        surf->u.normal.cacheable = gcvFALSE;
    }
    else
    {
        surf->pool          = gcvPOOL_VIRTUAL;
        surf->u.normal.cacheable = gcvTRUE;
    }

    surf->physical2     = gcvINVALID_ADDRESS;
    surf->physical3     = gcvINVALID_ADDRESS;

    for (i = 0; i < gcvHARDWARE_NUM_TYPES; i++)
    {
        surf->hardwareAddresses[i] = gcvINVALID_ADDRESS;
    }

    gcmONERROR(gcoHARDWARE_Lock(surf, &physical, gcvNULL));

    *Physical = physical;

    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
    {
        gcmONERROR(gcoHARDWARE_LockEx(surf, gcvENGINE_BLT,
                                      gcvNULL, gcvNULL));
    }

    *Node = surf;

    return gcvSTATUS_OK;

OnError:
    if (surf)
    {
        gcsSURF_NODE_Destroy(surf);
        gcoOS_Free(gcvNULL, surf);
    }

    return status;
}

/*******************************************************************************
**
**  gcoCL_WrapUserPhysicalMemory
**
*/
gceSTATUS
gcoCL_WrapUserPhysicalMemory(
    IN gctUINT32_PTR        Physical,
    IN gctUINT              Bytes,
    IN gctBOOL              VIVUnCached,
    OUT gctPOINTER  *       Logical,
    OUT gctUINT32   *       Address,
    OUT gcsSURF_NODE_PTR *  Node
    )
{
    gctUINT32 node = 0;
    gctUINT i;
    gceSTATUS status;
    gcsUSER_MEMORY_DESC desc;
    gctPOINTER pointer = gcvNULL;
    gcsSURF_NODE_PTR surf = gcvNULL;
    gctUINT32     address = gcvINVALID_ADDRESS;


    /* Allocate node. */
    gcoOS_ZeroMemory(&desc, gcmSIZEOF(desc));
    desc.flag     = gcvALLOC_FLAG_USERMEMORY;
    desc.physical = gcmPTR_TO_UINT64(Physical);
    desc.size     = Bytes;

    if(desc.physical >= (20000000000ULL) ) /*only support 40bit physical address*/
    {
        gcmONERROR(gcvSTATUS_INVALID_ADDRESS);
    }
    gcmONERROR(gcoHAL_WrapUserMemory(&desc, gcvVIDMEM_TYPE_BITMAP, &node));

    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(gcsSURF_NODE),
                              &pointer));
    gcoOS_ZeroMemory(pointer, gcmSIZEOF(gcsSURF_NODE));

    surf = pointer;

    surf->u.normal.node = node;
    surf->size          = Bytes;

    if (VIVUnCached)
    {
        surf->pool          = gcvPOOL_USER;
        surf->u.normal.cacheable = gcvFALSE;
    }
    else
    {
        surf->pool          = gcvPOOL_USER;
        surf->u.normal.cacheable = gcvTRUE;
    }

    surf->physical2     = gcvINVALID_ADDRESS;
    surf->physical3     = gcvINVALID_ADDRESS;

    for (i = 0; i < gcvHARDWARE_NUM_TYPES; i++)
    {
        surf->hardwareAddresses[i] = gcvINVALID_ADDRESS;
    }

    gcmONERROR(gcoHARDWARE_Lock(surf, &address, Logical));

    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
    {
        gcmONERROR(gcoHARDWARE_LockEx(surf, gcvENGINE_BLT,
                                      gcvNULL, gcvNULL));
    }

    *Address = address;

    *Node = surf;

    return gcvSTATUS_OK;

OnError:
    if (surf)
    {
        gcsSURF_NODE_Destroy(surf);
        gcoOS_Free(gcvNULL, surf);
    }

    return status;
}
/*******************************************************************************
**
**  gcoCL_FlushMemory
**
**  Flush memory to the kernel.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR  Node
**          Pointer to a gcsSURF_NODE structure
**          that describes the video memory to flush.
**
**      gctPOINTER Logical
**          The logical address of the allocation.
**
**      gctSIZE_T Bytes
**          Number of bytes allocated.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoCL_FlushMemory(
    IN gcsSURF_NODE_PTR     Node,
    IN gctPOINTER           Logical,
    IN gctSIZE_T            Bytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Bytes=%u",
                  Node, Logical, Bytes);

    if (Node /*&& Node->pool == gcvPOOL_VIRTUAL*/)
    {
        if (Node->pool == gcvPOOL_USER && !Node->u.normal.cacheable)
        {
            gcmFOOTER();
            return status;
        }

        /*gcmONERROR(gcoOS_CacheFlush(gcvNULL, Node->u.normal.node, Logical, Bytes));*/
        gcmONERROR(gcoSURF_NODE_Cache(Node,
                                      Logical,
                                      Bytes,
                                      gcvCACHE_FLUSH));
    }
    else
    {
        gcmONERROR(gcoOS_CacheFlush(gcvNULL, 0, Logical, Bytes));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_InvalidateMemoryCache
**
**  Invalidate memory cache in CPU.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR  Node
**          Pointer to a gcsSURF_NODE structure
**          that describes the video memory to flush.
**
**      gctPOINTER Logical
**          The logical address of the allocation.
**
**      gctSIZE_T Bytes
**          Number of bytes allocated.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoCL_InvalidateMemoryCache(
    IN gcsSURF_NODE_PTR     Node,
    IN gctPOINTER           Logical,
    IN gctSIZE_T            Bytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Bytes=%u",
                  Node, Logical, Bytes);

    if (Node /*&& Node->pool == gcvPOOL_VIRTUAL*/)
    {
        if (Node->pool == gcvPOOL_USER && !Node->u.normal.cacheable)
        {
            gcmFOOTER();
            return status;
        }

        /*gcmONERROR(gcoOS_CacheInvalidate(gcvNULL, Node->u.normal.node, Logical, Bytes));*/
        gcmONERROR(gcoSURF_NODE_Cache(Node,
                                      Logical,
                                      Bytes,
                                      gcvCACHE_INVALIDATE));
    }
    else
    {
        gcmONERROR(gcoOS_CacheInvalidate(gcvNULL, 0, Logical, Bytes));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_ShareMemoryWithStream
**
**  Share memory with a stream.
**
**  INPUT:
**
**      gcoSTREAM Stream
**          Pointer to the stream object.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the aligned number of bytes
**          allocated.
**
**      gctUINT32_PTR Physical
**          Pointer to a variable that will receive the gpu virtual addresses of
**          the allocated memory.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
**
**      gcsSURF_NODE_PTR  * Node
**          Pointer to a variable that will receive the gcsSURF_NODE structure
**          pointer that describes the video memory to lock.
*/
gceSTATUS
gcoCL_ShareMemoryWithStream(
    IN gcoSTREAM            Stream,
    OUT gctSIZE_T *         Bytes,
    OUT gctUINT32_PTR       Physical,
    OUT gctPOINTER *        Logical,
    OUT gcsSURF_NODE_PTR *  Node
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x", Stream);
    gcmVERIFY_ARGUMENT(Bytes != 0);

    *Bytes = gcoSTREAM_GetSize(Stream);
    gcmONERROR(gcoSTREAM_Node(Stream, Node));

    /* Lock the cl buffer. */
    gcmONERROR(gcoHARDWARE_Lock(*Node,
                                Physical,
                                Logical));

    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
    {
        gcmONERROR(gcoHARDWARE_LockEx(*Node, gcvENGINE_BLT, gcvNULL, gcvNULL));
    }

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%lu *Physical=0x%x *Logical=0x%x *Node=0x%x",
                  *Bytes, *Physical, *Logical, *Node);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_ShareMemoryWithBufObj
**
**  Share memory with a BufObj.
**
**  INPUT:
**
**      gcoBUFOBJ Stream
**          Pointer to the stream object.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the aligned number of bytes
**          allocated.
**
**      gctUINT32_PTR  Physical
**          Pointer to a variable that will receive the gpu virtual addresses of
**          the allocated memory.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
**
**      gcsSURF_NODE_PTR  * Node
**          Pointer to a variable that will receive the gcsSURF_NODE structure
**          pointer that describes the video memory to lock.
*/
gceSTATUS
gcoCL_ShareMemoryWithBufObj(
    IN gcoBUFOBJ            BufObj,
    OUT gctSIZE_T *         Bytes,
    OUT gctUINT32_PTR       Physical,
    OUT gctPOINTER *        Logical,
    OUT gcsSURF_NODE_PTR *  Node
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("BufObj=0x%x", BufObj);

    gcmONERROR(gcoBUFOBJ_GetSize(BufObj, Bytes));
    gcmONERROR(gcoBUFOBJ_GetNode(BufObj, Node));

    /* Lock the cl buffer. */
    gcmONERROR(gcoHARDWARE_Lock(*Node,
                                Physical,
                                Logical));

    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
    {
        gcmONERROR(gcoHARDWARE_LockEx(*Node, gcvENGINE_BLT, gcvNULL, gcvNULL));
    }

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%lu *Physical=0x%x *Logical=0x%x *Node=0x%x",
                  *Bytes, *Physical, *Logical, *Node);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_UnshareMemory
**
**  Unshare memory object with GL object.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR  Node
**          Pointer to a  gcsSURF_NODE structure
*/
gceSTATUS
gcoCL_UnshareMemory(
    IN gcsSURF_NODE_PTR     Node
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x", Node);

    /* Unlock the index buffer. */
    gcmVERIFY_OK(gcoHARDWARE_Unlock(Node, gcvSURF_INDEX));

    if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
        gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
    {
        gcmVERIFY_OK(gcoHARDWARE_UnlockEx(Node, gcvENGINE_BLT, gcvSURF_INDEX));
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_FlushSurface
**
**  Flush surface to the kernel.
**
**  INPUT:
**
**      gcoSURF           Surface
**          gcoSURF structure
**          that describes the surface to flush.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoCL_FlushSurface(
    IN gcoSURF              Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER srcMemory[3] = {gcvNULL};

    gcmHEADER_ARG("Surface=0x%x", Surface);

    if (Surface /*&& Surface->node.pool == gcvPOOL_VIRTUAL*/)
    {
        if (Surface->node.pool == gcvPOOL_USER)
        {
            if (!Surface->node.u.normal.cacheable)
            {
                gcmFOOTER();
                return status;
            }

            gcmONERROR(gcoOS_CacheFlush(gcvNULL,
                                           Surface->node.u.normal.node,
                                           Surface->node.logical,
                                           Surface->size));

            gcmONERROR(gcoSURF_NODE_Cache(&Surface->node,
                                          Surface->node.logical,
                                          Surface->size,
                                          gcvCACHE_INVALIDATE));
        }
        else
        {
            gcmONERROR(gcoSURF_NODE_Cache(&Surface->node,
                                        srcMemory[0],
                                        Surface->size,
                                        gcvCACHE_FLUSH));

            gcmONERROR(gcoSURF_NODE_Cache(&Surface->node,
                                        srcMemory[0],
                                        Surface->size,
                                        gcvCACHE_INVALIDATE));
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_LockSurface(
    IN gcoSURF Surface,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    status = gcoSURF_Lock(Surface, Address, Memory);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_UnlockSurface(
    IN gcoSURF Surface,
    IN gctPOINTER Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    status = gcoSURF_Unlock(Surface, Memory);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_CreateTexture
**
**  Create texture for image.
**
**  INPUT:
**
**      gctUINT Width
**          Width of the image.
**
**      gctUINT Heighth
**          Heighth of the image.
**
**      gctUINT Depth
**          Depth of the image.
**
**      gctCONST_POINTER Memory
**          Pointer to the data of the input image.
**
**      gctUINT Stride
**          Size of one row.
**
**      gctUINT Slice
**          Size of one plane.
**
**      gceSURF_FORMAT FORMAT
**          Format of the image.
**
**      gceENDIAN_HINT EndianHint
**          Endian needed to handle the image data.
**
**  OUTPUT:
**
**      gcoTEXTURE * Texture
**          Pointer to a variable that will receive the gcoTEXTURE structure.
**
**      gcoSURF * Surface
**          Pointer to a variable that will receive the gcoSURF structure.
**
**      gctUINT32 * Physical
**          Pointer to a variable that will receive the gpu virtual addresses of
**          the allocated memory.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
**
**      gctUINT * SurfStride
**          Pointer to a variable that will receive the stride of the texture.
*/
gceSTATUS
gcoCL_CreateTexture(
    IN OUT gceIMAGE_MEM_TYPE* MapHostMemory,
    IN gctUINT                Width,
    IN gctUINT                Height,
    IN gctUINT                Depth,
    IN gctCONST_POINTER       Memory,
    IN gctUINT                Stride,
    IN gctUINT                Slice,
    IN gceSURF_FORMAT         Format,
    IN gceENDIAN_HINT         EndianHint,
    OUT gcoTEXTURE *          Texture,
    OUT gcoSURF *             Surface,
    OUT gctUINT32  *          Physical,
    OUT gctPOINTER *          Logical,
    OUT gctUINT *             SurfStride,
    OUT gctUINT *             SurfSliceSize
    )
{
    gceSTATUS status;
    gcoTEXTURE texture = gcvNULL;
    gcoSURF surface = gcvNULL;

    gcmHEADER_ARG("Width=%u Height=%u Depth=%u Memory=0x%x "
                  "Stride=%u Slice=%u Format=%u EndianHint=%u",
                  Width, Height, Depth, Memory,
                  Stride, Slice, Format, EndianHint);

    gcmASSERT(gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_LINEAR));

    /* Try to map host memory first. */
    if (*MapHostMemory != gcvIMAGE_MEM_DEFAULT)
    {
        gctUINT32 alignedWidth= Width;
        gctUINT32 alignedHeight = Height;

        gcmASSERT(!(gcmALL_TO_UINT32(Memory) & 0x3F));

        gcmONERROR(
            gcoHARDWARE_AlignToTileCompatible(gcvNULL,
                                              gcvSURF_BITMAP,
                                              0,
                                              Format,
                                              &alignedWidth,
                                              &alignedHeight,
                                              Depth,
                                              gcvNULL,
                                              gcvNULL,
                                              gcvNULL));

        /* Check the alignment. */
        if ((alignedWidth == Width) && (alignedHeight == Height))
        {
            do {
                gcmERR_BREAK(gcoSURF_ConstructWrapper(gcvNULL, &surface));

                if(gcvIMAGE_MEM_HOST_PTR == *MapHostMemory || gcvIMAGE_MEM_HOST_PHY_PTR == *MapHostMemory)
                {
                    surface->node.u.normal.cacheable = gcvTRUE;
                }

                if(gcvIMAGE_MEM_HOST_PTR == *MapHostMemory || gcvIMAGE_MEM_HOST_PTR_UNCACHED == *MapHostMemory)
                {
                    gcmERR_BREAK(gcoSURF_SetBuffer(surface,
                                         gcvSURF_BITMAP,
                                         Format,
                                         Stride,
                                         (gctPOINTER) Memory,
                                         gcvINVALID_PHYSICAL_ADDRESS));
                }
                else
                {
                    gcmASSERT(gcvIMAGE_MEM_HOST_PHY_PTR == *MapHostMemory || gcvIMAGE_MEM_HOST_PHY_PTR_UNCACHED == *MapHostMemory);
                    gcmERR_BREAK(gcoSURF_SetBuffer(surface,
                                         gcvSURF_BITMAP,
                                         Format,
                                         Stride,
                                         gcvNULL,
                                         gcmPTR_TO_UINT64(Memory)));
                }

                gcmERR_BREAK(gcoSURF_SetImage(surface,
                                     0,
                                     0,
                                     Width,
                                     Height,
                                     Depth));

                if(Slice != 0)
                {
                    surface->sliceSize = Slice;
                }

                gcoSURF_Lock(surface, gcvNULL, gcvNULL);
            } while (gcvFALSE);

            if (gcmIS_ERROR(status))
            {
                if (surface)
                {
                    gcoSURF_Destroy(surface);
                    surface = gcvNULL;
                }

                *MapHostMemory = gcvFALSE;
            }
        }
        else
        {
            *MapHostMemory = gcvFALSE;
        }
    }

    /* If mapping failed, create a surface. */
    if (surface == gcvNULL)
    {
        /*gcvSURF_FORMAT_OCL used to add 64byte for cache overflow in this case*/
        Format |= gcvSURF_FORMAT_OCL;

        /* Construct the source surface. */
        gcmONERROR(gcoSURF_Construct(gcvNULL,
                                     Width, Height, Depth,
                                     gcvSURF_BITMAP,
                                     Format,
                                     gcvPOOL_DEFAULT,
                                     &surface));

        gcmASSERT(surface->tiling == gcvLINEAR);
        gcmASSERT(surface->node.logical);

        if (Memory)
        {
            gctUINT i,j, lineBytes;
            gctUINT8_PTR srcSliceBegin;
            gctUINT8_PTR srcLineBegin;
            gctUINT8_PTR dstSliceBegin;
            gctUINT8_PTR dstLineBegin;
            srcSliceBegin = (gctUINT8_PTR)Memory;
            dstSliceBegin = surface->node.logical;
            lineBytes     = surface->bitsPerPixel / 8 * Width;
            for (i = 0; i< Depth; ++i)
            {
                srcLineBegin = srcSliceBegin;
                dstLineBegin = dstSliceBegin;
                for (j = 0; j < Height; ++j)
                {
                    gcoOS_MemCopy(
                        dstLineBegin,
                        srcLineBegin,
                        lineBytes);  /*Only copy what needed.*/
                    srcLineBegin += Stride;
                    dstLineBegin += surface->stride;
                }
                srcSliceBegin += Slice;
                dstSliceBegin += surface->sliceSize;
            }

            gcoCL_FlushMemory(
                &surface->node,
                surface->node.logical,
                surface->stride * Height * Depth);
        }
    }

    gcmONERROR(gcoTEXTURE_Construct(gcvNULL, &texture));

    /*texture->endianHint = EndianHint;*/

    gcmONERROR(gcoTEXTURE_AddMipMapFromClient(texture, 0, surface));

    /* Return physical address. */
    gcmGETHARDWAREADDRESS(surface->node, *Physical);

    /* Return logical address. */
    *Logical = surface->node.logical;

    /* Return surface stride. */
    *SurfStride = surface->stride;
    *SurfSliceSize = surface->sliceSize;

    *Texture = texture;
    *Surface = surface;

    /* Success. */
    gcmFOOTER_ARG("*Texture=0x%x *Surface=0x%x *Physical=0x%x *Logical=0x%x",
                  *Texture, *Surface, *Physical, *Logical);
    return gcvSTATUS_OK;

OnError:

    if (surface != gcvNULL)
    {
        gcoSURF_Destroy(surface);
    }

    /* Return the status. */
    if(texture != gcvNULL)
    {
        gcoTEXTURE_Destroy(texture);
        texture = gcvNULL;
    }
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_DestroyTexture
**
**  Destroy an gcoTEXTURE object.
**
**  INPUT:
**
**      gcoTEXTURE Texture
**          Pointer to an gcoTEXTURE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoCL_DestroyTexture(
    IN gcoTEXTURE Texture,
    IN gcoSURF    Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Texture=0x%x", Texture);

    gcoTEXTURE_Destroy(Texture);

    gcoSURF_Destroy(Surface);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_SetupTexture(
    IN gcoTEXTURE           Texture,
    IN gcoSURF              Surface,
    IN gctUINT              SamplerNum,
    gceTEXTURE_ADDRESSING   AddressMode,
    gceTEXTURE_FILTER       FilterMode
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsTEXTURE info = {0};

    gcmHEADER_ARG("Texture=0x%x", Texture);

    info.s = info.t = info.r = AddressMode;
    info.magFilter = info.minFilter = FilterMode;
    info.mipFilter = gcvTEXTURE_NONE;
    info.anisoFilter = 1;

    /* No lod bias allowed. */
    info.lodBias = 0.0f;
    /* Need to relax these when supporting mipmap. */
    info.lodMin = 0.0f;
    info.lodMax = 0.0f;
    info.baseLevel = 0;
    info.maxLevel = 0;

    info.swizzle[gcvTEXTURE_COMPONENT_R] = gcvTEXTURE_SWIZZLE_R;
    info.swizzle[gcvTEXTURE_COMPONENT_G] = gcvTEXTURE_SWIZZLE_G;
    info.swizzle[gcvTEXTURE_COMPONENT_B] = gcvTEXTURE_SWIZZLE_B;
    info.swizzle[gcvTEXTURE_COMPONENT_A] = gcvTEXTURE_SWIZZLE_A;
    info.border[gcvTEXTURE_COMPONENT_R] = 0;
    info.border[gcvTEXTURE_COMPONENT_G] = 0;
    info.border[gcvTEXTURE_COMPONENT_B] = 0;
    info.border[gcvTEXTURE_COMPONENT_A] = 0 /*255*/;

    /* PCF is not allowed. */
    info.compareMode = gcvTEXTURE_COMPARE_MODE_INVALID;
    info.compareFunc = gcvCOMPARE_INVALID;

    if (gcvSTATUS_TRUE == gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_DESCRIPTOR))
    {
        gcoTEXTURE_BindTextureDesc(Texture, SamplerNum, &info, 0);
    }
    else
    {
        gcoTEXTURE_BindTextureEx(Texture, 0, SamplerNum, &info, 0);
    }

    gcoTEXTURE_Flush(Texture);
    gcoTEXTURE_FlushVS(Texture);


    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_QueryDeviceInfo
**
**  Query the OpenCL capabilities of the device.
**
**  INPUT:
**
**      Nothing
**
**  OUTPUT:
**
**      gcoCL_DEVICE_INFO_PTR DeviceInfo
**          Pointer to the device information
*/
gceSTATUS
gcoCL_QueryDeviceInfo(
    OUT gcoCL_DEVICE_INFO_PTR   DeviceInfo
    )
{

    gceSTATUS status;
    gctUINT threadCount;
    gctSIZE_T contiguousSize;
    gctUINT32 contiguousPhysName;
    gctSIZE_T physicalSystemMemSize;
    gceCHIPMODEL  chipModel = gcv200;
    gctUINT32 chipRevision = 0;
    gctBOOL chipEnableEP = gcvFALSE;
    VSC_HW_CONFIG hwCfg;

    gcmHEADER();
    gcmASSERT(DeviceInfo != gcvNULL);

    gcmONERROR(gcQueryShaderCompilerHwCfg(gcvNULL, &hwCfg));

    gcoHAL_SetHardwareType(gcvNULL,gcvHARDWARE_3D);

    /* Number of shader cores and threads */
    gcmONERROR(
        gcoHARDWARE_QueryShaderCaps(gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    &DeviceInfo->ShaderCoreCount,
                                    &threadCount,
                                    gcvNULL,
                                    gcvNULL));

    gcoCL_QueryDeviceCount(gcvNULL, &(DeviceInfo->maxComputeUnits));
    DeviceInfo->maxWorkItemDimensions = 3;

    /* The below restrictions are based on 16-bits for Global ID (per component)
     * and 10-bits for Local ID (also per-component).  Technically, the maximum
     * work group size supported is 1024*1024*1024; however, below, since there
     * is only an aggregate number present, we have to restrict ourselves to
     *      min(x,y,z)=1024
     * This should not be overly restrictive since larger than that will not fit
     * in our L1 cache well.
     */
    DeviceInfo->maxWorkItemSizes[0]   = gcmMIN(threadCount, 1024);
    DeviceInfo->maxWorkItemSizes[1]   = gcmMIN(threadCount, 1024);
    DeviceInfo->maxWorkItemSizes[2]   = gcmMIN(threadCount, 1024);
    DeviceInfo->maxWorkGroupSize      = hwCfg.maxWorkGroupSize;

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SHADER_ENHANCEMENTS2))
    {
        DeviceInfo->maxGlobalWorkSize     = (gctUINT64) 4*1024*1024*1024;
    }
    else
    {
        DeviceInfo->maxGlobalWorkSize     = 64*1024;
    }

    /* System configuration information */
    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_MMU))
    {
        gcmONERROR(gcoOS_GetPhysicalSystemMemorySize(&physicalSystemMemSize));
        /* CTS1.1 allocation tests are using global memory size
           CTS1.2 allocation tests are using global memory size. (Qcom changes)
           To avoid OOM in CTS allocation tests on some Android systems, limit CL_DEVICE_GLOBAL_MEM_SIZE no more than 256M.
           The first OCL1.2 submission actually reported 256M.
           Maybe we can increase that to 512M */
        gcmASSERT (physicalSystemMemSize >= 0x20000000);
        DeviceInfo->globalMemSize         = gcmMIN(physicalSystemMemSize / 2, 0x10000000);

        /* Follow FP spec here. The minimum CL_DEVICE_MAX_MEM_ALLOC_SIZE is max (1/4th of CL_DEVICE_GLOBAL_MEM_SIZE, 128*1024*1024).
          CTS thread dimension running time would be too long (>2 days) if CL_DEVICE_MAX_MEM_ALLOC_SIZE < 64M. */

        DeviceInfo->maxMemAllocSize       = gcmMAX(DeviceInfo->globalMemSize / 4, 0x8000000);
    }
    else
    {
        /* Report maxMemAllocSize and globalMemsize based on contiguous video mem size only on old chips without the MMU fix. */

        gcmONERROR(
        gcoOS_QueryVideoMemory(gcvNULL,
                               gcvNULL,            gcvNULL,
                               gcvNULL,            gcvNULL,
                               &contiguousPhysName, &contiguousSize));

        DeviceInfo->maxMemAllocSize       = contiguousSize / 4;
        DeviceInfo->globalMemSize         = contiguousSize / 2;
    }

    gcmONERROR(
        gcoHARDWARE_QueryShaderCapsEx(gcvNULL,
                                      &DeviceInfo->localMemSize,
                                      &DeviceInfo->addrBits,
                                      &DeviceInfo->globalMemCachelineSize,
                                      (gctUINT32_PTR)&DeviceInfo->globalMemCacheSize,
                                      &DeviceInfo->clockFrequency));

    DeviceInfo->localMemType          = 0x2         /* CL_GLOBAL */;
    DeviceInfo->globalMemCacheType    = 0x2         /* CL_READ_WRITE_CACHE */;

    /* Hardware capability: Constant size = 256*16B = 4KB
     * If unified constants were implemented, this would be 512*16B=8KB.
     * The constants can be divided any way the OpenCL software stack likes
     * between constant arguments, compiler needed constants, and the
     * required constants from the program itself.
     * The sum of these 3 cannot exceed the total number of constants.
     * maxConstantBufferSize = 256 * 16 - ConstantArgSize - ParameterSize
     */
    DeviceInfo->maxConstantBufferSize = 256 * 16;
    DeviceInfo->maxConstantArgs       = 9;
    DeviceInfo->maxParameterSize      = 256;

    DeviceInfo->maxPrintfBufferSize   = 1024 * 1024;

    /* Todo: need to check with HW the alignment requirement of BLT engine */
    DeviceInfo->memBaseAddrAlign      = 2048;

    /* Size (in bytes) of the largest OpenCL builtin data type (long16) */
    DeviceInfo->minDataTypeAlignSize  = 128;

    DeviceInfo->imageSupport          = gcvTRUE;

    /* Hardware capabilities.  14-bits per dimension */
    gcmONERROR(
        gcoHARDWARE_QueryTextureCaps(gcvNULL,
                                     &DeviceInfo->image3DMaxWidth,
                                     &DeviceInfo->image3DMaxHeight,
                                     &DeviceInfo->image3DMaxDepth,
                                     gcvNULL,
                                     gcvNULL,
                                     gcvNULL,
                                     &DeviceInfo->maxReadImageArgs,
                                     gcvNULL));

    DeviceInfo->image2DMaxWidth       = DeviceInfo->image3DMaxWidth;
    DeviceInfo->image2DMaxHeight      = DeviceInfo->image3DMaxHeight;

    if (DeviceInfo->image3DMaxDepth <= 1)
    {
        /* No image3D support. */
        DeviceInfo->image3DMaxWidth  = 0;
        DeviceInfo->image3DMaxHeight = 0;
        DeviceInfo->image3DMaxDepth  = 0;
    }

    DeviceInfo->imageMaxBufferSize   = 65536;

    DeviceInfo->maxReadImageArgs      = 8;

    /* Not limited by hardware.  Implemented by software.
     * Full profile requires 8.  Embedded profile requires 1.
     */
    DeviceInfo->maxWriteImageArgs     = 8;

    /* These should come from hardware capabilities based on the feature bit
     * that indicates higher perf math for 8-bits and 16-bits relative to 32-bits.
     */
    DeviceInfo->vectorWidthChar       = 4;      /* gc4000:16,   gc2100:4 */
    DeviceInfo->vectorWidthShort      = 4;      /* gc4000:8,    gc2100:4 */
    DeviceInfo->vectorWidthInt        = 4;
    DeviceInfo->vectorWidthLong       = 0;      /* unsupported extension */
    DeviceInfo->vectorWidthFloat      = 4;
    DeviceInfo->vectorWidthDouble     = 0;      /* unsupported extension */
    DeviceInfo->vectorWidthHalf       = 0;      /* unsupported extension */

    /* Until VS/PS samplers are unified for OpenCL.
     * Not presently in any product.
     * Full profile requires 16.  Embedded profile requires 8.
     */
    DeviceInfo->maxSamplers           = 8;

    /* These are software-stack dependent. */
    DeviceInfo->queueProperties       = 0x3;
                                    /*  CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                                        CL_QUEUE_PROFILING_ENABLE;
                                    */

    /* System/driver dependent */
    DeviceInfo->hostUnifiedMemory     = gcvTRUE;

    /* Hardware capability */
    DeviceInfo->errorCorrectionSupport= gcvTRUE;

    /* Not supported:   CL_FP_DENORM
     *                  CL_FP_SOFT_FLOAT
     *                  CL_FP_ROUND_TO_NEAREST
     *                  CL_FP_ROUND_TO_INF
     *                  CL_FP_INF_NAN
     *                  CL_FP_FMA
     *
     * MAD/FMA is supported, but IEEE754-2008 compliance is uncertain
     */
    DeviceInfo->singleFpConfig        = 0x8;    /* CL_FP_ROUND_TO_ZERO */

    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SHADER_HAS_RTNE))
    {
        DeviceInfo->singleFpConfig   |= 0x4;    /* CL_FP_ROUND_TO_NEAREST */
    }
    gcoHAL_QueryChipIdentity(gcvNULL,&chipModel,&chipRevision,gcvNULL,gcvNULL);
    DeviceInfo->chipModel = chipModel;
    DeviceInfo->chipRevision = chipRevision;
    chipEnableEP = ((chipModel == gcv1500 && chipRevision == 0x5246) ||
                    (chipModel == gcv2000 && chipRevision == 0x5108) ||
                    (chipModel == gcv3000 && chipRevision == 0x5513) ||
                    (chipModel == gcv5000));
    if(gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SHADER_HAS_ATOMIC) &&
       gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SHADER_HAS_RTNE) &&
       (chipEnableEP == gcvFALSE))
    {
        DeviceInfo->singleFpConfig |= 0x2; /* CL_FP_INF_NAN */
        DeviceInfo->localMemSize    = 32*1024;
        DeviceInfo->maxSamplers     = 16;
        DeviceInfo->maxReadImageArgs      = 128;
        DeviceInfo->maxParameterSize      = 1024;
        DeviceInfo->maxConstantBufferSize = 64*1024;
        DeviceInfo->vectorWidthLong       = 4;
    }

    DeviceInfo->doubleFpConfig        = 0;      /* unsupported extension */

    /* Computed from system configuration: mc_clk period in nanoseconds */
    DeviceInfo->profilingTimingRes    = 1000;   /* nanoseconds */

    /* System configuration dependent */
    DeviceInfo->endianLittle          = gcvTRUE;
    DeviceInfo->deviceAvail           = gcvTRUE;
    DeviceInfo->compilerAvail         = gcvTRUE;
    DeviceInfo->linkerAvail           = gcvTRUE;

    /* Not supported:   CL_EXEC_NATIVE_KERNEL */
    DeviceInfo->execCapability        = 0x1     /* CL_EXEC_KERNEL */;

    DeviceInfo->atomicSupport         = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SHADER_HAS_ATOMIC);

    /* VIP core: without graphich core, only compute core */
    DeviceInfo->computeOnlyGpu        = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_COMPUTE_ONLY);

    DeviceInfo->supportIMGInstr = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_IMG_INSTRUCTION);

    DeviceInfo->psThreadWalker  = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_CL_PS_WALKER);

    DeviceInfo->supportSamplerBaseOffset = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SAMPLER_BASE_OFFSET);

    DeviceInfo->maxRegisterCount = (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_GEOMETRY_SHADER) == gcvSTATUS_TRUE) ?
                                    109 : 113;

    DeviceInfo->TxIntegerSupport = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_INTEGER_COORDINATE) ||
                                   gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_TX_INTEGER_COORDINATE_V2);

    DeviceInfo->halti2 = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_HALTI2);

    DeviceInfo->multiWGPack  = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_SH_MULTI_WG_PACK);

    DeviceInfo->asyncBLT = gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT);

    status = gcvSTATUS_OK;

OnError:
    gcmFOOTER_ARG("status=%d", status);
    return status;
}


  /*  Three modes for openCL
   *      1. combined mode, one device use all gpucore.
   *         VIV_MGPU_AFFINITY=0, default mode
   *      2. independent mode, one device use one gpucore
   *         VIV_MGPU_AFFINITY=1:0 or 1:1,
   *      3. mulit-device mode, mulit device ,and every device will use one or more gpu count.
   *         this mode only work on independent mode.
   *         VIV_MGPU_AFFINITY=1:0
   *         VIV_OCL_USE_MULTI_DEVICE=1 or 1:1, 1:2,1:4, 1:2 means multi-device is enable, and every
   *         device will use 2 gpu core.
   *
   */

gceSTATUS
gcoCL_QueryDeviceCount(
    OUT gctUINT32 * DeviceCount,
    OUT gctUINT32 * GPUCountPerDevice
    )
{
    gctUINT chipIDs[32];
    gceMULTI_GPU_MODE mode;
    gctUINT coreIndex;
    gctSTRING attr;
    gctUINT32 gpuCount;
    static gctUINT gpuCountPerDevice = 1, deviceCount = 1;
    static gctBOOL queriedOnce = gcvFALSE;

    if(queriedOnce)
    {
        if(DeviceCount) *DeviceCount = deviceCount;
        if(GPUCountPerDevice) *GPUCountPerDevice = gpuCountPerDevice;

        return gcvSTATUS_OK;
    }

    queriedOnce = gcvTRUE;
    gcoHAL_QueryCoreCount(gcvNULL, gcvHARDWARE_3D, &gpuCount, chipIDs);

    if (gpuCount == 0)
    {
        gcoHAL_QueryCoreCount(gcvNULL, gcvHARDWARE_3D2D, &gpuCount, chipIDs);
    }

    gcoHAL_QueryMultiGPUAffinityConfig(gcvHARDWARE_3D, &mode, &coreIndex);

    if(mode == gcvMULTI_GPU_MODE_COMBINED)      /*Combined Mode*/
    {
        if(gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_USE_MULTI_DEVICES))
        {
            gcmPRINT("VIV Warning : VIV_OCL_USE_MULTI_DEVICES=1 only for INDEPENDENT mode");
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        gpuCountPerDevice = gpuCount;
        deviceCount = 1;
    }
    else    /* Indepedent mode*/
    {
        if(gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_USE_MULTI_DEVICES))  /*multi-device mode is enable */
        {
            gcoOS_GetEnv(gcvNULL, "VIV_OCL_USE_MULTI_DEVICE", &attr);

            if(attr && attr[0] == '1')
            {
                gpuCountPerDevice = 1;

                if(attr[1] == ':' && ( attr[2] == '2' || attr[2] == '4' || attr[2] =='1' ))
                {
                    gpuCountPerDevice = attr[2] - '0';
                }
                else if (attr[1] != '\0')
                {
                    gcmPRINT("VIV Warning : VIV_OCL_USE_MULIT_DEVICES only supporte 1 | 1:1 | 1:2 | 1:4");
                }
            }

            if((gpuCount % gpuCountPerDevice != 0) || (gpuCountPerDevice > gpuCount))
            {
                gcmPRINT("VIV Warning: Invalid VIV_OCL_USE_MULIT_DEVICES Env vars PerDevivceGPUCount is invalid");
                return gcvSTATUS_INVALID_ARGUMENT;
            }

            deviceCount = gpuCount / gpuCountPerDevice;
        }
        else    /* Independent mode , one device and device has only one gpucore */
        {
            gpuCountPerDevice = 1;
            deviceCount = 1;

            if(coreIndex >= gpuCount) /*coreIndex need small than maxCoreCount*/
            {
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }
    }

    if(DeviceCount) *DeviceCount = deviceCount;

    if(GPUCountPerDevice) *GPUCountPerDevice = gpuCountPerDevice;

    return gcvSTATUS_OK;
}


/*
 Example 2, a client wants to use 3D GPU0 and 3D GPU1 separately, it should do like this:
**      a) gcoHAL_SetHardwareType(gcvHARDWARE_3D)
**      b) gcoHAL_SetCurrentCoreIndex(0)
**      c) hardware0 = gcoHARDWARE_Construct()
**      d) gcoHARDWARE_SetMultiGPUMode(hardware0, gcvMULTI_GPU_MODE_INDEPENENT)
**      e) gcoHAL_SetCurrentCoreIndex(1)
**      f) hardware1 = gcoHARDWARE_Construct()
**      g) gcoHARDWARE_SetMultiGPUMode(hardware1, gcvMULTI_GPU_MODE_INDEPENENT)
**      h) gcoHAL_SetCurrentCoreIndex(0)
**      i) submit hardware0 related command to kernel
**      j) gcoHAL_SetCurrentCoreIndex(1)
**      k) submit hardware1 related command to kernel
*/

gceSTATUS
gcoCL_CreateHW(
    IN gctUINT32    DeviceId,
    OUT gcoHARDWARE * Hardware
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoHARDWARE  hardware = gcvNULL;
    gctUINT      gpuCountPerDevice, deviceCount;
    gctUINT32    gpuCoreIndexs[]={0, 1, 2, 3, 4, 5, 6, 7};
    gceMULTI_GPU_MODE  mode;
    gctUINT32 mainCoreIndex;
    gcmDECLARE_SWITCHVARS;
    gcmHEADER_ARG("DeviceId=%d", DeviceId);
    gcmSWITCH_TO_DEFAULT();
    gcoHAL_SetHardwareType(gcvNULL,gcvHARDWARE_3D);

    gcmONERROR(gcoCL_QueryDeviceCount(&deviceCount, &gpuCountPerDevice));

     if(deviceCount == 1 && gpuCountPerDevice == 1) /*Special deal with independent mode*/
    {
         gcoHAL_QueryMultiGPUAffinityConfig(gcvHARDWARE_3D, &mode, &mainCoreIndex);

         gpuCoreIndexs[0] = mainCoreIndex;
    }

    gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, gpuCoreIndexs[DeviceId * gpuCountPerDevice]));


    gcmONERROR(gcoHARDWARE_ConstructEx(gcPLS.hal,
                                       gcvFALSE,
                                       gcvFALSE,
                                       gcvHARDWARE_3D,
                                       gpuCountPerDevice,
                                       &gpuCoreIndexs[DeviceId * gpuCountPerDevice],
                                       &hardware));

    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_MCFE))
    {
        gcoHARDWARE_SelectChannel(hardware, 0, 1);
    }
    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(hardware, gcvPIPE_3D, gcvNULL));

    /* Set HAL API to OpenCL only when there is API is not set. */
    gcmVERIFY_OK(gcoHARDWARE_SetAPI(hardware, gcvAPI_OPENCL));

    if (!gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_PIPE_CL))
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Set rounding mode */
    if (gcoHARDWARE_IsFeatureAvailable(hardware, gcvFEATURE_SHADER_HAS_RTNE))
    {
        gcmVERIFY_OK(gcoHARDWARE_SetRTNERounding(hardware, gcvTRUE));
    }
    gcmONERROR(gcoHARDWARE_SetFenceEnabled(hardware, gcvTRUE));
    gcoHARDWARE_InitializeCL(hardware);
    gcmRESTORE_HW();
    *Hardware = hardware;
     gcmFOOTER();
    return status;

OnError:
    gcmRESTORE_HW();
    if(hardware)
    {
        gcoHARDWARE_Destroy(hardware, gcvFALSE);
    }

    gcmFOOTER();
    return status;
}


gceSTATUS
    gcoCL_DestroyHW(
    gcoHARDWARE  Hardware
    )
 {
     gcoHARDWARE_Destroy((gcoHARDWARE)Hardware,gcvFALSE);
     return gcvSTATUS_OK;
 }


gceSTATUS
    gcoCL_GetHWConfigGpuCount(
     gctUINT32 * GpuCount
    )
{
    gcoHAL_Query3DCoreCount(gcvNULL, GpuCount);
    return gcvSTATUS_OK;
}

/*
** For OCL Mulit-Device  we need flush more GPU cache for every GPU 's commit .
** The resource may be used in GPU1 but be released on GPU0 ,the event only trigger to
** flush GPU0's internel cache but the cache is still in  GPU1.
** So we flush all cache in user command in every OCL's commit for safe.
**
*/

gceSTATUS gcoCL_MultiDevcieCacheFlush()
{
    return gcoHARDWARE_MultiGPUCacheFlush(gcvNULL, gcvNULL);
}

/*******************************************************************************
**
**  gcoCL_Commit
**
**  Commit the current command buffer to hardware and optionally wait until the
**  hardware is finished.
**
**  INPUT:
**
**      gctBOOL Stall
**          gcvTRUE if the thread needs to wait until the hardware has finished
**          executing the committed command buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoCL_Commit(
    IN gctBOOL Stall
    )
{
    gceSTATUS status;
    gctUINT32  deviceCount;

    gcmHEADER_ARG("Stall=%d", Stall);

    gcoCL_QueryDeviceCount(&deviceCount, gcvNULL);

    if(deviceCount > 1)
    {
        gcmONERROR(gcoCL_MultiDevcieCacheFlush());
    }

    /* Commit the command buffer to hardware. */
    gcmONERROR(gcoHARDWARE_Commit(gcvNULL));

    if (Stall)
    {
        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall(gcvNULL));
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_Flush(
    IN gctBOOL      Stall
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Stall=%d", Stall);

    /* Flush the current pipe. */
    gcmONERROR(gcoHARDWARE_FlushPipe(gcvNULL, gcvNULL));

    gcmONERROR(gcoCL_Commit(Stall));

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_CreateSignal
**
**  Create a new signal.
**
**  INPUT:
**
**      gctBOOL ManualReset
**          If set to gcvTRUE, gcoOS_Signal with gcvFALSE must be called in
**          order to set the signal to nonsignaled state.
**          If set to gcvFALSE, the signal will automatically be set to
**          nonsignaled state by gcoOS_WaitSignal function.
**
**  OUTPUT:
**
**      gctSIGNAL * Signal
**          Pointer to a variable receiving the created gctSIGNAL.
*/
gceSTATUS
gcoCL_CreateSignal(
    IN gctBOOL ManualReset,
    OUT gctSIGNAL * Signal
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("ManualReset=%u Signal=0x%x", ManualReset, Signal);

    status = gcoOS_CreateSignal(gcvNULL, ManualReset, Signal);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_DestroySignal
**
**  Destroy a signal.
**
**  INPUT:
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoCL_DestroySignal(
    IN gctSIGNAL Signal
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Signal=0x%x", Signal);

    status = gcoOS_DestroySignal(gcvNULL, Signal);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_SubmitSignal(
    IN gctSIGNAL    Signal,
    IN gctHANDLE    Process,
    IN gceENGINE    Engine
    )
{
    gceSTATUS       status = gcvSTATUS_OK;
#if COMMAND_PROCESSOR_VERSION > 1
    gcsTASK_SIGNAL_PTR signal;
#else
    gcsHAL_INTERFACE iface;
#endif

    gcmHEADER_ARG("Signal=0x%x Process=0x%x", Signal, Process);


    /* Sometime we disable BLT engine */
    if (Signal == gcvNULL)
    {
        gcmFOOTER();
        return status;
    }

#if COMMAND_PROCESSOR_VERSION > 1
#if gcdGC355_PROFILER
    /* Allocate a cluster event. */
    gcmONERROR(gcoHAL_ReserveTask(gcvNULL,
                                  gcvNULL,
                                  0,0,0,
                                  gcvBLOCK_PIXEL,
                                  1,
                                  gcmSIZEOF(gcsTASK_SIGNAL),
                                  (gctPOINTER *) &signal));
#else
    gcmONERROR(gcoHAL_ReserveTask(gcvNULL,
                                  gcvBLOCK_PIXEL,
                                  1,
                                  gcmSIZEOF(gcsTASK_SIGNAL),
                                  (gctPOINTER *) &signal));
#endif
    /* Fill in event info. */
    signal->id       = gcvTASK_SIGNAL;
    signal->process  = Process;
    signal->signal   = Signal;
#else
    iface.command            = gcvHAL_SIGNAL;
    iface.engine             = Engine;
    iface.u.Signal.signal    = gcmPTR_TO_UINT64(Signal);
    iface.u.Signal.auxSignal = 0;
    iface.u.Signal.process   = gcmPTR_TO_UINT64(Process);
    iface.u.Signal.fromWhere = gcvKERNEL_COMMAND;

    gcmONERROR(gcoHARDWARE_CallEvent(gcvNULL, &iface));

    /* Commit the buffer. */
    gcmONERROR(gcoHARDWARE_Commit(gcvNULL));
#endif

OnError:

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_WaitSignal
**
**  Wait for a signal to become signaled.
**
**  INPUT:
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctUINT32 Wait
**          Number of milliseconds to wait.
**          Pass the value of gcvINFINITE for an infinite wait.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoCL_WaitSignal(
    IN gctSIGNAL Signal,
    IN gctUINT32 Wait
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Signal=0x%x Wait=%u", Signal, Wait);

    status = gcoOS_WaitSignal(gcvNULL, Signal, Wait);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCL_SetSignal
**
**  Make a signal to become signaled.
**
**  INPUT:
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoCL_SetSignal(
    IN gctSIGNAL Signal
    )
{
    gceSTATUS    status = gcvSTATUS_OK;

    gcmHEADER_ARG("Signal=0x%x", Signal);

    if (Signal != gcvNULL)
    {
        status = gcoOS_Signal(gcvNULL, Signal, gcvTRUE);
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                                gcoCL_LoadKernel
********************************************************************************
**
**  Load a pre-compiled and pre-linked kernel program into the hardware.
**
**  INPUT:
**
**      gcsPROGRAM_STATE *ProgramState
**          Program state pointer.
*/
gceSTATUS
gcoCL_LoadKernel(
    IN gcsPROGRAM_STATE *ProgramState
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("StateBufferSize=%u StateBuffer=0x%x Hints=0x%x",
                  ProgramState->stateBufferSize, ProgramState->stateBuffer, ProgramState->hints);

    /* Load kernel states. */
    status = gcLoadShaders(gcvNULL,
                          ProgramState);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_InvokeThreadWalker(
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;
    gceAPI currentApi;


    gcmHEADER_ARG("Info=0x%x", Info);

    /* Get Current API. */
    gcmVERIFY_OK(gcoHARDWARE_GetAPI(gcvNULL, &currentApi, gcvNULL));

    if (currentApi != gcvAPI_OPENCL)
    {
        /* Set HAL API to OpenCL. */
        gcmVERIFY_OK(gcoHARDWARE_SetAPI(gcvNULL, gcvAPI_OPENCL));
    }

    /* Route to hardware. */
    status = gcoHARDWARE_InvokeThreadWalkerCL(gcvNULL, Info);

    if (currentApi != gcvAPI_OPENCL)
    {
        /* Restore HAL API. */
        gcmVERIFY_OK(gcoHARDWARE_SetAPI(gcvNULL, currentApi));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_InvokeKernel(
    IN gctUINT             WorkDim,
    IN size_t              GlobalWorkOffset[3],
    IN size_t              GlobalScale[3],
    IN size_t              GlobalWorkSize[3],
    IN size_t              LocalWorkSize[3],
    IN gctUINT             ValueOrder,
    IN gctBOOL             BarrierUsed,
    IN gctUINT32           MemoryAccessFlag,
    IN gctBOOL             bDual16
    )
{
    gcsTHREAD_WALKER_INFO   info;
    gceSTATUS               status = gcvSTATUS_OK;

    gcmHEADER_ARG("WorkDim=%d", WorkDim);

    gcoOS_ZeroMemory(&info, gcmSIZEOF(gcsTHREAD_WALKER_INFO));

    switch(WorkDim)
    {
    case 3:
        info.globalSizeZ     = (gctUINT32)GlobalWorkSize[2];
        info.globalOffsetZ   = (gctUINT32)GlobalWorkOffset[2];
        info.workGroupSizeZ  = LocalWorkSize[2] ? (gctUINT32)LocalWorkSize[2] : 1;
        info.workGroupCountZ = info.globalSizeZ / info.workGroupSizeZ;

    case 2:
        info.globalSizeY     = (gctUINT32)GlobalWorkSize[1];
        info.globalOffsetY   = (gctUINT32)GlobalWorkOffset[1];
        info.workGroupSizeY  = LocalWorkSize[1] ? (gctUINT32)LocalWorkSize[1] : 1;
        info.workGroupCountY = info.globalSizeY / info.workGroupSizeY;
    case 1:
    default:
        info.dimensions      = WorkDim;
        info.globalSizeX     = (gctUINT32)GlobalWorkSize[0];
        info.globalOffsetX   = (gctUINT32)GlobalWorkOffset[0];
        info.workGroupSizeX  = LocalWorkSize[0] ? (gctUINT32)LocalWorkSize[0] : 1;
        info.workGroupCountX = info.globalSizeX / info.workGroupSizeX;
        info.globalScaleZ     = (gctUINT32)GlobalScale[2];
        info.globalScaleY     = (gctUINT32)GlobalScale[1];
        info.globalScaleX     = (gctUINT32)GlobalScale[0];
        break;
    }

    info.traverseOrder    = 0;  /* XYZ */
    info.enableSwathX     = 0;
    info.enableSwathY     = 0;
    info.enableSwathZ     = 0;
    info.swathSizeX       = 0;
    info.swathSizeY       = 0;
    info.swathSizeZ       = 0;
    info.valueOrder       = ValueOrder;
    info.barrierUsed      = BarrierUsed;
    info.memoryAccessFlag = MemoryAccessFlag;
    info.bDual16          = bDual16;

    gcmONERROR(gcoCL_InvokeThreadWalker(&info));

OnError:
    gcmFOOTER_ARG("%d", status);
    return status;
}

gceSTATUS
gcoCL_MemWaitAndGetFence(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE Engine,
    IN gceFENCE_TYPE GetType,
    IN gceFENCE_TYPE WaitType
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Node=0x%x", Node);

    if(gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_FENCE) == gcvSTATUS_FALSE)
    {
        gcmFOOTER();
        return status;
    }

    if (Node)
    {
        if (Engine == gcvENGINE_CPU)
        {
            gcmONERROR(gcsSURF_NODE_WaitFence(Node, Engine, gcvENGINE_RENDER, WaitType));
            gcmONERROR(gcsSURF_NODE_WaitFence(Node, Engine, gcvENGINE_BLT, WaitType));
        }
        else if (Engine == gcvENGINE_RENDER)
        {
            gcmONERROR(gcsSURF_NODE_WaitFence(Node, Engine, gcvENGINE_BLT, WaitType));
        }
        else if (Engine == gcvENGINE_BLT)
        {
            gcmONERROR(gcsSURF_NODE_WaitFence(Node, Engine, gcvENGINE_RENDER, WaitType));
        }

        if ((Engine != gcvENGINE_INVALID) && (Engine != gcvENGINE_CPU) && (GetType != gcvFNECE_TYPE_INVALID))
        {
            status = gcoHARDWARE_AppendFence(gcvNULL, Node, Engine, GetType);
            /* status = gcsSURF_NODE_GetFence(Node, Engine, GetType);*/
        }
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoCL_ChooseBltEngine(
    IN gcsSURF_NODE_PTR node,
    OUT gceENGINE * engine
    )
{
    if (node)
    {
        if (gcoHAL_GetOption(gcvNULL, gcvOPTION_OCL_ASYNC_BLT) &&
            gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_BLIT))
        {
            if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_ASYNC_FE_FENCE_FIX))
            {
                *engine = gcvENGINE_BLT;
            }
            else
            {
                if (!gcoHARDWARE_IsFenceBack(gcvNULL, node->fenceCtx, gcvENGINE_RENDER, gcvFENCE_TYPE_ALL))
                {
                    *engine = gcvENGINE_RENDER;
                }
                else
                {
                    *engine = gcvENGINE_BLT;
                }
            }
        }
        else
        {
            *engine = gcvENGINE_RENDER;
        }

        return gcvSTATUS_OK;
    }

    *engine = gcvENGINE_RENDER;

    return gcvSTATUS_INVALID_ARGUMENT;
}

gceSTATUS
gcoCL_MemBltCopy(
    IN gctUINT32 SrcAddress,
    IN gctUINT32 DestAddress,
    IN gctUINT32 CopySize,
    IN gceENGINE engine
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoHARDWARE Hardware = gcvNULL;
    gcmHEADER();

    gcmGETHARDWARE(Hardware);
    gcmONERROR(gcoHARDWARE_3DBlitCopy(gcvNULL, engine, SrcAddress, DestAddress, CopySize, gcvFALSE));


    gcoHARDWARE_OnIssueFence(gcvNULL, engine);
    gcoHARDWARE_SendFence(gcvNULL, gcvFALSE, engine, gcvNULL);


OnError:
    gcmFOOTER();
    return status;
}

#endif /* gcdENABLE_3D */

