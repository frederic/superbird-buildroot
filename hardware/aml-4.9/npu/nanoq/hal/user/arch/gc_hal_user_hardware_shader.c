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
**  Shader programming..
*/

#include "gc_hal_user_hardware_precomp.h"

#if gcdENABLE_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

#define IS_HW_SUPPORT(feature) (Hardware->features[feature])

gceSTATUS
gcoHARDWARE_QueryShaderCompilerHwCfg(
    IN gcoHARDWARE Hardware,
    OUT PVSC_HW_CONFIG pVscHwCfg
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 maxVaryingCount, maxAttribs;
    gctUINT32 samplerCount[gcvPROGRAM_STAGE_LAST] = {0};
    gctUINT32 samplerBase[gcvPROGRAM_STAGE_LAST] = {0};
    gctUINT32 totalCount = 0;
    gctUINT32 fragmentSizeInKbyte = 0;
    gctUINT32 attribBufSizeInKbyte = 0;
    gctUINT32 localStorageSizeInKbyte = 0;
    gctUINT32 threadCount = 0;
    gctSTRING env = gcvNULL;

    gcmHEADER_ARG("Hardware=0x%x pVscHwCfg=%d", Hardware, pVscHwCfg);

    gcmGETHARDWARE(Hardware);

    gcoOS_ZeroMemory(pVscHwCfg, sizeof(VSC_HW_CONFIG));

    gcmONERROR(gcoHARDWARE_QueryShaderCaps(gcvNULL,
                                           gcvNULL,
                                           gcvNULL,
                                           gcvNULL,
                                           &maxVaryingCount,
                                           gcvNULL,
                                           &threadCount,
                                           gcvNULL,
                                           gcvNULL));

    gcmONERROR(gcoHARDWARE_QuerySamplerBase(gcvNULL,
                                            samplerCount,
                                            samplerBase,
                                            &totalCount));

    gcmONERROR(gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                           &maxAttribs,
                                           gcvNULL,
                                           gcvNULL,
                                           gcvNULL,
                                           gcvNULL));

    pVscHwCfg->chipModel                             = Hardware->config->chipModel;
    pVscHwCfg->chipRevision                          = Hardware->config->chipRevision;
    pVscHwCfg->productID                             = Hardware->config->productID;
    pVscHwCfg->customerID                            = Hardware->config->customerID;
    pVscHwCfg->maxCoreCount                          = Hardware->config->shaderCoreCount;
    pVscHwCfg->maxThreadCountPerCore                 = Hardware->config->threadCount/Hardware->config->shaderCoreCount;
    pVscHwCfg->maxVaryingCount                       = maxVaryingCount;
    pVscHwCfg->maxAttributeCount                     = maxAttribs;
    pVscHwCfg->maxRenderTargetCount                  = Hardware->config->renderTargets;
    pVscHwCfg->maxGPRCountPerCore                    = 512;
    pVscHwCfg->maxGPRCountPerThread                  = Hardware->config->registerMax;
    pVscHwCfg->maxHwNativeTotalInstCount             = Hardware->config->instructionCount;
    pVscHwCfg->maxTotalInstCount                     = Hardware->config->instMax;
    pVscHwCfg->maxVSInstCount                        = Hardware->config->vsInstMax;
    pVscHwCfg->maxPSInstCount                        = Hardware->config->psInstMax;
    pVscHwCfg->vsInstBufferAddrBase                  = Hardware->config->vsInstBase;
    pVscHwCfg->psInstBufferAddrBase                  = Hardware->config->psInstBase;
    pVscHwCfg->maxHwNativeTotalConstRegCount         = Hardware->config->numConstants;
    pVscHwCfg->maxTotalConstRegCount                 = Hardware->config->constMax;
    pVscHwCfg->unifiedConst                          = Hardware->config->unifiedConst;
    pVscHwCfg->maxVSConstRegCount                    = Hardware->config->vsConstMax;
    /* use VS count for now */
    pVscHwCfg->maxTCSConstRegCount                   = Hardware->config->vsConstMax;
    pVscHwCfg->maxTESConstRegCount                   = Hardware->config->vsConstMax;
    pVscHwCfg->maxGSConstRegCount                    = Hardware->config->vsConstMax;
    pVscHwCfg->maxPSConstRegCount                    = Hardware->config->psConstMax;
    pVscHwCfg->vsConstRegAddrBase                    = Hardware->config->vsConstBase;
    /* use VS count for now */
    pVscHwCfg->tcsConstRegAddrBase                   = Hardware->config->vsConstBase;
    pVscHwCfg->tesConstRegAddrBase                   = Hardware->config->vsConstBase;
    pVscHwCfg->gsConstRegAddrBase                    = Hardware->config->vsConstBase;
    pVscHwCfg->psConstRegAddrBase                    = Hardware->config->psConstBase;
    /*
    ** Set sample base and count. Here is the sampler order:
    ** PS: 0~15
    ** CS/CL: 0~31
    ** VS: 16~31
    ** TCS: 32~47
    ** TES: 48~63
    ** GS: 64~79
    */
    pVscHwCfg->maxVSSamplerCount                     = samplerCount[gcvPROGRAM_STAGE_VERTEX];
    pVscHwCfg->maxTCSSamplerCount                    = samplerCount[gcvPROGRAM_STAGE_TCS];
    pVscHwCfg->maxTESSamplerCount                    = samplerCount[gcvPROGRAM_STAGE_TES];
    pVscHwCfg->maxGSSamplerCount                     = samplerCount[gcvPROGRAM_STAGE_GEOMETRY];
    pVscHwCfg->maxPSSamplerCount                     = samplerCount[gcvPROGRAM_STAGE_FRAGMENT];
    pVscHwCfg->maxCSSamplerCount                     = samplerCount[gcvPROGRAM_STAGE_COMPUTE];
    /* Right now there are 5 bits for sampler index, so the max sampler count is 32. */
    pVscHwCfg->maxSamplerCountPerShader              = 32;

    pVscHwCfg->maxHwNativeTotalSamplerCount          = totalCount;

    /* samplerNoBase for state programming */
    if (IS_HW_SUPPORT(gcvFEATURE_SAMPLER_BASE_OFFSET))
    {
        pVscHwCfg->vsSamplerRegNoBase                = samplerBase[gcvPROGRAM_STAGE_VERTEX];
        pVscHwCfg->psSamplerRegNoBase                = samplerBase[gcvPROGRAM_STAGE_FRAGMENT];
        pVscHwCfg->csSamplerRegNoBase                = samplerBase[gcvPROGRAM_STAGE_COMPUTE];
        pVscHwCfg->tcsSamplerRegNoBase               = samplerBase[gcvPROGRAM_STAGE_TCS];
        pVscHwCfg->tesSamplerRegNoBase               = samplerBase[gcvPROGRAM_STAGE_TES];
        pVscHwCfg->gsSamplerRegNoBase                = samplerBase[gcvPROGRAM_STAGE_GEOMETRY];
        /* samplerNoBase in instruction words */
        pVscHwCfg->vsSamplerNoBaseInInstruction      = 0;
        pVscHwCfg->psSamplerNoBaseInInstruction      = samplerBase[gcvPROGRAM_STAGE_FRAGMENT];
    }
    else
    {
        pVscHwCfg->vsSamplerRegNoBase                = 0;
        pVscHwCfg->psSamplerRegNoBase                = 0;
        pVscHwCfg->csSamplerRegNoBase                = 0;
        pVscHwCfg->tcsSamplerRegNoBase               = 0;
        pVscHwCfg->tesSamplerRegNoBase               = 0;
        pVscHwCfg->gsSamplerRegNoBase                = 0;
        /* samplerNoBase in instruction words */
        pVscHwCfg->vsSamplerNoBaseInInstruction      = samplerBase[gcvPROGRAM_STAGE_VERTEX];
        pVscHwCfg->psSamplerNoBaseInInstruction      = samplerBase[gcvPROGRAM_STAGE_FRAGMENT];
    }
    pVscHwCfg->vertexOutputBufferSize                = Hardware->config->vertexOutputBufferSize;
    pVscHwCfg->vertexCacheSize                       = Hardware->config->vertexCacheSize;
    pVscHwCfg->ctxStateCount                         = Hardware->maxState;

    if ((!IS_HW_SUPPORT(gcvFEATURE_SNAPPAGE_CMD_FIX) ||
         !IS_HW_SUPPORT(gcvFEATURE_SH_SNAP2PAGE_MAXPAGES_FIX)) &&
        IS_HW_SUPPORT(gcvFEATURE_GEOMETRY_SHADER)              &&
        IS_HW_SUPPORT(gcvFEATURE_TESSELLATION))
    {
        fragmentSizeInKbyte = 5;
    }
    if (IS_HW_SUPPORT(gcvFEATURE_USC))
    {
        if (IS_HW_SUPPORT(gcvFEATURE_SEPARATE_LS))
        {
            localStorageSizeInKbyte = Hardware->config->localStorageSizeInKbyte;
            attribBufSizeInKbyte = Hardware->config->uscPagesMaxInKbyte - fragmentSizeInKbyte;
        }
        else
        {
            static const gctFLOAT s_uscCacheRatio[] =
            {
                1.0f,
                0.5f,
                0.25f,
                0.125f,
                0.0625f,
                0.03125f,
                0.75f,
                0.0f,
            };

            attribBufSizeInKbyte = (gctUINT32)
                (Hardware->config->uscPagesMaxInKbyte
              - (Hardware->config->l1CacheSizeInKbyte * s_uscCacheRatio[Hardware->options.uscL1CacheRatio]));

            /* attribute cache size for multi cluster arch */
            if (IS_HW_SUPPORT(gcvFEATURE_MULTI_CLUSTER))
            {
                attribBufSizeInKbyte -= (gctUINT32)(Hardware->config->l1CacheSizeInKbyte
                    * s_uscCacheRatio[Hardware->options.uscAttribCacheRatio]);
            }
            attribBufSizeInKbyte -= fragmentSizeInKbyte;

            localStorageSizeInKbyte = attribBufSizeInKbyte;
        }
    }
    else
    {
        localStorageSizeInKbyte = Hardware->config->localStorageSizeInKbyte;
    }
    pVscHwCfg->maxUSCAttribBufInKbyte                = IS_HW_SUPPORT(gcvFEATURE_USC) ?  attribBufSizeInKbyte : 0;
    pVscHwCfg->maxLocalMemSizeInByte                 = localStorageSizeInKbyte * 1024;
    pVscHwCfg->maxResultCacheWinSize                 = Hardware->config->resultWindowMaxSize;

    pVscHwCfg->minPointSize                          = 0.5f;
    pVscHwCfg->maxPointSize                          = 128.0f;

    /*
    ** 1) Use the DEVICE_MAX_WORK_GROUP_SIZE as the default workGroupSize for a shader.
    ** 2) When we need to use the workGroupSize to calculate the maxRegCount(e.g., use BARRIER in shader),
    **    use initWorkGroupSizeToCalcRegCount as the workGroupSize. And we may also reduce it to use more HW registers.
    */
    if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "VIV_ENABLE_OPENCV_WORKGROUPSIZE", &env))
        && env
        && gcmIS_SUCCESS(gcoOS_StrCmp(env, "1")))
    {
        pVscHwCfg->initWorkGroupSizeToCalcRegCount       = 256;
        pVscHwCfg->maxWorkGroupSize                      = 256;
        pVscHwCfg->minWorkGroupSize                      = 256;
    }
    else
    {
        pVscHwCfg->initWorkGroupSizeToCalcRegCount       = 128;
        pVscHwCfg->maxWorkGroupSize                      = gcmMIN(threadCount, 1024);
        pVscHwCfg->minWorkGroupSize                      = 1;
    }

    gcmASSERT(pVscHwCfg->maxWorkGroupSize >= pVscHwCfg->initWorkGroupSizeToCalcRegCount);

    pVscHwCfg->hwFeatureFlags.hasHalti0              = IS_HW_SUPPORT(gcvFEATURE_HALTI0);
    pVscHwCfg->hwFeatureFlags.hasHalti1              = IS_HW_SUPPORT(gcvFEATURE_HALTI1);
    pVscHwCfg->hwFeatureFlags.hasHalti2              = IS_HW_SUPPORT(gcvFEATURE_HALTI2);
    pVscHwCfg->hwFeatureFlags.hasHalti3              = IS_HW_SUPPORT(gcvFEATURE_HALTI3);
    pVscHwCfg->hwFeatureFlags.hasHalti4              = IS_HW_SUPPORT(gcvFEATURE_HALTI4);
    pVscHwCfg->hwFeatureFlags.hasHalti5              = IS_HW_SUPPORT(gcvFEATURE_HALTI5);
    pVscHwCfg->hwFeatureFlags.supportGS              = IS_HW_SUPPORT(gcvFEATURE_GEOMETRY_SHADER);
    pVscHwCfg->hwFeatureFlags.supportTS              = IS_HW_SUPPORT(gcvFEATURE_TESSELLATION);
    pVscHwCfg->hwFeatureFlags.supportInteger         = IS_HW_SUPPORT(gcvFEATURE_SUPPORT_INTEGER);
    pVscHwCfg->hwFeatureFlags.hasSignFloorCeil       = IS_HW_SUPPORT(gcvFEATURE_EXTRA_SHADER_INSTRUCTIONS0);
    pVscHwCfg->hwFeatureFlags.hasSqrtTrig            = IS_HW_SUPPORT(gcvFEATURE_EXTRA_SHADER_INSTRUCTIONS1);
    pVscHwCfg->hwFeatureFlags.hasNewSinCosLogDiv     = IS_HW_SUPPORT(gcvFEATURE_EXTRA_SHADER_INSTRUCTIONS2);
    pVscHwCfg->hwFeatureFlags.hasMediumPrecision     = IS_HW_SUPPORT(gcvFEATURE_MEDIUM_PRECISION);
    pVscHwCfg->hwFeatureFlags.canBranchOnImm         = IS_HW_SUPPORT(gcvFEATURE_BRANCH_ON_IMMEDIATE_REG);
    pVscHwCfg->hwFeatureFlags.supportDual16          = IS_HW_SUPPORT(gcvFEATURE_DUAL_16);
    pVscHwCfg->hwFeatureFlags.hasBugFix8             = IS_HW_SUPPORT(gcvFEATURE_BUG_FIXES8);
    pVscHwCfg->hwFeatureFlags.hasBugFix10            = IS_HW_SUPPORT(gcvFEATURE_BUG_FIXES10);
    pVscHwCfg->hwFeatureFlags.hasBugFix11            = IS_HW_SUPPORT(gcvFEATURE_BUG_FIXES11);
    pVscHwCfg->hwFeatureFlags.hasSelectMapSwizzleFix = IS_HW_SUPPORT(gcvFEATURE_SELECTMAP_SRC0_SWIZZLE_FIX);
    pVscHwCfg->hwFeatureFlags.hasSamplePosSwizzleFix = IS_HW_SUPPORT(gcvFEATURE_SAMPLEPOS_SWIZZLE_FIX);
    pVscHwCfg->hwFeatureFlags.hasLoadAttrOOBFix      = IS_HW_SUPPORT(gcvFEATURE_LOADATTR_OOB_FIX);
    pVscHwCfg->hwFeatureFlags.hasSHEnhance2          = IS_HW_SUPPORT(gcvFEATURE_SHADER_ENHANCEMENTS2);
    pVscHwCfg->hwFeatureFlags.hasInstCache           = IS_HW_SUPPORT(gcvFEATURE_SHADER_HAS_INSTRUCTION_CACHE);
    pVscHwCfg->hwFeatureFlags.instBufferUnified      = Hardware->config->unifiedInst;
    pVscHwCfg->hwFeatureFlags.constRegFileUnified    = Hardware->config->unifiedConst;
    pVscHwCfg->hwFeatureFlags.samplerRegFileUnified  = IS_HW_SUPPORT(gcvFEATURE_UNIFIED_SAMPLERS);
    pVscHwCfg->hwFeatureFlags.bigEndianMI            = Hardware->bigEndian;
    pVscHwCfg->hwFeatureFlags.raPushPosW             = IS_HW_SUPPORT(gcvFEATURE_SHADER_HAS_W);
    pVscHwCfg->hwFeatureFlags.vtxInstanceIdAsAttr    = IS_HW_SUPPORT(gcvFEATURE_VERTEX_INST_ID_AS_ATTRIBUTE);
    pVscHwCfg->hwFeatureFlags.vtxInstanceIdAsInteger = IS_HW_SUPPORT(gcvFEATURE_VERTEX_INST_ID_AS_INTEGER);
    pVscHwCfg->hwFeatureFlags.hasSHEnhance3          = IS_HW_SUPPORT(gcvFEATURE_SHADER_ENHANCEMENTS3);
    pVscHwCfg->hwFeatureFlags.rtneRoundingEnabled    = IS_HW_SUPPORT(gcvFEATURE_SHADER_HAS_RTNE);
    pVscHwCfg->hwFeatureFlags.hasInstCachePrefetch   = IS_HW_SUPPORT(gcvFEATURE_SH_INSTRUCTION_PREFETCH);
    pVscHwCfg->hwFeatureFlags.hasThreadWalkerInPS    = Hardware->threadWalkerInPS;
    pVscHwCfg->hwFeatureFlags.has32Attributes        = IS_HW_SUPPORT(gcvFEATURE_PIPELINE_32_ATTRIBUTES);
    pVscHwCfg->hwFeatureFlags.newSteeringICacheFlush = IS_HW_SUPPORT(gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH);
    pVscHwCfg->hwFeatureFlags.gsSupportEmit          = IS_HW_SUPPORT(gcvFEATURE_GS_SUPPORT_EMIT);
    pVscHwCfg->hwFeatureFlags.hasSamplerBaseOffset   = IS_HW_SUPPORT(gcvFEATURE_SAMPLER_BASE_OFFSET);
    pVscHwCfg->hwFeatureFlags.supportStreamOut       = IS_HW_SUPPORT(gcvFEATURE_HW_TFB);
    pVscHwCfg->hwFeatureFlags.supportZeroAttrsInFE   = IS_HW_SUPPORT(gcvFEATURE_ZERO_ATTRIB_SUPPORT);
    pVscHwCfg->hwFeatureFlags.outputCountFix         = IS_HW_SUPPORT(gcvFEATURE_HAS_OUTPUT_COUNT_FIX);
    pVscHwCfg->hwFeatureFlags.varyingPackingLimited  = IS_HW_SUPPORT(gcvFEATURE_VARYING_PACKING_LIMITATION);
    pVscHwCfg->hwFeatureFlags.highpVaryingShift      = IS_HW_SUPPORT(gcvFEATURE_HIGHP_VARYING_SHIFT);
    pVscHwCfg->hwFeatureFlags.needCLXFixes           = IS_HW_SUPPORT(gcvFEATURE_NEED_FIX_FOR_CL_X);
    pVscHwCfg->hwFeatureFlags.needCLXEFixes          = IS_HW_SUPPORT(gcvFEATURE_NEED_FIX_FOR_CL_XE);
    pVscHwCfg->hwFeatureFlags.robustAtomic           = IS_HW_SUPPORT(gcvFEATURE_ROBUST_ATOMIC);
    pVscHwCfg->hwFeatureFlags.newGPIPE               = IS_HW_SUPPORT(gcvFEATURE_NEW_GPIPE);
    pVscHwCfg->hwFeatureFlags.flatDual16Fix          = IS_HW_SUPPORT(gcvFEATURE_SH_FLAT_INTERPOLATION_DUAL16_FIX);
    pVscHwCfg->hwFeatureFlags.supportEVIS            = IS_HW_SUPPORT(gcvFEATURE_EVIS);
    pVscHwCfg->hwFeatureFlags.supportImgAtomic       = IS_HW_SUPPORT(gcvFEATURE_SH_SUPPORT_V4);
    pVscHwCfg->hwFeatureFlags.supportAdvancedInsts   = IS_HW_SUPPORT(gcvFEATURE_ADVANCED_SH_INST);
    pVscHwCfg->hwFeatureFlags.noOneConstLimit        = IS_HW_SUPPORT(gcvFEATURE_SH_NO_ONECONST_LIMIT);
    pVscHwCfg->hwFeatureFlags.hasUniformB0           = IS_HW_SUPPORT(gcvFEATURE_INDEX_CONST_ON_B0);
    pVscHwCfg->hwFeatureFlags.hasSampleMaskInR0ZWFix = IS_HW_SUPPORT(gcvFEATURE_PSIO_SAMPLEMASK_IN_R0ZW_FIX);
    pVscHwCfg->hwFeatureFlags.supportOOBCheck        = IS_HW_SUPPORT(gcvFEATURE_ROBUSTNESS);
    pVscHwCfg->hwFeatureFlags.hasUniversalTexld      = IS_HW_SUPPORT(gcvFEATURE_TX_INTEGER_COORDINATE);
    pVscHwCfg->hwFeatureFlags.hasUniversalTexldV2    = IS_HW_SUPPORT(gcvFEATURE_TX_INTEGER_COORDINATE_V2);
    pVscHwCfg->hwFeatureFlags.hasTexldUFix           = IS_HW_SUPPORT(gcvFEATURE_SH_TEXLD_U_FIX);
    pVscHwCfg->hwFeatureFlags.canSrc0OfImgLdStBeTemp = IS_HW_SUPPORT(gcvFEATURE_SH_IMG_LDST_ON_TEMP);
    pVscHwCfg->hwFeatureFlags.supportImgLDSTClamp    = IS_HW_SUPPORT(gcvFEATURE_SH_IMG_LDST_CLAMP);
    pVscHwCfg->hwFeatureFlags.useSrc0SwizzleAsSrcBin = gcvTRUE;
    pVscHwCfg->hwFeatureFlags.hasICacheAllocCountFix = IS_HW_SUPPORT(gcvFEATURE_SH_ICACHE_ALLOC_COUNT_FIX);
    pVscHwCfg->hwFeatureFlags.hasPSIOInterlock       = IS_HW_SUPPORT(gcvFEATURE_PSIO_INTERLOCK);
    /*
    ** Actually HW can support 128bpp image LOAD/STORE, but since TX and PE can't support 128bpp texture and a image is normally bound with a texture,
    ** we assume that we can't support 128bpp image.
    */
    pVscHwCfg->hwFeatureFlags.support128BppImage     = gcvFALSE;
    pVscHwCfg->hwFeatureFlags.supportMSAATexture     = IS_HW_SUPPORT(gcvFEATURE_MSAA_TEXTURE);
    pVscHwCfg->hwFeatureFlags.supportPerCompDepForLS = IS_HW_SUPPORT(gcvFEATURE_LS_SUPPORT_PER_COMP_DEPENDENCY);
    pVscHwCfg->hwFeatureFlags.supportImgAddr         = IS_HW_SUPPORT(gcvFEATURE_IMG_INSTRUCTION);
    pVscHwCfg->hwFeatureFlags.hasUscGosAddrFix       = IS_HW_SUPPORT(gcvFEATURE_USC_GOS_ADDR_FIX);
    pVscHwCfg->hwFeatureFlags.multiCluster           = IS_HW_SUPPORT(gcvFEATURE_MULTI_CLUSTER);
    pVscHwCfg->hwFeatureFlags.smallBatch             = IS_HW_SUPPORT(gcvFEATURE_SMALL_BATCH) && Hardware->options.smallBatch;
    pVscHwCfg->hwFeatureFlags.hasImageOutBoundaryFix = IS_HW_SUPPORT(gcvFEATURE_IMAGE_OUT_BOUNDARY_FIX);
    pVscHwCfg->hwFeatureFlags.supportTexldCoordOffset= gcvFALSE;
    /* Whether HW can support ATOM operations for LS, now we use SEPARATE_LS to detect. */
    pVscHwCfg->hwFeatureFlags.supportLSAtom          = IS_HW_SUPPORT(gcvFEATURE_SEPARATE_LS);
    /* Now HW can't support unordered branch. */
    pVscHwCfg->hwFeatureFlags.supportUnOrdBranch     = gcvFALSE;
    /* Now HW can't support patchVerticesIn, we need to use a constant register to save it. */
    pVscHwCfg->hwFeatureFlags.supportPatchVerticesIn = gcvFALSE;
    pVscHwCfg->hwFeatureFlags.hasHalfDepFix          = IS_HW_SUPPORT(gcvFEATURE_SH_HALF_DEPENDENCY_FIX);
    pVscHwCfg->hwFeatureFlags.supportUSC             = IS_HW_SUPPORT(gcvFEATURE_USC);
    pVscHwCfg->hwFeatureFlags.supportPartIntBranch   = IS_HW_SUPPORT(gcvFEATURE_PARTLY_SUPPORT_INTEGER_BRANCH);
    pVscHwCfg->hwFeatureFlags.supportIntAttrib       = IS_HW_SUPPORT(gcvFEATURE_SUPPORT_INTEGER_ATTRIBUTE);
    pVscHwCfg->hwFeatureFlags.hasTxBiasLodFix        = IS_HW_SUPPORT(gcvFEATURE_TEXTURE_BIAS_LOD_FIX);
    pVscHwCfg->hwFeatureFlags.supportmovai           = IS_HW_SUPPORT(gcvFEATURE_SUPPORT_MOVAI);
    pVscHwCfg->hwFeatureFlags.useGLZ                 = IS_HW_SUPPORT(gcvFEATURE_USE_GL_Z);
    pVscHwCfg->hwFeatureFlags.supportHelperInv       = IS_HW_SUPPORT(gcvFEATURE_HELPER_INVOCATION);
    pVscHwCfg->hwFeatureFlags.supportAdvBlendPart0   = IS_HW_SUPPORT(gcvFEATURE_ADVANCED_BLEND_MODE_PART0);
    pVscHwCfg->hwFeatureFlags.supportStartVertexFE   = IS_HW_SUPPORT(gcvFEATURE_FE_START_VERTEX_SUPPORT);
    pVscHwCfg->hwFeatureFlags.supportTxGather        = IS_HW_SUPPORT(gcvFEATURE_TEXTURE_GATHER);
    pVscHwCfg->hwFeatureFlags.singlePipeHalti1       = IS_HW_SUPPORT(gcvFEATURE_SINGLE_PIPE_HALTI1);
    pVscHwCfg->hwFeatureFlags.supportEVISVX2         = IS_HW_SUPPORT(gcvFEATURE_EVIS_VX2);
    pVscHwCfg->hwFeatureFlags.computeOnly            = IS_HW_SUPPORT(gcvFEATURE_COMPUTE_ONLY);
    pVscHwCfg->hwFeatureFlags.hasBugFix7             = IS_HW_SUPPORT(gcvFEATURE_BUG_FIXES7);
    pVscHwCfg->hwFeatureFlags.hasExtraInst2          = IS_HW_SUPPORT(gcvFEATURE_SHADER_HAS_EXTRA_INSTRUCTIONS2);
    pVscHwCfg->hwFeatureFlags.hasAtomic              = IS_HW_SUPPORT(gcvFEATURE_SHADER_HAS_ATOMIC);
    pVscHwCfg->hwFeatureFlags.supportFullIntBranch   = IS_HW_SUPPORT(gcvFEATURE_FULLLY_SUPPORT_INTEGER_BRANCH);
    pVscHwCfg->hwFeatureFlags.supportUnifiedConstant = IS_HW_SUPPORT(gcvFEATURE_SMALL_BATCH) && Hardware->options.smallBatch;
    pVscHwCfg->hwFeatureFlags.supportUnifiedSampler  = IS_HW_SUPPORT(gcvFEATURE_SMALL_BATCH) && Hardware->options.smallBatch;
    /* Now HW can only support single component 8bit/16bit integer DIV. */
    pVscHwCfg->hwFeatureFlags.support32BitIntDiv     = gcvFALSE;
    pVscHwCfg->hwFeatureFlags.supportFullCompIntDiv  = gcvFALSE;
    pVscHwCfg->hwFeatureFlags.supportComplex         = IS_HW_SUPPORT(gcvFEATURE_SH_CMPLX);
    pVscHwCfg->hwFeatureFlags.supportBigEndianLdSt   = IS_HW_SUPPORT(gcvFEATURE_SH_GM_ENDIAN);
    pVscHwCfg->hwFeatureFlags.supportUSCUnalloc      = IS_HW_SUPPORT(gcvFEATURE_SH_GM_USC_UNALLOC);
    pVscHwCfg->hwFeatureFlags.supportEndOfBBReissue  = IS_HW_SUPPORT(gcvFEATURE_SH_END_OF_BB);
    pVscHwCfg->hwFeatureFlags.hasDynamicIdxDepFix    = IS_HW_SUPPORT(gcvFEATURE_HALTI5);
    pVscHwCfg->hwFeatureFlags.supportPSCSThrottle    = IS_HW_SUPPORT(gcvFEATURE_PSCS_THROTTLE);
    /* LODQ doesn't return the correct raw LOD value, which can match the spec requirement. */
    pVscHwCfg->hwFeatureFlags.hasLODQFix             = gcvFALSE;
    pVscHwCfg->hwFeatureFlags.supportHWManagedLS     = IS_HW_SUPPORT(gcvFEATURE_HWMANAGED_LS);
    pVscHwCfg->hwFeatureFlags.hasScatteredMemAccess  = IS_HW_SUPPORT(gcvFEATURE_SH_SCATTER_GATHER);
    pVscHwCfg->hwFeatureFlags.supportSeparatedTex    = gcvFALSE;
    pVscHwCfg->hwFeatureFlags.supportMultiGPU        = gcvTRUE;
    pVscHwCfg->hwFeatureFlags.hasPointSizeFix        = IS_HW_SUPPORT(gcvFEATURE_MAX_POINTSIZE_CLAMP);
    pVscHwCfg->hwFeatureFlags.supportVectorB0        = gcvFALSE;

    pVscHwCfg->hwFeatureFlags.FEDrawDirect           = IS_HW_SUPPORT(gcvFEATURE_FE_DRAW_DIRECT);
    pVscHwCfg->hwFeatureFlags.hasUSCAtomicFix2       = IS_HW_SUPPORT(gcvFEATURE_USC_ATOMIC_FIX2);

