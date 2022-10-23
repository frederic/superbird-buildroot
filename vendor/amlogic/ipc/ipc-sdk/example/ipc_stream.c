#include "aml_ipc_sdk.h"
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
#include <gst/rtsp-server/rtsp-onvif-server.h>
G_GNUC_END_IGNORE_DEPRECATIONS

struct IPCSDKStreamer;

struct IPCSDKAppSrc {
    GstRTSPMediaFactory *factory;
    struct AmlIPCVenc *enc;
    int index;
    AmlStatus hookreturn;
    GstElement *appsrc;
    volatile int ready_to_feed;
    int64_t base_pts;
};

struct IPCSDKOverlay {
    struct AmlIPCOSDFrameList *list;
    struct AmlIPCOSDContext *osd;
    struct AmlIPCGE2D *ge2d;
    struct AmlIPCOSDFrame *text[2];
    int draw_idx;
};

struct IPCSDKFaceInfo {
    struct AmlIPCOSDFrameList *list;
    struct AmlIPCOSDContext *osd;
    struct AmlIPCGE2D *ge2d;
    struct AmlIPCOSDFrame *text[2];
    int draw_idx;
    const char *name;
    struct AmlRect rect;
};

struct IPCSDKStreamer {
    GstElement *pipeline;
    GstRTSPServer *server;

    struct AmlIPCISP *isp;
    struct AmlIPCGE2D *ge2d_cover;
    struct AmlIPCGE2D *ge2d_scale;
    struct AmlIPCGE2D *ge2d_blend;
    struct AmlIPCVBPool *vbp;
    struct AmlIPCOSDContext *osd;
    struct AmlIPCQueue *queue1;
    struct AmlIPCQueue *queue2;
};

static int bexit = 0;

static GMainLoop *main_loop;
void signal_handle(int sig) {
    bexit = 1;
    g_main_loop_quit(main_loop);
}

static void start_feed(GstElement *pipeline, guint size, struct IPCSDKAppSrc *app) {
    app->ready_to_feed = 1;
    AML_LOGV("%d: ready_to_feed %d", app->index, app->ready_to_feed);
}

static void stop_feed(GstElement *pipeline, struct IPCSDKAppSrc *app) {
    app->ready_to_feed = 0;
    AML_LOGV("%d: ready_to_feed %d", app->index, app->ready_to_feed);
}

AmlStatus on_venc_frame(struct AmlIPCFrame *frame, void *param) {
    struct IPCSDKAppSrc *app = (struct IPCSDKAppSrc *)param;
    if (app->ready_to_feed) {
        GstBuffer *buf = gst_buffer_new_allocate(NULL, frame->size, NULL);
        gst_buffer_fill(buf, 0, frame->data, frame->size);
        if (app->base_pts == -1LL) {
            app->base_pts = frame->pts_us * 1000LL;
        }
        GST_BUFFER_PTS(buf) = frame->pts_us * 1000LL - app->base_pts;
        GstFlowReturn ret;
        g_signal_emit_by_name(app->appsrc, "push-buffer", buf, &ret);
        gst_buffer_unref(buf);
        AML_LOGV("%d: %p push buffer %d bytes ret %d, pts:%" PRId64, app->index, app, frame->size,
                 ret, frame->pts_us);
        if (ret != GST_FLOW_OK) {
            AML_LOGW("push buffer fail %d", ret);
            app->ready_to_feed = 0;
        }
    }
    // return AML_STATUS_HOOK_CONTINUE, pipeline will continue to process the frame
    return app->hookreturn;
}

