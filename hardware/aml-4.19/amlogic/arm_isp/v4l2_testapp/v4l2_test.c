//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------


/*
 * Apical(ARM) V4L2 test application 2018
 *
 * This is ARM internal development purpose SW tool running on JUNO.
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "common.h"
#include "isp_metadata.h"
#include "renderer.h"
#include "capture.h"
#include "logs.h"
#include "gdc_api.h"

#define STATIC_STREAM_COUNT (ARM_V4L2_TEST_STREAM_MAX - ARM_V4L2_TEST_HAS_RAW)
#define NB_BUFFER           6
#define DUMP_RAW            0
static capture_module_t g_cap_mod = { 0, };
static int sensor_bits = 10;

static char *str_on_off[2] = { "ON", "OFF" };

#if DUMP_RAW
static int dump_fd = -1;
#endif
unsigned char *displaybuf_fr;
unsigned char *displaybuf_ds1;
unsigned char *displaybuf_ds2;
unsigned char *displaybuf_meta;
static int fb_fd = 0;
/**********
 * thread parameters
 */
/* control parameters */
static uint32_t         v4l2_test_thread_exit = 0;
static uint32_t         v4l2_test_thread_preview = 0;
static capture_type_t   v4l2_test_thread_capture = V4L2_TEST_CAPTURE_NONE;
static uint32_t         v4l2_test_thread_capture_count = 0;
static uint32_t         v4l2_test_thread_dump = 0;

static int fps_test_port = -1;
static int open_port_cnt = 1;
static int fb_buffer_cnt = 3;

static int ir_cut_state = 1;
static uint32_t manual_exposure_enable = 0;
static uint32_t manual_sensor_integration_time = 1;
static uint32_t manual_sensor_analog_gain = 0;
static uint32_t manual_sensor_digital_gain = 0;
static uint32_t manual_isp_digital_gain = 0;

static uint32_t stop_sensor_update = 0;
static uint32_t max_int_time = 0;

#define GDC_CFG_FILE_NAME "nv12_1920_1080_cfg.bin"

#define LINE_SIZE 128
#define LINE_MASK (~(LINE_SIZE - 1))
#define LINE_ALIGN(size) ((size + LINE_SIZE - 1) & LINE_MASK)

#define ISP_METERING_ZONES_AE_MAX_H 33
#define ISP_METERING_ZONES_AE_MAX_V 33
#define ISP_METERING_ZONES_AWB_MAX_H 33
#define ISP_METERING_ZONES_AWB_MAX_V 33

static unsigned char ae_zone_weight[ISP_METERING_ZONES_AE_MAX_V][ISP_METERING_ZONES_AE_MAX_H];
static unsigned char awb_zone_weight[ISP_METERING_ZONES_AWB_MAX_V][ISP_METERING_ZONES_AWB_MAX_H];


/* config parameters */
struct thread_param {
    /* video device info */
    char                        * devname;

    /* display device info */
    char                        * fbp;
    struct fb_var_screeninfo    vinfo;
    struct fb_fix_screeninfo    finfo;

    /* format info */
    uint32_t                    width;
    uint32_t                    height;
    uint32_t                    pixformat;
    uint32_t                    wdr_mode;
    uint32_t                    exposure;

    /* for snapshot stream (non-zsl implementation) */
    int32_t                     capture_count;
    int32_t                     gdc_ctrl;
    int                         videofd;
    uint32_t  c_width;
    uint32_t  c_height;
    uint32_t a_ctrl;
};

pthread_t tid[STATIC_STREAM_COUNT];


/**********
 * helper functions
 */
uint64_t getTimestamp() {
    struct timespec ts;
    int rc;

    rc = clock_gettime(0x00, &ts);
    if (rc == 0) {
        return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    } else {
        return 0;
    }
}

int64_t GetTimeMsec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static uint8_t cmd_do_af_refocus = 0;

static void do_af_refocus(int videofd)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_AF_REFOCUS;
    ctrl.value = 0;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do AF refocus failed");
    }
}

static void do_sensor_preset(int videofd, int preset)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_SENSOR_PRESET;
    ctrl.value = preset;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do sensor preset failed");
    }
}

#if 0
static void do_other_commands(int videofd)
{
    if( cmd_do_af_refocus ) {
        cmd_do_af_refocus = 0;
        do_af_refocus(videofd);
    }
}
#endif

static void do_sensor_wdr_mode(int videofd, int mode)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_CUSTOM_SENSOR_WDR_MODE;
    ctrl.value = mode;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do sensor wdr mode failed\n");
    }
}

static void do_fr_fps(int videofd, int fps)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_FR_FPS;
    ctrl.value = fps;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do sensor fps failed");
    }
}

static void do_ds1_fps(int videofd, int fps)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_DS1_FPS;
    ctrl.value = fps;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do sensor fps failed");
    }
}

static int do_get_dma_buf_fd(int videofd, uint32_t index, uint32_t plane)
{
    struct v4l2_exportbuffer ex_buf;

    ex_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ex_buf.index = index;
    ex_buf.plane = plane;
    ex_buf.flags = 0;
    ex_buf.fd = -1;

    if (ioctl(videofd, VIDIOC_EXPBUF, &ex_buf))  {
        ERR("LIKE-0:Failed get dma buf fd\n");
    }

    return ex_buf.fd;
}

static void do_fr_set_ae_zone_weight(int videofd)
{
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;

    memset(&ctrls, 0, sizeof(ctrls));
    memset(&ctrls, 0, sizeof(ctrl));

    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_AE_ZONE_WEIGHT;
    ctrl.ptr = ae_zone_weight;

    ctrls.which = 0;
    ctrls.count = 1;
    ctrls.controls = &ctrl;

    if (-1 == ioctl (videofd, VIDIOC_S_EXT_CTRLS, &ctrls)) {
        printf("Do set ae zone weight failed\n");
    }
}

static void do_fr_set_awb_zone_weight(int videofd)
{
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;

    memset(&ctrls, 0, sizeof(ctrls));
    memset(&ctrls, 0, sizeof(ctrl));

    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_AWB_ZONE_WEIGHT;
    ctrl.ptr = awb_zone_weight;

    ctrls.which = 0;
    ctrls.count = 1;
    ctrls.controls = &ctrl;

   if (-1 == ioctl (videofd, VIDIOC_S_EXT_CTRLS, &ctrls)) {
        printf("Do set awb zone weight failed\n");
   }
}

static void do_sensor_exposure(int videofd, int exp)
{
    struct v4l2_control ctrl;

    ctrl.id = ISP_V4L2_CID_CUSTOM_SENSOR_EXPOSURE;
    ctrl.value = exp;

    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("Do sensor exposure failed\n");
    }
}

static void set_manual_exposure(int videofd, int enable)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_MANUAL_EXPOSURE;
    ctrl.value = enable;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_manual_exposure failed\n");
    }
}

static void do_sensor_ir_cut(int videofd, int ircut_state)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SENSOR_IR_CUT;
    ctrl.value = ircut_state;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("do_sensor_ir_cut failed\n");
    }
}

static void set_manual_sensor_integration_time(int videofd, uint32_t sensor_integration_time_state)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_SENSOR_INTEGRATION_TIME;
    ctrl.value = sensor_integration_time_state;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_manual_sensor_integration_time failed\n");
    }
}

static void set_manual_sensor_analog_gain(int videofd, uint32_t sensor_analog_gain_state)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_SENSOR_ANALOG_GAIN;
    ctrl.value = sensor_analog_gain_state;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_manual_sensor_analog_gain failed\n");
    }
}