OnError:
    gcmFOOTER();
    return status;
}

static void
_StallHw(
    gcoHARDWARE Hardware
    )
{
    gctBOOL stallStates = gcvFALSE;
    gctBOOL stallDraw = gcvFALSE;
    gcsHINT_PTR hints = Hardware->SHStates->programState.hints;
    gcsPROGRAM_UNIFIED_STATUS *prevProgUnifiedStatus = &Hardware->prevProgramUnfiedStatus;
    gctBOOL reconfigUSC = gcvFALSE;
    gctBOOL smallBatch = Hardware->features[gcvFEATURE_SMALL_BATCH] && Hardware->options.smallBatch;
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        if (Hardware->features[gcvFEATURE_USC])
        {
            if ((Hardware->prevProgramStageBits & gcvPROGRAM_STAGE_COMPUTE_BIT) !=
                (hints->stageBits & gcvPROGRAM_STAGE_COMPUTE_BIT))
            {
                reconfigUSC = gcvTRUE;
            }
        }

        if (((Hardware->config->instructionCount > 256) &&
             (Hardware->config->instructionCount <= 1024) &&
             (!Hardware->features[gcvFEATURE_BUG_FIXES7]))
            )
        {
            stallStates = gcvTRUE;
            break;
        }

        if ((Hardware->prevProgramStageBits & gcvPROGRAM_STAGE_COMPUTE_BIT) &&
             Hardware->prevProgramBarrierUsed &&
             !(hints->stageBits & gcvPROGRAM_STAGE_COMPUTE_BIT))
        {
            stallDraw = gcvTRUE;
        }

        if (prevProgUnifiedStatus->useIcache != hints->unifiedStatus.useIcache)
        {
            stallStates = gcvTRUE;
            break;
        }

        if (prevProgUnifiedStatus->instruction != hints->unifiedStatus.instruction)
        {
            gcmASSERT(prevProgUnifiedStatus->useIcache != hints->unifiedStatus.useIcache);
        }

        if ((prevProgUnifiedStatus->constantUnifiedMode == gcvUNIFORM_ALLOC_NONE_UNIFIED && hints->unifiedStatus.constantUnifiedMode != gcvUNIFORM_ALLOC_NONE_UNIFIED)
            ||
            (prevProgUnifiedStatus->constantUnifiedMode != gcvUNIFORM_ALLOC_NONE_UNIFIED && hints->unifiedStatus.constantUnifiedMode == gcvUNIFORM_ALLOC_NONE_UNIFIED))
        {
            gcmASSERT(gcvFALSE);
        }

        if ((hints->unifiedStatus.instruction) && (!hints->unifiedStatus.useIcache))
        {
            if ((hints->unifiedStatus.instVSEnd >= prevProgUnifiedStatus->instPSStart) ||
                (hints->unifiedStatus.instPSStart <= prevProgUnifiedStatus->instVSEnd))
            {
                stallStates = gcvTRUE;
                break;
            }
        }

        if (hints->unifiedStatus.constantUnifiedMode == gcvUNIFORM_ALLOC_GPIPE_TOP_PS_BOTTOM_FLOAT_BASE_OFFSET && !smallBatch)
        {
            if ((hints->unifiedStatus.constGPipeEnd >= prevProgUnifiedStatus->constPSStart) ||
                (hints->unifiedStatus.constPSStart <= prevProgUnifiedStatus->constGPipeEnd))
            {
                stallStates = gcvTRUE;
                break;
            }
        }

        if (hints->unifiedStatus.samplerUnifiedMode == gcvUNIFORM_ALLOC_PS_TOP_GPIPE_BOTTOM_FLOAT_BASE_OFFSET && !smallBatch)
        {
            if ((hints->unifiedStatus.samplerPSEnd >= prevProgUnifiedStatus->samplerGPipeStart) ||
                (hints->unifiedStatus.samplerGPipeStart <= prevProgUnifiedStatus->samplerPSEnd))
            {
                stallStates = gcvTRUE;
                break;
            }
        }

    }while (gcvFALSE);

    if (stallStates)
    {
        gcmONERROR(
            gcoHARDWARE_Semaphore(Hardware, gcvWHERE_COMMAND, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE_STALL, gcvNULL));
    }
    else if (stallDraw)
    {
        gcmONERROR(
            gcoHARDWARE_Semaphore(Hardware, gcvWHERE_COMMAND, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE, gcvNULL));
    }


    if (Hardware->features[gcvFEATURE_SNAPPAGE_CMD] &&
        Hardware->features[gcvFEATURE_SNAPPAGE_CMD_FIX])
    {
        gcePROGRAM_STAGE_BIT snapStags = ((Hardware->prevProgramStageBits & (~hints->stageBits)) &
                                          gcvPORGRAM_STAGE_GPIPE);
        if (snapStags)
        {
            gcmVERIFY_OK(
                gcoHARDWARE_SnapPages(
                    Hardware,
                    snapStags,
                    gcvNULL));
        }
    }

    if (reconfigUSC && gcoHAL_GetOption(gcvNULL, gcvOPTION_PREFER_USC_RECONFIG))
    {
        gctUINT32 uscConfig;
        gcmASSERT(Hardware->features[gcvFEATURE_USC]);

        if (hints->stageBits & (gcvPROGRAM_STAGE_COMPUTE_BIT | gcvPROGRAM_STAGE_OPENCL_BIT))
        {
            if (Hardware->features[gcvFEATURE_USC_FULLCACHE_FIX])
            {
                uscConfig = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:16) - (0 ?
 20:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:16) - (0 ?
 20:16) + 1))))))) << (0 ?
 20:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 20:16) - (0 ?
 20:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:16) - (0 ? 20:16) + 1))))))) << (0 ? 20:16)));
            }
            else
            {
                uscConfig = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) (0x6 & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:16) - (0 ?
 20:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:16) - (0 ?
 20:16) + 1))))))) << (0 ?
 20:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 20:16) - (0 ?
 20:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:16) - (0 ? 20:16) + 1))))))) << (0 ? 20:16)));
            }
        }
        else
        {
            uscConfig = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:0) - (0 ?
 2:0) + 1))))))) << (0 ?
 2:0))) | (((gctUINT32) ((gctUINT32) (Hardware->options.uscL1CacheRatio) & ((gctUINT32) ((((1 ?
 2:0) - (0 ?
 2:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 20:16) - (0 ?
 20:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 20:16) - (0 ?
 20:16) + 1))))))) << (0 ?
 20:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ?
 20:16) - (0 ?
 20:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 20:16) - (0 ? 20:16) + 1))))))) << (0 ? 20:16)));
        }

        gcmONERROR(gcoHARDWARE_LoadCtrlStateNEW(Hardware,
                                                0x0380C,
                                                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:5) - (0 ?
 5:5) + 1))))))) << (0 ?
 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
                                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:10) - (0 ?
 10:10) + 1))))))) << (0 ?
 10:10))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10)))
                                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 11:11) - (0 ?
 11:11) + 1))))))) << (0 ?
 11:11))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 11:11) - (0 ?
 11:11) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))),
                                                gcvNULL));

        if (Hardware->features[gcvFEATURE_HW_TFB])
        {
            gcmONERROR(gcoHARDWARE_LoadCtrlStateNEW(Hardware,
                                                    0x1C00C,
                                                    0x1,
                                                    gcvNULL));
        }

        if (Hardware->features[gcvFEATURE_SNAPPAGE_CMD] &&
            Hardware->features[gcvFEATURE_SNAPPAGE_CMD_FIX])
        {
            gcmONERROR(
                gcoHARDWARE_Semaphore(Hardware,
                                      gcvWHERE_COMMAND,
                                      gcvWHERE_PIXEL,
                                      gcvHOW_SEMAPHORE_STALL,
                                      gcvNULL));
        }


        gcmONERROR(
            gcoHARDWARE_SnapPages(
                    Hardware,
                    0x1F,
                    gcvNULL));

        gcmONERROR(gcoHARDWARE_LoadState32NEW(Hardware,
                                                0x03884,
                                                uscConfig,
                                                gcvNULL));
    }

    /* overwrite to previous one */
    gcoOS_MemCopy(&Hardware->prevProgramUnfiedStatus,
                  &hints->unifiedStatus,
                  gcmSIZEOF(gcsPROGRAM_UNIFIED_STATUS));

    Hardware->prevProgramStageBits = hints->stageBits;
    Hardware->prevProgramBarrierUsed |= (hints->threadGroupSync);