AmlStatus draw_frame_pts(struct AmlIPCFrame *frame, void *param) {
    struct IPCSDKOverlay *poverlay = (struct IPCSDKOverlay *)param;
    char ts[128];
    int64_t pts = frame->pts_us;
    sprintf(ts, "%" PRId64 ".%06" PRId64, pts / 1000000, pts % 1000000);
    struct AmlIPCOSDFrame *text = poverlay->text[poverlay->draw_idx];
    aml_ipc_osd_start_paint(poverlay->osd, AML_IPC_VIDEO_FRAME(text));
    aml_ipc_osd_clear_rect(poverlay->osd, NULL);
    // aml_ipc_ge2d_fill_rect(poverlay->ge2d, AML_IPC_VIDEO_FRAME(text), NULL,
    // poverlay->osd->bgcolor);
    int x = 0, y = 0, x2 = x, y2 = y;
    aml_ipc_osd_draw_text(poverlay->osd, ts, &x2, &y2);
    aml_ipc_osd_done_paint(poverlay->osd);
    // on right side of 1920x1080 surface
    aml_ipc_osd_frame_set_src_rect(text, 0, 0, x2, y2);
    aml_ipc_osd_frame_set_dst_rect(text, 1920 - x2 - 10, 10, x2, y2);
    poverlay->draw_idx = (poverlay->draw_idx + 1) % 2;
    aml_ipc_ge2d_static_buffer_list_replace(poverlay->ge2d, poverlay->text[poverlay->draw_idx],
                                            text);
    return AML_STATUS_HOOK_CONTINUE;
}

gboolean update_timestamp(struct IPCSDKOverlay *poverlay) {
    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    char ts[128];
    strftime(ts, sizeof(ts), "%T", tmp);
    struct AmlIPCOSDFrame *text = poverlay->text[poverlay->draw_idx];
    aml_ipc_osd_start_paint(poverlay->osd, AML_IPC_VIDEO_FRAME(text));
    aml_ipc_osd_clear_rect(poverlay->osd, NULL); // slow
    // aml_ipc_ge2d_fill_rect(poverlay->ge2d, AML_IPC_VIDEO_FRAME(text), NULL,
    // poverlay->osd->bgcolor);
    int x = 0, y = 0, x2 = x, y2 = y;
    aml_ipc_osd_draw_text(poverlay->osd, ts, &x2, &y2);
    aml_ipc_osd_done_paint(poverlay->osd);
    // on left side of 1920x1080 surface
    aml_ipc_osd_frame_set_src_rect(text, 0, 0, x2, y2);
    aml_ipc_osd_frame_set_dst_rect(text, 10, 10, x2, y2);
    poverlay->draw_idx = (poverlay->draw_idx + 1) % 2;
    aml_ipc_ge2d_static_buffer_list_replace(poverlay->ge2d, poverlay->text[poverlay->draw_idx],
                                            text);
    return TRUE;
}

gboolean draw_face_info(struct IPCSDKFaceInfo *poverlay) {
    struct AmlIPCOSDFrame *text = poverlay->text[poverlay->draw_idx];
    aml_ipc_osd_start_paint(poverlay->osd, AML_IPC_VIDEO_FRAME(text));
    aml_ipc_osd_clear_rect(poverlay->osd, NULL); // slow
    // aml_ipc_ge2d_fill_rect(poverlay->ge2d, AML_IPC_VIDEO_FRAME(text), NULL,
    // poverlay->osd->bgcolor);
    int x = 0, y = 0, x2 = x, y2 = y;
    int rcWidth = (rand() % 200) + 100;
    int rcHeight = (rand() % 200) + 100;
    aml_ipc_osd_set_width(poverlay->osd, 2);
    aml_ipc_osd_set_color(poverlay->osd, AML_RGBA(255, 255, 0, 255));
    aml_ipc_osd_draw_text(poverlay->osd, poverlay->name, &x2, &y2);
    aml_ipc_osd_set_color(poverlay->osd, AML_RGBA(0, 255, 0, 255));
    aml_ipc_osd_draw_rect(poverlay->osd, &(struct AmlRect){0, y2, rcWidth, rcHeight});
    aml_ipc_osd_done_paint(poverlay->osd);
    // on right side of 1920x1080 surface
    rcWidth += 4;
    rcHeight += y2 + 4;
    aml_ipc_osd_frame_set_src_rect(text, 0, 0, rcWidth, rcHeight + y2);
    aml_ipc_osd_frame_set_dst_rect(text, rand() % (1920 - rcWidth), rand() % (1080 - rcHeight),
                                   rcWidth, rcHeight);
    poverlay->draw_idx = (poverlay->draw_idx + 1) % 2;
    aml_ipc_ge2d_static_buffer_list_replace(poverlay->ge2d, poverlay->text[poverlay->draw_idx],
                                            text);
    int timeout = (rand() % 3000) + 1000;
    g_timeout_add(timeout, (GSourceFunc)draw_face_info, poverlay);
    return FALSE;
}

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media,
                            struct IPCSDKAppSrc *app) {
    AML_LOGW("%d: %p started...", app->index, app);
    GstElement *rtpbin = gst_rtsp_media_get_element(media);
    app->appsrc = gst_bin_get_by_name(GST_BIN(rtpbin), "vin");
    g_signal_connect(app->appsrc, "need-data", G_CALLBACK(start_feed), app);
    g_signal_connect(app->appsrc, "enough-data", G_CALLBACK(stop_feed), app);
    app->base_pts = -1LL;
}

