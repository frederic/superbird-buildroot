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

/******************************************************************************\
****************************** gcoHARDWARE API Code *****************************
\******************************************************************************/

static gceSTATUS  _GetClearDestinationFormat(gcoHARDWARE hardware, gcsSURF_FORMAT_INFO *fmtInfo, gctUINT32 *HWvalue, gcsPOINT *rectSize)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmASSERT(fmtInfo);
    gcmASSERT(rectSize);
    gcmASSERT(HWvalue);

    switch (fmtInfo->bitsPerPixel/fmtInfo->layers)
    {
        case 8:
            if (hardware->features[gcvFEATURE_RS_DS_DOWNSAMPLE_NATIVE_SUPPORT])
            {
                *HWvalue = 0x10;
            }
            else
            {
                status = gcvSTATUS_NOT_SUPPORTED;
            }
            break;

        case 16:
            *HWvalue = 0x01;
            break;
        case 32:
            *HWvalue = 0x06;
            break;
        case 64:
            gcmASSERT(hardware->features[gcvFEATURE_64BPP_HW_CLEAR_SUPPORT]);
            *HWvalue = 0x15;
            break;

        default:
            status = gcvSTATUS_NOT_SUPPORTED;
            break;

    }

    return status;
}

static gceSTATUS
_ClearTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 Address,
    IN gctSIZE_T Bytes,
    IN gceSURF_TYPE Type,
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask,
    IN gctBOOL  ClearAsDirty
    )
{
    gceSTATUS status;
    gctSIZE_T bytes;
    gcsPOINT rectSize        = {0, 0};
    gctUINT32 config, control;
    gctUINT32 dither[2]      = { ~0U, ~0U };
    gctUINT32 fillColor      = 0;
    gcsSAMPLES savedSampleInfo = {0, 0, 0};
    gctBOOL savedMsaa        = gcvFALSE;
    gctUINT32 srcStride      = 0;
    gctBOOL multiPipe;
    gctPOINTER *cmdBuffer = gcvNULL;
    gcoSURF Surface = SurfView->surf;

    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x Surface=0x%x Address=0x%x "
                  "Bytes=%u Type=%d ClearValue=0x%x ClearMask=0x%x",
                  Hardware, Surface, Address,
                  Bytes, Type, ClearValue, ClearMask);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurfView->numSlices == 1);

    if (ClearMask != 0xF)
    {
        gctBOOL bailOut = gcvTRUE;

        /* Allow ClearMask of 0x7, when Alpha is not needed. */
        if (((ClearMask == 0x7)
            && ((Surface->format == gcvSURF_X8R8G8B8)
             || (Surface->format == gcvSURF_X8B8G8R8)
             || (Surface->format == gcvSURF_R5G6B5)
             || (Surface->format == gcvSURF_B5G6R5))))
        {
            bailOut = gcvFALSE;
        }
        else if (((ClearMask == 0xE)
            && (Surface->hasStencilComponent)
            && (Surface->canDropStencilPlane)))
        {
           bailOut = gcvFALSE;
        }
        else if (Surface->format == gcvSURF_S8)
        {
            bailOut = gcvFALSE;
        }

        if (bailOut)
        {
            status = (gcvSTATUS_NOT_SUPPORTED);
            goto OnError;
        }
    }

    /* Query the tile status size. */
    gcmONERROR(
        gcoHARDWARE_QueryTileStatus(Hardware,
                                    Surface,
                                    Surface->size,
                                    &bytes,
                                    gcvNULL,
                                    &fillColor));

    if (ClearAsDirty)
    {
        /* Clear tile status as Request64. */
        fillColor = 0x0;
    }

    if (Bytes > 0)
    {
        bytes = Bytes;
    }

    multiPipe = (Surface->tiling & gcvTILING_SPLIT_BUFFER) || Hardware->multiPipeResolve;
    /* Compute the hw specific clear window. */
    gcmONERROR(
        gcoHARDWARE_ComputeClearWindow(Hardware,
                                       bytes,
                                       (gctUINT32_PTR)&rectSize.x,
                                       (gctUINT32_PTR)&rectSize.y));

    /* Disable MSAA setting for MSAA tile status buffer clear */
    savedSampleInfo = Surface->sampleInfo;
    savedMsaa       = Surface->isMsaa;

    Surface->sampleInfo = g_sampleInfos[1];
    Surface->isMsaa     = gcvFALSE;
    gcmONERROR(gcoHARDWARE_AdjustCacheMode(Hardware, Surface));
    /* restore sample information */
    Surface->sampleInfo = savedSampleInfo;
    Surface->isMsaa = savedMsaa;

    switch(Type)
    {
    case gcvSURF_HIERARCHICAL_DEPTH:
        Surface->fcValueHz = ClearValue;
        break;

    default:
        Surface->fcValue[SurfView->firstSlice] = ClearValue;
        Surface->fcValueUpper[SurfView->firstSlice] = ClearValueUpper;
        break;
    }

    /* Build AQRsConfig register. */
    config = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:14) - (0 ?
 14:14) + 1))))))) << (0 ?
 14:14))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)));

    /* Build AQRsClearControl register. */
    control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:16) - (0 ?
 17:16) + 1))))))) << (0 ?
 17:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)));

    /* Cache mode programming. */
    if (Hardware->features[gcvFEATURE_FAST_MSAA] ||
        Hardware->features[gcvFEATURE_SMALL_MSAA] ||
        Hardware->features[gcvFEATURE_128BTILE])
    {
        /* Disable 256B cache always for tile status buffer clear */
        srcStride = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:29) - (0 ?
 29:29) + 1))))))) << (0 ?
 29:29))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29)));
    }

    /* Switch to 3D pipe. */
    gcmONERROR(
        gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, gcvNULL));

    /* Flush cache. */
    gcmONERROR(
        gcoHARDWARE_FlushPipe(Hardware, gcvNULL));

    /* Flush tile status cache. */
    gcmONERROR(
        gcoHARDWARE_FlushTileStatus(Hardware, SurfView, gcvFALSE));

    if ((Hardware->config->chipModel != gcv4000)
        || (Hardware->config->chipRevision != 0x5222))
    {
        gcmONERROR(
            gcoHARDWARE_Semaphore(Hardware,
                                  gcvWHERE_RASTER,
                                  gcvWHERE_PIXEL,
                                  gcvHOW_SEMAPHORE_STALL,
                                  gcvNULL));
    }

    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, cmdBuffer);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0581) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0581, config);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2 <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x058C) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


        gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x058C, dither[0]);

        gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x058C+1, dither[1]);

        gcmSETFILLER_NEW(reserve, memory);

    gcmENDSTATEBATCH_NEW(reserve, memory);

