/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>  /* for gettimeofday() */
#include <pthread.h>
#include "ge2d_port.h"
#include "aml_ge2d.h"

/* #define FILE_DATA */
static char SRC1_FILE_NAME[64] = "./fb1.rgb32";
static char SRC2_FILE_NAME[64] = "./fb2.rgb32";
static char DST_FILE_NAME[64] = "./out1.rgb32";

static int SX_SRC1 = 1920;
static int SY_SRC1 = 1080;
static int SX_SRC2 = 1920;
static int SY_SRC2 = 1080;
static int SX_DST = 1920;
static int SY_DST = 1080;

static int src1_rect_x = 0;
static int src1_rect_y = 0;
static int src1_rect_w = 1920;
static int src1_rect_h = 1080;
static int src2_rect_x = 0;
static int src2_rect_y = 0;
static int src2_rect_w = 1920;
static int src2_rect_h = 1080;
static int dst_rect_x = 0;
static int dst_rect_y = 0;
static int dst_rect_w = 1920;
static int dst_rect_h = 1080;
static int rect_color = 0xff0000ff;
static int SRC1_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int SRC2_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int DST_PIXFORMAT = PIXEL_FORMAT_YV12;
static int src1_mem_alloc_type = AML_GE2D_MEM_DMABUF;
static int src2_mem_alloc_type = AML_GE2D_MEM_DMABUF;
static int dst_mem_alloc_type = AML_GE2D_MEM_DMABUF;
static int dst_canvas_alloc = 0;
static int src1_canvas_alloc = 1;
static int src2_canvas_alloc = 1;
static int src1_plane_number = 1;
static int src2_plane_number = 1;
static int dst_plane_number = 1;
static int OP = AML_GE2D_STRETCHBLIT;
static int src1_layer_mode = 0;
static int src2_layer_mode = 0;
static int gb1_alpha = 0xff;
static int gb2_alpha = 0xff;
static int num_process = 1;
static int num_thread = 1;
static int num_process_per_thread = 1;

#define THREADS_MAX_NUM (64)

static inline unsigned long myclock()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static void set_ge2dinfo(aml_ge2d_info_t *pge2dinfo)
{
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->src_info[0].plane_number = src1_plane_number;

    pge2dinfo->src_info[1].canvas_w = SX_SRC2;
    pge2dinfo->src_info[1].canvas_h = SY_SRC2;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;
    pge2dinfo->src_info[1].plane_number = src2_plane_number;

    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.plane_number = dst_plane_number;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->offset = 0;
    pge2dinfo->ge2d_op = OP;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;

    switch (pge2dinfo->ge2d_op)
    {
        case AML_GE2D_FILLRECTANGLE:
            pge2dinfo->src_info[0].memtype = GE2D_CANVAS_TYPE_INVALID;
            pge2dinfo->src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
            pge2dinfo->dst_info.memtype = dst_canvas_alloc == 1 ? GE2D_CANVAS_ALLOC : GE2D_CANVAS_OSD0;
            break;
        case AML_GE2D_BLIT:
        case AML_GE2D_STRETCHBLIT:
            /* if not need alloc, set to GE2D_CANVAS_TYPE_INVALID
                                            * otherwise set to GE2D_CANVAS_ALLOC
                                            */
            pge2dinfo->src_info[0].memtype = src1_canvas_alloc == 1 ? GE2D_CANVAS_ALLOC : GE2D_CANVAS_OSD0;;
            pge2dinfo->src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
            pge2dinfo->dst_info.memtype = dst_canvas_alloc == 1 ? GE2D_CANVAS_ALLOC : GE2D_CANVAS_OSD0;
            break;
         case AML_GE2D_BLEND:
            pge2dinfo->src_info[0].memtype = src1_canvas_alloc == 1 ? GE2D_CANVAS_ALLOC : GE2D_CANVAS_OSD0;;
            pge2dinfo->src_info[1].memtype = src2_canvas_alloc == 1 ? GE2D_CANVAS_ALLOC : GE2D_CANVAS_OSD0;;
            pge2dinfo->dst_info.memtype = dst_canvas_alloc == 1 ? GE2D_CANVAS_ALLOC : GE2D_CANVAS_OSD0;
            break;
         default:
            E_GE2D("not support ge2d op,exit test!\n");
            break;
    }
    /*set to  AML_GE2D_MEM_DMABUF  or AML_GE2D_MEM_ION*/
    pge2dinfo->src_info[0].mem_alloc_type = src1_mem_alloc_type;
    pge2dinfo->src_info[1].mem_alloc_type = src2_mem_alloc_type;
    pge2dinfo->dst_info.mem_alloc_type = dst_mem_alloc_type;
}

