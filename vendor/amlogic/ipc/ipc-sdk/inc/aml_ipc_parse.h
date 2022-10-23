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
#ifndef AML_IPC_PARSE_H

#define AML_IPC_PARSE_H

#ifdef __cplusplus
extern "C" {
#endif

enum JSONValueType {
    JSON_TYPE_UNKNOWN,
    JSON_TYPE_NULL,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_DOUBLE,
    JSON_TYPE_INT,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY,
    JSON_TYPE_STRING
};

struct JSONValueSimple {
    const char *start;
    const char *end;
};

struct JSONValue {
    struct JSONValue *next;
    struct JSONValue *parent;
    enum JSONValueType val_type;
    union {
        struct JSONValueSimple val_text;
        struct {
            struct JSONValue *val_list;
            struct JSONValue *val_last;
        };
    };
    union {
        int num_child;  // for array
        int val_state;  // for object internal use
    };
};

//AmlStatus aml_ipc_parse_launch(struct AmlIPCComponent **comp, const char *str);
struct AmlIPCPipeline *aml_ipc_parse_launch(const char *str);
AmlStatus aml_ipc_parse_json(const char *str, struct JSONValue **outval);
int aml_ipc_get_json_value_string(struct JSONValue *val, char *buf, int len);
void aml_ipc_free_json_value(struct JSONValue *val);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* end of include guard: AML_IPC_PARSE_H */
