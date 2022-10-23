#include "aml_ipc_sdk.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct cmd_val {
    const char *inputfile;
    const char *enc;
    const char *outputfile;
    const char *pixfmt;
    int width;
    int height;
};

static volatile int bexit = 0;

void signal_handle(int sig) { bexit = 1; }

AmlStatus on_frame(struct AmlIPCFrame *frame, void *param) {
    FILE *fp = (FILE *)param;
    if (frame) {
        if (fp) {
            fwrite(frame->data, 1, frame->size, fp);
        }
    } else {
        bexit = 1;
    }
    return AML_STATUS_OK;
}

void print_help() {
    printf("------------------------------------\n");
    printf("-i      : input file\n");
    printf("-e     : video encoder codec, h264 or h265\n");
    printf("-o     : output file\n");
    printf("-s     : output resolution, WxH\n");
    printf("-f     : output format, rgb or nv12\n");
    printf("-h    : print this help message\n");
    printf("------------------------------------\n");
    exit(0);
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
    int ch;
    while ((ch = getopt(argc, argv, "hi:e:o:f:s:")) != -1) {
        switch (ch) {
        case 'h':
            print_help();
            break;
        case 'i':
            val->inputfile = optarg;
            break;
        case 'e':
            val->enc = optarg;
            break;
        case 'o':
            val->outputfile = optarg;
            break;
        case 's':
            if (sscanf(optarg, "%dx%d", &val->width, &val->height) != 2) {
                printf("size:%s\n\n!!! resolution is not correct !!!\n", optarg);
                return -1;
            } else {
                if (val->width == 0 || val->width % 320 != 0) {
                    printf("\n!!! input correct width !!!\n");
                    return -1;
                } else if (val->height == 0) {
                    printf("\n!!! input correct height !!!\n");
                    return -1;
                }
            }
            break;
        case 'f':
            val->pixfmt = optarg;
            break;
        default:
            print_help();
        }
    }
    printf("input file: %s\n"
           "video encoder codec: %s\n"
           "output file: %s\n"
           "format: %s\n"
           "width: %d\n"
           "height: %d\n",
           val->inputfile, val->enc, val->outputfile, val->pixfmt, val->width, val->height);
    return 0;
}

int main(int argc, char *argv[]) {
    struct cmd_val val = {"/data/raw_video_nv12_1920_1080.dat",
                          "h264",
                          "/data/rawvideoparse_nv12_1920_1080_venc.dump",
                          "nv12",
                          1920,
                          1080};

    if (parse_cmd(argc, argv, &val) < 0) {
        return -1;
    }

    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_NV12, 1920, 1080, 30};
    if (strcmp(val.pixfmt, "rgb") == 0) {
        fmt.pixfmt = AML_PIXFMT_RGB_888;
    } else if (strcmp(val.pixfmt, "nv12") == 0) {
        fmt.pixfmt = AML_PIXFMT_NV12;
    }
    fmt.width = val.width;
    fmt.height = val.height;

    enum AmlIPCVideoCodec codec = AML_VCODEC_H264;
    if (strcmp(val.enc, "h265") == 0) {
        codec = AML_VCODEC_H265;
    } else if (strcmp(val.enc, "h264") != 0) {
        printf("!!! video encoder is not supported: %s !!!\n", val.enc);
        return -1;
    }

    FILE *dump_fp = fopen(val.outputfile, "wb");
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

    struct AmlIPCFileIn *filein = AML_OBJ_NEW(AmlIPCFileIn);
    aml_obj_set_properties(AML_OBJECT(filein), "path", val.inputfile, NULL);
    struct AmlIPCRawVideoParse *rvparse = aml_ipc_rawvideoparse_new(&fmt);
    struct AmlIPCVenc *venc = aml_ipc_venc_new(codec);
    aml_ipc_bind(AML_IPC_COMPONENT(filein), AML_FILEIN_OUTPUT, AML_IPC_COMPONENT(rvparse),
                 AML_RAWVIDEOPARSE_INPUT);
    aml_ipc_bind(AML_IPC_COMPONENT(rvparse), AML_RAWVIDEOPARSE_OUTPUT, AML_IPC_COMPONENT(venc),
                 AML_VENC_INPUT);
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(venc), AML_VENC_OUTPUT, dump_fp, on_frame);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(filein), AML_IPC_COMPONENT(rvparse),
                              AML_IPC_COMPONENT(venc), NULL);
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
    aml_ipc_start(AML_IPC_COMPONENT(venc));
    aml_ipc_start(AML_IPC_COMPONENT(rvparse));
    aml_ipc_start(AML_IPC_COMPONENT(filein));
#endif

    while (!bexit) {
        usleep(1000 * 10);
    }

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    aml_obj_release(AML_OBJECT(pipeline));
#else
    aml_ipc_stop(AML_IPC_COMPONENT(venc));
    aml_ipc_stop(AML_IPC_COMPONENT(rvparse));
    aml_ipc_stop(AML_IPC_COMPONENT(filein));
    aml_obj_release(AML_OBJECT(venc));
    aml_obj_release(AML_OBJECT(rvparse));
    aml_obj_release(AML_OBJECT(filein));
#endif
    fclose(dump_fp);
    return 0;
}