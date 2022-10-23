
/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include "vpuapifunc.h"
#include "product.h"
#include "vdi_osal.h"
#include "enc_regdefine.h"
#include "debug.h"


#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
#define MAX_LAVEL_IDX    16
static const int g_anLevel[MAX_LAVEL_IDX] =
{
    10, 11, 11, 12, 13,
    //10, 16, 11, 12, 13,
    20, 21, 22,
    30, 31, 32,
    40, 41, 42,
    50, 51
};

static const int g_anLevelMaxMBPS[MAX_LAVEL_IDX] =
{
    1485,   1485,   3000,   6000, 11880,
    11880,  19800,  20250,
    40500,  108000, 216000,
    245760, 245760, 522240,
    589824, 983040
};

static const int g_anLevelMaxFS[MAX_LAVEL_IDX] =
{
    99,    99,   396, 396, 396,
    396,   792,  1620,
    1620,  3600, 5120,
    8192,  8192, 8704,
    22080, 36864
};

static const int g_anLevelMaxBR[MAX_LAVEL_IDX] =
{
    64,     64,   192,  384, 768,
    2000,   4000,  4000,
    10000,  14000, 20000,
    20000,  50000, 50000,
    135000, 240000
};

static const int g_anLevelSliceRate[MAX_LAVEL_IDX] =
{
    0,  0,  0,  0,  0,
    0,  0,  0,
    22, 60, 60,
    60, 24, 24,
    24, 24
};

static const int g_anLevelMaxMbs[MAX_LAVEL_IDX] =
{
    28,   28,  56, 56, 56,
    56,   79, 113,
    113, 169, 202,
    256, 256, 263,
    420, 543
};

/******************************************************************************
    define value
******************************************************************************/

/******************************************************************************
    Codec Instance Slot Management
******************************************************************************/

RetCode InitCodecInstancePool(Uint32 coreIdx)
{
    int i;
    CodecInst * pCodecInst;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    if (vip->instance_pool_inited==0)
    {
        for( i = 0; i < MAX_NUM_INSTANCE; i++)
        {
            pCodecInst = (CodecInst *)vip->codecInstPool[i];
            pCodecInst->instIndex = i;
            pCodecInst->inUse = 0;
        }
        vip->instance_pool_inited = 1;
    }
    return RETCODE_SUCCESS;
}

/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(Uint32 coreIdx, CodecInst ** ppInst)
{
    int                     i;
    CodecInst*              pCodecInst = 0;
    vpu_instance_pool_t*    vip;
    Uint32                  handleSize;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        pCodecInst = (CodecInst *)vip->codecInstPool[i];

        if (!pCodecInst) {
            return RETCODE_FAILURE;
        }

        if (!pCodecInst->inUse) {
            break;
        }
    }

    if (i == MAX_NUM_INSTANCE) {
        *ppInst = 0;
        return RETCODE_FAILURE;
    }

    pCodecInst->inUse         = 1;
    pCodecInst->coreIdx       = coreIdx;
    pCodecInst->codecMode     = -1;
    pCodecInst->codecModeAux  = -1;
    pCodecInst->loggingEnable = 0;
    pCodecInst->isDecoder     = TRUE;
    pCodecInst->productId     = ProductVpuGetId(coreIdx);
    osal_memset((void*)&pCodecInst->CodecInfo, 0x00, sizeof(pCodecInst->CodecInfo));

    handleSize = sizeof(DecInfo);
    if (handleSize < sizeof(EncInfo)) {
        handleSize = sizeof(EncInfo);
    }
    if ((pCodecInst->CodecInfo=(void*)osal_malloc(handleSize)) == NULL) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }
    osal_memset(pCodecInst->CodecInfo, 0x00, sizeof(handleSize));

    *ppInst = pCodecInst;

    if (vdi_open_instance(pCodecInst->coreIdx, pCodecInst->instIndex) < 0) {
        return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst * pCodecInst)
{
    pCodecInst->codecMode    = -1;
    pCodecInst->codecModeAux = -1;

    vdi_close_instance(pCodecInst->coreIdx, pCodecInst->instIndex);

    osal_free(pCodecInst->CodecInfo);
    pCodecInst->CodecInfo = NULL;
    pCodecInst->inUse = 0;
}

RetCode CheckInstanceValidity(CodecInst * pCodecInst)
{
    int i;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        if ((CodecInst *)vip->codecInstPool[i] == pCodecInst)
            return RETCODE_SUCCESS;
    }

    return RETCODE_INVALID_HANDLE;
}

/******************************************************************************
    API Subroutines
******************************************************************************/

Uint64 GetTimestamp(
    EncHandle handle
    )
{
    CodecInst*  pCodecInst = (CodecInst*)handle;
    EncInfo*    pEncInfo   = NULL;
    Uint64      pts;
    Uint32      fps;

    if (pCodecInst == NULL) {
        return 0;
    }

    pEncInfo   = &pCodecInst->CodecInfo->encInfo;
    fps        = pEncInfo->openParam.frameRateInfo;
    if (fps == 0) {
        fps    = 30;        /* 30 fps */
    }

    pts        = pEncInfo->curPTS;
    pEncInfo->curPTS += 90000/fps; /* 90KHz/fps */

    return pts;
}

