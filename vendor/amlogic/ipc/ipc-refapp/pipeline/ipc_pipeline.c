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
#include <aml_ipc_internal.h>
#include <limits.h>
#include <nn_detect.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <unistd.h>

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
#include <gst/rtsp-server/rtsp-onvif-server.h>
G_GNUC_END_IGNORE_DEPRECATIONS

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_imgcap.h"
#include "ipc_isp_config.h"
#include "ipc_nn.h"
#include "ipc_osd.h"
#include "ipc_recording.h"
#include "ipc_shielding.h"
#include "ipc_timeout_task.h"
#include "ipc_day_night_switch.h"

AML_LOG_DEFINE(ipc_pipeline);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(ipc_pipeline)

#define AML_COMP_START(comp)                                                   \
  if (comp) {                                                                  \
    aml_ipc_start(AML_IPC_COMPONENT(comp));                                    \
  }

#define AML_COMP_STOP(comp)                                                    \
  if (comp) {                                                                  \
    aml_ipc_stop(AML_IPC_COMPONENT(comp));                                     \
  }

#define AML_OBJ_RELEASE(obj)                                                   \
  if (obj) {                                                                   \
    aml_obj_release(AML_OBJECT(obj));                                          \
    obj = NULL;                                                                \
  }

#define RTSP_PIPELINE_STRING                                                   \
  " ("                                                                         \
  " appsrc name=vin is-live=true"                                              \
  " ! %sparse config-interval=1"                                               \
  " ! rtp%spay pt=96 config-interval=1 name=pay0"                              \
  " appsrc name=ain is-live=true"                                              \
  " ! %s name=pay1"                                                            \
  " )"

#define RTSP_BACKCHANNEL_STRING                                                \
  "capsfilter "                                                                \
  "caps=\"application/"                                                        \
  "x-rtp,media=audio,payload=0,clock-rate=8000,encoding-name=PCMU\" "          \
  "name=depay_backchannel ! rtppcmudepay ! appsink name=aout async=false"

#define WEBSTREAM_PIPELINE_STRING                                              \
  "appsrc name=vin "                                                           \
  "! h264parse "                                                               \
  "! video/x-h264,stream-format=avc "                                          \
  "! amlwsssink port=%d async=false"

#define SUB_CHANNEL_COUNT 2

#define SUB_CHANNEL_VNAME "[RTSP]Sub%d.video"
#define SUB_CHANNEL_ANAME "[RTSP]Sub%d.audio"
#define SUB_CHANNEL_BNAME "[RTSP]Sub%d.backchannel"

struct ipc_app_info {
  GstRTSPMediaFactory *factory;
  struct ipc_app {
    GstElement *appsrc;
    volatile int ready_to_feed;
    int64_t base_pts;
    AmlStatus hookreturn;
    char *name;
  } audio, video;
  struct ipc_backchannel {
    GstElement *appsink;
    struct AmlIPCPipeline *pipeline;
    struct AmlIPCComponent *adec;
    char *name;
  } backchannel;
};

struct ipc_pipelines {
  struct {
    GstElement *pipeline;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
  } gst;

  struct {
    struct ipc_app_info main;
    struct ipc_app_info sub[SUB_CHANNEL_COUNT];
    struct ipc_app_info web;
    struct ipc_app_info vr;
  } app;

  struct {
    struct AmlIPCPipeline *pipeline;
    struct AmlIPCISP *isp;
    struct AmlIPCGDC *correction;
    struct AmlIPCGE2D *rotator;
    struct AmlIPCShielding *shielding;
    struct AmlIPCOSDApp *osd;
    struct AmlIPCOSDModule *osd_module_nn;
    bool should_isp_scale_down;
    struct {
      struct AmlIPCVenc *venc;
    } main;
    struct {
      struct AmlIPCVenc *venc;
    } sub[SUB_CHANNEL_COUNT];
    struct {
      struct AmlIPCGE2D *scaler;
      struct AmlIPCVenc *venc;
    } web;
    struct {
      unsigned long timer;
    } roi;
    struct {
      struct AmlIPCShielding *shielding;
      struct AmlIPCGE2D *rotator;
    } nn;
  } video;
  struct {
    struct AmlIPCPipeline *pipeline;
    struct AmlIPCComponent *ain; // node to share with ts and vr
    struct ipc_aenc_info {
      struct AmlIPCAudioFormat format;
      int bitrate;
    } enc;
    unsigned long timer; // used for audio pipeline delay start
  } audio;
  struct {
    unsigned long timer;
  } rtsp;
  struct {
    struct {
      FILE *file;
    } debug, trace;
    struct {
      char *file;
    } graph;
  } log;
};

struct video_configs {
  int32_t bitrate;
  char *codec;
  char *device;
  int32_t framerate;
  int32_t gop;
  struct ipc_resolution resolution;
  char *bitrate_type;
  int32_t video_quality_level;
};

struct gdc_configs {
  bool enabled;
  char *config_file;
  struct ipc_resolution input_res;
  struct ipc_resolution output_res;
};

struct configs {
  int rotation;
  struct {
    bool sub_enabled[SUB_CHANNEL_COUNT];
    struct video_configs main;
    struct video_configs sub[SUB_CHANNEL_COUNT];
    struct gdc_configs gdc;
    struct {
      int port;
    } web;
  } ts;

  struct {
    bool enabled;
    int bitrate;
    int channels;
    int samplerate;
    char *codec;
    char *device;
    char *format;
    char *device_options;
  } audio;

  struct {
    struct {
      char *user;
      char *passwd;
    } auth;
    struct {
      char *address;
      int port;
      struct {
        char *main;
        char *sub[SUB_CHANNEL_COUNT];
      } route;
    } network;
  } rtsp;
};

static struct ipc_pipelines *gsp = NULL;
static struct configs gs_config;

#define CONFIG_VIDEO "/ipc/video/"
#define CONFIG_VIDEO_ROTATION "/ipc/isp/rotation"
#define CONFIG_VIDEO_ROI CONFIG_VIDEO "roi/sections"
#define CONFIG_TS CONFIG_VIDEO "ts/"
#define CONFIG_TS_MAIN CONFIG_TS "main/"
#define CONFIG_TS_SUB CONFIG_TS "sub/"
#define CONFIG_TS_SUB1 CONFIG_TS "sub1/"
#define CONFIG_TS_WEB CONFIG_TS "web/"
#define CONFIG_TS_GDC CONFIG_TS "gdc/"

#define CONFIG_AUDIO "/ipc/audio/"

#define CONFIG_RTSP "/rtsp/"
#define CONFIG_RTSP_NETWORK CONFIG_RTSP "network/"
#define CONFIG_RTSP_AUTH CONFIG_RTSP "auth/"

static enum AmlIPCVideoCodec get_vcodec_type(const char *name) {
  enum AmlIPCVideoCodec type = AML_VCODEC_NONE;
  if (NULL == name)
    return type;
  if (strcasecmp(name, "h264") == 0) {
    type = AML_VCODEC_H264;
  } else if (strcasecmp(name, "h265") == 0) {
    type = AML_VCODEC_H265;
  }

  return type;
}

static int get_video_quality_level(struct video_configs *cfg) {
  return strcmp("VBR", cfg->bitrate_type) == 0 ? cfg->video_quality_level : 0;
}

void onchange_restart(void *user_data) {
  AML_LOGW("configuration changed, pipline restart required.");
  ipc_request_exit();
}

void onchange_ts_main_bitrate(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  if (pln->video.main.venc)
    aml_ipc_venc_set_bitrate(pln->video.main.venc, gs_config.ts.main.bitrate);
}

void onchange_ts_main_framerate(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;

  // sub channel use the same framerate as main channel
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    gs_config.ts.sub[i].framerate = gs_config.ts.main.framerate;
  }

  if (pln->video.isp)
    aml_ipc_isp_set_fps(pln->video.isp, AML_ISP_FR,
                        gs_config.ts.main.framerate);
}

void onchange_ts_main_gop(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  if (pln->video.main.venc)
    aml_ipc_venc_set_gop_size(pln->video.main.venc, gs_config.ts.main.gop);
}

void onchange_ts_main_bitrate_control(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;

  int quality_level = get_video_quality_level(&gs_config.ts.main);

  if (pln->video.main.venc)
    aml_ipc_venc_set_quality_level(pln->video.main.venc, quality_level);

  enum AmlIPCVideoCodec codec = get_vcodec_type(gs_config.ts.main.codec);
  if (codec != AML_VCODEC_H264) {
    if (pln->video.web.venc)
      aml_ipc_venc_set_quality_level(pln->video.web.venc, quality_level);
  }
}

void onchange_ts_sub_bitrate(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    if (pln->video.sub[i].venc)
      aml_ipc_venc_set_bitrate(pln->video.sub[i].venc,
                               gs_config.ts.sub[i].bitrate);
  }
}

void onchange_ts_sub_gop(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    if (pln->video.sub[i].venc)
      aml_ipc_venc_set_gop_size(pln->video.sub[i].venc,
                                gs_config.ts.sub[i].gop);
  }
}

void onchange_ts_sub_bitrate_control(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  int quality_level;
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    quality_level = get_video_quality_level(&gs_config.ts.sub[i]);
    if (pln->video.sub[i].venc)
      aml_ipc_venc_set_quality_level(pln->video.sub[i].venc, quality_level);
  }
}

void onchange_ts_gdc_enabled(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  if (pln->video.correction) {
    aml_ipc_gdc_set_passthrough(pln->video.correction,
                                !gs_config.ts.gdc.enabled);
  }
}