OnError:

    return;
}


/*******************************************************************************
**
**  gcoHARDWARE_LoadProgram
**
**  Load multiple program state.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcsPROGRAM_STATE_PTR ProgramState,
**          Pointer to program state.
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_LoadProgram(
    IN gcoHARDWARE Hardware,
    IN gcePROGRAM_STAGE_BIT StageBits,
    IN gctPOINTER ProgramState
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x ProgramState=0x%x",
                   Hardware, ProgramState);

    gcmGETHARDWARE(Hardware);

    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (ProgramState == gcvNULL)
    {
        gcoOS_ZeroMemory(&Hardware->SHStates->programState, sizeof(gcsPROGRAM_STATE));
    }
    else
    {
        Hardware->SHStates->programState = *(gcsPROGRAM_STATE_PTR)ProgramState;
    }

    Hardware->SHDirty->shaderDirty = gcvTRUE;
    /* If program changed, CTC also changed if exist.*/
    Hardware->SHDirty->uniformDirty = gcvTRUE;

    if ((Hardware->prevProgramStageBits & gcvPROGRAM_STAGE_COMPUTE_BIT) !=
        (Hardware->SHStates->programState.hints->stageBits & gcvPROGRAM_STAGE_COMPUTE_BIT) &&
        !Hardware->features[gcvFEATURE_PSIO_MSAA_CL_FIX])
    {
        Hardware->MsaaDirty->msaaConfigDirty = gcvTRUE;
    }

    /* Need to reprogram centroid table for some DEQP/CTS cases. */
    if ((!Hardware->features[gcvFEATURE_HALTI5]) &&
        (Hardware->patchID == gcvPATCH_DEQP || Hardware->patchID == gcvPATCH_GTFES30))
    {
        Hardware->MsaaDirty->centroidsDirty = gcvTRUE;

        Hardware->MsaaDirty->msaaConfigDirty = gcvTRUE;
        Hardware->MsaaDirty->msaaModeDirty = gcvTRUE;
    }

    /* Multiple core rendering mode flush relies on these flags,
    ** if hints changes, they need to be dirty to make sure rendering
    ** mode is flushed. */

    Hardware->multiGPURenderingModeDirty = gcvTRUE;

    _StallHw(Hardware);

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoHARDWARE_InvokeThreadWalkerCL
**
**  Start OCL thread walker.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**
**      gcsTHREAD_WALKER_INFO_PTR Info
**          Pointer to the informational structure.
*/

