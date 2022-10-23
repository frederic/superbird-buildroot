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
#include <pthread.h>

#ifndef AML_IPC_COMPONENT_H

#define AML_IPC_COMPONENT_H

#ifdef __cplusplus
extern "C" {
#endif

// SDK reserve channel ID: 0xFFFF0000 ~ 0xFFFFFFFF
// AML_DEFAULT_CHANNEL can be used to specify the default input/output channel, it is usually
// the first input/output channel
#define AML_DEFAULT_CHANNEL 0xFFFF0000

struct AmlIPCComponentPriv;
struct AmlIPCComponent {
    AML_OBJ_EXTENDS(AmlIPCComponent, AmlObject, AmlIPCComponentClass);
    struct AmlIPCComponentPriv *priv;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    TAILQ_ENTRY(AmlIPCComponent) next; // link to next component in the same pipeline
    uint32_t flags;
};

struct AmlIPCComponentClass {
    struct AmlObjectClass parent_class;
    AmlStatus (*get_frame)(struct AmlIPCComponent *self, int ch, int timeout_us,
                           struct AmlIPCFrame **out);
    AmlStatus (*handle_frame)(struct AmlIPCComponent *self, int ch, struct AmlIPCFrame *frame);
    AmlStatus (*start)(struct AmlIPCComponent *self);
    AmlStatus (*stop)(struct AmlIPCComponent *self);
};
AML_OBJ_DECLARE_TYPEID(AmlIPCComponent, AmlObject, AmlIPCComponentClass);
#define AML_IPC_COMPONENT(obj) ((struct AmlIPCComponent *)(obj))

/**
 * @brief send frame to this component (intput channel) or downstream components (output channel)
 *
 * @param comp
 * @param ch, channel ID
 *   for input channel, the frame will send to the component itself (comp)
 *   for output channel, the frame will be sent to downstream components bound to the channel
 *   AML_DEFAULT_CHANNEL can be used to specify the default input channel
 * @param frame, frame data to send, callee will take owner ship, caller shall not touch the frame
 *
 * @return, return the result
 */
AmlStatus aml_ipc_send_frame(struct AmlIPCComponent *comp, int ch, struct AmlIPCFrame *frame);

/**
 * @brief get frame from a component or from upstream component
 *
 * @param comp
 * @param ch, channel ID
 *   for input channel, it will get frame from upstream component bound to the channel
 *   for output channel, it will get frame from this component (comp)
 *   AML_DEFAULT_CHANNEL can be used to specify the default output channel
 * @param timeout_us, timeout for the frame
 *    0 : do not wait for the frame
 *    -1: wait the frame for ever (will not return until frame is ready, or component stop)
 *    >0: timeout in microseconds (1/1000000 sec)
 * @param out, output the frame address, caller shall free the frame when done with it
 *
 * @return, return the result
 */
AmlStatus aml_ipc_get_frame(struct AmlIPCComponent *comp, int ch, int timeout_us,
                            struct AmlIPCFrame **out);

/**
 * @brief hook the frames on a channel, this will callback for every frame flow on the channel
 *
 * @param comp
 * @param ch, channel ID
 *   AML_DEFAULT_CHANNEL can be used to specify the default output channel
 * @param param, user provided pointer which will pass to the callback
 * @param hook, user provided callback, it will be called when the frame flows on the channel,
 *   the callback takes two parameter: frame pointer and user provided pointer, the frame
 *   pointer is only valid in the callback, if you need to store the pointer and use it otherwhere,
 *   use aml_obj_addref, and don't forget to call aml_obj_release when done.
 *   callback is called in the streaming thread, it shall return as soon as possible, else it may
 *   block the streaming thread, which may cause frame dropping.
 * if the callback return AML_STATUS_HOOK_CONTINUE, the frame will continue to handle by other hooks
 * and the bound components, returning other values will stop further processing on the frame.
 *
 * @return
 */
AmlStatus aml_ipc_add_frame_hook(struct AmlIPCComponent *comp, int ch, void *param,
                                 AmlStatus (*callback)(struct AmlIPCFrame *, void *));
AmlStatus aml_ipc_remove_frame_hook(struct AmlIPCComponent *comp, int ch, void *param,
                                    AmlStatus (*callback)(struct AmlIPCFrame *, void *));

/**
 * @brief watch the frame on a channel, this is similar to frame hook, except that watch callback is
 * called earlier than hook, and watch does not return any value, watch callback shall do as few
 * things as possible
 *
 * @param comp
 * @param ch
 * @param param
 * @param watch
 *
 * @return
 */
AmlStatus aml_ipc_add_frame_watch(struct AmlIPCComponent *comp, int ch, void *param,
                                  void (*watch)(struct AmlIPCFrame *, void *));
AmlStatus aml_ipc_remove_frame_watch(struct AmlIPCComponent *comp, int ch, void *param,
                                     void (*watch)(struct AmlIPCFrame *, void *));

/**
 * @brief bind two channel and let data flow between the components (via the bind channels)
 *   An output channel can bind to more than one input channels, in such case all input channels
 *   will receive the data
 *
 * @param provider, data provider component
 * @param output, output channel ID of the provider
 *   AML_DEFAULT_CHANNEL can be used to specify the default output channel
 * @param receiver, data receiver component
 * @param input, input channel ID of the receiver
 *   AML_DEFAULT_CHANNEL can be used to specify the default input channel
 *
 * @return
 */
AmlStatus aml_ipc_bind(struct AmlIPCComponent *provider, int output,
                       struct AmlIPCComponent *receiver, int input);

/**
 * @brief unbind the receiver channel from it's provider
 *
 * @param receiver, data receive component
 * @param input, input channel ID of the receiver
 *   AML_DEFAULT_CHANNEL can be used to specify the default input channel
 *
 * @return
 */
AmlStatus aml_ipc_unbind(struct AmlIPCComponent *receiver, int input);

/**
 * @brief start producing data
 *
 * @param comp, usually an output component (isp,ain,queue)
 *
 * @return
 */
AmlStatus aml_ipc_start(struct AmlIPCComponent *comp);
AmlStatus aml_ipc_stop(struct AmlIPCComponent *comp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_COMPONENT_H */
