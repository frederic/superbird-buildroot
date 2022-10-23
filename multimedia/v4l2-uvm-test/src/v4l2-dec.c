/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <poll.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "aml_driver.h"
#include "demux.h"
#include "drm.h"
#include "v4l2-dec.h"
#ifdef CONFIG_SECMEM
#include "secmem.h"
#endif
#include "vp9.h"

static const char* video_dev_name = "/dev/video26";
static int video_fd;
static bool get_1st_data = false;
static int cur_output_index = -1;
static decode_finish_fn decode_finish_cb;
static bool res_evt_pending;
static bool eos_evt_pending;
static bool eos_received;
extern int g_dw_mode;
static pthread_mutex_t res_lock;
static enum v4l2_memory sInMemMode;
static uint8_t* es_buf;
//#define DEBUG_FRAME
#ifdef DEBUG_FRAME
static int frame_checksum;
#endif
#define ES_BUF_SIZE (4*1024*1024)

static pthread_t dec_thread;
bool quit_thread;
extern int config_sys_node(const char* path, const char* value);
extern int span(struct timeval* t1, struct timeval* t2);

struct frame_buffer {
    struct v4l2_buffer v4lbuf;
    struct v4l2_plane v4lplane[2];
    uint8_t *vaddr[2];
    int gem_fd[2];
    bool queued;

    /* output only */
    uint32_t used;
    struct secmem* smem;

    /* capture only */
    bool free_on_recycle;
    struct drm_frame *drm_frame;
};

struct port_config {
    enum v4l2_buf_type type;
    uint32_t pixelformat;
    struct v4l2_format sfmt;
    int buf_num;
    int plane_num;
    struct frame_buffer** buf;
    pthread_mutex_t lock;
    pthread_cond_t wait;
};

#define OUTPUT_BUF_CNT 4
static struct port_config output_p;
static struct port_config capture_p;

static int d_o_push_num;
static int d_o_rec_num;
static int d_c_push_num;
static int d_c_rec_num;

struct profile {
    bool res_change;
    struct timeval last_flag;
    struct timeval buffer_done;
    struct timeval first_frame;
};
static struct profile res_profile;

static enum vtype v4l2_fourcc_to_vtype(uint32_t fourcc) {
    switch (fourcc) {
        case V4L2_PIX_FMT_H264:
            return VIDEO_TYPE_H264;
        case V4L2_PIX_FMT_HEVC:
            return VIDEO_TYPE_H265;
        case V4L2_PIX_FMT_VP9:
            return VIDEO_TYPE_VP9;
        case V4L2_PIX_FMT_MPEG2:
            return VIDEO_TYPE_MPEG2;
	case V4L2_PIX_FMT_MJPEG:
	    return VIDEO_TYPE_MJPEG;
    }
    return VIDEO_TYPE_MAX;
}

static uint32_t get_driver_min_buffers (int fd, bool capture_port)
{
    struct v4l2_control control = { 0, };

    if (!capture_port)
        control.id = V4L2_CID_MIN_BUFFERS_FOR_OUTPUT;
    else
        control.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;

    if (!ioctl(fd, VIDIOC_G_CTRL, &control))
        return control.value;

    return 0;
}

static int setup_output_port(int fd)
{
    int i,ret;
    struct v4l2_requestbuffers req;

    req.count = get_driver_min_buffers(fd, false);
    if (!req.count) {
        printf("fail to get min buffers for output, use default\n");
        req.count = OUTPUT_BUF_CNT;
    }

    req.memory = sInMemMode;
    req.type = output_p.type;

    ret = ioctl(fd, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        printf("output VIDIOC_REQBUFS fail ret:%d\n", ret);
        return 1;
    }
    output_p.buf_num = req.count;
    printf("output gets %d buf\n", req.count);

    output_p.buf = calloc(req.count, sizeof(struct frame_buffer *));
    if (!output_p.buf) {
        printf("%d oom\n", __LINE__);
        return 2;
    }
    for (i = 0 ; i < req.count ; i++)
        output_p.buf[i] = calloc(1, sizeof(struct frame_buffer));

    for (i = 0; i < req.count; i++) {
        int j;
        struct frame_buffer* pb = output_p.buf[i];
        pb->v4lbuf.index = i;
        pb->v4lbuf.type = output_p.type;
        pb->v4lbuf.memory = sInMemMode;
        pb->v4lbuf.length = output_p.plane_num;
        pb->v4lbuf.m.planes = pb->v4lplane;

        ret = ioctl(fd, VIDIOC_QUERYBUF, &pb->v4lbuf);
        if (ret) {
            printf("VIDIOC_QUERYBUF %dth buf fail ret:%d\n", i, ret);
            return 3;
        }

        if (sInMemMode != V4L2_MEMORY_MMAP)
            continue;
        for (j = 0; j < output_p.plane_num; j++) {
            void *vaddr;
            vaddr = mmap(NULL, pb->v4lplane[j].length,
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, pb->v4lplane[j].m.mem_offset);
            if (vaddr == MAP_FAILED) {
                printf("%s mmap faild len:%d offset:%x\n", __func__,
                        pb->v4lplane[j].length, pb->v4lplane[j].m.mem_offset);
                return 4;
            }
            pb->vaddr[j] = (uint8_t *)vaddr;
        }

    }

    pthread_mutex_init(&output_p.lock, NULL);
    pthread_cond_init(&output_p.wait, NULL);

    if (sInMemMode == V4L2_MEMORY_DMABUF) {
        es_buf = malloc(ES_BUF_SIZE);
        if (!es_buf) {
            printf("%d OOM\n", __LINE__);
            return 5;
        }
    }
    return 0;
}