gceSTATUS
gcoHARDWARE_InvokeThreadWalkerCL(
                   IN gcoHARDWARE Hardware,
                   IN gcsTHREAD_WALKER_INFO_PTR Info
                   )
{
    gceSTATUS status;
    gctPOINTER *cmdBuffer = gcvNULL;
    gctUINT allocation;
    gctUINT groupNumberPerClusterEachGPU[gcdMAX_3DGPU_COUNT] = { 0 };
    gctUINT i;
    gctUINT localMemSizeInByte = 0;
    gcsTHREAD_WALKER_INFO eachGPUInfo[gcdMAX_3DGPU_COUNT];
    gcsTHREAD_WALKER_INFO_PTR info;
    gctUINT gpuCount, usedGPUCount;
    gctUINT groupCountX,groupCountY,groupCountZ,restGroupCount;
    gctINT32 maxDimIndex = 0;


    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x Info=0x%x", Hardware, Info);

    gcmGETHARDWARE(Hardware);

    gpuCount = Hardware->config->gpuCoreCount;
    usedGPUCount = gpuCount;
    if ((gpuCount > 1) && !(Info->memoryAccessFlag & gceMA_FLAG_ATOMIC))
    {
        maxDimIndex = Info->workGroupCountY > Info->workGroupCountX ?  1 : maxDimIndex;
        maxDimIndex = Info->workGroupCountZ > gcmMAX(Info->workGroupCountY, Info->workGroupCountX) ? 2: maxDimIndex;

        for(i = 0 ;i < gpuCount; i++)
        {
            eachGPUInfo[i] = *Info;
        }

        switch(maxDimIndex)
        {
        case 0:
            {
                groupCountX     = Info->workGroupCountX / gpuCount;
                restGroupCount  = Info->workGroupCountX % gpuCount;

                for (i = 0; i < gpuCount; i++)
                {
                    eachGPUInfo[i].workGroupCountX = groupCountX;
                    eachGPUInfo[i].globalSizeX = groupCountX * Info->workGroupSizeX;
                }

                for(i = 0; i < restGroupCount; i++)
                {
                     eachGPUInfo[i].workGroupCountX++;
                     eachGPUInfo[i].globalSizeX = eachGPUInfo[i].workGroupCountX * Info->workGroupSizeX;
                }

                if (groupCountX == 0)
                    usedGPUCount = restGroupCount;

                for(i = 1; i < usedGPUCount; i++)
                {
                    eachGPUInfo[i].globalOffsetX = eachGPUInfo[i-1].workGroupCountX * Info->workGroupSizeX * eachGPUInfo[i].globalScaleX + eachGPUInfo[i-1].globalOffsetX;
                }
            }
            break;
        case 1:
            {
                groupCountY     = Info->workGroupCountY / gpuCount;
                restGroupCount  = Info->workGroupCountY % gpuCount;

                for (i = 0; i < gpuCount; i++)
                {
                    eachGPUInfo[i].workGroupCountY = groupCountY;
                    eachGPUInfo[i].globalSizeY = groupCountY * Info->workGroupSizeY;
                }

                for(i = 0; i < restGroupCount; i++)
                {
                    eachGPUInfo[i].workGroupCountY++;
                    eachGPUInfo[i].globalSizeY = eachGPUInfo[i].workGroupCountY * Info->workGroupSizeY;
                }

                if (groupCountY == 0)
                    usedGPUCount = restGroupCount;

                for(i = 1; i < usedGPUCount; i++)
                {
                     eachGPUInfo[i].globalOffsetY = eachGPUInfo[i-1].workGroupCountY * Info->workGroupSizeY * eachGPUInfo[i].globalScaleY + eachGPUInfo[i-1].globalOffsetY;
                }
            }
            break;
        case 2:
            {
                groupCountZ     = Info->workGroupCountZ / gpuCount;
                restGroupCount  = Info->workGroupCountZ % gpuCount;

                for (i = 0; i < gpuCount; i++)
                {
                    eachGPUInfo[i].workGroupCountZ = groupCountZ;
                    eachGPUInfo[i].globalSizeZ = groupCountZ * Info->workGroupSizeZ;
                }

                for(i = 0; i < restGroupCount; i++)
                {
                    eachGPUInfo[i].workGroupCountZ++;
                    eachGPUInfo[i].globalSizeZ = eachGPUInfo[i].workGroupCountZ * Info->workGroupSizeZ;
                }

                if (groupCountZ == 0)
                    usedGPUCount = restGroupCount;

                for(i = 1; i < usedGPUCount; i++)
                {
                     eachGPUInfo[i].globalOffsetZ = eachGPUInfo[i-1].workGroupCountZ * Info->workGroupSizeZ * eachGPUInfo[i].globalScaleZ + eachGPUInfo[i-1].globalOffsetZ;
                }
            }
            break;
        default:
            gcmASSERT(0);
            break;

        }
    }
    else  /*atomicUsed true or independent mode, all workgroup will be send to GPU0*/
    {
        usedGPUCount = 1;
        eachGPUInfo[0] = *Info;
    }

    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, cmdBuffer);

    cmdBuffer = (gctPOINTER *)&memory;

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, cmdBuffer));

    if (Hardware->TXDirty->textureDirty)
    {
        gcmONERROR((*Hardware->funcPtr->programTexture)(Hardware, cmdBuffer));
    }

    if (Hardware->SHDirty->uniformDirty)
    {
        /* Flush uniform states*/
        gcmONERROR(gcoHARDWARE_FlushUniform(Hardware, cmdBuffer));
    }

    if (Hardware->SHDirty->shaderDirty)
    {
        /* Flush shader states. */
        gcmONERROR(gcoHARDWARE_FlushShaders(Hardware, gcvPRIMITIVE_TRIANGLE_LIST, cmdBuffer));
    }

    if (Hardware->features[gcvFEATURE_SH_INSTRUCTION_PREFETCH])
    {
        gcmONERROR(gcoHARDWARE_FlushPrefetchInst(Hardware, cmdBuffer));
    }

    if (Hardware->features[gcvFEATURE_DRAW_ID])
    {
        gcmONERROR(gcoHARDWARE_FlushDrawID(Hardware, gcvTRUE, cmdBuffer));
    }

    cmdBuffer = gcvNULL;

    /* Calculate thread allocation. */
    for(i = 0; i < usedGPUCount; i++)
    {
        allocation = eachGPUInfo[i].workGroupSizeX;

        if (eachGPUInfo[i].dimensions > 1)
        {
            allocation *= eachGPUInfo[i].workGroupSizeY;
        }

        if (eachGPUInfo[i].dimensions > 2)
        {
            allocation *= eachGPUInfo[i].workGroupSizeZ;
        }

        eachGPUInfo[i].threadAllocation = gcmCEIL((gctFLOAT)allocation / (Hardware->config->shaderCoreCount * 4));

        if (Hardware->config->clusterAliveMask > 0)
        {
            groupNumberPerClusterEachGPU[i] = (Hardware->config->shaderCoreCount * 4 * (eachGPUInfo[i].bDual16 ? 2 : 1)) / allocation;

            groupNumberPerClusterEachGPU[i] = groupNumberPerClusterEachGPU[i] > 1 ? groupNumberPerClusterEachGPU[i] -1 : 0;

            groupNumberPerClusterEachGPU[i] = gcmMIN(63, groupNumberPerClusterEachGPU[i]);
        }
    }

    if (Hardware->features[gcvFEATURE_SHADER_ENHANCEMENTS2])
    {
        gctBOOL bEnableWGPack = gcvFALSE;

        if (Hardware->features[gcvFEATURE_SH_MULTI_WG_PACK] &&
            !Info->barrierUsed &&
            !(Info->memoryAccessFlag & gceMA_FLAG_EVIS_ATOMADD))
        {
            bEnableWGPack = gcvTRUE;
        }

        for(i = 0; i < usedGPUCount; i++)
        {
            info = &eachGPUInfo[i];

            if (gpuCount > 1)
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
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(i); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(i));
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0240) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0240, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) ((gctUINT32) (info->dimensions) & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:4) - (0 ?
 6:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:4) - (0 ?
 6:4) + 1))))))) << (0 ?
 6:4))) | (((gctUINT32) ((gctUINT32) (info->traverseOrder) & ((gctUINT32) ((((1 ?
 6:4) - (0 ?
 6:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:4) - (0 ?
 6:4) + 1))))))) << (0 ?
 6:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:8) - (0 ?
 8:8) + 1))))))) << (0 ?
 8:8))) | (((gctUINT32) ((gctUINT32) (info->enableSwathX) & ((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:8) - (0 ?
 8:8) + 1))))))) << (0 ?
 8:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:9) - (0 ?
 9:9) + 1))))))) << (0 ?
 9:9))) | (((gctUINT32) ((gctUINT32) (info->enableSwathY) & ((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:9) - (0 ?
 9:9) + 1))))))) << (0 ?
 9:9))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:10) - (0 ?
 10:10) + 1))))))) << (0 ?
 10:10))) | (((gctUINT32) ((gctUINT32) (info->enableSwathZ) & ((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:10) - (0 ?
 10:10) + 1))))))) << (0 ?
 10:10))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:12) - (0 ?
 15:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:12) - (0 ?
 15:12) + 1))))))) << (0 ?
 15:12))) | (((gctUINT32) ((gctUINT32) (info->swathSizeX) & ((gctUINT32) ((((1 ?
 15:12) - (0 ?
 15:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:12) - (0 ?
 15:12) + 1))))))) << (0 ?
 15:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (info->swathSizeY) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:20) - (0 ?
 23:20) + 1))))))) << (0 ?
 23:20))) | (((gctUINT32) ((gctUINT32) (info->swathSizeZ) & ((gctUINT32) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:20) - (0 ?
 23:20) + 1))))))) << (0 ?
 23:20))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:24) - (0 ?
 26:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:24) - (0 ?
 26:24) + 1))))))) << (0 ?
 26:24))) | (((gctUINT32) ((gctUINT32) (info->valueOrder) & ((gctUINT32) ((((1 ?
 26:24) - (0 ?
 26:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


            if (Hardware->features[gcvFEATURE_PSCS_THROTTLE] && Hardware->SHStates->programState.hints)
            {
                localMemSizeInByte = gcmCEIL((gctFLOAT)Hardware->SHStates->programState.hints->localMemSizeInByte  / 16.0);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0249) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0249, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (localMemSizeInByte) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) (bEnableWGPack) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:20) - (0 ?
 25:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:20) - (0 ?
 25:20) + 1))))))) << (0 ?
 25:20))) | (((gctUINT32) ((gctUINT32) (groupNumberPerClusterEachGPU[i]) & ((gctUINT32) ((((1 ?
 25:20) - (0 ?
 25:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:20) - (0 ? 25:20) + 1))))))) << (0 ? 25:20))));    gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0247) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0247, info->threadAllocation );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x024B) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x024B, info->globalOffsetX );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x024D) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x024D, info->globalOffsetY );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x024F) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x024F, info->globalOffsetZ );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0256) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0256, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:0) - (0 ?
 7:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:0) - (0 ?
 7:0) + 1))))))) << (0 ?
 7:0))) | (((gctUINT32) ((gctUINT32) (info->globalScaleX) & ((gctUINT32) ((((1 ?
 7:0) - (0 ?
 7:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0257) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0257, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:0) - (0 ?
 7:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:0) - (0 ?
 7:0) + 1))))))) << (0 ?
 7:0))) | (((gctUINT32) ((gctUINT32) (info->globalScaleY) & ((gctUINT32) ((((1 ?
 7:0) - (0 ?
 7:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0258) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0258, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 7:0) - (0 ?
 7:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 7:0) - (0 ?
 7:0) + 1))))))) << (0 ?
 7:0))) | (((gctUINT32) ((gctUINT32) (info->globalScaleZ) & ((gctUINT32) ((((1 ?
 7:0) - (0 ?
 7:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


           {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)6  <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (6 ) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0250) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


           gcmSETSTATEDATA_NEW(
               stateDelta, reserve, memory, gcvFALSE, 0x0250,
               info->workGroupCountX - 1
               );

           gcmSETSTATEDATA_NEW(
               stateDelta, reserve, memory, gcvFALSE, 0x0251,
               info->workGroupCountY - 1
               );

           gcmSETSTATEDATA_NEW(
               stateDelta, reserve, memory, gcvFALSE, 0x0252,
               info->workGroupCountZ - 1
               );

           gcmSETSTATEDATA_NEW(
               stateDelta, reserve, memory, gcvFALSE, 0x0253,
               ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (info->workGroupSizeX - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
               );

           gcmSETSTATEDATA_NEW(
               stateDelta, reserve, memory, gcvFALSE, 0x0254,
               ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (info->workGroupSizeY - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
               );

           gcmSETSTATEDATA_NEW(
               stateDelta, reserve, memory, gcvFALSE, 0x0255,
               ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (info->workGroupSizeZ - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0248) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0248, 0xBADABEEB );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }

        if (gpuCount > 1)
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

        }
    }
    else
    {
        for(i = 0 ;i <usedGPUCount; i++)
        {
            info = &eachGPUInfo[i];

            if (gpuCount > 1)
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
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(i); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(i));
 } };

            }
            {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)8  <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (8 ) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0240) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0240,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) ((gctUINT32) (info->dimensions) & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:4) - (0 ?
 6:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:4) - (0 ?
 6:4) + 1))))))) << (0 ?
 6:4))) | (((gctUINT32) ((gctUINT32) (info->traverseOrder) & ((gctUINT32) ((((1 ?
 6:4) - (0 ?
 6:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:8) - (0 ?
 8:8) + 1))))))) << (0 ?
 8:8))) | (((gctUINT32) ((gctUINT32) (info->enableSwathX) & ((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:9) - (0 ?
 9:9) + 1))))))) << (0 ?
 9:9))) | (((gctUINT32) ((gctUINT32) (info->enableSwathY) & ((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:10) - (0 ?
 10:10) + 1))))))) << (0 ?
 10:10))) | (((gctUINT32) ((gctUINT32) (info->enableSwathZ) & ((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:12) - (0 ?
 15:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:12) - (0 ?
 15:12) + 1))))))) << (0 ?
 15:12))) | (((gctUINT32) ((gctUINT32) (info->swathSizeX) & ((gctUINT32) ((((1 ?
 15:12) - (0 ?
 15:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (info->swathSizeY) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:20) - (0 ?
 23:20) + 1))))))) << (0 ?
 23:20))) | (((gctUINT32) ((gctUINT32) (info->swathSizeZ) & ((gctUINT32) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:24) - (0 ?
 26:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:24) - (0 ?
 26:24) + 1))))))) << (0 ?
 26:24))) | (((gctUINT32) ((gctUINT32) (info->valueOrder) & ((gctUINT32) ((((1 ?
 26:24) - (0 ?
 26:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24)))
                );

            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0241,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (info->globalSizeX) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (info->globalOffsetX) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                );

            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0242,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (info->globalSizeY) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (info->globalOffsetY) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                );

            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0243,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (info->globalSizeZ) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (info->globalOffsetZ) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                );

            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0244,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (info->workGroupSizeX - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (info->workGroupCountX - 1) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                );

            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0245,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (info->workGroupSizeY - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (info->workGroupCountY - 1) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                );

            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0246,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (info->workGroupSizeZ - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (info->workGroupCountZ - 1) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
                );

            gcmSETSTATEDATA_NEW(
                stateDelta, reserve, memory, gcvFALSE, 0x0247,
                info->threadAllocation
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0248) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0248, 0xBADABEEB );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }

        if (gpuCount > 1)
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

        }
    }

    if (Hardware->features[gcvFEATURE_MCFE])
    {
        /* SubmitJob. */
        *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x16 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) (0x001 & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

        *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, cmdBuffer);

    gcoBUFFER_OnIssueFence(Hardware->engine[gcvENGINE_RENDER].buffer);

    if (!Hardware->features[gcvFEATURE_MCFE])
    {
        /* Flush the Shader L1 cache. */
        if (!Hardware->config->parallelNoFix || !Hardware->options.enableNNTPParallel || Hardware->options.enableSwtilingPhase1)
        {
            gcmONERROR(gcoHARDWARE_LoadCtrlState(
                Hardware,
                0x0380C,
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:5) - (0 ?
 5:5) + 1))))))) << (0 ?
 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
                  | (Hardware->features[gcvFEATURE_MULTI_CLUSTER] ?
                     ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) :
                     ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:2) - (0 ?
 2:2) + 1))))))) << (0 ?
 2:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))))
                ));
        }
        else
        {
            gcmONERROR(gcoHARDWARE_LoadCtrlState(
                Hardware,
                0x0380C,
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:5) - (0 ?
 5:5) + 1))))))) << (0 ?
 5:5))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
                  | (Hardware->features[gcvFEATURE_MULTI_CLUSTER] ?
                     ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) :
                     ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 2:2) - (0 ?
 2:2) + 1))))))) << (0 ?
 2:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 2:2) - (0 ?
 2:2) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))))
                ));
        }

        /* ICACHE need be invalidate when if the core is compute only and in combineMode */
        if(Hardware->features[gcvFEATURE_COMPUTE_ONLY] )
        {
            if(Hardware->config->gpuCoreCount > 1)
            {
                /* Invalidate Shader Icache. */
                if (gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_HALTI5))
                {
                    gcmONERROR(gcoHARDWARE_LoadCtrlState(
                        Hardware,
                        0x008B0, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
                        ));
                }

            }
        }

        /* Add FE - PE samaphore stall to prevent unwanted SHL1_CACHE flush. */
        gcmONERROR(gcoHARDWARE_Semaphore(
            Hardware, gcvWHERE_COMMAND, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE_STALL, gcvNULL));
        gcoHARDWARE_MultiGPUSync(Hardware, gcvNULL);

        gcoHARDWARE_SendFence(Hardware, gcvFALSE, gcvENGINE_RENDER, gcvNULL);
    }
    else
    {
        /* Flush the Shader L1 cache. */
        if (!Hardware->config->parallelNoFix || !Hardware->options.enableNNTPParallel || Hardware->options.enableSwtilingPhase1)
        {
            gcmONERROR(gcoHARDWARE_LoadCtrlState(
                Hardware,
                0x0380C, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:5) - (0 ?
 5:5) + 1))))))) << (0 ?
 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
                ));
        }
        else
        {
            gcmONERROR(gcoHARDWARE_LoadCtrlState(
                Hardware,
                0x0380C, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 5:5) - (0 ?
 5:5) + 1))))))) << (0 ?
 5:5))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ?
 5:5) - (0 ?
 5:5) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
                ));
        }

        /* Invalidate Shader Icache. */
        if (gcoHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_HALTI5))
        {
            gcmONERROR(gcoHARDWARE_LoadCtrlState(
                Hardware,
                0x008B0, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
                ));
        }

        /* SubmitJob is pared with the complete signal from FlushCache. */
        gcoHARDWARE_McfeSubmitJob(Hardware, gcvNULL);

        gcoHARDWARE_MultiGPUSync(Hardware, gcvNULL);
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoHARDWARE_InvokeThreadWalkerGL
**
**  Start OGL thread walker.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**
**      gcsTHREAD_WALKER_INFO_PTR Info
**          Pointer to the informational structure.
*/