static void set_manual_sensor_digital_gain(int videofd, uint32_t sensor_digital_gain_state)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_SENSOR_DIGITAL_GAIN;
    ctrl.value = sensor_digital_gain_state;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_manual_sensor_digital_gain failed\n");
    }
}

static void set_stop_sensor_update(int videofd, uint32_t stop_sensor_update_state)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_STOP_SENSOR_UPDATE;
    ctrl.value = stop_sensor_update_state;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_stop_sensor_update failed\n");
    }
}

static void set_manual_isp_digital_gain(int videofd, uint32_t isp_digital_gain_state)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_ISP_DIGITAL_GAIN;
    ctrl.value = isp_digital_gain_state;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_manual_isp_digital_gain failed\n");
    }
}

static void set_sensor_max_integration_time(int videofd, uint32_t time)
{
    struct v4l2_control ctrl;
    ctrl.id = ISP_V4L2_CID_CUSTOM_SET_MAX_INTEGRATION_TIME;
    ctrl.value = time;
    if (-1 == ioctl (videofd, VIDIOC_S_CTRL, &ctrl)) {
        printf("set_sensor_max_integration_time failed\n");
    }
}


void save_imgae(char *buff, unsigned int size, int flag, int num)
{
    char name[60] = {'\0'};
    int fd = -1;

    if (buff == NULL || size == 0) {
        printf("%s:Error input param\n", __func__);
        return;
    }

    if (num % 20 != 0)
        return;

    sprintf(name, "/media/ca_%d_dump-%d.raw", flag, num);

    fd = open(name, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("%s:Error open file\n", __func__);
        return;
    }
    write(fd, buff, size);
    close(fd);
}

int get_file_size(char *f_name)
{
    int f_size = -1;
    FILE *fp = NULL;

    if (f_name == NULL) {
        printf("Error file name\n");
        return f_size;
    }

    fp = fopen(f_name, "rb");
    if (fp == NULL) {
        printf("Error open file %s\n", f_name);
        return f_size;
    }

    fseek(fp, 0, SEEK_END);

    f_size = ftell(fp);

    fclose(fp);

    printf("%s: size %d\n", f_name, f_size);

    return f_size;
}

int gdc_set_config_param(struct gdc_usr_ctx_s *ctx,
                                    char *f_name, int len)
{
    FILE *fp = NULL;
    int r_size = -1;

    if (f_name == NULL || ctx == NULL || ctx->c_buff == NULL) {
        printf("Error input param\n");
        return r_size;
    }

    fp = fopen(f_name, "rb");
    if (fp == NULL) {
        printf("Error open file %s\n", f_name);
        return -1;
    }

    r_size = fread(ctx->c_buff, len, 1, fp);
    if (r_size <= 0) {
        printf("Failed to read file %s\n", f_name);
    }

    fclose(fp);

    return r_size;
}

int gdc_init_cfg(struct gdc_usr_ctx_s *ctx, struct thread_param *tparm, char *f_name)
{
    struct gdc_settings *gdc_gs = NULL;
    int ret = -1;
    uint32_t format = 0;
    uint32_t i_width = 0;
    uint32_t i_height = 0;
    uint32_t o_width = 0;
    uint32_t o_height = 0;
    uint32_t i_y_stride = 0;
    uint32_t i_c_stride = 0;
    uint32_t o_y_stride = 0;
    uint32_t o_c_stride = 0;
    uint32_t i_len = 0;
    uint32_t o_len = 0;
    uint32_t c_len = 0;

    if (ctx == NULL || tparm == NULL || f_name == NULL) {
        printf("Error invalid input param\n");
        return ret;
    }

    memset(ctx, 0, sizeof(*ctx));

    i_width = tparm->width;
    i_height = tparm->height;
    o_width = tparm->width;
    o_height = tparm->height;

    if (tparm->pixformat == V4L2_PIX_FMT_NV12)
        format = NV12;
    else if (tparm->pixformat == V4L2_PIX_FMT_GREY)
        format = Y_GREY;
    else {
        printf("Error format\n");
        return ret;
    }

    i_y_stride = i_width;
    o_y_stride = o_width;

    if (format == NV12) {
        i_c_stride = i_width;
        o_c_stride = o_width;
    } else if (format == YV12) {
        i_c_stride = i_width / 2;
        o_c_stride = o_width / 2;
    } else if (format == Y_GREY) {
        i_c_stride = 0;
        o_c_stride = 0;
    } else {
        printf("Error unknow format\n");
        return ret;
    }

    gdc_gs = &ctx->gs;

    gdc_gs->base_gdc = 0;

    gdc_gs->gdc_config.input_width = i_width;
    gdc_gs->gdc_config.input_height = i_height;
    gdc_gs->gdc_config.input_y_stride = i_y_stride;
    gdc_gs->gdc_config.input_c_stride = i_c_stride;
    gdc_gs->gdc_config.output_width = o_width;
    gdc_gs->gdc_config.output_height = o_height;
    gdc_gs->gdc_config.output_y_stride = o_y_stride;
    gdc_gs->gdc_config.output_c_stride = o_c_stride;
    gdc_gs->gdc_config.format = format;
    gdc_gs->magic = sizeof(*gdc_gs);

    gdc_create_ctx(ctx);

    c_len = get_file_size(f_name);
    if (c_len <= 0) {
        gdc_destroy_ctx(ctx);
        printf("Error gdc config file size\n");
        return ret;
    }

    ret = gdc_alloc_dma_buffer(ctx, CONFIG_BUFF_TYPE, c_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc cfg buff\n");
        return ret;
    }

    ret = gdc_set_config_param(ctx, f_name, c_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error cfg gdc param buff\n");
        return ret;
    }

    gdc_gs->gdc_config.config_size = c_len / 4;

    i_len = i_y_stride * i_height * 2;
    ret = gdc_alloc_dma_buffer(ctx, INPUT_BUFF_TYPE, i_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc input buff\n");
        return ret;
    }

    o_len = o_y_stride * o_height * 2;
    ret = gdc_alloc_dma_buffer(ctx, OUTPUT_BUFF_TYPE, o_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc input buff\n");
        return ret;
    }

    return ret;
}

