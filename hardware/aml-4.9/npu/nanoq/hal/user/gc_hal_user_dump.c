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
#include "gc_hal_user_debug.h"
#include "gc_hal_dump.h"

#if gcdDUMP_FRAMERATE
static gctINT32 totalFrames = -gcdDUMP_FRAMERATE;
static gctINT32 previousTime = 0;
static gctINT32 totalTicks = 0;
#endif

#if defined(ANDROID)
#include <android/log.h>
#include <pthread.h>
#include <sys/syscall.h>
#include "log/log.h"

static gctUINT32 prevThreadID = 0;

#define gcmGETTHREADID() \
    (gctUINT32)pthread_self()

#endif

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_OS

static gctBOOL setDumpFlag  = gcvFALSE;

static gctPOINTER dump_mutex = gcvNULL;

#define gcmLOCKDUMP() \
{ \
    if (dump_mutex == gcvNULL) \
        gcmVERIFY_OK(gcoOS_CreateMutex(gcvNULL, &dump_mutex)); \
    gcmVERIFY_OK(gcoOS_AcquireMutex(gcvNULL, dump_mutex, gcvINFINITE)); \
}

#define gcmUNLOCKDUMP() \
  gcmVERIFY_OK(gcoOS_ReleaseMutex(gcvNULL, dump_mutex));

/*******************************************************************************
** New dump code.
*/

/*******************************************************************************
**
**  gcoOS_SetDumpFlag
**
**  Enabel dump or not
**
**  INPUT:
**
**      gctBOOL DumpState
**          Dump state to set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_SetDumpFlag(
    IN gctBOOL DumpState
    )
{
    setDumpFlag = DumpState;
    return gcvSTATUS_OK;
}

void
gcoOS_DumpLock(
    void
    )
{
    gcmLOCKDUMP();
}

void
gcoOS_DumpUnlock(
    void
    )
{
    gcmUNLOCKDUMP();
}

#if gcdDUMP
gceSTATUS
gcoOS_Dump(
    IN gcoOS Os,
    IN gctCONST_STRING Message,
    ...
    )
{
    gctUINT offset = 0;
    gctARGUMENTS args;
    char buffer[512];
#if gcdDUMP_IN_KERNEL
    gcsHAL_INTERFACE ioctl;
#endif

    if (!setDumpFlag)
    {
        return gcvSTATUS_OK;
    }

#if gcdDUMP_IN_KERNEL
    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, sizeof(buffer) - 1, &offset, Message, args));
    gcmARGUMENTS_END(args);

    ioctl.ignoreTLS   = gcvFALSE;
    ioctl.command     = gcvHAL_DEBUG_DUMP;
    ioctl.u.DebugDump.type    = gcvDUMP_BUFFER_USER_STRING;
    ioctl.u.DebugDump.ptr     = gcmPTR_TO_UINT64(buffer);
    ioctl.u.DebugDump.address = ~0U; /* ignored. */
    ioctl.u.DebugDump.size    = offset + 1; /* include tailing \0. */

    gcmVERIFY_OK(gcoOS_DeviceControl(Os,
                                     IOCTL_GCHAL_INTERFACE,
                                     &ioctl, gcmSIZEOF(ioctl),
                                     &ioctl, gcmSIZEOF(ioctl)));
