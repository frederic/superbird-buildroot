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
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "vpuapifunc.h"
#include "main_helper.h"

#define MAX_FEEDING_SIZE        0x400000        /* 4MBytes */

typedef struct FeederEsContext {
    osal_file_t fp;
    Uint32      feedingSize;
    BOOL        eos;
} FeederEsContext;

void* BSFeederSizePlusEs_Create(
    const char* path,
    CodStd      codecId
    )
{
    osal_file_t     fp = NULL;
    FeederEsContext*  context=NULL;

    UNREFERENCED_PARAMETER(codecId);

    if ((fp=osal_fopen(path, "rb")) == NULL) {
        VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, path);
        return NULL;
    }

    context = (FeederEsContext*)osal_malloc(sizeof(FeederEsContext));
    if (context == NULL) {
        VLOG(ERR, "%s:%d failed to allocate memory\n", __FUNCTION__, __LINE__);
        osal_fclose(fp);
        return NULL;
    }

    context->fp          = fp;
    context->feedingSize = 0;
    context->eos         = FALSE;

    return (void*)context;
}

BOOL BSFeederSizePlusEs_Destroy(
    void* feeder
    )
{
    FeederEsContext* context = (FeederEsContext*)feeder;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (context->fp) 
        osal_fclose(context->fp);

    osal_free(context);

    return TRUE;
}

Int32 BSFeederSizePlusEs_Act(
    void*       feeder,
    BSChunk*    chunk
    )
{
    FeederEsContext*  context = (FeederEsContext*)feeder;
    size_t          nRead;
    Uint32          chunkSize = 0;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (context->eos == TRUE) {
        return 0;
    }

    osal_fread(&chunkSize, 1, 4, context->fp);

    nRead = osal_fread(chunk->data, 1, chunkSize, context->fp);
    if ((Int32)nRead < 0) {
        VLOG(ERR, "%s:%d failed to read bitstream(errno: %d)\n", __FUNCTION__, __LINE__, errno);
        return 0;
    } 
    else if (nRead < chunkSize) {
        context->eos = TRUE;
    }
    chunk->size = chunkSize;

    return nRead;
}

BOOL BSFeederSizePlusEs_Rewind(
    void*       feeder
    )
{
    FeederEsContext*  context = (FeederEsContext*)feeder;
    Int32           ret;

    if ((ret=osal_fseek(context->fp, 0, SEEK_SET)) != 0) {
        VLOG(ERR, "%s:%d failed osal_fseek(ret:%d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    return TRUE;
}

