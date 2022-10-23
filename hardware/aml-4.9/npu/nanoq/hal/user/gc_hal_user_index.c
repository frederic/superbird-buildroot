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

#if gcdNULL_DRIVER < 2

#define _GC_OBJ_ZONE            gcdZONE_INDEX

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

/* Number of hashed ranges. */
#define gcvRANGEHASHNUMBER      16
/* Number of spilit index ranges. */
#define SPILIT_INDEX_OFFSET       48
#define SPILIT_INDEX_CHUNCK_BYTE  64

#define COMPUTE_LAST_INDEX_ADDR(LastBufAddr, indexSize)  ((LastBufAddr) - (indexSize))

/* Index buffer range information. */
typedef struct _gcsINDEXRANGE
{
    gctSIZE_T                   offset;
    gctUINT32                   count;
    gctUINT32                   minIndex;
    gctUINT32                   maxIndex;
}
* gcsINDEXRANGE_PTR;

/* Dynamic buffer management. */
typedef struct _gcsINDEX_DYNAMIC
{
    gctUINT32                   physical;
    gctUINT8_PTR                logical;
    gctSIGNAL                   signal;

    gctSIZE_T                   bytes;
    gctSIZE_T                   free;

    gctUINT32                   lastStart;
    gctUINT32                   lastEnd;

    /* For dynamic allocation */
    gcsSURF_NODE                memory;
    struct _gcsINDEX_DYNAMIC *  next;
}
* gcsINDEX_DYNAMIC_PTR;

/* gcoINDEX object. */
struct _gcoINDEX
{
    /* Object. */
    gcsOBJECT                   object;

    /* Number of bytes allocated for memory. */
    gctSIZE_T                   bytes;

    /* Index buffer range hash table. */
    struct _gcsINDEXRANGE       indexRange[gcvRANGEHASHNUMBER];

    /* Index buffer node. */
    gcsSURF_NODE                memory;

    /* Dynamic management. */
    gctUINT                     dynamicCount;
    gcsINDEX_DYNAMIC_PTR        dynamic;
    gcsINDEX_DYNAMIC_PTR        dynamicHead;
    gcsINDEX_DYNAMIC_PTR        dynamicTail;

    /* For runtime allocation */
    gctUINT                     dynamicCurrent;
    gctSIZE_T                   dynamicCacheSize;
    gctUINT                     dynamicAllocatedCount;
    gctBOOL                     dynamicAllocate;

    struct
    {
        gctUINT32               hasIndexFetchFix        : 1;
    }hwFeature;
};

/******************************************************************************\
******************************* gcoINDEX API Code ******************************
\******************************************************************************/

static gceSTATUS
_FreeDynamic(
    IN gcsINDEX_DYNAMIC_PTR Dynamic
    );

gceSTATUS
gcoINDEX_UploadDynamicEx2(
                          IN gcoINDEX Index,
                          IN gceINDEX_TYPE IndexType,
                          IN gctCONST_POINTER Data,
                          IN gctSIZE_T Bytes,
                          IN gctBOOL ConvertToIndexedTriangleList
                          );

/*******************************************************************************
**
**  gcoINDEX_Construct
**
**  Construct a new gcoINDEX object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoINDEX * Index
**          Pointer to a variable that will receive the gcoINDEX object pointer.
*/
gceSTATUS
gcoINDEX_Construct(
    IN gcoHAL Hal,
    OUT gcoINDEX * Index
    )
{
    gcoOS os;
    gceSTATUS status;
    gcoINDEX index;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Index != gcvNULL);

    /* Extract the gcoOS object pointer. */
    os = gcvNULL;

    /* Allocate the gcoINDEX object. */
    gcmONERROR(gcoOS_Allocate(os,
                              gcmSIZEOF(struct _gcoINDEX),
                              &pointer));

    index = pointer;

    /* Initialize the gcoINDEX object. */
    index->object.type = gcvOBJ_INDEX;

    /* Reset ranage values. */
    gcoOS_ZeroMemory(&index->indexRange, gcmSIZEOF(index->indexRange));

    /* No attributes and memory assigned yet. */
    index->bytes                 = 0;
    index->memory.pool           = gcvPOOL_UNKNOWN;
    index->memory.valid          = gcvFALSE;
    index->dynamic               = gcvNULL;
    index->dynamicCount          = 0;

    index->dynamicAllocatedCount = 0;
    index->dynamicCacheSize      = 0;
    index->dynamicAllocate       = gcvFALSE;
    index->dynamicCurrent        = 0;

    index->hwFeature.hasIndexFetchFix = gcoHAL_IsFeatureAvailable(gcvNULL,gcvFEATURE_INDEX_FETCH_FIX);

    /* Return pointer to the gcoINDEX object. */
    *Index = index;

    /* Success. */
    gcmFOOTER_ARG("*Index=0x%x", *Index);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Destroy
**
**  Destroy a gcoINDEX object.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Destroy(
    IN gcoINDEX Index
    )
{
    gcsINDEX_DYNAMIC_PTR dynamic;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    if (Index->dynamic != gcvNULL)
    {
        if (Index->dynamicAllocate)
        {
            gctUINT i;
            gcsINDEX_DYNAMIC_PTR dynamic;

            for (i = 0; i < Index->dynamicCount; i++)
            {
                dynamic = Index->dynamic + i;

                _FreeDynamic(dynamic);

                /* Free all signal creations. */
                gcmVERIFY_OK(
                    gcoOS_DestroySignal(gcvNULL, dynamic->signal));
            }

            Index->dynamicAllocatedCount = 0;
            Index->dynamicCacheSize      = 0;
            Index->dynamicCurrent        = 0;
            Index->dynamicCount          = 0;
        }
        else
        {
            /* Free all signal creations. */
            for (dynamic = Index->dynamicHead;
                dynamic != gcvNULL;
                dynamic = dynamic->next)
            {
                gcmVERIFY_OK(
                    gcoOS_DestroySignal(gcvNULL, dynamic->signal));
            }
        }

        /* Free the buffer structures. */
        gcmVERIFY_OK(
            gcmOS_SAFE_FREE(gcvNULL, Index->dynamic));

        /* No buffers allocated. */
        Index->dynamic = gcvNULL;
    }

    /* Free the index buffer. */
    gcmVERIFY_OK(gcoINDEX_Free(Index));

    /* Mark the gcoINDEX object as unknown. */
    Index->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcoINDEX object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Index));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoINDEX_GetFence(
    IN gcoINDEX Index
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Index=0x%x", Index);

    if (Index)
    {
        status = gcsSURF_NODE_GetFence(&Index->memory, gcvENGINE_RENDER, gcvFENCE_TYPE_READ);
    }

    gcmFOOTER();
    return status;

}

