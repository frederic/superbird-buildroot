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


#ifndef __gc_hal_user_h_
#define __gc_hal_user_h_

#include "gc_hal.h"
#include "gc_hal_driver.h"
#include "gc_hal_enum.h"
#include "gc_hal_dump.h"
#include "gc_hal_base.h"
#include "gc_hal_priv.h"
#include "gc_hal_raster.h"
#include "gc_hal_user_math.h"
#include "gc_hal_user_os_memory.h"
#include "gc_hal_user_os_atomic.h"
#include "gc_hal_user_debug.h"


#include "gc_hal_engine.h"
#if gcdENABLE_3D
#include "drvi/gc_vsc_drvi_interface.h"
#include "gc_hal_user_shader.h"
#endif

#define OPT_VERTEX_ARRAY                  1

#if gcdENABLE_3D && gcdUSE_VX
#define GC_VX_ASM                         0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
********************************* Private Macro ********************************
\******************************************************************************/
#define gcmGETHARDWARE(Hardware) \
{ \
    if (Hardware == gcvNULL)  \
    {\
        gcsTLS_PTR __tls__; \
        \
        gcmONERROR(gcoOS_GetTLS(&__tls__)); \
        \
        if (__tls__->currentType == gcvHARDWARE_2D \
            && gcvSTATUS_TRUE == gcoHAL_QuerySeparated2D(gcvNULL) \
            && gcvSTATUS_TRUE == gcoHAL_Is3DAvailable(gcvNULL) \
            ) \
        { \
            if (__tls__->hardware2D == gcvNULL) \
            { \
                gcmONERROR(gcoHARDWARE_Construct(gcPLS.hal, gcvTRUE, gcvFALSE, &__tls__->hardware2D)); \
                gcmTRACE_ZONE( \
                    gcvLEVEL_VERBOSE, gcvZONE_HARDWARE, \
                    "%s(%d): hardware object 0x%08X constructed.\n", \
                    __FUNCTION__, __LINE__, __tls__->hardware2D \
                    ); \
            } \
            \
            Hardware = __tls__->hardware2D; \
        } \
        else \
        if (__tls__->currentType == gcvHARDWARE_VG) \
        {\
            Hardware = gcvNULL;\
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);\
        }\
        else \
        { \
            if (__tls__->defaultHardware == gcvNULL) \
            { \
                gcmONERROR(gcoHARDWARE_Construct(gcPLS.hal, gcvTRUE, gcvFALSE, &__tls__->defaultHardware)); \
                \
                if (__tls__->currentType != gcvHARDWARE_2D) \
                { \
                } \
                \
                gcmTRACE_ZONE( \
                    gcvLEVEL_VERBOSE, gcvZONE_HARDWARE, \
                    "%s(%d): hardware object 0x%08X constructed.\n", \
                    __FUNCTION__, __LINE__, __tls__->defaultHardware \
                    ); \
            } \
            \
            if (__tls__->currentHardware == gcvNULL)\
            {\
               __tls__->currentHardware = __tls__->defaultHardware; \
            }\
            Hardware = __tls__->currentHardware; \
            \
        } \
        gcmTRACE_ZONE( \
                    gcvLEVEL_VERBOSE, gcvZONE_HARDWARE, \
                    "%s(%d): TLS current hardware object is acquired.\n", \
                    __FUNCTION__, __LINE__\
                    ); \
    } \
}

#define gcmGETVGHARDWARE(Hardware) \
{ \
    gcsTLS_PTR __tls__; \
    gceSTATUS verifyStatus; \
    \
    verifyStatus = gcoOS_GetTLS(&__tls__); \
    if (gcmIS_ERROR(verifyStatus)) \
    { \
        gcmFOOTER_ARG("status = %d", verifyStatus); \
        return verifyStatus;                  \
    }                                   \
    \
    if (__tls__->vg == gcvNULL) \
    { \
        verifyStatus = gcoVGHARDWARE_Construct(gcPLS.hal, &__tls__->vg); \
        if (gcmIS_ERROR(verifyStatus))            \
        {                                   \
            gcmFOOTER_ARG("status = %d", verifyStatus); \
            return verifyStatus;                  \
        }                                   \
        \
        gcmTRACE_ZONE( \
            gcvLEVEL_VERBOSE, gcvZONE_HARDWARE, \
            "%s(%d): hardware object 0x%08X constructed.\n", \
            __FUNCTION__, __LINE__, __tls__->vg \
            ); \
    } \
    \
    Hardware = __tls__->vg; \
}

#define gcmGETCURRENTHARDWARE(hw) \
{ \
    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &hw));\
    gcmASSERT(hw != gcvHARDWARE_INVALID);\
}

#define gcmGETHARDWAREADDRESS(node, address) \
{ \
    gcmVERIFY_OK(gcsSURF_NODE_GetHardwareAddress( \
        &node, &address, gcvNULL, gcvNULL, gcvNULL)); \
} \

#define gcmGETHARDWAREADDRESSP(pNode, address) \
{ \
    gcmVERIFY_OK(gcsSURF_NODE_GetHardwareAddress( \
        pNode, &address, gcvNULL, gcvNULL, gcvNULL)); \
} \

#define gcmGETHARDWAREADDRESSBOTTOM(node, bottom) \
{ \
    gcmVERIFY_OK(gcsSURF_NODE_GetHardwareAddress( \
        &node, gcvNULL, gcvNULL, gcvNULL, &bottom)); \
} \

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

typedef struct _gcsSAMPLES
{
    gctUINT8 x;
    gctUINT8 y;
    gctUINT8 product;
} gcsSAMPLES;

#if gcdENABLE_3D

/* gco3D object. */
typedef struct _gco3D
{
    /* Object. */
    gcsOBJECT                   object;

    /* Render target. */
    gcoSURF                     mRT[gcdMAX_DRAW_BUFFERS];
    gctPOINTER                  mRTMemory[gcdMAX_DRAW_BUFFERS];
    gctUINT32                   mRTSliceOffset[gcdMAX_DRAW_BUFFERS];
    gctUINT32                   mRTSliceNum[gcdMAX_DRAW_BUFFERS];

    /* Depth buffer. */
    gcoSURF                     depth;
    gctPOINTER                  depthMemory;
    gctUINT32                   depthSliceOffset;
    gctUINT32                   depthSliceNum;

    /* Clear color. */
    gctBOOL                     clearColorDirty;
    gceVALUE_TYPE               clearColorType;
    gcuVALUE                    clearColorRed;
    gcuVALUE                    clearColorGreen;
    gcuVALUE                    clearColorBlue;
    gcuVALUE                    clearColorAlpha;

    /* Clear depth value. */
    gctBOOL                     clearDepthDirty;
    gceVALUE_TYPE               clearDepthType;
    gcuVALUE                    clearDepth;

    /* Clear stencil value. */
    gctBOOL                     clearStencilDirty;
    gctUINT                     clearStencil;

    /* API type. */
    gceAPI                      apiType;

    gctBOOL                     wClipEnable;

    gctBOOL                     mRTtileStatus;

    /* Hardware object for this 3D engine */
    gcoHARDWARE                 hardware;
} _gco3D;

/******************************************************************************\
*******************************Clear Range of HZ ******************************
\******************************************************************************/
typedef enum _gceHZCLEAR_RANGE
{
    gcvHZCLEAR_ALL   = 0x1,
    gcvHZCLEAR_RECT  = 0x2,
}
gceHZCLEAR_TYPE;

/******************************************************************************\
*******************************Tile status type  ******************************
\******************************************************************************/
typedef enum _gceTILESTATUS_TYPE
{
    gcvTILESTATUS_COLOR = 0x1,
    gcvTILESTATUS_DEPTH = 0x2,
}
gceTILESTATUS_TYPE;

/******************************************************************************\
***************************** gcoSAMPLER Structure *****************************
\******************************************************************************/

/* Sampler structure. */
typedef struct _gcsSAMPLER
{
    gctUINT32               width;
    gctUINT32               height;
    gctUINT32               depth;
    gctUINT32               faces;

    gceTEXTURE_TYPE         texType;

    gceSURF_FORMAT          format;
    gctBOOL                 unsizedDepthTexture;
    gctBOOL                 needYUVAssembler;
    gceTILING               tiling;
    gceCACHE_MODE           cacheMode;

    gctBOOL                 endianHint;

    gctUINT32               lodNum;
    gctUINT32               baseLod;
    gctUINT32               lodAddr[30];
    gctUINT32               lodStride[30];

    gcsTEXTURE_PTR          textureInfo;

    gceSURF_ALIGNMENT       hAlignment;
    gceSURF_ADDRESSING      addressing;

    /* texture surface tile status state */
    gctBOOL                 hasTileStatus;
    /* only base level surface, only for non-mipmap use-case */
    gcoSURF                 baseLevelSurf;
    gctINT32                compressedDecFormat;
    gctUINT32               astcSize[30];
    gctUINT32               astcSRGB[30];

    gcsSURF_FORMAT_INFO_PTR formatInfo;

    gctBOOL                 filterable;

    gcsSURF_NODE_PTR        descNode;
} gcsSAMPLER, *gcsSAMPLER_PTR;

typedef struct _gcsTXDESC_UPDATE_INFO
{
    gceTEXTURE_TYPE         type;
    gctBOOL                 unsizedDepthTexture;
    gctINT                  levels;

    gceENDIAN_HINT          endianHint;

    gctUINT32               baseLevelWidth;
    gctUINT32               baseLevelHeight;
    gctUINT32               baseLevelDepth;

    gcoSURF                 baseLevelSurf;
    /* Valid size should be 16. */
    gctUINT32               astcSize[20];
    gctUINT32               astcSRGB[20];
    gctUINT32               lodAddr[16];
    gctPOINTER              desc;

    /* For dump only */
    gctUINT32               physical;

    gctFLOAT                borderColor32[4];


} gcsTXDESC_UPDATE_INFO, *gcsTXDESC_UPDATE_INFO_PTR;

typedef struct _gcsFAST_FLUSH_UNIFORM
{
    gcUNIFORM           halUniform[2];
    gctUINT32           physicalAddress[2];

    gctUINT32           columns;
    gctUINT32           rows;

    gctSIZE_T           arrayStride;
    gctSIZE_T           matrixStride;

    gctPOINTER          data;
    gctBOOL             dirty;
}
gcsFAST_FLUSH_UNIFORM;

typedef struct _gcsFAST_FLUSH_VERTEX_ARRAY
{
    gctINT              size;
    gctINT              stride;
    gctPOINTER          pointer;
    gctUINT             divisor;
}
gcsFAST_FLUSH_VERTEX_ARRAY;

typedef struct _gcsFAST_FLUSH
{
    /* Uniform */
    gctBOOL                         programValid;
    gctINT                          userDefUniformCount;
    gcsFAST_FLUSH_UNIFORM           uniforms[9];

    /* Stream */
    gctUINT                         vsInputArrayMask;
    gctUINT                         attribMask;
    gctUINT                         vertexArrayEnable;
    gcsFAST_FLUSH_VERTEX_ARRAY      attribute[gcdATTRIBUTE_COUNT];
    gcoBUFOBJ                       boundObjInfo[gcdATTRIBUTE_COUNT];
    gctUINT                         attributeLinkage[gcdATTRIBUTE_COUNT];

    /* Index */
    gcoBUFOBJ                       bufObj;
    gctPOINTER                      indices;
    gctUINT32                       indexFormat;

    /* Alpha */
    gctBOOL                         blend;
    gctUINT32                       color;
    gceBLEND_MODE                   modeAlpha;
    gceBLEND_MODE                   modeColor;
    gceBLEND_FUNCTION               srcFuncColor;
    gceBLEND_FUNCTION               srcFuncAlpha;
    gceBLEND_FUNCTION               trgFuncColor;
    gceBLEND_FUNCTION               trgFuncAlpha;

    /* Depth compare */
    gceDEPTH_MODE                   depthMode;
    gceCOMPARE                      depthInfoCompare;
    gceCOMPARE                      compare;

    /* Draw index info */
    gctUINT                         drawCount;
    gctINT                          indexCount;
    gctINT                          instanceCount;
    gctBOOL                         hasHalti;
}
gcsFAST_FLUSH;
#endif /* gcdENABLE_3D */
/******************************************************************************\
****************************** Object Declarations *****************************
\******************************************************************************/

typedef struct _gcoBUFFER *             gcoBUFFER;
typedef struct _gcs2D_State*            gcs2D_State_PTR;

#if gcdENABLE_3D

/* Internal dynamic index buffers. */
gceSTATUS
gcoINDEX_UploadDynamicEx(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL ConvertToIndexedTriangleList
    );

gceSTATUS
gcoINDEX_BindDynamic(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    );

gceSTATUS
gcoINDEX_GetMemoryIndexRange(
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T indexCount,
    OUT gctUINT32* iMin,
    OUT gctUINT32* iMax
    );

/******************************************************************************\
********************************** gco3D Object ********************************
\******************************************************************************/

gceSTATUS
gco3D_ClearHzTileStatus(
    IN gco3D Engine,
    IN gcoSURF Surface,
    IN gcsSURF_NODE_PTR TileStatus
    );

#endif /* gcdENABLE_3D */

#if gcdUSE_VX && gcdENABLE_3D

gceSTATUS gcoVX_Construct(OUT gcoVX *VXObj);

gceSTATUS gcoVX_Destroy(IN gcoVX VXObj);
#endif

/******************************************************************************\
******************************* gcoHARDWARE Object ******************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*----------------------------- gcoHARDWARE Common ----------------------------*/

/* Construct a new gcoHARDWARE object. */
gceSTATUS
gcoHARDWARE_Construct(
    IN gcoHAL Hal,
    IN gctBOOL ThreadDefault,
    IN gctBOOL Robust,
    OUT gcoHARDWARE * Hardware
    );

gceSTATUS
gcoHARDWARE_ConstructEx(
    IN gcoHAL Hal,
    IN gctBOOL ThreadDefault,
    IN gctBOOL Robust,
    IN gceHARDWARE_TYPE Type,
    IN gctUINT32    AttachGpuCount,
    IN gctUINT32    CoreIndexs[],
    OUT gcoHARDWARE * Hardware
    );


/* Destroy an gcoHARDWARE object. */
gceSTATUS
gcoHARDWARE_Destroy(
    IN gcoHARDWARE Hardware,
    IN gctBOOL  ThreadDefault
    );

/* Query the identity of the hardware. */
gceSTATUS
gcoHARDWARE_QueryChipIdentity(
    IN  gcoHARDWARE Hardware,
    OUT gceCHIPMODEL* ChipModel,
    OUT gctUINT32* ChipRevision
    );

/* Query the identity of the hardware with structure parameter. */
gceSTATUS gcoHARDWARE_QueryChipIdentityEx(
    IN  gcoHARDWARE Hardware,
    IN  gctUINT32 SizeOfParam,
    OUT gcsHAL_CHIPIDENTITY *ChipIdentity
    );

/* Verify whether the specified feature is available in hardware. */
gceSTATUS
gcoHARDWARE_IsFeatureAvailable(
    IN gcoHARDWARE Hardware,
    IN gceFEATURE Feature
    );

/* Query command buffer requirements. */
gceSTATUS
gcoHARDWARE_QueryCommandBuffer(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    OUT gctUINT32 * Alignment,
    OUT gctUINT32 * ReservedHead,
    OUT gctUINT32 * ReservedTail,
    OUT gctUINT32 * ReservedUser,
    OUT gctUINT32 * MGPUModeSwitchBytes
    );

/* Select a graphics pipe. */
gceSTATUS
gcoHARDWARE_SelectPipe(
    IN gcoHARDWARE Hardware,
    IN gcePIPE_SELECT Pipe,
    INOUT gctPOINTER *Memory
    );

