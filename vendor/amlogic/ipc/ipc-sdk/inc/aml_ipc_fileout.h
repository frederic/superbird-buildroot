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
#ifndef AML_IPC_FILEOUT_H

#define AML_IPC_FILEOUT_H

#ifdef __cplusplus
extern "C" {
#endif

enum AmlFileOutChannel { AML_FILEOUT_INPUT };
enum AmlFileOutFormat { AML_FILEOUT_FMT_RAW, AML_FILEOUT_FMT_AML };
struct AmlIPCFileOutPriv;
struct AmlIPCFileOut {
    AML_OBJ_EXTENDS(AmlIPCFileOut, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCFileOutPriv *priv;
    char *file_path;
    int frames_to_skip;
    int frames_to_dump;
    enum AmlFileOutFormat format;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCFileOut, AmlIPCComponent, AmlIPCComponentClass);

/**
 * @brief set out file path
 *
 * @param fo
 * @param path
 *
 * @return
 */
AmlStatus aml_ipc_fileout_set_path(struct AmlIPCFileOut *fo, char *path);

/**
 * @brief set numbers of frames to skip
 *
 * @param fo
 * @param num
 *
 * @return
 */
AmlStatus aml_ipc_fileout_set_skip(struct AmlIPCFileOut *fo, int num);

/**
 * @brief set numbers of frames to dump, -1:dump all
 *
 * @param fo
 * @param num
 *
 * @return
 */
AmlStatus aml_ipc_fileout_set_dump(struct AmlIPCFileOut *fo, int num);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_FILEOUT_H */
