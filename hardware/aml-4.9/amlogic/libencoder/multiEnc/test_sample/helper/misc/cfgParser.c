
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
#include <errno.h>
#include "vpuapi.h"
#include "vpuapifunc.h"
#include "main_helper.h"

#include <string.h>

const VpCfgInfo vpCfgInfo[MAX_CFG] = {
    //name                          min            max              default
    {"InputFile",                   0,              0,                      0}, //0
    {"SourceWidth",                 0,              VP_MAX_ENC_PIC_WIDTH,   0},
    {"SourceHeight",                0,              VP_MAX_ENC_PIC_WIDTH,   0},
    {"InputBitDepth",               8,              10,                     8},
    {"FrameRate",                   0,              240,                    0},
    {"FrameSkip",                   0,              INT_MAX,                0},
    {"FramesToBeEncoded",           0,              INT_MAX,                0},
    {"IntraPeriod",                 0,              2047,                   0},
    {"DecodingRefreshType",         0,              2,                      1},
    {"GOPSize",                     1,              MAX_GOP_NUM,            1},
    {"IntraNxN",                    0,              1,                      1},// 10
    {"EnCu8x8",                     0,              1,                      1},
    {"EnCu16x16",                   0,              1,                      1},
    {"EnCu32x32",                   0,              1,                      1},
    {"IntraTransSkip",              0,              2,                      1},
    {"ConstrainedIntraPred",        0,              1,                      0},
    {"IntraCtuRefreshMode",         0,              4,                      0},
    {"IntraCtuRefreshArg",          0,              UI16_MAX,               0},
    {"MaxNumMerge",                 0,              2,                      2},
    {"EnTemporalMVP",               0,              1,                      1},
    {"ScalingList",                 0,              1,                      0}, // 20
    {"IndeSliceMode",               0,              1,                      0},
    {"IndeSliceArg",                0,              UI16_MAX,               0},
    {"DeSliceMode",                 0,              2,                      0},
    {"DeSliceArg",                  0,              UI16_MAX,               0},
    {"EnDBK",                       0,              1,                      1},
    {"EnSAO",                       0,              1,                      1},
    {"LFCrossSliceBoundaryFlag",    0,              1,                      1},
    {"BetaOffsetDiv2",             -6,              6,                      0},
    {"TcOffsetDiv2",               -6,              6,                      0},
    {"VpFrontSynchro",              0,              1,                      0}, // 30
    {"LosslessCoding",              0,              1,                      0},
    {"UsePresetEncTools",           0,              3,                      0},
    {"GopPreset",                   0,             16,                      0},
    {"RateControl",                 0,              1,                      0},
    {"EncBitrate",                  0,              700000000,              0},
    {"InitialDelay",                10,             3000,                   0},
    {"EnHvsQp",                     0,              1,                      1},
    {"CULevelRateControl",          0,              1,                      1},
    {"ConfWindSizeTop",             0,              VP_MAX_ENC_PIC_HEIGHT,  0},
    {"ConfWindSizeBot",             0,              VP_MAX_ENC_PIC_HEIGHT,  0}, //40
    {"ConfWindSizeRight",           0,              VP_MAX_ENC_PIC_WIDTH,   0},
    {"ConfWindSizeLeft",            0,              VP_MAX_ENC_PIC_WIDTH,   0},
    {"HvsQpScaleDiv2",              0,              4,                      2},
    {"MinQp",                       0,              63,                     8},
    {"MaxQp",                       0,              63,                    51},
    {"MaxDeltaQp",                  0,              12,                    10},
    {"QP",                          0,              63,                    30},
    {"BitAllocMode",                0,              2,                      0},
    {"FixedBitRatio%d",             1,              255,                    1},
    {"InternalBitDepth",            0,              10,                     0}, //50
    {"EnUserDataSei",               0,              1,                      0},
    {"UserDataEncTiming",           0,              1,                      0},
    {"UserDataSize",                0,              (1<<24) - 1,            1},
    {"UserDataPos",                 0,              1,                      0},
    {"EnRoi",                       0,              1,                      0},
    {"NumUnitsInTick",              0,              INT_MAX,                0},
    {"TimeScale",                   0,              INT_MAX,                0},
    {"NumTicksPocDiffOne",          0,              INT_MAX,                0},
    {"EncAUD",                      0,              1,                      0},
    {"EncEOS",                      0,              1,                      0}, //60
    {"EncEOB",                      0,              1,                      0},
    {"CbQpOffset",                  -12,            12,                     0},
    {"CrQpOffset",                  -12,            12,                     0},
    {"RcInitialQp",                 -1,              63,                   63},
    {"EnNoiseReductionY",           0,              1,                      0},
    {"EnNoiseReductionCb",          0,              1,                      0},
    {"EnNoiseReductionCr",          0,              1,                      0},
    {"EnNoiseEst",                  0,              1,                      1},
    {"NoiseSigmaY",                 0,              255,                    0},
    {"NoiseSigmaCb",                0,              255,                    0}, //70
    {"NoiseSigmaCr",                0,              255,                    0},
    {"IntraNoiseWeightY",           0,              31,                     7},
    {"IntraNoiseWeightCb",          0,              31,                     7},
    {"IntraNoiseWeightCr",          0,              31,                     7},
    {"InterNoiseWeightY",           0,              31,                     4},
    {"InterNoiseWeightCb",          0,              31,                     4},
    {"InterNoiseWeightCr",          0,              31,                     4},
    {"UseAsLongTermRefPeriod",      0,              INT_MAX,                0},
    {"RefLongTermPeriod",           0,              INT_MAX,                0},
    {"CropXPos",                    0,              VP_MAX_ENC_PIC_WIDTH,   0}, //80
    {"CropYPos",                    0,              VP_MAX_ENC_PIC_HEIGHT,  0},
    {"CropXSize",                   0,              VP_MAX_ENC_PIC_WIDTH,   0},
    {"CropYSize",                   0,              VP_MAX_ENC_PIC_HEIGHT,  0},
    {"BitstreamFile",               0,              0,                      0},
    {"EnCustomVpsHeader",           0,              1,                      0},
    {"EnCustomSpsHeader",           0,              1,                      0},
    {"EnCustomPpsHeader",           0,              1,                      0},
    {"CustomVpsPsId",               0,              15,                     0},
    {"CustomSpsPsId",               0,              15,                     0},
    {"CustomSpsActiveVpsId",        0,              15,                     0}, //90
    {"CustomPpsActiveSpsId",        0,              15,                     0},
    {"CustomVpsIntFlag",            0,              1,                      1},
    {"CustomVpsAvailFlag",          0,              1,                      1},
    {"CustomVpsMaxLayerMinus1",     0,              62,                     0},
    {"CustomVpsMaxSubLayerMinus1",  0,              6,                      0},
    {"CustomVpsTempIdNestFlag",     0,              1,                      0},
    {"CustomVpsMaxLayerId",         0,              31,                     0},
    {"CustomVpsNumLayerSetMinus1",  0,              2,                      0},
    {"CustomVpsExtFlag",            0,              1,                      0},
    {"CustomVpsExtDataFlag",        0,              1,                      0}, //100
    {"CustomVpsSubOrderInfoFlag",   0,              1,                      0},
    {"CustomSpsSubOrderInfoFlag",   0,              1,                      0},
    {"CustomVpsLayerId0",           0,              INT_MAX,                0},
    {"CustomVpsLayerId1",           0,              INT_MAX,                0},
    {"CustomSpsLog2MaxPocMinus4",   0,              12,                     4},
// newly added for VP520
    {"EncMonochrome",               0,              1,                      0},
    {"StrongIntraSmoothing",        0,              1,                      1},
    {"RoiAvgQP",                    0,              63,                     0},
    {"WeightedPred",                0,              3,                      0},
    {"EnBgDetect",                  0,              1,                      0}, // 110
    {"BgThDiff",                    0,              255,                    8},
    {"BgThMeanDiff",                0,              255,                    1},
    {"BgLambdaQp",                  0,              63,                    32},
    {"BgDeltaQp",                   -16,            15,                     3},
    {"TileNumColumns",              1,              6,                      1},
    {"TileNumRows",                 1,              6,                      1},
    {"TileUniformSpace",            0,              1,                      1},
    {"EnLambdaMap",                 0,              1,                      0},
    {"EnCustomLambda",              0,              1,                      0},
    {"EnCustomMD",                  0,              1,                      0}, //120
    {"PU04DeltaRate",               0,              255,                    0},
    {"PU08DeltaRate",               0,              255,                    0},
    {"PU16DeltaRate",               0,              255,                    0},
    {"PU32DeltaRate",               0,              255,                    0},
    {"PU04IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU04IntraDcDeltaRate",        0,              255,                    0},
    {"PU04IntraAngleDeltaRate",     0,              255,                    0},
    {"PU08IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU08IntraDcDeltaRate",        0,              255,                    0},
    {"PU08IntraAngleDeltaRate",     0,              255,                    0}, //130
    {"PU16IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU16IntraDcDeltaRate",        0,              255,                    0},
    {"PU16IntraAngleDeltaRate",     0,              255,                    0},
    {"PU32IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU32IntraDcDeltaRate",        0,              255,                    0},
    {"PU32IntraAngleDeltaRate",     0,              255,                    0},
    {"CU08IntraDeltaRate",          0,              255,                    0},
    {"CU08InterDeltaRate",          0,              255,                    0},
    {"CU08MergeDeltaRate",          0,              255,                    0},
    {"CU16IntraDeltaRate",          0,              255,                    0}, //140
    {"CU16InterDeltaRate",          0,              255,                    0},
    {"CU16MergeDeltaRate",          0,              255,                    0},
    {"CU32IntraDeltaRate",          0,              255,                    0},
    {"CU32InterDeltaRate",          0,              255,                    0},
    {"CU32MergeDeltaRate",          0,              255,                    0},
    {"DisableCoefClear",            0,              1,                      0},
    {"EnModeMap",                   0,              3,                      0},
    {"ForcePicSkipStart",          -1,              INT_MAX,               -1},
    {"ForcePicSkipEnd",            -1,              INT_MAX,               -1},
    {"ForceCoefDropStart",         -1,              INT_MAX,               -1}, //150
    {"ForceCoefDropEnd",           -1,              INT_MAX,               -1},
    {"EnUserFilterLevel",           0,              1,                      0},
    {"LfFilterLevel",              -63,            63,                      0},
    {"SharpnessLevel",              0,              7,                      0},
    {"LfRefDeltaIntra",            -63,            63,                      1},
    {"LfRefDeltaRef0",             -63,            63,                      0},
    {"LfRefDeltaRef1",             -63,            63,                     -1},
    {"LfModeDelta",                -63,            63,                      0},
    {"EnSVC",                       0,              1,                      0},
    {"YDcQpOffset",                -3,              3,                      0}, // 160
    {"CbCrDcQpOffset",             -3,              3,                      0},
    {"CbCrAcQpOffset",             -3,              3,                      0},
    {"StillPictureProfile",         0,              1,                      0},
    {"SvcMode",                     0,              1,                      1},
    {"VbvBufferSize",              10,             3000,                   3000},
    {"EncBitrateBL",                0,              700000000,              0},
    // newly added for H.264 on VP5
    {"IdrPeriod",                   0,              2047,                   0},
    {"RdoSkip",                     0,              1,                      1},
    {"LambdaScaling",               0,              1,                      1},
    {"Transform8x8",                0,              1,                      1},
    {"SliceMode",                   0,              1,                      0}, //170
    {"SliceArg",                    0,              INT_MAX,                0},
    {"IntraMbRefreshMode",          0,              3,                      0},
    {"IntraMbRefreshArg",           1,              INT_MAX,                1},
    {"MBLevelRateControl",          0,              1,                      0},
    {"CABAC",                       0,              1,                      1},
    {"RoiQpMapFile",                0,              1,                      1},
    {"S2fmeOff",                    0,              1,                      0},
    {"RcWeightParaCtrl",            1,              31,                     16},
    {"RcWeightBufCtrl",             1,              255,                    128}, //179, total 180
    {"EnForcedIDRHeader",           0,              2,                      0},
    {"ForceIdrPicIdx",             -1,              INT_MAX,               -1},
};

