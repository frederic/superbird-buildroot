#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "aml_ipc_sdk.h"

static void dump_frame(struct AmlIPCVideoFrame *frame, const char *name, const char *mode) {
    struct AmlIPCVideoBuffer *dmabuf = frame->dmabuf;
    FILE *fp = fopen(name, mode);
    if (fp) {
        aml_ipc_dma_buffer_map(dmabuf, AML_MAP_READ);
        fwrite(dmabuf->data, 1, dmabuf->size, fp);
        aml_ipc_dma_buffer_unmap(dmabuf);
        fclose(fp);
    }
}

static void dump_to_osd(const char *name, struct AmlIPCOSDContext *ctx, struct AmlIPCVideoFrame *vfrm,
                       struct AmlIPCGE2D *ge2d4, struct AmlIPCOSDFrame *text3) {
    aml_ipc_ge2d_bitblt(ge2d4, vfrm, AML_IPC_VIDEO_FRAME(text3), NULL, 0, 0);
    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(text3));
    uint8_t *p =
        aml_ipc_dma_buffer_map(AML_IPC_VIDEO_FRAME(text3)->dmabuf, AML_MAP_READ | AML_MAP_WRITE);
    char buf[256];
    sprintf(buf, "r:%d g:%d b:%d a:%d", p[0], p[1], p[2], p[3]);
    aml_ipc_osd_set_color(ctx, AML_RGBA(255-p[0],255-p[1],255-p[2],255));
    aml_ipc_osd_set_bgcolor(ctx, AML_RGBA(p[0],p[1],p[2],255));
    int x = 0;
    int y = 0;
    aml_ipc_osd_draw_text(ctx, buf, &x, &y);
    switch (vfrm->format.pixfmt) {
        case AML_PIXFMT_RGB_888:
            sprintf(buf, "%s: rgb888", name);
            break;
        case AML_PIXFMT_RGBA_8888:
            sprintf(buf, "%s: rgba8888", name);
            break;
        case AML_PIXFMT_NV12:
            sprintf(buf, "%s: nv12", name);
            break;
        default:
            sprintf(buf, "%s: unsupport", name);
            break;
    }
    x = 0;
    y = 24;
    aml_ipc_osd_draw_text(ctx, buf, &x, &y);
    aml_ipc_osd_done_paint(ctx);
}

static int ge2d_test(int argc, const char *argv[]) {
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 320, 100, 0};
    struct AmlIPCOSDFrame *logo_rgba = aml_ipc_osd_frame_new_alloc(&fmt);
    struct AmlIPCOSDFrame *temp_rgba = aml_ipc_osd_frame_new_alloc(&fmt);
    struct AmlIPCOSDFrame *video_rgba = aml_ipc_osd_frame_new_alloc(&fmt);
    fmt.pixfmt = AML_PIXFMT_NV12;
    struct AmlIPCOSDFrame *video_nv12 = aml_ipc_osd_frame_new_alloc(&fmt);

    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(ctx, "/usr/share/directfb-1.7.7/decker.ttf", 16);
    uint32_t c1 = strtoul(argv[1], NULL, 0);
    uint32_t c2 = strtoul(argv[2], NULL, 0);

    struct AmlIPCGE2D *ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    aml_ipc_ge2d_fill_rect(ge2d, AML_IPC_VIDEO_FRAME(logo_rgba), NULL, c2);
    aml_ipc_ge2d_fill_rect(ge2d, AML_IPC_VIDEO_FRAME(temp_rgba), NULL, c1);
    aml_ipc_ge2d_fill_rect(ge2d, AML_IPC_VIDEO_FRAME(video_rgba), NULL, c1);
    aml_ipc_ge2d_bitblt(ge2d, AML_IPC_VIDEO_FRAME(video_rgba), AML_IPC_VIDEO_FRAME(video_nv12), NULL, 0, 0);

    dump_to_osd("logo", ctx, AML_IPC_VIDEO_FRAME(logo_rgba), ge2d, temp_rgba);
    dump_frame(AML_IPC_VIDEO_FRAME(temp_rgba), "/root/osd.data", "w");

    dump_to_osd("video", ctx, AML_IPC_VIDEO_FRAME(video_rgba), ge2d, temp_rgba);
    dump_frame(AML_IPC_VIDEO_FRAME(temp_rgba), "/root/osd.data", "a");

