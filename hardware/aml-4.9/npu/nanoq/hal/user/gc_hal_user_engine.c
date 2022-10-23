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


/*
 * gco3D object for user HAL layers.
 */

#include "gc_hal_user_precomp.h"

#if gcdENABLE_3D

#if gcdNULL_DRIVER < 2

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_3D

/******************************************************************************\
********************************* gcoCL API Code ********************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoCLHardware_Construct
**
**  Initilize CL related hardware.
**
**  INPUT:
**
**
**  OUTPUT:
**
**
*/
gceSTATUS
gcoCLHardware_Construct(
    void
    )
{
    gceSTATUS status;
    gcmHEADER();

    /* Initialize CL hardware. */
    gcmONERROR(gcoHARDWARE_InitializeCL(gcvNULL));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/******************************************************************************\
********************************* gco3D API Code ********************************
\******************************************************************************/

/*******************************************************************************
**
**  gco3D_Construct
**
**  Contruct a new gco3D object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctBOOL Robust
**          Engine is robust or not.
**
**  OUTPUT:
**
**      gco3D * Engine
**          Pointer to a variable receiving the gco3D object pointer.
*/
gceSTATUS
gco3D_Construct(
    IN gcoHAL Hal,
    IN gctBOOL Robust,
    OUT gco3D * Engine
    )
{
    gco3D engine = gcvNULL;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;
    gctUINT32  coreIndex;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

    /* Allocate the gco3D object. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(struct _gco3D),
                       &pointer));

    engine = pointer;

    /* Zero out entire object. */
    gcoOS_ZeroMemory(engine, gcmSIZEOF(struct _gco3D));

    /* Initialize the gco3D object. */
    engine->object.type = gcvOBJ_3D;

    /* Mark everything as dirty. */
    engine->clearColorDirty   = gcvTRUE;
    engine->clearDepthDirty   = gcvTRUE;
    engine->clearStencilDirty = gcvTRUE;

    /* Set default API type. */
    engine->apiType = gcvAPI_OPENGL;

    /* Set MRT tile status buffer support flag */
    engine->mRTtileStatus = gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_MRT_TILE_STATUS_BUFFER);

    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

    /* Construct hardware object for this engine */
    gcmONERROR(gcoHARDWARE_Construct(Hal, gcvFALSE, Robust, &engine->hardware));

    /* ES construct hardware will query VIV_MGPU_AFFINITY
    and we need  keep the TLS->currentCoreIndex same with VIV_MGPU_AFFINITY attri
    */
    gcmONERROR(gcoHARDWARE_QueryCoreIndex(engine->hardware, 0, &coreIndex));
    gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));
    gcmONERROR(gcoHARDWARE_SelectPipe(engine->hardware, gcvPIPE_3D, gcvNULL));
    gcmONERROR(gcoHARDWARE_InvalidateCache(engine->hardware));
    /* Initialize 3D hardware. */
    gcmONERROR(gcoHARDWARE_Initialize3D(engine->hardware));

    /* Return pointer to the gco3D object. */
    *Engine = engine;

    /* Success. */
    gcmFOOTER_ARG("*Engine=0x%x", *Engine);
    return gcvSTATUS_OK;