static int destroy_output_port(int fd) {
    int i;
    struct v4l2_requestbuffers req = {
        .memory = sInMemMode,
        .type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
        .count = 0,
    };
    pthread_mutex_destroy(&output_p.lock);
    pthread_cond_destroy(&output_p.wait);
    ioctl(fd, VIDIOC_REQBUFS, &req);
    for (i = 0 ; i < req.count ; i++) {
        struct frame_buffer *buf = output_p.buf[i];

#ifdef CONFIG_SECMEM
        if (sInMemMode == V4L2_MEMORY_DMABUF)
            secmem_free(buf->smem);
#endif

        free(buf);
    }
    free(output_p.buf);
    return 0;
}

static int setup_capture_port(int fd)
{
    int i,ret;
    uint32_t coded_w, coded_h;
    struct v4l2_requestbuffers req;


    /* coded size should be ready now */
    memset(&capture_p.sfmt, 0, sizeof(struct v4l2_format));
    capture_p.sfmt.type = capture_p.type;
    ret = ioctl(video_fd, VIDIOC_G_FMT, &capture_p.sfmt);
    if (ret) {
        printf("%d VIDIOC_G_FMT fail :%d\n", __LINE__, ret);
        return -1;
    }
    coded_w = capture_p.sfmt.fmt.pix_mp.width;
    coded_h = capture_p.sfmt.fmt.pix_mp.height;
    printf("capture port (%dx%d)\n", coded_w, coded_h);

    if (g_dw_mode != VDEC_DW_AFBC_ONLY) {
        capture_p.sfmt.fmt.pix_mp.pixelformat =
            (output_p.pixelformat == V4L2_PIX_FMT_MJPEG) ?
            V4L2_PIX_FMT_YUV420M : V4L2_PIX_FMT_NV12M;
    } else {
        capture_p.sfmt.fmt.pix_mp.pixelformat =
            (output_p.pixelformat == V4L2_PIX_FMT_MJPEG) ?
            V4L2_PIX_FMT_YUV420 : V4L2_PIX_FMT_NV12;
    }
    ret = ioctl(video_fd, VIDIOC_S_FMT, &capture_p.sfmt);
    if (ret) {
        printf("VIDIOC_S_FMT fail %d\n", ret);
        return -1;
    }
    printf("set capture port to %s\n",
            (output_p.pixelformat == V4L2_PIX_FMT_MJPEG) ?
            (g_dw_mode == VDEC_DW_AFBC_ONLY)?"I420":"I420M" :
            (g_dw_mode == VDEC_DW_AFBC_ONLY)?"NV12":"NV12M");
    capture_p.plane_num = capture_p.sfmt.fmt.pix_mp.num_planes;
    printf("number of capture plane: %d\n", capture_p.plane_num);

    memset(&req, 0 , sizeof(req));
    req.memory = V4L2_MEMORY_DMABUF;
    req.type = capture_p.type;
    req.count = get_driver_min_buffers(fd, true);
    if (!req.count) {
        printf("get min buffers fail\n");
        return -1;
    }

    printf("req capture count:%d\n", req.count);
    ret = ioctl(fd, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        printf("capture VIDIOC_REQBUFS fail ret:%d\n", ret);
        return 1;
    }
    capture_p.buf_num = req.count;
    printf("capture gets %d buf\n", req.count);

    capture_p.buf = calloc(req.count, sizeof(struct frame_buffer *));
    if (!capture_p.buf) {
        printf("%d oom\n", __LINE__);
        return 2;
    }
    for (i = 0 ; i < req.count ; i++) {
        capture_p.buf[i] = calloc(1, sizeof(struct frame_buffer));
        if (!capture_p.buf[i]) {
            printf("%d oom\n", __LINE__);
            return 2;
        }
    }

    for (i = 0; i < req.count; i++) {
        struct frame_buffer* pb = capture_p.buf[i];
        int fd[4] = {0}, j;

        pb->v4lbuf.index = i;
        pb->v4lbuf.type = capture_p.type;
        pb->v4lbuf.memory = V4L2_MEMORY_DMABUF;
        pb->v4lbuf.length = capture_p.plane_num;
        pb->v4lbuf.m.planes = pb->v4lplane;
        /* allocate DRM-GEM buffers */
        pb->drm_frame = display_create_buffer(coded_w, coded_h,
                (g_dw_mode == VDEC_DW_AFBC_ONLY)? FRAME_FMT_AFBC:FRAME_FMT_NV12,
                capture_p.plane_num);
        if (ret) {
            printf("display_create_buffer %dth fail\n", i);
            return 2;
        }

        ret = ioctl(video_fd, VIDIOC_QUERYBUF, &pb->v4lbuf);
        if (ret) {
            printf("VIDIOC_QUERYBUF %dth buf fail ret:%d\n", i, ret);
            return 3;
        }
        ret = display_get_buffer_fds(pb->drm_frame, fd, 2);
        if (ret) {
            printf("display get %dth fds fail %d\n", i, ret);
            return 4;
        }
        /* bind GEM-fd with V4L2 buffer */
        for (j = 0; j < capture_p.plane_num; j++) {
            pb->vaddr[j] = NULL;
            pb->gem_fd[j] = fd[j];
            pb->v4lbuf.m.planes[j].m.fd = pb->gem_fd[j];
        }

        ret = ioctl(video_fd, VIDIOC_QBUF, &capture_p.buf[i]->v4lbuf);
        if (ret) {
            printf("VIDIOC_QBUF %dth buf fail ret:%d\n", i, ret);
            return 5;
        }
        capture_p.buf[i]->queued = true;
    }
    gettimeofday(&res_profile.buffer_done, NULL);

    return 0;
}