void onchange_ts_gdc_configfile(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  if (pln->video.correction) {
    aml_ipc_gdc_set_configfile(pln->video.correction,
                               gs_config.ts.gdc.config_file);
  }
}

void onchange_rotation(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  int degree = gs_config.rotation;
  aml_ipc_ge2d_set_rotation(pln->video.rotator, degree);
  // if rotation degree is 0 and scaler is not used
  aml_ipc_ge2d_set_passthrough(
      pln->video.rotator, degree == 0 && !gsp->video.should_isp_scale_down);
  if (pln->video.nn.rotator) {
    aml_ipc_ge2d_set_rotation(pln->video.nn.rotator, degree);
    aml_ipc_ge2d_set_passthrough(pln->video.nn.rotator, degree == 0);
  }
}

void onchange_sub_check_restart(void *user_data) {
  if (gs_config.ts.sub_enabled[0]) {
    onchange_restart(user_data);
  }
}

void onchange_sub1_check_restart(void *user_data) {
  if (gs_config.ts.sub_enabled[1]) {
    onchange_restart(user_data);
  }
}

void onchange_audio_enabled(void *user_data) {}
void onchange_audio_bitrate(void *user_data) { onchange_restart(user_data); }
void onchange_audio_channels(void *user_data) { onchange_restart(user_data); }
void onchange_audio_samplerate(void *user_data) { onchange_restart(user_data); }
void onchange_audio_codec(void *user_data) { onchange_restart(user_data); }
void onchange_audio_device(void *user_data) { onchange_restart(user_data); }
void onchange_audio_format(void *user_data) { onchange_restart(user_data); }

static void rtsp_set_auth(GstRTSPServer *server, const char *username,
                          const char *passwd);
static void rtsp_set_role_permission(GstRTSPMediaFactory *factory,
                                     const char *username);

static bool delay_set_rtsp_auth(void *data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)data;
  rtsp_set_auth(pln->gst.server, gs_config.rtsp.auth.user,
                gs_config.rtsp.auth.passwd);
  rtsp_set_role_permission(pln->app.main.factory, gs_config.rtsp.auth.user);
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    rtsp_set_role_permission(pln->app.sub[i].factory, gs_config.rtsp.auth.user);
  }
  pln->rtsp.timer = 0;
  return IPC_TIMEOUT_TASK_STOP;
}

void onchange_rtsp_auth(void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  if (pln->rtsp.timer > 0) {
    ipc_timeout_task_remove(pln->rtsp.timer);
  }
  pln->rtsp.timer = ipc_timeout_task_add(100, delay_set_rtsp_auth, user_data);
}

static struct ipc_config_kv gs_kvs[] = {
    // video rotation
    IPC_CONFIG_KV_I(CONFIG_VIDEO_ROTATION, &gs_config.rotation, 0,
                    onchange_rotation),
    // main channel
    IPC_CONFIG_KV_I(CONFIG_TS_MAIN "bitrate", &gs_config.ts.main.bitrate, 2000,
                    onchange_ts_main_bitrate),
    IPC_CONFIG_KV_STR(CONFIG_TS_MAIN "codec", &gs_config.ts.main.codec, "h264",
                      onchange_restart),
    IPC_CONFIG_KV_STR(CONFIG_TS_MAIN "device", &gs_config.ts.main.device,
                      "/dev/video0", onchange_restart),
    IPC_CONFIG_KV_I(CONFIG_TS_MAIN "framerate", &gs_config.ts.main.framerate,
                    30, onchange_ts_main_framerate),
    IPC_CONFIG_KV_I(CONFIG_TS_MAIN "gop", &gs_config.ts.main.gop, 10,
                    onchange_ts_main_gop),
    IPC_CONFIG_KV_RESOLUTION(CONFIG_TS_MAIN "resolution",
                             &gs_config.ts.main.resolution, 1920, 1080,
                             onchange_restart),
    IPC_CONFIG_KV_STR(CONFIG_TS_MAIN "bitrate-type",
                      &gs_config.ts.main.bitrate_type, "CBR",
                      onchange_ts_main_bitrate_control),
    IPC_CONFIG_KV_I(CONFIG_TS_MAIN "video-quality-level",
                    &gs_config.ts.main.video_quality_level, 6,
                    onchange_ts_main_bitrate_control),

    // sub channel
    IPC_CONFIG_KV_BOOL(CONFIG_TS_SUB "enabled", &gs_config.ts.sub_enabled[0],
                       false, onchange_restart),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB "bitrate", &gs_config.ts.sub[0].bitrate, 500,
                    onchange_ts_sub_bitrate),
    IPC_CONFIG_KV_STR(CONFIG_TS_SUB "codec", &gs_config.ts.sub[0].codec, "h264",
                      onchange_sub_check_restart),
    IPC_CONFIG_KV_STR(CONFIG_TS_SUB "device", &gs_config.ts.sub[0].device,
                      "/dev/video0", onchange_restart),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB "framerate", &gs_config.ts.sub[0].framerate,
                    30, NULL),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB "gop", &gs_config.ts.sub[0].gop, 10,
                    onchange_ts_sub_gop),
    IPC_CONFIG_KV_STR(CONFIG_TS_SUB "bitrate-type",
                      &gs_config.ts.sub[0].bitrate_type, "CBR",
                      onchange_ts_sub_bitrate_control),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB "video-quality-level",
                    &gs_config.ts.sub[0].video_quality_level, 6,
                    onchange_ts_sub_bitrate_control),
    IPC_CONFIG_KV_RESOLUTION(CONFIG_TS_SUB "resolution",
                             &gs_config.ts.sub[0].resolution, 704, 576,
                             onchange_sub_check_restart),

    // sub1 channel
    IPC_CONFIG_KV_BOOL(CONFIG_TS_SUB1 "enabled", &gs_config.ts.sub_enabled[1],
                       false, onchange_restart),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB1 "bitrate", &gs_config.ts.sub[1].bitrate, 500,
                    onchange_ts_sub_bitrate),
    IPC_CONFIG_KV_STR(CONFIG_TS_SUB1 "codec", &gs_config.ts.sub[1].codec,
                      "h264", onchange_sub1_check_restart),
    IPC_CONFIG_KV_STR(CONFIG_TS_SUB1 "device", &gs_config.ts.sub[1].device,
                      "/dev/video0", onchange_restart),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB1 "framerate", &gs_config.ts.sub[1].framerate,
                    30, NULL),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB1 "gop", &gs_config.ts.sub[1].gop, 10,
                    onchange_ts_sub_gop),
    IPC_CONFIG_KV_STR(CONFIG_TS_SUB1 "bitrate-type",
                      &gs_config.ts.sub[1].bitrate_type, "CBR",
                      onchange_ts_sub_bitrate_control),
    IPC_CONFIG_KV_I(CONFIG_TS_SUB1 "video-quality-level",
                    &gs_config.ts.sub[1].video_quality_level, 6,
                    onchange_ts_sub_bitrate_control),
    IPC_CONFIG_KV_RESOLUTION(CONFIG_TS_SUB1 "resolution",
                             &gs_config.ts.sub[1].resolution, 704, 576,
                             onchange_sub1_check_restart),

    // ts webstream
    IPC_CONFIG_KV_I(CONFIG_TS_WEB "port", &gs_config.ts.web.port, 8082,
                    onchange_restart),
    // ts gdc
    IPC_CONFIG_KV_BOOL(CONFIG_TS_GDC "enabled", &gs_config.ts.gdc.enabled,
                       false, onchange_ts_gdc_enabled),
    IPC_CONFIG_KV_STR(CONFIG_TS_GDC "config-file",
                      &gs_config.ts.gdc.config_file, "",
                      onchange_ts_gdc_configfile),
    IPC_CONFIG_KV_RESOLUTION(CONFIG_TS_GDC "input-resolution",
                             &gs_config.ts.gdc.input_res, 1920, 1080, NULL),
    IPC_CONFIG_KV_RESOLUTION(CONFIG_TS_GDC "output-resolution",
                             &gs_config.ts.gdc.output_res, 1920, 1080, NULL),
    // audio
    IPC_CONFIG_KV_BOOL(CONFIG_AUDIO "enabled", &gs_config.audio.enabled, false,
                       onchange_audio_enabled),
    IPC_CONFIG_KV_I(CONFIG_AUDIO "bitrate", &gs_config.audio.bitrate, 64,
                    onchange_audio_bitrate),
    IPC_CONFIG_KV_I(CONFIG_AUDIO "channels", &gs_config.audio.channels, 2,
                    onchange_audio_channels),
    IPC_CONFIG_KV_I(CONFIG_AUDIO "samplerate", &gs_config.audio.samplerate,
                    8000, onchange_audio_samplerate),
    IPC_CONFIG_KV_STR(CONFIG_AUDIO "codec", &gs_config.audio.codec, "g711",
                      onchange_audio_codec),
    IPC_CONFIG_KV_STR(CONFIG_AUDIO "device", &gs_config.audio.device, "default",
                      onchange_audio_device),
    IPC_CONFIG_KV_STR(CONFIG_AUDIO "format", &gs_config.audio.format, "S16LE",
                      onchange_audio_format),
    // rtsp
    IPC_CONFIG_KV_STR(CONFIG_RTSP_NETWORK "address",
                      &gs_config.rtsp.network.address, "0.0.0.0",
                      onchange_restart),
    IPC_CONFIG_KV_I(CONFIG_RTSP_NETWORK "port", &gs_config.rtsp.network.port,
                    554, onchange_restart),
    IPC_CONFIG_KV_STR(CONFIG_RTSP_NETWORK "route",
                      &gs_config.rtsp.network.route.main, "/live.sdp",
                      onchange_restart),
    IPC_CONFIG_KV_STR(CONFIG_RTSP_NETWORK "subroute",
                      &gs_config.rtsp.network.route.sub[0], "/sub.sdp",
                      onchange_restart),
    IPC_CONFIG_KV_STR(CONFIG_RTSP_NETWORK "sub1route",
                      &gs_config.rtsp.network.route.sub[1], "/sub1.sdp",
                      onchange_restart),
    IPC_CONFIG_KV_STR(CONFIG_RTSP_AUTH "username", &gs_config.rtsp.auth.user,
                      "", onchange_rtsp_auth),
    IPC_CONFIG_KV_STR(CONFIG_RTSP_AUTH "password", &gs_config.rtsp.auth.passwd,
                      "", onchange_rtsp_auth),
    {NULL},
};

