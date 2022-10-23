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

#ifndef _LIBIPC_PROPERTY_PRIV_H_
#define _LIBIPC_PROPERTY_PRIV_H_

#include <cstddef>

#define IPC_PROPERTY_SERVICE_CMD_SOCKET "/tmp/ipc.property.cmd.socket"
#define IPC_PROPERTY_SERVICE_EVENT_SOCKET "/tmp/ipc.property.event.socket"

enum ipc_prop_cmds {
  IPC_PROP_MSG_SETPROP = 1,
  IPC_PROP_MSG_GETPROP,
  IPC_PROP_MSG_REMOVEPROP,
  IPC_PROP_MSG_GETPROP_ARRAY_SIZE,
  IPC_PROP_MSG_LISTPROP,
};

enum ipc_prop_events {
  IPC_PROP_EVENT_CHANGED = 1,
  IPC_PROP_EVENT_PROC_EXIT=0xff,
};

struct ipc_prop_msg {
  unsigned int cmd;
  size_t keysz;
  size_t valsz;
  char buf[0];
};


#endif /* _LIBIPC_PROPERTY_PRIV_H_ */
