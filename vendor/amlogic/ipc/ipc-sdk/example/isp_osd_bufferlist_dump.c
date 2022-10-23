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
    if (frame && num_dump > 0) {
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

struct osd_timestamp {
    struct AmlIPCOSDContext *ctx;
    struct AmlIPCOSDFrame *surface[2]; // double buffer
    int current_surface_index;
};

struct osd_img {
    struct AmlIPCOSDContext *ctx;
};

struct osd_rect {
    struct AmlIPCOSDContext *ctx;
    struct AmlRect rect;
};

struct osds_overlay {
    struct AmlIPCOSDFrameList *list;
    struct AmlIPCGE2D *blender;
    struct osd_timestamp ts;
    struct osd_img img;
    struct osd_rect rect;
    FILE *fp;
};

#define FONT_FILE "/usr/share/directfb-1.7.7/decker.ttf"
#define FONT_SIZE 32
#define FONT_FG AML_RGBA(255, 255, 255, 255)
#define FONT_BG AML_RGBA(0, 0, 0, 0)

static void draw_timestamp(struct osds_overlay *posds) {
    struct osd_timestamp *ts = &posds->ts;
    time_t t = time(NULL);
    char buf[64];

    // YYYY-MM-DD hh:mm:ss (total: 19 char)
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));

    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 0, 0, 0};
    struct AmlIPCOSDFrame *current_surface;
    // create osd draw context
    if (ts->ctx == NULL) {
        ts->ctx = aml_ipc_osd_context_new();
        aml_ipc_osd_load_font(ts->ctx, FONT_FILE, FONT_SIZE);
        aml_ipc_osd_set_color(ts->ctx, FONT_FG);
        aml_ipc_osd_set_bgcolor(ts->ctx, FONT_BG);
        int maxw, maxh;
        aml_ipc_osd_get_font_bbox(ts->ctx, &maxw, &maxh);
        fmt.width = maxw * 19;
        fmt.height = maxh;

        ts->surface[0] = aml_ipc_osd_frame_new_alloc(&fmt);
        ts->surface[1] = aml_ipc_osd_frame_new_alloc(&fmt);
        ts->current_surface_index = 0;

        aml_ipc_osd_frame_list_append(posds->list, ts->surface[1]);
    }

    // preload text to get the surface size
    aml_ipc_osd_preload_text(ts->ctx, buf, &fmt.width, &fmt.height);

    current_surface =
        ts->surface[ts->current_surface_index]; // get the current working surface
    ts->current_surface_index =
        (ts->current_surface_index + 1) % 2; // switch surface for the next update

    // call start paint at each paint beginning
    aml_ipc_osd_start_paint(ts->ctx, AML_IPC_VIDEO_FRAME(current_surface));
    aml_ipc_osd_clear_rect(ts->ctx, NULL);
    // draw text surface
    int top = 0, left = 0, bottom = top, right = left;
    aml_ipc_osd_draw_text(ts->ctx, buf, &bottom, &right);
    // call done paint at each paint ending
    aml_ipc_osd_done_paint(ts->ctx);

    // set the surface position for blending
    aml_ipc_osd_frame_set_src_rect(current_surface, 0, 0, fmt.width, fmt.height);
    aml_ipc_osd_frame_set_dst_rect(current_surface, 10, 10, fmt.width, fmt.height);

    aml_ipc_osd_frame_list_replace(posds->list, ts->surface[ts->current_surface_index], current_surface);
    return;
}

