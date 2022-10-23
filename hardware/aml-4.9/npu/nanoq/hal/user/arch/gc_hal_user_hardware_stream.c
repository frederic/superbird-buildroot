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


#include "gc_hal_user_hardware_precomp.h"

#if gcdENABLE_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

#define _gcmSETSTATEDATA(StateDelta, Memory, Address, Data) \
    do \
    { \
        gctUINT32 __temp_data32__ = Data; \
        \
        *Memory++ = __temp_data32__; \
        \
        gcoHARDWARE_UpdateDelta(\
            StateDelta, Address, 0, __temp_data32__ \
            ); \
        \
        gcmDUMPSTATEDATA(StateDelta, gcvFALSE, Address, __temp_data32__); \
        \
        Address += 1; \
    } \
    while (gcvFALSE)

#define gcmCOMPUTE_FORMAT_AND_ENDIAN()                                  \
    switch (attrPtr->format)                                            \
    {                                                                   \
    case gcvVERTEX_SHORT:                                               \
        format = 0x2;                   \
        endian = Hardware->bigEndian                                    \
               ? 0x1        \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_UNSIGNED_SHORT:                                      \
        format = 0x3;                  \
        endian = Hardware->bigEndian                                    \
               ? 0x1        \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_INT:                                                 \
        format = 0x4;                     \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_UNSIGNED_INT:                                        \
        format = 0x5;                    \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_FIXED:                                               \
        format = 0xB;            \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_HALF:                                                \
        format = 0x9;                 \
        endian = Hardware->bigEndian                                    \
               ? 0x1        \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_FLOAT:                                               \
        format = 0x8;                   \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_UNSIGNED_INT_10_10_10_2:                             \
        format = 0xD;      \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_INT_10_10_10_2:                                      \
        format = 0xC;       \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_BYTE:                                                \
        format = 0x0;                    \
        endian = 0x0;         \
        break;                                                          \
    case gcvVERTEX_UNSIGNED_BYTE:                                       \
        format = 0x1;                   \
        endian = 0x0;         \
        break;                                                          \
    case gcvVERTEX_UNSIGNED_INT_2_10_10_10_REV:                         \
        format = 0x7;                    \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_INT_2_10_10_10_REV:                                  \
        format = 0x6;                     \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_INT8:                                                \
        format = 0xE;                    \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_INT16:                                               \
        format = 0xF;                   \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    case gcvVERTEX_INT32:                                               \
        if (Hardware->features[gcvFEATURE_MULTI_CLUSTER])               \
            format = 0x10;              \
        else                                                            \
            format = 0x8;               \
        endian = Hardware->bigEndian                                    \
               ? 0x2       \
               : 0x0;         \
        break;                                                          \
    default:                                                            \
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);                         \
    }


/**
 * Program the stream information into the hardware.
 *
 * @Hardware       Hardware object
 * @param Index    Stream number.
 * @param Address  Physical base address of the stream.
 * @param Stride   Stride of the stream in bytes.
 *
 * @return Status of the programming.
 */
gceSTATUS
gcoHARDWARE_SetStream(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Index,
    IN gctUINT32 Address,
    IN gctUINT32 Stride
    )
{
    gceSTATUS status;
    gctUINT32 offset;
    gctBOOL halti2Support;

    gcmHEADER_ARG("Hardward=0x%x Index=%d Address=%u Stride=%d",
                   Hardware, Index, Address, Stride);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    halti2Support = Hardware->features[gcvFEATURE_HALTI2];

    /* Verify the stream index. */
    if (Index >= Hardware->config->streamCount)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Compute the offset.*/
    offset = Index << 2;

    /* Program the stream base address and stride. */

    if (halti2Support)
    {
        gcmONERROR(
            gcoHARDWARE_LoadState32(Hardware,
                                    0x14600 + offset,
                                    Address));

        /* Program the stream stride. */
        gcmONERROR(
            gcoHARDWARE_LoadState32(Hardware,
                                    0x14640 + offset,
                                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:0) - (0 ?
 11:0) + 1))))))) << (0 ?
 11:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))));
    }
    else if (Hardware->config->streamCount > 1)
    {
        gcmONERROR(
            gcoHARDWARE_LoadState32(Hardware,
                                    0x00680 + offset,
                                    Address));

        /* Program the stream stride. */
        gcmONERROR(
            gcoHARDWARE_LoadState32(Hardware,
                                    0x006A0 + offset,
                                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))));
    }
    else
    {
        gcmASSERT(Index == 0);
        gcmONERROR(
            gcoHARDWARE_LoadState32(Hardware,
                                    0x0064C + offset,
                                    Address));

        /* Program the stream stride. */
        gcmONERROR(
            gcoHARDWARE_LoadState32(Hardware,
                                    0x00650 + offset,
                                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))));

    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Program the attributes required for the Vertex Shader into
 * the hardware.
 *
 * @param Hardware  Pointer to the hardware object
 * @param Attributes Pointer to the attribute information.
 * @param AttributeCount
 *                   Number of attributes to program.
 *
 * @return Status of the attributes.
 */
