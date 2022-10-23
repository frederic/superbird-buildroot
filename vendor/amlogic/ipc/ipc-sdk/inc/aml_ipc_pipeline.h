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

#include "aml_ipc_component.h"

#ifndef AML_IPC_PIPELINE_H

#define AML_IPC_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

struct AmlIPCPipelinePriv;
struct AmlIPCPipeline {
    AML_OBJ_EXTENDS(AmlIPCPipeline, AmlIPCComponent, AmlIPCComponentClass);
    TAILQ_HEAD(AmlIPCComponentListHead, AmlIPCComponent) children;
    struct AmlIPCPipelinePriv *priv;
    int sync_seq;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCPipeline, AmlIPCComponent, AmlIPCComponentClass);

struct AmlIPCPipelineIterator {
    struct AmlIPCComponent *last;
    int sync_seq;
};

struct AmlIPCPipeline *aml_ipc_pipeline_new();

/**
 * @brief add an component to the pipeline
 *
 * @param pipeline
 * @param comp, refcount remains unchange after the call
 *
 * @return
 */
AmlStatus aml_ipc_pipeline_add(struct AmlIPCPipeline *pipeline, struct AmlIPCComponent *comp);

/**
 * @brief add many components to the pipeline
 *
 * @param pipeline
 * @param comp
 * @param ...
 *
 * @return
 */
AmlStatus aml_ipc_pipeline_add_many(struct AmlIPCPipeline *pipeline, struct AmlIPCComponent *comp,
                                    ...);

/**
 * @brief remove the component from the pipeline
 *
 * @param pipeline
 * @param comp, refcount remains unchange after the call
 *
 * @return
 */
AmlStatus aml_ipc_pipeline_remove(struct AmlIPCPipeline *pipeline, struct AmlIPCComponent *comp);

/**
 * @brief iterate all component in the pipeline
 *
 * @param pipeline
 * @param comp, output the compnent pointer on each call
 * @param iter, pointer to 0 initialized AmlIPCPipelineIterator
 *
 *   struct AmlIPCComponent *comp;
 *   struct AmlIPCPipelineIterator iter = {NULL};
 *   while (aml_ipc_pipeline_iterate(pipeline,&comp,&iter)==AML_STATUS_OK) {
 *     // do something with *comp
 *   }
 *
 * @return AML_STATUS_OK if a valid AmlIPCComponent output from *comp
 */
AmlStatus aml_ipc_pipeline_iterate(struct AmlIPCPipeline *pipeline,
                                   struct AmlIPCPipelineIterator *iter,
                                   struct AmlIPCComponent **comp);

AmlStatus aml_ipc_pipeline_dump_to_markdown(struct AmlIPCPipeline *pipeline, FILE * fp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_PIPELINE_H */
