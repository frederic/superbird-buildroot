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

#ifndef __IPC_TIMEOUT_TASK_H__
#define __IPC_TIMEOUT_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define IPC_TIMEOUT_TASK_CONTINUE true
#define IPC_TIMEOUT_TASK_STOP false

typedef bool (*IPCTimeoutCallback)(void *user_data);
unsigned long ipc_timeout_task_add(unsigned int interval,
                                   IPCTimeoutCallback function, void *data);
unsigned long ipc_onetime_task_add(unsigned int interval,
                                   IPCTimeoutCallback function, void *data);

void ipc_timeout_task_remove(unsigned long timer_id);

void ipc_timeout_task_init();
void ipc_timeout_task_deinit();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IPC_TIMEOUT_TASK_H__ */