static int destroy_capture_port(int fd) {
    int i;
    struct v4l2_requestbuffers req = {
        .memory = V4L2_MEMORY_DMABUF,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
        .count = 0,
    };
    pthread_mutex_destroy(&capture_p.lock);
    pthread_cond_destroy(&capture_p.wait);
    ioctl(fd, VIDIOC_REQBUFS, &req);
    for (i = 0 ; i < req.count ; i++) {
        struct frame_buffer *buf = capture_p.buf[i];
        /* release GEM buf */
        buf->drm_frame->destroy(buf->drm_frame);
        free(capture_p.buf[i]);
    }
    free(capture_p.buf);
    return 0;
}

static void detect_res_change(struct v4l2_format *old, struct v4l2_format *new)
{
    if (old->fmt.pix_mp.width != new->fmt.pix_mp.width ||
        old->fmt.pix_mp.height != new->fmt.pix_mp.height) {
        printf("%dx%d --> %dx%d\n", old->fmt.pix_mp.width,
                old->fmt.pix_mp.height, new->fmt.pix_mp.width,
                new->fmt.pix_mp.height);
    }
}

static void handle_res_change()
{
    int i, ret;
    int free_cnt = 0, delay_cnt = 0;
    struct v4l2_format out_fmt = { 0 };
    struct v4l2_requestbuffers req = {
        .memory = V4L2_MEMORY_DMABUF,
        .type = capture_p.type,
        .count = 0,
    };

    out_fmt.type = output_p.type;
    ret = ioctl(video_fd, VIDIOC_G_FMT, &out_fmt);
    if (ret) {
        printf("output VIDIOC_G_FMT fail :%d\n",ret);
        goto error;
    }

    detect_res_change(&output_p.sfmt, &out_fmt);

    /* stop capture port */
    ret = ioctl(video_fd, VIDIOC_STREAMOFF, &capture_p.type);
    if (ret) {
        printf("VIDIOC_STREAMOFF fail ret:%d\n",ret);
        goto error;
    }

    pthread_mutex_lock(&res_lock);
    /* return all DRM-GEM buffers */
    ret = ioctl(video_fd, VIDIOC_REQBUFS, &req);
    if (ret) {
        printf("VIDIOC_REQBUFS fail ret:%d\n",ret);
        pthread_mutex_unlock(&res_lock);
        goto error;
    }
    for (i = 0 ; i < capture_p.buf_num; i++) {
        if (capture_p.buf[i]->queued) {
            struct drm_frame *drm_f = capture_p.buf[i]->drm_frame;

            drm_f->destroy(drm_f);
            free(capture_p.buf[i]);
            free_cnt++;
        } else {
            capture_p.buf[i]->free_on_recycle = true;
            delay_cnt++;
        }
    }
    pthread_mutex_unlock(&res_lock);
    free(capture_p.buf);
    printf("f/d/total: (%d/%d/%d)\n", free_cnt, delay_cnt, capture_p.buf_num);

    /* setup capture port again */
    ret = setup_capture_port(video_fd);
    if (ret) {
        printf(" setup_capture_port fail ret:%d\n",ret);
        goto error;
    }

    /* start capture again */
    ret = ioctl(video_fd, VIDIOC_STREAMON, &capture_p.type);
    if (ret) {
        printf("VIDIOC_STREAMON fail ret:%d\n",ret);
        goto error;
    }
    return;

error:
    printf("debug...\n");
    while (1)
        sleep(1);
}