RetCode CalcEncCropInfo(CodecInst* instance, EncVpParam* param, int rotMode, int srcWidth, int srcHeight)
{
    int alignedWidth, alignedHeight, pad_right, pad_bot;
    int crop_right, crop_left, crop_top, crop_bot;
    int prp_mode = rotMode>>1;  // remove prp_enable bit

    if (instance->codecMode == W_AVC_ENC) {
        alignedWidth = (srcWidth + 15)&~15;
        alignedHeight= (srcHeight+ 15)&~15;
    }
    else {
        alignedWidth = (srcWidth + 31)&~31;
        alignedHeight= (srcHeight+ 31)&~31;
    }

    pad_right = alignedWidth - srcWidth;
    pad_bot   = alignedHeight - srcHeight;

    if (param->confWinRight > 0)
        crop_right = param->confWinRight + pad_right;
    else
        crop_right = pad_right;

    if (param->confWinBot > 0)
        crop_bot = param->confWinBot + pad_bot;
    else
        crop_bot = pad_bot;

    crop_top     = param->confWinTop;
    crop_left    = param->confWinLeft;

    param->confWinTop   = crop_top;
    param->confWinLeft  = crop_left;
    param->confWinBot   = crop_bot;
    param->confWinRight = crop_right;


    /* prp_mode :
    *          | hor_mir | ver_mir |   rot_angle
    *              [3]       [2]         [1:0] = {0= NONE, 1:90, 2:180, 3:270}
    */

    if(prp_mode == 1 || prp_mode ==15)
    {
        param->confWinTop   = crop_right;
        param->confWinLeft  = crop_top;
        param->confWinBot   = crop_left;
        param->confWinRight = crop_bot;
    }
    else if(prp_mode == 2 || prp_mode ==12)
    {
        param->confWinTop   = crop_bot;
        param->confWinLeft  = crop_right;
        param->confWinBot   = crop_top;
        param->confWinRight = crop_left;
    }
    else if(prp_mode == 3 || prp_mode ==13)
    {
        param->confWinTop   = crop_left;
        param->confWinLeft  = crop_bot;
        param->confWinBot   = crop_right;
        param->confWinRight = crop_top;
    }
    else if(prp_mode == 4 || prp_mode ==10)
    {
        param->confWinTop   = crop_bot;
        param->confWinBot   = crop_top;
    }
    else if(prp_mode == 8 || prp_mode ==6)
    {
        param->confWinLeft  = crop_right;
        param->confWinRight = crop_left;
    }
    else if(prp_mode == 5 || prp_mode ==11)
    {
        param->confWinTop   = crop_left;
        param->confWinLeft  = crop_top;
        param->confWinBot   = crop_right;
        param->confWinRight = crop_bot;
    }
    else if(prp_mode == 7 || prp_mode ==9)
    {
        param->confWinTop   = crop_right;
        param->confWinLeft  = crop_bot;
        param->confWinBot   = crop_left;
        param->confWinRight = crop_top;
    }

    return RETCODE_SUCCESS;
}

RetCode CheckEncInstanceValidity(EncHandle handle)
{
    CodecInst * pCodecInst;
    RetCode ret;

    if (handle == NULL)
        return RETCODE_INVALID_HANDLE;

    pCodecInst = handle;
    ret = CheckInstanceValidity(pCodecInst);
    if (ret != RETCODE_SUCCESS) {
        return RETCODE_INVALID_HANDLE;
    }
    if (!pCodecInst->inUse) {
        return RETCODE_INVALID_HANDLE;
    }

    if (pCodecInst->codecMode != MP4_ENC &&
        pCodecInst->codecMode != W_HEVC_ENC &&
        pCodecInst->codecMode != W_SVAC_ENC &&
        pCodecInst->codecMode != W_AVC_ENC  &&
        pCodecInst->codecMode != AVC_ENC) {
        return RETCODE_INVALID_HANDLE;
    }
    return RETCODE_SUCCESS;
}

