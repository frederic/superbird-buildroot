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
**  gcoHEAP object for kernel HAL layer.  The heap implemented here is an arena-
**  based memory allocation.  An arena-based memory heap allocates data quickly
**  from specified arenas and reduces memory fragmentation.
**
*/
#include "gc_hal_user_precomp.h"

#define _GC_OBJ_ZONE                    gcvZONE_HEAP

#define gcdDEBUG_HEAP_SIGNATURE         0
#define gcdDEBUG_HEAP_WRITE_AFTER_FREE  0

#define gcdDEBUG_HEAP                   ( gcdDEBUG_HEAP_SIGNATURE \
                                        | gcdDEBUG_HEAP_WRITE_AFTER_FREE \
                                        )

#if gcdDEBUG_HEAP_SIGNATURE
static gctUINT8 _Signature[8] =         { 0x00, 0xFF, 0x55, 0xAA,
                                          0xFF, 0xAA, 0x55, 0x00 };
#endif

#define gcdDEBUG_FILLER                 0xCD

#if gcdUSE_NEW_HEAP
#define gcdNumOfBuckets                 12
#define gcdBucketThreshold              ((gcdNumOfBuckets - 1) * 8)
static gctUINT _InitNumber[gcdNumOfBuckets] = {  0, 50, 50, 50, 100, 100,
                                               100, 20, 50, 10, 100,  10 };
#endif

/*******************************************************************************
***** Structures ***************************************************************
*******************************************************************************/
#define gcdIN_USE               ((gcsNODE_PTR)gcvMAXUINTPTR_T)

typedef struct _gcsNODE *       gcsNODE_PTR;
typedef struct _gcsNODE
{
    /* Number of bytes in node. */
    gctSIZE_T                   bytes;

    /* Pointer to next free node, or gcvNULL to mark the node as freed, or
    ** gcdIN_USE to mark the node as used. */
    gcsNODE_PTR                 next;

#if gcmIS_DEBUG(gcdDEBUG_CODE)
    /* Time stamp of allocation. */
    gctUINT64                   timeStamp;
#endif

#if gcdDEBUG_HEAP
    /* Original size in bytes. */
    gctSIZE_T                   unalignedBytes;
#endif
#if gcdDEBUG_HEAP_SIGNATURE
    /* Signature. */
    gctUINT8                    signature[8];
#endif
}
gcsNODE;

#if gcdUSE_NEW_HEAP
typedef struct _gcsSNODE *      gcsSNODE_PTR;
typedef struct _gcsSNODE
{
    /* Number of bytes in node node when the node is in use. */
    gctSIZE_T                   bytes;

    /* Pointer to next free node when the node is free. */
    gcsSNODE_PTR                next;

#if gcmIS_DEBUG(gcdDEBUG_CODE)
    /* Time stamp of allocation. */
    gctUINT64                   timeStamp;
#endif

#if gcdDEBUG_HEAP
    /* Original size in bytes. */
    gctSIZE_T                   unalignedBytes;
#endif
#if gcdDEBUG_HEAP_SIGNATURE
    /* Signature. */
    gctUINT8                    signature[8];
#endif
}
gcsSNODE;

typedef struct _gcsBLOCK *      gcsBLOCK_PTR;
typedef struct _gcsBLOCK
{
    /* Pointer to next free node. */
    gcsBLOCK_PTR                next;
}
gcsBLOCK;
#endif

typedef struct _gcsHEAP *       gcsHEAP_PTR;
typedef struct _gcsHEAP
{
    /* Linked list. */
    gcsHEAP_PTR                 next;
    gcsHEAP_PTR                 prev;

    /* Heap size. */
    gctSIZE_T                   size;

    /* Free list. */
    gcsNODE_PTR                 freeList;
}
gcsHEAP;

struct _gcoHEAP
{
    /* Object. */
    gcsOBJECT                   object;

    /* Locking mutex. */
    gctPOINTER                  mutex;

    /* Allocation parameters. */
    gctSIZE_T                   allocationSize;