static uint32_t v4l2_get_event(int fd)
{
    int ret;
    struct v4l2_event evt = { 0 };

    ret = ioctl(fd, VIDIOC_DQEVENT, &evt);
    if (ret) {
        printf("VIDIOC_DQEVENT fail, ret:%d\n", ret);
        return 0;
    }
    return evt.type;
}

static void *dec_thread_func(void * arg) {
    int ret = 0;
    struct pollfd pfd = {
        /* default blocking capture */
        .events =  POLLIN | POLLRDNORM | POLLPRI | POLLOUT | POLLWRNORM,
        .fd = video_fd,
    };
    while (!quit_thread) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[2];

        for (;;) {
            ret = poll(&pfd, 1, 10);
            if (ret > 0)
                break;
            if (quit_thread)
                goto exit;
            if (errno == EINTR)
                continue;

        }

        /* error handling */
        if (pfd.revents & POLLERR)
            printf("POLLERR received\n");

        /* res change */
        if (pfd.revents & POLLPRI) {
            uint32_t evt;
            evt = v4l2_get_event(video_fd);
            if (evt == V4L2_EVENT_SOURCE_CHANGE)
                res_evt_pending = true;
            else if (evt == V4L2_EVENT_EOS)
                eos_evt_pending = true;
            printf("res_evt_pending:%d eos_evt_pending:%d\n",
                    res_evt_pending, eos_evt_pending);
        }



        /* dqueue output buf */
        if (pfd.revents & (POLLOUT | POLLWRNORM)) {
            memset(&buf, 0, sizeof(buf));
            memset(planes, 0, sizeof(planes));
            buf.memory = sInMemMode;
            buf.type = output_p.type;
            buf.length = 2;
            buf.m.planes = planes;

            ret = ioctl(video_fd, VIDIOC_DQBUF, &buf);
            if (ret) {
                printf("output VIDIOC_DQBUF fail %d\n", ret);
            } else {
                struct frame_buffer *fb = output_p.buf[buf.index];
#ifdef DEBUG_FRAME
                printf("dqueue output %d\n", buf.index);
#endif
                pthread_mutex_lock(&output_p.lock);
                fb->queued = false;
                pthread_mutex_unlock(&output_p.lock);
                fb->used = 0;
                d_o_rec_num++;
#ifdef CONFIG_SECMEM
                if (sInMemMode == V4L2_MEMORY_DMABUF) {
                    secmem_free(fb->smem);
                }
#endif
                pthread_cond_signal(&output_p.wait);
            }
        }
        /* dqueue capture buf */
        if (pfd.revents & (POLLIN | POLLRDNORM)) {
            memset(&buf, 0, sizeof(buf));
            memset(planes, 0, sizeof(planes));
            buf.memory = V4L2_MEMORY_DMABUF;
            buf.type = capture_p.type;
            buf.length = 2;
            buf.m.planes = planes;

            ret = ioctl(video_fd, VIDIOC_DQBUF, &buf);
            if (ret) {
                printf("capture VIDIOC_DQBUF fail %d\n", ret);
            } else {
#ifdef DEBUG_FRAME
                printf("dqueue cap %d\n", buf.index);
#endif
                capture_p.buf[buf.index]->queued = false;
                d_c_push_num++;
                /* ignore res change for 1st frame */
                if (d_c_push_num == 1 && res_evt_pending) {
                    printf("ignore res change for 1st frame\n");
                    res_evt_pending = false;
                }
                if (!(buf.flags & V4L2_BUF_FLAG_LAST)) {
                    struct drm_frame *drm_f = capture_p.buf[buf.index]->drm_frame;
                    /* display capture buf */
                    if (res_profile.res_change) {
                        res_profile.res_change = false;
                        gettimeofday(&res_profile.first_frame, NULL);
                        printf("--event to buffer done %dms--\n",
                                span(&res_profile.last_flag, &res_profile.buffer_done));
                        printf("--event to 1st frame %dms--\n",
                                span(&res_profile.last_flag, &res_profile.first_frame));
                    }
                    drm_f->pri_dec =  capture_p.buf[buf.index];
                    display_engine_show(drm_f);
                } else {
                    uint32_t evt = 0;
                    if (!res_evt_pending && !eos_evt_pending)
                        evt = v4l2_get_event(video_fd);

                    if (evt == V4L2_EVENT_SOURCE_CHANGE || res_evt_pending) {
                        printf("res changed\n");
                        res_evt_pending = false;
                        gettimeofday(&res_profile.last_flag, NULL);
                        res_profile.res_change = true;
                        /* this buffer will be freed in handle_res_change */
                        capture_p.buf[buf.index]->queued = true;
                        handle_res_change();
                        d_c_rec_num++;
                        continue;
                    } else if (evt == V4L2_EVENT_EOS || eos_evt_pending) {
                        printf("EOS received\n");
                        eos_received = true;
                        eos_evt_pending = false;
                        display_wait_for_display();
                        d_c_rec_num++;
                        dump_v4l2_decode_state();
                        decode_finish_cb();
                        break;
                    }
                }
            }
        }
    }