static void apply_venc_param(struct AmlIPCVenc *venc,
                             struct video_configs *cfg) {
  if (venc == NULL || cfg == NULL)
    return;

  int quality_level = get_video_quality_level(cfg);
  AML_LOGD("quality_level:%d\n", quality_level);

  aml_ipc_venc_set_bitrate(venc, cfg->bitrate);
  aml_ipc_venc_set_gop_size(venc, cfg->gop);
  aml_ipc_venc_set_quality_level(venc, quality_level);
}

#define CLEAR_ROI(venc)                                                        \
  do {                                                                         \
    if (venc)                                                                  \
      aml_ipc_venc_clear_roi(venc, -1);                                        \
  } while (0)

#define SET_ROI(venc)                                                          \
  do {                                                                         \
    if (venc)                                                                  \
      aml_ipc_venc_set_roi(venc, quality, &rect, NULL);                        \
  } while (0)

static void load_roi_config(struct ipc_pipelines *pln) {
  int array_size = ipc_config_array_get_size(CONFIG_VIDEO_ROI);

  CLEAR_ROI(pln->video.main.venc);
  CLEAR_ROI(pln->video.web.venc);
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    CLEAR_ROI(pln->video.sub[i].venc);
  }

  for (int i = 0; i < array_size; i++) {
    struct AmlRectFloat rect;
    int quality =
        ipc_config_array_get_int32_t(CONFIG_VIDEO_ROI, i, "quality", 0);
    rect.left =
        ipc_config_array_get_float(CONFIG_VIDEO_ROI, i, "position/left", 0);
    rect.top =
        ipc_config_array_get_float(CONFIG_VIDEO_ROI, i, "position/top", 0);
    rect.width =
        ipc_config_array_get_float(CONFIG_VIDEO_ROI, i, "position/width", 0);
    rect.height =
        ipc_config_array_get_float(CONFIG_VIDEO_ROI, i, "position/height", 0);

    SET_ROI(pln->video.main.venc);
    SET_ROI(pln->video.web.venc);
    for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
      SET_ROI(pln->video.sub[i].venc);
    }
  }
}

static bool delay_load_roi(void *data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)data;
  load_roi_config(pln);
  pln->video.roi.timer = 0;
  return IPC_TIMEOUT_TASK_STOP;
}

static void on_roi_changed(const char *key, const char *val, void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  if (pln->video.roi.timer > 0) {
    ipc_timeout_task_remove(pln->video.roi.timer);
  }
  pln->video.roi.timer = ipc_timeout_task_add(100, delay_load_roi, user_data);
}

static void on_config_changed(const char *key, const char *val,
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

#define CONFIG_DEBUG "/ipc/log/"

static void load_log_level_config(struct ipc_pipelines *pln) {
  char *level = ipc_config_get_string(CONFIG_DEBUG "debug/level", ".*:1");
  aml_ipc_log_set_from_string(level);
  free(level);

  level = ipc_config_get_string(CONFIG_DEBUG "gst/level", "*:1");
  gst_debug_set_threshold_from_string(level, TRUE);
  free(level);

  level = ipc_config_get_string(CONFIG_DEBUG "trace/level", ".*:1");
  aml_ipc_trace_set_from_string(level);
  free(level);
}

static void open_log_file(const char *cfg, FILE **pfp) {
  if (pfp == NULL || cfg == NULL)
    return;
  FILE *fp = *pfp;

  char *file = ipc_config_get_string(cfg, "stdout");

  if (fp && fp != stdout && fp != stderr) {
    fclose(fp);
  }

  if (strcmp(file, "stderr") == 0)
    fp = stderr;
  else if (strcmp(file, "stdout") == 0)
    fp = stdout;
  else {
    if ((fp = fopen(file, "wb"))) {
      setvbuf(fp, NULL, _IONBF, 0);
    }
  }

  free(file);

  *pfp = fp;
}

static void load_log_file_config(struct ipc_pipelines *pln) {
  open_log_file(CONFIG_DEBUG "debug/file", &pln->log.debug.file);
  aml_ipc_log_set_output_file(pln->log.debug.file);

  open_log_file(CONFIG_DEBUG "trace/file", &pln->log.trace.file);
  aml_ipc_trace_set_json_output(pln->log.trace.file);
}

static void load_graph_file_config(struct ipc_pipelines *pln) {
  FREE_STR(pln->log.graph.file);
  pln->log.graph.file = ipc_config_get_string(CONFIG_DEBUG "graph/file", "");
}

static void load_debug_config(struct ipc_pipelines *pln) {
  load_log_level_config(pln);
  load_log_file_config(pln);
  load_graph_file_config(pln);
}

static void dump_pipeline_graph(struct ipc_pipelines *pln) {
  FILE *fp = stdout;
  char *file = pln->log.graph.file;
  if (file && file[0] != '\0') {
    if (strcmp(file, "stderr") == 0)
      fp = stderr;
    else if (strcmp(file, "stdout") == 0)
      fp = stdout;
    else {
      fp = fopen(file, "wb");
    }
  }
  if (fp) {
    fprintf(fp, "Video Pipeline\n==\n");
    aml_ipc_pipeline_dump_to_markdown(pln->video.pipeline, fp);
    fprintf(fp, "\nAudio Pipeline\n==\n");
    aml_ipc_pipeline_dump_to_markdown(pln->audio.pipeline, fp);
    if (pln->app.main.backchannel.pipeline) {
      fprintf(fp, "\nBackChannel Pipeline [RTSP.Main]\n==\n");
      aml_ipc_pipeline_dump_to_markdown(pln->app.main.backchannel.pipeline, fp);
    }
    for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
      if (pln->app.sub[i].backchannel.pipeline) {
        fprintf(fp, "\nBackChannel Pipeline [RTSP.Sub.%d]\n==\n", i + 1);
        aml_ipc_pipeline_dump_to_markdown(pln->app.sub[i].backchannel.pipeline,
                                          fp);
      }
    }
  }
  if (fp && fp != stdout && fp != stderr) {
    fclose(fp);
  }
}

static void on_debug_changed(const char *key, const char *val,
                             void *user_data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)user_data;
  if (strcmp(key, CONFIG_DEBUG "debug/file") == 0 ||
      strcmp(key, CONFIG_DEBUG "trace/file") == 0) {
    load_log_file_config(pln);
  } else if (strcmp(key, CONFIG_DEBUG "graph/trigger") == 0 && val != NULL &&
             ipc_strtob(val) == true) {
    dump_pipeline_graph(pln);
    ipc_config_remove(CONFIG_DEBUG "graph/trigger");
  } else if (strcmp(key, CONFIG_DEBUG "graph/file") == 0) {
    FREE_STR(pln->log.graph.file);
    if (val) {
      pln->log.graph.file = strdup(val);
    }
  } else {
    load_log_level_config(pln);
  }
}

static AmlStatus register_config_handle(struct ipc_pipelines *pipeline) {
  AmlStatus ret;
  // remove graph trigger
  ipc_config_remove(CONFIG_DEBUG "graph/trigger");

  ret = ipc_config_register_handle(CONFIG_VIDEO, on_config_changed, pipeline);
  ret |= ipc_config_register_handle(CONFIG_AUDIO, on_config_changed, pipeline);
  ret |= ipc_config_register_handle(CONFIG_RTSP, on_config_changed, pipeline);

  ret |= ipc_config_register_handle(CONFIG_VIDEO_ROTATION, on_config_changed,
                                    pipeline);
  ret |= ipc_config_register_handle(CONFIG_VIDEO_ROI, on_roi_changed, pipeline);
  ret |= ipc_config_register_handle(CONFIG_DEBUG, on_debug_changed, pipeline);
  return ret;
}

static AmlStatus unregister_config_handle() {
  AmlStatus ret;
  ret = ipc_config_unregister_handle(CONFIG_VIDEO_ROI, on_roi_changed);
  ret |= ipc_config_unregister_handle(CONFIG_DEBUG, on_debug_changed);
  ret |= ipc_config_unregister_handle(CONFIG_VIDEO_ROTATION, on_config_changed);
  ret |= ipc_config_unregister_handle(CONFIG_RTSP, on_config_changed);
  ret |= ipc_config_unregister_handle(CONFIG_AUDIO, on_config_changed);
  ret |= ipc_config_unregister_handle(CONFIG_VIDEO, on_config_changed);
  return ret;
}

static AmlStatus load_configs(struct ipc_pipelines *pipeline) {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    ipc_config_load_kv(&gs_kvs[i]);
  }

  // sub channel framerate should be the same as main channel
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    gs_config.ts.sub[i].framerate = gs_config.ts.main.framerate;
  }

  load_debug_config(pipeline);
  return AML_STATUS_OK;
}

static bool check_video_pts(struct ipc_app *app, struct ipc_app_info *appinfo) {
  return app == &appinfo->audio && appinfo->video.base_pts <= 0;
}