#if gcdENABLE_TRUST_APPLICATION
    if (Hardware->features[gcvFEATURE_SECURITY])
    {
        gcoHARDWARE_SetProtectMode(
            Hardware,
            (Surface->hints & gcvSURF_PROTECTED_CONTENT),
            (gctPOINTER *)&memory);

        Hardware->GPUProtecedModeDirty = gcvTRUE;
    }
#endif

    if (Surface->tiling & gcvTILING_SPLIT_BUFFER)
    {
        gctUINT32 bottomHalf;

        {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2 <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


            gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory,
                            0x05B8,
                            Address);

            gcmSAFECASTSIZET(bottomHalf, (Address + (bytes / 2)));

            gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory,
                            0x05B8+1,
                            bottomHalf);

            gcmSETFILLER_NEW(reserve, memory);

        gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0584) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0584, Address);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        if (Hardware->features[gcvFEATURE_RS_NEW_BASEADDR])
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x05B8, Address);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }

    }
    /* Cache mode programming. */
    if (Hardware->features[gcvFEATURE_FAST_MSAA] ||
        Hardware->features[gcvFEATURE_SMALL_MSAA] ||
        Hardware->features[gcvFEATURE_128BTILE])
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0583) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0583, srcStride);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

    }

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0585) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0585, rectSize.x * 4);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0590) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0590, fillColor);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x058F) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x058F, control);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05A8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x05A8, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (!multiPipe) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    gcmDUMP(gcvNULL, "#[surface 0x%x 0x%x]", Address, Bytes);

    gcmONERROR(
        gcoHARDWARE_ProgramResolve(Hardware,
                                   rectSize,
                                   multiPipe,
                                   gcvMSAA_DOWNSAMPLE_AVERAGE,
                                   (gctPOINTER *)&memory));

#ifndef __clang__
    stateDelta = stateDelta;
#endif

    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, cmdBuffer);

OnError:
    /* restore sample information */
    if (savedSampleInfo.product != 0)
    {
        Surface->sampleInfo = savedSampleInfo;
        Surface->isMsaa = savedMsaa;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoHARDWARE_ClearTileStatus
**
**  Append a command buffer with a CLEAR TILE STATUS command.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcoSURF Surface
**          Pointer of the surface to clear.
**
**      gctUINT32 Address
**          Base address of tile status to clear.
**
**      gceSURF_TYPE Type
**          Type of surface to clear.
**
**      gctUINT32 ClearValue
**          Value to be used for clearing the surface.
**
**      gctUINT32 ClearValueUpper
**          Upper Value to be used for clearing the surface.
**          For depth/hz, it's same as ClearValue always
**
**
**      gctUINT8 ClearMask
**          Byte-mask to be used for clearing the surface.
**
**      gctBOOL ClearAsDirty
**          Clear tile status as dirty (Reqest64) state.
**          Otherwise cleared as FastClear state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ClearTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 Address,
    IN gctSIZE_T Bytes,
    IN gceSURF_TYPE Type,
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x SurfView=0x%x Address=0x%x "
                  "Bytes=%u Type=%d ClearValue=0x%x ClearMask=0x%x",
                  Hardware, SurfView, Address,
                  Bytes, Type, ClearValue, ClearMask);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Do the tile status clear. */
    status = _ClearTileStatus(Hardware,
                              SurfView,
                              Address,
                              Bytes,
                              Type,
                              ClearValue,
                              ClearValueUpper,
                              ClearMask,
                              gcvFALSE);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ComputeClearWindow
