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

#define _GC_OBJ_ZONE            gcdZONE_VERTEXARRAY
#define WCLIP_DATA_LIMIT        5000
#define SPILIT_INDEX_OFFSET       48
#define SPILIT_INDEX_CHUNCK_BYTE  64

#if gcdUSE_WCLIP_PATCH
#define readDataForWLimit(Logical, result)                               \
{                                                                        \
    gctUINT8* ptr;                                                       \
    gctUINT32 data;                                                      \
    ptr = (gctUINT8*)(Logical);                                          \
    data = ((ptr[0]) | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3]) << 24); \
    result = *((gctFLOAT*)(&data));                                      \
}

gceSTATUS
computeWLimit(gctFLOAT_PTR Logical,
        gctUINT Components,
        gctUINT StrideInBytes,
        gctUINT WLimitCount,
        gctFLOAT * wLimitRms,
        gctBOOL * wLimitRmsDirty)
{
    gctFLOAT bboxMin[3], bboxMax[3], rms = 0.f;
    gctUINT i, j;
    gctUINT upLimit;

    gcmHEADER();

    gcmDEBUG_VERIFY_ARGUMENT(Components > 1);
    gcmDEBUG_VERIFY_ARGUMENT(StrideInBytes > 0);

    upLimit = WLimitCount;
    upLimit = upLimit > WCLIP_DATA_LIMIT ? WCLIP_DATA_LIMIT : upLimit;

    if((((gcmPTR2SIZE(Logical)) & 3) == 0) && ((StrideInBytes & 3) == 0 ))
    {
        /* Initialize bbox to first entry. */
        for (j = 0; j < Components; j++)
        {
            bboxMin[j] = bboxMax[j] = Logical[j];
        }

        for (i = 0; i < upLimit; i++)
        {
            for (j = 0; j < Components; j++)
            {
                if (Logical[j] < bboxMin[j])
                {
                    bboxMin[j] = Logical[j];
                }
                else if (Logical[j] > bboxMax[j])
                {
                    bboxMax[j] = Logical[j];
                }

                if ((bboxMax[j] - bboxMin[j]) > 0.01f)
                {
                    *wLimitRms = 1.0f;
                    *wLimitRmsDirty = gcvFALSE;

                    gcmFOOTER_NO();
                    return gcvSTATUS_OK;
                }
            }
            Logical = (gctFLOAT_PTR)((gctUINTPTR_T)Logical + StrideInBytes);
        }
    }
    else
    {
        gctFLOAT value;

        gcmTRACE_ZONE(gcvLEVEL_WARNING, _GC_OBJ_ZONE,
                      "Logical address is not 4 byte aligned.");

        /* Initialize bbox to first entry. */
        for (j = 0; j < Components; j++)
        {
            readDataForWLimit(Logical + j, value);
            bboxMin[j] = bboxMax[j] = value;
        }

        for (i = 0; i < upLimit; i++)
        {
            for (j = 0; j < Components; j++)
            {
                readDataForWLimit(Logical + j, value);

                if (value < bboxMin[j])
                {
                    bboxMin[j] = value;
                }
                else if (value > bboxMax[j])
                {
                    bboxMax[j] = value;
                }

                if ((bboxMax[j] - bboxMin[j]) > 0.01f)
                {
                    *wLimitRms = 1.0f;
                    *wLimitRmsDirty = gcvFALSE;

                    gcmFOOTER_NO();
                    return gcvSTATUS_OK;
                }
            }

            Logical = (gctFLOAT_PTR)((gctUINTPTR_T)Logical + StrideInBytes);
        }
    }

    for (j = 0; j < Components; j++)
    {
        rms += (bboxMax[j] - bboxMin[j]) * (bboxMax[j] - bboxMin[j]);
    }

    if (rms < 1.0f)
    {
        rms = gcoMATH_SquareRoot(rms);

        *wLimitRms = rms;
        *wLimitRmsDirty = gcvTRUE;
    }
    else
    {
        *wLimitRms = 1.0f;
        *wLimitRmsDirty = gcvFALSE;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
#endif

#define gcmCOMPUTE_BYTES()                                              \
    if (vertexPtr->enable)                                              \
    {                                                                   \
        switch (vertexPtr->format)                                      \
        {                                                               \
        case gcvVERTEX_BYTE:                                            \
        case gcvVERTEX_UNSIGNED_BYTE:                                   \
        case gcvVERTEX_INT8:                                            \
            bytes = vertexPtr->size;                                    \
            break;                                                      \
        case gcvVERTEX_UNSIGNED_INT_10_10_10_2:                         \
        case gcvVERTEX_INT_10_10_10_2:                                  \
        case gcvVERTEX_UNSIGNED_INT_2_10_10_10_REV:                     \
        case gcvVERTEX_INT_2_10_10_10_REV:                              \
            /* 10_10_10_2 format is always in 4 bytes container. */     \
            bytes = 4;                                                  \
            break;                                                      \
        case gcvVERTEX_SHORT:                                           \
        case gcvVERTEX_UNSIGNED_SHORT:                                  \
        case gcvVERTEX_HALF:                                            \
        case gcvVERTEX_INT16:                                           \
            bytes = vertexPtr->size * 2;                                \
            break;                                                      \
        case gcvVERTEX_INT32:                                           \
            bytes = vertexPtr->size * 4;                                \
            break;                                                      \
        default:                                                        \
            bytes = vertexPtr->size * 4;                                \
            break;                                                      \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        bytes = vertexPtr->genericSize * 4;                             \
    }


#define gcmCOMPUTE_INDEX_BYTES(Count)                                   \
    switch (IndexType)                                                  \
    {                                                                   \
    case gcvINDEX_8:                                                    \
        bytes = Count;                                                  \
        break;                                                          \
    case gcvINDEX_16:                                                   \
        bytes = Count * 2;                                              \
        break;                                                          \
    case gcvINDEX_32:                                                   \
        bytes = Count * 4;                                              \
        break;                                                          \
    default:                                                            \
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);                         \
    }

/*******************************************************************************
***** gcoVERTEX Object ********************************************************/

struct _gcoVERTEXARRAY
{
    /* The object. */
    gcsOBJECT                   object;

    /* Hardware capabilities. */
    gctUINT                     maxAttribute;
    gctUINT                     maxStride;
    gctUINT                     maxStreams;
    gctUINT                     maxAttribOffset;

    /* Dynamic streams. */
    gcoSTREAM                   dynamicStream;
    gcoINDEX                    dynamicIndex;

    /* Uncacheable stream. */
    gcoSTREAM                   uncacheableStream;

    struct
    {
        gctUINT32               hasLineLoop                   : 1;
        gctUINT32               hasIndexedTriangleStripFix    : 1;
        gctUINT32               hasMixedStreams               : 1;
    }hwFeature;
};

gceSTATUS
gcoVERTEXARRAY_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEXARRAY * Vertex
    )
{
    gceSTATUS status;
    gcoVERTEXARRAY vertexPtr = gcvNULL;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Vertex != gcvNULL);

    /* Allocate the gcoVERTEXARRAY object structure. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(struct _gcoVERTEXARRAY),
                              &pointer));

    vertexPtr = pointer;

    /* Initialize the object. */
    vertexPtr->object.type = gcvOBJ_VERTEX;

    /* Initialize the structure. */
    vertexPtr->dynamicStream = gcvNULL;
    vertexPtr->dynamicIndex  = gcvNULL;
    vertexPtr->uncacheableStream = gcvNULL;

    /* Construct a cacheable stream. */
    gcmONERROR(gcoSTREAM_Construct(gcvNULL, &vertexPtr->dynamicStream));

    /* Construct a dynamic index. */
    gcmONERROR(gcoINDEX_Construct(gcvNULL, &vertexPtr->dynamicIndex));

    /* Query the stream capabilities. */
    gcmONERROR(gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                           &vertexPtr->maxAttribute,
                                           &vertexPtr->maxStride,
                                           &vertexPtr->maxStreams,
                                           gcvNULL,
                                           &vertexPtr->maxAttribOffset));


    vertexPtr->hwFeature.hasLineLoop = gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_LINE_LOOP);
    vertexPtr->hwFeature.hasIndexedTriangleStripFix = gcoHAL_IsFeatureAvailable(gcvNULL,
        gcvFEATURE_BUG_FIXED_INDEXED_TRIANGLE_STRIP);
    vertexPtr->hwFeature.hasMixedStreams = gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_MIXED_STREAMS);



    /* Return the gcoVERTEXARRAY object. */
    *Vertex = vertexPtr;

    /* Success. */
    gcmFOOTER_ARG("*Vertex=0x%x", *Vertex);
    return gcvSTATUS_OK;

