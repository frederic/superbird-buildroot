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

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

typedef enum __SPLIT_RECT_MODE
{
    SPLIT_RECT_MODE_NONE,
    SPLIT_RECT_MODE_COLUMN,
    SPLIT_RECT_MODE_LINE
} SPLIT_RECT_MODE;

#define SPLIT_COLUMN            4
#define SPLIT_COLUMN_WIDTH      (1 << SPLIT_COLUMN)
#define SPLIT_COLUMN_WIDTH_MASK (~(SPLIT_COLUMN_WIDTH - 1))


/*******************************************************************************
**
**  _RenderRectangle
**
**  2D software renderer.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gce2D_COMMAND Command
**          2D engine command to be executed.
**
**      gctUINT32 RectCount
**          Number of destination rectangles to be operated on.
**
**      gcsRECT_PTR DestRect
**          Pointer to an array of destination rectangles.
**
**      gctUINT32 FgRop
**      gctUINT32 BgRop
**          Foreground and background ROP codes.
**
**  OUTPUT:
**
**      Nothing .
*/

#if gcdENABLE_3D
gceSTATUS
gcoHARDWARE_FlushDrawID(
    IN gcoHARDWARE Hardware,
    IN gctBOOL  ForComputing,
    INOUT gctPOINTER *Memory
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gctUINT32 drawID;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

#if gcdFRAMEINFO_STATISTIC
    if (ForComputing)
    {
        gctUINT32  computeCount;
        gcoHAL_FrameInfoOps(gcvNULL,
                            gcvFRAMEINFO_COMPUTE_NUM,
                            gcvFRAMEINFO_OP_GET,
                            &computeCount);
        computeCount--;
        drawID = computeCount;

    }
    else
    {
        gctUINT32 drawCount;
        gctUINT32 frameCount;
        gcoHAL_FrameInfoOps(gcvNULL,
                            gcvFRAMEINFO_FRAME_NUM,
                            gcvFRAMEINFO_OP_GET,
                            &frameCount);

        gcoHAL_FrameInfoOps(gcvNULL,
                            gcvFRAMEINFO_DRAW_NUM,
                            gcvFRAMEINFO_OP_GET,
                            &drawCount);

        drawCount --;
        drawID = (frameCount << 16) | drawCount;

    }
#else
    {
        gctUINT32 programID;
        gcoHAL_FrameInfoOps(gcvNULL,
                            gcvFRAMEINFO_PROGRAM_ID,
                            gcvFRAMEINFO_OP_GET,
                            &programID);
        drawID = programID;
    }
#endif

    gcmONERROR(gcoHARDWARE_LoadCtrlStateNEW(Hardware, 0x0389C, drawID, Memory));

    gcmDUMP(gcvNULL, "#[info: drawID=0x%x]", drawID);

 OnError:

    gcmFOOTER();
    return status;
}

static gceSTATUS _CheckTargetDeferDither(
    IN gcoHARDWARE Hardware
    )
{
    /* If the draw required dither, while PE cannot support, defer until resolve time.
    ** If the draw didn't require, do not reset the flag, unless we are sure it's the
    ** draw can overwrite all of the surface.
    */
    gcsCOLOR_TARGET *pColorTarget = &Hardware->PEStates->colorStates.target[0];
    gcoSURF rtSurf = pColorTarget->surface;
    gctBOOL fakeFmt = rtSurf && rtSurf->formatInfo.fakedFormat;

    /* Disable dither for fake formats */
    if (!Hardware->features[gcvFEATURE_PE_DITHER_FIX] &&
        Hardware->PEStates->ditherEnable &&
        (rtSurf && !(rtSurf->flags & gcvSURF_FLAG_DITHER_DISABLED)) &&
        !fakeFmt)
    {
        gctUINT32 i;

        if (rtSurf)
        {
            rtSurf->deferDither3D = gcvTRUE;
        }

        for (i = 1; i < Hardware->PEStates->colorOutCount; ++i)
        {
            gcmASSERT(Hardware->PEStates->colorStates.target[i].surface);
            Hardware->PEStates->colorStates.target[i].surface->deferDither3D = gcvTRUE;
        }
    }

    return gcvSTATUS_OK;
}

static gceSTATUS gcoHARDWARE_FlushStates(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE Type,
    INOUT gctPOINTER *Memory
    )
{
    gceSTATUS status;
    gctPOINTER saveMemory = *Memory;

    /* Reset flush states. */
    Hardware->flushedColor = gcvFALSE;
    Hardware->flushedDepth = gcvFALSE;

    /* if PE dither enable and trigger draw(go though PE pipe), we need do dither in PE or resolve, no matter RT changed or not.*/
    _CheckTargetDeferDither(Hardware);

    if (Hardware->PEDirty->depthConfigDirty ||
        Hardware->PEDirty->colorConfigDirty ||
        Hardware->MsaaDirty->msaaConfigDirty  ||
        Hardware->SHDirty->shaderDirty      ||
        Hardware->PEDirty->alphaDirty)
    {
        /* Evaluate real depth only mode at the very beginning */
        gcmVERIFY_OK(gcoHARDWARE_FlushDepthOnly(Hardware));
    }

    /*If hardware support R8 format, these codes can be discarded.*/
    if (!Hardware->features[gcvFEATURE_R8_UNORM])
    {
        gctUINT32 i;

        for (i = 0; i < Hardware->PEStates->colorOutCount; ++i)
        {
            gcoSURF surface = Hardware->PEStates->colorStates.target[i].surface;

            if (surface && surface->paddingFormat &&
                Hardware->PEStates->alphaStates.blend[i] &&
                Hardware->PEStates->colorStates.target[i].colorWrite)
            {
                surface->garbagePadded = gcvTRUE;
            }
        }
    }

    /* switch to 3D pipe */
    gcmONERROR(gcoHARDWARE_SelectPipe(Hardware, gcvPIPE_3D, Memory));

    if (Hardware->QUERYStates->queryStatus[gcvQUERY_OCCLUSION] == gcvQUERY_Enabled &&
        !Hardware->features[gcvFEATURE_BUG_FIXES18] &&
        (Hardware->PEStates->depthStates.mode != gcvDEPTH_NONE))
    {
        Hardware->flushedDepth = gcvTRUE;

        gcmONERROR(gcoHARDWARE_LoadCtrlStateNEW(
            Hardware,
            0x0380C,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))),
            Memory
            ));
    }

    if (Hardware->FEDirty->indexDirty)
    {
        /* Program index states. */
        gcmONERROR(gcoHARDWARE_ProgramIndex(Hardware, Memory));
    }

