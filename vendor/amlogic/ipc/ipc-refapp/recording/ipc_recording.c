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

#include <dirent.h>
#include <pthread.h>
#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include "ipc_common.h"
#include "ipc_config.h"
#include "ipc_recording.h"
#include "ipc_timeout_task.h"

AML_LOG_DEFINE(recording);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(recording)

#define RECORDING_PIPELINE_STRING                                              \
  "splitmuxsink name=muxer"                                                    \
  " appsrc name=vin"                                                           \
  " ! %sparse name=vparse config-interval=1"                                   \
  " ! muxer."                                                                  \
  " appsrc name=ain"                                                           \
  " ! aacparse name=aparse"                                                    \
  " ! muxer.audio_0"

struct vr_pipeline {
  GstElement *pipeline;
  GstElement *muxer;
  struct {
    GstElement *appsrc;
  } video;
  struct {
    GstElement *appsrc;
  } audio;

  char *codec;
  char *filename;
  char *tempfile;

  bool is_running;

  unsigned long chunk_timer;
  unsigned long space_freeup_timer;
};

struct configs {
  bool enabled;
  char *location;
  int chunk_duration;
  int64_t reserved_space_size;
};

static void stop_chunk_timer(struct vr_pipeline *rec);
static bool chunk_timer_callback(void *data);
static void start_chunk_timer(struct vr_pipeline *rec);
static void stop_space_freeup_timer(struct vr_pipeline *rec);
static bool space_freeup_timer_callback(void *data);
static void start_space_freeup_timer(struct vr_pipeline *rec);

static struct configs gs_config;

#define CONFIG_PARENT_KEY "/ipc/recording/"

static bool split_file(struct vr_pipeline *rec) {
  GstFlowReturn ret;
  g_signal_emit_by_name(rec->muxer, "split-after", NULL, &ret);
  return ret == GST_FLOW_OK;
}

static void restart_chunker(struct vr_pipeline *rec) {
  if (!gs_config.enabled)
    return;

  stop_chunk_timer(rec);
  split_file(rec);
  start_chunk_timer(rec);
}

static void onchange_enabled(void *user_data) {
  if (gs_config.enabled) {
    ipc_recording_start();
  } else {
    ipc_recording_stop();
  }
}

static void onchange_chunk_duration(void *user_data) {
  struct vr_pipeline *rec = (struct vr_pipeline *)user_data;
  AML_LOGD("recording chunk duration changed...");
  restart_chunker(rec);
}

void onchange_location(void *user_data) {
  struct vr_pipeline *rec = (struct vr_pipeline *)user_data;
  AML_LOGD("recording location changed...");
  restart_chunker(rec);
}

void onchange_reserved_space_size(void *user_data) {
  if (!gs_config.enabled)
    return;

  struct vr_pipeline *rec = (struct vr_pipeline *)user_data;
  AML_LOGD("recording chunk duration changed...");
  stop_space_freeup_timer(rec);
  space_freeup_timer_callback(rec);
  start_space_freeup_timer(rec);
}

static struct ipc_config_kv gs_kvs[] = {
    IPC_CONFIG_KV_BOOL(CONFIG_PARENT_KEY "enabled", &gs_config.enabled, false,
                       onchange_enabled),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "chunk-duration",
                    &gs_config.chunk_duration, 5, onchange_chunk_duration),
    IPC_CONFIG_KV_I(CONFIG_PARENT_KEY "reserved-space-size",
                    &gs_config.reserved_space_size, 200,
                    onchange_reserved_space_size),
    IPC_CONFIG_KV_STR(CONFIG_PARENT_KEY "location", &gs_config.location, "",
                      onchange_location),
    {NULL},
};

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

static AmlStatus register_config_handle(struct vr_pipeline *pipeline) {
  return ipc_config_register_handle(CONFIG_PARENT_KEY, on_config_changed,
                                    pipeline);
}

static AmlStatus unregister_config_handle() {
  return ipc_config_unregister_handle(CONFIG_PARENT_KEY, on_config_changed);
}

static AmlStatus load_configs(struct vr_pipeline *pipeline) {
  for (int i = 0; gs_kvs[i].key != NULL; i++) {
    ipc_config_load_kv(&gs_kvs[i]);
  }
  return AML_STATUS_OK;
}

bool delay_stop_pipeline(void *user_data) {
  ipc_recording_stop();
  return IPC_TIMEOUT_TASK_STOP;
}