RetCode CheckEncParam(EncHandle handle, EncParam * param)
{
    CodecInst *pCodecInst;
    EncInfo *pEncInfo;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    if (param == 0) {
        return RETCODE_INVALID_PARAM;
    }

    if (param->skipPicture != 0 && param->skipPicture != 1) {
        return RETCODE_INVALID_PARAM;
    }
    if (param->skipPicture == 0) {
        if (param->sourceFrame == 0) {
            return RETCODE_INVALID_FRAME_BUFFER;
        }
        if (param->forceIPicture != 0 && param->forceIPicture != 1) {
            return RETCODE_INVALID_PARAM;
        }
    }
    if (pEncInfo->openParam.bitRate == 0) { // no rate control
        if (pCodecInst->codecMode == MP4_ENC) {
            if (param->quantParam < 1 || param->quantParam > 31) {
                return RETCODE_INVALID_PARAM;
            }
        }
        else if (pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_SVAC_ENC) {
            if (param->forcePicQpEnable == 1) {
                if (param->forcePicQpI < 0 || param->forcePicQpI > 63)
                    return RETCODE_INVALID_PARAM;

                if (param->forcePicQpP < 0 || param->forcePicQpP > 63)
                    return RETCODE_INVALID_PARAM;

                if (param->forcePicQpB < 0 || param->forcePicQpB > 63)
                    return RETCODE_INVALID_PARAM;
            }
            if (pEncInfo->ringBufferEnable == 0) {
                if (param->picStreamBufferAddr % 16 || param->picStreamBufferSize == 0)
                    return RETCODE_INVALID_PARAM;
            }
        }
        else { // AVC_ENC
            if (param->quantParam < 0 || param->quantParam > 51) {
                return RETCODE_INVALID_PARAM;
            }
        }
    }
    if (pEncInfo->ringBufferEnable == 0) {
        if (param->picStreamBufferAddr % 8 || param->picStreamBufferSize == 0) {
            return RETCODE_INVALID_PARAM;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode EnterLock(Uint32 coreIdx)
{
    if (vdi_lock(coreIdx) != 0)
        return RETCODE_FAILURE;
    SetClockGate(coreIdx, 1);
    return RETCODE_SUCCESS;
}

RetCode LeaveLock(Uint32 coreIdx)
{
    SetClockGate(coreIdx, 0);
    vdi_unlock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode EnterLock_noclk(Uint32 coreIdx)
{
    if (vdi_lock(coreIdx) != 0)
        return RETCODE_FAILURE;
    return RETCODE_SUCCESS;
}

RetCode LeaveLock_noclk(Uint32 coreIdx)
{
    vdi_unlock(coreIdx);
    return RETCODE_SUCCESS;
}
RetCode SetClockGate(Uint32 coreIdx, Uint32 on)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip) {
        VLOG(ERR, "SetClockGate: RETCODE_INSUFFICIENT_RESOURCE\n");
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    vdi_set_clock_gate(coreIdx, on);

    return RETCODE_SUCCESS;
}

void SetPendingInst(Uint32 coreIdx, CodecInst *inst)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return;

    vip->pendingInst = inst;
	if (inst)
		vip->pendingInstIdxPlus1 = (inst->instIndex+1);
	else
		vip->pendingInstIdxPlus1 = 0;
}

void ClearPendingInst(Uint32 coreIdx)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return;

    if(vip->pendingInst) {
        vip->pendingInst = 0;
		vip->pendingInstIdxPlus1 = 0;
	}
}

CodecInst *GetPendingInst(Uint32 coreIdx)
{
    vpu_instance_pool_t *vip;
    int pendingInstIdx;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return NULL;

    if (!vip->pendingInst)
        return NULL;

    pendingInstIdx = vip->pendingInstIdxPlus1-1;
    if (pendingInstIdx < 0 || pendingInstIdx >= MAX_NUM_INSTANCE)
        return NULL;

    return  (CodecInst *)vip->codecInstPool[pendingInstIdx];
}

int GetPendingInstIdx(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return -1;

	return (vip->pendingInstIdxPlus1-1);
}

RetCode UpdateFrameBufferAddr(
    TiledMapType            mapType,
    FrameBuffer*            fbArr,
    Uint32                  numOfFrameBuffers,
    Uint32                  sizeLuma,
    Uint32                  sizeChroma
    )
{
    Uint32      i;
    BOOL        yuv422Interleave = FALSE;
    BOOL        fieldFrame       = (BOOL)(mapType == LINEAR_FIELD_MAP);
    BOOL        cbcrInterleave   = (BOOL)(mapType >= COMPRESSED_FRAME_MAP || fbArr[0].cbcrInterleave == TRUE);
    BOOL        reuseFb          = FALSE;


    if (mapType < COMPRESSED_FRAME_MAP) {
        switch (fbArr[0].format) {
        case FORMAT_YUYV:
        case FORMAT_YUYV_P10_16BIT_MSB:
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            yuv422Interleave = TRUE;
            break;
        default:
            yuv422Interleave = FALSE;
            break;
        }
    }

    for (i=0; i<numOfFrameBuffers; i++) {
        reuseFb = (fbArr[i].bufY != (PhysicalAddress)-1 && fbArr[i].bufCb != (PhysicalAddress)-1 && fbArr[i].bufCr != (PhysicalAddress)-1);
        if (reuseFb == FALSE) {
            if (yuv422Interleave == TRUE) {
                fbArr[i].bufCb = (PhysicalAddress)-1;
                fbArr[i].bufCr = (PhysicalAddress)-1;
            }
            else {
                if (fbArr[i].bufCb == (PhysicalAddress)-1) {
                    fbArr[i].bufCb = fbArr[i].bufY + (sizeLuma >> fieldFrame);
                }
                if (fbArr[i].bufCr == (PhysicalAddress)-1) {
                    if (cbcrInterleave == TRUE) {
                        fbArr[i].bufCr = (PhysicalAddress)-1;
                    }
                    else {
                        fbArr[i].bufCr = fbArr[i].bufCb + (sizeChroma >> fieldFrame);
                    }
                }
            }
        }
    }

    return RETCODE_SUCCESS;
}

Int32 ConfigSecAXIVp(Uint32 coreIdx, Int32 codecMode, SecAxiInfo *sa, Uint32 width, Uint32 height, Uint32 profile, Uint32 levelIdc)
{
    vpu_buffer_t vb;
    int offset;
    Uint32 size = 0;
    Uint32 lumaSize = 0;
    Uint32 chromaSize = 0;
    Uint32 productId;

    UNREFERENCED_PARAMETER(codecMode);
    UNREFERENCED_PARAMETER(height);

    if (vdi_get_sram_memory(coreIdx, &vb) < 0)
        return 0;

    productId = ProductVpuGetId(coreIdx);

    if (!vb.size) {
        sa->bufSize                = 0;
        sa->u.vp.useIpEnable    = 0;
        sa->u.vp.useLfRowEnable = 0;
        sa->u.vp.useBitEnable   = 0;
        sa->u.vp.useEncImdEnable   = 0;
        sa->u.vp.useEncLfEnable    = 0;
        sa->u.vp.useEncRdoEnable   = 0;
        return 0;
    }

    sa->bufBase = vb.phys_addr;
    offset      = 0;
    /* Intra Prediction */
    if (sa->u.vp.useIpEnable == TRUE) {
        sa->u.vp.bufIp = sa->bufBase + offset;

        switch (productId) {
        case PRODUCT_ID_512:
        case PRODUCT_ID_515:
            if ( codecMode == W_VP9_DEC ) {
                lumaSize   = VPU_ALIGN128(width) * 10/8;
                chromaSize = VPU_ALIGN128(width) * 10/8;
            }
            else if (codecMode == W_HEVC_DEC) {
                if (profile == HEVC_PROFILE_MAIN) {
                    lumaSize   = VPU_ALIGN32(width);
                    chromaSize = VPU_ALIGN32(width);
                }
                else {
                    lumaSize   = VPU_ALIGN128(VPU_ALIGN16(width)*10)/8;
                    chromaSize = VPU_ALIGN128(VPU_ALIGN16(width)*10)/8;
                }
            }
            else if (codecMode == W_AVS2_DEC) {
                if (profile == AVS2_PROFILE_MAIN10) {
                    lumaSize   = VPU_ALIGN128(VPU_ALIGN16(width)*10)/8;
                    chromaSize = VPU_ALIGN128(VPU_ALIGN16(width)*10)/8;
                }
                else {
                    lumaSize   = VPU_ALIGN16(width);
                    chromaSize = VPU_ALIGN16(width);
                }
            }
            else if (codecMode == W_SVAC_DEC) {
                int bitDepth = (profile == SVAC_PROFILE_BASE ? 8 : 10);
                lumaSize   = VPU_ALIGN128(width)*bitDepth/8;
                chromaSize = VPU_ALIGN128(width/2)*2*bitDepth/8;
            }
            else if (codecMode == W_AVC_DEC) {
                if (profile == 110) {       // High 10
                    lumaSize   = VPU_ALIGN128(VPU_ALIGN16(width)*10)/8;
                    chromaSize = VPU_ALIGN128(VPU_ALIGN16(width)*10)/8;
                }
                else {
                    lumaSize   = VPU_ALIGN32(width);
                    chromaSize = VPU_ALIGN32(width);
                }
            }
            break;
        case PRODUCT_ID_521:
        case PRODUCT_ID_511:
            if (profile == HEVC_PROFILE_MAIN10) {
                lumaSize = VPU_ALIGN128(VPU_ALIGN16(width)*10)/8*2;
                chromaSize = 0;
            }
            else {
                lumaSize = VPU_ALIGN16(width)*2;
                chromaSize = 0;
            }
            break;
        default:
            return 0;
        }

        offset     = lumaSize + chromaSize;
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    /* Loopfilter row */
    if (sa->u.vp.useLfRowEnable == TRUE) {
        sa->u.vp.bufLfRow = sa->bufBase + offset;
        if ( codecMode == W_VP9_DEC ) {
            if ( profile == VP9_PROFILE_2)
            {
                lumaSize   = VPU_ALIGN64(width) * 8 * 10/8; /* lumaLIne   : 8 */
                chromaSize = VPU_ALIGN64(width) * 8 * 10/8; /* chromaLine : 8 */
            }
            else
            {
                lumaSize   = VPU_ALIGN64(width) * 8; /* lumaLIne   : 8 */
                chromaSize = VPU_ALIGN64(width) * 8; /* chromaLine : 8 */
            }
        }
        else if (codecMode == W_HEVC_DEC) {
            if (profile == HEVC_PROFILE_MAIN) {
                size = VPU_ALIGN32(width)*8;
            }
            else {
                Uint32 level = levelIdc/30;
                if (level >= 5) {
                    size = VPU_ALIGN32(width)/2 * 13 + VPU_ALIGN64(width)*4;
                }
                else {
                    size = VPU_ALIGN64(width)*13;
                }
            }
            lumaSize = size;
            chromaSize = 0;
        }
        else if (codecMode == W_AVC_DEC) {
            if (profile == 110) {
                Uint32 level = levelIdc/30;
                if (level >= 5) {
                    size = VPU_ALIGN32(width)/2 * 13 + VPU_ALIGN64(width)*4;
                }
                else {
                    size = VPU_ALIGN64(width)*13;
                }
            }
            else {
              size = VPU_ALIGN32(width)*8;
            }
            lumaSize = size;
            chromaSize = 0;
        }
        else if (codecMode == W_AVS2_DEC) {
            // AVS2
            if (profile == AVS2_PROFILE_MAIN10) {
                lumaSize    = VPU_ALIGN16(width)*5*2;
                chromaSize  = VPU_ALIGN16(width)*5*2;
            }
            else {
                lumaSize    = VPU_ALIGN16(width)*5;
                chromaSize  = VPU_ALIGN16(width)*5;
            }
        }
        else if (codecMode == W_SVAC_DEC) {
            int bitDepth = (profile == SVAC_PROFILE_BASE ? 8 : 10);
            lumaSize    = VPU_ALIGN128(width)*bitDepth/8*5;
            chromaSize  = VPU_ALIGN128(width/2)*2*bitDepth/8*5;
        }

        if (productId == PRODUCT_ID_511 || productId == PRODUCT_ID_521) {
            Uint32 luma = 0, chroma = 0;
            if (codecMode == W_HEVC_DEC) {
                luma = (profile == HEVC_PROFILE_MAIN10) ? 7 : 5;
                chroma = (profile == HEVC_PROFILE_MAIN10) ? 4 : 3;
            }
            else if (codecMode == W_AVC_DEC) {
                luma = (profile == HEVC_PROFILE_MAIN10) ? 5 : 4;
                chroma = (profile == HEVC_PROFILE_MAIN10) ? 3 : 3;
            }
            lumaSize = VPU_ALIGN32(width)*luma;
            chromaSize = VPU_ALIGN32(width)*chroma;
        }
        offset     += (lumaSize + chromaSize);
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.vp.useBitEnable == TRUE) {
        sa->u.vp.bufBit = sa->bufBase + offset;
        if (codecMode == W_VP9_DEC) {
            size = VPU_ALIGN64(width)/64 * (70*8);
        }
        else if (codecMode == W_HEVC_DEC) {
            size = VPU_ALIGN128(VPU_ALIGN32(width)/32*9*8);
        }
        else if (codecMode == W_AVS2_DEC) {
            size = VPU_ALIGN16(width/16)*40;
        }
        else if (codecMode == W_SVAC_DEC) {
            size = 0;
        }
        if (productId == PRODUCT_ID_511 || productId == PRODUCT_ID_521)
            size = VPU_ALIGN64(width)*4;

        offset += size;
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.vp.useEncImdEnable == TRUE) {
         /* Main   profile(8bit) : Align32(picWidth)
          * Main10 profile(10bit): Align32(picWidth)
          */
        sa->u.vp.bufImd = sa->bufBase + offset;
        offset    += VPU_ALIGN32(width);
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.vp.useEncLfEnable == TRUE) {

        Uint32 luma, chroma;

        sa->u.vp.bufLf = sa->bufBase + offset;

        if (codecMode == W_AVC_ENC) {
            luma   = (profile == HEVC_PROFILE_MAIN10 ? 5 : 4);
            chroma = 3;
            lumaSize   = VPU_ALIGN16(width) * luma;
            chromaSize = VPU_ALIGN16(width) * chroma;
        }
        else {
            luma   = (profile == HEVC_PROFILE_MAIN10 ? 7 : 5);
            if (productId == PRODUCT_ID_521)
                chroma = (profile == HEVC_PROFILE_MAIN10 ? 4 : 3);
            else
                chroma = (profile == HEVC_PROFILE_MAIN10 ? 5 : 3);

            lumaSize   = VPU_ALIGN64(width) * luma;
            chromaSize = VPU_ALIGN64(width) * chroma;
        }

        offset    += lumaSize + chromaSize;

        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.vp.useEncRdoEnable == TRUE) {

        switch (productId) {
        case PRODUCT_ID_521:
            sa->u.vp.bufRdo = sa->bufBase + offset;
            if (codecMode == W_AVC_ENC) {
                offset += (VPU_ALIGN64(width)>>6)*384;
            }
            else { // HEVC ENC
                offset += (VPU_ALIGN64(width)>>6)*288;
            }
            break;
        default:
             /* Main   profile(8bit) : (Align64(picWidth)/64) * 336
             * Main10 profile(10bit): (Align64(picWidth)/64) * 336
             */
            sa->u.vp.bufRdo = sa->bufBase + offset;
            offset    += (VPU_ALIGN64(width)/64) * 336;
            break;
        }

        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    sa->bufSize = offset;

    return 1;
}

Int32 CalcStride(
    Uint32              width,
    Uint32              height,
    FrameBufferFormat   format,
    BOOL                cbcrInterleave,
    TiledMapType        mapType,
    BOOL                isVP9
    )
{
    Uint32  lumaStride   = 0;
    Uint32  chromaStride = 0;

    lumaStride = VPU_ALIGN32(width);

    if (height > width) {
        if ((mapType >= TILED_FRAME_V_MAP && mapType <= TILED_MIXED_V_MAP) ||
            mapType == TILED_FRAME_NO_BANK_MAP ||
            mapType == TILED_FIELD_NO_BANK_MAP)
            width = VPU_ALIGN16(height);	// TiledMap constraints
    }
    if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
        Uint32 twice = 0;

        twice = cbcrInterleave == TRUE ? 2 : 1;
        switch (format) {
        case FORMAT_420:
            /* nothing to do */
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
            lumaStride = VPU_ALIGN32(width)*2;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
            if ( isVP9 == TRUE ) {
                lumaStride   = VPU_ALIGN32(((width+11)/12)*16);
                chromaStride = (((width/2)+11)*twice/12)*16;
            }
            else {
                width = VPU_ALIGN32(width);
                lumaStride   = ((VPU_ALIGN16(width)+11)/12)*16;
                chromaStride = ((VPU_ALIGN16(width/2)+11)*twice/12)*16;
                if ( (chromaStride*2) > lumaStride)
                {
                    lumaStride = chromaStride * 2;
                    VLOG(ERR, "double chromaStride size is bigger than lumaStride\n");
                }
            }
            if (cbcrInterleave == TRUE) {
                lumaStride = MAX(lumaStride, chromaStride);
            }
            break;
        case FORMAT_422:
            /* nothing to do */
            break;
        case FORMAT_YUYV:       // 4:2:2 8bit packed
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            lumaStride = VPU_ALIGN32(width) * 2;
            break;
        case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
            lumaStride = VPU_ALIGN32(width) * 4;
            break;
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            lumaStride = VPU_ALIGN32(width*2)*2;
            break;
        default:
            break;
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        switch (format) {
        case FORMAT_420:
        case FORMAT_422:
        case FORMAT_YUYV:
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
        case FORMAT_YUYV_P10_16BIT_MSB:
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            lumaStride = VPU_ALIGN32(VPU_ALIGN16(width)*5)/4;
            lumaStride = VPU_ALIGN32(lumaStride);
            break;
        default:
            return -1;
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_8BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_10BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSY) {
        lumaStride = VPU_ALIGN32(width);
    }
    else if (mapType == TILED_FRAME_NO_BANK_MAP || mapType == TILED_FIELD_NO_BANK_MAP) {
        lumaStride = (width > 4096) ? 8192 :
                     (width > 2048) ? 4096 :
                     (width > 1024) ? 2048 :
                     (width >  512) ? 1024 : 512;
    }
    else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) {
        lumaStride = VPU_ALIGN32(width);
    }
    else {
        width = (width < height) ? height : width;

        lumaStride = (width > 4096) ? 8192 :
                     (width > 2048) ? 4096 :
                     (width > 1024) ? 2048 :
                     (width >  512) ? 1024 : 512;
    }

    return lumaStride;
}

// 32 bit / 16 bit ==> 32-n bit remainder, n bit quotient
static int fixDivRq(int a, int b, int n)
{
    Int64 c;
    Int64 a_36bit;
    Int64 mask, signBit, signExt;
    int  i;

    // DIVS emulation for BPU accumulator size
    // For SunOS build
    mask = 0x0F; mask <<= 32; mask |= 0x00FFFFFFFF; // mask = 0x0FFFFFFFFF;
    signBit = 0x08; signBit <<= 32;                 // signBit = 0x0800000000;
    signExt = 0xFFFFFFF0; signExt <<= 32;           // signExt = 0xFFFFFFF000000000;

    a_36bit = (Int64) a;

    for (i=0; i<n; i++) {
        c =  a_36bit - (b << 15);
        if (c >= 0)
            a_36bit = (c << 1) + 1;
        else
            a_36bit = a_36bit << 1;

        a_36bit = a_36bit & mask;
        if (a_36bit & signBit)
            a_36bit |= signExt;
    }

    a = (int) a_36bit;
    return a;                           // R = [31:n], Q = [n-1:0]
}

static int math_div(int number, int denom)
{
    int  c;
    c = fixDivRq(number, denom, 17);             // R = [31:17], Q = [16:0]
    c = c & 0xFFFF;
    c = (c + 1) >> 1;                   // round
    return (c & 0xFFFF);
}

int LevelCalculation(int MbNumX, int MbNumY, int frameRateInfo, int interlaceFlag, int BitRate, int SliceNum)
{
    int mbps;
    int frameRateDiv, frameRateRes, frameRate;
    int mbPicNum = (MbNumX*MbNumY);
    int mbFrmNum;
    int MaxSliceNum;

    int LevelIdc = 0;
    int i, maxMbs;

    if (interlaceFlag)    {
        mbFrmNum = mbPicNum*2;
        MbNumY   *=2;
    }
    else                mbFrmNum = mbPicNum;

    frameRateDiv = (frameRateInfo >> 16) + 1;
    frameRateRes  = frameRateInfo & 0xFFFF;
    frameRate = math_div(frameRateRes, frameRateDiv);
    mbps = mbFrmNum*frameRate;

    for(i=0; i<MAX_LAVEL_IDX; i++)
    {
        maxMbs = g_anLevelMaxMbs[i];
        if ( mbps <= g_anLevelMaxMBPS[i]
        && mbFrmNum <= g_anLevelMaxFS[i]
        && MbNumX   <= maxMbs
            && MbNumY   <= maxMbs
            && BitRate  <= g_anLevelMaxBR[i] )
        {
            LevelIdc = g_anLevel[i];
            break;
        }
    }

    if (i==MAX_LAVEL_IDX)
        i = MAX_LAVEL_IDX-1;

    if (SliceNum)
    {
        SliceNum =  math_div(mbPicNum, SliceNum);

        if (g_anLevelSliceRate[i])
        {
            MaxSliceNum = math_div( MAX( mbPicNum, g_anLevelMaxMBPS[i]/( 172/( 1+interlaceFlag ) )), g_anLevelSliceRate[i] );

            if ( SliceNum> MaxSliceNum)
                return -1;
        }
    }

    return LevelIdc;
}

Int32 CalcLumaSize(
    CodecInst*          inst,
    Int32           productId,
    Int32           stride,
    Int32           height,
    FrameBufferFormat format,
    BOOL            cbcrIntl,
    TiledMapType    mapType,
    DRAMConfig      *pDramCfg
    )
{
    Int32 unit_size_hor_lum, unit_size_ver_lum, size_dpb_lum, field_map, size_dpb_lum_4k;
    UNREFERENCED_PARAMETER(cbcrIntl);
    VLOG(INFO, "CalcLumaSize stride %d height %d, mapType %d format %d\n",
        stride, height, mapType, format);

    if (mapType == TILED_FIELD_V_MAP || mapType == TILED_FIELD_NO_BANK_MAP || mapType == LINEAR_FIELD_MAP) {
        field_map = 1;
    }
    else {
        field_map = 0;
    }

    if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
        size_dpb_lum = stride * height;
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        size_dpb_lum = stride * height;
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_10BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_10BIT) {
        size_dpb_lum = VP5_ENC_FBC50_LOSSLESS_LUMA_10BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_8BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_8BIT) {
        size_dpb_lum = VP5_ENC_FBC50_LOSSLESS_LUMA_8BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSY || mapType == COMPRESSED_FRAME_MAP_V50_LOSSY_422) {
        if (pDramCfg == NULL)
            return 0;
        size_dpb_lum = VP5_ENC_FBC50_LOSSY_LUMA_FRAME_SIZE(stride, height, pDramCfg->tx16y);
    }
    else if (mapType == TILED_FRAME_NO_BANK_MAP || mapType == TILED_FIELD_NO_BANK_MAP) {
        unit_size_hor_lum = stride;
        unit_size_ver_lum = (((height>>field_map)+127)/128) * 128; // unit vertical size is 128 pixel (8MB)
        size_dpb_lum      = unit_size_hor_lum * (unit_size_ver_lum<<field_map);
    }
    else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) {
        if (productId == PRODUCT_ID_960) {
            size_dpb_lum   = stride * height;

            // aligned to 8192*2 (0x4000) for top/bot field
            // use upper 20bit address only
            size_dpb_lum_4k =  ((size_dpb_lum + 16383)/16384)*16384;

            if (mapType == TILED_FIELD_MB_RASTER_MAP) {
                size_dpb_lum_4k = ((size_dpb_lum_4k+(0x8000-1))&~(0x8000-1));
            }

            size_dpb_lum = size_dpb_lum_4k;
        }
        else {
            size_dpb_lum    = stride * (height>>field_map);
            size_dpb_lum_4k = ((size_dpb_lum+(16384-1))/16384)*16384;
            size_dpb_lum    = size_dpb_lum_4k<<field_map;
        }
    }
    else {
        if (productId == PRODUCT_ID_960) {
            Int32    VerSizePerRas,Ras1DBit;
            Int32    ChrSizeYField;
            Int32    ChrFieldRasSize,ChrFrameRasSize,LumFieldRasSize,LumFrameRasSize;

            ChrSizeYField = ((height/2)+1)>>1;

            if (pDramCfg == NULL)
                return 0;
            if (mapType == TILED_FRAME_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 16) {  // BITMAIN setting
                    VerSizePerRas = 64; // Tile (16x2)*(4*2)
                    Ras1DBit = 1;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 4 && pDramCfg->rasBit == 15) {  // BITMAIN setting
                    VerSizePerRas = 128; // Tile (8x4)*(8x2)
                    Ras1DBit = 1;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FRAME_H_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FIELD_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 16) {  // BITMAIN setting
                    VerSizePerRas = 64; // Tile (16x2)*(4*2)
                    Ras1DBit = 1;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 4 && pDramCfg->rasBit == 15) {  // BITMAIN setting
                    VerSizePerRas = 128; // Tile (8x4)*(8x2)
                    Ras1DBit = 1;
                }
                else
                    return 0;
            }
            else {         // TILED_FIELD_H_MAP
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;
            }
            ChrFieldRasSize = ((ChrSizeYField + (VerSizePerRas-1))/VerSizePerRas) << Ras1DBit;
            ChrFrameRasSize = ChrFieldRasSize *2;
            LumFieldRasSize = ChrFrameRasSize;
            LumFrameRasSize = LumFieldRasSize *2;
            size_dpb_lum    = LumFrameRasSize << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
        }
        else {  // productId != 960
            unit_size_hor_lum = stride;
            unit_size_ver_lum = (((height>>field_map)+63)/64) * 64; // unit vertical size is 64 pixel (4MB)
            size_dpb_lum      = unit_size_hor_lum * (unit_size_ver_lum<<field_map);
        }
    }
    VLOG(INFO, "CalcLumaSize size %d \n", size_dpb_lum);
    return size_dpb_lum;
}

