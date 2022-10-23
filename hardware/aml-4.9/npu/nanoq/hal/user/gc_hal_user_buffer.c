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
**  gcoBUFFER object for user HAL layers.
**
*/

#include "gc_hal_user_precomp.h"
#include "gc_hal_user_buffer.h"

#if (gcdENABLE_3D)

#define _GC_OBJ_ZONE            gcdZONE_BUFFER

#define gcdMAX_TEMPCMD_BUFFER_SIZE      0x20000

#define gcdPATCH_LIST_SIZE              1024

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

typedef struct _gcsCOMMAND_INFO * gcsCOMMAND_INFO_PTR;

struct _gcsCOMMAND_INFO
{
    gctUINT32                   alignment;
    gctUINT32                   reservedHead;
    gctUINT32                   reservedTail;
    gctUINT32                   reservedUser;
    gceENGINE                   engine;
};

struct _gcoBUFFER
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to the required objects. */
    gcoHAL                      hal;

    /* Pointer to the hardware objects. */
    gcoHARDWARE                 hardware;

    /* Flag to indicate thread default or not */
    gctBOOL                     threadDefault;

    /* Requested command buffer size. */
    gctSIZE_T                   bytes;

    /* Number of command buffers. */
    gctSIZE_T                   count;
    gctSIZE_T                   maxCount;

    /* List of command buffers. */
    gcoCMDBUF                   commandBufferList;
    gcoCMDBUF                   commandBufferTail;

    /* Reserved bytes. */
    struct _gcsCOMMAND_INFO     info;

    gctUINT32                   totalReserved;

    /* command-location head and tail. */
    gcsHAL_COMMAND_LOCATION     commandLocationHead;
    gcsHAL_COMMAND_LOCATION *   commandLocationTail;

    /* sub-commit head and tail. */
    gcsHAL_SUBCOMMIT            subCommitHead;
    gcsHAL_SUBCOMMIT *          subCommitTail;

    /* command-location and sub-commit repo. */
    gcsHAL_COMMAND_LOCATION *   freeCommandLocations;
    gcsHAL_SUBCOMMIT *          freeSubCommits;

    /* Patch lists. */
    gcsHAL_PATCH_LIST *         patchLists[gcvHAL_PATCH_TYPE_COUNT];
    gcsHAL_PATCH_LIST *         tempPatchLists[gcvHAL_PATCH_TYPE_COUNT];
    gcsHAL_PATCH_LIST *         freePatchLists[gcvHAL_PATCH_TYPE_COUNT];

    /* Current MCFE channel selection. */
    gctBOOL                     mcfePriority;
    gctUINT32                   mcfeChannelId;

    struct _gcsTEMPCMDBUF       tempCMDBUF;

#if gcdENABLE_3D
    gctBOOL                     tfbPaused;
    gctUINT32                   tfbResumeBytes;
    gctBOOL                     queryPaused[gcvQUERY_MAX_NUM];
    gctUINT32                   queryResumeBytes[gcvQUERY_MAX_NUM];
    gctBOOL                     probePaused;
    gctUINT32                   probeResumeBytes;
#endif

    gctBOOL                     inRerserved;

    /* Commit stamp of each commit. */
    gctUINT64                   commitStamp;

    gctUINT32                   mirrorCount;

    gcsFENCE_LIST_PTR           fenceList;

    /* capture slice of states in the command buffer */
    gctBOOL                     captureEnabled;
    gctBOOL                     dropCommandEnabled;
    gctUINT8_PTR                captureBuffer;
    gctUINT32                   captureBufferTotalSize;
    gctINT32                    captureRemainedSize;
    gctUINT32                   captureCommandOffset;

    struct
    {
        gctUINT32               hasFence                    : 1;
        gctUINT32               hasHWFence                  : 1;
        gctUINT32               hasPipe2D                   : 1;
        gctUINT32               hasPipe3D                   : 1;
        gctUINT32               hasNewMMU                   : 1;
        gctUINT32               hasHWTFB                    : 1;
        gctUINT32               hasProbe                    : 1;
        gctUINT32               hasComputeOnly              : 1;
        gctUINT32               hasMCFE                     : 1;

    }hwFeature;
};

static gctUINT32 _PatchItemSize[] =
{
    0,
    (gctUINT32)sizeof(gcsHAL_PATCH_VIDMEM_ADDRESS),
    (gctUINT32)sizeof(gcsHAL_PATCH_MCFE_SEMAPHORE),
    (gctUINT32)sizeof(gcsHAL_PATCH_VIDMEM_TIMESTAMP),
};

/******************************************************************************\
***************************** gcoCMDBUF Object Code *****************************
\******************************************************************************/

static void
_DoFreeCommandBufferMemory(
    IN gcoHARDWARE Hardware,
    IN gcsCOMMAND_INFO_PTR Info,
    IN gcoCMDBUF CommandBuffer
    )
{
    /* Unlock command buffer video memory. */
    gcmVERIFY_OK(gcoHAL_UnlockVideoMemory(
            CommandBuffer->videoMemNode,
            gcvVIDMEM_TYPE_COMMAND,
            Info->engine
            ));

    /* Free video memory. */
    gcmVERIFY_OK(gcoHAL_ReleaseVideoMemory(CommandBuffer->videoMemNode));
}

static void
_FreeCommandBuffer(
    IN gcoHARDWARE Hardware,
    IN gcsCOMMAND_INFO_PTR Info,
    IN gcoCMDBUF CommandBuffer
    )
{
    if (CommandBuffer != gcvNULL)
    {
        if (CommandBuffer->logical != 0)
        {
            /* Can not handle error case when free. */
            _DoFreeCommandBufferMemory(Hardware, Info, CommandBuffer);

            /* Reset the buffer pointer. */
            CommandBuffer->logical = 0;
        }
    }
}

/*******************************************************************************
**
**  _DestroyCommandBuffer
**
**  Destroy a gcoCMDBUF object.
**
**  INPUT:
**
**      gcoCMDBUF CommandBuffer
**          Pointer to an gcoCMDBUF object.
**
**  OUTPUT:
**
**      None.
*/
static void
_DestroyCommandBuffer(
    IN gcoHARDWARE Hardware,
    IN gcsCOMMAND_INFO_PTR Info,
    IN gcoCMDBUF CommandBuffer
    )
{
    gcmHEADER_ARG("CommandBuffer=0x%x", CommandBuffer);

    /* Destroy command buffer allocations. */
    _FreeCommandBuffer(Hardware, Info, CommandBuffer);

    /* Destroy signal. */
    if (CommandBuffer->signal != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, CommandBuffer->signal));
        CommandBuffer->signal = gcvNULL;
    }

    /* Free the gcoCMDBUF object. */
    gcoOS_Free(gcvNULL, CommandBuffer);

    /* Success. */
    gcmFOOTER_NO();
}

/*******************************************************************************
**
**  _ConstructCommandBuffer
**
**  Construct a new gcoCMDBUF object.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to a gcoOS object.
**
**      gcoHARDWARE Hardware
**          Pointer to a gcoHARDWARE object.
**
**      gctSIZE_T Bytes
**          Number of bytes for the buffer.
**
**      gcsCOMMAND_BUFFER_PTR Info
**          Alignment and head/tail information.
**
**  OUTPUT:
**
**      gcoCMDBUF * CommandBuffer
**          Pointer to a variable that will hold the the gcoCMDBUF object
**          pointer.
*/
static gceSTATUS
_ConstructCommandBuffer(
    IN gcoOS Os,
    IN gcoHARDWARE Hardware,
    IN gctSIZE_T Bytes,
    IN gcsCOMMAND_INFO_PTR Info,
    OUT gcoCMDBUF * CommandBuffer
    )
{
    gceSTATUS status;
    gcoCMDBUF commandBuffer = gcvNULL;
    gctSIZE_T objectSize    = 0;
    gctPOINTER pointer      = gcvNULL;

    gcmHEADER_ARG("Bytes=%lu Info=0x%x", Bytes, Info);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    /* Set the size of the object. */
    objectSize = gcmSIZEOF(struct _gcoCMDBUF);

    /* Allocate the gcoCMDBUF object. */
    gcmONERROR(gcoOS_Allocate(gcvNULL, objectSize, &pointer));
    commandBuffer = pointer;

    /* Reset the command buffer object. */
    gcoOS_ZeroMemory(commandBuffer, objectSize);

    /* Initialize the gcoCMDBUF object. */
    commandBuffer->object.type = gcvOBJ_COMMANDBUFFER;

    /* Create the signal. */
    gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &commandBuffer->signal));

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SIGNAL,
                  "%s(%d): buffer signal created 0x%08X",
                  __FUNCTION__, __LINE__, commandBuffer->signal);

    /* Mark the buffer as available. */
    gcmONERROR(gcoOS_Signal(gcvNULL, commandBuffer->signal, gcvTRUE));

    /* Try to allocate the command buffer space. */
    while (gcvTRUE)
    {
        gctUINT32 handle;
        gcsHAL_INTERFACE iface;

        iface.ignoreTLS = gcvFALSE;
        iface.engine  = Info->engine;
        iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        iface.u.AllocateLinearVideoMemory.alignment = Info->alignment;
        iface.u.AllocateLinearVideoMemory.type = gcvVIDMEM_TYPE_COMMAND;
        iface.u.AllocateLinearVideoMemory.pool = gcvPOOL_DEFAULT;
        iface.u.AllocateLinearVideoMemory.flag = 0;
        iface.u.AllocateLinearVideoMemory.bytes = (gctUINT32)Bytes;

        status = gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            );

        if (status == gcvSTATUS_OUT_OF_MEMORY)
        {
            goto retry;
        }

        gcmONERROR(status);

        handle  = iface.u.AllocateLinearVideoMemory.node;

        /* Lock video memory for both CPU and GPU address. */
        iface.ignoreTLS = gcvFALSE;
        iface.engine  = Info->engine;
        iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
        iface.u.LockVideoMemory.node = handle;
        iface.u.LockVideoMemory.cacheable = gcvFALSE;

        status = gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            );

        if (status == gcvSTATUS_OUT_OF_MEMORY)
        {
            goto retry;
        }

        gcmONERROR(status);

        commandBuffer->videoMemNode = handle;
        commandBuffer->address = iface.u.LockVideoMemory.address;
        commandBuffer->logical = iface.u.LockVideoMemory.memory;
        commandBuffer->bytes   = (gctUINT32)Bytes;

        /* Initialize command buffer. */
        commandBuffer->free = commandBuffer->bytes;

        /* The command buffer is successfully allocated. */
        break;