#if gcdSECURITY
    /* Always mark full textures dirty for security mode */
    Hardware->TXDirty->textureDirty = gcvTRUE;
    Hardware->TXDirty->hwTxSamplerDirty = 0xFFFFFFFF;
    gcoOS_MemFill(Hardware->TXDirty->hwTxSamplerAddressDirty, 0xFF, gcmSIZEOF(Hardware->TXDirty->hwTxSamplerAddressDirty));
#endif

    if (Hardware->PAAndSEDirty->viewportDirty)
    {
        /* Flush viewport states. */
        gcmONERROR(gcoHARDWARE_FlushViewport(Hardware, Memory));
    }

    if (Hardware->PAAndSEDirty->scissorDirty)
    {
        /* Flush scissor states. */
        gcmONERROR(gcoHARDWARE_FlushScissor(Hardware, Memory));
    }

    if (Hardware->PEDirty->colorConfigDirty)
    {
        /* Flush target states. */
        gcmONERROR(gcoHARDWARE_FlushTarget(Hardware, Memory));
    }

    if (Hardware->PEDirty->alphaDirty)
    {
        /* Flush alpha states. */
        gcmONERROR(gcoHARDWARE_FlushAlpha(Hardware, Memory));
    }

    if (Hardware->PEDirty->depthConfigDirty ||
        Hardware->PEDirty->depthRangeDirty  ||
        Hardware->PEDirty->depthNormalizationDirty)
    {
        /* Flush depth states after FlushTarget */
        gcmONERROR(gcoHARDWARE_FlushDepth(Hardware, Memory));
    }

    if (Hardware->PEDirty->stencilDirty)
    {
        /* Flush stencil states. */
        gcmONERROR(gcoHARDWARE_FlushStencil(Hardware, Memory));
    }

    if (Hardware->MsaaDirty->msaaConfigDirty)
    {
        /* Flush anti-alias states. */
        gcmONERROR(gcoHARDWARE_FlushSampling(Hardware, Memory));
    }

    if (Hardware->PAAndSEDirty->paConfigDirty || Hardware->PAAndSEDirty->paLineDirty)
    {
        /* Flush primitive assembly states. */
        gcmONERROR(gcoHARDWARE_FlushPA(Hardware, Memory));
    }

    if (Hardware->SHDirty->uniformDirty)
    {
        /* Flush uniform states*/
        gcmONERROR(gcoHARDWARE_FlushUniform(Hardware, Memory));
    }

    if (Hardware->SHDirty->shaderDirty)
    {
        /* Flush shader states. */
        gcmONERROR(gcoHARDWARE_FlushShaders(Hardware, Type, Memory));

        if (Hardware->features[gcvFEATURE_SH_INSTRUCTION_PREFETCH])
        {
            gcmONERROR(gcoHARDWARE_FlushPrefetchInst(Hardware, Memory));
        }
    }

    if (Hardware->TXDirty->textureDirty)
    {
        /* Program texture states. */
        gcmONERROR((*Hardware->funcPtr->programTexture)(Hardware, Memory));
    }

    if (Hardware->features[gcvFEATURE_HW_TFB] && Hardware->XFBDirty->xfbDirty)
    {
        /* Flush Xfb states*/
        gcmONERROR(gcoHARDWARE_FlushXfb(Hardware, Memory));
    }

    if (Hardware->flushL2)
    {
        /* Flush L2 cache. */
        gcmONERROR(gcoHARDWARE_FlushL2Cache(Hardware, Memory));
    }

    if (Hardware->flushVtxData && Hardware->features[gcvFEATURE_MULTI_CLUSTER])
    {
        /* Flush Vertex data cache. */
        gcmONERROR(gcoHARDWARE_FlushVtxDataCache(Hardware, Memory));
    }

    if (Hardware->multiGPURenderingModeDirty && (Hardware->config->gpuCoreCount > 1))
    {
        /* TODO: select optimum rendering mode for different statemetn */
        gcmONERROR(gcoHARDWARE_FlushMultiGPURenderingMode(Hardware, Memory, gcvMULTI_GPU_RENDERING_MODE_INTERLEAVED_128x64));
    }

    if (Hardware->features[gcvFEATURE_DRAW_ID])
    {
        gcmONERROR(gcoHARDWARE_FlushDrawID(Hardware, gcvFALSE, Memory));
    }

#if gcdENABLE_TRUST_APPLICATION
    if (Hardware->features[gcvFEATURE_SECURITY] && Hardware->GPUProtecedModeDirty)
    {
        gcmONERROR(gcoHARDWARE_FlushProtectMode(Hardware, Memory));
    }
#endif

    if (Hardware->stallSource < Hardware->stallDestination)
    {
        /* Stall if we have to. */
        gcmONERROR(gcoHARDWARE_Semaphore(Hardware,
                                         Hardware->stallSource,
                                         Hardware->stallDestination,
                                         gcvHOW_STALL,
                                         Memory));
    }

    return gcvSTATUS_OK;

OnError:

    /* roll back all commands */
    *Memory = saveMemory;

    /* Return the status. */
    return status;
}
#endif


/*******************************************************************************
**
**  gco2D_GetMaximumDataCount
**
**  Retrieve the maximum number of 32-bit data chunks for a single DE command.
**
**  INPUT:
**
**      Nothing
**
**  OUTPUT:
**
**      gctUINT32
**          Data count value.
*/
gctUINT32 gco2D_GetMaximumDataCount(
    void
    )
{
    gctUINT32 result = 0;


    return result;
}

/*******************************************************************************
**
**  gco2D_GetMaximumRectCount
**
**  Retrieve the maximum number of rectangles, that can be passed in a single DE command.
**
**  INPUT:
**
**      Nothing
**
**  OUTPUT:
**
**      gctUINT32
**          Rectangle count value.
*/
gctUINT32 gco2D_GetMaximumRectCount(
    void
    )
{
    gctUINT32 result = 0;


    return result;
}

/*******************************************************************************
**
**  gco2D_GetPixelAlignment
**
**  Returns the pixel alignment of the surface.
**
**  INPUT:
**
**      gceSURF_FORMAT Format
**          Pixel format.
**
**  OUTPUT:
**
**      gcsPOINT_PTR Alignment
**          Pointer to the pixel alignment values.
*/
gceSTATUS gco2D_GetPixelAlignment(
    gceSURF_FORMAT Format,
    gcsPOINT_PTR Alignment
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;


    return status;
}


