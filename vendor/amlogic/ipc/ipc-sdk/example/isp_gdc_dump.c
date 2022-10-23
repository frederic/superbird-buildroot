#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "aml_ipc_sdk.h"

struct cmd_val {
  char *device;
  char *config_file;
  char *file;
};

static volatile int bexit = 0;

static int num_dump = 100;

void signal_handle(int sig) { bexit = 1; }

AmlStatus on_frame(struct AmlIPCFrame *frame, void *param) {
  FILE *fp = (FILE *)param;
  if (frame && num_dump) {
    if (fp) {
      struct AmlIPCVideoFrame *vfrm = (struct AmlIPCVideoFrame *)frame;
      struct AmlIPCVideoBuffer *dmabuf = vfrm->dmabuf;
      aml_ipc_dma_buffer_map(dmabuf, AML_MAP_READ);
      fwrite(dmabuf->data, 1, dmabuf->size, fp);
      aml_ipc_dma_buffer_unmap(dmabuf);
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
  printf("-g      : gdc config file\n");
  printf("-o      : output file\n");
  printf("-h      : print this help message\n");
  printf("------------------------------------\n");
  exit(0);
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
  int ch;
  while ((ch = getopt(argc, argv, "hd:n:g:e:o:")) != -1) {
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
    case 'g':
      val->config_file = optarg;
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
         "gdc config file: %s\n"
         "output file: %s\n",
         val->device, num_dump, val->config_file, val->file);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_help();
  }
  struct cmd_val val = {
      "/dev/video0",
      "/etc/gdc_config/affine-1920_1080_180_1080_0_0-0_0_1920_1080-0_0_1.0-0-nv12.bin",
      "/root/dump.dat"};

  if (parse_cmd(argc, argv, &val) < 0) {
    return -1;
  }

  struct AmlIPCVideoFormat fmt = {AML_PIXFMT_NV12, 1920, 1080, 30};

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

  struct AmlIPCGDC *gdc = aml_ipc_gdc_new();
  aml_ipc_gdc_set_passthrough(gdc, false);
  if (aml_ipc_gdc_set_configfile(gdc, val.config_file) != AML_STATUS_OK) {
    printf("set config file fail\n");
    return -1;
  }

  struct AmlIPCVBPool *vbp = aml_ipc_vbpool_new(3, &fmt);

  aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR, AML_IPC_COMPONENT(gdc), AML_GDC_CH0);
  aml_ipc_bind(AML_IPC_COMPONENT(vbp), AML_VBPOOL_OUTPUT, AML_IPC_COMPONENT(gdc), AML_GDC_CH1);
  aml_ipc_add_frame_hook(AML_IPC_COMPONENT(gdc), AML_GDC_OUTPUT, dump_fp, on_frame);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
  struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
  aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(isp), gdc, vbp, NULL);
  aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
  aml_ipc_start(AML_IPC_COMPONENT(gdc));
  aml_ipc_start(AML_IPC_COMPONENT(vbp));
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
  aml_ipc_stop(AML_IPC_COMPONENT(gdc));
  aml_ipc_stop(AML_IPC_COMPONENT(vbp));
  aml_ipc_stop(AML_IPC_COMPONENT(isp));
  aml_obj_release(AML_OBJECT(vbp));
  aml_obj_release(AML_OBJECT(gdc));
  aml_obj_release(AML_OBJECT(isp));
#endif
  return 0;
}