int gdc_handle_init_cfg(struct gdc_usr_ctx_s *ctx, struct thread_param *tparm, char *f_name)
{
    struct gdc_settings *gdc_gs = NULL;
    int ret = -1;
    uint32_t format = 0;
    uint32_t i_width = 0;
    uint32_t i_height = 0;
    uint32_t o_width = 0;
    uint32_t o_height = 0;
    uint32_t i_y_stride = 0;
    uint32_t i_c_stride = 0;
    uint32_t o_y_stride = 0;
    uint32_t o_c_stride = 0;
    uint32_t i_len = 0;
    uint32_t o_len = 0;
    uint32_t c_len = 0;

    if (ctx == NULL || tparm == NULL || f_name == NULL) {
        printf("Error invalid input param\n");
        return ret;
    }

    memset(ctx, 0, sizeof(*ctx));

    i_width = tparm->width;
    i_height = tparm->height;
    o_width = tparm->width;
    o_height = tparm->height;

    if (tparm->pixformat == V4L2_PIX_FMT_NV12)
        format = NV12;
    else if (tparm->pixformat == V4L2_PIX_FMT_GREY)
        format = Y_GREY;
    else {
        printf("Error format\n");
        return ret;
    }

    i_y_stride = i_width;
    o_y_stride = o_width;

    if (format == NV12) {
        i_c_stride = i_width;
        o_c_stride = o_width;
    } else if (format == YV12) {
        i_c_stride = i_width / 2;
        o_c_stride = o_width / 2;
    } else if (format == Y_GREY) {
        i_c_stride = 0;
        o_c_stride = 0;
    } else {
        printf("Error unknow format\n");
        return ret;
    }

    gdc_gs = &ctx->gs;

    gdc_gs->base_gdc = 0;

    gdc_gs->gdc_config.input_width = i_width;
    gdc_gs->gdc_config.input_height = i_height;
    gdc_gs->gdc_config.input_y_stride = i_y_stride;
    gdc_gs->gdc_config.input_c_stride = i_c_stride;
    gdc_gs->gdc_config.output_width = o_width;
    gdc_gs->gdc_config.output_height = o_height;
    gdc_gs->gdc_config.output_y_stride = o_y_stride;
    gdc_gs->gdc_config.output_c_stride = o_c_stride;
    gdc_gs->gdc_config.format = format;
    gdc_gs->magic = sizeof(*gdc_gs);

    gdc_create_ctx(ctx);

    c_len = get_file_size(f_name);
    if (c_len <= 0) {
        gdc_destroy_ctx(ctx);
        printf("Error gdc config file size\n");
        return ret;
    }

    ret = gdc_alloc_dma_buffer(ctx, CONFIG_BUFF_TYPE, c_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc cfg buff\n");
        return ret;
    }

    ret = gdc_set_config_param(ctx, f_name, c_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error cfg gdc param buff\n");
        return ret;
    }

    gdc_gs->gdc_config.config_size = c_len / 4;

    o_len = o_y_stride * o_height * 2;
    ret = gdc_alloc_dma_buffer(ctx, OUTPUT_BUFF_TYPE, o_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc input buff\n");
        return ret;
    }

    return ret;
}

static void do_crop(int type, int videofd, int width, int height)
{
    struct v4l2_cropcap c_cap;
    struct v4l2_crop s_crop;
    struct v4l2_crop g_crop;
    int rc = -1;

    memset(&c_cap, 0, sizeof(c_cap));
    memset(&s_crop, 0, sizeof(s_crop));

    /* use this ioctl to get crop capability */
    c_cap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    rc = ioctl (videofd, VIDIOC_CROPCAP, &c_cap);
    if (rc != 0) {
        printf("Error get crop cap\n");
        return;
    }

    if (width == 0 || height == 0) {
        width = c_cap.bounds.width;
        height = c_cap.bounds.height;
    }

    /* default: crop the center of image */
    s_crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    s_crop.c.left = (c_cap.bounds.width - width) / 2;
    s_crop.c.top = (c_cap.bounds.height - height) / 2;
    s_crop.c.width = width;
    s_crop.c.height = height;
    rc = ioctl(videofd, VIDIOC_S_CROP, &s_crop);
    if (rc != 0) {
        printf("Error set crop\n");
        return;
    }

    /* use this ioctl to check crop setting */
    memset(&g_crop, 0, sizeof(g_crop));
    g_crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    rc = ioctl(videofd, VIDIOC_G_CROP, &g_crop);
    if (rc != 0) {
        printf("Error get crop\n");
        return;
    }
}

/**********
 * thread function
 */