gceSTATUS
gcoINDEX_WaitFence(
    IN gcoINDEX Index,
    IN gceFENCE_TYPE Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Index=0x%x Type=%d", Index, Type);

    if (Index)
    {
        status = gcsSURF_NODE_WaitFence(&Index->memory, gcvENGINE_CPU, gcvENGINE_RENDER, Type);
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Lock
**
**  Lock index in memory.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Physical address of the index buffer.
**
**      gctPOINTER * Memory
**          Logical address of the index buffer.
*/
gceSTATUS
gcoINDEX_Lock(
    IN gcoINDEX Index,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Do we have a buffer allocated? */
    if (Index->memory.pool == gcvPOOL_UNKNOWN)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Lock the index buffer. */
    gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                Address,
                                Memory));

    /* Success. */
    gcmFOOTER_ARG("*Address=0x%08x *Memory=0x%x",
                  gcmOPT_VALUE(Address), gcmOPT_POINTER(Memory));
    return status;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Unlock
**
**  Unlock index that was previously locked with gcoINDEX_Lock.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoINDEX_Unlock(
    IN gcoINDEX Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Do we have a buffer allocated? */
    if (Index->memory.pool == gcvPOOL_UNKNOWN)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Unlock the index buffer. */
    gcmONERROR(gcoHARDWARE_Unlock(&Index->memory,
                                  gcvSURF_INDEX));

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
**  gcoINDEX_Load
**
**  Upload index data into the memory.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**      gctUINT32 IndexCount
**          Number of indices in the buffer.
**
**      gctPOINTER IndexBuffer
**          Pointer to the index buffer.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Load(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctUINT32 IndexCount,
    IN gctPOINTER IndexBuffer
    )
{
    gceSTATUS status;
    gctUINT32 indexSize;
    gctUINT32 indexBufferSize;
    gctUINT32 address;
    gctUINT32 endAddress;
    gctUINT32 bufSize;

    gcmHEADER_ARG("Index=0x%x IndexType=%d IndexCount=%u IndexBuffer=0x%x",
                  Index, IndexType, IndexCount, IndexBuffer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(IndexCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(IndexBuffer != gcvNULL);

    /* Get index size from its type. */
    switch (IndexType)
    {
    case gcvINDEX_8:
        indexSize = 1;
        break;

    case gcvINDEX_16:
        indexSize = 2;
        break;

    case gcvINDEX_32:
        indexSize = 4;
        break;

    default:
        indexSize = 0;
        gcmASSERT(gcvFALSE);
    }

    /* Compute index buffer size. */
    indexBufferSize = indexSize * (IndexCount + 1);

    if (Index->bytes < indexBufferSize)
    {
        /* Free existing index buffer. */
        gcmONERROR(gcoINDEX_Free(Index));

        /* Allocate video memory. */
        gcmONERROR(gcsSURF_NODE_Construct(
            &Index->memory,
            indexBufferSize,
            64,
            gcvSURF_INDEX,
            gcvALLOC_FLAG_NONE,
            gcvPOOL_DEFAULT
            ));

        Index->bytes = indexBufferSize;

        /* Lock the index buffer. */
        gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                    gcvNULL,
                                    gcvNULL));
    }

    /* Upload the index buffer. */
    gcmONERROR(gcoINDEX_Upload(Index, IndexBuffer, indexBufferSize));

    gcmGETHARDWAREADDRESS(Index->memory, address);

    gcmSAFECASTSIZET(bufSize,Index->memory.size);

    endAddress = address + bufSize - 1;

    /* Program index buffer states. */
    gcmONERROR(gcoHARDWARE_BindIndex(gcvNULL,
                                     address,
                                     endAddress,
                                     IndexType,
                                     Index->bytes,
                                     0xFFFFFFFF));

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
**  gcoINDEX_Bind
**
**  Bind the index object to the hardware.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Bind(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    gceSTATUS status;
    gctUINT32 address;
    gctUINT32 endAddress;
    gctUINT32 bufSize;

    gcmHEADER_ARG("Index=0x%x Type=%d", Index, Type);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    if (Index->dynamic != gcvNULL)
    {
        address = Index->dynamicHead->physical
                + Index->dynamicHead->lastStart;

#if gcdENABLE_KERNEL_FENCE
        gcoHARDWARE_SetHWSlot(gcvNULL, gcvENGINE_RENDER, gcvHWSLOT_INDEX, 0, 0);
#endif
        gcmSAFECASTSIZET(bufSize, Index->dynamicHead->memory.size);
        endAddress = Index->dynamicHead->physical + bufSize - 1;


    }
    else
    {
        gcmGETHARDWAREADDRESS(Index->memory, address);
        gcmSAFECASTSIZET(bufSize, Index->memory.size);
        endAddress = address + bufSize -1;
#if gcdENABLE_KERNEL_FENCE
        gcoHARDWARE_SetHWSlot(gcvNULL, gcvENGINE_RENDER, gcvHWSLOT_INDEX, Index->memory.u.normal.node, 0);
#endif
    }

    /* Program index buffer states. */
    status = gcoHARDWARE_BindIndex(gcvNULL,
                                   address,
                                   endAddress,
                                   Type,
                                   Index->bytes,
                                   0xFFFFFFFF);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_BindOffset
**
**  Bind the index object to the hardware.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**      gctUINT32 Offset
**          Offset of stream.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_BindOffset(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset
    )
{
    gceSTATUS status;
    gctUINT32 address;
    gctUINT32 endAddress;
    gctUINT32 bufSize;

    gcmHEADER_ARG("Index=0x%x Type=%d Offset=%u", Index, Type, Offset);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    gcmGETHARDWAREADDRESS(Index->memory, address);
    gcmSAFECASTSIZET(bufSize,Index->memory.size);

    endAddress = address + bufSize - 1;

    /* Program index buffer states. */
    status = gcoHARDWARE_BindIndex(gcvNULL,
                                   address + Offset,
                                   endAddress,
                                   Type,
                                   Index->bytes - Offset,
                                   0xFFFFFFFF);
    gcmFOOTER();
    return status;
}

static gceSTATUS
_FreeDynamic(
    IN gcsINDEX_DYNAMIC_PTR Dynamic
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Dynamic=0x%x", Dynamic);

    /* Do we have a buffer allocated? */
    if (Dynamic->memory.pool != gcvPOOL_UNKNOWN)
    {
        /* Unlock the index buffer. */
        gcmONERROR(gcoHARDWARE_Unlock(&Dynamic->memory,
                                      gcvSURF_INDEX));

        /* Create an event to free the video memory. */
        gcmONERROR(gcsSURF_NODE_Destroy(&Dynamic->memory));

        /* Reset the pointer. */
        Dynamic->bytes        = 0;
        Dynamic->memory.pool  = gcvPOOL_UNKNOWN;
        Dynamic->memory.valid = gcvFALSE;
        Dynamic->free         = 0;
        Dynamic->lastEnd      = 0;
        Dynamic->lastStart    = ~0U;
        Dynamic->logical      = 0;
        Dynamic->physical     = 0;
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
_Free(
    IN gcoINDEX Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Do we have a buffer allocated? */
    if (Index->memory.pool != gcvPOOL_UNKNOWN)
    {
        /* Unlock the index buffer. */
        gcmONERROR(gcoHARDWARE_Unlock(&Index->memory,
                                      gcvSURF_INDEX));

        /* Create an event to free the video memory. */
        gcmONERROR(gcsSURF_NODE_Destroy(&Index->memory));

        /* Reset range values. */
        gcoOS_ZeroMemory(&Index->indexRange,
                         gcmSIZEOF(Index->indexRange));

        /* Reset the pointer. */
        Index->bytes        = 0;
        Index->memory.pool  = gcvPOOL_UNKNOWN;
        Index->memory.valid = gcvFALSE;
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
**  gcoINDEX_Free
**
**  Free existing index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Free(
    IN gcoINDEX Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Not available when the gcoINDEX is dynamic. */
    if (Index->dynamic != gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Perform the free. */
    gcmONERROR(_Free(Index));

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
**  gcoINDEX_Upload
**
**  Upload index data into an index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to a gcoINDEX object.
**
**      gctCONST_POINTER Buffer
**          Pointer to index data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoINDEX_Upload(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Buffer=0x%x Bytes=%lu", Index, Buffer, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Not availabe with dynamic buffers. */
    if (Index->dynamic != gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Reset ranage values. */
    gcoOS_ZeroMemory(&Index->indexRange, gcmSIZEOF(Index->indexRange));

    if (Index->bytes < Bytes)
    {
        /* Free any allocated video memory. */
        gcmONERROR(gcoINDEX_Free(Index));

        /* Allocate video memory. */
        gcmONERROR(gcsSURF_NODE_Construct(
            &Index->memory,
            Bytes,
            4,
            gcvSURF_INDEX,
            gcvALLOC_FLAG_NONE,
            gcvPOOL_DEFAULT
            ));

        /* Initialize index. */
        Index->bytes                = Bytes;

        /* Lock the index buffer. */
        gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                    gcvNULL,
                                    gcvNULL));
    }

    if (Buffer != gcvNULL)
    {
        /* Upload data into the stream. */
        gcmONERROR(gcoHARDWARE_CopyData(&Index->memory,
                                        0,
                                        Buffer,
                                        Bytes));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       gcvDUMP_BUFFER_INDEX,
                       gcsSURF_NODE_GetHWAddress(&Index->memory),
                       Index->memory.logical,
                       0,
                       Bytes);
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
**  gcoINDEX_UploadOffset
**
**  Upload index data into an index buffer at an offset.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to a gcoINDEX object.
**
**      gctUINT32 Offset
**          Offset into gcoINDEX buffer to start uploading.
**
**      gctCONST_POINTER Buffer
**          Pointer to index data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoINDEX_UploadOffset(
    IN gcoINDEX Index,
    IN gctSIZE_T Offset,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Offset=%u Buffer=0x%x Bytes=%lu",
                  Index, Offset, Buffer, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Not availabe with dynamic buffers. */
    if (Index->dynamic != gcvNULL)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_REQUEST);
        return gcvSTATUS_INVALID_REQUEST;
    }

    if (Offset + Bytes > Index->bytes)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_BUFFER_TOO_SMALL);
        return gcvSTATUS_BUFFER_TOO_SMALL;
    }

    if (Buffer != gcvNULL)
    {
        gcoINDEX_WaitFence(Index, gcvFENCE_TYPE_ALL);

        /* Reset ranage values. */
        gcoOS_ZeroMemory(&Index->indexRange, sizeof(Index->indexRange));

        /* Upload data into the stream. */
        status = gcoHARDWARE_CopyData(&Index->memory,
                                      Offset,
                                      Buffer,
                                      Bytes);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       gcvDUMP_BUFFER_INDEX,
                       gcsSURF_NODE_GetHWAddress(&Index->memory),
                       Index->memory.logical,
                       Offset,
                       Bytes);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
/************************************************************
**  gcoINDEX_CheckRange
**
**  Check if the draw is out of index buffer range
**
**  OUTPUT:
**
**      Nothing.
*/
gctBOOL
gcoINDEX_CheckRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctINT Count,
    IN gctUINT32  Indices
    )
{
    gctUINT32 indexSize;

    switch ( Type )
    {
        case gcvINDEX_8:
             indexSize = 1;
             break;

        case gcvINDEX_16:
             indexSize = 2;
             break;

        case gcvINDEX_32:
             indexSize = 4;
             break;

        default:
             return gcvFALSE;
    }

    indexSize = indexSize * Count;

    if(indexSize + Indices> Index->bytes)
    {
        return gcvFALSE;
    }
    else
    {
        return gcvTRUE;
    }
}

/*******************************************************************************
**
**  gcoINDEX_Merge
**
**  Merge index buffer 2 into index buffer 1.
**  index2 must be from 0, and the range must be part of buffer1
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoINDEX_Merge(
    IN gcoINDEX Index1,
    IN gcoINDEX Index2
    )
{
    gceSTATUS status;
    gctPOINTER buffer[3];

    gcmHEADER_ARG("Index1=0x%x Index2=0x%x", Index1, Index2);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index1, gcvOBJ_INDEX);
    gcmVERIFY_OBJECT(Index2, gcvOBJ_INDEX);

    /* Lock the index buffer. */
    gcmONERROR(gcoHARDWARE_Lock(&Index1->memory,
                                gcvNULL,
                                gcvNULL));

    gcmONERROR(gcoHARDWARE_Lock(&Index2->memory,
                                gcvNULL,
                                buffer));

    /* Upload data into the stream. */
    gcmONERROR(gcoHARDWARE_CopyData(&Index1->memory,
                                    0,
                                    buffer[0],
                                    Index2->bytes));

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_INDEX,
                   gcsSURF_NODE_GetHWAddress(&Index1->memory),
                   Index1->memory.logical,
                   0,
                   Index2->bytes);

    gcmONERROR(gcoHARDWARE_Unlock(&Index1->memory,
                                gcvSURF_INDEX));

    gcmONERROR(gcoHARDWARE_Unlock(&Index2->memory,
                                gcvSURF_INDEX));

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
**  gcoINDEX_QueryCaps
**
**  Query the index capabilities.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      gctBOOL * Index8
**          Pointer to a variable receiving the capability for 8-bit indices.
**
**      gctBOOL * Index16
**          Pointer to a variable receiving the capability for 16-bit indices.
**          target.
**
**      gctBOOL * Index32
**          Pointer to a variable receiving the capability for 32-bit indices.
**
**      gctUINT * MaxIndex
**          Maximum number of indices.
*/
gceSTATUS
gcoINDEX_QueryCaps(
    OUT gctBOOL * Index8,
    OUT gctBOOL * Index16,
    OUT gctBOOL * Index32,
    OUT gctUINT * MaxIndex
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Route to gcoHARDWARE. */
    status = gcoHARDWARE_QueryIndexCaps(gcvNULL, Index8, Index16, Index32, MaxIndex);
    gcmFOOTER_ARG("status=%d(%s) *Index8=%d *Index16=%d *Index32=%d *MaxIndex=%u",
                  status, gcoOS_DebugStatus2Name(status), gcmOPT_VALUE(Index8), gcmOPT_VALUE(Index16),
                  gcmOPT_VALUE(Index32), gcmOPT_VALUE(MaxIndex));
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_GetIndexRange
**
**  Determine the index range in the current index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that needs to be destroyed.
**
**      gceINDEX_TYPE Type
**          Index type.
**
**      gctUINT32 Offset
**          Offset into the buffer.
**
**      gctUINT32 Count
**          Number of indices to analyze.
**
**  OUTPUT:
**
**      gctUINT32 * MinimumIndex
**          Pointer to a variable receiving the minimum index value in
**          the index buffer.
**
**      gctUINT32 * MaximumIndex
**          Pointer to a variable receiving the maximum index value in
**          the index buffer.
*/
gceSTATUS
gcoINDEX_GetIndexRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctSIZE_T Offset,
    IN gctUINT32 Count,
    OUT gctUINT32 * MinimumIndex,
    OUT gctUINT32 * MaximumIndex
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Type=%d Offset=%u Count=%u",
                  Index, Type, Offset, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(MinimumIndex != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(MaximumIndex!= gcvNULL);

    do
    {
        /* Determine the hash table entry. */
        gctUINT32 index = gcoMATH_ModuloInt(Offset * 31 + Count + 31,
                                            gcvRANGEHASHNUMBER);

        /* Shortcut to the entry. */
        gcsINDEXRANGE_PTR entry = &Index->indexRange[index];

        /* Not yet determined? */
        if ((entry->offset != Offset) ||
            (entry->count  != Count))
        {
            gctUINT32 minIndex = ~0U;
            gctUINT32 maxIndex =  0;
            gctBOOL primRestart = gcoHARDWARE_IsPrimitiveRestart(gcvNULL) == gcvTRUE;

            /* Must have buffer. */
            if (Index->memory.pool == gcvPOOL_UNKNOWN)
            {
                status = gcvSTATUS_GENERIC_IO;
                break;
            }

            /* Assume no error. */
            status = gcvSTATUS_OK;

            /* Determine the range. */
            switch (Type)
            {
            case gcvINDEX_8:
                {
                    gctSIZE_T i;

                    gctUINT8_PTR indexBuffer
                        = Index->memory.logical
                        + Offset;

                    if (Offset + Count > Index->bytes)
                    {
                        status = gcvSTATUS_INVALID_ARGUMENT;
                        break;
                    }

                    for (i = 0; i < Count; i++)
                    {
                        gctUINT32 curIndex = *indexBuffer++;

                        if (primRestart && (curIndex == 0xFF))
                        {
                            continue;
                        }

                        if (curIndex < minIndex)
                        {
                            minIndex = curIndex;
                        }

                        if (curIndex > maxIndex)
                        {
                            maxIndex = curIndex;
                        }
                    }
                }
                break;

            case gcvINDEX_16:
                {
                    gctSIZE_T i;

                    gctUINT16_PTR indexBuffer
                        = (gctUINT16_PTR)(Index->memory.logical
                        + Offset);

                    if (Offset + Count * 2 > Index->bytes)
                    {
                        status = gcvSTATUS_INVALID_ARGUMENT;
                        break;
                    }

                    for (i = 0; i < Count; i++)
                    {
                        gctUINT32 curIndex = *indexBuffer++;

                        if (primRestart && (curIndex == 0xFFFF))
                        {
                            continue;
                        }

                        if (curIndex < minIndex)
                        {
                            minIndex = curIndex;
                        }

                        if (curIndex > maxIndex)
                        {
                            maxIndex = curIndex;
                        }
                    }
                }
                break;

            case gcvINDEX_32:
                {
                    gctSIZE_T i;

                    gctUINT32_PTR indexBuffer
                        = (gctUINT32_PTR)(Index->memory.logical
                        + Offset);

                    if (Offset + Count * 4 > Index->bytes)
                    {
                        status = gcvSTATUS_INVALID_ARGUMENT;
                        break;
                    }

                    for (i = 0; i < Count; i++)
                    {
                        gctUINT32 curIndex = *indexBuffer++;

                        if (primRestart && (curIndex == 0xFFFFFFFF))
                        {
                            continue;
                        }

                        if (curIndex < minIndex)
                        {
                            minIndex = curIndex;
                        }

                        if (curIndex > maxIndex)
                        {
                            maxIndex = curIndex;
                        }
                    }
                }
                break;
            }

            /* Set entry members. */
            entry->offset   = Offset;
            entry->count    = Count;
            entry->minIndex = minIndex;
            entry->maxIndex = maxIndex;
        }
        else
        {
            status = gcvSTATUS_OK;
        }

        /* Set results. */
        *MinimumIndex = entry->minIndex;
        *MaximumIndex = entry->maxIndex;
    }
    while (gcvFALSE);

    /* Return status. */
    if (gcmIS_SUCCESS(status))
    {
        gcmFOOTER_ARG("*MinimumIndex=%u *MaximumIndex=%u",
                      *MinimumIndex, *MaximumIndex);
    }
    else
    {
        gcmFOOTER();
    }
    return status;
}

/*******************************************************************************
*************************** DYNAMIC BUFFER MANAGEMENT **************************
*******************************************************************************/

static gceSTATUS
gcoINDEX_AllocateMemory(
    IN gcoINDEX Index,
    IN gctSIZE_T Bytes,
    IN gctUINT32 Buffers
    )
{
    gctSIZE_T bytes;
    gctUINT32 physical;
    gctPOINTER logical;
    gctUINT32 i, bytes32;
    gcsINDEX_DYNAMIC_PTR dynamic;
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Bytes=%lu Buffers=%u", Index, Bytes, Buffers);

    /* Free any memory. */
    gcmONERROR(_Free(Index));
    Index->dynamic->bytes = 0;

    /* Compute the number of total bytes. */
    bytes = gcmALIGN(Bytes, 64) * Buffers;

    /* Allocate the video memory. */
    gcmONERROR(gcsSURF_NODE_Construct(
        &Index->memory,
        bytes,
        64,
        gcvSURF_INDEX,
        gcvALLOC_FLAG_NONE,
        gcvPOOL_DEFAULT
        ));

    /* Initialize index. */
    Index->bytes = bytes;

    /* Lock the index buffer. */
    gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                &physical,
                                &logical));

    bytes = gcoMATH_DivideUInt(Index->bytes, Buffers);

    /* Initialize all buffer structures. */
    for (i = 0, dynamic = Index->dynamic; i < Buffers; ++i, ++dynamic)
    {
        /* Set buffer address. */
        dynamic->physical = physical;
        dynamic->logical  = logical;

        /* Set buffer size. */
        dynamic->bytes = bytes;
        dynamic->free  = bytes;

        /* Set usage. */
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        gcmSAFECASTSIZET(bytes32, bytes);
        /* Advance buffer addresses. */
        physical += bytes32;
        logical   = (gctUINT8_PTR) logical + bytes;
    }

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
gcoINDEX_AllocateDynamicMemory(
    IN gctSIZE_T Bytes,
    IN gcsINDEX_DYNAMIC_PTR Dynamic
    )
{
    gctSIZE_T bytes;
    gctUINT32 physical;
    gctPOINTER logical;
    gceSTATUS status;

    gcmHEADER_ARG("Bytes=%lu Dynamic=0x%x", Bytes, Dynamic);

    /* Free any memory. */
    gcmONERROR(_FreeDynamic(Dynamic));
    Dynamic->bytes = 0;

    /* Compute the number of total bytes. */
    bytes = gcmALIGN(Bytes, 64);

    /* Allocate the video memory. */
    gcmONERROR(gcsSURF_NODE_Construct(
        &Dynamic->memory,
        bytes,
        64,
        gcvSURF_INDEX,
        gcvALLOC_FLAG_NONE,
        gcvPOOL_DEFAULT
        ));

    /* Initialize index. */
    Dynamic->bytes = bytes;

    /* Lock the index buffer. */
    gcmONERROR(gcoHARDWARE_Lock(&Dynamic->memory,
                                &physical,
                                &logical));

    /* Initialize all buffer structures. */
    Dynamic->physical = physical;
    Dynamic->logical  = logical;
    Dynamic->bytes = bytes;
    Dynamic->free  = bytes;
    Dynamic->lastStart = ~0U;
    Dynamic->lastEnd   = 0;

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_SetDynamic
**
**  Mark the gcoINDEX object as dynamic.  A dynamic object will allocate the
**  specified number of buffers and upload data after the previous upload.  This
**  way there is no need for synchronizing the GPU or allocating new objects
**  each time an index buffer is required.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that needs to be converted to dynamic.
**
**      gctSIZE_T Bytes
**          Number of bytes per buffer.
**
**      gctUINT Buffers
**          Number of buffers.
*/
gceSTATUS
gcoINDEX_SetDynamic(
    IN gcoINDEX Index,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    )
{
    gceSTATUS status;
    gctUINT i;
    gcsINDEX_DYNAMIC_PTR dynamic;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Index=0x%x Bytes=%lu Buffers=%u", Index, Bytes, Buffers);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Buffers > 0);

    /* We can only do this once. */
    if (Index->dynamic != gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Allocate memory for the buffer structures. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              Buffers * gcmSIZEOF(struct _gcsINDEX_DYNAMIC),
                              &pointer));

    Index->dynamic = pointer;

    gcoOS_ZeroMemory(Index->dynamic,
                     Buffers * gcmSIZEOF(struct _gcsINDEX_DYNAMIC));

    /* Initialize all buffer structures. */
    for (i = 0, dynamic = Index->dynamic; i < Buffers; ++i, ++dynamic)
    {
        /* Create the signal. */
        gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvTRUE, &dynamic->signal));

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_SIGNAL,
            "%s(%d): index buffer %d signal created 0x%08X",
            __FUNCTION__, __LINE__,
            i, dynamic->signal);

        /* Mark buffer as usuable. */
        gcmONERROR(gcoOS_Signal(gcvNULL, dynamic->signal, gcvTRUE));

        /* Link buffer in chain. */
        dynamic->next = dynamic + 1;
    }

    if (1/*gcoHAL_QuerySpecialHint(gceSPECIAL_HINT4) == gcvSTATUS_FALSE*/)
    {
        /* Initilaize chain of buffer structures. */
        Index->dynamicAllocate   = gcvTRUE;
        Index->dynamicCount      = Buffers;
        Index->dynamicCacheSize  = Bytes;

        Index->dynamicAllocatedCount  = 0;
        Index->dynamicCurrent         = 0;

        Index->dynamicHead       = Index->dynamic;
        Index->dynamicTail       = Index->dynamic + Buffers - 1;
        Index->dynamicTail->next = gcvNULL;

        for (i = 0, dynamic = Index->dynamic; i < Buffers; ++i, ++dynamic)
        {
            /* Set buffer address. */
            dynamic->physical = 0;
            dynamic->logical  = 0;

            /* Set buffer size. */
            dynamic->bytes = 0;
            dynamic->free  = 0;

            /* Set usage. */
            dynamic->lastStart = ~0U;
            dynamic->lastEnd   = 0;

            dynamic->memory.pool = gcvPOOL_UNKNOWN;
            dynamic->memory.valid = gcvFALSE;
        }
    }
    else
    {

        Index->dynamicAllocate   = gcvFALSE;

        /* Initilaize chain of buffer structures. */
        Index->dynamicCount      = Buffers;
        Index->dynamicHead       = Index->dynamic;
        Index->dynamicTail       = Index->dynamic + Buffers - 1;
        Index->dynamicTail->next = gcvNULL;

        /* Allocate the memory. */
        gcmONERROR(gcoINDEX_AllocateMemory(Index, Bytes, Buffers));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Roll back allocation of buffer structures. */
    if (Index->dynamic != gcvNULL)
    {
        /* Roll back all signal creations. */
        for (i = 0; i < Buffers; ++i)
        {
            if (Index->dynamic[i].signal != gcvNULL)
            {
                gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL,
                                                 Index->dynamic[i].signal));
            }
        }

        /* Roll back the allocation of the buffer structures. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Index->dynamic));

        /* No buffers allocated. */
        Index->dynamic = gcvNULL;
    }

    /* Roll back memory allocation. */
    gcmVERIFY_OK(gcoINDEX_Free(Index));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
_PatchIndices(
    IN gctPOINTER PatchedIndices,
    IN gctCONST_POINTER Indices,
    IN gceINDEX_TYPE IndexType,
    IN gctSIZE_T Count
    )
{
    gctSIZE_T triangles = Count - 2;
    gctUINT i, j;
    gctPOINTER indices = PatchedIndices;
    gceSTATUS  status;

    /* Dispatch on index type. */
    switch (IndexType)
    {
    case gcvINDEX_8:
        {
            /* 8-bit indices. */
            gctUINT8 *dst;
            const gctUINT8 *src = (gctUINT8_PTR)Indices;

            dst = (gctUINT8 *)indices;
            for (i = 0, j = 0; i < triangles; i++)
            {
                dst[j * 3]     = src[(i % 2) == 0 ? i : i + 1];
                dst[j * 3 + 1] = src[(i % 2) == 0 ? i + 1 : i];
                dst[j * 3 + 2] = src[i + 2];

                j++;
            }
        }
        break;

    case gcvINDEX_16:
        /* 16-bit indices. */
        {
            gctUINT16 *dst;
            const gctUINT16 *src = (gctUINT16 *)Indices;

            dst = (gctUINT16 *)indices;
            for (i = 0, j = 0; i < triangles; i++)
            {
                dst[j * 3]     = src[(i % 2) == 0 ? i : i + 1];
                dst[j * 3 + 1] = src[(i % 2) == 0 ? i + 1 : i];
                dst[j * 3 + 2] = src[i + 2];

                j++;
            }
        }
        break;

    case gcvINDEX_32:
        /* 32-bit indices. */
        {
            gctUINT32 *dst;
            const gctUINT32 *src = (gctUINT32 *)Indices;

            dst = (gctUINT32 *)indices;
            for (i = 0, j = 0; i < triangles; i++)
            {
                dst[j * 3]     = src[(i % 2) == 0 ? i : i + 1];
                dst[j * 3 + 1] = src[(i % 2) == 0 ? i + 1 : i];
                dst[j * 3 + 2] = src[i + 2];

                j++;
            }
        }
        break;

    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    return gcvSTATUS_OK;
OnError:
    return status;
}

gceSTATUS
gcoINDEX_GetMemoryIndexRange(
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T indexCount,
    OUT gctUINT32* iMin,
    OUT gctUINT32* iMax
    )
{
    gceSTATUS status= gcvSTATUS_OK;
    gctUINT8_PTR src;
    gctSIZE_T count = 0;
    gctBOOL primRestart = gcvFALSE;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(indexCount > 0);

    /* Setup pointers for copying data. */
    src  = (gctUINT8_PTR) Data;
    primRestart = gcoHARDWARE_IsPrimitiveRestart(gcvNULL) == gcvTRUE;
    *iMin = ~0U;
    *iMax = 0;

    switch (IndexType)
    {
    case gcvINDEX_8:
        /* Copy all indices as 8-bit data. */
        for (count = 0; count < indexCount; ++count)
        {
            gctUINT8 index = *(gctUINT8_PTR) src++;

            /* Skip the primitive restart index */
            if (primRestart && (index == 0xFF))
            {
                continue;
            }

            /* Keep track of min/max index. */
            if (index < *iMin) *iMin = index;
            if (index > *iMax) *iMax = index;
        }
        break;

    case gcvINDEX_16:
        /* Copy all indices as 16-bit data. */
        for (count = 0; count < indexCount; ++count)
        {
            gctUINT16 index = *(gctUINT16_PTR) src;
            src += 2;

            /* Skip the primitive restart index */
            if (primRestart && (index == 0xFFFF))
            {
                continue;
            }

            /* Keep track of min/max index. */
            if (index < *iMin) *iMin = index;
            if (index > *iMax) *iMax = index;
        }
        break;

    case gcvINDEX_32:
        /* Copy all indices as 32-bit data. */
        for (count = 0; count < indexCount; ++count)
        {
            gctUINT32 index = *(gctUINT32_PTR) src;
            src += 4;

            /* Skip the primitive restart index */
            if (primRestart && (index == 0xFFFFFFFF))
            {
                continue;
            }

            /* Keep track of min/max index. */
            if (index < *iMin) *iMin = index;
            if (index > *iMax) *iMax = index;
        }
        break;
    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status.*/
    gcmFOOTER();
    return status;
}
/*******************************************************************************
**
**  gcoINDEX_UploadDynamicEx
**
**  Upload data into a dynamic index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that has been configured as dynamic.
**
**      gceINDEX_TYPE IndexType
**          Type of index data to upload.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer containing the data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes of data to upload.
**
**      gctBOOL ConvertToIndexedTriangleList
**          Whether convert stripped/fan indices to triangle list.
*/
gceSTATUS
gcoINDEX_UploadDynamicEx(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL ConvertToIndexedTriangleList
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;
    gcsINDEX_DYNAMIC_PTR dynamic;
    gctUINT32 aligned32, spilitIndexMod;
    gctUINT8_PTR src, dest;
    gctSIZE_T aligned;
    gctSIZE_T count = 0, convertedBytes = 0;
    gctUINT32 offset = 0;
    gctUINT indexSize = 0;

    gcmHEADER_ARG("Index=0x%x IndexType=%d Data=0x%x Bytes=%lu",
                  Index, IndexType, Data, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* If the index wasn't initialized as dynamic, do it now */
    if (Index->dynamic == gcvNULL)
    {
        gcmONERROR(gcoINDEX_SetDynamic(Index, 128 << 10, 32));
    }

    if (Index->dynamicAllocate)
    {
        status = gcoINDEX_UploadDynamicEx2(Index,
                                           IndexType,
                                           Data,
                                           Bytes,
                                           ConvertToIndexedTriangleList
                                           );

        gcmFOOTER_NO();
        return status;
    }

    /* Shorthand the pointer. */
    dynamic = Index->dynamicHead;
    gcmASSERT(dynamic != gcvNULL);

    /* Determine number of bytes in the index buffer. */
    switch (IndexType)
    {
    case gcvINDEX_8:
        indexSize = 1;
        break;

    case gcvINDEX_16:
        indexSize = 2;
        break;

    case gcvINDEX_32:
        indexSize = 4;
        break;

    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (ConvertToIndexedTriangleList)
    {
        /* Converting a triangle strip into triangle list. */
        count = Bytes / indexSize;
        convertedBytes = (3 * (count - 2)) * indexSize;
        Bytes = convertedBytes;
    }

    /* Make sure the dynamic index buffer is large enough to hold this data. */
    if (Bytes > dynamic->bytes)
    {
        /* Reallocate the index buffers. */
        gcmONERROR(gcoINDEX_AllocateMemory(Index,
                                           gcmALIGN(Bytes * 2, 4 << 10),
                                           Index->dynamicCount));
    }

    /* Compute the mod of spilit index chunck. */
    spilitIndexMod = COMPUTE_LAST_INDEX_ADDR(dynamic->physical+dynamic->lastEnd+Bytes, indexSize) % SPILIT_INDEX_CHUNCK_BYTE;
    if (!Index->hwFeature.hasIndexFetchFix &&  spilitIndexMod < SPILIT_INDEX_OFFSET)
    {
        offset = SPILIT_INDEX_OFFSET - spilitIndexMod;
        offset = gcmALIGN(offset, 4);
    }

    /* Compute number of aligned bytes. */
    aligned = gcmALIGN(Bytes + offset, 4);

    if (dynamic->free < aligned)
    {
        /* Not enough free bytes in this buffer, mark it busy. */
        gcmONERROR(gcoOS_Signal(gcvNULL,
                                dynamic->signal,
                                gcvFALSE));

        /* Schedule a signal event. */
        ioctl.command            = gcvHAL_SIGNAL;
        ioctl.engine             = gcvENGINE_RENDER;
        ioctl.u.Signal.signal    = gcmPTR_TO_UINT64(dynamic->signal);
        ioctl.u.Signal.auxSignal = 0;
        ioctl.u.Signal.process   = gcmPTR_TO_UINT64(gcoOS_GetCurrentProcessID());
        ioctl.u.Signal.fromWhere = gcvKERNEL_COMMAND;
        gcmONERROR(gcoHARDWARE_CallEvent(gcvNULL, &ioctl));

        /* Commit the buffer. */
        gcmONERROR(gcoHARDWARE_Commit(gcvNULL));

        /* Move it to the tail of the queue. */
        gcmASSERT(Index->dynamicTail != gcvNULL);
        Index->dynamicTail->next = dynamic;
        Index->dynamicTail       = dynamic;
        Index->dynamicHead       = dynamic->next;
        dynamic->next            = gcvNULL;
        gcmASSERT(Index->dynamicHead != gcvNULL);

        /* Reinitialize the top of the queue. */
        dynamic            = Index->dynamicHead;
        dynamic->free      = dynamic->bytes;
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        /* Compute the mod of spilit index chunck. */
        spilitIndexMod = COMPUTE_LAST_INDEX_ADDR(dynamic->physical+dynamic->lastEnd+Bytes, indexSize) % SPILIT_INDEX_CHUNCK_BYTE;

        if (!Index->hwFeature.hasIndexFetchFix &&  spilitIndexMod < SPILIT_INDEX_OFFSET)
        {
            offset= SPILIT_INDEX_OFFSET - spilitIndexMod;
            offset = gcmALIGN(offset, 4);
            aligned = gcmALIGN(Bytes + offset, 4);
        }

        status = gcoOS_WaitSignal(gcvNULL, dynamic->signal, 0);
        if (status == gcvSTATUS_TIMEOUT)
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_INDEX,
                          "Waiting for index buffer 0x%x",
                          dynamic);

            /* Wait for the top of the queue to become available. */
            gcmONERROR(gcoOS_WaitSignal(gcvNULL,
                                        dynamic->signal,
                                        gcvINFINITE));
        }
    }

    /* Setup pointers for copying data. */
    src  = (gctUINT8_PTR) Data;
    dest = dynamic->logical + dynamic->lastEnd + offset;

    /* Dispatch on index type. */
    if (ConvertToIndexedTriangleList)
    {
        _PatchIndices(dynamic->logical + dynamic->lastEnd,
                      Data,
                      IndexType,
                      count);
    }
    else
    {
        /* Full copy.*/
        gcoOS_MemCopy(dest, src, Bytes);
    }

    /* Flush the cache. */
    gcmONERROR(gcoSURF_NODE_Cache(&dynamic->memory,
                                  dynamic->logical + dynamic->lastEnd,
                                  Bytes,gcvCACHE_CLEAN));

    /* Update the pointers. */
    dynamic->lastStart = dynamic->lastEnd + offset;
    gcmSAFECASTSIZET(aligned32, aligned);
    dynamic->lastEnd   = dynamic->lastEnd + aligned32;
    dynamic->free     -= aligned;

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_INDEX,
                   dynamic->physical,
                   dynamic->logical,
                   dynamic->lastStart,
                   Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status.*/
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_UploadDynamicEx2
**
**  Upload data into a dynamic index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that has been configured as dynamic.
**
**      gceINDEX_TYPE IndexType
**          Type of index data to upload.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer containing the data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes of data to upload.
*/
gceSTATUS
gcoINDEX_UploadDynamicEx2(
                          IN gcoINDEX Index,
                          IN gceINDEX_TYPE IndexType,
                          IN gctCONST_POINTER Data,
                          IN gctSIZE_T Bytes,
                          IN gctBOOL ConvertToIndexedTriangleList
                          )
{
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;
    gcsINDEX_DYNAMIC_PTR dynamic;
    gctUINT32 aligned32, spilitIndexMod;
    gctUINT8_PTR src, dest;
    gctSIZE_T aligned, size;
    gctSIZE_T count = 0, convertedBytes = 0;
    gctUINT32 offset = 0;
    gctUINT indexSize = 0;

    gcmHEADER_ARG("Index=0x%x IndexType=%d Data=0x%x Bytes=%lu",
        Index, IndexType, Data, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmASSERT(Index->dynamicAllocate == gcvTRUE);

    if (Index->dynamic == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Shorthand the pointer. */
    dynamic = Index->dynamicHead;
    gcmASSERT(dynamic != gcvNULL);

    /* Determine number of bytes in the index buffer. */
    switch (IndexType)
    {
    case gcvINDEX_8:
        indexSize = 1;
        break;

    case gcvINDEX_16:
        indexSize = 2;
        break;

    case gcvINDEX_32:
        indexSize = 4;
        break;

    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (ConvertToIndexedTriangleList)
    {
        /* Converting a triangle strip into triangle list. */
        count = Bytes / indexSize;
        convertedBytes = (3 * (count - 2)) * indexSize;
        Bytes = convertedBytes;
    }

    /* Compute the mod of spilit index chunck. */
    spilitIndexMod = COMPUTE_LAST_INDEX_ADDR(dynamic->physical+dynamic->lastEnd+Bytes, indexSize) % SPILIT_INDEX_CHUNCK_BYTE;
    if (!Index->hwFeature.hasIndexFetchFix && spilitIndexMod < SPILIT_INDEX_OFFSET)
    {
        offset = SPILIT_INDEX_OFFSET - spilitIndexMod;
        offset = gcmALIGN(offset, 16);
    }

    /* Compute number of aligned bytes. We need to align index buffer by 16 bytes.
     * iMX6 instanced draw hangs if otherwise.
     * For tcs index issue, align to 64 bytes.
    */
    aligned = gcmALIGN(Bytes + offset, 64);

    if (dynamic->free < aligned)
    {
        if (dynamic->bytes > 0)
        {
            /* Not enough free bytes in this buffer, mark it busy. */
            gcmONERROR(gcoOS_Signal(gcvNULL,
                dynamic->signal,
                gcvFALSE));

            /* Schedule a signal event. */
            ioctl.command            = gcvHAL_SIGNAL;
            ioctl.engine             = gcvENGINE_RENDER;
            ioctl.u.Signal.signal    = gcmPTR_TO_UINT64(dynamic->signal);
            ioctl.u.Signal.auxSignal = 0;
            ioctl.u.Signal.process   = gcmPTR_TO_UINT64(gcoOS_GetCurrentProcessID());
            ioctl.u.Signal.fromWhere = gcvKERNEL_COMMAND;
            gcmONERROR(gcoHARDWARE_CallEvent(gcvNULL, &ioctl));

            /* Commit the buffer. */
            gcmONERROR(gcoHARDWARE_Commit(gcvNULL));
        }

        /* Go to next dynamic
        */
        if (Index->dynamicAllocatedCount == 0)
        {
            dynamic = Index->dynamic;
            Index->dynamicCurrent = 0;

            size = gcmALIGN(Bytes * 2, 4 << 10);

            size = size < Index->dynamicCacheSize ? Index->dynamicCacheSize : size;

            /* Reallocate the index buffers. */
            gcmONERROR(gcoINDEX_AllocateDynamicMemory(size,dynamic));

            Index->dynamicAllocatedCount++;
        }
        else
        {
            Index->dynamicCurrent ++;
            Index->dynamicCurrent = Index->dynamicCurrent % Index->dynamicAllocatedCount;

            dynamic = Index->dynamic + Index->dynamicCurrent;

            status = gcoOS_WaitSignal(gcvNULL, dynamic->signal, 0);

            if (status == gcvSTATUS_TIMEOUT || dynamic->bytes < Bytes)
            {
                if (Index->dynamicAllocatedCount == Index->dynamicCount)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_INDEX,
                        "Waiting for index buffer 0x%x",
                        dynamic);

                    /* Wait for the top of the queue to become available. */
                    gcmONERROR(gcoOS_WaitSignal(gcvNULL,
                        dynamic->signal,
                        gcvINFINITE));
                }
                else
                {
                    /* Go to new dynamic */
                    dynamic = Index->dynamic + Index->dynamicAllocatedCount;
                    Index->dynamicCurrent = Index->dynamicAllocatedCount;
                    Index->dynamicAllocatedCount ++;
                }

                size = gcmALIGN(Bytes * 2, 4 << 10);

                size = size < Index->dynamicCacheSize ? Index->dynamicCacheSize : size;

                if (dynamic->bytes < size)
                {
                    /* Reallocate the index buffers. */
                    gcmONERROR(gcoINDEX_AllocateDynamicMemory(size,dynamic));
                }
            }
        }

        Index->dynamicHead       = dynamic;

        /* Reinitialize the top of the queue. */
        dynamic            = Index->dynamicHead;

        dynamic->free      = dynamic->bytes;
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        /* Compute the mod of spilit index chunck. */
        spilitIndexMod = COMPUTE_LAST_INDEX_ADDR(dynamic->physical+dynamic->lastEnd+Bytes, indexSize) % SPILIT_INDEX_CHUNCK_BYTE;
        if(!Index->hwFeature.hasIndexFetchFix && spilitIndexMod < SPILIT_INDEX_OFFSET)
        {
            offset= SPILIT_INDEX_OFFSET - spilitIndexMod;
            offset = gcmALIGN(offset, 16);
            aligned = gcmALIGN(Bytes + offset, 16);
        }
    }


    /* Setup pointers for copying data. */
    src  = (gctUINT8_PTR) Data;
    dest = dynamic->logical + dynamic->lastEnd + offset;

    /* Dispatch on index type. */
    if (ConvertToIndexedTriangleList)
    {
        _PatchIndices(dynamic->logical + dynamic->lastEnd,
                      Data,
                      IndexType,
                      count);
    }
    else
    {
        /* Full copy.*/
        gcoOS_MemCopy(dest, src, Bytes);
    }

    /* Flush the cache. */
    gcmONERROR(gcoSURF_NODE_Cache(&dynamic->memory,
        dynamic->logical + dynamic->lastEnd,
        Bytes,gcvCACHE_CLEAN));

    /* Update the pointers. */
    dynamic->lastStart = dynamic->lastEnd + offset;
    gcmSAFECASTSIZET(aligned32, aligned);
    dynamic->lastEnd   = dynamic->lastEnd + aligned32;
    dynamic->free     -= aligned;

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
        gcvDUMP_BUFFER_INDEX,
        dynamic->physical,
        dynamic->logical,
        dynamic->lastStart,
        Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status.*/
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_BindDynamic
**
**  Bind the index object to the hardware.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_BindDynamic(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    gceSTATUS status;
    gctUINT32 endAddress;
    gctUINT32 bufSize;

    gcmHEADER_ARG("Index=0x%x Type=%d", Index, Type);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* This only works on dynamic buffers. */
    if (Index->dynamic == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    gcmSAFECASTSIZET(bufSize, Index->dynamicHead->memory.size);
    endAddress = Index->dynamicHead->physical + bufSize - 1;

    /* Program index buffer states. */
    gcmONERROR(gcoHARDWARE_BindIndex(gcvNULL,
                                     (Index->dynamicHead->physical + Index->dynamicHead->lastStart),
                                     endAddress,
                                     Type,
                                     (Index->dynamicHead->lastEnd - Index->dynamicHead->lastStart),
                                     0xFFFFFFFF));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gcoINDEX_Construct(
    IN gcoHAL Hal,
    OUT gcoINDEX * Index
    )
{
    *Index = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Destroy(
    IN gcoINDEX Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Lock(
    IN gcoINDEX Index,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    *Address = 0;
    *Memory = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Unlock(
    IN gcoINDEX Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Load(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctUINT32 IndexCount,
    IN gctPOINTER IndexBuffer
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Bind(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_BindOffset(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Free(
    IN gcoINDEX Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Upload(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_UploadOffset(
    IN gcoINDEX Index,
    IN gctSIZE_T Offset,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gctBOOL gcoINDEX_CheckRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctINT Count,
    IN gctUINT32  Indices
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoINDEX_Merge(
    IN gcoINDEX Index1,
    IN gcoINDEX Index2
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_QueryCaps(
    OUT gctBOOL * Index8,
    OUT gctBOOL * Index16,
    OUT gctBOOL * Index32,
    OUT gctUINT * MaxIndex
    )
{
    if (Index8 != gcvNULL)
        *Index8 = gcvTRUE;
    if (Index16 != gcvNULL)
        *Index16 = gcvTRUE;
    if (Index32 != gcvNULL)
        *Index32 = gcvTRUE;
    if (MaxIndex != gcvNULL)
        *MaxIndex = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_GetIndexRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctSIZE_T Offset,
    IN gctUINT32 Count,
    OUT gctUINT32 * MinimumIndex,
    OUT gctUINT32 * MaximumIndex
    )
{
    if (MinimumIndex != gcvNULL)
        *MinimumIndex = 0;
    if (MaximumIndex != gcvNULL)
        *MaximumIndex = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_SetDynamic(
    IN gcoINDEX Index,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_UploadDynamicEx(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL ConvertToIndexedTriangleList
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_BindDynamic(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoINDEX_GetMemoryIndexRange(
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T indexCount,
    OUT gctUINT32* iMin,
    OUT gctUINT32* iMax
    )
{
    if (iMin != gcvNULL)
        *iMin = 0;
    if (iMax != gcvNULL)
        *iMax = 0;
    return gcvSTATUS_OK;
}

gceSTATUS
gcoINDEX_GetFence(
    IN gcoINDEX Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoINDEX_WaitFence(
    IN gcoINDEX Index,
    IN gceFENCE_TYPE Type
    )
{
    return gcvSTATUS_OK;
}


#endif /* gcdNULL_DRIVER < 2 */
#endif /* gcdENABLE_3D */

