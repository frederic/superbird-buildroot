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
#ifndef __VP5_FUNCTION_H__
#define __VP5_FUNCTION_H__

#include "vpuapi.h"
#include "product.h"

#define VP5_TEMPBUF_OFFSET                (1024*1024)
#define VP5_TEMPBUF_SIZE                  (1024*1024)
#define VP5_TASK_BUF_OFFSET               (2*1024*1024)   // common mem = | codebuf(1M) | tempBuf(1M) | taskbuf0x0 ~ 0xF |

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VP5_SUBSAMPLED_ONE_SIZE(_w, _h)       ((((_w/4)+15)&~15)*(((_h/4)+7)&~7))
#define VP5_SUBSAMPLED_ONE_SIZE_AVC(_w, _h)   ((((_w/4)+31)&~31)*(((_h/4)+3)&~3))

#define BSOPTION_ENABLE_EXPLICIT_END        (1<<0)

#define WTL_RIGHT_JUSTIFIED          0
#define WTL_LEFT_JUSTIFIED           1
#define WTL_PIXEL_8BIT               0
#define WTL_PIXEL_16BIT              1
#define WTL_PIXEL_32BIT              2

extern Uint32 Vp5VpuIsInit(
    Uint32      coreIdx
    );

extern Int32 Vp5VpuIsBusy(
    Uint32 coreIdx
    );

extern Int32 VpVpuGetProductId(
    Uint32      coreIdx
    );

extern RetCode Vp5VpuEncGiveCommand(
    CodecInst *pCodecInst,
    CodecCommand cmd,
    void *param
    );

extern void Vp5BitIssueCommand(
    CodecInst* instance,
    Uint32 cmd
    );

extern RetCode Vp5VpuGetVersion(
    Uint32  coreIdx,
    Uint32* versionInfo,
    Uint32* revision
    );

extern RetCode Vp5VpuInit(
    Uint32      coreIdx,
    void*       firmware,
    Uint32      size
    );

extern RetCode Vp5VpuSleepWake(
    Uint32 coreIdx,
    int iSleepWake,
    const Uint16* code,
    Uint32 size
    );

extern RetCode Vp5VpuReset(
    Uint32 coreIdx,
    SWResetMode resetMode
    );

extern RetCode Vp5VpuReInit(
    Uint32 coreIdx,
    void* firmware,
    Uint32 size
    );

extern Int32 Vp5VpuWaitInterrupt(
    CodecInst*  instance,
    Int32       timeout,
    BOOL        pending
    );

extern RetCode Vp5VpuClearInterrupt(
    Uint32 coreIdx,
    Uint32 flags
    );

extern RetCode Vp5VpuGetBwReport(
    CodecInst*  instance,
    VPUBWData*  bwMon
    );


extern RetCode Vp5VpuGetDebugInfo(
    CodecInst* instance,
    VPUDebugInfo* info
    );

/***< VP5 Encoder >******/
RetCode Vp5VpuEncUpdateBS(
    CodecInst* instance,
    BOOL updateNewBsbuf
    );

RetCode Vp5VpuEncGetRdWrPtr(CodecInst* instance,
    PhysicalAddress *rdPtr,
    PhysicalAddress *wrPtr
    );

extern RetCode Vp5VpuBuildUpEncParam(
    CodecInst* instance,
    EncOpenParam* param
    );

extern RetCode Vp5VpuEncInitSeq(
    CodecInst*instance
    );

extern RetCode Vp5VpuEncGetSeqInfo(
    CodecInst* instance,
    EncInitialInfo* info
    );


extern RetCode Vp5VpuEncRegisterFramebuffer(
    CodecInst* inst,
    FrameBuffer* fbArr,
    TiledMapType mapType,
    Uint32 count
    );

extern RetCode Vp5EncWriteProtect(
    CodecInst* instance
    );

extern RetCode Vp5VpuEncode(
    CodecInst* instance,
    EncParam* option
    );

extern RetCode Vp5VpuEncGetResult(
    CodecInst* instance,
    EncOutputInfo* result
    );

extern RetCode Vp5VpuEncGetHeader(
    EncHandle instance,
    EncHeaderParam * encHeaderParam
    );

extern RetCode Vp5VpuEncFiniSeq(
    CodecInst*  instance
    );

extern RetCode Vp5VpuEncParaChange(
    EncHandle instance,
    EncChangeParam* param
    );

extern RetCode CheckEncCommonParamValid(
    EncOpenParam* pop
    );

extern RetCode CheckEncRcParamValid(
    EncOpenParam* pop
    );

extern RetCode CheckEncCustomGopParamValid(
    EncOpenParam* pop
    );

extern RetCode Vp5VpuGetSrcBufFlag(
    CodecInst* instance,
    Uint32* flag
    );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VP5_FUNCTION_H__ */