void * video_thread(void *arg)
{
    /* v4l2 variables */
    int                         videofd;
    struct v4l2_capability      v4l2_cap;
    struct v4l2_format          v4l2_fmt;
    struct v4l2_requestbuffers  v4l2_rb;
    int                         v4l2_buf_length = 0;
    int                         dma_fd = -1;

    void                        *v4l2_mem[NB_BUFFER*VIDEO_MAX_PLANES];
    int                         v4l2_dma_fd[NB_BUFFER * VIDEO_MAX_PLANES] = {0};
    int                         total_mapped_mem=0;

    /* thread parameters */
    struct thread_param         *tparm = (struct thread_param *)arg;
    pthread_t                   cur_pthread = pthread_self();
    int                         stream_type = -1;

    /* condition & loop flags */
    int                         rc = 0;
    int                         i,j;
    __u32	v4l2_enum_type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    unsigned char *displaybuf = NULL;
    uint64_t display_count = 0;
    int64_t start, end;
    struct gdc_usr_ctx_s gdc_ctx;
    int gdc_ret = -1;
    /**************************************************
     * find thread id
     *************************************************/
    for (i = 0; i < ARM_V4L2_TEST_STREAM_MAX; i++) {
        if(cur_pthread == tid[i]) {
            stream_type = i;
            break;
        }
    }

    for (i = 0; i < NB_BUFFER * VIDEO_MAX_PLANES; i++) {
        v4l2_dma_fd[i] = -1;
    }

    /*
    if (stream_type == ARM_V4L2_TEST_STREAM_RAW && ISP_RAW_PLANES > 1) {
        multiplanar=1;
        v4l2_enum_type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    */

    /**************************************************
     * device open
     *************************************************/
    /* Open the file for reading and writing */
    videofd = open(tparm->devname, O_RDWR);
    if (videofd == -1) {
        printf("Error: cannot open video device\n");
        exit(1);
    }
    INFO("[T#%d] The %s device was opened successfully.\n", stream_type, tparm->devname);

    tparm->videofd = videofd;

    /* check capability */
    memset (&v4l2_cap, 0, sizeof (struct v4l2_capability));
    rc = ioctl (videofd, VIDIOC_QUERYCAP, &v4l2_cap);
    if (rc < 0) {
        printf ("Error: get capability.\n");
        goto fatal;
    }
    INFO("[T#%d] VIDIOC_QUERYCAP: capabilities=0x%x, device_caps:0x%x\n",
        stream_type, v4l2_cap.capabilities, v4l2_cap.device_caps);

    //do_sensor_preset(videofd,1);

    /**************************************************
      * according exposure value control hdr sensor setting
     *************************************************/
    if (stream_type == ARM_V4L2_TEST_STREAM_FR) {
        printf("FR wdr_mode = %d, exp = %d\n", tparm->wdr_mode, tparm->exposure);
        do_sensor_wdr_mode(videofd, tparm->wdr_mode);
        do_sensor_exposure(videofd, tparm->exposure);
    }

     do_sensor_ir_cut(videofd, ir_cut_state);
    /**************************************************
     * format configuration
     *************************************************/
    /* set format */
    memset (&v4l2_fmt, 0, sizeof (struct v4l2_format));
    //v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_fmt.type = v4l2_enum_type;
    v4l2_fmt.fmt.pix_mp.width = tparm->width;
    v4l2_fmt.fmt.pix_mp.height = tparm->height;
    v4l2_fmt.fmt.pix_mp.pixelformat = tparm->pixformat;
    //v4l2_fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    //v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage = v4l2_fmt.fmt.pix_mp.width * v4l2_fmt.fmt.pix_mp.height;
    v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    //v4l2_fmt.fmt.pix_mp.num_planes = 2;

    INFO("[T#%d] VIDIO_S_FMT: type=%d, w=%d, h=%d, fmt=0x%x, field=%d\n",
        stream_type,
        v4l2_fmt.type,
        v4l2_fmt.fmt.pix.width,
        v4l2_fmt.fmt.pix.height,
        v4l2_fmt.fmt.pix.pixelformat,
        v4l2_fmt.fmt.pix.field);

    rc = ioctl (videofd, VIDIOC_S_FMT, &v4l2_fmt);
    if (rc < 0) {
        printf("Error: set format %d.\n", rc);
        goto fatal;
    }
    rc = ioctl (videofd, VIDIOC_G_FMT, &v4l2_fmt);
    if (rc < 0) {
        printf("Error: get format %d.\n", rc);
        goto fatal;
    }
    INFO("[T#%d] VIDIO_G_FMT: type=%d, w=%d, h=%d, fmt=0x%x, field=%d\n",
        stream_type,
        v4l2_fmt.type,
        v4l2_fmt.fmt.pix.width,
        v4l2_fmt.fmt.pix.height,
        v4l2_fmt.fmt.pix.pixelformat,
        v4l2_fmt.fmt.pix.field);

    //real type and planes here
    v4l2_enum_type=v4l2_fmt.type;
    if(v4l2_fmt.type==V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
        INFO("[T#%d] multiplanar support planes=%d\n",
            stream_type, v4l2_fmt.fmt.pix_mp.num_planes);
    }

    if (stream_type == ARM_V4L2_TEST_STREAM_FR ||
            stream_type == ARM_V4L2_TEST_STREAM_DS1) {
        do_crop(stream_type, videofd, tparm->c_width, tparm->c_height);
    }

    if (stream_type == ARM_V4L2_TEST_STREAM_FR && tparm->a_ctrl == 1) {
        do_fr_set_ae_zone_weight(videofd);
        do_fr_set_awb_zone_weight(videofd);
    }

    int fr_bitdepth;
    //get the pixel format
    switch(v4l2_fmt.fmt.pix.pixelformat){
    case V4L2_PIX_FMT_RGB32:
        fr_bitdepth = 24;
        break;
    case ISP_V4L2_PIX_FMT_ARGB2101010:
        fr_bitdepth = 30;
        break;
    default:
        fr_bitdepth = (v4l2_fmt.fmt.pix_mp.plane_fmt[i].bytesperline/v4l2_fmt.fmt.pix_mp.width)*8;
        break;
    }
    /**************************************************
     *set manual exposure
     *************************************************/
     if (stream_type == ARM_V4L2_TEST_STREAM_FR) {
         if ( tparm->wdr_mode == 2 ) {
             manual_exposure_enable = 0;
             set_manual_exposure(videofd, manual_exposure_enable);
         }
         set_manual_exposure(videofd, manual_exposure_enable);
         set_manual_sensor_integration_time(videofd, manual_sensor_integration_time);
         set_manual_sensor_analog_gain(videofd, manual_sensor_analog_gain);
         set_manual_sensor_digital_gain(videofd, manual_sensor_digital_gain);
         set_manual_isp_digital_gain(videofd, manual_isp_digital_gain);
         set_stop_sensor_update(videofd, stop_sensor_update);
         //set_sensor_max_integration_time(videofd, max_int_time);
     }


    /**************************************************
     * buffer preparation
     *************************************************/
    /* request buffers */
    memset (&v4l2_rb, 0, sizeof (struct v4l2_requestbuffers));
    v4l2_rb.count = NB_BUFFER;
    //v4l2_rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_rb.type = v4l2_enum_type;
    v4l2_rb.memory = V4L2_MEMORY_MMAP;
    rc = ioctl (videofd, VIDIOC_REQBUFS, &v4l2_rb);
    if (rc < 0) {
        printf("Error: request buffer.\n");
        goto fatal;
    }

    /* map buffers */
    for (i = 0; i < NB_BUFFER; i++) {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane buf_planes[v4l2_fmt.fmt.pix_mp.num_planes];
        memset (&v4l2_buf, 0, sizeof (struct v4l2_buffer));
        v4l2_buf.index = i;
        //v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.type = v4l2_enum_type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            v4l2_buf.m.planes=buf_planes;
            v4l2_buf.length = v4l2_fmt.fmt.pix_mp.num_planes;
        }
        rc = ioctl (videofd, VIDIOC_QUERYBUF, &v4l2_buf);
        if (rc < 0) {
            printf("Error: query buffer %d.\n", rc);
            goto fatal;
        }

        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE){
            v4l2_buf_length = v4l2_buf.length;
            INFO("[T#%d] length: %u offset: %u\n", stream_type, v4l2_buf.length, v4l2_buf.m.offset);
            v4l2_mem[i] = mmap (0, v4l2_buf.length, PROT_READ, MAP_SHARED,
                videofd, v4l2_buf.m.offset);
            ++total_mapped_mem;
            INFO("[T#%d] Buffer[%d] mapped at address %p total_mapped_mem:%d.\n", stream_type, i, v4l2_mem[i],total_mapped_mem);
        }
        else if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            for (j=0;j<v4l2_fmt.fmt.pix_mp.num_planes;j++) {
                v4l2_buf_length = v4l2_buf.m.planes[j].length;
                dma_fd = do_get_dma_buf_fd(videofd, i, j);
                INFO("[T#%d] plane:%d multiplanar length: %u offset: %u, dma_fd:%d\n",
                    stream_type, j, v4l2_buf.m.planes[j].length, v4l2_buf.m.planes[j].m.mem_offset, dma_fd);
                v4l2_mem[i*v4l2_fmt.fmt.pix_mp.num_planes + j] = mmap (0, v4l2_buf.m.planes[j].length, PROT_READ, MAP_SHARED,
                    videofd, v4l2_buf.m.planes[j].m.mem_offset);
                v4l2_dma_fd[i*v4l2_fmt.fmt.pix_mp.num_planes + j] = dma_fd;
                ++total_mapped_mem;
                INFO("[T#%d] Buffer[%d] mapped at address %p total_mapped_mem:%d.\n", stream_type,i*v4l2_fmt.fmt.pix_mp.num_planes + j, v4l2_mem[i*v4l2_fmt.fmt.pix_mp.num_planes + j],total_mapped_mem);
            }
        }
        if (v4l2_mem[i] == MAP_FAILED) {
            printf("Error: mmap buffers.\n");
            goto fatal;
        }
    }

    /* queue buffers */
    for (i = 0; i < NB_BUFFER; ++i) {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane buf_planes[v4l2_fmt.fmt.pix_mp.num_planes];
        memset (&v4l2_buf, 0, sizeof (struct v4l2_buffer));
        v4l2_buf.index = i;
        //v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.type = v4l2_enum_type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            v4l2_buf.m.planes=buf_planes;
            v4l2_buf.length = v4l2_fmt.fmt.pix_mp.num_planes;
        }
        rc = ioctl (videofd, VIDIOC_QBUF, &v4l2_buf);
        if (rc < 0) {
            printf("Error: queue buffers, rc:%d i:%d\n", rc, i);
            goto fatal;;
        }
    }
    DBG("[T#%d] Queue buf done.\n", stream_type);

    if (stream_type == ARM_V4L2_TEST_STREAM_DS1 && tparm->gdc_ctrl == 1) {
        gdc_ret = gdc_handle_init_cfg(&gdc_ctx, tparm, GDC_CFG_FILE_NAME);
        if (gdc_ret < 0)
            ERR("Failed to init gdc cfg\n");
    }


    /**************************************************
     * V4L2 stream on, get buffers
     *************************************************/
    /* stream on */
    //int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int type = v4l2_enum_type;
    rc = ioctl (videofd, VIDIOC_STREAMON, &type);
    if (rc < 0) {
        printf("Error: streamon.\n");
        goto fatal;
    }
    INFO("[T#%d] Video stream is on.\n", stream_type);

    if (stream_type == fps_test_port) {
        start = GetTimeMsec();
    }

    /* poll variables */
    const int POLL_TIMEOUT = 2000;
    struct pollfd pfds[1];
    int pollret;

    /* dequeue and display */
    do {
        struct v4l2_buffer v4l2_buf;
        //struct v4l2_plane buf_planes[v4l2_fmt.fmt.pix_mp.num_planes];
        frame_t newframe;
        int idx = -1;

        switch (stream_type) {
        case ARM_V4L2_TEST_STREAM_FR:
            memset(displaybuf_fr, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_fr;
            break;
        case ARM_V4L2_TEST_STREAM_META:
            memset(displaybuf_meta, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_meta;
            break;
        case ARM_V4L2_TEST_STREAM_DS1:
            memset(displaybuf_ds1, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_ds1;
            break;
        case ARM_V4L2_TEST_STREAM_DS2:
            memset(displaybuf_ds2, 0, tparm->width * tparm->height * 3);
            displaybuf = displaybuf_ds2;
            break;
        }

        /* wait (poll) for a frame event */
        //printf ("[T#%d] Start polling (exit flag = %d, capture count = %d)\n",
        //    stream_type, v4l2_test_thread_exit, tparm->capture_count);
        pfds[0].fd = videofd;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;
        pollret = poll(pfds, 1, POLL_TIMEOUT);
        if (pollret == 0) {
            INFO ("[T#%d] %d ms poll timeout.\n", stream_type, POLL_TIMEOUT);
            if (v4l2_test_thread_exit)
                break;
            else
               continue;
        } else if (pollret < 0) {
            printf ("Error: poll error\n");
            // break;
        } else {
            // DBG ("[T#%d] Frame ready (pollret = %d, results = %d)\n", stream_type, pollret, pfds[0].revents);
        }

        // dqbuf from video node
        memset (&v4l2_buf, 0, sizeof (struct v4l2_buffer));
        //v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.type = v4l2_enum_type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            //v4l2_buf.m.planes=buf_planes;
            v4l2_buf.m.planes=malloc(v4l2_fmt.fmt.pix_mp.num_planes*sizeof(struct v4l2_plane));
            v4l2_buf.length = v4l2_fmt.fmt.pix_mp.num_planes;
        }
        rc = ioctl (videofd, VIDIOC_DQBUF, &v4l2_buf);
        if (rc < 0) {
            printf ("Error: dequeue buffer.\n");
            if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
                free(v4l2_buf.m.planes);
            break;

        }
        idx = v4l2_buf.index;

        // fill frame_pack and do enqueue_buffer()
        newframe.vfd = videofd;
        newframe.vbuf = v4l2_buf;
        newframe.pixelformat=v4l2_fmt.fmt.pix_mp.pixelformat;
        if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE){
        //if(stream_type != ARM_V4L2_TEST_STREAM_RAW){
            newframe.width[0] = v4l2_fmt.fmt.pix.width;
            newframe.height[0] = v4l2_fmt.fmt.pix.height;
            newframe.bit_depth[0] = sensor_bits;
            newframe.bytes_per_line[0] = (((newframe.width[0] * ((sensor_bits + 7) >> 3)) + 127) >> 7 ) << 7;  // for padding
            newframe.paddr[0] = v4l2_mem[idx];
            DBG("[T#%d] Buffer[%d] single to capture.\n", stream_type, newframe.paddr[0]);
            newframe.num_planes=1;
        }else if(v4l2_buf.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
            newframe.num_planes=v4l2_fmt.fmt.pix_mp.num_planes;
            for (i=0;i<newframe.num_planes;i++) {
                newframe.width[i] = v4l2_fmt.fmt.pix_mp.width;
                newframe.height[i] = v4l2_fmt.fmt.pix_mp.height;
#if ARM_V4L2_TEST_HAS_RAW
                if (stream_type == ARM_V4L2_TEST_STREAM_RAW)
                    newframe.bit_depth[i] = sensor_bits;
                else
#endif
                    newframe.bit_depth[i] = fr_bitdepth;
                /*
                if (stream_type == ARM_V4L2_TEST_STREAM_RAW) {
                    printf("i:%d newframe.bit_depth[i]:%d sensor_bits:%d",i,newframe.bit_depth[i],sensor_bits);
                }
                */
                //newframe.bytes_per_line[i] = (((newframe.width[i] * ((sensor_bits + 7) >> 3)) + 127) >> 7 ) << 7;  // for padding
                newframe.bytes_per_line[i] = v4l2_fmt.fmt.pix_mp.plane_fmt[i].bytesperline;
                newframe.paddr[i] = v4l2_mem[idx*newframe.num_planes+i];
                //DBG("[T#%d] i:%d newframe.paddr:%p to capture idx:%d.\n", stream_type,i, newframe.paddr[i],idx);
                //newframe.paddr[i] = v4l2_mem[idx];
            }
        }

        image_info_t src;
        src.ptr = v4l2_mem[idx];
        src.width = v4l2_fmt.fmt.pix.width;
        src.height = v4l2_fmt.fmt.pix.height;
        src.bpp = 32; // Todo: fixed to ARGB for now
        src.fmt = v4l2_fmt.fmt.pix.pixelformat;

        if (src.fmt == V4L2_PIX_FMT_NV12) {
                if (stream_type == ARM_V4L2_TEST_STREAM_DS1 && tparm->gdc_ctrl == 1) {
                        if (gdc_ret >= 0) {
                                //memcpy(gdc_ctx.i_buff, v4l2_mem[idx * 2], v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage);
                                //memcpy(gdc_ctx.i_buff + v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage, v4l2_mem[idx * 2 + 1],
                                                        //v4l2_fmt.fmt.pix_mp.plane_fmt[1].sizeimage);
                                gdc_ctx.gs.y_base_fd = v4l2_dma_fd[idx * 2];
                                gdc_ctx.gs.uv_base_fd = v4l2_dma_fd[idx * 2 + 1];

                                gdc_handle(&gdc_ctx);
                                save_imgae(gdc_ctx.o_buff, gdc_ctx.o_len, stream_type, tparm->capture_count);
                        }
                } else {
                        memcpy(displaybuf, v4l2_mem[idx * 2], v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage);
                        memcpy(displaybuf + v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage, v4l2_mem[idx * 2 + 1],
                                                v4l2_fmt.fmt.pix_mp.plane_fmt[1].sizeimage);
                }
        } else {
                memcpy(displaybuf, src.ptr, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage);
        }

        rc = ioctl (videofd, VIDIOC_QBUF, &v4l2_buf);
        if (rc < 0) {
            printf ("Error: queue buffer.\n");
            break;
        }

        /***** select save file or display through different stream_type *****/
        if (stream_type == ARM_V4L2_TEST_STREAM_FR) {
            int fb_offset = display_count % fb_buffer_cnt;
            renderImage(tparm->fbp + (src.width * src.height * 3 * fb_offset), tparm->vinfo, tparm->finfo, displaybuf, src.width, src.height, AFD_RENDER_MODE_LEFT_TOP, fb_fd, fb_offset);
            //save_imgae(displaybuf, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage * 2, stream_type, tparm->capture_count);
        } else if (stream_type == ARM_V4L2_TEST_STREAM_META) {
        //do nothing
        } else if (stream_type == ARM_V4L2_TEST_STREAM_DS1) {
         //save_imgae(displaybuf, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage * 2, stream_type, tparm->capture_count);
        } else if (stream_type == ARM_V4L2_TEST_STREAM_DS2) {
        //save_imgae(displaybuf, v4l2_fmt.fmt.pix_mp.plane_fmt[0].sizeimage, stream_type, tparm->capture_count);
        }

        display_count++;
        if ((stream_type == fps_test_port) && (display_count % 100 == 0)) {
            end = GetTimeMsec();
            end = end - start;
            printf("stream port %s fps is : %d\n",
                (stream_type == 0) ? "FR" :
                ((stream_type == 1) ? "Meta":
                ((stream_type == 2) ? "DS1":
                ((stream_type == 3) ? "DS2": "Other"))), (100 * 1000) /end);
            start = GetTimeMsec();
        }

        if (tparm->capture_count > 0)
            tparm->capture_count--;

    } while (tparm->capture_count != 0);

    /**************************************************
     * resource clean-up
     *************************************************/
    /* release all buffers from capture_module */
    free(displaybuf);
    release_capture_module_stream(&g_cap_mod, stream_type);
    /* stream off */
    rc = ioctl (videofd, VIDIOC_STREAMOFF, &type);
    int ir_cut_state = 2;
    do_sensor_ir_cut(videofd, ir_cut_state);
    if (rc < 0) {
        printf("Error: streamoff.\n");
        goto fatal;
    }

    /* unmap buffers */
    //for (i = 0; i < NB_BUFFER; i++) {
    for (i = 0; i < total_mapped_mem; i++) {
        munmap (v4l2_mem[i], v4l2_buf_length);
        if (v4l2_dma_fd[i] >= 0)
            close(v4l2_dma_fd[i]);
    }

fatal:
    if (stream_type == ARM_V4L2_TEST_STREAM_DS1 && tparm->gdc_ctrl == 1)
        gdc_destroy_ctx(&gdc_ctx);

    close(videofd);

    MSG("thread %d terminated ...\n", stream_type);

    return NULL;
}