retry:
        if (Bytes <= (4 << 10))
        {
            gcmONERROR(status);
        }

        /* Free the command buffer objects. */
        _FreeCommandBuffer(Hardware, Info, commandBuffer);

        /* Try lower size. */
        Bytes >>= 1;
    }

    commandBuffer->reservedTail = Info->reservedTail;
    commandBuffer->reservedHead = Info->reservedHead;

    /* Return pointer to the gcoCMDBUF object. */
    *CommandBuffer = commandBuffer;

    /* Success. */
    gcmFOOTER_ARG("*CommandBuffer=0x%x", *CommandBuffer);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    if (commandBuffer)
    {
        _DestroyCommandBuffer(Hardware, Info, commandBuffer);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_DuplicateCommandBuffer(
    gcoCMDBUF Dest,
    gcoCMDBUF Src
    )
{
    gctUINT size;
    gctUINT offset;

    /* Do not copy reserved head and tail. */
    size   = Src->offset - Src->startOffset - Src->reservedHead;
    offset = Src->startOffset + Src->reservedHead;

    Dest->startOffset = Src->startOffset;
    Dest->offset      = Src->offset;

    gcoOS_MemCopy(
        gcmUINT64_TO_PTR(Dest->logical + offset),
        gcmUINT64_TO_PTR(Src->logical  + offset),
        size
        );

    return gcvSTATUS_OK;
}

/******************************************************************************/

static gceSTATUS
_ConstructMirrorCommandBuffer(
    IN gcoHARDWARE Hardware,
    IN gcoBUFFER Buffer,
    IN gcoCMDBUF CommandBuffer
    )
{
    gceSTATUS status;
    gctUINT32 i;

    gcmHEADER();

    /* Get mirror count from gcoBUFFER. */
    CommandBuffer->mirrorCount = Buffer->mirrorCount;

    if (CommandBuffer->mirrorCount)
    {
        /* Allocate mirror of command buffer. */
        gcmONERROR(gcoOS_Allocate(
            gcvNULL,
            gcmSIZEOF(gcoCMDBUF *) * CommandBuffer->mirrorCount,
            (gctPOINTER *)&CommandBuffer->mirrors
            ));

        for (i = 0; i < CommandBuffer->mirrorCount; i++)
        {
            gcmONERROR(_ConstructCommandBuffer(
                gcvNULL,
                Buffer->hardware,
                CommandBuffer->bytes,
                (gcsCOMMAND_INFO_PTR) &Buffer->info,
                &CommandBuffer->mirrors[i]
                ));

            if (CommandBuffer->bytes != CommandBuffer->mirrors[i]->bytes)
            {
                gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
            }
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* _ConstructCommandBuffer() will handle error and release the whole gcoBUFFER. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_GetCommandBuffer(
    IN gcoBUFFER Buffer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoCMDBUF commandBuffer = gcvNULL;
    gcePIPE_SELECT entryPipe;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    /* Determine the next command buffer. */
    if (Buffer->commandBufferTail == gcvNULL)
    {
        /* Select 3D pipe for the first buffer. */
        entryPipe = gcvPIPE_3D;

        /* Get the head of the list. */
        Buffer->commandBufferTail =
        commandBuffer = Buffer->commandBufferList;
    }
    else
    {
        /* Get current entry pipe. */
        entryPipe = Buffer->commandBufferTail->entryPipe;

        /* Get the next command buffer. */
        Buffer->commandBufferTail =
        commandBuffer = Buffer->commandBufferTail->next;
    }

    if (commandBuffer != gcvNULL)
    {
        /* Test if command buffer is available. */
        status = gcoOS_WaitSignal(gcvNULL, commandBuffer->signal, 0);
    }

    /* Not available? */
    if ((status == gcvSTATUS_TIMEOUT) || (commandBuffer == gcvNULL))
    {
        /* Construct new command buffer. */
        if ((Buffer->maxCount == 0) || (Buffer->count < Buffer->maxCount))
        {
            gcoCMDBUF temp = gcvNULL;
            gcmONERROR(_ConstructCommandBuffer(gcvNULL, Buffer->hardware,
                                               Buffer->bytes,
                                               (gcsCOMMAND_INFO_PTR) &Buffer->info,
                                               &temp));

            /* Insert into the list. */
            if (commandBuffer == gcvNULL)
            {
                Buffer->commandBufferList =
                temp->next = temp->prev = temp;
            }
            else
            {
                temp->prev = commandBuffer->prev;
                temp->next = commandBuffer;
                commandBuffer->prev->next = temp;
                commandBuffer->prev = temp;
            }

            Buffer->commandBufferTail = temp;

            Buffer->count += 1;
            gcmTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                          "Using %lu command buffers.",
                          Buffer->count);

            commandBuffer = temp;

            gcmONERROR(_ConstructMirrorCommandBuffer(
                Buffer->hardware,
                Buffer,
                commandBuffer
                ));
        }

        if (commandBuffer == gcvNULL)
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        /* Wait for buffer to become available. */
        gcmONERROR(gcoOS_WaitSignal(gcvNULL, commandBuffer->signal, gcvINFINITE));
    }
    else
    {
        gcmONERROR(status);
    }

    /* Set the entry pipe. */
    commandBuffer->entryPipe = entryPipe;

    /* Reset command buffer. */
    commandBuffer->startOffset = 0;
    commandBuffer->offset      = Buffer->info.reservedHead;
    commandBuffer->free        = commandBuffer->bytes - Buffer->totalReserved;

    /* Succees. */
    gcmFOOTER_ARG("commandBuffer=0x%x", commandBuffer);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


static void
_DestroyMirrorCommandBuffer(
    IN gcoBUFFER Buffer,
    IN gcoCMDBUF CommandBuffer
    )
{
    if (CommandBuffer->mirrors)
    {
        gctUINT32 i;
        gctUINT32 mirrorCount = CommandBuffer->mirrorCount;

        for (i = 0; i < mirrorCount; i++)
        {
            if (CommandBuffer->mirrors[i])
            {
                _DestroyCommandBuffer(
                    Buffer->hardware, &Buffer->info, CommandBuffer->mirrors[i]);
            }
        }

        gcoOS_Free(gcvNULL, CommandBuffer->mirrors);
    }
}

static void
_FinishCommandBufferRange(
    gcoBUFFER Buffer,
    gcoCMDBUF CommandBuffer
    )
{
    gctUINT32 newOffset;
    gctSIZE_T spaceLeft;

    /* Advance the offset for next commit. */
    newOffset = CommandBuffer->offset + Buffer->info.reservedTail;

    if (Buffer->hwFeature.hasMCFE)
    {
        newOffset = gcmALIGN(newOffset, 16);
    }

    /* Compute the size of the space left in the buffer. */
    spaceLeft = CommandBuffer->bytes - newOffset;

    if (spaceLeft > Buffer->totalReserved)
    {
        /* Adjust buffer offset and size. */
        CommandBuffer->startOffset = newOffset;
        CommandBuffer->offset      = CommandBuffer->startOffset + Buffer->info.reservedHead;
        CommandBuffer->free        = CommandBuffer->bytes - CommandBuffer->offset
                                   - Buffer->info.alignment - Buffer->info.reservedTail - Buffer->info.reservedUser;
    }
    else
    {
        /* Buffer is full. */
        CommandBuffer->startOffset = CommandBuffer->bytes;
        CommandBuffer->offset      = CommandBuffer->bytes;
        CommandBuffer->free        = 0;
    }

    if (Buffer->captureEnabled)
    {
        Buffer->captureCommandOffset = CommandBuffer->offset;
    }

    /* The exit pipe becomes the entry pipe for the next command buffer. */
    CommandBuffer->entryPipe = CommandBuffer->exitPipe;

    /* Reset usage flags. */
    CommandBuffer->using2D   = gcvFALSE;
    CommandBuffer->using3D   = gcvFALSE;
}

/******************************************************************************/

static gcsHAL_PATCH_LIST *
_AllocPatchList(
    IN gcoBUFFER Buffer,
    IN gctUINT32 PatchType,
    IN gcsHAL_PATCH_LIST ** FreePatchList,
    IN gctSIZE_T ItemSize
    )
{
    gcsHAL_PATCH_LIST * patchList = gcvNULL;

    if (*FreePatchList)
    {
        patchList = *FreePatchList;
        *FreePatchList = gcmUINT64_TO_PTR(patchList->next);

        patchList->count = 0;
        patchList->next  = 0;
    }

    if (!patchList)
    {
        void * patchArray = gcvNULL;

        /* Allocate the patchList item array. */
        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       ItemSize * gcdPATCH_LIST_SIZE,
                                       &patchArray)))
        {
            return gcvNULL;
        }

        gcoOS_ZeroMemory(patchArray, ItemSize * gcdPATCH_LIST_SIZE);

        /* Allocate the patchList structure. */
        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       sizeof(gcsHAL_PATCH_LIST),
                                       (gctPOINTER *)&patchList)))
        {
            gcmOS_SAFE_FREE(gcvNULL, patchArray);
            return gcvNULL;
        }

        /* Capacity is gcdPATCH_LIST_SIZE, and initial valid item is 0. */
        patchList->type       = PatchType;
        patchList->count      = 0;
        patchList->patchArray = gcmPTR_TO_UINT64(patchArray);
        patchList->next       = 0;
    }

    return patchList;
}

static gcsHAL_PATCH_LIST *
_GetPatchItem(
    IN gcoBUFFER Buffer,
    IN gctUINT32 PatchType,
    IN gctBOOL UseTemp
    )
{
    gcsHAL_PATCH_LIST ** patchList;

    patchList = UseTemp ? &Buffer->tempPatchLists[PatchType]
              : &Buffer->patchLists[PatchType];

    if (!*patchList || (* patchList)->count == gcdPATCH_LIST_SIZE)
    {
        /* Get new patchList list */
        gcsHAL_PATCH_LIST * _new;

        _new = _AllocPatchList(Buffer,
                               PatchType,
                               &Buffer->freePatchLists[PatchType],
                               _PatchItemSize[PatchType]);

        _new->next = gcmPTR_TO_UINT64(*patchList);

        *patchList = _new;
    }

    return *patchList;
}

static void
_DestroyPatchLists(
    IN gcsHAL_PATCH_LIST * PatchLists
    )
{
    gcsHAL_PATCH_LIST * patchList = PatchLists;

    while (patchList)
    {
        gcsHAL_PATCH_LIST * node = patchList;
        patchList = gcmUINT64_TO_PTR(patchList->next);

        gcoOS_FreeMemory(gcvNULL, gcmUINT64_TO_PTR(node->patchArray));
        gcoOS_FreeMemory(gcvNULL, node);
    }
}

/* Recycle CommandLocation referenced patchLists, ie ->patchHead. */
static void
_RecyclePatchList(
    IN gcoBUFFER Buffer,
    IN gcsHAL_COMMAND_LOCATION * CommandLocation
    )
{
    gcsHAL_PATCH_LIST * patchList;

    patchList = gcmUINT64_TO_PTR(CommandLocation->patchHead);
    CommandLocation->patchHead = 0;

    while (patchList)
    {
        gcsHAL_PATCH_LIST ** freePatch;
        gcsHAL_PATCH_LIST * next = gcmUINT64_TO_PTR(patchList->next);

        /* Free the patchList item. */
        patchList->count = 0;

        freePatch = &Buffer->freePatchLists[patchList->type];

        /* Put back the patchList to free list. */
        patchList->next = gcmPTR_TO_UINT64(*freePatch);
        *freePatch  = patchList;

        patchList = next;
    }
}

static gcmINLINE void
_CorrectVidmemAddressPatch(
    IN gcoCMDBUF CommandBuffer,
    IN gcsHAL_PATCH_VIDMEM_ADDRESS * PatchItem,
    IN gctUINT32 Count
    )
{
    gctUINT32 i;

    for (i = 0; i < Count; i++)
    {
        PatchItem[i].location += CommandBuffer->lastOffset;
    }
}