**
**  Compute the Width Height for clearing,
**  when only the bytes to clear are known.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**
**      gctSIZE_T Bytes
**          Number of Bytes to clear.
**
**  OUTPUT:
**
**      gctUINT32 Width
**          Width of clear window.
**
**      gctUINT32 Height
**          Height of clear window.
**
*/
gceSTATUS
gcoHARDWARE_ComputeClearWindow(
    IN gcoHARDWARE Hardware,
    IN gctSIZE_T Bytes,
    OUT gctUINT32 *Width,
    OUT gctUINT32 *Height
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 width, height, alignY;

    gcmHEADER_ARG("Bytes=%u Width=%d Height=%d",
                  Bytes, gcmOPT_VALUE(Width), gcmOPT_VALUE(Height));

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Compute alignment. */
    alignY = (Hardware->resolveAlignmentY * 2) - 1;

    /* Compute rectangular width and height. */
    width  = Hardware->resolveAlignmentX;
    gcmSAFECASTSIZET(height, Bytes / (width * 4));

    /* Too small? */
    if (height < (Hardware->resolveAlignmentY))
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    while ((width < 8192) && ((height & alignY) == 0))
    {
        width  *= 2;
        height /= 2;
    }

    gcmASSERT((gctSIZE_T) (width * height * 4) == Bytes);

    if ((gctSIZE_T) (width * height * 4) == Bytes)
    {
        *Width  = width;
        *Height = height;
    }
    else
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoHARDWARE_ClearTileStatusWindowAligned
**
**  This is core partial fast clear logic.
**  Try to clear aligned rectangle with fast clear, and clear other areas
**  outside the aligned rectangle using 3D draw in caller.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcoSURF Surface
**          Pointer of the surface to clear.
**
**      gceSURF_TYPE Type
**          Type of surface to clear.
**
**      gctUINT32 ClearValue
**          Value to be used for clearing the surface.
**
**      gctUINT32 ClearValueUpper
**          Upper Value to be used for clearing the surface.
**          For depth/hz, it's same as ClearValue always
**
**      gctUINT8 ClearMask
**          Byte-mask to be used for clearing the surface.
**
**      gcsRECT_PTR Rect
**          The original rectangle to be cleared.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ClearTileStatusWindowAligned(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gceSURF_TYPE Type,
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask,
    IN gcsRECT_PTR Rect,
    OUT gcsRECT_PTR AlignedRect
    )
{
    gceSTATUS status;

    /* Source source parameters. */
    gcsRECT rect;
    gctUINT bytesPerPixel;
    gcsPOINT size;

    /* Source surface alignment limitation. */
    gctUINT alignX, alignY;
    gctINT blockWidth, blockHeight;

    /* Aligned source surface parameters. */
    gcsPOINT clearOrigin;
    gcsPOINT clearSize;

    /* Tile status surface parameters. */
    gctSIZE_T bytes;
    gctUINT32 fillColor;
    gctUINT   stride;
    gctBOOL   multiPipe;

    gctUINT32 tileStatusAddress;

    gctPOINTER *cmdBuffer = gcvNULL;

    gcoSURF Surface = SurfView->surf;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmGETHARDWARE(Hardware);

    /* No multiSlice support when tile status enabled for now.*/
    gcmASSERT(SurfView->numSlices == 1);

    if (ClearMask != 0xF)
    {
        gctBOOL bailOut = gcvTRUE;

        /* Allow ClearMask of 0x7, when Alpha is not needed. */
        if (((ClearMask == 0x7)
            && ((Surface->format == gcvSURF_X8R8G8B8)
             || (Surface->format == gcvSURF_X8B8G8R8)
             || (Surface->format == gcvSURF_R5G6B5)
             || (Surface->format == gcvSURF_B5G6R5))))
        {
            bailOut = gcvFALSE;
        }
        else if (((ClearMask == 0xE)
            && (Surface->hasStencilComponent)
            && (Surface->canDropStencilPlane)))
        {
           bailOut = gcvFALSE;
        }
        else if (Surface->format == gcvSURF_S8)
        {
            gcmPRINT("TODO: partial fast clear for S8");
        }

        if (bailOut)
        {
            status = (gcvSTATUS_NOT_SUPPORTED);
            goto OnError;
        }
    }

    if (!Surface->superTiled)
    {
        status = gcvSTATUS_NOT_SUPPORTED;
        goto OnError;
    }

    multiPipe = (Surface->tiling & gcvTILING_SPLIT_BUFFER) || Hardware->multiPipeResolve;
    /* Get bytes per pixel. */
    bytesPerPixel = Surface->formatInfo.bitsPerPixel / 8;

    /*
     * Query the tile status size for a 64x64 supertile.
     * Because of resolve limitation, hw may not be available to clear ts of one
     * 64x64 supertile, ts size alignment is introduced in query. Here we
     * simply query ts size of 8 supertiles then div by 8 to get ts size of
     * a single supertile.
     */
    gcmONERROR(gcoHARDWARE_QueryTileStatus(Hardware, Surface,
                                           64 * 64 * 8 * bytesPerPixel,
                                           &bytes, gcvNULL, &fillColor));

    bytes >>= 3;

    if (Surface->tiling & gcvTILING_SPLIT_BUFFER)
    {
        /*
         * Extra pages needed to offset sub-buffers to different banks.
         * Calculate corresponding ts bytes(32B multiplier) of the offset.
         *  64 x 64 x bytesPerPixel : bytes / 32B
         *  bottomBufferOffset      : ?
         */
        if ((Surface->bottomBufferOffset * bytes / 32) % (64 * 64 * bytesPerPixel))
        {
            /* Corresponding second address in ts is not 32B aligned. */
            status = gcvSTATUS_NOT_SUPPORTED;
            goto OnError;
        }
    }

    /* Shadow clear rectangle. */
    rect = *Rect;

    /* Use aligned size if out of valid rectangle. */
    if (rect.right >= (gctINT)Surface->requestW)
    {
        rect.right = (gctINT)Surface->alignedW;
    }

    if (rect.bottom >= (gctINT)Surface->requestH)
    {
        rect.bottom = (gctINT)Surface->alignedH;
    }

    /*
     * Determine minimal resolve size alignment.
     * resolve rectangle must be 16x4 aligned. 16x4 size is 16x4x2B.
     */
    blockWidth  = 64 * (16*4*2 / (gctINT)bytes);
    blockHeight = (Surface->tiling & gcvTILING_SPLIT_BUFFER) ? 128 : 64;

    /*
     * Determine origin alignment.
     * Origin must be 32B aligned for RS dest.
     * 32B in ts = contiguous 128 tile in surface.
     *           = 1x1 supertile for 16bpp (64x64 in coord)
     *           = 0.5x1 supertile for 32bpp (32x64 in coord)
     *          (=>1x1 supertile for 32 bpp (64x64 in coord))
     */
    alignX = 64;
    alignY = (Surface->tiling & gcvTILING_SPLIT_BUFFER) ? 128 : 64;

    /* Calculate aligned origin. */
    clearOrigin.x = gcmALIGN(rect.left, alignX);
    clearOrigin.y = gcmALIGN(rect.top, alignY);

    /* Calculate aligned size. */
    clearSize.x = gcmALIGN_BASE(rect.right  - clearOrigin.x, alignX);
    clearSize.y = gcmALIGN_BASE(rect.bottom - clearOrigin.y, alignY);

    if (clearSize.x <= blockWidth || clearSize.y <= blockHeight)
    {
        /* Clear size too small. */
        status = gcvSTATUS_NOT_SUPPORTED;
        goto OnError;
    }

    /* Output aligned rectangle. */
    AlignedRect->left   = clearOrigin.x;
    AlignedRect->top    = clearOrigin.y;
    AlignedRect->right  = clearOrigin.x + clearSize.x;
    AlignedRect->bottom = clearOrigin.y + clearSize.y;

    if ((AlignedRect->left   != Rect->left) ||
        (AlignedRect->top    != Rect->top) ||
        (AlignedRect->right  != Rect->right) ||
        (AlignedRect->bottom != Rect->bottom))
    {
        /* check renderable. */
        status = gcoHARDWARE_QuerySurfaceRenderable(Hardware, Surface);

        if (gcmIS_ERROR(status))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            goto OnError;
        }
    }

    /* Switch to 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, gcvNULL));

    /* Flush cache. */
    gcmONERROR(gcoHARDWARE_FlushPipe(Hardware, gcvNULL));

    if (Surface->tileStatusDisabled[SurfView->firstSlice])
    {
        gcmGETHARDWAREADDRESS(Surface->tileStatusNode, tileStatusAddress);
        tileStatusAddress += SurfView->firstSlice * Surface->tileStatusSliceSize;

        /* Clear tile status as dirty, prepare to enable. */
        gcmVERIFY_OK(
            _ClearTileStatus(Hardware,
                             SurfView,
                             tileStatusAddress,
                             0,
                             Type,
                             ClearValue,
                             ClearValueUpper,
                             ClearMask,
                             gcvTRUE));
    }

    /* Calculate tile status surface stride. */
    stride = Surface->alignedW / 64 * (gctUINT)bytes;

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, cmdBuffer);

#if gcdENABLE_TRUST_APPLICATION
    if (Hardware->features[gcvFEATURE_SECURITY])
    {
        gcoHARDWARE_SetProtectMode(
            Hardware,
            (Surface->hints & gcvSURF_PROTECTED_CONTENT),
            (gctPOINTER *)&memory);

        Hardware->GPUProtecedModeDirty = gcvTRUE;
    }
#endif

    /* Program registers. */
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0581) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0581, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:14) - (0 ?
 14:14) + 1))))))) << (0 ?
 14:14))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2  <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x058C) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


        gcmSETCTRLSTATE_NEW(
            stateDelta, reserve, memory, 0x058C,
            ~0U
            );

        gcmSETCTRLSTATE_NEW(
            stateDelta, reserve, memory, 0x058C + 1,
            ~0U
            );

        gcmSETFILLER_NEW(
            reserve, memory
            );

    gcmENDSTATEBATCH_NEW(
        reserve, memory
        );

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0585) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0585, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:0) - (0 ?
 19:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:0) - (0 ?
 19:0) + 1))))))) << (0 ?
 19:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ?
 19:0) - (0 ?
 19:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:0) - (0 ?
 19:0) + 1))))))) << (0 ?
 19:0))) | ((((gctUINT32) (stride)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:30) - (0 ?
 30:30) + 1))))))) << (0 ?
 30:30))) | (((gctUINT32) ((gctUINT32) (((Surface->tiling & gcvTILING_SPLIT_BUFFER) != 0)) & ((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0590) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0590, fillColor );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x058F) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x058F, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:16) - (0 ?
 17:16) + 1))))))) << (0 ?
 17:16))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    /* Append new configuration register. */
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05A8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x05A8, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    /*
     * clear left area.
     *
     * +....----------------+----+
     * |x   x   x   x   x   |    |
     * |  x   x   x   x   x |    |
     * |x   x   x   x   x   |    |
     * |  x   x   x   x   x |    |
     * +-....---------------+----+
     * |<-  16x4 aligned  ->|
     */
    {
        gctUINT32 physical[2];
        gcsPOINT rectSize;
        gctUINT32 tileStatusAddress;

        size.x = gcmALIGN_BASE(clearSize.x, blockWidth);
        size.y = clearSize.y;

        gcmGETHARDWAREADDRESS(Surface->tileStatusNode, tileStatusAddress);

        physical[0] = tileStatusAddress
                    + (Surface->alignedW / 64) * (clearOrigin.y / blockHeight) * (gctUINT32)bytes
                    + (clearOrigin.x / 64) * (gctUINT32)bytes;

        physical[1] = physical[0]
                    + (gctUINT32)bytes * Surface->bottomBufferOffset / 64 / 64 / bytesPerPixel;

        if (Surface->tiling & gcvTILING_SPLIT_BUFFER)
        {
            {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2  <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


                gcmSETCTRLSTATE_NEW(
                    stateDelta, reserve, memory, 0x05B8,
                    physical[0]
                    );

                gcmSETCTRLSTATE_NEW(
                    stateDelta, reserve, memory, 0x05B8 + 1,
                    physical[1]
                    );

                gcmSETFILLER_NEW(
                    reserve, memory
                    );

            gcmENDSTATEBATCH_NEW(
                reserve, memory
                );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0584) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0584, physical[0] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (Hardware->features[gcvFEATURE_RS_NEW_BASEADDR])
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x05B8, physical[0] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
        }

        rectSize.x = 16 * (size.x / blockWidth);
        rectSize.y =  4 * (size.y / 64);

        /* Trigger clear. */
        gcmONERROR(gcoHARDWARE_ProgramResolve(Hardware, rectSize, multiPipe, gcvMSAA_DOWNSAMPLE_AVERAGE, (gctPOINTER *)&memory));
    }

    /*
     * clear right area.
     *
     * +....----------------+----+
     * |        x   x   x   |x   |
     * |      x   x   x   x |  x |
     * |        x   x   x   |x   |
     * |      x   x   x   x |  x |
     * +-....---------------+----+
     *       |<-     16x4      ->|
     */
    if (size.x != clearSize.x)
    {
        gctUINT32 physical[2];
        gcsPOINT rectSize;
        gctUINT32 tileStatusAddress;

        gcmGETHARDWAREADDRESS(Surface->tileStatusNode, tileStatusAddress);

        clearOrigin.x = clearOrigin.x + clearSize.x - blockWidth;

        physical[0] = tileStatusAddress
                    + (Surface->alignedW / 64) * (clearOrigin.y / blockHeight) * (gctUINT32) bytes
                    + (clearOrigin.x / 64) * (gctUINT32) bytes;

        physical[1] = physical[0]
                    + (gctUINT32) bytes * Surface->bottomBufferOffset / 64 / 64 / bytesPerPixel;

        if (Surface->tiling & gcvTILING_SPLIT_BUFFER)
        {
            {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2  <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


                gcmSETCTRLSTATE_NEW(
                    stateDelta, reserve, memory, 0x05B8,
                    physical[0]
                    );

                gcmSETCTRLSTATE_NEW(
                    stateDelta, reserve, memory, 0x05B8 + 1,
                    physical[1]
                    );

                gcmSETFILLER_NEW(
                    reserve, memory
                    );

            gcmENDSTATEBATCH_NEW(
                reserve, memory
                );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0584) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0584, physical[0] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (Hardware->features[gcvFEATURE_RS_NEW_BASEADDR])
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x05B8, physical[0] );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
        }


        rectSize.x = 16;
        rectSize.y =  4 * (size.y / 64);

        /* Trigger clear. */
        gcmONERROR(gcoHARDWARE_ProgramResolve(Hardware, rectSize, multiPipe, gcvMSAA_DOWNSAMPLE_AVERAGE, (gctPOINTER *)&memory));
    }

    gcmDUMP(gcvNULL, "#[surface 0x%x 0x%x]", gcsSURF_NODE_GetHWAddress(&Surface->tileStatusNode), Surface->tileStatusNode.size);

#ifndef __clang__
    stateDelta = stateDelta;