OnError:
    if (engine != gcvNULL)
    {
        /* Unroll. */
        if (engine->hardware)
        {
            gcoHARDWARE_Destroy(engine->hardware, gcvFALSE);
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, engine));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_Destroy
**
**  Destroy an gco3D object.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_Destroy(
    IN gco3D Engine
    )
{
    gctINT index;
    gcsTLS_PTR tls;
    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Mark the gco3D object as unknown. */
    Engine->object.type = gcvOBJ_UNKNOWN;

    /* Destroy referenced render target. */
    for (index = 0; index < 4; index++)
    {
        if (Engine->mRT[index] != gcvNULL)
        {
            gcmVERIFY_OK(
                gcoSURF_Unlock(Engine->mRT[index],
                               Engine->mRTMemory[index]));

            gcmVERIFY_OK(gcoSURF_Destroy(Engine->mRT[index]));
        }
    }

    /* Destroy referenced render target. */
    if (Engine->depth != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(Engine->depth, Engine->depthMemory));

        gcmVERIFY_OK(gcoSURF_Destroy(Engine->depth));
    }

    /* Free Hardware Object */
    gcmVERIFY_OK(gcoOS_GetTLS(&tls));
    gcmVERIFY_OK(gcoHARDWARE_Destroy(Engine->hardware, gcvFALSE));
    if (Engine->hardware == tls->defaultHardware)
    {
        tls->defaultHardware = gcvNULL;
    }
    if (Engine->hardware == tls->currentHardware)
    {
        tls->currentHardware    = gcvNULL;
    }

    /* Free the gco3D object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Engine));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetAPI
**
**  Set 3D API type.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gco3D * Engine
**          Pointer to a variable receiving the gco3D object pointer.
*/
gceSTATUS
gco3D_SetAPI(
    IN gco3D Engine,
    IN gceAPI ApiType
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x ApiType=%d", Engine, ApiType);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the API. */
    Engine->apiType = ApiType;

    /* Propagate to hardware. */
    status = gcoHARDWARE_SetAPI(Engine->hardware, ApiType);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_GetAPI
**
**  Get 3D API type.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gceAPI * ApiType
**          Pointer to a API type.
*/
gceSTATUS gco3D_GetAPI(
    IN gco3D Engine,
    OUT gceAPI * ApiType
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Get the API. */
    *ApiType = Engine->apiType;

    gcmFOOTER();
    return status;
}


gceSTATUS
gco3D_SetPSOutputMapping(
    IN gco3D Engine,
    IN gctINT32 * psOutputMapping
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x psOutputMapping=%d", Engine, psOutputMapping);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    gcmONERROR(
       gcoHARDWARE_SetPSOutputMapping(Engine->hardware, psOutputMapping));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetRenderLayered(
    IN gco3D Engine,
    IN gctBOOL Enable,
    IN gctUINT MaxLayers
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d MaxLayers=0x%x", Engine, Enable, MaxLayers);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetRenderLayered(Engine->hardware, Enable, MaxLayers);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_SetShaderLayered(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetShaderLayered(Engine->hardware, Enable);

    gcmFOOTER();

    return status;
}


gceSTATUS
gco3D_IsProgramSwitched(
    IN gco3D Engine
    )
{

    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_IsProgramSwitched(Engine->hardware);

    gcmFOOTER();

    return status;

}
/*******************************************************************************
**
**  gco3D_SetTarget
**
**  Unified API to set the current render target used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT32 TargetIndex
**          Base from 0
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object that becomes the new render target or
**          gcvNULL to delete the current render target.
**
**      gcoSURF SliceIndex
**          Which Slice of the Target will be used
**
**      gcoSURF LayerIndex
**          Which layer of the Target will be used
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gco3D_SetTarget(
    IN gco3D Engine,
    IN gctUINT32 TargetIndex,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 LayerIndex
    )
{
    gceSTATUS status;
    gcoSURF prevRT = gcvNULL;
    gcoSURF Surface = SurfView ? SurfView->surf : gcvNULL;
    gctUINT SliceIndex = SurfView ? SurfView->firstSlice : 0;
    gcsSURF_VIEW preView = {gcvNULL, 0, 1};

    gcmHEADER_ARG("Engine=0x%x TargetIndex=%d SurfView=0x%x LayerIndex=%d",
                   Engine, TargetIndex, SurfView, LayerIndex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Surface != gcvNULL)
    {
        gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    }

    gcmASSERT(TargetIndex < gcdMAX_DRAW_BUFFERS);

    /* Only process if different than current target. */
    if (Engine->mRT[TargetIndex] != Surface || Engine->mRTSliceOffset[TargetIndex] != SliceIndex)
    {
        /* Verify linear render target. */
        if ((Surface != gcvNULL)
        &&  (Surface->tiling == gcvLINEAR)
        &&  (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_LINEAR_RENDER_TARGET)
                != gcvSTATUS_TRUE)
        )
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        /* Verify if target is properly aligned. */
        if ((Surface != gcvNULL)
        &&  Surface->resolvable
        &&  ( (Surface->alignedW & 15) || (Surface->alignedH & 3))
        )
        {
            gcmONERROR(gcvSTATUS_NOT_ALIGNED);
        }

        /* Unset previous target if any. */
        if (Engine->mRT[TargetIndex] != gcvNULL)
        {
            prevRT = Engine->mRT[TargetIndex];

            preView.surf = prevRT;
            preView.firstSlice = Engine->mRTSliceOffset[TargetIndex];
            preView.numSlices = Engine->mRTSliceNum[TargetIndex];

            /* Disable tile status for this surface with color slot0 */
            gcmONERROR(gcoSURF_DisableTileStatus(&preView, gcvFALSE));

            /* Disable tile status setting for MRT */
            if (Engine->mRTtileStatus && (TargetIndex != 0))
            {
                gcmONERROR(
                    gcoHARDWARE_DisableHardwareTileStatus(Engine->hardware,
                                                          gcvTILESTATUS_COLOR,
                                                          TargetIndex));
            }
            /* Unlock the current render target. */
            gcmVERIFY_OK(gcoSURF_Unlock(prevRT, Engine->mRTMemory[TargetIndex]));

            /* Reset mapped memory pointer. */
            Engine->mRTMemory[TargetIndex] = gcvNULL;
        }

        /* Set new target. */
        if (Surface == gcvNULL)
        {
            /* Reset current target. */
            Engine->mRT[TargetIndex] = gcvNULL;
            Engine->mRTSliceOffset[TargetIndex] = 0;
            Engine->mRTSliceNum[TargetIndex] = 1;

            /* Invalidate target. */
            gcmONERROR(gcoHARDWARE_SetRenderTarget(Engine->hardware, TargetIndex, gcvNULL, 0, 1, 0));
            if (prevRT)
            {
                /* Dereference the surface. */
                gcmONERROR(gcoSURF_Destroy(prevRT));
            }
        }
        else
        {
            gctPOINTER targetMemory[3] = {gcvNULL};

            /* Set new render target. */
            Engine->mRT[TargetIndex] = Surface;
            Engine->mRTSliceOffset[TargetIndex] = SliceIndex;
            Engine->mRTSliceNum[TargetIndex] = SurfView->numSlices;

            /* Lock the surface. */
            gcmONERROR(gcoSURF_Lock(Surface, gcvNULL, targetMemory));

            Engine->mRTMemory[TargetIndex] = targetMemory[0];

            /* Program render target. */
            gcmONERROR(gcoHARDWARE_SetRenderTarget(Engine->hardware, TargetIndex, Surface, SliceIndex, SurfView->numSlices, LayerIndex));

            /* Reference the surface. */
            gcmONERROR(gcoSURF_ReferenceSurface(Surface));
            if (prevRT)
            {
                /* Dereference the surface. */
                gcmONERROR(gcoSURF_Destroy(prevRT));
            }
         }

         if (Engine->mRTtileStatus && Surface)
         {
            gcmONERROR(gcoSURF_EnableTileStatusEx(SurfView, TargetIndex));
         }
         else
         {
             if ((TargetIndex == 0) && Surface)
             {
                 /* Enable tile status. */
                 gcmONERROR(gcoSURF_EnableTileStatus(SurfView));
             }
         }

         /* Sync between GPUs.*/
         gcmONERROR(gcoHARDWARE_MultiGPUSync(gcvNULL, gcvNULL));
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
**
**  gco3D_UnsetTargetEx
**
**  Unified API which try to unset render target
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT32 TargetIndex
**          Base from 0
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object that will be unset if it's a current
**          render target
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_UnsetTarget(
    IN gco3D Engine,
    IN gctUINT32 TargetIndex,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x TargetIndex=%d Surface=0x%x ", Engine, TargetIndex, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    gcmASSERT(TargetIndex < gcdMAX_DRAW_BUFFERS);

    /* Only unset surface if it is the current render target. */
    if (Engine->mRT[TargetIndex] == Surface)
    {
        gcmONERROR(gco3D_SetTarget(Engine, TargetIndex, gcvNULL, 0));
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
**
**  gco3D_SetDepth
**
**  Set the current depth buffer used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object that becomes the new depth buffer or
**          gcvNULL to delete the current depth buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepth(
    IN gco3D Engine,
    IN gcsSURF_VIEW *SurfView
    )
{
    gceSTATUS status;
    gcoSURF Surface = SurfView ? SurfView->surf : gcvNULL;
    gctUINT SliceIndex = SurfView ? SurfView->firstSlice : 0;

    gcmHEADER_ARG("Engine=0x%x SurfView=0x%x ", Engine, SurfView);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Surface != gcvNULL)
    {
        gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    }

    /* Only process if different than current depth buffer. */
    if (Engine->depth != Surface || Engine->depthSliceOffset != SliceIndex)
    {
        /* Verify if depth buffer is properly aligned. */
        if ((Surface != gcvNULL)
        &&  Surface->resolvable
        &&  ((Surface->alignedW & 15) || (Surface->alignedH & 3) )
        )
        {
            gcmONERROR(gcvSTATUS_NOT_ALIGNED);
        }

        /* Unset previous depth buffer if any. */
        if (Engine->depth != gcvNULL)
        {
            gcoSURF prevDepth = Engine->depth;
            gcsSURF_VIEW prevView = {prevDepth, Engine->depthSliceOffset, Engine->depthSliceNum};

            /* Disable previous tile status. */
            gcmONERROR(gcoSURF_DisableTileStatus(&prevView, gcvFALSE));

            /* Unlock the current depth surface. */
            gcmONERROR(gcoSURF_Unlock(prevDepth, Engine->depthMemory));

            /* Reset mapped memory pointer. */
            Engine->depthMemory = gcvNULL;

            /* Set depth to NULL to avoid recursive destroying. */
            Engine->depth = gcvNULL;

            /* Dereference the surface. */
            gcmONERROR(gcoSURF_Destroy(prevDepth));
        }

        /* Set new depth buffer. */
        if (Surface == gcvNULL)
        {
            /* Reset current depth buffer. */
            Engine->depth = gcvNULL;
            Engine->depthSliceOffset = 0;
            Engine->depthSliceNum = 1;

            /* Disable depth. */
            gcmONERROR(gcoHARDWARE_SetDepthBuffer(Engine->hardware, gcvNULL, 0, 1));
        }
        else
        {
            gctPOINTER depthMemory[3] = {gcvNULL};

            /* Set new depth buffer. */
            Engine->depth = Surface;
            Engine->depthSliceOffset = SliceIndex;
            Engine->depthSliceNum = SurfView->numSlices;

            /* Lock depth buffer. */
            gcmONERROR(gcoSURF_Lock(Surface, gcvNULL, depthMemory));
            Engine->depthMemory = depthMemory[0];

            /* Set depth buffer. */
            gcmONERROR(gcoHARDWARE_SetDepthBuffer(Engine->hardware, Surface, SliceIndex, SurfView->numSlices));

            /* Enable tile status. */
            gcmONERROR(gcoSURF_EnableTileStatus(SurfView));

            /* Reference the surface. */
            gcmONERROR(gcoSURF_ReferenceSurface(Surface));
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

gceSTATUS
gco3D_UnsetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Surface=0x%x", Engine, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Only unset surface if it is the current depth buffer. */
    if (Engine->depth == Surface)
    {
        gcmONERROR(gco3D_SetDepth(Engine, gcvNULL));
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
**
**  gco3D_SetViewport
**
**  Set the current viewport used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctINT32 Left
**          Left coordinate of viewport.
**
**      gctINT32 Top
**          Top coordinate of viewport.
**
**      gctINT32 Right
**          Right coordinate of viewport.
**
**      gctINT32 Bottom
**          Bottom coordinate of viewport.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetViewport(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Left=%d Top=%d Right=%d Bottom=%d",
                  Engine, Left, Top, Right, Bottom);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program viewport. */
    status = gcoHARDWARE_SetViewport(Engine->hardware, Left, Top, Right, Bottom);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetScissors
**
**  Set the current scissors used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctINT32 Left
**          Left coordinate of scissors.
**
**      gctINT32 Top
**          Top coordinate of scissors.
**
**      gctINT32 Right
**          Right coordinate of scissors.
**
**      gctINT32 Bottom
**          Bottom coordinate of scissors.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetScissors(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Left=%d Top=%d Right=%d Bottom=%d",
                  Engine, Left, Top, Right, Bottom);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program scissors. */
    status = gcoHARDWARE_SetScissors(Engine->hardware, Left, Top, Right, Bottom);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetClearColor
**
**  Set the current clear color used for future primitives.  The clear color is
**  specified in unsigned integers between 0 and 255.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Red
**          Clear color for red channel.
**
**      gctUINT8 Green
**          Clear color for green channel.
**
**      gctUINT8 Blue
**          Clear color for blue channel.
**
**      gctUINT8 Alpha
**          Clear color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearColor(
    IN gco3D Engine,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    )
{
    gcmHEADER_ARG("Engine=0x%x Red=%u Green=%u Blue=%u Alpha=%u",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear color. */
    if ((Engine->clearColorType            != gcvVALUE_UINT)
    ||  (Engine->clearColorRed.uintValue   != Red)
    ||  (Engine->clearColorGreen.uintValue != Green)
    ||  (Engine->clearColorBlue.uintValue  != Blue)
    ||  (Engine->clearColorAlpha.uintValue != Alpha)
    )
    {
        /* Set the new clear colors.    Saturate between 0 and 255. */
        Engine->clearColorDirty           = gcvTRUE;
        Engine->clearColorType            = gcvVALUE_UINT;
        Engine->clearColorRed.uintValue   = Red;
        Engine->clearColorGreen.uintValue = Green;
        Engine->clearColorBlue.uintValue  = Blue;
        Engine->clearColorAlpha.uintValue = Alpha;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearColorX
**
**  Set the current clear color used for future primitives.  The clear color is
**  specified in fixed point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Red
**          Clear color for red channel.
**
**      gctFIXED_POINT Green
**          Clear color for green channel.
**
**      gctFIXED_POINT Blue
**          Clear color for blue channel.
**
**      gctFIXED_POINT Alpha
**          Clear color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear color. */
    if ((Engine->clearColorType             != gcvVALUE_FIXED)
    ||  (Engine->clearColorRed.fixedValue   != Red)
    ||  (Engine->clearColorGreen.fixedValue != Green)
    ||  (Engine->clearColorBlue.fixedValue  != Blue)
    ||  (Engine->clearColorAlpha.fixedValue != Alpha)
    )
    {
        /* Set the new clear color.  Saturate between 0.0 and 1.0. */
        Engine->clearColorDirty            = gcvTRUE;
        Engine->clearColorType             = gcvVALUE_FIXED;
        Engine->clearColorRed.fixedValue   = gcmCLAMP(Red,   0, gcvONE_X);
        Engine->clearColorGreen.fixedValue = gcmCLAMP(Green, 0, gcvONE_X);
        Engine->clearColorBlue.fixedValue  = gcmCLAMP(Blue,  0, gcvONE_X);
        Engine->clearColorAlpha.fixedValue = gcmCLAMP(Alpha, 0, gcvONE_X);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearColorF
**
**  Set the current clear color used for future primitives.  The clear color is
**  specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Red
**          Clear color for red channel.
**
**      gctFLOAT Green
**          Clear color for green channel.
**
**      gctFLOAT Blue
**          Clear color for blue channel.
**
**      gctFLOAT Alpha
**          Clear color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear color. */
    if ((Engine->clearColorType             != gcvVALUE_FLOAT)
    ||  (Engine->clearColorRed.floatValue   != Red)
    ||  (Engine->clearColorGreen.floatValue != Green)
    ||  (Engine->clearColorBlue.floatValue  != Blue)
    ||  (Engine->clearColorAlpha.floatValue != Alpha)
    )
    {
        /* Set the new clear color.  Saturate between 0.0 and 1.0. */
        Engine->clearColorDirty            = gcvTRUE;
        Engine->clearColorType             = gcvVALUE_FLOAT;
        Engine->clearColorRed.floatValue   = gcmCLAMP(Red,   0.0f, 1.0f);
        Engine->clearColorGreen.floatValue = gcmCLAMP(Green, 0.0f, 1.0f);
        Engine->clearColorBlue.floatValue  = gcmCLAMP(Blue,  0.0f, 1.0f);
        Engine->clearColorAlpha.floatValue = gcmCLAMP(Alpha, 0.0f, 1.0f);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearDepthX
**
**  Set the current clear depth value used for future primitives.  The clear
**  depth value is specified in fixed point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Depth
**          Clear depth value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearDepthX(
    IN gco3D Engine,
    IN gctFIXED_POINT Depth
    )
{
    gcmHEADER_ARG("Engine=0x%x Depth=%f", Engine, gcoMATH_Fixed2Float(Depth));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear depth value. */
    if ((Engine->clearDepthType        != gcvVALUE_FIXED)
    ||  (Engine->clearDepth.fixedValue != Depth)
    )
    {
        /* Set the new clear depth value.  Saturate between 0.0 and 1.0. */
        Engine->clearDepthDirty       = gcvTRUE;
        Engine->clearDepthType        = gcvVALUE_FIXED;
        Engine->clearDepth.fixedValue = gcmCLAMP(Depth, 0, gcvONE_X);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearDepthF
**
**  Set the current clear depth value used for future primitives.  The clear
**  depth value is specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Depth
**          Clear depth value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearDepthF(
    IN gco3D Engine,
    IN gctFLOAT Depth
    )
{
    gcmHEADER_ARG("Engine=0x%x Depth=%f", Engine, Depth);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear depth value. */
    if ((Engine->clearDepthType != gcvVALUE_FLOAT)
    ||  (Engine->clearDepth.floatValue != Depth)
    )
    {
        /* Set the new clear depth value.  Saturate between 0.0 and 1.0. */
        Engine->clearDepthDirty       = gcvTRUE;
        Engine->clearDepthType        = gcvVALUE_FLOAT;
        Engine->clearDepth.floatValue = gcmCLAMP(Depth, 0.0f, 1.0f);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearStencil
**
**  Set the current clear stencil value used for future primitives.  The clear
**  stencil value is specified in unsigned integer.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT32 Stencil
**          Clear stencil value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearStencil(
    IN gco3D Engine,
    IN gctUINT32 Stencil
    )
{
    gcmHEADER_ARG("Engine=0x%x Stencil=%u", Engine, Stencil);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear stencil value. */
    if (Engine->clearStencil != Stencil)
    {
        /* Set te new clear stencil value.  Mask with 0xFF. */
        Engine->clearStencilDirty = gcvTRUE;
        Engine->clearStencil      = Stencil & 0xFF;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetShading
**
**  Set the current shading technique used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSHADING Shading
**          Shading technique to select.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetShading(
    IN gco3D Engine,
    IN gceSHADING Shading
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Shading=%d", Engine, Shading);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program shading. */
    status = gcoHARDWARE_SetShading(Engine->hardware, Shading);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableBlending
**
**  Enable or disable blending.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**
**      gctBOOL Enable
**          gcvTRUE to enable blending, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableBlending(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    do
    {
        gctUINT i;
        gctUINT maxRTs;

        gcmERR_BREAK(gcoHAL_QueryTargetCaps(gcvNULL, gcvNULL, gcvNULL, &maxRTs, gcvNULL));

        for (i = 0; i < maxRTs; ++i)
        {
            /* Program color write enable bits. */
            status = gcoHARDWARE_SetBlendEnable(Engine->hardware, i, Enable);
        }
    } while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendFunction
**
**  Set blending function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceBLEND_UNIT Unit
**          Unit to set blending function for.  Can be one of the following
**          values:
**
**              gcvBLEND_SOURCE - Set source blending function.
**              gcvBLEND_TARGET - Set target blending function.
**
**      gceBLEND_FUNCTION FunctionRGB
**          Blending function for RGB channels.
**
**      gceBLEND_FUNCTION FunctionAlpha
**          Blending function for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendFunction(
    IN gco3D Engine,
    IN gceBLEND_UNIT Unit,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x Unit=%d FunctionRGB=%d FunctionAlpha=%d",
                  Engine, Unit, FunctionRGB, FunctionAlpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    do
    {
        gctUINT i;
        gctUINT maxRTs;

        gcmERR_BREAK(gcoHAL_QueryTargetCaps(gcvNULL, gcvNULL, gcvNULL, &maxRTs, gcvNULL));

        for (i = 0; i < maxRTs; ++i)
        {
            if (Unit == gcvBLEND_SOURCE)
            {
                /* Program source blend function. */
                status = gcoHARDWARE_SetBlendFunctionSource(Engine->hardware,
                                                            i,
                                                            FunctionRGB,
                                                            FunctionAlpha);
            }
            else
            {
                /* Program target blend function. */
                status = gcoHARDWARE_SetBlendFunctionTarget(Engine->hardware,
                                                            i,
                                                            FunctionRGB,
                                                            FunctionAlpha);
            }
        }
    } while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendMode
**
**  Set blending mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcvBLEND_Mode ModeRGB
**          Blending mode for RGB channels.
**
**      gceBLEND_MODE ModeAlpha
**          Blending mode for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendMode(
    IN gco3D Engine,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x ModeRGB=%d ModeAlpha=%d",
                  Engine, ModeRGB, ModeAlpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    do
    {
        gctUINT i;
        gctUINT maxRTs;

        gcmERR_BREAK(gcoHAL_QueryTargetCaps(gcvNULL, gcvNULL, gcvNULL, &maxRTs, gcvNULL));

        for (i = 0; i < maxRTs; ++i)
        {
            /* Program color write enable bits. */
            status = gcoHARDWARE_SetBlendMode(Engine->hardware,
                                              i,
                                              ModeRGB,
                                              ModeAlpha);
        }
    } while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableBlendingIndexed
**
**  Enable or disable blending.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT  Index
**          Buffer index for separate buffer control
**
**      gctBOOL Enable
**          gcvTRUE to enable blending, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableBlendingIndexed(
    IN gco3D Engine,
    IN gctUINT Index,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Index=%d Enable=%d", Engine, Index, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending. */
    status = gcoHARDWARE_SetBlendEnable(Engine->hardware, Index, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendFunctionIndexed
**
**  Set blending function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT  Index
**          Buffer index for separate buffer control
**
**      gceBLEND_UNIT Unit
**          Unit to set blending function for.  Can be one of the following
**          values:
**
**              gcvBLEND_SOURCE - Set source blending function.
**              gcvBLEND_TARGET - Set target blending function.
**
**      gceBLEND_FUNCTION FunctionRGB
**          Blending function for RGB channels.
**
**      gceBLEND_FUNCTION FunctionAlpha
**          Blending function for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendFunctionIndexed(
    IN gco3D Engine,
    IN gctUINT Index,
    IN gceBLEND_UNIT Unit,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Index=%d Unit=%d FunctionRGB=%d FunctionAlpha=%d",
                  Engine, Index, Unit, FunctionRGB, FunctionAlpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Unit == gcvBLEND_SOURCE)
    {
        /* Program source blend function. */
        status = gcoHARDWARE_SetBlendFunctionSource(Engine->hardware,
                                                    Index,
                                                    FunctionRGB,
                                                    FunctionAlpha);
    }
    else
    {
        /* Program target blend function. */
        status = gcoHARDWARE_SetBlendFunctionTarget(Engine->hardware,
                                                    Index,
                                                    FunctionRGB,
                                                    FunctionAlpha);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendModeIndexed
**
**  Set blending mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT  Index
**          Buffer index for separate buffer control
**
**      gcvBLEND_Mode ModeRGB
**          Blending mode for RGB channels.
**
**      gceBLEND_MODE ModeAlpha
**          Blending mode for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendModeIndexed(
    IN gco3D Engine,
    IN gctUINT  Index,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Index=%d ModeRGB=%d ModeAlpha=%d",
                  Engine, Index, ModeRGB, ModeAlpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending. */
    status = gcoHARDWARE_SetBlendMode(Engine->hardware,
                                      Index,
                                      ModeRGB,
                                      ModeAlpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendColor
**
**  Set the current blending color.  The blending color is specified in unsigned
**  integers between 0 and 255.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT Red
**          Blending color for red channel.
**
**      gctUINT Green
**          Blending color for green channel.
**
**      gctUINT Blue
**          Blending color for blue channel.
**
**      gctUINT Alpha
**          Blending color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendColor(
    IN gco3D Engine,
    IN gctUINT Red,
    IN gctUINT Green,
    IN gctUINT Blue,
    IN gctUINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%u Green=%u Blue=%u Alpha=%u",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending color.  Clamp between 0 and 255. */
    status = gcoHARDWARE_SetBlendColor(Engine->hardware,
                                       (gctUINT8) gcmMIN(Red,   255),
                                       (gctUINT8) gcmMIN(Green, 255),
                                       (gctUINT8) gcmMIN(Blue,  255),
                                       (gctUINT8) gcmMIN(Alpha, 255));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendColorX
**
**  Set the current blending color.  The blending color is specified in fixed
**  point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Red
**          Blending color for red channel.
**
**      gctFIXED_POINT Green
**          Blending color for green channel.
**
**      gctFIXED_POINT Blue
**          Blending color for blue channel.
**
**      gctFIXED_POINT Alpha
**          Blending color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending color.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetBlendColorX(Engine->hardware,
                                        gcmCLAMP(Red,   0, gcvONE_X),
                                        gcmCLAMP(Green, 0, gcvONE_X),
                                        gcmCLAMP(Blue,  0, gcvONE_X),
                                        gcmCLAMP(Alpha, 0, gcvONE_X));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendColorF
**
**  Set the current blending color.  The blending color is specified in floating
**  point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Red
**          Blending color for red channel.
**
**      gctFLOAT Green
**          Blending color for green channel.
**
**      gctFLOAT Blue
**          Blending color for blue channel.
**
**      gctFLOAT Alpha
**          Blending color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending color.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetBlendColorF(Engine->hardware,
                                        gcmCLAMP(Red,   0.0f, 1.0f),
                                        gcmCLAMP(Green, 0.0f, 1.0f),
                                        gcmCLAMP(Blue,  0.0f, 1.0f),
                                        gcmCLAMP(Alpha, 0.0f, 1.0f));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetCulling
**
**  Set culling mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceCULL Mode
**          Culling mode.  Can be one of the following values:
**
**              gcvCULL_NONE - Don't cull any triangle.
**              gcvCULL_CCW  - Cull counter clock-wise triangles.
**              gcvCULL_CW   - Cull clock-wise triangles.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetCulling(
    IN gco3D Engine,
    IN gceCULL Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the culling mode. */
    status = gcoHARDWARE_SetCulling(Engine->hardware, Mode);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetPointSizeEnable
**
**  Enable/Disable point size.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable point sprite, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetPointSizeEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the point size enable/disable. */
    status = gcoHARDWARE_SetPointSizeEnable(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetPointSprite
**
**  Set point sprite.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable point sprite, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetPointSprite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the point sprite enable/disable. */
    status = gcoHARDWARE_SetPointSprite(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetPrimitiveIdEnable
**
**  Enable/Disable primitive-id.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable primitive-id use, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetPrimitiveIdEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the primitive-id enable/disable. */
    status = gcoHARDWARE_SetPrimitiveIdEnable(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFill
**
**  Set fill mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceFILL Mode
**          Fill mode.  Can be one of the following values:
**
**              gcvFILL_POINT      - Draw points at each vertex.
**              gcvFILL_WIRE_FRAME - Draw triangle borders.
**              gcvFILL_SOLID      - Fill triangles.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFill(
    IN gco3D Engine,
    IN gceFILL Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the fill mode. */
    status = gcoHARDWARE_SetFill(Engine->hardware, Mode);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthCompare.
**
**  Set the depth compare.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceCOMPARE Compare
**          Depth compare.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Compare=%d", Engine, Compare);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program depth compare. */
    status = gcoHARDWARE_SetDepthCompare(Engine->hardware, Compare);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableDepthWrite.
**
**  Enable or disable writing of depth values.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable writing of depth values, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableDepthWrite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth write. */
    status = gcoHARDWARE_SetDepthWrite(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthMode.
**
**  Set the depth mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceDEPTH_MODE Mode
**          Mode for depth.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthMode(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth mode. */
    status = gcoHARDWARE_SetDepthMode(Engine->hardware, Mode);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthRangeX.
**
**  Set the depth range, specified in fixed point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceDEPTH_MODE Mode
**          Mode for depth.
**
**      gctFIXED_POINT Near
**          Near value for depth specified in fixed point.
**
**      gctFIXED_POINT Far
**          Far value for depth specified in fixed point.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthRangeX(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFIXED_POINT Near,
    IN gctFIXED_POINT Far
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d Near=%f Far=%f",
                  Engine, Mode, gcoMATH_Fixed2Float(Near),
                  gcoMATH_Fixed2Float(Far));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth range.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetDepthRangeX(Engine->hardware,
                                        Mode,
                                        gcmCLAMP(Near, 0, gcvONE_X),
                                        gcmCLAMP(Far,  0, gcvONE_X));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthRangeF.
**
**  Set the depth range, specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceDEPTH_MODE Mode
**          Mode for depth.
**
**      gctFLOAT Near
**          Near value for depth specified in floating point.
**
**      gctFLOAT Far
**          Far value for depth specified in floating point.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthRangeF(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d Near=%f Far=%f",
                  Engine, Mode, Near, Far);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth range.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetDepthRangeF(Engine->hardware,
                                        Mode,
                                        gcmCLAMP(Near, 0.0f, 1.0f),
                                        gcmCLAMP(Far,  0.0f, 1.0f));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthRangeF.
**
**  Set the depth near and far plane, specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**
**      gctFLOAT Near
**          Near plane value for depth specified in floating point.
**
**      gctFLOAT Far
**          Far plane value for depth specified in floating point.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthPlaneF(
    IN gco3D Engine,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Near=%f Far=%f",
                  Engine, Near, Far);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth plane, include near and far plane. */
    status = gcoHARDWARE_SetDepthPlaneF(Engine->hardware, Near, Far);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetLastPixelEnable.
**
**  Set the depth range, specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**      gctBOOL Enable
**          Enable the last pixel for line drawing (always 0 for openGL)
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetLastPixelEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the Last Pixel Enable register */
    status = gcoHARDWARE_SetLastPixelEnable(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthScaleBiasF.
**
**  Set the depth range, depth scale and bias.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT depthScale
**          Depth Scale to additional scale to the Z
**
**      gctFLOAT depthBias
**          Depth Bias addition to the Z
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthScaleBiasF(
    IN gco3D Engine,
    IN gctFLOAT DepthScale,
    IN gctFLOAT DepthBias
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x DepthScale=%f DepthBias=%f",
                  Engine, DepthScale, DepthBias);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth scale and Bias */
    status = gcoHARDWARE_SetDepthScaleBiasF(Engine->hardware,
                                            DepthScale,
                                            DepthBias);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthScaleBiasX.
**
**  Set the depth range, depth scale and bias.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT depthScale
**          Depth Scale to additional scale to the Z
**
**      gctFIXED_POINT depthBias
**          Depth Bias addition to the Z
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthScaleBiasX(
    IN gco3D Engine,
    IN gctFIXED_POINT DepthScale,
    IN gctFIXED_POINT DepthBias
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x DepthScale=%f DepthBias=%f",
                  Engine, gcoMATH_Fixed2Float(DepthScale),
                  gcoMATH_Fixed2Float(DepthBias));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth scale and Bias */
    status = gcoHARDWARE_SetDepthScaleBiasX(Engine->hardware,
                                            DepthScale,
                                            DepthBias);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableDither.
**
**  Enable or disable dithering.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable dithering, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableDither(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program dithering. */
    status = gcoHARDWARE_SetDither(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetColorWrite.
**
**  Set color write enable bits.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Enable
**          Color write enable bits:
**
**              bit 0: writing of blue enabled when set.
**              bit 1: writing of green enabled when set.
**              bit 2: writing of red enabled when set.
**              bit 3: writing of alpha enabled when set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetColorWrite(
    IN gco3D Engine,
    IN gctUINT8 Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    do
    {
        gctUINT i;
        gctUINT maxRTs;

        gcmERR_BREAK(gcoHAL_QueryTargetCaps(gcvNULL, gcvNULL, gcvNULL, &maxRTs, gcvNULL));

        for (i = 0; i < maxRTs; ++i)
        {
            /* Program color write enable bits. */
            status = gcoHARDWARE_SetColorWrite(Engine->hardware, i, Enable);
        }
    } while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetColorWriteIndexed
**
**  Set color write enable bits.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT  Index
**          Buffer index for separate buffer control
**
**      gctUINT8 Enable
**          Color write enable bits:
**
**              bit 0: writing of blue enabled when set.
**              bit 1: writing of green enabled when set.
**              bit 2: writing of red enabled when set.
**              bit 3: writing of alpha enabled when set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetColorWriteIndexed(
    IN gco3D Engine,
    IN gctUINT Index,
    IN gctUINT8 Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Index=%d Enable=%d", Engine, Index, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program color write enable bits. */
    status = gcoHARDWARE_SetColorWrite(Engine->hardware, Index, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gco3D_SetEarlyDepth.
**
**  Enable or disable early depth.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable early depth, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetEarlyDepth(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program early depth. */
    status = gcoHARDWARE_SetEarlyDepth(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAllEarlyDepthModes.
**  Deprecated. Use gco3D_SetAllEarlyDepthModesEx instead.
**
**  Enable or disable all early depth operations.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Disable
**          gcvTRUE to disable early depth operations, gcvFALSE to enable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAllEarlyDepthModes(
    IN gco3D Engine,
    IN gctBOOL Disable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Disable=%d", Engine, Disable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program early depth. */
    /* Disable Early Z and Early Z modify together by default. */
    status = gcoHARDWARE_SetAllEarlyDepthModes(Engine->hardware, Disable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}



gceSTATUS
gco3D_SetEarlyDepthFromAPP(
    IN gco3D Engine,
    IN gctBOOL EarlyDepthFromAPP
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Disable=%d", Engine, EarlyDepthFromAPP);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program RA write depth. */
    status = gcoHARDWARE_SetEarlyDepthFromAPP(Engine->hardware, EarlyDepthFromAPP);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetRADepthWrite.
**
**  Enable or disable RA Depth Write.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Disable
**          gcvTRUE to disable RA Depth Write, gcvFALSE to enable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetRADepthWrite(
    IN gco3D Engine,
    IN gctBOOL Disable,
    IN gctBOOL psReadZ,
    IN gctBOOL psReadW
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Disable=%d", Engine, Disable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program RA write depth. */
    status = gcoHARDWARE_SetRADepthWrite(Engine->hardware, Disable, psReadZ, psReadW);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gco3D_SetPatchVertices.
**
**  Set TCS input patch vertices number.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctINT PatchVertices
**          Number of control points for TCS input
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetPatchVertices(
    IN gco3D Engine,
    IN gctINT PatchVertices
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x PatchVertices=%d", Engine, PatchVertices);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program patch vertices number */
    status = gcoHARDWARE_SetPatchVertices(Engine->hardware, PatchVertices);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gco3D_SwitchDynamicEarlyDepthMode.
**
**  Switch Early Depth Mode
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SwitchDynamicEarlyDepthMode(
    IN gco3D Engine
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Program early depth. */
    status = gcoHARDWARE_SwitchDynamicEarlyDepthMode(Engine->hardware);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DisableDynamicEarlyDepthMode.
**
**  Switch Early Depth Mode
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Disable
**          gcvTRUE to disable dynamic depth, gcvFALSE to enable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DisableDynamicEarlyDepthMode(
    IN gco3D Engine,
    IN gctBOOL Disable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Disable=%d", Engine, Disable);

    /* Program early depth. */
    status = gcoHARDWARE_DisableDynamicEarlyDepthMode(Engine->hardware, Disable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthOnly.
**
**  Enable or disable depth-only mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable depth-only mode, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthOnly(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program depth only. */
    status =  gcoHARDWARE_SetDepthOnly(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilMode.
**
**  Set stencil mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_MODE Mode
**          Stencil mode.  Can be one of the following:
**
**              gcvSTENCIL_NONE        - Disable stencil.
**              gcvSTENCIL_SINGLE_SIDED - Use single-sided stencil.
**              gcvSTENCIL_DOUBLE_SIDED - Use double-sided stencil.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilMode(
    IN gco3D Engine,
    IN gceSTENCIL_MODE Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil mode. */
    status = gcoHARDWARE_SetStencilMode(Engine->hardware, Mode);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilMask.
**
**  Set stencil mask.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Mask
**          Stencil mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mask=0x%x", Engine, Mask);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil mask. */
    status = gcoHARDWARE_SetStencilMask(Engine->hardware, Mask);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilMaskBack.
**
**  Set stencil back face mask .
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Mask
**          Stencil back face mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilMaskBack(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x MaskBack=0x%x", Engine, Mask);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil mask. */
    status = gcoHARDWARE_SetStencilMaskBack(Engine->hardware, Mask);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilWriteMask.
**
**  Set stencil write mask.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Mask
**          Stencil write mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilWriteMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mask=0x%x", Engine, Mask);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil write mask. */
    status = gcoHARDWARE_SetStencilWriteMask(Engine->hardware, Mask);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilWriteMaskBack.
**
**  Set stencil write mask.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Mask
**          Stencil back write mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilWriteMaskBack(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x MaskBack=0x%x", Engine, Mask);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil write mask. */
    status = gcoHARDWARE_SetStencilWriteMaskBack(Engine->hardware, Mask);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilReference.
**
**  Set stencil reference.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Reference
**          Stencil reference.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctBOOL  Front
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%u", Engine, Reference);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil reference. */
    status = gcoHARDWARE_SetStencilReference(Engine->hardware, Reference, Front);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilCompare.
**
**  Set stencil compare for either the front or back stencil.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil compare to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil compare.
**              gcvSTENCIL_BACK  - Set the back stencil compare.
**
**      gceCOMPARE Compare
**          Stencil compare mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilCompare(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Compare=%d", Engine, Where, Compare);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare operation. */
    status = gcoHARDWARE_SetStencilCompare(Engine->hardware, Where, Compare);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilPass.
**
**  Set stencil operation for either the front or back stencil when the stencil
**  compare passes.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil operation to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil operation.
**              gcvSTENCIL_BACK  - Set the back stencil operation.
**
**      gceSTENCIL_OPERATION Operation
**          Stencil operation when stencil compare passes.  Can be one of the
**          following:
**
**              gcvSTENCIL_KEEP               - Keep the current stencil value.
**              gcvSTENCIL_REPLACE            - Replace the stencil value with
**                                              the reference value.
**              gcvSTENCIL_ZERO               - Set the stencil value to zero.
**              gcvSTENCIL_INVERT             - Invert the stencil value.
**              gcvSTENCIL_INCREMENT          - Increment the stencil value.
**              gcvSTENCIL_DECREMENT          - Decrement the stencil value.
**              gcvSTENCIL_INCREMENT_SATURATE - Increment the stencil value, but
**                                              don't overflow.
**              gcvSTENCIL_DECREMENT_SATURATE - Decrement the stncil value, but
**                                              don't underflow.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilPass(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Operation=%d",
                  Engine, Where, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare pass operation. */
    status = gcoHARDWARE_SetStencilPass(Engine->hardware, Where, Operation);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilFail.
**
**  Set stencil operation for either the front or back stencil when the stencil
**  compare fails.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil operation to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil operation.
**              gcvSTENCIL_BACK  - Set the back stencil operation.
**
**      gceSTENCIL_OPERATION Operation
**          Stencil operation when stencil compare fails.  Can be one of the
**          following:
**
**              gcvSTENCIL_KEEP               - Keep the current stencil value.
**              gcvSTENCIL_REPLACE            - Replace the stencil value with
**                                             the reference value.
**              gcvSTENCIL_ZERO               - Set the stencil value to zero.
**              gcvSTENCIL_INVERT             - Invert the stencil value.
**              gcvSTENCIL_INCREMENT          - Increment the stencil value.
**              gcvSTENCIL_DECREMENT          - Decrement the stencil value.
**              gcvSTENCIL_INCREMENT_SATURATE - Increment the stencil value, but
**                                             don't overflow.
**              gcvSTENCIL_DECREMENT_SATURATE - Decrement the stncil value, but
**                                             don't underflow.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Operation=%d",
                  Engine, Where, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare fail operation. */
    status = gcoHARDWARE_SetStencilFail(Engine->hardware, Where, Operation);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilDepthFail.
**
**  Set stencil operation for either the front or back stencil when the depth
**  compare fails.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil operation to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil operation.
**              gcvSTENCIL_BACK  - Set the back stencil operation.
**
**      gceSTENCIL_OPERATION Operation
**          Stencil operation when depth compare fails.  Can be one of the
**          following:
**
**              gcvSTENCIL_KEEP               - Keep the current stencil value.
**              gcvSTENCIL_REPLACE            - Replace the stencil value with
**                                             the reference value.
**              gcvSTENCIL_ZERO               - Set the stencil value to zero.
**              gcvSTENCIL_INVERT             - Invert the stencil value.
**              gcvSTENCIL_INCREMENT          - Increment the stencil value.
**              gcvSTENCIL_DECREMENT          - Decrement the stencil value.
**              gcvSTENCIL_INCREMENT_SATURATE - Increment the stencil value, but
**                                             don't overflow.
**              gcvSTENCIL_DECREMENT_SATURATE - Decrement the stncil value, but
**                                             don't underflow.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilDepthFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Operation=%d",
                  Engine, Where, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare depth fail operation. */
    status = gcoHARDWARE_SetStencilDepthFail(Engine->hardware,
                                             Where,
                                             Operation);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilAll.
**
**  Set all stncil states in one blow.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcsSTENCIL_INFO_PTR Info
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilAll(
    IN gco3D Engine,
    IN gcsSTENCIL_INFO_PTR Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Info=0x%x", Engine, Info);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

    /* Program the stencil compare depth fail operation. */
    status = gcoHARDWARE_SetStencilAll(Engine->hardware, Info);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaTest.
**
**  Enable or disable alpha test.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable alpha test, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaTest(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test. */
    status = gcoHARDWARE_SetAlphaTest(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaCompare.
**
**  Set the alpha test compare mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceCOMPARE Compare
**          Alpha test compare mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Compare=%d", Engine, Compare);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test compare. */
    status = gcoHARDWARE_SetAlphaCompare(Engine->hardware, Compare);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaReference
**
**  Set the alpha reference value.  The alpha reference value is specified as a
**  unsigned integer between 0 and 255.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Reference
**          Alpha reference value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctFLOAT FloatReference
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%u", Engine, Reference);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test reference. */
    status = gcoHARDWARE_SetAlphaReference(Engine->hardware, Reference,FloatReference);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaReferenceX
**
**  Set the alpha reference value.  The alpha reference value is specified as a
**  fixed point between 0 and 1.0.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Reference
**          Alpha reference value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaReferenceX(
    IN gco3D Engine,
    IN gctFIXED_POINT Reference
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%f",
                  Engine, gcoMATH_Fixed2Float(Reference));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test reference. */
    status = gcoHARDWARE_SetAlphaReferenceX(Engine->hardware, Reference);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaReferenceF
**
**  Set the alpha reference value.  The alpha reference value is specified as a
**  floating point between 0 and 1.0.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Reference
**          Alpha reference value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaReferenceF(
    IN gco3D Engine,
    IN gctFLOAT Reference
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%f", Engine, Reference);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test reference. */
    status = gcoHARDWARE_SetAlphaReferenceF(Engine->hardware, Reference);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if gcdALPHA_KILL_IN_SHADER
/*******************************************************************************
**
**  gco3D_SetAlphaKill
**
**  Set the alphaKill and colorKill state which will used in shader.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL AlphaKill
**          AlphaKill state value.
**
**      gctBOOL ColorKill
**          ColorKill state value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaKill(
    IN gco3D Engine,
    IN gctBOOL AlphaKill,
    IN gctBOOL ColorKill
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x AlphaKill=%d ColorKill=%d", Engine, AlphaKill, ColorKill);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test. */
    status = gcoHARDWARE_SetAlphaKill(Engine->hardware, AlphaKill, ColorKill);

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

/*******************************************************************************
**
**  gco3D_SetAntiAliasLine.
**
**  Enable or disable anti-alias line.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable anti-alias line, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAntiAliasLine(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program anti-alias line. */
    status = gcoHARDWARE_SetAntiAliasLine(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAALineWidth.
**
**  Set anti-alias line width scale.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Width
**          Anti-alias line width.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAALineWidth(
    IN gco3D Engine,
    IN gctFLOAT Width
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Width=%f", Engine, Width);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program anti-alias line width. */
    status = gcoHARDWARE_SetAALineWidth(Engine->hardware, Width);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAALineTexSlot.
**
**  Set anti-alias line texture slot.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctINT32 TexSlot
**          Anti-alias line texture slot number.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAALineTexSlot(
    IN gco3D Engine,
    IN gctUINT TexSlot
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x TexSlot=%u", Engine, TexSlot);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program anti-alias line width. */
    status = gcoHARDWARE_SetAALineTexSlot(Engine->hardware, TexSlot);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawPrimitives
**
**  Draw a number of primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
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
gco3D_DrawPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctSIZE_T StartVertex,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;
    gctINT startVertex;

    gcmHEADER_ARG("Engine=0x%x Type=%d StartVertex=%u PrimitiveCount=%lu",
                  Engine, Type, StartVertex, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    gcmSAFECASTSIZET(startVertex, StartVertex);
    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawPrimitives(Engine->hardware,
                                        Type,
                                        startVertex,
                                        PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawIndirectPrimitives
**
**  Draw a number of primitives through indirect parameter buffer.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
**      gctBOOL DrawIndex
**          Whether or not to draw index mode.
**
**      gctINT BaseOffset
**          Base offset which will be added to VBO that contains the draw parameters.
**
**      gcoBUFOBJ BufObj
**          Pointer to VBO that contains the draw parameters.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DrawIndirectPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctBOOL DrawIndex,
    IN gctINT BaseOffset,
    IN gcoBUFOBJ BufObj
    )
{
    gceSTATUS status = 0;
    gctUINT address = 0;

    gcmHEADER_ARG("Engine=0x%x Type=%d DrawIndex=%d BaseOffset=%d BufObj=%lu",
                  Engine, Type, DrawIndex, BaseOffset, BufObj);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    gcmONERROR(gcoBUFOBJ_Lock(BufObj, &address, gcvNULL));
    address += BaseOffset;

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawIndirectPrimitives(Engine->hardware,
                                                DrawIndex,
                                                Type,
                                                address);

OnError:
    /* Return the status. */
    gcmFOOTER();

    return status;
}

/*******************************************************************************
**
**  gco3D_MultiDrawIndirectPrimitives
**
**  Draw a number of primitives through indirect parameter buffer.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
**      gctBOOL DrawIndex
**          Whether or not to draw index mode.
**
**      gctINT BaseOffset
**          Base offset which will be added to VBO that contains the draw parameters.
**
**      gctINT DrawCount
**          An array of <drawcount> DrawArraysIndirectCommand structures.
**
**      gctINT Stride
**          Specifies the distance, in basic machine units, between the elements of the array.
**
**      gcoBUFOBJ BufObj
**          Pointer to VBO that contains the draw parameters.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_MultiDrawIndirectPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctBOOL DrawIndex,
    IN gctINT BaseOffset,
    IN gctINT DrawCount,
    IN gctINT Stride,
    IN gcoBUFOBJ BufObj
    )
{
    gceSTATUS status = 0;
    gctUINT address = 0;

    gcmHEADER_ARG("Engine=0x%x Type=%d DrawIndex=%d BaseOffset=%d DrawCount=%d Stride=%d BufObj=%lu",
                  Engine, Type, DrawIndex, BaseOffset, DrawCount, Stride, BufObj);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    gcmONERROR(gcoBUFOBJ_Lock(BufObj, &address, gcvNULL));
    address += BaseOffset;

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_MultiDrawIndirectPrimitives(Engine->hardware,
                                                     DrawIndex,
                                                     Type,
                                                     DrawCount,
                                                     Stride,
                                                     address);

OnError:
    /* Return the status. */
    gcmFOOTER();

    return status;
}

/*******************************************************************************
**
**  gco3D_DrawInstancedPrimitives
**
**  Draw a instanced number of primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
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
gco3D_DrawInstancedPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctBOOL DrawIndex,
    IN gctINT    StartVertex,
    IN gctSIZE_T StartIndex,
    IN gctSIZE_T PrimitiveCount,
    IN gctSIZE_T VertexCount,
    IN gctSIZE_T InstanceCount
    )
{
    gceSTATUS status;
    gctUINT startVertex, startIndex;

    gcmHEADER_ARG("Engine=0x%x Type=%d DrawIndex=%d StartVertex=%lu InstanceCount=%lu,VertexCount=%lu,PrimitiveCount=%lu",
                  Engine, Type, DrawIndex, StartVertex, InstanceCount, VertexCount, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(InstanceCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(VertexCount > 0);

    /* Check if instance count is at most 24 bits. And assert if it is bigger */
    gcmASSERT((InstanceCount & 0xFF000000) == 0);

    gcmSAFECASTSIZET(startVertex, StartVertex);
    gcmSAFECASTSIZET(startIndex, StartIndex);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawInstancedPrimitives(Engine->hardware,
                                                 DrawIndex,
                                                 Type,
                                                 startVertex,
                                                 startIndex,
                                                 PrimitiveCount,
                                                 VertexCount,
                                                 InstanceCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawNullPrimitives
**
**  Draw none of primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DrawNullPrimitives(
    IN gco3D Engine
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawNullPrimitives(Engine->hardware);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gco3D_DrawPrimitivesCount
**
**  Draw an array of of primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
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
gco3D_DrawPrimitivesCount(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT* StartVertex,
    IN gctSIZE_T* VertexCount,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d StartVertex=%u PrimitiveCount=%lu",
                  Engine, Type, *StartVertex, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(StartVertex != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(VertexCount != gcvNULL);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawPrimitivesCount(Engine->hardware,
                                             Type,
                                             StartVertex,
                                             VertexCount,
                                             PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawPrimitivesOffset
**
**  Draw a number of primitives using offsets.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
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
gco3D_DrawPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d StartOffset=%d PrimitiveCount=%lu",
                  Engine, Type, StartOffset, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawPrimitivesOffset(Type,
                                              StartOffset,
                                              PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawIndexedPrimitives
**
**  Draw a number of indexed primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
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
gco3D_DrawIndexedPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctSIZE_T BaseVertex,
    IN gctSIZE_T StartIndex,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;
    gctUINT baseVertex, startIndex;

    gcmHEADER_ARG("Engine=0x%x Type=%d BaseVertex=%d StartIndex=%u "
                  "PrimitiveCount=%lu",
                  Engine, Type, BaseVertex, StartIndex, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    gcmSAFECASTSIZET(baseVertex, BaseVertex);
    gcmSAFECASTSIZET(startIndex, StartIndex);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawIndexedPrimitives(Engine->hardware,
                                               Type,
                                               baseVertex,
                                               startIndex,
                                               PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gco3D_DrawIndexedPrimitivesOffset
**
**  Draw a number of indexed primitives using offsets.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
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
gco3D_DrawIndexedPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d BaseOffset=%d StartOffset=%d "
                  "PrimitiveCount=%lu",
                  Engine, Type, BaseOffset, StartOffset, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawIndexedPrimitivesOffset(Type,
                                                     BaseOffset,
                                                     StartOffset,
                                                     PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS
gco3D_DrawPattern(
    IN gco3D Engine,
    IN gcsFAST_FLUSH_PTR FastFlushInfo
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Call the gcoHARDWARE object */
    status = gcoHARDWARE_DrawPattern(Engine->hardware, FastFlushInfo);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawIndexedPrimitivesOffset
**
**  Draw a number of indexed primitives using offsets.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
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
gco3D_DrawDrawArraysIndirect(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d BaseOffset=%d StartOffset=%d "
                  "PrimitiveCount=%lu",
                  Engine, Type, BaseOffset, StartOffset, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawIndexedPrimitivesOffset(Type,
                                                     BaseOffset,
                                                     StartOffset,
                                                     PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAntiAlias
**
**  Enable or disable anti-alias.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          Enable anti-aliasing when gcvTRUE or disable anto-aliasing when
**          gcvFALSE.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAntiAlias(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Call hardware. */
    status = gcoHARDWARE_SetAntiAlias(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gco3D_SetSamples
**
**  Set MSAA Samples
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcsSAMPLER Samples
**          MSAA samples
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetSamples(
    IN gco3D Engine,
    IN gctUINT32 Samples
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Samples=%d", Engine, Samples);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Samples < 5 || Samples != 3)
    {
        status = gcoHARDWARE_SetSamples(Engine->hardware, g_sampleInfos[Samples]);
    }
    else
    {
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_WriteBuffer
**
**  Write data into the command buffer.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
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
gco3D_WriteBuffer(
    IN gco3D Engine,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Data=0x%x Bytes=%lu Aligned=%d",
                  Engine, Data, Bytes, Aligned);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Call hardware. */
    status = gcoHARDWARE_WriteBuffer(Engine->hardware, Data, Bytes, Aligned);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFragmentConfiguration
**
**  Set the fragment processor configuration.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctBOOL ColorFromStream
**          Selects whether the fragment color comes from the color stream or
**          from the constant.
**
**      gctBOOL EnableFog
**          Fog enable flag.
**
**      gctBOOL EnableSmoothPoint
**          Antialiased point enable flag.
**
**      gctUINT32 ClipPlanes
**          Clip plane enable bits:
**              [0] for plane 0;
**              [1] for plane 1;
**              [2] for plane 2;
**              [3] for plane 3;
**              [4] for plane 4;
**              [5] for plane 5.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFragmentConfiguration(
    IN gco3D Engine,
    IN gctBOOL ColorFromStream,
    IN gctBOOL EnableFog,
    IN gctBOOL EnableSmoothPoint,
    IN gctUINT32 ClipPlanes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x ColorFromStream=%d EnableFog=%d "
                  "EnableSmoothPoint=%d ClipPlanes=%u",
                  Engine, ColorFromStream, EnableFog, EnableSmoothPoint,
                  ClipPlanes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_SetFragmentConfiguration(Engine->hardware,
                                                  ColorFromStream,
                                                  EnableFog,
                                                  EnableSmoothPoint,
                                                  ClipPlanes);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableTextureStage
**
**  Enable/disable texture stage operation.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gctBOOL Enable
**          Stage enable flag.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableTextureStage(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Enable=%d", Engine, Stage, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_EnableTextureStage(Engine->hardware, Stage, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetTextureColorMask
**
**  Program the channel enable masks for the color texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gctBOOL ColorEnabled
**          Determines whether RGB color result from the color texture
**          function affects the overall result or should be ignored.
**
**      gctBOOL AlphaEnabled
**          Determines whether A color result from the color texture
**          function affects the overall result or should be ignored.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetTextureColorMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d ColorEnabled=%d AlphaEnabled=%d",
                  Engine, Stage, ColorEnabled, AlphaEnabled);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_SetTextureColorMask(Engine->hardware,
                                             Stage,
                                             ColorEnabled,
                                             AlphaEnabled);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetTextureAlphaMask
**
**  Program the channel enable masks for the alpha texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gctBOOL ColorEnabled
**          Determines whether RGB color result from the alpha texture
**          function affects the overall result or should be ignored.
**
**      gctBOOL AlphaEnabled
**          Determines whether A color result from the alpha texture
**          function affects the overall result or should be ignored.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetTextureAlphaMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d ColorEnabled=%d AlphaEnabled=%d",
                  Engine, Stage, ColorEnabled, AlphaEnabled);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_SetTextureAlphaMask(Engine->hardware,
                                             Stage,
                                             ColorEnabled,
                                             AlphaEnabled);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFragmentColor
**
**  Program the constant fragment color to be used when there is no color
**  defined stream.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      Red, Green, Blue, Alpha
**          Color value to be set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFragmentColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFragmentColorX(Engine->hardware, Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetFragmentColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFragmentColorF(Engine->hardware, Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFogColor
**
**  Program the constant fog color to be used in the fog equation when fog
**  is enabled.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      Red, Green, Blue, Alpha
**          Color value to be set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFogColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFogColorX(Engine->hardware, Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetFogColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFogColorF(Engine->hardware,
                                      Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetTetxureColor
**
**  Program the constant texture color to be used when selected by the tetxure
**  function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      Red, Green, Blue, Alpha
**          Color value to be set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetTetxureColorX(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetTetxureColorX(Engine->hardware,
                                          Stage,
                                          Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetTetxureColorF(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Stage, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetTetxureColorF(Engine->hardware,
                                          Stage,
                                          Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetColorTextureFunction
**
**  Configure color texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gceTEXTURE_FUNCTION Function
**          Texture function.
**
**      gceTEXTURE_SOURCE Source0, Source1, Source2
**          The source of the value for the function.
**
**      gceTEXTURE_CHANNEL Channel0, Channel1, Channel2
**          Determines whether the value comes from the color, alpha channel;
**          straight or inversed.
**
**      gctINT Scale
**          Result scale value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetColorTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Function=%d Source0=%d Channel0=%d "
                  "Source1=%d Channel1=%d Source2=%d Channel2=%d",
                  Engine, Stage, Function, Source0, Channel0, Source1, Channel1,
                  Source2, Channel2);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the texture function. */
    status = gcoHARDWARE_SetColorTextureFunction(Engine->hardware,
                                                 Stage,
                                                 Function,
                                                 Source0, Channel0,
                                                 Source1, Channel1,
                                                 Source2, Channel2,
                                                 Scale);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_GetClosestRenderFormat(
    IN gco3D Engine,
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("InFormat=%d", InFormat);

    /* Route to gcoHARDWARE function. */
    status = gcoHARDWARE_GetClosestRenderFormat(InFormat,
                                                OutFormat);

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetWClipEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
#if gcdUSE_WCLIP_PATCH
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%u",
                  Engine, Enable);

    Engine->wClipEnable = Enable;

    /* Route to hardware. */
    status = gcoHARDWARE_SetWClipEnable(Engine->hardware, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}

gceSTATUS
gco3D_GetWClipEnable(
    IN gco3D Engine,
    OUT gctBOOL * Enable
    )
{
    gcmHEADER_ARG("Engine=0x%x Enable=%u",
                  Engine, Enable);

    *Enable = Engine->wClipEnable;
    /* Return the status. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetWPlaneLimitF(
    IN gco3D Engine,
    IN gctFLOAT Value
    )
{
#if gcdUSE_WCLIP_PATCH
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Value=%f",
                  Engine, Value);

    /* Route to hardware. */
    status = gcoHARDWARE_SetWPlaneLimit(Engine->hardware, Value);

    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}

gceSTATUS
gco3D_SetWPlaneLimitX(
    IN gco3D Engine,
    IN gctFIXED_POINT Value
    )
{
#if gcdUSE_WCLIP_PATCH
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Value=%x",
                  Engine, Value);

    /* Route to hardware. */
    status = gcoHARDWARE_SetWPlaneLimit(Engine->hardware, Value / 65536.0f);

    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}

gceSTATUS
gco3D_SetWPlaneLimit(
        IN gco3D Engine,
        IN gctFLOAT Value
        )
{
#if gcdUSE_WCLIP_PATCH
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Value2=%f",
            Engine, Value);

    /* Route to hardware. */
    status = gcoHARDWARE_SetWPlaneLimit(Engine->hardware, Value);

    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}

/*******************************************************************************
**
**  gco3D_SetAlphaTextureFunction
**
**  Configure alpha texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gceTEXTURE_FUNCTION Function
**          Texture function.
**
**      gceTEXTURE_SOURCE Source0, Source1, Source2
**          The source of the value for the function.
**
**      gceTEXTURE_CHANNEL Channel0, Channel1, Channel2
**          Determines whether the value comes from the color, alpha channel;
**          straight or inversed.
**
**      gctINT Scale
**          Result scale value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Function=%d Source0=%d Channel0=%d "
                  "Source1=%d Channel1=%d Source2=%d Channel2=%d",
                  Engine, Stage, Function, Source0, Channel0, Source1, Channel1,
                  Source2, Channel2);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the texture function. */
    status = gcoHARDWARE_SetAlphaTextureFunction(Engine->hardware,
                                                 Stage,
                                                 Function,
                                                 Source0, Channel0,
                                                 Source1, Channel1,
                                                 Source2, Channel2,
                                                 Scale);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_Semaphore
**
**  Send sempahore and stall until sempahore is signalled.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gceWHERE From
**          Semaphore source.
**
**      GCHWERE To
**          Sempahore destination.
**
**      gceHOW How
**          What to do.  Can be a one of the following values:
**
**              gcvHOW_SEMAPHORE            Send sempahore.
**              gcvHOW_STALL                Stall.
**              gcvHOW_SEMAPHORE_STALL  Send semaphore and stall.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gco3D_Semaphore(
    IN gco3D Engine,
    IN gceWHERE From,
    IN gceWHERE To,
    IN gceHOW How
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x From=%d To=%d How=%d", Engine, From, To, How);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Route to hardware. */
    status = gcoHARDWARE_Semaphore(Engine->hardware, From, To, How, gcvNULL);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetCentroids
**
**  Set the subpixels center.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctUINT32 Index
**          Indicate which matrix need to be set.
**
**      gctPOINTER Centroids
**          Ponter to the centroid array.
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetCentroids(
    IN gco3D Engine,
    IN gctUINT32 Index,
    IN gctPOINTER Centroids
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Index=%u Centroids=0x%x",
                  Engine, Index, Centroids);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Route to hardware. */
    status = gcoHARDWARE_SetCentroids(Engine->hardware, Index, Centroids);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_FlushSHL1Cache
**
**  Explicitly flush shader L1 cache
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_FlushSHL1Cache(
    IN gco3D Engine
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Route to hardware. */
    status = gcoHARDWARE_FlushSHL1Cache(Engine->hardware);

    /* Return the status. */
    gcmFOOTER();
    return status;

}

/*******************************************************************************
**
**  gco3D_SetCentroids
**
**  Set the subpixels center.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctUINT32 Index
**          Indicate sample coordinate to be returned.
**
**      gctBOOL yInverted
**          yInverted flag
**
**  OUTPUT:
**      gctFLOAT_PTR Coords
**          x,y coordination inside one pixel
*/
gceSTATUS
gco3D_GetSampleCoords(
    IN gco3D Engine,
    IN gctUINT32 SampleIndex,
    IN gctBOOL yInverted,
    OUT gctFLOAT_PTR Coords
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Engine=0x%x SampleIndex=%d yInverted=%d Coords=0x%x",
                   Engine, SampleIndex, yInverted, Coords);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_GetSampleCoords(Engine->hardware, SampleIndex, yInverted, Coords);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gco3D_InvokeThreadWalker
**
**  Start OCL thread walker.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gcsTHREAD_WALKER_INFO_PTR Info
**          Pointer to the informational structure.
*/
gceSTATUS
gco3D_InvokeThreadWalker(
    IN gco3D Engine,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%08X Info=0x%08X", Engine, Info);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Route to hardware. */
    status = gcoHARDWARE_InvokeThreadWalkerGL(Engine->hardware, Info);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetLogicOp.
**
**  Set logic op.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Rop
**          4 bit rop code.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetLogicOp(
    IN gco3D Engine,
    IN gctUINT8 Rop
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Rop=%d", Engine, Rop);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program logicOp. */
    status = gcoHARDWARE_SetLogicOp(Engine->hardware, Rop);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetColorOutCount(
    IN gco3D Engine,
    IN gctUINT32 ColorOutCount)
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x ColorOutCount=%d", Engine, ColorOutCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program num mRT. */
    status = gcoHARDWARE_SetColorOutCount(Engine->hardware, ColorOutCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetColorCacheMode(
    IN gco3D Engine)
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program num mRT. */
    status = gcoHARDWARE_SetColorCacheMode(Engine->hardware);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetQuery(
    IN gco3D Engine,
    IN gctUINT32 QueryHeader,
    IN gceQueryType Type,
    IN gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x QueryHeader=0x%x Type=%d Enable=%d", Engine, QueryHeader, Type, Enable);

    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Enable)
    {
        status = gcoHARDWARE_SetQuery(Engine->hardware, QueryHeader, Type, gcvQUERYCMD_BEGIN, gcvNULL);
    }
    else
    {
        status = gcoHARDWARE_SetQuery(Engine->hardware, QueryHeader, Type, gcvQUERYCMD_END, gcvNULL);
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_GetQuery(
    IN gco3D Engine,
    IN gceQueryType Type,
    IN gcsSURF_NODE_PTR Node,
    IN gctUINT32    Size,
    IN gctPOINTER   Locked,
    OUT gctINT32 * Index)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x Type=%d Index=%d", Engine, Type, Index);

    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Invalidate CPU cache for it was written by GPU. */
    gcmONERROR(gcoSURF_NODE_Cache(Node, Locked, Size, gcvCACHE_INVALIDATE));

    gcmONERROR(gcoHARDWARE_GetQueryIndex(Engine->hardware, Type, Index));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetXfbHeader(
    gco3D Engine,
    gctUINT32 Physical
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Physical=0x%x", Engine, Physical);

    status = gcoHARDWARE_SetXfbHeader(Engine->hardware, Physical);

    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetXfbBuffer(
    IN gco3D Engine,
    IN gctUINT32 Index,
    IN gctUINT32 BufferAddr,
    IN gctUINT32 BufferStride,
    IN gctUINT32 BufferSize
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Index=%d BufferAddr=0x%x BufferStride=%d BufferSize=%d",
                   Engine, Index, BufferAddr, BufferStride, BufferSize);

    status = gcoHARDWARE_SetXfbBuffer(Engine->hardware, Index, BufferAddr, BufferStride, BufferSize);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_SetXfbCmd(
    IN gco3D Engine,
    IN gceXfbCmd Cmd
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Cmd=%d", Engine, Cmd);

    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetXfbCmd(Engine->hardware, Cmd, gcvNULL);

    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetRasterDiscard(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetRasterDiscard(Engine->hardware, Enable);

    gcmFOOTER();
    return status;

}



gceSTATUS
gco3D_PrimitiveRestart(
    IN gco3D Engine,
    IN gctBOOL PrimitiveRestart
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x PrimitiveRestart=%d", Engine, PrimitiveRestart);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program primitive restart. */
    status = gcoHARDWARE_PrimitiveRestart(Engine->hardware, PrimitiveRestart);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_LoadProgram(
    IN gco3D Engine,
    IN gcePROGRAM_STAGE_BIT StageBits,
    IN gctPOINTER ProgramState
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x StageBits=0x%x ProgramState=0x%x", Engine, StageBits, ProgramState);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_LoadProgram(Engine->hardware, StageBits, ProgramState);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_EnableAlphaToCoverage(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_EnableAlphaToCoverage(Engine->hardware, Enable);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_EnableSampleCoverage(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_EnableSampleCoverage(Engine->hardware, Enable);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_SetSampleCoverageValue(
    IN gco3D Engine,
    IN gctFLOAT CoverageValue,
    IN gctBOOL Invert
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x CoverageValue=%lf Invert=%d", Engine, CoverageValue, Invert);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetSampleCoverageValue(Engine->hardware, CoverageValue, Invert);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_EnableSampleMask(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_EnableSampleMask(Engine->hardware, Enable);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_SetSampleMask(
    IN gco3D Engine,
    IN gctUINT32 SampleMask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x SampleMask%d", Engine, SampleMask);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetSampleMask(Engine->hardware, SampleMask);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_EnableSampleShading(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_EnableSampleShading(Engine->hardware, Enable);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_SetSampleShading(
    IN gco3D Engine,
    IN gctBOOL Enable,
    IN gctBOOL IsSampleIn,
    IN gctFLOAT SampleShadingValue
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d, IsSampleIn=0x%x, SampleShadingValue=0x%x", Engine, Enable, IsSampleIn, SampleShadingValue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetSampleShading(Engine->hardware, Enable, IsSampleIn, SampleShadingValue);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_SetMinSampleShadingValue(
    IN gco3D Engine,
    IN gctFLOAT MinSampleShadingValue
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x MinSampleShadingValue=%d", Engine, MinSampleShadingValue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_SetMinSampleShadingValue(Engine->hardware, MinSampleShadingValue);

    gcmFOOTER();

    return status;
}

gceSTATUS
gco3D_EnableSampleMaskOut(
    IN gco3D Engine,
    IN gctBOOL Enable,
    IN gctINT SampleMaskLoc
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d SampleMaskLoc=0x%x", Engine, Enable, SampleMaskLoc);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    status = gcoHARDWARE_EnableSampleMaskOut(Engine->hardware, Enable, SampleMaskLoc);

    gcmFOOTER();

    return status;
}

/*******************************************************************************
**
**  gco3D_Get3DEngine
**
**  Get the pointer to the gco3D object which is the current one of this thread
**
**  OUTPUT:
**
**      gco3D * Engine
**          Pointer to a variable receiving the gco3D object pointer.
*/
gceSTATUS
gco3D_Get3DEngine(
    OUT gco3D * Engine
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

    do
    {
        gcmONERROR(gcoOS_GetTLS(&tls));

        /* Return pointer to the gco3D object. */
        *Engine = tls->engine3D;

        if (*Engine == gcvNULL)
        {
            status = gcvSTATUS_INVALID_OBJECT;
            gcmERR_BREAK(status);
        }
        /* Success. */
        gcmFOOTER_ARG("*Engine=0x%x", *Engine);

        return gcvSTATUS_OK;

    }while (gcvFALSE);

 OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*****************************************************************************
*********
**
**  gco3D_Set3DEngine
**
**  Set the pointer as the gco3D object to be current 3D engine of this thread
**
**
**   gco3D Engine
**       The gco3D object that needs to be set.
**
**  OUTPUT:
**
**   nothing.
*/
gceSTATUS
gco3D_Set3DEngine(
     IN gco3D Engine
     )
{
     gceSTATUS status;
     gcsTLS_PTR tls;
     gctUINT32 coreIndex;
     gcmHEADER();

     /* Verify the arguments. */
     gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

     gcmONERROR(gcoOS_GetTLS(&tls));

     /* Set the gco3D object. */
     tls->engine3D = Engine;

     gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

     /* Set this engine's hardware object to be current one */
     gcmONERROR(gcoHARDWARE_Set3DHardware(Engine->hardware));

     /*
      For ES ,We need  keep the TLS->currentCoreIndex same with VIV_MGPU_AFFINITY attri
     */
    gcmONERROR(gcoHARDWARE_QueryCoreIndex(Engine->hardware, 0, &coreIndex));
    gcmONERROR(gcoHAL_SetCoreIndex(gcvNULL, coreIndex));

     /* Success. */
     gcmFOOTER_NO();
     return gcvSTATUS_OK;

OnError:
     /* Return the status. */
     gcmFOOTER();
     return status;
}


/*****************************************************************************
*********
**
**  gcoHAL_UnSet3DEngine
**
**  UnSet the pointer as the gco3D object from this thread, restore the old
**  hardware object.
**
**  INPUT:
**
**
**   gco3D Engine
**       The gco3D object that needs to be unset.
**
**  OUTPUT:
**
**   nothing.
*/
gceSTATUS
gco3D_UnSet3DEngine(
     IN gco3D Engine
     )
{
     gceSTATUS status;
     gcsTLS_PTR tls;
     gcoHARDWARE currentHardware;

     gcmHEADER();

     /* Verify the arguments. */
     gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

     gcmONERROR(gcoOS_GetTLS(&tls));

     gcmONERROR(gcoHARDWARE_Get3DHardware(&currentHardware));

     gcmASSERT(currentHardware == Engine->hardware);

     /* Remove the gco3D object from TLS */
     tls->engine3D = gcvNULL;

     /* Set current hardware object of this thread is NULL */
     gcmONERROR(gcoHARDWARE_Set3DHardware(gcvNULL));

     /* Success. */
     gcmFOOTER_NO();
     return gcvSTATUS_OK;

OnError:
     /* Return the status. */
     gcmFOOTER();
     return status;
}

/*****************************************************************************
**
**  gco3D_QueryReset
**
**  Query if underneath GPU is reseted and if current context is innocent.
**
**  INPUT:
**
**
**   gco3D Engine
**       The gco3D object that needs to be unset.
**
**  OUTPUT:
**   gctBOOL_PTR Innocent
**       Pointer to bool value to receive innocent flag.
**
**   return gcvSTATUS_TRUE means GPU is reseted after last query
**
*/
gceSTATUS
gco3D_QueryReset(
    IN gco3D Engine,
    OUT gctBOOL_PTR Innocent
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Innocent=0x%x", Engine, Innocent);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    gcmONERROR(gcoHARDWARE_QueryReset(Engine->hardware, Innocent));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gco3D_Construct(
    IN gcoHAL Hal,
    IN gctBOOL Robust,
    OUT gco3D * Engine
    )
{
    *Engine = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_Destroy(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAPI(
    IN gco3D Engine,
    IN gceAPI ApiType
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_GetAPI(
    IN gco3D Engine,
    OUT gceAPI * ApiType
    )
{
    *ApiType = gcvAPI_OPENGL;
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetPSOutputMapping(
    IN gco3D Engine,
    IN gctINT32 * psOutputMapping
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetRenderLayered(
    IN gco3D Engine,
    IN gctBOOL Enable,
    IN gctUINT MaxLayers
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetShaderLayered(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetTarget(
    IN gco3D Engine,
    IN gctUINT32 TargetIndex,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 LayerIndex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_UnsetTarget(
    IN gco3D Engine,
    IN gctUINT32 TargetIndex,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetDepth(
    IN gco3D Engine,
    IN gcsSURF_VIEW *SurfView
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_UnsetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetViewport(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetScissors(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearColor(
    IN gco3D Engine,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearDepthX(
    IN gco3D Engine,
    IN gctFIXED_POINT Depth
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearDepthF(
    IN gco3D Engine,
    IN gctFLOAT Depth
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearStencil(
    IN gco3D Engine,
    IN gctUINT32 Stencil
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetShading(
    IN gco3D Engine,
    IN gceSHADING Shading
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableBlending(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendFunction(
    IN gco3D Engine,
    IN gceBLEND_UNIT Unit,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendMode(
    IN gco3D Engine,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableBlendingIndexed(
    IN gco3D Engine,
    IN gctUINT Index,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendFunctionIndexed(
    IN gco3D Engine,
    IN gctUINT Index,
    IN gceBLEND_UNIT Unit,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendModeIndexed(
    IN gco3D Engine,
    IN gctUINT Index,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendColor(
    IN gco3D Engine,
    IN gctUINT Red,
    IN gctUINT Green,
    IN gctUINT Blue,
    IN gctUINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetCulling(
    IN gco3D Engine,
    IN gceCULL Mode
    )
{
    return gcvSTATUS_OK;
}
gceSTATUS gco3D_SetPointSizeEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetPointSprite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFill(
    IN gco3D Engine,
    IN gceFILL Mode
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableDepthWrite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthMode(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthRangeX(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFIXED_POINT Near,
    IN gctFIXED_POINT Far
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthRangeF(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetDepthPlaneF(
    IN gco3D Engine,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetLastPixelEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthScaleBiasF(
    IN gco3D Engine,
    IN gctFLOAT DepthScale,
    IN gctFLOAT DepthBias
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthScaleBiasX(
    IN gco3D Engine,
    IN gctFIXED_POINT DepthScale,
    IN gctFIXED_POINT DepthBias
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableDither(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetColorWrite(
    IN gco3D Engine,
    IN gctUINT8 Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetColorWriteIndexed(
    IN gco3D Engine,
    IN gctUINT Index,
    IN gctUINT8 Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetEarlyDepth(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthOnly(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilMode(
    IN gco3D Engine,
    IN gceSTENCIL_MODE Mode
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilMaskBack(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilWriteMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilWriteMaskBack(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctBOOL  Front
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilCompare(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceCOMPARE Compare
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilPass(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetAllEarlyDepthModes(
    IN gco3D Engine,
    IN gctBOOL Disable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SwitchDynamicEarlyDepthMode(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_DisableDynamicEarlyDepthMode(
    IN gco3D Engine,
    IN gctBOOL Disable
    )
{
    return gcvSTATUS_OK;
}
gceSTATUS gco3D_SetStencilFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilDepthFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilAll(
    IN gco3D Engine,
    IN gcsSTENCIL_INFO_PTR Info
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaTest(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctFLOAT FloatReference
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaReferenceX(
    IN gco3D Engine,
    IN gctFIXED_POINT Reference
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaReferenceF(
    IN gco3D Engine,
    IN gctFLOAT Reference
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAntiAliasLine(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAALineWidth(
    IN gco3D Engine,
    IN gctFLOAT Width
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAALineTexSlot(
    IN gco3D Engine,
    IN gctUINT TexSlot
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctSIZE_T StartVertex,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_DrawInstancedPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctBOOL DrawIndex,
    IN gctINT    StartVertex,
    IN gctSIZE_T StartIndex,
    IN gctSIZE_T PrimitiveCount,
    IN gctSIZE_T VertexCount,
    IN gctSIZE_T InstanceCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_DrawPrimitivesCount(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT* StartVertex,
    IN gctSIZE_T* VertexCount,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawIndexedPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctSIZE_T BaseVertex,
    IN gctSIZE_T StartIndex,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawIndexedPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAntiAlias(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetSamples(
    IN gco3D Engine,
    IN gctUINT32 Samples
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_WriteBuffer(
    IN gco3D Engine,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFragmentConfiguration(
    IN gco3D Engine,
    IN gctBOOL ColorFromStream,
    IN gctBOOL EnableFog,
    IN gctBOOL EnableSmoothPoint,
    IN gctUINT32 ClipPlanes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableTextureStage(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTextureColorMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTextureAlphaMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFragmentColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFragmentColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFogColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFogColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTetxureColorX(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTetxureColorF(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetColorTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetWClipEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_GetWClipEnable(
    IN gco3D Engine,
    OUT gctBOOL * Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetWPlaneLimitF(
    IN gco3D Engine,
    IN gctFLOAT Value
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetWPlaneLimitX(
    IN gco3D Engine,
    IN gctFIXED_POINT Value
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetWPlaneLimit(
        IN gco3D Engine,
        IN gctFLOAT Value
        )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_Semaphore(
    IN gco3D Engine,
    IN gceWHERE From,
    IN gceWHERE To,
    IN gceHOW How
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_FlushSHL1Cache(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetCentroids(
    IN gco3D Engine,
    IN gctUINT32 Index,
    IN gctPOINTER Centroids
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_GetSampleCoords(
    IN gco3D Engine,
    IN gctUINT32 SampleIndex,
    IN gctBOOL yInverted,
    OUT gctFLOAT_PTR Coords
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS gco3D_InvokeThreadWalker(
    IN gco3D Engine,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetLogicOp(
    IN gco3D Engine,
    IN gctUINT8 Rop
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetXfbHeader(
    gco3D Engine,
    gctUINT32 Physical
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetXfbBuffer(
    IN gco3D Engine,
    IN gctUINT32 Index,
    IN gctUINT32 BufferAddr,
    IN gctUINT32 BufferStride,
    IN gctUINT32 BufferSize
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetXfbCmd(
    IN gco3D Engine,
    IN gceXfbCmd Cmd
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetRasterDiscard(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS gco3D_SetColorOutCount(
    IN gco3D Engine,
    IN gctUINT32 ColorOutCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetColorCacheMode(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetQuery(
    IN gco3D Engine,
    IN gctUINT32 QueryHeader,
    IN gceQueryType Type,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_GetQuery(
    IN gco3D Engine,
    IN gceQueryType Type,
    IN gcsSURF_NODE_PTR Node,
    IN gctUINT32    Size,
    IN gctPOINTER   Locked,
    OUT gctINT32 * Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_PrimitiveRestart(
    IN gco3D Engine,
    IN gctBOOL PrimitiveRestart
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS
gco3D_LoadProgram(
    IN gco3D Engine,
    IN gcePROGRAM_STAGE_BIT StageBits,
    IN gctPOINTER ProgramState
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS
gco3D_EnableAlphaToCoverage(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_EnableSampleCoverage(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetSampleCoverageValue(
    IN gco3D Engine,
    IN gctFLOAT CoverageValue,
    IN gctBOOL Invert
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_EnableSampleMask(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetSampleMask(
    IN gco3D Engine,
    IN gctUINT32 SampleMask
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS
gco3D_EnableSampleShading(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS
gco3D_SetSampleShading(
    IN gco3D Engine,
    IN gctBOOL Enable,
    IN gctBOOL IsSampleIn,
    IN gctFLOAT SampleShadingValue
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetMinSampleShadingValue(
    IN gco3D Engine,
    IN gctFLOAT MinSampleShadingValue
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_EnableSampleMaskOut(
    IN gco3D Engine,
    IN gctBOOL Enable,
    IN gctINT SampleMaskLoc
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_UnSet3DEngine(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}


gceSTATUS
gco3D_Get3DEngine(
    OUT gco3D * Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_QueryReset(
    IN gco3D Engine,
    OUT gctBOOL_PTR Innocent
    )
{
    return gcvSTATUS_FALSE;
}

gceSTATUS
gco3D_DrawIndirectPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctBOOL DrawIndex,
    IN gctINT BaseOffset,
    IN gcoBUFOBJ BufObj
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_DrawNullPrimitives(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_DrawPattern(
    IN gco3D Engine,
    IN gcsFAST_FLUSH_PTR FastFlushInfo
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_GetClosestRenderFormat(
    IN gco3D Engine,
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_IsProgramSwitched(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_MultiDrawIndirectPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctBOOL DrawIndex,
    IN gctINT BaseOffset,
    IN gctINT DrawCount,
    IN gctINT Stride,
    IN gcoBUFOBJ BufObj
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_Set3DEngine(
     IN gco3D Engine
     )
{
    return gcvSTATUS_OK;
}

 gceSTATUS
 gco3D_SetAlphaKill(
     IN gco3D Engine,
     IN gctBOOL AlphaKill,
     IN gctBOOL ColorKill
     )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetEarlyDepthFromAPP(
    IN gco3D Engine,
    IN gctBOOL EarlyDepthFromAPP
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetPatchVertices(
    IN gco3D Engine,
    IN gctINT PatchVertices
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetPrimitiveIdEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetRADepthWrite(
    IN gco3D Engine,
    IN gctBOOL Disable,
    IN gctBOOL psReadZ,
    IN gctBOOL psReadW
    )
{
    return gcvSTATUS_OK;
}


#endif /* gcdNULL_DRIVER < 2 */
#endif /* gcdENABLE_3D */
