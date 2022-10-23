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
#include "cnm_app.h"
#include "misc/bw_monitor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct EncListenerContext {
    Uint32      coreIdx;
    Comparator  es;
    BOOL        match;
    BOOL        matchOtherInfo;
    EndianMode  streamEndian;
    Uint32      reconWidth;
    Uint32      reconHeight;

    /* performance & bandwidth */
    BOOL        performance;
    BOOL        bandwidth;
    PFCtx       pfCtx;
    BWCtx*      bwCtx;
    Uint32      fps;
    Uint32      pfClock;
    char        cfgFileName[MAX_FILE_PATH];
    BOOL        headerEncDone[MAX_NUM_INSTANCE];
    BOOL        ringBufferEnable;
    BOOL        ringBufferWrapEnable;
} EncListenerContext;


void EncoderListener(
    Component   com,
    Uint64      event,
    void*       data,
    void*       context
    );

BOOL SetupEncListenerContext(
    EncListenerContext* ctx,
    CNMComponentConfig* config
    );

void ClearEncListenerContext(
    EncListenerContext* ctx
    );

void HandleEncFullEvent(
    Component com,
    CNMComListenerEncFull* param,
    EncListenerContext* ctx
    );

void HandleEncGetOutputEvent(
    Component               com,
    CNMComListenerEncDone*  param,
    EncListenerContext*     ctx
    );

void HandleEncCompleteSeqEvent(
    Component                       com,
    CNMComListenerEncCompleteSeq*   param,
    EncListenerContext*             ctx
    );

void HandleEncGetEncCloseEvent(
    Component               com,
    CNMComListenerEncClose* param,
    EncListenerContext*     ctx
    );
#ifdef __cplusplus
}
#endif /* __cplusplus */