//------------------------------------------------------------------------------
// ENCODE PARAMETER PARSE FUNCSIONS
//------------------------------------------------------------------------------
// Parameter parsing helper
static int GetValue(osal_file_t fp, char *para, char *value)
{
    char lineStr[256];
    char paraStr[256];
    osal_fseek(fp, 0, SEEK_SET);

    while (1) {
        if (fgets(lineStr, 256, fp) == NULL)
            return 0;
        sscanf(lineStr, "%s %s", paraStr, value);
        if (paraStr[0] != ';') {
            if (strcmp(para, paraStr) == 0)
                return 1;
        }
    }
}

// Parse "string number number ..." at most "num" numbers
// e.g. SKIP_PIC_NUM 1 3 4 5
static int GetValues(osal_file_t fp, char *para, int *values, int num)
{
    char line[1024];

    osal_fseek(fp, 0, SEEK_SET);

    while (1)
    {
        int  i;
        char *str;

        if (fgets(line, sizeof(line)-1, fp) == NULL)
            return 0;

        // empty line
        if ((str = strtok(line, " ")) == NULL)
            continue;

        if(strcmp(str, para) != 0)
            continue;

        for (i=0; i<num; i++)
        {
            if ((str = strtok(NULL, " ")) == NULL)
                return 1;
            if (!isdigit((Int32)str[0]))
                return 1;
            values[i] = atoi(str);
        }
        return 1;
    }
}

int parseMp4CfgFile(ENC_CFG *pEncCfg, char *FileName)
{
    osal_file_t fp;
    char sValue[1024];
    int  ret = 0;

    fp = osal_fopen(FileName, "rt");
    if (fp == NULL) {
        return ret;
    }

    if (GetValue(fp, "YUV_SRC_IMG", sValue) == 0)
        goto __end_parseMp4CfgFile;
    else
        strcpy(pEncCfg->SrcFileName, sValue);

    if (GetValue(fp, "FRAME_NUMBER_ENCODED", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->NumFrame = atoi(sValue);
    if (GetValue(fp, "PICTURE_WIDTH", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->PicX = atoi(sValue);
    if (GetValue(fp, "PICTURE_HEIGHT", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->PicY = atoi(sValue);
    if (GetValue(fp, "FRAME_RATE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    {
        double frameRate;
        int  timeRes, timeInc;
#ifdef ANDROID
        frameRate = atoi(sValue);
#else
        frameRate = atof(sValue);
#endif
        timeInc = 1;
        while ((int)frameRate != frameRate) {
            timeInc *= 10;
            frameRate *= 10;
        }
        timeRes = (int) frameRate;
        // divide 2 or 5
        if (timeInc%2 == 0 && timeRes%2 == 0) {
            timeInc /= 2;
            timeRes /= 2;
        }
        if (timeInc%5 == 0 && timeRes%5 == 0) {
            timeInc /= 5;
            timeRes /= 5;
        }
        if (timeRes == 2997 && timeInc == 100) {
            timeRes = 30000;
            timeInc = 1001;
        }
        pEncCfg->FrameRate = (timeInc - 1) << 16;
        pEncCfg->FrameRate |= timeRes;
    }
    if (GetValue(fp, "VERSION_ID", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->VerId = atoi(sValue);
    if (GetValue(fp, "DATA_PART_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->DataPartEn = atoi(sValue);
    if (GetValue(fp, "REV_VLC_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RevVlcEn = atoi(sValue);

    if (GetValue(fp, "INTRA_DC_VLC_THRES", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->IntraDcVlcThr = atoi(sValue);
    if (GetValue(fp, "SHORT_VIDEO", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->ShortVideoHeader = atoi(sValue);
    if (GetValue(fp, "ANNEX_I_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexI = atoi(sValue);
    if (GetValue(fp, "ANNEX_J_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexJ = atoi(sValue);
    if (GetValue(fp, "ANNEX_K_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexK = atoi(sValue);
    if (GetValue(fp, "ANNEX_T_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexT = atoi(sValue);

    if (GetValue(fp, "VOP_QUANT_SCALE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->VopQuant = atoi(sValue);
    if (GetValue(fp, "GOP_PIC_NUMBER", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->GopPicNum = atoi(sValue);
    if (GetValue(fp, "SLICE_MODE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_MODE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceSizeMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_NUMBER", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceSizeNum = atoi(sValue);

    if (GetValue(fp, "RATE_CONTROL_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcEnable = atoi(sValue);
    if (GetValue(fp, "BIT_RATE_KBPS", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcBitRate = atoi(sValue);
    if (GetValue(fp, "DELAY_IN_MS", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcInitDelay = atoi(sValue);
    if (GetValue(fp, "VBV_BUFFER_SIZE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcBufSize = atoi(sValue);
    if (GetValue(fp, "INTRA_MB_REFRESH", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->IntraRefreshNum = atoi(sValue);

    pEncCfg->ConscIntraRefreshEnable = 0;
    if (pEncCfg->IntraRefreshNum > 0)
    {
        if (GetValue(fp, "CONSC_INTRA_REFRESH_EN", sValue) == 0)
            pEncCfg->ConscIntraRefreshEnable = 0;
        else
            pEncCfg->ConscIntraRefreshEnable = atoi(sValue);
    }
    if (GetValue(fp, "CONST_INTRA_QP_EN", sValue) == 0)
        pEncCfg->ConstantIntraQPEnable = 0;
    else
        pEncCfg->ConstantIntraQPEnable = atoi(sValue);
    if (GetValue(fp, "CONST_INTRA_QP", sValue) == 0)
        pEncCfg->RCIntraQP = 0;
    else
        pEncCfg->RCIntraQP = atoi(sValue);

    if (GetValue(fp, "HEC_ENABLE", sValue) == 0)
        pEncCfg->HecEnable = 0;
    else
        pEncCfg->HecEnable = atoi(sValue);

    if (GetValue(fp, "SEARCH_RANGE", sValue) == 0)
        pEncCfg->SearchRange = 0;
    else
        pEncCfg->SearchRange = atoi(sValue);
    if (GetValue(fp, "ME_USE_ZERO_PMV", sValue) == 0)
        pEncCfg->MeUseZeroPmv = 0;
    else
        pEncCfg->MeUseZeroPmv = atoi(sValue);
    if (GetValue(fp, "WEIGHT_INTRA_COST", sValue) == 0)
        pEncCfg->intraCostWeight = 0;
    else
        pEncCfg->intraCostWeight = atoi(sValue);

    if (GetValue(fp, "MAX_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxQpSetEnable= 0;
    else
        pEncCfg->MaxQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MAX_QP", sValue) == 0)
        pEncCfg->MaxQp = 0;
    else
        pEncCfg->MaxQp = atoi(sValue);
    if (GetValue(fp, "GAMMA_SET_ENABLE", sValue) == 0)
        pEncCfg->GammaSetEnable = 0;
    else
        pEncCfg->GammaSetEnable = atoi(sValue);
    if (GetValue(fp, "GAMMA", sValue) == 0)
        pEncCfg->Gamma = 0;
    else
        pEncCfg->Gamma = atoi(sValue);

    if (GetValue(fp, "RC_INTERVAL_MODE", sValue) == 0)
        pEncCfg->rcIntervalMode = 0;
    else
        pEncCfg->rcIntervalMode = atoi(sValue);

    if (GetValue(fp, "RC_MB_INTERVAL", sValue) == 0)
        pEncCfg->RcMBInterval = 0;
    else
        pEncCfg->RcMBInterval = atoi(sValue);

    ret = 1; /* Success */

__end_parseMp4CfgFile:
    osal_fclose(fp);
    return ret;
}


int parseAvcCfgFile(ENC_CFG *pEncCfg, char *FileName)
{
    osal_file_t fp;
    char sValue[1024];
    int  ret = 0;

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        return 0;
    }

    if (GetValue(fp, "YUV_SRC_IMG", sValue) == 0)
        goto __end_parseAvcCfgFile;
    else
        strcpy(pEncCfg->SrcFileName, sValue);

    if (GetValue(fp, "FRAME_NUMBER_ENCODED", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->NumFrame = atoi(sValue);
    if (GetValue(fp, "PICTURE_WIDTH", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicX = atoi(sValue);
    if (GetValue(fp, "PICTURE_HEIGHT", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicY = atoi(sValue);
    if (GetValue(fp, "FRAME_RATE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    {
        double frameRate;
        int  timeRes, timeInc;

#ifdef ANDROID
        frameRate = atoi(sValue);
#else
        frameRate = atof(sValue);
#endif

        timeInc = 1;
        while ((int)frameRate != frameRate) {
            timeInc *= 10;
            frameRate *= 10;
        }
        timeRes = (int) frameRate;
        // divide 2 or 5
        if (timeInc%2 == 0 && timeRes%2 == 0) {
            timeInc /= 2;
            timeRes /= 2;
        }
        if (timeInc%5 == 0 && timeRes%5 == 0) {
            timeInc /= 5;
            timeRes /= 5;
        }

        if (timeRes == 2997 && timeInc == 100) {
            timeRes = 30000;
            timeInc = 1001;
        }
        pEncCfg->FrameRate = (timeInc - 1) << 16;
        pEncCfg->FrameRate |= timeRes;
    }
    if (GetValue(fp, "CONSTRAINED_INTRA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->ConstIntraPredFlag = atoi(sValue);
    if (GetValue(fp, "DISABLE_DEBLK", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DisableDeblk = atoi(sValue);
    if (GetValue(fp, "DEBLK_ALPHA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DeblkOffsetA = atoi(sValue);
    if (GetValue(fp, "DEBLK_BETA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DeblkOffsetB = atoi(sValue);
    if (GetValue(fp, "CHROMA_QP_OFFSET", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->ChromaQpOffset = atoi(sValue);

    if (GetValue(fp, "LEVEL", sValue) == 0)
    {
        pEncCfg->level = 0;//note : 0 means auto calculation.
    }
    else
    {
        pEncCfg->level = atoi(sValue);
        if (pEncCfg->level<0 || pEncCfg->level>51)
            goto __end_parseAvcCfgFile;
    }

    if (GetValue(fp, "PIC_QP_Y", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicQpY = atoi(sValue);
    if (GetValue(fp, "GOP_PIC_NUMBER", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->GopPicNum = atoi(sValue);
    if (GetValue(fp, "IDR_INTERVAL", sValue) == 0)
        pEncCfg->IDRInterval = 0;
    else
        pEncCfg->IDRInterval = atoi(sValue);
    if (GetValue(fp, "SLICE_MODE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_MODE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceSizeMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_NUMBER", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceSizeNum = atoi(sValue);
    if (GetValue(fp, "AUD_ENABLE", sValue) == 0)
        pEncCfg->aud_en = 0;
    else
        pEncCfg->aud_en = atoi(sValue);

    /**
    * Error Resilience
    */
    // Intra Cost Weight : not mandatory. default zero
    if (GetValue(fp, "WEIGHT_INTRA_COST", sValue) == 0)
        pEncCfg->intraCostWeight = 0;
    else
        pEncCfg->intraCostWeight = atoi(sValue);

    /**
    * CROP information
    */
    if (GetValue(fp, "FRAME_CROP_LEFT", sValue) == 0)
        pEncCfg->frameCropLeft = 0;
    else
        pEncCfg->frameCropLeft = atoi(sValue);

    if (GetValue(fp, "FRAME_CROP_RIGHT", sValue) == 0)
        pEncCfg->frameCropRight = 0;
    else
        pEncCfg->frameCropRight = atoi(sValue);

    if (GetValue(fp, "FRAME_CROP_TOP", sValue) == 0)
        pEncCfg->frameCropTop = 0;
    else
        pEncCfg->frameCropTop = atoi(sValue);

    if (GetValue(fp, "FRAME_CROP_BOTTOM", sValue) == 0)
        pEncCfg->frameCropBottom = 0;
    else
        pEncCfg->frameCropBottom = atoi(sValue);

    /**
    * ME Option
    */

    if (GetValue(fp, "ME_USE_ZERO_PMV", sValue) == 0)
        pEncCfg->MeUseZeroPmv = 0;
    else
        pEncCfg->MeUseZeroPmv = atoi(sValue);

    if (GetValue(fp, "ME_BLK_MODE_ENABLE", sValue) == 0)
        pEncCfg->MeBlkModeEnable = 0;
    else
        pEncCfg->MeBlkModeEnable = atoi(sValue);

    if (GetValue(fp, "RATE_CONTROL_ENABLE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcEnable = atoi(sValue);
    if (GetValue(fp, "BIT_RATE_KBPS", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcBitRate = atoi(sValue);
    if (GetValue(fp, "DELAY_IN_MS", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcInitDelay = atoi(sValue);

    if (GetValue(fp, "VBV_BUFFER_SIZE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcBufSize = atoi(sValue);
    if (GetValue(fp, "INTRA_MB_REFRESH", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->IntraRefreshNum = atoi(sValue);

    pEncCfg->ConscIntraRefreshEnable = 0;
    if (pEncCfg->IntraRefreshNum > 0)
    {
        if (GetValue(fp, "CONSC_INTRA_REFRESH_EN", sValue) == 0)
            pEncCfg->ConscIntraRefreshEnable = 0;
        else
            pEncCfg->ConscIntraRefreshEnable = atoi(sValue);
    }

    if (GetValue(fp, "FRAME_SKIP_DISABLE", sValue) == 0)
        pEncCfg->frameSkipDisable = 0;
    else
        pEncCfg->frameSkipDisable = atoi(sValue);

    if (GetValue(fp, "CONST_INTRAQP_ENABLE", sValue) == 0)
        pEncCfg->ConstantIntraQPEnable = 0;
    else
        pEncCfg->ConstantIntraQPEnable = atoi(sValue);

    if (GetValue(fp, "RC_INTRA_QP", sValue) == 0)
        pEncCfg->RCIntraQP = 0;
    else
        pEncCfg->RCIntraQP = atoi(sValue);
    if (GetValue(fp, "MAX_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxQpSetEnable= 0;
    else
        pEncCfg->MaxQpSetEnable = atoi(sValue);

    if (GetValue(fp, "MAX_QP", sValue) == 0)
        pEncCfg->MaxQp = 0;
    else
        pEncCfg->MaxQp = atoi(sValue);

    if (GetValue(fp, "MIN_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MinQpSetEnable= 0;
    else
        pEncCfg->MinQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MIN_QP", sValue) == 0)
        pEncCfg->MinQp = 0;
    else
        pEncCfg->MinQp = atoi(sValue);

    if (GetValue(fp, "MAX_DELTA_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxDeltaQpSetEnable= 0;
    else
        pEncCfg->MaxDeltaQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MAX_DELTA_QP", sValue) == 0)
        pEncCfg->MaxDeltaQp = 0;
    else
        pEncCfg->MaxDeltaQp = atoi(sValue);

    if (GetValue(fp, "MIN_DELTA_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MinDeltaQpSetEnable= 0;
    else
        pEncCfg->MinDeltaQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MIN_DELTA_QP", sValue) == 0)
        pEncCfg->MinDeltaQp = 0;
    else
        pEncCfg->MinDeltaQp = atoi(sValue);

    if (GetValue(fp, "GAMMA_SET_ENABLE", sValue) == 0)
        pEncCfg->GammaSetEnable = 0;
    else
        pEncCfg->GammaSetEnable = atoi(sValue);
    if (GetValue(fp, "GAMMA", sValue) == 0)
        pEncCfg->Gamma = 0;
    else
        pEncCfg->Gamma = atoi(sValue);
    /* 960 features */
    if (GetValue(fp, "RC_INTERVAL_MODE", sValue) == 0)
        pEncCfg->rcIntervalMode = 0;
    else
        pEncCfg->rcIntervalMode = atoi(sValue);

    if (GetValue(fp, "RC_MB_INTERVAL", sValue) == 0)
        pEncCfg->RcMBInterval = 0;
    else
        pEncCfg->RcMBInterval = atoi(sValue);
    /***************************************/
    if (GetValue(fp, "RC_INTERVAL_MODE", sValue) == 0)
        pEncCfg->rcIntervalMode = 0;
    else
        pEncCfg->rcIntervalMode = atoi(sValue);

    if (GetValue(fp, "RC_MB_INTERVAL", sValue) == 0)
        pEncCfg->RcMBInterval = 0;
    else
        pEncCfg->RcMBInterval = atoi(sValue);
    if (GetValue(fp, "SEARCH_RANGE", sValue) == 0)
        pEncCfg->SearchRange = 0;
    else
        pEncCfg->SearchRange = atoi(sValue);


    osal_memset(pEncCfg->skipPicNums, 0, sizeof(pEncCfg->skipPicNums));
    GetValues(fp, "SKIP_PIC_NUMS", pEncCfg->skipPicNums, sizeof(pEncCfg->skipPicNums));


    /**
    * VUI Parameter
    */
    if (GetValue(fp, "VUI_PARAMETERS_PRESENT_FLAG", sValue) == 0)
        pEncCfg->VuiPresFlag = 0;
    else
        pEncCfg->VuiPresFlag = atoi(sValue);

    if (pEncCfg->VuiPresFlag == 1) {

        if (GetValue(fp, "VIDEO_SIGNAL_TYPE_PRESENT_FLAG", sValue) == 0)
            pEncCfg->VideoSignalTypePresFlag = 0;
        else
            pEncCfg->VideoSignalTypePresFlag = atoi(sValue);

        if (pEncCfg->VideoSignalTypePresFlag) {
            if (GetValue(fp, "VIDEO_FORMAT", sValue) == 0)
                pEncCfg->VideoFormat = 5;
            else
                pEncCfg->VideoFormat = atoi(sValue);

            if (GetValue(fp, "VIDEO_FULL_RANGE_FLAG", sValue) == 0)
                pEncCfg->VideoFullRangeFlag = 0;
            else
                pEncCfg->VideoFullRangeFlag = atoi(sValue);

            if (GetValue(fp, "COLOUR_DESCRIPTION_PRESENT_FLAG", sValue) == 0)
                pEncCfg->ColourDescripPresFlag = 1;
            else
                pEncCfg->ColourDescripPresFlag = atoi(sValue);

            if (pEncCfg->ColourDescripPresFlag) {
                if (GetValue(fp, "COLOUR_PRIMARIES", sValue) == 0)
                    pEncCfg->ColourPrimaries = 1;
                else
                    pEncCfg->ColourPrimaries = atoi(sValue);

                if (GetValue(fp, "TRANSFER_CHARACTERISTICS", sValue) == 0)
                    pEncCfg->TransferCharacteristics = 2;
                else
                    pEncCfg->TransferCharacteristics = atoi(sValue);

                if (GetValue(fp, "MATRIX_COEFFICIENTS", sValue) == 0)
                    pEncCfg->MatrixCoeff = 2;
                else
                    pEncCfg->MatrixCoeff = atoi(sValue);
            }
        }
    }

    ret = 1; /* Success */
__end_parseAvcCfgFile:
    osal_fclose(fp);
    return ret;
}

static int VP_GetStringValue(
    osal_file_t fp,
    char* para,
    char* value
    )
{
    int pos = 0;
    char* token = NULL;
    char lineStr[256] = {0, };
    char valueStr[256] = {0, };
    osal_fseek(fp, 0, SEEK_SET);

    while (1) {
        if ( fgets(lineStr, 256, fp) == NULL ) {
            return 0;//not exist para in cfg file
        }

        if( (lineStr[0] == '#') || (lineStr[0] == ';') || (lineStr[0] == ':') ) { // check comment
            continue;
        }

        token = strtok(lineStr, ": "); // parameter name is separated by ' ' or ':'
        if( token != NULL ) {
            if ( strcasecmp(para, token) == 0) { // check parameter name
                token = strtok(NULL, ":\r\n");
                if ( token && strlen(token) == 1 && strncmp(token, " ", 1) == 0) //check space - ex) Frame1  : P  1  0  0
                    token = strtok(NULL, ":\r\n");
                if ( token == NULL )
                    return -1;
                osal_memcpy( valueStr, token, strlen(token) );
                while( valueStr[pos] == ' ' ) { // check space
                    pos++;
                }
                if ( valueStr[pos] == 0 )
                    return -1;//no value
                strcpy(value, &valueStr[pos]);
                return 1;
            }
            else {
                continue;
            }
        }
        else {
            continue;
        }
    }
}

static int VP_GetValue(
    osal_file_t fp,
    char* cfgName,
    int* value
    )
{
    int i;
    int iValue;
    int ret;
    char sValue[256] = {0, };

    for (i=0; i < MAX_CFG ;i++) {
        if ( strcmp(vpCfgInfo[i].name, cfgName) == 0)
            break;
    }
    if ( i == MAX_CFG ) {
        VLOG(ERR, "CFG param error : %s\n", cfgName);
        return 0;
    }

    ret = VP_GetStringValue(fp, cfgName, sValue);
    if(ret == 1) {
        iValue = atoi(sValue);
        if( (iValue >= vpCfgInfo[i].min) && (iValue <= vpCfgInfo[i].max) ) { // Check min, max
            *value = iValue;
            return 1;
        }
        else {
            VLOG(ERR, "CFG file error : %s value is not available. ( min = %d, max = %d)\n", vpCfgInfo[i].name, vpCfgInfo[i].min, vpCfgInfo[i].max);
            return 0;
        }
    }
    else if ( ret == -1 ) {
            VLOG(ERR, "CFG file error : %s value is not available. ( min = %d, max = %d)\n", vpCfgInfo[i].name, vpCfgInfo[i].min, vpCfgInfo[i].max);
            return 0;
    }
    else {
        *value = vpCfgInfo[i].def;
        return 1;
    }
}


static int VP_SetGOPInfo(
    char* lineStr,
    CustomGopPicParam* gopPicParam,
    int useDeriveLambdaWeight,
    int intraQp
    )
{
    int numParsed;
    char sliceType;

    osal_memset(gopPicParam, 0, sizeof(CustomGopPicParam));

    numParsed = sscanf(lineStr, "%c %d %d %d %d %d",
        &sliceType, &gopPicParam->pocOffset, &gopPicParam->picQp,
        &gopPicParam->temporalId, &gopPicParam->refPocL0, &gopPicParam->refPocL1);

    VLOG(DEBUG, "HEVC type %c, poc %d, qp %d, temporal %d, ref0 %d, ref1 %d, nums %d\n",
        sliceType, gopPicParam->pocOffset,gopPicParam->picQp, gopPicParam->temporalId,
        gopPicParam->refPocL0, gopPicParam->refPocL1, numParsed);

    if (sliceType=='I') {
        gopPicParam->picType = PIC_TYPE_I;
    }
    else if (sliceType=='P') {
        if (numParsed == 6) {
            gopPicParam->picType = PIC_TYPE_P;
            gopPicParam->numRefPicL0 = 2;
        }
        else {
            gopPicParam->picType = PIC_TYPE_P;
            gopPicParam->numRefPicL0 = 1;
        }
    }
    else if (sliceType=='B') {
        gopPicParam->picType = PIC_TYPE_B;
        gopPicParam->numRefPicL0 = 1;
    }
    else {
        return 0;
    }

    if (sliceType=='B' && numParsed != 6) {
        return 0;
    }
    if (gopPicParam->temporalId < 0) {
        return 0;
    }

    gopPicParam->picQp = MIN(63, gopPicParam->picQp + intraQp);

    return 1;
}

static int VP_AVCSetGOPInfo(
    char* lineStr,
    CustomGopPicParam* gopPicParam,
    int useDeriveLambdaWeight,
    int intraQp
    )
{
    int numParsed;
    char sliceType;

    osal_memset(gopPicParam, 0, sizeof(CustomGopPicParam));

    numParsed = sscanf(lineStr, "%c %d %d %d %d %d",
        &sliceType, &gopPicParam->pocOffset, &gopPicParam->picQp,
        &gopPicParam->temporalId, &gopPicParam->refPocL0, &gopPicParam->refPocL1);

    VLOG(DEBUG, "AVC type %c, poc %d, qp %d, temporal %d, ref0 %d, ref1 %d, nums %d\n",
        sliceType, gopPicParam->pocOffset,gopPicParam->picQp, gopPicParam->temporalId,
        gopPicParam->refPocL0, gopPicParam->refPocL1, numParsed);

    if (sliceType=='I') {
        gopPicParam->picType = PIC_TYPE_I;
    }
    else if (sliceType=='P') {

        if (numParsed == 6) {
            gopPicParam->picType = PIC_TYPE_P;
            gopPicParam->numRefPicL0 = 2;
        }
        else {
            gopPicParam->picType = PIC_TYPE_P;
            gopPicParam->numRefPicL0 = 1;
        }
    }
    else if (sliceType=='B') {
        gopPicParam->picType = PIC_TYPE_B;
        gopPicParam->numRefPicL0 = 1;
    }
    else {
        return 0;
    }
    if (sliceType == 'B' && numParsed != 6) {
        return 0;
    }

    gopPicParam->picQp = gopPicParam->picQp + intraQp;

    return 1;
}

int parseRoiCtuModeParam(
    char* lineStr,
    VpuRect* roiRegion,
    int* roiQp,
    int picX,
    int picY
    )
{
    int numParsed;

    osal_memset(roiRegion, 0, sizeof(VpuRect));
    *roiQp = 0;

    numParsed = sscanf(lineStr, "%d %d %d %d %d",
        &roiRegion->left, &roiRegion->right, &roiRegion->top, &roiRegion->bottom, roiQp);

    if (numParsed != 5) {
        return 0;
    }
    if (*roiQp < 0 || *roiQp > 51) {
        return 0;
    }
    if ((Int32)roiRegion->left < 0 || (Int32)roiRegion->top < 0) {
        return 0;
    }
    if (roiRegion->left > (Uint32)((picX + CTB_SIZE - 1) >> LOG2_CTB_SIZE) || \
        roiRegion->top > (Uint32)((picY + CTB_SIZE - 1) >> LOG2_CTB_SIZE)) {
        return 0;
    }
    if (roiRegion->right > (Uint32)((picX + CTB_SIZE - 1) >> LOG2_CTB_SIZE) || \
        roiRegion->bottom > (Uint32)((picY + CTB_SIZE - 1) >> LOG2_CTB_SIZE)) {
        return 0;
    }
    if (roiRegion->left > roiRegion->right) {
        return 0;
    }
    if (roiRegion->top > roiRegion->bottom) {
        return 0;
    }

    return 1;
}

int parseVpEncCfgFile(
    ENC_CFG *pEncCfg, 
    char *FileName,
    int bitFormat
    )
{
    osal_file_t fp;
    char sValue[256] = {0, };
    char tempStr[256] = {0, };
    int iValue = 0, ret = 0, i = 0;
    int intra8=0, intra16=0, intra32=0, frameSkip=0; // temp value
    UNREFERENCED_PARAMETER(frameSkip);

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        VLOG(ERR, "file open err : %s, errno(%d)\n", FileName, errno);
        return ret;
    }

    if (VP_GetStringValue(fp, "BitstreamFile", sValue) == 1)
        strcpy(pEncCfg->BitStreamFileName, sValue);

    if (VP_GetStringValue(fp, "InputFile", sValue) == 1)
        strcpy(pEncCfg->SrcFileName, sValue);
    else
        goto __end_parse;

    if (VP_GetValue(fp, "SourceWidth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.picX = iValue;
    if (VP_GetValue(fp, "SourceHeight", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.picY = iValue;
    if (VP_GetValue(fp, "FramesToBeEncoded", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->NumFrame = iValue;
    if (VP_GetValue(fp, "InputBitDepth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->SrcBitDepth = iValue; // BitDepth == 8 ? HEVC_PROFILE_MAIN : HEVC_PROFILE_MAIN10

    if (VP_GetValue(fp, "InternalBitDepth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.internalBitDepth   = iValue;

    if (VP_GetValue(fp, "LosslessCoding", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.losslessEnable = iValue;
    if (VP_GetValue(fp, "ConstrainedIntraPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.constIntraPredFlag = iValue;
    if (VP_GetValue(fp, "DecodingRefreshType", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.decodingRefreshType = iValue;

    if (VP_GetValue(fp, "StillPictureProfile", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.enStillPicture = iValue;

    // BitAllocMode
    if (VP_GetValue(fp, "BitAllocMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bitAllocMode = iValue;

    // FixedBitRatio 0 ~ 7
#define FIXED_BIT_RATIO 49
    for (i=0; i<MAX_GOP_NUM; i++) {
        sprintf(tempStr, "FixedBitRatio%d", i);
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            iValue = atoi(sValue);
            if ( iValue >= vpCfgInfo[FIXED_BIT_RATIO].min && iValue <= vpCfgInfo[FIXED_BIT_RATIO].max )
                pEncCfg->vpCfg.fixedBitRatio[i] = iValue;
            else
                pEncCfg->vpCfg.fixedBitRatio[i] = vpCfgInfo[FIXED_BIT_RATIO].def;
        }
		else
			pEncCfg->vpCfg.fixedBitRatio[i] = vpCfgInfo[FIXED_BIT_RATIO].def;

    }

    if (VP_GetValue(fp, "QP", &iValue) == 0) //INTRA_QP
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraQP = iValue;

    if (VP_GetValue(fp, "IntraPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraPeriod = iValue;

    if (VP_GetValue(fp, "ConfWindSizeTop", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.confWinTop = iValue;

    if (VP_GetValue(fp, "ConfWindSizeBot", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.confWinBot = iValue;

    if (VP_GetValue(fp, "ConfWindSizeRight", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.confWinRight = iValue;

    if (VP_GetValue(fp, "ConfWindSizeLeft", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.confWinLeft = iValue;

    if (VP_GetValue(fp, "FrameRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.frameRate = iValue;

    if (VP_GetValue(fp, "IndeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.independSliceMode = iValue;

    if (VP_GetValue(fp, "IndeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.independSliceModeArg = iValue;

    if (VP_GetValue(fp, "DeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.dependSliceMode = iValue;

    if (VP_GetValue(fp, "DeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.dependSliceModeArg = iValue;


    if (VP_GetValue(fp, "IntraCtuRefreshMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraRefreshMode = iValue;


    if (VP_GetValue(fp, "IntraCtuRefreshArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraRefreshArg = iValue;

    if (VP_GetValue(fp, "UsePresetEncTools", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.useRecommendEncParam = iValue;

    if (VP_GetValue(fp, "ScalingList", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.scalingListEnable = iValue;

    if (VP_GetValue(fp, "EnCu8x8", &iValue) == 0)
        goto __end_parse;
    else
        intra8 = iValue;

    if (VP_GetValue(fp, "EnCu16x16", &iValue) == 0)
        goto __end_parse;
    else
        intra16 = iValue;

    if (VP_GetValue(fp, "EnCu32x32", &iValue) == 0)
        goto __end_parse;
    else
        intra32 = iValue;

    pEncCfg->vpCfg.cuSizeMode = (intra8&0x01) | (intra16&0x01)<<1 | (intra32&0x01)<<2;

    if (VP_GetValue(fp, "EnTemporalMVP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.tmvpEnable = iValue;

    if (VP_GetValue(fp, "VpFrontSynchro", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.wppenable = iValue;

    if (VP_GetValue(fp, "MaxNumMerge", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.maxNumMerge = iValue;

    if (VP_GetValue(fp, "EnDBK", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.disableDeblk = !(iValue);

    if (VP_GetValue(fp, "LFCrossSliceBoundaryFlag", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.lfCrossSliceBoundaryEnable = iValue;

    if (VP_GetValue(fp, "BetaOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.betaOffsetDiv2 = iValue;

    if (VP_GetValue(fp, "TcOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.tcOffsetDiv2 = iValue;

    if (VP_GetValue(fp, "IntraTransSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.skipIntraTrans = iValue;

    if (VP_GetValue(fp, "EnSAO", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.saoEnable = iValue;

    if (VP_GetValue(fp, "IntraNxN", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraNxNEnable = iValue;

    if (VP_GetValue(fp, "RateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcEnable = iValue;

    if (VP_GetValue(fp, "EncBitrate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRate = iValue;

    if (VP_GetValue(fp, "EncBitrateBL", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRateBL = iValue;

    if (VP_GetValue(fp, "CULevelRateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cuLevelRCEnable = iValue;

    if (VP_GetValue(fp, "EnHvsQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.hvsQPEnable = iValue;

    if (VP_GetValue(fp, "HvsQpScaleDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.hvsQpScale = iValue;

    if (VP_GetValue(fp, "InitialDelay", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->VbvBufferSize = iValue;

    if (pEncCfg->VbvBufferSize == 0) {
        if (VP_GetValue(fp, "VbvBufferSize", &iValue) == 0)
            goto __end_parse;
        else
            pEncCfg->VbvBufferSize = iValue;
    }

    if (VP_GetValue(fp, "MinQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.minQp = iValue;

    if (VP_GetValue(fp, "MaxQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.maxQp = iValue;

    if (VP_GetValue(fp, "MaxDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.maxDeltaQp = iValue;

    if (VP_GetValue(fp, "GOPSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.gopParam.customGopSize = iValue;

    if (VP_GetValue(fp, "EnRoi", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.roiEnable = iValue;

    if (VP_GetValue(fp, "GopPreset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.gopPresetIdx = iValue;

    if (VP_GetValue(fp, "FrameSkip", &iValue) == 0)
        goto __end_parse;
    else
        frameSkip = iValue;

    if (VP_GetValue(fp, "NumUnitsInTick", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.numUnitsInTick = iValue;

    if (VP_GetValue(fp, "TimeScale", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.timeScale = iValue;

    if (VP_GetValue(fp, "NumTicksPocDiffOne", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.numTicksPocDiffOne = iValue;

    if (VP_GetValue(fp, "EncAUD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.encAUD = iValue;

    if (VP_GetValue(fp, "EncEOS", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.encEOS = iValue;

    if (VP_GetValue(fp, "EncEOB", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.encEOB = iValue;

    if (VP_GetValue(fp, "CbQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.chromaCbQpOffset = iValue;

    if (VP_GetValue(fp, "CrQpOffset", &iValue) == 0)
            goto __end_parse;
    else
        pEncCfg->vpCfg.chromaCrQpOffset = iValue;

    if (VP_GetValue(fp, "RcInitialQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.initialRcQp = iValue;

    if (VP_GetValue(fp, "EnNoiseReductionY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrYEnable = iValue;

    if (VP_GetValue(fp, "EnNoiseReductionCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrCbEnable = iValue;

    if (VP_GetValue(fp, "EnNoiseReductionCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrCrEnable = iValue;

    if (VP_GetValue(fp, "EnNoiseEst", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseEstEnable = iValue;

    if (VP_GetValue(fp, "NoiseSigmaY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseSigmaY = iValue;

    if (VP_GetValue(fp, "NoiseSigmaCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseSigmaCb = iValue;

    if (VP_GetValue(fp, "NoiseSigmaCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseSigmaCr = iValue;

    if (VP_GetValue(fp, "IntraNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrIntraWeightY = iValue;

    if (VP_GetValue(fp, "IntraNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrIntraWeightCb= iValue;

    if (VP_GetValue(fp, "IntraNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrIntraWeightCr = iValue;

    if (VP_GetValue(fp, "InterNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrInterWeightY = iValue;

    if (VP_GetValue(fp, "InterNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrInterWeightCb = iValue;

    if (VP_GetValue(fp, "InterNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrInterWeightCr = iValue;

    if (VP_GetValue(fp, "UseAsLongTermRefPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.useAsLongtermPeriod = iValue;

    if (VP_GetValue(fp, "RefLongTermPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.refLongtermPeriod = iValue;



    //GOP
    if (pEncCfg->vpCfg.intraPeriod == 1) {
        pEncCfg->vpCfg.gopParam.picParam[0].picType = PIC_TYPE_I;
        pEncCfg->vpCfg.gopParam.picParam[0].picQp = pEncCfg->vpCfg.intraQP;
        if (pEncCfg->vpCfg.gopParam.customGopSize > 1) {
            VLOG(ERR, "CFG file error : gop size should be smaller than 2 for all intra case\n");
            goto __end_parse;
        }
    }
    else {
        for (i = 0; pEncCfg->vpCfg.gopPresetIdx == PRESET_IDX_CUSTOM_GOP && i < pEncCfg->vpCfg.gopParam.customGopSize; i++) {

            sprintf(tempStr, "Frame%d", i+1);
            if (VP_GetStringValue(fp, tempStr, sValue) != 1) {
                VLOG(ERR, "CFG file error : %s value is not available. \n", tempStr);
                goto __end_parse;
            }

            if ( bitFormat == STD_AVC ) {
                if ( VP_AVCSetGOPInfo(sValue, &pEncCfg->vpCfg.gopParam.picParam[i], 0, pEncCfg->vpCfg.intraQP) != 1) {
                    VLOG(ERR, "CFG file error : %s value is not available. \n", tempStr);
                    goto __end_parse;
                }
            }
            else {
                if ( VP_SetGOPInfo(sValue, &pEncCfg->vpCfg.gopParam.picParam[i], 0, pEncCfg->vpCfg.intraQP) != 1) {
                    VLOG(ERR, "CFG file error : %s value is not available. \n", tempStr);
                    goto __end_parse;
                }
#if TEMP_SCALABLE_RC
                if ( (pEncCfg->vpCfg.gopParam.picParam[i].temporalId + 1) > MAX_NUM_TEMPORAL_LAYER) {
                    VLOG(ERR, "CFG file error : %s MaxTempLayer %d exceeds MAX_TEMP_LAYER(7). \n", tempStr, pEncCfg->vpCfg.gopParam.picParam[i].temporalId + 1);
                    goto __end_parse;
                }
#endif
            }
        }
    }

    //ROI
    if (pEncCfg->vpCfg.roiEnable) {
        sprintf(tempStr, "RoiFile");
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->vpCfg.roiFileName);
        }
    }

    if (pEncCfg->vpCfg.losslessEnable) {
        pEncCfg->vpCfg.disableDeblk = 1;
        pEncCfg->vpCfg.saoEnable = 0;
        pEncCfg->RcEnable = 0;
    }

    if (pEncCfg->vpCfg.roiEnable) {
        sprintf(tempStr, "RoiQpMapFile");
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->vpCfg.roiQpMapFile);
        }
    }

    /*======================================================*/
    /*          ONLY for H.264                              */
    /*======================================================*/
    if (VP_GetValue(fp, "IdrPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.idrPeriod = iValue;

    if (VP_GetValue(fp, "RdoSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.rdoSkip = iValue;

    if (VP_GetValue(fp, "LambdaScaling", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.lambdaScalingEnable = iValue;

    if (VP_GetValue(fp, "Transform8x8", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.transform8x8 = iValue;

    if (VP_GetValue(fp, "SliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.avcSliceMode = iValue;

    if (VP_GetValue(fp, "SliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.avcSliceArg = iValue;

    if (VP_GetValue(fp, "IntraMbRefreshMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraMbRefreshMode = iValue;

    if (VP_GetValue(fp, "IntraMbRefreshArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraMbRefreshArg = iValue;

    if (VP_GetValue(fp, "MBLevelRateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.mbLevelRc = iValue;

    if (VP_GetValue(fp, "CABAC", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.entropyCodingMode = iValue;

    // H.264 END



    if (VP_GetValue(fp, "EncMonochrome", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.monochromeEnable = iValue;

    if (VP_GetValue(fp, "StrongIntraSmoothing", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.strongIntraSmoothEnable = iValue;

    if (VP_GetValue(fp, "RoiAvgQP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.roiAvgQp = iValue;

    if (VP_GetValue(fp, "WeightedPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.weightPredEnable = iValue & 1;

    if (VP_GetValue(fp, "EnBgDetect", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgDetectEnable = iValue;

    if (VP_GetValue(fp, "BgThDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgThrDiff = iValue;

    if (VP_GetValue(fp, "S2fmeOff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.s2fmeDisable = iValue;

    if (VP_GetValue(fp, "BgThMeanDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgThrMeanDiff = iValue;

    if (VP_GetValue(fp, "BgLambdaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgLambdaQp = iValue;

    if (VP_GetValue(fp, "BgDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgDeltaQp = iValue;

    if (VP_GetValue(fp, "EnLambdaMap", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.customLambdaMapEnable = iValue;

    if (VP_GetValue(fp, "EnCustomLambda", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.customLambdaEnable = iValue;

    if (VP_GetValue(fp, "EnCustomMD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.customMDEnable = iValue;

    if (VP_GetValue(fp, "PU04DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04DeltaRate = iValue;

    if (VP_GetValue(fp, "PU08DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08DeltaRate = iValue;

    if (VP_GetValue(fp, "PU16DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16DeltaRate = iValue;

    if (VP_GetValue(fp, "PU32DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32DeltaRate = iValue;

    if (VP_GetValue(fp, "PU04IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU04IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU04IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "PU08IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU08IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU08IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "PU16IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU16IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU16IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "PU32IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU32IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU32IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "CU08IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu08IntraDeltaRate = iValue;

    if (VP_GetValue(fp, "CU08InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu08InterDeltaRate = iValue;

    if (VP_GetValue(fp, "CU08MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu08MergeDeltaRate = iValue;

    if (VP_GetValue(fp, "CU16IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu16IntraDeltaRate = iValue;

    if (VP_GetValue(fp, "CU16InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu16InterDeltaRate = iValue;

    if (VP_GetValue(fp, "CU16MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu16MergeDeltaRate = iValue;

    if (VP_GetValue(fp, "CU32IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu32IntraDeltaRate = iValue;

    if (VP_GetValue(fp, "CU32InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu32InterDeltaRate = iValue;

    if (VP_GetValue(fp, "CU32MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu32MergeDeltaRate = iValue;

    if (VP_GetValue(fp, "DisableCoefClear", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.coefClearDisable = iValue;

    if (VP_GetValue(fp, "EnModeMap", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.customModeMapFlag = iValue;

    if (VP_GetValue(fp, "ForcePicSkipStart", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.forcePicSkipStart = iValue;

    if (VP_GetValue(fp, "ForcePicSkipEnd", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.forcePicSkipEnd = iValue;

    if (VP_GetValue(fp, "ForceCoefDropStart", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.forceCoefDropStart = iValue;

    if (VP_GetValue(fp, "ForceCoefDropEnd", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.forceCoefDropEnd = iValue;

    if (VP_GetValue(fp, "RcWeightParaCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.rcWeightParam = iValue;

    if (VP_GetValue(fp, "RcWeightBufCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.rcWeightBuf = iValue;

    if (VP_GetValue(fp, "EnForcedIDRHeader", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.forcedIdrHeaderEnable = iValue;

    if (VP_GetValue(fp, "ForceIdrPicIdx", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.forceIdrPicIdx = iValue;

    // Scaling list
    if (pEncCfg->vpCfg.scalingListEnable) {
        sprintf(tempStr, "ScalingListFile");
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->vpCfg.scalingListFileName);
        }
    }
    // Custom Lambda
    if (pEncCfg->vpCfg.customLambdaEnable) {
        sprintf(tempStr, "CustomLambdaFile");
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->vpCfg.customLambdaFileName);
        }
    }


    // custom Lambda Map
    if (pEncCfg->vpCfg.customLambdaMapEnable) {
        sprintf(tempStr, "LambdaMapFile");
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->vpCfg.customLambdaMapFileName);
        }
    }

    // custom Lambda Map
    if (pEncCfg->vpCfg.customModeMapFlag) {
        sprintf(tempStr, "ModeMapFile");
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->vpCfg.customModeMapFileName);
        }
    }

    if (pEncCfg->vpCfg.weightPredEnable & 0x1) {
        sprintf(tempStr, "WpParamFile");
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->vpCfg.WpParamFileName);
        }
    }

#define NUM_MAX_PARAM_CHANGE    10
    for (i=0; i<NUM_MAX_PARAM_CHANGE; i++) {
        sprintf(tempStr, "SPCh%d", i+1);
        if (VP_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%d %x %s\n", &(pEncCfg->changeParam[i].setParaChgFrmNum), &(pEncCfg->changeParam[i].enableOption), pEncCfg->changeParam[i].cfgName);

        }
        else {
            pEncCfg->numChangeParam = i;
            break;
        }
    }
    ret = 1; /* Success */

__end_parse:
    osal_fclose(fp);
    return ret;
}

int parseVpChangeParamCfgFile(
    ENC_CFG *pEncCfg,
    char *FileName
    )
{
    osal_file_t fp;
    char sValue[256] = {0, };
    char tempStr[256] = {0, };
    int iValue = 0, ret = 0, i = 0;

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        VLOG(ERR, "file open err : %s, errno(%d)\n", FileName, errno);
        return ret;
    }

    if (VP_GetValue(fp, "ConstrainedIntraPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.constIntraPredFlag = iValue;

    // FixedBitRatio 0 ~ 7
    for (i=0; i<MAX_GOP_NUM; i++) {
        sprintf(tempStr, "FixedBitRatio%d", i);
        if (VP_GetStringValue(fp, tempStr, sValue) == 1) {
            iValue = atoi(sValue);
            if ( iValue >= vpCfgInfo[FIXED_BIT_RATIO].min && iValue <= vpCfgInfo[FIXED_BIT_RATIO].max )
                pEncCfg->vpCfg.fixedBitRatio[i] = iValue;
            else
                pEncCfg->vpCfg.fixedBitRatio[i] = vpCfgInfo[FIXED_BIT_RATIO].def;
        }
        else
            pEncCfg->vpCfg.fixedBitRatio[i] = vpCfgInfo[FIXED_BIT_RATIO].def;

    }

    if (VP_GetValue(fp, "IndeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.independSliceMode = iValue;

    if (VP_GetValue(fp, "IndeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.independSliceModeArg = iValue;

    if (VP_GetValue(fp, "DeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.dependSliceMode = iValue;

    if (VP_GetValue(fp, "DeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.dependSliceModeArg = iValue;

    if (VP_GetValue(fp, "MaxNumMerge", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.maxNumMerge = iValue;

    if (VP_GetValue(fp, "EnDBK", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.disableDeblk = !(iValue);

    if (VP_GetValue(fp, "LFCrossSliceBoundaryFlag", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.lfCrossSliceBoundaryEnable = iValue;

    if (VP_GetValue(fp, "DecodingRefreshType", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.decodingRefreshType = iValue;

    if (VP_GetValue(fp, "BetaOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.betaOffsetDiv2 = iValue;

    if (VP_GetValue(fp, "TcOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.tcOffsetDiv2 = iValue;

    if (VP_GetValue(fp, "IntraNxN", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraNxNEnable = iValue;

    if (VP_GetValue(fp, "QP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraQP = iValue;

    if (VP_GetValue(fp, "IntraPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.intraPeriod = iValue;

    if (VP_GetValue(fp, "EncBitrate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRate = iValue;

    if (VP_GetValue(fp, "EnHvsQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.hvsQPEnable = iValue;

    if (VP_GetValue(fp, "HvsQpScaleDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.hvsQpScale = iValue;

    if (VP_GetValue(fp, "InitialDelay", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->VbvBufferSize = iValue;

    if (pEncCfg->VbvBufferSize == 0) {
        if (VP_GetValue(fp, "VbvBufferSize", &iValue) == 0)
            goto __end_parse;
        else
            pEncCfg->VbvBufferSize = iValue;
    }

    if (VP_GetValue(fp, "MinQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.minQp = iValue;

    if (VP_GetValue(fp, "MaxQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.maxQp = iValue;

    if (VP_GetValue(fp, "MaxDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.maxDeltaQp = iValue;


    if (VP_GetValue(fp, "CbQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.chromaCbQpOffset = iValue;

    if (VP_GetValue(fp, "CrQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.chromaCrQpOffset = iValue;

    if (VP_GetValue(fp, "EnNoiseReductionY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrYEnable = iValue;

    if (VP_GetValue(fp, "EnNoiseReductionCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrCbEnable = iValue;

    if (VP_GetValue(fp, "EnNoiseReductionCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrCrEnable = iValue;

    if (VP_GetValue(fp, "EnNoiseEst", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseEstEnable = iValue;

    if (VP_GetValue(fp, "NoiseSigmaY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseSigmaY = iValue;

    if (VP_GetValue(fp, "NoiseSigmaCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseSigmaCb = iValue;

    if (VP_GetValue(fp, "NoiseSigmaCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrNoiseSigmaCr = iValue;

    if (VP_GetValue(fp, "IntraNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrIntraWeightY = iValue;

    if (VP_GetValue(fp, "IntraNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrIntraWeightCb= iValue;

    if (VP_GetValue(fp, "IntraNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrIntraWeightCr = iValue;

    if (VP_GetValue(fp, "InterNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrInterWeightY = iValue;

    if (VP_GetValue(fp, "InterNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrInterWeightCb = iValue;

    if (VP_GetValue(fp, "InterNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.nrInterWeightCr = iValue;


    /*======================================================*/
    /*          newly added for VP520                     */
    /*======================================================*/

    if (VP_GetValue(fp, "WeightedPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.weightPredEnable = iValue & 1;

    if (VP_GetValue(fp, "EnBgDetect", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgDetectEnable = iValue;

    if (VP_GetValue(fp, "BgThDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgThrDiff = iValue;

    if (VP_GetValue(fp, "S2fmeOff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.s2fmeDisable = iValue;

    if (VP_GetValue(fp, "BgThMeanDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgThrMeanDiff = iValue;

    if (VP_GetValue(fp, "BgLambdaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgLambdaQp = iValue;

    if (VP_GetValue(fp, "BgDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.bgDeltaQp = iValue;


    if (VP_GetValue(fp, "EnCustomLambda", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.customLambdaEnable = iValue;

    if (VP_GetValue(fp, "EnCustomMD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.customMDEnable = iValue;

    if (VP_GetValue(fp, "DisableCoefClear", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.coefClearDisable = iValue;

    if (VP_GetValue(fp, "PU04DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04DeltaRate = iValue;

    if (VP_GetValue(fp, "PU08DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08DeltaRate = iValue;

    if (VP_GetValue(fp, "PU16DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16DeltaRate = iValue;

    if (VP_GetValue(fp, "PU32DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32DeltaRate = iValue;

    if (VP_GetValue(fp, "PU04IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU04IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU04IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu04IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "PU08IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU08IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU08IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu08IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "PU16IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU16IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU16IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu16IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "PU32IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32IntraPlanarDeltaRate = iValue;

    if (VP_GetValue(fp, "PU32IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32IntraDcDeltaRate = iValue;

    if (VP_GetValue(fp, "PU32IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.pu32IntraAngleDeltaRate = iValue;

    if (VP_GetValue(fp, "CU08IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu08IntraDeltaRate = iValue;

    if (VP_GetValue(fp, "CU08InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu08InterDeltaRate = iValue;

    if (VP_GetValue(fp, "CU08MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu08MergeDeltaRate = iValue;

    if (VP_GetValue(fp, "CU16IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu16IntraDeltaRate = iValue;

    if (VP_GetValue(fp, "CU16InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu16InterDeltaRate = iValue;

    if (VP_GetValue(fp, "CU16MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu16MergeDeltaRate = iValue;

    if (VP_GetValue(fp, "CU32IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu32IntraDeltaRate = iValue;

    if (VP_GetValue(fp, "CU32InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu32InterDeltaRate = iValue;

    if (VP_GetValue(fp, "CU32MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.cu32MergeDeltaRate = iValue;

    /*======================================================*/
    /*          only for H.264 encoder                      */
    /*======================================================*/
    if (VP_GetValue(fp, "IdrPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.idrPeriod = iValue;

    if (VP_GetValue(fp, "Transform8x8", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.transform8x8 = iValue;

    if (VP_GetValue(fp, "SliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.avcSliceMode = iValue;

    if (VP_GetValue(fp, "SliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.avcSliceArg = iValue;

    if (VP_GetValue(fp, "CABAC", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.entropyCodingMode = iValue;

    if (VP_GetValue(fp, "RdoSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.rdoSkip = iValue;

    if (VP_GetValue(fp, "LambdaScaling", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->vpCfg.lambdaScalingEnable = iValue;

    ret = 1; /* Success */

__end_parse:
    osal_fclose(fp);
    return ret;
}