/* Flush the current graphics pipe. */
gceSTATUS
gcoHARDWARE_FlushPipe(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

/* Invalidate the cache. */
gceSTATUS
gcoHARDWARE_InvalidateCache(
    IN gcoHARDWARE Hardware
    );

#if gcdENABLE_3D
/* Set patch ID */
gceSTATUS
gcoHARDWARE_SetPatchID(
    IN gcoHARDWARE Hardware,
    IN gcePATCH_ID   PatchID
    );

/* Get patch ID */
gceSTATUS
gcoHARDWARE_GetPatchID(
    IN gcoHARDWARE Hardware,
    OUT gcePATCH_ID *PatchID
    );

/* Send semaphore down the current pipe. */
gceSTATUS
gcoHARDWARE_Semaphore(
    IN gcoHARDWARE Hardware,
    IN gceWHERE From,
    IN gceWHERE To,
    IN gceHOW How,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_SnapPages(
    IN gcoHARDWARE Hardware,
    IN gcePROGRAM_STAGE_BIT Stages,
    INOUT gctPOINTER *Memory
    );

/* Load one ctrl state, which will not be put in state buffer */
gceSTATUS
gcoHARDWARE_LoadCtrlState(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    );

gceSTATUS
gcoHARDWARE_LoadCtrlStateNEW(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_LoadCtrlStateNEW_GPU0(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_McfeSemapore(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 SemaHandle,
    IN gctBOOL SendSema,
    INOUT gctPOINTER *Memory
    );

/* Load a number of load states in fixed-point (3D pipe). */
gceSTATUS
gcoHARDWARE_LoadStateX(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Count,
    IN gctPOINTER States
    );

gceSTATUS
gcoHARDWARE_McfeSubmitJob(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER *Memory
    );
#endif

/* Load a number of load states. */
gceSTATUS
gcoHARDWARE_LoadState(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Count,
    IN gctPOINTER States
    );

/* Commit the current command buffer. */
gceSTATUS
gcoHARDWARE_Commit(
    IN gcoHARDWARE Hardware
    );

/* Stall the pipe. */
gceSTATUS
gcoHARDWARE_Stall(
    IN gcoHARDWARE Hardware
    );

#if gcdENABLE_3D
gceSTATUS
gcoHARDWARE_SetChipEnable(
    IN gcoHARDWARE Hardware,
    IN gceCORE_3D_MASK ChipEnable
    );

gceSTATUS
gcoHARDWARE_GetChipEnable(
    IN gcoHARDWARE Hardware,
    OUT gceCORE_3D_MASK *ChipEnable
    );

gceSTATUS
gcoHARDWARE_SetMultiGPUMode(
    IN gcoHARDWARE Hardware,
    IN gceMULTI_GPU_MODE MultiGPUMode
    );

gceSTATUS
gcoHARDWARE_GetMultiGPUMode(
    IN gcoHARDWARE Hardware,
    OUT gceMULTI_GPU_MODE *MultiGPUMode
    );

gceSTATUS
gcoHARDWARE_MultiGPUSync(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32_PTR *Memory
);

gceSTATUS
gcoHARDWARE_MultiGPUCacheFlush(
    IN gcoHARDWARE Hardware,
    INOUT gctUINT32_PTR *Memory
    );

gceSTATUS
gcoHARDWARE_QueryReset(
    IN gcoHARDWARE Hardware,
    OUT gctBOOL_PTR Innocent
    );

/* Load the vertex and pixel shaders. */
gceSTATUS
gcoHARDWARE_LoadProgram(
    IN gcoHARDWARE Hardware,
    IN gcePROGRAM_STAGE_BIT StageBits,
    IN gctPOINTER ProgramState
    );

/* Resolve. */
gceSTATUS
gcoHARDWARE_ResolveRect(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args
    );

gceSTATUS
gcoHARDWARE_IsHWResolveable(
    IN gcoSURF      SrcSurf,
    IN gcoSURF      DstSurf,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DstOrigin,
    IN gcsPOINT_PTR RectSize
    );

/* Resolve depth buffer. */
gceSTATUS
gcoHARDWARE_ResolveDepth(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args
    );

/* Preserve pixels from source surface. */
gceSTATUS
gcoHARDWARE_PreserveRects(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsRECT Rects[],
    IN gctUINT RectCount
    );

gceSTATUS
gcoHARDWARE_SelectChannel(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Priority,
    IN gctUINT32 ChannelId
    );

gceSTATUS
gcoHARDWARE_GetChannelInfo(
    IN gcoHARDWARE Hardware,
    OUT gctBOOL *Priority,
    OUT gctUINT32 *ChannelId
    );

gceSTATUS
gcoHARDWARE_AllocateMcfeSemaphore(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32 * SemaHandle
    );

gceSTATUS
gcoHARDWARE_FreeMcfeSemaphore(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 SemaHandle
    );

#endif /* gcdENABLE_3D */

/* Query tile sizes. */
gceSTATUS
gcoHARDWARE_QueryTileSize(
    OUT gctINT32 * TileWidth2D,
    OUT gctINT32 * TileHeight2D,
    OUT gctINT32 * TileWidth3D,
    OUT gctINT32 * TileHeight3D,
    OUT gctUINT32 * StrideAlignment
    );

#if gcdENABLE_3D

/* Get tile status sizes for a surface. */
gceSTATUS
gcoHARDWARE_QueryTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctSIZE_T Bytes,
    OUT gctSIZE_T_PTR TsSize,
    OUT gctUINT_PTR Alignment,
    OUT gctUINT32_PTR Filler
    );

/* Get Hz tile status for a surface. */
gceSTATUS
gcoHARDWARE_QueryHzTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctSIZE_T Bytes,
    OUT gctSIZE_T_PTR TsSize,
    OUT gctUINT_PTR Alignment
    );

/* Disable (Turn off) tile status hardware programming. */
gceSTATUS
gcoHARDWARE_DisableHardwareTileStatus(
    IN gcoHARDWARE Hardware,
    IN gceTILESTATUS_TYPE Type,
    IN gctUINT  RtIndex
    );

/*
 * Enable (turn on) tile status for a surface.
 *
 * If surface tileStatusDisabled or no tile status buffer
 *   then DISABLE (turn off) hardware tile status
 * Else
 *   If surface is current
 *     then ENABLE (turn on) hardware tile status on surface
 *   Else
 *     do not need turn on, skip
 */
gceSTATUS
gcoHARDWARE_EnableTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *surfView,
    IN gctUINT32 TileStatusAddress,
    IN gcsSURF_NODE_PTR HzTileStatus,
    IN gctUINT  RtIndex
    );

/*
 * Flush and disable (turn off) tile status for a surface.
 * Always uses RT0 if it's RT.
 *
 * If CpuAccess,
 *   then decompress the suface either use resolve or fc fill.
 *   (Auto switch surface to current and restore after decompress)
 * Flush the tile status cache after.
 * DISABLE (turn off) hardware tile status.
 */
gceSTATUS
gcoHARDWARE_DisableTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctBOOL CpuAccess
    );

/*
 * Flush tile status cache.
 *
 * If Decompress,
 *   then decompress the surface either use resolve or fc fill.
 *   (Auto switch surface to current and restore after decompress)
 * FLUSH the tile status cache after.
 */
gceSTATUS
gcoHARDWARE_FlushTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctBOOL Decompress
    );

/*
 * Fill surface from tile status cache.
 * Auto switch current to the surface and restore after.
 */
gceSTATUS
gcoHARDWARE_FillFromTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView
    );

/* Query resolve alignment requirement to resolve this surface */
gceSTATUS gcoHARDWARE_GetSurfaceResolveAlignment(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    OUT gctUINT *originX,
    OUT gctUINT *originY,
    OUT gctUINT *sizeX,
    OUT gctUINT *sizeY
    );

#endif /* gcdENABLE_3D */

/* Lock a surface. */
gceSTATUS
gcoHARDWARE_Lock(
    IN gcsSURF_NODE_PTR Node,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    );

gceSTATUS
gcoHARDWARE_LockAddCpuPhysicalAddr(
    IN gcsSURF_NODE_PTR Node,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory,
    OUT gctUINT32 * CpuPhysicalAddress
);

/* Unlock a surface. */
gceSTATUS
gcoHARDWARE_Unlock(
    IN gcsSURF_NODE_PTR Node,
    IN gceSURF_TYPE Type
    );

gceSTATUS
gcoHARDWARE_LockExAddCpuPhysicalAddr(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE Engine,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory,
    OUT gctUINT32 * CpuPhysicalAddress
    );

gceSTATUS
gcoHARDWARE_LockEx(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE Engine,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    );

gceSTATUS
gcoHARDWARE_UnlockEx(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE Engine,
    IN gceSURF_TYPE Type
    );

/* Call kernel for event. */
gceSTATUS
gcoHARDWARE_CallEvent(
    IN gcoHARDWARE Hardware,
    IN OUT gcsHAL_INTERFACE * Interface
    );

gceSTATUS
gcoHARDWARE_QueryQueuedMaxUnlockBytes(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT_PTR Bytes
    );

/* Schedule destruction for the specified video memory node. */
gceSTATUS
gcoHARDWARE_ScheduleVideoMemory(
    IN gctUINT32 Node
    );

/* Allocate a temporary surface with specified parameters. */
gceSTATUS
gcoHARDWARE_AllocTmpSurface(
    IN gcoHARDWARE Hardware,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gcsSURF_FORMAT_INFO_PTR Format,
    IN gceSURF_TYPE Type,
    IN gceSURF_TYPE Hints
    );

/* Free the temporary surface. */
gceSTATUS
gcoHARDWARE_FreeTmpSurface(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Synchronized
    );

/* Get a 2D temporary surface with specified parameters. */
gceSTATUS
gcoHARDWARE_Get2DTempSurface(
    IN gcoHARDWARE Hardware,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gceSURF_FORMAT Format,
    IN gceSURF_TYPE Hints,
    OUT gcoSURF *Surface
    );

/* Put back the 2D temporary surface from gcoHARDWARE_Get2DTempSurface. */
gceSTATUS
gcoHARDWARE_Put2DTempSurface(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface
    );

#if gcdENABLE_3D
/* Copy a rectangular area with format conversion. */
gceSTATUS
gcoHARDWARE_CopyPixels(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args
    );

