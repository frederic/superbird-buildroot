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
#include "ge2d_port.h"
#include "ge2d_com.h"
#include "aml_ge2d.h"
#include "ge2d.h"

/* #define FILE_DATA */
#define SRC1_FILE_NAME   ("./fb1.rgb32")
#define SRC2_FILE_NAME   ("./fb2.rgb32")
#define SRC_FILE_NAME   ("./fb.rgb32")
#define DST_FILE_NAME   ("./out1.rgb32")

static int SX_SRC1 = 1920;
static int SY_SRC1 = 1080;
static int SX_SRC2 = 720;
static int SY_SRC2 = 576;
static int SX_DST = 1920;
static int SY_DST = 1080;

static int DEMOTIME = 3000;  /* milliseconds */
static int SRC1_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int SRC2_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
static int DST_PIXFORMAT = PIXEL_FORMAT_YV12;

static int OP = AML_GE2D_STRETCHBLIT;
static int src1_layer_mode = 0;
static int src2_layer_mode = 0;
static int gb1_alpha = 0xff;
static int gb2_alpha = 0xff;
extern aml_ge2d_t amlge2d;

static void set_ge2dinfo(aml_ge2d_info_t *pge2dinfo)
{
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[1].canvas_w = SX_SRC2;
    pge2dinfo->src_info[1].canvas_h = SY_SRC2;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;

    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->offset = 0;
    pge2dinfo->ge2d_op = (GE2DOP)OP;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
}

static void print_usage(void)
{
    int i;
    printf ("Usage: ge2d_feature_test [options]\n\n");
    printf ("Options:\n\n");
    printf ("  --op <0:fillrect, 1:blend, 2:strechblit, 3:blit>    ge2d operation case.\n");
    printf ("  --help                       Print usage information.\n");
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
        }
    }
    return ge2d_success;
}


int aml_read_file_src1(const char* url , aml_ge2d_info_t *pge2dinfo)
{
    int fd = -1;
    int length = 0;
    int read_num = 0;
    if (amlge2d.src_size == 0)
        return 0;

    fd = open(url,O_RDONLY );
    if (fd < 0) {
        E_GE2D("read source file:%s open error\n",url);
        return ge2d_fail;
    }

    amlge2d.src_data = (char*)malloc(amlge2d.src_size);
    if (!amlge2d.src_data) {
        E_GE2D("malloc for src_data failed\n");
        return ge2d_fail;
    }

    read_num = read(fd,amlge2d.src_data,amlge2d.src_size);
    if (read_num <= 0) {
        E_GE2D("read file read_num=%d error\n",read_num);
        return ge2d_fail;
    }

    memcpy(pge2dinfo->src_info[0].vaddr, amlge2d.src_data, amlge2d.src_size);
    close(fd);
    return ge2d_success;
}

int aml_read_file_src2(const char* url , aml_ge2d_info_t *pge2dinfo)
{
    int fd = -1;
    int length = 0;
    int read_num = 0;
    if (amlge2d.src2_size == 0)
        return 0;

    fd = open(url,O_RDONLY );
    if (fd < 0) {
        E_GE2D("read source file:%s open error\n",url);
        return ge2d_fail;
    }

    amlge2d.src2_data = (char*)malloc(amlge2d.src2_size);
    if (!amlge2d.src2_data) {
        E_GE2D("malloc for src_data failed\n");
        return ge2d_fail;
    }

    read_num = read(fd,amlge2d.src2_data,amlge2d.src2_size);
    if (read_num <= 0) {
        E_GE2D("read file read_num=%d error\n",read_num);
        return ge2d_fail;
    }

    memcpy(pge2dinfo->src_info[1].vaddr, amlge2d.src2_data, amlge2d.src2_size);
    close(fd);
    return ge2d_success;
}

