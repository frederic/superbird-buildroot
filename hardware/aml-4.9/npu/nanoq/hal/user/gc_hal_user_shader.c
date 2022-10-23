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
**
** Manage shader related stuff except compiler in HAL
**
*/

#include "gc_hal_user_precomp.h"

#if gcdENABLE_3D

#define _GC_OBJ_ZONE      gcdZONE_SHADER

gceSTATUS gcQueryShaderCompilerHwCfg(
    IN  gcoHAL Hal,
    OUT PVSC_HW_CONFIG pVscHwCfg)
{
    gceSTATUS status;

    gcmHEADER_ARG("Hal=0x%x pVscHwCfg=%d", Hal, pVscHwCfg);

    /* Call down to the hardware object. */
    status = gcoHARDWARE_QueryShaderCompilerHwCfg(gcvNULL, pVscHwCfg);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                                gcLoadShaders
********************************************************************************
**
**  Load a pre-compiled and pre-linked shader program into the hardware.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gcsPROGRAM_STATE *ProgramState
**          program state.
*/
gceSTATUS
gcLoadShaders(
    IN gcoHAL Hal,
    IN gcsPROGRAM_STATE *ProgramState
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hal=0x%x StateBufferSize=%u StateBuffer=0x%x Hints=0x%x",
        Hal, ProgramState->stateBufferSize, ProgramState->stateBuffer, ProgramState->hints);

    /* Call down to the hardware object. */
    status = gcoHARDWARE_LoadProgram(gcvNULL, ProgramState->hints->stageBits, ProgramState);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcInvokeThreadWalker(
    IN gcoHARDWARE Hardware,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Info=0x%x", Hardware, Info);

    /* Call down to the hardware object. */
    status = gcoHARDWARE_InvokeThreadWalkerCL(Hardware, Info);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSHADER_ProgramUniform(
    IN gcoHAL Hal,
    IN gctUINT32 Address,
    IN gctUINT Columns,
    IN gctUINT Rows,
    IN gctCONST_POINTER Values,
    IN gctBOOL FixedPoint,
    IN gctBOOL ConvertToFloat,
    IN gcSHADER_KIND Type
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hal=0x%x, Address=%u Columns=%u Rows=%u Values=%p FixedPoint=%d Type=%d",
        Hal, Address, Columns, Rows, Values, FixedPoint, Type);

    status = gcoHARDWARE_ProgramUniform(gcvNULL, Address, Columns, Rows, Values,
                                        FixedPoint, ConvertToFloat, Type);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSHADER_ProgramUniformEx(
    IN gcoHAL Hal,
    IN gctUINT32 Address,
    IN gctSIZE_T Columns,
    IN gctSIZE_T Rows,
    IN gctSIZE_T Arrays,
    IN gctBOOL   IsRowMajor,
    IN gctSIZE_T MatrixStride,
    IN gctSIZE_T ArrayStride,
    IN gctCONST_POINTER Values,
    IN gceUNIFORMCVT Convert,
    IN gcSHADER_KIND Type
    )
{
    gceSTATUS status;
    gctUINT32 columns, rows, arrays, matrixStride, arrayStride;

    gcmHEADER_ARG("Hal=0x%x, Address=%u Columns=%u Rows=%u Arrays=%u IsRowMajor=%d "
                  "MatrixStride=%u ArrayStride=%u Values=%p Convert=%d Type=%d",
                  Hal, Address, Columns, Rows, Arrays, IsRowMajor,
                  MatrixStride, ArrayStride, Values, Convert, Type);

    gcmSAFECASTSIZET(columns, Columns);
    gcmSAFECASTSIZET(rows, Rows);
    gcmSAFECASTSIZET(arrays, Arrays);
    gcmSAFECASTSIZET(matrixStride, MatrixStride);
    gcmSAFECASTSIZET(arrayStride, ArrayStride);
    status = gcoHARDWARE_ProgramUniformEx(gcvNULL, Address, columns, rows, arrays, IsRowMajor,
                                          matrixStride, arrayStride, Values, Convert, Type);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSHADER_BindUniform(
    IN gcoHAL Hal,
    IN gctUINT32 Address,
    IN gctINT32 Physical,
    IN gctSIZE_T Columns,
    IN gctSIZE_T Rows,
    IN gctSIZE_T Arrays,
    IN gctBOOL   IsRowMajor,
    IN gctSIZE_T MatrixStride,
    IN gctSIZE_T ArrayStride,
    IN gctCONST_POINTER Values,
    IN gceUNIFORMCVT Convert,
    IN gcSHADER_KIND Type
    )
{
    gceSTATUS status;
    gctUINT32 columns, rows, arrays, matrixStride, arrayStride;

    gcmHEADER_ARG("Hal=0x%x, Address=%u Physical=%d Columns=%u Rows=%u Arrays=%u "
                  "IsRowMajor=%d MatrixStride=%u ArrayStride=%u Values=%p Convert=%d Type=%d",
                  Hal, Address, Physical, Columns, Rows, Arrays, IsRowMajor,
                  MatrixStride, ArrayStride, Values, Convert, Type);

    gcmSAFECASTSIZET(columns, Columns);
    gcmSAFECASTSIZET(rows, Rows);
    gcmSAFECASTSIZET(arrays, Arrays);
    gcmSAFECASTSIZET(matrixStride, MatrixStride);
    gcmSAFECASTSIZET(arrayStride, ArrayStride);

    status = gcoHARDWARE_BindUniformEx(gcvNULL, Address, Physical, columns, rows, arrays, IsRowMajor,
                                       matrixStride, arrayStride, &Values, Convert, Type, gcvFALSE);

    gcmFOOTER();
    return status;
}


gceSTATUS
gcoSHADER_BindUniformCombinedMode(
    IN gcoHAL Hal,
    IN gctUINT32 Address,
    IN gctINT32 Physical,
    IN gctSIZE_T Columns,
    IN gctSIZE_T Rows,
    IN gctSIZE_T Arrays,
    IN gctBOOL   IsRowMajor,
    IN gctSIZE_T MatrixStride,
    IN gctSIZE_T ArrayStride,
    IN gctCONST_POINTER Values[],
    IN gctUINT32 ValuesCount,
    IN gceUNIFORMCVT Convert,
    IN gcSHADER_KIND Type
    )
{
    gceSTATUS status;
    gctUINT32 columns, rows, arrays, matrixStride, arrayStride;
    gctUINT32 gpuCount = 1;
    gcmHEADER_ARG("Hal=0x%x, Address=%u Physical=%d Columns=%u Rows=%u Arrays=%u "
                  "IsRowMajor=%d MatrixStride=%u ArrayStride=%u Values=%p Convert=%d Type=%d",
                  Hal, Address, Physical, Columns, Rows, Arrays, IsRowMajor,
                  MatrixStride, ArrayStride, Values, Convert, Type);

    gcmSAFECASTSIZET(columns, Columns);
    gcmSAFECASTSIZET(rows, Rows);
    gcmSAFECASTSIZET(arrays, Arrays);
    gcmSAFECASTSIZET(matrixStride, MatrixStride);
    gcmSAFECASTSIZET(arrayStride, ArrayStride);

    gcoHARDWARE_Query3DCoreCount(gcvNULL, &gpuCount);
    gcmASSERT(ValuesCount == gpuCount);   /*Combined mode need send all gpus unform data */
    status = gcoHARDWARE_BindUniformEx(gcvNULL, Address, Physical, columns, rows, arrays, IsRowMajor,
                                       matrixStride, arrayStride, Values, Convert, Type, gcvTRUE);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSHADER_BindBufferBlock(
    IN gcoHAL Hal,
    IN gctUINT32 Address,
    IN gctUINT32 Base,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Size,
    IN gcSHADER_KIND Type
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hal=0x%x Address=%u Base=%u Offset=%lu Size=%lu Type=%d",
        Hal, Address, Base, Offset, Size, Type);

    status = gcoHARDWARE_BindBufferBlock(gcvNULL, Address, Base, Offset, Size, Type);

    gcmFOOTER();
    return status;
}

void
gcoSHADER_AllocateVidMem(
    gctPOINTER context,
    gceSURF_TYPE type,
    gctSTRING tag,
    gctSIZE_T size,
    gctUINT32 align,
    gctPOINTER *opaqueNode,
    gctPOINTER *memory,
    gctUINT32 *physical,
    gctPOINTER initialData,
    gctBOOL zeroMemory
    )
{
    gceSTATUS           status = gcvSTATUS_OK;
    gcsSURF_NODE_PTR    node = gcvNULL;
    gctPOINTER          pointer;
    gctPOINTER          logical = gcvNULL;

    gcmHEADER_ARG("context=%p type=%d ta%s size=%u align=%u opaqueNode=%p"
                  "memory=%p physical=%p initialData=%p zeroMemory=%d",
                  context, type, tag, size, align, opaqueNode,
                  memory, physical, initialData, zeroMemory);

    gcmASSERT(physical);
    gcmASSERT(opaqueNode);
    if (size)
    {
        /* Allocate node. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(gcsSURF_NODE),
                                  &pointer));

        node = pointer;

        gcmONERROR(gcsSURF_NODE_Construct(
            node,
            size,
            align,
            type,
            gcvALLOC_FLAG_NONE,
            gcvPOOL_DEFAULT
            ));

        /* Lock the inst buffer. */
        gcmONERROR(gcoSURF_LockNode(node,
                                    physical,
                                    &logical));
        gcmDUMP(gcvNULL, "#[info: video memory allocate for VSC %s]", tag);

        if (initialData)
        {
#if gcdENDIAN_BIG
            gctSIZE_T i;
            gctUINT_PTR pDst = (gctUINT_PTR)logical;
            gctUINT_PTR pSrc = (gctUINT_PTR)initialData;

            gcmASSERT(size % 4 == 0);

            for (i = 0; i < size / 4; ++i)
            {
                gctUINT src = *pSrc++;
                *pDst++ = gcmBSWAP32(src);
            }
#else
            gcoOS_MemCopy(logical, initialData, size);
#endif
        }
        else if (zeroMemory)
        {
            gcoOS_ZeroMemory(logical, size);
        }

        gcmDUMP_BUFFER(gcvNULL, gcvDUMP_BUFFER_MEMORY, *physical, logical, 0, size);

        if (node->pool == gcvPOOL_VIRTUAL)
        {
            gcmONERROR(gcoOS_CacheFlush(gcvNULL, node->u.normal.node, logical, size));
        }
    }

    *opaqueNode = (gctPOINTER)node;

    if (memory)
    {
        *memory = logical;
    }

OnError:
    if (gcmIS_ERROR(status) && node != gcvNULL)
    {
        gcoOS_Free(gcvNULL, node);
    }
    gcmFOOTER();
    return;
}

void
gcoSHADER_FreeVidMem(
    gctPOINTER context,
    gceSURF_TYPE type,
    gctSTRING tag,
    gctPOINTER opaqueNode
    )
{
    gceSTATUS           status = gcvSTATUS_OK;
    gcsSURF_NODE_PTR    node = (gcsSURF_NODE_PTR)opaqueNode;

    gcmHEADER_ARG("context=%p type=%d tag=%s opaqueNode=%p", context, type, tag, opaqueNode);

    if (node && node->pool != gcvPOOL_UNKNOWN)
    {
        /* Borrow as index buffer. */
        gcmONERROR(gcoHARDWARE_Unlock(node,
                                      type));

        /* Create an event to free the video memory. */
        gcmONERROR(gcsSURF_NODE_Destroy(node));

        /* Free node. */
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, node));
    }

OnError:
    gcmFOOTER();
    return;
}

#endif /*gcdENABLE_3D*/