/* Enable or disable anti-aliasing. */
gceSTATUS
gcoHARDWARE_SetAntiAlias(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

/* Set msaa samples. */
gceSTATUS
gcoHARDWARE_SetSamples(
    IN gcoHARDWARE Hardware,
    IN gcsSAMPLES SampleInfo
    );


/* Write data into the command buffer. */
gceSTATUS
gcoHARDWARE_WriteBuffer(
    IN gcoHARDWARE Hardware,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    );
#endif /* gcdENABLE_3D */

/* Convert RGB8 color value to YUV color space. */
void gcoHARDWARE_RGB2YUV(
    gctUINT8 R,
    gctUINT8 G,
    gctUINT8 B,
    gctUINT8_PTR Y,
    gctUINT8_PTR U,
    gctUINT8_PTR V
    );

/* Convert YUV color value to RGB8 color space. */
void gcoHARDWARE_YUV2RGB(
    gctUINT8 Y,
    gctUINT8 U,
    gctUINT8 V,
    gctUINT8_PTR R,
    gctUINT8_PTR G,
    gctUINT8_PTR B
    );

/* Convert an API format. */
gceSTATUS
gcoHARDWARE_ConvertFormat(
    IN gceSURF_FORMAT Format,
    OUT gctUINT32 * BitsPerPixel,
    OUT gctUINT32 * BytesPerTile
    );

/* Align size to tile boundary. */
gceSTATUS
gcoHARDWARE_AlignToTile(
    IN gcoHARDWARE Hardware,
    IN gceSURF_TYPE Type,
    IN gceSURF_TYPE Hint,
    IN gceSURF_FORMAT Format,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height,
    IN gctUINT32 Depth,
    OUT gceTILING * Tiling,
    OUT gctBOOL_PTR SuperTiled,
    OUT gceSURF_ALIGNMENT *hAlignment
    );

/* Align size compatible for all core in hardware. */
gceSTATUS
gcoHARDWARE_AlignToTileCompatible(
    IN gcoHARDWARE Hardware,
    IN gceSURF_TYPE Type,
    IN gceSURF_TYPE Hint,
    IN gceSURF_FORMAT Format,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height,
    IN gctUINT32 Depth,
    OUT gceTILING * Tiling,
    OUT gctBOOL_PTR SuperTiled,
    OUT gceSURF_ALIGNMENT *hAlignment
    );

gceSTATUS
gcoHARDWARE_AlignResolveRect(
    IN gcoSURF Surface,
    IN gcsPOINT_PTR RectOrigin,
    IN gcsPOINT_PTR RectSize,
    OUT gcsPOINT_PTR AlignedOrigin,
    OUT gcsPOINT_PTR AlignedSize
    );



gceSTATUS
gcoHARDWARE_SetDepthOnly(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

/*----------------------------------------------------------------------------*/
/*------------------------------- gcoHARDWARE 2D ------------------------------*/

/* Verifies whether 2D engine is available. */
gceSTATUS
gcoHARDWARE_Is2DAvailable(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_Query2DSurfaceAllocationInfo(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN OUT gctUINT* Bytes,
    IN OUT gctUINT* Alignment
    );

/* Check whether primitive restart is enabled. */
gceSTATUS
gcoHARDWARE_IsPrimitiveRestart(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_Query3DCoreCount(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32 *Count
    );

gceSTATUS
gcoHARDWARE_QueryCoreIndex(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 CoreID,
    OUT gctUINT32 *CoreIndex
    );

gceSTATUS
gcoHARDWARE_QueryCluster(
    IN gcoHARDWARE Hardware,
    OUT gctINT32   *ClusterMinID,
    OUT gctINT32   *ClusterMaxID,
    OUT gctUINT32  *ClusterCount,
    OUT gctUINT32  *ClusterIDWidth
    );

gceSTATUS
gcoHARDWARE_IsFlatMapped(
    IN gcoHARDWARE Hardware,
    IN gctPHYS_ADDR_T Address
    );

gceSTATUS
gcoHARDWARE_QuerySuperTileMode(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32_PTR SuperTileMode
    );

gceSTATUS
gcoHARDWARE_QueryChipAxiBusWidth(
    IN gcoHARDWARE Hardware,
    OUT gctBOOL *AXI128Bits
    );

gceSTATUS
gcoHARDWARE_QueryMultiGPUSyncLength(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32_PTR Bytes
    );

gceSTATUS
gcoHARDWARE_QueryMultiGPUCacheFlushLength(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32_PTR Bytes
    );

/* Translate API destination color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateDestinationFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT APIValue,
    IN gctBOOL EnableXRGB,
    OUT gctUINT32* HwValue,
    OUT gctUINT32* HwSwizzleValue,
    OUT gctUINT32* HwIsYUVValue
    );

#if gcdENABLE_3D && gcdUSE_VX
gceSTATUS
gcoHARDWARE_QueryNNConfig(
    IN gcoHARDWARE Hardware,
    OUT gctPOINTER Config
    );

gceSTATUS
gcoHARDWARE_QueryHwChipInfo(
    IN  gcoHARDWARE  Hardware,
    OUT gctUINT32   *customerID,
    OUT gctUINT32   *ecoID
    );
#endif

#if gcdENABLE_3D
gceSTATUS
gcoHARDWARE_SetStream(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Index,
    IN gctUINT32 Address,
    IN gctUINT32 Stride
    );

gceSTATUS
gcoHARDWARE_SetAttributes(
    IN gcoHARDWARE Hardware,
    IN gcsVERTEX_ATTRIBUTES_PTR Attributes,
    IN gctUINT32 AttributeCount
    );

/*----------------------------------------------------------------------------*/
/*----------------------- gcoHARDWARE Fragment Processor ---------------------*/

/* Set the fragment processor configuration. */
gceSTATUS
gcoHARDWARE_SetFragmentConfiguration(
    IN gcoHARDWARE Hardware,
    IN gctBOOL ColorFromStream,
    IN gctBOOL EnableFog,
    IN gctBOOL EnableSmoothPoint,
    IN gctUINT32 ClipPlanes
    );

/* Enable/disable texture stage operation. */
gceSTATUS
gcoHARDWARE_EnableTextureStage(
    IN gcoHARDWARE Hardware,
    IN gctINT Stage,
    IN gctBOOL Enable
    );

/* Program the channel enable masks for the color texture function. */
gceSTATUS
gcoHARDWARE_SetTextureColorMask(
    IN gcoHARDWARE Hardware,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    );

/* Program the channel enable masks for the alpha texture function. */
gceSTATUS
gcoHARDWARE_SetTextureAlphaMask(
    IN gcoHARDWARE Hardware,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    );

/* Program the constant fragment color. */
gceSTATUS
gcoHARDWARE_SetFragmentColorX(
    IN gcoHARDWARE Hardware,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

gceSTATUS
gcoHARDWARE_SetFragmentColorF(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Program the constant fog color. */
gceSTATUS
gcoHARDWARE_SetFogColorX(
    IN gcoHARDWARE Hardware,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

gceSTATUS
gcoHARDWARE_SetFogColorF(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Program the constant texture color. */
gceSTATUS
gcoHARDWARE_SetTetxureColorX(
    IN gcoHARDWARE Hardware,
    IN gctINT Stage,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

gceSTATUS
gcoHARDWARE_SetTetxureColorF(
    IN gcoHARDWARE Hardware,
    IN gctINT Stage,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Configure color texture function. */
gceSTATUS
gcoHARDWARE_SetColorTextureFunction(
    IN gcoHARDWARE Hardware,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    );

/* Configure alpha texture function. */
gceSTATUS
gcoHARDWARE_SetAlphaTextureFunction(
    IN gcoHARDWARE Hardware,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    );

/* Flush the evrtex caches. */
gceSTATUS
gcoHARDWARE_FlushVertex(
    IN gcoHARDWARE Hardware
    );

#if gcdUSE_WCLIP_PATCH
gceSTATUS
gcoHARDWARE_SetWClipEnable(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetWPlaneLimit(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT Value
    );
#endif

gceSTATUS
gcoHARDWARE_SetRTNERounding(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

/* Draw a number of primitives. */
gceSTATUS
gcoHARDWARE_DrawPrimitives(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE Type,
    IN gctINT StartVertex,
    IN gctSIZE_T PrimitiveCount
    );

gceSTATUS
gcoHARDWARE_DrawIndirectPrimitives(
    IN gcoHARDWARE Hardware,
    IN gctBOOL DrawIndex,
    IN gcePRIMITIVE Type,
    IN gctUINT ParamAddress
    );

gceSTATUS
gcoHARDWARE_MultiDrawIndirectPrimitives(
    IN gcoHARDWARE Hardware,
    IN gctBOOL DrawIndex,
    IN gcePRIMITIVE Type,
    IN gctINT DrawCount,
    IN gctINT Stride,
    IN gctUINT ParamAddress
    );

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
    );

gceSTATUS
gcoHARDWARE_DrawNullPrimitives(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_DrawPrimitivesCount(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE Type,
    IN gctINT* StartVertex,
    IN gctSIZE_T* VertexCount,
    IN gctSIZE_T PrimitiveCount
    );

/* Draw a number of primitives using offsets. */
gceSTATUS
gcoHARDWARE_DrawPrimitivesOffset(
    IN gcePRIMITIVE Type,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    );

/* Draw a number of indexed primitives. */
gceSTATUS
gcoHARDWARE_DrawIndexedPrimitives(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE Type,
    IN gctINT BaseVertex,
    IN gctINT StartIndex,
    IN gctSIZE_T PrimitiveCount
    );


/* Draw a number of indexed primitives using offsets. */
gceSTATUS
gcoHARDWARE_DrawIndexedPrimitivesOffset(
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    );

/* Draw elements from a pattern */
gceSTATUS
gcoHARDWARE_DrawPattern(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo
    );

/* Flush the texture cache. */
gceSTATUS
gcoHARDWARE_FlushTexture(
    IN gcoHARDWARE Hardware,
    IN gctBOOL vsUsed
    );

/* Disable a specific texture sampler. */
gceSTATUS
gcoHARDWARE_DisableTextureSampler(
    IN gcoHARDWARE Hardware,
    IN gctINT Sampler,
    IN gctBOOL DefaultInteger
    );

/* Set sampler registers. */
gceSTATUS
gcoHARDWARE_BindTexture(
    IN gcoHARDWARE Hardware,
    IN gctINT Sampler,
    IN gcsSAMPLER_PTR SamplerInfo
    );

gceSTATUS
gcoHARDWARE_BindTextureDesc(
    IN gcoHARDWARE Hardware,
    IN gctINT Sampler,
    IN gcsSAMPLER_PTR SamplerInfo
    );


/* Bind texture tile status setting */
gceSTATUS
gcoHARDWARE_BindTextureTS(
    IN gcoHARDWARE Hardware
    );

/* Query the index capabilities. */
gceSTATUS
gcoHARDWARE_QueryIndexCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctBOOL * Index8,
    OUT gctBOOL * Index16,
    OUT gctBOOL * Index32,
    OUT gctUINT * MaxIndex
    );

/* Query the texture capabilities. */
gceSTATUS
gcoHARDWARE_QueryTextureCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers,
    OUT gctUINT * MaxAnisoValue
    );

/* Query the shader support. */
gceSTATUS
gcoHARDWARE_QueryShaderCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT * UnifiedUniforms,
    OUT gctUINT * VertUniforms,
    OUT gctUINT * FragUniforms,
    OUT gctUINT * Varyings,
    OUT gctUINT * ShaderCoreCount,
    OUT gctUINT * ThreadCount,
    OUT gctUINT * VertInstructionCount,
    OUT gctUINT * FragInstructionCount
    );

/* Query the shader support. */
gceSTATUS
gcoHARDWARE_QueryShaderCapsEx(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT64 * LocalMemSize,
    OUT gctUINT * AddressBits,
    OUT gctUINT * GlobalMemCachelineSize,
    OUT gctUINT * GlobalMemCacheSize,
    OUT gctUINT * ClockFrequency
    );

/* Query the texture support. */
gceSTATUS
gcoHARDWARE_QueryTexture(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT Format,
    IN gceTILING Tiling,
    IN gctUINT Level,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    OUT gctUINT * WidthAlignment,
    OUT gctUINT * HeightAlignment
    );

/* Query the stream capabilities. */
gceSTATUS
gcoHARDWARE_QueryStreamCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT32 * MaxAttributes,
    OUT gctUINT32 * MaxStreamStride,
    OUT gctUINT32 * NumberOfStreams,
    OUT gctUINT32 * Alignment,
    OUT gctUINT32 * MaxAttribOffset
    );

gceSTATUS
gcoHARDWARE_GetClosestTextureFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT InFormat,
    IN gceTEXTURE_TYPE texType,
    OUT gceSURF_FORMAT* OutFormat
    );

gceSTATUS
gcoHARDWARE_GetClosestRenderFormat(
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    );

/* Upload data into a texture. */
gceSTATUS
gcoHARDWARE_UploadTexture(
    IN gcsSURF_VIEW *DstView,
    IN gctUINT32 Offset,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    );

gceSTATUS
gcoHARDWARE_UploadTextureYUV(
    IN gceSURF_FORMAT TargetFormat,
    IN gctUINT32 Address,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctPOINTER Memory[3],
    IN gctINT SourceStride[3],
    IN gceSURF_FORMAT SourceFormat
    );

gceSTATUS
gcoHARDWARE_UploadCompressedTexture(
    IN gcoSURF DstSurf,
    IN gctCONST_POINTER Logical,
    IN gctUINT32 Offset,
    IN gctUINT32 XOffset,
    IN gctUINT32 YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT ImageSize
    );

gceSTATUS
gcoHARDWARE_ProgramTexture(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_ProgramTextureDesc(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );


gceSTATUS
gcoHARDWARE_UpdateTextureDesc(
    gcoHARDWARE Hardware,
    gcsTEXTURE_PTR TexParam,
    gcsTXDESC_UPDATE_INFO_PTR UpdateInfo
    );

/* Query if a surface is renderable or not. */
gceSTATUS
gcoHARDWARE_QuerySurfaceRenderable(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface
    );

#endif /* gcdENABLE_3D */

/* Query the target capabilities. */
gceSTATUS
gcoHARDWARE_QueryTargetCaps(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MultiTargetCount,
    OUT gctUINT * MaxSamples
    );

gceSTATUS
gcoHARDWARE_QueryFormat(
    IN gceSURF_FORMAT Format,
    OUT gcsSURF_FORMAT_INFO_PTR * Info
    );

gceSTATUS
gcoHARDWARE_QueryBPP(
    IN gceSURF_FORMAT Format,
    OUT gctFLOAT_PTR BppArray
    );

/* Copy data into video memory. */
gceSTATUS
gcoHARDWARE_CopyData(
    IN gcsSURF_NODE_PTR Memory,
    IN gctSIZE_T Offset,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    );

/* Sets the software 2D renderer force flag. */
gceSTATUS
gcoHARDWARE_UseSoftware2D(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

/* Clear one or more rectangular areas. */
gceSTATUS
gcoHARDWARE_Clear2D(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gctUINT32 RectCount,
    IN gcsRECT_PTR Rect
    );

/* Draw one or more Bresenham lines using solid color(s). */
gceSTATUS
gcoHARDWARE_Line2DEx(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gctUINT32 LineCount,
    IN gcsRECT_PTR Position,
    IN gctUINT32 ColorCount,
    IN gctUINT32_PTR Color32
    );

/* Determines the usage of 2D resources (source/pattern/destination). */
void
gcoHARDWARE_Get2DResourceUsage(
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gce2D_TRANSPARENCY Transparency,
    OUT gctBOOL_PTR UseSource,
    OUT gctBOOL_PTR UsePattern,
    OUT gctBOOL_PTR UseDestination
    );

/* Translate SURF API transparency mode to PE 2.0 transparency values. */
gceSTATUS
gcoHARDWARE_TranslateSurfTransparency(
    IN gceSURF_TRANSPARENCY APIValue,
    OUT gce2D_TRANSPARENCY* srcTransparency,
    OUT gce2D_TRANSPARENCY* dstTransparency,
    OUT gce2D_TRANSPARENCY* patTransparency
    );

gceSTATUS
gcoHARDWARE_ColorPackToARGB8(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Color,
    OUT gctUINT32_PTR Color32
    );

gceSTATUS
gcoHARDWARE_SetDither(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

/* Calculate stretch factor. */
gctUINT32
gcoHARDWARE_GetStretchFactor(
    IN gctBOOL GdiStretch,
    IN gctINT32 SrcSize,
    IN gctINT32 DestSize
    );

/* Calculate the stretch factors. */
gceSTATUS
gcoHARDWARE_GetStretchFactors(
    IN gctBOOL GdiStretch,
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DestRect,
    OUT gctUINT32 * HorFactor,
    OUT gctUINT32 * VerFactor
    );

/* Start a DE command. */
gceSTATUS
gcoHARDWARE_StartDE(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gce2D_COMMAND Command,
    IN gctUINT32 SrcRectCount,
    IN gcsRECT_PTR SrcRect,
    IN gctUINT32 DestRectCount,
    IN gcsRECT_PTR DestRect
    );

/* Start a DE command to draw one or more Lines,
   with a common or individual color. */
gceSTATUS
gcoHARDWARE_StartDELine(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gce2D_COMMAND Command,
    IN gctUINT32 RectCount,
    IN gcsRECT_PTR DestRect,
    IN gctUINT32 ColorCount,
    IN gctUINT32_PTR Color32
    );

/* Start a DE command with a monochrome stream. */
gceSTATUS
gcoHARDWARE_StartDEStream(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gcsRECT_PTR DestRect,
    IN gctUINT32 StreamSize,
    OUT gctPOINTER * StreamBits
    );

/* Frees the temporary buffer allocated by filter blit operation. */
gceSTATUS
gcoHARDWARE_FreeFilterBuffer(
    IN gcoHARDWARE Hardware
    );

/* Split filter blit in specific case. */
gceSTATUS
gcoHARDWARE_SplitFilterBlit(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf,
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DstRect,
    IN gcsRECT_PTR DestSubRect
    );

/* Filter blit. */
gceSTATUS
gcoHARDWARE_FilterBlit(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf,
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DstRect,
    IN gcsRECT_PTR DstSubRect
    );

gceSTATUS gcoHARDWARE_SplitYUVFilterBlit(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf,
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DstRect,
    IN gcsRECT_PTR DstSubRect
    );

gceSTATUS gcoHARDWARE_MultiPlanarYUVConvert(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf,
    IN gcsRECT_PTR DstRect
    );

/* Monochrome blit. */
gceSTATUS
gcoHARDWARE_MonoBlit(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gctPOINTER StreamBits,
    IN gcsPOINT_PTR StreamSize,
    IN gcsRECT_PTR StreamRect,
    IN gceSURF_MONOPACK SrcStreamPack,
    IN gceSURF_MONOPACK DestStreamPack,
    IN gcsRECT_PTR DestRect
    );

/* Generic blit. */
gceSTATUS
gcoHARDWARE_Blit(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gctUINT32 SrcRectCount,
    IN gcsRECT_PTR SrcRect,
    IN gctUINT32 DstRectCount,
    IN gcsRECT_PTR DstRect
    );

/* Set the GPU clock cycles, after which the idle 2D engine
   will trigger a flush. */
gceSTATUS
gcoHARDWARE_SetAutoFlushCycles(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Cycles
    );

gceSTATUS
gcoHARDWARE_Set2DHardware(
     IN gcoHARDWARE Hardware
     );

gceSTATUS
gcoHARDWARE_Get2DHardware(
     OUT gcoHARDWARE * Hardware
     );

/*----------------------------------------------------------------------------*/
/*------------------------------- gcoHARDWARE DEC ------------------------------*/

gceSTATUS
gcoDECHARDWARE_CheckSurface(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface
    );

gceSTATUS
gcoDECHARDWARE_QueryStateCmdLen(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gce2D_COMMAND Command,
    OUT gctUINT32* Size
    );

gceSTATUS
gcoDECHARDWARE_EnableDECCompression(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoDECHARDWARE_SetSrcDECCompression(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gce2D_TILE_STATUS_CONFIG TileStatusConfig,
    IN gctUINT32 ReadId,
    IN gctBOOL FastClear,
    IN gctUINT32 ClearValue
    );

gceSTATUS
gcoDECHARDWARE_SetDstDECCompression(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gce2D_TILE_STATUS_CONFIG TileStatusConfig,
    IN gctUINT32 ReadId,
    IN gctUINT32 WriteId
    );

gceSTATUS
gcoDECHARDWARE_FlushDECCompression(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Flush,
    IN gctBOOL Wait
    );

#define gcmHAS2DCOMPRESSION(Surface) \
    ((Surface)->tileStatusConfig == gcv2D_TSC_2D_COMPRESSED)

#define gcmHASOTHERCOMPRESSION(Surface) \
    ( \
        ((Surface)->tileStatusConfig & gcv2D_TSC_TPC_COMPRESSED) \
     || ((Surface)->tileStatusConfig & gcv2D_TSC_DEC_COMPRESSED) \
     || ((Surface)->tileStatusConfig & gcv2D_TSC_V4_COMPRESSED) \
    )

#define gcmHASOTHERCOMPRESSIONNO3D(Surface) \
    ( \
        ((Surface)->tileStatusConfig & gcv2D_TSC_TPC_COMPRESSED) \
     || (((Surface)->tileStatusConfig & gcv2D_TSC_DEC_COMPRESSED) && \
          !((Surface)->tileStatusConfig & (gcv2D_TSC_ENABLE | gcv2D_TSC_COMPRESSED | gcv2D_TSC_DOWN_SAMPLER | gcv2D_TSC_V4_COMPRESSED))) \
    )

#define gcmHASCOMPRESSION(Surface) \
    ( \
        gcmHAS2DCOMPRESSION(Surface) \
     || gcmHASOTHERCOMPRESSION(Surface) \
    )

#if gcdENABLE_3D
/*----------------------------------------------------------------------------*/
/*------------------------------- gcoHARDWARE 3D ------------------------------*/

typedef struct _gcs3DBLIT_INFO * gcs3DBLIT_INFO_PTR;
typedef struct _gcs3DBLIT_INFO
{
    gctUINT32    destAddress;
    gctUINT32    destTileStatusAddress;
    gctUINT32    clearValue[2];
    gctUINT32    fcClearValue[2];
    gctUINT32    clearBitMask;
    gctUINT32    clearBitMaskUpper;
    gcsPOINT_PTR origin;
    gcsPOINT_PTR rect;

    gcoSURF      LODs[14];
    gctUINT32    LODsSliceSize[14];
}
gcs3DBLIT_INFO;

gceSTATUS
gcoHARDWARE_BindIndex(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 EndAddress,
    IN gceINDEX_TYPE IndexType,
    IN gctSIZE_T Bytes,
    IN gctUINT   RestartElement
    );

/* Initialize CL hardware. */
gceSTATUS
gcoHARDWARE_InitializeCL(
    IN gcoHARDWARE Hardware
    );

/* Initialize the 3D hardware. */
gceSTATUS
gcoHARDWARE_Initialize3D(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_SetPSOutputMapping(
    IN gcoHARDWARE Hardware,
    IN gctINT32 * psOutputMapping
    );

gceSTATUS
gcoHARDWARE_SetRenderLayered(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable,
    IN gctUINT  MaxLayers
    );

gceSTATUS
gcoHARDWARE_SetShaderLayered(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_IsProgramSwitched(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_SetRenderTarget(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 TargetIndex,
    IN gcoSURF Surface,
    IN gctUINT32 SliceIndex,
    IN gctUINT32 SliceNUm,
    IN gctUINT32 LayerIndex
    );

gceSTATUS
gcoHARDWARE_SetDepthBuffer(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctUINT32 SliceIndex,
    IN gctUINT32 SliceNum
    );

gceSTATUS
gcoHARDWARE_SetAPI(
    IN gcoHARDWARE Hardware,
    IN gceAPI Api
    );

gceSTATUS
gcoHARDWARE_GetAPI(
    IN gcoHARDWARE Hardware,
    OUT gceAPI * CurrentApi,
    OUT gceAPI * Api
    );

/* Resolve(3d) clear. */
gceSTATUS
gcoHARDWARE_Clear(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SurfView,
    IN gctUINT32 LayerIndex,
    IN gcsRECT_PTR Rect,
    IN gctUINT32 ClearValue,
    IN gctUINT32 ClearValueUpper,
    IN gctUINT8 ClearMask
    );

/* Software clear. */
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
    );

/* Append a TILE STATUS CLEAR command to a command queue. */
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
    );

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
    );

gceSTATUS
gcoHARDWARE_ComputeClearWindow(
    IN gcoHARDWARE Hardware,
    IN gctSIZE_T Bytes,
    gctUINT32 *Width,
    gctUINT32 *Height
    );

gceSTATUS
gcoHARDWARE_SetViewport(
    IN gcoHARDWARE Hardware,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    );

gceSTATUS
gcoHARDWARE_SetScissors(
    IN gcoHARDWARE Hardware,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    );

gceSTATUS
gcoHARDWARE_SetShading(
    IN gcoHARDWARE Hardware,
    IN gceSHADING Shading
    );

gceSTATUS
gcoHARDWARE_SetBlendEnable(
    IN gcoHARDWARE Hardware,
    IN gctUINT  Index,
    IN gctBOOL Enabled
    );

gceSTATUS
gcoHARDWARE_SetBlendFunctionSource(
    IN gcoHARDWARE Hardware,
    IN gctUINT  Index,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    );

gceSTATUS
gcoHARDWARE_SetBlendFunctionTarget(
    IN gcoHARDWARE Hardware,
    IN gctUINT  Index,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    );

gceSTATUS
gcoHARDWARE_SetBlendMode(
    IN gcoHARDWARE Hardware,
    IN gctUINT  Index,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    );

gceSTATUS
gcoHARDWARE_SetBlendColor(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    );

gceSTATUS
gcoHARDWARE_SetBlendColorX(
    IN gcoHARDWARE Hardware,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

gceSTATUS
gcoHARDWARE_SetBlendColorF(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

gceSTATUS
gcoHARDWARE_SetCulling(
    IN gcoHARDWARE Hardware,
    IN gceCULL Mode
    );

gceSTATUS
gcoHARDWARE_SetPointSizeEnable(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetPointSprite(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetPrimitiveIdEnable(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetFill(
    IN gcoHARDWARE Hardware,
    IN gceFILL Mode
    );

gceSTATUS
gcoHARDWARE_SetDepthCompare(
    IN gcoHARDWARE Hardware,
    IN gceCOMPARE DepthCompare
    );

gceSTATUS
gcoHARDWARE_SetDepthWrite(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetDepthMode(
    IN gcoHARDWARE Hardware,
    IN gceDEPTH_MODE DepthMode
    );

gceSTATUS
gcoHARDWARE_SetDepthRangeX(
    IN gcoHARDWARE Hardware,
    IN gceDEPTH_MODE DepthMode,
    IN gctFIXED_POINT Near,
    IN gctFIXED_POINT Far
    );

gceSTATUS
gcoHARDWARE_SetDepthRangeF(
    IN gcoHARDWARE Hardware,
    IN gceDEPTH_MODE DepthMode,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    );

gceSTATUS
gcoHARDWARE_SetDepthScaleBiasX(
    IN gcoHARDWARE Hardware,
    IN gctFIXED_POINT DepthScale,
    IN gctFIXED_POINT DepthBias
    );

gceSTATUS
gcoHARDWARE_SetDepthScaleBiasF(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT DepthScale,
    IN gctFLOAT DepthBias
    );

gceSTATUS
gcoHARDWARE_SetDepthPlaneF(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    );

gceSTATUS
gcoHARDWARE_SetColorWrite(
    IN gcoHARDWARE Hardware,
    IN gctUINT  Index,
    IN gctUINT8 Enable
    );

gceSTATUS
gcoHARDWARE_SetEarlyDepth(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetAllEarlyDepthModes(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Disable
    );

gceSTATUS
gcoHARDWARE_SetEarlyDepthFromAPP(
    IN gcoHARDWARE Hardware,
    IN gctBOOL EarlyDepthFromAPP
    );

gceSTATUS
gcoHARDWARE_SetRADepthWrite(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Disable,
    IN gctBOOL psReadZ,
    IN gctBOOL psReadW
    );

gceSTATUS
gcoHARDWARE_SetPatchVertices(
    IN gcoHARDWARE Hardware,
    IN gctINT PatchVertices
    );


gceSTATUS
gcoHARDWARE_SwitchDynamicEarlyDepthMode(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_DisableDynamicEarlyDepthMode(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Disable
    );

gceSTATUS
gcoHARDWARE_SetStencilMode(
    IN gcoHARDWARE Hardware,
    IN gceSTENCIL_MODE Mode
    );

gceSTATUS
gcoHARDWARE_SetStencilMask(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 Mask
    );

gceSTATUS
gcoHARDWARE_SetStencilMaskBack(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 Mask
    );

gceSTATUS
gcoHARDWARE_SetStencilWriteMask(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 Mask
    );

gceSTATUS
gcoHARDWARE_SetStencilWriteMaskBack(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 Mask
    );

gceSTATUS
gcoHARDWARE_SetStencilReference(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 Reference,
    IN gctBOOL  Front
    );

gceSTATUS
gcoHARDWARE_SetStencilCompare(
    IN gcoHARDWARE Hardware,
    IN gceSTENCIL_WHERE Where,
    IN gceCOMPARE Compare
    );

gceSTATUS
gcoHARDWARE_SetStencilPass(
    IN gcoHARDWARE Hardware,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    );

gceSTATUS
gcoHARDWARE_SetStencilFail(
    IN gcoHARDWARE Hardware,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    );

gceSTATUS
gcoHARDWARE_SetStencilDepthFail(
    IN gcoHARDWARE Hardware,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    );

gceSTATUS
gcoHARDWARE_SetStencilAll(
    IN gcoHARDWARE Hardware,
    IN gcsSTENCIL_INFO_PTR Info
    );

gceSTATUS
gcoHARDWARE_SetAlphaTest(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetAlphaCompare(
    IN gcoHARDWARE Hardware,
    IN gceCOMPARE Compare
    );

gceSTATUS
gcoHARDWARE_SetAlphaReference(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 Reference,
    IN gctFLOAT FloatReference
    );

gceSTATUS
gcoHARDWARE_SetAlphaReferenceX(
    IN gcoHARDWARE Hardware,
    IN gctFIXED_POINT Reference
    );

gceSTATUS
gcoHARDWARE_SetAlphaReferenceF(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT Reference
    );

#if gcdALPHA_KILL_IN_SHADER
gceSTATUS
gcoHARDWARE_SetAlphaKill(
    IN gcoHARDWARE Hardware,
    IN gctBOOL AlphaKill,
    IN gctBOOL ColorKill
    );
#endif

gceSTATUS
gcoHARDWARE_SetAlphaAll(
    IN gcoHARDWARE Hardware,
    IN gcsALPHA_INFO_PTR Info
    );

gceSTATUS
gcoHARDWARE_SetAntiAliasLine(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetAALineTexSlot(
    IN gcoHARDWARE Hardware,
    IN gctUINT TexSlot
    );

gceSTATUS
gcoHARDWARE_SetAALineWidth(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT Width
    );

gceSTATUS
gcoHARDWARE_SetLastPixelEnable(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetLogicOp(
    IN gcoHARDWARE Hardware,
    IN gctUINT8     Rop
    );

gceSTATUS
gcoHARDWARE_SetCentroids(
    IN gcoHARDWARE Hardware,
    IN gctUINT32    Index,
    IN gctPOINTER   Centroids
    );

gceSTATUS
gcoHARDWARE_GetSampleCoords(
    IN gcoHARDWARE Hardware,
    IN gctUINT SampleIndex,
    IN gctBOOL yInverted,
    OUT gctFLOAT_PTR Coords
    );

gceSTATUS
gcoHARDWARE_QuerySamplerBase(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT32 * SamplerCount,
    OUT gctUINT32 * SamplerBase,
    OUT gctUINT32 * TotalCount
    );

gceSTATUS
gcoHARDWARE_QueryUniformBase(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT32 * VertexBase,
    OUT gctUINT32 * FragmentBase
    );

gceSTATUS
gcoHARDWARE_SetQuery(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 QueryHeader,
    IN gceQueryType Type,
    IN gceQueryCmd QueryCmd,
    IN gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_GetQueryIndex(
    IN gcoHARDWARE Hardware,
    IN gceQueryType Type,
    IN gctINT32 * Index
    );

gceSTATUS
gcoHARDWARE_SetColorOutCount(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 ColorOutCount
    );

gceSTATUS
gcoHARDWARE_SetColorCacheMode(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_PrimitiveRestart(
    IN gcoHARDWARE Hardware,
    IN gctBOOL PrimitiveRestart
    );

gceSTATUS
gcoHARDWARE_FlushSHL1Cache(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_EnableAlphaToCoverage(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_EnableSampleCoverage(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetSampleCoverageValue(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT CoverageValue,
    IN gctBOOL Invert
    );

gceSTATUS
gcoHARDWARE_EnableSampleMask(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetSampleMask(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 SampleMask
    );


gceSTATUS
gcoHARDWARE_EnableSampleShading(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_SetMinSampleShadingValue(
    IN gcoHARDWARE Hardware,
    IN gctFLOAT MinSampleShadingValue
    );

gceSTATUS
gcoHARDWARE_SetSampleShading(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable,
    IN gctBOOL IsSampleIn,
    IN gctFLOAT SampleShadingValue
    );

gceSTATUS
gcoHARDWARE_EnableSampleMaskOut(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable,
    IN gctINT  SampleMaskLoc
    );

gceSTATUS
gcoHARDWARE_SetXfbHeader(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 HeaderPhysical
    );

gceSTATUS
gcoHARDWARE_SetXfbBuffer(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Index,
    IN gctUINT32 BufferAddr,
    IN gctUINT32 BufferStride,
    IN gctUINT32 BufferSize
    );

gceSTATUS
gcoHARDWARE_SetXfbCmd(
    IN gcoHARDWARE Hardware,
    IN gceXfbCmd Cmd,
    IN gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_SetRasterDiscard(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHARDWARE_InvokeThreadWalkerCL(
    IN gcoHARDWARE Hardware,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    );

gceSTATUS
gcoHARDWARE_InvokeThreadWalkerGL(
    IN gcoHARDWARE Hardware,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    );


#if gcdSYNC
gceSTATUS
gcoHARDWARE_WaitFence(
    IN gcoHARDWARE Hardware,
    IN gcsSYNC_CONTEXT_PTR Ctx,
    IN gceENGINE From,
    IN gceENGINE On,
    IN gceFENCE_TYPE Type
    );

gceSTATUS
gcoHARDWARE_OnIssueFence( IN gcoHARDWARE Hardware,
                          IN gceENGINE engine);

gceSTATUS
gcoHARDWARE_SendFence(
    IN gcoHARDWARE Hardware,
    IN gctBOOL SyncAndFlush,
    IN gceENGINE engine,
    OUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_GetFence(
    IN gcoHARDWARE Hardware,
    IN gcsSYNC_CONTEXT_PTR *Ctx,
    IN gceENGINE engine,
    IN gceFENCE_TYPE Type
    );

gceSTATUS
gcoHARDWARE_GetFenceEnabled(
    IN gcoHARDWARE Hardware,
    OUT gctBOOL_PTR Enabled
    );

gctBOOL
gcoHARDWARE_IsFenceBack(
    IN gcoHARDWARE Hardware,
    IN gcsSYNC_CONTEXT_PTR Ctx,
    IN gceENGINE Engine,
    IN gceFENCE_TYPE Type
    );

gceSTATUS
gcoHARDWARE_SetFenceEnabled(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enabled
    );

gceSTATUS
gcoHARDWARE_AppendFence(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE Engine,
    IN gceFENCE_TYPE Type
    );
#endif

gceSTATUS
gcoHARDWARE_SetMultiGPURenderingMode(
    IN gcoHARDWARE Hardware,
    IN gceMULTI_GPU_RENDERING_MODE Mode
    );

gceSTATUS
gcoHARDWARE_FlushMultiGPURenderingMode(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER * Memory,
    IN gceMULTI_GPU_RENDERING_MODE mode
    );

gceSTATUS
gcoHARDWARE_3DBlitCopy(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gctUINT32 SrcAddress,
    IN gctUINT32 DestAddress,
    IN gctUINT32 CopySize,
    IN gctBOOL forceSingle
    );

gceSTATUS
gcoHARDWARE_3DBlitClear(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gcsSURF_VIEW *DstView,
    IN gcs3DBLIT_INFO_PTR Info,
    IN gcsPOINT_PTR DstOrigin,
    IN gcsPOINT_PTR RectSize,
    IN gctBOOL forceSingle
    );

gceSTATUS
gcoHARDWARE_3DBlitBlt(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_VIEW *SrcView,
    IN gcsSURF_VIEW *DstView,
    IN gcsSURF_RESOLVE_ARGS *Args,
    IN gctBOOL forceSingle
    );

gceSTATUS
gcoHARDWARE_3DBlitTileFill(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gcsSURF_VIEW *DstView,
    IN gctBOOL forceSingle
    );

gceSTATUS
gcoHARDWARE_3DBlitMipMap(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gcs3DBLIT_INFO_PTR Info,
    IN gctUINT SliceIdx,
    IN gctBOOL sRGBDecode,
    INOUT gctPOINTER * Memory
    );

gceSTATUS
gcoHARDWARE_3DBlit420Tiler(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gcoSURF SrcSurf,
    IN gcoSURF DstSurf,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DstOrigin,
    IN gcsPOINT_PTR RectSize
    );

#if gcdENABLE_3D
gceSTATUS
gcoHARDWARE_GetForceVirtual(
    IN gcoHARDWARE Hardware,
    OUT gctBOOL* bForceVirtual
    );

gceSTATUS
gcoHARDWARE_SetForceVirtual(
    IN gcoHARDWARE Hardware,
    IN gctBOOL bForceVirtual
    );
#endif

#if gcdENABLE_3D && gcdUSE_VX
typedef enum _gceVX_KERNEL
{
    gcvVX_KERNEL_COLOR_CONVERT,
    gcvVX_KERNEL_CHANNEL_EXTRACT,
    gcvVX_KERNEL_CHANNEL_COMBINE,
    gcvVX_KERNEL_SOBEL_3x3,
    gcvVX_KERNEL_MAGNITUDE,
    gcvVX_KERNEL_PHASE,
    gcvVX_KERNEL_SCALE_IMAGE,
    gcvVX_KERNEL_TABLE_LOOKUP,
    gcvVX_KERNEL_HISTOGRAM,
    gcvVX_KERNEL_EQUALIZE_HISTOGRAM,
    gcvVX_KERNEL_ABSDIFF,
    gcvVX_KERNEL_MEAN_STDDEV,
    gcvVX_KERNEL_THRESHOLD,
    gcvVX_KERNEL_INTEGRAL_IMAGE,
    gcvVX_KERNEL_DILATE_3x3,
    gcvVX_KERNEL_ERODE_3x3,
    gcvVX_KERNEL_MEDIAN_3x3,
    gcvVX_KERNEL_BOX_3x3,
    gcvVX_KERNEL_GAUSSIAN_3x3,
    gcvVX_KERNEL_CUSTOM_CONVOLUTION,
    gcvVX_KERNEL_GAUSSIAN_PYRAMID,
    gcvVX_KERNEL_ACCUMULATE,
    gcvVX_KERNEL_ACCUMULATE_WEIGHTED,
    gcvVX_KERNEL_ACCUMULATE_SQUARE,
    gcvVX_KERNEL_MINMAXLOC,
    gcvVX_KERNEL_CONVERTDEPTH,
    gcvVX_KERNEL_CANNY_EDGE_DETECTOR,
    gcvVX_KERNEL_AND,
    gcvVX_KERNEL_OR,
    gcvVX_KERNEL_XOR,
    gcvVX_KERNEL_NOT,
    gcvVX_KERNEL_MULTIPLY,
    gcvVX_KERNEL_ADD,
    gcvVX_KERNEL_SUBTRACT,
    gcvVX_KERNEL_WARP_AFFINE,
    gcvVX_KERNEL_WARP_PERSPECTIVE,
    gcvVX_KERNEL_HARRIS_CORNERS,
    gcvVX_KERNEL_FAST_CORNERS,
    gcvVX_KERNEL_OPTICAL_FLOW_PYR_LK,
    gcvVX_KERNEL_REMAP,
    gcvVX_KERNEL_HALFSCALE_GAUSSIAN,

    gcvVX_KERNEL_SCHARR_3x3,
    gcvVX_KERNEL_ELEMENTWISE_NORM,
    gcvVX_KERNEL_NONMAXSUPPRESSION_CANNY,
    gcvVX_KERNEL_EUCLIDEAN_NONMAXSUPPRESSION,
    gcvVX_KERNEL_EDGE_TRACE,
    gcvVX_KERNEL_IMAGE_LISTER,
    gcvVX_KERNEL_SOBEL_MxN,

    gcvVX_KERNEL_SGM,

    gcvVX_KERNEL_NONLINEAR_FILTER_MIN,
    gcvVX_KERNEL_NONLINEAR_FILTER_MAX,
    gcvVX_KERNEL_NONLINEAR_FILTER_MEDIAN,

    gcvVX_KERNEL_LAPLACIAN_3x3,
    gcvVX_KERNEL_CENSUS_3x3,
    gcvVX_KERNEL_COPY_IMAGE,

    gcvVX_KERNEL_MAXPOOL,
    gcvVX_KERNEL_LRN,

    gcvVX_KERNEL_EUCLIDEAN_NONMAXSUPPRESSION_MAX,
    gcvVX_KERNEL_EUCLIDEAN_NONMAXSUPPRESSION_SORT,
    gcvVX_KERNEL_EUCLIDEAN_NONMAXSUPPRESSION_NONMAX,
}
gceVX_KERNEL;

typedef enum _gceVX_InterPolicy
{
    gcvVX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR,
    gcvVX_INTERPOLATION_TYPE_BILINEAR,
    gcvVX_INTERPOLATION_TYPE_AREA,
}
gceVX_InterPolicy;

typedef enum _gceVX_BorderMode
{
    gcvVX_BORDER_MODE_UNDEFINED,
    gcvVX_BORDER_MODE_CONSTANT,
    gcvVX_BORDER_MODE_REPLACEMENT,
}
gceVX_BorderMode;

typedef enum _gceVX_PatternMode
{
    gcvVX_PARTTERN_MODE_BOX,
    gcvVX_PARTTERN_MODE_CROSS,
    gcvVX_PARTTERN_MODE_DISK,
    gcvVX_PARTTERN_MODE_OTHER,
}
gceVX_PatternMode;

typedef struct _gcoVX_Instruction
{
    gctUINT32 binary[4];
}
gcoVX_Instruction;

typedef struct _gcoVX_Instructions
{

#if GC_VX_ASM
    char * source;
#endif

    gcoVX_Instruction binarys[1024 * 10];
    gctUINT32 count;
    gctUINT32 regs_count;
#if gcdVX_OPTIMIZER
    gctUINT32 physical;
#endif
}
gcoVX_Instructions;
typedef union _gcoVX_Kernel_Context_Reg
{
    gctUINT8 bin8[16];
    gctUINT16 bin16[8];
    gctUINT32 bin32[4];
}
gcoVX_Kernel_Context_Reg;

typedef struct _gcoVX_Kernel_Context_UniformItem
{
    gctUINT32 termConfig;
    gctUINT32 aSelect;
    gctUINT32 aBin[2];
    gctUINT32 bSelect;
    gctUINT32 bBin[2];
    gctUINT32 miscConfig;
    gcoVX_Kernel_Context_Reg bins[2];
}
gcoVX_Kernel_Context_UniformItem;

typedef struct _gcoVX_Kernel_Context_Uniform
{
    gcoVX_Kernel_Context_UniformItem uniform;
    gctUINT32 index;
    gctUINT32 num;
}
gcoVX_Kernel_Context_Uniform;

typedef struct _vx_evis_no_inst_struct
{
    gctBOOL isSet;
    gctBOOL noAbsDiff;
    gctBOOL noBitReplace;
    gctBOOL noMagPhase;
    gctBOOL noDp32;
    gctBOOL noFilter;
    gctBOOL noBoxFilter;
    gctBOOL noIAdd;
    gctBOOL noSelectAdd;
    gctBOOL clamp8Output;
    gctBOOL lerp7Output;
    gctBOOL accsq8Output;
    gctBOOL noBilinear;
    gctBOOL isVX2;  /* it is for evis 2*/
    gctBOOL supportEVIS; /* it is for evis 1 */
}
vx_evis_no_inst_s;

typedef struct _vx_nn_config
{
    gctBOOL  isSet;
    gcsNN_FIXED_FEATURE      fixedFeature;
    gcsNN_CUSTOMIZED_FEATURE customizedFeature;
    gcsNN_UNIFIED_FEATURE    unifiedFeature;
    gcsNN_DERIVED_FEATURE    derivedFeature;
}
vx_nn_config;

typedef struct _vx_hw_chip_info
{
    gctUINT32                customerID;
    gctUINT32                ecoID;
}
vx_hw_chip_info;

typedef struct _vx_drv_option
{
    gctUINT enableTP;
    gctUINT enableMultiTP;
    gctUINT8_PTR flagTPFunc;
    gctUINT_PTR  typeTPFunc;
    gctUINT enableSRAM;
    gctUINT enableSramStreamMode;
    gctUINT enableCNNPerf;
    gctUINT enableBrickMode;
    gctUINT enableNonZeroBalance;
    gctUINT enableBorderMode;
    gctUINT enableTPReorder;
    gctUINT enableTPInterleave8;
    gctUINT enableTPRTNE;
    gctUINT enableShader;
    gctUINT enableNNXYDP9;
    gctUINT enableNNXYDP6;
    gctUINT enableSwtilingPhase1;
    gctUINT enableSwtilingPhase2;
    gctUINT enableSwtilingPhase3;
    gctUINT enableHandleBranch; /*merge more branches to use AB Buffer for SWTiling for arch model*/
    gctUINT enableNNFirstPixelPooling;
    gctUINT enableNNDepthWiseSupport;
    gctUINT enablePrintOperaTarget;
    gctUINT enableSaveBinary;
    gctUINT enableGraphCommandBuffer;
    gctUINT nnFormulaOpt;
    gctFLOAT ddrLatency;
    gctFLOAT ddrReadBWLimit;
    gctFLOAT ddrWriteBWLimit;
    gctFLOAT ddrTotalBWLimit;
    gctFLOAT axiSramReadBWLimit;
    gctFLOAT axiSramWriteBWLimit;
    gctFLOAT axiSramTotalBWLimit;
    gctFLOAT axiBusReadBWLimit;
    gctFLOAT axiBusWriteBWLimit;
    gctFLOAT axiBusTotalBWLimit;
    gctUINT vipSRAMSize;
    gctUINT axiSRAMSize;
    gctSTRING graphPerfLogFile;
    gctUINT nnZeroRunLen;
    gctINT  tpZeroRunLen;
    gctUINT enableNNArchPerfPrint;
    gctUINT enableNNLayerDump;
    gctUINT enableInterleave8;
    gctSTRING nnRoundingMode;
    gctSTRING vxcShaderSourcePath;
    gctUINT fcZMax;
    gctUINT enableMemPool;
    gctUINT memPoolSize;
#define COLLECT_PERF_RUN       0
#define COLLECT_PERF_ESTIMATE  1
    gctUINT collectPerfType;
    gctUINT enableGraphAdapter;
    gctUINT enableZdpOpt;
    gctUINT do1xnAfterSwtiling;
    gctUINT nn1x1To1xN;
    gctUINT enableGraphTranform;
    gctUINT enableGraphWAR7;
    gctUINT enableGraphPadConv;
    gctUINT enableGraphMerge;
    gctUINT enableGraphDump;
    gctUINT enableTransformNMConv;
    gctUINT enableGraphConvertAvgPool2Conv;
    gctUINT enableGraphUnrollDWConv;
    gctUINT enableGraphOptimizationToTest;
    gctUINT enableGraphConvertBatchFC2NNConv;
    gctUINT enableGraphConvertTensorAdd;
    gctUINT enableGraphEltwiseOpShape;
    gctUINT enableGraphConvertConv2Fc;
    gctUINT enableGraphSwaplayer;
    gctUINT enableGraphReshapelayer;
    gctUINT enableGraphConcalayer;
    gctUINT enableGraphMergeTranspose;
    gctUINT enableGraphDeleteRelu;
    gctUINT enableGraphDeleteSqueeze;
    gctUINT enableGraphWar1x1x1weight;
    gctUINT freqInMHZ;
    gctUINT axiClockFreqInMHZ;
    gctUINT maxSocOTNumber;
    gctUINT enableHuffmanEnhancement;
    gctUINT enableTPHuffman;
    gctUINT enableMultiVIPCombined;
    gctUINT enableNNTPParallel;
    gctUINT enableVectorPrune;
    gctUINT enableYUV2RGBScaler;
    gctUINT enableVIPDEC400;
    gctUINT enableCacheBinaryGraph;
    gctSTRING enableOpsDebugInfo;
    gctUINT tpCoreCount;
    gctUINT tpLiteCoreCount;
    gctUINT enableForce64BitsBiasNN;
    gctUINT enableAllocateContigousMemForKernel;
    gctUINT enableNNTranspose;
    gctUINT disableTPNNEvis;
}
vx_drv_option;

typedef union _vx_nn_cmd_split_info_union
{
    struct _vx_nn_general_cmd_split_info
    {
        gctUINT32 inImageXSize;
        gctUINT32 inImageYSize;
        gctUINT32 inImageAddress;
        gctINT32  inImageXOffset;
        gctINT32  inImageYOffset;

        gctUINT32 outImageXSize;
        gctUINT32 outImageYSize;
        gctUINT32 outImageZSize;
        gctUINT32 outImageAddress;
        gctUINT32 kernelAddress;
    }
    vx_nn_general_cmd_split_info;

    struct _vx_tp_general_cmd_split_info
    {
        gctUINT32 inImageXSize;
        gctUINT32 inImageYSize;
        gctUINT32 inImageZSize;
        gctUINT32 inImageStride;
        gctUINT32 inImageSlice;
        gctINT32  inWindowXStart;
        gctINT32  inWindowYStart;
        gctUINT32 inWindowXEnd;
        gctUINT32 inWindowYEnd;
        gctUINT32 inImageBaseAddress;
        gctUINT32 inTileListAddress;
        gctUINT32 inTileXSize;
        gctUINT32 inTileYSize;
        gctUINT32 inTileXInc;
        gctUINT32 inTileYInc;
        gctUINT32 inTileSequence;

        gctUINT32 outBaseAddress;
        gctUINT32 outLoop1Reset;
        gctUINT32 outLoop2Reset;
        gctUINT32 outLoop3Reset;
        gctUINT32 outLoop0Inc;
        gctUINT32 outLoop1Inc;
        gctUINT32 outLoop2Inc;
        gctUINT32 outLoop3Inc;
        gctUINT32 outLoop4Inc;
        gctUINT32 outLoop5Inc;
        gctUINT32 outLoop6Inc;
        gctUINT32 outLoop0Count;
        gctUINT32 outLoop1Count;
        gctUINT32 outLoop2Count;
        gctUINT32 outLoop3Count;
        gctUINT32 outLoop4Count;
        gctUINT32 outLoop5Count;

        gctUINT32 aluReorderBitsUsed;
        gctUINT32 aluReorderLoop2Mode;

        gctUINT32 inImageCircularBufSize;
        gctUINT32 inImageCircularBufEndAddrPlus1;
        gctUINT32 outImageCircularBufSize;
        gctUINT32 outImageCircularBufEndAddrPlus1;

        gctUINT32 needReorder;
        gctINT32  noFlush;
        gctUINT32 last;
    }
    vx_tp_general_cmd_split_info;

    struct _vx_tp_fc_cmd_split_info
    {
        gctUINT32 inImageXSize;
        gctUINT32 inImageYSize;
        gctUINT32 inImageZSize;
        gctUINT32 inImageStride;
        gctUINT32 inImageSlice;
        gctINT32  inWindowXStart;
        gctINT32  inWindowYStart;
        gctUINT32 inWindowXEnd;
        gctUINT32 inWindowYEnd;
        gctUINT32 inImageBaseAddress;
        gctUINT32 inTileListAddress;
        gctUINT32 inTileXSize;
        gctUINT32 inTileYSize;
        gctUINT32 inTileXInc;
        gctUINT32 inTileYInc;
        gctUINT32 inTileSequence;

        gctUINT32 outBaseAddress;
        gctUINT32 outLoop1Reset;
        gctUINT32 outLoop2Reset;
        gctUINT32 outLoop3Reset;
        gctUINT32 outLoop0Inc;
        gctUINT32 outLoop1Inc;
        gctUINT32 outLoop2Inc;
        gctUINT32 outLoop3Inc;
        gctUINT32 outLoop4Inc;
        gctUINT32 outLoop5Inc;
        gctUINT32 outLoop6Inc;
        gctUINT32 outLoop0Count;
        gctUINT32 outLoop1Count;
        gctUINT32 outLoop2Count;
        gctUINT32 outLoop3Count;
        gctUINT32 outLoop4Count;
        gctUINT32 outLoop5Count;

        gctUINT32 aluReorderBitsUsed;
        gctUINT32 aluReorderLoop2Mode;

        gctUINT32 inImageCircularBufSize;
        gctUINT32 inImageCircularBufEndAddrPlus1;
        gctUINT32 outImageCircularBufSize;
        gctUINT32 outImageCircularBufEndAddrPlus1;

        gctUINT32 needReorder;
        gctINT32  noFlush;
        gctUINT32 last;

        gctUINT32 aluHorzProcCount;
        gctUINT32 aluVertProcCount;
        gctUINT32 aluLoadPwlLUTAddress;
    }
    vx_tp_fc_cmd_split_info;
}
vx_nn_cmd_split_info_u;

typedef union _vx_nn_cmd_info_union
{
    struct _vx_nn_general_cmd_info
    {
        gctUINT32 inImageXSize;
        gctUINT32 inImageYSize;
        gctUINT32 inImageAddress;
        gctINT32  inImageXOffset;
        gctINT32  inImageYOffset;
        gctUINT32 outImageXSize;
        gctUINT32 outImageYSize;
        gctUINT32 outImageZSize;
        gctUINT32 outImageAddress;

        gctUINT32 kernelAddress;
        gctUINT32 kernelXSize;
        gctUINT32 kernelYSize;
        gctUINT32 kernelZSize;
        gctUINT32 kernelsPerCore;
        gctUINT32 pooling;
        gctUINT32 poolingXYSize;
        gctUINT32 inImageXstride;
        gctUINT32 inImageYstride;
        gctUINT32 inImageCircularBufSize;
        gctUINT32 inImageCircularBufEndAddrPlus1;

        gctUINT32 outImageTileXSize;
        gctUINT32 outImageTileYSize;
        gctUINT32 outImageXstride;
        gctUINT32 outImageYstride;
        gctUINT32 outImageCircularBufSize;
        gctUINT32 outImageCircularBufEndAddrPlus1;
        gctUINT32  roundingMode;
        gctUINT32  relu;
        gctINT32   nn_layer_flush;
        gctUINT32  postMultiplier;
        gctUINT32  postMultiplierBit6to1;
        gctUINT32  postMultiplierBit14to7;
        gctUINT32  postMultiplierBit22to15;
        gctINT32   postShift;
        gctUINT32  postShiftBit6to5;
        gctUINT32  wSize;
        gctUINT8   kernelDataType;
        gctUINT8   inImageDataType;
        gctUINT8   outImageDataType;
        gctUINT32  coefZP;
        gctUINT32  outputZP;
        gctUINT8   brickMode;
        gctUINT32  brickDistance;

        gctUINT8   slowOutput;

        gctUINT8   bFloat16Mode;
        gctUINT8   inImageTransposeChMinusOne;
        gctUINT8   outImageTransposeChMinusOne;
        gctUINT32  outImageTransposeBufStartAddr;
        gctUINT32  outImageTransposeBufEndAddr;

        /* for SRAM */
        gctUINT32 imageCachingMode;
        gctUINT32 kernelCachingMode;
        gctUINT32 partialCacheDataUnit;
        gctUINT32 kernelPatternMsb;
        gctUINT32 kernelPatternLow32Bits;
        gctUINT32 kernelPatternHigh32Bits;
        gctUINT32 kernelCacheStartAddress;
        gctUINT32 kernelCacheEndAddress;
        gctUINT32 kernelDirectStreamFromVipSram;
        gctUINT32 imageStartAddress;
        gctUINT32 imageEndAddress;
        gctUINT32 inImageBorderMode;
        gctINT32  inImageBorderConst;
        gctUINT8  kernelDataTypeMsb;
        gctUINT8  inImageDataTypeMsb;
        gctUINT8  outImageDataTypeMsb;
        gctUINT8  outImageCacheEvictPolicy;
        gctUINT32 noFlush;
        gctUINT8  hwDepthWise;
        gctUINT8  noZOffset;
        gctUINT8  perChannelPostMul;
        gctUINT8  pRelu;
    }
    vx_nn_general_cmd_info;

    struct _vx_nn_tp_cmd_info
    {
        gctUINT32 inImageXSize;
        gctUINT32 inImageYSize;
        gctUINT32 inImageZSize;
        gctUINT32 inImageStride;
        gctUINT32 inImageSlice;
        gctINT32  inWindowXStart;
        gctINT32  inWindowYStart;
        gctUINT32 inWindowXEnd;
        gctUINT32 inWindowYEnd;
        gctUINT32 inImageBaseAddress;
        gctUINT32 inTileListAddress;
        gctUINT32 inTileXSize;
        gctUINT32 inTileYSize;
        gctUINT32 inTileXInc;
        gctUINT32 inTileYInc;
        gctUINT32 inTileSequence;
        gctUINT32 outBaseAddress;
        gctUINT32 outLoop1Reset;
        gctUINT32 outLoop2Reset;
        gctUINT32 outLoop3Reset;
        gctUINT32 outLoop0Inc;
        gctUINT32 outLoop1Inc;
        gctUINT32 outLoop2Inc;
        gctUINT32 outLoop3Inc;
        gctUINT32 outLoop4Inc;
        gctUINT32 outLoop5Inc;
        gctUINT32 outLoop6Inc;
        gctUINT32 outLoop0Count;
        gctUINT32 outLoop1Count;
        gctUINT32 outLoop2Count;
        gctUINT32 outLoop3Count;
        gctUINT32 outLoop4Count;
        gctUINT32 outLoop5Count;
        gctUINT32 aluReorderBitsUsed;
        gctUINT32 aluReorderLoop2Mode;
        gctUINT32 inImageCircularBufSize;
        gctUINT32 inImageCircularBufEndAddrPlus1;
        gctUINT32 outImageCircularBufSize;
        gctUINT32 outImageCircularBufEndAddrPlus1;
        gctUINT32 needReorder;
        gctINT32  noFlush;
        gctUINT32 last;
        gctUINT32 aluHorzProcCount;
        gctUINT32 aluVertProcCount;
        gctUINT32 aluLoadPwlLUTAddress;
        gctUINT32 inImageBorderMode;
        gctUINT32 inTileListGlobalMem;
        gctUINT32 inImageGlobalMem;
        gctUINT32 aluI2FEnable;
        gctUINT32 aluSquareEnable;
        gctUINT32 aluHorzProcessing;
        gctUINT32 aluHorzProcStride;
        gctUINT32 aluVertProcessing;
        gctUINT32 aluVertProcStride;
        gctUINT32 aluNmsEnable;
        gctUINT32 aluPwlEnable;
        gctUINT32 aluMultEnable;
        gctUINT32 aluF2IEnable;
        gctUINT32 aluLoadPwlLUT;
        gctUINT32 aluLoadPwlLUTGlobalMem;
        gctUINT32 outTileSkipAtborder;
        gctUINT32 outGlobalMem;
        gctUINT32 outBrickMode;
        gctUINT32 aluZFilterMode;
        gctUINT32 inWindowZStartOverfetch;
        gctUINT32 inWindowZEndOverfetch;
        gctUINT32 aluSquarePreshift;
        gctUINT32 inImageDataType;
        gctUINT32 outImageDataType;
        gctUINT32 kernelDataType;
        gctUINT32 aluFilterPwlSwap;
        gctUINT32 aluPwlSignSupport;
        gctUINT32 aluReluEnable;
        gctUINT32 floatRoundingMode;
        gctUINT32 integeroundingMode;
        gctINT32  inImageBorderConst;
        gctINT32  aluInputPreshift;
        gctINT32  aluOutputPostshift;
        gctINT32  aluOutputPostshiftBit6to5;
        gctUINT32 coefZP;
        gctUINT32 inputZP;
        gctUINT32 outputZP;
        gctUINT32 aluOutputPostMultiplier;
        gctUINT32 aluOutputPostMultiplierBit22to15;
    }
    vx_nn_tp_cmd_info;

    struct _vx_nn_image_cmd_info
    {
        gctUINT32 width;
        gctUINT32 height;
        gctUINT32 shift;
        gctUINT32 physical;
    }
    vx_nn_image_cmd_info;

    struct _vx_yuv2rgb_scaler_cmd_info
    {
        gctUINT32 inImageBaseY;
        gctUINT32 inImageBaseU;
        gctUINT32 inImageBaseV;
        gctUINT16 inRectX;
        gctUINT16 inRectY;
        gctUINT16 inRectWidth;
        gctUINT16 inRectHeight;
        gctUINT16 inImageWidth;
        gctUINT16 inImageHeight;
        gctUINT16 inImageStrideY;
        gctUINT32 outImageBaseR;
        gctUINT32 outImageBaseG;
        gctUINT32 outImageBaseB;
        gctUINT16 outImageWidth;
        gctUINT16 outImageHeight;
        gctUINT16 outImageStride;
        gctUINT16 outImageBitsSize;
        gctUINT32 scaleX;
        gctUINT32 scaleY;
        gctUINT16 inImageInitErrX;
        gctUINT16 inImageInitErrY;
        gctUINT16 inImageInitIntErrX;
        gctUINT16 inImageInitIntErrY;
        gctUINT8  yOnly;
        gctUINT8  outSigned;
        gctUINT8  postShift;
        gctUINT16 c0;
        gctUINT16 c1;
        gctUINT16 c2;
        gctUINT16 c3;
        gctUINT16 c4;
        gctINT32  c5;
        gctINT32  c6;
        gctINT32  c7;
        gctUINT32 outRequestCount;
        gctINT16  minRClamp;
        gctINT16  maxRClamp;
        gctINT16  minGClamp;
        gctINT16  maxGClamp;
        gctINT16  minBClamp;
        gctINT16  maxBClamp;
    }
    vx_yuv2rgb_scaler_cmd_info;
}
vx_nn_cmd_info_u;

typedef struct _gcoVX_Hardware_Context
{
    gctUINT32           kernel;

    gctUINT32           step;

    gctUINT32           xmin;
    gctUINT32           xmax;
    gctUINT32           xstep;
    gctUINT32           ymin;
    gctUINT32           ymax;
    gctUINT32           ystep;

#if gcdVX_OPTIMIZER > 2
    gctBOOL             tiled;
    gctUINT32           xstart;
    gctUINT32           xcount;
    gctUINT32           xoffset;
    gctUINT32           ystart;
    gctUINT32           ycount;
    gctUINT32           yoffset;
#endif

    gctUINT32           groupSizeX;
    gctUINT32           groupSizeY;

    gctUINT32           threadcount;
    gctUINT32           policy;
    gctUINT32           rounding;
    gctFLOAT            scale;
    gctFLOAT            factor;
    gctUINT32           borders;
    gctUINT32           constant_value;
    gctUINT32           volume;
    gctUINT32           clamp;

    gctUINT32           inputMultipleWidth;
    gctUINT32           outputMultipleWidth;

#if gcdVX_OPTIMIZER > 1
    gctUINT32           order;
#else
    gctUINT32           *order;
#endif

    gctUINT32           input_type[10];
    gctUINT32           output_type[10];
    gctUINT32           input_count;
    gctUINT32           output_count;

    gctINT16            *matrix;
    gctINT16            *matrix1;
    gctUINT32           col;
    gctUINT32           row;

    gcsSURF_NODE_PTR    node;

    gceSURF_FORMAT      inputFormat;
    gceSURF_FORMAT      outputFormat;

    gctUINT8            isUseInitialEstimate;
    gctINT32            maxLevel;
    gctINT32            winSize;

    gcoVX_Instructions  *instructions;

    gcoVX_Kernel_Context_Uniform    *uniform;
    gctUINT32                       *unifor_num;

#if !gcdVX_OPTIMIZER
    gctUINT32           nodePhysicalAdress;
#endif

    vx_evis_no_inst_s    evisNoInst;

    gctUINT32            optionalOutputs[3];

#if gcdVX_OPTIMIZER > 1
    gctBOOL                 tileMode;
    gctUINT32               curDeviceID;
    gctUINT32               usedDeviceCount;
    gcsTHREAD_WALKER_INFO   splitInfo[4];
    gctUINT32               deviceCount;
    gctPOINTER              *devices;
#endif

    gctBOOL             hasBarrier;
    gctBOOL             hasAtomic;
}
gcoVX_Hardware_Context;

typedef struct _gcoVX_Hardware_Kernel
{
    gctUINT32   index;
    gctSTRING   desc;
    gctUINT32   kernel;
    gceSTATUS   (*func)(gcoVX_Hardware_Context *Context);
}
gcoVX_Hardware_Kernel;

typedef gcoVX_Hardware_Kernel gcoVX_Hardware_Step;

typedef enum
{
    gcvVX_ACCELERATOR_INVALID = -1,
    gcvVX_ACCELERATOR_NN      = 0,
    gcvVX_ACCELERATOR_TP      = 1,
}
gceVX_ACCELERATOR_TYPE;

typedef enum
{
   /* AXI SRAM REMAP */
    gcvVX_OCB_REMAP           = 0,
    /* VIP SRAM REMAP */
    gcvVX_SRAM_REMAP          = 1,
}
gceVX_REMAP_TYPE;

gceSTATUS
gcoHARDWAREVX_CommitCmd(
    IN gcoHARDWARE          Hardware,
    OUT gctPOINTER*         pCmdLogical,
    OUT gctUINT32*          pCmdBytes
    );

gceSTATUS
gcoHARDWAREVX_ReplayCmd(
    IN gcoHARDWARE          Hardware,
    OUT gctPOINTER          CmdLogical,
    OUT gctUINT32           CmdBytes
    );

gceSTATUS
gcoHARDWAREVX_SetImageInfo(
    IN gcoHARDWARE          Hardware,
    IN gctUINT32            RegAddress,
    IN gctUINT32            Physical,
    IN gcsVX_IMAGE_INFO_PTR Info
    );

gceSTATUS
gcoHARDWAREVX_BindImage(
    IN gcoHARDWARE          Hardware,
    IN gctUINT32            Index,
    IN gcsVX_IMAGE_INFO_PTR Info
    );

gceSTATUS
gcoHARDWAREVX_BindUniform(
    IN gcoHARDWARE          Hardware,
    IN gctUINT32            RegAddress,
    IN gctUINT32            Index,
    IN gctUINT32*           Value,
    IN gctUINT32            Num
    );

gceSTATUS
gcoHARDWAREVX_InvokeThreadWalker(
    IN gcoHARDWARE                          Hardware,
    IN gcsVX_THREAD_WALKER_PARAMETERS_PTR   Parameters
    );

gceSTATUS
gcoHARDWAREVX_LoadKernel(
    IN gcoHARDWARE          Hardware,
    IN gctUINT32            Address,
    IN gctUINT32            Size,
    IN gctUINT32            TempRegCount,
    OUT gctUINT32_PTR       ValueOrder
    );

gceSTATUS
gcoHARDWAREVX_KenrelConstruct(
    IN OUT gcoVX_Hardware_Context   *Context
);

gceSTATUS
gcoHARDWAREVX_InitVX(
    IN gcoHARDWARE          Hardware
);

gceSTATUS
gcoHARDWAREVX_TriggerAccelerator(
    IN gcoHARDWARE              Hardware,
    IN gctUINT32                CmdAddress,
    IN gceVX_ACCELERATOR_TYPE   Type,
    IN gctUINT32                EventId,
    IN gctBOOL                  waitEvent,
    IN gctUINT32                gpuId,
    IN gctBOOL                  sync,
    IN gctUINT32                syncEventID
);

gceSTATUS
gcoHARDWAREVX_ProgrammeNNEngine(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Info,
    IN gctPOINTER Options,
    IN OUT gctUINT32_PTR *Instruction
);

gceSTATUS
gcoHARDWAREVX_ProgrammeTPEngine(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Info,
    IN OUT gctUINT32_PTR *Instruction
);

gceSTATUS
gcoHARDWAREVX_SetNNImage(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Info,
    IN OUT gctUINT32_PTR *Instruction
);

gceSTATUS
gcoHARDWAREVX_WaitNNEvent(
    IN gcoHARDWARE Hardware,
    IN gctUINT32   EventId
);

gceSTATUS
gcoHARDWAREVX_SetRemapAddress(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 remapStart,
    IN gctUINT32 remapEnd,
    IN gceVX_REMAP_TYPE remapType
);

gceSTATUS
gcoHARDWAREVX_YUV2RGBScale(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Info,
    IN gctUINT32  gpuId,
    IN gctBOOL    mGpuSync
    );

#if GC_VX_ASM
gceSTATUS
gcoHARDWAREVX_GenerateMC(
    IN OUT gcoVX_Hardware_Context   *Context
);
#endif

gceSTATUS
gcoHARDWAREVX_FlushCache(
    IN gcoHARDWARE          Hardware,
    IN gctBOOL              InvalidateICache,
    IN gctBOOL              FlushPSSHL1Cache,
    IN gctBOOL              FlushNNL1Cache,
    IN gctBOOL              FlushTPL1Cache,
    IN gctBOOL              FlushSHL1Cache
    );


gceSTATUS
gcoHARDWAREVX_CaptureState(
    IN gcoHARDWARE  Hardware,
    IN OUT gctUINT8 *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *pOutputSizeInByte,
    IN gctBOOL Enabled,
    IN gctBOOL dropCommandEnabled
    );

gceSTATUS
gcoHARDWAREVX_CaptureInitState(
    IN gcoHARDWARE Hardware,
    IN OUT gctPOINTER CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *OutputSizeInByte
    );

#endif

gceSTATUS
gcoHARDWARE_GetContext(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32 * Context
    );

gceSTATUS
gcoHARDWARE_SetProbeCmd(
    IN gcoHARDWARE Hardware,
    IN gceProbeCmd Cmd,
    IN gctUINT32 ProbeAddres,
    IN gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_EnableCounters(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_ProbeCounter(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 address,
    IN gceCOUNTER module,
    IN gceProbeCmd probeCmd,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_Set3DHardware(
     IN gcoHARDWARE Hardware
     );

gceSTATUS
gcoHARDWARE_Get3DHardware(
     OUT gcoHARDWARE * Hardware
     );

#endif /* gcdENABLE_3D */
/******************************************************************************\
******************************** gcoBUFFER Object *******************************
\******************************************************************************/

/* Construct a new gcoBUFFER object. */
gceSTATUS
gcoBUFFER_Construct(
    IN gcoHAL Hal,
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gctSIZE_T MaxSize,
    IN gctBOOL ThreadDefault,
    OUT gcoBUFFER * Buffer
    );

/* Destroy an gcoBUFFER object. */
gceSTATUS
gcoBUFFER_Destroy(
    IN gcoBUFFER Buffer
    );

/* Reserve space in a command buffer. */
gceSTATUS
gcoBUFFER_Reserve(
    IN gcoBUFFER Buffer,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned,
    IN gctUINT32 Usage,
    OUT gcoCMDBUF * Reserve
    );


typedef struct _gcsTEMPCMDBUF       * gcsTEMPCMDBUF;

/* start a temp command buffer */
gceSTATUS
gcoBUFFER_StartTEMPCMDBUF(
    IN gcoBUFFER Buffer,
    IN gcoQUEUE Queue,
    OUT gcsTEMPCMDBUF *tempCMDBUF
    );

/* end of a temp command buffer */
gceSTATUS
gcoBUFFER_EndTEMPCMDBUF(
    IN gcoBUFFER Buffer,
    IN gctBOOL  Drop
    );

/* Write data into the command buffer. */
gceSTATUS
gcoBUFFER_Write(
    IN gcoBUFFER Buffer,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    );

/* Doesn't have implementation */
/* Write 32-bit data into the command buffer. */
gceSTATUS
gcoBUFFER_Write32(
    IN gcoBUFFER Buffer,
    IN gctUINT32 Data,
    IN gctBOOL Aligned
    );
/* Doesn't have implementation */
/* Write 64-bit data into the command buffer. */
gceSTATUS
gcoBUFFER_Write64(
    IN gcoBUFFER Buffer,
    IN gctUINT64 Data,
    IN gctBOOL Aligned
    );

/* Commit the command buffer. */
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
    );

gceSTATUS
gcoBUFFER_AppendFence(
    IN gcoBUFFER Buffer,
    IN gcsSURF_NODE_PTR Node,
    IN gceFENCE_TYPE Type
    );

gceSTATUS
gcoBUFFER_OnIssueFence(
    IN gcoBUFFER Buffer
    );

gceSTATUS
gcoBUFFER_GetFence(
    IN gcoBUFFER Buffer
    );

gceSTATUS
gcoBUFFER_IsEmpty(
    IN gcoBUFFER Buffer
    );

gceSTATUS
gcoBUFFER_SelectChannel(
    IN gcoBUFFER Buffer,
    IN gctBOOL Priority,
    IN gctUINT32 ChannelId
    );

gceSTATUS
gcoBUFFER_GetChannelInfo(
    IN gcoBUFFER Buffer,
    OUT gctBOOL *Priority,
    OUT gctUINT32 *ChannelId
    );

gceSTATUS
gcoBUFFER_AddTimestampPatch(
    IN gcoBUFFER Buffer,
    IN gctUINT32 Handle,
    IN gctUINT32 Flag
    );

gceSTATUS
gcoBUFFER_AddVidmemAddressPatch(
    IN gcoBUFFER Buffer,
    IN gctUINT32_PTR Logical,
    IN gctUINT32 Handle,
    IN gctUINT32 NodeOffset
    );

gceSTATUS
gcoBUFFER_AddMCFESemaphorePatch(
    IN gcoBUFFER Buffer,
    IN gctUINT32_PTR Logica,
    IN gctBOOL SendSema,
    IN gctUINT32 SemaHandle
    );

gceSTATUS
gcoBUFFER_Capture(
    IN gcoBUFFER Buffer,
    IN OUT gctUINT8 *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *pOutputSizeInByte,
    IN gctBOOL Enabled,
    IN gctBOOL dropCommandEnabled
    );

gceSTATUS
gcoBUFFER_CaptureInitState(
    IN gcoBUFFER Buffer,
    IN OUT gctUINT8 *CaptureBuffer,
    IN gctUINT32 InputSizeInByte,
    IN OUT gctUINT32 *pOutputSizeInByte
    );

gceSTATUS
gcoBUFFER_IsCaptureEnabled(
    IN gcoBUFFER Buffer
    );


#if gcdSYNC
typedef enum _gceFENCE_STATUS
{
    gcvFENCE_DISABLE,
    gcvFENCE_GET,
    gcvFENCE_ENABLE,
}
gceFENCE_STATUS;
#endif

/******************************************************************************\
********************************* gcoHAL object *********************************
\******************************************************************************/

struct _gcoHAL
{
    gcsOBJECT               object;

#if gcdFRAME_DB
    gctINT                  frameDBIndex;
    gcsHAL_FRAME_INFO       frameDB[gcdFRAME_DB];
#endif

    gctINT32                chipCount;
    gceHARDWARE_TYPE        chipTypes[gcdCHIP_COUNT];
    gctUINT                 chipIDs[gcvCORE_COUNT];
    gctBOOL                 separated2D;
    gctBOOL                 hybrid2D;
    gctBOOL                 is3DAvailable;
    gctBOOL                 isGpuBenchSmoothTriangle;

    gceHARDWARE_TYPE        defaultHwType;
#if gcdENABLE_3D
    gcsSTATISTICS           statistics;
#endif

    gcsUSER_DEBUG_OPTION    *userDebugOption;
};

/******************************************************************************\
********************************* gcoSURF object ********************************
\******************************************************************************/

typedef void  (* _PFNcalcPixelAddr)(
    gcoSURF surf,
    gctSIZE_T x,
    gctSIZE_T y,
    gctSIZE_T z,
    gctPOINTER addr[gcdMAX_SURF_LAYERS]
    );

#if gcdSYNC
typedef struct {
    gctUINT64               writeFenceID;
    gctUINT64               readFenceID;
    gctPOINTER              fence;
    gctINT32                id;
    gctBOOL                 mark[gcvENGINE_ALL_COUNT];
}gcsFENCE_ENGINE_CONTEXT;

typedef struct _gcsSYNC_CONTEXT
{
    gcsFENCE_ENGINE_CONTEXT  engine[gcvENGINE_ALL_COUNT];
    gcsSYNC_CONTEXT_PTR      next;
}
gcsSYNC_CONTEXT;
#endif

typedef struct _gcsSURF_NODE
{
    /* Surface memory pool. */
    gcePOOL                 pool;

    /* Lock count for the surface. */
    gctINT                  lockCounts[gcvHARDWARE_NUM_TYPES][gcvENGINE_GPU_ENGINE_COUNT];

    /* If not zero, the node is locked in the kernel. */
    gctBOOL                 lockedInKernel;

    /* Number of planes in the surface for planar format support. */
    gctUINT                 count;

    /* Node valid flag for the surface pointers. */
    gctBOOL                 valid;

    /* The physical addresses of the surface. */
    gctUINT32               physical2;
    gctUINT32               physical3;

    /* The logical addresses of the surface. */
    gctUINT8_PTR            logical;

    /* If the buffer was split, they record the physical and logical addr
    ** of the bottom buffer.
    */
    gctUINT32               physicalBottom;
    gctUINT8_PTR            logicalBottom;

    /* hardware addresses for each hardware type of the node. */
    gctUINT32               hardwareAddresses[gcvHARDWARE_NUM_TYPES];
    gctUINT32               hardwareAddressesBottom[gcvHARDWARE_NUM_TYPES];

    /* Offset from node start to valid buffer address. */
    gctSIZE_T               bufferOffset;

    /* Linear size for tile status. */
    gctSIZE_T               size;

    /* CPU write to the video memory.*/
    gctBOOL                 bCPUWrite;

    union _gcuSURF_NODE_LIST
    {
        /* Allocated through HAL. */
        struct _gcsMEM_NODE_NORMAL
        {
            gctUINT32           node;
            gctBOOL             cacheable;
        }
        normal;

        /* Wrapped around user-allocated surface (gcvPOOL_USER). */
        struct _gcsMEM_NODE_WRAPPED
        {
            gctPOINTER          mappingInfo;
            gctPHYS_ADDR_T      physical;
            gctBOOL             lockedInKernel[gcvHARDWARE_NUM_TYPES];
        }
        wrapped;
    }
    u;

#if gcdSYNC
    gceFENCE_STATUS             fenceStatus;
    gcsSYNC_CONTEXT_PTR         fenceCtx;
#endif

}
gcsSURF_NODE;

struct _gcoSURF
{
    /* Object. */
    gcsOBJECT               object;

    /* Type usage and format of surface. */
    gceSURF_TYPE            type;
    gceSURF_TYPE            hints;
    gceSURF_FORMAT          format;
    gceTILING               tiling;
    gceCACHE_MODE           cacheMode;

    /* Surface size. */
    gctUINT                 requestW;   /* client request width  */
    gctUINT                 requestH;   /* client request height */
    gctUINT                 requestD;   /* client request depth  */
    gctUINT                 allocedW;   /* allocated width  applied msaa */
    gctUINT                 allocedH;   /* allocated height applied msaa */
    gctUINT                 alignedW;   /* aligned allocated width  */
    gctUINT                 alignedH;   /* aligned allocated height */

    gctUINT32               bitsPerPixel;

    /*
    ** Offset in byte of the bottom buffer from beginning of the top buffer.
    ** If HW does not require to be programmed wtih 2 addresses, it should be 0.
    */
    gctUINT32               bottomBufferOffset;

    /* Rotation flag. */
    gceSURF_ROTATION        rotation;
    gceORIENTATION          orientation;

    /* Dither flag. */
    gctBOOL                 dither2D;
    gctBOOL                 dither3D;

    /* For some chips, PE doesn't support dither. If app request it, we defer it until resolve time. */
    gctBOOL                 deferDither3D;

    /* Surface stride, sliceSize, size in bytes. */

    /*
    ** In one surface(miplevel), there maybe are some layers(patch format),
    ** in every layer, there squeeze multiple slices(cube,3d,array),
    ** sliceSize is to calculate offset for different slice.
    ** layerSize is to calculate offset for different layer.
    ** stride is the pitch of a single layer.
    */
    gctUINT                 stride;
    gctUINT                 sliceSize;
    gctUINT                 layerSize;
    gctUINT                 size;

    gctUINT                 tileStatusSliceSize;

    /* YUV planar surface parameters. */
    gctUINT                 uOffset;
    gctUINT                 vOffset;
    gctUINT                 uStride;
    gctUINT                 vStride;
#if gcdVG_ONLY
    gctUINT                 aOffset;
    gctUINT                 aStride;
#endif

    gceHARDWARE_TYPE        initType;

    /* Video memory node for surface. */
    gcsSURF_NODE            node;
    /*
    ** For multi-node surface, such as imported from linux dma buffer by
    ** EGL_EXT_dma_buf_import.
    */
    gcsSURF_NODE            node2;
    gcsSURF_NODE            node3;

    /* Surface color type. */
    gceSURF_COLOR_TYPE      colorType;

    /* Surface color space */
    gceSURF_COLOR_SPACE     colorSpace;

    /* Only useful for R8_1_X8R8G8B8/G8R8_1_X8R8G8B8 formats.
    ** garbagePadded=0 means driver is sure the padding channels of whole surface are default values;
    ** garbagePadded=1 means padding channels of some subrect of surface might be garbages.
    */
    gctBOOL                 paddingFormat;
    gctBOOL                 garbagePadded;

    /* Samples. */
    gcsSAMPLES              sampleInfo;

    /* isMsaa(true) means real msaa surface.
    ** Exclude resample case, which samples.x * samples.y > 1, but isMsaa=false
    */
    gctBOOL                 isMsaa;
    gctBOOL                 vMsaa;

#if gcdENABLE_3D
    gctBOOL                 resolvable;

    /* Tile status. */
    /* FC clear status and its values */
    gctBOOL_PTR             tileStatusDisabled;
    gctUINT32*              fcValue;
    gctUINT32*              fcValueUpper;
    /*
    ** Only meaningful when tileStatusDisabled == gcvFALSE
    */
    gctBOOL                 compressed;
    gctINT32                compressFormat;
    gctINT32                compressDecFormat;

    /* If dirty == gcvTRUE, means master has some data which is not resolved through clear color.
    */
    gctBOOL_PTR             dirty;

    gctBOOL                 hzDisabled;
    gctBOOL                 superTiled;

    /* Clear value of last time, for value passing and reference */
    gctUINT32               clearValue[gcdMAX_SURF_LAYERS];
    gctUINT32               clearValueUpper[gcdMAX_SURF_LAYERS];
    gctUINT32               clearValueHz;

    /*
    ** 4-bit channel Mask: ARGB:MSB->LSB
    */
    gctUINT8                clearMask[gcdMAX_SURF_LAYERS];

    /*
    ** 32-bit bit Mask
    */
    gctUINT32               clearBitMask[gcdMAX_SURF_LAYERS];
    gctUINT32               clearBitMaskUpper[gcdMAX_SURF_LAYERS];

    /* Hierarchical Z buffer pointer and its fc value */
    gcsSURF_NODE            hzNode;
    gctUINT32               fcValueHz;

    /* Video memory node for tile status. */
    gcsSURF_NODE            tileStatusNode;
    gcsSURF_NODE            hzTileStatusNode;
    gctBOOL                 TSDirty;
    gctUINT32               tileStatusFiller;
    gctUINT32               hzTileStatusFiller;
    gctBOOL                 tileStatusFirstLock;
    gctBOOL                 hzTileStatusFirstLock;

    /* When only clear depth for packed depth stencil surface, we can fast clear
    ** both depth and stencil (to 0) if stencil bits are never initialized.
    */
    gctBOOL                 hasStencilComponent;
    gctBOOL                 canDropStencilPlane;

    /* Horizontal alignment in pixels of the surface. */
    gceSURF_ALIGNMENT       hAlignment;

#endif /* gcdENABLE_3D */

    /* 2D related members. */
    gce2D_TILE_STATUS_CONFIG    tileStatusConfig;
    gceSURF_FORMAT              tileStatusFormat;
    gctUINT32                   tileStatusClearValue;
    gctUINT32                   tileStatusGpuAddress;

    /* For 2D DEC Compression. */
    gctBOOL                     srcDECTPC10BitNV12;
    /* For possible YUV compression. */
    gctUINT32                   tileStatusGpuAddressEx[2];

    /* Format information. */
    gcsSURF_FORMAT_INFO         formatInfo;

    gceSURF_FLAG                flags;

    /* Reference count of surface. */
    gctINT32                    refCount;

    /* User pointers for the surface wrapper. */
    gctPOINTER                  userLogical;
    gctPHYS_ADDR_T              userPhysical;

    /* Timestamp, indicate when surface is changed. */
    gctUINT64                   timeStamp;

    /* Shared buffer. */
    gctSHBUF                    shBuf;

    _PFNcalcPixelAddr           pfGetAddr;
};


#define gcvSURF_SHARED_INFO_MAGIC     gcmCC('s', 'u', 's', 'i')

/* Surface states need share across processes. */
typedef struct _gcsSURF_SHARED_INFO
{
    gctUINT32 magic;
    gctUINT64 timeStamp;

#if gcdENABLE_3D
    gctBOOL   tileStatusDisabled;
    gctBOOL   dirty;
    gctUINT32 fcValue;
    gctUINT32 fcValueUpper;
    gctBOOL   compressed;
#endif
}
gcsSURF_SHARED_INFO;

typedef void  (* _PFNreadPixel)(gctPOINTER inAddr[gcdMAX_SURF_LAYERS], gcsPIXEL* outPixel);
typedef void  (* _PFNwritePixel)(gcsPIXEL* inPixel, gctPOINTER outAddr[gcdMAX_SURF_LAYERS], gctUINT flags);

_PFNreadPixel gcoSURF_GetReadPixelFunc(gcoSURF surf);
_PFNwritePixel gcoSURF_GetWritePixelFunc(gcoSURF surf);
_PFNcalcPixelAddr gcoHARDWARE_GetProcCalcPixelAddr(gcoHARDWARE Hardware, gcoSURF Surf);
void gcoSURF_PixelToNonLinear(gcsPIXEL* inPixel);
void gcoSURF_PixelToLinear(gcsPIXEL* inPixel);

#define gcd_QUERY_COLOR_SPACE(format)   \
    (format == gcvSURF_A8_SBGR8 ||      \
     format == gcvSURF_SBGR8    ||      \
     format == gcvSURF_X8_SBGR8 ||      \
     format == gcvSURF_A8_SRGB8 ||      \
     format == gcvSURF_X8_SRGB8         \
    ) ? gcvSURF_COLOR_SPACE_NONLINEAR : gcvSURF_COLOR_SPACE_LINEAR

#define gcdMULTI_SOURCE_NUM 8

typedef struct _gcs2D_MULTI_SOURCE
{
    gce2D_SOURCE srcType;
    struct _gcoSURF srcSurface;
    gceSURF_MONOPACK srcMonoPack;
    gctUINT32 srcMonoTransparencyColor;
    gctBOOL   srcColorConvert;
    gctUINT32 srcFgColor;
    gctUINT32 srcBgColor;
    gctUINT32 srcColorKeyLow;
    gctUINT32 srcColorKeyHigh;
    gctBOOL srcRelativeCoord;
    gctBOOL srcStream;
    gcsRECT srcRect;
    gce2D_YUV_COLOR_MODE srcYUVMode;
    gctBOOL srcDeGamma;

    gce2D_TRANSPARENCY srcTransparency;
    gce2D_TRANSPARENCY dstTransparency;
    gce2D_TRANSPARENCY patTransparency;

    gctBOOL enableDFBColorKeyMode;

    gctUINT8 fgRop;
    gctUINT8 bgRop;

    gctBOOL enableAlpha;
    gceSURF_PIXEL_ALPHA_MODE  srcAlphaMode;
    gceSURF_PIXEL_ALPHA_MODE  dstAlphaMode;
    gceSURF_GLOBAL_ALPHA_MODE srcGlobalAlphaMode;
    gceSURF_GLOBAL_ALPHA_MODE dstGlobalAlphaMode;
    gceSURF_BLEND_FACTOR_MODE srcFactorMode;
    gceSURF_BLEND_FACTOR_MODE dstFactorMode;
    gceSURF_PIXEL_COLOR_MODE  srcColorMode;
    gceSURF_PIXEL_COLOR_MODE  dstColorMode;
    gce2D_PIXEL_COLOR_MULTIPLY_MODE srcPremultiplyMode;
    gce2D_PIXEL_COLOR_MULTIPLY_MODE dstPremultiplyMode;
    gce2D_GLOBAL_COLOR_MULTIPLY_MODE srcPremultiplyGlobalMode;
    gce2D_PIXEL_COLOR_MULTIPLY_MODE dstDemultiplyMode;
    gctUINT32 srcGlobalColor;
    gctUINT32 dstGlobalColor;

    gcsRECT     clipRect;
    gcsRECT     dstRect;

    /* Stretch factors. */
    gctBOOL     enableGDIStretch;
    gctUINT32   horFactor;
    gctUINT32   verFactor;

    /* Mirror. */
    gctBOOL horMirror;
    gctBOOL verMirror;

} gcs2D_MULTI_SOURCE, *gcs2D_MULTI_SOURCE_PTR;

/* FilterBlt information. */
#define gcvMAXKERNELSIZE        9
#define gcvSUBPIXELINDEXBITS    5

#define gcvSUBPIXELCOUNT \
    (1 << gcvSUBPIXELINDEXBITS)

#define gcvSUBPIXELLOADCOUNT \
    (gcvSUBPIXELCOUNT / 2 + 1)

#define gcvWEIGHTSTATECOUNT \
    (((gcvSUBPIXELLOADCOUNT * gcvMAXKERNELSIZE + 1) & ~1) / 2)

#define gcvKERNELTABLESIZE \
    (gcvSUBPIXELLOADCOUNT * gcvMAXKERNELSIZE * sizeof(gctUINT16))

#define gcvKERNELSTATES \
    (gcmALIGN(gcvKERNELTABLESIZE + 4, 8))

typedef struct _gcsFILTER_BLIT_ARRAY
{
    gceFILTER_TYPE              filterType;
    gctUINT8                    kernelSize;
    gctUINT32                   scaleFactor;
    gctBOOL                     kernelChanged;
    gctUINT32_PTR               kernelStates;
}
gcsFILTER_BLIT_ARRAY;

typedef gcsFILTER_BLIT_ARRAY *  gcsFILTER_BLIT_ARRAY_PTR;

#define gcd2D_CSC_TABLE_SIZE        12
#define gcd2D_GAMMA_TABLE_SIZE      256

typedef struct _gcs2D_State
{
    gctUINT             currentSrcIndex;
    gcs2D_MULTI_SOURCE  multiSrc[gcdMULTI_SOURCE_NUM];
    gctUINT32           srcMask;

    /* dest info. */
    struct _gcoSURF dstSurface;
    gctUINT32    dstColorKeyLow;
    gctUINT32    dstColorKeyHigh;
    gce2D_YUV_COLOR_MODE dstYUVMode;
    gctBOOL      dstEnGamma;
    gcsRECT      dstClipRect;

    /* brush info. */
    gce2D_PATTERN brushType;
    gctUINT32 brushOriginX;
    gctUINT32 brushOriginY;
    gctUINT32 brushColorConvert;
    gctUINT32 brushFgColor;
    gctUINT32 brushBgColor;
    gctUINT64 brushBits;
    gctUINT64 brushMask;
    gctUINT32 brushAddress;
    gceSURF_FORMAT brushFormat;

    /* Dithering enabled. */
    gctBOOL dither;

    /* Clear color. */
    gctUINT32 clearColor;

    /* Palette Table for source. */
    gctUINT32  paletteIndexCount;
    gctUINT32  paletteFirstIndex;
    gctBOOL    paletteConvert;
    gctBOOL    paletteProgram;
    gctPOINTER paletteTable;
    gceSURF_FORMAT paletteConvertFormat;

    /* Filter blit. */
    gceFILTER_TYPE              newFilterType;
    gctUINT8                    newHorKernelSize;
    gctUINT8                    newVerKernelSize;

    gctBOOL                     horUserFilterPass;
    gctBOOL                     verUserFilterPass;

    gcsFILTER_BLIT_ARRAY        horSyncFilterKernel;
    gcsFILTER_BLIT_ARRAY        verSyncFilterKernel;

    gcsFILTER_BLIT_ARRAY        horBlurFilterKernel;
    gcsFILTER_BLIT_ARRAY        verBlurFilterKernel;

    gcsFILTER_BLIT_ARRAY        horUserFilterKernel;
    gcsFILTER_BLIT_ARRAY        verUserFilterKernel;

    gctBOOL                     specialFilterMirror;

    gctINT32                    cscYUV2RGB[gcd2D_CSC_TABLE_SIZE];
    gctINT32                    cscRGB2YUV[gcd2D_CSC_TABLE_SIZE];

    gctUINT32                   deGamma[gcd2D_GAMMA_TABLE_SIZE];
    gctUINT32                   enGamma[gcd2D_GAMMA_TABLE_SIZE];

    gce2D_SUPER_TILE_VERSION    superTileVersion;
    gctBOOL                     unifiedDstRect;

    gctBOOL                     multiBilinearFilter;

   /* XRGB */
    gctBOOL                     enableXRGB;

    gctBOOL                     forceHWStuck;

    gce2D_COMMAND               command;

    gcsRECT_PTR                 dstRect;
    gctUINT32                   dstRectCount;

} gcs2D_State;

typedef struct _gcsOPF_BLOCKSIZE_TABLE
{
    gctUINT8 width;
    gctUINT8 height;
    gctUINT8 blockDirect;
    gctUINT8 pixelDirect;
    gctUINT16 horizontal;
    gctUINT16 vertical;
}
gcsOPF_BLOCKSIZE_TABLE;

typedef gcsOPF_BLOCKSIZE_TABLE *  gcsOPF_BLOCKSIZE_TABLE_PTR;

extern gcsOPF_BLOCKSIZE_TABLE_PTR gcsOPF_TABLE_TABLE_ONEPIPE[];

/******************************************************************************\
******************************** gcoQUEUE Object *******************************
\******************************************************************************/

/* Construct a new gcoQUEUE object. */
gceSTATUS
gcoQUEUE_Construct(
    IN gcoOS Os,
    IN gceENGINE Engine,
    OUT gcoQUEUE * Queue
    );

/* Destroy a gcoQUEUE object. */
gceSTATUS
gcoQUEUE_Destroy(
    IN gcoQUEUE Queue
    );

/* Append an event to a gcoQUEUE object. */
gceSTATUS
gcoQUEUE_AppendEvent(
    IN gcoQUEUE Queue,
    IN gcsHAL_INTERFACE * Interface
    );

/* Commit and event queue. */
gceSTATUS
gcoQUEUE_Commit(
    IN gcoQUEUE Queue,
    IN gctBOOL Stall
    );

/* Commit and event queue. */
gceSTATUS
gcoQUEUE_Free(
    IN gcoQUEUE Queue
    );

#if gcdENABLE_3D
/*******************************************************************************
***** Vertex Management *******************************************************/

typedef struct _gcsVERTEXARRAY_ATTRIBUTE    gcsVERTEXARRAY_ATTRIBUTE;
typedef struct _gcsVERTEXARRAY_ATTRIBUTE *  gcsVERTEXARRAY_ATTRIBUTE_PTR;

struct _gcsVERTEXARRAY_ATTRIBUTE
{
    /* Pointer to vertex information. */
    gcsVERTEXARRAY_PTR                      vertexPtr;

    /* Pointer to next attribute in a stream. */
    gcsVERTEXARRAY_ATTRIBUTE_PTR            next;

    /* Logical address of this attribute. */
    gctCONST_POINTER                        logical;

    /* Format of this attribute. */
    gceVERTEX_FORMAT                        format;

    /* Offset into the stream of this attribute. */
    gctUINT                                 offset;

    /* Number of bytes of this attribute. */
    gctUINT                                 bytes;
};

typedef struct _gcsSTREAM_SUBSTREAM    gcsSTREAM_SUBSTREAM;
typedef struct _gcsSTREAM_SUBSTREAM *  gcsSTREAM_SUBSTREAM_PTR;
struct _gcsSTREAM_SUBSTREAM
{
    /* Current range for the sub-stream. */
    gctSIZE_T                               start;
    gctSIZE_T                               end;

    /* Maximum range of the window for the sub-stream. */
    gctSIZE_T                               minStart;
    gctSIZE_T                               maxEnd;

    /* Stride for the sub-stream. */
    gctUINT                                 stride;

    /* Pointer to associated gcoSTREAM object. */
    gcoSTREAM                               stream;

    /* Divisor for instanced drawing */
    gctUINT                                 divisor;

    /* Pointer to next sub-stream. */
    gcsSTREAM_SUBSTREAM_PTR                 next;
};

typedef struct _gcsVERTEXARRAY_STREAM
{
    /* Pointer to gcoSTREAM object. */
    gcoSTREAM                               stream;

    /* Logical address of stream data. */
    gctUINT8_PTR                            logical;

    /* Physical address of the stream data. */
    gctUINT32                               physical;

    /* Pointer to first attribute part of this stream. */
    gcsVERTEXARRAY_ATTRIBUTE_PTR            attribute;

    /* Pointer to sub-streams. */
    gcsSTREAM_SUBSTREAM_PTR                 subStream;
}
gcsVERTEXARRAY_STREAM,
* gcsVERTEXARRAY_STREAM_PTR;

#if OPT_VERTEX_ARRAY
typedef struct _gcsVERTEXARRAY_BUFFER
{
    /* Attribute Maps */
    gctUINT                                 map[gcdATTRIBUTE_COUNT];
    gctUINT                                 numAttribs;

    /* Indicate a buffer is combined from multi-buffer. */
    gctBOOL                                 combined;

    /* Logical address of stream data. */
    gctUINT8_PTR                            start;
    gctUINT8_PTR                            end;

    gctUINT8_PTR                            minStart;
    gctUINT8_PTR                            maxEnd;

    /* Stride for the sub-buffer. */
    gctUINT                                 stride;

    /* How many vertex required by app? DrawInstance should calc separately */
    gctUINT                                 count;

    /* Buffer offset in a stream. */
    gctUINT32                               offset;

    /* Divisor for the buffer */
    gctUINT32                               divisor;

}gcsVERTEXARRAY_BUFFER,
* gcsVERTEXARRAY_BUFFER_PTR;

typedef struct _gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE     gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE;
typedef struct _gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE *   gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR;

struct _gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE
{
    /* Format of this attribute. */
    gceVERTEX_FORMAT                        format;

    gctUINT32                               linkage;

    gctUINT32                               size;

    gctBOOL                                 normalized;

    gctBOOL                                 enabled;

    gctSIZE_T                               offset;

    gctCONST_POINTER                        pointer;

    gctUINT32                               bytes;

    gctBOOL                                 isPosition;

    gctUINT                                 stride;

    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR     next;
};

typedef struct _gcsVERTEXARRAY_BUFOBJ   gcsVERTEXARRAY_BUFOBJ;
typedef struct _gcsVERTEXARRAY_BUFOBJ * gcsVERTEXARRAY_BUFOBJ_PTR;

struct _gcsVERTEXARRAY_BUFOBJ
{
    gcoBUFOBJ                               stream;

    gcsSURF_NODE_PTR                        nodePtr;

    gctUINT                                 stride;

    gctUINT                                 divisor;

    gctUINT32                               physical;

    gctUINT8_PTR                            logical;

    gctSIZE_T                               count;

    gctUINT                                 dynamicCacheStride;
    gctSIZE_T                               streamCopySize;

    gctBOOL                                 copyAll;

    gctBOOL                                 merged;

    /* Attribute List */
    gctUINT                                 attributeCount;
    gcsVERTEXARRAY_BUFOBJ_ATTRIBUTE_PTR     attributePtr;

    gctSIZE_T                               maxAttribOffset;

    /* pointer to next stream */
    gcsVERTEXARRAY_BUFOBJ_PTR               next;
};

#endif

gceSTATUS
gcoSTREAM_SetAttribute(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Offset,
    IN gctUINT Bytes,
    IN gctUINT Stride,
    IN gctUINT Divisor,
    IN OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    );

gceSTATUS
gcoSTREAM_QuerySubStreams(
    IN gcoSTREAM Stream,
    IN gcsSTREAM_SUBSTREAM_PTR SubStream,
    OUT gctUINT_PTR SubStreamCount
    );

gceSTATUS
gcoSTREAM_Rebuild(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    OUT gctUINT_PTR SubStreamCount
    );

gceSTATUS
gcoSTREAM_SetCache(
    IN gcoSTREAM Stream
    );

gctSIZE_T
gcoSTREAM_GetSize(
        IN gcoSTREAM Stream
        );

#if OPT_VERTEX_ARRAY

gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT TotalBytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    );

gceSTATUS
gcoSTREAM_CacheAttributesEx(
    IN gcoSTREAM Stream,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gctUINT First,
    IN OUT gcoSTREAM * UncacheableStream
    );

gceSTATUS
gcoSTREAM_UploadUnCacheableAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT TotalBytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical,
    OUT gcoSTREAM * OutStream
);

#else

gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Stride,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    );

#endif

gceSTATUS
gcoSTREAM_UnAlias(
    IN gcoSTREAM Stream,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream,
    OUT gctUINT8_PTR * Logical,
    OUT gctUINT32 * Physical
    );

#if OPT_VERTEX_ARRAY
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
    );

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
    );

#else
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
    );
#endif

gceSTATUS
gcoVERTEX_AdjustStreamPoolEx(
    IN gcoSTREAM Stream,
    IN gcsVERTEXARRAY_BUFOBJ_PTR Streams,
    IN gctUINT StreamCount,
    IN gctUINT StartVertex,
    IN gctUINT FirstCopied,
    IN gctBOOL DrawElements,
    IN OUT gcoSTREAM * UncacheableStream
);

#if OPT_VERTEX_ARRAY
gceSTATUS
gcoVERTEX_AdjustStreamPool(
    IN gcoSTREAM Stream,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT TotalBytes,
    IN gctBOOL DrawArraysInstanced,
    IN OUT gctUINT32_PTR Physical,
    OUT gcoSTREAM * OutStream
);
#endif

gceSTATUS
gcoSTREAM_MergeStreams(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams,
    OUT gcoSTREAM * MergedStream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical,
    OUT gcsVERTEXARRAY_ATTRIBUTE_PTR * Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    );

gceSTATUS
gcoHARDWARE_HzClearValueControl(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 ZClearValue,
    OUT gctUINT32 * HzClearValue OPTIONAL,
    OUT gctUINT32 * Control OPTIONAL
    );

gceSTATUS
gcoHARDWARE_QueryShaderCompilerHwCfg(
    IN gcoHARDWARE Hardware,
    OUT PVSC_HW_CONFIG pVscHwCfg
    );

gceSTATUS
gcoHARDWARE_ProgramUniform(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT Columns,
    IN gctUINT Rows,
    IN gctCONST_POINTER Values,
    IN gctBOOL FixedPoint,
    IN gctBOOL ConvertToFloat,
    IN gcSHADER_KIND Type
    );

gceSTATUS
gcoHARDWARE_ProgramUniformEx(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT Columns,
    IN gctUINT Rows,
    IN gctUINT Arrays,
    IN gctBOOL IsRowMajor,
    IN gctUINT MatrixStride,
    IN gctUINT ArrayStride,
    IN gctCONST_POINTER Values,
    IN gceUNIFORMCVT Convert,
    IN gcSHADER_KIND Type
    );

gceSTATUS
gcoHARDWARE_BindUniformEx(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctINT32 Physical,
    IN gctUINT Columns,
    IN gctUINT Rows,
    IN gctUINT Arrays,
    IN gctBOOL IsRowMajor,
    IN gctUINT MatrixStride,
    IN gctUINT ArrayStride,
    IN gctCONST_POINTER Values[],
    IN gceUNIFORMCVT Convert,
    IN gcSHADER_KIND Type,
    IN gctBOOL CombinedMode /* every GPU has his own uniform value*/
    );

gceSTATUS
gcoHARDWARE_BindBufferBlock(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Base,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Size,
    IN gcSHADER_KIND Type
    );

gceSTATUS
gcoHARDWARE_QueryCompression(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    OUT gctBOOL *Compressed,
    OUT gctINT32 *CompressedFormat,
    OUT gctINT32 *CompressedDecFormat
    );

#endif /* gcdENABLE_3D */

gceSTATUS
gcoHARDWARE_MapCompressionFormat(
    IN  gctUINT32 InputFormat,
    OUT gctUINT32 *OutputFormat
);

gceSTATUS
gcoHARDWARE_GetSpecialHintData(
    IN gcoHARDWARE Hardware,
    OUT gctINT * Hint
    );

gceSTATUS
gcoOS_PrintStrVSafe(
    OUT gctSTRING String,
    IN gctSIZE_T StringSize,
    IN OUT gctUINT * Offset,
    IN gctCONST_STRING Format,
    IN gctARGUMENTS Arguments
    );

#if gcdENABLE_3D

gceSTATUS
gcoSURF_LockTileStatus(
    IN gcoSURF Surface
    );

gceSTATUS
gcoSURF_LockHzBuffer(
    IN gcoSURF Surface
    );

gceSTATUS
gcoSURF_AllocateTileStatus(
    IN gcoSURF Surface
    );

gceSTATUS
gcoSURF_AllocateHzBuffer(
    IN gcoSURF Surface
    );

gceSTATUS
gcoHARDWARE_FlushCache(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER *Memory
    );
#endif

gceSTATUS
gcsSURF_NODE_Construct(
    IN gcsSURF_NODE_PTR Node,
    IN gctSIZE_T Bytes,
    IN gctUINT Alignment,
    IN gceSURF_TYPE Type,
    IN gctUINT32 Flag,
    IN gcePOOL Pool
    );

gceSTATUS
gcsSURF_NODE_Destroy(
    IN gcsSURF_NODE_PTR Node
    );

gceSTATUS
gcsSURF_NODE_Lock(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE engine,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    );

gceSTATUS
gcsSURF_NODE_Unlock(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE engine
    );

gceSTATUS
gcsSURF_NODE_GetFence(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE engine,
    IN gceFENCE_TYPE Type
);

gceSTATUS
gcsSURF_NODE_WaitFence(
    IN gcsSURF_NODE_PTR Node,
    IN gceENGINE From,
    IN gceENGINE On,
    IN gceFENCE_TYPE Type
);

gceSTATUS
gcsSURF_NODE_IsFenceEnabled(
    IN gcsSURF_NODE_PTR Node
    );

gceSTATUS
gcoHARDWARE_GetProductName(
    IN gcoHARDWARE Hardware,
    OUT gctSTRING *ProductName,
    OUT gctUINT *PID
    );

gceSTATUS
gcoHARDWARE_GetBaseAddr(
    IN  gcoHARDWARE Hardware,
    OUT gctUINT32 *BaseAddr
    );

gceSTATUS
gcoHARDWARE_QuerySRAM(
    IN gcoHARDWARE Hardware,
    IN gcePOOL  Type,
    OUT gctUINT32 *Size,
    OUT gctUINT32 *GPUVirtAddr,
    OUT gctPHYS_ADDR_T *GPUPhysAddr,
    OUT gctUINT32 *GPUPhysName,
    OUT gctPHYS_ADDR_T *CPUPhysAddr
    );

/******************************************************************************
*******************************HW slot*****************************************
*******************************************************************************/
#if gcdENABLE_3D && gcdENABLE_KERNEL_FENCE
typedef enum
{
    gcvHWSLOT_INDEX,
    gcvHWSLOT_STREAM,
    gcvHWSLOT_TEXTURE,
    gcvHWSLOT_RT,
    gcvHWSLOT_DEPTH_STENCIL,
    gcvHWSLOT_BLT_SRC,
    gcvHWSLOT_BLT_DST,
}
gceHWSLOT_TYPE;

gceSTATUS
gcoHARDWARE_SetHWSlot(
    IN gcoHARDWARE Hardware,
    IN gceENGINE Engine,
    IN gctUINT type,
    IN gctUINT32 node,
    IN gctUINT32 slot
    );

gceSTATUS
gcoHARDWARE_BuildTimestampPatch(
    IN gcoHARDWARE Hardware,
    IN gcoBUFFER Buffer,
    IN gcoCMDBUF CommandBuffer,
    IN gceENGINE Engine
    );

gceSTATUS
gcoHARDWARE_ResetHWSlot(
    IN gcoHARDWARE Hardware
    );
#endif

/******************************************************************************
**********************gcsBITMAKS **********************************************
*******************************************************************************/

/*
** extended bitmask type to support flexiable bits operation bigger that native type
*/
#define gcmBITMASK_ELT_BITS     32
#define gcmBITMASK_ELT_TYPE     gctUINT32

#define gcmBITMASK_ELT_MAXNUM   4    /* (128 / gcmBITMASK_ELT_BITS) */

struct _gcsBITMASKFUNCS;

typedef struct _gcsBITMASK
{
    /* Multiple element array */
    gcmBITMASK_ELT_TYPE me[gcmBITMASK_ELT_MAXNUM];
    /* Number of elements in elts pointers. */
    gctUINT32 numOfElts;
    /* Size of bits */
    gctUINT32 size;
    /* Remained size */
    gctUINT32 remainedSize;
    /* op table */
    struct _gcsBITMASKFUNCS *op;

}
gcsBITMASK, * gcsBITMASK_PTR;

void
gcsBITMASK_InitAllOne(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 size
    );

void
gcsBITMASK_InitAllZero(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 size
    );

void
gcsBITMASK_InitOR(
    gcsBITMASK_PTR BitmaskResult,
    gcsBITMASK_PTR Bitmask1,
    gcsBITMASK_PTR Bitmask2
    );

void
gcsBITMASK_InitWithValue(
    gcsBITMASK_PTR BitmaskResult,
    gctUINT32 InitValue
    );

gctBOOL
gcsBITMASK_Test(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    );

gctBOOL
gcsBITMASK_TestAndClear(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    );

gctBOOL
gcsBITMASK_IsAllZero(
    gcsBITMASK_PTR Bitmask
    );

void
gcsBITMASK_Set(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    );

void
gcsBITMASK_Clear(
    gcsBITMASK_PTR Bitmask,
    gctUINT32 Loc
    );

void
gcsBITMASK_SetAll(
    gcsBITMASK_PTR Bitmask,
    gctBOOL AllOne
    );

void
gcsBITMASK_MergeBitMaskArray(
    gcsBITMASK_PTR BitmaskResult,
    gcsBITMASK_PTR *BitmaskArray,
    gctUINT32 Count
    );

void
gcsBITMASK_OR(
    gcsBITMASK_PTR BitmaskResult,
    gcsBITMASK_PTR Bitmask
    );

/******************************************************************************\
***************************** Patch Management *********************************
\******************************************************************************/

typedef struct _gcsSYMBOLSLIST      gcsSYMBOLSLIST;

struct _gcsSYMBOLSLIST
{
    gcePATCH_ID patchId;
    gctCONST_STRING symList[10];
};


#if gcdENABLE_3D
/******************************************************************************\
***************************** Compiler Function Table **************************
\******************************************************************************/
typedef struct _gcsVSC_APIS
{
    gceSTATUS (*gcCompileShader)(gctINT, gctUINT, gctCONST_STRING, gcSHADER*, gctSTRING*);

    gceSTATUS (*gcLinkShaders)(gcSHADER, gcSHADER, gceSHADER_FLAGS, gcsPROGRAM_STATE*);

    gceSTATUS (*gcSHADER_Construct)(gctINT, gcSHADER*);

    gceSTATUS (*gcSHADER_AddAttribute)(gcSHADER, gctCONST_STRING, gcSHADER_TYPE, gctUINT32,
                                       gctBOOL, gcSHADER_SHADERMODE, gcSHADER_PRECISION, gcATTRIBUTE*);

    gceSTATUS (*gcSHADER_AddUniform)(gcSHADER, gctCONST_STRING, gcSHADER_TYPE, gctUINT32, gcSHADER_PRECISION, gcUNIFORM*);

    gceSTATUS (*gcSHADER_AddOpcode)(gcSHADER, gcSL_OPCODE, gctUINT32, gctUINT8, gcSL_FORMAT, gcSHADER_PRECISION, gctUINT);

    gceSTATUS (*gcSHADER_AddOpcodeConditional)(gcSHADER, gcSL_OPCODE, gcSL_CONDITION, gctUINT, gctUINT);

    gceSTATUS (*gcSHADER_AddSourceUniformIndexedFormattedWithPrecision)(
        gcSHADER, gcUNIFORM, gctUINT8, gctINT,
        gcSL_INDEXED, gcSL_INDEXED_LEVEL, gctUINT16, gcSL_FORMAT, gcSHADER_PRECISION);

    gceSTATUS (*gcSHADER_AddSourceAttribute)(gcSHADER, gcATTRIBUTE, gctUINT8, gctINT);

    gceSTATUS (*gcSHADER_AddSourceConstant)(gcSHADER, gctFLOAT);

    gceSTATUS (*gcSHADER_AddOutput)(gcSHADER, gctCONST_STRING, gcSHADER_TYPE, gctUINT32, gctUINT32, gcSHADER_PRECISION Precision);

    gceSTATUS (*gcSHADER_SetCompilerVersion)(gcSHADER, gctUINT32_PTR);

    gceSTATUS (*gcSHADER_Pack)(gcSHADER);

    gceSTATUS (*gcSHADER_Destroy)(gcSHADER);

    gceSTATUS (*gcSHADER_Copy)(gcSHADER, gcSHADER);

    gceSTATUS (*gcSHADER_DynamicPatch)(gcSHADER, gcPatchDirective*, gctUINT);

    gceSTATUS (*gcCreateOutputConversionDirective)(IN gctINT                  OutputLocation,
                                                   IN gcsSURF_FORMAT_INFO_PTR FormatInfo,
                                                   IN gctUINT                 Layers,
                                                   IN gctBOOL                 AppendToLast,
                                                   OUT gcPatchDirective  **   PatchDirectivePtr);

    gceSTATUS (*gcCreateInputConversionDirective)(IN gcUNIFORM               Sampler,
                                                  IN gctINT                  ArrayIndex,
                                                  IN gcsSURF_FORMAT_INFO_PTR FormatInfo,
                                                  IN gceTEXTURE_SWIZZLE *    Swizzle,
                                                  IN gctUINT                 Layers,
                                                  IN gcTEXTURE_MODE          MipFilter,
                                                  IN gcTEXTURE_MODE          MagFilter,
                                                  IN gcTEXTURE_MODE          MinFilter,
                                                  IN gctFLOAT                LODBias,
                                                  IN gctINT                  Projected,
                                                  IN gctINT                  Width,
                                                  IN gctINT                  Height,
                                                  IN gctINT                  Depth,
                                                  IN gctINT                  Dimension,
                                                  IN gctINT                  MipLevelMax,
                                                  IN gctINT                  MipLevelMin,
                                                  IN gctBOOL                 SRGB,
                                                  IN gctBOOL                 AppendToLast,
                                                  IN gctBOOL                 DepthStencilMode,
                                                  IN gctBOOL                 NeedFormatConvert,
                                                  IN gcSHADER_KIND           ShaderKind,
                                                  OUT gcPatchDirective  **   PatchDirectivePtr);

    gceSTATUS (*gcFreeProgramState)(gcsPROGRAM_STATE);

    void      (*gcSetGLSLCompiler)(gctGLSLCompiler);

    gceSTATUS (*gcDestroyPatchDirective)(IN OUT gcPatchDirective **);

} gcsVSC_APIS;

gceSTATUS
gcoHAL_SetCompilerFuncTable(
    IN gcoHAL Hal,
    IN gcsVSC_APIS *VscAPIs
    );

gceSTATUS
gcoHARDWARE_SetCompilerFuncTable(
    IN gcoHARDWARE Hardware,
    IN gcsVSC_APIS *VscAPIs
    );

/******************************************************************************\
****************************** Blit/Clear Functions ****************************
\******************************************************************************/

gceSTATUS
gcoHARDWARE_CanDo3DBlitBlt(
    IN gceSURF_FORMAT srcFormat,
    IN gceSURF_FORMAT dstFormat
    );

gceSTATUS
gcoHARDWARE_DrawClear(
    IN gcsSURF_VIEW *RtView,
    IN gcsSURF_VIEW *DsView,
    IN gcsSURF_CLEAR_ARGS_PTR args
    );

gceSTATUS
gcoHARDWARE_DrawBlit(
    gcsSURF_VIEW *SrcView,
    gcsSURF_VIEW *DstView,
    gscSURF_BLITDRAW_BLIT *args
    );


/******************************************************************************
**********************gcsTXDescNode *******************************************
*******************************************************************************/

#define gcdMAX_TXDESC_ARRAY_SIZE 16

typedef struct _gcsTXDescNode
{
    gcsSURF_NODE_PTR            descNode[2];
    gctPOINTER                  descLocked[2];
} gcsTXDescNode;


gceSTATUS
gcoHAL_FreeTXDescArray(
    gcsTXDescNode *DescArray,
    gctINT CurIndex
    );


#endif

extern const gcsSAMPLES g_sampleInfos[];

#if gcdDUMP_2D
extern gctPOINTER dumpMemInfoListMutex;
extern gctBOOL    dump2DFlag;
#endif


#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_user_h_ */