static gboolean on_source_message(GstBus *bus, GstMessage *message,
                                  struct vr_pipeline *rec) {
  gboolean ret = TRUE;
  switch (GST_MESSAGE_TYPE(message)) {
  case GST_MESSAGE_ERROR:
    if (rec->muxer == GST_ELEMENT_CAST(message->src)) {
      AML_LOGE("recording pipeline error, stopping ...");
      ipc_timeout_task_add(10, delay_stop_pipeline, NULL);
    } else {
      GError *err = NULL;
      gchar *dbg_info = NULL;

      gst_message_parse_error(message, &err, &dbg_info);
      AML_LOGE("ERROR from element %s: %s\n", GST_OBJECT_NAME(message->src),
               err->message);
      AML_LOGE("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
      g_error_free(err);
      g_free(dbg_info);
      ipc_request_exit();
    }
    ret = FALSE;
    break;
  default:
    break;
  }
  return ret;
}

static int64_t get_available_space(const char *location) {
  struct statvfs stat;
  if (statvfs(location, &stat) != 0) {
    return -1;
  }

  return stat.f_bsize * stat.f_bavail;
}

struct media_file_info {
  TAILQ_ENTRY(media_file_info) node;
  char *file;
};

TAILQ_HEAD(media_file_list_head, media_file_info);

int media_file_filter(const struct dirent *d) {
  if (d->d_type != DT_REG) {
    return 0;
  }

  regex_t reg;
  const char *pattern =
      "[[:digit:]]\\{4\\}-[[:digit:]]\\{2\\}-[[:digit:]]\\{2\\}_"
      "[[:digit:]]\\{2\\}-[[:digit:]]\\{2\\}-[[:digit:]]\\{2\\}"
      "\\.mp4";
  if (0 != regcomp(&reg, pattern, 0)) {
    AML_LOGE("pattern could not be compiled");
    return 0;
  }
  int ret = regexec(&reg, d->d_name, 0, NULL, 0) == 0;
  regfree(&reg);
  return ret;
}

static void load_media_file_list(const char *location,
                                 struct media_file_list_head *list) {
  struct dirent **namelist;
  int n;

  n = scandir(location, &namelist, media_file_filter, alphasort);
  if (n == -1) {
    AML_LOGE("scandir failed");
    return;
  }

  for (int i = 0; i < n; i++) {
    char *fn = namelist[i]->d_name;
    struct media_file_info *info = ipc_mem_new0(struct media_file_info, 1);
    info->file = strdup(fn);
    TAILQ_INSERT_TAIL(list, info, node);
    free(namelist[i]);
  }
  free(namelist);
}

static void freeup_space(const char *location, int64_t reserved_space_size) {
  int64_t valid_space = get_available_space(location);
  if (valid_space > reserved_space_size) {
    return;
  }

  struct media_file_list_head filelist;
  TAILQ_INIT(&filelist);
  load_media_file_list(location, &filelist);
  if (TAILQ_EMPTY(&filelist))
    return;

  struct media_file_info *info = TAILQ_FIRST(&filelist);
  while (info) {
    struct media_file_info *next = TAILQ_NEXT(info, node);
    if (valid_space < reserved_space_size) {
      int sz = snprintf(NULL, 0, "%s/%s", location, info->file) + 1;
      char *file_to_remove = ipc_mem_new0(char, sz);
      snprintf(file_to_remove, sz, "%s/%s", location, info->file);
      AML_LOGD("\n"
               "  removing %s ... \n"
               "    free space        : %" PRId64 " bytes\n"
               "    request to reserve: %" PRId64 " bytes",
               file_to_remove, valid_space, reserved_space_size);
      unlink(file_to_remove);
      free(file_to_remove);
      valid_space = get_available_space(location);
    }
    TAILQ_REMOVE(&filelist, info, node);
    free(info->file);
    free(info);
    info = next;
  }
}

static void save_media_file(struct vr_pipeline *rec) {
  if (rec->filename == NULL || strlen(rec->filename) == 0)
    return;

  rename(rec->tempfile, rec->filename);
}

static void create_dir(const char *path) {
  DIR *dir = opendir(path);
  if (!dir) {
    if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
      perror("create storage dir");
    }
  }
  closedir(dir);
}

char *build_filename() {
  char buf[64];
  time_t now;
  now = time(NULL);
  strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", localtime(&now));
  return strdup(buf);
}