static gcmINLINE void
_CorrectMCFESemaphorePatch(
    IN gcoCMDBUF CommandBuffer,
    IN gcsHAL_PATCH_VIDMEM_ADDRESS * PatchItem,
    IN gctUINT32 Count
    )
{
    gctUINT32 i;

    for (i = 0; i < Count; i++)
    {
        PatchItem[i].location += CommandBuffer->lastOffset;
    }
}

static gceSTATUS
_MoveTempPatchListSingle(
    IN gcoBUFFER Buffer,
    IN gcsHAL_PATCH_LIST * PatchList
    )
{
    gcoCMDBUF commandBuffer = Buffer->commandBufferTail;
    gcsHAL_PATCH_LIST * dstList;
    gctPOINTER dstItem;
    gctPOINTER srcItem;
    gctUINT32 itemSize = _PatchItemSize[PatchList->type];

    gctUINT32 index = 0;

    while (index < PatchList->count)
    {
        gctUINT32 count;

        dstList = _GetPatchItem(Buffer, PatchList->type, gcvFALSE);

        /* Determine the copy batch. */
        count = gcmMIN((gcdPATCH_LIST_SIZE - dstList->count),
                       (PatchList->count - index));

        dstItem = gcmUINT64_TO_PTR(dstList->patchArray + dstList->count * itemSize);

        srcItem = gcmUINT64_TO_PTR(PatchList->patchArray + index * itemSize);

        /* Copy to dest. */
        gcoOS_MemCopy(dstItem, srcItem, itemSize * count);

        /* Check if need correct patch location in gcoCMDBUF object. */
        if (PatchList->type == gcvHAL_PATCH_VIDMEM_ADDRESS)
        {
            _CorrectVidmemAddressPatch(commandBuffer, dstItem, count);
        }
        else if (PatchList->type == gcvHAL_PATCH_MCFE_SEMAPHORE)
        {
            _CorrectMCFESemaphorePatch(commandBuffer, dstItem, count);
        }

        dstList->count += count;
        index += count;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_MoveTempPatchLists(
    IN gcoBUFFER Buffer
    )
{
    gctUINT32 i;

    for (i = gcvHAL_PATCH_VIDMEM_ADDRESS; i < gcvHAL_PATCH_TYPE_COUNT; i++)
    {
        gcsHAL_PATCH_LIST * patchList = Buffer->tempPatchLists[i];

        while (patchList)
        {
            gcsHAL_PATCH_LIST * next = gcmUINT64_TO_PTR(patchList->next);

            /* Move one patch list. */
            _MoveTempPatchListSingle(Buffer, patchList);

            /* Recycle it. */
            patchList->next = gcmPTR_TO_UINT64(Buffer->freePatchLists[i]);
            Buffer->freePatchLists[i] = patchList;

            /* One patch list is recycleed, advance to next. */
            Buffer->tempPatchLists[i] = patchList = next;
        }
    }

    return gcvSTATUS_OK;
}

/******************************************************************************/

static gcsHAL_COMMAND_LOCATION *
_GetCommandLocation(
    IN gcoBUFFER Buffer
    )
{
    gcsHAL_COMMAND_LOCATION * cmdLoc;

    if (Buffer->freeCommandLocations)
    {
        cmdLoc = Buffer->freeCommandLocations;
        Buffer->freeCommandLocations = gcmUINT64_TO_PTR(cmdLoc->next);
    }
    else
    {
        /* Allocate. */
        if (gcmIS_ERROR(gcoOS_AllocateSharedMemory(gcvNULL,
                                                   sizeof(gcsHAL_COMMAND_LOCATION),
                                                   (gctPOINTER *)&cmdLoc)))
        {
            return gcvNULL;
        }
    }

    return cmdLoc;
}

static gcsHAL_SUBCOMMIT *
_GetSubCommit(
    IN gcoBUFFER Buffer
    )
{
    gcsHAL_SUBCOMMIT * subCommit;

    if (Buffer->freeSubCommits)
    {
        subCommit = Buffer->freeSubCommits;
        Buffer->freeSubCommits = gcmUINT64_TO_PTR(subCommit->next);
    }
    else
    {
        /* Allocate. */
        if (gcmIS_ERROR(gcoOS_AllocateSharedMemory(gcvNULL,
                                                   sizeof(gcsHAL_SUBCOMMIT),
                                                   (gctPOINTER *)&subCommit)))
        {
            return gcvNULL;
        }
    }

    return subCommit;
}

/*
 * Recycle SubCommit referenced CommandLocations, ie ->commandBuffer.next. */
static void
_RecycleCommandLocations(
    gcoBUFFER Buffer,
    gcsHAL_SUBCOMMIT * SubCommit
    )
{
    gcsHAL_COMMAND_LOCATION * cmdLoc = gcmUINT64_TO_PTR(SubCommit->commandBuffer.next);
    SubCommit->commandBuffer.next = 0;

    /* Recycle patch list referenced by nested command-location struct. */
    _RecyclePatchList(Buffer, &SubCommit->commandBuffer);

    while (cmdLoc)
    {
        gcsHAL_COMMAND_LOCATION * next = gcmUINT64_TO_PTR(cmdLoc->next);

        /* Recycle contained patch list. */
        _RecyclePatchList(Buffer, cmdLoc);

        /* Recycle it. */
        cmdLoc->next = gcmPTR_TO_UINT64(Buffer->freeCommandLocations);
        Buffer->freeCommandLocations = cmdLoc;

        cmdLoc = next;
    }
}

/* Recycle Buffer referenced subCommits, ie ->subCommitHead.next. */
static void
_RecycleSubCommits(
    gcoBUFFER Buffer
    )
{
    gcsHAL_SUBCOMMIT * subCommit = gcmUINT64_TO_PTR(Buffer->subCommitHead.next);
    Buffer->subCommitHead.next = 0;

    /* Recycle command-location referenced by nested sub-commit struct. */
    _RecycleCommandLocations(Buffer, &Buffer->subCommitHead);

    while (subCommit)
    {
        gcsHAL_SUBCOMMIT * next = gcmUINT64_TO_PTR(subCommit->next);

        /* Recycle contained command buffer locations. */
        _RecycleCommandLocations(Buffer, subCommit);

        /* Recycle it. */
        subCommit->next = gcmPTR_TO_UINT64(Buffer->freeSubCommits);
        Buffer->freeSubCommits = subCommit;

        subCommit = next;
    }
}

static void
_DestroyCommandLocations(
    gcsHAL_COMMAND_LOCATION * CommandLocations
    )
{
    gcsHAL_COMMAND_LOCATION * cmdLoc = CommandLocations;

    while (cmdLoc)
    {
        gcsHAL_COMMAND_LOCATION * next = gcmUINT64_TO_PTR(cmdLoc->next);

        /* Free memory. */
        gcmOS_SAFE_FREE_SHARED_MEMORY(gcvNULL, cmdLoc);

        cmdLoc = next;
    }
}

static void
_DestroySubCommits(
    gcsHAL_SUBCOMMIT * SubCommits
    )
{
    gcsHAL_SUBCOMMIT * subCommit = SubCommits;

    while (subCommit)
    {
        gcsHAL_SUBCOMMIT * next = gcmUINT64_TO_PTR(subCommit->next);

        /* Free memory. */
        gcmOS_SAFE_FREE_SHARED_MEMORY(gcvNULL, subCommit);

        subCommit = next;
    }
}


/* Install patchLists to patchHead of command-location. */
static void
_InstallPatchLists(
    gcoBUFFER Buffer,
    gcsHAL_COMMAND_LOCATION * CommandLocation
    )
{
    gctUINT i;

    CommandLocation->patchHead = 0;

    for (i = gcvHAL_PATCH_VIDMEM_ADDRESS; i < gcvHAL_PATCH_TYPE_COUNT; i++)
    {
        gcsHAL_PATCH_LIST * patchList = Buffer->patchLists[i];

        if (!patchList)
        {
            continue;
        }

        while (patchList->next)
        {
            patchList = gcmUINT64_TO_PTR(patchList->next);
        }

        patchList->next = CommandLocation->patchHead;

        CommandLocation->patchHead = gcmPTR_TO_UINT64(Buffer->patchLists[i]);
        Buffer->patchLists[i] = gcvNULL;
    }
}

/*
 * Link cmdLoc to Buffer->commandLocationTail, with its head is
 * Buffer->commandLocationHead.
 */
static gceSTATUS
_LinkCommandLocation(
    IN gcoBUFFER Buffer,
    IN gcoCMDBUF CommandBuffer,
    IN gctUINT32 ExitIndex
    )
{
    gceSTATUS status;
    gcsHAL_COMMAND_LOCATION * cmdLoc;
    gcoCMDBUF cmdBuf = CommandBuffer;

    gcmHEADER_ARG("Buffer=%p CommandBuffer=%p ExitIndex=%d",
                  Buffer, CommandBuffer, ExitIndex);

    if ((cmdBuf == gcvNULL) ||
        (cmdBuf->offset - cmdBuf->startOffset <= cmdBuf->reservedHead))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if (Buffer->commandLocationTail)
    {
        cmdLoc = _GetCommandLocation(Buffer);
        Buffer->commandLocationTail->next = gcmPTR_TO_UINT64(cmdLoc);
        Buffer->commandLocationTail = cmdLoc;

        if (!cmdLoc)
        {
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }
    }
    else
    {
        cmdLoc = Buffer->commandLocationTail = &Buffer->commandLocationHead;
    }

    cmdLoc->priority     = (gctUINT16)Buffer->mcfePriority;
    cmdLoc->channelId    = (gctUINT16)Buffer->mcfeChannelId;

    cmdLoc->videoMemNode = cmdBuf->videoMemNode;
    cmdLoc->address      = cmdBuf->address;
    cmdLoc->logical      = cmdBuf->logical;

    cmdLoc->startOffset  = cmdBuf->startOffset;
    cmdLoc->size         = cmdBuf->offset - cmdBuf->startOffset
                                          + cmdBuf->reservedTail;

    /* Define the command range. */
    cmdLoc->reservedHead = cmdBuf->reservedHead;
    cmdLoc->reservedTail = cmdBuf->reservedTail;

    _InstallPatchLists(Buffer, cmdLoc);

    cmdLoc->entryPipe    = cmdBuf->entryPipe;
    cmdLoc->exitPipe     = cmdBuf->exitPipe;
    cmdLoc->exitIndex    = ExitIndex;

    cmdLoc->next         = 0;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*
 * Link sub-commit to Buffer->subCommitTail, with its head is
 * Buffer->subCommitHead.
 * And also connect to commandLocations.
 */
static gceSTATUS
_LinkSubCommit(
    IN gcoBUFFER Buffer,
    IN gctUINT32 CoreId,
    IN gcsSTATE_DELTA_PTR StateDelta,
    IN gctUINT32 Context,
    IN gcoQUEUE Queue
    )
{
    gceSTATUS status;
    gcsHAL_SUBCOMMIT * subCommit;
    gcsQUEUE_PTR queue;

    gcmHEADER_ARG("Buffer=%p CoreId=%d StateDelta=%p Context=0x%x Queue=%p",
                  Buffer, CoreId, StateDelta, Context, Queue);

    if (!Buffer->commandLocationTail)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Get a new sub-commit and link next to subCommitTail. */
    if (Buffer->subCommitTail)
    {
        subCommit = _GetSubCommit(Buffer);
        Buffer->subCommitTail->next = gcmPTR_TO_UINT64(subCommit);
        Buffer->subCommitTail = subCommit;

        if (!subCommit)
        {
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }
    }
    else
    {
        subCommit = Buffer->subCommitTail = &Buffer->subCommitHead;
    }

    queue = Queue ? Queue->head : gcvNULL;

    subCommit->coreId    = CoreId;
    subCommit->delta     = gcmPTR_TO_UINT64(StateDelta);
    subCommit->context   = Context;
    subCommit->queue     = gcmPTR_TO_UINT64(queue);
    subCommit->next      = 0;

    subCommit->commandBuffer = Buffer->commandLocationHead;

    Buffer->commandLocationTail = gcvNULL;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/******************************************************************************\
******************************* gcoBUFFER API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoBUFFER_Construct
**
**  Construct a new gcoBUFFER object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gcoHARDWARE Hardware
**          Pointer to a gcoHARDWARE object.
**
**      gctSIZE_T MaxSize
**          Maximum size of buffer.
**
**      gctBOOL ThreadDefault
**          If gcvTRUE, the buffer is for thread default's hardware object
**
**  OUTPUT:
**
**      gcoBUFFER * Buffer
**          Pointer to a variable that will hold the the gcoBUFFER object
**          pointer.
*/
gceSTATUS
gcoBUFFER_Construct(
    IN gcoHAL Hal,
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gctSIZE_T MaxSize,
    IN gctBOOL ThreadDefault,
    OUT gcoBUFFER * Buffer
    )
{
    gceSTATUS status;
    gcoBUFFER buffer = gcvNULL;
    gctPOINTER pointer = gcvNULL;
    gcoCMDBUF commandBuffer;
    gctUINT32 mGpuModeSwitchBytes = 0;
    gctUINT i = 0;

    gcmHEADER_ARG("Hal=0x%x Hardware=0x%x MaxSize=%lu",
                  Hal, Hardware, MaxSize);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hal, gcvOBJ_HAL);
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Buffer != gcvNULL);


    /***************************************************************************
    ** Allocate and reset the gcoBUFFER object.
    */

    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(struct _gcoBUFFER), &pointer));

    gcoOS_ZeroMemory(pointer, gcmSIZEOF(struct _gcoBUFFER));

    buffer = (gcoBUFFER) pointer;

    /* Initialize the gcoBUFFER object. */
    buffer->object.type = gcvOBJ_BUFFER;
    buffer->hal         = Hal;
    buffer->threadDefault = ThreadDefault;

    /* Zero the command buffers. */
    buffer->commandBufferList = gcvNULL;
    buffer->commandBufferTail = gcvNULL;

    /* Set the requested size. */
    buffer->bytes = MaxSize;

    /* Set the default maximum number of command buffers. */
    buffer->maxCount = gcdMAX_CMD_BUFFERS;

    buffer->hardware = Hardware;
    buffer->info.engine   = Engine;

    /**************************************************************************
    ** Allocate temp command buffer.
    */
    gcmONERROR(gcoOS_Allocate(gcvNULL, gcdMAX_TEMPCMD_BUFFER_SIZE, &buffer->tempCMDBUF.buffer));
    buffer->tempCMDBUF.currentByteSize = 0;
    buffer->tempCMDBUF.inUse = gcvFALSE;

    /***************************************************************************
    ** Query alignment.
    */
    gcmONERROR(gcoHARDWARE_QueryCommandBuffer(
        buffer->hardware,
        buffer->info.engine,
        &buffer->info.alignment,
        &buffer->info.reservedHead,
        &buffer->info.reservedTail,
        &buffer->info.reservedUser,
        &mGpuModeSwitchBytes
        ));

#if gcdENABLE_3D
    {
        gctUINT32 gpuCount = 0;
        gcoHARDWARE_Query3DCoreCount(buffer->hardware, &gpuCount);
        if (gpuCount > 1)
        {
            buffer->queryResumeBytes[gcvQUERY_OCCLUSION] = gpuCount * (2 * gcmSIZEOF(gctUINT32) + gcdRESUME_OQ_LENGTH)
                                                         + 2 * gcmSIZEOF(gctUINT32);
            buffer->probeResumeBytes = gpuCount * (2 * gcmSIZEOF(gctUINT32) + gcdRESUME_PROBE_LENGH)
                                     + 2 * gcmSIZEOF(gctUINT32);;
        }
        else
        {
            buffer->queryResumeBytes[gcvQUERY_OCCLUSION] = gcdRESUME_OQ_LENGTH;
            buffer->probeResumeBytes = gcdRESUME_PROBE_LENGH;
        }
    }

    buffer->tfbResumeBytes                            = mGpuModeSwitchBytes + gcdRESUME_XFB_LENGH;
    buffer->queryResumeBytes[gcvQUERY_XFB_WRITTEN]    = mGpuModeSwitchBytes + gcdRESUME_XFBWRITTEN_QUERY_LENGTH;
    buffer->queryResumeBytes[gcvQUERY_PRIM_GENERATED] = mGpuModeSwitchBytes + gcdRESUME_PRIMGEN_QUERY_LENGTH;
#endif

    buffer->totalReserved
        = buffer->info.reservedHead
        + buffer->info.reservedTail
        + buffer->info.reservedUser
        + buffer->info.alignment;

    if (!gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_CHIPENABLE_LINK))
    {
        /* NOTE: This function only valid before gcoHARDWARE_Construct() exits. */
        gcoHARDWARE_Query3DCoreCount(Hardware, &buffer->mirrorCount);

        buffer->mirrorCount--;
    }

    buffer->hwFeature.hasFence = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_FENCE);
    buffer->hwFeature.hasPipe2D = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_PIPE_2D);
    buffer->hwFeature.hasPipe3D = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_PIPE_3D);
    buffer->hwFeature.hasNewMMU = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_MMU);
    buffer->hwFeature.hasHWTFB = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_HW_TFB);
    buffer->hwFeature.hasProbe = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_PROBE);
    buffer->hwFeature.hasHWFence = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_FENCE_32BIT)
                                || gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_FENCE_64BIT);
    buffer->hwFeature.hasComputeOnly = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_COMPUTE_ONLY);
    buffer->hwFeature.hasMCFE = gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_MCFE);
    /* Initialize commits. */
    buffer->subCommitTail       = gcvNULL;
    buffer->commandLocationTail = gcvNULL;

    /***************************************************************************
    ** Initialize the command buffers.
    */