#else
    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
                                     &offset,
                                     Message, args));
    gcmARGUMENTS_END(args);

    gcoOS_Print("%s", buffer);
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DumpBuffer(
    IN gcoOS Os,
    IN gceDUMP_BUFFER_TYPE Type,
    IN gctUINT32 Physical,
    IN gctPOINTER Logical,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes
    )
{
    gctUINT32_PTR ptr = (gctUINT32_PTR) Logical + (Offset >> 2);
    gctUINT32 phys    = Physical + (Offset & ~3);
    gctSIZE_T bytes   = Bytes;

#if gcdDUMP_IN_KERNEL
    gcsHAL_INTERFACE ioctl;
#else
    static gctCONST_STRING tagString[] =
    {
        "string",
        "verify",
        "memory",
        "texture",
        "stream",
        "index",
        "bufobj",
        "image",
        "memory",   /* use memory for instruction. */
        "context",
        "command",
        "async",
    };

    gctUINT8_PTR bytePtr = gcvNULL;

    gcmSTATIC_ASSERT(gcvDUMP_BUFFER_ASYNC_COMMAND == gcmCOUNTOF(tagString) - 1,
                     "tagString array does not match buffer types");
#endif

    if (!setDumpFlag)
    {
        return gcvSTATUS_OK;
    }

    if (Type >= gcvDUMP_BUFFER_USER_TYPE_LAST)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

#if gcdDUMP_IN_KERNEL
    ioctl.ignoreTLS = gcvFALSE;
    ioctl.command   = gcvHAL_DEBUG_DUMP;
    ioctl.u.DebugDump.type    = (gctUINT32)Type;
    ioctl.u.DebugDump.ptr     = gcmPTR_TO_UINT64(ptr);
    ioctl.u.DebugDump.address = phys;
    ioctl.u.DebugDump.size    = (gctUINT32)bytes;

    gcmVERIFY_OK(gcoOS_DeviceControl(Os,
        IOCTL_GCHAL_INTERFACE,
        &ioctl, gcmSIZEOF(ioctl),
        &ioctl, gcmSIZEOF(ioctl)));
#else
    gcmLOCKDUMP();

    gcmDUMP(Os, "@[%s 0x%08X 0x%08X",
            tagString[Type], phys, bytes);

    while (bytes >= 16)
    {
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X 0x%08X",
                ptr[0], ptr[1], ptr[2], ptr[3]);

        ptr   += 4;
        bytes -= 16;
    }

    switch (bytes)
    {
    case 15:
        bytePtr = (gctUINT8 *)&ptr[3];
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X 0x00%02X%02X%02X", ptr[0], ptr[1], ptr[2], bytePtr[2], bytePtr[1], bytePtr[0]);
        break;

    case 14:
        bytePtr = (gctUINT8 *)&ptr[3];
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X 0x0000%02X%02X", ptr[0], ptr[1], ptr[2], bytePtr[1], bytePtr[0]);
        break;

    case 13:
        bytePtr = (gctUINT8 *)&ptr[3];
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X 0x000000%02X", ptr[0], ptr[1], ptr[2], bytePtr[0]);
        break;

    case 12:
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X", ptr[0], ptr[1], ptr[2]);
        break;

    case 11:
        bytePtr = (gctUINT8 *)&ptr[2];
        gcmDUMP(Os, "  0x%08X 0x%08X 0x00%02X%02X%02X", ptr[0], ptr[1], bytePtr[2], bytePtr[1], bytePtr[0]);
        break;

    case 10:
        bytePtr = (gctUINT8 *)&ptr[2];
        gcmDUMP(Os, "  0x%08X 0x%08X 0x0000%02X%02X", ptr[0], ptr[1], bytePtr[1], bytePtr[0]);
        break;

    case 9:
        bytePtr = (gctUINT8 *)&ptr[2];
        gcmDUMP(Os, "  0x%08X 0x%08X 0x000000%02X", ptr[0], ptr[1], bytePtr[0]);
        break;

    case 8:
        gcmDUMP(Os, "  0x%08X 0x%08X", ptr[0], ptr[1]);
        break;

    case 7:
        bytePtr = (gctUINT8 *)&ptr[1];
        gcmDUMP(Os, "  0x%08X 0x00%02X%02X%02X", ptr[0], bytePtr[2], bytePtr[1], bytePtr[0]);
        break;

    case 6:
        bytePtr = (gctUINT8 *)&ptr[1];
        gcmDUMP(Os, "  0x%08X 0x0000%02X%02X", ptr[0], bytePtr[1], bytePtr[0]);
        break;

    case 5:
        bytePtr = (gctUINT8 *)&ptr[1];
        gcmDUMP(Os, "  0x%08X 0x000000%02X", ptr[0], bytePtr[0]);
        break;

    case 4:
        gcmDUMP(Os, "  0x%08X", ptr[0]);
        break;

    case 3:
        bytePtr = (gctUINT8 *)&ptr[0];
        gcmDUMP(Os, "  0x00%02X%02X%02X", bytePtr[2], bytePtr[1], bytePtr[0]);
        break;

    case 2:
        bytePtr = (gctUINT8 *)&ptr[0];
        gcmDUMP(Os, "  0x0000%02X%02X", bytePtr[1], bytePtr[0]);
        break;

    case 1:
        bytePtr = (gctUINT8 *)&ptr[0];
        gcmDUMP(Os, "  0x000000%02X", bytePtr[0]);
        break;

    default:
        break;
    }

    gcmDUMP(Os, "] -- %s", tagString[Type]);

    gcmUNLOCKDUMP();
