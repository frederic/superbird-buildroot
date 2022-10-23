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

#ifndef AML_IPC_RAWVIDEOPARSE_H

#define AML_IPC_RAWVIDEOPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlRawVideoParseChannel {
    AML_RAWVIDEOPARSE_INPUT,
    AML_RAWVIDEOPARSE_OUTPUT,
    AML_RAWVIDEOPARSE_MAX
};

struct AmlIPCRawVideoParse {
    AML_OBJ_EXTENDS(AmlIPCRawVideoParse, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCRawVideoParsePriv *priv;
    struct AmlIPCVideoFormat format;
    int pitch;
    int is_live;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCRawVideoParse, AmlIPCComponent, AmlIPCComponentClass);

/**
 * @brief create rawvideoparse component
 * @return
 */
struct AmlIPCRawVideoParse *aml_ipc_rawvideoparse_new(struct AmlIPCVideoFormat *format);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