gceSTATUS
gcoHARDWARE_InvokeThreadWalkerGL(
    IN gcoHARDWARE Hardware,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;
    gctPOINTER  *outSide = gcvNULL;
    gctUINT allocation;
    gctUINT groupNumberPerCluster = 0;
    gctUINT localMemSizeInByte = 0;
    gctBOOL bEnableWGPack = gcvFALSE;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x Info=0x%x", Hardware, Info);

    gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

    gcmGETHARDWARE(Hardware);
    /* Calculate thread allocation. */
    allocation = Info->workGroupSizeX *Info->workGroupSizeY * Info->workGroupSizeZ;

    Info->threadAllocation = gcmCEIL((gctFLOAT)allocation / (Hardware->config->shaderCoreCount * 4));
    Info->valueOrder = Hardware->SHStates->programState.hints->valueOrder;

    if (Hardware->features[gcvFEATURE_SH_MULTI_WG_PACK] &&
        !Info->barrierUsed)
    {
        bEnableWGPack = gcvTRUE;
    }

    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outSide);

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D, (gctPOINTER*)&memory));

    if (Hardware->SHDirty->uniformDirty)
    {
        /* Flush uniform states*/
        gcmONERROR(gcoHARDWARE_FlushUniform(Hardware, (gctPOINTER*)&memory));
    }

    if (Hardware->TXDirty->textureDirty)
    {
        gcmONERROR((*Hardware->funcPtr->programTexture)(Hardware, (gctPOINTER*)&memory));
    }

    if (Hardware->SHDirty->shaderDirty)
    {
        gcmONERROR(gcoHARDWARE_FlushDepthOnly(Hardware));
        /* Flush shader states. */
        gcmONERROR(gcoHARDWARE_FlushShaders(Hardware, gcvPRIMITIVE_TRIANGLE_LIST, (gctPOINTER*)&memory));
    }

    if (Hardware->MsaaDirty->msaaConfigDirty)
    {
        /* Flush anti-alias states. */
        gcmONERROR(gcoHARDWARE_FlushSampling(Hardware, (gctPOINTER*)&memory));
    }

    if (Hardware->features[gcvFEATURE_SH_INSTRUCTION_PREFETCH])
    {
        gcmONERROR(gcoHARDWARE_FlushPrefetchInst(Hardware, (gctPOINTER*)&memory));
    }

    if (Hardware->features[gcvFEATURE_DRAW_ID])
    {
        gcmONERROR(gcoHARDWARE_FlushDrawID(Hardware, gcvFALSE, (gctPOINTER*)&memory));
    }

    if (Hardware->config->gpuCoreCount > 1)
    {
        /* TODO: select optimum rendering mode for different statemetn */
        gcmONERROR(gcoHARDWARE_FlushMultiGPURenderingMode(Hardware, (gctPOINTER*)&memory, gcvMULTI_GPU_RENDERING_MODE_INTERLEAVED_128x64));
    }