static gchar *set_file_location(GstElement *splitmux, guint fragment_id,
                                gpointer *user_data) {
  struct vr_pipeline *rec = (struct vr_pipeline *)(user_data);
  // save the previous temp recording file
  save_media_file(rec);

  FREE_STR(rec->filename);
  FREE_STR(rec->tempfile);

  if (gs_config.enabled && gs_config.location[0] != '\0') {
    // check dir existance
    create_dir(gs_config.location);
    char *file = build_filename();

    int sz = snprintf(NULL, 0, "%s/%s.mp4", gs_config.location, file) + 1;
    rec->filename = ipc_mem_new0(char, sz);
    snprintf(rec->filename, sz, "%s/%s.mp4", gs_config.location, file);
    free(file);

    sz = snprintf(NULL, 0, "%s/.recording.tmp", gs_config.location) + 1;
    rec->tempfile = ipc_mem_new0(char, sz);
    snprintf(rec->tempfile, sz, "%s/.recording.tmp", gs_config.location);
  } else {
    rec->tempfile = strdup("/dev/null");
    rec->filename = strdup("");
  }

  AML_LOGD("saving file to %s",
           rec->filename[0] == '\0' ? rec->tempfile : rec->filename);
  return strdup(rec->tempfile);
}

static bool chunk_timer_callback(void *data) {
  struct vr_pipeline *rec = (struct vr_pipeline *)data;
  if (rec->pipeline == NULL) {
    AML_LOGE("In chunk timeout callback, pipeline was NULL!");

    return IPC_TIMEOUT_TASK_STOP;
  }

  split_file(rec);

  return IPC_TIMEOUT_TASK_CONTINUE;
}

static bool space_freeup_timer_callback(void *data) {
  struct vr_pipeline *rec = (struct vr_pipeline *)data;
  if (rec->pipeline == NULL) {
    AML_LOGE("In space_freeup timeout callback, pipeline was NULL!");

    return G_SOURCE_REMOVE;
  }

  freeup_space(gs_config.location, gs_config.reserved_space_size << 20);

  return G_SOURCE_CONTINUE;
}

static bool recording_init(struct vr_pipeline *rec) {
  char *pipeline_str;

  int sz = snprintf(NULL, 0, RECORDING_PIPELINE_STRING, rec->codec) + 1;
  pipeline_str = ipc_mem_new0(char, sz);
  snprintf(pipeline_str, sz, RECORDING_PIPELINE_STRING, rec->codec);

  rec->chunk_timer = 0;
  rec->space_freeup_timer = 0;
  rec->is_running = false;
  rec->pipeline = gst_parse_launch(pipeline_str, NULL);
  free(pipeline_str);

  gst_element_set_name(rec->pipeline, "video recording pipeline");

  rec->muxer = gst_bin_get_by_name(GST_BIN(rec->pipeline), "muxer");

  g_signal_connect(rec->muxer, "format-location", G_CALLBACK(set_file_location),
                   rec);

  rec->video.appsrc = gst_bin_get_by_name(GST_BIN(rec->pipeline), "vin");

  rec->audio.appsrc = gst_bin_get_by_name(GST_BIN(rec->pipeline), "ain");

  return true;
}

static void recording_uninit(struct vr_pipeline *rec) {
  GST_OBJ_FREE(rec->pipeline);

  GST_OBJ_FREE(rec->video.appsrc);
  GST_OBJ_FREE(rec->audio.appsrc);
  GST_OBJ_FREE(rec->muxer);
}

static void start_chunk_timer(struct vr_pipeline *rec) {
  int interval = gs_config.chunk_duration * 60; // seconds

  if (rec->chunk_timer) {
    ipc_timeout_task_remove(rec->chunk_timer);
    rec->chunk_timer = 0;
    AML_LOGD("chunk timer not closed");
  }

  if (interval > 0) {
    AML_LOGD("media chunk duration: %d minutes", interval / 60);
    rec->chunk_timer =
        ipc_timeout_task_add(interval * 1000, chunk_timer_callback, rec);
  }
}

static void stop_chunk_timer(struct vr_pipeline *rec) {
  if (rec->chunk_timer) {
    ipc_timeout_task_remove(rec->chunk_timer);
    rec->chunk_timer = 0;
  }
}

static void start_space_freeup_timer(struct vr_pipeline *rec) {
  if (rec->chunk_timer)
    return;

  rec->space_freeup_timer =
      ipc_timeout_task_add(1 * 1000, space_freeup_timer_callback, rec);
}

static void stop_space_freeup_timer(struct vr_pipeline *rec) {
  if (rec->space_freeup_timer) {
    ipc_timeout_task_remove(rec->space_freeup_timer);
    rec->space_freeup_timer = 0;
  }
}

struct vr_pipeline *gsp = NULL;