#endif

    return gcvSTATUS_OK;
}
#else
gceSTATUS
gcoOS_Dump(
    IN gcoOS Os,
    IN gctCONST_STRING Message,
    ...
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DumpBuffer(
    IN gcoOS Os,
    IN gceDUMP_BUFFER_TYPE Type,
    IN gctUINT32 Physical,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}
#endif

gceSTATUS
gcoOS_DumpFrameRate(
    void
    )
{
#if gcdDUMP_FRAMERATE
    gctINT32 averageFPS = 0;
    gctINT32 currentTime = gcoOS_GetTicks();
    if (totalFrames > 0) {
        gctINT32 timePassed = currentTime - previousTime;
        totalTicks += timePassed;
        averageFPS = 1000 / (totalTicks/totalFrames);
        gcoOS_Print("Average FPS is :%d for %d frames", averageFPS, totalFrames);
    }
    totalFrames ++;
    previousTime = currentTime;
#endif
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DumpApi(
    IN gctCONST_STRING Message,
    ...
    )
{
    char buffer[512];
    gctUINT offset = 0;
    gctARGUMENTS args;

    if (!setDumpFlag)
        return gcvSTATUS_OK;

#ifdef ANDROID
    gctUINT32 threadID;

    if (!Message)
    return gcvSTATUS_OK;

    threadID = gcmGETTHREADID();
    if ((prevThreadID != 0) && (prevThreadID != threadID))
    {
        //LOGD("!!!!!!!!!!!!Terri: tid %d vs %d\n!!!!!!!!!!!!!!!\n", prevThreadID, threadID);
        //return gcvSTATUS_OK;
    }
    prevThreadID = threadID;

    if (strncmp(Message, "${", 2) == 0)
        gcmLOCKDUMP();

    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
                                     &offset,
                                     Message, args));
    gcmARGUMENTS_END(args);
    gcoOS_Print(buffer);

    if (strstr(Message, "}"))
        gcmUNLOCKDUMP();

#else
    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
                                     &offset,
                                     Message, args));
    gcmARGUMENTS_END(args);
    gcoOS_Print(buffer);
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DumpArray(
    IN gctCONST_POINTER Data,
    IN gctUINT32 Size
    )
{
    const gctUINT32_PTR data = (gctUINT32_PTR) Data;

    if (!setDumpFlag)
        return gcvSTATUS_OK;

    if (Size > 0)
    {
        if (Data == gcvNULL)
        {
            gcoOS_DumpApi("$$ <nil>");
        }
        else
        {
            gctUINT index;

            for (index = 0; index < Size;)
            {
                switch (Size - index)
                {
                case 1:
                    gcoOS_DumpApi("$$ 0x%p: 0x%08X", data + index, data[index]);
                    index += 1;
                    break;
                case 2:
                    gcoOS_DumpApi("$$ 0x%p: 0x%08X 0x%08X",
                               data + index, data[index], data[index + 1]);
                    index += 2;
                    break;
                case 3:
                    gcoOS_DumpApi("$$ 0x%p: 0x%08X 0x%08X 0x%08X",
                               data + index, data[index], data[index + 1],
                               data[index + 2]);
                    index += 3;
                    break;
                default:
                    gcoOS_DumpApi("$$ 0x%p: 0x%08X 0x%08X 0x%08X 0x%08X",
                               data + index, data[index], data[index + 1],
                               data[index + 2], data[index + 3]);
                    index += 4;
                    break;
                }
            }
        }

        gcoOS_DumpApi("$$ **********");
    }

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DumpArrayToken(
    IN gctCONST_POINTER Data,
    IN gctUINT32 Termination
    )
{
    const gctUINT32_PTR data = (gctUINT32_PTR) Data;

    if (!setDumpFlag)
        return gcvSTATUS_OK;

    if (Data == gcvNULL)
    {
        gcoOS_DumpApi("$$ <nil>");
    }
    else
    {
        gctUINT index;

        for (index = 0; data[index] != Termination; index += 2)
        {
            gcoOS_DumpApi("$$ 0x%p: 0x%08X 0x%08X",
                       data + index, data[index], data[index + 1]);
        }
        gcoOS_DumpApi("$$ 0x%p: 0x%08X", data + index, Termination);
    }

    gcoOS_DumpApi("$$ **********");

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DumpApiData(
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Size
    )
{
    const gctUINT8_PTR data = (gctUINT8_PTR) Data;

    if (!setDumpFlag)
        return gcvSTATUS_OK;

    if (Data == gcvNULL)
    {
        gcoOS_DumpApi("$$ <nil>");
    }
    else
    {
        gctSIZE_T index;

        if (Size == 0)
        {
            Size = gcoOS_StrLen(Data, gcvNULL) + 1;
        }

        for (index = 0; index < Size;)
        {
            switch (Size - index)
            {
            case 1:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X", data + index, data[index]);
                index += 1;
                break;

            case 2:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1]);
                index += 2;
                break;

            case 3:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2]);
                index += 3;
                break;

            case 4:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X 0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3]);
                index += 4;
                break;

            case 5:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4]);
                index += 5;
                break;

            case 6:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
                           "0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4], data[index + 5]);
                index += 6;
                break;

            case 7:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
                           "0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4], data[index + 5],
                           data[index + 6]);
                index += 7;
                break;

            default:
                gcoOS_DumpApi("$$ 0x%p: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
                           "0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4], data[index + 5],
                           data[index + 6], data[index + 7]);
                index += 8;
                break;
            }
        }
    }

    gcoOS_DumpApi("$$ **********");

    return gcvSTATUS_OK;
}

#if gcdDUMP_2D

typedef struct _gcsDumpMemInfoNode
{
    gctUINT32                   gpuAddress;
    gctPOINTER                  logical;
    gctUINT64                   physical;
    gctUINT32                   size;
    struct _gcsDumpMemInfoNode *prev;
    struct _gcsDumpMemInfoNode *next;
}
gcsDumpMemInfoNode;

static gcsDumpMemInfoNode dumpMemInfoList = {
    0, gcvNULL, gcvINVALID_PHYSICAL_ADDRESS, 0, &dumpMemInfoList, &dumpMemInfoList,
};

gctPOINTER dumpMemInfoListMutex = gcvNULL;
gctBOOL    dump2DFlag           = gcv2D_STATE_PROFILE_ALL;

gceSTATUS
gcoOS_Dump2DCommand(
    IN gctUINT32_PTR Command,
    IN gctUINT32 Size
    )
{
    gctUINT32 index;

    if (!(dump2DFlag & gcv2D_STATE_PROFILE_COMMAND))
    {
        return gcvSTATUS_OK;
    }

    /* Dump the 2D command buffer. */
    gcoOS_DumpApi("$$[2D Command Buffer, size = %d bytes]", Size * 4);

    for (index = 0; index < Size; index++)
    {
        gcoOS_DumpApi("$$ 0x%08X", Command[index]);
    }

    gcoOS_DumpApi("$$ **********");

    gcoHAL_Commit(gcvNULL, gcvTRUE);

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_Dump2DSurface(
    IN gctBOOL Src,
    IN gctUINT32 Address
    )
{
    gceSTATUS status;
    gcsDumpMemInfoNode *pMemory;

    gctUINT32 physical = 0;
    gctUINT32 size = 0, offset = 0, index = 0;
    gctPOINTER logical = gcvNULL;
    gctUINT32_PTR data;
    gctBOOL mapped = gcvFALSE;

    if (!(dump2DFlag & gcv2D_STATE_PROFILE_SURFACE))
    {
        return gcvSTATUS_OK;
    }

    /* Acquire the mutex. */
    gcmONERROR(gcoOS_AcquireMutex(gcvNULL, dumpMemInfoListMutex, gcvINFINITE));

    /* Loop the list to query the logical address. */
    for (pMemory = dumpMemInfoList.next; pMemory != &dumpMemInfoList; pMemory = pMemory->next)
    {
        size = pMemory->size;

        if ((Address >= pMemory->gpuAddress) && (Address < (pMemory->gpuAddress + size)))
        {
            offset = Address - pMemory->gpuAddress;
            size   = size - offset;

            if (pMemory->logical != gcvNULL)
            {
                physical = 0;
                logical  = gcmINT2PTR(gcmPTR2INT32(pMemory->logical) + offset);
            }
            else
            {
                physical = pMemory->physical + offset;
                logical  = gcvNULL;
            }

            gcmONERROR(gcoOS_ReleaseMutex(gcvNULL, dumpMemInfoListMutex));
            goto Found;
        }
    }

    /* Release the mutex. */
    gcmONERROR(gcoOS_ReleaseMutex(gcvNULL, dumpMemInfoListMutex));

    /* Not found. */
    gcmONERROR(gcvSTATUS_NOT_FOUND);

Found:
    /* Map it into the user space if we only have the physical address. */
    if (logical == gcvNULL)
    {
        gcmONERROR(gcoHAL_MapMemory(gcvNULL, physical, size, &logical));
        mapped = gcvTRUE;
    }

    /* Dump the 2D surface. */
    gcoOS_DumpApi("$$[2D %s Surface, GPU Address = 0x%08X, offset = %d, size = %d bytes]",
        Src ? "Src" : "Dst", Address, offset, size);

    size /= 4;
    data = (gctUINT32_PTR)logical;

    while (index < size)
    {
        switch (size - index)
        {
        case 1:
            gcoOS_DumpApi("$$ 0x%08X", data[index]);
            index += 1;
            break;
        case 2:
            gcoOS_DumpApi("$$ 0x%08X 0x%08X", data[index], data[index + 1]);
            index += 2;
            break;
        case 3:
            gcoOS_DumpApi("$$ 0x%08X 0x%08X 0x%08X",
                    data[index], data[index + 1], data[index + 2]);
            index += 3;
            break;
        default:
            gcoOS_DumpApi("$$ 0x%08X 0x%08X 0x%08X 0x%08X",
                    data[index], data[index + 1], data[index + 2], data[index + 3]);
            index += 4;
            break;
        }
    }

    gcoOS_DumpApi("$$ **********");

    /* Unmap the memory. */
    if (mapped)
    {
        gcmVERIFY_OK(gcoHAL_UnmapMemory(gcvNULL, physical, size, logical));
    }

    return gcvSTATUS_OK;

OnError:
    /* Dump the info. */
    gcoOS_DumpApi("$$[2D %s Surface, GPU Address = 0x%08X -- not found]",
        Src ? "Src" : "Dst", Address);
    gcoOS_DumpArray(gcvNULL, 0);

    return status;
}

gceSTATUS
gcfAddMemoryInfo(
    IN gctUINT32 GPUAddress,
    IN gctPOINTER Logical,
    IN gctUINT64 Physical,
    IN gctUINT32 Size
    )
{
    gceSTATUS status;
    gcsDumpMemInfoNode *pNode, *prev;
    gctPOINTER pointer = gcvNULL;

    gcmASSERT(dumpMemInfoListMutex != gcvNULL);

    /* Allocate the node structure. */
    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(*pNode), &pointer));

    pNode = pointer;

    /* Setup the structure. */
    pNode->gpuAddress = GPUAddress;
    pNode->logical    = Logical;
    pNode->physical   = Physical;
    pNode->size       = Size;

    /* Acquire the mutex. */
    gcmONERROR(gcoOS_AcquireMutex(gcvNULL, dumpMemInfoListMutex, gcvINFINITE));

    /* Add the new node into the list. */
    prev                 = dumpMemInfoList.prev;

    dumpMemInfoList.prev = pNode;
    pNode->next          = &dumpMemInfoList;
    pNode->prev          = prev;
    prev->next           = pNode;

    /* Release the mutex. */
    gcmONERROR(gcoOS_ReleaseMutex(gcvNULL, dumpMemInfoListMutex));

    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS
gcfDelMemoryInfo(
    IN gctUINT32 Address
    )
{
    gceSTATUS status;
    gcsDumpMemInfoNode *pNode, *prev, *next;

    /* Acquire the mutex. */
    gcmONERROR(gcoOS_AcquireMutex(gcvNULL, dumpMemInfoListMutex, gcvINFINITE));

    /* Loop the list. */
    for (pNode = dumpMemInfoList.next; pNode != &dumpMemInfoList; pNode = pNode->next)
    {
        /* Delete the node from the list. */
        if (pNode->gpuAddress == Address)
        {
            prev       = pNode->prev;
            next       = pNode->next;
            next->prev = prev;
            prev->next = next;

            gcoOS_Free(gcvNULL, pNode);
            break;
        }
    }

    /* Release the mutex. */
    gcmONERROR(gcoOS_ReleaseMutex(gcvNULL, dumpMemInfoListMutex));

    return gcvSTATUS_OK;

OnError:
    return status;
}

#else

gceSTATUS
gcoOS_Dump2DCommand(
    IN gctUINT32_PTR Command,
    IN gctUINT32 Size
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_Dump2DSurface(
    IN gctBOOL Src,
    IN gctUINT32 Address
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcfAddMemoryInfo(
    IN gctUINT32 GPUAddress,
    IN gctPOINTER Logical,
    IN gctUINT64 Physical,
    IN gctUINT32 Size
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcfDelMemoryInfo(
    IN gctUINT32 Address
    )
{
    return gcvSTATUS_OK;
}

#endif /* gcdDUMP_2D */