#if ARM_V4L2_TEST_HAS_RAW
/**********
 * raw capture functions
 */
int prepareRawCapture(struct thread_param * tparam) {
    // Reset capture count
    tparam->capture_count = 10000;

    return pthread_create(&tid[ARM_V4L2_TEST_STREAM_RAW], NULL, &video_thread, tparam);
}

void finishRawCapture(struct thread_param * tparam) {
    tparam->capture_count = 0; //force thread exit
    pthread_join(tid[ARM_V4L2_TEST_STREAM_RAW], NULL);
}
#endif

void usage(char * prog){
    MSG("\nUsage:\t%s { -c command} {-p sensor preset} {-f FR_OUT fmt} {-r resolution} ( {-b fb device} {-v v4l2 device} )\n"
        "\tcommand		: menu command 1-7 (Optional,  default=-1 not enabled)\n\n"
        "\tsensor preset: sensor 0-n-1 sensor modes   (Optional,  default=current preset)\n\n"
        "\tFR_OUT fmt   : output format of FR  (Optional, default 0:ARGB2101010, 1:ARGB8888, 2: YUV NV12)\n"
        "\tresolution   : output resolution        (Optional, default :current preset  0:1080p , 1:12MP, 2:720p)\n"
        "\tfb device    : device name of fb node   (Optional,  default=/dev/fb0)\n"
        "\tv4l2 device  : device name of v4l2 node (Optional,  default=/dev/video0)\n\n"
        , prog);
}

