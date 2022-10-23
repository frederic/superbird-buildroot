
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
#include <stdio.h>
#include <string.h>
#include "main_helper.h"

#ifdef PLATFORM_WIN32
#pragma warning(disable : 4996)     //!<< disable waring C4996: The POSIX name for this item is deprecated. 
#endif

#ifndef min
#define min(a,b)       (((a) < (b)) ? (a) : (b))
#endif
/*******************************************************************************
 * REPORT                                                                      *
 *******************************************************************************/
#define USER_DATA_INFO_OFFSET       (8*17)
#define FN_PIC_INFO             "dec_pic_disp_info.log"
#define FN_SEQ_INFO             "dec_seq_disp_info.log"
#define FN_PIC_TYPE             "dec_pic_type.log"
#define FN_USER_DATA            "dec_user_data.log"
#define FN_SEQ_USER_DATA        "dec_seq_user_data.log"

// VC1 specific
enum {
    BDU_SEQUENCE_END               = 0x0A,
    BDU_SLICE                      = 0x0B,
    BDU_FIELD                      = 0x0C,
    BDU_FRAME                      = 0x0D,
    BDU_ENTRYPOINT_HEADER          = 0x0E,
    BDU_SEQUENCE_HEADER            = 0x0F,
    BDU_SLICE_LEVEL_USER_DATA      = 0x1B,
    BDU_FIELD_LEVEL_USER_DATA      = 0x1C,
    BDU_FRAME_LEVEL_USER_DATA      = 0x1D,
    BDU_ENTRYPOINT_LEVEL_USER_DATA = 0x1E,
    BDU_SEQUENCE_LEVEL_USER_DATA   = 0x1F
};

// AVC specific - SEI
enum {
    SEI_REGISTERED_ITUTT35_USERDATA = 0x04,
    SEI_UNREGISTERED_USERDATA       = 0x05,
    SEI_MVC_SCALABLE_NESTING        = 0x25
};

