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

/**
 * @brief aml_ipc_internal.h this header file includes all internal APIs used by IPC component
 * it is usually used by component developer, in most case application developper can ignore this
 * header file
 */

#include "aml_ipc_sdk.h"

#ifndef AML_IPC_INTERNAL_H

#define AML_IPC_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define AML_ALLOC_PRIV(priv) priv = calloc(1, sizeof(*priv))

// list head and list node for a generic structure
STAILQ_HEAD(AmlIPCListHead, AmlIPCListNode);
struct AmlIPCListNode {
    STAILQ_ENTRY(AmlIPCListNode) node;
};
// get container pointer from a member address
// struct MyStruct {
//      int x;
//      struct AmlIPCListNode linknode;
// };
// struct AmlIPCListNode * node = STAILQ_FIRST(..);
// struct MyStruct * mystruct = container_of(node, struct MyStruct, linknode);
#define container_of(ptr, type, member)                                                            \
    ({                                                                                             \
        const typeof(((type *)0)->member) *__mptr = (ptr);                                         \
        (type *)((char *)__mptr - offsetof(type, member));                                         \
    })


enum AmlIPCChannelDir {
    AML_CHANNEL_INPUT = (1 << 0),
    AML_CHANNEL_OUTPUT = (1 << 1),
    AML_CHANNEL_BOTH = AML_CHANNEL_INPUT | AML_CHANNEL_OUTPUT
};

#define AML_IPC_COMP_FLAG_FLUSH (1 << 0)
#define AML_IPC_COMP_FLAG_HANDLE_FRAME (1 << 1)
#define AML_IPC_COMP_FLAG_GET_FRAME (1 << 2)

/**
 * @brief get number of a component channel
 *
 * @param comp
 * @param type, can be AML_CHANNEL_INPUT, AML_CHANNEL_OUTPUT and AML_CHANNEL_BOTH
 *
 * @return
 */
int aml_ipc_num_channel(struct AmlIPCComponent *comp, enum AmlIPCChannelDir type);

/**
 * @brief add a channel in the component, this is usually called in a component
 *
 * @param comp, the component
 * @param ch, specify the channel ID, this ID is used in other API
 * @param type, can be AML_CHANNEL_INPUT or AML_CHANNEL_OUTPUT
 * @param name, channel name, can be NULL
 *
 * @return AML_STATUS_OK
 */
AmlStatus aml_ipc_add_channel(struct AmlIPCComponent *comp, int ch, enum AmlIPCChannelDir type,
                              const char *name);

AmlStatus aml_ipc_remove_channel(struct AmlIPCComponent *comp, int ch);

struct AmlIPCComponentChannelIterator {
    struct AmlIPCListNode *cur;
    int sync_seq;
};

/**
 * @brief iterator channel information
 *
 * @param comp
 * @param type, one of AML_CHANNEL_INPUT, AML_CHANNEL_OUTPUT and AML_CHANNEL_BOTH
 * @param iter, cur field must NULL initialize on the first iteration, and keeep unchange for
 *   further iteration
 * @param id, if not NULL, output the  channel ID
 * @param dir, if not NULL, output the channel dir
 * @param name, if not NULL, output the the channel name, the memory of the name is hold by the
 *   channel, do not write to it or free the memory
 *
 * @return AML_STATUS_OK, a valid channel information is written to *id,*dir,*name
 *   AML_STATUS_ITERATE_DONE, all components has iterated, no more
 *   AML_STATUS_ITERATE_UNSYNC, channels changed during the iteration
 */
AmlStatus aml_ipc_comp_iterate_channel(struct AmlIPCComponent *comp, enum AmlIPCChannelDir type,
                                       struct AmlIPCComponentChannelIterator *iter, int *id,
                                       enum AmlIPCChannelDir *dir, const char **name);
/**
 * @brief get channel information by specifing the channel index,
 *   channel index: [0, num channel -1], num channel can be obtained by aml_ipc_num_channel
 *
 * @param comp
 * @param id, input the channel index, output the channel ID
 * @param dir, input the channel type, output the channel dir
 * @param name, output the channel name
 *
 * @return
 */
AmlStatus aml_ipc_get_channel_by_index(struct AmlIPCComponent *comp, int *id,
                                       enum AmlIPCChannelDir *dir, const char **name);

AmlStatus aml_ipc_get_channel_by_name(struct AmlIPCComponent *comp, int *id,
                                      enum AmlIPCChannelDir *dir, const char *name);

AmlStatus aml_ipc_comp_flush(struct AmlIPCComponent *comp, bool wait);


struct AmlIPCComponentBindIterator {
    void *chan;
    void *cur;
    int sync_seq;
};

/**
 * @brief iterate all components bound to specified channel
 *
 * @param comp, specify the component to which the iterated component is bound
 * @param ch, channel ID, if the channel is an input channel, there'll be no more than one compnent
 *   output from *out, if it is output channel, there could be multiple components
 * @param iter, chan field must NULL initialize on the first iteration, and keeep unchange for
 *   further iteration
 * @param out, output the component pointer on each call
 * @param id, output the bound channel id
 * @param name, output the bound channel name
 *
 * @return AML_STATUS_OK, a none NULL pointer written to *out
 *   AML_STATUS_ITERATE_DONE, all components has iterated, no more
 *   AML_STATUS_ITERATE_UNSYNC, bind components changed during the iteration
 *
 */
AmlStatus aml_ipc_comp_iterate_bind(struct AmlIPCComponent *comp, int chan,
                                    struct AmlIPCComponentBindIterator *iter,
                                    struct AmlIPCComponent **out, int *id, const char **name);

#define AML_AUTO_VAR(cleanf) __attribute__((cleanup(cleanf)))

static inline void __auto_mutex_unlock__(void *p) { pthread_mutex_unlock(*(pthread_mutex_t **)p); }
#define AML_LOCK_GUARD(var, pmutex)                                                                \
    AML_AUTO_VAR(__auto_mutex_unlock__) pthread_mutex_t *var = pmutex;                             \
    pthread_mutex_lock(var)

struct AmlTypeClass *aml_obj_get_registerd_type();

extern struct AmlValueEnum aml_ipc_sdk_pixfmt_enum[];

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* end of include guard: AML_IPC_INTERNAL_H */