/**********
 * main function
 */

void parse_fmt_res(uint8_t fmt, int res, uint32_t fr_wdr_mode, uint32_t fr_exposure, void *param)
{
    struct thread_param *t_param = NULL;
    uint32_t pixel_format = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t wdr_mode = 0;
    uint32_t exposure = 0;

    if (param == NULL) {
        ERR("Error input param\n");
        return;
    }

    t_param = param;

    switch (fmt) {
    case 0:
        pixel_format = V4L2_PIX_FMT_RGB24;
        break;
    case 1:
        pixel_format = V4L2_PIX_FMT_NV12;
        break;
    case 2:
        pixel_format = V4L2_PIX_FMT_SBGGR16;
        break;
    default:
        ERR("Invalid FR_OUT fmt %d !\n", fmt);
        break;
    }

    switch (res) {
    case 0:
        width = 3840;
        height = 2160;
        break;
    case 1:
        width = 1920;
        height = 1080;
        break;
    case 2:
        width = 1280;
        height = 720;
        break;
    case 3:
        width = 640;
        height = 480;
        break;
    default:
        ERR("Invalid resolution %d !\n", res);
        break;
    }

    switch (fr_wdr_mode) {
    case 0:
        wdr_mode = 0;
        break;
    case 1:
        wdr_mode = 1;
        break;
    case 2:
        wdr_mode = 2;
        break;
    default:
        ERR("Invalid FR wdr mode %d !\n", fr_wdr_mode);
        break;
    }

    switch (fr_exposure) {
    case 1:
        exposure = 1;
        break;
    case 2:
        exposure = 2;
        break;
    case 3:
        exposure = 3;
        break;
    case 4:
        exposure = 4;
        break;
    default:
        ERR("Invalid FR exposure %d !\n", fr_exposure);
        break;
    }

    t_param->pixformat = pixel_format;
    t_param->width = width;
    t_param->height = height;
    t_param->wdr_mode = wdr_mode;
    t_param->exposure = exposure;

    ERR("pixel fmt 0x%x, width %d, height %d, wdr_mode %d, exposure %d",
        pixel_format, width, height, wdr_mode, exposure);
}

