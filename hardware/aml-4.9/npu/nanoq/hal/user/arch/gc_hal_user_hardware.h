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


#ifndef __gc_hal_user_hardware_h_
#define __gc_hal_user_hardware_h_

#include "gc_hal_user_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define gcdSAMPLERS                 32
#define gcdSAMPLER_TS               8
#define gcdLOD_LEVELS               14
#define gcdTEMP_SURFACE_NUMBER      3

#define gcdTXDESCRIPTORS             80

#define gcdSTREAMS                  32

#define gcdMAX_XFB_BUFFERS          4

#define gcvINVALID_TEXTURE_FORMAT   ((gctUINT)(gceSURF_FORMAT) -1)
#define gcvINVALID_RENDER_FORMAT    ((gctUINT)(gceSURF_FORMAT) -1)

#define gcmTO_CHIP_ID(Index) (Hardware->chipIDs[Hardware->coreIndexs[Index]])

#define gcmENABLE3DCORE(Memory, CoreId) \
{ \
    if (Hardware->config->gpuCoreCount > 1) \
    { \
    *Memory++ = gcmSETFIELDVALUE(0, GCCMD_CHIP_ENABLE_COMMAND, OPCODE, CHIP_ENABLE) \
              | CoreId; \
    \
    Memory++; \
    \
    gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", CoreId); \
    }; \
}

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

typedef enum
{
    gcvFILTER_BLIT_KERNEL_UNIFIED,
    gcvFILTER_BLIT_KERNEL_VERTICAL,
    gcvFILTER_BLIT_KERNEL_HORIZONTAL,
    gcvFILTER_BLIT_KERNEL_NUM,
}
gceFILTER_BLIT_KERNEL;

#if gcdENABLE_3D

typedef enum
{
    gcvMSAA_DOWNSAMPLE_AVERAGE = 0,
    gcvMSAA_DOWNSAMPLE_SAMPLE  = 1,
}
gceMSAA_DOWNSAMPLE_MODE;


typedef struct _gcsCOLOR_TARGET
{
    gctUINT32                   format;
    gctBOOL                     superTiled;
    gcoSURF                     surface;
    /* sliceIndex to indicate which slice is */
    gctUINT32                   sliceIndex;
    /* layerIndx to indicate which layer is */
    gctUINT32                   layerIndex;
    gctUINT32                   sliceNum;

    gctUINT8                    colorWrite;

    /* colorWrite programmed to HW, which could
    ** be differnt for 32bit per-channel format.
    */
    gctUINT8                    hwColorWrite;
}
gcsCOLOR_TARGET;

typedef struct _gcsCOLOR_INFO
{
    /* per-RT state */
    gcsCOLOR_TARGET             target[gcdMAX_DRAW_BUFFERS];

    /* cacheMode depend on MSAA, consistent between RTs. */
    gctUINT32                   cacheMode;

    /* Do we need the MSAA cacheMode too?  Keep that to trigger 0x0E06, refine later */
    gceCACHE_MODE               cacheMode128;
    gctBOOL                     vMsaa;

    /*True if either RT need compression*/
    gctBOOL                     colorCompression;
    /*True if color pipe is not disabled */
    gctBOOL                     colorPipeEnabled;

    /* global color state */
    gctUINT8                    rop;
    gctBOOL                     anyPartialColorWrite;
    gctBOOL                     allColorWriteOff;
    gctUINT32                   destinationRead;
}
gcsCOLOR_INFO;

typedef struct _gcsDEPTH_INFO
{
    gcoSURF                     surface;
    gctUINT32                   sliceIndex;
    gctUINT32                   sliceNum;
    gctUINT32                   cacheMode;

    /* Some depth related register value */
    gctUINT32                   regDepthConfig;
    gctUINT32                   regControlHZ;
    gctUINT32                   regRAControl;
    gctUINT32                   regRAControlHZ;

    /* Depth config components. */
    gceDEPTH_MODE               mode;
    gctBOOL                     only;
    gctBOOL                     realOnly;
    gctBOOL                     early;
    gctBOOL                     write;
    gceCOMPARE                  compare;

    /* Depth range. */
    gctFLOAT                    near;
    gctFLOAT                    far;
}
gcsDEPTH_INFO;

typedef struct _gcsPA_INFO
{
    gctBOOL                     aaLine;
    gctUINT                     aaLineTexSlot;
    gctFLOAT                    aaLineWidth;

    gceSHADING                  shading;
    gceCULL                     culling;
    gctBOOL                     pointSize;
    gctBOOL                     pointSprite;
    gctBOOL                     primitiveId;
    gceFILL                     fillMode;

    gctBOOL                        wclip;
}
gcsPA_INFO;


typedef struct _gcsCENTROIDS * gcsCENTROIDS_PTR;
typedef struct _gcsCENTROIDS
{
    gctUINT32                   value[4];
}
gcsCENTROIDS;

typedef struct _gcsUNIFORM_STATE
{
    gctUINT32 address;
    gctUINT32 data[gcdMAX_3DGPU_COUNT][4];
    gctUINT32 combinedDirty;
    struct _gcsUNIFORM_STATE_INFO
    {
        gctUINT32                         dirty   : 4;  /* 4 component */
        gctUINT32                         type    : 4;  /* shader type count */
        gctUINT32                         index   : 24; /* constMax */
    } info;
}
gcsUNIFORM_STATE;
#endif

#if gcdSYNC
typedef enum{
    gcvFENCE_RLV,
    gcvFENCE_HW,
    gcvFENCE_CPU,
}gctFENCE;

typedef gceSTATUS
        (* pfn_WaitFence)(
        IN gcoHARDWARE Hardware,
        IN gcoFENCE Fence,
        IN gctUINT64 WaitID,
        IN gceENGINE CmdEngine,
        OUT gctPOINTER *Memory
        );

struct _gcoFENCE
{
    gctBOOL                     fenceEnable;
    gceENGINE                   engine;
    gctUINT64                   fenceID;
    gctUINT64                   fenceIDSend;
    gctUINT64                   commitID;
    gctBOOL                     addSync;
    gctUINT64                   resetTimeStamp;
    gctUINT                     loopCount;
    gctUINT                     delayCount;
    gctBOOL                     fromCommit;

    gctFENCE                    type;
    gctINT32                    id;

    pfn_WaitFence               waitFunc[gcvENGINE_ALL_COUNT];
    gctUINT64                   waitID[gcvENGINE_ALL_COUNT];

    union{
        struct{
            gcoSURF             fenceSurface;
            gcoSURF             srcIDSurface;
            gctUINT32           srcWidth,srcHeight;
            gctUINT32           srcX,srcY;
        }rlvFence;

        struct{
            gcsSURF_NODE_PTR    dstSurfNode;
            gctPOINTER          dstAddr;
            gctUINT32           dstPhysic;
        }hwFence;
    }u;
};
#endif

typedef struct _gcsHARDWARE_CONFIG
{
    /* Chip registers. */
    gceCHIPMODEL                chipModel;
    gctUINT32                   chipRevision;
    gctUINT32                   productID;
    gctUINT32                   customerID;
    gctUINT32                   ecoID;

    gceCHIP_FLAG                chipFlags;

    gctUINT64                   platformFlagBits;

    /* hw features fields. */
    gctUINT32                   chipFeatures;
    gctUINT32                   chipMinorFeatures;
    gctUINT32                   chipMinorFeatures1;
    gctUINT32                   chipMinorFeatures2;
    gctUINT32                   chipMinorFeatures3;
    gctUINT32                   chipMinorFeatures4;
    gctUINT32                   chipMinorFeatures5;
    gctUINT32                   chipMinorFeatures6;

    /* Data extracted from specs bits. */
#if gcdENABLE_3D
    /* gcChipSpecs. */
    gctUINT32                   streamCount;    /* number of vertex streams */
    gctUINT32                   attribCount;    /* number of attributes. */
    gctUINT32                   registerMax;
    gctUINT32                   threadCount;
    gctUINT32                   vertexCacheSize;
    gctUINT32                   shaderCoreCount;
#endif
    gctUINT32                   pixelPipes;
    gctUINT32                   resolvePipes;
    gctUINT32                   gpuCoreCount;
    gctUINT32                   clusterCount;
    gctINT32                    clusterMaxID;
    gctINT32                    clusterMinID;
    gctUINT32                   clusterIDWidth;
    gctUINT32                   clusterAliveMask; /* physical cluster mask really enabled, may not all clusters */
    gctUINT32                   uscCacheControllers;
    gctUINT32                   uscBanks;
    const gctCHAR               *productName;
#if gcdENABLE_3D
    gctUINT32                   vertexOutputBufferSize;
    /* gcChipSpecs2. */
    gctUINT32                   bufferSize;     /* not used? */
    gctUINT32                   instructionCount;
    gctUINT32                   numConstants;
    /* gcChipSpecs3. */
    gctUINT32                   varyingsCount;
    /* total USC size including cache part */
    gctUINT32                   localStorageSizeInKbyte;
    gctUINT32                   l1CacheSizeInKbyte;
    /* gcChipSpecs4. */
    gctUINT32                   instructionMemory;
    gctUINT32                   shaderPCLength;
    /*gctUINT32                   streamCount1;     number of streams */

    /* Derived data. */
    gctUINT                     unifiedConst;
    gctUINT                     vsConstBase;
    gctUINT                     psConstBase;
    gctUINT                     vsConstMax;
    gctUINT                     psConstMax;
    gctUINT                     constMax;

    gctUINT                     unifiedInst;
    gctUINT                     vsInstBase;
    gctUINT                     psInstBase;
    gctUINT                     vsInstMax;
    gctUINT                     psInstMax;
    gctUINT                     instMax;
    gctUINT                     uscPagesMaxInKbyte; /* always same as localStorageSizeInKbyte */
    gctUINT                     resultWindowMaxSize;

    /* Info not in features/specs. */
#endif

#if gcdENABLE_3D
    gctUINT32                   renderTargets; /* num of mRT */
#endif

    gctUINT32                   superTileMode;

#if gcdENABLE_3D && gcdUSE_VX
    vx_nn_config                nnConfig;
    vx_hw_chip_info             hwChipInfo;
#endif
    gctBOOL                     parallelNoFix;
}
gcsHARDWARE_CONFIG;


#if gcdENABLE_3D

#define gcmMAX_VARIATION_NUM 32

/* All after-link state should be here */
typedef struct _gcsPROGRAM_STATE_VARIATION
{
    gcsPROGRAM_STATE programState;
    /* key for this variation */
    gceSURF_FORMAT srcFmt;
    gceSURF_FORMAT destFmt;

    /* LRU list, not used for now */
    gctINT32 prev;
    gctINT32 next;
}
gcsPROGRAM_STATE_VARIATION;

typedef struct _gcsHARDWARE_BLITDRAW
{
    gcSHADER vsShader[gcvBLITDRAW_NUM_TYPE];

    gcSHADER psShader[gcvBLITDRAW_NUM_TYPE];

    /* sampler uniform for blit shader */
    gcUNIFORM  bliterSampler;

    /* color uniform for clear shader */
    gcUNIFORM  clearColorUnfiorm;

    /* stream pointer.*/
    gcoSTREAM dynamicStream;

    gcsPROGRAM_STATE_VARIATION programState[gcvBLITDRAW_NUM_TYPE][gcmMAX_VARIATION_NUM];

#if !gcdSTATIC_LINK && !defined(__CHROME_OS__)
    gcsVSC_APIS vscAPIs;
    gctHANDLE vscLib;
    gctHANDLE glslcLib;
#endif

    gctINT descCurIndex;
    gcsTXDescNode descArray[gcdMAX_TXDESC_ARRAY_SIZE];

}
gcsHARDWARE_BLITDRAW, * gcsHARDWARE_BLITDRAW_PTR;

typedef enum _gceMSAA_FRAGOP
{
    gcvMSAA_FRAGOP_ALPHA_TO_COVERAGE  = 1 << 0,
    gcvMSAA_FRAGOP_SAMPLE_TO_COVERAGE = 1 << 1,
    gcvMSAA_FRAGOP_SAMPLE_MASK        = 1 << 2,
}
gceMSAA_FRGAOP;

typedef struct _gcsXfbBuffer
{
    gctUINT32 address;
    gctUINT32 stride;
    gctUINT32 size;
}
gcsXfbBuffer;

typedef struct _gcsTEXTUREDESCRIPTORREGS
{
    gctUINT32 gcregTXAddress[16];
    gctUINT32 gcregTXConfig;
    gctUINT32 gcregTXSize;
    gctUINT32 gcregTXLinearStride;
    gctUINT32 gcregTXExtConfig;
    gctUINT32 gcregTXControlYUVEx;
    gctUINT32 gcregTXStrideYUVEx;
    gctUINT32 gcregTXASTC0Ex;
    gctUINT32 gcregTXASTC1Ex;
    gctUINT32 gcregTXASTC2Ex;
    gctUINT32 gcregTXASTC3Ex;
    gctUINT32 gcregTXBaseLOD;
    gctUINT32 gcregTXConfig2;
    gctUINT32 gcregTXConfig3;
    gctUINT32 gcregTXSizeExt;
    gctUINT32 gcregTXVolumeExt;
    gctUINT32 gcregTXSlice;
    gctUINT32 gcregTXBorderColor;
    gctUINT32 gcregTX3D;
    gctUINT32 gcregTXLogSize;

    /* gctUINT32 gcregTXLodExt; */
    /* gctUINT32 gcregTXLodBiasExt; */

    gctUINT32 gcregTXBorderColorBlue32;
    gctUINT32 gcregTXBorderColorGreen32;
    gctUINT32 gcregTXBorderColorRed32;
    gctUINT32 gcregTXBorderColorAlpha32;
}
gcsTEXTUREDESCRIPTORREGS;

typedef struct _gcsHARDWARE_ExeFuncs
{
    /* Texture programming func */
    gceSTATUS (*programTexture)(IN gcoHARDWARE Hardware, INOUT gctPOINTER *Memory);
}
gcsHARDWARE_ExeFuncs;

typedef struct _gcsHARDWARE_SLOT
{
    /* 3D render engine slot */
    struct{
        /* used count */
        gctUINT32         streamCount;
        /* hw max support count */
        gctUINT32         streamMaxCount;
        /* all chip set max array to record node */
        gctUINT32         streams[gcdSTREAMS];

        gctUINT32         index;

        gctUINT32         textureCount;
        gctUINT32         textureMaxCount;
        gctUINT32         textures[gcdSAMPLERS];

        gctUINT32         rtCount;
        gctUINT32         rtMaxCount;
        gctUINT32         rt[8];

        gctUINT32         depthStencil;
    }renderEngine;

    struct{
        /* BLT engine slot */
        gctUINT32         bltSrc;
        gctUINT32         bltDst;
    }bltEngine;
}
gcsHARDWARE_SLOT;
#endif

struct _gcoENGINE
{
    /* Command buffer. */
    gcoBUFFER                   buffer;
    /* Event queue. */
    gcoQUEUE                    queue;

    /* Stall signal. */
    gctSIGNAL                   stallSignal;

    /* Idle flag, idle when stalled. */
    gctBOOL                     idle;

};

#if gcdENABLE_3D
/******* FE states ***********************
*/
typedef struct _gcsFESTATES
{
    gctUINT32                   indexAddress;
    gctUINT32                   indexEndAddress;
    gctUINT32                   indexFormat;
    gctUINT32                   indexEndian;
    gctBOOL                     primitiveRestart;
    gctUINT32                   restartElement;
}gcsFESTATES;

/******* FE dirty ***********************
*/
typedef struct _gcsFEDIRTY
{
    gctBOOL                     indexDirty;
}gcsFEDIRTY;

/******* PA/SE states ****************************
*/
typedef struct _gcsPAANDSESTATES
{
    gcsRECT                     viewportStates;
    gcsRECT                     scissorStates;
    gcsPA_INFO                  paStates;
}gcsPAANDSESTATES;

/******* PA/SE dirty ****************************
*/
typedef struct _gcsPAANDSEDIRTY
{
    gctBOOL                     viewportDirty;
    gctBOOL                     scissorDirty;
    gctBOOL                     paConfigDirty;
    gctBOOL                     paLineDirty;
}gcsPAANDSEDIRTY;

/******* MSAA states ****************************
*/
typedef struct _gcsMSAASTATES
{
    /* Anti-alias mode. */
    gctUINT32                   sampleMask;
    gctUINT32                   sampleEnable;
    gcsSAMPLES                  sampleInfo;
    gctUINT32                   jitterIndex;
    gctUINT32                   sampleCoords2;
    gctUINT32                   sampleCoords4[3];
    gcsCENTROIDS                centroids2;
    gcsCENTROIDS                centroids4[3];

    /* Sample shading state */
    gctBOOL                     sampleShading;
    gctUINT                     minSampleShadingValue;
    gctBOOL                     sampleMaskOut;
    gctINT                      sampleMaskLoc;
    gctBOOL                     sampleShadingByPS;
    gctUINT                     sampleShadingValue;
    gctBOOL                     isSampleIn;
    gctBOOL                     useSubSampleZInPS;
    gctBOOL                     sampleMaskInPos;
    gctBOOL                     EarlyDepthFromAPP;
    gctBOOL                     psReadZ;
    gctBOOL                     psReadW;
    gctBOOL                     disableRAWriteDepth;

    /* MSAA operation state */
    gctUINT                     MsaaFragmentOp;
}gcsMSAASTATES;

/******* MSAA dirty ****************************
*/
typedef struct _gcsMSAADIRTY
{
    /* When support Sample Shading, these dirty also control some PS states. */
    gctBOOL                     msaaConfigDirty;
    gctBOOL                     msaaModeDirty;

    gctBOOL                     centroidsDirty;
}gcsMSAADIRTY;

/******** SH state ********************************
*/
typedef struct _gcsSHSTATES
{
    gcsPROGRAM_STATE            programState;

    /* ps output-> RT mapping
    ** in:  sequential number from 0 to numRT-1;
    ** out: RT index set up by APP.
    */
    gctINT32                    psOutputMapping[gcdMAX_DRAW_BUFFERS];

    /* Layered rendering state, shared by RT and depth */
    gctBOOL                     rtLayered;
    gctBOOL                     shaderLayered;
    gctUINT                     maxLayers;

    /* Rounding mode */
    gctBOOL                     rtneRounding;

    /*record the const/sampler count when program shader uniform/sampler allocation size*/
    gctUINT                     constCount;
    gctUINT                     samplerCount;

#if gcdALPHA_KILL_IN_SHADER
    gctBOOL                     alphaKill;
    gctBOOL                     colorKill;
#endif
    gcsUNIFORM_STATE*           uniformStatesPool;
    gctUINT32*                  uniformStatesIndex;
    gctUINT32                   uniformBindCount;
}gcsSHSTATES;

/******** SH dirty ********************************
*/
typedef struct _gcsSHDIRTY
{
    gctBOOL                     shaderDirty;
    gctBOOL                     programSwitched;
    gctBOOL                     uniformDirty;
}gcsSHDIRTY;

/********* PE states *********************************
*/
typedef struct _gcsPESTATES
{
    gctBOOL                     earlyDepth;
    /* Render target states. */
    gcsCOLOR_INFO               colorStates;

    /* Depth states. */
    gcsDEPTH_INFO               depthStates;
    gctBOOL                     disableAllEarlyDepth;
    gctBOOL                     disableAllEarlyDepthFromStatistics;

    /* Alpha states. */
    gcsALPHA_INFO               alphaStates;

    /* Stencil states. */
    gctBOOL                     stencilEnabled;
    gctBOOL                     stencilKeepFront[3];
    gctBOOL                     stencilKeepBack[3];

    gcsSTENCIL_INFO             stencilStates;

    gctUINT32                   colorOutCount; /* num of mRT */

    /* Depth clipping plane in the clipping space. */
    gcuFLOAT_UINT32             depthNear;
    gcuFLOAT_UINT32             depthFar;

    /* Maximum depth value. */
    gctUINT32                   maxDepth;

    /* Dither. */
    gctUINT32                   ditherTable[3][2];
    gctBOOL                     ditherEnable;

    /* True if either one RT or DS bpp is <= 16 */
    gctBOOL                     singleBuffer8x4;

    /* True if MRT but one (not all) is 8bit and MSAA off*/
    gctBOOL                     hasOne8BitRT;

    gctBOOL                     singlePEpipe;
    gctUINT32                   peConfigExReg;
}gcsPESTATES;

/********* PE dirty *********************************
*/
typedef struct _gcsPEDIRTY
{
    gctBOOL                     colorConfigDirty;
    gctBOOL                     colorTargetDirty;
    gctBOOL                     depthConfigDirty;
    gctBOOL                     depthRangeDirty;
    gctBOOL                     depthNormalizationDirty;
    gctBOOL                     depthTargetDirty;
    gctBOOL                     alphaDirty;
    gctBOOL                     stencilDirty;
    gctBOOL                     peDitherDirty;
}gcsPEDIRTY;


/********* Texture states ****************************
*/
typedef struct _gcsTXSTATES
{
    gctBOOL                     hwTxLODGuardbandEnable;
    gctUINT32                   hwTxLODGuardbandIndex;

#if gcdSECURITY
    gctUINT32                   txLodNums[gcdSAMPLERS];
#endif

    gctUINT32                   hwTxSamplerMode[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerModeEx[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerMode2[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerSize[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerSizeLog[gcdSAMPLERS];

    gctUINT32                   hwTxSampler3D[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerLOD[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerBaseLOD[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerAddress[gcdLOD_LEVELS][gcdSAMPLERS];

    gctUINT32                   hwTxSamplerYUVControl[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerYUVStride[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerLinearStride[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerASTCSize[gcdLOD_LEVELS][gcdSAMPLERS];
    gctUINT32                   hwTxSamplerASTCSRGB[gcdLOD_LEVELS][gcdSAMPLERS];

    /* For highp UV */
    gctUINT32                   hwTxSamplerSizeLogExt[gcdSAMPLERS];

    gctUINT32                   hwTxSampler3DExt[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerLodExt[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerLodBiasExt[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerAnisoCtrl[gcdSAMPLERS];

    gctUINT32                   hwTxSamplerConfig3[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerSliceSize[gcdSAMPLERS];

    /* Texture descriptor setting */
    gctUINT32                   hwSamplerControl0[gcdTXDESCRIPTORS];
    gctUINT32                   hwSamplerControl1[gcdTXDESCRIPTORS];
    gctUINT32                   hwSamplerLodMinMax[gcdTXDESCRIPTORS];
    gctUINT32                   hwSamplerLODBias[gcdTXDESCRIPTORS];
    gctUINT32                   hwSamplerAnisCtrl[gcdTXDESCRIPTORS];
    gctUINT32                   hwTxDescAddress[gcdTXDESCRIPTORS];
    gctUINT32                   hwTextureControl[gcdTXDESCRIPTORS];

    gcsSURF_NODE_PTR            nullTxDescNode[2];
    gctPOINTER                  nullTxDescLocked[2];
}gcsTXSTATES;

/********* Texture dirty ****************************
*/
typedef struct _gcsTXDIRTY
{
    gctBOOL                     hwTxFlushVS;
    gctBOOL                     hwTxFlushPS;
    gctBOOL                     hwTxFlushL1Cache;
    gctBOOL                     hwTxDirty;
    gctUINT32                   hwTxSamplerModeDirty;
    gctUINT32                   hwTxSamplerSizeDirty;
    gctUINT32                   hwTxSamplerSizeLogDirty;
    gctUINT32                   hwTxSampler3DDirty;
    gctUINT32                   hwTxSamplerLODDirty;
    gctUINT32                   hwTxSamplerBaseLODDirty;
    gctUINT32                   hwTxSamplerAddressDirty[gcdLOD_LEVELS];
    gctUINT32                   hwTxSamplerYUVControlDirty;
    gctUINT32                   hwTxSamplerYUVStrideDirty;
    gctUINT32                   hwTxSamplerLinearStrideDirty;
    gctUINT32                   hwTxSamplerASTCDirty[gcdLOD_LEVELS];
    gctUINT32                   hwTxSamplerSizeLogExtDirty;
    gctUINT32                   hwTxSampler3DExtDirty;
    gctUINT32                   hwTxSamplerLodExtDirty;
    gctUINT32                   hwTxSamplerLodBiasExtDirty;
    gctUINT32                   hwTxSamplerAnisoCtrlDirty;
    gctUINT32                   hwTxSamplerConfig3Dirty;

    /* Conventional sampler dirty */
    gctUINT32                   hwTxSamplerDirty;

    /* Texture descriptor setting */
    gcsBITMASK                  hwSamplerBindDirty;
    gcsBITMASK                  hwSamplerControl0Dirty;
    gcsBITMASK                  hwSamplerControl1Dirty;
    gcsBITMASK                  hwSamplerLodMinMaxDirty;
    gcsBITMASK                  hwSamplerLODBiasDirty;
    gcsBITMASK                  hwSamplerAnisCtrlDirty;
    gcsBITMASK                  hwTxDescAddressDirty;
    gcsBITMASK                  hwTextureControlDirty;

    gcsBITMASK                  hwTxDescDirty;

    /* sampler dirty or tex TS dirty */
    gctBOOL                     textureDirty;
}gcsTXDIRTY;

/********* MC States *********************************
*/
typedef struct _gcsMCSTATES
{
    /* texture tile status information */
    gcoSURF                     texBaseLevelSurfInfoWithTS[gcdTXDESCRIPTORS];
    gctBOOL                     texHasTileStatus[gcdTXDESCRIPTORS];
    gctINT                      texTileStatusSlotIndex[gcdTXDESCRIPTORS];
    gctINT                      texTileStatusSlotUser[gcdSAMPLER_TS];

    /* Texture tile status buffer setting */
    gctUINT32                   hwTXSampleTSConfig[gcdSAMPLER_TS];
    gctUINT32                   hwTXSampleTSBuffer[gcdSAMPLER_TS];
    gctUINT32                   hwTXSampleTSClearValue[gcdSAMPLER_TS];
    gctUINT32                   hwTXSampleTSClearValueUpper[gcdSAMPLER_TS];
    gctUINT32                   hwTxSamplerTxBaseBuffer[gcdSAMPLER_TS];

    /* Tile status information. */
    gctUINT32                   memoryConfig;
    gctUINT32                   memoryConfigMRT[gcdMAX_DRAW_BUFFERS];
    gctBOOL                     paused;
    gctBOOL                     inFlush;
}gcsMCSTATES;

/********* MC dirty *********************************
*/
typedef struct _gcsMCDIRTY
{
    gctINT32                    texTileStatusSlotDirty;

    /* Texture tile status buffer setting */
    gctUINT32                   hwTxSamplerTSDirty;

    gctBOOL                     cacheDirty;
}gcsMCDIRTY;

/********* XFB States *******************************
*/
typedef struct _gcsXFBSTATES
{
    /* Xfb */
    gceXfbStatus status;
    /* record xfb stauts in cmd buffer */
    gceXfbStatus statusInCmd;
    gceXfbCmd cmd;
    gctUINT32  headerPhysical;
    gcsXfbBuffer  buffer[gcdMAX_XFB_BUFFERS];

    gctBOOL     internalXFB;
    gcsSURF_NODE_PTR internalXFBNode;
    gctPOINTER   internalXFBLocked;

    /* Raster discard */
    gctBOOL  enableDiscard;
}gcsXFBSTATES;

/********* XFB dirty *********************************
*/
typedef union _gcsXFBDIRTY
{
    struct {
        gctUINT                   cmdDirty          : 1;
        gctUINT                   headerDirty       : 1;
        gctUINT                   bufferDirty       : 1;
        gctUINT                   internalXFBDirty  : 1;
        gctUINT                   discardDirty      : 1;
        gctUINT                   reserved          : 27;
    } s;

    gctUINT                       xfbDirty;

}gcsXFBDIRTY;

/********* QUERY States *******************************
*/
typedef struct _gcsQUERYSTATES
{
    gceQueryStatus              queryStatus[gcvQUERY_MAX_NUM];
    gctUINT32                   queryHeaderPhysical[gcvQUERY_MAX_NUM];
    gctINT32                    queryHeaderIndex[gcvQUERY_MAX_NUM];
}gcsQUERYSTATES;


#endif

/* gcoHARDWARE object. */
struct _gcoHARDWARE
{
    /* Object. */
    gcsOBJECT                   object;

    /* Default hardware object or not for this thread */
    gctBOOL                     threadDefault;

    /* Hardware is robust and automatically do OOB check */
    gctBOOL                     robust;

    /* Core array. */
    gceCORE                     core[4];

    /* Handle of gckCONTEXT object. */
    gctUINT32                   context;

    gctUINT32                   *contexts;

    /* Number of states managed by the context manager. */
    gctUINT32                   maxState;

    gctUINT32                   numStates;

    struct _gcoENGINE           engine[gcvENGINE_GPU_ENGINE_COUNT];

    /* Context buffer. */
    gcePIPE_SELECT              currentPipe;

    /* List of state delta buffers. */
    gcsSTATE_DELTA_PTR          delta;

    gcsSTATE_DELTA_PTR          *deltas;
    gcsSTATE_DELTA_PTR          tempDelta;

    /* Count of deltas it is needed because gpuCoreCount will be changed. */
    gctUINT32                    deltasCount;

    /* Chip configuration. */
    gcsHARDWARE_CONFIG *        config;
    gctBOOL                     features[gcvFEATURE_COUNT];
    gcsHAL_QUERY_CHIP_OPTIONS   options;

    gctUINT32                   mcClk;
    gctUINT32                   shClk;

#if gcdENABLE_3D
    /* API type. */
    gceAPI                      api;

    /* Current API type. */
    gceAPI                      currentApi;

    gctUINT                     unifiedConst;
    gctUINT                     vsConstBase;
    gctUINT                     psConstBase;
    gctUINT                     vsConstMax;
    gctUINT                     psConstMax;
    gctUINT                     constMax;

    gctUINT                     unifiedInst;
    gctUINT                     vsInstBase;
    gctUINT                     psInstBase;
    gctUINT                     vsInstMax;
    gctUINT                     psInstMax;
    gctUINT                     instMax;
    /* gctUINT32                   gpuCoreCount; */
    gceMULTI_GPU_RENDERING_MODE gpuRenderingMode;
    gceCORE_3D_MASK             chipEnable;
    gceMULTI_GPU_MODE           gpuMode;
    gctUINT                     interGpuSemaphoreId;
#endif

    gctINT                      needStriping;
    gctBOOL                     mixedStreams;

    /* Big endian */
    gctBOOL                     bigEndian;

    /* Special hint flag */
    gctUINT32                   specialHint;

    gctINT                      specialHintData;

    /* Temporary buffer parameters. */
    struct _gcoSURF             tmpSurf;

    /* Temporary render target for MSAA/depth-only case */
    struct _gcoSURF             tmpRT;

    /* Temporary surface for 2D. */
    gcoSURF                     temp2DSurf[gcdTEMP_SURFACE_NUMBER];

     /*  The hardware need a read operation when faked generic attrib if no vs input . */
    gcsSURF_NODE                tempBuffer;

    /* Filter blit. */
    struct __gcsLOADED_KERNEL_INFO
    {
        gceFILTER_TYPE              type;
        gctUINT8                    kernelSize;
        gctUINT32                   scaleFactor;
        gctUINT32                   kernelAddress;
    } loadedKernel[gcvFILTER_BLIT_KERNEL_NUM];

#if gcdENABLE_3D
    gctBOOL                     flushedColor;
    gctBOOL                     flushedDepth;

    gcsHARDWARE_BLITDRAW_PTR    blitDraw;

    /* Multi-pixel pipe information. */
    gctUINT32                   resolveAlignmentX;
    gctUINT32                   resolveAlignmentY;
    /* If true, have to use multipipe to do resolve.
    ** else on single-pipe hardware, always single-pipe
    ** on multi-pipe hardware, it can switch between one
    ** or two pipes.
    */
    gctBOOL                     multiPipeResolve;


    /* State to collect flushL2 requests, and process it at draw time. */
    gctBOOL                     flushL2;

    /* State to collect flush vertex data cache requests, and process it at draw time. */
    gctBOOL                     flushVtxData;

    gcsHARDWARE_ExeFuncs        *funcPtr;

    /* Previsous states. */
    gctBOOL                     prevEarlyZ;
    gctBOOL                     prevHZ;
    gctBOOL                     preRADepth;
    gceSTENCIL_MODE             prevStencilMode;
    gctBOOL                     previousPEDepth;
    gceCOMPARE                  prevDepthCompare;
    gcsPROGRAM_UNIFIED_STATUS   prevProgramUnfiedStatus;
    gcePROGRAM_STAGE_BIT        prevProgramStageBits;
    gctBOOL                     prevProgramBarrierUsed;

    gctBOOL                     prevSingleCore;
    gceCACHE_MODE               preColorCacheMode128;
    gceCACHE_MODE               preDepthCacheMode128;

    /* HW Module states. */
    gcsFESTATES                 *FEStates;
    gcsPAANDSESTATES            *PAAndSEStates;
    gcsMSAASTATES               *MsaaStates;
    gcsSHSTATES                 *SHStates;
    gcsPESTATES                 *PEStates;
    gcsMCSTATES                 *MCStates;
    gcsTXSTATES                 *TXStates;
    gcsXFBSTATES                *XFBStates;
    gcsQUERYSTATES              *QUERYStates;
    gcsPROBESTATES              *PROBEStates;

    /* HW Module states dirty. */
    gcsFEDIRTY                  *FEDirty;
    gcsPAANDSEDIRTY             *PAAndSEDirty;
    gcsMSAADIRTY                *MsaaDirty;
    gcsSHDIRTY                  *SHDirty;
    gcsPEDIRTY                  *PEDirty;
    gcsMCDIRTY                  *MCDirty;
    gcsTXDIRTY                  *TXDirty;
    gcsXFBDIRTY                 *XFBDirty;

    /* MCFE semaphore handles. */
    gctUINT32 *                 mcfeSemaBitmap;
    gctUINT32                   mcfeSemaCapacity;
    gctUINT32                   mcfeSemaFreeCount;
    gctUINT32                   currentMcfeSemaHandle;
#endif /* gcdENABLE_3D */

    gctBOOL                     multiGPURenderingModeDirty;
    gctBOOL                     GPUProtecedModeDirty;

    /* Stall from source to destination if it's legal */
    gceWHERE                    stallSource;
    gceWHERE                    stallDestination;

    /***************************************************************************
    ** 2D states.
    */

    /* 2D hardware availability flag. */
    gctBOOL                     hw2DEngine;

    /* 3D hardware availability flag. */
    gctBOOL                     hw3DEngine;

    /* Software 2D force flag. */
    gctBOOL                     sw2DEngine;

    /* Byte write capability. */
    gctBOOL                     byteWrite;

    /* Need to shadow RotAngleReg? */
    gctBOOL                     shadowRotAngleReg;

    /* The shadow value. */
    gctUINT32                   rotAngleRegShadow;

    /* Support L2 cache for 2D 420 input. */
    gctBOOL                     hw2D420L2Cache;
    gctBOOL                     needOffL2Cache;

#if gcdENABLE_3D
    /* OpenCL threadwalker in PS. */
    gctBOOL                     threadWalkerInPS;
#endif

    /* Temp surface for fast clear */
    gcoSURF                     tempSurface;

    /* Blt optimization. */
    gceSURF_ROTATION            srcRot;
    gceSURF_ROTATION            dstRot;
    gctBOOL                     horMirror;
    gctBOOL                     verMirror;
    gcsRECT                     clipRect;
    gctBOOL                     srcRelated;

    gctUINT32_PTR               hw2DCmdBuffer;
    gctUINT32                   hw2DCmdIndex;
    gctUINT32                   hw2DCmdSize;

    gctBOOL                     hw2DSplitRect;

    gctUINT32                   hw2DAppendCacheFlush;
    gctUINT32                   hw2DCacheFlushAfterCompress;
    gctUINT32_PTR               hw2DCacheFlushCmd;
    gcoSURF                     hw2DCacheFlushSurf;

    gcoSURF                     hw2DClearDummySurf;
    gcsRECT_PTR                 hw2DClearDestRect;

    gctBOOL                     hw2DCurrentRenderCompressed;
    gcoSURF                     hw2DDummySurf;

    gctBOOL                     hw2DDoMultiDst;

    gctBOOL                     hw2DBlockSize;

    gctBOOL                     hw2DQuad;

    gcePATCH_ID                 patchID;

#if gcdSYNC
    gcoFENCE                    fence[gcvENGINE_GPU_ENGINE_COUNT];
    gctBOOL                     fenceEnabled;
#endif

   /* XRGB */
    gctBOOL                     enableXRGB;

    gcoSURF                     clearSrcSurf;
    gcsRECT                     clearSrcRect;

    gcoSURF                     blitTmpSurf;

    gctBOOL                     notAdjustRotation;

#if gcdDUMP
    gctPOINTER                  contextLogical[gcdCONTEXT_BUFFER_COUNT];
    gctUINT32                   contextBytes;
    gctUINT8                    currentContext;
#endif

#if gcdDUMP_2D
    gctUINT32                   newDump2DLevel;
#endif

    /* baseAddr for old MMU without MC20 feature */
    gctUINT32                   baseAddr;

    /* Flat mapping range. */
    gctUINT32                   flatMappingRangeCount;
    gcsFLAT_MAPPING_RANGE       flatMappingRanges[gcdMAX_FLAT_MAPPING_COUNT];

#if gcdENABLE_3D
    gcsHARDWARE_SLOT           *hwSlot;
    gctUINT64                   resetTimeStamp;
#endif

    gctPOINTER                  featureDatabase;

    /* Chip ID of multiple GPUs. */
    gctUINT                     chipIDs[gcvCORE_COUNT];

    /* Core Index of multiple GPUs. */
    gctUINT                     coreIndexs[gcvCORE_COUNT];

    gceHARDWARE_TYPE            constructType;

#if gcdENABLE_3D
    /* For Mixed streams bug.*/
    gctBOOL                     bForceVirtual;
    /* quick reference */
    gctBOOL                     streamRegV2;
    gctUINT32                   streamAddressState;
    gctUINT32                   streamStrideState;
    gctUINT32                   shaderCtrlState;
    gctUINT32                   genericWState;
#endif
};

#if gcdENABLE_3D
gceSTATUS
gcoHARDWARE_ComputeCentroids(
    IN gcoHARDWARE Hardware,
    IN gctUINT Count,
    IN gctUINT32_PTR SampleCoords,
    OUT gcsCENTROIDS_PTR Centroids
    );

/* Flush the vertex caches. */
gceSTATUS
gcoHARDWARE_FlushL2Cache(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushVtxDataCache(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushViewport(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushScissor(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushAlpha(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushStencil(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushTarget(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushDepth(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushSampling(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushPA(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushQuery(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushDepthOnly(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushUniform(
    IN gcoHARDWARE Hardware,
    IN OUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushShaders(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE PrimitiveType,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushPrefetchInst(
    IN gcoHARDWARE Hardware,
    IN OUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushXfb(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FlushDrawID(
    IN gcoHARDWARE Hardware,
    IN gctBOOL ForComputing,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FastFlushUniforms(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FastFlushStream(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FastProgramIndex(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FastFlushAlpha(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_FastFlushDepthCompare(
    IN gcoHARDWARE Hardware,
    IN gcsFAST_FLUSH_PTR FastFlushInfo,
    INOUT gctPOINTER *Memory
    );

gceSTATUS gcoHARDWARE_ProgramIndex(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    );

#if gcdENABLE_TRUST_APPLICATION
gceSTATUS
gcoHARDWARE_SetProtectMode(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable,
    INOUT gctPOINTER *Memory
);

gceSTATUS
gcoHARDWARE_FlushProtectMode(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
);

#endif

#endif

gceSTATUS
gcoHARDWARE_Load2DState32(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    );

gceSTATUS
gcoHARDWARE_Load2DState(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Count,
    IN gctPOINTER Data
    );

/* Set clipping rectangle. */
gceSTATUS
gcoHARDWARE_SetClipping(
    IN gcoHARDWARE Hardware,
    IN gcsRECT_PTR Rect
    );

/* Translate API source color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateSourceFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT APIValue,
    OUT gctUINT32* HwValue,
    OUT gctUINT32* HwSwizzleValue,
    OUT gctUINT32* HwIsYUVValue
    );

/* Translate API format to internal format . */
gceSTATUS gcoHARDWARE_TranslateXRGBFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT InputFormat,
    OUT gceSURF_FORMAT* OutputFormat
    );

/* Translate API pattern color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePatternFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT APIValue,
    OUT gctUINT32* HwValue,
    OUT gctUINT32* HwSwizzleValue,
    OUT gctUINT32* HwIsYUVValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateTransparency(
    IN gceSURF_TRANSPARENCY APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API transparency mode to its PE 1.0 hardware value. */
gceSTATUS
gcoHARDWARE_TranslateTransparencies(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 srcTransparency,
    IN gctUINT32 dstTransparency,
    IN gctUINT32 patTransparency,
    OUT gctUINT32* HwValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateSourceTransparency(
    IN gce2D_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API rotation mode to its hardware value. */
gceSTATUS gcoHARDWARE_TranslateSourceRotation(
    IN gceSURF_ROTATION APIValue,
    OUT gctUINT32 * HwValue
    );

gceSTATUS gcoHARDWARE_TranslateDestinationRotation(
    IN gceSURF_ROTATION APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateDestinationTransparency(
    IN gce2D_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePatternTransparency(
    IN gce2D_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API DFB color key mode to its hardware value. */
gceSTATUS gcoHARDWARE_TranslateDFBColorKeyMode(
    IN  gctBOOL APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API pixel color multiply mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelColorMultiplyMode(
    IN  gce2D_PIXEL_COLOR_MULTIPLY_MODE APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API global color multiply mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateGlobalColorMultiplyMode(
    IN  gce2D_GLOBAL_COLOR_MULTIPLY_MODE APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API mono packing mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateMonoPack(
    IN gceSURF_MONOPACK APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API 2D command to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateCommand(
    IN gce2D_COMMAND APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API per-pixel alpha mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelAlphaMode(
    IN gceSURF_PIXEL_ALPHA_MODE APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API global alpha mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateGlobalAlphaMode(
    IN gceSURF_GLOBAL_ALPHA_MODE APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API per-pixel color mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelColorMode(
    IN gceSURF_PIXEL_COLOR_MODE APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API alpha factor mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateAlphaFactorMode(
    IN gcoHARDWARE Hardware,
    IN  gctBOOL IsSrcFactor,
    IN  gceSURF_BLEND_FACTOR_MODE APIValue,
    OUT gctUINT32_PTR HwValue,
    OUT gctUINT32_PTR FactorExpansion
    );

/* Configure monochrome source. */
gceSTATUS gcoHARDWARE_SetMonochromeSource(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 MonoTransparency,
    IN gceSURF_MONOPACK DataPack,
    IN gctBOOL CoordRelative,
    IN gctUINT32 FgColor32,
    IN gctUINT32 BgColor32,
    IN gctBOOL ColorConvert,
    IN gceSURF_FORMAT DstFormat,
    IN gctBOOL Stream,
    IN gctUINT32 Transparency
    );

/* Configure color source. */
gceSTATUS
gcoHARDWARE_SetColorSource(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctBOOL CoordRelative,
    IN gctUINT32 Transparency,
    IN gce2D_YUV_COLOR_MODE Mode,
    IN gctBOOL DeGamma,
    IN gctBOOL Filter
    );

/* Configure masked color source. */
gceSTATUS
gcoHARDWARE_SetMaskedSource(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctBOOL CoordRelative,
    IN gceSURF_MONOPACK MaskPack,
    IN gctUINT32 Transparency
    );

/* Setup the source rectangle. */
gceSTATUS
gcoHARDWARE_SetSource(
    IN gcoHARDWARE Hardware,
    IN gcsRECT_PTR SrcRect
    );

/* Setup the fraction of the source origin for filter blit. */
gceSTATUS
gcoHARDWARE_SetOriginFraction(
    IN gcoHARDWARE Hardware,
    IN gctUINT16 HorFraction,
    IN gctUINT16 VerFraction
    );

/* Load 256-entry color table for INDEX8 source surfaces. */
gceSTATUS
gcoHARDWARE_LoadPalette(
    IN gcoHARDWARE Hardware,
    IN gctUINT FirstIndex,
    IN gctUINT IndexCount,
    IN gctPOINTER ColorTable,
    IN gctBOOL ColorConvert,
    IN gceSURF_FORMAT DstFormat,
    IN OUT gctBOOL *Program,
    IN OUT gceSURF_FORMAT *ConvertFormat
    );

/* Setup the source global color value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetSourceGlobalColor(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Color
    );

/* Setup the target global color value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetTargetGlobalColor(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Color
    );

/* Setup the source and target pixel multiply modes. */
gceSTATUS
gcoHARDWARE_SetMultiplyModes(
    IN gcoHARDWARE Hardware,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE SrcPremultiplySrcAlpha,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstPremultiplyDstAlpha,
    IN gce2D_GLOBAL_COLOR_MULTIPLY_MODE SrcPremultiplyGlobalMode,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstDemultiplyDstAlpha,
    IN gctUINT32 SrcGlobalColor
    );

/*
 * Set the transparency for source, destination and pattern.
 * It also enable or disable the DFB color key mode.
 */
gceSTATUS
gcoHARDWARE_SetTransparencyModesEx(
    IN gcoHARDWARE Hardware,
    IN gce2D_TRANSPARENCY SrcTransparency,
    IN gce2D_TRANSPARENCY DstTransparency,
    IN gce2D_TRANSPARENCY PatTransparency,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gctBOOL EnableDFBColorKeyMode
    );

/* Setup the source, target and pattern transparency modes.
   Used only for have backward compatibility.
*/
gceSTATUS
gcoHARDWARE_SetAutoTransparency(
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop
    );

/* Setup the source color key value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetSourceColorKeyRange(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 ColorLow,
    IN gctUINT32 ColorHigh,
    IN gctBOOL ColorPack,
    IN gceSURF_FORMAT SrcFormat
    );

gceSTATUS
gcoHARDWARE_SetMultiSource(
    IN gcoHARDWARE Hardware,
    IN gctUINT RegGroupIndex,
    IN gctUINT SrcIndex,
    IN gcs2D_State_PTR State
    );

gceSTATUS
gcoHARDWARE_SetMultiSourceEx(
    IN gcoHARDWARE Hardware,
    IN gctUINT RegGroupIndex,
    IN gctUINT SrcIndex,
    IN gcs2D_State_PTR State,
    IN gctBOOL MultiDstRect
    );

/* Configure destination. */
gceSTATUS
gcoHARDWARE_SetTarget(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gctBOOL Filter,
    IN gce2D_YUV_COLOR_MODE Mode,
    IN gctINT32_PTR CscRGB2YUV,
    IN gctUINT32_PTR GammaTable,
    IN gctBOOL GdiStretch,
    OUT gctUINT32_PTR DestConfig
    );

gceSTATUS gcoHARDWARE_SetMultiTarget(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface,
    IN gceSURF_FORMAT SrcFormat
    );

/* Setup the destination color key value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetTargetColorKeyRange(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 ColorLow,
    IN gctUINT32 ColorHigh
    );

/* Load solid (single) color pattern. */
gceSTATUS
gcoHARDWARE_LoadSolidColorPattern(
    IN gcoHARDWARE Hardware,
    IN gctBOOL ColorConvert,
    IN gctUINT32 Color,
    IN gctUINT64 Mask,
    IN gceSURF_FORMAT DstFormat
    );

/* Load monochrome pattern. */
gceSTATUS
gcoHARDWARE_LoadMonochromePattern(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctBOOL ColorConvert,
    IN gctUINT32 FgColor,
    IN gctUINT32 BgColor,
    IN gctUINT64 Bits,
    IN gctUINT64 Mask,
    IN gceSURF_FORMAT DstFormat
    );

/* Load color pattern. */
gceSTATUS
gcoHARDWARE_LoadColorPattern(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctUINT32 Address,
    IN gceSURF_FORMAT Format,
    IN gctUINT64 Mask
    );

/* Calculate and program the stretch factors. */
gceSTATUS
gcoHARDWARE_SetStretchFactors(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 HorFactor,
    IN gctUINT32 VerFactor
    );

/* Set 2D clear color in A8R8G8B8 format. */
gceSTATUS
gcoHARDWARE_Set2DClearColor(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Color,
    IN gceSURF_FORMAT DstFormat
    );

/* Enable/disable 2D BitBlt mirrorring. */
gceSTATUS
gcoHARDWARE_SetBitBlitMirror(
    IN gcoHARDWARE Hardware,
    IN gctBOOL HorizontalMirror,
    IN gctBOOL VerticalMirror,
    IN gctBOOL DstMirror
    );

/* Enable alpha blending engine in the hardware and disengage the ROP engine. */
gceSTATUS
gcoHARDWARE_EnableAlphaBlend(
    IN gcoHARDWARE Hardware,
    IN gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode,
    IN gceSURF_PIXEL_ALPHA_MODE DstAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode,
    IN gceSURF_BLEND_FACTOR_MODE SrcFactorMode,
    IN gceSURF_BLEND_FACTOR_MODE DstFactorMode,
    IN gceSURF_PIXEL_COLOR_MODE SrcColorMode,
    IN gceSURF_PIXEL_COLOR_MODE DstColorMode,
    IN gctUINT32 SrcGlobalAlphaValue,
    IN gctUINT32 DstGlobalAlphaValue
    );

/* Disable alpha blending engine in the hardware and engage the ROP engine. */
gceSTATUS
gcoHARDWARE_DisableAlphaBlend(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_ColorConvertToARGB8(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 NumColors,
    IN gctUINT32_PTR Color,
    OUT gctUINT32_PTR Color32
    );

gceSTATUS
gcoHARDWARE_ColorConvertFromARGB8(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 NumColors,
    IN gctUINT32_PTR Color32,
    OUT gctUINT32_PTR Color
    );

gceSTATUS
gcoHARDWARE_ColorPackFromARGB8(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Color32,
    OUT gctUINT32_PTR Color
    );

/* Convert pixel format. */
gceSTATUS
gcoHARDWARE_ConvertPixel(
    IN gctPOINTER SrcPixel,
    OUT gctPOINTER TrgPixel,
    IN gctUINT SrcBitOffset,
    IN gctUINT TrgBitOffset,
    IN gcsSURF_FORMAT_INFO_PTR SrcFormat,
    IN gcsSURF_FORMAT_INFO_PTR TrgFormat,
    IN OPTIONAL gcsBOUNDARY_PTR SrcBoundary,
    IN OPTIONAL gcsBOUNDARY_PTR TrgBoundary,
    IN gctBOOL SrcPixelOdd,
    IN gctBOOL TrgPixelOdd
    );


#if gcdENABLE_3D
typedef enum _gceTILE_STATUS_CONTROL
{
    gcvTILE_STATUS_PAUSE,
    gcvTILE_STATUS_RESUME,
}
gceTILE_STATUS_CONTROL;

/* Pause or resume tile status. */
gceSTATUS
gcoHARDWARE_PauseTileStatus(
    IN gcoHARDWARE Hardware,
    IN gceTILE_STATUS_CONTROL Control
    );

/* Compute the offset of the specified pixel location. */
gceSTATUS
gcoHARDWARE_ComputeOffset(
    IN gcoHARDWARE Hardware,
    IN gctINT32 X,
    IN gctINT32 Y,
    IN gctUINT Stride,
    IN gctINT BytesPerPixel,
    IN gceTILING Tiling,
    OUT gctUINT32_PTR Offset
    );

/* Adjust cache mode to make RS work fine with MSAA config switch*/
gceSTATUS
gcoHARDWARE_AdjustCacheMode(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface
    );


/* Query the tile size of the given surface. */
gceSTATUS
gcoHARDWARE_GetSurfaceTileSize(
    IN gcoSURF Surface,
    OUT gctINT32 * TileWidth,
    OUT gctINT32 * TileHeight
    );

/* Program the Resolve offset, Window and then trigger the resolve. */
gceSTATUS
gcoHARDWARE_ProgramResolve(
    IN gcoHARDWARE Hardware,
    IN gcsPOINT RectSize,
    IN gctBOOL  MultiPipe,
    IN gceMSAA_DOWNSAMPLE_MODE DownsampleMode,
    INOUT gctPOINTER *Memory
    );

#endif /* gcdENABLE_3D */

/* Program the Probe command and address to control probe counter. */
gceSTATUS
gcoHARDWARE_ProgramProbe(
    IN gcoHARDWARE Hardware,
    IN gceProbeCmd Cmd,
    IN gctUINT32 ProbeAddress,
    IN gctINT32 Module,
    IN gctINT32 Counter,
    INOUT gctPOINTER *Memory
    );

/* Load one 32-bit state. */
gceSTATUS
gcoHARDWARE_LoadState32(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    );

gceSTATUS
gcoHARDWARE_LoadState32NEW(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data,
    INOUT gctPOINTER *Memory
    );

gceSTATUS
gcoHARDWARE_LoadState32WithMask(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Mask,
    IN gctUINT32 Data
    );

gceSTATUS
gcoHARDWARE_LoadState32WithMaskNEW(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Mask,
    IN gctUINT32 Data,
    INOUT gctPOINTER *Memory
    );


/* Load one 32-bit load state. */
gceSTATUS
gcoHARDWARE_LoadState32x(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctFIXED_POINT Data
    );

/* Load one 64-bit load state. */
gceSTATUS
gcoHARDWARE_LoadState64(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT64 Data
    );

gceSTATUS
gcoHARDWARE_SetDither2D(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

gceSTATUS gcoHARDWARE_Begin2DRender(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State
    );

gceSTATUS gcoHARDWARE_End2DRender(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State
    );

gceSTATUS gcoHARDWARE_UploadCSCTable(
    IN gcoHARDWARE Hardware,
    IN gctBOOL YUV2RGB,
    IN gctINT32_PTR Table
    );

/* Query hardware frequency. */
gceSTATUS
gcoHARDWARE_QueryFrequency(
    IN gcoHARDWARE Hardware
    );

/* About Compression. */
gceSTATUS
gcoHARDWARE_CheckConstraint(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gce2D_COMMAND Command,
    IN gctBOOL AnyCompressed
    );

gceSTATUS
gcoHARDWARE_GetCompressionCmdSize(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gcoSURF SrcSurface,
    IN gcoSURF DstSurface,
    IN gctUINT CompressNum,
    IN gce2D_COMMAND Command,
    OUT gctUINT32 *CmdSize
    );

gceSTATUS
gcoHARDWARE_SetCompression(
    IN gcoHARDWARE Hardware,
    IN gcs2D_State_PTR State,
    IN gcoSURF SrcSurface,
    IN gcoSURF DstSurface,
    IN gce2D_COMMAND Command,
    IN gctUINT SrcCompress,
    IN gctBOOL DstCompress
    );

typedef struct {
    gctUINT inputBase;
    gctUINT count;
    gctUINT outputBase;
}
gcsSTATEMIRROR;

extern const gcsSTATEMIRROR mirroredStates[];
extern gctUINT mirroredStatesCount;

/* Update the state delta. */
static gcmINLINE void
gcoHARDWARE_UpdateDelta(
    IN gcsSTATE_DELTA_PTR StateDelta,
    IN gctUINT32 Address,
    IN gctUINT32 Mask,
    IN gctUINT32 Data
    )
{
    gcsSTATE_DELTA_RECORD_PTR recordArray;
    gcsSTATE_DELTA_RECORD_PTR recordEntry;
    gctUINT32_PTR mapEntryID;
    gctUINT32_PTR mapEntryIndex;
    gctUINT deltaID;
    gctUINT32 i;

    if (!StateDelta)
    {
        return;
    }

    /* Get the current record array. */
    recordArray = (gcsSTATE_DELTA_RECORD_PTR)(gcmUINT64_TO_PTR(StateDelta->recordArray));

    /* Get shortcuts to the fields. */
    deltaID       = StateDelta->id;
    mapEntryID    = (gctUINT32_PTR)(gcmUINT64_TO_PTR(StateDelta->mapEntryID));
    mapEntryIndex = (gctUINT32_PTR)(gcmUINT64_TO_PTR(StateDelta->mapEntryIndex));

    gcmASSERT(Address < (StateDelta->mapEntryIDSize / gcmSIZEOF(gctUINT)));

    for (i = 0; i < mirroredStatesCount; i++)
    {
        if ((Address >= mirroredStates[i].inputBase) &&
            (Address < (mirroredStates[i].inputBase + mirroredStates[i].count)))
        {
            Address = mirroredStates[i].outputBase + (Address - mirroredStates[i].inputBase);
            break;
        }
    }

    /* Has the entry been initialized? */
    if (mapEntryID[Address] != deltaID)
    {
        /* No, initialize the map entry. */
        mapEntryID    [Address] = deltaID;
        mapEntryIndex [Address] = StateDelta->recordCount;

        /* Get the current record. */
        recordEntry = &recordArray[mapEntryIndex[Address]];

        /* Add the state to the list. */
        recordEntry->address = Address;
        recordEntry->mask    = Mask;
        recordEntry->data    = Data;

        /* Update the number of valid records. */
        StateDelta->recordCount += 1;
    }

    /* Regular (not masked) states. */
    else if (Mask == 0)
    {
        /* Get the current record. */
        recordEntry = &recordArray[mapEntryIndex[Address]];

        /* Update the state record. */
        recordEntry->mask = 0;
        recordEntry->data = Data;
    }

    /* Masked states. */
    else
    {
        /* Get the current record. */
        recordEntry = &recordArray[mapEntryIndex[Address]];

        /* Update the state record. */
        recordEntry->mask |=  Mask;
        recordEntry->data &= ~Mask;
        recordEntry->data |= (Data & Mask);
    }
}

static gcmINLINE void
gcoHARDWARE_UpdateTempDelta(
    IN gcoHARDWARE Hardware
    )
{
    gcsSTATE_DELTA_PTR tempDelta = Hardware->tempDelta;
    gcsSTATE_DELTA_PTR currentDelta = Hardware->delta;
    gctUINT count;
    gctUINT i;
    gcsSTATE_DELTA_RECORD_PTR record;

    if (tempDelta == NULL)
        return;

    /* Get the record count. */
    count = tempDelta->recordCount;

    /* Set the first record. */
    record = (gcsSTATE_DELTA_RECORD_PTR)gcmUINT64_TO_PTR(tempDelta->recordArray);

    /* Go through all records. */
    for (i = 0; i < count; i += 1)
    {
        /* Update the delta. */
        gcoHARDWARE_UpdateDelta(
            currentDelta, record->address, record->mask, record->data
            );

        /* Advance to the next state. */
        record += 1;
    }
    /* Update the element count. */
    if (tempDelta->elementCount != 0)
    {
        currentDelta->elementCount = tempDelta->elementCount;
    }

    /* Reset tempDelta*/
    tempDelta->id += 1;

    if (tempDelta->id == 0)
    {
        /* Reset the map to avoid erroneous ID matches. */
        gcoOS_ZeroMemory(gcmUINT64_TO_PTR(tempDelta->mapEntryID), tempDelta->mapEntryIDSize);

        /* Increment the main ID to avoid matches after reset. */
        tempDelta->id += 1;
    }

    /* Reset the vertex element count. */
    tempDelta->elementCount = 0;

    /* Reset the record count. */
    tempDelta->recordCount = 0;
}

static gcmINLINE void
gcoHARDWARE_CopyDelta(
    IN gcsSTATE_DELTA_PTR DestStateDelta,
    IN gcsSTATE_DELTA_PTR SrcStateDelta
    )
{
    DestStateDelta->recordCount = SrcStateDelta->recordCount;

    if (DestStateDelta->recordCount)
    {
        gcoOS_MemCopy(
            gcmUINT64_TO_PTR(DestStateDelta->recordArray),
            gcmUINT64_TO_PTR(SrcStateDelta->recordArray),
            gcmSIZEOF(gcsSTATE_DELTA_RECORD) * DestStateDelta->recordCount
            );
    }

    DestStateDelta->elementCount = SrcStateDelta->elementCount;
}

gceSTATUS gcoHARDWARE_InitializeFormatArrayTable(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_2DAppendNop(
    IN gcoHARDWARE Hardware
    );

gceSTATUS gcoHARDWARE_Reset2DCmdBuffer(
    IN gcoHARDWARE Hardware,
    IN gctBOOL CleanCmd
    );

gctBOOL
gcoHARDWARE_NeedUserCSC(
    IN gce2D_YUV_COLOR_MODE Mode,
    IN gceSURF_FORMAT Format);

gceSTATUS
gcoHARDWARE_SetSuperTileVersion(
    IN gcoHARDWARE Hardware,
    IN gce2D_SUPER_TILE_VERSION Version
    );

gceSTATUS
gcoHARDWARE_Alloc2DSurface(
    IN gcoHARDWARE Hardware,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gceSURF_FORMAT Format,
    IN gceSURF_TYPE Hints,
    OUT gcoSURF *Surface
    );

gceSTATUS
gcoHARDWARE_Free2DSurface(
    IN gcoHARDWARE Hardware,
    IN gcoSURF Surface
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_user_hardware_h_ */