//    dump_to_osd("video", ctx, AML_IPC_VIDEO_FRAME(video_nv12), ge2d, temp_rgba);
//    dump_frame(AML_IPC_VIDEO_FRAME(temp_rgba), "/root/osd.data", "a");

    aml_ipc_ge2d_blend(ge2d, AML_IPC_VIDEO_FRAME(logo_rgba), AML_IPC_VIDEO_FRAME(video_rgba),
                       AML_IPC_VIDEO_FRAME(temp_rgba), NULL, 0, 0, 0, 0);
    dump_to_osd("logo+rgba", ctx, AML_IPC_VIDEO_FRAME(temp_rgba), ge2d, temp_rgba);
    dump_frame(AML_IPC_VIDEO_FRAME(temp_rgba), "/root/osd.data", "a");

    aml_ipc_ge2d_blend(ge2d, AML_IPC_VIDEO_FRAME(logo_rgba), AML_IPC_VIDEO_FRAME(video_nv12),
                       AML_IPC_VIDEO_FRAME(logo_rgba), NULL, 0, 0, 0, 0);
    dump_to_osd("logo+nv12", ctx, AML_IPC_VIDEO_FRAME(logo_rgba), ge2d, temp_rgba);
    dump_frame(AML_IPC_VIDEO_FRAME(temp_rgba), "/root/osd.data", "a");
    return 0;
}

static int fill_rect_test(int argc, const char *argv[]) {
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 320, 100, 0};
    struct AmlIPCOSDFrame *text = aml_ipc_osd_frame_new_alloc(&fmt);
    fmt.pixfmt = AML_PIXFMT_NV12;
    struct AmlIPCOSDFrame *nv12 = aml_ipc_osd_frame_new_alloc(&fmt);

    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(ctx, "/root/simsun.ttc", 24);
    struct AmlIPCGE2D *ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    uint32_t c1 = strtoul(argv[1], NULL, 0);
    aml_ipc_ge2d_fill_rect(ge2d, AML_IPC_VIDEO_FRAME(nv12), NULL, AML_RGBA(255,0,0,255));
    dump_to_osd("nv12", ctx, AML_IPC_VIDEO_FRAME(nv12), ge2d, text);
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "w");
    aml_ipc_ge2d_fill_rect(ge2d, AML_IPC_VIDEO_FRAME(nv12), NULL, AML_RGBA(0,255,0,255));
    dump_to_osd("nv12", ctx, AML_IPC_VIDEO_FRAME(nv12), ge2d, text);
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "a");
    aml_ipc_ge2d_fill_rect(ge2d, AML_IPC_VIDEO_FRAME(nv12), NULL, AML_RGBA(0,0,255,255));
    dump_to_osd("nv12", ctx, AML_IPC_VIDEO_FRAME(nv12), ge2d, text);
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "a");

    aml_ipc_ge2d_fill_rect(ge2d, AML_IPC_VIDEO_FRAME(nv12), NULL, c1);
    dump_to_osd("nv12", ctx, AML_IPC_VIDEO_FRAME(nv12), ge2d, text);
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "a");
    return 0;
}

static int osd_rect_test(int argc, const char *argv[]) {
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 320, 100, 0};
    struct AmlIPCOSDFrame *text = aml_ipc_osd_frame_new_alloc(&fmt);
    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    aml_ipc_osd_set_color(ctx, AML_RGBA(0x40, 0x80, 0xc0, 255));
    aml_ipc_osd_set_bgcolor(ctx, AML_RGBA(0, 0, 0, 255));
    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(text));
    aml_ipc_osd_clear_rect(ctx, NULL);
    aml_ipc_osd_draw_rect(ctx, NULL);
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "w");
    aml_ipc_osd_clear_rect(ctx, NULL);
    aml_ipc_osd_draw_rect(ctx, &(struct AmlRect){10, 20, 100, 60});
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "a");
    aml_ipc_osd_set_width(ctx, 5);
    aml_ipc_osd_clear_rect(ctx, NULL);
    aml_ipc_osd_draw_rect(ctx, &(struct AmlRect){10, 20, 100, 60});
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "a");
    aml_ipc_osd_clear_rect(ctx, NULL);
    aml_ipc_osd_draw_rect(ctx, &(struct AmlRect){-3, -2, 324, 104});
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "a");
    aml_ipc_osd_done_paint(ctx);
    return 0;
}