static void print_usage(void)
{
    int i;
    printf ("Usage: ge2d_feature_test [options]\n\n");
    printf ("Options:\n\n");
    printf ("  --op <0:fillrect, 1:blend, 2:strechblit, 3:blit>  ge2d operation case.\n");
    printf ("  --size <WxH>                                      define src1/src2/dst size.\n");
    printf ("  --src1_memtype <0: ion, 1: dmabuf>                define memory alloc type.\n");
    printf ("  --src2_memtype <0: ion, 1: dmabuf>                define memory alloc type.\n");
    printf ("  --dst_memtype <0: ion, 1: dmabuf>                 define memory alloc type.\n");
    printf ("  --src1_format  <num>                              define src1 format.\n");
    printf ("  --src2_format <num>                               define src2 format.\n");
    printf ("  --dst_format  <num>                               define dst format.\n");
    printf ("  --src1_size  <WxH>                                define src1 size.\n");
    printf ("  --src2_size <WxH>                                 define src2 size.\n");
    printf ("  --dst_size  <WxH>                                 define dst size.\n");
    printf ("  --src1_file  <name>                               define src1 file.\n");
    printf ("  --src2_file <name>                                define src2 file.\n");
    printf ("  --dst_file  <name>                                define dst file.\n");
    printf ("  --src1_canvas_alloc  <num>                        define whether src1 need alloc mem   0:GE2D_CANVAS_OSD0 1:GE2D_CANVAS_ALLOC.\n");
    printf ("  --src2_canvas_alloc <num>                         defien whether src2 need alloc mem  0:GE2D_CANVAS_OSD0 1:GE2D_CANVAS_ALLOC.\n");
    printf ("  --src1_rect  <x_y_w_h>                            define src1 rect.\n");
    printf ("  --src2_rect <x_y_w_h>                             define src2 rect.\n");
    printf ("  --dst_rect  <x_y_w_h>                             define dst rect.\n");
    printf ("  --bo1 <layer_mode_num>                            define src1_layer_mode.\n");
    printf ("  --bo2 <layer_mode_num>                            define src2_layer_mode.\n");
    printf ("  --gb1 <gb1_alpha>                                 define src1 global alpha.\n");
    printf ("  --gb2 <gb2_alpha>                                 define src2 global alpha.\n");
    printf ("  --strechblit <x0_y0_w_h-x1_y1_w1_h1>              define strechblit info.\n");
    printf ("  --fillrect <color_x_y_w_h>                        define fillrect info, color in rgba format.\n");
    printf ("  --src1_planenumber <num>                          define src1 plane number.\n");
    printf ("  --src2_planenumber <num>                          define src2 plane number.\n");
    printf ("  --dst_planenumber <num>                           define dst plane number.\n");
    printf ("  --n <num>                                         process num times for performance test.\n");
    printf ("  --p <num>                                         multi-thread process, num of threads");
    printf ("  --p_n <num>                                       num of process for every thread.\n");
    printf ("  --help                                            Print usage information.\n");
    printf ("\n");
}

static int parse_command_line(int argc, char *argv[])
{
    int i;
    /* parse command line */
    for (i = 1; i < argc; i++) {
        if (strncmp (argv[i], "--", 2) == 0) {
            if (strcmp (argv[i] + 2, "help") == 0) {
                print_usage();
                return ge2d_fail;
            }
            else if (strcmp (argv[i] + 2, "op") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &OP) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src1_memtype") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src1_mem_alloc_type) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src2_memtype") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src2_mem_alloc_type) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "dst_memtype") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &dst_mem_alloc_type) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "bo1") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src1_layer_mode) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "bo2") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src2_layer_mode) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "gb1") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &gb1_alpha) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "gb2") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &gb2_alpha) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src1_format") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &SRC1_PIXFORMAT) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src2_format") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &SRC2_PIXFORMAT) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "dst_format") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &DST_PIXFORMAT) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "size") == 0 && ++i < argc &&
                sscanf (argv[i], "%dx%d", &SX_SRC1, &SY_SRC1) == 2) {
                SX_SRC2 = SX_DST = SX_SRC1;
                SY_SRC2 = SY_DST = SY_SRC1;
                continue;
            }
            else if (strcmp (argv[i] + 2, "dst_size") == 0 && ++i < argc &&
                sscanf (argv[i], "%dx%d", &SX_DST, &SY_DST) == 2) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src1_size") == 0 && ++i < argc &&
                sscanf (argv[i], "%dx%d", &SX_SRC1, &SY_SRC1) == 2) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src2_size") == 0 && ++i < argc &&
                sscanf (argv[i], "%dx%d", &SX_SRC2, &SY_SRC2) == 2) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src1_rect") == 0 && ++i < argc &&
                sscanf (argv[i], "%d_%d_%d_%d",
                        &src1_rect_x, &src1_rect_y, &src1_rect_w, &src1_rect_h) == 4) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src2_rect") == 0 && ++i < argc &&
                sscanf (argv[i], "%d_%d_%d_%d",
                        &src2_rect_x, &src2_rect_y, &src2_rect_w, &src2_rect_h) == 4) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "dst_rect") == 0 && ++i < argc &&
                sscanf (argv[i], "%d_%d_%d_%d",
                        &dst_rect_x, &dst_rect_y, &dst_rect_w, &dst_rect_h) == 4) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "strechblit") == 0 && ++i < argc &&
                sscanf (argv[i], "%d_%d_%d_%d-%d_%d_%d_%d",
                    &src1_rect_x, &src1_rect_y, &src1_rect_w, &src1_rect_h,
                    &dst_rect_x, &dst_rect_y, &dst_rect_w, &dst_rect_h) == 8) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "fillrect") == 0 && ++i < argc &&
                sscanf (argv[i], "%x_%d_%d_%d_%d",
                    &rect_color, &dst_rect_x, &dst_rect_y, &dst_rect_w, &dst_rect_h) == 5) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src1_file") == 0 && ++i < argc &&
                sscanf (argv[i], "%s", SRC1_FILE_NAME) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src2_file") == 0 && ++i < argc &&
                sscanf (argv[i], "%s", SRC2_FILE_NAME) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "dst_file") == 0 && ++i < argc &&
                sscanf (argv[i], "%s", DST_FILE_NAME) == 1) {
                dst_canvas_alloc = 1;
                continue;
            }
            else if (strcmp (argv[i] + 2, "src1_canvas_alloc") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src1_canvas_alloc) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src2_canvas_alloc") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src2_canvas_alloc) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src1_planenumber") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src1_plane_number) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "src2_planenumber") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &src2_plane_number) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "dst_planenumber") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &dst_plane_number) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "n") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &num_process) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "p") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &num_thread) == 1) {
                continue;
            }
            else if (strcmp (argv[i] + 2, "p_n") == 0 && ++i < argc &&
                sscanf (argv[i], "%d", &num_process_per_thread) == 1) {
                continue;
            }
        }
    }
    return ge2d_success;
}