static void draw_img(struct osds_overlay *posds) {
    struct osd_img *img = &posds->img;
    if (img->ctx == NULL) {
        img->ctx = aml_ipc_osd_context_new();
    }
    const char *pngfile = "/var/www/ipc-webui/logo-amlogic.png";
    int w, h;
    aml_ipc_osd_preload_png(img->ctx, pngfile, &w, &h);
    struct AmlIPCOSDFrame *png_overlay =
        aml_ipc_osd_frame_new_alloc(&(struct AmlIPCVideoFormat){AML_PIXFMT_RGBA_8888, w, h, 0});
    aml_ipc_osd_start_paint(img->ctx, AML_IPC_VIDEO_FRAME(png_overlay));
    aml_ipc_osd_load_png(img->ctx, pngfile, 0, 0);
    aml_ipc_osd_done_paint(img->ctx);
    aml_ipc_osd_frame_set_src_rect(png_overlay, 0, 0, w, h);
    aml_ipc_osd_frame_set_dst_rect(png_overlay, 500, 500, w, h);
    aml_ipc_osd_frame_list_append(posds->list, png_overlay);

    return;
}

static void draw_rect(struct osds_overlay *posds) {
    struct osd_rect *rect = &posds->rect;
    if (rect->ctx == NULL) {
        rect->ctx = aml_ipc_osd_context_new();
    }
    aml_ipc_osd_set_bgcolor(rect->ctx, AML_RGBA(255, 255, 255, 0));
    aml_ipc_osd_set_color(rect->ctx, AML_RGBA(0, 0, 0, 0));

    int rcWidth = 300;
    int rcHeight = 200;

    struct AmlIPCOSDFrame *rect_overlay =
        aml_ipc_osd_frame_new_alloc(&(struct AmlIPCVideoFormat){AML_PIXFMT_RGBA_8888, rcWidth, rcHeight, 0});

    struct AmlRect rect_info = {0, 0, rcWidth, rcHeight};
    aml_ipc_osd_start_paint(rect->ctx, AML_IPC_VIDEO_FRAME(rect_overlay));
    aml_ipc_osd_clear_rect(rect->ctx, NULL);
    aml_ipc_osd_draw_rect(rect->ctx, &rect_info);
    aml_ipc_osd_fill_rect(rect->ctx, &rect_info, AML_RGBA(0, 0, 0, 128));
    aml_ipc_osd_done_paint(rect->ctx);
    aml_ipc_osd_frame_set_src_rect(rect_overlay, 0, 0, rcWidth, rcHeight);
    aml_ipc_osd_frame_set_dst_rect(rect_overlay, rand() % (1920 - rcWidth),
        rand() % (1080 - rcHeight), rcWidth, rcHeight);
    aml_ipc_osd_frame_list_append(posds->list, rect_overlay);

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

    struct osds_overlay osds;
    memset(&osds, 0, sizeof(struct osds_overlay));

    osds.blender = aml_ipc_ge2d_new(AML_GE2D_OP_ALPHABLEND_TO);
    osds.list = aml_ipc_osd_frame_list_new();
    aml_ipc_ge2d_set_static_buffer(osds.blender, (struct AmlIPCFrame *)osds.list);

    draw_timestamp(&osds);
    draw_img(&osds);
    draw_rect(&osds);

    // isp -> blender
    aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR, AML_IPC_COMPONENT(osds.blender), AML_GE2D_CH0);

    // add frame hook to dump data for output
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(osds.blender), AML_GE2D_OUTPUT, dump_fp, on_frame);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(isp), osds.blender, NULL);
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
    aml_ipc_start(AML_IPC_COMPONENT(isp));
#endif

    while (!bexit) {
        draw_timestamp(&osds);
        usleep(1000 * 10);
    }

    fclose(dump_fp);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    aml_obj_release(AML_OBJECT(pipeline));
#else
    aml_ipc_stop(AML_IPC_COMPONENT(osds.blender));
    aml_ipc_stop(AML_IPC_COMPONENT(isp));
    aml_obj_release(AML_OBJECT(osds.blender));
    aml_obj_release(AML_OBJECT(isp));
#endif

    if (osds.ts.surface[osds.ts.current_surface_index])
        aml_obj_release(AML_OBJECT(osds.ts.surface[osds.ts.current_surface_index]));

    return 0;
}