static int osd_text_test(int argc, const char *argv[]) {
    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(ctx, "/usr/share/directfb-1.7.7/decker.ttf", 500);
    int w,h;
    const char * str = "text";
    aml_ipc_osd_preload_text(ctx, str, &w, &h);
    printf("need %dx%d surface\n", w, h);
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, w, h, 0};
    struct AmlIPCOSDFrame *text = aml_ipc_osd_frame_new_alloc(&fmt);
    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(text));
    aml_ipc_osd_clear_rect(ctx, NULL);
    int x = 0, y = 0;
    aml_ipc_osd_draw_text(ctx, str, &x, &y);
    aml_ipc_osd_done_paint(ctx);
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "w");
    return 0;
}

static int osd_line_test(int argc, const char *argv[]) {
    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 320, 240, 0};
    struct AmlIPCOSDFrame *text = aml_ipc_osd_frame_new_alloc(&fmt);
    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(text));
    aml_ipc_osd_clear_rect(ctx, NULL);
    aml_ipc_osd_set_color(ctx, AML_RGBA(0xff, 0x00, 0x00, 255));
    aml_ipc_osd_draw_line(ctx, 50, 100, 100, 190);
    aml_ipc_osd_set_color(ctx, AML_RGBA(0x00, 0xff, 0x00, 255));
    aml_ipc_osd_draw_line(ctx, 100, 190, 150, 100);
    aml_ipc_osd_set_color(ctx, AML_RGBA(0x00, 0x00, 0xff, 255));
    aml_ipc_osd_draw_line(ctx, 150, 100, 100, 10);
    aml_ipc_osd_set_color(ctx, AML_RGBA(0x00, 0xff, 0xff, 255));
    aml_ipc_osd_draw_line(ctx, 100, 10, 50, 100);
    dump_frame(AML_IPC_VIDEO_FRAME(text), "/root/osd.data", "w");
    return 0;
}

static int osd_blend_test(int argc, const char *argv[]) {
    if (argc != 3) {
        printf("osd_blend_test fgcol bgcol, col format: 0xaabbggrr\n");
        exit(0);
    }
    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 64, 16, 0};
    struct AmlIPCOSDFrame *bgimg = aml_ipc_osd_frame_new_alloc(&fmt);
    struct AmlIPCOSDFrame *fgimg = aml_ipc_osd_frame_new_alloc(&fmt);

    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(fgimg));
    aml_ipc_osd_fill_rect(ctx, NULL, strtoul(argv[1], NULL, 0));
    aml_ipc_osd_done_paint(ctx);
    dump_frame(AML_IPC_VIDEO_FRAME(fgimg), "/root/osd.data", "w");

    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(bgimg));
    aml_ipc_osd_fill_rect(ctx, NULL, strtoul(argv[2], NULL, 0));
    aml_ipc_osd_done_paint(ctx);

    dump_frame(AML_IPC_VIDEO_FRAME(bgimg), "/root/osd.data", "a");

    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(bgimg));
    aml_ipc_osd_alpha_blend(ctx, AML_IPC_VIDEO_FRAME(fgimg), NULL, 0, 0);
    aml_ipc_osd_done_paint(ctx);

    dump_frame(AML_IPC_VIDEO_FRAME(bgimg), "/root/osd.data", "a");
    return 0;
}