#endif

    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, cmdBuffer);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ClearHardware(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 LayerIndex,
    IN gcsRECT_PTR Rect,
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask
    )
{
    gceSTATUS status;
    gctUINT alignX, alignY;
    gctUINT alignWidth, alignHeight;
    gctUINT32 stride;
    gcsPOINT rectSize;
    gctUINT32 dstFormat;
    gctUINT32 srcStride = 0;
    gctPOINTER  *cmdBuffer = gcvNULL;

    gcoSURF surf = SurfView->surf;
    gcsRECT rect = *Rect;

    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x SurfView=0x%08x Rect=0x%08x ClearValue=0x%08x "
                  "ClearValueUpper=0x%08x ClearMask=0x%02x",
                  Hardware, SurfView, Rect, ClearValue, ClearValueUpper,ClearMask);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /*
    ** Bail out for 64bpp surface if RS can't support it.
    */
    if (!Hardware->features[gcvFEATURE_64BPP_HW_CLEAR_SUPPORT] &&
        ((surf->bitsPerPixel / surf->formatInfo.layers) > 32))
    {
        status = gcvSTATUS_NOT_SUPPORTED;
        goto OnError;
    }

    /* Query the resolve alignemnt requirement to resolve this surface */
    gcmONERROR(
        gcoHARDWARE_GetSurfaceResolveAlignment(Hardware,
                                               surf,
                                               &alignX,
                                               &alignY,
                                               &alignWidth,
                                               &alignHeight));

    /* Increase clear region to aligned region. */
    if (rect.right == (gctINT)surf->allocedW)
    {
        rect.right = surf->alignedW;
    }

    if (rect.bottom == (gctINT)surf->allocedH)
    {
        rect.bottom = surf->alignedH;
    }

    rectSize.x = rect.right  - rect.left;
    rectSize.y = rect.bottom - rect.top;

    /* For resolve clear, we need to be 4x1 tile aligned. */
    if (((rect.left  & (alignX - 1)) == 0)
    &&  ((rect.top   & (alignY - 1)) == 0)
    &&  ((rectSize.x & (alignWidth  - 1)) == 0)
    &&  ((rectSize.y & (alignHeight - 1)) == 0)
    )
    {
        gctINT dstTileEnable = 0;
        gctINT dstTileMode   = 0;
        gctINT dstStride = 0;
        gctUINT32 config, control;
        gctUINT32 dither[2] = { ~0U, ~0U };
        gctUINT32 offset, address;
        gctBOOL halti2Support = Hardware->features[gcvFEATURE_HALTI2];
        gctBOOL multiPipe = (surf->tiling & gcvTILING_SPLIT_BUFFER) || Hardware->multiPipeResolve;

        gctPOINTER logical[gcdMAX_SURF_LAYERS] = {gcvNULL};

        if (Hardware->features[gcvFEATURE_128BTILE])
        {
            if (surf->isMsaa)
            {
                /* Currently we will not clear part of msaa surface with RS-clear,
                ** as it will disable FC, but we need msaa FC always on.
                */
                gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
            }
            else
            {
                srcStride = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:29) - (0 ?
 29:29) + 1))))))) << (0 ?
 29:29))) | (((gctUINT32) ((gctUINT32) (surf->cacheMode == gcvCACHE_256) & ((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29)));
            }
        }
        /* Determine Fast MSAA mode. */
        else if (Hardware->features[gcvFEATURE_FAST_MSAA] ||
            Hardware->features[gcvFEATURE_SMALL_MSAA])
        {
            if (surf->isMsaa)
            {
                /* Currently we will not clear part of msaa surface with RS-clear,
                ** as it will disable FC, but we need msaa FC always on.
                */
                gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
            }
            else
            {
                srcStride = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 29:29) - (0 ?
 29:29) + 1))))))) << (0 ?
 29:29))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 29:29) - (0 ?
 29:29) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29)));
            }
        }

        /*
        ** Adjust MSAA setting for RS clear. Here Surface must be non-msaa.
        */
        gcmONERROR(gcoHARDWARE_AdjustCacheMode(Hardware, surf));

        surf->pfGetAddr(surf, rect.left, rect.top, SurfView->firstSlice, logical);
        offset = (gctUINT32)((gctUINT8_PTR)logical[LayerIndex] - surf->node.logical);
        if (((rect.left >> 3) ^ (rect.top >> 2)) & 0x01)
        {
            /* Move to the top buffer */
            offset -= surf->bottomBufferOffset;
        }

        /* Determine the starting address. */
        gcmGETHARDWAREADDRESS(surf->node, address);

        gcmONERROR(_GetClearDestinationFormat(Hardware, &surf->formatInfo, &dstFormat, &rectSize));

        if (surf->tiling == gcvLINEAR)
        {
            /* Disable 'tileEnable' for linear surface. */
            dstTileEnable = 0x0;
            dstTileMode   = 0;
            dstStride     = surf->stride;
        }
        else
        {
            dstTileEnable = 0x1;

            if (Hardware->features[gcvFEATURE_128BTILE])
            {
                if (surf->tiling == gcvSUPERTILED)
                {
                    dstTileMode = 0x1;
                }
                else if (surf->tiling == gcvYMAJOR_SUPERTILED)
                {
                    dstTileMode = 0x2;
                }
                else
                {
                    dstTileMode = 0x0;
                }
            }

            /* Requires linear stride multiplied by 4 for tile mode. */
            dstStride = (surf->stride << 2);
        }

        /* Build AQRsConfig register. src format is useless for clear
        ** Only dword size should be correct for destination surface
        */
        config = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:0) - (0 ?
 4:0) + 1))))))) << (0 ?
 4:0))) | (((gctUINT32) ((gctUINT32) (dstFormat) & ((gctUINT32) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 12:8) - (0 ?
 12:8) + 1))))))) << (0 ?
 12:8))) | (((gctUINT32) ((gctUINT32) (dstFormat) & ((gctUINT32) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:14) - (0 ?
 14:14) + 1))))))) << (0 ?
 14:14))) | (((gctUINT32) ((gctUINT32) (dstTileEnable) & ((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)));

        if (halti2Support)
        {
            /* Build AQRsClearControl register. */
            control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) ((ClearMask | (ClearMask << 4))) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:16) - (0 ?
 17:16) + 1))))))) << (0 ?
 17:16))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)));
        }
        else
        {
            /* Build AQRsClearControl register. */
            control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (ClearMask) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:16) - (0 ?
 17:16) + 1))))))) << (0 ?
 17:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 17:16) - (0 ?
 17:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)));
        }

        /* Determine the stride. */
        stride = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:0) - (0 ?
 19:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:0) - (0 ?
 19:0) + 1))))))) << (0 ?
 19:0))) | (((gctUINT32) ((gctUINT32) (dstStride) & ((gctUINT32) ((((1 ?
 19:0) - (0 ?
 19:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:0) - (0 ? 19:0) + 1))))))) << (0 ? 19:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:31) - (0 ?
 31:31) + 1))))))) << (0 ?
 31:31))) | (((gctUINT32) ((gctUINT32) (((surf->tiling & gcvSUPERTILED) > 0)) & ((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 30:30) - (0 ?
 30:30) + 1))))))) << (0 ?
 30:30))) | (((gctUINT32) ((gctUINT32) (((surf->tiling & gcvTILING_SPLIT_BUFFER) > 0)) & ((gctUINT32) ((((1 ?
 30:30) - (0 ?
 30:30) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 28:27) - (0 ?
 28:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 28:27) - (0 ?
 28:27) + 1))))))) << (0 ?
 28:27))) | (((gctUINT32) ((gctUINT32) (dstTileMode) & ((gctUINT32) ((((1 ?
 28:27) - (0 ?
 28:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 28:27) - (0 ? 28:27) + 1))))))) << (0 ? 28:27)));

        gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, cmdBuffer);

        /* Switch to 3D pipe. */
        gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, (gctPOINTER *)&memory));

        /* Flush cache. */
        gcmONERROR(gcoHARDWARE_FlushPipe(Hardware, (gctPOINTER *)&memory));

#if gcdENABLE_TRUST_APPLICATION
        if (Hardware->features[gcvFEATURE_SECURITY])
        {
            gcoHARDWARE_SetProtectMode(
                Hardware,
                (surf->hints & gcvSURF_PROTECTED_CONTENT),
                (gctPOINTER *)&memory);

            Hardware->GPUProtecedModeDirty = gcvTRUE;
        }