int setVpEncOpenParam(EncOpenParam *pEncOP, TestEncConfig *pEncConfig, ENC_CFG *pCfg)
{
    Int32   i = 0;
    Int32   srcWidth;
    Int32   srcHeight;
    Int32   outputNum;
    Int32   bitrate;
    Int32   bitrateBL;

    EncVpParam *param = &pEncOP->EncStdParam.vpParam;

    srcWidth  = (pEncConfig->picWidth > 0)  ? pEncConfig->picWidth  : pCfg->vpCfg.picX;
    srcHeight = (pEncConfig->picHeight > 0) ? pEncConfig->picHeight : pCfg->vpCfg.picY;
    if(pCfg->vpCfg.enStillPicture) {
        outputNum   = 1;
    }
    else {
        if ( pEncConfig->outNum != 0 ) {
            outputNum = pEncConfig->outNum;
        } 
        else {
            outputNum = pCfg->NumFrame;
        }
    }
    bitrate   = (pEncConfig->kbps > 0)      ? pEncConfig->kbps*1024 : pCfg->RcBitRate;
    bitrateBL = (pEncConfig->kbps > 0)      ? pEncConfig->kbps*1024 : pCfg->RcBitRateBL;

    pEncConfig->outNum      = outputNum;
    pEncOP->picWidth        = srcWidth;
    pEncOP->picHeight       = srcHeight;
    pEncOP->frameRateInfo   = pCfg->vpCfg.frameRate;

    param->level            = 0;
    param->tier             = 0;
    pEncOP->srcBitDepth     = pCfg->SrcBitDepth;

    if (pCfg->vpCfg.internalBitDepth == 0)
        param->internalBitDepth = pCfg->SrcBitDepth;
    else
        param->internalBitDepth = pCfg->vpCfg.internalBitDepth;

    if ( param->internalBitDepth == 10 )
        pEncOP->outputFormat  = FORMAT_420_P10_16BIT_MSB;
    if ( param->internalBitDepth == 8 )
        pEncOP->outputFormat  = FORMAT_420;

    if (param->internalBitDepth > 8)
        param->profile   = HEVC_PROFILE_MAIN10;
    else
        param->profile   = HEVC_PROFILE_MAIN;

    if (pCfg->vpCfg.enStillPicture) {
        if (param->internalBitDepth > 8)
            param->profile   = HEVC_PROFILE_MAIN10_STILLPICTURE;
        else
            param->profile   = HEVC_PROFILE_STILLPICTURE;
        param->enStillPicture = 1;
    }

    param->losslessEnable   = pCfg->vpCfg.losslessEnable;
    param->constIntraPredFlag = pCfg->vpCfg.constIntraPredFlag;

    if (pCfg->vpCfg.useAsLongtermPeriod > 0 || pCfg->vpCfg.refLongtermPeriod > 0)
        param->useLongTerm = 1;
    else
        param->useLongTerm = 0;

    /* for CMD_ENC_SEQ_GOP_PARAM */
    param->gopPresetIdx     = pCfg->vpCfg.gopPresetIdx;

    /* for CMD_ENC_SEQ_INTRA_PARAM */
    param->decodingRefreshType = pCfg->vpCfg.decodingRefreshType;
    param->intraPeriod      = pCfg->vpCfg.intraPeriod;
    param->intraQP          = pCfg->vpCfg.intraQP;
        param->forcedIdrHeaderEnable = pCfg->vpCfg.forcedIdrHeaderEnable;

    /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
    param->confWinTop    = pCfg->vpCfg.confWinTop;
    param->confWinBot    = pCfg->vpCfg.confWinBot;
    param->confWinLeft   = pCfg->vpCfg.confWinLeft;
    param->confWinRight  = pCfg->vpCfg.confWinRight;

    if ((pEncOP->picHeight % 16) && param->confWinBot == 0
        && pEncOP->bitstreamFormat == STD_AVC) {
        /*  need set the drop flag */
       if (pEncConfig->rotAngle != 90 && pEncConfig->rotAngle != 270) // except rotation
       {
                param->confWinBot = 16 - (pEncOP->picHeight % 16);
       }
    }

    /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
    param->independSliceMode     = pCfg->vpCfg.independSliceMode;
    param->independSliceModeArg  = pCfg->vpCfg.independSliceModeArg;

    /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
    param->dependSliceMode     = pCfg->vpCfg.dependSliceMode;
    param->dependSliceModeArg  = pCfg->vpCfg.dependSliceModeArg;

    /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
    param->intraRefreshMode     = pCfg->vpCfg.intraRefreshMode;
    param->intraRefreshArg      = pCfg->vpCfg.intraRefreshArg;
    param->useRecommendEncParam = pCfg->vpCfg.useRecommendEncParam;

    /* for CMD_ENC_PARAM */
    param->scalingListEnable        = pCfg->vpCfg.scalingListEnable;
    param->cuSizeMode               = 0x7; // always set cu8x8/16x16/32x32 enable to 1.
    param->tmvpEnable               = pCfg->vpCfg.tmvpEnable;
    param->wppEnable                = pCfg->vpCfg.wppenable;
    param->maxNumMerge              = pCfg->vpCfg.maxNumMerge;

    param->disableDeblk             = pCfg->vpCfg.disableDeblk;

    param->lfCrossSliceBoundaryEnable   = pCfg->vpCfg.lfCrossSliceBoundaryEnable;
    param->betaOffsetDiv2           = pCfg->vpCfg.betaOffsetDiv2;
    param->tcOffsetDiv2             = pCfg->vpCfg.tcOffsetDiv2;
    param->skipIntraTrans           = pCfg->vpCfg.skipIntraTrans;
    param->saoEnable                = pCfg->vpCfg.saoEnable;
    param->intraNxNEnable           = pCfg->vpCfg.intraNxNEnable;

    /* for CMD_ENC_RC_PARAM */
    pEncOP->rcEnable             = pCfg->RcEnable;
    pEncOP->vbvBufferSize        = pCfg->VbvBufferSize;
    param->cuLevelRCEnable       = pCfg->vpCfg.cuLevelRCEnable;
    param->hvsQPEnable           = pCfg->vpCfg.hvsQPEnable;
    param->hvsQpScale            = pCfg->vpCfg.hvsQpScale;

    param->bitAllocMode          = pCfg->vpCfg.bitAllocMode;
    for (i = 0; i < MAX_GOP_NUM; i++) {
        param->fixedBitRatio[i] = pCfg->vpCfg.fixedBitRatio[i];
    }

    param->minQpI           = pCfg->vpCfg.minQp;
    param->minQpP           = pCfg->vpCfg.minQp;
    param->minQpB           = pCfg->vpCfg.minQp;
    param->maxQpI           = pCfg->vpCfg.maxQp;
    param->maxQpP           = pCfg->vpCfg.maxQp;
    param->maxQpB           = pCfg->vpCfg.maxQp;

    param->hvsMaxDeltaQp    = pCfg->vpCfg.maxDeltaQp;
    pEncOP->bitRate          = bitrate;
    pEncOP->bitRateBL        = bitrateBL;

    /* for CMD_ENC_CUSTOM_GOP_PARAM */
    param->gopParam.customGopSize     = pCfg->vpCfg.gopParam.customGopSize;

    for (i= 0; i<param->gopParam.customGopSize; i++) {
        param->gopParam.picParam[i].picType      = pCfg->vpCfg.gopParam.picParam[i].picType;
        param->gopParam.picParam[i].pocOffset    = pCfg->vpCfg.gopParam.picParam[i].pocOffset;
        param->gopParam.picParam[i].picQp        = pCfg->vpCfg.gopParam.picParam[i].picQp;
        param->gopParam.picParam[i].refPocL0     = pCfg->vpCfg.gopParam.picParam[i].refPocL0;
        param->gopParam.picParam[i].refPocL1     = pCfg->vpCfg.gopParam.picParam[i].refPocL1;
        param->gopParam.picParam[i].temporalId   = pCfg->vpCfg.gopParam.picParam[i].temporalId;
        param->gopParam.picParam[i].numRefPicL0  = pCfg->vpCfg.gopParam.picParam[i].numRefPicL0;
    }

    param->roiEnable = pCfg->vpCfg.roiEnable;
    // VPS & VUI

    param->numUnitsInTick       = pCfg->vpCfg.numUnitsInTick;
    param->timeScale            = pCfg->vpCfg.timeScale;
    param->numTicksPocDiffOne   = pCfg->vpCfg.numTicksPocDiffOne;
    param->chromaCbQpOffset = pCfg->vpCfg.chromaCbQpOffset;
    param->chromaCrQpOffset = pCfg->vpCfg.chromaCrQpOffset;
    param->initialRcQp      = pCfg->vpCfg.initialRcQp;

    param->nrYEnable        = pCfg->vpCfg.nrYEnable;
    param->nrCbEnable       = pCfg->vpCfg.nrCbEnable;
    param->nrCrEnable       = pCfg->vpCfg.nrCrEnable;
    param->nrNoiseEstEnable = pCfg->vpCfg.nrNoiseEstEnable;
    param->nrNoiseSigmaY    = pCfg->vpCfg.nrNoiseSigmaY;
    param->nrNoiseSigmaCb   = pCfg->vpCfg.nrNoiseSigmaCb;
    param->nrNoiseSigmaCr   = pCfg->vpCfg.nrNoiseSigmaCr;
    param->nrIntraWeightY   = pCfg->vpCfg.nrIntraWeightY;
    param->nrIntraWeightCb  = pCfg->vpCfg.nrIntraWeightCb;
    param->nrIntraWeightCr  = pCfg->vpCfg.nrIntraWeightCr;
    param->nrInterWeightY   = pCfg->vpCfg.nrInterWeightY;
    param->nrInterWeightCb  = pCfg->vpCfg.nrInterWeightCb;
    param->nrInterWeightCr  = pCfg->vpCfg.nrInterWeightCr;

    param->monochromeEnable            = pCfg->vpCfg.monochromeEnable;
    param->strongIntraSmoothEnable     = pCfg->vpCfg.strongIntraSmoothEnable;
    param->weightPredEnable            = pCfg->vpCfg.weightPredEnable;
    param->bgDetectEnable              = pCfg->vpCfg.bgDetectEnable;
    param->bgThrDiff                   = pCfg->vpCfg.bgThrDiff;
    param->bgThrMeanDiff               = pCfg->vpCfg.bgThrMeanDiff;
    param->bgLambdaQp                  = pCfg->vpCfg.bgLambdaQp;
    param->bgDeltaQp                   = pCfg->vpCfg.bgDeltaQp;
    param->customLambdaEnable          = pCfg->vpCfg.customLambdaEnable;
    param->customMDEnable              = pCfg->vpCfg.customMDEnable;
    param->pu04DeltaRate               = pCfg->vpCfg.pu04DeltaRate;
    param->pu08DeltaRate               = pCfg->vpCfg.pu08DeltaRate;
    param->pu16DeltaRate               = pCfg->vpCfg.pu16DeltaRate;
    param->pu32DeltaRate               = pCfg->vpCfg.pu32DeltaRate;
    param->pu04IntraPlanarDeltaRate    = pCfg->vpCfg.pu04IntraPlanarDeltaRate;
    param->pu04IntraDcDeltaRate        = pCfg->vpCfg.pu04IntraDcDeltaRate;
    param->pu04IntraAngleDeltaRate     = pCfg->vpCfg.pu04IntraAngleDeltaRate;
    param->pu08IntraPlanarDeltaRate    = pCfg->vpCfg.pu08IntraPlanarDeltaRate;
    param->pu08IntraDcDeltaRate        = pCfg->vpCfg.pu08IntraDcDeltaRate;
    param->pu08IntraAngleDeltaRate     = pCfg->vpCfg.pu08IntraAngleDeltaRate;
    param->pu16IntraPlanarDeltaRate    = pCfg->vpCfg.pu16IntraPlanarDeltaRate;
    param->pu16IntraDcDeltaRate        = pCfg->vpCfg.pu16IntraDcDeltaRate;
    param->pu16IntraAngleDeltaRate     = pCfg->vpCfg.pu16IntraAngleDeltaRate;
    param->pu32IntraPlanarDeltaRate    = pCfg->vpCfg.pu32IntraPlanarDeltaRate;
    param->pu32IntraDcDeltaRate        = pCfg->vpCfg.pu32IntraDcDeltaRate;
    param->pu32IntraAngleDeltaRate     = pCfg->vpCfg.pu32IntraAngleDeltaRate;
    param->cu08IntraDeltaRate          = pCfg->vpCfg.cu08IntraDeltaRate;
    param->cu08InterDeltaRate          = pCfg->vpCfg.cu08InterDeltaRate;
    param->cu08MergeDeltaRate          = pCfg->vpCfg.cu08MergeDeltaRate;
    param->cu16IntraDeltaRate          = pCfg->vpCfg.cu16IntraDeltaRate;
    param->cu16InterDeltaRate          = pCfg->vpCfg.cu16InterDeltaRate;
    param->cu16MergeDeltaRate          = pCfg->vpCfg.cu16MergeDeltaRate;
    param->cu32IntraDeltaRate          = pCfg->vpCfg.cu32IntraDeltaRate;
    param->cu32InterDeltaRate          = pCfg->vpCfg.cu32InterDeltaRate;
    param->cu32MergeDeltaRate          = pCfg->vpCfg.cu32MergeDeltaRate;
    param->coefClearDisable            = pCfg->vpCfg.coefClearDisable;


    param->rcWeightParam                = pCfg->vpCfg.rcWeightParam;
    param->rcWeightBuf                  = pCfg->vpCfg.rcWeightBuf;

    param->s2fmeDisable                = pCfg->vpCfg.s2fmeDisable;
    // for H.264 on VP
    param->avcIdrPeriod         = pCfg->vpCfg.idrPeriod;
    param->rdoSkip              = pCfg->vpCfg.rdoSkip;
    param->lambdaScalingEnable  = pCfg->vpCfg.lambdaScalingEnable;
    param->transform8x8Enable   = pCfg->vpCfg.transform8x8;
    param->avcSliceMode         = pCfg->vpCfg.avcSliceMode;
    param->avcSliceArg          = pCfg->vpCfg.avcSliceArg;
    param->intraMbRefreshMode   = pCfg->vpCfg.intraMbRefreshMode;
    param->intraMbRefreshArg    = pCfg->vpCfg.intraMbRefreshArg;
    param->mbLevelRcEnable      = pCfg->vpCfg.mbLevelRc;
    param->entropyCodingMode    = pCfg->vpCfg.entropyCodingMode;;

    return 1;
}


