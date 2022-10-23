#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <nn_detect.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include "aml_ipc_sdk.h"

AML_LOG_DEFINE(example);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(example)

#define WEBSTREAM_PIPELINE_STRING                                                                  \
    "appsrc name=vin "                                                                             \
    "! h264parse "                                                                                 \
    "! video/x-h264,stream-format=avc "                                                            \
    "! amlwsssink port=%d async=false"

#define DEFAULT_WEBSTREAM_PORT 8083

#define DEFAULT_NN_RECT_COLOR AML_RGBA(255, 255, 0, 255)
#define DEFAULT_NN_RECT_BOARDER 3

struct cmd_val {
    char *device;
    int port;
};

struct ipc_app {
    GstElement *appsrc;
    volatile int ready_to_feed;
    int64_t base_pts;
    const char *name;
};

struct nn_model_info {
    det_model_type model;
    det_param_t param;
    bool initialized;
    int width;
    int height;
    int channel;
    int rowbytes;
    int stride;
    struct AmlIPCGE2D *ge2d;
    struct AmlIPCOSDFrame *vframe;
    struct {
        struct AmlIPCGE2D *blender;
        struct {
            int width;
            int height;
        } screen;
    } osd;
    int64_t last_pts;
};

static volatile bool gs_running = true;
static GMainLoop *gs_main_loop = NULL;

void signal_handle(int sig) {
    gs_running = false;
    g_main_loop_quit(gs_main_loop);
}

AmlStatus on_enc_frame(struct AmlIPCFrame *frame, void *param) {
    struct ipc_app *app = (struct ipc_app *)param;
    if (app->ready_to_feed && gs_running) {
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
        AML_LOGV("%s: %p push buffer %d bytes ret %d, pts:%" PRId64, app->name, app, frame->size,
                 ret, frame->pts_us);
        if (ret != GST_FLOW_OK) {
            AML_LOGW("%s push buffer fail %d", app->name, ret);
            app->ready_to_feed = 0;
        }
    }
    return AML_STATUS_OK;
}

static void start_feed(GstElement *pipeline, guint size, struct ipc_app *app) {
    app->ready_to_feed = 1;
}

static void stop_feed(GstElement *pipeline, struct ipc_app *app) { app->ready_to_feed = 0; }

static void bind_enc_to_appsrc(struct ipc_app *psrc) {
    if (psrc->appsrc) {
        g_signal_connect(psrc->appsrc, "need-data", G_CALLBACK(start_feed), psrc);
        g_signal_connect(psrc->appsrc, "enough-data", G_CALLBACK(stop_feed), psrc);
        psrc->base_pts = -1LL;
    }
}

static bool nn_model_open(struct nn_model_info *m) {
    if (m->initialized)
        return true;

    if (m->model == DET_BUTT)
        return false;

    if (det_set_model(m->model) != DET_STATUS_OK) {
        return false;
    }

    if (det_get_param(m->model, &m->param) != DET_STATUS_OK) {
        return false;
    }

    if (det_get_model_size(m->model, &m->width, &m->height, &m->channel) != DET_STATUS_OK) {
        det_release_model(m->model);
        return false;
    }
    m->rowbytes = m->width * m->channel;
    m->stride = (m->rowbytes + 31) & ~31;
    AML_LOGD("model set to %d", m->model);
    // init preprocesser
    m->ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_STRETCHBLT_TO);
    // preallocate stretch buffer
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGB_888, m->width, m->height, 0};
    m->vframe = aml_ipc_osd_frame_new_alloc(&fmt);
    // init osd related
    m->osd.screen.width = 1920;
    m->osd.screen.height = 1080;
    m->initialized = true;
    return true;
}

static bool nn_model_close(struct nn_model_info *m) {
    if (!m->initialized)
        return true;

    if (m->model == DET_BUTT)
        return false;

    if (det_release_model(m->model) != DET_STATUS_OK) {
        return false;
    }

    aml_obj_release(AML_OBJECT(m->ge2d));
    aml_obj_release(AML_OBJECT(m->vframe));

    m->initialized = false;
    return true;
}