static AmlStatus on_enc_frame(struct AmlIPCFrame *frame, void *param) {
  struct ipc_app *app = (struct ipc_app *)param;
  if (app->ready_to_feed && app->appsrc) {
    if (AML_OBJ_INSTANCEOF(frame, AmlIPCAudioFrame)) {
      if (check_video_pts(app, &gsp->app.main) ||
          check_video_pts(app, &gsp->app.sub[0]) ||
          check_video_pts(app, &gsp->app.sub[1]) ||
          check_video_pts(app, &gsp->app.vr)) {
        AML_LOGV("%s: video not ready, skip frame", app->name);
        return app->hookreturn;
      }
    } else if (AML_OBJ_INSTANCEOF(frame, AmlIPCVideoBitstream)) {
      if (app->base_pts <= 0) {
        struct AmlIPCVideoBitstream *bs = (struct AmlIPCVideoBitstream *)frame;
        if (!bs->is_key_frame) {
          AML_LOGV("%s: waiting for the first iframe...", app->name);
          return app->hookreturn;
        }
      }
    }
    GstBuffer *buf = gst_buffer_new_allocate(NULL, frame->size, NULL);
    gst_buffer_fill(buf, 0, frame->data, frame->size);
    if (app->base_pts == -1LL) {
      app->base_pts = frame->pts_us * 1000LL;
    }
    GST_BUFFER_PTS(buf) = frame->pts_us * 1000LL - app->base_pts;
    GST_BUFFER_DTS(buf) = GST_BUFFER_PTS(buf);
    GstFlowReturn ret;
    g_signal_emit_by_name(app->appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);

    AML_LOGV("%s: %p push buffer %d bytes ret %d, pts:%" PRId64, app->name, app,
             frame->size, ret, frame->pts_us);
    if (ret != GST_FLOW_OK) {
      AML_LOGW("%s push buffer fail %d", app->name, ret);
      app->ready_to_feed = 0;
    }
  }
  // return AML_STATUS_HOOK_CONTINUE, pipeline will continue to process the
  // frame
  return app->hookreturn;
}

static void start_feed(GstElement *pipeline, guint size, struct ipc_app *app) {
  app->ready_to_feed = 1;
}

static void stop_feed(GstElement *pipeline, struct ipc_app *app) {
  app->ready_to_feed = 0;
}

static GstFlowReturn backchannel_start(GstElement *pipeline,
                                       struct ipc_backchannel *backchannel) {
  AML_LOGW("start up backchannel pipeline");
  if (backchannel->pipeline == NULL) {
    backchannel->pipeline = aml_ipc_pipeline_new();
    backchannel->adec =
        AML_IPC_COMPONENT(aml_ipc_g711_new(AML_ACODEC_PCM_S16LE));
    struct AmlIPCAudioFormat fmt = {AML_ACODEC_PCM_S16LE, 48000, 16, 2};
    struct AmlIPCAConvert *aconv = aml_ipc_aconv_new(&fmt);
    aml_ipc_aconv_set_channel_mapping(aconv, "00");
    struct AmlIPCComponent *aout = AML_IPC_COMPONENT(aml_ipc_aout_new());
    aml_ipc_bind(backchannel->adec, AML_ACODEC_OUTPUT, AML_IPC_COMPONENT(aconv),
                 AML_ACONVERT_INPUT);
    aml_ipc_bind(AML_IPC_COMPONENT(aconv), AML_ACONVERT_OUTPUT, aout,
                 AML_AOUT_INPUT);
    aml_ipc_pipeline_add_many(backchannel->pipeline, backchannel->adec,
                              AML_IPC_COMPONENT(aconv), aout, NULL);
    aml_ipc_start(AML_IPC_COMPONENT(backchannel->pipeline));
  }
  return GST_FLOW_OK;
}

static GstFlowReturn backchannel_pushdata(GstElement *pipeline,
                                          struct ipc_backchannel *backchannel) {
  GstSample *sample;
  GstFlowReturn ret = GST_FLOW_OK;

  /* Retrieve the buffer */
  g_signal_emit_by_name(backchannel->appsink, "pull-sample", &sample);
  if (sample) {
    if (backchannel->pipeline) {
      GstBuffer *buffer = gst_sample_get_buffer(sample);
      GstMapInfo bufinfo;
      unsigned char *data = NULL;
      int datalen = 0;
      if (gst_buffer_map(buffer, &bufinfo, GST_MAP_READ)) {
        if ((data = (unsigned char *)malloc(bufinfo.size))) {
          datalen = bufinfo.size;
          memcpy(data, bufinfo.data, datalen);
        } else {
          AML_LOGE("failed to malloc data");
          ret = GST_FLOW_ERROR;
          goto end_push;
        }
        gst_buffer_unmap(buffer, &bufinfo);
      } else {
        AML_LOGE("failed to map buffer");
        ret = GST_FLOW_ERROR;
        goto end_push;
      }
      struct AmlIPCAudioFrame *frame = AML_OBJ_NEW(AmlIPCAudioFrame);
      frame->format.codec = AML_ACODEC_PCM_ULAW;
      frame->format.sample_rate = 8000;
      frame->format.num_channel = 1;
      AML_IPC_FRAME(frame)->data = data;
      AML_IPC_FRAME(frame)->size = datalen;

      AmlStatus status = aml_ipc_send_frame(backchannel->adec, AML_ACODEC_INPUT,
                                            AML_IPC_FRAME(frame));
      if (status != AML_STATUS_OK) {
        AML_LOGE("failed to send buffer");
        ret = GST_FLOW_ERROR;
      }
    }
  end_push:
    gst_sample_unref(sample);
    return ret;
  }
  return GST_FLOW_ERROR;
}

static void backchannel_stop(GstElement *pipeline,
                             struct ipc_backchannel *backchannel) {
  AML_LOGW("stop backchannel pipeline");
  if (backchannel->pipeline) {
    aml_ipc_stop(AML_IPC_COMPONENT(backchannel->pipeline));
    aml_obj_release(AML_OBJECT(backchannel->pipeline));
    backchannel->pipeline = NULL;
  }
}

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media,
                            struct ipc_app_info *app) {
  GstElement *rtpbin = gst_rtsp_media_get_element(media);
  app->backchannel.appsink = gst_bin_get_by_name(GST_BIN(rtpbin), "aout");
  if (app->backchannel.appsink != NULL) {
    // backchannel
    AML_LOGW("%s started...", app->backchannel.name);
    g_object_set(app->backchannel.appsink, "emit-signals", TRUE, NULL);
    g_signal_connect(app->backchannel.appsink, "new-preroll",
                     G_CALLBACK(backchannel_start), &app->backchannel);
    g_signal_connect(app->backchannel.appsink, "new-sample",
                     G_CALLBACK(backchannel_pushdata), &app->backchannel);
    g_signal_connect(app->backchannel.appsink, "eos",
                     G_CALLBACK(backchannel_stop), &app->backchannel);
  }
  AML_LOGW("%s started...", app->video.name);
  app->video.appsrc = gst_bin_get_by_name(GST_BIN(rtpbin), "vin");
  g_signal_connect(app->video.appsrc, "need-data", G_CALLBACK(start_feed),
                   &app->video);
  g_signal_connect(app->video.appsrc, "enough-data", G_CALLBACK(stop_feed),
                   &app->video);

  AML_LOGW("%s started...", app->audio.name);
  app->audio.appsrc = gst_bin_get_by_name(GST_BIN(rtpbin), "ain");
  g_signal_connect(app->audio.appsrc, "need-data", G_CALLBACK(start_feed),
                   &app->audio);
  g_signal_connect(app->audio.appsrc, "enough-data", G_CALLBACK(stop_feed),
                   &app->audio);
  app->video.base_pts = -1LL;
  app->audio.base_pts = -1LL;
  return;
}

enum AmlIPCAudioCodec get_audio_codec(const char *codec_name) {
  if (strcasecmp(codec_name, "g711") == 0 ||
      strcasecmp(codec_name, "mulaw") == 0) {
    return AML_ACODEC_PCM_ULAW;
  }
  if (strcasecmp(codec_name, "g726") == 0) {
    return AML_ACODEC_ADPCM_G726;
  }
  if (strcasecmp(codec_name, "aac") == 0) {
    return AML_ACODEC_AAC;
  }
  return AML_ACODEC_SDK_MAX;
}

#define GST_G711_PARAM                                                         \
  "audio/x-mulaw,rate=%d,channels=%d,format=S16LE ! rtppcmupay"
#define GST_G726_PARAM                                                         \
  "audio/x-adpcm,rate=%d,channels=%d,bitrate=%d ! rtpg726pay"
#define GST_AAC_PARAM "aacparse ! rtpmp4apay"

char *gst_audio_param(struct ipc_aenc_info *aenc) {
  char *ret = NULL;
  switch (aenc->format.codec) {
  case AML_ACODEC_PCM_ULAW: {
    int sz = snprintf(NULL, 0, GST_G711_PARAM, aenc->format.sample_rate,
                      aenc->format.num_channel) +
             1;
    ret = ipc_mem_new0(char, sz);
    snprintf(ret, sz, GST_G711_PARAM, aenc->format.sample_rate,
             aenc->format.num_channel);
  } break;
  case AML_ACODEC_ADPCM_G726: {
    int sz = snprintf(NULL, 0, GST_G726_PARAM, aenc->format.sample_rate,
                      aenc->format.num_channel, aenc->bitrate) +
             1;
    ret = ipc_mem_new0(char, sz);
    snprintf(ret, sz, GST_G726_PARAM, aenc->format.sample_rate,
             aenc->format.num_channel, aenc->bitrate);
  } break;
  case AML_ACODEC_AAC:
    ret = strdup(GST_AAC_PARAM);
    break;
  default:
    AML_LOGE("audio codec not supported");
  }
  return ret;
}

