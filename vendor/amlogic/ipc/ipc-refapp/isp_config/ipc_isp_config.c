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

#include <string.h>

#include "ipc_config.h"
#include "ipc_common.h"

// isp config
struct isp_config {
  struct AmlIPCISP *isp;
  bool wdr_enabled;
  int brightness;
  int contrast;
  int saturation;
  int hue;
  int sharpness;

  struct {
    bool _auto;
    int _absolute;
  } exposure;

  struct {
    bool _auto;
    int cr_gain;
    int cb_gain;
  } wb;

  char *mirroring;
  int anti_banding;

  struct {
    char *action;
    int hlc_value;
    int blc_value;
  } compensation;

  struct {
    char *action;
    int space_normal;
    int space_expert;
    int time;
  } nr;

  struct {
    char *action;
    int automatic;
    int manual;
  } defog;

  struct {
    bool automatic;
    int value;
  } gain;
};

static struct isp_config gs_config = {.isp = NULL};

#define CONFIG_PARENT_KEY "/ipc/isp/"

static void onchanged_brightness(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_brightness(isp, gs_config.brightness);
}

static void onchanged_contrast(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_contrast(isp, gs_config.contrast);
}

static void onchanged_saturation(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_saturation(isp, gs_config.saturation);
}

static void onchanged_hue(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_hue(isp, gs_config.hue);
}

static void onchanged_sharpness(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_sharpness(isp, gs_config.sharpness);
}

static void onchanged_exposure(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_exposure(isp, gs_config.exposure._auto
                                    ? AML_EXPOSURE_AUTO
                                    : gs_config.exposure._absolute);
}

static void onchanged_wb(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_WB_gain(
      isp, gs_config.wb._auto ? AML_WB_GAIN_AUTO : gs_config.wb.cr_gain,
      gs_config.wb._auto ? AML_WB_GAIN_AUTO : gs_config.wb.cb_gain);
}

static void onchanged_mirroring(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  bool hflip_enabled, vflip_enabled;
  if (strcmp(gs_config.mirroring, "HV") == 0) {
    hflip_enabled = true;
    vflip_enabled = true;
  } else if (strcmp(gs_config.mirroring, "H") == 0) {
    hflip_enabled = true;
    vflip_enabled = false;
  } else if (strcmp(gs_config.mirroring, "V") == 0) {
    hflip_enabled = false;
    vflip_enabled = true;
  } else {
    hflip_enabled = false;
    vflip_enabled = false;
  }
  aml_ipc_isp_set_hflip(isp, hflip_enabled);
  aml_ipc_isp_set_vflip(isp, vflip_enabled);
}

static void onchanged_compensation(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  const char *action = gs_config.compensation.action;
  int compensation_value;
  if (strcmp(action, "hlc") == 0) {
    compensation_value = 128 - gs_config.compensation.hlc_value;
  } else if (strcmp(action, "blc") == 0) {
    compensation_value = 128 + gs_config.compensation.blc_value;
  } else {
    compensation_value = 128;
  }

  aml_ipc_isp_set_compensation(isp, compensation_value);
}

static void onchanged_nr(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  const char *action = gs_config.nr.action;
  int space_value;
  int time_value;
  if (strcmp(action, "normal") == 0) {
    space_value = gs_config.nr.space_normal;
    time_value = AML_NR_NONE;
  } else if (strcmp(action, "expert") == 0) {
    space_value = gs_config.nr.space_expert;
    time_value = gs_config.nr.time;
  } else {
    space_value = AML_NR_NONE;
    time_value = AML_NR_NONE;
  }

  aml_ipc_isp_set_nr(isp, space_value, time_value);
}

static void onchanged_defog(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  const char *action = gs_config.defog.action;
  int mode;
  int value;
  if (strcmp(action, "auto") == 0) {
    mode = 2;
    value = gs_config.defog.automatic;
  } else if (strcmp(action, "manual") == 0) {
    mode = 1;
    value = gs_config.defog.manual;
  } else {
    mode = 0;
    value = 0;
  }

  aml_ipc_isp_set_defog(isp, mode, value);
}

static void onchanged_gain(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_gain(isp, gs_config.gain.automatic, gs_config.gain.value);
}

static void onchanged_anti_binding(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_set_sensor_fps(isp, gs_config.anti_banding == 50 ? 25 : 30);
}

static void onchanged_wdr(void *user_data) {
  struct AmlIPCISP *isp = (struct AmlIPCISP *)user_data;
  aml_ipc_isp_set_wdr(isp, gs_config.wdr_enabled);
}

