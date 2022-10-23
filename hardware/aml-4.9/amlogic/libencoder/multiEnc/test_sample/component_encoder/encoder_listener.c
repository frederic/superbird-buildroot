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
#include "encoder_listener.h"
#include "misc/debug.h"
#include "misc/bw_monitor.h"



static void HandleEncHandlingIntEvent(Component com, CNMComListenerHandlingInt* param, EncListenerContext* ctx)
{
    if (ctx->bwCtx != NULL) {
        BWMonitorUpdate(ctx->bwCtx, 1);
    }
}

void HandleEncFullEvent(Component com, CNMComListenerEncFull* param, EncListenerContext* ctx)
{
}

void HandleEncGetEncCloseEvent(Component com, CNMComListenerEncClose* param, EncListenerContext* ctx)
{
    if (ctx->pfCtx != NULL) {
        PFMonitorRelease(ctx->pfCtx);
    }
    if (ctx->bwCtx != NULL) {
        BWMonitorRelease(ctx->bwCtx);
    }
}

void HandleEncCompleteSeqEvent(Component com, CNMComListenerEncCompleteSeq* param, EncListenerContext* ctx)
{
    if (ctx->performance == TRUE) {
        Uint32 fps = (ctx->fps == 0) ? 30 : ctx->fps;
        ctx->pfCtx = PFMonitorSetup(ctx->coreIdx, 0, ctx->pfClock, fps, GetBasename((const char *)ctx->cfgFileName), 1);
    }
    if (ctx->bandwidth == TRUE) {
        ctx->bwCtx = BWMonitorSetup(param->handle, TRUE, GetBasename((const char *)ctx->cfgFileName));
    }
}

void HandleEncGetOutputEvent(Component com, CNMComListenerEncDone* param, EncListenerContext* ctx)
{
    EncOutputInfo* output = param->output;
    if (output->reconFrameIndex == RECON_IDX_FLAG_ENC_END)
        return;


    if (ctx->pfCtx != NULL) {
        PFMonitorUpdate(ctx->coreIdx, ctx->pfCtx, output->frameCycle, output->encPrepareEndTick - output->encPrepareStartTick,
            output->encProcessingEndTick - output->encProcessingStartTick, output->encEncodeEndTick- output->encEncodeStartTick);
    }
    if (ctx->bwCtx != NULL) {
        BWMonitorUpdatePrint(ctx->bwCtx, output->picType);
    }

    if (ctx->headerEncDone[param->handle->instIndex] == FALSE) {
        ctx->headerEncDone[param->handle->instIndex] = TRUE;
    }

    if (ctx->match == FALSE) CNMAppStop();
}

void EncoderListener(Component com, Uint64 event, void* data, void* context)
{
    int         productId;
    EncHandle   handle;
    int         key=0;

    if (osal_kbhit()) {
        key = osal_getch();
        osal_flush_ch();
        if (key) {
            if (key == 'q' || key == 'Q') {
                CNMAppStop();
                return;
            }
        }
    }
    switch (event) {
    case COMPONENT_EVENT_ENC_OPEN:
        break;
    case COMPONENT_EVENT_ENC_ISSUE_SEQ:
        break;
    case COMPONENT_EVENT_ENC_COMPLETE_SEQ:
        HandleEncCompleteSeqEvent(com, (CNMComListenerEncCompleteSeq*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_REGISTER_FB:
        break;
    case COMPONENT_EVENT_ENC_READY_ONE_FRAME:
        break;
    case COMPONENT_EVENT_ENC_START_ONE_FRAME:
        break;
    case COMPONENT_EVENT_ENC_HANDLING_INT:
        HandleEncHandlingIntEvent(com, (CNMComListenerHandlingInt*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_GET_OUTPUT_INFO:
        handle = ((CNMComListenerEncDone*)data)->handle;
        productId = VPU_GetProductId(VPU_HANDLE_CORE_INDEX(handle));
        if (TRUE == PRODUCT_ID_VP_SERIES(productId)) {
            HandleEncGetOutputEvent(com, (CNMComListenerEncDone*)data, (EncListenerContext*)context);
        }
        break;
    case COMPONENT_EVENT_ENC_ENCODED_ALL:
        break;
    case COMPONENT_EVENT_ENC_CLOSE:
        HandleEncGetEncCloseEvent(com, (CNMComListenerEncClose*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_FULL_INTERRUPT:
        HandleEncFullEvent(com, (CNMComListenerEncFull*)data, (EncListenerContext*)context);
        break;
    default:
        break;
    }
}

BOOL SetupEncListenerContext(EncListenerContext* ctx, CNMComponentConfig* config)
{
    TestEncConfig* encConfig = &config->testEncConfig;

    osal_memset((void*)ctx, 0x00, sizeof(EncListenerContext));

    if (encConfig->compareType & (1 << MODE_COMP_ENCODED)) {
        if ((ctx->es=Comparator_Create(STREAM_COMPARE, encConfig->ref_stream_path, encConfig->cfgFileName)) == NULL) {
            VLOG(ERR, "%s:%d Failed to Comparator_Create(%s)\n", __FUNCTION__, __LINE__, encConfig->ref_stream_path);
            return FALSE;
        }
    }
    ctx->coreIdx       = encConfig->coreIdx;
    ctx->streamEndian  = encConfig->stream_endian;
    ctx->match         = TRUE;
    ctx->matchOtherInfo= TRUE;
    ctx->performance   = encConfig->performance;
    ctx->bandwidth     = encConfig->bandwidth;
    ctx->pfClock       = encConfig->pfClock;
    osal_memcpy(ctx->cfgFileName, encConfig->cfgFileName, sizeof(ctx->cfgFileName));
    ctx->ringBufferEnable     = encConfig->ringBufferEnable;
    ctx->ringBufferWrapEnable = encConfig->ringBufferWrapEnable;

    return TRUE;
}

void ClearEncListenerContext(EncListenerContext* ctx)
{
    if (ctx->es)    Comparator_Destroy(ctx->es);
}

