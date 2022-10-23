#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "aml_ipc_sdk.h"

struct cmd_val {
  char *device;
  char *enc;
  char *file;
};

static volatile int bexit = 0;

static int num_dump = 100;

void signal_handle(int sig) { bexit = 1; }

AmlStatus on_frame(struct AmlIPCFrame *frame, void *param) {
  FILE *fp = (FILE *)param;
  if (frame && num_dump) {
    if (fp) {
      fwrite(frame->data, 1, frame->size, fp);
    }
  } else {
    bexit = 1;
  }
  num_dump--;
  return AML_STATUS_OK;
}

void print_help() {
  printf("------------------------------------\n");
  printf("-d      : device\n");
  printf("-n      : number of frame\n");
  printf("-e      : video encoder codec, h264 or h265\n");
  printf("-o      : output file\n");
  printf("-h      : print this help message\n");
  printf("------------------------------------\n");
  exit(0);
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
  int ch;
  while ((ch = getopt(argc, argv, "hd:n:f:e:o:")) != -1) {
    switch (ch) {
    case 'h':
      print_help();
      break;
    case 'd':
      val->device = optarg;
      break;
    case 'n':
      num_dump = atoi(optarg);
      break;
    case 'e':
      val->enc = optarg;
      break;
    case 'o':
      val->file = optarg;
      break;
    default:
      print_help();
    }
  }
  printf("device: %s\n"
         "number of frame: %d\n"
         "video encoder codec: %s\n"
         "output file: %s\n",
         val->device, num_dump, val->enc, val->file);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_help();
  }
  struct cmd_val val = {"/dev/video0", "h264", "/root/dump.dat"};

  if (parse_cmd(argc, argv, &val) < 0) {
    return -1;
  }

  struct AmlIPCVideoFormat fmt = {AML_PIXFMT_NV12, 1920, 1080, 30};

  enum AmlIPCVideoCodec codec = AML_VCODEC_H264;
  if (strcmp(val.enc, "h265") == 0) {
    codec = AML_VCODEC_H265;
  } else if (strcmp(val.enc, "h264") != 0) {
    printf("!!! video encoder is not supported: %s !!!\n", val.enc);
    return -1;
  }

  FILE *dump_fp = fopen(val.file, "wb");
  if (dump_fp == NULL) {
    printf("file open failed\n");
    return -1;
  }

  aml_ipc_sdk_init();

  const char *debuglevel = getenv("AML_DEBUG");
  if (!debuglevel)
    debuglevel = ".*:3";
  aml_ipc_log_set_from_string(debuglevel);
  const char *tracelevel = getenv("AML_TRACE");
  if (!tracelevel)
    tracelevel = ".*:3";
  aml_ipc_trace_set_from_string(tracelevel);

  signal(SIGINT, signal_handle);

  struct AmlIPCISP *isp = aml_ipc_isp_new(val.device, 4, 0, 0);
  aml_ipc_isp_set_capture_format(isp, AML_ISP_FR, &fmt);

  struct AmlIPCVenc *enc = aml_ipc_venc_new(codec);
  aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR, AML_IPC_COMPONENT(enc), AML_VENC_INPUT);
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(enc), AML_VENC_OUTPUT, dump_fp, on_frame);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
  struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
  aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(isp), enc, NULL);
  aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
  aml_ipc_start(AML_IPC_COMPONENT(enc));
  aml_ipc_start(AML_IPC_COMPONENT(isp));
#endif

  while (!bexit) {
    usleep(1000 * 10);
  }

  fclose(dump_fp);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
  aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
  aml_obj_release(AML_OBJECT(pipeline));
#else
  aml_ipc_stop(AML_IPC_COMPONENT(enc));
  aml_ipc_stop(AML_IPC_COMPONENT(isp));
  aml_obj_release(AML_OBJECT(enc));
  aml_obj_release(AML_OBJECT(isp));
#endif
  return 0;
}
