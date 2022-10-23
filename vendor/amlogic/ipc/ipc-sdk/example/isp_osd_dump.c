#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "aml_ipc_sdk.h"

struct cmd_val {
    char *device;
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
    printf("-o      : output file\n");
    printf("-h      : print this help message\n");
    printf("------------------------------------\n");
    exit(0);
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
    int ch;
    while ((ch = getopt(argc, argv, "hd:n:o:")) != -1) {
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
        case 'o':
            val->file = optarg;
            break;
        default:
            print_help();
        }
    }
    printf("device: %s\n"
           "number of frame: %d\n"
           "output file: %s\n\n",
           val->device, num_dump, val->file);
    return 0;
}

struct osd_comp {
    struct AmlIPCGE2D *blender;
    struct AmlIPCOSDContext *ctx;
    struct AmlIPCOSDFrame *surface[2]; // double buffer
    int current_surface_index;
};

#define FONT_FILE "/usr/share/directfb-1.7.7/decker.ttf"
#define FONT_SIZE 32
#define FONT_FG AML_RGBA(255, 255, 255, 255)
#define FONT_BG AML_RGBA(0, 0, 0, 0)

static void draw_osd(struct osd_comp *comp) {
    struct tm *t;
    time_t now;
    char buf[256];

    // generate the time string
    now = time(NULL);
    t = localtime(&now);
    if (t == NULL)
        sprintf(buf, "--:--:--");
    if (strftime(buf, sizeof(buf), "%F %H:%M:%S", t) == 0)
        return;

    // create osd draw context
    if (comp->ctx == NULL) {
        comp->ctx = aml_ipc_osd_context_new();
        aml_ipc_osd_load_font(comp->ctx, FONT_FILE, FONT_SIZE);
        aml_ipc_osd_set_color(comp->ctx, FONT_FG);
        aml_ipc_osd_set_bgcolor(comp->ctx, FONT_BG);
    }

    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 0, 0, 0};
    // preload text to get the surface size
    aml_ipc_osd_preload_text(comp->ctx, buf, &fmt.width, &fmt.height);

    struct AmlIPCOSDFrame *current_surface =
        comp->surface[comp->current_surface_index]; // get the current working surface
    comp->current_surface_index =
        (comp->current_surface_index + 1) % 2; // switch surface for the next update

    if (current_surface) {
        // check if the surface size changed
        if (AML_IPC_VIDEO_FRAME(current_surface)->format.width < fmt.width ||
            AML_IPC_VIDEO_FRAME(current_surface)->format.height < fmt.height) {
            // release surface if width / height is not match
            aml_obj_release(AML_OBJECT(current_surface));
            current_surface = NULL;
        }
    }
    // allocate osd surface
    if (current_surface == NULL) {
        current_surface = aml_ipc_osd_frame_new_alloc(&fmt);
    }

    // call start paint at each paint beginning
    aml_ipc_osd_start_paint(comp->ctx, AML_IPC_VIDEO_FRAME(current_surface));
    aml_ipc_osd_clear_rect(comp->ctx, NULL);
    // draw text surface
    int top = 0, left = 0, bottom = top, right = left;
    aml_ipc_osd_draw_text(comp->ctx, buf, &bottom, &right);
    // call done paint at each paint ending
    aml_ipc_osd_done_paint(comp->ctx);

    // set the surface position for blending
    aml_ipc_osd_frame_set_src_rect(current_surface, 0, 0, fmt.width, fmt.height);
    aml_ipc_osd_frame_set_dst_rect(current_surface, 10, 10, fmt.width, fmt.height);

    // surface would be unref while removing from static buffer
    // add ref here
    aml_obj_addref(AML_OBJECT(current_surface));
    // flip new surface
    aml_ipc_ge2d_set_static_buffer(comp->blender, AML_IPC_FRAME(current_surface));

    return;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
    }
    struct cmd_val val = {"/dev/video0", "/root/dump.dat"};

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

    struct osd_comp osd;
    memset(&osd, 0, sizeof(struct osd_comp));
    osd.blender = aml_ipc_ge2d_new(AML_GE2D_OP_ALPHABLEND_TO);

    // isp -> blender
    aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR, AML_IPC_COMPONENT(osd.blender), AML_GE2D_CH0);

    // add frame hook to dump data for output
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(osd.blender), AML_GE2D_OUTPUT, dump_fp, on_frame);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(isp), osd.blender, NULL);
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
    aml_ipc_start(AML_IPC_COMPONENT(isp));
#endif

    while (!bexit) {
        draw_osd(&osd);
        usleep(1000 * 10);
    }

    fclose(dump_fp);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    aml_obj_release(AML_OBJECT(pipeline));
#else
    aml_ipc_stop(AML_IPC_COMPONENT(isp));
    aml_obj_release(AML_OBJECT(osd.blender));
    aml_obj_release(AML_OBJECT(isp));
#endif
    aml_obj_release(AML_OBJECT(osd.ctx));
    for (int i = 0; i < 2; i++) {
        if (osd.surface[i]) {
            aml_obj_release(AML_OBJECT(osd.surface[i]));
        }
    }
    return 0;
}
