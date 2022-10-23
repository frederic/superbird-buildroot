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

#ifndef AML_IPC_GDC_H

#define AML_IPC_GDC_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlGDCChannel { AML_GDC_CH0, AML_GDC_CH1, AML_GDC_OUTPUT };
struct AmlIPCGDCPriv;
struct AmlIPCGDC {
    AML_OBJ_EXTENDS(AmlIPCGDC, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCGDCPriv *priv;
    char *config_file;
    int pass_through;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCGDC, AmlIPCComponent, AmlIPCComponentClass);

struct AmlIPCGDC *aml_ipc_gdc_new();

AmlStatus aml_ipc_gdc_set_passthrough(struct AmlIPCGDC *gdc, bool passthrough);
AmlStatus aml_ipc_gdc_set_configfile(struct AmlIPCGDC *gdc, const char *config);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_GDC_H */