AmlStatus on_nn_frame(struct AmlIPCFrame *frame, void *param) {
    struct nn_model_info *nn = (struct nn_model_info *)param;
    struct AmlIPCVideoFrame *vframe = AML_IPC_VIDEO_FRAME(frame);
    struct AmlIPCVideoFrame *nn_vframe = AML_IPC_VIDEO_FRAME(nn->vframe);
    bool ret = true;

    AML_LOGD("face detection process begin");

    if (!nn->initialized) {
        AML_LOGE("nn model not initialized");
        return AML_STATUS_OK;
    }

    if (nn->last_pts == -1LL) {
        nn->last_pts = frame->pts_us;
    }

    // check pts, do some frame drop
    int64_t diff = frame->pts_us - nn->last_pts;
    nn->last_pts = frame->pts_us;

    // expect the nn detection process time within 33+10ms
    if (diff > (33 + 10) * 1000) {
        AML_LOGD("%" PRId64 " ms expired: skip frame detection\n", diff);
        return AML_STATUS_OK;
    }

    AML_SCTIME_LOGD("on nn frame");

    DetectResult res;
    {
        AML_SCTIME_LOGD("face detection process");
        {
            struct AmlRect srect = {0, 0, vframe->format.width, vframe->format.height};
            struct AmlRect drect = {0, 0, nn->width, nn->height};
            AML_SCTIME_LOGD("detection: stretch image from %dx%d to %dx%d", srect.width,
                            srect.height, drect.width, drect.height);

            if (AML_STATUS_OK !=
                aml_ipc_ge2d_stretchblt(nn->ge2d, vframe, nn_vframe, &srect, &drect)) {
                AML_LOGE("failed to stretch the input image");
                return AML_STATUS_OK;
            }
        }

        struct AmlIPCVideoBuffer *dmabuf = nn_vframe->dmabuf;
        unsigned char *buffer = aml_ipc_dma_buffer_map(dmabuf, AML_MAP_READ | AML_MAP_WRITE);

        if (nn->rowbytes != nn->stride) {
            int rowbytes = nn->rowbytes;
            int stride = nn->stride;
            for (int i = 0; i < nn->height; i++) {
                memcpy(&buffer[rowbytes * i], &buffer[stride * i], rowbytes);
            }
        }

        input_image_t im;
        im.data = buffer;
        im.pixel_format = PIX_FMT_RGB888;
        im.width = nn->width;
        im.height = nn->height;
        im.channel = nn->channel;
        {
            AML_SCTIME_LOGD("detection: set input buffer");
            det_status_t rc = det_set_input(im, nn->model);

            if (rc != DET_STATUS_OK) {
                AML_LOGE("failed to set input to detection model");
                aml_ipc_dma_buffer_unmap(dmabuf);
                return AML_STATUS_OK;
            }
        }

        AML_LOGD("waiting for detection result");
        res.result.det_result.detect_num = 0;
        res.result.det_result.point = (det_position_float_t *)malloc(
            sizeof(det_position_float_t) * nn->param.param.det_param.detect_num);
        res.result.det_result.result_name = (det_classify_result_t *)malloc(
            sizeof(det_classify_result_t) * nn->param.param.det_param.detect_num);
        {
            AML_SCTIME_LOGD("detection: waiting for result");
            det_get_result(&res, nn->model);
        }
        AML_LOGD("detection result got, facenum: %d", res.result.det_result.detect_num);
        aml_ipc_dma_buffer_unmap(dmabuf);
    }

    {
        AML_SCTIME_LOGD("prepare osd content");
        struct AmlIPCOSDFrameList *frames = aml_ipc_osd_frame_list_new();
        if (res.result.det_result.detect_num) {
            // draw face detection result
            struct AmlIPCOSDContext *ctx = aml_ipc_osd_context_new();
            aml_ipc_osd_set_color(ctx, DEFAULT_NN_RECT_COLOR);
            aml_ipc_osd_set_bgcolor(ctx, 0);
            aml_ipc_osd_set_width(ctx, DEFAULT_NN_RECT_BOARDER);


            for (int i = 0; i < res.result.det_result.detect_num; i++) {
                int width = (int)((res.result.det_result.point[i].point.rectPoint.right -
                                   res.result.det_result.point[i].point.rectPoint.left) *
                                  nn->osd.screen.width);
                int height = (int)((res.result.det_result.point[i].point.rectPoint.bottom -
                                    res.result.det_result.point[i].point.rectPoint.top) *
                                   nn->osd.screen.height);
                int left = (int)(res.result.det_result.point[i].point.rectPoint.left *
                                 nn->osd.screen.width);
                int top = (int)(res.result.det_result.point[i].point.rectPoint.top *
                                nn->osd.screen.height);

                {
                    // draw rect
                    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGBA_8888, width, height, 0};
                    struct AmlIPCOSDFrame *frame = aml_ipc_osd_frame_new_alloc(&fmt);
                    aml_ipc_osd_frame_set_src_rect(frame, 0, 0, width, height);
                    aml_ipc_osd_frame_set_dst_rect(frame, left, top, width, height);

                    struct AmlRect rc = {0, 0, width, height};
                    aml_ipc_osd_start_paint(ctx, AML_IPC_VIDEO_FRAME(frame));
                    aml_ipc_osd_clear_rect(ctx, NULL);
                    aml_ipc_osd_draw_rect(ctx, &rc);
                    aml_ipc_osd_done_paint(ctx);
                    aml_ipc_osd_frame_list_append(frames, frame);
                }
            }
            aml_obj_release(AML_OBJECT(ctx));
        }
        aml_ipc_ge2d_set_static_buffer(nn->osd.blender, AML_IPC_FRAME(frames));
    }

    free(res.result.det_result.point);
    free(res.result.det_result.result_name);

    return ret;
}

void print_help() {
    printf("------------------------------------\n");
    printf("-d      : device\n");
    printf("-p      : webstream port, default:8083\n");
    printf("-h      : print this help message\n");
    printf("------------------------------------\n");
    exit(0);
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
    int ch;
    while ((ch = getopt(argc, argv, "hd:p:")) != -1) {
        switch (ch) {
        case 'd':
            val->device = optarg;
            break;
        case 'p':
            val->port = atoi(optarg);
            if (val->port <= 0) {
                val->port = DEFAULT_WEBSTREAM_PORT;
            }
            break;
        case 'h':
        default:
            print_help();
            break;
        }
    }
    printf("device: %s\nport: %d\n", val->device, val->port);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
    }
    struct cmd_val val = {"/dev/video0", DEFAULT_WEBSTREAM_PORT};

    if (parse_cmd(argc, argv, &val) < 0) {
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

    // init nn module
    struct nn_model_info nn = {
        .model = DET_AML_FACE_DETECTION, .initialized = false, .last_pts = -1};
    det_set_log_config(DET_DEBUG_LEVEL_ERROR, DET_LOG_TERMINAL);
    if (!nn_model_open(&nn)) {
        AML_LOGE("failed to open nn model");
        return -1;
    }

    // set the detection limits
    nn.param.param.det_param.detect_num = 10;
    det_set_param(nn.model, nn.param);

    // create pipeline
    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    // create isp
    struct AmlIPCISP *isp = aml_ipc_isp_new(val.device, 6, 4, 0);
    struct AmlIPCVideoFormat fmt[2] = {{AML_PIXFMT_NV12, 1920, 1080, 30},
                                       {AML_PIXFMT_RGB_888, 1280, 720, 30}};
    // FR as streaming source
    aml_ipc_isp_set_capture_format(isp, AML_ISP_FR, &fmt[0]);
    // DS1 as nn detection input
    aml_ipc_isp_set_capture_format(isp, AML_ISP_DS1, &fmt[1]);

    // add nn frame hook
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(isp), AML_ISP_DS1, &nn, on_nn_frame);

    // create osd blender
    struct AmlIPCGE2D *blender = aml_ipc_ge2d_new(AML_GE2D_OP_ALPHABLEND_TO);
    // set nn osd blender
    nn.osd.blender = blender;
    // create video encoder
    struct AmlIPCVenc *venc = aml_ipc_venc_new(AML_VCODEC_H264);
    // isp -> blender
    aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR, AML_IPC_COMPONENT(blender), AML_GE2D_CH0);
    // blender -> venc
    aml_ipc_bind(AML_IPC_COMPONENT(blender), AML_GE2D_OUTPUT, AML_IPC_COMPONENT(venc),
                 AML_VENC_INPUT);

    aml_ipc_pipeline_add_many(pipeline, //
                              AML_IPC_COMPONENT(isp), AML_IPC_COMPONENT(blender),
                              AML_IPC_COMPONENT(venc), NULL);

    // create webstream
    struct ipc_app app;
    gst_init(&argc, &argv);
    char *pipeline_str;
    int sz = snprintf(NULL, 0, WEBSTREAM_PIPELINE_STRING, val.port) + 1;
    pipeline_str = (char *)malloc(sz);
    snprintf(pipeline_str, sz, WEBSTREAM_PIPELINE_STRING, val.port);

    app.ready_to_feed = 0;
    app.name = "Web Stream";
    GstElement *gst_pipeline = gst_parse_launch(pipeline_str, NULL);
    app.appsrc = gst_bin_get_by_name(GST_BIN(gst_pipeline), "vin");
    free(pipeline_str);

    // add encoder frame hook
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(venc), AML_VENC_OUTPUT, &app, on_enc_frame);
    bind_enc_to_appsrc(&app);

    // startup webstream pipeline
    gst_element_set_state(gst_pipeline, GST_STATE_PLAYING);

    // startup isp pipeline
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));

    gs_main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(gs_main_loop);

    gst_element_set_state(gst_pipeline, GST_STATE_NULL);
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    aml_obj_release(AML_OBJECT(pipeline));

    nn_model_close(&nn);
    return 0;
}