int main(int argc, char *argv[])
{
    /* variables for device name / format with default values  */
    int sensor_preset = 0;
    uint8_t fr_out_fmt = 0;
    uint8_t ds1_out_fmt = 0;
    uint8_t ds2_out_fmt = 0;
    int fr_res = -1;
    int ds1_res = -1;
    int ds2_res = -1;
    int fr_num = -1;
    int ds_num = -1;
    uint32_t pixel_format = ISP_V4L2_PIX_FMT_ARGB2101010;
    uint32_t wdr_mode = 0;
    uint32_t exposure = 1;
    char *fbdevname = "/dev/fb0";
    char *v4ldevname = "/dev/video0";
    int rc = 0;
    int i;
    int command = -1;
    int ds_gdc_ctrl = 0;
    uint32_t fr_c_width = 0;
    uint32_t fr_c_height = 0;
    uint32_t fr_a_ctrl = 0;

    uint32_t ds_c_width = 0;
    uint32_t ds_c_height = 0;

    if (argc < 25) {
        printf("v4l test API\n");
        printf("usage:\n");
        printf(" example   : ./v4l2_test  -c 1 -p 0 -F 0 -f 0 -D 0 -R 1 -r 2 -d 2 -N 1000 -n 800 -w 0 -e 1 -b /dev/fb0 -v /dev/video0 \n");
        printf("    c : command           : default 1, 7: set_manual_exposure(just enable manual exposure)\n");
        printf("    p : sensor_preset     : default 0 \n");
        printf("    F : fr_out_fmt        : 0: rgb24  1:nv12 2: raw16 \n");
        printf("    f : ds1_out_fmt       : 0: rgb24  1:nv12 \n");
        printf("    D : ds2_out_fmt       : 0: rgb24  1:nv12 \n");
        printf("    R : fr_out_resolution : 0  : 4k   1: 1080p  2: 720p  3. 480p\n");
        printf("    r : ds1_out_resolution: 0  : 4k   1: 1080p  2: 720p  3. 480p\n");
        printf("    d : ds2_out_resolution: 0  : 4k   1: 1080p  2: 720p  3. 480p\n");
        printf("    w : wdr mode          : 0: linear 1: native 2: fs lin\n");
        printf("    e : exposure value    : min 1, max 4, default is 1\n");
        printf("    b : fbdev            : default: /dev/fb0\n");
        printf("    v : videodev         : default: /dev/video0\n");
        printf("    N : fr frame count \n");
        printf("    n : ds1 & ds2 frame count \n");
        printf("    t : run the port count, default is 1\n");
        printf("    x : fps print port. default: -1, no print. 0:  fr, 1: meta, 2: ds1, 3: ds2\n");
        printf("    g : enable or disable gdc module: 0: disable, 1: enable\n");
        printf("    I : set sensor ir cut state, 0: close, 1: open\n");
        printf("    W : FR crop width\n");
        printf("    H : FR crop height\n");
        printf("    Y : DS1 crop width\n");
        printf("    Z : DS1 crop height\n");
        printf("    a : FR zone weight ctrl\n");
        printf("    M : manual exposure\n");
        printf("    L : integration timet\n");
        printf("    A : sensor analog gain\n");
        printf("    G : sensor digital gain\n");
        printf("    S : isp digital gain\n");
        printf("    K : stop sensor update, 0: enable sensor update, 1: stop sensor update\n");
        return -1;
    }

    int c;

    while(optind < argc){
        if ((c = getopt (argc, argv, "c:p:F:f:D:R:r:d:N:n:w:e:b:v:t:x:g:I:W:H:Y:Z:a:M:L:A:G:S:K:m:")) != -1) {
            switch (c) {
            case 'c':
                command = atoi(optarg);
                break;
            case 'p':
                sensor_preset = atoi(optarg);
                break;
            case 'F':
                fr_out_fmt = atoi(optarg);
                break;
            case 'f':
                ds1_out_fmt = atoi(optarg);
                break;
            case 'D':
                ds2_out_fmt = atoi(optarg);
                break;
            case 'R':
                fr_res = atoi(optarg);
                break;
            case 'r':
                ds1_res = atoi(optarg);
                break;
            case 'd':
                ds2_res = atoi(optarg);
                break;
            case 'N':
                fr_num = atoi(optarg);
                break;
            case 'n':
                ds_num = atoi(optarg);
                break;
            case 'w':
                wdr_mode = atoi(optarg);
                break;
            case 'e':
                exposure = atoi(optarg);
                break;
            case 'b':
                fbdevname = optarg;
                break;
            case 'v':
                v4ldevname = optarg;
                break;
            case 't':
                open_port_cnt = atoi(optarg);
                break;
            case 'x':
                fps_test_port = atoi(optarg);
                break;
            case 'g':
                ds_gdc_ctrl = atoi(optarg);
                break;
            case 'I':
                ir_cut_state = atoi(optarg);
                break;
            case 'W':
                fr_c_width = atoi(optarg);
                break;
            case 'H':
                fr_c_height = atoi(optarg);
                break;
            case 'Y':
                ds_c_width = atoi(optarg);
                break;
            case 'Z':
                ds_c_height = atoi(optarg);
                break;
            case 'a':
                fr_a_ctrl = atoi(optarg);
                break;
            case 'M':
                manual_exposure_enable = atoi(optarg);
            case 'L':
                manual_sensor_integration_time = atoi(optarg);
                break;
            case 'A':
                manual_sensor_analog_gain = atoi(optarg);
                break;
            case 'G':
                manual_sensor_digital_gain = atoi(optarg);
                break;
            case 'S':
                manual_isp_digital_gain = atoi(optarg);
                break;
            case 'K':
                stop_sensor_update = atoi(optarg);
                break;
            case 'm':
                max_int_time = atoi(optarg);
                break;
            case '?':
                usage(argv[0]);
                exit(1);
            }
        }else{
            MSG("Invalid argument %s\n",argv[optind]);
            usage(argv[0]);
            exit(1);
        }
    }

    printf("ds1_out_fmt = %d, ds2_out_fmt = %d, ds1_res = %d, ds2_res = %d\n", ds1_out_fmt, ds2_out_fmt, ds1_res, ds2_res);
    /**************************************************
     * Frame buffer initialize
     *************************************************/
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;

    /* Open the file for reading and writing */
    fb_fd = open(fbdevname, O_RDWR);
    if (fb_fd == -1) {
        printf("Error: cannot open framebuffer device\n");
        exit(1);
    }
    MSG("The %s device was opened successfully.\n", fbdevname);

    /* Get fixed screen information */
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        printf("Error reading fixed information\n");
        exit(2);
    }

    /* Get variable screen information */
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        printf("Error reading variable information\n");
        exit(3);
    }

    MSG("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    /* Figure out the size of the screen in bytes */
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel * fb_buffer_cnt / 8; // 3 buffers

    /* Map the device to memory */
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE,
        MAP_SHARED, fb_fd, 0);
    if (fbp < 0) {
        printf("Error: failed to map framebuffer device to memory\n");
        exit(4);
    }
    MSG("The framebuffer device was mapped to memory successfully.\n");

    if (fr_num <= 0 && ds_num <= 0) {
        printf("can't display both fr and ds\n");
        fr_num = 200;
        ds_num = 200;
    }
    printf("fr_num = %d, ds_num = %d\n", fr_num, ds_num);

    /**************************************************
     * Starting streams
     *************************************************/
    /* init thread parameters */
    v4l2_test_thread_exit = 0;
    v4l2_test_thread_preview = 0;
    v4l2_test_thread_capture = V4L2_TEST_CAPTURE_NONE;
    v4l2_test_thread_capture_count = 0;
    v4l2_test_thread_dump = 0;

    /* init thread parameters */
    struct thread_param tparam[STATIC_STREAM_COUNT] = {
        {
            .devname    = v4ldevname,
            .fbp        = fbp,
            .finfo      = finfo,
            .vinfo      = vinfo,

            .width      = 1920,
            .height     = 1080,
            .pixformat  = pixel_format,
            .wdr_mode	= 0,
            .exposure	= 1,
            .gdc_ctrl = 0,

            .capture_count = fr_num,
            .c_width = fr_c_width,
            .c_height = fr_c_height,
            .a_ctrl = fr_a_ctrl,
        },
#if ARM_V4L2_TEST_HAS_META
        {
            .devname    = v4ldevname,
            .fbp        = fbp,
            .finfo      = finfo,
            .vinfo      = vinfo,

            .width      = 1*1024*1024,
            .height     = 1,
            .pixformat  = ISP_V4L2_PIX_FMT_META,
            .wdr_mode	= 0,
            .exposure	= 1,
            .gdc_ctrl = 0,

            .capture_count = -1,
        },
#endif
        {
            .devname    = v4ldevname,
            .fbp        = fbp,
            .finfo      = finfo,
            .vinfo      = vinfo,

            .width      = 1280,
            .height     = 720,
            .pixformat  = V4L2_PIX_FMT_RGB24,
            .wdr_mode	= 0,
            .exposure	= 1,
            .gdc_ctrl = ds_gdc_ctrl,

            .capture_count = ds_num,
            .c_width = ds_c_width,
            .c_height = ds_c_height,
        },
        {
            .devname    = v4ldevname,
            .fbp        = fbp,
            .finfo      = finfo,
            .vinfo      = vinfo,

            .width      = 1280,
            .height     = 720,
            .pixformat  = V4L2_PIX_FMT_RGB24,
            .wdr_mode	= 0,
            .exposure	= 1,
            .gdc_ctrl = 0,

            .capture_count = ds_num,
        },
    };

    parse_fmt_res(fr_out_fmt, fr_res, wdr_mode, exposure, &tparam[ARM_V4L2_TEST_STREAM_FR]);
    parse_fmt_res(ds1_out_fmt, ds1_res, wdr_mode, exposure, &tparam[ARM_V4L2_TEST_STREAM_DS1]);
    parse_fmt_res(ds2_out_fmt, ds2_res, wdr_mode, exposure, &tparam[ARM_V4L2_TEST_STREAM_DS2]);

    displaybuf_fr = (unsigned char *)malloc(tparam[0].width * tparam[0].height * 3);
    displaybuf_meta = (unsigned char *)malloc(tparam[1].width * tparam[1].height * 3);
    displaybuf_ds1 = (unsigned char *)malloc(tparam[2].width * tparam[2].height * 3);
    displaybuf_ds2 = (unsigned char *)malloc(tparam[3].width * tparam[2].height * 3);