#if gcdENABLE_3D
static gcmINLINE gceSTATUS
_InternalTFBSwitch(
    gcoHARDWARE Hardware,
    gctBOOL Enable,
    gctPOINTER *Memory
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmASSERT(Hardware->features[gcvFEATURE_HW_TFB]);

    if ((Hardware->XFBStates->status != gcvXFB_Enabled) &&
        (Hardware->QUERYStates->queryStatus[gcvQUERY_PRIM_GENERATED] == gcvQUERY_Enabled))
    {
        gctUINT32 tfbCmd = Enable
                         ?
 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                         : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));

        /* Define state buffer variables. */
        gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

        gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, Memory);

        if (Enable)
        {
            gctUINT32 physical;

            gcmASSERT(Hardware->XFBStates->internalXFB == gcvFALSE);

            if (gcvNULL == Hardware->XFBStates->internalXFBNode)
            {
                gcoOS_Allocate(gcvNULL, gcmSIZEOF(gcsSURF_NODE), (gctPOINTER *)&Hardware->XFBStates->internalXFBNode);
                gcoOS_ZeroMemory((gctPOINTER)Hardware->XFBStates->internalXFBNode, gcmSIZEOF(gcsSURF_NODE));
                gcmONERROR(gcsSURF_NODE_Construct(Hardware->XFBStates->internalXFBNode,
                                                  64,
                                                  64,
                                                  gcvSURF_TFBHEADER,
                                                  0,
                                                  gcvPOOL_DEFAULT
                                                  ));

                gcmONERROR(gcoSURF_LockNode(Hardware->XFBStates->internalXFBNode, gcvNULL, &Hardware->XFBStates->internalXFBLocked));

                gcoOS_ZeroMemory(Hardware->XFBStates->internalXFBLocked, 64);

                gcmASSERT(Hardware->XFBStates->internalXFBLocked);
            }

            gcmGETHARDWAREADDRESSP(Hardware->XFBStates->internalXFBNode, physical);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x7002) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x7002, physical);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x7020) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x7020, 0);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x7030) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x7030, 16);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            Hardware->XFBStates->internalXFB = gcvTRUE;
        }
        else
        {
            gcmASSERT(Hardware->XFBStates->internalXFB);


            Hardware->XFBStates->internalXFB = gcvFALSE;
            Hardware->XFBDirty->s.headerDirty = gcvTRUE;
            Hardware->XFBDirty->s.bufferDirty = gcvTRUE;
        }

        /* Switch to single GPU mode */
        if (Hardware->config->gpuCoreCount > 1)
        {
            gcoHARDWARE_MultiGPUSync(Hardware, &memory);
            { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x7001) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x7001, tfbCmd);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        /* Resume to multiple GPU mode */
        if (Hardware->config->gpuCoreCount > 1)
        {
            { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

            gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        }

        gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, Memory);
    }
OnError:
    return status;
}


static gcmINLINE gceSTATUS
_SinglePEPipeSwitch(
    gcoHARDWARE Hardware,
    gctBOOL Single,
    gctPOINTER *Memory
    )
{

    gctUINT32 flush = 0;
    gceSTATUS status = gcvSTATUS_OK;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    /* switch from dual-pipe to single-pipe */
    if (Single)
    {
        if (!Hardware->flushedColor)
        {
            flush |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:1) - (0 ?
 1:1) + 1))))))) << (0 ?
 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)));
        }
        if (!Hardware->flushedDepth)
        {
            flush |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));
        }
    }

    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, Memory);

    if (flush)
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0E03, flush);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x052F) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x052F, ((((gctUINT32) (Hardware->PEStates->peConfigExReg)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 0:0) - (0 ?
 0:0) + 1))))))) << (0 ?
 0:0))) | (((gctUINT32) ((gctUINT32) (Single ?
 0x1 : 0x0) & ((gctUINT32) ((((1 ?
 0:0) - (0 ?
 0:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, Memory);
OnError:
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_DrawPrimitives
**
**  Draw a number of primitives.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctINT StartVertex
**          Starting vertex number to start drawing.  The starting vertex is
**          multiplied by the stream stride to compute the starting offset.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawPrimitives(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE Type,
    IN gctINT StartVertex,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;
    gctPOINTER *outside = gcvNULL;
    gctBOOL useOneCore = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    static const gctINT32 xlate[] =
    {
        /* gcvPRIMITIVE_POINT_LIST */
        0x1,
        /* gcvPRIMITIVE_LINE_LIST */
        0x2,
        /* gcvPRIMITIVE_LINE_STRIP */
        0x3,
        /* gcvPRIMITIVE_LINE_LOOP */
        0x7,
        /* gcvPRIMITIVE_TRIANGLE_LIST */
        0x4,
        /* gcvPRIMITIVE_TRIANGLE_STRIP */
        0x5,
        /* gcvPRIMITIVE_TRIANGLE_FAN */
        0x6,
        /* gcvPRIMITIVE_RECTANGLE */
        0x8
    };

    gcmHEADER_ARG("Hardware=0x%x Type=%d StartVertex=%d PrimitiveCount=%u",
                  Hardware, Type, StartVertex, PrimitiveCount);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NO_DRAW_PRIMITIVES
    {
        gcePATCH_ID patchId = gcvPATCH_INVALID;

        gcoHARDWARE_GetPatchID(gcvNULL, &patchId);

        if (patchId == gcvPATCH_DEBUG)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }
#endif

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    /* Flush pipe states. */
    gcmONERROR(gcoHARDWARE_FlushStates(Hardware, Type, (gctPOINTER *)&memory));

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    if (!Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        switch (Type)
        {
        case gcvPRIMITIVE_LINE_STRIP:
        case gcvPRIMITIVE_LINE_LOOP:
        case gcvPRIMITIVE_TRIANGLE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_FAN:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;

        default:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;
        }
    }

    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }

    if (useOneCore)
    {
        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

    }

    /* Program the AQCommandDX8Primitive.Command data. */
    memory[0] =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x05 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

    /* Program the AQCommandDX8Primitive.Primtype data. */
    memory[1] =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (xlate[Type]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)));

    /* Program the AQCommandDX8Primitive.Vertexstart data. */
    memory[2] = StartVertex;

    /* Program the AQCommandDX8Primitive.Primcount data. */
    gcmSAFECASTSIZET(memory[3], PrimitiveCount);

    memory += 4;

    /* Dump the draw primitive. */
    gcmDUMP(gcvNULL,
            "#[draw 0x%08X 0x%08X 0x%08X]",
            xlate[Type],
            StartVertex,
            PrimitiveCount);

    if (useOneCore)
    {
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
    }

    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }
    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);

#if gcdFPGA_BUILD
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
#endif


    /* Mark tile status buffers as dirty. */
    Hardware->MCDirty->cacheDirty        = gcvTRUE;

    /* change xfb/query status in cmdbuffer */
    Hardware->XFBStates->statusInCmd = Hardware->XFBStates->status;

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
**  gcoHARDWARE_DrawIndirectPrimitives
**
**  Draw a number of primitives through indirect parameter buffer
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctBOOL DrawIndex
**          Whether or not to draw index mode.
**
**      gctUINT Address
**          Address to the draw parameter buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawIndirectPrimitives(
    IN gcoHARDWARE Hardware,
    IN gctBOOL DrawIndex,
    IN gcePRIMITIVE Type,
    IN gctUINT Address
    )
{
    gceSTATUS status;

    gctUINT32 drawMode = DrawIndex ?
                         0x1:
                         0x0;

    gctPOINTER *outside = gcvNULL;
    gctBOOL useOneCore = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    static const gctINT32 xlate[] =
    {
        /* gcvPRIMITIVE_POINT_LIST */
        0x1,
        /* gcvPRIMITIVE_LINE_LIST */
        0x2,
        /* gcvPRIMITIVE_LINE_STRIP */
        0x3,
        /* gcvPRIMITIVE_LINE_LOOP */
        0x7,
        /* gcvPRIMITIVE_TRIANGLE_LIST */
        0x4,
        /* gcvPRIMITIVE_TRIANGLE_STRIP */
        0x5,
        /* gcvPRIMITIVE_TRIANGLE_FAN */
        0x6,
        /* gcvPRIMITIVE_RECTANGLE */
        0x8,
        /* gcvPRIMITIVE_LINES_ADJACENCY */
        0x9,
        /* gcvPRIMITIVE_LINE_STRIP_ADJACENCY */
        0xA,
        /* gcvPRIMITIVE_TRIANGLES_ADJACENCY */
        0xB,
        /* gcvPRIMITIVE_TRIANGLE_STRIP_ADJACENCY */
        0xC,
        /* gcvPRIMITIVE_PATCH_LIST */
        0xD,
    };

    gcmHEADER_ARG("Hardware=0x%x Type=%d DrawIndex=%d Address=0x%x",
                  Hardware, Type, DrawIndex, Address);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NO_DRAW_PRIMITIVES
    {
        gcePATCH_ID patchId = gcvPATCH_INVALID;

        gcoHARDWARE_GetPatchID(gcvNULL, &patchId);

        if (patchId == gcvPATCH_DEBUG)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }
#endif

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    /* Flush pipe states. */
    gcmONERROR(gcoHARDWARE_FlushStates(Hardware, Type, (gctPOINTER *)&memory));

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    if (!Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        switch (Type)
        {
        case gcvPRIMITIVE_LINE_STRIP:
        case gcvPRIMITIVE_LINE_LOOP:
        case gcvPRIMITIVE_TRIANGLE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_FAN:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;

        default:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;
        }
    }

    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }

    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }

    if (useOneCore)
    {
        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

    }


    /* Program the GCCMD_DRAW_INDIRECT_COMMAND.Command data. */
    memory[0] =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x10 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:8) - (0 ?
 8:8) + 1))))))) << (0 ?
 8:8))) | (((gctUINT32) ((gctUINT32) (drawMode) & ((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (xlate[Type]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:14) - (0 ?
 14:14) + 1))))))) << (0 ?
 14:14))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)));


    /* Program the GCCMD_DRAW_INDIRECT_ADDRESS data. */

    memory[1] = Address;

    memory += 2;

    /* Dump the draw primitive. */
    gcmDUMP(gcvNULL,
            "#[draw.indirectprim 0x%08X 0x%08X]",
            xlate[Type],
            Address);

    if (useOneCore)
    {
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
    }

    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }
    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);