static void bind_enc_to_rtsp(struct ipc_app_info *psrc,
                             GstRTSPMountPoints *mounts,
                             const char *mount_point, const char *vcodec,
                             struct ipc_aenc_info *aenc) {
  char *aparam = gst_audio_param(aenc);
  char *pipeline_str;
  int sz = snprintf(NULL, 0, RTSP_PIPELINE_STRING, vcodec, vcodec, aparam) + 1;
  pipeline_str = ipc_mem_new0(char, sz);
  snprintf(pipeline_str, sz, RTSP_PIPELINE_STRING, vcodec, vcodec, aparam);
  free(aparam);

  AML_LOGD("pipeline: %s\n", pipeline_str);

  psrc->factory = gst_rtsp_onvif_media_factory_new();
  g_signal_connect(psrc->factory, "media-configure",
                   G_CALLBACK(media_configure), psrc);
  gst_rtsp_media_factory_set_media_gtype(psrc->factory,
                                         GST_TYPE_RTSP_ONVIF_MEDIA);
  gst_rtsp_media_factory_set_latency(psrc->factory, 100);
  gst_rtsp_media_factory_set_publish_clock_mode(
      psrc->factory, GST_RTSP_PUBLISH_CLOCK_MODE_CLOCK_AND_OFFSET);
  gst_rtsp_media_factory_set_launch(psrc->factory, pipeline_str);
  gst_rtsp_media_factory_set_shared(psrc->factory, TRUE);
  gst_rtsp_mount_points_add_factory(mounts, mount_point, psrc->factory);
  free(pipeline_str);

  // backchannel
  gst_rtsp_onvif_media_factory_set_backchannel_launch(
      GST_RTSP_ONVIF_MEDIA_FACTORY(psrc->factory), RTSP_BACKCHANNEL_STRING);

  // set role permission
  rtsp_set_role_permission(psrc->factory, gs_config.rtsp.auth.user);
}

static void bind_enc_to_appsrc(struct ipc_app_info *psrc) {
  if (psrc->video.appsrc) {
    g_signal_connect(psrc->video.appsrc, "need-data", G_CALLBACK(start_feed),
                     &psrc->video);
    g_signal_connect(psrc->video.appsrc, "enough-data", G_CALLBACK(stop_feed),
                     &psrc->video);
    psrc->video.base_pts = -1LL;
  }
  if (psrc->audio.appsrc) {
    g_signal_connect(psrc->audio.appsrc, "need-data", G_CALLBACK(start_feed),
                     &psrc->audio);
    g_signal_connect(psrc->audio.appsrc, "enough-data", G_CALLBACK(stop_feed),
                     &psrc->audio);
    psrc->audio.base_pts = -1LL;
  }
}

bool get_proper_resolution(struct AmlIPCISP *isp,
                           struct AmlIPCVideoFormat *fmt) {
  bool match = false;
  int num_fmts;
  if (AML_STATUS_OK == aml_ipc_isp_query_resolutions(
                           isp, AML_ISP_FR, fmt->pixfmt, &num_fmts, NULL) &&
      num_fmts > 0) {
    struct AmlIPCVideoFormat *fmts =
        ipc_mem_new0(struct AmlIPCVideoFormat, num_fmts);
    aml_ipc_isp_query_resolutions(isp, AML_ISP_FR, fmt->pixfmt, &num_fmts,
                                  fmts);
    struct res_diff {
      int min_w;
      int min_h;
      struct AmlIPCVideoFormat proper_fmt;
    } bigger, smaller;
    bigger.min_h = bigger.min_w = INT_MAX;
    smaller.min_h = smaller.min_w = INT_MAX;
    for (int i = 0; i < num_fmts; i++) {
      if (fmts[i].width == fmt->width && fmts[i].height == fmt->height) {
        AML_LOGD("required resolution supported by isp");
        match = true;
        break;
      } else {
        // if the format not match the setting, scaler should be used
        int wdiff = fmts[i].width - fmt->width;
        int hdiff = fmts[i].height - (fmts[i].width * fmt->height / fmt->width);
        hdiff = hdiff < 0 ? (-hdiff) : hdiff;
        struct res_diff *pdiff;

        if (wdiff < 0) {
          pdiff = &smaller;
          wdiff = -wdiff;
        } else {
          pdiff = &bigger;
        }
        if (hdiff <= pdiff->min_h && wdiff <= pdiff->min_w) {
          pdiff->min_h = hdiff;
          pdiff->min_w = wdiff;
          pdiff->proper_fmt.width = fmts[i].width;
          pdiff->proper_fmt.height = fmts[i].height;
        }
      }
    }
    free(fmts);
    if (!match) {
      struct res_diff *pdiff = &bigger;
      if (bigger.min_h == INT_MAX) {
        pdiff = &smaller;
      }
      fmt->width = pdiff->proper_fmt.width;
      fmt->height = pdiff->proper_fmt.height;
    }
    AML_LOGD("get proper isp resolution: %dx%d", fmt->width, fmt->height);
  } else {
    AML_LOGW("isp resolution query method is not supported!");
  }
  return match;
}

static bool renew_nn_result(void *data) {
  struct AmlIPCFrame *result = AML_IPC_FRAME(data);
  aml_ipc_osd_nn_push_result(gsp->video.osd_module_nn, result);
  return IPC_TIMEOUT_TASK_STOP;
}

AmlStatus nn_push_result(struct AmlIPCFrame *frame, void *nouse) {
  AML_LOGD("nn result pushed");
  aml_obj_addref(AML_OBJECT(frame));
  ipc_timeout_task_add(1, renew_nn_result, frame);
  return AML_STATUS_OK;
}

/*
 *                    +--> shielding hook
 *                    |
 * isp --> gdc --> rotator --> queue --> osd
 *  |
 *  +--> image capture
 */
static AmlStatus video_source_create() {
  gsp->video.pipeline = aml_ipc_pipeline_new();
  struct AmlIPCISP *isp = gsp->video.isp =
      aml_ipc_isp_new(gs_config.ts.main.device, 6, 4, 0);

  // load isp config
  ipc_isp_config_load(isp);

  // apply wdr/anti-banding before set format
  ipc_isp_config_apply_wdr();
  ipc_isp_config_apply_anti_banding();

  struct AmlIPCVideoFormat fmt = {
      AML_PIXFMT_NV12, gs_config.ts.main.resolution.width,
      gs_config.ts.main.resolution.height, gs_config.ts.main.framerate};

  // query isp format
  // select the most proper format
  gsp->video.should_isp_scale_down = !get_proper_resolution(isp, &fmt);
  // set isp format
  AmlStatus ret = aml_ipc_isp_set_capture_format(isp, AML_ISP_FR, &fmt);
  if (ret != AML_STATUS_OK) {
    AML_LOGE("isp set format failed\n");
    return ret;
  }

  // apply other isp configs
  ipc_isp_config_apply_other();

  // create imgcap module
  struct AmlIPCImgCap *imgcap = aml_ipc_imgcap_new();
  // default with software encoder,
  // switch to hardware encoder
  // aml_ipc_imgcap_use_hw_enc(imgcap, true);
  // isp -> imgcap
  aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR, //
               AML_IPC_COMPONENT(imgcap), AML_IMG_CAP_INPUT);

  struct AmlIPCVBPool *vbp[2];
  // create gdc
  gsp->video.correction = aml_ipc_gdc_new();
  aml_ipc_gdc_set_passthrough(gsp->video.correction, !gs_config.ts.gdc.enabled);
  aml_ipc_gdc_set_configfile(gsp->video.correction,
                             gs_config.ts.gdc.config_file);

  // create buffer pool for gdc
  vbp[0] = aml_ipc_vbpool_new(3, &fmt);

  // isp -> gdc [ch0]
  aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR,
               AML_IPC_COMPONENT(gsp->video.correction), AML_GDC_CH0);
  // buffer pool -> gdc [ch1]
  aml_ipc_bind(AML_IPC_COMPONENT(vbp[0]), AML_VBPOOL_OUTPUT,
               AML_IPC_COMPONENT(gsp->video.correction), AML_GDC_CH1);

  if (gsp->video.should_isp_scale_down) {
    // set the width/height for scaling
    fmt.width = gs_config.ts.main.resolution.width;
    fmt.height = gs_config.ts.main.resolution.height;
  }
  // create buffer pool for rotator
  vbp[1] = aml_ipc_vbpool_new(3, &fmt);
  // create rotator
  gsp->video.rotator = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_FROM);

  aml_ipc_ge2d_set_rotation(gsp->video.rotator, gs_config.rotation);
  aml_ipc_ge2d_set_passthrough(gsp->video.rotator,
                               gs_config.rotation == 0 &&
                                   !gsp->video.should_isp_scale_down);

  // gdc -> rotator
  aml_ipc_bind(AML_IPC_COMPONENT(gsp->video.correction), AML_GDC_OUTPUT,
               AML_IPC_COMPONENT(gsp->video.rotator), AML_GE2D_CH0);
  // buffer pool -> rotator [ch1]
  aml_ipc_bind(AML_IPC_COMPONENT(vbp[1]), AML_VBPOOL_OUTPUT,
               AML_IPC_COMPONENT(gsp->video.rotator), AML_GE2D_CH1);

  // create osd
  struct AmlIPCOSDApp *osd = gsp->video.osd = aml_ipc_osd_app_new();
  aml_ipc_osd_app_set_refresh_mode(osd, AML_OSD_APP_REFRESH_PARTIAL);
  aml_ipc_osd_app_add_module(osd, aml_ipc_osd_clock_new());
  aml_ipc_osd_app_add_module(osd, aml_ipc_osd_text_new());
  aml_ipc_osd_app_add_module(osd, aml_ipc_osd_image_new());
  gsp->video.osd_module_nn = aml_ipc_osd_nn_new();
  aml_ipc_osd_app_add_module(osd, gsp->video.osd_module_nn);
  // rotator -> queue
  struct AmlIPCQueue *queue = aml_ipc_queue_new(3);
  aml_ipc_bind(AML_IPC_COMPONENT(gsp->video.rotator), AML_GE2D_OUTPUT,
               AML_IPC_COMPONENT(queue), AML_QUEUE_INPUT);
  // queue -> osd
  aml_ipc_bind(AML_IPC_COMPONENT(queue), AML_QUEUE_OUTPUT,
               AML_IPC_COMPONENT(osd), AML_OSD_APP_INPUT);

  // create shielding
  gsp->video.shielding = aml_ipc_shielding_new();
  // add hook function in rotator component to shield video frame
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(gsp->video.rotator), AML_GE2D_OUTPUT,
                         gsp->video.shielding, on_shielding_frame);

  // add into pipeline
  aml_ipc_pipeline_add_many(
      gsp->video.pipeline,               //
      AML_IPC_COMPONENT(gsp->video.isp), //
      AML_IPC_COMPONENT(imgcap),         //
      AML_IPC_COMPONENT(gsp->video.correction), AML_IPC_COMPONENT(vbp[0]),
      AML_IPC_COMPONENT(gsp->video.rotator), AML_IPC_COMPONENT(vbp[1]),
      AML_IPC_COMPONENT(queue), AML_IPC_COMPONENT(osd), NULL);
  return AML_STATUS_OK;
}