exit:
    /* stop output port */
    ret = ioctl(video_fd, VIDIOC_STREAMOFF, &output_p.type);
    if (ret) {
        printf("VIDIOC_STREAMOFF fail ret:%d\n",ret);
    }

    /* stop capture port */
    ret = ioctl(video_fd, VIDIOC_STREAMOFF, &capture_p.type);
    if (ret) {
        printf("VIDIOC_STREAMOFF fail ret:%d\n",ret);
    }

    return NULL;
}

static int config_decoder(int fd, enum vtype type)
{
    int ret = -1;
    struct v4l2_streamparm para;
    struct v4l2_event_subscription sub;
    struct aml_dec_params *dec_p = (struct aml_dec_params *)para.parm.raw_data;

    memset(&para, 0 , sizeof(para));
    para.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    dec_p->parms_status = V4L2_CONFIG_PARM_DECODE_CFGINFO;
    dec_p->cfg.double_write_mode = g_dw_mode;
    /* number of extra buffer for display pipelien to run */
    /* MPEG will use hardcoded size in driver */
    if (type != VIDEO_TYPE_MPEG2)
        dec_p->cfg.ref_buf_margin = 7;

    //optional adjust dec_p->hdr for hdr support

    if ((ret = ioctl(video_fd, VIDIOC_S_PARM, &para))) {
        printf("VIDIOC_S_PARM fails ret:%d\n", ret);
        return ret;
    }

    /* subscribe to resolution change and EOS event */
    memset(&sub, 0, sizeof(sub));
    sub.type = V4L2_EVENT_SOURCE_CHANGE;
    ret = ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    if (ret) {
        printf("subscribe V4L2_EVENT_SOURCE_CHANGE fail\n");
        return ret;
    }

    memset(&sub, 0, sizeof(sub));
    sub.type = V4L2_EVENT_EOS;
    ret = ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    if (ret) {
        printf("subscribe V4L2_EVENT_EOS fail\n");
        return ret;
    }
    return 0;
}