#if gcdFPGA_BUILD
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
#endif


    /* Mark tile status buffers as dirty. */
    Hardware->MCDirty->cacheDirty        = gcvTRUE;

    /* change xfb/query status in cmdbuffer */
    Hardware->XFBStates->statusInCmd = Hardware->XFBStates->status;

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
**  gcoHARDWARE_MultiDrawIndirectPrimitives
**
**  Draw a number of primitives through indirect parameter buffer
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctBOOL DrawIndex
**          Whether or not to draw index mode.
**
**      gctINT DrawCount
**          Draw command number.
**
**      gctINT Stride
**          Draw parameter stride.
**
**      gctUINT Address
**          Address to the draw parameter buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_MultiDrawIndirectPrimitives(
    IN gcoHARDWARE Hardware,
    IN gctBOOL DrawIndex,
    IN gcePRIMITIVE Type,
    IN gctINT DrawCount,
    IN gctINT Stride,
    IN gctUINT Address
    )
{
    gceSTATUS status;

    gctUINT32 drawMode = DrawIndex ?
                         0x1:
                         0x0;

    gctPOINTER *outside = gcvNULL;

    gctBOOL useOneCore = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    static const gctINT32 xlate[] =
    {
        /* gcvPRIMITIVE_POINT_LIST */
        0x1,
        /* gcvPRIMITIVE_LINE_LIST */
        0x2,
        /* gcvPRIMITIVE_LINE_STRIP */
        0x3,
        /* gcvPRIMITIVE_LINE_LOOP */
        0x7,
        /* gcvPRIMITIVE_TRIANGLE_LIST */
        0x4,
        /* gcvPRIMITIVE_TRIANGLE_STRIP */
        0x5,
        /* gcvPRIMITIVE_TRIANGLE_FAN */
        0x6,
        /* gcvPRIMITIVE_RECTANGLE */
        0x8,
        /* gcvPRIMITIVE_LINES_ADJACENCY */
        0x9,
        /* gcvPRIMITIVE_LINE_STRIP_ADJACENCY */
        0xA,
        /* gcvPRIMITIVE_TRIANGLES_ADJACENCY */
        0xB,
        /* gcvPRIMITIVE_TRIANGLE_STRIP_ADJACENCY */
        0xC,
        /* gcvPRIMITIVE_PATCH_LIST */
        0xD,
    };

    gcmHEADER_ARG("Hardware=0x%x Type=%d DrawIndex=%d Address=0x%x DrawCount=%d Stride=%d",
        Hardware, Type, DrawIndex, DrawCount, Stride,Address);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NO_DRAW_PRIMITIVES
    {
        gcePATCH_ID patchId = gcvPATCH_INVALID;

        gcoHARDWARE_GetPatchID(gcvNULL, &patchId);

        if (patchId == gcvPATCH_DEBUG)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }
#endif
    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    /* Flush pipe states. */
    gcmONERROR(gcoHARDWARE_FlushStates(Hardware, Type, (gctPOINTER *)&memory));

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    if (!Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        switch (Type)
        {
        case gcvPRIMITIVE_LINE_STRIP:
        case gcvPRIMITIVE_LINE_LOOP:
        case gcvPRIMITIVE_TRIANGLE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_FAN:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;

        default:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;
        }
    }

    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }

    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }

    if (useOneCore)
    {
        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

    }

    /* Program the GCCMD_DRAW_INDIRECT_COMMAND.Command data. */

    memory[0] =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x10 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:8) - (0 ?
 8:8) + 1))))))) << (0 ?
 8:8))) | (((gctUINT32) ((gctUINT32) (drawMode) & ((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (xlate[Type]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 14:14) - (0 ?
 14:14) + 1))))))) << (0 ?
 14:14))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ?
 14:14) - (0 ?
 14:14) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)));


    /* Program the GCCMD_DRAW_INDIRECT_ADDRESS data. */

    memory[1] = Address;
    memory[2] =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 17:0) - (0 ?
 17:0) + 1))))))) << (0 ?
 17:0))) | (((gctUINT32) ((gctUINT32) (DrawCount) & ((gctUINT32) ((((1 ?
 17:0) - (0 ?
 17:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 17:0) - (0 ? 17:0) + 1))))))) << (0 ? 17:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:18) - (0 ?
 31:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:18) - (0 ?
 31:18) + 1))))))) << (0 ?
 31:18))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ?
 31:18) - (0 ?
 31:18) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:18) - (0 ? 31:18) + 1))))))) << (0 ? 31:18)));

    memory += 3;

    /* Dump the draw primitive. */
    gcmDUMP(gcvNULL,
            "#[multidraw.indirectprim count 0x%08X 0x%08X 0x%08X]",
            DrawCount,
            xlate[Type],
            Address);

    if (useOneCore)
    {
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
    }

    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }
    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);