/*
 * osd --> queue --> venc --> appsrc hook
 */
static AmlStatus main_channel_create() {
  // rtsp main channel
  // create video encoder
  enum AmlIPCVideoCodec codec = get_vcodec_type(gs_config.ts.main.codec);
  gsp->video.main.venc = aml_ipc_venc_new(codec);
  apply_venc_param(gsp->video.main.venc, &gs_config.ts.main);
  // create queue for venc input
  struct AmlIPCQueue *queue = aml_ipc_queue_new(3);
  struct AmlIPCOSDApp *osd = gsp->video.osd;
  // osd -> queue
  aml_ipc_bind(AML_IPC_COMPONENT(osd), AML_OSD_APP_OUTPUT,
               AML_IPC_COMPONENT(queue), AML_QUEUE_INPUT);
  // queue -> venc
  aml_ipc_bind(AML_IPC_COMPONENT(queue), AML_QUEUE_OUTPUT,
               AML_IPC_COMPONENT(gsp->video.main.venc), AML_VENC_INPUT);

  // add into pipeline
  aml_ipc_pipeline_add_many(gsp->video.pipeline, //
                            AML_IPC_COMPONENT(queue),
                            AML_IPC_COMPONENT(gsp->video.main.venc), //
                            NULL);

  gsp->app.main.video.ready_to_feed = 0;
  gsp->app.main.video.name = "[RTSP]Main.video";
  gsp->app.main.audio.ready_to_feed = 0;
  gsp->app.main.audio.name = "[RTSP]Main.audio";
  gsp->app.main.backchannel.name = "[RTSP]Main.backchannel";

  // add encoder frame hook
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(gsp->video.main.venc),
                         AML_VENC_OUTPUT, &gsp->app.main.video, on_enc_frame);
  return AML_STATUS_OK;
}

/*
 * osd --> scaler --> venc --> appsrc hook
 */
static AmlStatus sub_channel_create(int index) {
  if (index > SUB_CHANNEL_COUNT)
    return AML_STATUS_OK;

  if (!gs_config.ts.sub_enabled[index])
    return AML_STATUS_OK;
  // rtsp sub channel
  struct AmlIPCVideoFormat fmt = {AML_PIXFMT_NV12,
                                  gs_config.ts.sub[index].resolution.width,
                                  gs_config.ts.sub[index].resolution.height,
                                  gs_config.ts.sub[index].framerate};

  enum AmlIPCVideoCodec codec = get_vcodec_type(gs_config.ts.sub[index].codec);

  // create queue
  struct AmlIPCQueue *queue = aml_ipc_queue_new(3);

  // create scaler
  struct AmlIPCGE2D *scaler = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_FROM);
  // create buffer pool for scale
  struct AmlIPCVBPool *vbp = aml_ipc_vbpool_new(3, &fmt);
  // create video encoder
  gsp->video.sub[index].venc = aml_ipc_venc_new(codec);
  apply_venc_param(gsp->video.sub[index].venc, &gs_config.ts.sub[index]);

  if (gsp->video.web.scaler) {
    aml_ipc_bind(AML_IPC_COMPONENT(gsp->video.web.scaler), AML_GE2D_OUTPUT,
                 AML_IPC_COMPONENT(scaler), AML_GE2D_CH0);
  } else {
    // gdc -> queue
    aml_ipc_bind(AML_IPC_COMPONENT(gsp->video.osd), AML_OSD_APP_OUTPUT,
                 AML_IPC_COMPONENT(scaler), AML_GE2D_CH0);
  }
  // buffer pool -> scaler [ch1]
  aml_ipc_bind(AML_IPC_COMPONENT(vbp), AML_VBPOOL_OUTPUT,
               AML_IPC_COMPONENT(scaler), AML_GE2D_CH1);

  // scaler -> queue
  aml_ipc_bind(AML_IPC_COMPONENT(scaler), AML_GE2D_OUTPUT,
               AML_IPC_COMPONENT(queue), AML_QUEUE_INPUT);
  // queue -> venc
  aml_ipc_bind(AML_IPC_COMPONENT(queue), AML_QUEUE_OUTPUT,
               AML_IPC_COMPONENT(gsp->video.sub[index].venc), AML_VENC_INPUT);

  aml_ipc_pipeline_add_many(gsp->video.pipeline, //
                            AML_IPC_COMPONENT(scaler), AML_IPC_COMPONENT(vbp),
                            AML_IPC_COMPONENT(queue),
                            AML_IPC_COMPONENT(gsp->video.sub[index].venc),
                            NULL);

  int sz;
  gsp->app.sub[index].video.ready_to_feed = 0;
  sz = snprintf(NULL, 0, SUB_CHANNEL_VNAME, index) + 1;
  gsp->app.sub[index].video.name = ipc_mem_new0(char, sz);
  snprintf(gsp->app.sub[index].video.name, sz, SUB_CHANNEL_VNAME, index);

  gsp->app.sub[index].audio.ready_to_feed = 0;
  sz = snprintf(NULL, 0, SUB_CHANNEL_ANAME, index) + 1;
  gsp->app.sub[index].audio.name = ipc_mem_new0(char, sz);
  snprintf(gsp->app.sub[index].audio.name, sz, SUB_CHANNEL_ANAME, index);

  sz = snprintf(NULL, 0, SUB_CHANNEL_BNAME, index) + 1;
  gsp->app.sub[index].backchannel.name = ipc_mem_new0(char, sz);
  snprintf(gsp->app.sub[index].backchannel.name, sz, SUB_CHANNEL_BNAME, index);

  // add encoder frame hook
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(gsp->video.sub[index].venc),
                         AML_VENC_OUTPUT, &gsp->app.sub[index].video,
                         on_enc_frame);

  return AML_STATUS_OK;
}

static AmlStatus sub_channel_free() {
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    if (gsp->app.sub[i].video.name != NULL)
      free(gsp->app.sub[i].video.name);
    if (gsp->app.sub[i].audio.name != NULL)
      free(gsp->app.sub[i].audio.name);
    if (gsp->app.sub[i].backchannel.name != NULL)
      free(gsp->app.sub[i].backchannel.name);
  }
  return AML_STATUS_OK;
}

/*
 * if main stream encoded as h264, then the encoded stream is directly pushed to
 * webstream.
 * venc --> appsrc hook
 *
 * if main stream encoded as h265, then another h264 pipeline should
 * be created.
 * osd --> scaler --> queue --> venc --> appsrc hook
 */