AmlStatus ipc_recording_create(const char *codec) {
  if (gsp)
    return AML_STATUS_OK;

  gsp = ipc_mem_new0(struct vr_pipeline, 1);

  load_configs(gsp);

  gsp->codec = strdup(codec);
  if (!recording_init(gsp)) {
    return AML_STATUS_FAIL;
  }

  register_config_handle(gsp);
  return AML_STATUS_OK;
}

AmlStatus ipc_recording_destroy() {
  if (gsp == NULL)
    return AML_STATUS_OK;

  AML_LOGD("destroying recording pipeline");
  unregister_config_handle();

  recording_uninit(gsp);

  if (gsp->codec)
    free(gsp->codec);

  free(gsp);
  gsp = NULL;
  AML_LOGD("recording pipeline destroyed");
  return AML_STATUS_OK;
}

AmlStatus ipc_recording_start() {
  if (!gs_config.enabled) {
    AML_LOGD("Storage disabled");
    return AML_STATUS_FAIL;
  }

  if (gsp == NULL) {
    AML_LOGE("pipeline not created");
    return AML_STATUS_FAIL;
  }

  if (gsp->is_running == true) {
    return AML_STATUS_OK;
  }

  start_space_freeup_timer(gsp);

  GstBus *bus = gst_element_get_bus(gsp->pipeline);
  gst_bus_add_watch(bus, (GstBusFunc)on_source_message, gsp);
  gst_object_unref(bus);

  AML_LOGD("start recording pipeline...");

  if (gsp->muxer == NULL) {
    gsp->muxer = gst_element_factory_make("splitmuxsink", "muxer");

    g_signal_connect(gsp->muxer, "format-location",
                     G_CALLBACK(set_file_location), gsp);

    gst_bin_add(GST_BIN(gsp->pipeline), gsp->muxer);
    GstPad *sinkpad;
    if (!(sinkpad = gst_element_get_static_pad(gsp->muxer, "video"))) {
      sinkpad = gst_element_get_request_pad(gsp->muxer, "video");
    }

    GstElement *parse = gst_bin_get_by_name(GST_BIN(gsp->pipeline), "vparse");
    GstPad *srcpad = gst_element_get_static_pad(parse, "src");
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(parse);
    gst_object_unref(srcpad);

    if (!(sinkpad = gst_element_get_static_pad(gsp->muxer, "audio_0"))) {
      sinkpad = gst_element_get_request_pad(gsp->muxer, "audio_0");
      gst_object_unref(sinkpad);
    }
    parse = gst_bin_get_by_name(GST_BIN(gsp->pipeline), "aparse");
    srcpad = gst_element_get_static_pad(parse, "src");
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(parse);
    gst_object_unref(srcpad);
  }

  gst_element_set_state(gsp->pipeline, GST_STATE_PLAYING);

  start_chunk_timer(gsp);

  AML_LOGD("recording pipeline activated ...");

  gsp->is_running = true;
  return AML_STATUS_OK;
}

AmlStatus ipc_recording_stop() {
  if (gsp == NULL || gsp->pipeline == NULL || !gsp->is_running) {
    return AML_STATUS_OK;
  }

  gsp->is_running = false;

  /* terminating, set pipeline to NULL and clean up */
  AML_LOGD("Closing recording pipeline");

  stop_chunk_timer(gsp);
  stop_space_freeup_timer(gsp);

  GstBus *bus = gst_element_get_bus(gsp->pipeline);
  gst_bus_remove_watch(bus);
  gst_object_unref(bus);

  gst_element_send_event(gsp->pipeline, gst_event_new_eos());
  /* wait for EOS message on the pipeline bus */
  GstMessage *msg = gst_bus_timed_pop_filtered(
      GST_ELEMENT_BUS(gsp->pipeline), 1000 * GST_MSECOND,
      GST_MESSAGE_EOS | GST_MESSAGE_ERROR);
  gst_message_unref(msg);

  gst_element_set_state(gsp->pipeline, GST_STATE_NULL);
  gst_bin_remove(GST_BIN(gsp->pipeline), gsp->muxer);
  gsp->muxer = NULL;

  save_media_file(gsp);

  FREE_STR(gsp->filename);
  FREE_STR(gsp->tempfile);

  AML_LOGD("recording pipeline stopped");
  return AML_STATUS_OK;
}

AmlStatus ipc_recording_get_appsrc(struct ipc_recsrc *appsrcs) {
  AmlStatus ret = AML_STATUS_FAIL;
  if (appsrcs == NULL || gsp == NULL) {
    return ret;
  }
  appsrcs->video = gsp->video.appsrc;
  appsrcs->audio = gsp->audio.appsrc;
  return AML_STATUS_OK;
}