#if gcdFPGA_BUILD
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
        0x00A18,
        0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
        0x00A18,
        0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
        0x00A18,
        0x00000000));
#endif


    /* Mark tile status buffers as dirty. */
    Hardware->MCDirty->cacheDirty        = gcvTRUE;

    /* change xfb/query status in cmdbuffer */
    Hardware->XFBStates->statusInCmd = Hardware->XFBStates->status;

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
**  gcoHARDWARE_DrawInstancedPrimitives
**
**  Draw a number of primitives.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctINT StartVertex
**          Starting vertex number to start drawing.  The starting vertex is
**          multiplied by the stream stride to compute the starting offset.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawInstancedPrimitives(
    IN gcoHARDWARE Hardware,
    IN gctBOOL DrawIndex,
    IN gcePRIMITIVE Type,
    IN gctINT StartVertex,
    IN gctINT StartIndex,
    IN gctSIZE_T PrimitiveCount,
    IN gctSIZE_T VertexCount,
    IN gctSIZE_T InstanceCount
    )
{
    gceSTATUS status;
    gctSIZE_T vertexCount;
    gctUINT32 drawCommand;
    gctUINT32 drawCount;
    gctUINT32 drawMode = DrawIndex ?
                         0x1 :
                         0x0;
    gctPOINTER *outside = gcvNULL;
    gctBOOL useOneCore = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    static const gctINT32 xlate[] =
    {
        /* gcvPRIMITIVE_POINT_LIST */
        0x1,
        /* gcvPRIMITIVE_LINE_LIST */
        0x2,
        /* gcvPRIMITIVE_LINE_STRIP */
        0x3,
        /* gcvPRIMITIVE_LINE_LOOP */
        0x7,
        /* gcvPRIMITIVE_TRIANGLE_LIST */
        0x4,
        /* gcvPRIMITIVE_TRIANGLE_STRIP */
        0x5,
        /* gcvPRIMITIVE_TRIANGLE_FAN */
        0x6,
        /* gcvPRIMITIVE_RECTANGLE */
        0x8,
        /* gcvPRIMITIVE_LINES_ADJACENCY */
        0x9,
        /* gcvPRIMITIVE_LINE_STRIP_ADJACENCY */
        0xA,
        /* gcvPRIMITIVE_TRIANGLES_ADJACENCY */
        0xB,
        /* gcvPRIMITIVE_TRIANGLE_STRIP_ADJACENCY */
        0xC,
        /* gcvPRIMITIVE_PATCH_LIST */
        0xD,
    };

    gcmHEADER_ARG("Hardware=0x%x Type=%d StartVertex=%d PrimitiveCount=%u, VertexCount=%u, IntanceCount=%u",
                  Hardware, Type, StartVertex, PrimitiveCount, VertexCount, InstanceCount);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(VertexCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(InstanceCount > 0);

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NO_DRAW_PRIMITIVES
    {
        gcePATCH_ID patchId = gcvPATCH_INVALID;

        gcoHARDWARE_GetPatchID(gcvNULL, &patchId);

        if (patchId == gcvPATCH_DEBUG)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }
#endif

    if (!Hardware->features[gcvFEATURE_HALTI2])
    {
        switch (Type)
        {
            case gcvPRIMITIVE_TRIANGLE_LIST:
                vertexCount = VertexCount - (VertexCount % 3);
                break;

            case gcvPRIMITIVE_LINE_LIST:
                vertexCount = VertexCount - (VertexCount % 2);
                break;

            case gcvPRIMITIVE_LINE_STRIP:
            case gcvPRIMITIVE_TRIANGLE_STRIP:
            case gcvPRIMITIVE_TRIANGLE_FAN:
            default:
                vertexCount = VertexCount;
                break;
        }
    }
    else
    {
        vertexCount = VertexCount;
    }

    /* Determine draw command. */
    drawCommand = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0C & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:20) - (0 ?
 22:20) + 1))))))) << (0 ?
 22:20))) | (((gctUINT32) ((gctUINT32) (drawMode) & ((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (InstanceCount) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (xlate[Type]) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)));

    drawCount = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:0) - (0 ?
 23:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:0) - (0 ?
 23:0) + 1))))))) << (0 ?
 23:0))) | (((gctUINT32) ((gctUINT32) (vertexCount) & ((gctUINT32) ((((1 ?
 23:0) - (0 ?
 23:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:0) - (0 ? 23:0) + 1))))))) << (0 ? 23:0)))
              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) ((InstanceCount >> 16)) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)));

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    /* Flush pipe states. */
    gcmONERROR(gcoHARDWARE_FlushStates(Hardware, Type, (gctPOINTER *)&memory));

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    if (!Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        switch (Type)
        {
        case gcvPRIMITIVE_LINE_STRIP:
        case gcvPRIMITIVE_LINE_LOOP:
        case gcvPRIMITIVE_TRIANGLE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_FAN:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;
        default:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;
        }
    }

    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }
    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }

    if (useOneCore)
    {
        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

    }

    /* Program the AQCommandDX8Primitive.Command data. */
    *memory++ = drawCommand;
    *memory++ = drawCount;

    if (Hardware->features[gcvFEATURE_FE_START_VERTEX_SUPPORT])
    {
        *memory++ = StartVertex;
        *memory++ = 0;
    }
    else
    {
        /* start index is not set */
        *memory++ = 0;
        *memory++ = 0;
    }

    /* Dump the draw primitive. */
    gcmDUMP(gcvNULL,
            "#[draw.instanced 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X]",
            drawMode, /* 22:20 */
            xlate[Type], /*  19:16 */
            InstanceCount, /* 15:0 */
            vertexCount, /* 23:0 */
            StartVertex /* 2 */
            );

    if (useOneCore)
    {
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
    }

    if (Hardware->features[gcvFEATURE_HALTI5] &&
        !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
        (Hardware->config->pixelPipes > 1) &&
        (Hardware->PEStates->singlePEpipe))
    {
        _SinglePEPipeSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }
    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);


    /* Mark tile status buffers as dirty. */
    Hardware->MCDirty->cacheDirty        = gcvTRUE;

    /* change xfb/query status in cmdbuffer */
    Hardware->XFBStates->statusInCmd = Hardware->XFBStates->status;

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
**  gcoHARDWARE_DrawNullPrimitives
**
**  Draw none of primitives.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawNullPrimitives(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gctPOINTER *outside = gcvNULL;
    gctBOOL useOneCore = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

#if gcdDEBUG_OPTION && gcdDEBUG_OPTION_NO_DRAW_PRIMITIVES
    {
        gcePATCH_ID patchId = gcvPATCH_INVALID;

        gcoHARDWARE_GetPatchID(gcvNULL, &patchId);

        if (patchId == gcvPATCH_DEBUG)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }
#endif

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    /* Flush pipe states. */
    gcmONERROR(gcoHARDWARE_FlushStates(Hardware, gcvPRIMITIVE_TRIANGLE_LIST, (gctPOINTER *)&memory));

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
    }

    if (useOneCore)
    {
        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

    }
    /* null draw command */

    if (useOneCore)
    {
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
    }

    if (Hardware->features[gcvFEATURE_HW_TFB])
    {
        _InternalTFBSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
    }

    /* Make compiler happy */