int aml_write_file(const char* url , aml_ge2d_info_t *pge2dinfo)
{
    int fd = -1;
    int length = 0;
    int write_num = 0;
    unsigned int *value;
    if (amlge2d.dst_size == 0)
        return 0;
    if ((GE2D_CANVAS_OSD0 == pge2dinfo->dst_info.memtype)
        || (GE2D_CANVAS_OSD1 == pge2dinfo->dst_info.memtype))
        return 0;

    fd = open(url,O_RDWR | O_CREAT,0660);
    if (fd < 0) {
        E_GE2D("write file:%s open error\n",url);
        return ge2d_fail;
    }

    amlge2d.dst_data = (char*)malloc(amlge2d.dst_size);
    if (!amlge2d.dst_data) {
        E_GE2D("malloc for dst_data failed\n");
        return ge2d_fail;
    }

    memcpy(amlge2d.dst_data,pge2dinfo->dst_info.vaddr,amlge2d.dst_size);
    printf("pixel: 0x%2x, 0x%2x,0x%2x,0x%2x, 0x%2x,0x%2x,0x%2x,0x%2x\n",
        amlge2d.dst_data[0],
        amlge2d.dst_data[1],
        amlge2d.dst_data[2],
        amlge2d.dst_data[3],
        amlge2d.dst_data[4],
        amlge2d.dst_data[5],
        amlge2d.dst_data[6],
        amlge2d.dst_data[7]);
    write_num = write(fd,amlge2d.dst_data,amlge2d.dst_size);
    if (write_num <= 0) {
        E_GE2D("read file read_num=%d error\n",write_num);
    }
    close(fd);
    return ge2d_success;
}


static int do_fill_rectangle(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    char code;
    ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
    if (ret < 0)
       return  ge2d_fail;
	#if 0
    ret = aml_read_file_src2(SRC1_FILE_NAME,pge2dinfo);
    if (ret < 0)
       return ge2d_fail;
	#endif
    printf("do_fill_rectangle test case:\n");
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_OSD0;
    pge2dinfo->src_info[0].canvas_w = SX_DST;
    pge2dinfo->src_info[0].canvas_h = SY_DST;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->dst_info.canvas_w = 0;
    pge2dinfo->dst_info.canvas_h = 0;
    pge2dinfo->dst_info.format = SRC1_PIXFORMAT;
    #if 1
    int x,y,w,h;
    printf("please input fill color:[RGBA]\n");
    scanf("%x",&pge2dinfo->color);
    printf("please input fill rect:x,y,w,h]\n");
    scanf("%d,%d,%d,%d",&x,&y,&w,&h);
    //if ((x > SX)|| (x > SY) || (w > SX) || (h > SY))
    //    E_GE2D("do_fill_rectangle:input parameter error!\n");

    pge2dinfo->dst_info.rect.x = x;
    pge2dinfo->dst_info.rect.y = y;
    pge2dinfo->dst_info.rect.w = w;
    pge2dinfo->dst_info.rect.h = h;
    #else
    pge2dinfo->color = 0xffffffff;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = pge2dinfo->src_info[0].canvas_w/2;
    pge2dinfo->dst_info.rect.h = pge2dinfo->src_info[0].canvas_h/2;
    #endif
    ret = aml_ge2d_process(pge2dinfo);
	#if 0
    sleep(5);

    printf("please enter any key do rotation test[90]\n");
    code = getc(stdin);
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_180;
    ret = aml_ge2d_process(pge2dinfo);
	#endif
    return ret;
}