    /* Heap list. */
    gcsHEAP_PTR                 heap;
#if gcmIS_DEBUG(gcdDEBUG_CODE)
    gctUINT64                   timeStamp;
#endif

#if gcdUSE_NEW_HEAP
    /* Allocated block list for snodes in buckets. */
    gcsBLOCK_PTR                blockList;

    /* Snodes list for each bucket. */
    gcsSNODE_PTR                buckets[gcdNumOfBuckets];

    /* Number of snodes allocated in each bucket. */
    gctUINT                     nodesInBuckets[gcdNumOfBuckets];

    /* Snode overhead in terms of gcmSIZEOF(gctINT_PTR). */
    gctUINT                     snodeOverhead;
#endif

#if VIVANTE_PROFILER_SYSTEM_MEMORY || gcmIS_DEBUG(gcdDEBUG_CODE)
    /* Profile information. */
    gctUINT32                   allocCount;
    gctUINT64                   allocBytes;
    gctUINT64                   allocBytesMax;
    gctUINT64                   allocBytesTotal;
    gctUINT32                   heapCount;
    gctUINT32                   heapCountMax;
    gctUINT64                   heapMemory;
    gctUINT64                   heapMemoryMax;
#endif
};

#if gcdUSE_NEW_HEAP
#ifdef __GNUC__
#define SNODE_NEXT_OFFSET       (gctUINT) (gctSIZE_T) &(((gcsSNODE_PTR)0)->next)
#else
#ifdef UNDER_CE
#define SNODE_NEXT_OFFSET       (gctUINT) (gctSIZE_T) &(((gcsSNODE_PTR)0)->next)
#else
#define SNODE_NEXT_OFFSET       (gctUINT) (gctUINT64) &(((gcsSNODE_PTR)0)->next)
#endif
#endif
#endif

/*******************************************************************************
***** Static Support Functions *************************************************
*******************************************************************************/

#if gcmIS_DEBUG(gcdDEBUG_CODE)
static gctSIZE_T
_DumpHeap(
    IN gcsHEAP_PTR Heap
    )
{
    gctPOINTER p;
    gctSIZE_T leaked = 0;

    /* Start at first node. */
    for (p = Heap + 1;;)
    {
        /* Convert the pointer. */
        gcsNODE_PTR node = (gcsNODE_PTR) p;

        /* Check if this is a used node. */
        if (node->next == gcdIN_USE)
        {
            /* Print the leaking node. */
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_HEAP,
                          "Detected leaking: node=0x%x bytes=%lu timeStamp=%llu "
                          "(%08X %c%c%c%c)",
                          node, node->bytes, node->timeStamp,
                          ((gctUINT32_PTR) (node + 1))[0],
                          gcmPRINTABLE(((gctUINT8_PTR) (node + 1))[0]),
                          gcmPRINTABLE(((gctUINT8_PTR) (node + 1))[1]),
                          gcmPRINTABLE(((gctUINT8_PTR) (node + 1))[2]),
                          gcmPRINTABLE(((gctUINT8_PTR) (node + 1))[3]));

            /* Add leaking byte count. */
            leaked += node->bytes;
        }

        /* Test for end of heap. */
        if (node->bytes == 0)
        {
            break;
        }

        else
        {
            /* Move to next node. */
            p = (gctUINT8_PTR) node + node->bytes;
        }
    }

    /* Return the number of leaked bytes. */
    return leaked;
}
#endif