#ifndef __clang__
    stateDelta = stateDelta;
#endif

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);



    /* Mark tile status buffers as dirty. */
    Hardware->MCDirty->cacheDirty        = gcvTRUE;

    /* change xfb/query status in cmdbuffer */
    Hardware->XFBStates->statusInCmd = Hardware->XFBStates->status;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


static gceSTATUS _GetPrimitiveCount(
    IN gcePRIMITIVE PrimitiveMode,
    IN gctSIZE_T VertexCount,
    OUT gctSIZE_T * PrimitiveCount
    )
{
    gceSTATUS result = gcvSTATUS_OK;

    /* Translate primitive count. */
    switch (PrimitiveMode)
    {
    case gcvPRIMITIVE_POINT_LIST:
        *PrimitiveCount = VertexCount;
        break;

    case gcvPRIMITIVE_LINE_LIST:
        *PrimitiveCount = VertexCount / 2;
        break;

    case gcvPRIMITIVE_LINE_LOOP:
        *PrimitiveCount = VertexCount;
        break;

    case gcvPRIMITIVE_LINE_STRIP:
        *PrimitiveCount = VertexCount - 1;
        break;

    case gcvPRIMITIVE_TRIANGLE_LIST:
        *PrimitiveCount = VertexCount / 3;
        break;

    case gcvPRIMITIVE_TRIANGLE_STRIP:
    case gcvPRIMITIVE_TRIANGLE_FAN:
        *PrimitiveCount = VertexCount - 2;
        break;

    default:
        result = gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Return result. */
    return result;
}

/*******************************************************************************
**
**  gcoHARDWARE_DrawPrimitivesCount
**
**  Draw an array of primitives.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctINT StartVertex
**          Starting vertex array of number to start drawing.  The starting vertex is
**          multiplied by the stream stride to compute the starting offset.
**
**      gctSIZE_T VertexCount
**          Number of vertices to draw per primitive.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawPrimitivesCount(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE Type,
    IN gctINT* StartVertex,
    IN gctSIZE_T* VertexCount,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;
    gctUINT32 i;
    gctPOINTER *outside = gcvNULL;
    gctBOOL useOneCore = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    static const gctINT32 xlate[] =
    {
        /* gcvPRIMITIVE_POINT_LIST */
        0x1,
        /* gcvPRIMITIVE_LINE_LIST */
        0x2,
        /* gcvPRIMITIVE_LINE_STRIP */
        0x3,
        /* gcvPRIMITIVE_LINE_LOOP */
        0x7,
        /* gcvPRIMITIVE_TRIANGLE_LIST */
        0x4,
        /* gcvPRIMITIVE_TRIANGLE_STRIP */
        0x5,
        /* gcvPRIMITIVE_TRIANGLE_FAN */
        0x6,
        /* gcvPRIMITIVE_RECTANGLE */
        0x8
    };

    gcmHEADER_ARG("Hardware=0x%x Type=%d StartVertex=%d PrimitiveCount=%u",
                  Hardware, Type, StartVertex, PrimitiveCount);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);
    gcmVERIFY_ARGUMENT(Type <= gcvPRIMITIVE_RECTANGLE);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    /* Flush pipe states. */
    gcmONERROR(gcoHARDWARE_FlushStates(Hardware, Type, (gctPOINTER *)&memory));

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    if (!Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        switch (Type)
        {
        case gcvPRIMITIVE_LINE_STRIP:
        case gcvPRIMITIVE_LINE_LOOP:
        case gcvPRIMITIVE_TRIANGLE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_FAN:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;

        default:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;
        }
    }

    if (useOneCore)
    {
        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

    }

    for (i = 0; i < PrimitiveCount; i++)
    {
        gctSIZE_T count = 0;

        gcmVERIFY_OK(_GetPrimitiveCount(Type, VertexCount[i], &count));

        /* Program the AQCommandDX8Primitive.Command data. */
        *memory++ =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x05 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

        /* Program the AQCommandDX8Primitive.Primtype data. */
        *memory++ =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (xlate[Type]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)));

        /* Program the AQCommandDX8Primitive.Vertexstart data. */
        *memory++ = StartVertex[i];

        /* Program the AQCommandDX8Primitive.Primcount data. */
        gcmSAFECASTSIZET(*memory++, count);
    }

    /* Dump the draw primitive. */
    gcmDUMP(gcvNULL,
            "#[draw 0x%08X 0x%08X 0x%08X]",
            xlate[Type],
            (gctUINTPTR_T) StartVertex,
            PrimitiveCount);

    if (useOneCore)
    {
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);

