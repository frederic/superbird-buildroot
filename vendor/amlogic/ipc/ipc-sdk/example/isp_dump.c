#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aml_ipc_sdk.h"

struct cmd_val {
    char *device;
    int channel;
    char *file;
};

static const struct {
    enum AmlPixelFormat fmt;
    const char *name;
} fmt_list[] = {
    {AML_PIXFMT_RGB_565, "RGB565"},     {AML_PIXFMT_RGB_888, "RGB888"},
    {AML_PIXFMT_BGR_888, "BGR888"},     {AML_PIXFMT_RGBA_8888, "RGBA8888"},
    {AML_PIXFMT_BGRA_8888, "BGRA8888"}, {AML_PIXFMT_RGBX_8888, "RGBX8888"},
    {AML_PIXFMT_YU12, "YU12"},          {AML_PIXFMT_YV12, "YV12"},
    {AML_PIXFMT_NV12, "NV12"},          {AML_PIXFMT_NV21, "NV21"},
    {AML_PIXFMT_NV16, "NV16"},          {AML_PIXFMT_YUYV, "YUYV"},
    {AML_PIXFMT_UYVY, "UYVY"},
};

static volatile int bexit = 0;

static int num_dump = 100;

void signal_handle(int sig) { bexit = 1; }

AmlStatus on_frame_dummy(struct AmlIPCFrame *frame, void *param) { return AML_STATUS_OK; }

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

const char *get_fmt_name(enum AmlPixelFormat fmt) {
    for (int i = 0;; i++) {
        if (fmt_list[i].name == NULL)
            break;
        if (fmt_list[i].fmt == fmt)
            return fmt_list[i].name;
    }
    return NULL;
}

void print_fmt(struct AmlIPCVideoFormat *fmt, void *param) {
    printf("%s: %dx%d\n", get_fmt_name(fmt->pixfmt), fmt->width, fmt->height);
}

void print_help() {
    printf("------------------------------------\n");
    printf("-d      : device\n");
    printf("-c      : isp channel, 0: FR 1: DS1\n");
    printf("-n      : number of frame\n");
    printf("-o      : output file\n");
    printf("-h      : print this help message\n");
    printf("------------------------------------\n");
    exit(0);
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
    int ch;
    while ((ch = getopt(argc, argv, "hd:c:n:o:")) != -1) {
        switch (ch) {
        case 'h':
            print_help();
            break;
        case 'd':
            val->device = optarg;
            break;
        case 'c':
            val->channel = atoi(optarg);
            break;
        case 'n':
            num_dump = atoi(optarg);
            break;
        case 'o':
            val->file = optarg;
            break;
        default:
            print_help();
        }
    }
    printf("device: %s\n"
           "channel: %s\n"
           "number of frame: %d\n"
           "resolution: %s\n"
           "output file: %s\n",
           val->device, val->channel == 0 ? "FR" : "DS1", num_dump,
           val->channel == 0 ? "1920x1080" : "1280x720", val->file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
    }
    struct cmd_val val = {"/dev/video0", 0, "/root/dump.dat"};

    if (parse_cmd(argc, argv, &val) < 0) {
        return -1;
    }

    struct AmlIPCVideoFormat fmts[] = {{AML_PIXFMT_NV12, 1920, 1080, 30},
                                       {AML_PIXFMT_NV12, 1280, 720, 30}};

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

    struct AmlIPCISP *isp = aml_ipc_isp_new(val.device, 4, val.channel ? 4 : 0, 0);

    printf("Try format: \n");
    struct AmlIPCVideoFormat fmt = {0, 0, 0, 30};
    aml_ipc_isp_enumerate_capture_formats(isp, val.channel ? AML_ISP_DS1 : AML_ISP_FR, &fmt, NULL,
                                          print_fmt);
    aml_ipc_isp_set_capture_format(isp, AML_ISP_FR, &fmts[0]);

    if (val.channel) {
        // use ds1 as output
        aml_ipc_isp_set_capture_format(isp, AML_ISP_DS1, &fmts[1]);
        aml_ipc_add_frame_hook(AML_IPC_COMPONENT(isp), AML_ISP_FR, NULL, on_frame_dummy);
        aml_ipc_add_frame_hook(AML_IPC_COMPONENT(isp), AML_ISP_DS1, dump_fp, on_frame);
    } else {
        aml_ipc_add_frame_hook(AML_IPC_COMPONENT(isp), AML_ISP_FR, dump_fp, on_frame);
    }

    aml_ipc_start(AML_IPC_COMPONENT(isp));

    while (!bexit) {
        usleep(1000 * 10);
    }

    fclose(dump_fp);

    aml_ipc_stop(AML_IPC_COMPONENT(isp));
    aml_obj_release(AML_OBJECT(isp));
    return 0;
}
