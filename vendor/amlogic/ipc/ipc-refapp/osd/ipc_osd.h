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

#ifndef __IPC_OSD_H__
#define __IPC_OSD_H__

#include <aml_ipc_component.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "ipc_osd_clock.h"
#include "ipc_osd_text.h"
#include "ipc_osd_image.h"
#include "ipc_osd_nn.h"

enum AmlIPCOSDAppChannel {
  AML_OSD_APP_INPUT,
  AML_OSD_APP_INTERNAL,
  AML_OSD_APP_OUTPUT
};

enum AmlIPCOSDAppRefreshMode {
  AML_OSD_APP_REFRESH_GLOBAL,
  AML_OSD_APP_REFRESH_PARTIAL,
};

struct AmlIPCOSDModule;
struct AmlIPCOSDAppPriv;
struct AmlIPCOSDApp {
  AML_OBJ_EXTENDS(AmlIPCOSDApp, AmlIPCComponent, AmlIPCComponentClass);
  struct AmlIPCOSDAppPriv *priv;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCOSDApp, AmlIPCComponent, AmlIPCComponentClass);
#define AML_IPC_OSD_APP(obj) ((struct AmlIPCOSDApp *)(obj))

struct AmlIPCOSDApp *aml_ipc_osd_app_new();
AmlStatus aml_ipc_osd_app_set_refresh_mode(struct AmlIPCOSDApp *osd,
                                           enum AmlIPCOSDAppRefreshMode mode);
AmlStatus aml_ipc_osd_app_add_module(struct AmlIPCOSDApp *osd,
                                     struct AmlIPCOSDModule *module);
AmlStatus aml_ipc_osd_app_remove_module(struct AmlIPCOSDApp *osd,
                                        struct AmlIPCOSDModule *module);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IPC_OSD_H__ */