#if gcdENABLE_TRUST_APPLICATION&&0
    if (Hardware->features[gcvFEATURE_SECURITY] && Hardware->GPUProtecedModeDirty)
    {
        gcmONERROR(gcoHARDWARE_FlushProtectMode(Hardware, (gctPOINTER*)&memory));
    }
#endif

    if (Hardware->stallSource < Hardware->stallDestination)
    {
        /* Stall if we have to. */
        gcmONERROR(gcoHARDWARE_Semaphore(Hardware,
                                         Hardware->stallSource,
                                         Hardware->stallDestination,
                                         gcvHOW_STALL,
                                         (gctPOINTER*)&memory));
    }

    gcmASSERT(Hardware->features[gcvFEATURE_SHADER_ENHANCEMENTS2]);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0240) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0240, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | (((gctUINT32) ((gctUINT32) (Info->dimensions) & ((gctUINT32) ((((1 ?
 1:0) - (0 ?
 1:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 1:0) - (0 ?
 1:0) + 1))))))) << (0 ?
 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 6:4) - (0 ?
 6:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:4) - (0 ?
 6:4) + 1))))))) << (0 ?
 6:4))) | (((gctUINT32) ((gctUINT32) (Info->traverseOrder) & ((gctUINT32) ((((1 ?
 6:4) - (0 ?
 6:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 6:4) - (0 ?
 6:4) + 1))))))) << (0 ?
 6:4))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:8) - (0 ?
 8:8) + 1))))))) << (0 ?
 8:8))) | (((gctUINT32) ((gctUINT32) (Info->enableSwathX) & ((gctUINT32) ((((1 ?
 8:8) - (0 ?
 8:8) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:8) - (0 ?
 8:8) + 1))))))) << (0 ?
 8:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:9) - (0 ?
 9:9) + 1))))))) << (0 ?
 9:9))) | (((gctUINT32) ((gctUINT32) (Info->enableSwathY) & ((gctUINT32) ((((1 ?
 9:9) - (0 ?
 9:9) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:9) - (0 ?
 9:9) + 1))))))) << (0 ?
 9:9))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:10) - (0 ?
 10:10) + 1))))))) << (0 ?
 10:10))) | (((gctUINT32) ((gctUINT32) (Info->enableSwathZ) & ((gctUINT32) ((((1 ?
 10:10) - (0 ?
 10:10) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 10:10) - (0 ?
 10:10) + 1))))))) << (0 ?
 10:10))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:12) - (0 ?
 15:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:12) - (0 ?
 15:12) + 1))))))) << (0 ?
 15:12))) | (((gctUINT32) ((gctUINT32) (Info->swathSizeX) & ((gctUINT32) ((((1 ?
 15:12) - (0 ?
 15:12) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:12) - (0 ?
 15:12) + 1))))))) << (0 ?
 15:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | (((gctUINT32) ((gctUINT32) (Info->swathSizeY) & ((gctUINT32) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 19:16) - (0 ?
 19:16) + 1))))))) << (0 ?
 19:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:20) - (0 ?
 23:20) + 1))))))) << (0 ?
 23:20))) | (((gctUINT32) ((gctUINT32) (Info->swathSizeZ) & ((gctUINT32) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 23:20) - (0 ?
 23:20) + 1))))))) << (0 ?
 23:20))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:24) - (0 ?
 26:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:24) - (0 ?
 26:24) + 1))))))) << (0 ?
 26:24))) | (((gctUINT32) ((gctUINT32) (Info->valueOrder) & ((gctUINT32) ((((1 ?
 26:24) - (0 ?
 26:24) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    if (Hardware->config->clusterAliveMask > 0)
    {
        groupNumberPerCluster = (Hardware->config->shaderCoreCount * 4 * (Info->bDual16 ? 2 : 1)) / allocation;

        groupNumberPerCluster = groupNumberPerCluster > 1 ? groupNumberPerCluster -1 : 0;

        groupNumberPerCluster = gcmMIN(63, groupNumberPerCluster);
    }

    if (Hardware->features[gcvFEATURE_PSCS_THROTTLE] && Hardware->SHStates->programState.hints)
    {
        localMemSizeInByte = gcmCEIL((gctFLOAT)Hardware->SHStates->programState.hints->localMemSizeInByte  / 16.0);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0249) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0249, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (localMemSizeInByte) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) (bEnableWGPack) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:20) - (0 ?
 25:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:20) - (0 ?
 25:20) + 1))))))) << (0 ?
 25:20))) | (((gctUINT32) ((gctUINT32) (groupNumberPerCluster) & ((gctUINT32) ((((1 ?
 25:20) - (0 ?
 25:20) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:20) - (0 ? 25:20) + 1))))))) << (0 ? 25:20))));    gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0247) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0247, Info->threadAllocation );
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x024B) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x024B, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (Info->globalOffsetX) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x024D) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x024D, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (Info->globalOffsetY) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x024F) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x024F, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:16) - (0 ?
 31:16) + 1))))))) << (0 ?
 31:16))) | (((gctUINT32) ((gctUINT32) (Info->globalOffsetZ) & ((gctUINT32) ((((1 ?
 31:16) - (0 ?
 31:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) );    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    if (Hardware->features[gcvFEATURE_COMPUTE_INDIRECT] && Info->indirect)
    {
        {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)3  <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (3 ) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0253) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0253,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeX - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            );

        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0254,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeY - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            );

        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0255,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeZ - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            );

        gcmENDSTATEBATCH_NEW(
            reserve, memory
            );
    }
    else
    {
        {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)6  <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (6 ) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0250) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0250,
            Info->workGroupCountX - 1
            );

        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0251,
            Info->workGroupCountY - 1
            );

        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0252,
            Info->workGroupCountZ - 1
            );

        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0253,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeX - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            );

        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0254,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeY - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            );

        gcmSETSTATEDATA_NEW(
            stateDelta, reserve, memory, gcvFALSE, 0x0255,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 9:0) - (0 ?
 9:0) + 1))))))) << (0 ?
 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeZ - 1) & ((gctUINT32) ((((1 ?
 9:0) - (0 ?
 9:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            );

        gcmSETFILLER_NEW(
            reserve, memory
            );

        gcmENDSTATEBATCH_NEW(
            reserve, memory
            );
    }

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


    if (Hardware->features[gcvFEATURE_COMPUTE_INDIRECT] && Info->indirect)
    {
        gctUINT32 constantBase = 0;
        gceUNIFOEM_ALLOC_MODE unifiedConst = Hardware->SHStates->programState.hints->unifiedStatus.constantUnifiedMode;

        if (unifiedConst == gcvUNIFORM_ALLOC_NONE_UNIFIED)
        {
            constantBase = 0x00000100;
        }

        gcmASSERT(Hardware->threadWalkerInPS);
        if ((unifiedConst != gcvUNIFORM_ALLOC_NONE_UNIFIED && unifiedConst != gcvUNIFORM_ALLOC_FULL_UNIFIED) && !Hardware->features[gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH])
        {
            /* ComputeIndirect will program unified c0, set correct mode */
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, ((((gctUINT32) (Hardware->SHStates->programState.hints->shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 16:16) - (0 ?
 16:16) + 1))))))) << (0 ?
 16:16))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ?
 16:16) - (0 ?
 16:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

        }

        /* set uniform to store work group count */
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x01F3) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x01F3, constantBase + Info->groupNumberUniformIdx);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        /* Program the GCCMD_DRAW_INDIRECT_COMMAND.Command data. */
        memory[0] =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x11 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

        memory[1] = Info->baseAddress;

        memory += 2;


        /* Dump the indirect compute command. */
        gcmDUMP(gcvNULL,
                "#[compute.indirectindex (%d,%d,%d) 0x%08X]",
                Info->workGroupSizeX, Info->workGroupSizeY, Info->workGroupSizeZ, Info->baseAddress);

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0248) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETCTRLSTATE_NEW(stateDelta, reserve, memory, 0x0248, 0xBADABEEB );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


        if (Hardware->features[gcvFEATURE_MCFE])
        {
            /* SubmitJob. */
            *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x16 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) (0x001 & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

            *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ?
 31:27))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
        }

        gcmDUMP(gcvNULL,
                "#[compute (%d,%d,%d), workGroupNum(%d,%d,%d)]",
                 Info->workGroupSizeX, Info->workGroupSizeY, Info->workGroupSizeZ,
                 Info->workGroupCountX, Info->workGroupCountY, Info->workGroupCountZ);
    }

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

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outSide);


OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


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
    )
{
    gceSTATUS status;
    gctUINT i, j;
    gctUINT32_PTR src = (gctUINT32_PTR) Values;
    gctUINT32 address = Address >> 2;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x, Address=%u Columns=%u Rows=%u Values=%p FixedPoint=%d Type=%d",
                  Hardware, Address, Columns, Rows, Values, FixedPoint, Type);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Values != gcvNULL);

    /* Get the current hardware object. */
    gcmGETHARDWARE(Hardware);

    gcmASSERT(Hardware->maxState == 0 || address < Hardware->maxState);

    /* Determine the size of the buffer to reserve. */
    reserveSize = Rows * gcmALIGN((1 + Columns) * gcmSIZEOF(gctUINT32), 8);

    if (Hardware->unifiedConst && !Hardware->features[gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH])
    {
        reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
    }

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    if (Hardware->unifiedConst && !Hardware->features[gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH])
    {
        gctUINT32 shaderConfigData = Hardware->SHStates->programState.hints ?
                                     Hardware->SHStates->programState.hints->shaderConfigData : 0;

        shaderConfigData = ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) ((Type != gcSHADER_TYPE_VERTEX)) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

        {    {    gcmASSERT(((memory - gcmUINT64_TO_TYPE(reserve->lastReserve, gctUINT32_PTR)) & 1) == 0);
    gcmASSERT((gctUINT32)1 <= 1024);
    gcmVERIFYLOADSTATEDONE(reserve);
    gcmSTORELOADSTATE(reserve, memory, 0x0218, 1);
    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
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
 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
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
 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
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
 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0218, shaderConfigData);
     gcmENDSTATEBATCH(reserve, memory);
};

    }

    /* Walk all rows. */
    for (i = 0; i < Rows; i++)
    {
        /* Begin the state batch. */
        {    gcmASSERT(((memory - gcmUINT64_TO_TYPE(reserve->lastReserve, gctUINT32_PTR)) & 1) == 0);
    gcmASSERT((gctUINT32)Columns <= 1024);
    gcmVERIFYLOADSTATEDONE(reserve);
    gcmSTORELOADSTATE(reserve, memory, address, Columns);
    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
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
 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | (((gctUINT32) ((gctUINT32) (FixedPoint) & ((gctUINT32) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 26:26) - (0 ?
 26:26) + 1))))))) << (0 ?
 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | (((gctUINT32) ((gctUINT32) (Columns) & ((gctUINT32) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 25:16) - (0 ?
 25:16) + 1))))))) << (0 ?
 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 15:0) - (0 ?
 15:0) + 1))))))) << (0 ?
 15:0))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


        /* Walk all columns. */
        for (j = 0; j < Columns; j++)
        {
            gctINT32 data = *src++;
            if (ConvertToFloat)
            {
                gctFLOAT fData = (gctFLOAT)data;
                data = *((gctUINT32_PTR)&fData);
            }

            /* Set the state value. */
            gcmSETSTATEDATA(stateDelta,
                            reserve,
                            memory,
                            FixedPoint,
                            address + j,
                            data);
        }

        if ((j & 1) == 0)
        {
            /* Align. */
            gcmSETFILLER(reserve, memory);
        }

        /* End the state batch. */
        gcmENDSTATEBATCH(reserve, memory);

        /* Next row. */
        address += 4;
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
gcoHARDWARE_FlushUniform(
    IN gcoHARDWARE Hardware,
    INOUT gctPOINTER *Memory
    )
{
    gceSTATUS status;
    gctINT32 i, j, uniformCount;
    gctUINT32 k;
    gcsUNIFORM_STATE * uniformState;
    gcsHINT_PTR hints = gcvNULL;
    gctBOOL smallBatch = gcvFALSE;
    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Get the current hardware object. */
    gcmGETHARDWARE(Hardware);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, Memory);

    /* user defined uniforms.*/
    uniformCount = Hardware->SHStates->uniformBindCount;
    hints        = Hardware->SHStates->programState.hints;
    smallBatch   = Hardware->features[gcvFEATURE_SMALL_BATCH] && Hardware->options.smallBatch;

    /* before program uniforms, add "allocation".*/
    if (smallBatch &&
        hints->unifiedStatus.constCount != 0 &&
        hints->unifiedStatus.constantUnifiedMode != gcvUNIFORM_ALLOC_NONE_UNIFIED)
    {
        /* now, always copy.*/
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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x042B) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x042B, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | (((gctUINT32) ((gctUINT32) (hints->unifiedStatus.constCount) & ((gctUINT32) ((((1 ?
 8:0) - (0 ?
 8:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 8:0) - (0 ?
 8:0) + 1))))))) << (0 ?
 8:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 31:31) - (0 ?
 31:31) + 1))))))) << (0 ?
 31:31))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ?
 31:31) - (0 ?
 31:31) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))));    gcmENDSTATEBATCH_NEW(reserve, memory);
};

    }

    for (i = 0; i < uniformCount; i++)
    {
        uniformState = &Hardware->SHStates->uniformStatesPool[i];

        if (uniformState->info.dirty)
        {
            if (Hardware->unifiedConst && !Hardware->features[gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH])
            {
                gctUINT32 shaderConfigData = hints ?
                                             hints->shaderConfigData : 0;

                shaderConfigData = ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) ((uniformState->info.type != gcSHADER_TYPE_VERTEX)) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, shaderConfigData);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

            }

            if (uniformState->info.dirty == 0xf && !uniformState->combinedDirty)
            {
                /* Begin the state batch. */
                    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)4 <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (4) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (uniformState->address) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


                    for (j = 0; j < 4; j++)
                    {
                        /* Set the state value. */
                        gcmSETSTATEDATA_NEW(stateDelta,
                            reserve,
                            memory,
                            gcvFALSE,
                            uniformState->address + j,
                            uniformState->data[0][j]);
                    }
                    /* Align. */
                    gcmSETFILLER_NEW(reserve, memory);

                    /* End the state batch. */
                    gcmENDSTATEBATCH_NEW(reserve, memory);

            }
            else if(uniformState->info.dirty == 0xf && uniformState->combinedDirty == 0xf)
            {
                for(k = 0; k < Hardware->config->gpuCoreCount; k++)
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
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(k); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(k));
 } };

                    {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)4 <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (4) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (uniformState->address) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


                    for (j = 0; j < 4; j++)
                    {
                        /* Set the state value. */
                        gcmSETSTATEDATA_NEW(stateDelta,
                            reserve,
                            memory,
                            gcvFALSE,
                            uniformState->address + j,
                            uniformState->data[k][j]);
                    }
                    /* Align. */
                    gcmSETFILLER_NEW(reserve, memory);

                    /* End the state batch. */
                    gcmENDSTATEBATCH_NEW(reserve, memory);


                }
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

            }
            else
            {

                for (j = 0; j < 4; j++)  /*Flush Uniform data that is not combined*/
                {
                    if ((uniformState->info.dirty & (1 << j)) & (~uniformState->combinedDirty))
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
 15:0))) | (((gctUINT32) ((gctUINT32) (uniformState->address + j) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, uniformState->address + j, uniformState->data[0][j]);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                    }
                }

                for(k = 0; k < Hardware->config->gpuCoreCount; k++)  /*Flush Uniform data that is combined*/
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
 31:27) - (0 ?
 31:27) + 1))))))) << (0 ? 31:27))) | gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(k); memory++;
  gcmDUMP(gcvNULL, "#[chip.enable 0x%04X]", gcvCORE_3D_0_MASK << gcmTO_CHIP_ID(k));
 } };

                    for (j = 0; j < 4; j++)
                    {
                        if ((uniformState->info.dirty & (1 << j)) & (uniformState->combinedDirty))
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
 15:0))) | (((gctUINT32) ((gctUINT32) (uniformState->address + j) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, uniformState->address + j, uniformState->data[k][j]);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

                        }
                    }
                }
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


            }

            /* reset uniformState */
            uniformState->info.dirty = 0;
            uniformState->combinedDirty = 0;
            uniformState->address = 0;
            Hardware->SHStates->uniformStatesIndex[uniformState->info.index] = 0xffffffff;
            uniformState->info.index = 0;
        }
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, Memory);

    Hardware->SHDirty->uniformDirty = gcvFALSE;
    Hardware->SHStates->uniformBindCount = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

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
    IN gctBOOL CombinedMode
    )
{
    gceSTATUS status;
    gctUINT arr, row, col, k;
    gctUINT8_PTR pArray[gcdMAX_3DGPU_COUNT];
    gctUINT32 address;
    gcsUNIFORM_STATE * uniformState;
    gctINT32 constIndex = 0;
    gcsHINT_PTR hints;
    gctUINT shift, columns, rows;
    static const gctUINT32 _shaderTypeToProgramType[] = {
        0, /* gcSHADER_TYPE_UNKNOWN.*/
        gcvPROGRAM_STAGE_VERTEX, /* gcSHADER_TYPE_VERTEX.*/
        gcvPROGRAM_STAGE_FRAGMENT, /* gcSHADER_TYPE_FRAGMENT.*/
        gcvPROGRAM_STAGE_COMPUTE, /* gcSHADER_TYPE_COMPUTE.*/
        gcvPROGRAM_STAGE_OPENCL, /* gcSHADER_TYPE_CL.*/
        0, /* gcSHADER_TYPE_PRECOMPILED.*/
        0, /* gcSHADER_TYPE_LIBRARY.*/
        0, /* gcSHADER_TYPE_VERTEX_DEFAULT_UBO.*/
        0, /* gcSHADER_TYPE_FRAGMENT_DEFAULT_UBO.*/
        gcvPROGRAM_STAGE_TCS, /* gcSHADER_TYPE_TCS.*/
        gcvPROGRAM_STAGE_TES, /* gcSHADER_TYPE_TES.*/
        gcvPROGRAM_STAGE_GEOMETRY       /* gcSHADER_TYPE_GEOMETRY.*/
    };

    static gctUINT32 _shaderStageBase[] = {
        0, /* gcSHADER_TYPE_UNKNOWN.*/
        0, /* gcSHADER_TYPE_VERTEX.*/
        0, /* gcSHADER_TYPE_FRAGMENT.*/
        0, /* gcSHADER_TYPE_COMPUTE.*/
        0     /* gcSHADER_TYPE_CL.*/
    };

    gcmHEADER_ARG("Hardware=0x%x, Address=%u Physical=%d Columns=%u Rows=%u Arrays=%u IsRowMajor=%d "
                  "MatrixStride=%d ArrayStride=%d Values=%p Convert=%d Type=%d",
                  Hardware, Address, Physical, Columns, Rows, Arrays, IsRowMajor,
                  MatrixStride, ArrayStride, Values, Convert, Type);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Values != gcvNULL);

    /* Get the current hardware object. */
    gcmGETHARDWARE(Hardware);
    pArray[0] = (gctUINT8_PTR)Values[0];

    if(CombinedMode)
    {
        for(k = 1; k < Hardware->config->gpuCoreCount; k++)
        {
            pArray[k] = (gctUINT8_PTR) Values[k];
        }
    }
    hints = Hardware->SHStates->programState.hints;

    address = gcmALIGN_BASE(Address, 16) >> 2;

    gcmASSERT(Hardware->maxState == 0 || address < Hardware->maxState);

    shift = (Address >> 2) - address;
    columns = Columns;
    rows = Rows;

    if (Hardware->unifiedConst)
    {
        gcmASSERT(Type == gcSHADER_TYPE_VERTEX ||
            Type == gcSHADER_TYPE_FRAGMENT ||
            Type == gcSHADER_TYPE_COMPUTE ||
            Type == gcSHADER_TYPE_CL ||
            Type == gcSHADER_TYPE_TCS ||
            Type == gcSHADER_TYPE_TES ||
            Type == gcSHADER_TYPE_GEOMETRY);

        constIndex = hints->constRegNoBase[_shaderTypeToProgramType[Type]] + Physical;
    }
    else
    {
        gcmASSERT(Type == gcSHADER_TYPE_VERTEX ||
            Type == gcSHADER_TYPE_FRAGMENT ||
            Type == gcSHADER_TYPE_CL);

        _shaderStageBase[gcSHADER_TYPE_FRAGMENT] = Hardware->vsConstMax;
        constIndex = _shaderStageBase[Type] + Physical;
    }

    for (arr = 0; arr < Arrays; ++arr)
    {
        /* Walk all rows. */
        for (row = 0; row < rows; ++row)
        {
            /* get uniformStatesIndex.*/
            if (Hardware->SHStates->uniformStatesIndex[constIndex] == 0xffffffff)
            {
                Hardware->SHStates->uniformStatesIndex[constIndex] = Hardware->SHStates->uniformBindCount++;
            }

            /* Walk all columns. */
            uniformState = &Hardware->SHStates->uniformStatesPool[Hardware->SHStates->uniformStatesIndex[constIndex]];
            gcmASSERT((gctUINT32)Hardware->SHStates->uniformBindCount < Hardware->constMax);

            for (col = 0; col < columns; ++col)
            {
                gctUINT32 dataPerGpu[gcdMAX_3DGPU_COUNT];


                gctUINT8_PTR pData = IsRowMajor
                                   ? pArray[0] + col * MatrixStride + (row << 2)
                                   : pArray[0] + row * MatrixStride + (col << 2);

                gctUINT32 data = *(gctUINT32_PTR)pData;


                if (Convert == gcvUNIFORMCVT_TO_BOOL)
                {
                    data = (data == 0) ? 0 : 1;
                }
                else if (Convert == gcvUNIFORMCVT_TO_FLOAT)
                {
                    gctFLOAT fData = (gctFLOAT)(gctINT)data;
                    data = *((gctUINT32_PTR)&fData);
                }

                dataPerGpu[0] = data;

                if(CombinedMode)
                {
                    for(k = 1; k < Hardware->config->gpuCoreCount; k++)
                    {
                         pData = IsRowMajor
                            ? pArray[k] + col * MatrixStride + (row << 2)
                            : pArray[k] + row * MatrixStride + (col << 2);

                         data = *(gctUINT32_PTR)pData;


                        if (Convert == gcvUNIFORMCVT_TO_BOOL)
                        {
                            data = (data == 0) ? 0 : 1;
                        }
                        else if (Convert == gcvUNIFORMCVT_TO_FLOAT)
                        {
                            gctFLOAT fData = (gctFLOAT)(gctINT)data;
                            data = *((gctUINT32_PTR)&fData);
                        }
                        dataPerGpu[k] = data;

                    }
                }

                /*handle the situation which the uniform beyonds the boundry of the vec4*/
                if (shift + col == 4)
                {
                    shift = 0;
                    rows++;
                    columns -= col;
                    pArray[0] += (col << 2);
                    if(CombinedMode)
                    {
                        for(k = 1 ; k < Hardware->config->gpuCoreCount; k++)
                        {
                            pArray[k] += (col << 2);
                        }
                    }
                    break;
                }

                uniformState->data[0][shift + col] = dataPerGpu[0];
                 if(CombinedMode)
                    {
                        for(k = 1 ; k < Hardware->config->gpuCoreCount; k++)
                        {
                           uniformState->data[k][shift + col]=  dataPerGpu[k];
                        }
                        uniformState->combinedDirty |= 1 << (shift + col); /*Means the index shift + col 's data is for mulitGPU */
                    }
                uniformState->info.dirty |= 1 << (shift + col);
            }

            /*record the uniform physical, address and type*/
            uniformState->address = address;
            uniformState->info.type = Type;
            uniformState->info.index = constIndex;

            /* update iterater.*/
            constIndex++;
            address += 4;
        }

        pArray[0] += ArrayStride;

        if(CombinedMode)
        {
            for(k = 1 ; k < Hardware->config->gpuCoreCount; k++)
            {
                pArray[k] +=  ArrayStride;
            }
        }
    }

    Hardware->SHDirty->uniformDirty = gcvTRUE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:

    /* Return the status. */
    gcmFOOTER();
    return status;
}

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
    )
{
    gceSTATUS status;
    gctUINT arr, row, col;
    gctUINT32 address = Address >> 2;
    gctPOINTER *outSide = gcvNULL;
    gctUINT8_PTR pArray = (gctUINT8_PTR)Values;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x, Address=%u Columns=%u Rows=%u Arrays=%u IsRowMajor=%d "
                  "MatrixStride=%d ArrayStride=%d Values=%p Convert=%d Type=%d",
                  Hardware, Address, Columns, Rows, Arrays, IsRowMajor,
                  MatrixStride, ArrayStride, Values, Convert, Type);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Values != gcvNULL);

    /* Get the current hardware object. */
    gcmGETHARDWARE(Hardware);

    gcmASSERT(Hardware->maxState == 0 || address < Hardware->maxState);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outSide);

    if (Hardware->unifiedConst && !Hardware->features[gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH])
    {
        gctUINT32 shaderConfigData = Hardware->SHStates->programState.hints ?
                                     Hardware->SHStates->programState.hints->shaderConfigData : 0;

        shaderConfigData = ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) ((Type != gcSHADER_TYPE_VERTEX)) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, shaderConfigData);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

    }

    for (arr = 0; arr < Arrays; ++arr)
    {
        /* Walk all rows. */
        for (row = 0; row < Rows; ++row)
        {
            /* Begin the state batch. */
            {    gcmVERIFYLOADSTATEALIGNED(reserve, memory);
    gcmASSERT((gctUINT32)Columns <= 1024);
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
 25:16))) | (((gctUINT32) ((gctUINT32) (Columns) & ((gctUINT32) ((((1 ?
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
 15:0))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};


            /* Walk all columns. */
            for (col = 0; col < Columns; ++col)
            {
                gctUINT8_PTR pData = IsRowMajor
                                   ? pArray + col * MatrixStride + (row << 2)
                                   : pArray + row * MatrixStride + (col << 2);

                gctUINT data = *(gctUINT_PTR)pData;

                if (Convert == gcvUNIFORMCVT_TO_BOOL)
                {
                    data = (data == 0) ? 0 : 1;
                }
                else if (Convert == gcvUNIFORMCVT_TO_FLOAT)
                {
                    gctFLOAT fData = (gctFLOAT)(gctINT)data;
                    data = *((gctUINT32_PTR)&fData);
                }

                /* Set the state value. */
                gcmSETSTATEDATA_NEW(stateDelta,
                                    reserve,
                                    memory,
                                    gcvFALSE,
                                    address + col,
                                    data);
            }

            if ((col & 1) == 0)
            {
                /* Align. */
                gcmSETFILLER_NEW(reserve, memory);
            }

            /* End the state batch. */
            gcmENDSTATEBATCH_NEW(reserve, memory);

            /* Next row. */
            address += 4;
        }

        pArray += ArrayStride;
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outSide);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_BindBufferBlock(
        IN gcoHARDWARE Hardware,
        IN gctUINT32 Address,
        IN gctUINT32 Base,
        IN gctSIZE_T Offset,
        IN gctSIZE_T Size,
        IN gcSHADER_KIND Type
        )
{
    gceSTATUS status;
    gctUINT32 address = Address >> 2;
    gctPOINTER *outSide = gcvNULL;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER_NEW(reserve, stateDelta, memory);

    gcmHEADER_ARG("Hardware=0x%x Address=%u Base=%u Offset=%lu Size=%lu Type=%d",
                  Hardware, Address, Base, Offset, Size, Type);

    /* Get the current hardware object. */
    gcmGETHARDWARE(Hardware);

    gcmASSERT(Hardware->maxState == 0 || address < Hardware->maxState);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER_NEW(Hardware, reserve, stateDelta, memory, outSide);

    if (Hardware->unifiedConst && !Hardware->features[gcvFEATURE_NEW_STEERING_AND_ICACHE_FLUSH])
    {
        gctUINT32 shaderConfigData = Hardware->SHStates->programState.hints ?
                                        Hardware->SHStates->programState.hints->shaderConfigData : 0;

        shaderConfigData = ((((gctUINT32) (shaderConfigData)) & ~(((gctUINT32) (((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ?
 4:4) - (0 ?
 4:4) + 1))))))) << (0 ?
 4:4))) | (((gctUINT32) ((gctUINT32) ((Type != gcSHADER_TYPE_VERTEX)) & ((gctUINT32) ((((1 ?
 4:4) - (0 ?
 4:4) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

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
 15:0))) | (((gctUINT32) ((gctUINT32) (0x0218) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, 0x0218, shaderConfigData);
    gcmENDSTATEBATCH_NEW(reserve, memory);
};

     }

    /* Set buffer location. */
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
 15:0))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));};
    gcmSETSTATEDATA_NEW(stateDelta, reserve, memory, gcvFALSE, address, Base + Offset );
    gcmENDSTATEBATCH_NEW(reserve, memory);
};


    /* Validate the state buffer. */
    gcmENDSTATEBUFFER_NEW(Hardware, reserve, memory, outSide);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif


