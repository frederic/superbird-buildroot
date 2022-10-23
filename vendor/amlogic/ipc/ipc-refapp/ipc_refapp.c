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
#include <aml_ipc_sdk.h>
#include <gst/gst.h>

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_timeout_task.h"
#include "ipc_pipeline.h"

bool check_running_status(void *data) {
  GMainLoop *loop = (GMainLoop *)data;
  if (loop && !ipc_is_running()) {
    g_main_loop_quit(loop);
    return IPC_TIMEOUT_TASK_STOP;
  }
  return IPC_TIMEOUT_TASK_CONTINUE;
}

void signal_handle(int sig) { ipc_request_exit(); }

int main(int argc, char *argv[]) {
  aml_ipc_sdk_init();

  signal(SIGINT, signal_handle);
  signal(SIGTERM, signal_handle);

  gst_init(&argc, &argv);

  ipc_timeout_task_init();
  ipc_config_init();

  ipc_pipeline_start();

  GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);

  ipc_timeout_task_add(100, check_running_status, main_loop);

  g_main_loop_run(main_loop);

  g_main_loop_unref(main_loop);

  ipc_pipeline_stop();

  ipc_config_deinit();

  ipc_timeout_task_deinit();

  return 0;
}