#define V4L2_CID_USER_AMLOGIC_BASE (V4L2_CID_USER_BASE + 0x1100)
#define AML_V4L2_SET_DRMMODE (V4L2_CID_USER_AMLOGIC_BASE + 0)
static int v4l2_dec_set_drmmode(bool drm_enable)
{
    int rc = 0;
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;

    memset (&queryctrl, 0, sizeof (queryctrl));
    queryctrl.id = AML_V4L2_SET_DRMMODE;

    if (-1 == ioctl (video_fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        printf ("AML_V4L2_SET_DRMMODE is not supported\n");
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        printf ("AML_V4L2_SET_DRMMODE is disable\n");
    } else {
        memset (&control, 0, sizeof (control));
        control.id = AML_V4L2_SET_DRMMODE;
        control.value = drm_enable;

        rc = ioctl (video_fd, VIDIOC_S_CTRL, &control);
        if (rc) {
            printf ("AML_V4L2_SET_DRMMODE fail\n");
        }
    }
    return rc;
}

int v4l2_dec_init(enum vtype type, int secure, decode_finish_fn cb)
{
    int ret = -1;
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fdesc;
    bool supported;

    if (!cb)
        return 1;
    decode_finish_cb = cb;

    if (secure) {
#ifdef CONFIG_SECMEM
        sInMemMode = V4L2_MEMORY_DMABUF;
        ret = secmem_init();
        if (ret) {
            printf("secmem_init fail %d\n",ret);
            return 1;
        }
#endif
    } else
        sInMemMode = V4L2_MEMORY_MMAP;

    /* check decoder mode */
    if ((type == VIDEO_TYPE_H264 ||
        type == VIDEO_TYPE_MPEG2) &&
        g_dw_mode != 16) {
        printf("mpeg2/h264 only support DW 16 mode\n");
        goto error;
    }

    /* config sys nodes */
    if (config_sys_node("/sys/module/amvdec_ports/parameters/multiplanar", "1")) {
        printf("set multiplanar fails\n");
        goto error;
    }
    if (type == VIDEO_TYPE_VP9 && !secure) {
        if (config_sys_node("/sys/module/amvdec_ports/parameters/vp9_need_prefix", "1")) {
            printf("set vp9_need_prefix fails\n");
            goto error;
        }
    }
    if (type == VIDEO_TYPE_MPEG2) {
        //TODO: delete after ucode updated for mpeg2
        if (config_sys_node("/sys/module/amvdec_ports/parameters/param_sets_from_ucode", "0")) {
            printf("set param_sets_from_ucode 0 fails\n");
            goto error;
        }
    }

    pthread_mutex_init(&res_lock, NULL);
    video_fd = open(video_dev_name, O_RDWR | O_NONBLOCK, 0);
    if (video_fd < 0) {
        printf("Unable to open video node: %s\n",
            strerror(errno));
        goto error;
    }

    if (type != VIDEO_TYPE_MPEG2) {
        ret = v4l2_dec_set_drmmode(secure);
        if (ret) {
            printf("aml_v4l2_dec_set_drmmode fail\n");
            goto error;
        }
    }

    if ((ret = ioctl(video_fd, VIDIOC_QUERYCAP, &cap))) {
        printf("VIDIOC_QUERYCAP fails ret:%d\n", ret);
        goto error;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE)) {
        printf("V4L2_CAP_VIDEO_M2M_MPLANE fails\n");
        goto error;
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("V4L2_CAP_STREAMING fails\n");
        goto error;
    }

    /* output fmt */
    output_p.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    supported = false;
    memset(&fdesc, 0, sizeof(fdesc));
    fdesc.type = output_p.type;
    for (;;) {
        ret = ioctl(video_fd, VIDIOC_ENUM_FMT, &fdesc);
        if (ret)
            break;
        if (v4l2_fourcc_to_vtype(fdesc.pixelformat) == type) {
            supported = true;
            output_p.pixelformat = fdesc.pixelformat;
            break;
        }
        fdesc.index++;
    }
    if (!supported) {
        printf("output format not supported:%d\n", type);
        goto error;
    }

    if (config_decoder(video_fd, type)) {
        printf("config_decoder error\n");
        goto error;
    }

    memset(&output_p.sfmt, 0, sizeof(struct v4l2_format));
    output_p.sfmt.type = output_p.type;
    ret = ioctl(video_fd, VIDIOC_G_FMT, &output_p.sfmt);
    if (ret) {
        printf("VIDIOC_G_FMT fail\n");
        goto error;
    }

    output_p.sfmt.fmt.pix_mp.pixelformat = output_p.pixelformat;
    /* 4K frame should fit into 2M */
    output_p.sfmt.fmt.pix_mp.plane_fmt[0].sizeimage = ES_BUF_SIZE;
    ret = ioctl(video_fd, VIDIOC_S_FMT, &output_p.sfmt);
    if (ret) {
        printf("VIDIOC_S_FMT 0x%x fail\n", output_p.pixelformat);
        goto error;
    }
    output_p.plane_num = output_p.sfmt.fmt.pix_mp.num_planes;

    /* capture fmt */
    capture_p.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    supported = false;
    memset(&fdesc, 0, sizeof(fdesc));
    fdesc.type = capture_p.type;
    for (;;) {
        ret = ioctl(video_fd, VIDIOC_ENUM_FMT, &fdesc);
        if (ret)
            break;
        if ((fdesc.pixelformat == V4L2_PIX_FMT_NV12M &&
                g_dw_mode != VDEC_DW_AFBC_ONLY) ||
             (fdesc.pixelformat == V4L2_PIX_FMT_NV12 &&
              g_dw_mode == VDEC_DW_AFBC_ONLY)) {
            supported = true;
            capture_p.pixelformat = fdesc.pixelformat;
            break;
        }
        fdesc.index++;
    }
    if (!supported) {
        printf("capture format not supported\n");
        goto error;
    }

    /* setup output port */
    ret = setup_output_port(video_fd);
    if (ret) {
        printf("setup_output_port fail\n");
        goto error;
    }

    /* start output port */
    ret = ioctl(video_fd, VIDIOC_STREAMON, &output_p.type);
    if (ret) {
        printf("VIDIOC_STREAMON fail ret:%d\n",ret);
        goto error;
    }

    printf("output stream on\n");
    return 0;

error:
    close(video_fd);
    video_fd = 0;
    return ret;
}