static int osd_png_test(int argc, const char *argv[]) {
    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    int w,h;
    aml_ipc_osd_preload_png(ctx, argv[1], &w, &h);
    struct AmlIPCOSDFrame *frame =
        aml_ipc_osd_frame_new_alloc(&(struct AmlIPCVideoFormat){AML_PIXFMT_RGB_888, w, h, 0});
    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(frame));
    aml_ipc_osd_load_png(ctx, argv[1], 0, 0);
    aml_ipc_osd_done_paint(ctx);
    dump_frame(AML_IPC_VIDEO_FRAME(frame), "/root/osd.data", "w");
    return 0;
}

static int osd_jpeg_test(int argc, const char *argv[]) {
    struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
    int w,h;
    aml_ipc_osd_preload_jpeg(ctx, argv[1], &w, &h);
    struct AmlIPCOSDFrame *frame =
        aml_ipc_osd_frame_new_alloc(&(struct AmlIPCVideoFormat){AML_PIXFMT_RGBA_8888, w, h, 0});
    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(frame));
    aml_ipc_osd_load_jpeg(ctx, argv[1], 0, 0);
    aml_ipc_osd_done_paint(ctx);
    dump_frame(AML_IPC_VIDEO_FRAME(frame), "/root/osd.data", "w");
    return 0;
}

static int ge2d_perf_test(int argc, const char *argv[]) {
        int testitem = 0, count = 100, rotation = 0;
        char *stest = NULL, *sfmt = "rgba", *file_path = NULL;
        FILE *fp = NULL;
        struct AmlIPCVideoFrame *frame = NULL;
        struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, 1920, 1080, 0};
        int opt;
        while ((opt = getopt(argc, (char**)argv, "t:s:f:c:i:r:")) != -1) {
            switch (opt) {
            case 's':
                if (sscanf(optarg, "%dx%d", &fmt.width, &fmt.height) != 2) {
                    goto error;
                }
                break;
            case 't':
                stest = optarg;
                if (strcmp(optarg, "blit") == 0) testitem = 1;
                else if (strcmp(optarg, "sblit") == 0) testitem = 2;
                else if (strcmp(optarg, "blend") == 0) testitem = 3;
                else if (strcmp(optarg, "fill") == 0) testitem = 4;
                else goto error;
                break;
            case 'f':
                sfmt = optarg;
                if (strcmp(optarg, "rgb") == 0) fmt.pixfmt = AML_PIXFMT_RGB_888;
                else if (strcmp(optarg, "rgba") == 0) fmt.pixfmt = AML_PIXFMT_RGBA_8888;
                else if (strcmp(optarg, "nv12") == 0) fmt.pixfmt = AML_PIXFMT_NV12;
                else goto error;
                break;
            case 'c':
                count = atoi(optarg);
                break;
            case 'i':
                file_path = optarg;
                if ((fp = fopen(file_path, "rb")) == NULL)
                goto error;
                break;
            case 'r':
                rotation = atoi(optarg);
                break;
            default:
                printf("unrecognized option '%c'\n", opt);
    error:
                printf("USAGE: %s -t (blit|sblit|blend|fill) [-s wxh] [-f (rgb|rgba|nv12)] [-c count] [-i (file path)] [-r rotation]\n", argv[0]);
                exit(1);
            }
        }
        if (testitem == 0) goto error;
        if (file_path != NULL) {
            int framesize = fmt.width * fmt.height;
            switch (fmt.pixfmt) {
                case AML_PIXFMT_RGB_888:
                    framesize = framesize * 3;
                    break;
               case AML_PIXFMT_RGBA_8888:
                    framesize = framesize * 4;
                    break;
                case AML_PIXFMT_NV12:
                    framesize = framesize * 3/2;
                    break;
                 default:
                    AML_LOGW("pixel format error only support RGB888 |NV12 |RGBA8888");
                    exit(1);
            }
            struct AmlIPCVideoBuffer *dmabuf = AML_OBJ_NEW(AmlIPCVideoBuffer);
            if (aml_ipc_dma_buffer_alloc(dmabuf, framesize) != AML_STATUS_OK) {
                AML_LOGW("failed to allocate dmabuf %d bytes", framesize);
                aml_obj_release(AML_OBJECT(dmabuf));
                exit(1);
            }
            aml_ipc_dma_buffer_map(dmabuf, AML_MAP_WRITE);
            if (fread(dmabuf->data, 1, framesize, fp) != framesize) {
                aml_ipc_dma_buffer_unmap(dmabuf);
                aml_obj_release(AML_OBJECT(dmabuf));
                fclose(fp);
                exit(1);
            }
            fclose(fp);
            aml_ipc_dma_buffer_unmap(dmabuf);
            frame = AML_OBJ_NEW(AmlIPCVideoFrame);
            aml_ipc_video_frame_init(frame, &fmt, dmabuf, framesize /fmt.height);
        }
        else {
            frame = (struct AmlIPCVideoFrame*)aml_ipc_osd_frame_new_alloc(&fmt);
        }
        struct AmlIPCVideoFormat fmt2 = {AML_PIXFMT_RGBA_8888, fmt.width, fmt.height, 0};
        if (rotation == 90 || rotation == 270) {
            if (testitem == 1) {
                fmt2.height = fmt.width;
            }
            else if (testitem == 2) {
                fmt2.height = fmt.width;
                fmt2.width = fmt.height;
            }
        }
        struct AmlIPCVideoFrame *temp = (struct AmlIPCVideoFrame *)aml_ipc_osd_frame_new_alloc(&fmt2);
        struct AmlIPCGE2D *ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
        int i;
        AML_SCTIME_LOGW("test %s %dx%d fmt %s rotation %d %d times", stest, fmt.width, fmt.height, sfmt, rotation, count);
        switch (testitem) {
            case 1:
                for (i=0; i<count; ++i) {
                    aml_ipc_ge2d_bitblt(ge2d, frame, temp, NULL, 0, 0);
                }
                break;
            case 2:
                for (i=0; i<count; ++i) {
                    aml_ipc_ge2d_stretchblt(ge2d, frame, temp, NULL, NULL);
                }
                break;
            case 3:
                for (i=0; i<count; ++i) {
                    aml_ipc_ge2d_blend(ge2d, temp, frame, temp, NULL, 0, 0, 0, 0);
                }
                break;
            case 4:
                for (i=0; i<count; ++i) {
                    aml_ipc_ge2d_fill_rect(ge2d, frame, NULL, 0);
                }
                break;
        }
        return 0;
    }

static AmlStatus ain_hook(struct AmlIPCFrame *frm, void *param) {
    FILE *fp = (FILE *)param;
    static int dump_size = 48000 * 10 * 4;
    if (dump_size > 0) {
        fwrite(frm->data, 1, frm->size, fp);
        dump_size -= frm->size;
        if (dump_size <= 0) {
            fclose(fp);
        }
    }
    AML_LOGW("frame %d bytes, pts %" PRId64, frm->size, frm->pts_us);
    return AML_STATUS_HOOK_CONTINUE;
}

static int ain_test(int argc, const char *argv[]) {
    struct AmlIPCAIn *ain = aml_ipc_ain_new();
    aml_obj_set_properties(AML_OBJECT(ain), "device", "default",
                           //"buffer-time", 1000*1000, "period-time", 1000*500,
                           "codec", AML_ACODEC_PCM_S16LE, NULL);
    FILE *fp = fopen("/root/dump.data", "w");
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(ain), AML_AIN_OUTPUT, fp, ain_hook);
    aml_ipc_start(AML_IPC_COMPONENT(ain));
    while (1) {
        usleep(1000 * 500);
    }
    return 0;
}

static int aout_test(int argc, const char *argv[]) {
    struct AmlIPCAIn *ain = aml_ipc_ain_new();
    aml_obj_set_properties(AML_OBJECT(ain), "device", argv[1], "codec", AML_ACODEC_PCM_S16LE,
                           "source", AML_AIN_SRC_FILE, NULL);
    struct AmlIPCAOut *aout = aml_ipc_aout_new();
    aml_obj_set_properties(AML_OBJECT(aout), "device", "default", NULL);
    aml_ipc_bind(AML_IPC_COMPONENT(ain), AML_DEFAULT_CHANNEL, AML_IPC_COMPONENT(aout),
                 AML_DEFAULT_CHANNEL);
    FILE *fp = fopen("/root/dump.data", "w");
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(ain), AML_AIN_OUTPUT, fp, ain_hook);
    aml_ipc_start(AML_IPC_COMPONENT(ain));
    while (1) {
        usleep(1000 * 500);
    }
    return 0;
}

static int enum_isp_fmt_test(int argc, const char *argv[]) {
    struct AmlIPCISP *isp = aml_ipc_isp_new(argv[1], 3, 3, 3);
    int numres = 32;
    struct AmlIPCVideoFormat fmts[numres];
    aml_ipc_isp_query_resolutions(isp, AML_ISP_FR, AML_PIXFMT_NV21, &numres, fmts);
    return 0;
}

int main(int argc, const char *argv[])
{
    aml_ipc_sdk_init();
    const char *debuglevel = getenv("AML_DEBUG");
    if (!debuglevel) debuglevel = ".*:3";
    aml_ipc_log_set_from_string(debuglevel);
    const char *tracelevel = getenv("AML_TRACE");
    if (!tracelevel) tracelevel = ".*:3";
    aml_ipc_trace_set_from_string(tracelevel);

    return osd_blend_test(argc, argv);
    return osd_line_test(argc, argv);
    return ge2d_test(argc, argv);
    return enum_isp_fmt_test(argc, argv);
    return aout_test(argc, argv);
    return ain_test(argc, argv);
    return osd_rect_test(argc, argv);
    return ge2d_perf_test(argc, argv);
    return osd_jpeg_test(argc, argv);
    return osd_png_test(argc, argv);
    return osd_text_test(argc, argv);
    return fill_rect_test(argc, argv);
    struct AmlIPCISP *isp = aml_ipc_isp_new(argv[1], 3, 0, 0);
    struct AmlIPCGE2D *ge2d1 = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    struct AmlRect rc1 = {0,0,300,200};
    struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(&rc1, AML_RGBA(255, 0, 0, 0));
    aml_ipc_ge2d_set_static_buffer(ge2d1, (struct AmlIPCFrame*)fill1);
    aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_DEFAULT_CHANNEL, AML_IPC_COMPONENT(ge2d1), AML_DEFAULT_CHANNEL);
    struct AmlIPCVideoFormat fmt[] = {{AML_PIXFMT_NV12, 1920, 1080, 30},
                                      {AML_PIXFMT_NV12, 1280, 720, 30},
                                      {AML_PIXFMT_NV12, 320, 240, 30}};
    aml_ipc_isp_set_capture_format(isp, AML_ISP_FR, &fmt[0]);
    aml_ipc_isp_set_capture_format(isp, AML_ISP_DS1, &fmt[1]);
    aml_ipc_isp_set_capture_format(isp, AML_ISP_DS2, &fmt[2]);
    struct AmlIPCGE2D *ge2d2 = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    aml_ipc_ge2d_set_static_buffer(ge2d2, (struct AmlIPCFrame*)fill1);

    struct AmlIPCVenc *enc = aml_ipc_venc_new(AML_VCODEC_H265);
    aml_ipc_bind(AML_IPC_COMPONENT(ge2d1), AML_DEFAULT_CHANNEL, AML_IPC_COMPONENT(enc), AML_DEFAULT_CHANNEL);

    aml_ipc_start(AML_IPC_COMPONENT(isp));
    int dump_num = 90;
    FILE *fp = fopen("/root/dump.h265", "w");
    while (1) {
        struct AmlIPCFrame *frame = NULL;
        AmlStatus status = aml_ipc_get_frame(AML_IPC_COMPONENT(enc), AML_DEFAULT_CHANNEL, 1000 * 1000, &frame);
        if (status == AML_STATUS_OK && frame) {
            if (dump_num > 0) {
                if (fp) {
                    fwrite(frame->data, 1, frame->size, fp);
                }
                --dump_num;
            } else {
                if (fp) {
                    fclose(fp);
                    fp = NULL;
                }
            }
            AML_LOGD("got frame pts:%" PRId64 " data:%p size:%d", frame->pts_us, frame->data,
                     frame->size);
            aml_obj_release(AML_OBJECT(frame));
        }
    }
    return 0;
}
