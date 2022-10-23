/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "aml_ipc_define.h"
#include "aml_ipc_ge2d.h"
#include "aml_ipc_isp.h"
#include "aml_ipc_log.h"
#include "aml_ipc_osd.h"
#include "aml_ipc_parse.h"
#include "aml_ipc_queue.h"
#include "aml_ipc_vbpool.h"
#include "aml_ipc_venc.h"
#include "aml_ipc_gdc.h"
#include "aml_ipc_ain.h"
#include "aml_ipc_aout.h"
#include "aml_ipc_acodec.h"
#include "aml_ipc_filein.h"
#include "aml_ipc_fileout.h"
#include "aml_ipc_pipeline.h"
#include "aml_ipc_aconvert.h"
#include "aml_ipc_aprocess.h"
#include "aml_ipc_jpegenc.h"
#include "aml_ipc_rawvideoparse.h"

#ifndef AML_IPC_SDK_H

#define AML_IPC_SDK_H

#ifdef __cplusplus
extern "C" {
#endif

#define AML_IPC_SDK_VERSION "0.0.1"

extern const char *aml_ipc_sdk_version;

AmlStatus aml_ipc_sdk_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_SDK_H */