static AmlStatus webstream_create() {
  enum AmlIPCVideoCodec codec = get_vcodec_type(gs_config.ts.main.codec);
  struct AmlIPCVenc *venc = NULL;
  if (codec == AML_VCODEC_H264) {
    // use main stream
    gsp->video.web.venc = NULL;
    gsp->video.web.scaler = NULL;
    venc = gsp->video.main.venc;
  } else {
    // create 720p stream
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_NV12, 1280, 720, 30};

    // create queue
    struct AmlIPCQueue *queue = aml_ipc_queue_new(3);

    // create scaler
    struct AmlIPCGE2D *scaler = gsp->video.web.scaler =
        aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_FROM);
    // create buffer pool for scale
    struct AmlIPCVBPool *vbp = aml_ipc_vbpool_new(3, &fmt);
    // create video encoder
    venc = gsp->video.web.venc = aml_ipc_venc_new(AML_VCODEC_H264);
    struct video_configs cfg = {
        .bitrate = 2000, .gop = 30, .bitrate_type = "CBR"};
    apply_venc_param(gsp->video.web.venc, &cfg);

    // osd -> scaler
    aml_ipc_bind(AML_IPC_COMPONENT(gsp->video.osd), AML_OSD_APP_OUTPUT,
                 AML_IPC_COMPONENT(scaler), AML_GE2D_CH0);
    // buffer pool -> scaler [ch1]
    aml_ipc_bind(AML_IPC_COMPONENT(vbp), AML_VBPOOL_OUTPUT,
                 AML_IPC_COMPONENT(scaler), AML_GE2D_CH1);

    // scaler -> queue
    aml_ipc_bind(AML_IPC_COMPONENT(scaler), AML_GE2D_OUTPUT,
                 AML_IPC_COMPONENT(queue), AML_QUEUE_INPUT);
    // queue -> venc
    aml_ipc_bind(AML_IPC_COMPONENT(queue), AML_QUEUE_OUTPUT,
                 AML_IPC_COMPONENT(gsp->video.web.venc), AML_VENC_INPUT);
    // add into pipeline
    aml_ipc_pipeline_add_many(gsp->video.pipeline, //
                              AML_IPC_COMPONENT(scaler), AML_IPC_COMPONENT(vbp),
                              AML_IPC_COMPONENT(queue),
                              AML_IPC_COMPONENT(gsp->video.web.venc), NULL);
  }

  char *pipeline_str;
  int sz =
      snprintf(NULL, 0, WEBSTREAM_PIPELINE_STRING, gs_config.ts.web.port) + 1;
  pipeline_str = ipc_mem_new0(char, sz);
  snprintf(pipeline_str, sz, WEBSTREAM_PIPELINE_STRING, gs_config.ts.web.port);

  gsp->app.web.video.ready_to_feed = 0;
  gsp->app.web.video.name = "Web Stream";
  gsp->gst.pipeline = gst_parse_launch(pipeline_str, NULL);
  gsp->app.web.video.appsrc =
      gst_bin_get_by_name(GST_BIN(gsp->gst.pipeline), "vin");
  free(pipeline_str);

  gsp->app.web.audio.appsrc = NULL;

  // add encoder frame hook
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(venc), AML_VENC_OUTPUT,
                         &gsp->app.web.video, on_enc_frame);
  bind_enc_to_appsrc(&gsp->app.web);
  return AML_STATUS_OK;
}

/*
 * main stream venc --> appsrc hook
 */
static AmlStatus vr_channel_create() {
  struct AmlIPCVenc *venc = gsp->video.main.venc;

  // link venc to appsrc
  struct ipc_recsrc appsrc;
  if (ipc_recording_get_appsrc(&appsrc) != AML_STATUS_OK) {
    // recording not enabled
    return AML_STATUS_OK;
  }
  gsp->app.vr.video.appsrc = appsrc.video;
  gsp->app.vr.video.ready_to_feed = 0;
  gsp->app.vr.video.name = "[VR]Video";
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(venc), AML_VENC_OUTPUT,
                         &gsp->app.vr.video, on_enc_frame);

  // audio
  // create audio converter
  struct AmlIPCAudioFormat fmt = {AML_ACODEC_PCM_S16LE, 48000, 16, 2};
  struct AmlIPCAConvert *aconv = aml_ipc_aconv_new(&fmt);
  aml_ipc_aconv_set_channel_mapping(aconv, "10");
  // create audio codec
  struct AmlIPCACodecFDK *aenc = aml_ipc_fdkaac_new(AML_ACODEC_AAC, AML_AAC_LC);
  aml_ipc_fdkaac_set_bitrate(aenc, 64000);
  // create audio queue, share the audio input with video recording pipeline
  struct AmlIPCQueue *queue = aml_ipc_queue_new(3);
  // ain -> queue
  aml_ipc_bind(gsp->audio.ain, AML_AIN_OUTPUT, AML_IPC_COMPONENT(queue),
               AML_QUEUE_INPUT);
  // queue -> aconv
  aml_ipc_bind(AML_IPC_COMPONENT(queue), AML_QUEUE_OUTPUT,
               AML_IPC_COMPONENT(aconv), AML_ACONVERT_INPUT);
  // aconv -> aenc
  aml_ipc_bind(AML_IPC_COMPONENT(aconv), AML_ACONVERT_OUTPUT,
               AML_IPC_COMPONENT(aenc), AML_ACODEC_INPUT);

  aml_ipc_pipeline_add_many(gsp->audio.pipeline, //
                            AML_IPC_COMPONENT(queue), AML_IPC_COMPONENT(aconv),
                            AML_IPC_COMPONENT(aenc), NULL);

  // link aenc to appsrc
  gsp->app.vr.audio.appsrc = appsrc.audio;
  gsp->app.vr.audio.ready_to_feed = 0;
  gsp->app.vr.audio.name = "[VR]Audio";

  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(aenc), AML_ACODEC_OUTPUT,
                         &gsp->app.vr.audio, on_enc_frame);

  bind_enc_to_appsrc(&gsp->app.vr);
  return AML_STATUS_OK;
}

/*
 * DS1 --> rotator --> queue --> nn
 *            |
 *            +--> shielding hook
 */
static AmlStatus nn_channel_create() {
  struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGB_888, 1280, 720, 15};
  // ds1 set as video recording channel
  aml_ipc_isp_set_capture_format(gsp->video.isp, AML_ISP_DS1, &fmt);

  // create buffer pool for rotator
  struct AmlIPCVBPool *vbp = aml_ipc_vbpool_new(3, &fmt);
  // create rotator
  gsp->video.nn.rotator = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_FROM);

  aml_ipc_ge2d_set_rotation(gsp->video.nn.rotator, gs_config.rotation);
  aml_ipc_ge2d_set_passthrough(gsp->video.nn.rotator, gs_config.rotation == 0);

  // ds1 -> rotator
  aml_ipc_bind(AML_IPC_COMPONENT(gsp->video.isp), AML_ISP_DS1,
               AML_IPC_COMPONENT(gsp->video.nn.rotator), AML_GE2D_CH0);
  // buffer pool -> rotator [ch1]
  aml_ipc_bind(AML_IPC_COMPONENT(vbp), AML_VBPOOL_OUTPUT,
               AML_IPC_COMPONENT(gsp->video.nn.rotator), AML_GE2D_CH1);

  struct AmlIPCQueue *queue = aml_ipc_queue_new(3);
  // rotator -> queue
  aml_ipc_bind(AML_IPC_COMPONENT(gsp->video.nn.rotator), AML_GE2D_OUTPUT,
               AML_IPC_COMPONENT(queue), AML_QUEUE_INPUT);

  // create nn
  struct AmlIPCNN *nn = aml_ipc_nn_new();
  // queue -> nn
  aml_ipc_bind(AML_IPC_COMPONENT(queue), AML_QUEUE_OUTPUT,
               AML_IPC_COMPONENT(nn), AML_NN_INPUT);
  // framehook to push nn detection result
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(nn), AML_NN_OUTPUT, NULL,
                         nn_push_result);

  // create video shielding
  gsp->video.nn.shielding = aml_ipc_shielding_new();
  // add hook function in rotator component to shield video frame
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(gsp->video.nn.rotator),
                         AML_GE2D_OUTPUT, gsp->video.nn.shielding,
                         on_shielding_frame);
  // add into pipeline
  aml_ipc_pipeline_add_many(
      gsp->video.pipeline, //
      AML_IPC_COMPONENT(vbp), AML_IPC_COMPONENT(gsp->video.nn.rotator),
      AML_IPC_COMPONENT(queue), AML_IPC_COMPONENT(nn), NULL);

  return AML_STATUS_OK;
}

AmlStatus audio_channel_create() {
  // check audio format
  gsp->audio.enc.format.codec = get_audio_codec(gs_config.audio.codec);
  gsp->audio.enc.format.num_channel = 1;
  // only 1 channel is supported by g711 and g726
  // otherwise, use the configured value
  if (gsp->audio.enc.format.codec == AML_ACODEC_AAC)
    gsp->audio.enc.format.num_channel = gs_config.audio.channels;
  gsp->audio.enc.format.sample_rate = gs_config.audio.samplerate;
  gsp->audio.enc.format.sample_width = 16;
  gsp->audio.enc.bitrate = gs_config.audio.bitrate * 1000;

  // create audio pipeline
  gsp->audio.pipeline = aml_ipc_pipeline_new();
  // create audio input
  struct AmlIPCComponent *ain = gsp->audio.ain =
      AML_IPC_COMPONENT(aml_ipc_ain_new());
  // int blksize = 1024 * (gs_config.audio.bitrate / 8);
  int blksize = 768 * 3;
  aml_obj_set_properties(AML_OBJECT(ain), "device", "default", "codec",
                         AML_ACODEC_PCM_S16LE, "rate", 48000, "blksize",
                         blksize, NULL);
  // create audio converter
  struct AmlIPCAudioFormat fmt = gsp->audio.enc.format;
  fmt.codec = AML_ACODEC_PCM_S16LE;
  struct AmlIPCAConvert *aconv = aml_ipc_aconv_new(&fmt);
  aml_ipc_aconv_set_channel_mapping(aconv, "10");
  struct AmlIPCComponent *aenc = NULL;
  // create audio codec
  switch (gsp->audio.enc.format.codec) {
  case AML_ACODEC_PCM_ULAW:
    aenc = AML_IPC_COMPONENT(aml_ipc_g711_new(gsp->audio.enc.format.codec));
    break;
  case AML_ACODEC_ADPCM_G726: {
    enum AmlG726BitRate bitrate = (enum AmlG726BitRate)(gsp->audio.enc.bitrate);
    aenc = AML_IPC_COMPONENT(
        aml_ipc_g726_new(gsp->audio.enc.format.codec, bitrate, AML_G726_LE));
  } break;
  case AML_ACODEC_AAC: {
    struct AmlIPCACodecFDK *fdk =
        aml_ipc_fdkaac_new(gsp->audio.enc.format.codec, AML_AAC_LC);
    aml_ipc_fdkaac_set_bitrate(fdk, gsp->audio.enc.bitrate);
    aenc = AML_IPC_COMPONENT(fdk);
  } break;
  default:
    AML_LOGE("audio codec not supported: %s", gs_config.audio.codec);
    break;
  }
  // ain -> aconv
  aml_ipc_bind(ain, AML_AIN_OUTPUT, AML_IPC_COMPONENT(aconv),
               AML_ACONVERT_INPUT);
  // aconv -> aenc
  aml_ipc_bind(AML_IPC_COMPONENT(aconv), AML_ACONVERT_OUTPUT, aenc,
               AML_ACODEC_INPUT);

  aml_ipc_add_frame_hook(aenc, AML_ACODEC_OUTPUT, &gsp->app.main.audio,
                         on_enc_frame);

  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    if (gs_config.ts.sub_enabled[i]) {
      aml_ipc_add_frame_hook(aenc, AML_ACODEC_OUTPUT, &gsp->app.sub[i].audio,
                             on_enc_frame);
    }
  }

  aml_ipc_pipeline_add_many(gsp->audio.pipeline, ain, AML_IPC_COMPONENT(aconv),
                            aenc, NULL);
  return AML_STATUS_OK;
}