static int check_plane_number(int plane_number, int format)
{
    int ret = -1;
    printf("plane_number=%d,format=%d\n", plane_number, format);
    if (plane_number == 1) {
        ret = 0;
    } else if (plane_number == 2) {
        if ((format == PIXEL_FORMAT_YCrCb_420_SP) ||
            (format == PIXEL_FORMAT_YCbCr_420_SP_NV12) ||
            (format == PIXEL_FORMAT_YCbCr_422_SP))
            ret = 0;
    } else if (plane_number == 3) {
        if (format == PIXEL_FORMAT_YV12)
            ret = 0;
    }
    return ret;
}

static int aml_read_file_src1(aml_ge2d_t *amlge2d, const char* url)
{
    int fd = -1;
    int length = 0;
    int read_num = 0;
    int i;

    fd = open(url,O_RDONLY );
    if (fd < 0) {
        E_GE2D("read source file:%s open error\n",url);
        return ge2d_fail;
    }
    for (i = 0; i < amlge2d->ge2dinfo.src_info[0].plane_number; i++) {
        if (amlge2d->src_size[i] == 0) {
            E_GE2D("src_size[i]=%d\n",amlge2d->src_size[i]);
            close(fd);
            return 0;
        }
        amlge2d->src_data[i] = (char*)malloc(amlge2d->src_size[i]);
        if (!amlge2d->src_data[i]) {
            E_GE2D("malloc for src_data failed\n");
            close(fd);
            return ge2d_fail;
        }

        read_num = read(fd,amlge2d->src_data[i],amlge2d->src_size[i]);
        if (read_num <= 0) {
            E_GE2D("read file read_num=%d error\n",read_num);
            close(fd);
            return ge2d_fail;
        }

        memcpy(amlge2d->ge2dinfo.src_info[0].vaddr[i], amlge2d->src_data[i], amlge2d->src_size[i]);
    }
    close(fd);
    return ge2d_success;
}

static int aml_read_file_src2(aml_ge2d_t *amlge2d, const char* url)
{
    int fd = -1;
    int length = 0;
    int read_num = 0;
    int i;

    fd = open(url,O_RDONLY );
    if (fd < 0) {
        E_GE2D("read source file:%s open error\n",url);
        return ge2d_fail;
    }
    for (i = 0; i < amlge2d->ge2dinfo.src_info[1].plane_number; i++) {
        if (amlge2d->src2_size[i] == 0) {
            E_GE2D("src2_size[i]=%d\n",amlge2d->src2_size[i]);
            close(fd);
            return 0;
        }
        amlge2d->src2_data[i] = (char*)malloc(amlge2d->src2_size[i]);
        if (!amlge2d->src2_data[i]) {
            E_GE2D("malloc for src_data failed\n");
            close(fd);
            return ge2d_fail;
        }

        read_num = read(fd,amlge2d->src2_data[i],amlge2d->src2_size[i]);
        if (read_num <= 0) {
            E_GE2D("read file read_num=%d error\n",read_num);
            close(fd);
            return ge2d_fail;
        }

        memcpy(amlge2d->ge2dinfo.src_info[1].vaddr[i], amlge2d->src2_data[i], amlge2d->src2_size[i]);
    }
    close(fd);
    return ge2d_success;
}

static int aml_write_file(aml_ge2d_t *amlge2d, const char* url)
{
    int fd = -1;
    int length = 0;
    int write_num = 0;
    unsigned int *value;
    int i;

    if ((GE2D_CANVAS_OSD0 == amlge2d->ge2dinfo.dst_info.memtype)
        || (GE2D_CANVAS_OSD1 == amlge2d->ge2dinfo.dst_info.memtype))
        return 0;

    fd = open(url,O_RDWR | O_CREAT,0660);
    if (fd < 0) {
        E_GE2D("write file:%s open error\n",url);
        return ge2d_fail;
    }
    for (i = 0; i < amlge2d->ge2dinfo.dst_info.plane_number; i++) {
        if (amlge2d->dst_size[i] == 0) {
            E_GE2D("dst_size[%d]=%d\n",i, amlge2d->dst_size[i]);
            close(fd);
            return 0;
        }
        amlge2d->dst_data[i] = (char*)malloc(amlge2d->dst_size[i]);
        if (!amlge2d->dst_data[i]) {
            E_GE2D("malloc for dst_data failed\n");
            close(fd);
            return ge2d_fail;
        }
        memcpy(amlge2d->dst_data[i], amlge2d->ge2dinfo.dst_info.vaddr[i], amlge2d->dst_size[i]);
        printf("pixel: 0x%2x, 0x%2x,0x%2x,0x%2x, 0x%2x,0x%2x,0x%2x,0x%2x\n",
            amlge2d->dst_data[i][0],
            amlge2d->dst_data[i][1],
            amlge2d->dst_data[i][2],
            amlge2d->dst_data[i][3],
            amlge2d->dst_data[i][4],
            amlge2d->dst_data[i][5],
            amlge2d->dst_data[i][6],
            amlge2d->dst_data[i][7]);
        write_num = write(fd,amlge2d->dst_data[i],amlge2d->dst_size[i]);
        if (write_num <= 0) {
            E_GE2D("write file write_num=%d error\n",write_num);
            close(fd);
        }
    }
    close(fd);
    return ge2d_success;
}

static int do_fill_rectangle(aml_ge2d_t *amlge2d)
{
    int ret = -1;
    char code;
    int i;
    unsigned long stime;
    aml_ge2d_info_t *pge2dinfo = &amlge2d->ge2dinfo;

    printf("do_fill_rectangle test case:\n");

    pge2dinfo->color = rect_color;
    pge2dinfo->dst_info.rect.x = dst_rect_x;
    pge2dinfo->dst_info.rect.y = dst_rect_x;
    pge2dinfo->dst_info.rect.w = dst_rect_w;
    pge2dinfo->dst_info.rect.h = dst_rect_h;
    stime = myclock();
    for (i = 0; i < num_process; i++) {
       ret = aml_ge2d_process(pge2dinfo);
       if (ret < 0) {
           printf("ge2d process failed, %s (%d)\n", __func__, __LINE__);
           return ret;
       }
    }
    printf("time=%ld ms\n", myclock() - stime);
    #if 0
    sleep(5);

    printf("please enter any key do rotation test[90]\n");
    code = getc(stdin);
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_180;
    ret = aml_ge2d_process(pge2dinfo);
    #endif
    if (ret < 0)
        printf("%s failed\n", __func__);
    return ret;
}


