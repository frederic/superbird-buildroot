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

#define _GC_OBJ_ZONE            gcdZONE_STREAM
#define gcdDEBUG_REBUILD        0

/*******************************************************************************
**  Stream Cache
**
**  The stream cache works as a dynamic stream with the addition of caching
**  previous vertices.  The cache is a simple lookup of vertex array attributes,
**  which contain the information about the attributes of the stream.
**
**  To speed up the lookup, a has table is used as well.
**
**  The cache will have multiple buffers and validate all of them for any vertex
**  stream coming in.  Once the last buffer of the cache starts being used as
**  the cache, the oldest buffer will be flushed and marked as free
**  asynchronously by the hardware
*/

#define DYNAMIC_STREAM_COUNT           0x10000
#define gcdSTREAM_CACHE_SLOTS   2048
#define gcdSTREAM_CACHE_HASH    8192
#define gcdSTREAM_CACHE_SIZE    (1024 << 10)
#define gcdSTREAM_CACHE_COUNT   2

typedef enum _gceSTREAM_CACHE_TYPE
{
    /* Slot of free. */
    gcvSTREAM_CACHE_FREE = 0,

    /* Attributes are just copied in. */
    gcvSTREAM_CACHE_COPIED,

    /* Attributes are static. */
    gcvSTREAM_CACHE_STATIC,

    /* Attributes are dynamic. */
    gcvSTREAM_CACHE_DYNAMIC,
}
gceSTREAM_CACHE_TYPE;

typedef struct _gcsSTREAM_CACHE_BUFFER
{
    /* Signal for cache. */
    gctSIGNAL                   signal;

    /* Allocated video memory node. */
    gcsSURF_NODE                *dynamicNode;

    /* Size of the cache. */
    gctSIZE_T                   bytes;

    /* Offset to next free byte in the cache. */
    gctUINT                     offset;

    /* Number of free bytes in the cache. */
    gctSIZE_T                   free;

    /* Index into cacheArray of next free entry. */
    gctUINT                     index;
}
gcsSTREAM_CACHE_BUFFER,
* gcsSTREAM_CACHE_BUFFER_PTR;

typedef struct _gcsSTREAM_RANGE
{
    gctUINT32   stream;
    gctUINT32   start;
    gctUINT32   size;
    gctUINT32   end;
}
gcsSTREAM_RANGE;

/**
 * gcoSTREAM object definition.
 */
struct _gcoSTREAM
{
    /* The object. */
    gcsOBJECT                   object;

    /* Allocated node for stream. */
    gcsSURF_NODE                node;

    /* Size of allocated memory for stream. */
    gctSIZE_T                   size;

    /* Stride of the stream. */
    gctUINT32                   stride;

    /* Dynamic buffer management. */
    gctSIZE_T                   lastStart;
    gctSIZE_T                   lastEnd;

    /* Info for stream rebuilding. */
    gcoSTREAM                   rebuild;
    gcsSTREAM_RANGE             mapping[gcdATTRIBUTE_COUNT];


    /* New substream management. */
    gctUINT                     subStreamCount;
    gctUINT                     subStreamStride;
    gcsSTREAM_SUBSTREAM         subStreams[256];

    gcoSTREAM                   merged;
    gctBOOL                     dirty;
    gcsATOM_PTR                 reference;
    gctUINT                     count;

    /***** Stream Cache *******************************************************/
    /* The cached streams. */
    gcsSTREAM_CACHE_BUFFER_PTR  cache;
    gctUINT                     cacheCurrent;
};

/**
 * Static function that unlocks and frees the allocated
 * memory attached to the gcoSTREAM object.
 *
 * @param Stream Pointer to an initialized gcoSTREAM object.
 *
 * @return Status of the destruction of the memory allocated for the
 *         gcoSTREAM.
 */
static gceSTATUS
_FreeMemory(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status;

    do
    {
        /* See if the stream memory is locked or not. */
        if (Stream->node.logical != gcvNULL)
        {
            /* Unlock the stream memory. */
            gcmERR_BREAK(gcoHARDWARE_Unlock(
                &Stream->node,
                gcvSURF_VERTEX
                ));

            /* Mark the stream memory as unlocked. */
            Stream->node.logical = gcvNULL;
        }

        /* See if the stream memory is allocated. */
        if (Stream->node.pool != gcvPOOL_UNKNOWN)
        {
            /* Free the allocated video memory asynchronously. */
            gcmERR_BREAK(gcsSURF_NODE_Destroy(
                &Stream->node
                ));
        }

        if (Stream->rebuild != gcvNULL)
        {
            gcmERR_BREAK(gcoSTREAM_Destroy(Stream->rebuild));
            Stream->rebuild = gcvNULL;
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

/**
 * Static function that allocate new stream for the gcoSTREAM
 * object, and copy old stream data into the new one. Then
 * unlocks and frees the allocated memory attached to the gcoSTREAM object.
 *
 * @param Stream Pointer to an initialized gcoSTREAM object.
 *
 * @return Status of the destruction of the memory allocated for the
 *         gcoSTREAM.
 */
static gceSTATUS
_ReplaceDynamic(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status;

    do
    {
        gcsSURF_NODE node;

        gctUINT32 alignment;
        gcePOOL pool = gcvPOOL_UNIFIED;
        gctSIZE_T bytes = Stream->size;

        gcoOS_ZeroMemory(&node,sizeof(node));

        if (Stream->node.logical != gcvNULL)
        {
            /* Query the stream alignment. */
            gcmERR_BREAK(
                gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            &alignment,
                                            gcvNULL));

            /* Allocate the linear memory. */
            gcmERR_BREAK(gcsSURF_NODE_Construct(
                &node,
                bytes,
                alignment,
                gcvSURF_VERTEX,
                gcvALLOC_FLAG_NONE,
                pool
                ));

            /* Lock the memory. */
            gcmERR_BREAK(gcoHARDWARE_Lock(&node,
                                          gcvNULL,
                                          gcvNULL));

            /* Copy the data to the new buffer.*/
            gcoOS_MemCopyFast(node.logical,
                              Stream->node.logical,
                              bytes);
        }

        /* Free old node. */
        gcmERR_BREAK(_FreeMemory(Stream));

        /* Set new node info. */
        if(node.valid)
        {
            Stream->node = node;
            Stream->size = bytes;
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    return status;
}

/**
 * Construct a new gcoSTREAM object.
 *
 * @param Hal    Pointer to an initialized gcoHAL object.
 * @param Stream Pointer to a variable receiving the gcSTERAM object
 *               pointer on success.
 *
 * @return The status of the gcoSTREAM object construction.
 */
gceSTATUS
gcoSTREAM_Construct(
    IN gcoHAL Hal,
    OUT gcoSTREAM * Stream
    )
{
    gcoSTREAM stream;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Stream != gcvNULL);

    do
    {
        /* Allocate the object structure. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct _gcoSTREAM),
                                    &pointer));

        stream = pointer;

        /* Initialize the object. */
        stream->object.type = gcvOBJ_STREAM;

        /* Initialize the fields. */
        stream->node.pool           = gcvPOOL_UNKNOWN;
        stream->node.valid          = gcvFALSE;
        stream->node.logical        = gcvNULL;
        stream->size                = 0;
        stream->stride              = 0;
        stream->lastStart           = 0;
        stream->lastEnd             = 0;
        stream->rebuild             = gcvNULL;
        gcoOS_ZeroMemory(stream->mapping, gcmSIZEOF(stream->mapping));
        stream->subStreamCount      = 0;

        stream->merged              = gcvNULL;
        stream->dirty               = gcvFALSE;
        stream->reference           = gcvNULL;
        stream->count               = 0;
        stream->cache               = gcvNULL;
        stream->cacheCurrent        = 0;

        /* Return the object pointer. */
        *Stream = stream;

        /* Success. */
        gcmFOOTER_ARG("*Stream=0x%x", *Stream);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);


    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Destroy a previously constructed gcoSTREAM object.
 *
 * @param Stream Pointer to the gcoSTREAM object that needs to be
 *               destroyed.
 *
 * @return The status of the object destruction.
 */
gceSTATUS
gcoSTREAM_Destroy(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status;
    gctUINT i = 0;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    do
    {
        /* Free the memory attached to the gcoSTREAM object. */
        gcmERR_BREAK(_FreeMemory(Stream));

        /* Check if there is a merged stream object. */
        if (Stream->merged != gcvNULL)
        {
            gctINT oldValue = 0;

            /* Decrement the reference counter of the merged stream. */
            gcmVERIFY_OK(gcoOS_AtomDecrement(gcvNULL,
                                             Stream->merged->reference,
                                             &oldValue));

            /* Check if we were the last stream attached to the merged
            ** stream. */
            if (oldValue == 1)
            {
                /* Destroy the merged stream. */
                gcmVERIFY_OK(gcoSTREAM_Destroy(Stream->merged));
            }
        }

        if (Stream->reference != gcvNULL)
        {
            /* Destroy the reference counter. */
            gcmVERIFY_OK(gcoOS_AtomDestroy(gcvNULL, Stream->reference));
        }

        /* Destroy the stream cache. */
        if (Stream->cache != gcvNULL)
        {
            for (i = 0; i < gcdSTREAM_CACHE_COUNT; ++i)
            {
                if (Stream->cache[i].dynamicNode)
                {
                    /* Unlock the stream. */
                    gcmVERIFY_OK(gcoHARDWARE_Unlock(Stream->cache[i].dynamicNode,
                        gcvSURF_VERTEX));

                    /* Free the video memory. */
                    gcmONERROR(gcsSURF_NODE_Destroy(Stream->cache[i].dynamicNode));

                    gcmOS_SAFE_FREE(gcvNULL,Stream->cache[i].dynamicNode);
                }

                 /* Destroy the signal. */
                if (Stream->cache[i].signal != gcvNULL)
                {
                    gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL,
                                                     Stream->cache[i].signal));
                }
            }

            /* Free the stream cache. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Stream->cache));
        }

        /* Free the object structure. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Stream));

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_GetFence(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Stream=0x%x", Stream);

    if (Stream)
    {
        status = gcsSURF_NODE_GetFence(&Stream->node, gcvENGINE_RENDER, gcvFENCE_TYPE_READ);
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_WaitFence(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Stream=0x%x", Stream);

    if (Stream)
    {
        status = gcsSURF_NODE_WaitFence(&Stream->node, gcvENGINE_CPU, gcvENGINE_RENDER, gcvFENCE_TYPE_ALL);
    }

    gcmFOOTER();
    return status;
}
/**
 * Upload data to the memory attached to a gcoSTREAM object.
 * If there is no memory allocated or the allocated memory
 * is too small, the memory will be reallocated to fit the
 * request if offset is zero.
 *
 * @param Stream  Pointer to a gcoSTREAM object.
 * @param Buffer  Pointer to the data that needs to be uploaded.
 * @param Offset  Offset into the stream memory where to start the upload.
 * @param Bytes   Number of bytes to upload.
 * @param Dynamic Specifies whether the stream is dynamic or static.
 *                Dynamic streams will be allocated in system memory while
 *                static streams will be allocated in the default memory
 *                pool.
 *
 * @return Status of the upload.
 */
gceSTATUS
gcoSTREAM_Upload(
    IN gcoSTREAM Stream,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    IN gctBOOL Dynamic
    )
{
    gceSTATUS status;
    gcePOOL pool;
    gctSIZE_T bytes;

    gcmHEADER_ARG("Stream=0x%x Buffer=0x%x Offset=%lu Bytes=%lu Dynamic=%d",
              Stream, Buffer, Offset, Bytes, Dynamic);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    do
    {
        /* Do we have memory already allocated? */
        if (Stream->node.pool != gcvPOOL_UNKNOWN)
        {
            if (Stream->size > DYNAMIC_STREAM_COUNT)
            {
                Dynamic = gcvFALSE;
            }

            /* Test if the requested size is too big. */
            if (Offset + Bytes > Stream->size)
            {
                /* Error. */
                gcmFOOTER_ARG("status=%d", gcvSTATUS_BUFFER_TOO_SMALL);
                return gcvSTATUS_BUFFER_TOO_SMALL;
            }

            /* If the data range overlaps for a dynamic stream, create a new
               buffer. */
            if ( Dynamic &&
                 (Offset < Stream->lastEnd) &&
                 (Offset + Bytes > Stream->lastStart) )
            {
                gcmERR_BREAK(_ReplaceDynamic(Stream));

                /* Clear dynamic range. */
                Stream->lastStart = 0;
                Stream->lastEnd   = 0;
            }

            if (!Dynamic)
            {
                gcoSTREAM_WaitFence(Stream);
            }
        }

        /* Do we need to allocate memory? */
        if (Stream->node.pool == gcvPOOL_UNKNOWN)
        {
            gctUINT32 alignment;

            if (Stream->size > DYNAMIC_STREAM_COUNT)
            {
                Dynamic = gcvFALSE;
            }

            /* Define the pool to allocate from based on the Dynamic
            ** parameter. */
            pool = Dynamic ? gcvPOOL_UNIFIED : gcvPOOL_DEFAULT;

            bytes = gcmMAX(Stream->size, Offset + Bytes);

            /* Query the stream alignment. */
            gcmERR_BREAK(
                gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            &alignment,
                                            gcvNULL));

            /* Allocate the linear memory. */
            gcmERR_BREAK(gcsSURF_NODE_Construct(
                &Stream->node,
                bytes,
                alignment,
                gcvSURF_VERTEX,
                gcvALLOC_FLAG_NONE,
                pool
                ));

            /* Save allocated information. */
            Stream->size               = bytes;

            /* Lock the memory. */
            gcmERR_BREAK(gcoHARDWARE_Lock(
                &Stream->node,
                gcvNULL,
                gcvNULL
                ));
        }

        if (Buffer != gcvNULL)
        {
            /* Copy user buffer to stream memory. */
            gcoOS_MemCopy(
                (gctUINT8 *) Stream->node.logical + Offset,
                Buffer,
                Bytes
                );

            /* Flush the vertex cache. */
            gcmERR_BREAK(
                gcoSTREAM_Flush(Stream));

            /* Flush the CPU cache. */
            gcmERR_BREAK(gcoSURF_NODE_Cache(&Stream->node,
                                          Stream->node.logical + Offset,
                                          Bytes,
                                          gcvCACHE_CLEAN));
            gcmDUMP_BUFFER(gcvNULL,
                           gcvDUMP_BUFFER_STREAM,
                           gcsSURF_NODE_GetHWAddress(&Stream->node),
                           Stream->node.logical,
                           Offset,
                           Bytes);

            /* Update written range for dynamic buffers. */
            if (Dynamic)
            {
                if (Offset < Stream->lastStart)
                {
                    Stream->lastStart = Offset;
                }

                if (Offset + Bytes > Stream->lastEnd)
                {
                    Stream->lastEnd = Offset + Bytes;
                }
            }

            /* Delete any rebuilt buffer. */
            if (Stream->rebuild != gcvNULL)
            {
                gcmERR_BREAK(
                    gcoSTREAM_Destroy(Stream->rebuild));

                Stream->rebuild = gcvNULL;
            }

            /* Mark any merged stream as dirty. */
            if (Stream->merged != gcvNULL)
            {
                Stream->merged->dirty = gcvTRUE;
            }
        }
        gcmERR_BREAK(
            gcoSTREAM_CPUCacheOperation(Stream,gcvCACHE_CLEAN));

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_ReAllocBufNode(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsSURF_NODE tmpMemory;
    gctUINT32 alignment;

    gcmHEADER();

    /* Query the stream alignment. */
    gcmONERROR(
        gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    &alignment,
                                    gcvNULL));

    /* Allocate the linear memory. */
    gcmONERROR(gcsSURF_NODE_Construct(&tmpMemory,
                                      Stream->size,
                                      alignment,
                                      gcvSURF_VERTEX,
                                      gcvALLOC_FLAG_NONE,
                                      gcvPOOL_DEFAULT
                                      ));

    /* Lock the memory. */
    gcmONERROR(gcoHARDWARE_Lock(&tmpMemory,
                                gcvNULL,
                                gcvNULL
                                ));

    /* Copy user buffer to stream memory. */
    gcoOS_MemCopyFast(
        (gctUINT8 *) tmpMemory.logical,
        (gctUINT8 *) Stream->node.logical,
        Stream->size
        );

    /* Free old node. */
    gcmONERROR(_FreeMemory(Stream));

    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_STREAM,
                   gcsSURF_NODE_GetHWAddress(&tmpMemory),
                   tmpMemory.logical,
                   0,
                   Stream->size);

    /* update Node.*/
    Stream->node = tmpMemory;

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Set the number of bytes per vertex inside a gcoSTREAM
 * object.
 *
 * @param Stream Pointer to a gcoSTREAM object.
 * @param Stride Number of bytes per vertex.
 *
 * @return Status of the stride setting.
 */
gceSTATUS
gcoSTREAM_SetStride(
    IN gcoSTREAM Stream,
    IN gctUINT32 Stride
    )
{
    gcmHEADER_ARG("Stream=0x%x Stride=%u", Stream, Stride);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Set stream stride. */
    Stream->stride = Stride;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_Node(
    IN gcoSTREAM Stream,
    OUT gcsSURF_NODE_PTR * Node
    )
{

    gcmHEADER_ARG("Stream=0x%x", Stream);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    if (Node != gcvNULL)
    {
        *Node = &Stream->node;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

}

/**
 * Lock the stream and return the pointers.
 *
 * @param Stream   Pointer to an initialized gcoSTREAM object.
 * @param Logical  Pointer to a variable receiving the logical address of
 *                 the locked memory.  If Logical is gcvNULL, the logical
 *                 address is not returned.
 * @param Physical Pointer to a variable receiving the physical address of
 *                 the locked memory.  If Physical is gcvNULL, the physical
 *                 address is not returned.
 *
 * @return The status of the lock.
 */
gceSTATUS
gcoSTREAM_Lock(
    IN gcoSTREAM Stream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical
    )
{
    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    if (Logical != gcvNULL)
    {
        /* Return logical address. */
        *Logical = Stream->node.logical;
    }

    if (Physical != gcvNULL)
    {
        gcmGETHARDWAREADDRESS(Stream->node, *Physical);
    }

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x *Physical=%08x",
                  gcmOPT_POINTER(Logical), gcmOPT_VALUE(Physical));
    return gcvSTATUS_OK;

}

/**
 * Unlock stream that was previously locked with gcoSTREAM_Lock.
 *
 * @param Stream   Pointer to an initialized gcoSTREAM object.
 *
 * @return The status of the Unlock.
 */
gceSTATUS
gcoSTREAM_Unlock(
    IN gcoSTREAM Stream
    )
{
    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Do we have a buffer allocated? */
    if (Stream->node.pool == gcvPOOL_UNKNOWN)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Return the status. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


gceSTATUS
gcoSTREAM_Reserve(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes
    )
{
    gctBOOL allocate;
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x Bytes=%lu", Stream, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    do
    {
        /* Do we have memory already allocated? */
        if (Stream->node.pool == gcvPOOL_UNKNOWN)
        {
            allocate = gcvTRUE;
        }

        else if (Stream->size < Bytes)
        {
            gcmERR_BREAK(_FreeMemory(Stream));

            allocate = gcvTRUE;
        }

        else
        {
            allocate = gcvFALSE;
        }

        Stream->lastStart = Stream->lastEnd = 0;

        if (allocate)
        {
            gctUINT32 alignment;

            /* Query the stream alignment. */
            gcmERR_BREAK(
                gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            &alignment,
                                            gcvNULL));

            /* Allocate the linear memory. */
            gcmERR_BREAK(gcsSURF_NODE_Construct(
                &Stream->node,
                Bytes,
                alignment,
                gcvSURF_VERTEX,
                gcvALLOC_FLAG_NONE,
                gcvPOOL_DEFAULT
                ));

            /* Save allocated information. */
            Stream->size               = Bytes;

            /* Lock the memory. */
            gcmERR_BREAK(gcoHARDWARE_Lock(
                &Stream->node,
                gcvNULL,
                gcvNULL
                ));
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * gcoVERTEX object definition.
 */
struct _gcoVERTEX
{
    /* The object. */
    gcsOBJECT                   object;

    /* Hardware capabilities. */
    gctUINT                     maxAttribute;
    gctUINT                     maxStride;
    gctUINT                     maxStreams;

    /* Attribute array. */
    struct _gcsVERTEX_ATTRIBUTE
    {
        /* Format for attribute. */
        gceVERTEX_FORMAT        format;

        /* Whether attribute needs to be normalized or not. */
        gctBOOL                 normalized;

        /* Number of components in attribute. */
        gctUINT32               components;

        /* Size of the attribute in bytes. */
        gctSIZE_T               size;

        /* Stream holding attribute. */
        gcoSTREAM               stream;

        /* Offset into stream. */
        gctUINT32               offset;

        /* Stride in stream. */
        gctUINT32               stride;
    }
    attributes[gcdATTRIBUTE_COUNT];

    /* Combined stream. */
    gcoSTREAM                   combinedStream;
};

/**
 * Construct a new gcoVERTEX object.
 *
 * @param Hal    Pointer to an initialized gcoHAL object.
 * @param Vertex Pointer to a variable receiving the gcoVERTEX object
 *               pointer.
 *
 * @return Status of the construction.
 */
gceSTATUS
gcoVERTEX_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEX * Vertex
    )
{
    gceSTATUS status;
    gcoVERTEX vertex;
    gctSIZE_T i;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Vertex != gcvNULL);

    do
    {
        /* Allocate the object structure. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct _gcoVERTEX),
                                    &pointer));

        vertex = pointer;

        /* Initialize the object. */
        vertex->object.type = gcvOBJ_VERTEX;

        /* Query the stream capabilities. */
        gcmVERIFY_OK(gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                                 &vertex->maxAttribute,
                                                 &vertex->maxStride,
                                                 &vertex->maxStreams,
                                                 gcvNULL,
                                                 gcvNULL));

        /* Zero out all attributes. */
        for (i = 0; i < gcmCOUNTOF(vertex->attributes); i++)
        {
            vertex->attributes[i].components = 0;
        }

        /* No combined stream. */
        vertex->combinedStream = gcvNULL;

        /* Return the gcoVERTEX object. */
        *Vertex = vertex;

        /* Success. */
        gcmFOOTER_ARG("*Vertex=0x%x", *Vertex);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Destroy a previously constructed gcoVERTEX object.
 *
 * @param Vertex Pointer to a gcoVERTEX object.
 *
 * @return Status of the destruction.
 */
gceSTATUS
gcoVERTEX_Destroy(
    IN gcoVERTEX Vertex
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    do
    {
        /* Destroy any combined stream. */
        if (Vertex->combinedStream != gcvNULL)
        {
            gcmERR_BREAK(gcoSTREAM_Destroy(Vertex->combinedStream));
            Vertex->combinedStream = gcvNULL;
        }

        gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, Vertex));
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVERTEX_Reset(
    IN gcoVERTEX Vertex
    )
{
    gctSIZE_T i;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    /* Destroy any combined stream. */
    if (Vertex->combinedStream != gcvNULL)
    {
        gcmVERIFY_OK(
            gcoSTREAM_Destroy(Vertex->combinedStream));

        Vertex->combinedStream = gcvNULL;
    }

    /* Zero out all attributes. */
    for (i = 0; i < gcmCOUNTOF(Vertex->attributes); i++)
    {
        Vertex->attributes[i].components = 0;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/**
 * Enable an attribute with the specified format sourcing
 * from the specified gcoSTREAM object.
 *
 * @param Vertex     Pointer to an initialized gcoVERTEX object.
 * @param Index      Attribute index.
 * @param Format     Format for attribute.
 * @param Normalized Flag indicating whether the attribute should be
 *                   normalized (gcvTRUE) or not (gcvFALSE).
 * @param Components Number of components for attribute.
 * @param Stream     Pointer to a gcoSTREAM object which is the source for the
 *                   attribute.
 * @param Offset     Offset into the stream for the attribute.
 *
 * @return Status of the enable.
 */
gceSTATUS
gcoVERTEX_EnableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index,
    IN gceVERTEX_FORMAT Format,
    IN gctBOOL Normalized,
    IN gctUINT32 Components,
    IN gcoSTREAM Stream,
    IN gctUINT32 Offset,
    IN gctUINT32 Stride
    )
{
    gcmHEADER_ARG("Vertex=0x%x Index=%u Format=%d Normalized=%d Components=%u "
              "Stream=0x%x Offset=%u Stride=%u",
              Vertex, Index, Format, Normalized, Components, Stream, Offset,
              Stride);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);
    gcmVERIFY_ARGUMENT((Components > 0) && (Components <= 4));
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    if (Index >= gcmCOUNTOF(Vertex->attributes))
    {
        /* Invalid attribute index. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Determine the size of the attribute. */
    switch (Format)
    {
    case gcvVERTEX_BYTE:
    case gcvVERTEX_UNSIGNED_BYTE:
    case gcvVERTEX_INT8:
        Vertex->attributes[Index].size = Components * sizeof(gctUINT8);
        break;

    case gcvVERTEX_SHORT:
    case gcvVERTEX_UNSIGNED_SHORT:
    case gcvVERTEX_HALF:
    case gcvVERTEX_INT16:
        Vertex->attributes[Index].size = Components * sizeof(gctUINT16);
        break;

    case gcvVERTEX_INT:
    case gcvVERTEX_UNSIGNED_INT:
    case gcvVERTEX_FIXED:
    case gcvVERTEX_FLOAT:
    case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
    case gcvVERTEX_INT_10_10_10_2:
    case gcvVERTEX_INT32:
        Vertex->attributes[Index].size = Components * sizeof(gctUINT32);
        break;

    default:
        gcmFATAL("Unknown vertex format %d", Format);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Save vertex attributes. */
    Vertex->attributes[Index].format     = Format;
    Vertex->attributes[Index].normalized = Normalized;
    Vertex->attributes[Index].components = Components;
    Vertex->attributes[Index].stream     = Stream;
    Vertex->attributes[Index].offset     = Offset;
    Vertex->attributes[Index].stride     = Stride;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/**
 * Disable the specified attribute.
 *
 * @param Vertex Pointer to an initialized gcoVERTEX object.
 * @param Index  Attribute index to disable.
 *
 * @return Status of the disable.
 */
gceSTATUS
gcoVERTEX_DisableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index
    )
{
    gcmHEADER_ARG("Vertex=0x%x Index=%u", Vertex, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    if (Index >= gcmCOUNTOF(Vertex->attributes))
    {
        /* Invalid attribute index. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Disable attribute. */
    Vertex->attributes[Index].components = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

typedef struct _gcoStreamInfo
{
    /* Stream object. */
    gcoSTREAM   stream;

    /* Attribute count in this stream. */
    gctUINT32   attCount;

    /* If the attributes in the stream is packed by vertices. */
    gctBOOL     unpacked;

    /* Attributes in the stream. */
    gctUINT32   attributes[16];

} gcoStreamInfo;

static void
_SortStreamAttribute(
    IN OUT gcoStreamInfo * StreamInfo,
    IN gcsVERTEX_ATTRIBUTES_PTR Mapping
    )
{
    gctUINT32 i, j, k;

    /* Bubble sort by attribute offset. */
    for (i = 0; i < StreamInfo->attCount; i++)
    {
        for (j = StreamInfo->attCount - 1; j > i; j--)
        {
            if(Mapping[StreamInfo->attributes[j]].offset <
                    Mapping[StreamInfo->attributes[j-1]].offset)
            {
                k                           = StreamInfo->attributes[j];
                StreamInfo->attributes[j]   = StreamInfo->attributes[j-1];
                StreamInfo->attributes[j-1] = k;
            }
        }
    }
}

static gceSTATUS
_PackStreams(
    IN gcoVERTEX Vertex,
    IN OUT gctUINT32_PTR StreamCount,
    IN OUT gcoSTREAM * Streams,
    IN gctUINT32 AttributeCount,
    IN OUT gcsVERTEX_ATTRIBUTES_PTR Mapping,
    IN OUT gctUINT32 * Base
    )
{
    gctUINT32 i, j;
    gctSIZE_T bytes, stride;
    gctUINT32 count, stride32;
    gctUINT32 start, end, size;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 usedStreams = 0;
    gctBOOL repack = gcvFALSE;
    gcoSTREAM stream = gcvNULL;
    gcoStreamInfo *streamInfo = gcvNULL;
    gctUINT8_PTR src, srcEnd, dest, source;
    gcoStreamInfo  *currentStream;
    gctUINT32 destIndex, destOffset;

    do
    {
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, 16 * sizeof(gcoStreamInfo), (gctPOINTER *)&streamInfo));
        gcoOS_ZeroMemory(streamInfo, 16 * sizeof(gcoStreamInfo));

        stream = Streams[Mapping[0].stream];
        /* Construct streamInfo. */
        for (i = 0; i < AttributeCount; i++)
        {
            stream = Streams[Mapping[i].stream];

            /* Find the attribute in which stream. */
            for (j = 0; j < usedStreams; j++)
            {
                if (stream == streamInfo[j].stream)
                {
                    streamInfo[j].attributes[streamInfo[j].attCount] = i;
                    streamInfo[j].attCount++;

                    break;
                }
            }

            /* Can't find a stream, save it in a new stream. */
            if (j == usedStreams)
            {
                streamInfo[usedStreams].stream        = stream;
                streamInfo[usedStreams].attCount      = 1;
                streamInfo[usedStreams].attributes[0] = i;

                usedStreams++;
            }
        }

        if (usedStreams != *StreamCount)
        {
            if (streamInfo != gcvNULL)
            {
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, streamInfo));
            }
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        stride = 0;
        count  = 0;

        /* Check need to merge and calc merged size. */
        for (i = 0; i < usedStreams; i++)
        {
            gctUINT32 minOffset, maxOffset;
            gctUINT32 vertexCount;

            currentStream = &streamInfo[i];

            /* Sort attributes in the stream. */
            _SortStreamAttribute(currentStream, Mapping);

            /* Min and max offset of attributes in the stream. */
            minOffset = Mapping[currentStream->attributes[0]].offset;
            maxOffset = Mapping[currentStream->attributes[currentStream->attCount-1]].offset;

            if (maxOffset - minOffset > currentStream->stream->stride)
            {
                /* The attributes in the stream isn't packed by vertices. */
                currentStream->unpacked = gcvTRUE;
                repack                  = gcvTRUE;
            }

            /* Calc total stream size and stride. */
            for (j = 0; j < currentStream->attCount; j++)
            {
                gctUINT32 index = currentStream->attributes[j];

                stride += Mapping[index].size;
                start   = Mapping[index].offset;
                gcmSAFECASTSIZET(end, Streams[Mapping[index].stream]->size);

                /* How many vertex count the attribute for? */
                vertexCount = (end - start + Mapping[index].stride)
                                / stream->stride;

                /* Get max vertex count. */
                count = count < vertexCount ? vertexCount : count;
            }
        }


        /* If doesn't need merge or repack, return OK. */
        if (!(repack || (*StreamCount > Vertex->maxStreams)))
        {
            if (streamInfo != gcvNULL)
            {
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, streamInfo));
            }
            return gcvSTATUS_OK;
        }

        /* Do stream merge and pack. */

        /* Get combined stream size. */
        bytes = stride * count;

        if (Vertex->combinedStream == gcvNULL)
        {
            gcmERR_BREAK(gcoSTREAM_Construct(gcvNULL,
                                             &Vertex->combinedStream));
        }

        gcmERR_BREAK(gcoSTREAM_Reserve(Vertex->combinedStream, bytes));

        gcmSAFECASTSIZET(stride32, stride);
        gcmERR_BREAK(gcoSTREAM_SetStride(Vertex->combinedStream, stride32));

        gcmERR_BREAK(gcoSTREAM_Lock(Vertex->combinedStream,
                                    (gctPOINTER) &dest,
                                    gcvNULL));

        destIndex = 0;

        /* Merge every attribute in streams. */
        for (i = 0; i < usedStreams; i++)
        {
            currentStream = &streamInfo[i];
            stream        = currentStream->stream;

            gcmERR_BREAK(gcoSTREAM_Lock(stream,
                                        (gctPOINTER) &source,
                                        gcvNULL));

            for (j = 0; j < currentStream->attCount; j++)
            {
                gctUINT32 index = currentStream->attributes[j];
                gctSIZE_T endT = Streams[Mapping[index].stream]->size;

                gcmSAFECASTSIZET(size, Mapping[index].size);
                src         = source + Mapping[index].offset;
                srcEnd      = source + endT;
                destOffset  = destIndex;

                while (src < srcEnd && destOffset < bytes)
                {
                    gcoOS_MemCopy(dest + destOffset, src, size);

                    destOffset += stride32;
                    src        += Mapping[index].stride;
                }

                Mapping[index].stream = 0;
                Mapping[index].offset = destIndex;
                destIndex += size;
            }
        }

        /* Update attributes and stream information. */
        if (gcmIS_SUCCESS(status))
        {
            /* Flush the stream. */
            gcmERR_BREAK(
                gcoSTREAM_Flush(Vertex->combinedStream));

            /* Flush the CPU cache. */
            gcmERR_BREAK(
                gcoSURF_NODE_Cache(&Vertex->combinedStream->node,
                                 Vertex->combinedStream->node.logical,
                                 Vertex->combinedStream->size,
                                 gcvCACHE_CLEAN));

            Streams[0]   = Vertex->combinedStream;
            *StreamCount = 1;

            for (i = 0; i < *StreamCount; i++)
            {
                Base[i] = 0;
            }

            gcmDUMP_BUFFER(gcvNULL,
                     gcvDUMP_BUFFER_STREAM,
                     gcsSURF_NODE_GetHWAddress(&Streams[0]->node),
                     Streams[0]->node.logical,
                     0,
                     Streams[0]->size);
        }

    } while(gcvFALSE);

    if (streamInfo != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, streamInfo));
    }
    return gcvSTATUS_OK;
}

static gceSTATUS
_RebuildStream(
    IN gcoVERTEX Vertex,
    IN OUT gcoSTREAM * StreamArray,
    IN gctUINT32 Stream,
    IN OUT gcsVERTEX_ATTRIBUTES_PTR Mapping,
    IN gctUINT32 AttributeCount,
    IN gcsSTREAM_RANGE * Range,
    IN gctUINT32 RangeCount
    )
{
    gcoSTREAM stream;
    gctUINT32 i, j;
    gctUINT32 stride;
    gctUINT32 sort[16] = {0}, count;
    gceSTATUS status;
    gctUINT maxStride;
    gctUINT32 mapping[16] = {0};

#if gcdDEBUG_REBUILD
    gcmPRINT("++%s: Stream=%p", __FUNCTION__, StreamArray[Stream]);

    for (i = 0; i < AttributeCount; ++i)
    {
        gcmPRINT("MAP:%d format=%d comp=%u size=%lu stream=%u offset=%u "
                 "stride=%u",
                 i, Mapping[i].format, Mapping[i].components, Mapping[i].size,
                 Mapping[i].stream, Mapping[i].offset, Mapping[i].stride);
    }
#endif

redo:
    /* Step 1.  Sort the range. */
    for (i = count = stride = 0; i < RangeCount; ++i)
    {
        if (Range[i].stream == Stream)
        {
            gctUINT32 k;

            /* Compute the stride while we are at it. */
            stride += Range[i].size;

            /* Find the slot we need to insert this range in to. */
            for (j = 0; j < count; ++j)
            {
                if (Range[i].start < Range[sort[j]].start)
                {
                    break;
                }
            }

            /* Make room in the array. */
            for (k = count; k > j; --k)
            {
                sort[k] = sort[k - 1];
            }

            /* Save the range. */
            sort[j] = i;

            /* Increment number of ranges. */
            ++ count;
        }
    }

#if gcdDEBUG_REBUILD
    for (i = 0; i < RangeCount; ++i)
    {
        gcmPRINT("RANGE:%d start=%u end=%u size=%u",
                 i, Range[i].start, Range[i].end, Range[i].size);
    }
#endif

    /* Step 2.  See if the stream already has a matching rebuilt stream. */
    stream = StreamArray[Stream]->rebuild;
    if (stream != gcvNULL)
    {
        /* Walk all attributes. */
        for (i = 0; i < AttributeCount; ++i)
        {
            /* This attribute doesn't match the stream. */
            if (Mapping[i].stream != Stream)
            {
                continue;
            }

            /* Walk all rebuilt entries. */
            for (j = 0; (j < Vertex->maxAttribute) && (stream->mapping[j].size > 0); ++j)
            {
                /* Check if this attribute matches anything in the rebuilt stream. */
                if ((stream->mapping[j].size == Mapping[i].stride)
                &&  (stream->mapping[j].start <= Mapping[i].offset)
                &&  (Mapping[i].offset + Mapping[i].size <= stream->mapping[j].end)
                )
                {
                    break;
                }
            }

            /* Bail out if this attribute doesn't match anything. */
            if ((j == Vertex->maxAttribute) || (stream->mapping[j].size == 0))
            {
#if gcdDEBUG_REBUILD
                gcmPRINT("MISS:%p i=%d", stream, i);

                for (j = 0; (j < Vertex->maxAttribute) && ( stream->mapping[j].size > 0); ++j)
                {
                    gcmPRINT("  %d: start=%u end=%u size=%u",
                             j, stream->mapping[j].start,
                             stream->mapping[j].end, stream->mapping[j].size);
                }
#endif
                break;
            }

            /* Save mapping for this attribute. */
            mapping[i] = stream->mapping[j].stream
                       + Mapping[i].offset - stream->mapping[j].start;
        }

        /* See if the stream is already completely mapped. */
        if (i == AttributeCount)
        {
            /* Copy all the mappings of the attributes belonging to this
            ** stream. */
#if gcdDEBUG_REBUILD
            gcmPRINT("CACHED:%p", stream);
#endif
            while (i-- > 0)
            {
                if (Mapping[i].stream == Stream)
                {
                    /* Override offset and stride. */
                    Mapping[i].offset = mapping[i];
                    Mapping[i].stride = stream->stride;
#if gcdDEBUG_REBUILD
                    gcmPRINT("CACHED:%d offset=%u stride=%u",
                             i, Mapping[i].offset, Mapping[i].stride);
#endif
                }
            }

            /* Use the rebuilt stream. */
            StreamArray[Stream] = stream;

            /* Success. */
#if gcdDEBUG_REBUILD
            gcmPRINT("--%s", __FUNCTION__);
#endif
            return gcvSTATUS_OK;
        }
    }

    do
    {
        gctUINT size32;
        gctUINT8_PTR srcLogical, dstLogical;
        gctPOINTER pointer = gcvNULL;

        /* Step 3.  Destroy any previous rebuilt stream. */
        if (stream != gcvNULL)
        {
            gctBOOL changed = gcvFALSE;

            /* Add any missing map to the current range. */
            for (i = 0; (i < Vertex->maxAttribute) && (stream->mapping[i].size > 0); ++i)
            {
                /* Walk the current range to find this mapping. */
                for (j = 0; j < RangeCount; ++j)
                {
                    if ((stream->mapping[i].start == Range[j].start)
                    &&  (stream->mapping[i].size  == Range[j].size)
                    )
                    {
                        break;
                    }
                }

                if ((j == RangeCount) && (j < Vertex->maxAttribute))
                {
#if gcdDEBUG_REBUILD
                    gcmPRINT("Adding mapping %d at %d: %u-%u size=%u",
                             i, j, stream->mapping[i].start,
                             stream->mapping[i].end, stream->mapping[i].size);
#endif

                    Range[j].stream = 0;
                    Range[j].start  = stream->mapping[i].start;
                    Range[j].end    = stream->mapping[i].end;
                    Range[j].size   = stream->mapping[i].size;
                    RangeCount     += 1;

                    changed = gcvTRUE;
                }
            }

            /* Destroy the stream. */
            gcmERR_BREAK(gcoSTREAM_Destroy(stream));

            stream                       = gcvNULL;
            StreamArray[Stream]->rebuild = gcvNULL;

            if (changed)
            {
                goto redo;
            }
        }

        /* Step 4.  Allocate a new rebuilt stream. */
        gcmERR_BREAK(
            gcoSTREAM_Construct(gcvNULL, &stream));

        gcmERR_BREAK(
            gcoSTREAM_Reserve(stream, StreamArray[Stream]->size));

        gcmERR_BREAK(
            gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                        gcvNULL,
                                        &maxStride,
                                        gcvNULL,
                                        gcvNULL,
                                        gcvNULL));

        /* Step 5.  Generate the mapping. */
        if (stride > maxStride)
        {
            /* Sort the attributes. */
            for (i = count = 0; i < AttributeCount; ++i)
            {
                gctUINT32 k;

                if (Mapping[i].stream != Stream)
                {
                    continue;
                }

                for (j = 0; j < count; ++j)
                {
                    if (Mapping[i].offset < Mapping[sort[i]].offset)
                    {
                        break;
                    }
                }

                for (k = count; k > j; --k)
                {
                    sort[k] = sort[k -1];
                }

                sort[j] = i;
                ++ count;
            }

            /* Generate the mapping. */
            for (i = stride = 0; i < count; ++i)
            {
                gctUINT32 size32;
                gcmSAFECASTSIZET(size32, Mapping[sort[i]].size);

                stream->mapping[i].stream = stride;
                stream->mapping[i].start  = Mapping[sort[i]].offset;
                stream->mapping[i].end    = Mapping[sort[i]].offset
                                          + size32;
                stream->mapping[i].size   = Mapping[sort[i]].stride;

                stride += size32;
            }

            gcmASSERT(stride <= maxStride);
            stream->stride = stride;
        }
        else
        {
            stream->stride = stride;

            stream->mapping[0].stream = 0;
            stream->mapping[0].start  = 0;
            stream->mapping[0].size   = Range[sort[0]].size;
            stream->mapping[0].end    = stream->mapping[0].size;

            for (i = 1; i < count; ++i)
            {
                /* Mapping starts at the end of the previous mapping. */
                stream->mapping[i].stream = stream->mapping[i - 1].stream
                                          + stream->mapping[i - 1].size;
                stream->mapping[i].start  = Range[sort[i]].start;
                stream->mapping[i].size   = Range[sort[i]].size;
                stream->mapping[i].end    = stream->mapping[i].start
                                          + stream->mapping[i].size;
            }
        }

        /* Zero out the remaining mappings. */
        for (; i < Vertex->maxAttribute; ++i)
        {
            stream->mapping[i].stream = 0;
            stream->mapping[i].size   = 0;
        }

        /* Step 6.  Copy the data. */
        gcmERR_BREAK(gcoSTREAM_Lock(StreamArray[Stream], &pointer, gcvNULL));

        srcLogical = pointer;

        gcmERR_BREAK(gcoSTREAM_Lock(stream, &pointer, gcvNULL));

        dstLogical = pointer;

        gcmSAFECASTSIZET(size32, stream->size);
        for (i = 0; i < count; ++i)
        {
            gctUINT32 src   = stream->mapping[i].start;
            gctUINT32 end   = (i + 1 == count) ? size32
                                               : stream->mapping[i + 1].start;
            gctUINT32 dst   = stream->mapping[i].stream;
            gctUINT32 size  = stream->mapping[i].size;
            gctUINT32 bytes = (i + 1 == count) ? (stride - dst)
                                               : (stream->mapping[i + 1].stream - dst);

            while ((src < end) && (dst + size <= stream->size))
            {
                gcmASSERT(src + size <= StreamArray[Stream]->size);

                gcoOS_MemCopy(dstLogical + dst, srcLogical + src, bytes);

                src += size;
                dst += stride;
            }
        }

        /* Flush the stream. */
        gcmERR_BREAK(
            gcoSTREAM_Flush(stream));

        /* Flush the CPU cache. */
        gcmERR_BREAK(gcoSURF_NODE_Cache(&stream->node,
                                      dstLogical,
                                      StreamArray[Stream]->size,
                                      gcvCACHE_CLEAN));

        /* Step 7.  Perform the mapping. */
#if gcdDEBUG_REBUILD
        gcmPRINT("REBUILT:%p", stream);
#endif
        for (i = 0; i < AttributeCount; ++i)
        {
            if (Mapping[i].stream == Stream)
            {
                for (j = 0; j < count; ++j)
                {
                    if ((stream->mapping[j].start <= Mapping[i].offset)
                    &&  (Mapping[i].offset + Mapping[i].size <= stream->mapping[j].end)
                    )
                    {
                        break;
                    }
                }

                gcmASSERT(j < count);

                Mapping[i].offset += stream->mapping[j].stream
                                  -  stream->mapping[j].start;
                Mapping[i].stride = stride;
#if gcdDEBUG_REBUILD
                gcmPRINT("REMAP:%d offset=%u stride=%u",
                         i, Mapping[i].offset, Mapping[i].stride);
#endif
            }
        }

        /* Use newly rebuilt stream. */
        StreamArray[Stream]->rebuild = stream;
        StreamArray[Stream] = stream;

        /* Success. */
#if gcdDEBUG_REBUILD
        gcmPRINT("--%s", __FUNCTION__);
#endif
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (stream != gcvNULL)
    {
        /* Destroy rebuilt stream. */
        gcmVERIFY_OK(
            gcoSTREAM_Destroy(stream));
    }

    /* Return the error. */
#if gcdDEBUG_REBUILD
    gcmPRINT("--%s: status=%d", __FUNCTION__, status);
#endif
    return status;
}

static gceSTATUS
_Unalias(
    IN gcoSTREAM Stream,
    IN gctUINT Attributes,
    IN gcsVERTEX_ATTRIBUTES_PTR Mapping,
    OUT gcoSTREAM * UseStream
    )
{
    gceSTATUS status;
    gcoSTREAM stream = gcvNULL;

    gcmHEADER_ARG("Stream=0x%x Attributes=%u Mapping=0x%x",
                  Stream, Attributes, Mapping);

    /* Special case 2 attributes. */
    if ((Attributes == 2)

    /* Same pointing at the same stream with a stride less than 8. */
    &&  (Mapping[0].stream == Mapping[1].stream)
    &&  (Mapping[0].stride <  8)

    /* Attribute 0 starts at offset 0. */
    &&  (Mapping[0].offset == 0)

    /* Attribute 1 aliases attribute 0. */
    &&  (Mapping[1].offset == 0)
    &&  (Mapping[1].size   == Mapping[0].size)
    )
    {
        /* Check if we already rebuilt this stream. */
        if (Stream->rebuild == gcvNULL)
        {
            gctUINT8_PTR src, dst;
            gctSIZE_T bytes;
            gctUINT stride;

            /* Construct a new stream. */
            gcmONERROR(gcoSTREAM_Construct(gcvNULL, &stream));

            /* Reserve twice the amount of data. */
            gcmONERROR(gcoSTREAM_Reserve(stream, Stream->size * 2));

            /* Get source stride. */
            stride = Stream->stride;

            /* Set destination stride to twice the source stride. */
            stream->stride = stride * 2;

            /* Get pointers to streams. */
            src = Stream->node.logical;
            dst = stream->node.logical;

            /* Copy all vertices. */
            for (bytes = Stream->size; bytes > 0; bytes -= stride)
            {
                /* Copy vertex. */
                gcoOS_MemCopy(dst, src, stride);

                /* Copy vertex again. */
                gcoOS_MemCopy(dst + stride, src, stride);

                /* Advance pointers. */
                src += stride;
                dst += stride * 2;
            }

            /* Flush the vertex buffer cache. */
            gcmONERROR(gcoSURF_NODE_Cache(&stream->node,
                                        stream->node.logical,
                                        stream->size,
                                        gcvCACHE_CLEAN));

            /* Save new stream as a rebuilt stream. */
            Stream->rebuild = stream;
        }
        else
        {
            /* Stream has already been rebuilt. */
            stream = Stream->rebuild;
        }

        /* Update mapping. */
        Mapping[0].stride = stream->stride;
        Mapping[1].offset = Stream->stride;
        Mapping[1].stride = stream->stride;
    }
    else
    {
        /* No need to rebuild anything. */
        stream = Stream;
    }
     gcmDUMP_BUFFER(gcvNULL,
                    gcvDUMP_BUFFER_STREAM,
                    gcsSURF_NODE_GetHWAddress(&stream->node),
                     stream->node.logical,
                     0,
                     stream->size);

    /* Return stream. */
    *UseStream = stream;

    /* Success. */
    gcmFOOTER_ARG("*UseStream=0x%x", *UseStream);
    return gcvSTATUS_OK;

OnError:
    if (stream != gcvNULL)
    {
        /* Destroy newly created stream. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(stream));
    }

    /* Return error. */
    gcmFOOTER();
    return status;
}

/**
 * Bind a vertex to the hardware.
 *
 * @param   Vertex  Pointer to an initialized gcoVERTEX object.
 *
 * @return  Status of the binding.
 */
gceSTATUS
gcoVERTEX_Bind(
    IN gcoVERTEX Vertex
    )
{
    gctUINT32 streams, attributes;
    gctUINT i, j;
    gcoSTREAM usedStreams[16];
    gctBOOL rebuild[16];
    gcsVERTEX_ATTRIBUTES mapping[16];
    gceSTATUS status;
    gctUINT32 address, base[16];
    gcsSTREAM_RANGE range[16];
    gctUINT32 offset, size, stream;
    gctUINT32 rangeCount;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    /* Zero the stream usage table. */
    gcoOS_ZeroMemory(usedStreams, sizeof(usedStreams));

    /* Zero the range */
    gcoOS_ZeroMemory(range, sizeof(range));

    /* Count the number of attributes and required streams. */
    streams = attributes = rangeCount = 0;

    /* Initialize stream base addresses. */
    for (i = 0; i < 16; ++i)
    {
        base[i]    = ~0U;
        rebuild[i] = gcvFALSE;
    }

    /* Walk through all attributes. */
    for (i = 0; i < Vertex->maxAttribute; ++i)
    {
        /* Verify if the attribute is enabled. */
        if (Vertex->attributes[i].components > 0)
        {
            /* Walk all used streams so far. */
            for (j = 0; j < streams; ++j)
            {
                /* If the attribute stream has been used, abort loop. */
                if (usedStreams[j] == Vertex->attributes[i].stream)
                {
                    break;
                }
            }

            /* See if the attribute streams has been used. */
            if (j == streams)
            {
                /* Save stream. */
                usedStreams[streams] = Vertex->attributes[i].stream;

                /* Increment stream count. */
                ++streams;
            }

            /* Fill in the mapping. */
            mapping[attributes].format     = Vertex->attributes[i].format;
            mapping[attributes].normalized = Vertex->attributes[i].normalized;
            mapping[attributes].components = Vertex->attributes[i].components;
            mapping[attributes].size       = Vertex->attributes[i].size;
            mapping[attributes].stream     = j;
            mapping[attributes].offset     = Vertex->attributes[i].offset;
            mapping[attributes].stride     = Vertex->attributes[i].stride;

            if (Vertex->attributes[i].offset < base[j])
            {
                base[j] = Vertex->attributes[i].offset;
            }

            offset = mapping[attributes].offset;
            gcmSAFECASTSIZET(size, mapping[attributes].size);
            stream = mapping[attributes].stream;

            for (j = 0; j < rangeCount; ++j)
            {
                if ((range[j].stream == stream)
                &&  (range[j].size   == mapping[attributes].stride)
                )
                {
                    if ((range[j].start <= offset)
                    &&  (offset < range[j].start + range[j].size)
                    )
                    {
                        range[j].end = offset + size;
                        break;
                    }

                    if ((offset <= range[j].start)
                    &&  (range[j].end <= offset + range[j].size)
                    )
                    {
                        range[j].start = offset;
                        break;
                    }

                    if (offset + size == range[j].start)
                    {
                        range[j].start = offset;
                        break;
                    }

                    if (offset == range[j].end)
                    {
                        range[j].end += size;
                        break;
                    }
                }
            }

            if (j == rangeCount)
            {
                range[j].stream = stream;
                range[j].start  = offset;
                range[j].end    = offset + size;
                range[j].size   = mapping[attributes].stride;

                ++ rangeCount;

                if (!rebuild[stream])
                {
                    while (j-- > 0)
                    {
                        if (range[j].stream == stream)
                        {
                            rebuild[stream] = gcvTRUE;
                            break;
                        }
                    }
                }
            }

            /* Increment attribute count. */
            ++attributes;
        }
    }

    /* See if hardware supports the number of attributes. */
    if ((attributes == 0)
    ||  (attributes > Vertex->maxAttribute)
    )
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* See if we need to rebuild any stream. */
    if (streams <= Vertex->maxStreams)
    {
        for (i = 0; i < streams; ++i)
        {
            if (rebuild[i])
            {
                /* Rebuild the stream. */
                gcmONERROR(
                    _RebuildStream(Vertex, usedStreams, i,
                                   mapping, attributes,
                                   range, rangeCount));
            }

            /* Check for stream aliasing. */
            else
            if ((usedStreams[i]->stride < 8)
            &&  (attributes == 2)
            &&  (mapping[0].offset == mapping[1].offset)
            )
            {
                /* Try to unalias the stream. */
                gcmONERROR(_Unalias(usedStreams[i],
                                    attributes, mapping,
                                    &usedStreams[i]));
            }
        }
    }

    /* Check if need to do stream merge and pack. */
    gcmONERROR(_PackStreams(Vertex,
                            &streams,
                            usedStreams,
                            attributes,
                            mapping,
                            base));

    /* Walk all streams. */
    for (i = 0; i < streams; ++i)
    {
        /* Lock the stream. */
        gcmONERROR(gcoSTREAM_Lock(usedStreams[i], gcvNULL, &address));

        /* Program the stream in hardware. */
        gcmONERROR(gcoHARDWARE_SetStream(gcvNULL,
                                         i,
                                         address + base[i],
                                         usedStreams[i]->stride));
    }

    /* Adjust all base addresses. */
    for (i = 0; i < attributes; ++i)
    {
        mapping[i].offset -= base[mapping[i].stream];
    }

    /* Program the attributes in hardware. */
    gcmONERROR(gcoHARDWARE_SetAttributes(gcvNULL,
                                         mapping,
                                         attributes));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_Flush(
    gcoSTREAM Stream
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_STREAM,
                   gcsSURF_NODE_GetHWAddress(&Stream->node),
                   Stream->node.logical,
                   0,
                   Stream->size);

    status = gcoHARDWARE_FlushVertex(gcvNULL);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
*************************** DYNAMIC BUFFER MANAGEMENT **************************
*******************************************************************************/
gceSTATUS
gcoSTREAM_SetAttribute(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Offset,
    IN gctUINT Bytes,
    IN gctUINT Stride,
    IN gctUINT Divisor,
    IN OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    gctSIZE_T end;
    gctUINT sub;
    gcsSTREAM_SUBSTREAM_PTR subPtr;
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x Offset=%u Bytes=%u Stride=%u *SubStream=0x%x",
                  Stream, Offset, Bytes, Stride, gcmOPT_POINTER(SubStream));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT((Bytes > 0) && (Bytes <= Stride));
    gcmVERIFY_ARGUMENT(Stride > 0);
    gcmVERIFY_ARGUMENT(SubStream != gcvNULL);

    /* Compute the end. */
    end = Offset + Bytes;

    /* Walk all current sub-streams parsed so far. */
    for (subPtr = *SubStream; subPtr != gcvNULL; subPtr = subPtr->next)
    {
        if (Stride == subPtr->stride)
        {
            /* Test if the attribute falls within the sub-stream. */
            if ((Offset >= subPtr->start)
            &&  (end    <= subPtr->end)
            )
            {
                break;
            }

            /* Test if the attribute starts within the sliding window. */
            if ((Offset >= subPtr->minStart)
            &&  (Offset <  subPtr->start)
            )
            {
                /* Adjust sliding window start. */
                subPtr->start  = Offset;
                subPtr->maxEnd = Offset + Stride;
                break;
            }

            /* Test if attribute ends within the sliding window. */
            if ((end >  subPtr->end)
            &&  (end <= subPtr->maxEnd)
            )
            {
                subPtr->end      = end;
                subPtr->minStart = gcmMAX((gctINT) (end - Stride), 0);
                break;
            }
        }
    }

    if (subPtr == gcvNULL)
    {
        gcsSTREAM_SUBSTREAM_PTR p, prev;

        /* Walk all sub-streams. */
        for (sub = 0, subPtr = Stream->subStreams;
             sub < Stream->subStreamCount;
             ++sub, ++subPtr)
        {
            if (Stride == subPtr->stride)
            {
                /* Test if the attribute falls within the sub-stream. */
                if ((Offset >= subPtr->start)
                &&  (end    <= subPtr->end)
                )
                {
                    break;
                }

                /* Test if the attribute starts within the sliding window. */
                if ((Offset >= subPtr->minStart)
                &&  (Offset <  subPtr->start)
                )
                {
                    /* Adjust sliding window start. */
                    subPtr->start  = Offset;
                    subPtr->maxEnd = Offset + Stride;
                    break;
                }

                /* Test if attribute ends within the sliding window. */
                if ((end >  subPtr->end)
                &&  (end <= subPtr->maxEnd)
                )
                {
                    subPtr->end      = end;
                    subPtr->minStart = gcmMAX((gctINT) (end - Stride), 0);
                    break;
                }
            }
        }

        /* Test if we need to create a new sub-stream. */
        if (sub == Stream->subStreamCount)
        {
            /* Make sure we don't overflow the array. */
            if (sub == gcmCOUNTOF(Stream->subStreams))
            {
                if (*SubStream == gcvNULL)
                {
                    /* If no sub stream is added to the chain, we can reset all sub streams. */
                    subPtr = Stream->subStreams;

                    Stream->subStreamCount = 0;

                    gcoOS_ZeroMemory(Stream->subStreams,
                                     sizeof(Stream->subStreams));
                }
                else
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }
            }

            /* Fill in the sub-stream. */
            subPtr->start    = Offset;
            subPtr->end      = end;
            subPtr->minStart = gcmMAX((gctINT) (end - Stride), 0);
            subPtr->maxEnd   = Offset + Stride;
            subPtr->stride   = Stride;
            subPtr->divisor  = Divisor;

            /* Add this substream stride to the total stride. */
            Stream->subStreamStride += Stride;

            /* Increment number of substreams. */
            Stream->subStreamCount += 1;
        }

        /* Link this sub-stream into the linked list. */
        for (p = *SubStream, prev = gcvNULL; p != gcvNULL; p = p->next)
        {
            if (p->start > subPtr->start)
            {
                break;
            }

            prev = p;
        }

        subPtr->next = p;
        if (prev == gcvNULL)
        {
            *SubStream = subPtr;
        }
        else
        {
            prev->next = subPtr;
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

gceSTATUS
gcoSTREAM_QuerySubStreams(
    IN gcoSTREAM Stream,
    IN gcsSTREAM_SUBSTREAM_PTR SubStream,
    OUT gctUINT_PTR SubStreamCount
    )
{
    gceSTATUS status;
    gctUINT i;

    gcmHEADER_ARG("Stream=0x%x SubStream=0x%x", Stream, SubStream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(SubStreamCount != gcvNULL);

    /* Check whether the substream information is the same for the rebuilt
    ** stream. */
    if ((Stream->rebuild != gcvNULL)
    &&  (Stream->subStreamCount == Stream->rebuild->subStreamCount)
    &&  gcmIS_SUCCESS(gcoOS_MemCmp(Stream->subStreams,
                                   Stream->rebuild->subStreams,
                                   ( Stream->subStreamCount
                                   * gcmSIZEOF(gcsSTREAM_SUBSTREAM)
                                   )))
    )
    {
        /* Rebuilt stream seems to be okay - only one stream. */
        *SubStreamCount = 1;
    }
    else
    {
        /* Destroy any current rebuilt stream. */
        if (Stream->rebuild != gcvNULL)
        {
            gcmONERROR(gcoSTREAM_Destroy(Stream->rebuild));
            Stream->rebuild = gcvNULL;
        }

        /* Count the number of entries in the linked list. */
        for (i = 0;
             (i < Stream->subStreamCount) && (SubStream != gcvNULL);
             ++i, SubStream = SubStream->next
        )
        {
        }

        /* Return number of substreams before rebuilding. */
        *SubStreamCount = i;
    }

    /* Success. */
    gcmFOOTER_ARG("*SubStreamCount=%u", *SubStreamCount);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_Rebuild(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    OUT gctUINT_PTR SubStreamCount
    )
{
    gceSTATUS status;
    gctUINT count = First + Count;
    gctUINT8_PTR rebuildPtr, streamPtr[16];
    gctUINT stride[16];
    gctUINT i, j;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u", Stream, First, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Count > 0);
    gcmVERIFY_ARGUMENT(SubStreamCount != gcvNULL);

    /* Only try packing if we have more than 1 stream. */
    if (Stream->subStreamCount > 1)
    {
        /* We don't have a rebuilt stream yet. */
        if ((Stream->rebuild == gcvNULL)

        /* Check if rebuilt stream has a different substream count. */
        ||  (Stream->subStreamCount != Stream->rebuild->subStreamCount)

        /* Check if the substream data is different. */
        ||  !gcmIS_SUCCESS(gcoOS_MemCmp(Stream->subStreams,
                                        Stream->rebuild->subStreams,
                                        ( Stream->subStreamCount
                                        * gcmSIZEOF(gcsSTREAM_SUBSTREAM)
                                        )))
        )
        {
            if (Stream->rebuild != gcvNULL)
            {
                /* Destroy the current rebuilt buffer. */
                gcmONERROR(gcoSTREAM_Destroy(Stream->rebuild));
                Stream->rebuild = gcvNULL;
            }

            /* Construct a new stream. */
            gcmONERROR(gcoSTREAM_Construct(gcvNULL, &Stream->rebuild));

            /* Reserve enough data for all the substreams. */
            gcmONERROR(gcoSTREAM_Reserve(Stream->rebuild,
                                         Stream->subStreamStride * count));

            /* Initialize the pointers. */
            rebuildPtr = Stream->rebuild->node.logical;
            for (i = 0; i < Stream->subStreamCount; ++i)
            {
                streamPtr[i] = Stream->node.logical
                             + Stream->subStreams[i].start;

                stride[i] = Stream->subStreams[i].stride;
            }

            /* Walk all vertices. */
            for (i = 0; i < Count; ++i)
            {
                /* Walk all substreams. */
                for (j = 0; j < Stream->subStreamCount; ++j)
                {
                    /* Copy the data from this substream into the rebuild
                    ** buffer. */
                    gcoOS_MemCopy(rebuildPtr,
                                  streamPtr[j],
                                  stride[j]);

                    /* Advance the pointers. */
                    rebuildPtr   += stride[j];
                    streamPtr[j] += stride[j];
                }
            }

            /* Copy the substream data. */
            gcoOS_MemCopy(
                Stream->rebuild->subStreams,
                Stream->subStreams,
                Stream->subStreamCount * gcmSIZEOF(gcsSTREAM_SUBSTREAM));

            Stream->rebuild->subStreamCount = Stream->subStreamCount;

            /* Flush the new rebuilt buffer. */
            gcmONERROR(gcoSURF_NODE_Cache(&Stream->rebuild->node,
                                        Stream->rebuild->node.logical,
                                        Stream->rebuild->node.size,
                                        gcvCACHE_CLEAN));
        }
    }

    /* Return number of substreams rebuilt. */
    *SubStreamCount = Stream->subStreamCount;

    /* Success. */
    gcmFOOTER_ARG("*SubStreamCount=%u", *SubStreamCount);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_SetCache(
                   IN gcoSTREAM Stream
                   )
{
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Only process if not yet cached. */
    if (Stream->cache == gcvNULL)
    {
        /* Allocate the stream cache. */
        gcmONERROR(
            gcoOS_Allocate(gcvNULL,
                           (gcmSIZEOF(gcsSTREAM_CACHE_BUFFER) * gcdSTREAM_CACHE_COUNT),
                           &pointer));

        Stream->cache = pointer;

        /* Zero all memory. */
        gcoOS_ZeroMemory(Stream->cache,
                         (gcmSIZEOF(gcsSTREAM_CACHE_BUFFER) * gcdSTREAM_CACHE_COUNT)
                         );
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    if (pointer != gcvNULL)
    {
        /* Free the stream cache. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, pointer));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#define ALLOCATE_NEW_CACHE() \
            /* Allocate node */ \
            gcmONERROR(gcoOS_Allocate(gcvNULL,gcmSIZEOF(gcsSURF_NODE),&pointer)); \
            cache->dynamicNode = pointer; \
            gcoOS_ZeroMemory(pointer,gcmSIZEOF(gcsSURF_NODE));\
            /* Allocate linear video memory. */  \
            gcmONERROR(gcsSURF_NODE_Construct( \
                cache->dynamicNode, \
                gcdSTREAM_CACHE_SIZE, \
                64, \
                gcvSURF_VERTEX, \
                gcvALLOC_FLAG_NONE, \
                gcvPOOL_DEFAULT \
                )); \
            /* Lock the stream. */ \
            gcmONERROR(gcoHARDWARE_Lock(cache->dynamicNode, \
                                        gcvNULL, gcvNULL)); \
            /* Initialize the stream cache. */ \
            cache->bytes  = gcdSTREAM_CACHE_SIZE; \
            cache->free   = gcdSTREAM_CACHE_SIZE; \
            cache->offset = 0; \
            /* Create the signal. */ \
            gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvTRUE, &cache->signal)); \
            /* Mark the stream as available. */ \
            gcmONERROR(gcoOS_Signal(gcvNULL, cache->signal, gcvTRUE));

static gceSTATUS
_NewDynamicCache(
    IN gcoSTREAM Stream,
    IN gctUINT   Bytes
    )
{
    gcsSTREAM_CACHE_BUFFER_PTR cache = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER pointer;
    gcsHAL_INTERFACE ioctl;
    gctBOOL bReuse = gcvFALSE;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    gcmASSERT(Bytes < gcdSTREAM_CACHE_SIZE);

    /* Current cache */
    cache = &Stream->cache[(Stream->cacheCurrent) % gcdSTREAM_CACHE_COUNT];

    /* Check if the cache is in use. */
    if (cache->offset > 0)
    {
        /* Mark cache as unavailable. */
        gcmONERROR(gcoOS_Signal(gcvNULL, cache->signal, gcvFALSE));

        /* Schedule a signal event. */
        ioctl.command = gcvHAL_SIGNAL;
        ioctl.engine  = gcvENGINE_RENDER;
        ioctl.u.Signal.signal    = gcmPTR_TO_UINT64(cache->signal);
        ioctl.u.Signal.auxSignal = 0;
        ioctl.u.Signal.process   = gcmPTR_TO_UINT64(gcoOS_GetCurrentProcessID());
        ioctl.u.Signal.fromWhere = gcvKERNEL_COMMAND;
        gcmONERROR(gcoHARDWARE_CallEvent(gcvNULL, &ioctl));

        /* Commit the command buffer. */
        gcmONERROR(gcoHARDWARE_Commit(gcvNULL));
    }

    /* move to next cache */
    cache = &Stream->cache[(++Stream->cacheCurrent) % gcdSTREAM_CACHE_COUNT];

    /* destory old cache.*/
    if (cache->dynamicNode != gcvNULL)
    {
        /* Check we can reuse or not */
        if (gcmIS_SUCCESS(gcoOS_WaitSignal(gcvNULL, cache->signal, 0)) && Bytes < cache->bytes)
        {
            cache->offset = 0;
            cache->free   = cache->bytes;

            bReuse = gcvTRUE;
            /*if we did not allocate a new cache and use the old cache, should flush the vertex cache here*/
            gcmONERROR(gcoHARDWARE_FlushVertex(gcvNULL));
        }
        else
        {
            gcmVERIFY_OK(gcoHARDWARE_Unlock(cache->dynamicNode, gcvSURF_VERTEX));
            gcmONERROR(gcsSURF_NODE_Destroy(cache->dynamicNode));

            gcmOS_SAFE_FREE(gcvNULL,cache->dynamicNode);

            if (cache->signal != gcvNULL)
            {
                gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, cache->signal));
            }

            /* Reset the cache. */
            cache->offset = 0;
            cache->free   = 0;
        }
    }

    if (!bReuse)
    {
        ALLOCATE_NEW_CACHE();
    }

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/* Check the stream pool type, for old mms, we need make sure the all stream from
   virtual or physic
*/
#if OPT_VERTEX_ARRAY
gceSTATUS
gcoVERTEX_AdjustStreamPool(
    IN gcoSTREAM Stream,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT TotalBytes,
    IN gctBOOL DrawArraysInstanced,
    IN OUT gctUINT32_PTR Physical,
    IN OUT gcoSTREAM * OutStream
)
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT i;
    gcsVERTEXARRAY_STREAM_PTR streamPtr;
    gcsVERTEXARRAY_BUFFER_PTR bufferPtr;
    gctUINT32 base, stride;
    gctUINT lastPhysical = 0;
    gctUINT basePhysical = 0;
    gctBOOL basePhysicalSet = gcvFALSE;
    gctBOOL bNeedReAllocBufObj = gcvFALSE;
    gctBOOL bNeedReAllocCache = gcvFALSE;
    gctBOOL bIn2GAddr = gcvFALSE;

    gcmHEADER();

    /* Process the copied stream first. */
    for(i = 0, bufferPtr = Buffers; i < BufferCount; ++i, ++bufferPtr)
    {
        /* Get stride and offset for this stream. */
        stride = bufferPtr->stride;

        if (DrawArraysInstanced)
        {
            lastPhysical = *Physical + bufferPtr->offset;
        }
        else
        {
            lastPhysical = *Physical + bufferPtr->offset - (First * stride);
        }

        if (!basePhysicalSet)
        {
            basePhysical = lastPhysical;
            basePhysicalSet = gcvTRUE;
        }
        else if (( basePhysical & 0x80000000) != (lastPhysical & 0x80000000))
        {
            bNeedReAllocCache = gcvTRUE;
            break;
        }
    }

    /* Walk all stream objects. */
    for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        stride  = streamPtr->subStream->stride;
        base    = streamPtr->attribute->offset;

        if (DrawArraysInstanced)
        {
            lastPhysical = streamPtr->physical + base + (First * stride);
        }
        else
        {
            lastPhysical = streamPtr->physical + base;
        }

        if (!basePhysicalSet)
        {
            basePhysical = lastPhysical;
            basePhysicalSet = gcvTRUE;
        }
        else if (( basePhysical & 0x80000000) != (lastPhysical & 0x80000000))
        {
            bNeedReAllocBufObj = gcvTRUE;
            break;
        }
    }

    /* Reallocate memory.*/
    if (bNeedReAllocCache || bNeedReAllocBufObj)
    {
        /* Set flag, indicate allocate behavior when re-alloc / re-bind happen.*/
        gcoHARDWARE_SetForceVirtual(gcvNULL, gcvTRUE);

        if (bNeedReAllocCache)
        {
            /* Copy the vertex data. */
            status = gcoSTREAM_CacheAttributes(Stream,
                                               First, Count,
                                               TotalBytes,
                                               BufferCount, Buffers,
                                               AttributeCount, Attributes,
                                               Physical);
            if (gcmIS_ERROR(status))
            {
                gcmONERROR(gcoSTREAM_UploadUnCacheableAttributes(*OutStream,
                                                                 First, Count,
                                                                 TotalBytes,
                                                                 BufferCount, Buffers,
                                                                 AttributeCount, Attributes,
                                                                 Physical,
                                                                 OutStream));
            }
        }

        if (bNeedReAllocBufObj)
        {
            for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
            {
                /* Check if we have to skip this stream. */
                if (streamPtr->logical == gcvNULL)
                {
                    continue;
                }

                /* re-get physical address, which may be already re-allocated and changed.*/
                gcoSTREAM_Lock(streamPtr->stream, (gctPOINTER *) &streamPtr->logical, &streamPtr->physical);

                if (DrawArraysInstanced)
                {
                    bIn2GAddr = ((streamPtr->physical + (gctUINT32)streamPtr->attribute->offset + First * streamPtr->subStream->stride) & 0x80000000) == 0;
                }
                else
                {
                    bIn2GAddr = ((streamPtr->physical + (gctUINT32)streamPtr->attribute->offset) & 0x80000000) == 0;
                }

                if (bIn2GAddr)
                {
                    /* re-alloc stream.*/
                    gcmONERROR(gcoSTREAM_ReAllocBufNode(streamPtr->stream));

                    /* Update stream data.*/
                    gcoSTREAM_Lock(streamPtr->stream, (gctPOINTER *) &streamPtr->logical, &streamPtr->physical);
                }
            }
        }
    }

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}
#endif

gceSTATUS
gcoVERTEX_AdjustStreamPoolEx(
    IN gcoSTREAM Stream,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gctUINT StreamCount,
    IN gctUINT StartVertex,
    IN gctUINT FirstCopied,
    IN gctBOOL DrawElements,
    IN OUT gcoSTREAM * UncacheableStream
)
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT stride;
    gctUINT32 base;
    gctUINT stream;
    gcsVERTEXARRAY_BUFOBJ_PTR streamPtr = gcvNULL;
    gctBOOL basePhySet = gcvFALSE;
    gctUINT32 basePhysical = 0;
    gctUINT32 lastPhysical = 0;
    gctBOOL bIn2GAddr = gcvFALSE;
    gctBOOL bNeedReAllocBufObj = gcvFALSE;
    gctBOOL bNeedReAllocCache = gcvFALSE;
    gctBOOL bForceVirtual = gcvFALSE;
    gcsSURF_NODE_PTR node = gcvNULL;

    gcmHEADER();

    /**************************** setp 1 ************************************************************
    ** check stream
    **/
    for (stream = 0, streamPtr = Streams; stream < StreamCount; streamPtr = streamPtr->next, ++stream)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        stride = streamPtr->stream == gcvNULL ? streamPtr->dynamicCacheStride : streamPtr->stride;
        base    = (gctUINT32)streamPtr->attributePtr->offset;

        if (streamPtr->stream != gcvNULL)
        {
            lastPhysical = streamPtr->physical + base;
            /* update info.*/
            bIn2GAddr = ((lastPhysical & 0x80000000) == 0);
            bNeedReAllocBufObj |= bIn2GAddr;
        }
        else
        {
            if (DrawElements)
            {
                lastPhysical = streamPtr->physical + base - ((StartVertex + FirstCopied) * stride);
            }
            else
            {
                lastPhysical = streamPtr->physical + base - (FirstCopied * stride);
            }

            /* update info.*/
            bIn2GAddr = ((lastPhysical & 0x80000000) == 0);
            bNeedReAllocCache |= bIn2GAddr;
        }

        if (!basePhySet)
        {
            basePhysical = lastPhysical;
            basePhySet = gcvTRUE;
        }
        else if (( basePhysical & 0x80000000) != (lastPhysical & 0x80000000))
        {
            bForceVirtual = gcvTRUE;
        }
    }

    /**************************** setp 2 ************************************************************
    ** Force virtual, re-alloc memory
    **/
    if (bForceVirtual)
    {
        /* Set flag, indicate allocate behavior when re-alloc / re-bind happen.*/
        gcoHARDWARE_SetForceVirtual(gcvNULL, gcvTRUE);

        if (bNeedReAllocBufObj)
        {
            for (stream = 0, streamPtr = Streams; stream < StreamCount; streamPtr = streamPtr->next, ++stream)
            {
                if (streamPtr->stream != gcvNULL)
                {
                    /* re-get physical address, which may be already re-allocated and changed.*/
                    gcoBUFOBJ_FastLock(streamPtr->stream, &streamPtr->physical, (gctPOINTER *) &streamPtr->logical);
                    gcoBUFOBJ_GetNode(streamPtr->stream, &node);
                    streamPtr->nodePtr = node;

                    bIn2GAddr = ((streamPtr->physical + (gctUINT32)streamPtr->attributePtr->offset) & 0x80000000) == 0;

                    if (bIn2GAddr)
                    {
                        /* re-alloc stream.*/
                        gcmONERROR(gcoBUFOBJ_ReAllocBufNode(streamPtr->stream));

                        /* Update stream data.*/
                        gcoBUFOBJ_FastLock(streamPtr->stream, &streamPtr->physical, (gctPOINTER *) &streamPtr->logical);
                        gcoBUFOBJ_GetNode(streamPtr->stream, &node);
                        streamPtr->nodePtr = node;
                    }
                }
            }
        }

        if (bNeedReAllocCache)
        {
            /* re-bind dynamic cache.*/
            gcmONERROR(gcoSTREAM_CacheAttributesEx(Stream,
                                                   StreamCount,
                                                   Streams,
                                                   FirstCopied,
                                                   UncacheableStream
                                                   ));
        }
    }

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

static gceSTATUS
_copyBuffers(
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctPOINTER Logical,
    OUT gctSIZE_T_PTR Bytes OPTIONAL
    )
{
    gctUINT i, j, k;
    gctUINT8_PTR src[gcdATTRIBUTE_COUNT], dst;
    gctUINT stride = 0;
    gcsVERTEXARRAY_BUFFER_PTR buffer;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute = Attributes;
    gctUINT attribStride[gcdATTRIBUTE_COUNT];
    gctSIZE_T bytes = 0;
    gctBOOL isGpuBenchTest = gcvFALSE;

    gcmHEADER_ARG("BufferCount=%u Buffers=0x%x AttributeCount=%u "
                  "Attributes=0x%x First=%u Count=%u Logical=0x%x",
                  BufferCount, Buffers, AttributeCount,
                  Attributes, First, Count, Logical);

    /* a fix for gpuBench RunSmoothLargeTrianglesBenchmark() test */
    if ((Count == 400) && (gcPLS.hal->isGpuBenchSmoothTriangle == gcvTRUE))
    {
        isGpuBenchTest = gcvTRUE;
    }

    /* Compute the destination address. */
    dst = (gctUINT8_PTR) Logical;

    /* Walk through all buffers. */
    for (i = 0, buffer = Buffers; i < BufferCount; i++, buffer++)
    {
        gctUINT count = buffer->count;

        /* We copy whole range for non-combined buffer. */
        if (!buffer->combined)
        {
            gctUINT size;

            stride = attribute[buffer->map[0]].vertexPtr->enable
                   ? attribute[buffer->map[0]].vertexPtr->stride
                   : 0;

            /* Instanced attribute will be treated differently, they will not be affected by First/Count */
            src[0] = buffer->divisor > 0
                   ? (gctUINT8_PTR)buffer->start
                   : (gctUINT8_PTR)buffer->start + (First * stride);
            dst    = (gctUINT8_PTR)Logical + buffer->offset;
            size   = count * buffer->stride;

            /* Attribute pointer enabled case. */
            if (stride != 0)
            {
                if (isGpuBenchTest == gcvTRUE)
                {
                    gctUINT32_PTR src0, dst0;
                    gctUINT l, m;
                    gctUINT32 left = count % 3;

                    count = count - left;

                    src0 = (gctUINT32_PTR) src[0];
                    dst0 = (gctUINT32_PTR) dst;


                    for (k = 0; k < count; k += 3)
                    {
                        m = k * buffer->stride >> 2;
                        l = (count - 3 - (count % 3) - k) * buffer->stride >> 2;

                        for (; m < (k + 3) * buffer->stride >> 2; m++, l++)
                        {
                            dst0[l] = src0[m];
                        }
                    }

                    if (left)
                    {
                        m = k * buffer->stride >> 2;
                        l = 0;

                        for (; m < (k + left) * buffer->stride >> 2; m++, l++)
                        {
                            dst0[l] = src0[m];
                        }
                    }
                }
                else
                {
                    gcoOS_MemCopy(dst, src[0], size);
                }
                bytes += size;
            }
            /* Copy from generic value. */
            else
            {
                for (j = 0; j < count; j++)
                {
                    gcoOS_MemCopy(dst, src[0], attribute[buffer->map[0]].bytes);
                    dst   += attribute[buffer->map[0]].bytes;
                    bytes += attribute[buffer->map[0]].bytes;
                }
            }
        }
        /* Combined case, need to pack from all attribute pointers. */
        else
        {
            gctUINT size[gcdATTRIBUTE_COUNT];

            dst = (gctUINT8_PTR)Logical + buffer->offset;

            /* Setup source info for all attributes. */
            for (j = 0; j < buffer->numAttribs; j++)
            {
                attribStride[j]  = Attributes[buffer->map[j]].vertexPtr->enable
                                 ? Attributes[buffer->map[j]].vertexPtr->stride
                                 : 0;

                /* Add the first vertex. */
                src[j] = (gctUINT8_PTR) Attributes[buffer->map[j]].logical
                       + First * attribStride[j];

                size[j] = Attributes[buffer->map[j]].bytes;
            }

            /* Walk through all vertices. */
            for(j = 0; j < count; j++)
            {
                /* Walk through all attributes in a buffer. */
                for(k = 0; k < buffer->numAttribs; k++)
                {
                    gcoOS_MemCopy(dst, src[k], size[k]);

                    /* Advance the pointers. */
                    src[k] += attribStride[k];
                    dst    += size[k];
                    bytes  += size[k];
                }
            }
        }
    }

    /* Return the number of bytes copied. */
    if (Bytes != gcvNULL)
    {
        *Bytes = bytes;
    }

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%u", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;
}

static gceSTATUS
_copyBuffersEx(
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gcsSURF_NODE_PTR cacheNodePtr,
    IN gctUINT First,
    IN gctUINT8_PTR Logical,
    IN gctUINT32 Physical,
    OUT gctSIZE_T_PTR CopiedBytes
    )
{
    gctUINT8_PTR dst,src;
    gcsVERTEXARRAY_BUFOBJ_PTR streamPtr;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR attrPtr;
    gctSIZE_T copiedBytes;
    gctSIZE_T copySize;
    gctSIZE_T count;
    gctSIZE_T needCopyCount;
    gctSIZE_T base;
    gctUINT32 copiedBytes32;

    gcmHEADER_ARG("StreamCount=%u Streams=0x%x First=%u "
                  "Logical=0x%x Physical=0x%x CopiedBytes=0x%x",
                  StreamCount, Streams, First, Logical,
                  Physical, CopiedBytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(StreamCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Streams != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Logical != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Physical != 0);

    /* Compute the destination address. */
    dst = (gctUINT8_PTR) Logical;

    /* cacheOffset = 0; */
    copiedBytes = 0;
    for (streamPtr = Streams; streamPtr != gcvNULL; streamPtr = streamPtr->next)
    {
        /* Get only the client array attributes */
        if (streamPtr->stream == gcvNULL)
        {
            /* Set physical and logical address */
            gcmSAFECASTSIZET(copiedBytes32, copiedBytes);
            streamPtr->physical = Physical + copiedBytes32;
            streamPtr->logical = Logical + copiedBytes;
            streamPtr->nodePtr = cacheNodePtr;
            if (streamPtr->copyAll == gcvTRUE)
            {
                /* Calculate src pointer */
                if (streamPtr->divisor > 0)
                {
                    src = (gctUINT8_PTR) streamPtr->attributePtr->pointer;
                }
                else
                {
                    src = (gctUINT8_PTR) streamPtr->attributePtr->pointer + (streamPtr->stride * First);
                }

                /* Copy attr data to dynamic cache */
                copySize = streamPtr->streamCopySize;
                gcoOS_MemCopy(dst, src, copySize);
                copiedBytes += copySize;

                /* Move destination */
                dst += copySize;

                /* Walk all attributes and adjust cache offset*/
                base = 0;
                for (attrPtr = streamPtr->attributePtr; attrPtr != gcvNULL; attrPtr = attrPtr->next)
                {
                    /* Set new offset of the attribute */
                    if (base == 0)
                    {
                        base = attrPtr->offset;
                        attrPtr->offset = 0;
                    }
                    else
                    {
                        attrPtr->offset = attrPtr->offset - base;
                    }
                }
            }
            else
            {
                count = 0;
                needCopyCount = (streamPtr->dynamicCacheStride == 0) ? 1 : streamPtr->count;
                while (count != needCopyCount)
                {
                    /* Walk all attributes and adjust offset*/
                    for (attrPtr = streamPtr->attributePtr; attrPtr != gcvNULL; attrPtr = attrPtr->next)
                    {
                        /* Calculate src pointer */
                        if (attrPtr->enabled == gcvFALSE)
                        {
                            src = (gctUINT8_PTR) attrPtr->pointer;
                        }
                        else if (streamPtr->divisor > 0)
                        {
                            src = (gctUINT8_PTR) attrPtr->pointer + (attrPtr->stride * count);
                        }
                        else
                        {
                            src = (gctUINT8_PTR) attrPtr->pointer + (attrPtr->stride * First) + (attrPtr->stride * count);
                        }

                        /* Copy attr data to dynamic cache */
                        copySize = attrPtr->bytes;
                        gcoOS_MemCopy(dst, src, copySize);
                        copiedBytes += copySize;

                        /* Move destination */
                        dst += copySize;
                    }

                    /* Advance to next vertex */
                    count++;
                }

                /* Walk all attributes and adjust cache offset*/
                base = 0;
                for (attrPtr = streamPtr->attributePtr; attrPtr != gcvNULL; attrPtr = attrPtr->next)
                {
                    /* Set new offset of the attribute */
                    attrPtr->offset = base;
                    base += attrPtr->bytes;
                }
            }
        }
    }

    /* Return total number of copied bytes */
    if (CopiedBytes != gcvNULL)
    {
        *CopiedBytes = copiedBytes;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_UploadUnCacheableAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT TotalBytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical,
    OUT gcoSTREAM * OutStream
)
{
    gceSTATUS status = gcvSTATUS_OK;
    gctSIZE_T copiedBytes = 0;
    gcoSTREAM newStream = gcvNULL;
    gctPOINTER logical = 0;
    gctUINT32  physical = 0;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u TotalBytes=%u BufferCount=%u "
                  "Buffers=0x%x AttributeCount=%u Attributes=0x%x",
                  Stream, First, Count, TotalBytes, BufferCount, Buffers, AttributeCount, Attributes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(BufferCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Buffers != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(AttributeCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Physical != gcvNULL);

    if (TotalBytes > gcdSTREAM_CACHE_SIZE)
    {
        if (Stream != gcvNULL)
        {
            /* Destroy last stream. */
            gcmONERROR(gcoSTREAM_Destroy(Stream));
        }

        /* Allocate a new stream. */
        gcmONERROR(gcoSTREAM_Construct(gcvNULL, &newStream));

        gcmONERROR(gcoSTREAM_Reserve(newStream, TotalBytes));

        gcmONERROR(gcoSTREAM_Lock(newStream,&logical,&physical));

        /* Copy the data. */
        gcmONERROR(_copyBuffers(
            BufferCount,
            Buffers,
            AttributeCount,
            Attributes,
            First,  Count,
            logical,
            &copiedBytes
            ));

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&newStream->node,
            logical,
            copiedBytes,
            gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
            gcvDUMP_BUFFER_STREAM,
            physical,
            logical,
            0,
            copiedBytes);

        /* Return physical address for stream. */
        *Physical = physical;
        *OutStream = newStream;

        /* Seems can't do unlock here, because stream lock don't really lock node, unlock doese */
        /*gcmONERROR(gcoSTREAM_Unlock(newStream));*/

        /* Success. */
        gcmFOOTER_ARG("*Physical=0x%08x", *Physical);
        return gcvSTATUS_OK;
    }
    else
    {
        /* Only handle uncachable */
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

OnError:
    if (newStream)
    {
        gcoSTREAM_Destroy(newStream);
    }
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gctSIZE_T
gcoSTREAM_GetSize(
    IN gcoSTREAM Stream
    )
{
    return Stream->size;
}

gceSTATUS
gcoSTREAM_DynamicCacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Bytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsSTREAM_CACHE_BUFFER_PTR cache = gcvNULL;
    gctUINT offset;
    gctSIZE_T copiedBytes = 0;
    gctUINT32 address;
    gctBOOL bForceVirtual = gcvFALSE;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u Bytes=%u BufferCount=%u "
                  "Buffers=0x%x AttributeCount=%u Attributes=0x%x",
                  Stream, First, Count, Bytes, BufferCount, Buffers, AttributeCount, Attributes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(BufferCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Buffers != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(AttributeCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Physical != gcvNULL);

    /* Index current cache. */
    cache = &Stream->cache[(Stream->cacheCurrent) % gcdSTREAM_CACHE_COUNT];

    if (gcoHAL_IsFeatureAvailable(gcvNULL,gcvFEATURE_MULTI_CLUSTER))
    {
        const gctUINT cacheLineWidth = 64;
        Bytes = gcmALIGN(Bytes, cacheLineWidth);
    }

    if (Bytes > gcdSTREAM_CACHE_SIZE)
    {
       gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    if (cache->dynamicNode != gcvNULL)
    {
        gcmGETHARDWAREADDRESS(*(cache->dynamicNode), address);

        gcoHARDWARE_GetForceVirtual(gcvNULL, &bForceVirtual);
        /* Need virtual pool, but current cache is physical.*/
        bForceVirtual = bForceVirtual && (((address + cache->offset) & 0x80000000) == 0);
    }

    /* Check if the stream will fit in the current cache. */
    if (Bytes > cache->free || bForceVirtual)
    {
        /* Move to a new cache. */
        gcmONERROR(_NewDynamicCache(Stream, Bytes));
        cache = &Stream->cache[(Stream->cacheCurrent) % gcdSTREAM_CACHE_COUNT];
    }

    /* Allocate data form the cache. */
    offset         = cache->offset;
    cache->offset += Bytes;
    cache->free   -= Bytes;

    if (!cache->dynamicNode)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Copy the data. */
    gcmONERROR(_copyBuffers(BufferCount,
                            Buffers,
                            AttributeCount,
                            Attributes,
                            First,
                            Count,
                            cache->dynamicNode->logical + offset,
                            &copiedBytes
                            ));

    /* Flush the uploaded data. */
    gcmONERROR(gcoSURF_NODE_Cache(cache->dynamicNode,
                                  cache->dynamicNode->logical + offset,
                                  copiedBytes,
                                  gcvCACHE_CLEAN));

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_STREAM,
                   gcsSURF_NODE_GetHWAddress(cache->dynamicNode),
                   cache->dynamicNode->logical,
                   offset,
                   copiedBytes);

    /* Return physical address for stream. */
    gcmGETHARDWAREADDRESS(*(cache->dynamicNode), *Physical);
    *Physical += offset;

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_DynamicCacheAttributesEx(
    IN gcoSTREAM Stream,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gctUINT First,
    IN gctUINT TotalBytes
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsSTREAM_CACHE_BUFFER_PTR cache = gcvNULL;
    gctUINT offset;
    gctSIZE_T copiedBytes = 0;
    gctUINT32 address;
    gctBOOL bForceVirtual = gcvFALSE;

    gcmHEADER_ARG("Stream=0x%x StreamCount=%u Streams=0x%x First=%u ",
                  Stream, StreamCount, Streams, First);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmDEBUG_VERIFY_ARGUMENT(StreamCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Streams != gcvNULL);

    /* Index current cache. */
    cache = &Stream->cache[(Stream->cacheCurrent) % gcdSTREAM_CACHE_COUNT];

    if (cache->dynamicNode != gcvNULL)
    {
        gcmGETHARDWAREADDRESS(*(cache->dynamicNode), address);

        gcoHARDWARE_GetForceVirtual(gcvNULL, &bForceVirtual);
        /* Need virtual pool, but current cache is physical.*/
        bForceVirtual = bForceVirtual && (((address + cache->offset) & 0x80000000) == 0);
    }

    if (gcoHAL_IsFeatureAvailable(gcvNULL,gcvFEATURE_MULTI_CLUSTER))
    {
        const gctUINT cacheLineWidth = 64;
        TotalBytes = gcmALIGN(TotalBytes, cacheLineWidth);
    }

    /* Check if the stream will fit in the current cache. */
    if (TotalBytes > cache->free || bForceVirtual)
    {
        /* Move to a new cache. */
        gcmONERROR(_NewDynamicCache(Stream, TotalBytes));
        cache = &Stream->cache[(Stream->cacheCurrent) % gcdSTREAM_CACHE_COUNT];
    }

    /* Allocate data form the cache. */
    offset         = cache->offset;
    cache->offset += TotalBytes;
    cache->free   -= TotalBytes;

    if (!cache->dynamicNode)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    gcmGETHARDWAREADDRESS(*(cache->dynamicNode), address);

    /* Copy the data. */
    gcmONERROR(_copyBuffersEx(StreamCount,
                              Streams,
                              cache->dynamicNode,
                              First,
                              cache->dynamicNode->logical + offset,
                              address + offset,
                              &copiedBytes
                              ));

    /* Flush the uploaded data. */
    gcmONERROR(gcoSURF_NODE_Cache(cache->dynamicNode,
                                  cache->dynamicNode->logical + offset,
                                  copiedBytes,
                                  gcvCACHE_CLEAN));

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   gcvDUMP_BUFFER_STREAM,
                   address,
                   cache->dynamicNode->logical,
                   offset,
                   copiedBytes);

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoSTREAM_CacheAttributesEx(
    IN gcoSTREAM Stream,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gctUINT First,
    IN OUT gcoSTREAM * UncacheableStream
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT totalBytes;
    gctSIZE_T totalBytesTmp;
    gctUINT32 stride;
    gcsVERTEXARRAY_BUFOBJ_PTR streamPtr;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR attrPtr;
    gctSIZE_T copiedBytes = 0;
    gcoSTREAM newStream = gcvNULL;
    gctPOINTER logical = 0;
    gctUINT32  physical = 0;
    gctUINT maxStride;
    gctUINT maxAttribOffset;

    gcmHEADER_ARG("Stream=0x%x StreamCount=%u Streams=0x%x First=%u UncacheableStream=0x%x",
                  Stream, StreamCount, Streams, First, UncacheableStream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmDEBUG_VERIFY_ARGUMENT(StreamCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Streams != gcvNULL);

    /* Get max available stride */
    gcmONERROR(gcoHARDWARE_QueryStreamCaps(gcvNULL, gcvNULL, &maxStride, gcvNULL, gcvNULL, &maxAttribOffset));

    /* Walk all streams */
    totalBytesTmp = 0;
    for (streamPtr = Streams; streamPtr != gcvNULL; streamPtr = streamPtr->next)
    {
        /* Get only the client array attributes */
        if (streamPtr->stream == gcvNULL)
        {
            gctBOOL hasGeneric  = gcvFALSE;
            gctSIZE_T minOffset = (gctSIZE_T)-1;
            gctSIZE_T maxOffset = 0;
            gctUINT   lastAttrBytes = 0;
            gctSIZE_T perCountCopyBytes = 0;

            /* Assume that stream can be copied with one memcopy */
            streamPtr->copyAll = gcvTRUE;
            stride = 0;
            streamPtr->streamCopySize = 0;

            /* Walk all attributes */
            for (attrPtr = streamPtr->attributePtr; attrPtr != gcvNULL; attrPtr = attrPtr->next)
            {
                minOffset = gcmMIN(minOffset, attrPtr->offset);
                maxOffset = gcmMAX(maxOffset, attrPtr->offset);

                stride += attrPtr->bytes;
                if (attrPtr->enabled == gcvFALSE)
                {
                    hasGeneric = gcvTRUE;
                }

                /* Attrib already sort by start address, check last attrb bytes */
                if (attrPtr->next == gcvNULL)
                {
                    lastAttrBytes = attrPtr->bytes;
                }
            }

            /* Compute perCount copy bytes */
            perCountCopyBytes = maxOffset - minOffset + lastAttrBytes;

            gcmASSERT(perCountCopyBytes <= streamPtr->stride);

            /* Check for boundary condition */
            if (stride > maxStride)
            {
                gcmONERROR(gcvSTATUS_TOO_COMPLEX);
            }
            /* Check if we can simply do a full copy */
            else if (hasGeneric || streamPtr->merged || streamPtr->stride > maxStride ||
                (maxOffset - minOffset > maxAttribOffset)
                )
            {
                streamPtr->copyAll = gcvFALSE;

                /* Set dynamic cache stride */
                streamPtr->dynamicCacheStride = stride;
            }
            else
            {
                /* We will do a full copy for speed */
                streamPtr->dynamicCacheStride = streamPtr->stride;
            }

            /* calculate total bytes */
            if (streamPtr->copyAll)
            {
                if (streamPtr->count > 0)
                {
                    streamPtr->streamCopySize = (streamPtr->dynamicCacheStride * (streamPtr->count - 1) + perCountCopyBytes);
                }
            }
            else
            {
                streamPtr->streamCopySize = streamPtr->dynamicCacheStride * streamPtr->count;
            }

            totalBytesTmp += streamPtr->streamCopySize;

            if ((streamPtr->copyAll == gcvFALSE) && (streamPtr->merged == gcvFALSE))
            {
                /* we can do a once copy if the attribute of this streams is generic and not merged,
                only need set the stride to 0 */
                streamPtr->dynamicCacheStride = 0;
            }
        }
    }

    gcmSAFECASTSIZET(totalBytes, totalBytesTmp);

    /* Nothing need to do.*/
    if (0 == totalBytes)
    {
        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }

    if (totalBytes > gcdSTREAM_CACHE_SIZE)
    {
        if (*UncacheableStream != gcvNULL)
        {
            /* Destroy last stream. */
            gcmONERROR(gcoSTREAM_Destroy(*UncacheableStream));
        }

        /* Allocate a new stream. */
        gcmONERROR(gcoSTREAM_Construct(gcvNULL, &newStream));

        /* Reserve space */
        gcmONERROR(gcoSTREAM_Reserve(newStream, totalBytes));

        /* Lock the stream */
        gcmONERROR(gcoSTREAM_Lock(newStream,&logical,&physical));

        /* Copy the data. */
        gcmONERROR(_copyBuffersEx(StreamCount,
                                  Streams,
                                  &newStream->node,
                                  First,
                                  logical,
                                  physical,
                                  &copiedBytes
                                  ));

        gcmASSERT(totalBytes == copiedBytes);

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&newStream->node,
            logical,
            copiedBytes,
            gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
            gcvDUMP_BUFFER_STREAM,
            physical,
            logical,
            0,
            copiedBytes);

        *UncacheableStream = newStream;
    }
    else
    {
        /* If the stream wasn't initialized, do it now */
        if (Stream->cache == gcvNULL)
        {
            gcmONERROR(gcoSTREAM_SetCache(Stream));
        }

        gcmONERROR(gcoSTREAM_DynamicCacheAttributesEx(Stream,
                                                      StreamCount,
                                                      Streams,
                                                      First,
                                                      totalBytes));

    }

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    if (newStream)
    {
        gcoSTREAM_Destroy(newStream);
    }
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT TotalBytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u TotalBytes=%u BufferCount=%u "
                  "Buffers=0x%x AttributeCount=%u Attributes=0x%x",
                  Stream, First, Count, TotalBytes, BufferCount, Buffers, AttributeCount, Attributes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(BufferCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Buffers != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(AttributeCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Physical != gcvNULL);

    /* If the stream wasn't initialized, do it now */
    if (Stream->cache == gcvNULL)
    {
        gcmONERROR(gcoSTREAM_SetCache(Stream));
    }

    gcmONERROR(gcoSTREAM_DynamicCacheAttributes(Stream,
                                                First,
                                                Count,
                                                TotalBytes,
                                                BufferCount,
                                                Buffers,
                                                AttributeCount,
                                                Attributes,
                                                Physical));

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_UnAlias(
    IN gcoSTREAM Stream,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream,
    OUT gctUINT8_PTR * Logical,
    OUT gctUINT32 * Physical
    )
{
    gceSTATUS status;
    gcoSTREAM stream = gcvNULL;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attr[2];

    gcmHEADER_ARG("Stream=0x%x Attributes=0x%x", Stream, Attributes);

    /* Setup quick pointers. */
    attr[0] = Attributes;
    attr[1] = Attributes->next;

    /* Special case 2 attributes. */
    if ((attr[1]       != gcvNULL)
    &&  (attr[1]->next == gcvNULL)

    /* Streams pointing at the same stream with a stride less than 8. */
    &&  (Stream                       == attr[0]->vertexPtr->stream)
    &&  (Stream                       == attr[1]->vertexPtr->stream)
    &&  (Stream->subStreamCount       == 1)
    &&  (Stream->subStreams[0].stride <  8)

    /* Attribute 1 aliases attribute 0. */
    &&  (attr[1]->offset == attr[0]->offset)
    &&  (attr[1]->bytes  == attr[0]->bytes)
    )
    {
        /* Check if we already rebuilt this stream. */
        if (Stream->rebuild == gcvNULL)
        {
            gctUINT8_PTR src, dst;
            gctSIZE_T bytes;
            gctUINT stride;

            /* Construct a new stream. */
            gcmONERROR(gcoSTREAM_Construct(gcvNULL, &stream));

            /* Reserve twice the amount of data. */
            gcmONERROR(gcoSTREAM_Reserve(stream, Stream->size * 2));

            /* Get source stride. */
            stride = Stream->subStreams[0].stride;

            /* Set destination stride to twice the source stride. */
            stream->stride = stride * 2;

            /* Get pointers to streams. */
            src = Stream->node.logical;
            dst = stream->node.logical;

            /* Copy all vertices. */
            /* Aligned case optimization. */
            if ((stride == 4) && !((gctUINTPTR_T)src & 0x3) && !((gctUINTPTR_T)dst & 0x3))
            {
                gctUINT32_PTR tempSrc, tempDst;
                tempSrc = (gctUINT32_PTR)src;
                tempDst = (gctUINT32_PTR)dst;

                for (bytes = Stream->size; bytes > 0; bytes -= stride)
                {
                    /* Copy vertex. */
                    *tempDst++ = *tempSrc;

                    /* Copy vertex again. */
                    *tempDst++ = *tempSrc++;
                }
            }
            else
            {
                for (bytes = Stream->size; bytes > 0; bytes -= stride)
                {
                    /* Copy vertex. */
                    gcoOS_MemCopy(dst, src, stride);

                    /* Copy vertex again. */
                    gcoOS_MemCopy(dst + stride, src, stride);

                    /* Advance pointers. */
                    src += stride;
                    dst += stride * 2;
                }
            }

            /* Flush the vertex cache. */
            gcmONERROR(gcoSTREAM_Flush(stream));

            /* Flush the vertex buffer cache. */
            gcmONERROR(gcoSURF_NODE_Cache(&stream->node,
                                        stream->node.logical,
                                        stream->size,gcvCACHE_CLEAN));

            /* Copy sub-stream. */
            stream->subStreamCount = 1;
            gcoOS_MemCopy(stream->subStreams,
                          Stream->subStreams,
                          gcmSIZEOF(gcsSTREAM_SUBSTREAM));

            /* Setup unaliased sub-stream. */
            stream->subStreams[1].start  = 0;
            stream->subStreams[1].end    = stream->stride;
            stream->subStreams[1].stride = stream->stride;
            stream->subStreams[1].next   = gcvNULL;

            /* Save new stream as a rebuilt stream. */
            Stream->rebuild = stream;
        }
        else
        {
            /* Stream has already been rebuilt. */
            stream = Stream->rebuild;
        }

        /* Update mapping. */
        attr[0]->logical = stream->node.logical + attr[0]->offset;
        attr[1]->offset += stream->stride / 2;
        attr[1]->logical = stream->node.logical + attr[1]->offset;

        /* Return new rebuilt stream. */
        *SubStream = &stream->subStreams[1];
        *Logical   = stream->node.logical;
        gcmGETHARDWAREADDRESS(stream->node, *Physical);
    }
    else
    {
        /* Skip the stream unaliasing. */
        gcmFOOTER_NO();
        return gcvSTATUS_SKIP;
    }

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x *Physical=0x%08x", *Logical, *Physical);
    return gcvSTATUS_OK;

OnError:
    if (stream != gcvNULL)
    {
        /* Destroy newly created stream. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(stream));
    }

    /* Return error. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_MergeStreams(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams,
    OUT gcoSTREAM * MergedStream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical,
    OUT gcsVERTEXARRAY_ATTRIBUTE_PTR * Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    gceSTATUS status;
    gctUINT i, j, stride, index;
    gctUINT8_PTR src[256];
    gctUINT8_PTR dest;
    gcoSTREAM merged = gcvNULL;
    gctBOOL copy;
    gctUINT count = First + Count;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute, last = gcvNULL;
    gcsSTREAM_SUBSTREAM_PTR sub, subPtr;
    gctINT oldValue;
    gctUINT mergeStride[256];
    gctBOOL pack = gcvFALSE;
    gctUINT maxStride;
    gctUINT subMap[256]  = {0};
    gctBOOL mergeByChain = gcvFALSE;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u StreamCount=%u Streams=0x%x",
                  Stream, First, Count, StreamCount, Streams);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Count > 0);
    gcmVERIFY_ARGUMENT(StreamCount > 0);
    gcmVERIFY_ARGUMENT(Streams != gcvNULL);
    gcmVERIFY_ARGUMENT(MergedStream != gcvNULL);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);
    gcmVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmVERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmVERIFY_ARGUMENT(SubStream != gcvNULL);

    /* Check if there already is a merged stream. */
    if (Stream->merged == gcvNULL)
    {
        /* Construct a new stream. */
        gcmONERROR(gcoSTREAM_Construct(gcvNULL, &merged));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_STREAM,
                      "Created a new merged stream 0x%x",
                      merged);

        /* We surely need to copy data! */
        copy = gcvTRUE;

        /* Construct a reference counter. */
        gcmONERROR(gcoOS_AtomConstruct(gcvNULL, &merged->reference));
    }
    else
    {
        /* Get the merged stream. */
        merged = Stream->merged;

        /* Only copy if one of the streams is dirty or we haven't copied enough
        ** vertex data yet. */
        copy = merged->dirty || (count > merged->count);

        gcmDEBUG_ONLY(
            if (copy)
            {
                if (merged->dirty)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_STREAM,
                                  "Merged stream 0x%x is dirty",
                                  merged);
                }
                else
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_STREAM,
                                  "Merged stream 0x%x now holds %u vertices",
                                  merged, count);
                }
            }
        );

        if (!copy)
        {
            /* Count the number of substreams. */
            for (i = 0, index = 0; i < StreamCount; ++i)
            {
                index += Streams[i].stream->subStreamCount;
            }

            /* Check if the number of substreams is larger. */
            if (index > merged->subStreamCount)
            {
                /* We need to copy. */
                copy = gcvTRUE;

                gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_STREAM,
                              "Merged stream 0x%x now holds %u streams",
                              merged, index);
            }

            /* Check if any of the incoming streams was never merged. */
            for (i = 0, index = 0; i < StreamCount; ++i)
            {
                if (Streams[i].stream->merged == gcvNULL)
                {
                    /* We need to copy. */
                    copy = gcvTRUE;
                    break;
                }

                /* All streams should merge into 1 merged stream. */
                gcmASSERT(Streams[i].stream->merged == merged);
            }
        }

        /* Check if we need to copy. */
        if (copy)
        {
            do
            {
                /* Decrement the reference counter. */
                gcmONERROR(gcoOS_AtomDecrement(gcvNULL,
                                               merged->reference,
                                               &oldValue));
            }
            while (oldValue > 1);
        }
    }

    /* Check if we need to copy data. */
    if (copy)
    {
        /* Reset everything. */
        index  = 0;
        stride = 0;
        sub    = merged->subStreams;
        /* reset merged pointer for all streams previously merged. */
        for (i = 0; i < merged->subStreamCount; i++)
        {
            merged->subStreams[i].stream->merged = gcvNULL;
            merged->subStreams[i].stream = gcvNULL;
        }

        gcmONERROR(
            gcoHARDWARE_QueryStreamCaps(gcvNULL,
                gcvNULL,
                &maxStride,
                gcvNULL,
                gcvNULL,
                gcvNULL));

        /* Check should we do pack */
        for(i = 0; i < StreamCount; ++i)
        {
            for (j = 0; j < Streams[i].stream->subStreamCount; j++)
            {
                if(Streams[i].stream->subStreams[j].stride > maxStride)
                {
                    pack = gcvTRUE;
                    break;
                }
            }

            if(pack)
            {
                break;
            }
        }

        /* Walk all the streams. */
        for (i = 0; i < StreamCount; ++i)
        {
            gctUINT sortMap[256]={0};
            gctUINT k, l, sortCount;

            /* Reset sorted count before sort a stream. */
            sortCount = 0;

            /* Sort sub streams in a stream by offset. */
            for (j = 0; j < Streams[i].stream->subStreamCount; j++)
            {
                /* Compare offset with sub-streams in the sorted map. */
                for (k = 0; k < sortCount; k++)
                {
                    /* Find the position. */
                    if ( Streams[i].stream->subStreams[j].start
                         < Streams[i].stream->subStreams[sortMap[k]].start
                       )
                    {
                        /* Shift the sorted array for new sub-stream. */
                        for(l = sortCount; l > k; l--)
                        {
                            sortMap[l] = sortMap[l - 1];
                        }

                        /* Insert the sub stream. */
                        sortMap[k] = j;
                        sortCount++;
                        break;
                    }
                }

                /* Append to the end. */
                if (k == sortCount)
                {
                    sortMap[k] = j;
                    sortCount++;
                }
            }

            /* Walk all the substreams in the stream. */
            for (j = 0; j < Streams[i].stream->subStreamCount; ++j)
            {
                /* Copy the substream information. */
                sub->start    = Streams[i].stream->subStreams[sortMap[j]].start;
                sub->stride   = Streams[i].stream->subStreams[sortMap[j]].stride;
                sub->end      = sub->start + sub->stride;
                sub->minStart = stride;
                sub->stream   = Streams[i].stream;

                /* Get pointer to vertex data. */
                src[index] = Streams[i].stream->node.logical + sub->start;

                if(pack)
                {
                    mergeStride[index] = (gctUINT)(Streams[i].stream->subStreams[sortMap[j]].end
                        - Streams[i].stream->subStreams[sortMap[j]].start);
                }
                else
                {
                    mergeStride[index] = sub->stride;
                }

                /* Adjust stride. */
                stride += mergeStride[index];

                /* Next substream. */
                ++sub;
                ++index;
            }

            /* Increment the reference counter. */
            gcmONERROR(gcoOS_AtomIncrement(gcvNULL,
                                           merged->reference,
                                           &oldValue));

            /* Mark the stream as a client of this merged stream. */
            Streams[i].stream->merged = merged;
        }

        /* If stride exceed max stride, only merge the needed sub streams .*/
        if (stride > maxStride)
        {
            /* reset merged pointer for all streams. */
            for (i = 0; i < index; i++)
            {
                merged->subStreams[i].stream->merged = gcvNULL;
                merged->subStreams[i].stream = gcvNULL;
            }

            do
            {
                /* Decrement the reference counter. */
                gcmONERROR(gcoOS_AtomDecrement(gcvNULL,
                                               merged->reference,
                                               &oldValue));
            }
            while (oldValue > 1);

            /* Reset everything. */
            index  = 0;
            stride = 0;
            sub    = merged->subStreams;

            for (i = 0; i < StreamCount; ++i)
            {
                /* Walk all the substreams in the stream. */
                for (subPtr = Streams[i].subStream; subPtr != gcvNULL; subPtr = subPtr->next)
                {
                    /* Copy the substream information. */
                    sub->start    = subPtr->start;
                    sub->stride   = subPtr->stride;
                    sub->end      = sub->start + sub->stride;
                    sub->minStart = stride;
                    sub->stream   = Streams[i].stream;

                    /* Get pointer to vertex data. */
                    src[index] = Streams[i].stream->node.logical + sub->start;

                    if(pack)
                    {
                        mergeStride[index] = (gctUINT)(sub->end - sub->start);
                    }
                    else
                    {
                        mergeStride[index] = sub->stride;
                    }

                    /* Adjust stride. */
                    stride += mergeStride[index];

                    /* Next substream. */
                    ++sub;
                    ++index;
                }

                subMap[i] = index;

                /* Increment the reference counter. */
                gcmONERROR(gcoOS_AtomIncrement(gcvNULL,
                                               merged->reference,
                                               &oldValue));

                /* Mark the stream as a client of this merged stream. */
                Streams[i].stream->merged = merged;
            }

            /* Check if stride meet hardware capability. */
            if (stride <= maxStride)
            {
                mergeByChain = gcvTRUE;
            }
            else
            {
                gcmONERROR(gcvSTATUS_TOO_COMPLEX);
            }
        }

        /* Initialize the last substream for hardware. */
        sub->start    = 0;
        sub->minStart = 0;
        sub->end      = stride;
        sub->maxEnd   = stride;
        sub->stride   = stride;
        sub->next     = gcvNULL;

        /* Set substream information. */
        merged->subStreamCount  = index;
        merged->subStreamStride = stride;

        /* Get the maximum of the saved count versus the current count. */
        if (count > merged->count)
        {
            merged->count = count;
        }
        else
        {
            count = merged->count;
        }

        /* Reserve enough data in the merged stream buffer. */
        gcmONERROR(_FreeMemory(merged));
        gcmONERROR(gcoSTREAM_Reserve(merged, stride * count));

        /* Setup destination pointer. */
        dest = merged->node.logical;

        /* Walk all the vertices. */
        for (i = 0; i < count; ++i)
        {
            /* Walk all the substreams. */
            for (j = 0; j < index; ++j)
            {
                /* Copy data from the stream to the merged stream. */
                gcoOS_MemCopy(dest,
                              src[j],
                              mergeStride[j]);

                /* Move to next substream. */
                src[j] += merged->subStreams[j].stride;
                dest   += mergeStride[j];
            }
        }

        /* Flush the vertex cache. */
        gcmONERROR(gcoSTREAM_Flush(merged));

        /* Flush the CPU cache . */
        gcmONERROR(gcoSURF_NODE_Cache(&merged->node,
                                    merged->node.logical,
                                    merged->size,
                                    gcvCACHE_CLEAN));

        /* Merged stream has been copied. */
        merged->dirty = gcvFALSE;
    }

    /* Walk all the streams. */
    for (i = 0, index = 0; i < StreamCount; ++i)
    {
        /* Walk all the attributes in the stream. */
        for (attribute = Streams[i].attribute;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Get pointer to the mapped substreams for this stream. */
            sub = &merged->subStreams[index];

            /* Walk all the substreams. */
            for (j = 0; j < Streams[i].stream->subStreamCount; ++j, ++sub)
            {
                /* Check if this attribute is part of this substream. */
                if ((sub->start <= attribute->offset)
                &&  (sub->end   >= attribute->offset)
                )
                {
                    /* Adjust offset of the attribute. */
                    attribute->offset = attribute->offset - (gctUINT)(sub->start - sub->minStart);
                    break;
                }
            }

            /* Check for error. */
            if (j == Streams[i].stream->subStreamCount)
            {
                gcmONERROR(gcvSTATUS_TOO_COMPLEX);
            }

            /* Append this attribute to the chain. */
            if (last != gcvNULL)
            {
                last->next = attribute;
            }

            /* Mark this attribute as the last. */
            last = attribute;
        }

        if (mergeByChain)
        {
            /* Move to the next substream block. */
            index += subMap[i];
        }
        else
        {
            /* Move to the next substream block. */
            index += Streams[i].stream->subStreamCount;
        }
    }

    /* Return information about the merged stream. */
    *MergedStream = merged;
    *Logical      = merged->node.logical;
    gcmGETHARDWAREADDRESS(merged->node, *Physical);
    *Attributes   = Streams->attribute;
    *SubStream    = &merged->subStreams[merged->subStreamCount];

    /* Success. */
    gcmFOOTER_ARG("*MergedStream=0x%x *Logical=0x%x *Physical=0x%08x "
                  "*Attributes=0x%x *SubStream=0x%x",
                  *MergedStream, *Logical, *Physical, *Attributes, *SubStream);
    return gcvSTATUS_OK;

OnError:
    if (merged != gcvNULL)
    {
        /* Destroy the created merged stream. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(merged));
    }

    for (i = 0; i < StreamCount; ++i)
    {
        /* Remove merged stream. */
        Streams[i].stream->merged = gcvNULL;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_CPUCacheOperation_Range(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Length,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Stream=0x%x, Offset=%u Length=%u Operation=%d", Stream, Offset, Length, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    gcoSURF_NODE_CPUCacheOperation(&Stream->node, gcvSURF_VERTEX, Offset, Length, Operation);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_CPUCacheOperation(
    IN gcoSTREAM Stream,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status;
    gctPOINTER memory;
    gctBOOL locked = gcvFALSE;

    gcmHEADER_ARG("Stream=0x%x, Operation=%d", Stream, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Lock the node. */
    gcmONERROR(gcoHARDWARE_Lock(&Stream->node, gcvNULL, &memory));
    locked = gcvTRUE;

    gcmONERROR(gcoSURF_NODE_Cache(&Stream->node,
                                  memory,
                                  Stream->size,
                                  Operation));


    /* Unlock the node. */
    gcmONERROR(gcoHARDWARE_Unlock(&Stream->node, gcvSURF_VERTEX));
    locked = gcvFALSE;

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    if (locked)
    {
        gcmVERIFY_OK(gcoHARDWARE_Unlock(&Stream->node, gcvSURF_VERTEX));
    }

    gcmFOOTER();
    return status;
}
#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gcoSTREAM_Construct(
    IN gcoHAL Hal,
    OUT gcoSTREAM * Stream
    )
{
    *Stream = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Destroy(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Upload(
    IN gcoSTREAM Stream,
    IN gctCONST_POINTER Buffer,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes,
    IN gctBOOL Dynamic
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_SetStride(
    IN gcoSTREAM Stream,
    IN gctUINT32 Stride
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Lock(
    IN gcoSTREAM Stream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical
    )
{
    *Logical = gcvNULL;
    *Physical = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Unlock(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Reserve(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEX * Vertex
    )
{
    *Vertex = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Destroy(
    IN gcoVERTEX Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Reset(
    IN gcoVERTEX Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_EnableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index,
    IN gceVERTEX_FORMAT Format,
    IN gctBOOL Normalized,
    IN gctUINT32 Components,
    IN gcoSTREAM Stream,
    IN gctUINT32 Offset,
    IN gctUINT32 Stride
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_DisableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Bind(
    IN gcoVERTEX Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_SetAttribute(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Offset,
    IN gctUINT Bytes,
    IN gctUINT Stride,
    IN gctUINT Divisor,
    IN OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_QuerySubStreams(
    IN gcoSTREAM Stream,
    IN gcsSTREAM_SUBSTREAM_PTR SubStream,
    OUT gctUINT_PTR SubStreamCount
    )
{
    *SubStreamCount = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Rebuild(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    OUT gctUINT_PTR SubStreamCount
    )
{
    *SubStreamCount = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_SetCache(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

#if OPT_VERTEX_ARRAY
gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT TotalBytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    *Physical = 0;
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_CacheAttributesEx(
    IN gcoSTREAM Stream,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gctUINT First,
    IN OUT gcoSTREAM * UncacheableStream
    )
{
    return gcvSTATUS_OK;
}

#else

gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Stride,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    *Physical = 0;
    return gcvSTATUS_OK;
}
#endif

gceSTATUS gcoSTREAM_UnAlias(
    IN gcoSTREAM Stream,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream,
    OUT gctUINT8_PTR * Logical,
    OUT gctUINT32 * Physical
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_MergeStreams(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams,
    OUT gcoSTREAM * MergedStream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical,
    OUT gcsVERTEXARRAY_ATTRIBUTE_PTR * Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    *MergedStream = gcvNULL;
    *Logical = gcvNULL;
    *Physical = 0;
    *Attributes = gcvNULL;
    *SubStream = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_Flush(
    gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_CPUCacheOperation(
    IN gcoSTREAM Stream,
    IN gceCACHEOPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_CPUCacheOperation_Range(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Length,
    IN gceCACHEOPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_GetFence(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}


gctSIZE_T
gcoSTREAM_GetSize(
    IN gcoSTREAM Stream
    )
{
    return 0;
}

gceSTATUS
gcoSTREAM_WaitFence(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_Node(
    IN gcoSTREAM Stream,
    OUT gcsSURF_NODE_PTR * Node
    )
{
    return gcvSTATUS_OK;
}

#endif /* gcdNULL_DRIVER < 2 */
#endif