static struct ipc_config_kv gs_kvs[] = {
    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "wdr/enabled", &gs_config.wdr_enabled,
                       false, onchanged_wdr),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "brightness", &gs_config.brightness, 128,
                    onchanged_brightness),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "contrast", &gs_config.contrast, 128,
                    onchanged_contrast),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "saturation", &gs_config.saturation, 128,
                    onchanged_saturation),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "hue", &gs_config.hue, 128,
                    onchanged_hue),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "sharpness", &gs_config.sharpness, 128,
                    onchanged_sharpness),

    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "exposure/auto",
                       &gs_config.exposure._auto, true, onchanged_exposure),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "exposure/absolute",
                    &gs_config.exposure._absolute, 128, onchanged_exposure),

    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "whitebalance/auto",
                       &gs_config.wb._auto, true, onchanged_wb),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "whitebalance/crgain",
                    &gs_config.wb.cr_gain, 0, onchanged_wb),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "whitebalance/cbgain",
                    &gs_config.wb.cb_gain, 0, onchanged_wb),

    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "mirroring", &gs_config.mirroring,
                      "NONE", onchanged_mirroring),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "anti-banding", &gs_config.anti_banding,
                    50, onchanged_anti_binding),

    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "compensation/action",
                      &gs_config.compensation.action, "disable",
                      onchanged_compensation),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "compensation/hlc",
                    &gs_config.compensation.hlc_value, 0,
                    onchanged_compensation),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "compensation/blc",
                    &gs_config.compensation.blc_value, 0,
                    onchanged_compensation),

    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "nr/action", &gs_config.nr.action,
                      "disable", onchanged_nr),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "nr/space-normal",
                    &gs_config.nr.space_normal, 0, onchanged_nr),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "nr/space-expert",
                    &gs_config.nr.space_expert, 0, onchanged_nr),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "nr/time", &gs_config.nr.time, 0,
                    onchanged_nr),

    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "defog/action", &gs_config.defog.action,
                      "disable", onchanged_defog),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "defog/auto", &gs_config.defog.automatic,
                    0, onchanged_defog),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "defog/manual", &gs_config.defog.manual,
                    0, onchanged_defog),

    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "gain/auto", &gs_config.gain.automatic,
                       true, onchanged_gain),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "gain/manual", &gs_config.gain.value, 0,
                    onchanged_gain),

    {NULL},
};

static void on_isp_config_changed(const char *key, const char *val,
                                  void *user_data) {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    if (strcmp(gs_kvs[i].key, key) == 0) {
      ipc_config_load_kv(&gs_kvs[i]);
      if (gs_kvs[i].handle) {
        gs_kvs[i].handle(user_data);
      }
      break;
    }
  }
}

static AmlStatus register_isp_config_handle(struct AmlIPCISP *isp) {
  ipc_config_register_handle(CONFIG_PARENT_KEY, on_isp_config_changed, isp);
  return AML_STATUS_OK;
}

static AmlStatus unregister_isp_config_handle() {
  return ipc_config_unregister_handle(CONFIG_PARENT_KEY, on_isp_config_changed);
}

static AmlStatus load_configs(struct AmlIPCISP *isp) {
  if (isp == NULL)
    return AML_STATUS_WRONG_PARAM;

  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    ipc_config_load_kv(&gs_kvs[i]);
  }

  return AML_STATUS_OK;
}

static AmlStatus apply_configs(struct AmlIPCISP *isp) {
  if (isp == NULL)
    return AML_STATUS_WRONG_PARAM;

  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    if (gs_kvs[i].handle) {
      gs_kvs[i].handle((void *)isp);
    }
  }

  return AML_STATUS_OK;
}

void ipc_isp_config_load(struct AmlIPCISP *isp) {
  gs_config.isp = isp;
  load_configs(isp);
  register_isp_config_handle(isp);
}

void ipc_isp_config_apply_other() { apply_configs(gs_config.isp); }

AmlStatus ipc_isp_config_apply_wdr() {
  struct AmlIPCISP *isp = gs_config.isp;
  if (NULL == isp)
    return AML_STATUS_WRONG_PARAM;

  aml_ipc_isp_set_wdr(isp, gs_config.wdr_enabled);
  return AML_STATUS_OK;
}

AmlStatus ipc_isp_config_apply_anti_banding() {
  struct AmlIPCISP *isp = gs_config.isp;
  if (NULL == isp)
    return AML_STATUS_WRONG_PARAM;

  aml_ipc_set_sensor_fps(isp, gs_config.anti_banding == 50 ? 25 : 30);
  return AML_STATUS_OK;
}

void ipc_isp_config_unload() {
  unregister_isp_config_handle();

  FREE_STR(gs_config.mirroring);
  FREE_STR(gs_config.compensation.action);
  FREE_STR(gs_config.defog.action);
  FREE_STR(gs_config.nr.action);
}