static int handle_first_frame()
{
    int ret;
    /* Wait for 1st video ES data and setup capture port */

    ret = setup_capture_port(video_fd);
    if (ret) {
        printf(" setup_capture_port fail ret:%d\n",ret);
        goto error;
    }

    pthread_mutex_init(&capture_p.lock, NULL);
    pthread_cond_init(&capture_p.wait, NULL);

    /* start capture port */
    ret = ioctl(video_fd, VIDIOC_STREAMON, &capture_p.type);
    if (ret) {
        printf("VIDIOC_STREAMON fail ret:%d\n",ret);
        goto error;
    }
    printf("capture stream on\n");

    if (pthread_create(&dec_thread, NULL, dec_thread_func, NULL)) {
        printf("dec thread fail\n");
        return -1;
    }
    return 0;
error:
    printf("handle_first_frame fatal\n");
    while (1) sleep(1);
    exit(1);
}

int v4l2_dec_destroy()
{
    quit_thread = true;
    pthread_cond_signal(&output_p.wait);
    pthread_join(dec_thread, NULL);
    destroy_output_port(video_fd);
    destroy_capture_port(video_fd);
    get_1st_data = false;
    pthread_mutex_destroy(&res_lock);
    close(video_fd);
#ifdef CONFIG_SECMEM
    secmem_destroy();
#endif
    if (es_buf)
        free(es_buf);
    return 0;
}

static int get_free_output_buf()
{
    int i;
    pthread_mutex_lock(&output_p.lock);
    do {
        for (i = 0; i < output_p.buf_num; i++) {
            if (!output_p.buf[i]->queued) {
                break;
            }
        }

        if (i == output_p.buf_num) {
            pthread_cond_wait(&output_p.wait, &output_p.lock);
            if (quit_thread)
                break;
        } else {
            break;
        }
    } while (1);
    pthread_mutex_unlock(&output_p.lock);

    if (i == output_p.buf_num)
        return -1;

    return i;
}

#if 0
static void dump(const char* path, const uint8_t *data, int size) {
    FILE* fd = fopen(path, "ab");
    if (!fd)
        return;
    fwrite(data, 1, size, fd);
    fclose(fd);
}
#endif

int v4l2_dec_write_es(const uint8_t *data, int size)
{
    struct frame_buffer *p;

    if (cur_output_index == -1)
        cur_output_index = get_free_output_buf();

    if (cur_output_index < 0) {
        printf("%s %d can not get output buf\n", __func__, __LINE__);
        return 0;
    }

    p = output_p.buf[cur_output_index];
    if ((sInMemMode == V4L2_MEMORY_MMAP &&
            (p->used + size) > p->v4lplane[0].length) ||
        (sInMemMode == V4L2_MEMORY_DMABUF &&
          (p->used + size) > ES_BUF_SIZE)) {
        printf("fatal frame too big %d\n",
                size + p->used);
        return 0;
    }

    if (sInMemMode == V4L2_MEMORY_MMAP)
        memcpy(p->vaddr[0] + p->used, data, size);
    else
        memcpy(es_buf + p->used, data, size);

    p->used += size;

#ifdef DEBUG_FRAME
    int i;
    for (i = 0 ; i < size ; i++)
        frame_checksum += data[i];
#endif

    //dump("/data/es.h265", data, size);
    //
    return size;
}