Int32 CalcChromaSize(
    CodecInst*          inst,
    Int32               productId,
    Int32               stride,
    Int32               height,
    FrameBufferFormat   format,
    BOOL                cbcrIntl,
    TiledMapType        mapType,
    DRAMConfig*         pDramCfg
    )
{
    Int32 chr_size_y, chr_size_x;
    Int32 chr_vscale, chr_hscale;
    Int32 unit_size_hor_chr, unit_size_ver_chr;
    Int32 size_dpb_chr, size_dpb_chr_4k;
    Int32 field_map;

    unit_size_hor_chr = 0;
    unit_size_ver_chr = 0;

    chr_hscale = 1;
    chr_vscale = 1;
    VLOG(INFO, "CalcChromaSize stride %d height %d, mapType %d format %d\n",
        stride, height, mapType, format);
    switch (format) {
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_420:
        chr_hscale = 2;
        chr_vscale = 2;
        break;
    case FORMAT_224:
        chr_vscale = 2;
        break;
    case FORMAT_422:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        chr_hscale = 2;
        break;
    case FORMAT_444:
    case FORMAT_400:
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
    case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        break;
    default:
        return 0;
    }

    if (mapType == TILED_FIELD_V_MAP || mapType == TILED_FIELD_NO_BANK_MAP || mapType == LINEAR_FIELD_MAP) {
        field_map = 1;
    }
    else {
        field_map = 0;
    }

    if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
        switch (format) {
        case FORMAT_420:
            unit_size_hor_chr = stride/2;
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
            unit_size_hor_chr = stride/2;
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
            unit_size_hor_chr = VPU_ALIGN16(stride/2);
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_422:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
            unit_size_hor_chr = VPU_ALIGN32(stride/2);
            unit_size_ver_chr = height;
            break;
        case FORMAT_YUYV:
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
        case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            unit_size_hor_chr = 0;
            unit_size_ver_chr = 0;
            break;
        default:
            break;
        }
        size_dpb_chr = (format == FORMAT_400) ? 0 : unit_size_ver_chr * unit_size_hor_chr;
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        switch (format) {
        case FORMAT_420:
        case FORMAT_YUYV:       // 4:2:2 8bit packed
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            size_dpb_chr = VPU_ALIGN16(stride/2)*height;
            break;
        default:
            /* 10bit */
            stride = VPU_ALIGN64(stride/2)+12; /* FIXME: need width information */
            size_dpb_chr = VPU_ALIGN32(stride)*VPU_ALIGN4(height);
            break;
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_10BIT) {
        size_dpb_chr = VP5_ENC_FBC50_LOSSLESS_CHROMA_10BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_8BIT) {
        size_dpb_chr = VP5_ENC_FBC50_LOSSLESS_CHROMA_8BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSY) {
        if (pDramCfg == NULL)
            return 0;
        size_dpb_chr = VP5_ENC_FBC50_LOSSY_CHROMA_FRAME_SIZE(stride, height, pDramCfg->tx16c);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_10BIT) {
        size_dpb_chr = VP5_ENC_FBC50_LOSSLESS_422_CHROMA_10BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_8BIT) {
        size_dpb_chr = VP5_ENC_FBC50_LOSSLESS_422_CHROMA_8BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSY_422) {
        if (pDramCfg == NULL)
            return 0;
        size_dpb_chr = VP5_ENC_FBC50_LOSSY_422_CHROMA_FRAME_SIZE(stride, height, pDramCfg->tx16c);
    }

    else if (mapType == TILED_FRAME_NO_BANK_MAP || mapType == TILED_FIELD_NO_BANK_MAP) {
        chr_size_y = (height>>field_map)/chr_hscale;
        chr_size_x = stride/chr_vscale;

        unit_size_hor_chr = (chr_size_x > 4096) ? 8192:
                            (chr_size_x > 2048) ? 4096 :
                            (chr_size_x > 1024) ? 2048 :
                            (chr_size_x >  512) ? 1024 : 512;
        unit_size_ver_chr = ((chr_size_y+127)/128) * 128; // unit vertical size is 128 pixel (8MB)

        size_dpb_chr = (format==FORMAT_400) ? 0 : (unit_size_hor_chr * (unit_size_ver_chr<<field_map));
    }
    else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) {
        if (productId == PRODUCT_ID_960) {
            chr_size_x = stride/chr_hscale;
            chr_size_y = height/chr_hscale;
            size_dpb_chr   = chr_size_y * chr_size_x;

            // aligned to 8192*2 (0x4000) for top/bot field
            // use upper 20bit address only
            size_dpb_chr_4k	=  ((size_dpb_chr + 16383)/16384)*16384;

            if (mapType == TILED_FIELD_MB_RASTER_MAP) {
                size_dpb_chr_4k = ((size_dpb_chr_4k+(0x8000-1))&~(0x8000-1));
            }

            size_dpb_chr = size_dpb_chr_4k;
        }
        else {
            size_dpb_chr    = (format==FORMAT_400) ? 0 : ((stride * (height>>field_map))/(chr_hscale*chr_vscale));
            size_dpb_chr_4k = ((size_dpb_chr+(16384-1))/16384)*16384;
            size_dpb_chr    = size_dpb_chr_4k<<field_map;
        }
    }
    else {
        if (productId == PRODUCT_ID_960) {
            int  VerSizePerRas,Ras1DBit;
            int  ChrSizeYField;
            int  divY;
            int  ChrFieldRasSize,ChrFrameRasSize;

            divY = format == FORMAT_420 || format == FORMAT_224 ? 2 : 1;

            ChrSizeYField = ((height/divY)+1)>>1;
            if (pDramCfg == NULL)
                return 0;
            if (mapType == TILED_FRAME_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 16) {  // BITMAIN setting
                    VerSizePerRas = 64; // Tile (16x2)*(4*2)
                    Ras1DBit = 1;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 4 && pDramCfg->rasBit == 15) {  // BITMAIN setting
                    VerSizePerRas = 128; // Tile (8x4)*(8x2)
                    Ras1DBit = 1;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FRAME_H_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FIELD_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 16) {  // BITMAIN setting
                    VerSizePerRas = 64; // Tile (16x2)*(4*2)
                    Ras1DBit = 1;
                }
                else if (pDramCfg->casBit == 10 && pDramCfg->bankBit == 4 && pDramCfg->rasBit == 15) {  // BITMAIN setting
                    VerSizePerRas = 128; // Tile (8x4)*(8x2)
                    Ras1DBit = 1;
                }
                else
                    return 0;
            } else {         // TILED_FIELD_H_MAP
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;
            }
            ChrFieldRasSize = ((ChrSizeYField + (VerSizePerRas-1))/VerSizePerRas) << Ras1DBit;
            ChrFrameRasSize = ChrFieldRasSize *2;
            size_dpb_chr = (ChrFrameRasSize << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit)) / 2;      // divide 2  = to calucate one Cb(or Cr) size;

        }
        else {  // productId != 960
            chr_size_y = (height>>field_map)/chr_hscale;
            chr_size_x = cbcrIntl == TRUE ? stride : stride/chr_vscale;

            unit_size_hor_chr = (chr_size_x> 4096) ? 8192:
                                (chr_size_x> 2048) ? 4096 :
                                (chr_size_x > 1024) ? 2048 :
                                (chr_size_x >  512) ? 1024 : 512;
            unit_size_ver_chr = ((chr_size_y+63)/64) * 64; // unit vertical size is 64 pixel (4MB)

            size_dpb_chr  = (format==FORMAT_400) ? 0 : unit_size_hor_chr * (unit_size_ver_chr<<field_map);
            size_dpb_chr /= (cbcrIntl == TRUE ? 2 : 1);
        }

    }
    VLOG(INFO, "CalcChromaSize size %d \n", size_dpb_chr);
    return size_dpb_chr;
}

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
#include <string.h>
#include <pthread.h>
typedef struct  {
	int core_idx;
	pthread_t thread_id;
	int sw_uart_thread_run;
} SwUartContext;
static SwUartContext s_SwUartContext;
void SwUartHandler(void *context)
{
	unsigned int regSwUartStatus;
	unsigned int regSwUartTxData;
	unsigned char *strRegSwUartTxData;
	int i = 0;
	char uartTx[1024];

	osal_memset(uartTx, 0, sizeof(char)*1024);
	while(s_SwUartContext.sw_uart_thread_run == 1)
	{
		regSwUartStatus = vdi_read_register(s_SwUartContext.core_idx, VP5_SW_UART_STATUS);
		if (regSwUartStatus == (unsigned int)-1)
		{
			continue;
		}

		if ((regSwUartStatus & (1<<1)))
		{
			regSwUartTxData = vdi_read_register(s_SwUartContext.core_idx, VP5_SW_UART_TX_DATA);
			if (regSwUartTxData == (unsigned int)-1)
			{
				continue;
			}
			regSwUartStatus &= ~(1<<1);
			vdi_write_register(s_SwUartContext.core_idx, VP5_SW_UART_STATUS, regSwUartStatus);
			strRegSwUartTxData = (unsigned char *)&regSwUartTxData;
			for (i=0; i < 4; i++)
			{
				if (strRegSwUartTxData[i] == '\n')
				{
					VLOG(TRACE, "%s \n", uartTx);
					osal_memset(uartTx, 0, sizeof(unsigned char)*1024);
				}
				else
				{
					strncat((char *)uartTx, (const char *)(strRegSwUartTxData + i), 1);
				}
			}
		}

	}
	VLOG(1, "exit SwUartHandler\n");

}