static gceSTATUS
_CompactHeap(
    IN gcoHEAP Heap
    )
{
    gcsHEAP_PTR heap, next;
    gctPOINTER p;
    gcsHEAP_PTR freeList = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Heap=0x%x", Heap);

    /* Walk all the heaps. */
    for (heap = Heap->heap; heap != gcvNULL; heap = next)
    {
        gcsNODE_PTR lastFree = gcvNULL;

        /* Zero out the free list. */
        heap->freeList = gcvNULL;

        /* Start at the first node. */
        for (p = (gctUINT8_PTR) (heap + 1);;)
        {
            /* Convert the pointer. */
            gcsNODE_PTR node = (gcsNODE_PTR) p;
            gctSIZE_T bytes  = node->bytes;

            gcmASSERT(p <=
                      (gctPOINTER) ((gctUINT8_PTR) (heap + 1) + heap->size));

            /* Test if this node not used. */
            if (node->next != gcdIN_USE)
            {
                /* Test if this is the end of the heap. */
                if (bytes == 0)
                {
                    break;
                }

#if gcdDEBUG_HEAP_WRITE_AFTER_FREE
                if (node->unalignedBytes > 0)
                {
                    gctSIZE_T i;

                    /* Verify if this free node has been changed. */
                    for (i = gcmSIZEOF(*node); i < node->bytes; ++i)
                    {
                        if (((gctUINT8_PTR) node)[i] != gcdDEBUG_FILLER)
                        {
                            gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
                                          "%s(%d): Free memory modified at "
                                          "0x%x := 0x%02x,%02x,%02x,%02x",
                                          __FUNCTION__, __LINE__,
                                          (gctUINT8_PTR) node + i,
                                          ((gctUINT8_PTR) node)[i],
                                          ((gctUINT8_PTR) node)[i + 1],
                                          ((gctUINT8_PTR) node)[i + 2],
                                          ((gctUINT8_PTR) node)[i + 3]);
                            break;
                        }
                    }

                    node->unalignedBytes = 0;
                }
#endif

                /* Test if this is the first free node. */
                if (lastFree == gcvNULL)
                {
                    /* Initialzie the free list. */
                    heap->freeList = node;
                    lastFree       = node;
                }

                else
                {
                    /* Test if this free node is contiguous with the previous
                    ** free node. */
                    if ((gctUINT8_PTR) lastFree + lastFree->bytes == p)
                    {
                        /* Just increase the size of the previous free node. */
                        lastFree->bytes += bytes;
                    }
                    else
                    {
                        /* Add to linked list. */
                        lastFree->next = node;
                        lastFree       = node;
                    }
                }
            }

            /* Move to next node. */
            p = (gctUINT8_PTR) node + bytes;
        }

        /* Mark the end of the chain. */
        if (lastFree != gcvNULL)
        {
            lastFree->next = gcvNULL;
        }

        /* Get next heap. */
        next = heap->next;

        /* Check if the entire heap is free. */
        if ((heap->freeList != gcvNULL)
        &&  (heap->freeList->bytes == heap->size - gcmSIZEOF(gcsNODE))
        )
        {
            /* Remove the heap from the linked list. */
            if (heap->prev == gcvNULL)
            {
                Heap->heap = next;
            }
            else
            {
                heap->prev->next = next;
            }

            if (heap->next != gcvNULL)
            {
                heap->next->prev = heap->prev;
            }

#if VIVANTE_PROFILER_SYSTEM_MEMORY || gcmIS_DEBUG(gcdDEBUG_CODE)
            /* Update profiling. */
            Heap->heapCount  -= 1;
            Heap->heapMemory -= heap->size + gcmSIZEOF(gcsHEAP);
#endif

            /* Add this heap to the list of heaps that need to be freed. */
            heap->next = freeList;
            freeList   = heap;
        }
    }

    if (freeList != gcvNULL)
    {
        /* Release the mutex, remove any chance for a dead lock. */
        gcmVERIFY_OK(
            gcoOS_ReleaseMutex(gcvNULL, Heap->mutex));

        /* Free all heaps in the free list. */
        for (heap = freeList; heap != gcvNULL; heap = next)
        {
            /* Get pointer to the next heap. */
            next = heap->next;

            /* Free the heap. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                          "Freeing heap 0x%x (%lu bytes)",
                          heap, heap->size + gcmSIZEOF(gcsHEAP));
            gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, heap));
            heap = gcvNULL;
        }

        /* Acquire the mutex again. */
        gcmONERROR(
            gcoOS_AcquireMutex(gcvNULL, Heap->mutex, gcvINFINITE));
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
***** gcoHEAP API Code *********************************************************
*******************************************************************************/

/*******************************************************************************
**
**  gcoHEAP_Construct
**
**  Construct a new gcoHEAP object.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to a gcoOS object.
**
**      gctSIZE_T AllocationSize
**          Minimum size per arena.
**
**  OUTPUT:
**
**      gcoHEAP * Heap
**          Pointer to a variable that will hold the pointer to the gcoHEAP
**          object.
*/
gceSTATUS
gcoHEAP_Construct(
    IN gcoOS Os,
    IN gctSIZE_T AllocationSize,
    OUT gcoHEAP * Heap
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoHEAP_Destroy
**
**  Destroy a gcoHEAP object.
**
**  INPUT:
**
**      gcoHEAP Heap
**          Pointer to a gcoHEAP object to destroy.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHEAP_Destroy(
    IN gcoHEAP Heap
    )
{
    gcsHEAP_PTR heap;
#if gcdUSE_NEW_HEAP
    gcsBLOCK_PTR block;
#endif
#if gcmIS_DEBUG(gcdDEBUG_CODE)
    gctSIZE_T leaked = 0;
#endif

    gcmHEADER_ARG("Heap=0x%x", Heap);

#if gcmIS_DEBUG(gcdDEBUG_CODE)
    gcoHEAP_ProfileEnd(Heap, "HAL USER");
#endif

    for (heap = Heap->heap; heap != gcvNULL; heap = Heap->heap)
    {
        /* Unlink heap from linked list. */
        Heap->heap = heap->next;

#if gcmIS_DEBUG(gcdDEBUG_CODE)
        /* Check for leaked memory. */
        leaked += _DumpHeap(heap);
#endif

        /* Free the heap. */
        gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, heap));
        heap = gcvNULL;
    }

#if gcdUSE_NEW_HEAP
    for (block = Heap->blockList; block != gcvNULL; block = Heap->blockList)
    {
        /* Unlink block from linked list. */
        Heap->blockList = block->next;

        /* Free the block. */
        gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, block));
        block = gcvNULL;
    }
#endif

    /* Free the mutex. */
    gcmVERIFY_OK(gcoOS_DeleteMutex(gcvNULL, Heap->mutex));

    /* Free the heap structure. */
    gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, Heap));
    Heap = gcvNULL;

    /* Success. */
#if gcmIS_DEBUG(gcdDEBUG_CODE)
    gcmFOOTER_ARG("leaked=%lu", leaked);
#else
    gcmFOOTER_NO();
#endif
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHEAP_Allocate
**
**  Allocate data from the heap.
**
**  INPUT:
**
**      gcoHEAP Heap
**          Pointer to a gcoHEAP object.
**
**      IN gctSIZE_T Bytes
**          Number of byte to allocate.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the address of the allocated
**          memory.
*/
gceSTATUS
gcoHEAP_Allocate(
    IN gcoHEAP Heap,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gctBOOL acquired = gcvFALSE;
    gcsHEAP_PTR heap;
    gceSTATUS status;
    gctSIZE_T bytes;
    gcsNODE_PTR node, used, prevFree = gcvNULL;
    gctPOINTER memory = gcvNULL;

    gcmHEADER_ARG("Heap=0x%x Bytes=%lu", Heap, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

#if gcdUSE_NEW_HEAP
    /* Use buckets for requests of small sizes. */
    if (Bytes <= gcdBucketThreshold)
    {
        gctUINT bucketIdx;
        gcsSNODE_PTR snode;

        /* Find the bucket. */
        bytes = gcmALIGN(Bytes, 8);
        bucketIdx = bytes >> 3;

        /* Acquire the mutex. */
        gcmONERROR(
            gcoOS_AcquireMutex(gcvNULL, Heap->mutex, gcvINFINITE));

        acquired = gcvTRUE;

        if (Heap->buckets[bucketIdx] == gcvNULL)
        {
            gcsBLOCK_PTR block;
            gctUINT number = _InitNumber[bucketIdx];
            /*gctUINT nodeBytes = bytes + Heap->snodeOverhead * gcmSIZEOF(gctINT_PTR);*/
            gctUINT nodeBytes = bytes + gcmSIZEOF(gcsSNODE);
            gctUINT blockBytes = number * nodeBytes + gcmSIZEOF(gcsBLOCK);
            gctINT i;

            /* Add more free nodes to the bucket. */

            /* Release the mutex. */
            gcmONERROR(
                gcoOS_ReleaseMutex(gcvNULL, Heap->mutex));

            acquired = gcvFALSE;

            /* Allocate a new block. */
            gcmONERROR(
                gcoOS_AllocateMemory(gcvNULL,
                                     blockBytes,
                                     (gctPOINTER *) &memory));

            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                          "Allocated block 0x%x (%lu bytes)",
                          memory, blockBytes);

            /* Acquire the mutex. */
            gcmONERROR(
                gcoOS_AcquireMutex(gcvNULL, Heap->mutex, gcvINFINITE));

            acquired = gcvTRUE;

            /* Add new memory block to blockList. */
            block = memory;
            block->next = Heap->blockList;
            Heap->blockList = block;

            /* Create linked list. */
            snode = Heap->buckets[bucketIdx] = (gcsSNODE_PTR) (block + 1);
            for (i = number - 1; i > 0 ; i--)
            {
                snode->bytes = bytes;
                snode->next = (gcsSNODE_PTR) (((gctUINT8_PTR) snode) + nodeBytes);
                snode = snode->next;
            }
            snode->next = gcvNULL;

            Heap->nodesInBuckets[bucketIdx] += number;
        }

        /* Unlink the first snode. */
        snode = Heap->buckets[bucketIdx];
        Heap->buckets[bucketIdx] = snode->next;

        /* Release the mutex. */
        gcmVERIFY_OK(
            gcoOS_ReleaseMutex(gcvNULL, Heap->mutex));

        /* Return pointer to memory. */
        /**Memory = (gctINT_PTR) snode + Heap->snodeOverhead;*/
        *Memory = snode + 1;

        /* Success. */
        gcmFOOTER_ARG("*Memory=0x%x", *Memory);
        return gcvSTATUS_OK;
    }
#endif

    /* Determine number of bytes required for a node. */
    bytes = gcmALIGN(Bytes + gcmSIZEOF(gcsNODE), 8);
#if gcdDEBUG_HEAP_SIGNATURE
    bytes += 8; /* Add room for signature. */
#endif

    /* Acquire the mutex. */
    gcmONERROR(
            gcoOS_AcquireMutex(gcvNULL, Heap->mutex, gcvINFINITE));

    acquired = gcvTRUE;

    /* Check if this allocation is bigger than the default allocation size.
       Need to account for the sentinel as well, hence the gcmSIZEOF(gcsNODE). */
    if ((bytes + gcmSIZEOF(gcsHEAP) + gcmSIZEOF(gcsNODE)) >= Heap->allocationSize)
    {
        /* Check if increasing allocation size causes overflow */
        if (bytes < ((gcvMAXSIZE_T - gcmSIZEOF(gcsHEAP) - gcmSIZEOF(gcsNODE)) >> 1))
        {
            /* Increase allocation size. */
            Heap->allocationSize = (bytes << 1) + gcmSIZEOF(gcsHEAP) + gcmSIZEOF(gcsNODE);
        }
        else
        {
            /* Unable to increase allocation size due to overflow. */
            gcmASSERT(bytes + gcmSIZEOF(gcsHEAP) + gcmSIZEOF(gcsNODE) < Heap->allocationSize);
            status = gcvSTATUS_DATA_TOO_LARGE;
            goto OnError;
        }
    }
    else if (Heap->heap != gcvNULL)
    {
        gctINT i;

        /* 2 retries, since we might need to compact. */
        for (i = 0; i < 2; ++i)
        {
            /* Walk all the heaps. */
            for (heap = Heap->heap; heap != gcvNULL; heap = heap->next)
            {
                /* Check if this heap has enough bytes to hold the request. */
                if (bytes <= heap->size - gcmSIZEOF(gcsNODE))
                {
                    prevFree = gcvNULL;

                    /* Walk the chain of free nodes. */
                    for (node = heap->freeList;
                         node != gcvNULL;
                         node = node->next
                    )
                    {
                        gcmASSERT(node->next != gcdIN_USE);

                        /* Check if this free node has enough bytes. */
                        if (node->bytes >= bytes)
                        {
                            /* Use the node. */
                            goto UseNode;
                        }

                        /* Save current free node for linked list management. */
                        prevFree = node;
                    }
                }
            }

            if (i == 0)
            {
                /* Compact the heap. */
                gcmVERIFY_OK(_CompactHeap(Heap));
            }
        }
    }

    /* Allocate a new heap. */
    gcmONERROR(
        gcoOS_AllocateMemory(gcvNULL,
                             Heap->allocationSize,
                             (gctPOINTER *) &memory));

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Allocated heap 0x%x (%lu bytes)",
                  memory, Heap->allocationSize);

    /* Use the allocated memory as the heap. */
    heap = (gcsHEAP_PTR) memory;

    /* Insert this heap to the head of the chain. */
    heap->next = Heap->heap;
    heap->prev = gcvNULL;
    heap->size = Heap->allocationSize - gcmSIZEOF(gcsHEAP);

    if (heap->next != gcvNULL)
    {
        heap->next->prev = heap;
    }
    Heap->heap = heap;

    /* Mark the end of the heap. */
    node = (gcsNODE_PTR) ( (gctUINT8_PTR) heap
                         + Heap->allocationSize
                         - gcmSIZEOF(gcsNODE)
                         );
    node->bytes = 0;
    node->next  = gcvNULL;

    /* Create a free list. */
    node           = (gcsNODE_PTR) (heap + 1);
    heap->freeList = node;

    /* Initialize the free list. */
    node->bytes = heap->size - gcmSIZEOF(gcsNODE);
    node->next  = gcvNULL;

#if gcdDEBUG_HEAP
    node->unalignedBytes = 0;
#endif

    /* No previous free. */
    prevFree = gcvNULL;

#if VIVANTE_PROFILER_SYSTEM_MEMORY || gcmIS_DEBUG(gcdDEBUG_CODE)
    /* Update profiling. */
    Heap->heapCount  += 1;
    Heap->heapMemory += Heap->allocationSize;

    if (Heap->heapCount > Heap->heapCountMax)
    {
        Heap->heapCountMax = Heap->heapCount;
    }
    if (Heap->heapMemory > Heap->heapMemoryMax)
    {
        Heap->heapMemoryMax = Heap->heapMemory;
    }
#endif

UseNode:
    /* Verify some stuff. */
    gcmASSERT(heap != gcvNULL);
    gcmASSERT(node != gcvNULL);
    gcmASSERT(node->bytes >= bytes);

    if (heap->prev != gcvNULL)
    {
        /* Unlink the heap from the linked list. */
        heap->prev->next = heap->next;
        if (heap->next != gcvNULL)
        {
            heap->next->prev = heap->prev;
        }

        /* Move the heap to the front of the list. */
        heap->next       = Heap->heap;
        heap->prev       = gcvNULL;
        Heap->heap       = heap;
        heap->next->prev = heap;
    }

    /* Check if there is enough free space left after usage for another free
    ** node. */
    if (node->bytes > 2*gcmSIZEOF(gcsNODE) + bytes)
    {
        /* Allocate used space from the back of the free list. */
        used = (gcsNODE_PTR) ((gctUINT8_PTR) node + node->bytes - bytes);

        /* Adjust the number of free bytes. */
        node->bytes -= bytes;
    }
    else
    {
        /* Remove this free list from the chain. */
        if (prevFree == gcvNULL)
        {
            heap->freeList = node->next;
        }
        else
        {
            prevFree->next = node->next;
        }

        /* Consume the entire free node. */
        used  = (gcsNODE_PTR) node;
        bytes = node->bytes;
    }

    /* Mark node as used. */
    used->bytes = bytes;
    used->next  = gcdIN_USE;

#if gcmIS_DEBUG(gcdDEBUG_CODE)
    used->timeStamp      = ++Heap->timeStamp;
#endif

#if gcdDEBUG_HEAP
    /* Save the unaligned number of bytes. */
    used->unalignedBytes = Bytes;
#endif

#if gcdDEBUG_HEAP_SIGNATURE
    /* Create the signatures. */
    gcoOS_MemCopy(used->signature, _Signature, 8);

    gcoOS_MemCopy((gctUINT8_PTR) (used + 1) + Bytes, _Signature, 8);
#endif

#if VIVANTE_PROFILER_SYSTEM_MEMORY || gcmIS_DEBUG(gcdDEBUG_CODE)
    /* Update profile counters. */
    Heap->allocCount      += 1;
    Heap->allocBytes      += bytes;
    Heap->allocBytesMax    = gcmMAX(Heap->allocBytes, Heap->allocBytesMax);
    Heap->allocBytesTotal += bytes;
#endif

    /* Release the mutex. */
    gcmVERIFY_OK(
        gcoOS_ReleaseMutex(gcvNULL, Heap->mutex));

    /* Return pointer to memory. */
    *Memory = used + 1;

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
    if (acquired)
    {
        /* Release the mutex. */
        gcmVERIFY_OK(
            gcoOS_ReleaseMutex(gcvNULL, Heap->mutex));
    }

    if (memory != gcvNULL)
    {
        /* Free the heap memory. */
        gcoOS_FreeMemory(gcvNULL, memory);
        memory = gcvNULL;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHEAP_GetMemorySize
**
**  Get allocated memory size.
**
**  INPUT:
**
**      gcoHEAP Heap
**          Pointer to a gcoHEAP object.
**
**      gctPOINTER  Memory
**          Pointer to the memory
**          allocation.
**
**  OUTPUT:
**
**      gctPOINTER MemorySize
**          Pointer to a variable that will hold the pointer to the memory
**          size.
*/
gceSTATUS
gcoHEAP_GetMemorySize(
    IN gcoHEAP Heap,
    IN gctPOINTER Memory,
    OUT gctSIZE_T_PTR MemorySize
    )
{
    gcsNODE_PTR node;

    gcmHEADER_ARG("Heap=0x%x Memory=0x%x", Heap, Memory);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    /* Pointer to structure. */
    node = (gcsNODE_PTR) Memory - 1;

    *MemorySize = (gctSIZE_T)node->bytes;

    /* Return the status. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHEAP_Free
**
**  Free allocated memory from the heap.
**
**  INPUT:
**
**      gcoHEAP Heap
**          Pointer to a gcoHEAP object.
**
**      IN gctPOINTER Memory
**          Pointer to memory to free.
**
**  OUTPUT:
**
**      NOTHING.
*/
gceSTATUS
gcoHEAP_Free(
    IN gcoHEAP Heap,
    IN gctPOINTER Memory
    )
{
    gcsNODE_PTR node;
#if gcdUSE_NEW_HEAP
    gcsSNODE_PTR snode;
#endif
    gceSTATUS status;

    gcmHEADER_ARG("Heap=0x%x Memory=0x%x", Heap, Memory);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

#if gcdUSE_NEW_HEAP
    /* Pointer to structure. */
    /*snode = (gcsSNODE_PTR) ((gctINT_PTR) Memory - Heap->snodeOverhead);*/
    snode = (gcsSNODE_PTR) Memory - 1;

    /* Check if this is gcsSNODE. */
    if (snode->bytes <= gcdBucketThreshold)
    {
        gctUINT bucketIdx = snode->bytes >> 3;

        /* Acquire the mutex. */
        gcmONERROR(
            gcoOS_AcquireMutex(gcvNULL, Heap->mutex, gcvINFINITE));

        /* Put snode to free list of its bucket. */
        snode->next = Heap->buckets[bucketIdx];
        Heap->buckets[bucketIdx] = snode;

        /* Release the mutex. */
        gcmVERIFY_OK(
            gcoOS_ReleaseMutex(gcvNULL, Heap->mutex));

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
#endif

    /* Acquire the mutex. */
    gcmONERROR(
        gcoOS_AcquireMutex(gcvNULL, Heap->mutex, gcvINFINITE));

    /* Pointer to structure. */
    node = (gcsNODE_PTR) Memory - 1;

    /* Mark the node as freed. */
    node->next = gcvNULL;

#if gcdDEBUG_HEAP_SIGNATURE
    /* Validate the signature in front of the memory. */
    if (gcoOS_MemCmp(node->signature, _Signature, 8) != gcvSTATUS_OK)
    {
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
                      "%s(%d): Header modified in 0x%x",
                      __FUNCTION__, __LINE__, Memory);
    }

    /* Validate the signature just after the memory. */
    if (gcoOS_MemCmp((gctUINT8_PTR) (node + 1) + node->unalignedBytes,
                     _Signature,
                     8) != gcvSTATUS_OK)
    {
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HEAP,
                      "%s(%d): Footer modified in 0x%x",
                      __FUNCTION__, __LINE__, Memory);
    }
#endif

#if gcdDEBUG_HEAP_WRITE_AFTER_FREE
    /* Fill the memory. */
    gcoOS_MemFill(Memory, gcdDEBUG_FILLER, node->bytes - gcmSIZEOF(*node));
#endif

#if VIVANTE_PROFILER_SYSTEM_MEMORY || gcmIS_DEBUG(gcdDEBUG_CODE)
    /* Update profile counters. */
    Heap->allocBytes -= node->bytes;
#endif

    /* Release the mutex. */
    gcmVERIFY_OK(
        gcoOS_ReleaseMutex(gcvNULL, Heap->mutex));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if VIVANTE_PROFILER_SYSTEM_MEMORY || gcmIS_DEBUG(gcdDEBUG_CODE)
gceSTATUS
gcoHEAP_ProfileStart(
    IN gcoHEAP Heap
    )
{
    gcmHEADER_ARG("Heap=0x%x", Heap);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);

    /* Zero the counters. */
    Heap->allocCount      = 0;
    Heap->allocBytes      = 0;
    Heap->allocBytesMax   = 0;
    Heap->allocBytesTotal = 0;
    Heap->heapCount       = 0;
    Heap->heapCountMax    = 0;
    Heap->heapMemory      = 0;
    Heap->heapMemoryMax   = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoHEAP_ProfileEnd(
    IN gcoHEAP Heap,
    IN gctCONST_STRING Title
    )
{
    gcmHEADER_ARG("Heap=0x%x Title=0x%x", Heap, Title);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Heap, gcvOBJ_HEAP);
    gcmDEBUG_VERIFY_ARGUMENT(Title != gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "=====[ HEAP - %s ]=====", Title);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Number of allocations           : %12u",
                  Heap->allocCount);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Number of bytes allocated       : %12llu",
                  Heap->allocBytes);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Maximum allocation size         : %12u",
                  Heap->allocBytesMax);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Total number of bytes allocated : %12llu",
                  Heap->allocBytesTotal);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Number of heaps                 : %12u",
                  Heap->heapCount);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Heap memory in bytes            : %12llu",
                  Heap->heapMemory);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Maximum number of heaps         : %12u",
                  Heap->heapCountMax);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "Maximum heap memory in bytes    : %12llu",
                  Heap->heapMemoryMax);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HEAP,
                  "==============================================");

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
#endif /* VIVANTE_PROFILER_SYSTEM_MEMORY */