OnError:
    if (vertexPtr != gcvNULL)
    {
        if (vertexPtr->dynamicStream != gcvNULL)
        {
            /* Destroy the dynamic stream. */
            gcmVERIFY_OK(gcoSTREAM_Destroy(vertexPtr->dynamicStream));
        }

        if (vertexPtr->dynamicIndex != gcvNULL)
        {
            /* Destroy the dynamic index. */
            gcmVERIFY_OK(gcoINDEX_Destroy(vertexPtr->dynamicIndex));
        }

        /* Free the gcoVERTEXARRAY object. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, vertexPtr));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVERTEXARRAY_Destroy(
    IN gcoVERTEXARRAY Vertex
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    if (Vertex->dynamicStream != gcvNULL)
    {
        /* Destroy the dynamic stream. */
        gcmONERROR(gcoSTREAM_Destroy(Vertex->dynamicStream));
        Vertex->dynamicStream = gcvNULL;
    }

    if (Vertex->dynamicIndex != gcvNULL)
    {
        /* Destroy the dynamic index. */
        gcmONERROR(gcoINDEX_Destroy(Vertex->dynamicIndex));
        Vertex->dynamicIndex = gcvNULL;
    }

    if (Vertex->uncacheableStream != gcvNULL)
    {
        /* Destroy the uncacheable stream. */
        gcmONERROR(gcoSTREAM_Destroy(Vertex->uncacheableStream));
        Vertex->uncacheableStream = gcvNULL;
    }

    /* Free the gcoVERTEX object. */
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Vertex));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
_SpilitIndex(
    IN gcoBUFOBJ Index,
    IN gcoINDEX _Index,
    IN gctPOINTER IndexMemory,
    IN gceINDEX_TYPE IndexType,
    IN gcePRIMITIVE * PrimitiveType,
    IN gctSIZE_T Count,
    IN OUT gctSIZE_T * SpilitCount
    )
{
    gceSTATUS status = gcvSTATUS_TRUE;
    gctUINT32 address = 0, tempAddress;
    gctUINT32 indexSize = 0;
    gctUINT32 spilitIndexMod;
    gctSIZE_T cutCount = 0;

    gcmHEADER_ARG("Index=0x%x IndexMemory=0x%x IndexType=%u PrimitiveType=%u SpilitCount=%u ",
                  Index, IndexMemory, IndexType, gcmOPT_VALUE(PrimitiveType), gcmOPT_VALUE(SpilitCount));

    if (Index)
    {
        gcmONERROR(gcoBUFOBJ_Lock(Index, &address, gcvNULL));
        /* Add offset */
        address += gcmPTR2INT32(IndexMemory);
        /* Unlock the bufobj buffer. */
        gcmONERROR(gcoBUFOBJ_Unlock(Index));
    }
    else if (_Index)
    {
        gcmONERROR(gcoINDEX_Lock(_Index, &address, gcvNULL));
        /* Add offset */
        address += gcmPTR2INT32(IndexMemory);
        /* Unlock the bufobj buffer. */
        gcmONERROR(gcoINDEX_Unlock(_Index));
    }


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

    if (*PrimitiveType == gcvPRIMITIVE_TRIANGLE_LIST)
    {
        cutCount = Count % 3;
    }
    else if (*PrimitiveType == gcvPRIMITIVE_LINE_LIST)
    {
        cutCount = Count % 2;
    }

    /* compute the last index address.*/
    tempAddress = address + (gctUINT32)(Count-cutCount-1) * indexSize;
    spilitIndexMod = tempAddress % SPILIT_INDEX_CHUNCK_BYTE;

    if (spilitIndexMod >= SPILIT_INDEX_OFFSET)
    {
        gcmFOOTER();
        return gcvSTATUS_FALSE;
    }

    switch (*PrimitiveType)
    {
        case gcvPRIMITIVE_POINT_LIST:
        case gcvPRIMITIVE_LINE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_FAN:
            *SpilitCount = spilitIndexMod /indexSize +1;
            break;
        case gcvPRIMITIVE_LINE_LOOP:
            *SpilitCount = spilitIndexMod /indexSize +1;
            break;
        case gcvPRIMITIVE_TRIANGLE_LIST:
            *SpilitCount = ((spilitIndexMod /(indexSize*3))+1)*3 + cutCount;
            break;
        case gcvPRIMITIVE_LINE_LIST:
            *SpilitCount = ((spilitIndexMod /(indexSize*2))+1)*2 + cutCount;
            break;
        default:
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    gcmFOOTER();
    return gcvSTATUS_TRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
_CopySpilitIndex(
    IN gcoBUFOBJ Index,
    IN gcoINDEX _Index,
    IN gctPOINTER IndexMemory,
    IN gctUINT32 Offset,
    IN gceINDEX_TYPE IndexType,
    IN gcePRIMITIVE * PrimitiveType,
    IN gctPOINTER * Buffer,
    IN gctSIZE_T * SpilitCount
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER indexBase = gcvNULL;
    gctPOINTER tempIndices = gcvNULL;
    gctPOINTER indexMemory = gcvNULL;
    gctSIZE_T count, primCount, i, j, bytes;
    gctSIZE_T indexSize = 0;
    gcePATCH_ID patchId = gcvPATCH_INVALID;
    gctBOOL indexLocked;

    gcmHEADER_ARG("Index=0x%x IndexMemory=0x%x Offset=%u IndexType=%d "
                  "PrimitiveType=%u Buffer=0x%x SpilitCount=%u",
                  Index, IndexMemory, Offset, IndexType,
                  gcmOPT_VALUE(PrimitiveType), gcmOPT_VALUE(Buffer), gcmOPT_VALUE(SpilitCount));

    /* Is locked? */
    indexLocked = gcvFALSE;

    gcoHARDWARE_GetPatchID(gcvNULL, &patchId);

    /* Lock the index buffer. */
    if (Index)
    {
#if gcdSYNC
        if (patchId == gcvPATCH_GTFES30)
        {
            gcoBUFOBJ_WaitFence(Index, gcvFENCE_TYPE_WRITE);
        }
#endif
        gcmONERROR(gcoBUFOBJ_Lock(Index, gcvNULL, &indexBase));
    }
    else if (_Index)
    {
#if gcdSYNC
        if (patchId == gcvPATCH_GTFES30)
        {
            gcoINDEX_WaitFence(_Index, gcvFENCE_TYPE_WRITE);
        }
#endif
        gcmONERROR(gcoINDEX_Lock(_Index, gcvNULL, &indexBase));
    }
    indexLocked = gcvTRUE;

    indexBase = (gctUINT8_PTR)indexBase + gcmPTR2INT32(IndexMemory);

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

    bytes = *SpilitCount * indexSize;

    switch (*PrimitiveType)
    {
        case gcvPRIMITIVE_POINT_LIST:
        case gcvPRIMITIVE_LINE_LIST:
        case gcvPRIMITIVE_TRIANGLE_LIST:
            {
                indexMemory = (gctUINT8_PTR)indexBase + Offset;

                gcmONERROR(gcoOS_Allocate(gcvNULL,
                                          bytes,
                                          &tempIndices));
                gcoOS_MemCopy(tempIndices , indexMemory, bytes);
            }
            break;
        case gcvPRIMITIVE_LINE_STRIP:
            {
                indexMemory = (gctUINT8_PTR)indexBase + Offset - indexSize;
                bytes += indexSize;
                gcmONERROR(gcoOS_Allocate(gcvNULL,
                                          bytes,
                                          &tempIndices));
                /* line strip need copy the last data */
                gcoOS_MemCopy(tempIndices , indexMemory, bytes);
            }
            break;
        case gcvPRIMITIVE_LINE_LOOP:
            {
                indexMemory = (gctUINT8_PTR)indexBase + Offset - indexSize;
                bytes += 2 * indexSize;
                gcmONERROR(gcoOS_Allocate(gcvNULL,
                                          bytes,
                                          &tempIndices));

                /* line loop need copy the last data and the first data */
                gcoOS_MemCopy(tempIndices , indexMemory, bytes - indexSize);
                gcoOS_MemCopy((gctUINT8_PTR)tempIndices + bytes - indexSize, indexBase, indexSize);
                *PrimitiveType = gcvPRIMITIVE_LINE_STRIP;
            }
            break;
        case gcvPRIMITIVE_TRIANGLE_STRIP:
            {
                primCount = bytes / indexSize;
                bytes = 3 * primCount * indexSize;
                gcmONERROR(gcoOS_Allocate(gcvNULL,
                                          bytes,
                                          &tempIndices));
                count = Offset / indexSize;
                switch (IndexType)
                {
                    case gcvINDEX_8:
                        {
                            gctUINT8_PTR src = (gctUINT8_PTR)indexBase;
                            gctUINT8_PTR dst = (gctUINT8_PTR)tempIndices;
                            for(i = 0, j = count - 2; i < primCount; i++, j++)
                            {
                                dst[i * 3]     = src[(j % 2) == 0? j : j + 1];
                                dst[i * 3 + 1] = src[(j % 2) == 0? j + 1 : j];
                                dst[i * 3 + 2] = src[j + 2];
                            }
                        }
                        break;
                    case gcvINDEX_16:
                        {
                            gctUINT16_PTR src = (gctUINT16_PTR)indexBase;
                            gctUINT16_PTR dst = (gctUINT16_PTR)tempIndices;
                            for(i = 0, j = count - 2; i < primCount; i++, j++)
                            {
                                dst[i * 3]     = src[(j % 2) == 0? j : j + 1];
                                dst[i * 3 + 1] = src[(j % 2) == 0? j + 1 : j];
                                dst[i * 3 + 2] = src[j + 2];
                            }
                        }
                        break;
                    case gcvINDEX_32:
                        {
                            gctUINT32_PTR src = (gctUINT32_PTR)indexBase;
                            gctUINT32_PTR dst = (gctUINT32_PTR)tempIndices;
                            for(i = 0, j = count - 2; i < primCount; i++, j++)
                            {
                                dst[i * 3]     = src[(j % 2) == 0? j : j + 1];
                                dst[i * 3 + 1] = src[(j % 2) == 0? j + 1 : j];
                                dst[i * 3 + 2] = src[j + 2];
                            }
                        }
                        break;
                    default:
                        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
                }
                *PrimitiveType = gcvPRIMITIVE_TRIANGLE_LIST;
            }
            break;
        case gcvPRIMITIVE_TRIANGLE_FAN:
            {
                indexMemory = (gctUINT8_PTR)indexBase + Offset - indexSize;
                bytes += 2 * indexSize;
                gcmONERROR(gcoOS_Allocate(gcvNULL,
                                          bytes,
                                          &tempIndices));

                /* trianglefan need copy the first data */
                gcoOS_MemCopy(tempIndices , indexBase, indexSize);
                gcoOS_MemCopy((gctUINT8_PTR)tempIndices + indexSize , indexMemory, bytes - indexSize);
            }
            break;
        default:
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    *Buffer = tempIndices;
    *SpilitCount = bytes / indexSize;

OnError:

    if (indexLocked)
    {
        /* Unlock the index buffer. */
        if (Index)
        {
            gcmONERROR(gcoBUFOBJ_Unlock(Index));
        }
        else if (_Index)
        {
            gcmONERROR(gcoINDEX_Unlock(_Index));
        }
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/* After move fakeLineLoop and convert triangle list to upper level,
** streamInfo can be removed and this fuc is the same as gcoVERTEXARRAY_IndexBind
** then, it can be merge with gcoVERTEXARRAY_IndexBind.*/
gceSTATUS
gcoVERTEXARRAY_IndexBind_Ex(
    IN gcoVERTEXARRAY Vertex,
    IN OUT gcsVERTEXARRAY_STREAM_INFO_PTR StreamInfo,
    IN gcsVERTEXARRAY_INDEX_INFO_PTR IndexInfo
    )
{
    gceSTATUS status                     = gcvSTATUS_OK;
    gctBOOL fakeLineLoop                 = gcvFALSE;
    gctBOOL convertToIndexedTriangleList = gcvFALSE;
    gctPOINTER indexBuffer               = gcvNULL;
    gceINDEX_TYPE indexType              = IndexInfo->indexType;
    gctUINT bytesPerIndex                = 0;
    gctUINT count32;
    gctUINT first                        = (gctUINT32)StreamInfo->first;

    gcmHEADER();

    gcmSAFECASTSIZET(count32, IndexInfo->count);

    switch (IndexInfo->indexType)
    {
    case gcvINDEX_8:
        bytesPerIndex = 1;
        break;
    case gcvINDEX_16:
        bytesPerIndex = 2;
        break;
    case gcvINDEX_32:
        bytesPerIndex = 4;
        break;
    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Check for hardware line loop support. */
    fakeLineLoop = (StreamInfo->primMode == gcvPRIMITIVE_LINE_LOOP)
                 ? !Vertex->hwFeature.hasLineLoop
                 : gcvFALSE;

    convertToIndexedTriangleList = (StreamInfo->primMode == gcvPRIMITIVE_TRIANGLE_STRIP)
                                && ((IndexInfo->u.es11.indexBuffer == gcvNULL) && (IndexInfo->indexMemory != gcvNULL))
                                && !Vertex->hwFeature.hasIndexedTriangleStripFix;


    /* Check if we need to fake the line loop */
    if (fakeLineLoop)
    {
        gctUINT bytes = 0;

        /* Check if there is an index object or buffer. */
        if ((IndexInfo->u.es11.indexBuffer != gcvNULL) || (IndexInfo->indexMemory != gcvNULL))
        {
            bytes = count32 * bytesPerIndex;
        }
        else
        {
            /* Check if the count fits in 8-bit. */
            if (first + count32 + 1 < 256)
            {
                /* 8-bit indices. */
                indexType     = gcvINDEX_8;
                bytes         = count32;
                bytesPerIndex = 1;
            }

            /* Check if the count fits in 16-bit. */
            else if (first + count32 + 1 < 65536)
            {
                /* 16-bit indices. */
                indexType     = gcvINDEX_16;
                bytes         = count32 * 2;
                bytesPerIndex = 2;
            }

            else
            {
                /* 32-bit indices. */
                indexType     = gcvINDEX_32;
                bytes         = count32 * 4;
                bytesPerIndex = 4;
            }
        }

        /* Allocate a temporary buffer. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  bytes + bytesPerIndex,
                                  &indexBuffer));

        /* Test if there is a gcoINDEX stream. */
        if (IndexInfo->u.es11.indexBuffer != gcvNULL)
        {
            gctPOINTER logical;

            /* Lock the index stream. */
            gcmONERROR(gcoINDEX_Lock(IndexInfo->u.es11.indexBuffer, gcvNULL, &logical));

            /* Copy the indices to the temporary memory. */
            gcoOS_MemCopy(indexBuffer,
                          (gctUINT8_PTR) logical
                          + gcmPTR2INT32(IndexInfo->indexMemory),
                          bytes);

            /* Append the first index to the temporary memory. */
            gcoOS_MemCopy((gctUINT8_PTR) indexBuffer + bytes,
                          (gctUINT8_PTR) logical
                          + gcmPTR2INT32(IndexInfo->indexMemory),
                          bytesPerIndex);
        }
        else if (IndexInfo->indexMemory != gcvNULL)
        {
            /* Copy the indices to the temporary memory. */
            gcoOS_MemCopy(indexBuffer, IndexInfo->indexMemory, bytes);

            /* Append the first index to the temporary memory. */
            gcoOS_MemCopy((gctUINT8_PTR) indexBuffer + bytes,
                          IndexInfo->indexMemory,
                          bytesPerIndex);
        }
        else
        {
            gctUINT i;

            /* Test for 8-bit indices. */
            if (bytesPerIndex == 1)
            {
                /* Cast pointer to index buffer. */
                gctUINT8_PTR ptr = (gctUINT8_PTR) indexBuffer;

                /* Fill index buffer with First through First + Count - 1. */
                for (i = 0; i < StreamInfo->count; ++i)
                {
                    *ptr++ = (gctUINT8) (first + i);
                }

                /* Append First to index buffer. */
                *ptr = (gctUINT8) first;
            }
            else if (bytesPerIndex == 2)
            {
                /* Cast pointer to index buffer. */
                gctUINT16_PTR ptr = (gctUINT16_PTR) indexBuffer;

                /* Fill index buffer with First through First + Count - 1. */
                for (i = 0; i < StreamInfo->count; ++i)
                {
                    *ptr++ = (gctUINT16) (first + i);
                }

                /* Append First to index buffer. */
                *ptr = (gctUINT16) first;
            }
            else
            {
                /* Cast pointer to index buffer. */
                gctUINT32_PTR ptr = (gctUINT32_PTR) indexBuffer;

                /* Fill index buffer with First through First + Count - 1. */
                for (i = 0; i < StreamInfo->count; ++i)
                {
                    *ptr++ = (gctUINT32)first + i;
                }

                /* Append First to index buffer. */
                *ptr = (gctUINT32)first;
            }
        }

        /* Upload the data to the dynamic index buffer. */
        gcmONERROR(gcoINDEX_UploadDynamicEx(Vertex->dynamicIndex,
                                            indexType,
                                            indexBuffer,
                                            bytes + bytesPerIndex,
                                            gcvFALSE));

        /* Bind the index buffer to the hardware. */
        gcmONERROR(gcoINDEX_BindDynamic(Vertex->dynamicIndex, indexType));

        /* Free the temporary index buffer. */
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, indexBuffer));
    }
    else
    {
        /* Test if there is a gcoINDEX stream. */
        if (IndexInfo->u.es11.indexBuffer != gcvNULL)
        {
            /* Bind the index buffer to the hardware. */
            gcmONERROR(gcoINDEX_BindOffset(IndexInfo->u.es11.indexBuffer,
                                           IndexInfo->indexType,
                                           gcmPTR2INT32(IndexInfo->indexMemory)));
        }
        /* Test if there are client indices. */
        else if (IndexInfo->indexMemory != gcvNULL)
        {
            gctUINT bytes = 0;

            /* Determine number of bytes in the index buffer. */
            bytes = count32 * bytesPerIndex;

            /* Upload the data to the dynamic index buffer. */
            gcmONERROR(gcoINDEX_UploadDynamicEx(Vertex->dynamicIndex,
                                                IndexInfo->indexType,
                                                IndexInfo->indexMemory,
                                                bytes,
                                                convertToIndexedTriangleList));

            /* Bind the index buffer to the hardware. */
            gcmONERROR(gcoINDEX_BindDynamic(Vertex->dynamicIndex, IndexInfo->indexType));
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (indexBuffer != gcvNULL)
    {
        /* Free the temporary index buffer. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, indexBuffer));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVERTEXARRAY_StreamBind_Ex(
    IN gcoVERTEXARRAY Vertex,
#if gcdUSE_WCLIP_PATCH
    IN OUT gctFLOAT * WLimitRms,
    IN OUT gctBOOL * WLimitRmsDirty,
#endif
    gcsVERTEXARRAY_STREAM_INFO_PTR StreamInfo,
    gcsVERTEXARRAY_INDEX_INFO_PTR IndexInfo
    )
{
    gceSTATUS status                     = gcvSTATUS_OK;
    gcsVERTEXARRAY_PTR vertexPtr = StreamInfo->u.es11.attributes;
    gctUINT first                = (gctUINT)StreamInfo->first;
    gctUINT i;
    /* total stream count, include client pointer and vbo.*/
    gctUINT n;
    gctUINT count, count32;

    /* Zero the arrays. */
    gcsVERTEXARRAY_ATTRIBUTE attributeArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_ATTRIBUTE_PTR attributePtr = attributeArray;
    gctUINT attributeCount = 0;

    /* Zero the arrays. */
    gcsVERTEXARRAY_ATTRIBUTE copyArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_ATTRIBUTE_PTR copyPtr = copyArray;
    gctUINT copyCount = 0, copyBytes     = 0;
    gctUINT32 copyPhysical = ~0U;

    /* Zero the arrays. */
    gcsVERTEXARRAY_STREAM streamArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_STREAM_PTR streamPtr  = gcvNULL;
    gctUINT stream, streamCount = 0;

#if OPT_VERTEX_ARRAY
    gcsVERTEXARRAY_BUFFER bufferArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_BUFFER_PTR bufferPtr  = bufferArray;
    gctUINT bufferCount = 0;
#endif

    gctBOOL fakeLineLoop                 = gcvFALSE;
    gctBOOL convertToIndexedTriangleList = gcvFALSE;
    gcePATCH_ID patchId                  = gcvPATCH_INVALID;
    gctUINT enableBits                   = StreamInfo->attribMask;
    gctUINT totalBufSize                 = 0;

    gcmHEADER_ARG("Vertex=0x%x StreamInfo=0x%x IndexInfo=0x%x",
                  Vertex, StreamInfo, IndexInfo);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);
    gcmVERIFY_ARGUMENT(StreamInfo->attribMask != 0);
    gcmVERIFY_ARGUMENT(StreamInfo->u.es11.attributes != gcvNULL);
    gcmVERIFY_ARGUMENT(StreamInfo->count > 0);

    gcmSAFECASTSIZET(count32, StreamInfo->count);

    /* Check for hardware line loop support. */
    fakeLineLoop = (StreamInfo->primMode == gcvPRIMITIVE_LINE_LOOP)
                 ? !Vertex->hwFeature.hasLineLoop
                 : gcvFALSE;

    convertToIndexedTriangleList = (StreamInfo->primMode == gcvPRIMITIVE_TRIANGLE_STRIP)
        && ((IndexInfo->u.es11.indexBuffer == gcvNULL) && (IndexInfo->indexMemory != gcvNULL))
        && !Vertex->hwFeature.hasIndexedTriangleStripFix;

    if((StreamInfo->primMode == gcvPRIMITIVE_LINE_LOOP) && (!fakeLineLoop))
    {
        /*for line loop the last line is automatic added in hw implementation*/
        StreamInfo->primCount = StreamInfo->primCount - 1;
    }

    gcoHARDWARE_GetPatchID(gcvNULL, &patchId);
    if (patchId == gcvPATCH_GPUBENCH)
    {
        gcPLS.hal->isGpuBenchSmoothTriangle = (StreamInfo->primCount == 133);
    }

    /***************************************************************************
    ***** Phase 1: Information Gathering **************************************/

    /* Walk through all attributes. */
    for (i = 0;
         enableBits != 0;
         ++i, enableBits >>= 1, ++vertexPtr
    )
    {
        gctBOOL needCopy;

        /* Skip disabled vertex attributes. */
        if ((enableBits & 1) == 0)
        {
            continue;
        }

        /* Initialize the attribute information. */
        attributePtr->vertexPtr = vertexPtr;
        attributePtr->next      = gcvNULL;

        if (vertexPtr->enable)
        {
            attributePtr->format = vertexPtr->format;

            /* Compute the size of the attribute. */
            switch (vertexPtr->format)
            {
            case gcvVERTEX_BYTE:
            case gcvVERTEX_UNSIGNED_BYTE:
            case gcvVERTEX_INT8:
                attributePtr->bytes = vertexPtr->size;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
            case gcvVERTEX_INT_10_10_10_2:
            case gcvVERTEX_UNSIGNED_INT_2_10_10_10_REV:
            case gcvVERTEX_INT_2_10_10_10_REV:
                /* 10_10_10_2 format is always in 4 bytes container. */
                attributePtr->bytes = 4;
                break;

            case gcvVERTEX_SHORT:
            case gcvVERTEX_UNSIGNED_SHORT:
            case gcvVERTEX_HALF:
            case gcvVERTEX_INT16:
                attributePtr->bytes = vertexPtr->size * 2;
                break;
            case gcvVERTEX_INT32:
                attributePtr->bytes = vertexPtr->size * 4;
                break;

            default:
                attributePtr->bytes = vertexPtr->size * 4;
                break;
            }
        }

        /* Test if this vertex attribute is a gcoSTREAM. */
        if (vertexPtr->enable && (vertexPtr->stream != gcvNULL))
        {
            gcsVERTEXARRAY_ATTRIBUTE_PTR node, prev;
            gctSIZE_T Size = 0;
            /* Save offset for this attribute. */
            attributePtr->offset = gcmPTR2INT32(vertexPtr->pointer);

            /* Find the stream for this vertex attribute in the stream array. */
            for (stream = 0, streamPtr = streamArray;
                 stream < streamCount;
                 ++stream, ++streamPtr
            )
            {
                if (streamPtr->stream == vertexPtr->stream)
                {
                    /* Found it. */
                    break;
                }
            }

            /* Test if the stream is not yet known. */
            if (stream == streamCount)
            {
                gctPOINTER pointer = gcvNULL;

                /* Make sure we don't overflow the array. */
                if (stream == gcmCOUNTOF(streamArray))
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }

                /* Increase the number of streams. */
                streamCount += 1;

                /* Initialize the stream information. */
                streamPtr->stream    = vertexPtr->stream;
                streamPtr->subStream = gcvNULL;

                /* Lock the stream. */
                gcmONERROR(gcoSTREAM_Lock(vertexPtr->stream,
                                          &pointer,
                                          &streamPtr->physical));
                Size = gcoSTREAM_GetSize(vertexPtr->stream);
                if(Size < attributePtr->offset)
                {
                    gcmONERROR(gcvSTATUS_INVALID_DATA);
                }
                streamPtr->logical = (gctUINT8_PTR)pointer;
                /* Initialize attribute to gcvNULL. */
                streamPtr->attribute = gcvNULL;
            }

            /* Sort this attribute by offset. */
            for (node = streamPtr->attribute, prev = gcvNULL;
                 node != gcvNULL;
                 node = node->next)
            {
                /* Check if we have found an attribute with a larger offset. */
                if (node->offset > attributePtr->offset)
                {
                    break;
                }

                /* Save attribute and move to next one. */
                prev = node;
            }

            /* Link attribute to this stream. */
            attributePtr->next = node;
            if (prev == gcvNULL)
            {
                streamPtr->attribute = attributePtr;
            }
            else
            {
                prev->next = attributePtr;
            }

            /* Set stream attribute. */
            gcmONERROR(gcoSTREAM_SetAttribute(streamPtr->stream,
                                              gcmPTR2SIZE(vertexPtr->pointer),
                                              attributePtr->bytes,
                                              vertexPtr->stride,
                                              vertexPtr->divisor,
                                              &streamPtr->subStream));

            /* Set logical pointer to attribute. */
            attributePtr->logical = streamPtr->logical
                                  + gcmPTR2INT32(vertexPtr->pointer);

#if gcdUSE_WCLIP_PATCH
            if ((attributePtr->vertexPtr->isPosition == gcvTRUE)
                    && (attributePtr->format == gcvVERTEX_FLOAT)
                    && (attributePtr->bytes/4 == 3)
                    && (WLimitRms != gcvNULL))
            {
                computeWLimit(
                        (gctFLOAT_PTR)((gctUINT8_PTR)attributePtr->logical + first * attributePtr->vertexPtr->stride),
                        attributePtr->bytes / 4,
                        attributePtr->vertexPtr->stride,
                        count32,
                        WLimitRms,
                        WLimitRmsDirty);
            }
#endif

            /* Don't need to copy this attribute yet. */
            needCopy = gcvFALSE;
        }

        /* Test if this vertex attribute is a client pointer. */
        else if (vertexPtr->enable && (vertexPtr->pointer != gcvNULL))
        {
            /* Set logical pointer to attribute. */
            attributePtr->logical = vertexPtr->pointer;
            /* Copy this attribute. */
            needCopy = gcvTRUE;

#if gcdUSE_WCLIP_PATCH
            if ((attributePtr->vertexPtr->isPosition == gcvTRUE)
                 && (attributePtr->format == gcvVERTEX_FLOAT)
                 && (attributePtr->bytes/4 == 3)
                 && (WLimitRms != gcvNULL))
            {
                computeWLimit((gctFLOAT_PTR)((gctUINT8_PTR)attributePtr->logical + first * attributePtr->vertexPtr->stride),
                              attributePtr->bytes / 4,
                              attributePtr->vertexPtr->stride,
                              count32,
                              WLimitRms,
                              WLimitRmsDirty);
            }
#endif
        }

        /* The vertex attribute is not set. */
        else
        {
            /* Need to copy generic value. */
            attributePtr->format  = gcvVERTEX_FLOAT;
            attributePtr->bytes   = vertexPtr->genericSize * 4;

            /* Set logical pointer to attribute. */
            attributePtr->logical = vertexPtr->genericValue;

            /* Copy this attribute. */
            needCopy = gcvTRUE;
        }

        if (needCopy)
        {
            /* Fill in attribute information that needs to be copied. */
            copyPtr->vertexPtr = vertexPtr;
            copyPtr->logical   = attributePtr->logical;
            copyPtr->offset    = copyBytes;

            /* No generic data. */
            copyPtr->format = attributePtr->format;
            copyPtr->bytes  = attributePtr->bytes;

            /* Increase number of bytes per vertex. */
            copyBytes += copyPtr->bytes;

            /* Move to next array. */
            copyCount += 1;
            copyPtr   += 1;
        }

        /* Next attribute. */
        attributeCount += 1;
        attributePtr   += 1;
    }

    /* Test if there are too many attributes. */
    if (attributeCount > Vertex->maxAttribute)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Check for aliasing older hardware has issues with. */
    if ((streamCount == 1)
    &&  (attributeCount == 2)
    &&  (attributeArray[0].logical == attributeArray[1].logical)
    &&  (attributeArray[0].bytes < 8)
    &&  (attributeArray[0].vertexPtr->stream != gcvNULL)
    &&  (count32 > 31)
    )
    {
        /* Unalias the stream buffer. */
        gcmONERROR(gcoSTREAM_UnAlias(attributeArray[0].vertexPtr->stream,
                                     streamArray->attribute,
                                     &streamArray[0].subStream,
                                     &streamArray[0].logical,
                                     &streamArray[0].physical));
    }

    /* Compute total number of streams. */
    n = (copyCount > 0) ? 1 : 0;
    for (i = 0, streamPtr = streamArray; i < streamCount; ++i, ++streamPtr)
    {
        gctUINT subStreams;

        /* Query number of substreams from this stream. */
        gcmONERROR(gcoSTREAM_QuerySubStreams(streamPtr->stream,
                                             streamPtr->subStream,
                                             &subStreams));

        /* Increase the number of streams. */
        n += subStreams;
    }


    /***************************************************************************
    ***** Phase 3: Stream Management ******************************************/
    /* Test if need to copy vertex data. */
    if ((copyCount > 0) || (n > Vertex->maxStreams))
    {
        gctUINT minIndex, maxIndex;

        /* Test if there is a gcoINDEX object. */
        if (IndexInfo->u.es11.indexBuffer != gcvNULL)
        {
            /* Get the index range. */
            gcmONERROR(gcoINDEX_GetIndexRange(IndexInfo->u.es11.indexBuffer,
                                              IndexInfo->indexType,
                                              gcmPTR2SIZE(IndexInfo->indexMemory),
                                              count32,
                                              &minIndex, &maxIndex));

            /* Compute vertex range. */
            first = minIndex;
            count = maxIndex - minIndex + 1;
        }

        /* Test if there is an index array. */
        else if (IndexInfo->indexMemory != gcvNULL)
        {
            /* Get the index range. */
            gcmONERROR(gcoINDEX_GetMemoryIndexRange(IndexInfo->indexType,
                                                    IndexInfo->indexMemory,
                                                    count32,
                                                    &minIndex, &maxIndex));

            /* Compute vertex range. */
            first = minIndex;
            count = maxIndex - minIndex + 1;
        }

        /* No indices present. */
        else
        {
            /* Copy vertex range. */
            count = count32;
        }
    }
    else
    {
        /* No need to verify or copy anything. */
        count = count32;
    }

    /* Check if we have too many streams. */
    if ((n > Vertex->maxStreams) && (copyCount == 0))
    {
        gcoSTREAM streamObject;
        gctPOINTER logical;
        gctUINT32 physical;
        gcsSTREAM_SUBSTREAM_PTR subStream;

        /* Merge the streams. */
        status = gcoSTREAM_MergeStreams(streamArray[0].stream,
                                        first, count,
                                        streamCount, streamArray,
                                        &streamObject, &logical, &physical,
                                        &attributePtr, &subStream);

        if (gcmIS_SUCCESS(status))
        {
            /* Copy stream information to stream array. */
            streamArray[0].stream    = streamObject;
            streamArray[0].logical   = (gctUINT8_PTR)logical;
            streamArray[0].physical  = physical;
            streamArray[0].attribute = attributePtr;
            streamArray[0].subStream = subStream;
            streamCount              = 1;
            n                        = 1;
        }
    }

    /* Check if we still have too many streams. */
    if (n > Vertex->maxStreams)
    {
        /* Reset number of streams. */
        n = (copyCount > 0) ? 1 : 0;

        /* Walk all streams. */
        for (i = 0, streamPtr = streamArray;
             i < streamCount;
             ++i, ++streamPtr
        )
        {
            gctUINT subStreams;

            /* Query number of streams. */
            gcmONERROR(gcoSTREAM_QuerySubStreams(streamPtr->stream,
                                                 streamPtr->subStream,
                                                 &subStreams));

            /* Check if we can rebuild this stream and if it makes sense. */
            if ((subStreams > 1) && (n + 1 < Vertex->maxStreams))
            {
                /* Rebuild the stream. */
                status = gcoSTREAM_Rebuild(streamPtr->stream,
                                           first, count,
                                           &subStreams);
                if (gcmIS_ERROR(status))
                {
                    /* Error rebuilding. */
                    gcmTRACE_ZONE(gcvLEVEL_WARNING, _GC_OBJ_ZONE,
                                  "gcoSTREAM_Rebuild returned status=%d",
                                  status);
                }
            }

            if ((n + subStreams > Vertex->maxStreams)
            ||  (Vertex->maxStreams == 1)
            )
            {
                /* This stream would overflow the maximum stream count, so we
                ** have to copy it. */

                /* Walk all the stream attributes and append them to the list to
                ** copy. */
                for (attributePtr = streamPtr->attribute;
                     attributePtr != gcvNULL;
                     attributePtr = attributePtr->next
                     )
                {
                    /* Copy this attribute. */
                    copyPtr->vertexPtr     = attributePtr->vertexPtr;
                    copyPtr->logical       = attributePtr->logical;
                    copyPtr->format        = attributePtr->format;
                    copyPtr->offset        = copyBytes;
                    copyPtr->bytes         = attributePtr->bytes;

                    /* Increase number of bytes per vertex. */
                    copyBytes += copyPtr->bytes;

                    /* Move to next array. */
                    copyCount += 1;
                    copyPtr   += 1;
                }

                /* This stream doesn't need to be programmed. */
                streamPtr->logical = gcvNULL;
            }
            else
            {
                /* Advance the number of streams. */
                n += subStreams;
            }
        }
    }

    /***************************************************************************
    ***** Phase 4: Vertex Copy ************************************************/

    if (copyCount > 0)
    {
#if OPT_VERTEX_ARRAY
        /* Total size in bytes of all attributes need to be copied */
        gctUINT hwStream = Vertex->maxStreams - n + 1;
        gctUINT j, k;

        /* Zero memory */
        gcoOS_ZeroMemory(bufferArray, copyCount * sizeof(bufferArray[0]));

        /* Setup buffers. */
        for (i = 0, copyPtr = copyArray; i < copyCount; i++, copyPtr++)
        {
            gctUINT8_PTR start = (gctUINT8_PTR)copyPtr->logical;
            gctUINT8_PTR end   = (gctUINT8_PTR)copyPtr->logical + copyPtr->bytes;

            for (j = 0, bufferPtr = bufferArray;
                 j < bufferCount;
                 j++, bufferPtr++)
            {
                /* If in a same range and have the same divisor*/
                if((bufferPtr->stride == copyPtr->vertexPtr->stride) && (bufferPtr->divisor == copyPtr->vertexPtr->divisor))
                {
                    /* Test if the attribute falls within the buffer range.*/
                    if ((start >= bufferPtr->start) &&
                        (end   <= bufferPtr->end)
                       )
                    {
                        break;
                    }

                    /* Test if the attribute starts within the sliding window. */
                    if ((start <  bufferPtr->start) &&
                        (start >= bufferPtr->minStart)
                       )
                    {
                        bufferPtr->start  = start;
                        bufferPtr->maxEnd = start + bufferPtr->stride;
                        break;
                    }

                    /* Test if attribute ends within the sliding window. */
                    if ((end >  bufferPtr->end) &&
                        (end <= bufferPtr->maxEnd)
                       )
                    {
                        bufferPtr->end      = end;
                        bufferPtr->minStart = end - bufferPtr->stride;
                        break;
                    }
                }
            }

            /* Create a new vertex buffer. */
            if (j == bufferCount)
            {
                bufferCount++;

                bufferPtr->numAttribs = 1;
                bufferPtr->map[0]     = i;

                bufferPtr->stride     = copyPtr->vertexPtr->enable
                                      ? copyPtr->vertexPtr->stride
                                      : copyPtr->bytes;
                 /*Force the buffer is combined if stride < bytes  */
                if(bufferPtr->stride > 0 && bufferPtr->stride < copyPtr->bytes)
                {
                    bufferPtr->combined = gcvTRUE;
                    bufferPtr->stride = copyPtr->bytes;
                }
                bufferPtr->divisor    = copyPtr->vertexPtr->divisor;
                bufferPtr->start      = (gctUINT8_PTR)copyPtr->logical;
                bufferPtr->end        = bufferPtr->start + copyPtr->bytes;
                bufferPtr->minStart   = bufferPtr->end   - bufferPtr->stride;
                bufferPtr->maxEnd     = bufferPtr->start + bufferPtr->stride;
            }
            else
            {
                /* Sort the attribute in a buffer. */
                for (k = bufferPtr->numAttribs; k != 0; k--)
                {
                    if (copyPtr->logical >= copyArray[bufferPtr->map[k - 1]].logical)
                    {
                        bufferPtr->map[k] = i;
                        break;
                    }

                    bufferPtr->map[k] = bufferPtr->map[k - 1];
                }

                if (k == 0)
                {
                    bufferPtr->map[0] = i;
                }

                bufferPtr->numAttribs += 1;
            }
        }

        /* Adjust attributes. */
        for (i = 0, bufferPtr = bufferArray;
             i < bufferCount;
             i++, bufferPtr++, --hwStream)
        {
            /* Break if only one hw stream available, while more than one buffer left. */
            if ((hwStream == 1) && (i != (bufferCount -1)))
            {
                break;
            }

            /* Set offset for a buffer. */
            bufferPtr->offset = totalBufSize;

            /* Adjust attributes offset in a buffer. */
            for (j = 0; j < bufferPtr->numAttribs; j++)
            {
                copyPtr = copyArray + bufferPtr->map[j];
                copyPtr->offset = (gctUINT)((gctUINT8_PTR)copyPtr->logical
                                - (gctUINT8_PTR)bufferPtr->start);
            }

            /* Instance Divisor required count is different than usual cases */
            bufferPtr->count = bufferPtr->divisor
                             ? (gctUINT) gcoMATH_Ceiling((gctFLOAT) StreamInfo->instanceCount / (gctFLOAT)bufferPtr->divisor)
                             : count;
            totalBufSize += (bufferPtr->count * bufferPtr->stride);
        }

        /* Check if all buffers have been handled? */
        /* If no, combine all left attributes into the last buffer. */
        if (i < bufferCount)
        {
            gcsVERTEXARRAY_BUFFER_PTR last;
            gctUINT attribCount = 0;
            gctUINT offset = 0;
            gctUINT adjustedBufferCount = i + 1;

            /* Mark last buffer as combined. */
            last           = bufferArray + i;
            last->offset   = totalBufSize;
            last->combined = gcvTRUE;

            /* Merge all left into the last buffer. */
            for (bufferPtr = last;
                 i < bufferCount;
                 ++i, ++bufferPtr)
            {
                for (j = 0; j < bufferPtr->numAttribs; j++, attribCount++)
                {
                    /* Adjust attribute offset. */
                    copyPtr = copyArray + bufferPtr->map[j];
                    copyPtr->offset = offset;

                    /* Merge attribute into last buffer.*/
                    if (last != bufferPtr)
                    {
                        last->map[attribCount] = bufferPtr->map[j];
                        last->numAttribs++;
                    }

                    if (last->divisor != copyPtr->vertexPtr->divisor)
                    {
                        gcmASSERT(0);
                    }

                    /* Increase offset. */
                    offset += copyPtr->bytes;
                }
            }

            /* Adjust buffer stride. */
            last->stride = offset;

            /* Re-adjust total buffer count. */
            bufferCount = adjustedBufferCount;


            /* Instance Divisor required count is different than usual cases */
            last->count = last->divisor
                        ? (gctUINT) gcoMATH_Ceiling((gctFLOAT) StreamInfo->instanceCount / (gctFLOAT)last->divisor)
                        : count;
            totalBufSize += (last->count * last->stride);
        }

        /* Copy the vertex data. */
        status = gcoSTREAM_CacheAttributes(Vertex->dynamicStream,
                                           first, count,
                                           totalBufSize,
                                           bufferCount, bufferArray,
                                           copyCount, copyArray,
                                           &copyPhysical);
        if (gcmIS_ERROR(status))
        {
            gcmONERROR(gcoSTREAM_UploadUnCacheableAttributes(Vertex->uncacheableStream,
                                                             first, count,
                                                             totalBufSize,
                                                             bufferCount, bufferArray,
                                                             copyCount, copyArray,
                                                             &copyPhysical,
                                                             &Vertex->uncacheableStream));
        }

#else

        /* Copy the vertex data. */
        gcmONERROR(gcoSTREAM_CacheAttributes(Vertex->dynamicStream,
                                             first, count,
                                             copyBytes,
                                             copyCount, copyArray,
                                             &copyPhysical));
#endif
    }

    /* Fix mixed stream issue.*/
#if OPT_VERTEX_ARRAY
    if (!Vertex->hwFeature.hasMixedStreams && (Vertex->maxStreams > 1) && (n > 1))
    {
        gcmONERROR(gcoVERTEX_AdjustStreamPool(Vertex->dynamicStream,
                                              bufferCount, bufferArray,
                                              copyCount, copyArray,
                                              streamCount, streamArray,
                                              first, count,
                                              totalBufSize,
                                              StreamInfo->instanced,
                                              &copyPhysical,
                                              &Vertex->uncacheableStream
                                              ));
    }
#endif

    /***************************************************************************
    ***** Phase 5: Program the Hardware ***************************************/

    /* Here it is easy - let the next layer worry about the actual registers. */
#if OPT_VERTEX_ARRAY
    gcmONERROR(gcoHARDWARE_SetVertexArray(gcvNULL,
                                          StreamInfo->instanced,
                                          first, copyPhysical,
                                          bufferCount, bufferArray,
                                          copyCount, copyArray,
                                          streamCount, streamArray));

#else
    gcmONERROR(gcoHARDWARE_SetVertexArray(gcvNULL,
                                          first,
                                          copyPhysical, copyBytes,
                                          copyCount, copyArray,
                                          streamCount, streamArray));
#endif

    if (fakeLineLoop)
    {
        /* Switch to Line Strip. */
        StreamInfo->primMode = gcvPRIMITIVE_LINE_STRIP;
    }
    else if (convertToIndexedTriangleList)
    {
        /* Switch to Triangle List. */
        StreamInfo->primMode = gcvPRIMITIVE_TRIANGLE_LIST;
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
gcoVERTEXARRAY_Bind(
    IN gcoVERTEXARRAY Vertex,
    IN gctUINT32 EnableBits,
    IN gcsVERTEXARRAY_PTR VertexArray,
    IN gctUINT First,
    IN gctSIZE_T * Count,
    IN gceINDEX_TYPE IndexType,
    IN gcoINDEX IndexObject,
    IN gctPOINTER IndexMemory,
    IN OUT gcePRIMITIVE * PrimitiveType,
#if gcdUSE_WCLIP_PATCH
    IN OUT gctUINT * PrimitiveCount,
    IN OUT gctFLOAT * WLimitRms,
    IN OUT gctBOOL * WLimitRmsDirty
#else
    IN OUT gctUINT * PrimitiveCount
#endif
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsVERTEXARRAY_STREAM_INFO streamInfo, origStreamInfo;
    gcsVERTEXARRAY_INDEX_INFO  indexInfo;

    gcmHEADER();

    /* Collect hal leve info.*/
    streamInfo.attribMask = EnableBits;
    streamInfo.u.es11.attributes = VertexArray;
    streamInfo.first = First;
    streamInfo.count = *Count;
    streamInfo.primMode = *PrimitiveType;
    streamInfo.primCount = *PrimitiveCount;
    streamInfo.instanced = gcvFALSE;
    streamInfo.instanceCount = 1;

    gcoOS_MemCopy(&origStreamInfo, &streamInfo, sizeof(gcsVERTEXARRAY_STREAM_INFO));

    indexInfo.count = *Count;
    indexInfo.indexType = IndexType;
    indexInfo.u.es11.indexBuffer = IndexObject;
    indexInfo.indexMemory = IndexMemory;

#if gcdUSE_WCLIP_PATCH
    /* Collect hal level info.*/
    gcmONERROR(gcoVERTEXARRAY_StreamBind_Ex(Vertex,
                                            WLimitRms,
                                            WLimitRmsDirty,
                                            &streamInfo,
                                            &indexInfo));
#else
    /* Collect hal level info.*/
    gcmONERROR(gcoVERTEXARRAY_StreamBind_Ex(Vertex,
                                            &streamInfo,
                                            &indexInfo));
#endif

    gcmONERROR(gcoVERTEXARRAY_IndexBind_Ex(Vertex,
                                           &origStreamInfo,
                                           &indexInfo));

    /* es11 path has some split draw path in hal level,
    ** and will change StreamInfo data, need update.*/
    *PrimitiveType = streamInfo.primMode;
    *PrimitiveCount = (gctUINT)streamInfo.primCount;

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
**  _findStream
**
**  Finds the stream in the stream list that would be a fit for the incoming
**  attribute. If there is none, it will return gcvNULL
**
**  INPUT:
**
**      gcsVERTEXARRAY_BUFOBJ_PTR Streams
**          Head pointer to the list of streams
**
**      gcsATTRIBUTE_PTR VertexPtr
**          Pointer to the attribute
**
**      gctUINT32 Bytes
**          Bytes of the attribute.
**
**  OUTPUT:
**
**      gcsVERTEXARRAY_BUFOBJ_PTR
**          Stream that will hold the attribute or gcvNULL
**
*/
gcsVERTEXARRAY_BUFOBJ_PTR
_findStream(
    IN gcoVERTEXARRAY Vertex,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gcsATTRIBUTE_PTR VertexPtr,
    IN gctUINT32 Bytes
    )
{
    gcsVERTEXARRAY_BUFOBJ_PTR stream;

    gcmHEADER_ARG("Vertex=0x%x Streams=0x%x VertexPtr=0x%x Bytes=%u ", Vertex, Streams, VertexPtr, Bytes);

    stream = gcvNULL;
    if (VertexPtr->enable)
    {
        gctSIZE_T attribOffset = gcmPTR2SIZE(VertexPtr->pointer);

        for (stream = Streams; stream != gcvNULL; stream = stream->next)
        {
            gctSIZE_T min;
            gctSIZE_T max;
            gctSIZE_T relativeOffset;

            /* Assert that there is at least one attribute */
            gcmASSERT(stream->attributePtr != gcvNULL);

            /* Find sliding window, can overlap with other attribs */
            if (stream->attributePtr->offset < attribOffset)
            {
                relativeOffset = attribOffset - stream->attributePtr->offset;
                max = stream->attributePtr->offset + stream->stride - Bytes;
                min = stream->attributePtr->offset;
            }
            else
            {
                relativeOffset = stream->maxAttribOffset - attribOffset;
                max = stream->attributePtr->offset + stream->attributePtr->bytes - Bytes;
                min = stream->attributePtr->offset + stream->attributePtr->bytes - stream->stride;
            }

            if (VertexPtr->stream != gcvNULL)
            {
                if ((VertexPtr->stream == stream->stream)
                    && (VertexPtr->stride == stream->stride)
                    && (VertexPtr->divisor == stream->divisor)
                    && (relativeOffset <= Vertex->maxAttribOffset)
                    && (max >= attribOffset)
                    && (min <= attribOffset)
                    )
                {
                    /* found it */
                    break;
                }
            }
            else
            {
                if ((VertexPtr->stride == stream->stride)
                    && (VertexPtr->divisor == stream->divisor)
                    && (max >= attribOffset)
                    && (min <= attribOffset)
                    )
                {
                    /* found it */
                     break;
                }
            }
        }
    }

    gcmFOOTER_ARG ("Stream=0x%x", stream);
    return stream;
}

gceSTATUS gcoVERTEXARRAY_MergeClientStreams(IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
                                            IN gctUINT MaxStreamCount,
                                            IN OUT gctUINT_PTR StreamCount,
                                            IN OUT gctUINT_PTR CopyCount
                                           )

{
    gceSTATUS status = gcvSTATUS_OK;
    gcsVERTEXARRAY_BUFOBJ_PTR streamPtr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_PTR streamLoopPtr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR lastAttr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_PTR first = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_PTR prev = gcvNULL;
    gctBOOL removed = gcvFALSE;

    /* We loop the stream until *StreamCount <= MaxStreamCount.
     * It may be much better to sort stream w.r.t. attribute count so that we first merge
     * streams with less attributes.
     */
    for (streamLoopPtr = Streams;
         (streamLoopPtr != gcvNULL) && ((*CopyCount >= 2) && (*StreamCount > MaxStreamCount));
         streamLoopPtr = streamLoopPtr->next)
    {
        if (streamLoopPtr->stream == gcvNULL)
        {
            /* Found it */
            first = streamLoopPtr;
        }

        if (first != gcvNULL)
        {
            /* Find last attribute */
            lastAttr = first->attributePtr;
            while ((lastAttr != gcvNULL) && (lastAttr->next != gcvNULL))
            {
                lastAttr = lastAttr->next;
            }

            /* Find first client stream */
            for (streamPtr = first; streamPtr != gcvNULL; streamPtr = streamPtr->next)
            {
                if ((streamPtr->stream == gcvNULL) && streamPtr != first)
                {
                    /* It can happen that first stream divisor would not allow any
                    * merge. If we had chosen a different first then it would be possible.
                    * We need to address this when needed
                    */
                    if (first->divisor == streamPtr->divisor)
                    {
                        /* Merge streams */
                        if (lastAttr != gcvNULL)
                        {
                            lastAttr->next = streamPtr->attributePtr;
                        }
                        else
                        {
                            lastAttr = streamPtr->attributePtr;
                        }

                        /* Update last attribute */
                        while ((lastAttr != gcvNULL) && (lastAttr->next != gcvNULL))
                        {
                            lastAttr = lastAttr->next;
                        }

                        first->merged = gcvTRUE;
                        first->attributeCount += streamPtr->attributeCount;
                        (*CopyCount)--;
                        (*StreamCount)--;

                        /* remove stream */
                        prev->next = streamPtr->next;
                        removed = gcvTRUE;
                    }
                }

                /* Set this stream as previous stream */
                if (!removed)
                {
                    prev = streamPtr;
                }

                /* Reset */
                removed = gcvFALSE;

                if (*StreamCount <= MaxStreamCount)
                {
                    /* We're done */
                    break;
                }

                if (*CopyCount < 2)
                {
                    /* We're done */
                    break;
                }
            }
        }
        first = gcvNULL;
    }
    return status;
}

gceSTATUS gcoVERTEXARRAY_MergeAllStreams(IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
                                         IN gctUINT MaxStreamCount,
                                         IN OUT gctUINT_PTR StreamCount,
                                         IN OUT gctUINT_PTR CopyCount)

{
    gceSTATUS status = gcvSTATUS_OK;
    gcsVERTEXARRAY_BUFOBJ_PTR streamPtr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_PTR streamLoopPtr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR lastAttr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR attr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_PTR prev = gcvNULL;
    gctBOOL removed = gcvFALSE;

    /* We loop the stream until *StreamCount <= MaxStreamCount.
     * It may be much better to sort stream w.r.t. attribute count so that we first merge
     * streams with less attributes.
     */
    /* If stream count is 0, don't merge */
    if (MaxStreamCount == 0)
        return status;

    /* We loop the stream until *StreamCount <= MaxStreamCount or loop all streams.
     */
    for (streamLoopPtr = Streams; (streamLoopPtr != gcvNULL) && (*StreamCount > MaxStreamCount);
        streamLoopPtr = streamLoopPtr->next)
    {
        if (streamLoopPtr->stream != gcvNULL)
        {
            /* Find last attribute */
            for (attr = streamLoopPtr->attributePtr; attr != gcvNULL; attr = attr->next)
            {
                attr->pointer = streamLoopPtr->logical + attr->offset;
                lastAttr = attr;
            }

            streamLoopPtr->stream = gcvNULL;
        }
        else
        {
            /* Find last attribute */
            lastAttr = streamLoopPtr->attributePtr;
            while((lastAttr != gcvNULL) && (lastAttr->next != gcvNULL))
            {
                lastAttr = lastAttr->next;
            }
        }

        prev = streamLoopPtr;

        /* Find next stream */
        for (streamPtr = streamLoopPtr->next; (streamPtr != gcvNULL) && (*StreamCount > MaxStreamCount); streamPtr = streamPtr->next)
        {
            /* It can happen that first stream divisor would not allow any
            * merge. If we had chosen a different first then it would be possible.
            * We need to address this when needed
            */
            if (streamLoopPtr->divisor == streamPtr->divisor)
            {
                /* Merge streams */
                if (lastAttr != gcvNULL)
                {
                    lastAttr->next = streamPtr->attributePtr;
                }
                else
                {
                    lastAttr = streamPtr->attributePtr;
                }

                if (streamPtr->stream == gcvNULL)
                {
                    /* Update last attribute */
                    while((lastAttr != gcvNULL) && (lastAttr->next != gcvNULL))
                    {
                        lastAttr = lastAttr->next;
                    }
                    (*CopyCount)--;
                }
                else
                {
                    /* Find last attribute */
                    for (attr = lastAttr->next; attr != gcvNULL; attr = attr->next)
                    {
                        attr->pointer = streamPtr->logical + attr->offset;
                        lastAttr = attr;
                    }

                    streamLoopPtr->stream = gcvNULL;
                }

                streamLoopPtr->merged = gcvTRUE;
                streamLoopPtr->attributeCount += streamPtr->attributeCount;
                (*StreamCount)--;

                /* remove stream */
                prev->next = streamPtr->next;
                removed = gcvTRUE;
            }

            if (!removed)
            {
                prev = streamPtr;
            }

            /* Reset */
            removed = gcvFALSE;
        }
    }

    return status;
}
/**************************************************************************************
**
** gcoVERTEXARRAY_IndexBind
**
** Bind index data to HW
**
** Now sperate bind stream / index to hw. Becareful, BindStream need index data to decide
** update, so, BindIndex should before BindStream
** 1) many app/game only update index perframe vertex data not change,
** 2) many internal special patch will only deal with index.
***************************************************************************************/
gceSTATUS
gcoVERTEXARRAY_IndexBind(
    IN gcoVERTEXARRAY Vertex,
    IN gcsVERTEXARRAY_INDEX_INFO_PTR IndexInfo
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT count32 = 0;
    gctUINT bytes = 0;
    gceINDEX_TYPE IndexType = IndexInfo->indexType;

    gcmHEADER();

    gcmSAFECASTSIZET(count32, IndexInfo->count);

    /* Test if there is a gcoINDEX stream. */
    if (IndexInfo->u.es30.indexBuffer != gcvNULL)
    {
        /* Bind the index buffer to the hardware. */
        gcmONERROR(gcoBUFOBJ_IndexBind(IndexInfo->u.es30.indexBuffer,
                                       IndexInfo->indexType,
                                       gcmPTR2SIZE(IndexInfo->indexMemory),
                                       count32,
                                       IndexInfo->restartElement));
    }
    /* Test if there are client indices. */
    else if (IndexInfo->indexMemory != gcvNULL)
    {
        gcmCOMPUTE_INDEX_BYTES(count32);
        /* Upload the data to the dynamic index buffer. */
        gcmONERROR(gcoINDEX_UploadDynamicEx(Vertex->dynamicIndex,
                                            IndexInfo->indexType,
                                            IndexInfo->indexMemory,
                                            bytes,
                                            gcvFALSE));

        /* Bind the index buffer to the hardware. */
        gcmONERROR(gcoINDEX_BindDynamic(Vertex->dynamicIndex, IndexInfo->indexType));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**************************************************************************************
**
** gcoVERTEXARRAY_StreamBind
**
** Bind stream data to HW
**
** Now sperate bind stream / index to hw.
** 1) many app/game only update index perframe vertex data not change,
** 2) many internal special patch will only deal with index.
***************************************************************************************/
gceSTATUS
gcoVERTEXARRAY_StreamBind(
    IN gcoVERTEXARRAY Vertex,
#if gcdUSE_WCLIP_PATCH
    IN OUT gctFLOAT * WLimitRms,
    IN OUT gctBOOL * WLimitRmsDirty,
#endif
    IN gcsVERTEXARRAY_STREAM_INFO_CONST_PTR StreamInfo,
    IN gcsVERTEXARRAY_INDEX_INFO_CONST_PTR IndexInfo
    )
{
    gceSTATUS   status;
    gctUINT     i;
    gctUINT     streamCount, count32;
    gctUINT     attributeCount;
    gctUINT     firstCopied;
    gctUINT     minIndex;
    gctUINT     maxIndex;
    gctUINT32   bytes;
    gctUINT     copyCount;
    gctUINT     attribMask;
    gctBOOL     merged;

    /* Pointer to In coming attribute array */
    gcsATTRIBUTE_PTR vertexPtr = StreamInfo->u.es30.attributes;

    /* Stream pointers */
    gcsVERTEXARRAY_BUFOBJ_PTR streams       = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_PTR streamPtr     = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ streamPool[gcdVERTEXARRAY_POOL_CAPACITY];

    /* Attribute pointers */
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR attrPtr = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR prev = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR node = gcvNULL;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE attrPool[gcdVERTEXARRAY_POOL_CAPACITY];

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);
    gcmVERIFY_ARGUMENT(StreamInfo->u.es30.attributes != gcvNULL);
    gcmVERIFY_ARGUMENT(StreamInfo->count > 0);

    gcmSAFECASTSIZET(count32, StreamInfo->count);

    /* No streams so far. */
    streamCount = 0;
    attributeCount = 0;
    copyCount = 0;

    firstCopied = (gctUINT)StreamInfo->first;

    /***************************************************************************
    ***** Phase 1: Information Gathering & Stream Management ******************/

    /* Walk through all attributes. */
    for (i = 0, attribMask = StreamInfo->attribMask;
         attribMask != 0;
         ++i, attribMask >>= 1, ++vertexPtr
    )
    {
        /* Skip disabled vertex attributes. */
        if ((attribMask & 1) == 0)
        {
            continue;
        }

        /* compute bytes */
        gcmCOMPUTE_BYTES();

        /* Find the gcsVERTEXARRAY_BUFOBJ object */
        streamPtr = _findStream(Vertex, streams, vertexPtr, bytes);

        /* Is it a new Stream? Then initialize */
        if (streamPtr == gcvNULL)
        {
            /* Get next stream from the pool. Avoid gcoOS_Allocate */
            if (streamCount < gcdVERTEXARRAY_POOL_CAPACITY )
            {
                streamPtr = &streamPool[streamCount];
            }
            else
            {
                /* Bail out on error */
                gcmONERROR(gcvSTATUS_TOO_COMPLEX);
            }

            /* initialize stream */
            gcoOS_MemFill(streamPtr, 0, sizeof(gcsVERTEXARRAY_BUFOBJ));

            /* check if it is a generic value */
            if (vertexPtr->enable)
            {
                gcmSAFECASTSIZET(streamPtr->stride, vertexPtr->stride);
                streamPtr->divisor = vertexPtr->divisor;
            }
            else
            {
                streamPtr->stride = bytes;
                streamPtr->divisor = 0;
            }

            /* Set count */
            if (streamPtr->divisor == 0)
            {
                streamPtr->count = StreamInfo->count;
            }
            else
            {
                streamPtr->count = (gctSIZE_T)gcoMATH_Ceiling((gctFLOAT)StreamInfo->instanceCount / (gctFLOAT)streamPtr->divisor);
            }

            /* Is this a stream? */
            if (vertexPtr->stream != gcvNULL)
            {
                gcsSURF_NODE_PTR node;

                /* get logical and physical addresses of the stream */
                streamPtr->stream = vertexPtr->stream;
                gcoBUFOBJ_FastLock(streamPtr->stream, &streamPtr->physical, (gctPOINTER *) &streamPtr->logical);

                gcoBUFOBJ_GetNode(streamPtr->stream, &node);
                streamPtr->nodePtr = node;
            }
            else
            {
                copyCount++;
            }

            /* Add this to streams */
            if (streams == gcvNULL)
            {
                streams = streamPtr;
            }
            else
            {
                /* Always keep streamPool stream order */
                streamPool[streamCount - 1].next = streamPtr;
            }

            /* increase stream count */
            streamCount++;
        }
        gcmASSERT(streamPtr);
        /* We have our stream */
        {
            attrPtr = gcvNULL;

            /* Get next attribute from the pool. Avoid gcoOS_Allocate */
            if (attributeCount < gcdVERTEXARRAY_POOL_CAPACITY )
            {
                attrPtr = &attrPool[attributeCount];
            }
            else
            {
                /* Bail out on error */
                gcmONERROR(gcvSTATUS_TOO_COMPLEX);
            }

            /* initialize atribute */
            gcoOS_MemFill(attrPtr, 0, sizeof(gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE));

            if (vertexPtr->enable)
            {
                attrPtr->bytes      = bytes;
                attrPtr->format     = vertexPtr->format;
                attrPtr->linkage    = vertexPtr->linkage;
                attrPtr->size       = vertexPtr->size;
                attrPtr->normalized = vertexPtr->normalized;
                attrPtr->offset     = gcmPTR2SIZE(vertexPtr->pointer);
                attrPtr->enabled    = gcvTRUE;
#if gcdUSE_WCLIP_PATCH
                attrPtr->isPosition = vertexPtr->isPosition;
#endif

                /* We need to know original attr stride when merging */
                attrPtr->stride     = streamPtr->stride;

                /* Is this a client array? */
                if (vertexPtr->stream == gcvNULL)
                {
                    attrPtr->pointer = vertexPtr->pointer;
                }
                else if ((vertexPtr->stream != gcvNULL) && (streamPtr->stride > Vertex->maxStride))
                {
                    copyCount++;
                    attrPtr->pointer = (gctCONST_POINTER)(streamPtr->logical + attrPtr->offset);
                    streamPtr->stream = gcvNULL;
                }
                else
                {
                    attrPtr->pointer = gcvNULL;
                }

#if gcdUSE_WCLIP_PATCH
                /* Fix wLimit issue */
                if ((attrPtr->isPosition == gcvTRUE)
                    && (attrPtr->format == gcvVERTEX_FLOAT)
                    && (attrPtr->bytes/4 == 3)
                    && (WLimitRms != gcvNULL)
                    && (streamPtr->stream != gcvNULL)
                    )
                {
                    computeWLimit(
                            (gctFLOAT_PTR)(streamPtr->logical + attrPtr->offset + StreamInfo->first * attrPtr->stride),
                            attrPtr->bytes / 4,
                            attrPtr->stride,
                            (gctUINT)streamPtr->count,
                            WLimitRms,
                            WLimitRmsDirty);
                }
#endif
            }
            else
            {
                attrPtr->bytes      = bytes;
                attrPtr->format     = gcvVERTEX_FLOAT;
                attrPtr->pointer    = vertexPtr->genericValue;
                attrPtr->size       = vertexPtr->genericSize;
                attrPtr->linkage    = vertexPtr->linkage;
            }

            /* Sort this attribute by offset. */
            for (node = streamPtr->attributePtr, prev = gcvNULL;
                 node != gcvNULL;
                 node = node->next)
            {
                /* Check if we have found an attribute with a larger offset. */
                if (node->offset > attrPtr->offset)
                {
                    break;
                }

                /* Save attribute and move to next one. */
                prev = node;
            }

            /* Link attribute to this stream. */
            attrPtr->next = node;
            if (prev == gcvNULL)
            {
                streamPtr->attributePtr = attrPtr;
            }
            else
            {
                prev->next = attrPtr;
            }

            /* Update maxAttribOffset if tail */
            if (!node)
            {
                streamPtr->maxAttribOffset = attrPtr->offset;
            }

            /* increase attribute count */
            streamPtr->attributeCount++;
            attributeCount++;
        }
    }

    /* Check for attribute count */
    if (attributeCount > Vertex->maxAttribute)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    /* Check for aliasing older hardware has issues with. */
    if ((streamCount == 1)
    &&  (attributeCount == 2)
    &&  (streams->stream != gcvNULL)
    &&  (streams->attributePtr != gcvNULL)
    &&  (streams->attributePtr->next != gcvNULL)
    &&  (streams->attributePtr->offset ==  streams->attributePtr->next->offset)
    &&  (streams->attributePtr->bytes < 8)
    &&  (StreamInfo->count > 31)
    )
    {
        /* TBD: Handle this problem as 4.6.9 does */
        gcmASSERT(0);
    }

    merged = gcvFALSE;
    if (streamCount > Vertex->maxStreams)
    {
        /* Try to merge client streams first */
        if ((streamCount > Vertex->maxStreams) && (copyCount > 1))
        {
            gcmONERROR(gcoVERTEXARRAY_MergeClientStreams(streams, Vertex->maxStreams, &streamCount, &copyCount));
        }

        /* Then try to merge video memory streams */
        if (streamCount > Vertex->maxStreams)
        {
            gcmONERROR(gcoVERTEXARRAY_MergeAllStreams(streams, Vertex->maxStreams, &streamCount, &copyCount));
        }

        /* Bail out if we still exceed max streams. */
        if (streamCount > Vertex->maxStreams)
        {
            gcmONERROR(gcvSTATUS_TOO_COMPLEX);
        }

        merged = gcvTRUE;
    }

    /***************************************************************************
    ***** Phase 2: Bind Hardware *******************************************/
    if ((copyCount > 0) || (merged))
    {
        /* Test if there is a gcoINDEX object. */
        minIndex = 0;
        maxIndex = 0;

        if (IndexInfo->u.es30.indexBuffer != gcvNULL)
        {
            /* Get the index range. */
            gcmONERROR(gcoBUFOBJ_IndexGetRange(IndexInfo->u.es30.indexBuffer,
                                               IndexInfo->indexType,
                                               gcmPTR2SIZE(IndexInfo->indexMemory),
                                               count32,
                                               &minIndex,
                                               &maxIndex));
        }
        /* Test if there is an index array. */
        else if (IndexInfo->indexMemory != gcvNULL)
        {
            /* Get the index range. */
            gcmONERROR(gcoINDEX_GetMemoryIndexRange(IndexInfo->indexType,
                                                    IndexInfo->indexMemory,
                                                    count32,
                                                    &minIndex,
                                                    &maxIndex));
        }

        /* Any kind of index at all? */
        if ((IndexInfo->u.es30.indexBuffer != gcvNULL) || (IndexInfo->indexMemory != gcvNULL))
        {
            /* Adjust new counts for all streams that will be copied */
            for (streamPtr = streams; streamPtr != gcvNULL; streamPtr = streamPtr->next)
            {
                if ((streamPtr->divisor == 0) && (streamPtr->stream == gcvNULL))
                {
                    streamPtr->count = (gctSIZE_T) (maxIndex - minIndex + 1);
                }
            }

            /* Get adjusted first */
            firstCopied = minIndex;
        }

        /* Copy the vertex data. */
        gcmONERROR(gcoSTREAM_CacheAttributesEx(Vertex->dynamicStream,
                                               streamCount,
                                               streams,
                                               firstCopied,
                                               &Vertex->uncacheableStream
                                               ));
    }

    /* Check Stream pool and fix the issue.*/
    if (!Vertex->hwFeature.hasMixedStreams && (streamCount > 1))
    {
        gcmONERROR(gcoVERTEX_AdjustStreamPoolEx(Vertex->dynamicStream,
                                                streams,
                                                streamCount,
                                                (gctUINT)StreamInfo->first,
                                                firstCopied,
                                                (IndexInfo->u.es30.indexBuffer != gcvNULL) || (IndexInfo->indexMemory != gcvNULL),
                                                &Vertex->uncacheableStream));
    }

    /* Bind Hardware */
    gcmONERROR(gcoHARDWARE_SetVertexArrayEx(gcvNULL,
                                            StreamInfo->instanced,
                                            (IndexInfo->u.es30.indexBuffer != gcvNULL) || (IndexInfo->indexMemory != gcvNULL),
                                            streamCount,
                                            streams,
                                            (gctUINT)StreamInfo->first,
                                            firstCopied,
                                            StreamInfo->vertexInstIndex
                                            ));

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

gceSTATUS gcoVERTEXARRAY_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEXARRAY * Vertex
    )
{
    *Vertex = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEXARRAY_Destroy(
    IN gcoVERTEXARRAY Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoVERTEXARRAY_Bind(
    IN gcoVERTEXARRAY Vertex,
    IN gctUINT32 EnableBits,
    IN gcsVERTEXARRAY_PTR VertexArray,
    IN gctUINT First,
    IN gctSIZE_T * Count,
    IN gceINDEX_TYPE IndexType,
    IN gcoINDEX IndexObject,
    IN gctPOINTER IndexMemory,
    IN OUT gcePRIMITIVE * PrimitiveType,
#if gcdUSE_WCLIP_PATCH
    IN OUT gctUINT * PrimitiveCount,
    IN OUT gctFLOAT * WLimitRms,
    IN OUT gctBOOL * WLimitRmsDirty
#else
    IN OUT gctUINT * PrimitiveCount
#endif
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoVERTEXARRAY_IndexBind(
    IN gcoVERTEXARRAY Vertex,
    IN gcsVERTEXARRAY_INDEX_INFO_PTR IndexInfo
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoVERTEXARRAY_IndexBind_Ex(
    IN gcoVERTEXARRAY Vertex,
    IN OUT gcsVERTEXARRAY_STREAM_INFO_PTR StreamInfo,
    IN gcsVERTEXARRAY_INDEX_INFO_PTR IndexInfo
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoVERTEXARRAY_StreamBind(
    IN gcoVERTEXARRAY Vertex,
#if gcdUSE_WCLIP_PATCH
    IN OUT gctFLOAT * WLimitRms,
    IN OUT gctBOOL * WLimitRmsDirty,
#endif
    IN gcsVERTEXARRAY_STREAM_INFO_CONST_PTR StreamInfo,
    IN gcsVERTEXARRAY_INDEX_INFO_CONST_PTR IndexInfo
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoVERTEXARRAY_StreamBind_Ex(
    IN gcoVERTEXARRAY Vertex,
#if gcdUSE_WCLIP_PATCH
    IN OUT gctFLOAT * WLimitRms,
    IN OUT gctBOOL * WLimitRmsDirty,
#endif
    gcsVERTEXARRAY_STREAM_INFO_PTR StreamInfo,
    gcsVERTEXARRAY_INDEX_INFO_PTR IndexInfo
    )
{
    return gcvSTATUS_OK;
}



#endif /* gcdNULL_DRIVER < 2 */
#endif