/******************************************************************************
EncOpenParam Initialization
******************************************************************************/
/**
* To init EncOpenParam by runtime evaluation
* IN
*   EncConfigParam *pEncConfig
* OUT
*   EncOpenParam *pEncOP
*/
#define DEFAULT_ENC_OUTPUT_NUM      30
Int32 GetEncOpenParamDefault(EncOpenParam *pEncOP, TestEncConfig *pEncConfig)
{
    int bitFormat;
    Int32   productId;
    productId                       = VPU_GetProductId(pEncOP->coreIdx);
    pEncConfig->outNum              = pEncConfig->outNum == 0 ? DEFAULT_ENC_OUTPUT_NUM : pEncConfig->outNum;
    bitFormat                       = pEncOP->bitstreamFormat;

    pEncOP->picWidth                = pEncConfig->picWidth;
    pEncOP->picHeight               = pEncConfig->picHeight;
    pEncOP->frameRateInfo           = 30;
    pEncOP->MESearchRange           = 3;
    pEncOP->bitRate                 = pEncConfig->kbps;
    pEncOP->rcInitDelay             = 0;
    pEncOP->vbvBufferSize           = 0;        // 0 = ignore
    pEncOP->meBlkMode               = 0;        // for compare with C-model ( C-model = only 0 )
    pEncOP->frameSkipDisable        = 1;        // for compare with C-model ( C-model = only 1 )
    pEncOP->gopSize                 = 30;       // only first picture is I
    pEncOP->sliceMode.sliceMode     = 1;        // 1 slice per picture
    pEncOP->sliceMode.sliceSizeMode = 1;
    pEncOP->sliceMode.sliceSize     = 115;
    pEncOP->intraRefreshNum         = 0;
    pEncOP->rcIntraQp               = -1;       // disable == -1
    pEncOP->userQpMax               = -1;				// disable == -1
    pEncOP->userGamma               = (Uint32)(0.75*32768);   //  (0*32768 < gamma < 1*32768)
    pEncOP->rcIntervalMode          = 1;                        // 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level
    pEncOP->mbInterval              = 0;
    pEncConfig->picQpY              = 23;

    if (bitFormat == STD_MPEG4)
        pEncOP->MEUseZeroPmv = 1;            
    else
        pEncOP->MEUseZeroPmv = 0;            

    pEncOP->intraCostWeight = 400;

    // Standard specific
    if( bitFormat == STD_MPEG4 ) {
        pEncOP->EncStdParam.mp4Param.mp4DataPartitionEnable = 0;
        pEncOP->EncStdParam.mp4Param.mp4ReversibleVlcEnable = 0;
        pEncOP->EncStdParam.mp4Param.mp4IntraDcVlcThr = 0;
        pEncOP->EncStdParam.mp4Param.mp4HecEnable	= 0;
        pEncOP->EncStdParam.mp4Param.mp4Verid = 2;		
    }
    else if( bitFormat == STD_H263 ) {
        pEncOP->EncStdParam.h263Param.h263AnnexIEnable = 0;
        pEncOP->EncStdParam.h263Param.h263AnnexJEnable = 0;
        pEncOP->EncStdParam.h263Param.h263AnnexKEnable = 0;
        pEncOP->EncStdParam.h263Param.h263AnnexTEnable = 0;		
    }
    else if( bitFormat == STD_AVC && productId != PRODUCT_ID_521) {
		// AVC
        pEncOP->EncStdParam.avcParam.constrainedIntraPredFlag = 0;
        pEncOP->EncStdParam.avcParam.disableDeblk = 1;
        pEncOP->EncStdParam.avcParam.deblkFilterOffsetAlpha = 6;
        pEncOP->EncStdParam.avcParam.deblkFilterOffsetBeta = 0;
        pEncOP->EncStdParam.avcParam.chromaQpOffset = 10;
        pEncOP->EncStdParam.avcParam.audEnable = 0;
        pEncOP->EncStdParam.avcParam.frameCroppingFlag = 0;
        pEncOP->EncStdParam.avcParam.frameCropLeft = 0;
        pEncOP->EncStdParam.avcParam.frameCropRight = 0;
        pEncOP->EncStdParam.avcParam.frameCropTop = 0;
        pEncOP->EncStdParam.avcParam.frameCropBottom = 0;
        pEncOP->EncStdParam.avcParam.level = 0;

        // Update cropping information : Usage example for H.264 frame_cropping_flag
        if (pEncOP->picHeight == 1080) 
        {
            // In case of AVC encoder, when we want to use unaligned display width(For example, 1080),
            // frameCroppingFlag parameters should be adjusted to displayable rectangle
            if (pEncConfig->rotAngle != 90 && pEncConfig->rotAngle != 270) // except rotation
            {        
                if (pEncOP->EncStdParam.avcParam.frameCroppingFlag == 0) 
                {
                    pEncOP->EncStdParam.avcParam.frameCroppingFlag = 1;
                    // frameCropBottomOffset = picHeight(MB-aligned) - displayable rectangle height
                    pEncOP->EncStdParam.avcParam.frameCropBottom = 8;
                }
            }
        }
    }
    else if( bitFormat == STD_HEVC || (bitFormat == STD_AVC && productId == PRODUCT_ID_521)) {
        EncVpParam *param = &pEncOP->EncStdParam.vpParam;
        Int32   rcBitrate   = pEncConfig->kbps * 1000;
        Int32   i=0;

        pEncOP->bitRate         = rcBitrate;
        param->profile          = HEVC_PROFILE_MAIN;

        param->level            = 0;
        param->tier             = 0;
        param->internalBitDepth = 8;
        pEncOP->srcBitDepth     = 8;
        param->losslessEnable   = 0;
        param->constIntraPredFlag = 0;
        param->useLongTerm  = 0;

        /* for CMD_ENC_SEQ_GOP_PARAM */
        param->gopPresetIdx     = PRESET_IDX_IBBBP;

        /* for CMD_ENC_SEQ_INTRA_PARAM */
        param->decodingRefreshType = 1;
        param->intraPeriod         = 28;
        param->intraQP             = 0;
        param->forcedIdrHeaderEnable = 0;

        /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
        param->confWinTop    = 0;
        param->confWinBot    = 0;
        param->confWinLeft   = 0;
        param->confWinRight  = 0;

        /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
        param->independSliceMode     = 0;
        param->independSliceModeArg  = 0;

        /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
        param->dependSliceMode     = 0;
        param->dependSliceModeArg  = 0;

        /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
        param->intraRefreshMode     = 0;
        param->intraRefreshArg      = 0;
        param->useRecommendEncParam = 1;

        pEncConfig->roi_enable = 0;
        /* for CMD_ENC_PARAM */
        if (param->useRecommendEncParam != 1) {		// 0 : Custom,  2 : Boost mode (normal encoding speed, normal picture quality),  3 : Fast mode (high encoding speed, low picture quality)
            param->scalingListEnable        = 0;
            param->cuSizeMode               = 0x7;  
            param->tmvpEnable               = 1;
            param->wppEnable                = 0;
            param->maxNumMerge              = 2;
            param->disableDeblk             = 0;
            param->lfCrossSliceBoundaryEnable   = 1;
            param->betaOffsetDiv2           = 0;
            param->tcOffsetDiv2             = 0;
            param->skipIntraTrans           = 1;
            param->saoEnable                = 1;
            param->intraNxNEnable           = 1;
        }

        /* for CMD_ENC_RC_PARAM */
        pEncOP->rcEnable             = rcBitrate == 0 ? FALSE : TRUE;
        pEncOP->vbvBufferSize        = 3000;
        param->roiEnable    = 0;
        param->bitAllocMode          = 0;
        for (i = 0; i < MAX_GOP_NUM; i++) {
            param->fixedBitRatio[i] = 1;
        }
        param->cuLevelRCEnable       = 0;
        param->hvsQPEnable           = 1;
        param->hvsQpScale            = 2;

        /* for CMD_ENC_RC_MIN_MAX_QP */
        param->minQpI            = 8;
        param->maxQpI            = 51;
        param->minQpP            = 8;
        param->maxQpP            = 51;
        param->minQpB            = 8;
        param->maxQpB            = 51;

        param->hvsMaxDeltaQp     = 10;

        /* for CMD_ENC_CUSTOM_GOP_PARAM */
        param->gopParam.customGopSize     = 0;

        for (i= 0; i<param->gopParam.customGopSize; i++) {
            param->gopParam.picParam[i].picType      = PIC_TYPE_I;
            param->gopParam.picParam[i].pocOffset    = 1;
            param->gopParam.picParam[i].picQp        = 30;
            param->gopParam.picParam[i].refPocL0     = 0;
            param->gopParam.picParam[i].refPocL1     = 0;
            param->gopParam.picParam[i].temporalId   = 0;
        }

        // for VUI / time information.
        param->numTicksPocDiffOne   = 0;
        param->timeScale            =  pEncOP->frameRateInfo * 1000;
        param->numUnitsInTick       =  1000;

        param->chromaCbQpOffset = 0;
        param->chromaCrQpOffset = 0;
        param->initialRcQp      = 63;       // 63 is meaningless.
        param->nrYEnable        = 0;
        param->nrCbEnable       = 0;
        param->nrCrEnable       = 0;
        param->nrNoiseEstEnable = 0;

        pEncConfig->roi_avg_qp              = 0;
        pEncConfig->lambda_map_enable       = 0;

        param->monochromeEnable            = 0;
        param->strongIntraSmoothEnable     = 1;
        param->weightPredEnable            = 0;
        param->bgDetectEnable              = 0;
        param->bgThrDiff                   = 8;
        param->bgThrMeanDiff               = 1;
        param->bgLambdaQp                  = 32;
        param->bgDeltaQp                   = 3;

        param->customLambdaEnable          = 0;
        param->customMDEnable              = 0;
        param->pu04DeltaRate               = 0;
        param->pu08DeltaRate               = 0;
        param->pu16DeltaRate               = 0;
        param->pu32DeltaRate               = 0;
        param->pu04IntraPlanarDeltaRate    = 0;
        param->pu04IntraDcDeltaRate        = 0;
        param->pu04IntraAngleDeltaRate     = 0;
        param->pu08IntraPlanarDeltaRate    = 0;
        param->pu08IntraDcDeltaRate        = 0;
        param->pu08IntraAngleDeltaRate     = 0;
        param->pu16IntraPlanarDeltaRate    = 0;
        param->pu16IntraDcDeltaRate        = 0;
        param->pu16IntraAngleDeltaRate     = 0;
        param->pu32IntraPlanarDeltaRate    = 0;
        param->pu32IntraDcDeltaRate        = 0;
        param->pu32IntraAngleDeltaRate     = 0;
        param->cu08IntraDeltaRate          = 0;
        param->cu08InterDeltaRate          = 0;
        param->cu08MergeDeltaRate          = 0;
        param->cu16IntraDeltaRate          = 0;
        param->cu16InterDeltaRate          = 0;
        param->cu16MergeDeltaRate          = 0;
        param->cu32IntraDeltaRate          = 0;
        param->cu32InterDeltaRate          = 0;
        param->cu32MergeDeltaRate          = 0;
        param->coefClearDisable            = 0;

        param->rcWeightParam                = 2;
        param->rcWeightBuf                  = 128;
        // for H.264 encoder
        param->avcIdrPeriod                 = 0;
        param->rdoSkip                      = 1;
        param->lambdaScalingEnable          = 1;

        param->transform8x8Enable           = 1;
        param->avcSliceMode                 = 0;
        param->avcSliceArg                  = 0;
        param->intraMbRefreshMode           = 0;
        param->intraMbRefreshArg            = 1;
        param->mbLevelRcEnable              = 0;
        param->entropyCodingMode            = 1;
        if (bitFormat == STD_AVC)
            param->disableDeblk             = 2;

    }
    else {
        VLOG(ERR, "Invalid codec standard mode: bitFormat(%d) \n", bitFormat);
        return 0;
    }


    return 1;
}


/**
* To init EncOpenParam by CFG file
* IN
*   EncConfigParam *pEncConfig
* OUT
*   EncOpenParam *pEncOP
*   char *srcYuvFileName
*/
Int32 GetEncOpenParam(EncOpenParam *pEncOP, TestEncConfig *pEncConfig, ENC_CFG *pEncCfg)
{
    int bitFormat;
    ENC_CFG encCfgInst;
    ENC_CFG *pCfg;
    Int32   productId;
    char yuvDir[256] = ".";

    productId   = VPU_GetProductId(pEncOP->coreIdx);

    // Source YUV Image File to load
    if (pEncCfg) {
        pCfg = pEncCfg;
    }
    else {
        osal_memset( &encCfgInst, 0x00, sizeof(ENC_CFG));
        pCfg = &encCfgInst;
    }
    bitFormat = pEncOP->bitstreamFormat;


    if ( PRODUCT_ID_VP_SERIES(productId) == TRUE ) {
        // for VP
        switch (bitFormat)
        {
        case STD_HEVC:
        case STD_AVC:
            if (parseVpEncCfgFile(pCfg, pEncConfig->cfgFileName, bitFormat) == 0)
                return 0;
            if (pEncCfg)
                strcpy(pEncConfig->yuvFileName, pCfg->SrcFileName);
            else
                sprintf(pEncConfig->yuvFileName,  "%s/%s", yuvDir, pCfg->SrcFileName);

            if (pEncConfig->bitstreamFileName[0] == 0 && pCfg->BitStreamFileName[0] != 0)
                sprintf(pEncConfig->bitstreamFileName, "%s", pCfg->BitStreamFileName);
            else if ( pEncConfig->bitstreamFileName[0] == 0 )
                sprintf(pEncConfig->bitstreamFileName, "%s", "output_stream.265");

            if (pCfg->vpCfg.roiEnable) {
                strcpy(pEncConfig->roi_file_name, pCfg->vpCfg.roiFileName);
                if (!strcmp(pCfg->vpCfg.roiQpMapFile, "0") || pCfg->vpCfg.roiQpMapFile[0] == 0) {
                    //invalid value exist or not exist
                }
                else {
                    //valid value exist
                    strcpy(pEncConfig->roi_file_name, pCfg->vpCfg.roiQpMapFile);
                }
            }

            pEncConfig->roi_enable  = pCfg->vpCfg.roiEnable;
            pEncConfig->encAUD  = pCfg->vpCfg.encAUD;
            pEncConfig->encEOS  = pCfg->vpCfg.encEOS;
            pEncConfig->encEOB  = pCfg->vpCfg.encEOB;
            pEncConfig->useAsLongtermPeriod = pCfg->vpCfg.useAsLongtermPeriod;
            pEncConfig->refLongtermPeriod   = pCfg->vpCfg.refLongtermPeriod;

            pEncConfig->roi_avg_qp          = pCfg->vpCfg.roiAvgQp;
            pEncConfig->lambda_map_enable   = pCfg->vpCfg.customLambdaMapEnable;
            pEncConfig->mode_map_flag       = pCfg->vpCfg.customModeMapFlag;
            pEncConfig->wp_param_flag       = pCfg->vpCfg.weightPredEnable;
            pEncConfig->forceIdrPicIdx      = pCfg->vpCfg.forceIdrPicIdx;
            if (pCfg->vpCfg.scalingListEnable)
                strcpy(pEncConfig->scaling_list_fileName, pCfg->vpCfg.scalingListFileName);

            if (pCfg->vpCfg.customLambdaEnable)
                strcpy(pEncConfig->custom_lambda_fileName, pCfg->vpCfg.customLambdaFileName);

            // custom map
            if (pCfg->vpCfg.customLambdaMapEnable)
                strcpy(pEncConfig->lambda_map_fileName, pCfg->vpCfg.customLambdaMapFileName);

            if (pCfg->vpCfg.customModeMapFlag)
                strcpy(pEncConfig->mode_map_fileName, pCfg->vpCfg.customModeMapFileName);

            if (pCfg->vpCfg.weightPredEnable&1)
                strcpy(pEncConfig->wp_param_fileName, pCfg->vpCfg.WpParamFileName);

            pEncConfig->force_picskip_start =   pCfg->vpCfg.forcePicSkipStart;
            pEncConfig->force_picskip_end   =   pCfg->vpCfg.forcePicSkipEnd;
            pEncConfig->force_coefdrop_start=   pCfg->vpCfg.forceCoefDropStart;
            pEncConfig->force_coefdrop_end  =   pCfg->vpCfg.forceCoefDropEnd;

            break;
        default :
            break;
        }
    }

    if (bitFormat == STD_HEVC || (bitFormat == STD_AVC && productId == PRODUCT_ID_521)) {
        if (setVpEncOpenParam(pEncOP, pEncConfig, pCfg) == 0)
            return 0;
    }

    return 1;
}