#endif

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0581) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0581, config);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2 <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x058C) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


            gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x058C, dither[0]);

            gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x058C+1, dither[1]);

            gcmSETFILLER_NEW(reserve, memory);

        gcmENDSTATEBATCH_NEW(reserve, memory);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0585) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0585, stride);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        /* cache mode programming */
        if (Hardware->features[gcvFEATURE_FAST_MSAA] ||
            Hardware->features[gcvFEATURE_SMALL_MSAA] ||
            Hardware->features[gcvFEATURE_128BTILE])
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0583) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0583, srcStride);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }

        if (halti2Support)
        {
            {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2 <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0590) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


                gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0590, ClearValue);

                gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0590+1, ClearValueUpper);

                gcmSETFILLER_NEW(reserve, memory);

            gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0590) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0590, ClearValue);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x058F) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x058F, control);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05A8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x05A8, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:20) - (0 ?
 20:20) + 1))))))) << (0 ?
 20:20))) | (((gctUINT32) ((gctUINT32) (!multiPipe) & ((gctUINT32) ((((1 ?
 20:20) - (0 ?
 20:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};



        if (surf->tiling & gcvTILING_SPLIT_BUFFER)
        {
            {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)2 <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


                gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory,
                                0x05B8,
                                address + offset);

                gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory,
                                0x05B8+1,
                                address + offset + surf->bottomBufferOffset);

                gcmSETFILLER_NEW(reserve, memory);

            gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0584) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0584, address + offset);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (Hardware->features[gcvFEATURE_RS_NEW_BASEADDR])
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x05B8, address + offset);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }
        }

        gcmONERROR(gcoHARDWARE_ProgramResolve(Hardware,
                                              rectSize,
                                              multiPipe,
                                              gcvMSAA_DOWNSAMPLE_AVERAGE,
                                              (gctPOINTER *)&memory));

        gcmDUMP(gcvNULL, "#[surface 0x%x 0x%x]", address, surf->node.size);

#ifndef __clang__
        stateDelta = stateDelta;
#endif

        gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, cmdBuffer);

    }
    else
    {
        /* Removed 2D clear as it does not work for tiled buffers. */
        status = gcvSTATUS_NOT_ALIGNED;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ClearSoftware(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 LayerIndex,
    IN gcsRECT_PTR Rect,
    IN gctUINT32 channelMask[4],
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask,
    IN gceSURF_TYPE Type,
    IN gctUINT8 BitMask
    )
{
    gctINT32 x, y;
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF surf = SurfView->surf;
    gctUINT32 bpp = surf->formatInfo.bitsPerPixel;
    gctUINT32 tLeft = (bpp == 16) ? gcmALIGN(Rect->left - 1, 2) : Rect->left;

    for (y = Rect->top;  y < Rect->bottom; y++)
    {
        for (x = tLeft; x < Rect->right; x++)
        {
            gctPOINTER address[gcdMAX_SURF_LAYERS] = {gcvNULL};

            surf->pfGetAddr(surf, x, y, SurfView->firstSlice, address);

            /* Draw only if x,y within clear rectangle. */
            if (bpp >= 64)
            {
                if(surf->formatInfo.format == gcvSURF_G32R32 ||
                    surf->formatInfo.format == gcvSURF_G32R32F ||
                    surf->formatInfo.format == gcvSURF_G32R32I ||
                    surf->formatInfo.format == gcvSURF_G32R32UI ||
                    surf->formatInfo.format == gcvSURF_X32B32G32R32I_2_G32R32I ||
                    surf->formatInfo.format == gcvSURF_A32B32G32R32I_2_G32R32I ||
                    surf->formatInfo.format == gcvSURF_A32B32G32R32I_2_G32R32F ||
                    surf->formatInfo.format == gcvSURF_X32B32G32R32UI_2_G32R32UI ||
                    surf->formatInfo.format == gcvSURF_A32B32G32R32UI_2_G32R32UI ||
                    surf->formatInfo.format == gcvSURF_A32B32G32R32UI_2_G32R32F )
                {
                    gctUINT32_PTR addr32 = (gctUINT32_PTR)address[LayerIndex];
                    if ((ClearMask >> (LayerIndex << 1)) & 0x1)
                    {
                        /* Clear byte 0. */
                        addr32[0] = (gctUINT32) ClearValue;
                    }

                    if ((ClearMask >> (LayerIndex << 1)) & 0x2)
                    {
                        /* Clear byte 1. */
                        addr32[1] = (gctUINT32) ClearValueUpper;
                    }
                }
                else
                {
                    gctUINT16_PTR addr16 = (gctUINT16_PTR)address[LayerIndex];
                    if (ClearMask & 0x1)
                    {
                        /* Clear byte 0. */
                        addr16[0] = (gctUINT16) ClearValue;
                    }

                    if (ClearMask & 0x2)
                    {
                        /* Clear byte 1. */
                        addr16[1] = (gctUINT16) (ClearValue >> 16);
                    }

                    if (ClearMask & 0x4)
                    {
                        /* Clear byte 2. */
                        addr16[2] = (gctUINT16) (ClearValueUpper);
                    }

                    if (ClearMask & 0x8)
                    {
                        /* Clear byte 3. */
                        addr16[3] = (gctUINT16) (ClearValueUpper >> 16);
                    }
                }
            }
            else if (bpp == 32)
            {
                gctUINT8_PTR addr8 = (gctUINT8_PTR)address[LayerIndex];

                switch (ClearMask)
                {
                case 0x1:
                    /* Common: Clear stencil only. */
                    if (Type == gcvSURF_DEPTH)
                    {
                        (*addr8) = (*addr8) & (~BitMask);
                        (*addr8) = (*addr8) | (ClearValue & BitMask);
                    }
                    else
                        *addr8 = (gctUINT8)ClearValue;
                    break;

                case 0xE:
                    /* Common: Clear depth only. */
                                       addr8[1] = (gctUINT8) (ClearValue >> 8);
                    * (gctUINT16_PTR) &addr8[2] = (gctUINT16)(ClearValue >> 16);
                    break;

                case 0xF:
                    /* Common: Clear everything. */
                    if (Type == gcvSURF_DEPTH)
                    {
                        gctUINT32 mask = 0xffffff00 | BitMask;
                        (*(gctUINT32_PTR) addr8) = (*(gctUINT32_PTR) addr8) & (~mask);
                        (*(gctUINT32_PTR) addr8) = (*(gctUINT32_PTR) addr8) | (ClearValue & mask);
                    }
                    else
                        * (gctUINT32_PTR) addr8 = ClearValue;
                    break;

                default:
                    if (ClearMask & 0x1)
                    {
                        /* Clear channel 0. */
                        *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR)addr8 & ~channelMask[0])
                                               | (ClearValue &  channelMask[0]);
                    }

                    if (ClearMask & 0x2)
                    {
                        /* Clear channel 1. */
                        *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR) addr8 & ~channelMask[1])
                                               | (ClearValue &  channelMask[1]);
                    }

                    if (ClearMask & 0x4)
                    {
                        /* Clear channel 2. */
                        *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR) addr8 & ~channelMask[2])
                                               | (ClearValue &  channelMask[2]);
                    }

                    if (ClearMask & 0x8)
                    {
                        /* Clear channel 3. */
                        *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR) addr8 & ~channelMask[3])
                                               | (ClearValue &  channelMask[3]);
                    }
                }
            }
            else if (bpp == 16)
            {
                gctUINT8_PTR addr8 = (gctUINT8_PTR)address[LayerIndex];

                if ((x + 1) == Rect->right)
                {
                    /* Dont write on Right pixel. */
                    channelMask[0] = channelMask[0] & 0x0000FFFF;
                    channelMask[1] = channelMask[1] & 0x0000FFFF;
                    channelMask[2] = channelMask[2] & 0x0000FFFF;
                    channelMask[3] = channelMask[3] & 0x0000FFFF;
                }

                if ((x + 1) == Rect->left)
                {
                    /* Dont write on Left pixel. */
                    channelMask[0] = channelMask[0] & 0xFFFF0000;
                    channelMask[1] = channelMask[1] & 0xFFFF0000;
                    channelMask[2] = channelMask[2] & 0xFFFF0000;
                    channelMask[3] = channelMask[3] & 0xFFFF0000;
                }

                if (ClearMask & 0x1)
                {
                    /* Clear channel 0. */
                    *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR) addr8 & ~channelMask[0])
                                           | (ClearValue &  channelMask[0]);
                }

                if (ClearMask & 0x2)
                {
                    /* Clear channel 1. */
                    *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR) addr8 & ~channelMask[1])
                                           | (ClearValue &  channelMask[1]);
                }

                if (ClearMask & 0x4)
                {
                    /* Clear channel 2. */
                    *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR) addr8 & ~channelMask[2])
                                           | (ClearValue &  channelMask[2]);
                }

                if (ClearMask & 0x8)
                {
                    /* Clear channel 3. */
                    *(gctUINT32_PTR) addr8 = (*(gctUINT32_PTR) addr8 & ~channelMask[3])
                                           | (ClearValue &  channelMask[3]);
                }

                if ((x + 1) ==  Rect->left)
                {
                    /* Restore channel mask. */
                    channelMask[0] = channelMask[0] | (channelMask[0] >> 16);
                    channelMask[1] = channelMask[1] | (channelMask[1] >> 16);
                    channelMask[2] = channelMask[2] | (channelMask[2] >> 16);
                    channelMask[3] = channelMask[3] | (channelMask[3] >> 16);
                }

                if ((x + 1) ==  Rect->right)
                {
                    /* Restore channel mask. */
                    channelMask[0] = channelMask[0] | (channelMask[0] << 16);
                    channelMask[1] = channelMask[1] | (channelMask[1] << 16);
                    channelMask[2] = channelMask[2] | (channelMask[2] << 16);
                    channelMask[3] = channelMask[3] | (channelMask[3] << 16);
                }

                /* 16bpp pixels clear 1 extra pixel at a time. */
                x++;
            }
            else if (bpp == 8)
            {
                gctUINT8_PTR addr8 = (gctUINT8_PTR)address[LayerIndex];
                (*addr8) = (gctUINT8) (((*addr8) & (~BitMask)) | (ClearValue & BitMask));
            }
        }
    }

    return status;
}

