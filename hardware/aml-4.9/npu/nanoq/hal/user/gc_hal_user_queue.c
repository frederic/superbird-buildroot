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
**  Architecture independent event queue management.
**
*/

#include "gc_hal_user_precomp.h"
#include "gc_hal_user_buffer.h"

#if (gcdENABLE_3D)
/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_BUFFER

gceSTATUS
gcoQUEUE_Construct(
    IN gcoOS Os,
    IN gceENGINE Engine,
    OUT gcoQUEUE * Queue
    )
{
    gcoQUEUE queue = gcvNULL;
    gceSTATUS status;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Queue != gcvNULL);

    /* Create the queue. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(struct _gcoQUEUE),
                       (gctPOINTER *)&queue));

    /* Initialize the object. */
    queue->object.type = gcvOBJ_QUEUE;

    /* Nothing in the queue yet. */
    queue->head = queue->tail = gcvNULL;

    queue->recordCount = 0;
    queue->maxUnlockBytes= 0;
    queue->tmpBufferRecordCount = 0;

    queue->chunks   = gcvNULL;
    queue->freeList = gcvNULL;

    queue->engine = Engine;

    /* Return gcoQUEUE pointer. */
    *Queue = queue;

    /* Success. */
    gcmFOOTER_ARG("*Queue=0x%x", *Queue);
    return gcvSTATUS_OK;

OnError:
    if (queue != gcvNULL)
    {
        /* Roll back. */
        gcoOS_Free(gcvNULL, queue);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoQUEUE_Destroy(
    IN gcoQUEUE Queue
    )
{
    gceSTATUS status;
    gcsQUEUE_CHUNK * chunk;

    gcmHEADER_ARG("Queue=0x%x", Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

    /* Commit the event queue. */
    gcmONERROR(gcoQUEUE_Commit(Queue, gcvTRUE));

    while (Queue->chunks != gcvNULL)
    {
        /* Unlink the first chunk. */
        chunk = Queue->chunks;
        Queue->chunks = chunk->next;
        chunk->next = gcvNULL;

        /* Free the memory. */
        gcoOS_FreeSharedMemory(gcvNULL, chunk);
    }

    /* Free the queue. */
    gcoOS_Free(gcvNULL, Queue);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoQUEUE_AppendEvent(
    IN gcoQUEUE Queue,
    IN gcsHAL_INTERFACE * Interface
    )
{
    gceSTATUS status;
    gcsQUEUE_PTR record = gcvNULL;
    gcsQUEUE_CHUNK * chunk;
    gctSIZE_T i;

    gcmHEADER_ARG("Queue=0x%x Interface=0x%x", Queue, Interface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);
    gcmVERIFY_ARGUMENT(Interface != gcvNULL);

    /* Check if we have records on the free list. */
    if (Queue->freeList == gcvNULL)
    {
        gcmONERROR(
            gcoOS_AllocateSharedMemory(gcvNULL,
                                       sizeof(*chunk),
                                       (gctPOINTER *)&chunk));

        /* Put it on the chunk list. */
        chunk->next   = Queue->chunks;
        Queue->chunks = chunk;

        /* Put the records on free list. */
        for (i = 0; i < gcmCOUNTOF(chunk->record); i++)
        {
            chunk->record[i].next = gcmPTR_TO_UINT64(Queue->freeList);
            Queue->freeList = &chunk->record[i];
        }
    }

    /* Allocate from the free list. */
    record          = Queue->freeList;
    Queue->freeList = gcmUINT64_TO_PTR(record->next);

    /* Initialize record. */
    record->next  = 0;
    gcoOS_MemCopy(&record->iface, Interface, gcmSIZEOF(record->iface));

    if (Queue->head == gcvNULL)
    {
        /* Initialize queue. */
        Queue->head = record;
    }
    else
    {
        /* Append record to end of queue. */
        Queue->tail->next = gcmPTR_TO_UINT64(record);
    }

    /* Mark end of queue. */
    Queue->tail = record;

    /* update count */
    Queue->recordCount++;

    if (Interface->command == gcvHAL_UNLOCK_VIDEO_MEMORY &&
        Interface->u.UnlockVideoMemory.asynchroneous &&
        Interface->u.UnlockVideoMemory.pool > gcvPOOL_UNKNOWN &&
        Interface->u.UnlockVideoMemory.pool <= gcvPOOL_SYSTEM
       )
    {
        if (Queue->maxUnlockBytes < Interface->u.UnlockVideoMemory.bytes)
        {
            Queue->maxUnlockBytes = (gctUINT)Interface->u.UnlockVideoMemory.bytes;
        }
    }

    if (Interface->command != gcvHAL_SIGNAL)
    {
        Queue->tmpBufferRecordCount++;
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
gcoQUEUE_Free(
    IN gcoQUEUE Queue
    )
{
    gcmHEADER_ARG("Queue=0x%x", Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

    /* Free any records in the queue. */
    if (Queue->tail)
    {
        Queue->tail->next = gcmPTR_TO_UINT64(Queue->freeList);
        Queue->freeList = Queue->head;

        Queue->head = Queue->tail = gcvNULL;
    }

    /* Update count */
    Queue->recordCount = 0;
    Queue->maxUnlockBytes = 0;
    Queue->tmpBufferRecordCount = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoQUEUE_Commit(
    IN gcoQUEUE Queue,
    IN gctBOOL Stall
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;
    gcmHEADER_ARG("Queue=0x%x", Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

    if (Queue->head != gcvNULL)
    {
        /* Initialize event commit command. */
        iface.ignoreTLS     = gcvFALSE;
        iface.command       = gcvHAL_EVENT_COMMIT;
        iface.commitMutex   = gcvFALSE;
        iface.engine        = Queue->engine;
        iface.u.Event.queue = gcmPTR_TO_UINT64(Queue->head);

        /* Send command to kernel. */
        gcmONERROR(
            gcoOS_DeviceControl(gcvNULL,
                                IOCTL_GCHAL_INTERFACE,
                                &iface, gcmSIZEOF(iface),
                                &iface, gcmSIZEOF(iface)));

        /* Test for error. */
        gcmONERROR(iface.status);

        /* Free any records in the queue. */
        gcmONERROR(gcoQUEUE_Free(Queue));

        /* Wait for the execution to complete. */
        if (Stall)
        {
            gcmONERROR(gcoHARDWARE_Stall(gcvNULL));
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
#endif  /*  (gcdENABLE_3D || gcdENABLE_2D) */