static void rtsp_set_role_permission(GstRTSPMediaFactory *factory,
                                     const char *username) {
  if (factory == NULL || username == NULL || username[0] == '\0')
    return;
  gst_rtsp_media_factory_add_role(
      factory, username, GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN,
      TRUE, GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);
}

static void rtsp_set_auth(GstRTSPServer *server, const char *username,
                          const char *passwd) {
  if (NULL == server)
    return;

  GstRTSPAuth *auth = NULL;
  if (NULL == username || NULL == passwd || username[0] == '\0' ||
      passwd[0] == '\0') {
    AML_LOGD("remove auth");
    goto set_auth_mgr;
  }

  GstRTSPToken *token;
  gchar *basic;
  /* make a new authentication manager */
  auth = gst_rtsp_auth_new();

  /* make admin token */
  token = gst_rtsp_token_new(GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING,
                             username, NULL);

  basic = gst_rtsp_auth_make_basic(username, passwd);
  gst_rtsp_auth_add_basic(auth, basic, token);

  g_free(basic);
  gst_rtsp_token_unref(token);
  AML_LOGD("set auth, %s:%s", username, passwd);

set_auth_mgr:
  /* set as the server authentication manager */
  gst_rtsp_server_set_auth(server, auth);
  if (auth) {
    g_object_unref(auth);
  }
}

AmlStatus rtsp_server_create() {
  char portnum[16];
  snprintf(portnum, sizeof(portnum), "%d", gs_config.rtsp.network.port);
  gsp->gst.server = gst_rtsp_onvif_server_new();
  gst_rtsp_server_set_address(gsp->gst.server, gs_config.rtsp.network.address);
  gst_rtsp_server_set_service(gsp->gst.server, portnum);
  gsp->gst.mounts = gst_rtsp_server_get_mount_points(gsp->gst.server);
  // main -> rtsp
  bind_enc_to_rtsp(&gsp->app.main, gsp->gst.mounts,
                   gs_config.rtsp.network.route.main, gs_config.ts.main.codec,
                   &gsp->audio.enc);
  // sub -> rtsp
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    if (gs_config.ts.sub_enabled[i]) {
      bind_enc_to_rtsp(&gsp->app.sub[i], gsp->gst.mounts,
                       gs_config.rtsp.network.route.sub[i],
                       gs_config.ts.sub[i].codec, &gsp->audio.enc);
    }
  }

  g_object_unref(gsp->gst.mounts);

  rtsp_set_auth(gsp->gst.server, gs_config.rtsp.auth.user,
                gs_config.rtsp.auth.passwd);

  return AML_STATUS_OK;
}

void config_app_hookreturn() {
  enum AmlIPCVideoCodec codec = get_vcodec_type(gs_config.ts.main.codec);
  gsp->app.main.video.hookreturn = AML_STATUS_HOOK_CONTINUE;
  if (codec == AML_VCODEC_H264) {
    // main -> web ->vr (h264)
    gsp->app.web.video.hookreturn = AML_STATUS_HOOK_CONTINUE;
  } else {
    // main -> vr (h265)
    // web (h264)
    gsp->app.web.video.hookreturn = AML_STATUS_OK;
  }
  gsp->app.vr.video.hookreturn = AML_STATUS_OK;

  if (gs_config.ts.sub_enabled[0] || gs_config.ts.sub_enabled[1]) {
    gsp->app.main.audio.hookreturn = AML_STATUS_HOOK_CONTINUE;
  } else {
    gsp->app.main.audio.hookreturn = AML_STATUS_OK;
  }
  gsp->app.sub[0].video.hookreturn = AML_STATUS_HOOK_CONTINUE;
  gsp->app.sub[0].audio.hookreturn = AML_STATUS_HOOK_CONTINUE;
  gsp->app.sub[1].video.hookreturn = AML_STATUS_OK;
  gsp->app.sub[1].audio.hookreturn = AML_STATUS_OK;
}

static bool delay_start_audio_pipeline(void *data) {
  struct ipc_pipelines *pln = (struct ipc_pipelines *)data;
  if (pln->app.main.video.base_pts > 0 || pln->app.web.video.base_pts > 0) {
    AML_LOGV("video pipeline ready, start audio pipeline ...");
    AML_COMP_START(gsp->audio.pipeline);
    pln->audio.timer = 0;
    return IPC_TIMEOUT_TASK_STOP;
  } else {
    AML_LOGV("video pipeline not ready, waiting ...");
    return IPC_TIMEOUT_TASK_CONTINUE;
  }
}

AmlStatus ipc_pipeline_start() {
  if (gsp)
    return AML_STATUS_OK;

  gsp = ipc_mem_new0(struct ipc_pipelines, 1);

  load_configs(gsp);

  ipc_recording_create(gs_config.ts.main.codec);

  audio_channel_create();

  video_source_create();
  main_channel_create();
  webstream_create();

  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    sub_channel_create(i);
  }

  vr_channel_create();
  nn_channel_create();

  rtsp_server_create();

  load_roi_config(gsp);

  config_app_hookreturn();

  ipc_day_night_switch_init(gsp->video.isp);

  ipc_recording_start();

  // delay start audio pipeline until video frame ready
  // resolve the av sync issue
  gsp->audio.timer = ipc_timeout_task_add(10, delay_start_audio_pipeline, gsp);
  AML_COMP_START(gsp->video.pipeline);

  // startup webstream pipeline
  if (gsp->gst.pipeline) {
    gst_element_set_state(gsp->gst.pipeline, GST_STATE_PLAYING);
  }
  gst_rtsp_server_attach(gsp->gst.server, NULL);

  register_config_handle(gsp);
  return AML_STATUS_OK;
}

void ipc_pipeline_stop() {
  unregister_config_handle();
  ipc_isp_config_unload();

  ipc_day_night_switch_deinit();

  if (gsp->rtsp.timer > 0) {
    ipc_timeout_task_remove(gsp->rtsp.timer);
    gsp->rtsp.timer = 0;
  }
  if (gsp->video.roi.timer > 0) {
    ipc_timeout_task_remove(gsp->video.roi.timer);
    gsp->video.roi.timer = 0;
  }

  if (gsp->audio.timer) {
    ipc_timeout_task_remove(gsp->audio.timer);
    gsp->audio.timer = 0;
  }

  if (gsp->gst.pipeline) {
    gst_element_set_state(gsp->gst.pipeline, GST_STATE_NULL);
  }

  GST_OBJ_FREE(gsp->app.main.video.appsrc);
  GST_OBJ_FREE(gsp->app.main.audio.appsrc);
  for (int i = 0; i < SUB_CHANNEL_COUNT; i++) {
    GST_OBJ_FREE(gsp->app.sub[i].video.appsrc);
    GST_OBJ_FREE(gsp->app.sub[i].audio.appsrc);
  }
  GST_OBJ_FREE(gsp->app.web.video.appsrc);
  GST_OBJ_FREE(gsp->app.web.audio.appsrc);

  ipc_recording_stop();
  ipc_recording_destroy();
  gsp->app.vr.video.appsrc = NULL;
  gsp->app.vr.audio.appsrc = NULL;

  AML_COMP_STOP(gsp->audio.pipeline);
  AML_COMP_STOP(gsp->video.pipeline);

  aml_ipc_shielding_free(gsp->video.nn.shielding);
  aml_ipc_shielding_free(gsp->video.shielding);

  sub_channel_free();

  AML_OBJ_RELEASE(gsp->audio.pipeline);
  AML_OBJ_RELEASE(gsp->video.pipeline);

  if (gsp->log.trace.file) {
    aml_ipc_trace_set_json_output(NULL);
    if (gsp->log.trace.file != stdout && gsp->log.trace.file != stderr) {
      fclose(gsp->log.trace.file);
    }
    gsp->log.trace.file = NULL;
  }

  if (gsp->log.debug.file) {
    aml_ipc_log_set_output_file(NULL);
    if (gsp->log.debug.file != stdout && gsp->log.debug.file != stderr) {
      fclose(gsp->log.debug.file);
    }
    gsp->log.debug.file = NULL;
  }

  FREE_STR(gsp->log.graph.file);

  free(gsp);
  gsp = NULL;
}