gceSTATUS
gcoHARDWARE_Clear(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 LayerIndex,
    IN gcsRECT_PTR Rect,
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask
    )
{
    gceSTATUS status;

    gcoSURF surf = SurfView->surf;

    gcmHEADER_ARG("SurfView=0x%08X LayerIndex=%u Rect=0x%08x ClearValue=0x%08x "
                  "ClearValueUpper=0x%08x ClearMask=0x%02x",
                  SurfView, LayerIndex, Rect, ClearValue, ClearValueUpper, ClearMask);

    switch (surf->format)
    {
    case gcvSURF_X4R4G4B4:
    case gcvSURF_X1R5G5B5:
    case gcvSURF_R5G6B5:
    case gcvSURF_X4B4G4R4:
    case gcvSURF_X1B5G5R5:
    case gcvSURF_B5G6R5:
        if (ClearMask == 0x7)
        {
            /* When the format has no alpha channel, fake the ClearMask to
            ** include alpha channel clearing.   This will allow us to use
            ** resolve clear. */
            ClearMask = 0xF;
        }
        break;

    default:
        break;
    }

    if ((ClearMask != 0xF)
    &&  (surf->format != gcvSURF_X8R8G8B8)
    &&  (surf->format != gcvSURF_A8R8G8B8)
    &&  (surf->format != gcvSURF_X8B8G8R8)
    &&  (surf->format != gcvSURF_A8B8G8R8)
    &&  (surf->format != gcvSURF_D24S8)
    &&  (surf->format != gcvSURF_D24X8)
    &&  (surf->format != gcvSURF_D16)
    &&  (surf->format != gcvSURF_S8)
    &&  (surf->format != gcvSURF_X24S8)
    )
    {
        /* Don't clear with mask when channels are not byte sized. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Try hardware clear. */
    status = _ClearHardware(Hardware,
                            SurfView,
                            LayerIndex,
                            Rect,
                            ClearValue,
                            ClearValueUpper,
                            ClearMask);

    if (gcmIS_ERROR(status))
    {
        status = gcvSTATUS_NOT_ALIGNED;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_ClearSoftware(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 LayerIndex,
    IN gcsRECT_PTR Rect,
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask,
    IN gctUINT8 StencilWriteMask
    )
{
    gceSTATUS status;
    gctUINT32 bitsPerPixel   = 0;
    gctUINT32 channelMask[4] = {0};
    static gctBOOL printed   = gcvFALSE;
    gceSURF_TYPE type        = gcvSURF_TYPE_UNKNOWN;

    gcoSURF surf = SurfView->surf;

    gcmHEADER_ARG("Hardware=0x%x SurfView=0x%08X LayerIndex=%u Rect=0x%08x "
                  "ClearValue=0x%08x ClearValueUpper=0x%08x ClearMask=0x%02x "
                  "StencilWriteMask=0x%02x",
                  Hardware, SurfView, LayerIndex, Rect, ClearValue, ClearValueUpper,
                  ClearMask, StencilWriteMask);

    /* For a clear that is not tile aligned, our hardware might not be able to
       do it.  So here is the software implementation. */
    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (!printed)
    {
        printed = gcvTRUE;

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
                      "%s: Performing a software clear!",
                      __FUNCTION__);
    }

    /* Flush the pipe. */
    gcmONERROR(gcoHARDWARE_FlushPipe(Hardware, gcvNULL));

    /* Commit the command queue. */
    gcmONERROR(gcoHARDWARE_Commit(Hardware));

    /* Stall the hardware. */
    gcmONERROR(gcoHARDWARE_Stall(Hardware));


    /* Query pixel depth. */
    gcmONERROR(gcoHARDWARE_ConvertFormat(surf->format,
                                         &bitsPerPixel,
                                         gcvNULL));

    switch (surf->format)
    {
    case gcvSURF_X4R4G4B4: /* 12-bit RGB color without alpha channel. */
    case gcvSURF_X4B4G4R4: /* Treat as X4R4G4B4, RB channel flipped. */
        channelMask[0] = 0x000F;
        channelMask[1] = 0x00F0;
        channelMask[2] = 0x0F00;
        channelMask[3] = 0x0;
        break;

    case gcvSURF_D16:      /* 16-bit Depth. */
    case gcvSURF_A4R4G4B4: /* 12-bit RGB color with alpha channel. */
    case gcvSURF_A4B4G4R4: /* Treat as A4R4G4B4, RB channel flipped. */
        channelMask[0] = 0x000F;
        channelMask[1] = 0x00F0;
        channelMask[2] = 0x0F00;
        channelMask[3] = 0xF000;
        break;

    case gcvSURF_X1R5G5B5: /* 15-bit RGB color without alpha channel. */
        channelMask[0] = 0x001F;
        channelMask[1] = 0x03E0;
        channelMask[2] = 0x7C00;
        channelMask[3] = 0x0;
        break;

    case gcvSURF_A1R5G5B5: /* 15-bit RGB color with alpha channel. */
        channelMask[0] = 0x001F;
        channelMask[1] = 0x03E0;
        channelMask[2] = 0x7C00;
        channelMask[3] = 0x8000;
        break;

    case gcvSURF_R5G6B5: /* 16-bit RGB color without alpha channel. */
    case gcvSURF_B5G6R5: /* Treat as R5G6B5, RB channel flipped. */
        channelMask[0] = 0x001F;
        channelMask[1] = 0x07E0;
        channelMask[2] = 0xF800;
        channelMask[3] = 0x0;
        break;

    case gcvSURF_D24S8:    /* 24-bit Depth with 8 bit Stencil. */
    case gcvSURF_D24X8:    /* 24-bit Depth with 8 bit Stencil. */
    case gcvSURF_X24S8:    /* 24-bit Depth with 8 bit Stencil. */
    case gcvSURF_X8R8G8B8: /* 24-bit RGB without alpha channel. */
    case gcvSURF_A8R8G8B8: /* 24-bit RGB with alpha channel. */
    case gcvSURF_X8B8G8R8: /* Treat as X8R8G8B8, RB channel flipped. */
    case gcvSURF_A8B8G8R8: /* Treat as A8R8G8B8, RB channel flipped. */
    case gcvSURF_G8R8:     /* The clear value do not have channel 2 and 3 */
    case gcvSURF_R8_1_X8R8G8B8:
    case gcvSURF_G8R8_1_X8R8G8B8:
        channelMask[0] = 0x000000FF;
        channelMask[1] = 0x0000FF00;
        channelMask[2] = 0x00FF0000;
        channelMask[3] = 0xFF000000;
        break;

    case gcvSURF_X2B10G10R10:
        channelMask[0] = 0x000003FF;
        channelMask[1] = 0x000FFC00;
        channelMask[2] = 0x3FF00000;
        channelMask[3] = 0x0;
        break;

    case gcvSURF_A2B10G10R10:
        channelMask[0] = 0x000003FF;
        channelMask[1] = 0x000FFC00;
        channelMask[2] = 0x3FF00000;
        channelMask[3] = 0xC0000000;
        break;

    case gcvSURF_X16B16G16R16:
    case gcvSURF_A16B16G16R16:
    case gcvSURF_X16B16G16R16F:
    case gcvSURF_A16B16G16R16F:
    case gcvSURF_X16B16G16R16I:
    case gcvSURF_A16B16G16R16I:
    case gcvSURF_X16B16G16R16UI:
    case gcvSURF_A16B16G16R16UI:
    case gcvSURF_X16B16G16R16I_1_G32R32F:
    case gcvSURF_A16B16G16R16I_1_G32R32F:
    case gcvSURF_X16B16G16R16UI_1_G32R32F:
    case gcvSURF_A16B16G16R16UI_1_G32R32F:
        break;

    case gcvSURF_G32R32F:
    case gcvSURF_G32R32I:
    case gcvSURF_G32R32UI:
        break;

    case gcvSURF_X32B32G32R32I_2_G32R32I:
    case gcvSURF_A32B32G32R32I_2_G32R32I:
    case gcvSURF_A32B32G32R32I_2_G32R32F:
    case gcvSURF_X32B32G32R32UI_2_G32R32UI:
    case gcvSURF_A32B32G32R32UI_2_G32R32UI:
    case gcvSURF_A32B32G32R32UI_2_G32R32F:
        break;

    case gcvSURF_S8:
        break;

    default:
        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER();
        return status;
    }

    /* Expand 16-bit mask into 32-bit mask. */
    if (bitsPerPixel == 16)
    {
        channelMask[0] = channelMask[0] | (channelMask[0] << 16);
        channelMask[1] = channelMask[1] | (channelMask[1] << 16);
        channelMask[2] = channelMask[2] | (channelMask[2] << 16);
        channelMask[3] = channelMask[3] | (channelMask[3] << 16);
    }

    type = gcvSURF_RENDER_TARGET;

    if ((surf->format == gcvSURF_D16)
        || (surf->format == gcvSURF_D24S8)
        || (surf->format == gcvSURF_D32)
        || (surf->format == gcvSURF_D24X8)
        || (surf->format == gcvSURF_X24S8)
        )
    {
        type = gcvSURF_DEPTH;
    }

    gcmONERROR(gcoSURF_NODE_Cache(&surf->node,
                                   surf->node.logical,
                                   surf->size,
                                   gcvCACHE_INVALIDATE));

    gcmONERROR(_ClearSoftware(
        Hardware,
        SurfView,
        LayerIndex,
        Rect,
        channelMask,
        ClearValue,
        ClearValueUpper,
        ClearMask,
        type,
        StencilWriteMask));

    gcmONERROR(gcoSURF_NODE_Cache(&surf->node,
                                   surf->node.logical,
                                   surf->size,
                                   gcvCACHE_CLEAN));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif


