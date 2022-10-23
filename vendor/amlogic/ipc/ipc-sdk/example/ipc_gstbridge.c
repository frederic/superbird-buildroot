#include "aml_ipc_internal.h"
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
#include <gst/rtsp-server/rtsp-onvif-server.h>
G_GNUC_END_IGNORE_DEPRECATIONS
#include <sys/prctl.h>

AML_LOG_DEFINE(gst);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(gst)

struct IPCRtspAppSrc {
    LIST_ENTRY(IPCRtspAppSrc) next;
    GstElement *appsrc;
    GstRTSPMedia *media;
    int64_t base_pts;
    volatile int ready_to_feed;
};

struct IPCAppSrc {
    struct IPCAppSrc *next;
    LIST_HEAD(IPCRtspAppSrcList,IPCRtspAppSrc) rtsp_session;
    GstElement *appsrc;
    gchar *src_name;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int64_t base_pts;
    volatile int ready_to_feed;
    int chan_id;
};

struct IPCAppSink {
    struct IPCAppSink *next;
    GstElement *appsink;
    int chan_id;
};


struct AmlIPCGstBridge {
    AML_OBJ_EXTENDS(AmlIPCGstBridge, AmlIPCComponent, AmlIPCComponentClass);
    char *gst_pipeline; // gst launch pipeline
    char *rtsp_bind_addr;
    char *rtsp_service;
    char *rtsp_mount;
    char *rtsp_server_name;
    struct AmlIPCGstBridge *next;

    GstElement *pipeline;
    struct IPCAppSrc *list_appsrc;
    struct IPCAppSink *list_appsink;
    struct IPCAppSrc **array_appsrc;
    struct IPCAppSink **array_appsink;
    int num_channel;
    int num_appsrc;
    int num_appsink;

    GstRTSPServer *rtsp_server;
    GstRTSPMediaFactory *factory;
    pthread_t thread_id;
    GMainLoop *main_loop;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCGstBridge, AmlIPCComponent, AmlIPCComponentClass);

AML_OBJ_PROP_STR(PROP_PIPELINE, "launch", "gst launch string", NULL);
AML_OBJ_PROP_STR(PROP_RTSP_BIND_ADDR, "rtsp-bind", "rtsp bind address", "0.0.0.0");
AML_OBJ_PROP_STR(PROP_RTSP_SERVICE, "rtsp-port", "rtsp bind port", NULL);
AML_OBJ_PROP_STR(PROP_RTSP_MOUNT, "rtsp-mount", "rtsp mount point", NULL);
AML_OBJ_PROP_STR(PROP_RTSP_SERVER, "rtsp-server", "bind to rtsp server", NULL);

static struct AmlIPCGstBridge *all_gst_bridges = NULL;

static void start_feed(GstElement *pipeline, guint size, struct IPCAppSrc *src) {
    {
        AML_LOCK_GUARD(lock, &src->lock);
        src->ready_to_feed = 1;
        pthread_cond_signal(&src->cond);
    }
}

static void stop_feed(GstElement *pipeline, struct IPCAppSrc *src) {
    {
        AML_LOCK_GUARD(lock, &src->lock);
        src->ready_to_feed = 0;
        pthread_cond_signal(&src->cond);
    }
}

static void rtsp_start_feed(GstElement *pipeline, guint size, struct IPCRtspAppSrc *src) {
    src->ready_to_feed = 1;
}

static void rtsp_stop_feed(GstElement *pipeline, struct IPCRtspAppSrc *src) {
    src->ready_to_feed = 0;
}

static void *gstbr_thread(void *param) {
    struct AmlIPCGstBridge *br = (struct AmlIPCGstBridge *)param;
    const char *objname = AML_OBJECT_NAME(br);
    if (strncasecmp(objname, "AmlIPC", 6) == 0) {
        objname += 6;
    }
    prctl(PR_SET_NAME, objname, NULL, NULL, NULL);
    g_main_loop_run(br->main_loop);
    return NULL;
}

static void media_unprepared(GstRTSPMedia *media, struct AmlIPCGstBridge *br) {
    AML_LOGW("media %p", media);
    for (struct IPCAppSrc *p = br->list_appsrc; p; p = p->next) {
        AML_LOCK_GUARD(lock, &p->lock);
        struct IPCRtspAppSrc *rtspsrc;
        LIST_FOREACH(rtspsrc, &p->rtsp_session, next) {
            if (rtspsrc->media == media) {
                LIST_REMOVE(rtspsrc, next);
                free(rtspsrc);
                break;
            }
        }
    }
}

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media,
                            struct AmlIPCGstBridge *br) {
    AML_LOGW("media %p", media);
    g_signal_connect(media, "unprepared", G_CALLBACK(media_unprepared), br);
    GstElement *rtpbin = gst_rtsp_media_get_element(media);
    for (struct IPCAppSrc *p = br->list_appsrc; p; p = p->next) {
        struct IPCRtspAppSrc *src = calloc(1, sizeof(*src));
        src->appsrc = gst_bin_get_by_name(GST_BIN(rtpbin), p->src_name);
        src->media = media;
        src->base_pts = -1LL;
        g_signal_connect(src->appsrc, "need-data", G_CALLBACK(rtsp_start_feed), src);
        g_signal_connect(src->appsrc, "enough-data", G_CALLBACK(rtsp_stop_feed), src);
        AML_LOCK_GUARD(lock, &p->lock);
        LIST_INSERT_HEAD(&p->rtsp_session, src, next);
    }
}

static AmlStatus gstbr_start(struct AmlIPCGstBridge *br) {
    if (br->main_loop) {
        AML_LOGI("already started");
        return AML_STATUS_OK;
    }
    if (br->pipeline == NULL) {
        AML_LOGW("no pipeline created");
        return AML_STATUS_FAIL;
    }
    if (br->rtsp_service || (br->rtsp_mount == NULL)) {
        GMainContext *context = g_main_context_new();
        br->main_loop = g_main_loop_new(context, false);
        if (br->rtsp_mount == NULL) {
            GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(br->pipeline));
            GSource *source = gst_bus_create_watch(bus);
            // g_source_set_callback(source, (GSourceFunc) BusHandler, data, NULL);
            g_source_attach(source, context);
            g_source_unref(source);
            gst_object_unref(bus);
        }
        pthread_create(&br->thread_id, NULL, &gstbr_thread, br);
        if (br->rtsp_service) {
            GstRTSPServer *server = br->rtsp_server = gst_rtsp_onvif_server_new();
            gst_rtsp_server_set_address(server, br->rtsp_bind_addr);
            gst_rtsp_server_set_service(server, br->rtsp_service);
            gst_rtsp_server_attach(server, context);
        }
    }
    if (br->rtsp_mount) {
        GstRTSPServer *server = br->rtsp_server;
        if (server == NULL) {
            for (struct AmlIPCGstBridge *p = all_gst_bridges; p; p = p->next) {
                if (p->rtsp_service == NULL)
                    continue;
                if ((br->rtsp_server_name == NULL) ||
                    (strcmp(AML_OBJECT(p)->name, br->rtsp_server_name) == 0)) {
                    if (p->rtsp_server == NULL) {
                        AML_LOGI("will start rtsp server first");
                        aml_ipc_start(AML_IPC_COMPONENT(p));
                    }
                    server = br->rtsp_server = p->rtsp_server;
                    break;
                }
            }
            if (server == NULL) {
                AML_LOGW("need to create rtsp server first");
                return AML_STATUS_FAIL;
            }
        }
        GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
        GstRTSPMediaFactory *factory = br->factory = gst_rtsp_onvif_media_factory_new();
        g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), br);
        gst_rtsp_media_factory_set_media_gtype(factory, GST_TYPE_RTSP_ONVIF_MEDIA);
        gst_rtsp_media_factory_set_latency(factory, 100);
        gst_rtsp_media_factory_set_publish_clock_mode(factory,
                                                      GST_RTSP_PUBLISH_CLOCK_MODE_CLOCK_AND_OFFSET);
        gst_rtsp_media_factory_set_launch(factory, br->gst_pipeline);
        gst_rtsp_media_factory_set_shared(factory, TRUE);
        gst_rtsp_mount_points_add_factory(mounts, br->rtsp_mount, factory);
        g_object_unref(mounts);
    } else {
        gst_element_set_state(br->pipeline, GST_STATE_PLAYING);
    }
    return AML_STATUS_OK;
}

static AmlStatus gstbr_stop(struct AmlIPCGstBridge *br) {
    aml_ipc_comp_flush(AML_IPC_COMPONENT(br), false);
    if (br->rtsp_mount == NULL) {
        for (int i = 0; i < br->num_appsrc; ++i) {
            struct IPCAppSrc *src = br->array_appsrc[i];
            AML_LOCK_GUARD(lock, &src->lock);
            src->ready_to_feed = 1;
            pthread_cond_signal(&src->cond);
        }
    }
    if (br->main_loop == NULL) {
        AML_LOGI("not started");
        return AML_STATUS_FAIL;
    }
    if (br->pipeline == NULL) {
        AML_LOGW("no pipeline created");
        return AML_STATUS_FAIL;
    }
    gst_element_set_state(br->pipeline, GST_STATE_NULL);
    g_main_loop_quit(br->main_loop);
    pthread_join(br->thread_id, NULL);
    return AML_STATUS_OK;
}

static AmlStatus gstbr_handle_frame(struct AmlIPCGstBridge *br, int ch, struct AmlIPCFrame *frame) {
    if (ch < 0 || ch >= br->num_appsrc) {
        AML_LOGW("invalid channel id %d, num_appsrc %d", ch, br->num_appsrc);
        aml_obj_release(AML_OBJECT(frame));
        return AML_STATUS_FAIL;
    }
    struct IPCAppSrc *src = br->array_appsrc[ch];
    if (src->appsrc == NULL) {
        AML_LOGW("there's no appsrc element");
        aml_obj_release(AML_OBJECT(frame));
        return AML_STATUS_FAIL;
    } else if (br->rtsp_mount) {
        GstBuffer *buf = gst_buffer_new_allocate(NULL, frame->size, NULL);
        gst_buffer_fill(buf, 0, frame->data, frame->size);
        struct IPCRtspAppSrc *rtspsrc;
        {
            AML_LOCK_GUARD(lock, &src->lock);
            LIST_FOREACH(rtspsrc, &src->rtsp_session, next) {
                if (rtspsrc->ready_to_feed) {
                    GstFlowReturn ret;
                    if (rtspsrc->base_pts == -1LL) {
                        rtspsrc->base_pts = frame->pts_us * 1000LL;
                    }
                    GST_BUFFER_PTS(buf) = frame->pts_us * 1000LL - rtspsrc->base_pts;
                    g_signal_emit_by_name(rtspsrc->appsrc, "push-buffer", buf, &ret);
                    AML_LOGV("push buffer %d bytes ret %d, pts:%" PRId64, frame->size, ret, frame->pts_us);
                }
            }
        }
        gst_buffer_unref(buf);
        aml_obj_release(AML_OBJECT(frame));
        return AML_STATUS_OK;
    } else {
        AML_LOCK_GUARD(lock, &src->lock);
        while (!src->ready_to_feed) {
            pthread_cond_wait(&src->cond, lock);
        }
    }
    if (AML_IPC_COMPONENT(br)->flags & AML_IPC_COMP_FLAG_FLUSH) {
        aml_obj_release(AML_OBJECT(frame));
        return AML_STATUS_FLUSHING;
    }
    GstBuffer *buf = gst_buffer_new_allocate(NULL, frame->size, NULL);
    gst_buffer_fill(buf, 0, frame->data, frame->size);
    if (src->base_pts == -1LL) {
        src->base_pts = frame->pts_us * 1000LL;
    }
    GST_BUFFER_PTS(buf) = frame->pts_us * 1000LL - src->base_pts;
    GstFlowReturn ret;
    g_signal_emit_by_name(src->appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);
    AML_LOGV("push buffer %d bytes ret %d, pts:%" PRId64, frame->size, ret, frame->pts_us);
    if (ret != GST_FLOW_OK) {
        AML_LOGW("push buffer fail %d", ret);
        AML_LOCK_GUARD(lock, &src->lock);
        src->ready_to_feed = 0;
    }
    return AML_STATUS_OK;
}

static AmlStatus gstbr_setup_appsrc(struct AmlIPCGstBridge *br, GstElement *appsrc) {
    gchar *name = NULL;
    g_object_get(G_OBJECT(appsrc), "name", &name, NULL);
    if (name == NULL) {
        AML_LOGW("can't bridge an appsrc without a name");
        return AML_STATUS_FAIL;
    }
    struct IPCAppSrc *ipcsrc = calloc(1, sizeof(*ipcsrc));
    LIST_INIT(&ipcsrc->rtsp_session);
    ipcsrc->src_name = name;
    ipcsrc->next = br->list_appsrc;
    br->list_appsrc = ipcsrc;
    ipcsrc->appsrc = appsrc;
    ipcsrc->chan_id = br->num_channel++;
    br->num_appsrc++;
    aml_ipc_add_channel(AML_IPC_COMPONENT(br), ipcsrc->chan_id, AML_CHANNEL_INPUT, name);
    if (br->rtsp_mount == NULL) {
        g_signal_connect(ipcsrc->appsrc, "need-data", G_CALLBACK(start_feed), ipcsrc);
        g_signal_connect(ipcsrc->appsrc, "enough-data", G_CALLBACK(stop_feed), ipcsrc);
    }
    AML_LOGV("add appsrc %d %s", ipcsrc->chan_id, name);
    // use CLOCK_MONOTONIC for pthread_cond_timedwait
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    pthread_cond_init(&ipcsrc->cond, &cond_attr);
    pthread_mutex_init(&ipcsrc->lock, NULL);
    ipcsrc->base_pts = -1LL;
    return AML_STATUS_OK;
}

static AmlStatus gstbr_setup_appsink(struct AmlIPCGstBridge *br, GstElement *appsink) {
    gchar *name = NULL;
    g_object_get(G_OBJECT(appsink), "name", &name, NULL);
    if (name == NULL) {
        AML_LOGW("can't bridge an appsink without a name");
        return AML_STATUS_FAIL;
    }
    struct IPCAppSink *ipcsink = calloc(1, sizeof(*ipcsink));
    ipcsink->next = br->list_appsink;
    br->list_appsink = ipcsink;
    ipcsink->appsink = appsink;
    ipcsink->chan_id = br->num_channel++;
    br->num_appsink++;
    aml_ipc_add_channel(AML_IPC_COMPONENT(br), ipcsink->chan_id, AML_CHANNEL_OUTPUT, name);
    AML_LOGV("add appsink %d %s", ipcsink->chan_id, name);
    g_free(name);
    return AML_STATUS_OK;
}

static AmlStatus gstbr_setup_pipeline(struct AmlIPCGstBridge *br) {
    gboolean done = FALSE;
    GValue data = { 0, };
    GstIterator *iter = gst_bin_iterate_sources(GST_BIN_CAST(br->pipeline));
    while (!done) {
        switch (gst_iterator_next(iter, &data)) {
        case GST_ITERATOR_OK: {
            GstElement *child = g_value_get_object(&data);
            if (GST_IS_APP_SRC(child)) {
                gstbr_setup_appsrc(br, child);
            }
            g_value_reset(&data);
            break;
        }
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync(iter);
            break;
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        case GST_ITERATOR_ERROR:
            g_assert_not_reached();
            break;
        }
    }
    g_value_unset(&data);
    gst_iterator_free(iter);
    if (br->num_appsrc) {
        br->array_appsrc = calloc(br->num_appsrc, sizeof(*br->array_appsrc));
        struct IPCAppSrc *p = br->list_appsrc;
        for (int i = br->num_appsrc; i--; p = p->next) {
            br->array_appsrc[i] = p;
        }
    }

    done = FALSE;
    iter = gst_bin_iterate_sinks(GST_BIN_CAST(br->pipeline));
    while (!done) {
        switch (gst_iterator_next(iter, &data)) {
        case GST_ITERATOR_OK: {
            GstElement *child = g_value_get_object(&data);
            if (GST_IS_APP_SINK(child)) {
                gstbr_setup_appsink(br, child);
            }
            g_value_reset(&data);
            break;
        }
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync(iter);
            break;
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        case GST_ITERATOR_ERROR:
            g_assert_not_reached();
            break;
        }
    }
    g_value_unset(&data);
    gst_iterator_free(iter);
    if (br->num_appsink) {
        br->array_appsink = calloc(br->num_appsink, sizeof(*br->array_appsink));
        struct IPCAppSink *p = br->list_appsink;
        for (int i = br->num_appsink; i--; p = p->next) {
            br->array_appsink[i] = p;
        }
    }
    br->next = all_gst_bridges;
    all_gst_bridges = br;
    return AML_STATUS_OK;
}

static AmlStatus gstbr_set_property(struct AmlIPCGstBridge *br, struct AmlPropertyInfo *info,
                                    void *val) {
    struct AmlObjectClass *objcls = (struct AmlObjectClass *)AML_OBJ_PARENT_CLASS(br);
    if (info == PROP_PIPELINE) {
        AmlStatus status = objcls->set_property(AML_OBJECT(br), info, val);
        if (status != AML_STATUS_OK || br->gst_pipeline == NULL) {
            AML_LOGW("set property %d pipeline %s", status, br->gst_pipeline);
            return status;
        }
        if (!gst_init_check(NULL, NULL, NULL)) {
            AML_LOGW("failed to init gstreamer");
            return AML_STATUS_FAIL;
        }
        br->pipeline = gst_parse_launch(br->gst_pipeline, NULL);
        if (br->pipeline == NULL) {
            AML_LOGW("faile to parse launch string %s", br->gst_pipeline);
            return AML_STATUS_FAIL;
        }
        return gstbr_setup_pipeline(br);
    } else {
        struct AmlObjectClass *objcls = (struct AmlObjectClass *)AML_OBJ_PARENT_CLASS(br);
        return objcls->set_property(AML_OBJECT(br), info, val);
    }
    return AML_STATUS_OK;
}

static void gstbr_init(struct AmlIPCGstBridge *br) {
}

static void gstbr_free(struct AmlIPCGstBridge *br) {
    if (br->array_appsrc) {
        for (int i = 0; i < br->num_appsrc; ++i) {
            struct IPCAppSrc *src = br->array_appsrc[i];
            struct IPCRtspAppSrc *rtspsrc;
            while ((rtspsrc = LIST_FIRST(&src->rtsp_session)) != NULL) {
                LIST_REMOVE(rtspsrc, next);
                free(rtspsrc);
            }
            g_free(src->src_name);
            pthread_cond_destroy(&src->cond);
            pthread_mutex_destroy(&src->lock);
            free(src);
        }
        free(br->array_appsrc);
    }
    if (br->array_appsink) {
        for (int i = 0; i < br->num_appsink; ++i) {
            struct IPCAppSink *sink = br->array_appsink[i];
            free(sink);
        }
        free(br->array_appsink);
    }
}

static void gstbr_class_init(struct AmlIPCComponentClass *cls) {
    AML_OBJ_SET_CLASS_MEMBER(cls, start, gstbr_start);
    AML_OBJ_SET_CLASS_MEMBER(cls, stop, gstbr_stop);
    AML_OBJ_SET_CLASS_MEMBER(cls, handle_frame, gstbr_handle_frame);
    AML_OBJ_SET_CLASS_MEMBER((struct AmlObjectClass *)cls, set_property, gstbr_set_property);
    aml_obj_add_properties((struct AmlObjectClass *)cls,                                          //
                           PROP_PIPELINE, offsetof(struct AmlIPCGstBridge, gst_pipeline),         //
                           PROP_RTSP_BIND_ADDR, offsetof(struct AmlIPCGstBridge, rtsp_bind_addr), //
                           PROP_RTSP_SERVICE, offsetof(struct AmlIPCGstBridge, rtsp_service),     //
                           PROP_RTSP_MOUNT, offsetof(struct AmlIPCGstBridge, rtsp_mount),         //
                           PROP_RTSP_SERVER, offsetof(struct AmlIPCGstBridge, rtsp_server_name),  //
                           NULL);
}

AML_OBJ_DEFINE_TYPEID(AmlIPCGstBridge, AmlIPCComponent, AmlIPCComponentClass, gstbr_class_init,
                      gstbr_init, gstbr_free);