gceSTATUS
gcoHARDWARE_SetAttributes(
    IN gcoHARDWARE Hardware,
    IN gcsVERTEX_ATTRIBUTES_PTR Attributes,
    IN gctUINT32 AttributeCount
    )
{
    gcsVERTEX_ATTRIBUTES_PTR mapping[16];
    gctUINT32 i, j, k, attribCountMax;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 format = 0, size = 0, endian = 0, normalize, fetchBreak, fetchSize;
    gctUINT32 link, linkState = 0;

    gctSIZE_T vertexCtrlStateCount, vertexCtrlReserveCount;
    gctSIZE_T shaderCtrlStateCount, shaderCtrlReserveCount;
    gctSIZE_T feFetchStateCount = 0, feFetchStateReserveCount = 0;
    gctUINT32_PTR vertexCtrl;
    gctUINT32_PTR shaderCtrl;
    gctUINT32_PTR feFetchCmd = gcvNULL;

    gctUINT vertexCtrlState;
    gctUINT shaderCtrlState;
    gctUINT feFetchState = 0;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x Attributes=0x%x AttributeCount=%d",
                  Hardware, Attributes, AttributeCount);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);


    if (Hardware->features[gcvFEATURE_PIPELINE_32_ATTRIBUTES])
    {
        attribCountMax = 32;
    }
    else if (Hardware->features[gcvFEATURE_HALTI0])
    {
        attribCountMax = 16;
    }
    else
    {
        attribCountMax = 12;
    }

    /* Verify the number of attributes. */
    if (AttributeCount >= attribCountMax)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    gcoOS_ZeroMemory(mapping, gcmSIZEOF(mapping));

    /***************************************************************************
    ** Sort all attributes by stream/offset.
    */

    for (i = 0; i < AttributeCount; ++i)
    {
        /* Find the slot for the current attribute. */
        for (j = 0; j < i; ++j)
        {
            if (
                /* If the current attribute's stream lower? */
                (Attributes[i].stream < mapping[j]->stream)

                /* Or is it the same and the offset is lower? */
            ||  ((Attributes[i].stream == mapping[j]->stream)
                && (Attributes[i].offset <  mapping[j]->offset)
                )
            )
            {
                /* We found our slot. */
                break;
            }
        }

        /* Make space at the current slot. */
        for (k = i; k > j; --k)
        {
            mapping[k] = mapping[k - 1];
        }

        /* Save the sorting order. */
        mapping[j] = &Attributes[i];
    }

    /***************************************************************************
    ** Determine the counts and reserve size.
    */

    /* State counts. */
    vertexCtrlStateCount = AttributeCount;
    shaderCtrlStateCount = gcmALIGN(AttributeCount, 4) >> 2;

    /* Reserve counts. */
    vertexCtrlReserveCount = 1 + (vertexCtrlStateCount | 1);
    shaderCtrlReserveCount = 1 + (shaderCtrlStateCount | 1);

    reserveSize = 0;

    if (Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        vertexCtrlState = 0x5E00;
        shaderCtrlState = 0x0230;
        feFetchState    = 0x5EA0;
        feFetchStateCount = AttributeCount;
        feFetchStateReserveCount = 1 + (feFetchStateCount | 1);
        reserveSize = feFetchStateReserveCount * gcmSIZEOF(gctUINT32);
    }
    else
    {
        /* Set initial state addresses. */
        vertexCtrlState = 0x0180;
        shaderCtrlState = 0x0208;
    }

    /* Determine the size of the buffer to reserve. */
    reserveSize
        += (vertexCtrlReserveCount + shaderCtrlReserveCount)
         *  gcmSIZEOF(gctUINT32);

    /***************************************************************************
    ** Reserve command buffer state and init state commands.
    */

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Update the number of the elements. */
    stateDelta->elementCount = AttributeCount + 1;

    if (Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        feFetchCmd = memory; memory += feFetchStateReserveCount;
    }
    /* Determine buffer pointers. */
    vertexCtrl = memory; memory += vertexCtrlReserveCount;
    shaderCtrl = memory; memory += shaderCtrlReserveCount;

    /* Init load state commands. */
    if (Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        *feFetchCmd++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (feFetchStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (feFetchState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
    }

    *vertexCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (vertexCtrlStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (vertexCtrlState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *shaderCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (shaderCtrlStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (shaderCtrlState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    /* Zero out fetch size. */
    fetchSize = 0;

    /* Walk all attributes. */
    for (i = 0; i < AttributeCount; ++i)
    {
        /* Convert format. */
        switch (mapping[i]->format)
        {
        case gcvVERTEX_BYTE:
            format = 0x0;
            endian = 0x0;
            size   = 1;
            break;

        case gcvVERTEX_UNSIGNED_BYTE:
            format = 0x1;
            endian = 0x0;
            size   = 1;
            break;

        case gcvVERTEX_SHORT:
            format = 0x2;
            endian = Hardware->bigEndian
                   ? 0x1
                   : 0x0;
            size   = 2;
            break;

        case gcvVERTEX_UNSIGNED_SHORT:
            format = 0x3;
            endian = Hardware->bigEndian
                   ? 0x1
                   : 0x0;
            size   = 2;
            break;

        case gcvVERTEX_INT:
            format = 0x4;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_UNSIGNED_INT:
            format = 0x5;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_FIXED:
            format = 0xB;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_HALF:
            format = 0x9;
            endian = Hardware->bigEndian
                   ? 0x1
                   : 0x0;
            size   = 2;
            break;

        case gcvVERTEX_FLOAT:
            format = 0x8;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
            format = 0xD;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            break;

        case gcvVERTEX_INT_10_10_10_2:
            format = 0xC;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            break;

        case gcvVERTEX_INT8:
            format = 0xE;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            break;
        case gcvVERTEX_INT16:
            format = 0xF;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            break;
        case gcvVERTEX_INT32:
            format = 0x8;
            endian = Hardware->bigEndian
                   ? 0x2
                   : 0x0;
            break;

        default:
            gcmFATAL("Unknown format: %d", mapping[i]->format);
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        /* Convert normalized. */
        normalize = (mapping[i]->normalized)
                    ? (Hardware->api == gcvAPI_OPENGL)
                      ? (Hardware->currentApi == gcvAPI_OPENGL_ES30)
                        ? 0x1
                        : 0x2
                      : 0x1
                    : 0x0;

        /* Adjust fetch size. */
        fetchSize += size * mapping[i]->components;

        /* Determine if this is a fetch break. */
        fetchBreak =  (i + 1              == AttributeCount)
                   || (mapping[i]->stream != mapping[i + 1]->stream)
                   || (fetchSize          != mapping[i + 1]->offset);

        if (Hardware->features[gcvFEATURE_NEW_GPIPE])
        {
             /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (mapping[i]->components) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (mapping[i]->stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (mapping[i]->offset) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))));

             _gcmSETSTATEDATA(
                stateDelta, feFetchCmd, feFetchState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:11) - (0 ?
 11:11) + 1))))))) << (0 ?
 11:11))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))));
        }
        else
        {
            /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (mapping[i]->components) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (mapping[i]->stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | (((gctUINT32) ((gctUINT32) (mapping[i]->offset) & ((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                );
        }

        /* Compute vertex shader input register for attribute. */
        link = (gctUINT32) (mapping[i] - Attributes);

        /* Set vertex shader input linkage. */
        switch (i & 3)
        {
        case 0:
            linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            break;

        case 1:
            linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
            break;

        case 2:
            linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:16) - (0 ?
 21:16) + 1))))))) << (0 ?
 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
            break;

        case 3:
            linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:24) - (0 ?
 29:24) + 1))))))) << (0 ?
 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

            /* Store the current shader input control value. */
            _gcmSETSTATEDATA(
                stateDelta, shaderCtrl, shaderCtrlState, linkState
                );
            break;

        default:
            break;
        }

        /* Reset fetch size on a break. */
        if (fetchBreak)
        {
            fetchSize = 0;
        }
    }

    /* See if there are any attributes left to program in the vertex shader
    ** shader input registers. */
    if ((i & 3) != 0)
    {
        /* Store the current shader input control value. */
        _gcmSETSTATEDATA(
            stateDelta, shaderCtrl, shaderCtrlState, linkState
            );
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(Hardware, reserve, memory, reserveSize);

    /* Return the status. */
    gcmFOOTER();
    return status;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushVertex(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    /* Verify the input parameters. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->config->chipModel == gcv700
        || Hardware->config->gpuCoreCount > 1
        )
    {
        /* Flush L2 cache for GC700. */
        Hardware->flushL2 = gcvTRUE;
    }

    if (Hardware->features[gcvFEATURE_MULTI_CLUSTER])
    {
        /*Flush vertex cache when needed*/
        Hardware->flushVtxData = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if !OPT_VERTEX_ARRAY
gceSTATUS
gcoHARDWARE_SetVertexArray(
    IN gcoHARDWARE Hardware,
    IN gctUINT First,
    IN gctUINT32 Physical,
    IN gctUINT Stride,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams
    )
{
    gceSTATUS status;
    gctUINT i;
    gctBOOL multiStream;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute;
    gcsVERTEXARRAY_STREAM_PTR streamPtr;
    gctUINT32 streamCount = 0;
    gctUINT32 streamsTotal;
    gctUINT32 attributeCount = 0;
    gctUINT32 attributesTotal;
    gctUINT32 format = 0, size, endian = 0, normalize;
    gctUINT32 offset, fetchSize, fetchBreak;
    gctUINT32 stream, base, stride, link;
    gctUINT linkState = 0, linkCount = 0;

    gcoCMDBUF reserve;
    gcsSTATE_DELTA_PTR stateDelta;
    gctSIZE_T vertexCtrlStateCount, vertexCtrlReserveCount;
    gctSIZE_T shaderCtrlStateCount, shaderCtrlReserveCount;
    gctSIZE_T streamAddressStateCount, streamAddressReserveCount;
    gctSIZE_T streamStrideStateCount, streamStrideReserveCount;
    gctSIZE_T reserveSize;
    gctUINT32_PTR vertexCtrl;
    gctUINT32_PTR shaderCtrl;
    gctUINT32_PTR streamAddress;
    gctUINT32_PTR streamStride;

    gctUINT vertexCtrlState;
    gctUINT shaderCtrlState;
    gctUINT streamAddressState;
    gctUINT streamStrideState;
    gctUINT lastPhysical = 0;

    gcmHEADER_ARG("Hardware=0x%x First=%u Physical=0x%08x Stride=%u "
                  "AttributeCount=%u Attributes=0x%x StreamCount=%u "
                  "Streams=0x%x",
                  Hardware, First, Physical, Stride, AttributeCount, Attributes,
                  StreamCount, Streams);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT((AttributeCount == 0) || (Attributes != gcvNULL));
    gcmDEBUG_VERIFY_ARGUMENT((StreamCount == 0) || (Streams != gcvNULL));

    /* Determine whether the hardware supports more then one stream. */
    multiStream = (Hardware->config->streamCount > 1);

    /***************************************************************************
    ** Determine number of attributes and streams.
    */

    attributesTotal =  AttributeCount;
    streamsTotal    = (AttributeCount > 0) ? 1 : 0;

    for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        /* Get stride and offset for this stream. */
        stream = streamsTotal;
        stride = streamPtr->subStream->stride;
        base   = streamPtr->attribute->offset;

        /* Make sure the streams don't overflow. */
        if ((stream >= Hardware->config->streamCount)
        ||  (stream >= Hardware->config->attribCount)
        )
        {
            gcmONERROR(gcvSTATUS_TOO_COMPLEX);
        }

        /* Next stream. */
        ++streamsTotal;

        /* Walk all attributes in the stream. */
        for (attribute = streamPtr->attribute;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attribute->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamsTotal;
                stride = attribute->vertexPtr->stride;
                base   = attribute->offset;

                /* Make sure the streams don't overflow. */
                if ((stream >= Hardware->config->streamCount)
                ||  (stream >= Hardware->config->attribCount)
                )
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }

                /* Next stream. */
                ++streamsTotal;
            }

            /* Next attribute. */
            ++attributesTotal;
        }
    }

    /* Return error if no attribute is enabled. */
    if (attributesTotal == 0)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    /***************************************************************************
    ** Determine the counts and reserve size.
    */

    /* State counts. */
    vertexCtrlStateCount = attributesTotal;
    shaderCtrlStateCount = gcmALIGN(attributesTotal, 4) >> 2;

    /* Reserve counts. */
    vertexCtrlReserveCount = 1 + (vertexCtrlStateCount | 1);
    shaderCtrlReserveCount = 1 + (shaderCtrlStateCount | 1);

    /* Set initial state addresses. */
    vertexCtrlState = 0x0180;
    shaderCtrlState = 0x0208;

    /* Determine the initial size of the buffer to reserve. */
    reserveSize
        = (vertexCtrlReserveCount + shaderCtrlReserveCount)
        *  gcmSIZEOF(gctUINT32);

    if (multiStream)
    {
        /* State counts. */
        streamAddressStateCount = Hardware->mixedStreams
                                ? streamsTotal
                                : Hardware->config->streamCount;
        streamStrideStateCount  = streamsTotal;

        /* Reserve counts. */
        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 1 + (streamStrideStateCount  | 1);

        /* Set initial state addresses. */
        streamAddressState = 0x01A0;
        streamStrideState  = 0x01A8;

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount)
             *  gcmSIZEOF(gctUINT32);
    }
    else
    {
        /* State counts. */
        streamAddressStateCount = 2;
        streamStrideStateCount  = 0;

        /* Reserve counts. */
        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 0;

        /* Set initial state addresses. */
        streamAddressState = 0x0193;
        streamStrideState  = 0x0194;

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount)
             *  gcmSIZEOF(gctUINT32);
    }

    /***************************************************************************
    ** Reserve command buffer state and init state commands.
    */

    /* Reserve space in the command buffer. */
    gcmONERROR(gcoBUFFER_Reserve(
        Hardware->engine[gcvENGINE_RENDER].buffer, reserveSize, gcvTRUE, &reserve
        ));

    /* Shortcut to the current delta. */
    stateDelta = Hardware->delta;

    /* Update the number of the elements. */
    stateDelta->elementCount = attributesTotal + 1;

    /* Determine buffer pointers. */
    vertexCtrl    = (gctUINT32_PTR) reserve->lastReserve;
    shaderCtrl    = vertexCtrl + vertexCtrlReserveCount;
    streamAddress = shaderCtrl + shaderCtrlReserveCount;
    streamStride  = multiStream
                  ? streamAddress + streamAddressReserveCount
                  : streamAddress + 2;

    /* Init load state commands. */
    *vertexCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (vertexCtrlStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (vertexCtrlState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *shaderCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (shaderCtrlStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (shaderCtrlState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *streamAddress++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (streamAddressStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (streamAddressState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    if (multiStream)
    {
        *streamStride++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (streamStrideStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (streamStrideState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
    }

    /* Process the copied stream first. */
    if (AttributeCount > 0)
    {
        /* Store the stream address. */
        _gcmSETSTATEDATA(
            stateDelta, streamAddress, streamAddressState,
            lastPhysical = Physical - First * Stride
            );

        /* Store the stream stride. */
        _gcmSETSTATEDATA(
            stateDelta, streamStride, streamStrideState,
            multiStream
                ?
 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
            );

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes. */
        for (i = 0, attribute = Attributes, fetchSize = 0;
             i < AttributeCount;
             ++i, ++attribute
        )
        {
            /* Dispatch on vertex format. */
            switch (attribute->format)
            {
            case gcvVERTEX_BYTE:
                format = 0x0;
                endian = 0x0;
                break;

            case gcvVERTEX_UNSIGNED_BYTE:
                format = 0x1;
                endian = 0x0;
                break;

            case gcvVERTEX_SHORT:
                format = 0x2;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_SHORT:
                format = 0x3;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_INT:
                format = 0x4;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT:
                format = 0x5;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_FIXED:
                format = 0xB;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_HALF:
                format = 0x9;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_FLOAT:
                format = 0x8;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
                format = 0xD;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT_10_10_10_2:
                format = 0xC;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT8:
                format = 0xE;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT16:
                format = 0xF;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT32:
                format = 0x8;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Get size. */
            size = attribute->vertexPtr->enable ? (gctUINT)attribute->vertexPtr->size : attribute->bytes / 4;
            link = attribute->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = (attribute->vertexPtr->normalized)
                        ? (Hardware->currentApi == gcvAPI_OPENGL_ES30)
                          ? 0x1
                          : 0x2
                        : 0x0;

            /* Get vertex offset and size. */
            offset      = attribute->offset;
            fetchSize  += attribute->bytes;
            fetchBreak
                = (((i + 1) == AttributeCount)
                || (offset + attribute->bytes != attribute[1].offset))
                    ? 1 : 0;

            /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) (AQ_VERTEX_ELEMENT_CTRL_STREAM_ID_STREAM0 & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | (((gctUINT32) ((gctUINT32) (offset) & ((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                );

            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:16) - (0 ?
 21:16) + 1))))))) << (0 ?
 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:24) - (0 ?
 29:24) + 1))))))) << (0 ?
 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
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

        /* Get stride and offset for this stream. */
        stream = streamCount;
        stride = streamPtr->subStream->stride;
        base   = streamPtr->attribute->offset;

        /* Store the stream address. */
        _gcmSETSTATEDATA(
            stateDelta, streamAddress, streamAddressState,
            lastPhysical = streamPtr->physical + base
            );

        /* Store the stream stride. */
        _gcmSETSTATEDATA(
            stateDelta, streamStride, streamStrideState,
            multiStream
                ?
 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
            );

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes in the stream. */
        for (attribute = streamPtr->attribute, fetchSize = 0;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attribute->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamCount;
                stride = attribute->vertexPtr->stride;
                base   = attribute->offset;

                /* Store the stream address. */
                _gcmSETSTATEDATA(
                    stateDelta, streamAddress, streamAddressState,
                    lastPhysical = streamPtr->physical + base
                    );

                /* Store the stream stride. */
                _gcmSETSTATEDATA(
                    stateDelta, streamStride, streamStrideState,
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                    );

                /* Next stream. */
                ++streamCount;
            }

            /* Dispatch on vertex format. */
            switch (attribute->format)
            {
            case gcvVERTEX_SHORT:
                format = 0x2;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_SHORT:
                format = 0x3;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_INT:
                format = 0x4;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT:
                format = 0x5;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_FIXED:
                format = 0xB;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_HALF:
                format = 0x9;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_FLOAT:
                format = 0x8;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
                format = 0xD;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT_10_10_10_2:
                format = 0xC;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_BYTE:
                format = 0x0;
                endian = 0x0;
                break;

            case gcvVERTEX_UNSIGNED_BYTE:
                format = 0x1;
                endian = 0x0;
                break;

            case gcvVERTEX_INT8:
                format = 0xE;
                endian = 0x0;
                break;

            case gcvVERTEX_INT16:
                format = 0xF;
                endian = 0x0;
                break;

            case gcvVERTEX_INT32:
                format = 0x8;
                endian = 0x0;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Get size. */
            size = attribute->vertexPtr->size;
            link = attribute->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = (attribute->vertexPtr->normalized)
                        ? (Hardware->currentApi == gcvAPI_OPENGL_ES30)
                          ? 0x1
                          : 0x2
                        : 0x0;

            /* Get vertex offset and size. */
            offset      = attribute->offset;
            fetchSize  += attribute->bytes;
            fetchBreak
                =  (((i + 1) == AttributeCount)
                || (attribute->next == gcvNULL)
                || (offset + attribute->bytes != attribute->next->offset))
                    ? 1 : 0;

            /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | (((gctUINT32) ((gctUINT32) (offset - base) & ((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                );

            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:16) - (0 ?
 21:16) + 1))))))) << (0 ?
 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:24) - (0 ?
 29:24) + 1))))))) << (0 ?
 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
        }
    }

    /* Check if the IP requires all streams to be programmed. */
    if (!Hardware->mixedStreams)
    {
        for (i = streamsTotal; i < streamAddressStateCount; ++i)
        {
            /* Replicate the last physical address for unknown stream
            ** addresses. */
            _gcmSETSTATEDATA(
                stateDelta, streamAddress, streamAddressState,
                lastPhysical
                );
        }
    }

    /* See if there are any attributes left to program in the vertex shader
    ** shader input registers. */
    if ((linkCount & 3) != 0)
    {
        _gcmSETSTATEDATA(
            stateDelta, shaderCtrl, shaderCtrlState, linkState
            );
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else

gceSTATUS
gcoHARDWARE_SetVertexArray(
    IN gcoHARDWARE Hardware,
    IN gctBOOL DrawArraysInstanced,
    IN gctUINT First,
    IN gctUINT32 Physical,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams
    )
{
    gceSTATUS status;
    gctUINT i, j;
    gctBOOL multiStream;
    gctBOOL halti2Support;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attrPtr;
    gcsVERTEXARRAY_STREAM_PTR streamPtr;
    gcsVERTEXARRAY_BUFFER_PTR bufferPtr;
    gctUINT32 streamCount = 0;
    gctUINT32 streamsTotal;
    gctUINT32 attributeCount = 0;
    gctUINT32 attributesTotal;
    gctUINT32 format = 0, size, endian = 0, normalize;
    gctUINT32 offset, fetchSize, fetchBreak;
    gctUINT32 stream, base, stride, link, divisor;
    gctUINT linkState = 0, linkCount = 0;

    gctSIZE_T vertexCtrlStateCount, vertexCtrlReserveCount;
    gctSIZE_T shaderCtrlStateCount, shaderCtrlReserveCount;
    gctSIZE_T streamAddressStateCount, streamAddressReserveCount;
    gctSIZE_T streamStrideStateCount, streamStrideReserveCount;
    gctSIZE_T streamDivisorStateCount, streamDivisorReserveCount;
    gctSIZE_T feFetchStateCount = 0, feFetchStateReserveCount = 0;
    gctUINT32_PTR vertexCtrl;
    gctUINT32_PTR shaderCtrl;
    gctUINT32_PTR streamAddress;
    gctUINT32_PTR streamStride;
    gctUINT32_PTR streamDivisor;
    gctUINT32_PTR feFetchCmd = gcvNULL;

    gctUINT vertexCtrlState;
    gctUINT shaderCtrlState;
    gctUINT streamAddressState;
    gctUINT streamStrideState;
    gctUINT streamDivisorState;
    gctUINT lastPhysical = 0;
    gctUINT feFetchState = 0;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x First=%u Physical=0x%08x BufferCount=%u "
                  "Buffers=0x%x StreamCount=%u Streams=0x%x",
                  Hardware, First, Physical, BufferCount, Buffers,
                  StreamCount, Streams);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT((BufferCount == 0) || (Buffers != gcvNULL));
    gcmDEBUG_VERIFY_ARGUMENT((AttributeCount == 0) || (Attributes != gcvNULL));
    gcmDEBUG_VERIFY_ARGUMENT((StreamCount == 0) || (Streams != gcvNULL));

    /* Determine whether the hardware supports more then one stream. */
    multiStream = (Hardware->config->streamCount > 1);
    halti2Support = Hardware->features[gcvFEATURE_HALTI2];
    reserveSize = 0;

    /***************************************************************************
    ** Determine number of attributes and streams.
    */

    attributesTotal = 0;
    streamsTotal    = 0;

    for (i = 0, bufferPtr = Buffers; i < BufferCount; ++i, ++bufferPtr)
    {
        /* Check if we have to skip this buffer. */
        if (bufferPtr->numAttribs == 0)
        {
            continue;
        }

        stream = streamsTotal;

        /* Make sure the streams don't overflow. */
        if ((stream >= Hardware->config->streamCount)
            ||  (stream >= Hardware->config->attribCount)
        )
        {
            gcmONERROR(gcvSTATUS_TOO_COMPLEX);
        }

        attributesTotal += bufferPtr->numAttribs;
        streamsTotal++;
    }

    /* If attribute total in buffers isn't the same as Attribute */
    /* Something must be wrong. */
    if (attributesTotal != AttributeCount)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        /* Get stride and offset for this stream. */
        stream = streamsTotal;
        stride = streamPtr->subStream->stride;
        base   = streamPtr->attribute->offset;

        /* Make sure the streams don't overflow. */
        if ((stream >= Hardware->config->streamCount)
        ||  (stream >= Hardware->config->attribCount)
        )
        {
            gcmONERROR(gcvSTATUS_TOO_COMPLEX);
        }

        /* Next stream. */
        ++streamsTotal;

        /* Walk all attributes in the stream. */
        for (attrPtr = streamPtr->attribute;
             attrPtr != gcvNULL;
             attrPtr = attrPtr->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attrPtr->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamsTotal;
                stride = attrPtr->vertexPtr->stride;
                base   = attrPtr->offset;

                /* Make sure the streams don't overflow. */
                if ((stream >= Hardware->config->streamCount)
                ||  (stream >= Hardware->config->attribCount)
                )
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }

                /* Next stream. */
                ++streamsTotal;
            }

            /* Next attribute. */
            ++attributesTotal;
        }
    }

    /* Return error if no attribute is enabled. */
    if (attributesTotal == 0)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    /***************************************************************************
    ** Determine the counts and reserve size.
    */

    /* State counts. */
    vertexCtrlStateCount = attributesTotal;
    shaderCtrlStateCount = gcmALIGN(attributesTotal, 4) >> 2;

    /* Reserve counts. */
    vertexCtrlReserveCount = 1 + (vertexCtrlStateCount | 1);
    shaderCtrlReserveCount = 1 + (shaderCtrlStateCount | 1);

    if (Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        vertexCtrlState = 0x5E00;
        shaderCtrlState = 0x0230;
    }
    else
    {
        /* Set initial state addresses. */
        vertexCtrlState = 0x0180;
        shaderCtrlState = 0x0208;
    }

    if (Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        feFetchStateCount = attributesTotal;
        feFetchState = 0x5EA0;
        feFetchStateReserveCount = 1 + (feFetchStateCount | 1);

        reserveSize
            =  feFetchStateReserveCount *  gcmSIZEOF(gctUINT32);
    }

    /* Determine the initial size of the buffer to reserve. */
    reserveSize
        += (vertexCtrlReserveCount + shaderCtrlReserveCount)
         *  gcmSIZEOF(gctUINT32);

    if (multiStream)
    {
        /* State counts. */
        streamAddressStateCount = Hardware->mixedStreams
                                ? streamsTotal
                                : Hardware->config->streamCount;
        streamStrideStateCount  = streamsTotal;

        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 1 + (streamStrideStateCount  | 1);

        if (halti2Support)
        {
            streamDivisorStateCount = streamsTotal;
            streamDivisorReserveCount  = 1 + (streamDivisorStateCount  | 1);

            /* Set initial state addresses. */
            streamAddressState = 0x5180;
            streamStrideState  = 0x5190;
            streamDivisorState  = 0x51A0;
        }
        else
        {
            streamDivisorStateCount = 0;
            streamDivisorReserveCount = 0;

            /* Set initial state addresses. */
            streamAddressState = 0x01A0;
            streamStrideState  = 0x01A8;
            streamDivisorState  = 0;
        }

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount + streamDivisorReserveCount)
            *  gcmSIZEOF(gctUINT32);
    }
    else
    {
        /* State counts. */
        streamAddressStateCount = 2;
        streamStrideStateCount  = 0;
        streamDivisorStateCount = 0;

        /* Reserve counts. */
        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 0;
        streamDivisorReserveCount  = 0;

        /* Set initial state addresses. */
        streamAddressState = 0x0193;
        streamStrideState  = 0x0194;
        streamDivisorState  = 0;

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount)
             *  gcmSIZEOF(gctUINT32);
    }

    /***************************************************************************
    ** Reserve command buffer state and init state commands.
    */

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Update the number of the elements. */
    stateDelta->elementCount = attributesTotal + 1;

    /* Determine buffer pointers. */
    if (Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        feFetchCmd    = memory; memory += feFetchStateReserveCount;
    }
    vertexCtrl    = memory; memory += vertexCtrlReserveCount;
    shaderCtrl    = memory; memory += shaderCtrlReserveCount;
    streamAddress = memory; memory += multiStream ? streamAddressReserveCount : 2;
    streamStride  = memory; memory += streamStrideReserveCount;
    streamDivisor  = memory; memory += streamDivisorReserveCount;

    if (Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        *feFetchCmd++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (feFetchStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (feFetchState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
    }

    /* Init load state commands. */
    *vertexCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (vertexCtrlStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (vertexCtrlState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *shaderCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (shaderCtrlStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (shaderCtrlState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *streamAddress++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (streamAddressStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (streamAddressState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    if (multiStream)
    {
        *streamStride++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (streamStrideStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (streamStrideState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
        if (halti2Support)
        {
            *streamDivisor++
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (streamDivisorStateCount) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (streamDivisorState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
        }
    }

    /* Process the copied stream first. */
    for(i = 0, bufferPtr = Buffers; i < BufferCount; ++i, ++bufferPtr)
    {
        /* Get stride and offset for this stream. */
        stream = streamCount;
        stride = bufferPtr->stride;
        divisor = bufferPtr->divisor;

        /* 1. For instanced attributes, they will not be controlled by "First" and all their data from
        **    base are copied.
        ** 2. For non-instanced attributes:
        **     2.1 If DrawArraysInstanced, the start bias was added to the bufferBase and programmed here,
        **         DrawArraysInstanced cmd itself doesn't support any start bias.
        **     2.2 Otherwise, the bufferBase need to be programmed here, the later draw commands or index
        **         acess will skip to valid data range.
        **         In fact, data are garbage before the start bias for copied buffer, but it doesn't
        **         matter because GPU will skip to read them.
        */
        if (bufferPtr->divisor > 0)
        {
            lastPhysical = Physical + bufferPtr->offset;
        }
        else if (DrawArraysInstanced)
        {
            lastPhysical = Physical + bufferPtr->offset;
        }
        else
        {
            lastPhysical = Physical + bufferPtr->offset - (First * stride);
        }

        /* Store the stream address. */
        _gcmSETSTATEDATA(stateDelta, streamAddress, streamAddressState, lastPhysical);

        if (halti2Support)
        {
            /* Store the stream stride. */
            _gcmSETSTATEDATA(
                stateDelta, streamStride, streamStrideState,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:0) - (0 ?
 11:0) + 1))))))) << (0 ?
 11:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                );

            /* Store the stream divisor. */
            _gcmSETSTATEDATA(
                stateDelta, streamDivisor, streamDivisorState, divisor
                );
        }
        else if (multiStream)
        {
            /* Store the stream stride. */
            _gcmSETSTATEDATA(
                stateDelta, streamStride, streamStrideState,
                (((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (divisor) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))))
                );
        }
        else
        {
            /* Store the stream stride. */
            _gcmSETSTATEDATA(
                stateDelta, streamStride, streamStrideState,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                );
        }

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes. */
        for (j = 0, fetchSize = 0; j < bufferPtr->numAttribs; j++)
        {
            attrPtr = Attributes + bufferPtr->map[j];

            gcmCOMPUTE_FORMAT_AND_ENDIAN();

            /* Get size. */
            size = attrPtr->vertexPtr->enable ? (gctUINT)attrPtr->vertexPtr->size : attrPtr->bytes / 4;
            link = attrPtr->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = (attrPtr->vertexPtr->normalized)
                        ? (Hardware->currentApi == gcvAPI_OPENGL_ES30)
                          ? 0x1
                          : 0x2
                        : 0x0;

            /* Get vertex offset and size. */
            offset      = attrPtr->offset;
            fetchSize  += attrPtr->bytes;
            fetchBreak
                = ((j + 1) == bufferPtr->numAttribs)
                || offset + attrPtr->bytes != Attributes[bufferPtr->map[j+1]].offset
                    ? 1 : 0;
            if (Hardware->features[gcvFEATURE_NEW_GPIPE])
            {
                 /* Store the current vertex element control value. */
                _gcmSETSTATEDATA(
                    stateDelta, vertexCtrl, vertexCtrlState,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (offset) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (format >> 4) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))));

                 _gcmSETSTATEDATA(
                    stateDelta, feFetchCmd, feFetchState,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:11) - (0 ?
 11:11) + 1))))))) << (0 ?
 11:11))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))));

            }
            else
            {
                /* Store the current vertex element control value. */
                _gcmSETSTATEDATA(
                    stateDelta, vertexCtrl, vertexCtrlState,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | (((gctUINT32) ((gctUINT32) (offset) & ((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                    );
            }
            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:16) - (0 ?
 21:16) + 1))))))) << (0 ?
 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:24) - (0 ?
 29:24) + 1))))))) << (0 ?
 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
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

        /* Get stride and offset for this stream. */
        stream  = streamCount;
        stride  = streamPtr->subStream->stride;
        divisor = streamPtr->subStream->divisor;
        base    = streamPtr->attribute->offset;

        /* 1. For instanced attributes, they will not be controlled by "First" and all their data from
        **    base are copied.
        ** 2. For non-instanced attributes:
        **     2.1 If DrawArraysInstanced, the start bias was added to the bufferBase and programmed here,
        **         DrawArraysInstanced cmd itself doesn't support any start bias.
        **     2.2 Otherwise, the bufferBase need to be programmed here, the later draw commands or index
        **         acess will skip to valid data range.
        */
        if (divisor > 0)
        {
            lastPhysical = streamPtr->physical + base;
        }
        else if (DrawArraysInstanced)
        {
            lastPhysical = streamPtr->physical + base + (First * stride);
        }
        else
        {
            lastPhysical = streamPtr->physical + base;
        }

        /* Store the stream address. */
        _gcmSETSTATEDATA(stateDelta, streamAddress, streamAddressState,lastPhysical);

        /* Store the stream stride. */
        if (halti2Support)
        {
            /* Store the stream stride. */
            _gcmSETSTATEDATA(
                stateDelta, streamStride, streamStrideState,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:0) - (0 ?
 11:0) + 1))))))) << (0 ?
 11:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                );

            /* Store the stream divisor. */
            _gcmSETSTATEDATA(
                stateDelta, streamDivisor, streamDivisorState, divisor
                );
        }
        else if (multiStream)
        {
            /* Store the stream stride. */
            _gcmSETSTATEDATA(
                stateDelta, streamStride, streamStrideState,
                (((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (divisor) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))))
                );
        }
        else
        {
            /* Store the stream stride. */
            _gcmSETSTATEDATA(
                stateDelta, streamStride, streamStrideState,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                );
        }

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes in the stream. */
        for (attrPtr = streamPtr->attribute, fetchSize = 0;
             attrPtr != gcvNULL;
             attrPtr = attrPtr->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attrPtr->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamCount;
                stride = attrPtr->vertexPtr->stride;
                base   = attrPtr->offset;

                /* Store the stream address. */
                _gcmSETSTATEDATA(
                    stateDelta, streamAddress, streamAddressState,
                    lastPhysical = streamPtr->physical + base
                    );

                /* Store the stream stride. */
                if (halti2Support)
                {
                    /* Store the stream stride. */
                    _gcmSETSTATEDATA(
                        stateDelta, streamStride, streamStrideState,
                        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:0) - (0 ?
 11:0) + 1))))))) << (0 ?
 11:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                        );

                    /* Store the stream stride. */
                    _gcmSETSTATEDATA(
                        stateDelta, streamDivisor, streamDivisorState,divisor
                        );
                }
                else
                {
                    /* Store the stream stride. */
                    _gcmSETSTATEDATA(
                        stateDelta, streamStride, streamStrideState,
                        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                        );
                }

                /* Next stream. */
                ++streamCount;
            }

            gcmCOMPUTE_FORMAT_AND_ENDIAN();

            /* Get size. */
            size = attrPtr->vertexPtr->size;
            link = attrPtr->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = (attrPtr->vertexPtr->normalized)
                        ? (Hardware->currentApi == gcvAPI_OPENGL_ES30)
                          ? 0x1
                          : 0x2
                        : 0x0;

            /* Get vertex offset and size. */
            offset      = attrPtr->offset;
            fetchSize  += attrPtr->bytes;
            fetchBreak
                = (attrPtr->next == gcvNULL)
                || (offset + attrPtr->bytes != attrPtr->next->offset)
                || (attrPtr->next->offset >= base + stride)
                    ? 1 : 0;

            if (Hardware->features[gcvFEATURE_NEW_GPIPE])
            {
                 /* Store the current vertex element control value. */
                _gcmSETSTATEDATA(
                    stateDelta, vertexCtrl, vertexCtrlState,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (offset - base) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (format >> 4) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))));

                 _gcmSETSTATEDATA(
                    stateDelta, feFetchCmd, feFetchState,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:11) - (0 ?
 11:11) + 1))))))) << (0 ?
 11:11))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))));

            }
            else
            {
                /* Store the current vertex element control value. */
                _gcmSETSTATEDATA(
                    stateDelta, vertexCtrl, vertexCtrlState,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | (((gctUINT32) ((gctUINT32) (offset - base) & ((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                    );
            }
            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:16) - (0 ?
 21:16) + 1))))))) << (0 ?
 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:24) - (0 ?
 29:24) + 1))))))) << (0 ?
 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
        }
    }

    /* Check if the IP requires all streams to be programmed. */
    if (!Hardware->mixedStreams)
    {
        for (i = streamsTotal; i < streamAddressStateCount; ++i)
        {
            /* Replicate the last physical address for unknown stream
            ** addresses. */
            _gcmSETSTATEDATA(
                stateDelta, streamAddress, streamAddressState,
                lastPhysical
                );
        }
    }

    /* See if there are any attributes left to program in the vertex shader
    ** shader input registers. */
    if ((linkCount & 3) != 0)
    {
        _gcmSETSTATEDATA(
            stateDelta, shaderCtrl, shaderCtrlState, linkState
            );
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(Hardware, reserve, memory, reserveSize);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoHARDWARE_SetVertexArrayEx(
    IN gcoHARDWARE Hardware,
    IN gctBOOL DrawInstanced,
    IN gctBOOL DrawElements,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gctINT  StartVertex,
    IN gctUINT FirstCopied,
    IN gctINT VertexInstanceIdLinkage
    )
{
    gcsVERTEXARRAY_BUFOBJ_PTR streamPtr;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR attrPtr;
    gceSTATUS status = gcvSTATUS_OK;

    gctUINT i, stream;
    gctUINT attributeCount;
    gctUINT linkState = 0;
    gctUINT linkCount = 0;

    gctUINT32 lastPhysical = 0;

    /* The flag of flush vertex cache */
    gctBOOL bFlushVertexCache = gcvFALSE;
    gctPOINTER *outside = gcvNULL;
    gctUINT attribIndex = 0, shaderInputIndex = 0;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x DrawInstanced=%u DrawElements=%u "
                  "StreamCount=%u Streams=0x%x StartVertex=%u FirstCopied=%u VertexInstanceIdLinkage=%d",
                  Hardware, DrawInstanced, DrawElements, StreamCount, Streams, StartVertex, FirstCopied,
                  VertexInstanceIdLinkage);

    /* Get Hardware */
    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Process streams */
    attributeCount = 0;
    for(streamPtr = Streams; streamPtr != gcvNULL; streamPtr = streamPtr->next)
    {
        attributeCount += streamPtr->attributeCount;

        if (streamPtr->stream)
        {
            gcsSURF_NODE_PTR node = gcvNULL;

            gcoBUFOBJ_GetNode(streamPtr->stream, &node);

            if (node != gcvNULL && node->bCPUWrite)
            {
                bFlushVertexCache = gcvTRUE;
                /* Reset flag.*/
                node->bCPUWrite = gcvFALSE;
            }
        }
    }

    if (bFlushVertexCache)
    {
        gcmVERIFY_OK(gcoHARDWARE_FlushVertex(gcvNULL));
    }
    /* Reset streamCount to 0, as we flush stream everytime */
    Hardware->hwSlot->renderEngine.streamCount = 0;
    Hardware->hwSlot->renderEngine.streamMaxCount = Hardware->config->streamCount;
    gcoOS_ZeroMemory(Hardware->hwSlot->renderEngine.streams, gcmSIZEOF(gctUINT32) * gcdSTREAMS);

    if (attributeCount == Hardware->config->attribCount &&
        !((Hardware->config->attribCount == 16) &&
          (VertexInstanceIdLinkage == 16) &&
          Hardware->features[gcvFEATURE_NEW_GPIPE]))
    {
        VertexInstanceIdLinkage = -1;
    }

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    if (attributeCount > 0)
    {
        /* Update the number of the elements. */
        stateDelta->elementCount = attributeCount + 1;

        /* Walk all stream objects. */
        for (stream = 0, streamPtr = Streams; stream < StreamCount; streamPtr = streamPtr->next, ++stream)
        {
            gctUINT stride;
            gctUINT divisor;
            gctUINT32 base;
            gctUINT32 fetchSize = 0;
            /* Check if we have to skip this stream. */
            if (streamPtr->logical == gcvNULL)
            {
                continue;
            }

            stride = streamPtr->stream == gcvNULL ? streamPtr->dynamicCacheStride : streamPtr->stride;
            divisor = streamPtr->divisor;
            /* now, the offset become the real offset, not a pointer address, < 4GB is safe.*/
            gcmSAFECASTSIZET(base, streamPtr->attributePtr->offset);

            if (streamPtr->stream != gcvNULL)
            {
#if gcdENABLE_KERNEL_FENCE
                gcoHARDWARE_SetHWSlot(Hardware, gcvENGINE_RENDER, gcvHWSLOT_STREAM, streamPtr->nodePtr->u.normal.node, stream);
#endif
                if (DrawInstanced)
                {
                    if (Hardware->features[gcvFEATURE_FE_START_VERTEX_SUPPORT])
                    {
                        if (divisor && !Hardware->features[gcvFEATURE_DIVISOR_STREAM_ADDR_FIX])
                        {
                            lastPhysical = streamPtr->physical + base - (StartVertex * stride);
                        }
                        else
                        {
                            lastPhysical = streamPtr->physical + base;
                        }
                    }
                    else
                    {
                        if (divisor)
                        {
                            lastPhysical = streamPtr->physical + base;
                        }
                        else
                        {
                            lastPhysical = streamPtr->physical + base + (StartVertex * stride);
                        }
                    }
                }
                else
                {
                    gcmASSERT(!divisor || Hardware->features[gcvFEATURE_DIVISOR_STREAM_ADDR_FIX]);
                    lastPhysical = streamPtr->physical + base;
                }
            }
            else
            {
                if (DrawElements)
                {
                    if (divisor)
                    {
                        if (Hardware->features[gcvFEATURE_DIVISOR_STREAM_ADDR_FIX])
                        {
                            lastPhysical = streamPtr->physical + base;
                        }
                        else
                        {
                            lastPhysical = streamPtr->physical + base - (StartVertex * stride);
                        }
                    }
                    else
                    {
                        lastPhysical = streamPtr->physical + base - ((StartVertex + FirstCopied) * stride);
                    }
                }
                else
                {
                    if (DrawInstanced)
                    {
                        gcmASSERT((gctUINT)StartVertex == FirstCopied);
                        if (Hardware->features[gcvFEATURE_FE_START_VERTEX_SUPPORT])
                        {
                            if (divisor && Hardware->features[gcvFEATURE_DIVISOR_STREAM_ADDR_FIX])
                            {
                                lastPhysical = streamPtr->physical + base;
                            }
                            else
                            {
                                lastPhysical = streamPtr->physical + base - (FirstCopied * stride);
                            }
                        }
                        else
                        {
                            lastPhysical = streamPtr->physical + base;
                        }
                    }
                    else
                    {
                        gcmASSERT(divisor == 0);
                        lastPhysical = streamPtr->physical + base - (FirstCopied * stride);
                    }
                }
            }

            /* Store the stream address. */
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->streamAddressState + stream) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->streamAddressState + stream, lastPhysical);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (Hardware->robust)
            {
                gctUINT32 endAddress;
                gctUINT32 bufBaseAddr;
                gctUINT32 bufSize;
                gcmASSERT(streamPtr->nodePtr);
                gcmSAFECASTSIZET(bufSize, streamPtr->nodePtr->size);
                gcmGETHARDWAREADDRESS(*streamPtr->nodePtr, bufBaseAddr);
                endAddress = bufBaseAddr + bufSize - 1;
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x51B0 + stream) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x51B0 + stream, endAddress);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }

            if (Hardware->streamRegV2)
            {
                /* Store the stream stride. */
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->streamStrideState + stream) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->streamStrideState + stream, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:0) - (0 ?
 11:0) + 1))))))) << (0 ?
 11:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 11:0) - (0 ?
 11:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                /* Store the stream divisor. */
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x51A0 + stream) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x51A0 + stream, divisor);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
            else
            {
                /* Store the stream stride/ and divisor */
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->streamStrideState + stream) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->streamStrideState + stream, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (divisor) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }

            /* Walk all attributes in the stream. */
            for (attrPtr = streamPtr->attributePtr, fetchSize = 0; attrPtr != gcvNULL; attrPtr = attrPtr->next)
            {
                gctUINT32 link = attrPtr->linkage;
                gctUINT32 offset = 0;
                gctUINT32 normalize;
                gctUINT32 fetchBreak = 0;
                gctUINT32 format = 0;
                gctUINT32 endian = 0;

                gcmCOMPUTE_FORMAT_AND_ENDIAN();

                /* Now, the offset become the real offset, not a pointer address, < 4GB is safe.*/
                gcmSAFECASTSIZET(offset, attrPtr->offset);

                /* Get normalized flag. */
                normalize = (attrPtr->normalized)
                            ? (Hardware->currentApi == gcvAPI_OPENGL_ES30)
                              ? 0x1
                              : 0x2
                            : 0x0;

                /* Get vertex offset and size. */
                fetchSize  += attrPtr->bytes;
                fetchBreak = (attrPtr->next == gcvNULL) ||
                             (offset + attrPtr->bytes != attrPtr->next->offset) ||
                             (attrPtr->next->offset >= base + stride)
                           ? 1 : 0;

                if (Hardware->features[gcvFEATURE_NEW_GPIPE])
                {
                     /* Store the current vertex element control value. */
                     {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x5E00 + attribIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x5E00 + attribIndex, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (attrPtr->size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (offset - base) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (format >> 4) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};


                     {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x5EA0 + attribIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x5EA0 + attribIndex, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:11) - (0 ?
 11:11) + 1))))))) << (0 ?
 11:11))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }
                else
                {
                     /* Store the current vertex element control value. */
                     {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0180 + attribIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0180 + attribIndex, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (attrPtr->size) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | (((gctUINT32) ((gctUINT32) (offset - base) & ((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }

                /* Set vertex shader input linkage. */
                switch (linkCount & 3)
                {
                case 0:
                    linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                    break;
                case 1:
                    linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                    break;
                case 2:
                    linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:16) - (0 ?
 21:16) + 1))))))) << (0 ?
 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                    break;
                case 3:
                    linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:24) - (0 ?
 29:24) + 1))))))) << (0 ?
 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));
                    /* Store the current shader input control value. */
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->shaderCtrlState + shaderInputIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->shaderCtrlState + shaderInputIndex, linkState);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    ++shaderInputIndex;
                    break;
                default:
                    break;
                }

                /* Next vertex shader linkage. */
                ++linkCount;

                if (fetchBreak)
                {
                    /* Reset fetch size on a break. */
                    fetchSize = 0;
                }

                if (Hardware->features[gcvFEATURE_GENERIC_ATTRIB] ||
                    Hardware->features[gcvFEATURE_NEW_GPIPE])
                {
                    gctUINT32 wDefaultValue = (attrPtr->format == gcvVERTEX_INT8  ||
                        attrPtr->format == gcvVERTEX_INT16 ||
                        attrPtr->format == gcvVERTEX_INT32)
                                             ? 0x1 : 0x3F800000;
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->genericWState + attribIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->genericWState + attribIndex, wDefaultValue);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                attribIndex++;
            }
        }

        /* Check if the IP requires all streams to be programmed. */
        if (!Hardware->mixedStreams)
        {
            for (i = StreamCount; i < Hardware->config->streamCount; ++i)
            {
                /* Replicate the last physical address for unknown stream
                ** addresses.
                */
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->streamAddressState + i) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->streamAddressState + i, lastPhysical);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
        }

        if (Hardware->features[gcvFEATURE_NEW_GPIPE])
        {
            if (VertexInstanceIdLinkage != -1)
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x01F1) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x01F1, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:8) - (0 ?
 14:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:8) - (0 ?
 14:8) + 1))))))) << (0 ?
 14:8))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage * 4) & ((gctUINT32) ((((1 ?
 14:8) - (0 ?
 14:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:8) - (0 ?
 14:8) + 1))))))) << (0 ?
 14:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:16) - (0 ?
 22:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:16) - (0 ?
 22:16) + 1))))))) << (0 ?
 22:16))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage * 4 + 1) & ((gctUINT32) ((((1 ?
 22:16) - (0 ?
 22:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
            else
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x01F1) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x01F1, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
         }
        /* See if there are any attributes left to program in the vertex shader
        ** shader input registers. And also check if we need to add vertex instance linkage
        */
        if (((linkCount & 3) != 0) || (VertexInstanceIdLinkage != -1))
        {
            /* Do we need to add vertex instance linkage? */
            if (VertexInstanceIdLinkage != -1)
            {
                /* Set vertex shader input linkage. */
                switch (linkCount & 3)
                {
                case 0:
                    linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage ) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                    break;
                case 1:
                    linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage ) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                    break;
                case 2:
                    linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 21:16) - (0 ?
 21:16) + 1))))))) << (0 ?
 21:16))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage ) & ((gctUINT32) ((((1 ?
 21:16) - (0 ?
 21:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                    break;
                case 3:
                    linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:24) - (0 ?
 29:24) + 1))))))) << (0 ?
 29:24))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage ) & ((gctUINT32) ((((1 ?
 29:24) - (0 ?
 29:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));
                    break;
                default:
                    break;
                }

            }
            /* Program attributes */
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->shaderCtrlState + shaderInputIndex) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->shaderCtrlState + shaderInputIndex, linkState);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            shaderInputIndex++;
        }
    }
    else
    {
        if (Hardware->features[gcvFEATURE_ZERO_ATTRIB_SUPPORT])
        {
            if (VertexInstanceIdLinkage == -1)
            {
                VertexInstanceIdLinkage = 0;
            }

            /* Update the number of the elements. */
            stateDelta->elementCount = 1;

            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x01F2) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x01F2, 1);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (Hardware->features[gcvFEATURE_NEW_GPIPE])
            {
                if (VertexInstanceIdLinkage != -1)
                {
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x01F1) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x01F1, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) ((gctUINT32) (0x3) & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:8) - (0 ?
 14:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:8) - (0 ?
 14:8) + 1))))))) << (0 ?
 14:8))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage * 4) & ((gctUINT32) ((((1 ?
 14:8) - (0 ?
 14:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:8) - (0 ?
 14:8) + 1))))))) << (0 ?
 14:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:16) - (0 ?
 22:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:16) - (0 ?
 22:16) + 1))))))) << (0 ?
 22:16))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage * 4 + 1) & ((gctUINT32) ((((1 ?
 22:16) - (0 ?
 22:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }
                else
                {
                    {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x01F1) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x01F1, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                }
            }
            gcmASSERT(VertexInstanceIdLinkage != -1);
            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->shaderCtrlState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->shaderCtrlState, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }
        else
        {
            gctUINT32 addr = 0;
            gcmASSERT(!Hardware->features[gcvFEATURE_NEW_GPIPE]);

            /* Hardware  have a read operation ,so we create a buffer and program a address for reading. */
            if(Hardware->tempBuffer.valid == gcvFALSE)
            {
                gcmONERROR(gcsSURF_NODE_Construct(&Hardware->tempBuffer, 16, 16, gcvSURF_VERTEX, 0, gcvPOOL_DEFAULT));
                gcmONERROR(gcoHARDWARE_Lock(&Hardware->tempBuffer, &addr, gcvNULL));
            }
            else
            {
                gcmGETHARDWAREADDRESS(Hardware->tempBuffer, addr);
            }
            /* Update the number of the elements. */
            stateDelta->elementCount = 2;

            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->streamAddressState) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, Hardware->streamAddressState, addr);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0180) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0180, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 13:12) - (0 ?
 13:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:12) - (0 ?
 13:12) + 1))))))) << (0 ?
 13:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 15:14) - (0 ?
 15:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:14) - (0 ?
 15:14) + 1))))))) << (0 ?
 15:14))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 5:4) - (0 ?
 5:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:4) - (0 ?
 5:4) + 1))))))) << (0 ?
 5:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 6:6) - (0 ?
 6:6) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:6) - (0 ?
 6:6) + 1))))))) << (0 ?
 6:6))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 11:8) - (0 ?
 11:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:8) - (0 ?
 11:8) + 1))))))) << (0 ?
 11:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 23:16) - (0 ?
 23:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:16) - (0 ?
 23:16) + 1))))))) << (0 ?
 23:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) (4) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:7) - (0 ?
 7:7) + 1))))))) << (0 ?
 7:7))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 7:7) - (0 ?
 7:7) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (VertexInstanceIdLinkage != -1)
            {
                {    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)1 <= 1024);
    *memory++        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16)))        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0208) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0208, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 5:0) - (0 ?
 5:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:0) - (0 ?
 5:0) + 1))))))) << (0 ?
 5:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 13:8) - (0 ?
 13:8) + 1))))))) << (0 ?
 13:8))) | (((gctUINT32) ((gctUINT32) (VertexInstanceIdLinkage) & ((gctUINT32) ((((1 ?
 13:8) - (0 ?
 13:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
        }
    }
    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


#endif

#endif