int create_sw_uart_thread(unsigned long coreIdx)
{
	int ret;

	if (s_SwUartContext.sw_uart_thread_run == 1)
		return 1;

	vdi_write_register(coreIdx, VP5_SW_UART_STATUS,  (1<<0)); // enable SW UART. this will be checked by firmware to know SW UART enabled

	s_SwUartContext.core_idx = coreIdx;
	s_SwUartContext.sw_uart_thread_run = 1;
	ret = pthread_create(&s_SwUartContext.thread_id, NULL, (void*)SwUartHandler, &s_SwUartContext);

	if (ret != 0)
	{
		destory_sw_uart_thread();
		return 0;
	}

	return 1;
}

void destory_sw_uart_thread(unsigned long coreIdx)
{
	int inst_num;
	int task_num;
	int i;

	inst_num = 0;
	for (i=0; i < MAX_NUM_VPU_CORE; i++)
	{
		inst_num = inst_num + vdi_get_instance_num(i);
	}

	if (inst_num > 0)
		return;

	task_num = 0;
	for (i=0; i < MAX_NUM_VPU_CORE; i++)
	{
		task_num = task_num + vdi_get_task_num(i);
	}

	if (task_num > 1)
		return;

	if (s_SwUartContext.sw_uart_thread_run == 1)
	{
		s_SwUartContext.sw_uart_thread_run = 0;

		vdi_write_register(coreIdx, VP5_SW_UART_STATUS, 0); // disable SW UART. this will be checked by firmware to know SW UART enabled

		if (s_SwUartContext.thread_id)
		{
			pthread_join(s_SwUartContext.thread_id, NULL);
			s_SwUartContext.thread_id = 0;
		}
	}

	return ;
}
#endif