int v4l2_dec_frame_done(int64_t  pts)
{
    int ret;
    struct frame_buffer *p;

    if (quit_thread)
        return 0;

    if (cur_output_index < 0 || cur_output_index >= output_p.buf_num) {
        printf("BUG %s %d idx:%d\n", __func__, __LINE__, cur_output_index);
        return 0;
    }
    p = output_p.buf[cur_output_index];
#ifdef CONFIG_SECMEM
    if (sInMemMode == V4L2_MEMORY_DMABUF) {
        int frame_size = p->used;
        int offset = 0;
        struct vp9_superframe_split s;

        if (output_p.pixelformat == V4L2_PIX_FMT_VP9) {
            s.data = es_buf;
            s.data_size = p->used;
            ret = vp9_superframe_split_filter(&s);
            if (ret) {
                printf("parse vp9 superframe fail\n");
                return 0;
            }
            frame_size = s.size + s.nb_frames * 16;
        }

        p->smem = secmem_alloc(frame_size);
        if (!p->smem) {
            printf("%s %d oom:%d\n", __func__, __LINE__, p->used);
            return 0;
        }

        /* copy to secmem */
        if (output_p.pixelformat == V4L2_PIX_FMT_VP9) {
            int stream_offset = s.size;
            offset = frame_size;
            for (int i = s.nb_frames; i > 0; i--) {
                uint8_t header[16];
                uint32_t sub_size = s.sizes[i - 1];

                /* frame body */
                offset -= sub_size;
                stream_offset -= sub_size;
                secmem_fill(p->smem, es_buf + stream_offset, offset, sub_size);

                /* prepend header */
                offset -= 16;
                fill_vp9_header(header, sub_size);
                secmem_fill(p->smem, header, offset, 16);
            }
        } else
            secmem_fill(p->smem, es_buf, offset, p->used);

        p->v4lbuf.m.planes[0].m.fd = p->smem->fd;
        p->v4lbuf.m.planes[0].length = frame_size;
        p->v4lbuf.m.planes[0].data_offset = 0;
    }
#endif
    p->v4lbuf.m.planes[0].bytesused = p->used;

    /* convert from ns to timeval */
    p->v4lbuf.timestamp.tv_sec = pts / DMX_SECOND;
    p->v4lbuf.timestamp.tv_usec = pts % DMX_SECOND;

    pthread_mutex_lock(&output_p.lock);
    p->queued = true;
    pthread_mutex_unlock(&output_p.lock);
    ret = ioctl(video_fd, VIDIOC_QBUF, &p->v4lbuf);
    if (ret) {
        printf("write es VIDIOC_QBUF %dth buf fail ret:%d\n",
                cur_output_index, ret);
        return 0;
    }
#ifdef DEBUG_FRAME
    printf("%s queue output %d frame_checksum:%x\n", __func__, cur_output_index, frame_checksum);
    frame_checksum = 0;
#endif
    d_o_push_num++;
    cur_output_index = -1;
    /* can setup capture port now */
    if (!get_1st_data) {
        printf("%s 1st frame done\n", __func__);
        get_1st_data = true;
        handle_first_frame();
    } else {
    }
    return ret;
}

int capture_buffer_recycle(void* handle)
{
    int ret = 0;
    struct frame_buffer *frame = handle;

    d_c_rec_num++;
    pthread_mutex_lock(&res_lock);
    if (frame->free_on_recycle) {
        printf("free index:%d\n", frame->v4lbuf.index);
        frame->drm_frame->destroy(frame->drm_frame);
        free(frame);
        goto exit;
    }

    if (eos_received)
        goto exit;

    ret = ioctl(video_fd, VIDIOC_QBUF, &frame->v4lbuf);
    if (ret) {
        printf("VIDIOC_QBUF %dth buf fail ret:%d\n", frame->v4lbuf.index, ret);
        goto exit;
    } else {
#ifdef DEBUG_FRAME
        printf("queue cap %d\n", frame->v4lbuf.index);
#endif
        frame->queued = true;
    }
exit:
    pthread_mutex_unlock(&res_lock);
    return ret;
}

int v4l2_dec_eos()
{
    int ret;
    struct v4l2_decoder_cmd cmd = {
        .cmd = V4L2_DEC_CMD_STOP,
        .flags = 0,
    };

    printf("%s EOS received\n", __func__);
    /* flush decoder */
    ret = ioctl(video_fd, VIDIOC_DECODER_CMD, &cmd);
    if (ret)
        printf("V4L2_DEC_CMD_STOP output fail ret:%d\n",ret);

    return ret;
}

void dump_v4l2_decode_state()
{
    printf("-----------------------\n");
    printf("output port:  push(%d) pop(%d)\n", d_o_push_num, d_o_rec_num);
    printf("capture port: push(%d) pop(%d)\n", d_c_push_num, d_c_rec_num);
    printf("-----------------------\n");
}