static int do_blend(aml_ge2d_t *amlge2d)
{
    int ret = -1, i;
    char code = 0;
    int shared_fd_bakup;
    unsigned long offset_bakup = 0;
    unsigned long stime;
    aml_ge2d_info_t *pge2dinfo = &amlge2d->ge2dinfo;

    printf("do_blend test case:\n");

    if (pge2dinfo->cap_attr == 0x1) {
        /* do blend src1 blend src2(dst) to dst */
        printf("one step blend\n");
        ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
        if (ret < 0)
           return  ge2d_fail;
        ret = aml_read_file_src2(amlge2d, SRC2_FILE_NAME);
        if (ret < 0)
           return ge2d_fail;

        pge2dinfo->src_info[0].rect.x = src1_rect_x;
        pge2dinfo->src_info[0].rect.y = src1_rect_y;
        pge2dinfo->src_info[0].rect.w = src1_rect_w;
        pge2dinfo->src_info[0].rect.h = src1_rect_h;
        pge2dinfo->src_info[0].fill_color_en = 0;

        pge2dinfo->src_info[1].rect.x = src2_rect_x;
        pge2dinfo->src_info[1].rect.y = src2_rect_y;
        pge2dinfo->src_info[1].rect.w = src2_rect_w;
        pge2dinfo->src_info[1].rect.h = src2_rect_h;
        pge2dinfo->src_info[1].fill_color_en = 0;
        if ((src2_layer_mode == LAYER_MODE_NON) && (src1_layer_mode == LAYER_MODE_PREMULTIPLIED))
           pge2dinfo->src_info[0].format = PIXEL_FORMAT_RGBX_8888;

        pge2dinfo->dst_info.rect.x = dst_rect_x;
        pge2dinfo->dst_info.rect.y = dst_rect_y;
        pge2dinfo->dst_info.rect.w = dst_rect_w;
        pge2dinfo->dst_info.rect.h = dst_rect_h;
        pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

        pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
        pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
        pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
        pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
        stime = myclock();
        for (i = 0; i < num_process; i++) {
           ret = aml_ge2d_process(pge2dinfo);
           if (ret < 0) {
               printf("ge2d process failed, %s (%d)\n", __func__, __LINE__);
               return ret;
           }
        }
        printf("time=%ld ms\n", myclock() - stime);
    } else {
        if (((gb1_alpha != 0xff)
        && (gb2_alpha != 0xff))){
        printf("two steps blend,two plane alpha\n");

        if (src2_layer_mode == LAYER_MODE_COVERAGE) {
            printf("two steps blend,src2 LAYER_MODE_COVERAGE\n");
            ret = aml_read_file_src1(amlge2d, SRC2_FILE_NAME);
            if (ret < 0)
               return  ge2d_fail;

            /* both plane alpha, do 2 steps */
            /* step 1: strechbilt */
            pge2dinfo->ge2d_op = AML_GE2D_BLEND;
            /* src2 do strechbilt to dst */
            pge2dinfo->src_info[0].canvas_w = SX_SRC1;
            pge2dinfo->src_info[0].canvas_h = SY_SRC1;
            pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
            pge2dinfo->src_info[0].rect.x = 0;
            pge2dinfo->src_info[0].rect.y = 0;
            pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->src_info[0].fill_color_en = 0;

            pge2dinfo->src_info[1].canvas_w = SX_DST;
            pge2dinfo->src_info[1].canvas_h = SY_DST;
            pge2dinfo->src_info[1].format = DST_PIXFORMAT;
            pge2dinfo->src_info[1].rect.x = 0;
            pge2dinfo->src_info[1].rect.y = 0;
            pge2dinfo->src_info[1].rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->src_info[1].rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->src_info[1].fill_color_en = 1;
            pge2dinfo->src_info[1].def_color = 0x00;

            pge2dinfo->dst_info.canvas_w = SX_DST;
            pge2dinfo->dst_info.canvas_h = SY_DST;
            pge2dinfo->dst_info.format = DST_PIXFORMAT;
            pge2dinfo->dst_info.rect.x = 0;
            pge2dinfo->dst_info.rect.y = 0;
            pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

            pge2dinfo->src_info[0].layer_mode = LAYER_MODE_COVERAGE;
            pge2dinfo->src_info[1].layer_mode = LAYER_MODE_COVERAGE;
            pge2dinfo->src_info[0].plane_alpha = gb2_alpha;
            pge2dinfo->src_info[1].plane_alpha = 0xff;
            ret = aml_ge2d_process(pge2dinfo);
            ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
            if (ret < 0)
               return  ge2d_fail;

            /* step2: blend src1 blend src2(dst) to dst */
            pge2dinfo->ge2d_op = AML_GE2D_BLEND;

            pge2dinfo->src_info[0].canvas_w = SX_SRC1;
            pge2dinfo->src_info[0].canvas_h = SY_SRC1;
            pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
            pge2dinfo->src_info[0].rect.x = 0;
            pge2dinfo->src_info[0].rect.y = 0;
            pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->src_info[0].fill_color_en = 0;

            pge2dinfo->src_info[1].canvas_w = pge2dinfo->dst_info.canvas_w;
            pge2dinfo->src_info[1].canvas_h = pge2dinfo->dst_info.canvas_h;
            pge2dinfo->src_info[1].format = pge2dinfo->dst_info.format;
            pge2dinfo->src_info[1].rect.x = pge2dinfo->dst_info.rect.x;
            pge2dinfo->src_info[1].rect.y = pge2dinfo->dst_info.rect.y;
            pge2dinfo->src_info[1].rect.w = pge2dinfo->dst_info.rect.w;
            pge2dinfo->src_info[1].rect.h = pge2dinfo->dst_info.rect.h;
            pge2dinfo->src_info[1].shared_fd[0] = pge2dinfo->dst_info.shared_fd[0];
            pge2dinfo->src_info[1].offset[0] = pge2dinfo->dst_info.offset[0];
            pge2dinfo->src_info[1].fill_color_en = 0;

            pge2dinfo->dst_info.canvas_w = SX_DST;
            pge2dinfo->dst_info.canvas_h = SY_DST;
            pge2dinfo->dst_info.format = DST_PIXFORMAT;
            pge2dinfo->dst_info.rect.x = 0;
            pge2dinfo->dst_info.rect.y = 0;
            pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

            pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
            pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
            pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
            pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
            ret = aml_ge2d_process(pge2dinfo);
        } else {
            ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
            if (ret < 0)
               return  ge2d_fail;
            ret = aml_read_file_src2(amlge2d, SRC2_FILE_NAME);
            if (ret < 0)
               return ge2d_fail;
            printf("two step: strechbilt+blend\n");
            /* both plane alpha, do 2 steps */
            /* step 1: strechbilt */
            pge2dinfo->ge2d_op = AML_GE2D_STRETCHBLIT;
            /* src2 do strechbilt to dst */
            pge2dinfo->src_info[0].canvas_w = SX_SRC2;
            pge2dinfo->src_info[0].canvas_h = SY_SRC2;
            pge2dinfo->src_info[0].format = SRC2_PIXFORMAT;
            pge2dinfo->src_info[0].rect.x = 0;
            pge2dinfo->src_info[0].rect.y = 0;
            pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;

            shared_fd_bakup = pge2dinfo->src_info[0].shared_fd[0];
            offset_bakup = pge2dinfo->src_info[0].offset[0];
            pge2dinfo->src_info[0].shared_fd[0] = pge2dinfo->src_info[1].shared_fd[0];
            pge2dinfo->src_info[0].offset[0] = pge2dinfo->src_info[1].offset[0];

            pge2dinfo->src_info[0].layer_mode = src2_layer_mode;
            pge2dinfo->src_info[0].plane_alpha = gb2_alpha;
            pge2dinfo->dst_info.canvas_w = SX_DST;
            pge2dinfo->dst_info.canvas_h = SY_DST;
            pge2dinfo->dst_info.format = DST_PIXFORMAT;
            pge2dinfo->dst_info.rect.x = 0;
            pge2dinfo->dst_info.rect.y = 0;
            pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
            ret = aml_ge2d_process(pge2dinfo);

            /* step2: blend src1 blend src2(dst) to dst */
            pge2dinfo->ge2d_op = AML_GE2D_BLEND;

            pge2dinfo->src_info[0].canvas_w = SX_SRC1;
            pge2dinfo->src_info[0].canvas_h = SY_SRC1;
            pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
            pge2dinfo->src_info[0].rect.x = 0;
            pge2dinfo->src_info[0].rect.y = 0;
            pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->src_info[0].shared_fd[0] = shared_fd_bakup;
            pge2dinfo->src_info[0].offset[0] = offset_bakup;
            pge2dinfo->src_info[0].fill_color_en = 0;

            pge2dinfo->src_info[1].canvas_w = pge2dinfo->dst_info.canvas_w;
            pge2dinfo->src_info[1].canvas_h = pge2dinfo->dst_info.canvas_h;
            pge2dinfo->src_info[1].format = pge2dinfo->dst_info.format;
            pge2dinfo->src_info[1].rect.x = pge2dinfo->dst_info.rect.x;
            pge2dinfo->src_info[1].rect.y = pge2dinfo->dst_info.rect.y;
            pge2dinfo->src_info[1].rect.w = pge2dinfo->dst_info.rect.w;
            pge2dinfo->src_info[1].rect.h = pge2dinfo->dst_info.rect.h;
            pge2dinfo->src_info[1].shared_fd[0] = pge2dinfo->dst_info.shared_fd[0];
            pge2dinfo->src_info[1].offset[0] = pge2dinfo->dst_info.offset[0];
            pge2dinfo->src_info[1].fill_color_en = 0;

            pge2dinfo->dst_info.canvas_w = SX_DST;
            pge2dinfo->dst_info.canvas_h = SY_DST;
            pge2dinfo->dst_info.format = DST_PIXFORMAT;
            pge2dinfo->dst_info.rect.x = 0;
            pge2dinfo->dst_info.rect.y = 0;
            pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
            pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
            pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

            pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
            pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
            pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
            pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
            ret = aml_ge2d_process(pge2dinfo);
        }
    } else  if (src2_layer_mode == LAYER_MODE_COVERAGE){
        printf("two steps blend,src2 LAYER_MODE_COVERAGE: two blend\n");
        ret = aml_read_file_src1(amlge2d, SRC2_FILE_NAME);
        if (ret < 0)
            return ge2d_fail;
        /* both plane alpha, do 2 steps */
        /* step 1: strechbilt */
        pge2dinfo->ge2d_op = AML_GE2D_BLEND;
        /* src2 do strechbilt to dst */
        pge2dinfo->src_info[0].canvas_w = SX_SRC1;
        pge2dinfo->src_info[0].canvas_h = SY_SRC1;
        pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
        pge2dinfo->src_info[0].rect.x = 0;
        pge2dinfo->src_info[0].rect.y = 0;
        pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->src_info[0].fill_color_en = 0;

        pge2dinfo->src_info[1].canvas_w = SX_DST;
        pge2dinfo->src_info[1].canvas_h = SY_DST;
        pge2dinfo->src_info[1].format = DST_PIXFORMAT;
        pge2dinfo->src_info[1].rect.x = 0;
        pge2dinfo->src_info[1].rect.y = 0;
        pge2dinfo->src_info[1].rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->src_info[1].rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->src_info[1].fill_color_en = 1;
        pge2dinfo->src_info[1].def_color = 0x00;

        pge2dinfo->dst_info.canvas_w = SX_DST;
        pge2dinfo->dst_info.canvas_h = SY_DST;
        pge2dinfo->dst_info.format = DST_PIXFORMAT;
        pge2dinfo->dst_info.rect.x = 0;
        pge2dinfo->dst_info.rect.y = 0;
        pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

        pge2dinfo->src_info[0].layer_mode = LAYER_MODE_COVERAGE;
        pge2dinfo->src_info[1].layer_mode = LAYER_MODE_COVERAGE;
        pge2dinfo->src_info[0].plane_alpha = gb2_alpha;
        pge2dinfo->src_info[1].plane_alpha = 0xff;
        ret = aml_ge2d_process(pge2dinfo);
        ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
        if (ret < 0)
            return ge2d_fail;
        /* step2: blend src1 blend src2(dst) to dst */
        pge2dinfo->ge2d_op = AML_GE2D_BLEND;

        pge2dinfo->src_info[0].canvas_w = SX_SRC1;
        pge2dinfo->src_info[0].canvas_h = SY_SRC1;
        pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
        pge2dinfo->src_info[0].rect.x = 0;
        pge2dinfo->src_info[0].rect.y = 0;
        pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->src_info[0].fill_color_en = 0;

        pge2dinfo->src_info[1].canvas_w = pge2dinfo->dst_info.canvas_w;
        pge2dinfo->src_info[1].canvas_h = pge2dinfo->dst_info.canvas_h;
        pge2dinfo->src_info[1].format = pge2dinfo->dst_info.format;
        pge2dinfo->src_info[1].rect.x = pge2dinfo->dst_info.rect.x;
        pge2dinfo->src_info[1].rect.y = pge2dinfo->dst_info.rect.y;
        pge2dinfo->src_info[1].rect.w = pge2dinfo->dst_info.rect.w;
        pge2dinfo->src_info[1].rect.h = pge2dinfo->dst_info.rect.h;
        pge2dinfo->src_info[1].shared_fd[0] = pge2dinfo->dst_info.shared_fd[0];
        pge2dinfo->src_info[1].offset[0] = pge2dinfo->dst_info.offset[0];
        pge2dinfo->src_info[1].fill_color_en = 0;

        pge2dinfo->dst_info.canvas_w = SX_DST;
        pge2dinfo->dst_info.canvas_h = SY_DST;
        pge2dinfo->dst_info.format = DST_PIXFORMAT;
        pge2dinfo->dst_info.rect.x = 0;
        pge2dinfo->dst_info.rect.y = 0;
        pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

        pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
        pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
        pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
        pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
        ret = aml_ge2d_process(pge2dinfo);
    } else if ((src2_layer_mode == LAYER_MODE_NON)
        && (src1_layer_mode != LAYER_MODE_PREMULTIPLIED)){
        printf("two steps blend,src2 LAYER_MODE_NON:strechbilt+blend\n");
        ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
        if (ret < 0)
            return ge2d_fail;
        ret = aml_read_file_src2(amlge2d, SRC2_FILE_NAME);
        if (ret < 0)
            return ge2d_fail;
        /* both plane alpha, do 2 steps */
        /* step 1: strechbilt */
        pge2dinfo->ge2d_op = AML_GE2D_STRETCHBLIT;
        /* src2 do strechbilt to dst */
        pge2dinfo->src_info[0].canvas_w = SX_SRC2;
        pge2dinfo->src_info[0].canvas_h = SY_SRC2;
        pge2dinfo->src_info[0].format = SRC2_PIXFORMAT;
        pge2dinfo->src_info[0].rect.x = 0;
        pge2dinfo->src_info[0].rect.y = 0;
        pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;

        shared_fd_bakup = pge2dinfo->src_info[0].shared_fd[0];
        offset_bakup = pge2dinfo->src_info[0].offset[0];
        pge2dinfo->src_info[0].shared_fd[0] = pge2dinfo->src_info[1].shared_fd[0];
        pge2dinfo->src_info[0].offset[0] = pge2dinfo->src_info[1].offset[0];
        pge2dinfo->src_info[0].layer_mode = src2_layer_mode;
        pge2dinfo->src_info[0].plane_alpha = 0xff;
        pge2dinfo->src_info[0].format = PIXEL_FORMAT_RGBX_8888;

        pge2dinfo->dst_info.canvas_w = SX_DST;
        pge2dinfo->dst_info.canvas_h = SY_DST;
        pge2dinfo->dst_info.format = DST_PIXFORMAT;
        pge2dinfo->dst_info.rect.x = 0;
        pge2dinfo->dst_info.rect.y = 0;
        pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
        ret = aml_ge2d_process(pge2dinfo);

        /* step2: blend src1 blend src2(dst) to dst */
        pge2dinfo->ge2d_op = AML_GE2D_BLEND;

        pge2dinfo->src_info[0].canvas_w = SX_SRC1;
        pge2dinfo->src_info[0].canvas_h = SY_SRC1;
        pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
        pge2dinfo->src_info[0].rect.x = 0;
        pge2dinfo->src_info[0].rect.y = 0;
        pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->src_info[0].shared_fd[0] = shared_fd_bakup;
        pge2dinfo->src_info[0].offset[0] = offset_bakup;
        pge2dinfo->src_info[0].fill_color_en = 0;

        pge2dinfo->src_info[1].canvas_w = pge2dinfo->dst_info.canvas_w;
        pge2dinfo->src_info[1].canvas_h = pge2dinfo->dst_info.canvas_h;
        pge2dinfo->src_info[1].format = pge2dinfo->dst_info.format;
        pge2dinfo->src_info[1].rect.x = pge2dinfo->dst_info.rect.x;
        pge2dinfo->src_info[1].rect.y = pge2dinfo->dst_info.rect.y;
        pge2dinfo->src_info[1].rect.w = pge2dinfo->dst_info.rect.w;
        pge2dinfo->src_info[1].rect.h = pge2dinfo->dst_info.rect.h;
        pge2dinfo->src_info[1].shared_fd[0] = pge2dinfo->dst_info.shared_fd[0];
        pge2dinfo->src_info[1].offset[0] = pge2dinfo->dst_info.offset[0];
        pge2dinfo->src_info[1].fill_color_en = 0;

        pge2dinfo->dst_info.canvas_w = SX_DST;
        pge2dinfo->dst_info.canvas_h = SY_DST;
        pge2dinfo->dst_info.format = DST_PIXFORMAT;
        pge2dinfo->dst_info.rect.x = 0;
        pge2dinfo->dst_info.rect.y = 0;
        pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
        printf("two steps blend,src1_layer_mode=%d,src2_layer_mode=%d\n",src1_layer_mode,src2_layer_mode);

        pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
        pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
        pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
        pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
        ret = aml_ge2d_process(pge2dinfo);
    }  else {
        /* do blend src1 blend src2(dst) to dst */
        printf("one step blend\n");
        ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
        if (ret < 0)
           return  ge2d_fail;
        ret = aml_read_file_src2(amlge2d, SRC2_FILE_NAME);
        if (ret < 0)
           return ge2d_fail;
        pge2dinfo->src_info[0].canvas_w = SX_SRC1;
        pge2dinfo->src_info[0].canvas_h = SY_SRC1;
        pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
        pge2dinfo->src_info[0].rect.x = 0;
        pge2dinfo->src_info[0].rect.y = 0;
        pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->src_info[0].fill_color_en = 0;

        pge2dinfo->src_info[1].canvas_w = SX_SRC2;
        pge2dinfo->src_info[1].canvas_h = SY_SRC2;
        pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;
        pge2dinfo->src_info[1].rect.x = 0;
        pge2dinfo->src_info[1].rect.y = 0;
        pge2dinfo->src_info[1].rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->src_info[1].rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->src_info[1].fill_color_en = 0;
        if (src2_layer_mode == LAYER_MODE_NON)
            pge2dinfo->src_info[0].format = PIXEL_FORMAT_RGBX_8888;
        pge2dinfo->dst_info.canvas_w = SX_DST;
        pge2dinfo->dst_info.canvas_h = SY_DST;
        pge2dinfo->dst_info.format = DST_PIXFORMAT;
        pge2dinfo->dst_info.rect.x = 0;
        pge2dinfo->dst_info.rect.y = 0;
        pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w;
        pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h;
        pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;

        pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
        pge2dinfo->src_info[1].layer_mode = src2_layer_mode;
        pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
        pge2dinfo->src_info[1].plane_alpha = gb2_alpha;
        ret = aml_ge2d_process(pge2dinfo);
    }
    }
    if (ret < 0)
        printf("%s failed\n", __func__);
    return ret;
}