#if ARM_V4L2_TEST_HAS_RAW
    struct thread_param tparam_raw = {
        .devname    = v4ldevname,
        .fbp        = fbp,
        .finfo      = finfo,
        .vinfo      = vinfo,

        .width      = 1920,
        .height     = 1080,
        .pixformat  = V4L2_PIX_FMT_SBGGR16,
        .wdr_mode	= 0,
        .exposure	= 1,
        .capture_count = 1
    };
#endif

    /* init mutex lock for frame_pack_t */
    init_capture_module(&g_cap_mod);

    if(sensor_preset>=0){
        int videofd = open(v4ldevname, O_RDWR);
        if (videofd == -1) {
            printf("Error: cannot open video device\n");
            exit(1);
        }
        MSG("Setting %d sensor preset\n", sensor_preset);
        do_sensor_preset(videofd,sensor_preset);
        close(videofd);
        sleep(3); //let the sensor settle
    }

    /* Launch threads */
    for (i = 0; i < open_port_cnt; i++) {
        rc = pthread_create(&tid[i], NULL, &video_thread, &tparam[i]);
        if (rc != 0)
            ERR("can't create thread :[%s]\n", strerror(rc));
        else
            MSG("Thread %d created successfully\n", i);

        usleep(300000);
    }


    /**************************************************
     * Control interface
     *************************************************/
    enum {
        V4L2_TEST_MENU_PREVIEW_ON_OFF = 1,
        V4L2_TEST_MENU_DO_CAPTURE_LEGACY,
        V4L2_TEST_MENU_DO_CAPTURE_FRM,
        V4L2_TEST_MENU_DO_CAPTURE_DNG,
        V4L2_TEST_MENU_DO_AF_REFOCUS,
        V4L2_TEST_MENU_DUMP_LAST_CAPTURE,
        V4L2_TEST_MENU_EXIT,
        V4L2_TEST_MENU_MAX
    };

    if(command>=V4L2_TEST_MENU_MAX)
        command=-1;

    do {
        int menu;
        if(command>=V4L2_TEST_MENU_PREVIEW_ON_OFF){
            menu=command;
        }else{
            MSG("\nV4L2 test application\n");
            MSG("%d) turn preview %s\n",
                V4L2_TEST_MENU_PREVIEW_ON_OFF, str_on_off[v4l2_test_thread_preview]);
            MSG("%d) Do capture (Legacy)\n", V4L2_TEST_MENU_DO_CAPTURE_LEGACY);
            MSG("%d) Do capture (FRM)\n", V4L2_TEST_MENU_DO_CAPTURE_FRM);
            MSG("%d) Do capture (DNG)\n", V4L2_TEST_MENU_DO_CAPTURE_DNG);
            MSG("%d) Do AF Refocus\n", V4L2_TEST_MENU_DO_AF_REFOCUS);
            MSG("%d) Dump last capture\n", V4L2_TEST_MENU_DUMP_LAST_CAPTURE);
            MSG("%d) Exit\n", V4L2_TEST_MENU_EXIT);
            MSG("Choose menu > ");
            //fflush(stdout);
            scanf("%d", &menu);
        }
        switch(menu) {
        case V4L2_TEST_MENU_PREVIEW_ON_OFF:
            v4l2_test_thread_preview = (v4l2_test_thread_preview + 1) % 2;
            usleep(250000);
            break;
        case V4L2_TEST_MENU_DO_CAPTURE_LEGACY:
        case V4L2_TEST_MENU_DO_CAPTURE_FRM:
#if ARM_V4L2_TEST_HAS_RAW
            /* turn on raw capture stream */
            if (prepareRawCapture(&tparam_raw) != 0) {
                ERR("Error: Can't start raw stream, cancelling capture.\n");
                break;
            }
#endif
            if(menu==V4L2_TEST_MENU_DO_CAPTURE_LEGACY)
                v4l2_test_thread_capture = V4L2_TEST_CAPTURE_LEGACY;
            else
                v4l2_test_thread_capture = V4L2_TEST_CAPTURE_FRM;
            v4l2_test_thread_capture_count = 1;

            do {
                printf("sleeping while v4l2_test_thread_capture:%d\n",v4l2_test_thread_capture);
                usleep(200000);
            } while(v4l2_test_thread_capture != V4L2_TEST_CAPTURE_NONE);

#if ARM_V4L2_TEST_HAS_RAW
            /* wait raw capture stream to be turned off */
            finishRawCapture(&tparam_raw);
#endif

            break;
        case V4L2_TEST_MENU_DO_CAPTURE_DNG:
#if ARM_V4L2_TEST_HAS_RAW
            /* turn on raw capture stream */
            if (prepareRawCapture(&tparam_raw) != 0) {
                ERR("Error: Can't start raw stream, cancelling capture.\n");
                break;
            }
#endif

            v4l2_test_thread_capture = V4L2_TEST_CAPTURE_DNG;
            v4l2_test_thread_capture_count = 1;

            do {
                usleep(200000);
            } while(v4l2_test_thread_capture != V4L2_TEST_CAPTURE_NONE);

#if ARM_V4L2_TEST_HAS_RAW
            /* wait raw capture stream to be turned off */
            finishRawCapture(&tparam_raw);
#endif
            break;
        case V4L2_TEST_MENU_DO_AF_REFOCUS:
            cmd_do_af_refocus = 1;
            break;
        case V4L2_TEST_MENU_DUMP_LAST_CAPTURE:
            MSG("Turning off preview ...\n");
            v4l2_test_thread_preview = 0;
            v4l2_test_thread_dump = STREAM_FLAG_PREVIEW;
            do {
                usleep(200000);
            } while(v4l2_test_thread_dump);
            break;
        case V4L2_TEST_MENU_EXIT:
            v4l2_test_thread_exit = 1;
            break;
        default:
            ERR("invalid input.\n");
            break;
        }

        if(command>=V4L2_TEST_MENU_PREVIEW_ON_OFF)
            v4l2_test_thread_exit = 1;

    } while (!v4l2_test_thread_exit);


    /**************************************************
     * Terminating threads and process
     *************************************************/
    MSG("terminating all threads ...\n");

    for (i = 0; i < open_port_cnt; i++) {
        pthread_join(tid[i], NULL);
    }

    MSG("terminating v4l2 test app, thank you ...\n");

    munmap(fbp, screensize);
    close(fb_fd);
    return 0;
}