#if defined(ANDROID)
    /* No buffer when initialization. */
    buffer->count = gcdCMD_BUFFERS;
#else
    /* Construct a command buffer. */
    for (i = 0; i < gcdCMD_BUFFERS; ++i)
    {
        gcmONERROR(_ConstructCommandBuffer(gcvNULL, buffer->hardware,
                                           MaxSize,
                                           (gcsCOMMAND_INFO_PTR) &buffer->info,
                                           &commandBuffer));

        if (buffer->commandBufferList == gcvNULL)
        {
            buffer->commandBufferList = commandBuffer;
            commandBuffer->prev =
            commandBuffer->next = commandBuffer;
        }
        else
        {
            /* Add to the tail. */
            commandBuffer->prev = buffer->commandBufferList->prev;
            commandBuffer->next = buffer->commandBufferList;
            buffer->commandBufferList->prev->next = commandBuffer;
            buffer->commandBufferList->prev = commandBuffer;
        }

        gcmONERROR(_ConstructMirrorCommandBuffer(Hardware, buffer, commandBuffer));
    }

    /* Number of buffers initialized. */
    buffer->count = gcdCMD_BUFFERS;

    /* Get the current command buffer. */
    gcmONERROR(_GetCommandBuffer(buffer));
#endif

    /* Return pointer to the gcoBUFFER object. */
    *Buffer = buffer;

    /* Success. */
    gcmFOOTER_ARG("*Buffer=0x%x", *Buffer);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    if (buffer)
    {
        gcmVERIFY_OK(gcoBUFFER_Destroy(buffer));
    }
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gctUINT32
_GetResumeCommandLength(
    IN gcoBUFFER Buffer
    )
{
    gctUINT sizeInBytes = 0;

#if gcdENABLE_3D

    gctINT32 type;

    for (type = gcvQUERY_OCCLUSION; type < gcvQUERY_MAX_NUM; type++)
    {
        if (Buffer->queryPaused[type])
        {
            sizeInBytes += Buffer->queryResumeBytes[type];
        }
    }

    if (Buffer->tfbPaused)
    {
        sizeInBytes += Buffer->tfbResumeBytes;
    }

    if (Buffer->probePaused)
    {
        sizeInBytes += Buffer->probeResumeBytes;
    }
#endif

    return sizeInBytes;
}


