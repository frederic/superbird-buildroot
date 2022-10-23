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


#ifndef __gc_hal_user_shader_h_
#define __gc_hal_user_shader_h_

#if gcdENABLE_3D

#include "gc_hal_priv.h"

#ifdef __cplusplus
extern "C" {
#endif


gceSTATUS gcQueryShaderCompilerHwCfg(
    IN  gcoHAL Hal,
    OUT PVSC_HW_CONFIG pVscHwCfg);

/*******************************************************************************
**                                gcLoadShaders
********************************************************************************
**
**    Load a pre-compiled and pre-linked shader program into the hardware.
**
**    INPUT:
**
**        gcoHAL Hal
**            Pointer to a gcoHAL object.
**
**        gcsPROGRAM_STATE *ProgramState
**            Program state.
*/
gceSTATUS
gcLoadShaders(
    IN gcoHAL Hal,
    IN gcsPROGRAM_STATE *ProgramState
    );

gceSTATUS
gcInvokeThreadWalker(
    IN gcoHARDWARE Hardware,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    );

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
    );

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
    );

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
    );

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
    );

gceSTATUS
gcoSHADER_BindBufferBlock(
    IN gcoHAL Hal,
    IN gctUINT32 Address,
    IN gctUINT32 Base,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Size,
    IN gcSHADER_KIND Type
    );

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
    );

void
gcoSHADER_FreeVidMem(
    gctPOINTER context,
    gceSURF_TYPE type,
    gctSTRING tag,
    gctPOINTER opaqueNode
    );


#ifdef __cplusplus
}
#endif

#endif /* gcdENABLE_3D */
#endif /* __gc_hal_user_shader_h_ */