static void bind_enc_to_webstream(struct IPCSDKAppSrc *psrc, GstElement *appsrc) {
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(psrc->enc), AML_DEFAULT_CHANNEL, psrc, on_venc_frame);
    psrc->appsrc = appsrc;
    g_signal_connect(psrc->appsrc, "need-data", G_CALLBACK(start_feed), psrc);
    g_signal_connect(psrc->appsrc, "enough-data", G_CALLBACK(stop_feed), psrc);
    psrc->base_pts = -1LL;
}

static void bind_enc_to_rtsp(struct IPCSDKAppSrc *psrc, enum AmlIPCVideoCodec codec,
                             GstRTSPMountPoints *mounts, const char *mount_point,
                             const char *pipeline_str) {
    psrc->enc = aml_ipc_venc_new(codec);
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(psrc->enc), AML_DEFAULT_CHANNEL, psrc, on_venc_frame);
    psrc->factory = gst_rtsp_onvif_media_factory_new();
    g_signal_connect(psrc->factory, "media-configure", G_CALLBACK(media_configure), psrc);
    gst_rtsp_media_factory_set_media_gtype(psrc->factory, GST_TYPE_RTSP_ONVIF_MEDIA);
    gst_rtsp_media_factory_set_latency(psrc->factory, 100);
    gst_rtsp_media_factory_set_publish_clock_mode(psrc->factory,
                                                  GST_RTSP_PUBLISH_CLOCK_MODE_CLOCK_AND_OFFSET);
    gst_rtsp_media_factory_set_launch(psrc->factory, pipeline_str);
    gst_rtsp_media_factory_set_shared(psrc->factory, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, mount_point, psrc->factory);
}

int main(int argc, char *argv[]) {
    aml_ipc_sdk_init();
    struct IPCSDKStreamer streamer;
    bzero(&streamer, sizeof(streamer));
    const char *debuglevel = getenv("AML_DEBUG");
    if (!debuglevel)
        debuglevel = ".*:3";
    aml_ipc_log_set_from_string(debuglevel);
    const char *tracelevel = getenv("AML_TRACE");
    if (tracelevel) {
        aml_ipc_trace_set_from_string(tracelevel);
        const char *tracefile = getenv("AML_TRACE_FILE");
        if (tracefile) {
            FILE *ftrace = NULL;
            if (strcmp(tracefile, "stderr") == 0)
                ftrace = stderr;
            else if (strcmp(tracefile, "stdout") == 0)
                ftrace = stdout;
            else
                ftrace = fopen(tracefile, "wb");
            aml_ipc_trace_set_json_output(ftrace);
        }
    }

    signal(SIGINT, signal_handle);

    if (argc == 1) {
        printf("USAGE: %s dev\n      i.e. %s /dev/video0\n", argv[0], argv[0]);
        exit(1);
    }
    // configure ISP, enable FR and DS1
    streamer.isp = aml_ipc_isp_new(argv[1], 6, 4, 0);
    struct AmlIPCVideoFormat fmt[] = {{AML_PIXFMT_NV12, 1920, 1080, 30},
                                      {AML_PIXFMT_NV12, 1280, 720, 30},
                                      {AML_PIXFMT_NV12, 704, 576, 30}};
    aml_ipc_isp_set_capture_format(streamer.isp, AML_ISP_FR, &fmt[0]);
    aml_ipc_isp_set_capture_format(streamer.isp, AML_ISP_DS1, &fmt[1]);

    streamer.queue1 = aml_ipc_queue_new(3);
    streamer.queue2 = aml_ipc_queue_new(3);

    streamer.ge2d_cover = aml_ipc_ge2d_new(AML_GE2D_OP_FILLRECT);
    // create buffer pool for scale
    streamer.vbp = aml_ipc_vbpool_new(3, &fmt[2]);
    streamer.ge2d_scale = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_FROM);
    streamer.ge2d_blend = aml_ipc_ge2d_new(AML_GE2D_OP_ALPHABLEND_TO);

    struct AmlIPCOSDContext *osd = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(osd, "/usr/share/directfb-1.7.7/decker.ttf", 100);
    aml_ipc_osd_set_bgcolor(osd, AML_RGBA(128, 128, 128, 64));
    aml_ipc_osd_set_color(osd, AML_RGBA(255, 0, 0, 255));

    // cover the rect 100,200,300x200
    struct AmlIPCOSDFrame *fill1 = aml_ipc_osd_frame_new_fill_rect(NULL, AML_RGBA(0, 0, 128, 255));
    aml_ipc_osd_frame_set_dst_rect(fill1, 100, 200, 300, 200);
    aml_ipc_ge2d_set_static_buffer(streamer.ge2d_cover, (struct AmlIPCFrame *)fill1);

    struct AmlIPCOSDFrameList *list = aml_ipc_osd_frame_list_new();
    aml_ipc_ge2d_set_static_buffer(streamer.ge2d_blend, (struct AmlIPCFrame *)list);
    // create double buffer to draw overlay (maximium 10 chars)
    int maxw, maxh;
    aml_ipc_osd_get_font_bbox(osd, &maxw, &maxh);
    struct AmlIPCVideoFormat textfmt = {AML_PIXFMT_RGBA_8888, maxw * 10, maxh, 0};
    struct IPCSDKOverlay time_overlay = {
        list,
        osd,
        streamer.ge2d_blend,
        {aml_ipc_osd_frame_new_alloc(&textfmt), aml_ipc_osd_frame_new_alloc(&textfmt)},
        0};
    aml_ipc_osd_frame_list_append(list, time_overlay.text[1]);
    update_timestamp(&time_overlay);

    osd = aml_ipc_osd_context_new();
    aml_ipc_osd_load_font(osd, "/usr/share/directfb-1.7.7/decker.ttf", 60);
    aml_ipc_osd_set_bgcolor(osd, AML_RGBA(0, 128, 128, 64));
    aml_ipc_osd_set_color(osd, AML_RGBA(0, 0, 255, 128));
    aml_ipc_osd_get_font_bbox(osd, &maxw, &maxh);
    textfmt.width = maxw * 10;
    textfmt.height = maxh;
    struct IPCSDKOverlay pts_overlay = {
        list,
        osd,
        streamer.ge2d_blend,
        {aml_ipc_osd_frame_new_alloc(&textfmt), aml_ipc_osd_frame_new_alloc(&textfmt)},
        0};
    aml_ipc_osd_frame_list_append(list, pts_overlay.text[1]);

    gst_init(&argc, &argv);
    streamer.pipeline =
        gst_parse_launch("appsrc name=vin1 ! h264parse ! video/x-h264,stream-format=avc ! "
                         "amlwsssink port=8082 async=false "
                         "appsrc name=vin2 ! h264parse ! video/x-h264, stream-format=avc ! "
                         "amlwsssink port=8083 async=false",
                         NULL);

    GstRTSPServer *server = streamer.server = gst_rtsp_onvif_server_new();
    gst_rtsp_server_set_address(server, "0.0.0.0");
    gst_rtsp_server_set_service(server, "554");
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    struct IPCSDKAppSrc src[5];
    for (int i = 0; i < 5; ++i) {
        src[i].index = i;
        src[i].hookreturn = AML_STATUS_OK;
        src[i].ready_to_feed = 0;
    }

    // FR scale -> src[0]
    bind_enc_to_rtsp(&src[0], AML_VCODEC_H264, mounts, "/test1.sdp",
                     "appsrc name=vin ! h264parse config-interval=1 ! rtph264pay pt=96 "
                     "config-interval=1 name=pay0");
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.isp), AML_DEFAULT_CHANNEL,
                 AML_IPC_COMPONENT(streamer.ge2d_scale), AML_DEFAULT_CHANNEL);
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.vbp), AML_DEFAULT_CHANNEL,
                 AML_IPC_COMPONENT(streamer.ge2d_scale), AML_GE2D_CH1);
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.ge2d_scale), AML_DEFAULT_CHANNEL,
                 AML_IPC_COMPONENT(src[0].enc), AML_DEFAULT_CHANNEL);

    // FR + cover + overlay -> src[1]
    bind_enc_to_rtsp(&src[1], AML_VCODEC_H265, mounts, "/test2.sdp",
                     "appsrc name=vin is-live=true ! h265parse config-interval=1 ! rtph265pay "
                     "pt=96 config-interval=1 name=pay0");
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.isp), AML_ISP_FR, AML_IPC_COMPONENT(streamer.queue2),
                 AML_DEFAULT_CHANNEL);
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.queue2), AML_DEFAULT_CHANNEL,
                 AML_IPC_COMPONENT(streamer.ge2d_cover), AML_DEFAULT_CHANNEL);
    // aml_ipc_bind(AML_IPC_COMPONENT(streamer.isp), AML_ISP_FR,
    // AML_IPC_COMPONENT(streamer.ge2d_cover), AML_DEFAULT_CHANNEL);
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.ge2d_cover), AML_DEFAULT_CHANNEL,
                 AML_IPC_COMPONENT(streamer.ge2d_blend), AML_DEFAULT_CHANNEL);
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.ge2d_blend), AML_DEFAULT_CHANNEL,
                 AML_IPC_COMPONENT(src[1].enc), AML_DEFAULT_CHANNEL);
    // hook the frame before ge2d_blend handle it, we'll draw pts on the frame
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(streamer.ge2d_blend), AML_DEFAULT_CHANNEL,
                           &pts_overlay, draw_frame_pts);

    // DS1 -> src[2]
    bind_enc_to_rtsp(&src[2], AML_VCODEC_H264, mounts, "/test3.sdp",
                     "appsrc name=vin ! h264parse config-interval=1 ! rtph264pay pt=96 "
                     "config-interval=1 name=pay0");
    aml_ipc_bind(AML_IPC_COMPONENT(streamer.isp), AML_ISP_DS1, AML_IPC_COMPONENT(src[2].enc),
                 AML_DEFAULT_CHANNEL);

    // FR scale -> vin1 -> webstream
    // src[3] continue to hook the frame after src[0], so we make src[0] hook return CONTINUE
    src[0].hookreturn = AML_STATUS_HOOK_CONTINUE;
    src[3].enc = src[0].enc;
    bind_enc_to_webstream(&src[3], gst_bin_get_by_name(GST_BIN(streamer.pipeline), "vin1"));

    // DS1 -> vin2 -> webstream
    // src[4] continue to hook the frame after src[2], so we make src[2] hook return CONTINUE
    src[2].hookreturn = AML_STATUS_HOOK_CONTINUE;
    src[4].enc = src[2].enc;
    bind_enc_to_webstream(&src[4], gst_bin_get_by_name(GST_BIN(streamer.pipeline), "vin2"));

    g_object_unref(mounts);

    aml_ipc_start(AML_IPC_COMPONENT(streamer.queue1));
    aml_ipc_start(AML_IPC_COMPONENT(streamer.queue2));
    aml_ipc_start(AML_IPC_COMPONENT(streamer.isp));
    gst_element_set_state(streamer.pipeline, GST_STATE_PLAYING);
    gst_rtsp_server_attach(server, NULL);
    main_loop = g_main_loop_new(NULL, FALSE);
    g_timeout_add_seconds(1, (GSourceFunc)update_timestamp, &time_overlay);

    osd = aml_ipc_osd_context_new();
    int w, h;
    const char *pngfile = "/var/www/ipc-webui/logo-amlogic.png";
    aml_ipc_osd_preload_png(osd, pngfile, &w, &h);
    struct AmlIPCOSDFrame *png_overlay =
        aml_ipc_osd_frame_new_alloc(&(struct AmlIPCVideoFormat){AML_PIXFMT_RGBA_8888, w, h, 0});
    aml_ipc_osd_start_paint(osd, AML_IPC_VIDEO_FRAME(png_overlay));
    aml_ipc_osd_load_png(osd, pngfile, 0, 0);
    aml_ipc_osd_done_paint(osd);
    aml_ipc_osd_frame_set_src_rect(png_overlay, 0, 0, w, h);
    aml_ipc_osd_frame_set_dst_rect(png_overlay, 500, 500, w, h);
    aml_ipc_osd_frame_list_append(list, png_overlay);

    aml_ipc_osd_load_font(osd, "/usr/share/directfb-1.7.7/decker.ttf", 30);
    aml_ipc_osd_set_bgcolor(osd, AML_RGBA(255, 255, 255, 0));
    aml_ipc_osd_set_color(osd, AML_RGBA(0, 255, 0, 255));
    aml_ipc_osd_get_font_bbox(osd, &maxw, &maxh);
    textfmt.width = 320;
    textfmt.height = 400;
    struct IPCSDKFaceInfo face_overlay[4] = {
        {list,
         osd,
         streamer.ge2d_blend,
         {aml_ipc_osd_frame_new_alloc(&textfmt), aml_ipc_osd_frame_new_alloc(&textfmt)},
         0,
         "Richard",
         {0, 0, 300, 200}},
        {list,
         osd,
         streamer.ge2d_blend,
         {aml_ipc_osd_frame_new_alloc(&textfmt), aml_ipc_osd_frame_new_alloc(&textfmt)},
         0,
         "Robert",
         {0, 0, 300, 200}},
        {list,
         osd,
         streamer.ge2d_blend,
         {aml_ipc_osd_frame_new_alloc(&textfmt), aml_ipc_osd_frame_new_alloc(&textfmt)},
         0,
         "Charles",
         {0, 0, 300, 200}},
        {list,
         osd,
         streamer.ge2d_blend,
         {aml_ipc_osd_frame_new_alloc(&textfmt), aml_ipc_osd_frame_new_alloc(&textfmt)},
         0,
         "Edward",
         {0, 0, 300, 200}}};
    srand(time(0));
    for (int i = 0; i < 4; ++i) {
        aml_ipc_osd_frame_list_append(list, face_overlay[i].text[1]);
        draw_face_info(&face_overlay[i]);
        int timeout = (rand() % 1000) + 300;
        g_timeout_add(timeout, (GSourceFunc)draw_face_info, &face_overlay[i]);
    }
    g_main_loop_run(main_loop);

    aml_ipc_stop(AML_IPC_COMPONENT(src[0].enc));
    aml_ipc_stop(AML_IPC_COMPONENT(src[1].enc));
    aml_ipc_stop(AML_IPC_COMPONENT(src[2].enc));
    aml_ipc_stop(AML_IPC_COMPONENT(streamer.vbp));
    aml_ipc_stop(AML_IPC_COMPONENT(streamer.queue1));
    aml_ipc_stop(AML_IPC_COMPONENT(streamer.queue2));
    aml_ipc_stop(AML_IPC_COMPONENT(streamer.isp));

    aml_obj_release(AML_OBJECT(time_overlay.osd));
    aml_obj_release(AML_OBJECT(pts_overlay.osd));
    aml_obj_release(AML_OBJECT(osd));
    aml_obj_release(AML_OBJECT(time_overlay.text[time_overlay.draw_idx]));
    aml_obj_release(AML_OBJECT(pts_overlay.text[pts_overlay.draw_idx]));
    aml_obj_release(AML_OBJECT(face_overlay[0].text[face_overlay[0].draw_idx]));
    aml_obj_release(AML_OBJECT(face_overlay[1].text[face_overlay[1].draw_idx]));
    aml_obj_release(AML_OBJECT(face_overlay[2].text[face_overlay[2].draw_idx]));
    aml_obj_release(AML_OBJECT(face_overlay[3].text[face_overlay[3].draw_idx]));
    aml_obj_release(AML_OBJECT(src[0].enc));
    aml_obj_release(AML_OBJECT(src[1].enc));
    aml_obj_release(AML_OBJECT(src[2].enc));
    aml_obj_release(AML_OBJECT(streamer.vbp));
    aml_obj_release(AML_OBJECT(streamer.queue1));
    aml_obj_release(AML_OBJECT(streamer.queue2));
    aml_obj_release(AML_OBJECT(streamer.isp));
    aml_obj_release(AML_OBJECT(streamer.ge2d_cover));
    aml_obj_release(AML_OBJECT(streamer.ge2d_scale));
    aml_obj_release(AML_OBJECT(streamer.ge2d_blend));

    return 0;
}