gceSTATUS
gcoBUFFER_AddTimestampPatch(
    IN gcoBUFFER Buffer,
    IN gctUINT32 Handle,
    IN gctUINT32 Flag
    )
{
    gcsHAL_PATCH_LIST * patchList;
    gcsHAL_PATCH_VIDMEM_TIMESTAMP * patchItem;

    gcmHEADER_ARG("Buffer=0x%x, Handle=0x%x, Flag=0x%x", Buffer, Handle, Flag);
    gcmASSERT(Handle != 0);

    patchList = _GetPatchItem(Buffer,
                              gcvHAL_PATCH_VIDMEM_TIMESTAMP,
                              Buffer->tempCMDBUF.inUse);

    patchItem = gcmUINT64_TO_PTR(patchList->patchArray);

    /* Pointer to current one. */
    patchItem += patchList->count;

    patchItem->handle = Handle;
    patchItem->flag = Flag;

    patchList->count++;

    /* Succees. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Calculate the patch location in command buffer. */
static gcmINLINE gctUINT32
_CalculatePatchLocation(
    gcoBUFFER Buffer,
    gctPOINTER Logical
    )
{
    gctUINT32 location;
    struct _gcsTEMPCMDBUF * tempCMDBUF = &Buffer->tempCMDBUF;

    if (tempCMDBUF->inUse)
    {
        /* Calculate location in the gcsTEMPCMDBUF. */
        location = (gctUINT32)((gctUINTPTR_T)Logical - (gctUINTPTR_T)tempCMDBUF->buffer);

        gcmASSERT(location < gcdMAX_TEMPCMD_BUFFER_SIZE);
    }
    else
    {
        gcoCMDBUF commandBuffer = Buffer->commandBufferTail;

        /* Calculate location in the gcoCMDBUF object. */
        location = (gctUINT32)((gctUINTPTR_T)Logical - (gctUINTPTR_T)commandBuffer->logical);

        gcmASSERT(location < commandBuffer->offset);
    }

    return location;
}

gceSTATUS
gcoBUFFER_AddVidmemAddressPatch(
    IN gcoBUFFER Buffer,
    IN gctUINT32_PTR Logical,
    IN gctUINT32 Handle,
    IN gctUINT32 NodeOffset
    )
{
    gcsHAL_PATCH_LIST * patchList;
    gcsHAL_PATCH_VIDMEM_ADDRESS * patchItem;

    gcmHEADER_ARG("Buffer=%p Logical=%p Handle=0x%x NodeOffset=0x%x",
                  Buffer, Logical, Handle, NodeOffset);

    /* Add to temp video memory patch list when temp buffer used. */
    patchList = _GetPatchItem(Buffer,
                              gcvHAL_PATCH_VIDMEM_ADDRESS,
                              Buffer->tempCMDBUF.inUse);

    patchItem = gcmUINT64_TO_PTR(patchList->patchArray);

    /* Pointer to current one. */
    patchItem += patchList->count;

    patchItem->location = _CalculatePatchLocation(Buffer, Logical);
    patchItem->node     = Handle;
    patchItem->offset   = NodeOffset;

    patchList->count++;

    /* Succees. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoBUFFER_AddMCFESemaphorePatch(
    IN gcoBUFFER Buffer,
    IN gctUINT32_PTR Logical,
    IN gctBOOL SendSema,
    IN gctUINT32 SemaHandle
    )
{
    gcsHAL_PATCH_LIST * patchList;
    gcsHAL_PATCH_MCFE_SEMAPHORE * patchItem;

    gcmHEADER_ARG("Buffer=0x%x Logical=%p, SemaHandle=%u",
                  Buffer, Logical, SemaHandle);

    if (!Buffer->hwFeature.hasMCFE)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_REQUEST);
        return gcvSTATUS_INVALID_REQUEST;
    }

    /* Add to temp mcfe semaphore patch list when temp buffer used. */
    patchList = _GetPatchItem(Buffer,
                              gcvHAL_PATCH_MCFE_SEMAPHORE,
                              Buffer->tempCMDBUF.inUse);

    patchItem = gcmUINT64_TO_PTR(patchList->patchArray);

    /* Pointer to current one. */
    patchItem += patchList->count;

    patchItem->location = _CalculatePatchLocation(Buffer, Logical);
    patchItem->sendSema = SendSema;
    patchItem->semaHandle = SemaHandle;

    patchList->count++;

    /* Succees. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#if gcdENABLE_3D
static gceSTATUS
_FreeFenceList(
    gcsFENCE_LIST_PTR fenceList
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("fenceList=0x%x", fenceList);

    if (fenceList)
    {
        if (fenceList->pendingList)
        {
            gcoOS_Free(gcvNULL, fenceList->pendingList);
            fenceList->pendingList = gcvNULL;
        }

        if (fenceList->onIssueList)
        {
            gcoOS_Free(gcvNULL, fenceList->onIssueList);
            fenceList->onIssueList = gcvNULL;
        }

        gcoOS_Free(gcvNULL, fenceList);
    }

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AllocFenceList(
    IN gcsFENCE_LIST_PTR srcList,
    IN gcsFENCE_LIST_PTR * outList
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER pointer;
    gcsFENCE_LIST_PTR fenceList = gcvNULL;
    gctUINT pendingCount;
    gctUINT onIssueCount;
    gctUINT size;

    gcmHEADER_ARG("srcList=0x%x, outList=0x%x", srcList, outList);

    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(gcsFENCE_LIST), &pointer));
    fenceList = (gcsFENCE_LIST_PTR)pointer;
    gcoOS_ZeroMemory(fenceList, gcmSIZEOF(gcsFENCE_LIST));

    pendingCount = FENCE_NODE_LIST_INIT_COUNT;
    onIssueCount = FENCE_NODE_LIST_INIT_COUNT;

    if (srcList)
    {
        pendingCount += srcList->pendingCount;
        onIssueCount += srcList->onIssueCount;
    }

    size = gcmSIZEOF(gcsFENCE_APPEND_NODE) * pendingCount;
    gcmONERROR(gcoOS_Allocate(gcvNULL, size, &pointer));
    fenceList->pendingList = (gcsFENCE_APPEND_NODE_PTR)pointer;
    fenceList->pendingAllocCount = pendingCount;

    if (srcList && srcList->pendingCount > 0)
    {
        gcoOS_MemCopy(fenceList->pendingList,
            srcList->pendingList,
            srcList->pendingCount * gcmSIZEOF(gcsFENCE_APPEND_NODE));
        fenceList->pendingCount += srcList->pendingCount;
    }

    size = gcmSIZEOF(gcsFENCE_APPEND_NODE) * onIssueCount;
    gcmONERROR(gcoOS_Allocate(gcvNULL, size, &pointer));
    fenceList->onIssueList = ((gcsFENCE_APPEND_NODE_PTR)pointer);
    fenceList->onIssueAllocCount = onIssueCount;

    if (srcList && srcList->onIssueCount > 0)
    {
        gcoOS_MemCopy(fenceList->onIssueList,
            srcList->onIssueList,
            srcList->onIssueCount * gcmSIZEOF(gcsFENCE_APPEND_NODE));
        fenceList->onIssueCount += srcList->onIssueCount;
    }

    *outList = fenceList;

    gcmFOOTER();
    return status;

OnError:
    _FreeFenceList(fenceList);
    *outList = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoBUFFER_AppendFence(
    IN gcoBUFFER Buffer,
    IN gcsSURF_NODE_PTR Node,
    IN gceFENCE_TYPE Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsFENCE_LIST_PTR fenceList;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    fenceList = Buffer->fenceList;

    /* empty or full */
    if (!fenceList ||
        (fenceList->pendingAllocCount == fenceList->pendingCount))
    {
        gcmONERROR(_AllocFenceList(fenceList, &fenceList));

        if (Buffer->fenceList)
        {
            _FreeFenceList(Buffer->fenceList);
        }

        Buffer->fenceList = fenceList;
    }

    fenceList->pendingList[fenceList->pendingCount].node = Node;
    fenceList->pendingList[fenceList->pendingCount].type = Type;
    fenceList->pendingCount++;

OnError:
    gcmFOOTER();
    return status;
}

/*after command that use resource in pending list, we need move it to onIssue list  */
gceSTATUS
gcoBUFFER_OnIssueFence(
    IN gcoBUFFER Buffer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT count;
    gctPOINTER pointer;
    gcsFENCE_LIST_PTR fenceList = gcvNULL;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    if(!Buffer->hwFeature.hasFence)
    {
        gcmFOOTER();
        return status;
    }

    fenceList = Buffer->fenceList;

    if (Buffer->tempCMDBUF.inUse)
    {
        gcmPRINT("Warning, should not OnIssue Fence in temp command buffer");
    }

    if (fenceList && fenceList->pendingCount > 0)
    {
        if (fenceList->onIssueAllocCount - fenceList->onIssueCount < fenceList->pendingCount)
        {
            gcsFENCE_APPEND_NODE_PTR ptr;

            count = fenceList->onIssueCount + fenceList->pendingCount + FENCE_NODE_LIST_INIT_COUNT;

            gcmONERROR(gcoOS_Allocate(gcvNULL, count * gcmSIZEOF(gcsFENCE_APPEND_NODE), &pointer));

            ptr = (gcsFENCE_APPEND_NODE_PTR)pointer;
            fenceList->onIssueAllocCount = count;

            if (fenceList->onIssueCount)
            {
                gcoOS_MemCopy(ptr, fenceList->onIssueList, fenceList->onIssueCount * gcmSIZEOF(gcsFENCE_APPEND_NODE));
            }
            gcoOS_Free(gcvNULL, fenceList->onIssueList);

            fenceList->onIssueList = ptr;
        }

        gcoOS_MemCopy(&fenceList->onIssueList[fenceList->onIssueCount],
            fenceList->pendingList,
            fenceList->pendingCount * gcmSIZEOF(gcsFENCE_APPEND_NODE));

        fenceList->onIssueCount += fenceList->pendingCount;
        fenceList->pendingCount = 0;
    }

OnError:
    gcmFOOTER();
    return status;
}

/* Before send fence, we need get fence for all onIssue fence */
gceSTATUS
gcoBUFFER_GetFence(
    IN gcoBUFFER Buffer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT i;
    gcsSURF_NODE_PTR node;
    gcsFENCE_LIST_PTR fenceList;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    fenceList = Buffer->fenceList;

    if(fenceList)
    {
        for (i = 0; i < fenceList->onIssueCount; i++)
        {
            node = fenceList->onIssueList[i].node;
            gcoHARDWARE_GetFence(gcvNULL, &node->fenceCtx, Buffer->info.engine, fenceList->onIssueList[i].type);
        }

        fenceList->onIssueCount = 0;
    }

    gcmFOOTER();
    return status;
}
#endif

/*******************************************************************************
**
**  gcoBUFFER_Destroy
**
**  Destroy an gcoBUFFER object.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to an gcoBUFFER object to delete.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoBUFFER_Destroy(
    IN gcoBUFFER Buffer
    )
{
    gcoCMDBUF commandBuffer;
    gctUINT i;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    /* Destroy all command buffers. */
    while (Buffer->commandBufferList != gcvNULL)
    {
        /* Get the head of the list. */
        commandBuffer = Buffer->commandBufferList;

        if(Buffer->commandBufferTail == commandBuffer)
        {
            Buffer->commandBufferTail = gcvNULL;
        }

        /* Remove the head buffer from the list. */
        if (commandBuffer->next == commandBuffer)
        {
            Buffer->commandBufferList = gcvNULL;
        }
        else
        {
            commandBuffer->prev->next =
            Buffer->commandBufferList = commandBuffer->next;
            commandBuffer->next->prev = commandBuffer->prev;
        }

        /* Destroy command buffer. */
        _DestroyMirrorCommandBuffer(Buffer, commandBuffer);
        _DestroyCommandBuffer(Buffer->hardware, &Buffer->info, commandBuffer);
    }

#if gcdENABLE_3D
    gcmVERIFY_OK(_FreeFenceList(Buffer->fenceList));
    Buffer->fenceList = gcvNULL;
#endif

    /* Recycle sub-commits. */
    _RecycleSubCommits(Buffer);

    /* Destroy command locations. */
    _DestroyCommandLocations(Buffer->freeCommandLocations);
    Buffer->freeCommandLocations = gcvNULL;

    /* Destroy sub-commits. */
    _DestroySubCommits(Buffer->freeSubCommits);
    Buffer->freeSubCommits = gcvNULL;

    for (i = 0; i < gcvHAL_PATCH_TYPE_COUNT; i++)
    {
        _DestroyPatchLists(Buffer->freePatchLists[i]);
        Buffer->freePatchLists[i] = gcvNULL;

        _DestroyPatchLists(Buffer->tempPatchLists[i]);
        Buffer->tempPatchLists[i] = gcvNULL;
    }

    gcoOS_Free(gcvNULL, Buffer->tempCMDBUF.buffer);

    /* Free the object memory. */
    gcmOS_SAFE_FREE(gcvNULL, Buffer);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gcmINLINE gceSTATUS _CaptureCommandBuffer(gcoBUFFER Buffer, gcoCMDBUF commandBuffer)
{
    if (Buffer->captureEnabled)
    {
        gctINT32 remainedSize = commandBuffer->offset - Buffer->captureCommandOffset;

        gcmASSERT(remainedSize >= 0);

        if (remainedSize)
        {
            if (remainedSize <= Buffer->captureRemainedSize)
            {
                gctUINT8 *src, *dst;
                src = (gctUINT8_PTR)gcmUINT64_TO_PTR(commandBuffer->logical) + Buffer->captureCommandOffset;
                dst = Buffer->captureBuffer
                    + Buffer->captureBufferTotalSize
                    - Buffer->captureRemainedSize;

                gcoOS_MemCopy(dst, src, remainedSize);
                Buffer->captureRemainedSize -= remainedSize;
            }
            else
            {
                /* overflowed, disable capture */
                Buffer->captureRemainedSize =  -1;
                Buffer->captureEnabled = gcvFALSE;
                return gcvSTATUS_OUT_OF_MEMORY;
            }
        }
    }

    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoBUFFER_Write
**
**  Copy a number of bytes into the buffer.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to an gcoBUFFER object.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer that contains the data to be copied.
**
**      IN gctSIZE_T Bytes
**          Number of bytes to copy.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned to 64-bit.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoBUFFER_Write(
    IN gcoBUFFER Buffer,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;
    gcoCMDBUF reserve;

    gcmHEADER_ARG("Buffer=0x%x Data=0x%x Bytes=%lu Aligned=%d",
                  Buffer, Data, Bytes, Aligned);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Reserve data in the buffer. */
    gcmONERROR(gcoBUFFER_Reserve(Buffer, Bytes, Aligned, gcvCOMMAND_3D, &reserve));

    /* Write data into the buffer. */
    gcoOS_MemCopy(gcmUINT64_TO_PTR(reserve->lastReserve), Data, Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoBUFFER_Reserve
**
**  Reserve a number of bytes in the buffer.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to an gcoBUFFER object.
**
**      gctSIZE_T Bytes
**          Number of bytes to reserve.
**
**      gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned to 64-bit.
**
**  OUTPUT:
**
**      gctUINT32_PTR ** AddressHints
**          Pointer to a variable that receives the current position in the
**          state hint array.  gcvNULL is allowed.
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the address of location in the
**          buffer that has been reserved.
*/
gceSTATUS
gcoBUFFER_Reserve(
    IN gcoBUFFER Buffer,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned,
    IN gctUINT32 Usage,
    OUT gcoCMDBUF * Reserve
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoCMDBUF commandBuffer;
    gctUINT32 alignBytes, finalBytes = 0, originalBytes, resumeBytes, reserveBytes, alignedBytes;
    gctUINT32 offset;
    gcsHAL_INTERFACE iface;
    gctBOOL notInSamePage = gcvFALSE;

#if gcdENABLE_3D
    gctINT32 queryType;
#endif

    gcmHEADER_ARG("Buffer=0x%x Bytes=%lu Aligned=%d Reserve=0x%x",
                  Buffer, Bytes, Aligned, Reserve);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmDEBUG_VERIFY_ARGUMENT(Reserve != gcvNULL);

    gcmSAFECASTSIZET(originalBytes, Bytes);

    /* Recursive call must be detected and avoided */
    gcmASSERT(Buffer->inRerserved == gcvFALSE);
    Buffer->inRerserved = gcvTRUE;

    resumeBytes = _GetResumeCommandLength(Buffer);

    reserveBytes = originalBytes + resumeBytes;

    /* Get the current command buffer. */
    commandBuffer = Buffer->commandBufferTail;

    if (commandBuffer == gcvNULL)
    {
        /* Grab a new command buffer. */
        gcmONERROR(_GetCommandBuffer(Buffer));

        /* Get the new buffer. */
        commandBuffer = Buffer->commandBufferTail;
    }

    if (Buffer->threadDefault)
    {
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, _GC_OBJ_ZONE,
            "Thread Default command buffer is accumulating commands");
    }


    if (Buffer->tempCMDBUF.inUse)
    {
        gcmFATAL("!!!temp command buffer is in using, should not send command into real command buffer");
    }

    /* Compute the number of aligned bytes. */
    alignBytes = Aligned
               ? ( gcmALIGN(commandBuffer->offset, Buffer->info.alignment)
                 - commandBuffer->offset
                 )
               : 0;

    /* Compute the number of required bytes. */
    alignedBytes = reserveBytes + alignBytes;

    if (Buffer->hwFeature.hasPipe2D
     && Buffer->hwFeature.hasPipe3D
     && !Buffer->hwFeature.hasNewMMU
     && (Usage == gcvCOMMAND_2D))
    {
        offset = commandBuffer->offset + alignBytes;

        if (((offset + reserveBytes - 1) & ~0xFFF) != (offset & ~0xFFF))
        {
            notInSamePage = gcvTRUE;
        }
    }

    if (alignedBytes > commandBuffer->free || notInSamePage)
    {
        if (!Buffer->dropCommandEnabled)
        {
            /* Sent event to signal when command buffer completes. */
            iface.command            = gcvHAL_SIGNAL;
            iface.engine             = Buffer->info.engine;
            iface.u.Signal.signal    = gcmPTR_TO_UINT64(commandBuffer->signal);
            iface.u.Signal.auxSignal = 0;
            iface.u.Signal.process   = gcmPTR_TO_UINT64(gcoOS_GetCurrentProcessID());
            iface.u.Signal.fromWhere = gcvKERNEL_COMMAND;

            /* Send event. */
            gcmONERROR(gcoHARDWARE_CallEvent(Buffer->hardware, &iface));
        }
        else
        {
            /* as we already commit true for the command buffer
             * before capture with dropCommandEnabled, we could signal this commandbuffer
             * right away.
             */
            gcmONERROR(gcoOS_Signal(gcvNULL, commandBuffer->signal, gcvTRUE));
        }


        if (Buffer->hwFeature.hasMCFE)
        {
            /* Do not commit for MCFE, but accumulates more. */
            gcmONERROR(_LinkCommandLocation(Buffer, commandBuffer, 0));
        }
        else
        {
            /* Commit current command buffer. */
            gcmONERROR(gcoHARDWARE_Commit(Buffer->hardware));
        }

        /* Grab a new command buffer. */
        gcmONERROR(_GetCommandBuffer(Buffer));

        /* Get new buffer. */
        commandBuffer = Buffer->commandBufferTail;

        if (Buffer->captureEnabled)
        {
            Buffer->captureCommandOffset = commandBuffer->offset;
        }

        if (resumeBytes == 0)
        {
            resumeBytes = _GetResumeCommandLength(Buffer);
            reserveBytes = originalBytes + resumeBytes;
        }

        if (reserveBytes > commandBuffer->free)
        {
            /* This just won't fit! */
            gcmFATAL("FATAL: Command of %lu original bytes + %lu resume bytes is too big!", originalBytes, resumeBytes);
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }
        /* Calculate total bytes again. */
        alignBytes = 0;
        finalBytes = reserveBytes;
    }
    else
    {
        finalBytes = alignedBytes;
    }

    gcmASSERT(commandBuffer != gcvNULL);
    gcmASSERT(finalBytes <= commandBuffer->free);

    /* Determine the data offset. */
    offset = commandBuffer->offset + alignBytes;

    /* Update the last reserved location. */
    commandBuffer->lastReserve = gcmPTR_TO_UINT64((gctUINT8_PTR) gcmUINT64_TO_PTR(commandBuffer->logical) + offset);
    commandBuffer->lastOffset  = offset;

    /* Adjust command buffer size. */
    commandBuffer->offset += finalBytes;
    commandBuffer->free   -= finalBytes;

#if gcdENABLE_3D
    for (queryType = gcvQUERY_OCCLUSION; queryType < gcvQUERY_MAX_NUM; queryType++)
    {
        if (Buffer->queryPaused[queryType])
        {
            gctUINT64 resumeCommand, resumeCommandSaved;
            resumeCommand = resumeCommandSaved = commandBuffer->lastReserve;
            gcoHARDWARE_SetQuery(Buffer->hardware,
                                 ~0U,
                                 queryType,
                                 gcvQUERYCMD_RESUME,
                                 (gctPOINTER *)&resumeCommand);
            gcmASSERT((resumeCommand - resumeCommandSaved) == Buffer->queryResumeBytes[queryType]);
            commandBuffer->lastReserve = resumeCommand;
            commandBuffer->lastOffset += (gctUINT32)(resumeCommand - resumeCommandSaved);
            Buffer->queryPaused[queryType] = gcvFALSE;
        }
    }

    if (Buffer->tfbPaused)
    {
        gctUINT64 resumeCommand, resumeCommandSaved;
        resumeCommand = resumeCommandSaved = commandBuffer->lastReserve;
        gcoHARDWARE_SetXfbCmd(Buffer->hardware, gcvXFBCMD_RESUME_INCOMMIT, (gctPOINTER *)&resumeCommand);
        gcmASSERT((resumeCommand - resumeCommandSaved) == Buffer->tfbResumeBytes);
        commandBuffer->lastReserve = resumeCommand;
        commandBuffer->lastOffset += (gctUINT32)(resumeCommand - resumeCommandSaved);
        Buffer->tfbPaused = gcvFALSE;
    }

    if (Buffer->probePaused)
    {
        gctUINT64 resumeCommand, resumeCommandSaved;
        resumeCommand = resumeCommandSaved = commandBuffer->lastReserve;
        gcoHARDWARE_SetProbeCmd(Buffer->hardware, gcvPROBECMD_RESUME, ~0U, (gctPOINTER *)&resumeCommand);
        gcmASSERT((resumeCommand - resumeCommandSaved) == Buffer->probeResumeBytes);
        commandBuffer->lastReserve = resumeCommand;
        commandBuffer->lastOffset += (gctUINT32)(resumeCommand - resumeCommandSaved);
        Buffer->probePaused = gcvFALSE;
    }
#endif

    if (Usage & gcvCOMMAND_3D)
    {
        commandBuffer->using3D = gcvTRUE;
    }

    if (Usage & gcvCOMMAND_2D)
    {
        commandBuffer->using2D = gcvTRUE;
    }

    /* Set the result. */
    * Reserve = commandBuffer;

    Buffer->inRerserved = gcvFALSE;

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
**  gcoBUFFER_Commit
**
**  Commit the command buffer to the hardware.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to a gcoBUFFER object.
**
**      gcePIPE_SELECT CurrentPipe
**          Current graphics pipe.
**
**      gcsSTATE_DELTA_PTR StateDelta
**          Pointer to the state delta.
**
**      gcoQUEUE Queue
**          Pointer to a gcoQUEUE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoBUFFER_Commit(
    IN gcoBUFFER Buffer,
    IN gcePIPE_SELECT CurrentPipe,
    IN gcsSTATE_DELTA_PTR StateDelta,
    IN gcsSTATE_DELTA_PTR *StateDeltas,
    IN gctUINT32 Context,
    IN gctUINT32_PTR Contexts,
    IN gcoQUEUE Queue,
    OUT gctPOINTER *DumpLogical,
    OUT gctUINT32 *DumpBytes
    )
{
    gceSTATUS status;
    gcoCMDBUF commandBuffer;
    gctBOOL emptyBuffer;
    gctUINT32 currentCoreId = 0;
    gctUINT32 coreIndex = 0;
    gctUINT32 coreCount = 1;
    gcmHEADER_ARG("Buffer=0x%x Queue=0x%x",
                  Buffer, Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

    if (Buffer->threadDefault)
    {
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, _GC_OBJ_ZONE, "Thread Default command buffer is comitting commands");
    }

    gcmONERROR(gcoHAL_GetCurrentCoreIndex(gcvNULL, &currentCoreId));
    gcmONERROR(gcoHARDWARE_QueryCoreIndex(Buffer->hardware,0, &coreIndex));

    gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));
    /* When gcoBUFFER_EndTEMPCMDBUF, currentByteSize will set at last,
     * so currentByteSize > 0, means we still under temp Command buffer mode,
     * if the even is resource free and that resource is used by this tmp
     * command, this have possible issue.
     */
    if (Buffer->tempCMDBUF.currentByteSize > 0 && Queue->tmpBufferRecordCount > 0)
    {
        gcmPRINT("warning: some event kick off during the tmp command period");
    }

    /* Grab the head command buffer. */
    commandBuffer = Buffer->commandBufferTail;

    if (commandBuffer == gcvNULL)
    {
        /* No current command buffer, of course empty buffer. */
        emptyBuffer = gcvTRUE;
    }
    else
    {
        /* Advance commit count. */
        commandBuffer->commitCount++;

        /* Determine whether the buffer is empty. */
        emptyBuffer = (commandBuffer->offset - commandBuffer->startOffset <= Buffer->info.reservedHead);
    }

    /* Send for execution. */
    if (emptyBuffer && !Buffer->subCommitTail)
    {
        gcmONERROR(gcoQUEUE_Commit(Queue, gcvFALSE));
    }
    else if (commandBuffer != gcvNULL)
    {
        gcsHAL_INTERFACE iface;
        gctUINT32 context = 0;
        gcoCMDBUF *commandBufferMirrors = commandBuffer->mirrors;

#if gcdENABLE_3D
        gcoCMDBUF tailCommandBuffer = Buffer->commandBufferTail;
        gctUINT alignedBytes = gcmALIGN(tailCommandBuffer->offset, Buffer->info.alignment) - tailCommandBuffer->offset;

        if (Buffer->captureEnabled)
        {
            _CaptureCommandBuffer(Buffer, commandBuffer);
        }

        tailCommandBuffer->offset += alignedBytes;

        if (tailCommandBuffer->using3D)
        {
            gctINT32 type;

            if (Buffer->info.engine == gcvENGINE_RENDER)
            {
                if (!Buffer->hwFeature.hasComputeOnly)
                {
                    /* need pauseQuery per commit and resumeQuery when next draw flushQuery */
                    for (type = gcvQUERY_OCCLUSION; type < gcvQUERY_MAX_NUM; type++)
                    {
                        gctUINT64 pauseQueryCommand, pauseQueryCommandsaved;
                        pauseQueryCommandsaved =
                        pauseQueryCommand      = gcmPTR_TO_UINT64((gctUINT8_PTR) gcmUINT64_TO_PTR(tailCommandBuffer->logical)
                                                        + tailCommandBuffer->offset);

                        gcoHARDWARE_SetQuery(gcvNULL, ~0U, (gceQueryType)type, gcvQUERYCMD_PAUSE, (gctPOINTER*)&pauseQueryCommand);
                        if ((pauseQueryCommand - pauseQueryCommandsaved) > 0)
                        {
                            tailCommandBuffer->offset += (gctUINT32)(pauseQueryCommand - pauseQueryCommandsaved);
                            Buffer->queryPaused[type] = gcvTRUE;
                        }
                    }

                    /* need pauseXFB per commit and resuemeXFB when next draw flushXFB */
                    if (Buffer->hwFeature.hasHWTFB)
                    {
                        gctUINT64 pauseXfbCommand, pauseXfbCommandsaved;

                        pauseXfbCommandsaved =
                        pauseXfbCommand      = gcmPTR_TO_UINT64((gctUINT8_PTR) gcmUINT64_TO_PTR(tailCommandBuffer->logical)
                                                    + tailCommandBuffer->offset);

                        gcoHARDWARE_SetXfbCmd(gcvNULL, gcvXFBCMD_PAUSE_INCOMMIT, (gctPOINTER*)&pauseXfbCommand);
                        if (pauseXfbCommand - pauseXfbCommandsaved > 0)
                        {
                            tailCommandBuffer->offset += (gctUINT32)(pauseXfbCommand - pauseXfbCommandsaved);
                            Buffer->tfbPaused = gcvTRUE;
                        }
                    }
                }

                if (Buffer->hwFeature.hasProbe)
                {
                    gctUINT64 pauseProbeCommand, pauseProbeCommandsaved;

                    pauseProbeCommandsaved =
                    pauseProbeCommand      = gcmPTR_TO_UINT64((gctUINT8_PTR)gcmUINT64_TO_PTR(tailCommandBuffer->logical)
                                                    + tailCommandBuffer->offset);

                    gcoHARDWARE_SetProbeCmd(gcvNULL, gcvPROBECMD_PAUSE, ~0U, (gctPOINTER*)&pauseProbeCommand);
                    if (pauseProbeCommand - pauseProbeCommandsaved > 0)
                    {
                        tailCommandBuffer->offset += (gctUINT32)(pauseProbeCommand - pauseProbeCommandsaved);
                        Buffer->probePaused = gcvTRUE;
                    }
                }

                gcoHARDWARE_Query3DCoreCount(gcvNULL, &coreCount);
                gcoHAL_GetCurrentCoreIndex(gcvNULL, &coreIndex);

                if (!Buffer->hwFeature.hasComputeOnly)
                {
                    alignedBytes = gcmALIGN(tailCommandBuffer->offset, Buffer->info.alignment) - tailCommandBuffer->offset;
                    if (coreCount > 1)
                    {
                        gctUINT32 bytes;
                        gctUINT32_PTR flushCacheCommandLogical;

                        flushCacheCommandLogical = (gctUINT32_PTR)((gctUINT8_PTR)gcmUINT64_TO_PTR(tailCommandBuffer->logical) +
                                                   tailCommandBuffer->offset + alignedBytes);

                        gcoHARDWARE_MultiGPUCacheFlush(gcvNULL, &flushCacheCommandLogical);

                        gcoHARDWARE_QueryMultiGPUCacheFlushLength(gcvNULL, &bytes);
                        tailCommandBuffer->offset += bytes + alignedBytes;
                    }
                    else
                    {
                        gctUINT8_PTR flushCacheCommand, flushCacheCommandsaved;

                        flushCacheCommandsaved =
                        flushCacheCommand = (gctUINT8_PTR)gcmUINT64_TO_PTR(tailCommandBuffer->logical)
                                          + tailCommandBuffer->offset + alignedBytes;

                        /* need flush ccache zcache and shl1_cache per commit */
                        gcoHARDWARE_FlushCache(gcvNULL, (gctPOINTER*)&flushCacheCommand);
                        tailCommandBuffer->offset += (gctUINT32)(flushCacheCommand - flushCacheCommandsaved) + alignedBytes;
                    }
                }

                if (coreCount > 1)
                {
                    gctUINT32 bytes;
                    gctUINT32_PTR syncCommandLogical;

                    alignedBytes = gcmALIGN(tailCommandBuffer->offset, Buffer->info.alignment) - tailCommandBuffer->offset;

                    syncCommandLogical = (gctUINT32_PTR)((gctUINT8_PTR)gcmUINT64_TO_PTR(tailCommandBuffer->logical) +
                                         tailCommandBuffer->offset + alignedBytes);

                    gcoHARDWARE_MultiGPUSync(gcvNULL, &syncCommandLogical);

                    gcoHARDWARE_QueryMultiGPUSyncLength(gcvNULL, &bytes);
                    tailCommandBuffer->offset += bytes + alignedBytes;
                }
            }

#if gcdSYNC
            /*Only send hw fence when commit, the fence type is hw fence when support gcvFEAUTRE_FENCE_32BIT*/
            if (Buffer->hwFeature.hasHWFence &&
                Buffer->inRerserved == gcvFALSE)
            {
                gctUINT8_PTR fenceCommand, fenceCommandSaved;

                alignedBytes = gcmALIGN(tailCommandBuffer->offset, Buffer->info.alignment) - tailCommandBuffer->offset;

                fenceCommandSaved =
                fenceCommand = (gctUINT8_PTR)gcmUINT64_TO_PTR(tailCommandBuffer->logical)
                             + tailCommandBuffer->offset + alignedBytes;

                /*no need flush cache and do multi gpu sync here*/
                if (Buffer->info.engine == gcvENGINE_BLT)
                {
                    /*no need flush cache and do multi gpu sync here*/
                    gcoHARDWARE_SendFence(gcvNULL, gcvFALSE, gcvENGINE_BLT,(gctPOINTER*)&fenceCommand);
                }
                else
                {
                    /*no need flush cache and do multi gpu sync here*/
                    gcoHARDWARE_SendFence(gcvNULL, gcvFALSE, gcvENGINE_RENDER,(gctPOINTER*)&fenceCommand);
                }
                tailCommandBuffer->offset += (gctUINT32)(fenceCommand - fenceCommandSaved) + alignedBytes;
            }
#endif
        }

#endif
        /* Make sure the tail got aligned properly. */
        commandBuffer->offset = gcmALIGN(commandBuffer->offset, Buffer->info.alignment);

#if (gcdDUMP || (gcdUSE_VX && gcdENABLE_3D))
        if (!Buffer->dropCommandEnabled)
        {
            *DumpLogical = (gctUINT8_PTR) gcmUINT64_TO_PTR(commandBuffer->logical)
                         + commandBuffer->startOffset
                         + Buffer->info.reservedHead;

            *DumpBytes = commandBuffer->offset
                       - commandBuffer->startOffset
                       - Buffer->info.reservedHead;
        }
#endif

        /* The current pipe becomes the exit pipe for the current command buffer. */
        commandBuffer->exitPipe = CurrentPipe;

#if gcdENABLE_3D && gcdENABLE_KERNEL_FENCE
        if (gcoHAL_GetOption(gcvNULL, gcvOPTION_KERNEL_FENCE))
        {
            gcmONERROR(
                gcoHARDWARE_BuildTimestampPatch(Buffer->hardware,
                                                Buffer,
                                                commandBuffer,
                                                Buffer->info.engine));
        }
#endif

#if gcdENABLE_3D
        context = (commandBuffer->using2D && !commandBuffer->using3D) ? 0
                : Context;
#endif

        /* New command location. */
        gcmONERROR(_LinkCommandLocation(Buffer, commandBuffer, 0));

        /*
         * New sub commit for current core, link to previous command locations.
         * For single-core mode, current core is the core we need to use.
         * For combine-core mode, current core should be 0.
         */
        /* Event queue is put to the first subCommit. */
        gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, 0, &coreIndex));

        gcmONERROR(_LinkSubCommit(Buffer, coreIndex, gcvNULL, 0, Queue));

        /* Put context in the first sub commit. */
        Buffer->subCommitHead.context = context;
        Buffer->subCommitHead.delta   = gcmPTR_TO_UINT64(StateDelta);

        if (Buffer->info.engine == gcvENGINE_RENDER && coreCount > 1)
        {
            gctUINT32 i;

            for (i = 1; i < coreCount; i++)
            {
                gctUINT32 coreIndex;
                gctUINT32 ctx;
                gcsSTATE_DELTA_PTR delta;
                gcoCMDBUF cmdBuf;

                /* Convert core index in this hardware to global core index. */
                gcmONERROR(gcoHARDWARE_QueryCoreIndex(gcvNULL, i, &coreIndex));

                ctx = Contexts ? Contexts[coreIndex] : context;
                delta = StateDeltas ? StateDeltas[coreIndex] : StateDelta;

                if (commandBufferMirrors)
                {
                    /* It is not needed to index commandBufferMirrors with real coreIndex,
                    *  because content in mirrors always keeps same as command buffer, no
                    *  matter what its index is. */
                    cmdBuf = commandBufferMirrors[i - 1];

                    _DuplicateCommandBuffer(cmdBuf, commandBuffer);

                    /* Add mirrored command buffer. */
                    gcmONERROR(_LinkCommandLocation(Buffer, cmdBuf, 0));

                    /* Advance commit count. */
                    cmdBuf->commitCount++;
                }
                else
                {
                    /* Add the same command buffer. */
                    gcmONERROR(_LinkCommandLocation(Buffer, commandBuffer, i));
                }

                /* Add sub commit for core coreIndex. */
                gcmONERROR(_LinkSubCommit(Buffer, coreIndex, delta, ctx, gcvNULL));

                /* Set it to TLS to find correct command queue. */
                gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));
            }
        }

        /* Finish link sub-commits. */
        Buffer->subCommitTail = gcvNULL;

        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_COMMIT;
        iface.engine = Buffer->info.engine;
        iface.u.Commit.subCommit = Buffer->subCommitHead;
        iface.u.Commit.shared  = (coreCount > 1);
        iface.commitMutex = gcvFALSE;

        if (!Buffer->dropCommandEnabled)
        {
            /* Call kernel service. */
            gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                           IOCTL_GCHAL_INTERFACE,
                                           &iface, gcmSIZEOF(iface),
                                           &iface, gcmSIZEOF(iface)));
            gcmONERROR(iface.status);

            /* Update commit stamp. */
            Buffer->commitStamp = iface.u.Commit.commitStamp;
        }

        /* Finish current buffer range and will use next. */
        _FinishCommandBufferRange(Buffer, commandBuffer);

        /* Recycle the sub-commits. */
        _RecycleSubCommits(Buffer);
    }
    else
    {
        gcmONERROR(gcvSTATUS_INVALID_DATA);
    }

    /* Empty buffer must be the tail. */
    gcmONERROR(gcoQUEUE_Free(Queue));

    /* Restore core index in TLS. */
    gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, currentCoreId));
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
**  gcoBUFFER_IsEmpty
**
**  Check if no command acculated in the buffer.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to a gcoBUFFER object
**
**  OUTPUT:
**
**      None.
*/
gceSTATUS
gcoBUFFER_IsEmpty(
    gcoBUFFER Buffer
    )
{
    gceSTATUS status;
    gcoCMDBUF commandBuffer;
    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    commandBuffer = Buffer->commandBufferTail;

    if (commandBuffer == gcvNULL)
    {
        status = gcvSTATUS_TRUE;
    }
    else if (commandBuffer->offset - commandBuffer->startOffset <= Buffer->info.reservedHead)
    {
        status = gcvSTATUS_TRUE;
    }
    else
    {
        status = gcvSTATUS_FALSE;
    }

    gcmFOOTER_ARG("status=%d", status);
    return status;
}

/*******************************************************************************
**
**  gcoBUFFER_StartTEMPCMDBUF
**
**  Star to use temp command buffer
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to a gcoBUFFER object
**
**
**
**  OUTPUT:
**
**      gcsTEMPCMDBUF *tempCMDBUF
**          Pointer to a variable that will hold the gcsTEMPCMDBUF
*/

gceSTATUS
gcoBUFFER_StartTEMPCMDBUF(
    IN gcoBUFFER Buffer,
    IN gcoQUEUE Queue,
    OUT gcsTEMPCMDBUF *tempCMDBUF
    )
{
    gcmHEADER_ARG("Buffer=0x%x tempCMDBUF=0x%x", Buffer, tempCMDBUF);

    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_ARGUMENT(tempCMDBUF != gcvNULL);

    *tempCMDBUF = &Buffer->tempCMDBUF;

    gcmASSERT((*tempCMDBUF)->currentByteSize == 0);

    gcmASSERT((*tempCMDBUF)->inUse == gcvFALSE);

    (*tempCMDBUF)->inUse = gcvTRUE;
    Queue->tmpBufferRecordCount = 0;


    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoBUFFER_EndTEMPCMDBUF
**
**  End using temp command buffer and send to real command buffer.
**
**  INPUT:
**      gcoBUFFER Buffer
**          Pointer to a gcoBUFFER object
**
**      gctBOOL Drop
**          The tempCmdbuffer has incomplete command to drop off.
**
**  OUTPUT:
**
**      Nothing
*/

gceSTATUS
gcoBUFFER_EndTEMPCMDBUF(
    IN gcoBUFFER Buffer,
    IN gctBOOL   Drop
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsTEMPCMDBUF tempCMDBUF;
    gcmHEADER_ARG("Buffer=0x%x Drop=%d", Buffer, Drop);

    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    tempCMDBUF = &Buffer->tempCMDBUF;

    if (tempCMDBUF->currentByteSize >  gcdMAX_TEMPCMD_BUFFER_SIZE)
    {
        gcmPRINT(" Temp command buffer is overflowed!");
    }

    tempCMDBUF->inUse = gcvFALSE;

    if (tempCMDBUF->currentByteSize && !Drop)
    {
        status = gcoBUFFER_Write(Buffer,
                                 tempCMDBUF->buffer,
                                 tempCMDBUF->currentByteSize,
                                 gcvTRUE);

        _MoveTempPatchLists(Buffer);
    }

    tempCMDBUF->currentByteSize = 0;

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoBUFFER_SelectChannel(
    IN gcoBUFFER Buffer,
    IN gctBOOL Priority,
    IN gctUINT32 ChannelId
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=%p Priority=%d ChannelId=%d",
                  Buffer, Priority, ChannelId);

    if (Priority  == Buffer->mcfePriority &&
        ChannelId == Buffer->mcfeChannelId)
    {
        /* Nothing to do. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Complete previous command location. */
    gcmONERROR(_LinkCommandLocation(Buffer, Buffer->commandBufferTail, 0));

    /* Finish buffer range. */
    _FinishCommandBufferRange(Buffer, Buffer->commandBufferTail);

    /* Switch channel. */
    Buffer->mcfePriority  = Priority;
    Buffer->mcfeChannelId = ChannelId;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoBUFFER_GetChannelInfo(
    IN gcoBUFFER Buffer,
    OUT gctBOOL *Priority,
    OUT gctUINT32 *ChannelId
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Buffer=%p Priority=%p ChannelId=%p",
                  Buffer, Priority, ChannelId);

    if (Priority)
    {
        *Priority = Buffer->mcfePriority;
    }

    if (ChannelId)
    {
        *ChannelId = Buffer->mcfeChannelId;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoBUFFER_Capture(
    IN gcoBUFFER Buffer,
    IN OUT gctUINT8 *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *pOutputSizeInByte,
    IN gctBOOL Enabled,
    IN gctBOOL dropCommandEnabled
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoCMDBUF commandBuffer;

    gcmHEADER_ARG("Buffer=%p CaptureBuffer=0x%x InputSizeInByte=%d"
                  "pOutputSizeInByte=0x%x Enabled=%d dropCommandEnabled=%d",
                  Buffer, CaptureBuffer, InputSizeInByte, pOutputSizeInByte, Enabled, dropCommandEnabled);

    commandBuffer = Buffer->commandBufferTail;
    gcmASSERT(commandBuffer);

    if (Enabled)
    {
        gcmASSERT(!Buffer->captureEnabled);
        gcmASSERT(CaptureBuffer);
        gcmASSERT(InputSizeInByte);
        /* If we want to skip sending command to hardware during the capture,
         * we need commit existing commands in current command buffer first
         * and stall it. so we could make the command buffer could be used for
         * capture only.
         */
        if (dropCommandEnabled)
        {
            status = gcoHAL_Commit(Buffer->hal, gcvTRUE);
        }
        Buffer->captureEnabled = gcvTRUE;
        Buffer->captureBuffer = CaptureBuffer;
        Buffer->captureBufferTotalSize = InputSizeInByte;
        Buffer->captureRemainedSize = (gctINT)InputSizeInByte;
        Buffer->captureCommandOffset = commandBuffer->offset;
        Buffer->dropCommandEnabled = dropCommandEnabled;
    }
    else
    {
        gcmASSERT(!Buffer->tempCMDBUF.inUse);
        gcmASSERT(pOutputSizeInByte);

        *pOutputSizeInByte = 0;
        /* overflowed capture buffer */
        if (Buffer->captureRemainedSize < 0)
        {
            status = gcvSTATUS_OUT_OF_MEMORY;
        }
        else
        {
            status = _CaptureCommandBuffer(Buffer, commandBuffer);
            if (gcmIS_SUCCESS(status))
            {
                *pOutputSizeInByte = (gctINT32)Buffer->captureBufferTotalSize
                                   - Buffer->captureRemainedSize;
            }
        }

        if (Buffer->dropCommandEnabled)
        {
            Buffer->dropCommandEnabled = gcvFALSE;
            /* Force to set current command buffer full,
             * so it would start with next command for
             * resuming command submission.
             */
            commandBuffer->startOffset = commandBuffer->bytes;
            commandBuffer->offset      = commandBuffer->bytes;
            commandBuffer->free        = 0;

        }
        Buffer->captureBuffer = gcvNULL;
        Buffer->captureEnabled = gcvFALSE;
    }

    gcmFOOTER();
    return status;
}


gceSTATUS gcoBUFFER_IsCaptureEnabled(
    gcoBUFFER Buffer
    )
{
    gceSTATUS status = gcvSTATUS_FALSE;
    gcmHEADER_ARG("Buffer=%p", Buffer);

    status = Buffer && Buffer->captureEnabled && Buffer->dropCommandEnabled? gcvSTATUS_TRUE : gcvSTATUS_FALSE;

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoBUFFER_CaptureInitState(
    IN gcoBUFFER Buffer,
    IN OUT gctUINT8 *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *pOutputSizeInByte
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoCMDBUF commandBuffer = gcvNULL;
    gctUINT8 *src = gcvNULL, *dst = gcvNULL;
    gctUINT32 commandSize = 0;
    gctUINT32 captureCommandOffset = 0;

    gcmHEADER_ARG("Buffer=%p CaptureBuffer=0x%x InputSizeInByte=%d"
                  "pOutputSizeInByte=0x%x", Buffer, CaptureBuffer, InputSizeInByte, pOutputSizeInByte);

    commandBuffer = Buffer->commandBufferTail;
    gcmASSERT(commandBuffer);

    captureCommandOffset = Buffer->info.reservedHead;
    commandSize = commandBuffer->offset - captureCommandOffset;

    src = (gctUINT8_PTR)gcmUINT64_TO_PTR(commandBuffer->logical) + captureCommandOffset;
    dst = (gctUINT8_PTR)CaptureBuffer;

    gcoOS_MemCopy(dst, src, commandSize);

    *pOutputSizeInByte = commandSize;

    gcmFOOTER();
    return status;
}

#endif  /* gcdENABLE_3D */