static int do_blend(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    char code = 0;
    int shared_fd_bakup;
    unsigned long offset_bakup = 0;
    printf("do_blend test case:\n");
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    if (cap_attr == 0x1) {
        /* do blend src1 blend src2(dst) to dst */
        printf("one step blend\n");
        ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
        if (ret < 0)
           return  ge2d_fail;
        ret = aml_read_file_src2(SRC1_FILE_NAME,pge2dinfo);
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
        if ((src2_layer_mode == LAYER_MODE_NON) && (src1_layer_mode == LAYER_MODE_PREMULTIPLIED))
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
    } else {
        if (((gb1_alpha != 0xff)
        && (gb2_alpha != 0xff))){
        printf("two steps blend,two plane alpha\n");

        if (src2_layer_mode == LAYER_MODE_COVERAGE) {
            printf("two steps blend,src2 LAYER_MODE_COVERAGE\n");
            ret = aml_read_file_src1(SRC1_FILE_NAME,pge2dinfo);
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
            ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
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
            pge2dinfo->src_info[1].shared_fd = pge2dinfo->dst_info.shared_fd;
            pge2dinfo->src_info[1].offset = pge2dinfo->dst_info.offset;
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
            ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
            if (ret < 0)
               return  ge2d_fail;
            ret = aml_read_file_src2(SRC1_FILE_NAME,pge2dinfo);
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

            shared_fd_bakup = pge2dinfo->src_info[0].shared_fd;
            offset_bakup = pge2dinfo->src_info[0].offset;
            pge2dinfo->src_info[0].shared_fd = pge2dinfo->src_info[1].shared_fd;
            pge2dinfo->src_info[0].offset = pge2dinfo->src_info[1].offset;

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
            pge2dinfo->src_info[0].shared_fd = shared_fd_bakup;
            pge2dinfo->src_info[0].offset = offset_bakup;
            pge2dinfo->src_info[0].fill_color_en = 0;

            pge2dinfo->src_info[1].canvas_w = pge2dinfo->dst_info.canvas_w;
            pge2dinfo->src_info[1].canvas_h = pge2dinfo->dst_info.canvas_h;
            pge2dinfo->src_info[1].format = pge2dinfo->dst_info.format;
            pge2dinfo->src_info[1].rect.x = pge2dinfo->dst_info.rect.x;
            pge2dinfo->src_info[1].rect.y = pge2dinfo->dst_info.rect.y;
            pge2dinfo->src_info[1].rect.w = pge2dinfo->dst_info.rect.w;
            pge2dinfo->src_info[1].rect.h = pge2dinfo->dst_info.rect.h;
            pge2dinfo->src_info[1].shared_fd = pge2dinfo->dst_info.shared_fd;
            pge2dinfo->src_info[1].offset = pge2dinfo->dst_info.offset;
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
        ret = aml_read_file_src1(SRC1_FILE_NAME,pge2dinfo);
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
        ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
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
        pge2dinfo->src_info[1].shared_fd = pge2dinfo->dst_info.shared_fd;
        pge2dinfo->src_info[1].offset = pge2dinfo->dst_info.offset;
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
        ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
        if (ret < 0)
            return ge2d_fail;
        ret = aml_read_file_src2(SRC1_FILE_NAME,pge2dinfo);
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

        shared_fd_bakup = pge2dinfo->src_info[0].shared_fd;
        offset_bakup = pge2dinfo->src_info[0].offset;
        pge2dinfo->src_info[0].shared_fd = pge2dinfo->src_info[1].shared_fd;
        pge2dinfo->src_info[0].offset = pge2dinfo->src_info[1].offset;
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
        pge2dinfo->src_info[0].shared_fd = shared_fd_bakup;
        pge2dinfo->src_info[0].offset = offset_bakup;
        pge2dinfo->src_info[0].fill_color_en = 0;

        pge2dinfo->src_info[1].canvas_w = pge2dinfo->dst_info.canvas_w;
        pge2dinfo->src_info[1].canvas_h = pge2dinfo->dst_info.canvas_h;
        pge2dinfo->src_info[1].format = pge2dinfo->dst_info.format;
        pge2dinfo->src_info[1].rect.x = pge2dinfo->dst_info.rect.x;
        pge2dinfo->src_info[1].rect.y = pge2dinfo->dst_info.rect.y;
        pge2dinfo->src_info[1].rect.w = pge2dinfo->dst_info.rect.w;
        pge2dinfo->src_info[1].rect.h = pge2dinfo->dst_info.rect.h;
        pge2dinfo->src_info[1].shared_fd = pge2dinfo->dst_info.shared_fd;
        pge2dinfo->src_info[1].offset = pge2dinfo->dst_info.offset;
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
        ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
        if (ret < 0)
           return  ge2d_fail;
        ret = aml_read_file_src2(SRC1_FILE_NAME,pge2dinfo);
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
    return ret;
}


static int do_strechblit(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    char code = 0;
    printf("do_strechblit test case:\n");
    ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
    if (ret < 0)
       return ge2d_fail;
	#if 0
    ret = aml_read_file_src2(SRC1_FILE_NAME,pge2dinfo);
    if (ret < 0)
       return ge2d_fail;
	#endif
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
#if 1
	int x,y,w,h;
	printf("please input src crop rect info\n");
	scanf("%d,%d,%d,%d",&x,&y,&w,&h);
#endif
    pge2dinfo->src_info[0].rect.x = x;
    pge2dinfo->src_info[0].rect.y = y;
    pge2dinfo->src_info[0].rect.w = w;
    pge2dinfo->src_info[0].rect.h = h;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = SX_DST;
    pge2dinfo->dst_info.rect.h = SY_DST;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;
    ret = aml_ge2d_process(pge2dinfo);
	#if 0
    sleep(5);

    printf("please enter any key do rotation test[90]\n");
    code = getc(stdin);
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_90;
    ret = aml_ge2d_process(pge2dinfo);
	#endif

    return ret;

}

static int do_blit(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    char code = 0;
    printf("do_blit test case:\n");
    ret = aml_read_file_src1(SRC_FILE_NAME,pge2dinfo);
    if (ret < 0)
       return ge2d_fail;
	#if 0
    ret = aml_read_file_src2(SRC1_FILE_NAME,pge2dinfo);
    if (ret < 0)
       return ge2d_fail;
	#endif
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_OSD0;
    pge2dinfo->src_info[0].canvas_w = SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    pge2dinfo->dst_info.canvas_w = SX_DST;
    pge2dinfo->dst_info.canvas_h = SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;

    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = pge2dinfo->src_info[0].canvas_w;
    pge2dinfo->src_info[0].rect.h = pge2dinfo->src_info[0].canvas_h;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;

    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->src_info[0].layer_mode = src1_layer_mode;
    pge2dinfo->src_info[0].plane_alpha = gb1_alpha;

    ret = aml_ge2d_process(pge2dinfo);
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
    return ret;
}

int main(int argc, char **argv)
{
    int ret = -1;
    int i = 0;
    unsigned long stime;
    aml_ge2d_info_t ge2dinfo;
    memset(&amlge2d,0x0,sizeof(aml_ge2d_t));
    amlge2d.pge2d_info = &ge2dinfo;
    memset(&ge2dinfo, 0, sizeof(aml_ge2d_info_t));
    memset(&(ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
    memset(&(ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
    memset(&(ge2dinfo.dst_info), 0, sizeof(buffer_info_t));
    cap_attr = 0;
    ret = parse_command_line(argc,argv);
    if (ret == ge2d_fail)
        return ge2d_success;

    set_ge2dinfo(&ge2dinfo);

    ret = aml_ge2d_init();
    if (ret < 0)
        return ge2d_fail;
    aml_ge2d_get_cap();
    ret = aml_ge2d_mem_alloc(&ge2dinfo);
    if (ret < 0)
        goto exit;

    switch (ge2dinfo.ge2d_op)
    {
        case AML_GE2D_FILLRECTANGLE:
            do_fill_rectangle(&ge2dinfo);
            break;
        case AML_GE2D_BLEND:
            do_blend(&ge2dinfo);
            break;
         case AML_GE2D_STRETCHBLIT:
            do_strechblit(&ge2dinfo);
            break;
         case AML_GE2D_BLIT:
            do_blit(&ge2dinfo);
            break;
         default:
            E_GE2D("not support ge2d op,exit test!\n");
            break;
    }

    aml_write_file(DST_FILE_NAME,&ge2dinfo);

exit:
    if (amlge2d.src_data) {
        free(amlge2d.src_data);
        amlge2d.src_data = NULL;
    }
    if (amlge2d.src2_data) {
        free(amlge2d.src2_data);
        amlge2d.src2_data = NULL;
    }
    if (amlge2d.dst_data) {
        free(amlge2d.dst_data);
        amlge2d.dst_data = NULL;
    }
    aml_ge2d_mem_free(&ge2dinfo);
    aml_ge2d_exit();
    printf("ge2d feature_test exit!!!\n");
    return ge2d_success;
}