static int do_strechblit(aml_ge2d_t *amlge2d)
{
    int ret = -1;
    char code = 0;
    int i;
    unsigned long stime;
    aml_ge2d_info_t *pge2dinfo = &amlge2d->ge2dinfo;

    printf("do_strechblit test case:\n");
    ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
    if (ret < 0)
       return ge2d_fail;

    pge2dinfo->src_info[0].rect.x = src1_rect_x;
    pge2dinfo->src_info[0].rect.y = src1_rect_y;
    pge2dinfo->src_info[0].rect.w = src1_rect_w;
    pge2dinfo->src_info[0].rect.h = src1_rect_h;
    pge2dinfo->dst_info.rect.x = dst_rect_x;
    pge2dinfo->dst_info.rect.y = dst_rect_y;
    pge2dinfo->dst_info.rect.w = dst_rect_w;
    pge2dinfo->dst_info.rect.h = dst_rect_h;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;

    stime = myclock();
    for (i = 0; i < num_process; i++) {
       ret = aml_ge2d_process(pge2dinfo);
       if (ret < 0) {
           printf("ge2d process failed, %s (%d)\n", __func__, __LINE__);
           return ret;
       }
    }
    printf("time=%ld ms\n", myclock() - stime);
    #if 0
    sleep(5);

    printf("please enter any key do rotation test[90]\n");
    code = getc(stdin);
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_90;
    ret = aml_ge2d_process(pge2dinfo);
    #endif
    if (ret < 0)
        printf("%s failed\n", __func__);
    return ret;

}