#if gcdFPGA_BUILD
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
#endif


    /* Mark tile status buffers as dirty. */
    Hardware->MCDirty->cacheDirty        = gcvTRUE;

    /* change xfb/query status in cmdbuffer */
    Hardware->XFBStates->statusInCmd = Hardware->XFBStates->status;

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
**  gcoHARDWARE_DrawPrimitivesOffset
**
**  Draw a number of primitives using offsets.
**
**  INPUT:
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctINT32 StartOffset
**          Starting offset.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawPrimitivesOffset(
    IN gcePRIMITIVE Type,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    gcmHEADER_ARG("Type=%d StartOffset=%d PrimitiveCount=%u",
                   Type, StartOffset, PrimitiveCount);

    /* No supported on XAQ2. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_NOT_SUPPORTED);
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoHARDWARE_DrawIndexedPrimitives
**
**  Draw a number of indexed primitives.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctINT BaseVertex
**          Base vertex number which will be added to any indexed vertex read
**          from the index buffer.
**
**      gctINT StartIndex
**          Starting index number to start drawing.  The starting index is
**          multiplied by the index buffer stride to compute the starting
**          offset.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawIndexedPrimitives(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE Type,
    IN gctINT BaseVertex,
    IN gctINT StartIndex,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gctPOINTER *outside = gcvNULL;

    gctBOOL useOneCore = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    static const gctINT32 xlate[] =
    {
        /* gcvPRIMITIVE_POINT_LIST */
        0x1,
        /* gcvPRIMITIVE_LINE_LIST */
        0x2,
        /* gcvPRIMITIVE_LINE_STRIP */
        0x3,
        /* gcvPRIMITIVE_LINE_LOOP */
        0x7,
        /* gcvPRIMITIVE_TRIANGLE_LIST */
        0x4,
        /* gcvPRIMITIVE_TRIANGLE_STRIP */
        0x5,
        /* gcvPRIMITIVE_TRIANGLE_FAN */
        0x6,
        /* gcvPRIMITIVE_RECTANGLE */
        0x8
    };

    gcmHEADER_ARG("Hardware=0x%x Type=%d BaseVertex=%d "
                    "StartIndex=%d PrimitiveCount=%u",
                    Hardware, Type, BaseVertex,
                    StartIndex, PrimitiveCount);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);
    gcmVERIFY_ARGUMENT(Type <= gcvPRIMITIVE_RECTANGLE);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outside);

    /* Flush pipe states. */
    gcmONERROR(gcoHARDWARE_FlushStates(Hardware, Type, (gctPOINTER *)&memory));

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    if (!Hardware->features[gcvFEATURE_NEW_GPIPE])
    {
        switch (Type)
        {
        case gcvPRIMITIVE_LINE_STRIP:
        case gcvPRIMITIVE_LINE_LOOP:
        case gcvPRIMITIVE_TRIANGLE_STRIP:
        case gcvPRIMITIVE_TRIANGLE_FAN:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;

        default:
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            break;
        }
    }

    if (useOneCore)
    {
        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

    }

    /* Program the AQCommandDX8IndexPrimitive.Command data. */
    memory[0] =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

    /* Program the AQCommandDX8IndexPrimitive.Primtype data. */
    memory[1] =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (xlate[Type]) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)));

    /* Program the AQCommandDX8IndexPrimitive.Start data. */
    memory[2] = StartIndex;

    /* Program the AQCommandDX8IndexPrimitive.Primcount data. */
    gcmSAFECASTSIZET(memory[3], PrimitiveCount);

    /* Program the AQCommandDX8IndexPrimitive.Basevertex data. */
    memory[4] = BaseVertex;

    memory += 5;

    /* Dump the indexed draw primitive. */
    gcmDUMP(gcvNULL,
            "#[draw.index 0x%08X 0x%08X 0x%08X 0x%08X]",
            xlate[Type],
            StartIndex,
            PrimitiveCount,
            BaseVertex);

    if (useOneCore)
    {
        { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

        gcoHARDWARE_MultiGPUSync(Hardware, &memory);
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outside);

#if gcdFPGA_BUILD
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A18,
                                       0x00000000));
#endif


    /* Mark tile status bufefrs as dirty. */
    Hardware->MCDirty->cacheDirty        = gcvTRUE;

    /* change xfb/query status in cmdbuffer */
    Hardware->XFBStates->statusInCmd = Hardware->XFBStates->status;
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
**  gcoHARDWARE_DrawIndexedPrimitivesOffset
**
**  Draw a number of indexed primitives using offsets.
**
**  INPUT:
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctINT BaseOffset
**          Base offset which will be added to any indexed vertex offset read
**          from the index buffer.
**
**      gctINT StartOffset
**          Starting offset into the index buffer to start drawing.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DrawIndexedPrimitivesOffset(
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    gcmHEADER_ARG("Type=%d BaseOffset=%d "
                  "StartOffset=%d PrimitiveCount=%u",
                  Type, BaseOffset,
                  StartOffset, PrimitiveCount);

    /* No supported on XAQ2. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_NOT_SUPPORTED);
    return gcvSTATUS_NOT_SUPPORTED;
}

static gceSTATUS
_FastDrawIndexedPrimitive(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo,
    INOUT gctPOINTER *Memory)
{
    gceSTATUS status;
    gctUINT primCount = FastFlushInfo->drawCount / 3;
    gctBOOL useOneCore = gcvFALSE;

    gcmHEADER_ARG("Hardware=0x%x FastFlushInfo=0x%x", Hardware, FastFlushInfo);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->gpuRenderingMode == gcvMULTI_GPU_RENDERING_MODE_OFF)
    {
        useOneCore = gcvTRUE;
    }

    /* Send command */
    if (FastFlushInfo->hasHalti)
    {
        gctSIZE_T vertexCount;
        gctUINT32 drawCommand;
        gctUINT32 drawCount;
        gctUINT32 drawMode = (FastFlushInfo->indexCount != 0) ?
                0x1 :
                0x0;

        /* Define state buffer variables. */
        gcmDEFINESTATEBUFFER_NEW_FAST(reserve, memory);

        if (!Hardware->features[gcvFEATURE_HALTI2])
        {
            vertexCount = FastFlushInfo->drawCount - (FastFlushInfo->drawCount % 3);
        }
        else
        {
            vertexCount = FastFlushInfo->drawCount;
        }

        /* Determine draw command. */
        drawCommand = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0C & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 22:20) - (0 ?
 22:20) + 1))))))) << (0 ?
 22:20))) | (((gctUINT32) ((gctUINT32) (drawMode) & ((gctUINT32) ((((1 ?
 22:20) - (0 ?
 22:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (FastFlushInfo->instanceCount) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (0x4) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)));

        drawCount = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:0) - (0 ?
 23:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:0) - (0 ?
 23:0) + 1))))))) << (0 ?
 23:0))) | (((gctUINT32) ((gctUINT32) (vertexCount) & ((gctUINT32) ((((1 ?
 23:0) - (0 ?
 23:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:0) - (0 ? 23:0) + 1))))))) << (0 ? 23:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:24) - (0 ?
 31:24) + 1))))))) << (0 ?
 31:24))) | (((gctUINT32) ((gctUINT32) ((FastFlushInfo->instanceCount >> 16)) & ((gctUINT32) ((((1 ?
 31:24) - (0 ?
 31:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)));

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER_NEW_FAST(Hardware, reserve, memory, Memory);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW_FAST(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        if (Hardware->features[gcvFEATURE_HW_TFB])
        {
            _InternalTFBSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
        }

        if (Hardware->features[gcvFEATURE_HALTI5] &&
            !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
            (Hardware->config->pixelPipes > 1) &&
            (Hardware->PEStates->singlePEpipe))
        {
            _SinglePEPipeSwitch(Hardware, gcvTRUE, (gctPOINTER *)&memory);
        }
        if (useOneCore)
        {
            gcoHARDWARE_MultiGPUSync(Hardware, &memory);
            { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

        }

        /* Program the AQCommandDX8Primitive.Command data. */
        *memory++ = drawCommand;
        *memory++ = drawCount;

        if (Hardware->features[gcvFEATURE_HALTI2])
        {
            *memory++ = 0;
            *memory++ = 0;
        }
        else
        {
            /* start index is not set */
            *memory++ = 0;

            gcmSETFILLER_NEW(
                reserve,
                memory
                );
        }

        if (useOneCore)
        {
            { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

            gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        }

        if (Hardware->features[gcvFEATURE_HALTI5] &&
            !Hardware->features[gcvFEATURE_MRT_8BIT_DUAL_PIPE_FIX] &&
            (Hardware->config->pixelPipes > 1) &&
            (Hardware->PEStates->singlePEpipe))
        {
            _SinglePEPipeSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
        }
        if (Hardware->features[gcvFEATURE_HW_TFB])
        {
            _InternalTFBSwitch(Hardware, gcvFALSE, (gctPOINTER *)&memory);
        }

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER_NEW_FAST(Hardware, reserve, memory, Memory);
    }
    else
    {
        gcmDEFINESTATEBUFFER_NEW_FAST(reserve, memory);

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER_NEW_FAST(Hardware, reserve, memory, Memory);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E05) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW_FAST(stateDelta, reserve, memory, gcvFALSE, 0x0E05, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        if (useOneCore)
        {
            gcoHARDWARE_MultiGPUSync(Hardware, &memory);
            { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(0));
 } };

        }

        /* Program the AQCommandDX8IndexPrimitive.Command data. */
        memory[0] =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

        /* Program the AQCommandDX8IndexPrimitive.Primtype data. */
        memory[1] =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 3:0) - (0 ?
 3:0) + 1))))))) << (0 ?
 3:0))) | (((gctUINT32) ((gctUINT32) (0x4) & ((gctUINT32) ((((1 ?
 3:0) - (0 ?
 3:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)));

        /* Program the AQCommandDX8IndexPrimitive.Start data. */
        memory[2] = 0;

        /* Program the AQCommandDX8IndexPrimitive.Primcount data. */
        gcmSAFECASTSIZET(memory[3], primCount);

        /* Program the AQCommandDX8IndexPrimitive.Basevertex data. */
        memory[4] = 0;

        memory += 5;

        if (5 & 1)
        {
            gcmSETFILLER_NEW(
                reserve,
                memory
                );
        }

        if (useOneCore)
        {
            { if (Hardware->config->gpuCoreCount > 1) { *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_ALL_MASK; memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_ALL_MASK);
 } };

            gcoHARDWARE_MultiGPUSync(Hardware, &memory);
        }

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER_NEW_FAST(Hardware, reserve, memory, Memory);
    }

    /* Success */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status */
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoHARDWARE_DrawPattern(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo
    )
{
    gceSTATUS status;

    gctPOINTER cmdBuffer = gcvNULL;

    gcsTEMPCMDBUF tempCMD;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmONERROR(gcoBUFFER_StartTEMPCMDBUF(Hardware->engine[gcvENGINE_RENDER].buffer, Hardware->engine[gcvENGINE_RENDER].queue, &tempCMD));

    cmdBuffer = tempCMD->buffer;

    gcmONERROR(gcoHARDWARE_FastFlushUniforms(Hardware, FastFlushInfo, &cmdBuffer));

    gcmONERROR(gcoHARDWARE_FastFlushStream(Hardware, FastFlushInfo, &cmdBuffer));

    gcmONERROR(gcoHARDWARE_FastProgramIndex(Hardware, FastFlushInfo, &cmdBuffer));

    gcmONERROR(gcoHARDWARE_FastFlushAlpha(Hardware, FastFlushInfo, &cmdBuffer));

    gcmONERROR(gcoHARDWARE_FastFlushDepthCompare(Hardware, FastFlushInfo, &cmdBuffer));

    gcmONERROR(_FastDrawIndexedPrimitive(Hardware, FastFlushInfo, &cmdBuffer));

    tempCMD->currentByteSize =  (gctUINT32)((gctUINT8_PTR)cmdBuffer -
                                (gctUINT8_PTR)tempCMD->buffer);

    gcmONERROR(gcoBUFFER_EndTEMPCMDBUF(Hardware->engine[gcvENGINE_RENDER].buffer, gcvFALSE));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}
#endif

/*******************************************************************************
**
**  gcoHARDWARE_TranslateCommand
**
**  Translate API 2D command to its hardware value.
**
**  INPUT:
**
**      gce2D_COMMAND APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoHARDWARE_TranslateCommand(
    IN gce2D_COMMAND APIValue,
    OUT gctUINT32 * HwValue
    )
{
    gcmHEADER_ARG("APIValue=%d HwValue=0x%x", APIValue, HwValue);

    /* Dispatch on command. */
    switch (APIValue)
    {
    case gcv2D_CLEAR:
        *HwValue = 0x0;
        break;

    case gcv2D_LINE:
        *HwValue = 0x1;
        break;

    case gcv2D_BLT:
        *HwValue = 0x2;
        break;

    case gcv2D_STRETCH:
        *HwValue = 0x4;
        break;

    case gcv2D_HOR_FILTER:
        *HwValue = 0x5;
        break;

    case gcv2D_VER_FILTER:
        *HwValue = 0x6;
        break;

    case gcv2D_MULTI_SOURCE_BLT:
        *HwValue = 0x8;
        break;

    default:
        /* Not supported. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_NOT_SUPPORTED);
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    gcmFOOTER_ARG("*HwValue=%d", *HwValue);
    return gcvSTATUS_OK;
}


