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

#ifndef AML_IPC_QUEUE_H
#define AML_IPC_QUEUE_H

#include "aml_ipc_component.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AmlIPCQueueChannel { AML_QUEUE_INPUT, AML_QUEUE_OUTPUT };

struct AmlIPCQueue {
    AML_OBJ_EXTENDS(AmlIPCQueue, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCFrame **buffer;
    pthread_t tid;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    volatile int started;
    volatile int flush;
    volatile int lock_count;

    int buffer_rp;
    int buffer_wp;
    int buffer_num;
    int buffer_num_alloc;
    int active;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCQueue, AmlIPCComponent, AmlIPCComponentClass);

struct AmlIPCQueue *aml_ipc_queue_new(int numbuffer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_QUEUE_H */