static int do_blit(aml_ge2d_t *amlge2d)
{
    int ret = -1;
    char code = 0;
    int i;
    unsigned long stime;
    aml_ge2d_info_t *pge2dinfo = &amlge2d->ge2dinfo;

    printf("do_blit test case:\n");
    ret = aml_read_file_src1(amlge2d, SRC1_FILE_NAME);
    if (ret < 0)
       return ge2d_fail;

    pge2dinfo->src_info[0].rect.x = src1_rect_x;
    pge2dinfo->src_info[0].rect.y = src1_rect_y;
    pge2dinfo->src_info[0].rect.w = src1_rect_w;
    pge2dinfo->src_info[0].rect.h = src1_rect_h;
    pge2dinfo->dst_info.rect.x = dst_rect_x;
    pge2dinfo->dst_info.rect.y = dst_rect_y;

    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;

    stime = myclock();
    for (i = 0; i < num_process; i++) {
       ret = aml_ge2d_process(pge2dinfo);
       if (ret < 0) {
           printf("ge2d process failed, %s (%d)\n", __func__, __LINE__);
           return ret;
       }
    }
    printf("time=%ld ms\n", myclock() - stime);
    #if 0
    sleep(5);

    printf("please enter any key do rotation test[90]\n");
    code = getc(stdin);
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_90;
    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;

    ret = aml_ge2d_process(pge2dinfo);
    #endif
    if (ret < 0)
        printf("%s failed\n", __func__);
    return ret;
}

void *main_run(void *arg)
{
    int ret = -1;
    int i = 0, run_time = 0;
    unsigned long stime;
    aml_ge2d_t amlge2d;
    char dst_file_name[128] = {};

    for (run_time = 0; run_time < num_process_per_thread; run_time++) {
        printf("ThreadIdx -- %d, run time -- %d\n", *(int *)arg, run_time);
        memset(&amlge2d, 0, sizeof(aml_ge2d_t));
        memset(&(amlge2d.ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
        memset(&(amlge2d.ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
        memset(&(amlge2d.ge2dinfo.dst_info), 0, sizeof(buffer_info_t));

        for (i = 0; i < MAX_PLANE; i++) {
            amlge2d.ge2dinfo.src_info[0].shared_fd[i] = -1;
            amlge2d.ge2dinfo.src_info[1].shared_fd[i] = -1;
            amlge2d.ge2dinfo.dst_info.shared_fd[i] = -1;
        }
        set_ge2dinfo(&amlge2d.ge2dinfo);
        ret = check_plane_number(amlge2d.ge2dinfo.src_info[0].plane_number,
            amlge2d.ge2dinfo.src_info[0].format);
        if (ret < 0) {
            printf("Error src1 plane_number=[%d], format=%d\n",
                amlge2d.ge2dinfo.src_info[0].plane_number,
                amlge2d.ge2dinfo.src_info[0].format);
            return NULL;
        }
        ret = check_plane_number(amlge2d.ge2dinfo.src_info[1].plane_number,
            amlge2d.ge2dinfo.src_info[1].format);
        if (ret < 0) {
            printf("Error src2 plane_number=[%d],format=%d\n",
                amlge2d.ge2dinfo.src_info[1].plane_number,
                amlge2d.ge2dinfo.src_info[1].format);
            return NULL;
        }
        ret = check_plane_number(amlge2d.ge2dinfo.dst_info.plane_number,
            amlge2d.ge2dinfo.dst_info.format);
        if (ret < 0) {
            printf("Error dst plane_number=[%d],format=%d\n",
                amlge2d.ge2dinfo.dst_info.plane_number,
                amlge2d.ge2dinfo.dst_info.format);
            return NULL;
        }

        ret = aml_ge2d_init(&amlge2d);
        if (ret < 0)
            return NULL;

        ret = aml_ge2d_mem_alloc(&amlge2d);
        if (ret < 0)
            goto exit;
    #if 0
        /* if dma_buf and used fd alloc other driver */
        /* set dma buf fd */
        amlge2d.ge2dinfo.src_info[0].shared_fd = dma_fd;
        amlge2d.ge2dinfo.src_info[0].memtype = GE2D_CANVAS_ALLOC;
    #endif

        switch (amlge2d.ge2dinfo.ge2d_op)
        {
            case AML_GE2D_FILLRECTANGLE:
                ret = do_fill_rectangle(&amlge2d);
                break;
            case AML_GE2D_BLEND:
                ret = do_blend(&amlge2d);
                break;
             case AML_GE2D_STRETCHBLIT:
                ret = do_strechblit(&amlge2d);
                break;
             case AML_GE2D_BLIT:
                ret = do_blit(&amlge2d);
                break;
             default:
                E_GE2D("not support ge2d op,exit test!\n");
                break;
        }
        if (ret < 0)
            goto exit;

        if (amlge2d.ge2dinfo.dst_info.mem_alloc_type == AML_GE2D_MEM_ION)
            aml_ge2d_invalid_cache(&amlge2d.ge2dinfo);
        sprintf(dst_file_name, "%s_thread%d_%d", DST_FILE_NAME, *(int *)arg, run_time);
        ret = aml_write_file(&amlge2d, dst_file_name);
        if (ret < 0)
            goto exit;
    exit:
        for (i = 0; i < amlge2d.ge2dinfo.src_info[0].plane_number; i++) {
            if (amlge2d.src_data[i]) {
                free(amlge2d.src_data[i]);
                amlge2d.src_data[i] = NULL;
            }
        }

        for (i = 0; i < amlge2d.ge2dinfo.src_info[1].plane_number; i++) {
            if (amlge2d.src2_data[i]) {
                free(amlge2d.src2_data[i]);
                amlge2d.src2_data[i] = NULL;
            }
        }

        for (i = 0; i < amlge2d.ge2dinfo.dst_info.plane_number; i++) {
            if (amlge2d.dst_data[i]) {
                free(amlge2d.dst_data[i]);
                amlge2d.dst_data[i] = NULL;
            }
        }
        aml_ge2d_mem_free(&amlge2d);
        aml_ge2d_exit(&amlge2d);
    }
    printf("ge2d feature_test exit!!!\n");

    return NULL;
}

int main(int argc, char **argv)
{
    int ret = -1, i;
    pthread_t thread[THREADS_MAX_NUM];
    int threadindex[THREADS_MAX_NUM];

    if (num_thread > THREADS_MAX_NUM) {
        E_GE2D("num of thread greater than THREADS_MAX_NUM\n");
        return -1;
    }
    memset(&thread, 0, sizeof(thread));

    ret = parse_command_line(argc, argv);
    if (ret == ge2d_fail)
        return ge2d_success;

    for (i = 0; i < num_thread; i++) {
        threadindex[i] = i;
        ret = pthread_create(&(thread[i]), NULL, main_run, (void *)&threadindex[i]);
        if (ret != 0) {
            E_GE2D("integral thread %d creation failed!", i);
            return -1;
        }
    }
    for (i = 0; i < num_thread; i++)
        pthread_join(thread[i], NULL);

    return 0;
}
