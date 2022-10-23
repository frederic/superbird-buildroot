/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include "ge2d_port.h"
#include "ge2d_com.h"
#include "IONmem.h"
#include "aml_ge2d.h"
#include "ge2d.h"

IONMEM_AllocParams cmemParm_src;
IONMEM_AllocParams cmemParm_src2;
IONMEM_AllocParams cmemParm_dst;

#define CANVAS_ALIGNED(x)	(((x) + 31) & ~31)

aml_ge2d_t amlge2d;
static int fd_ge2d = -1;

static void fill_data(buffer_info_t *pinfo,unsigned int color)
{
    unsigned int i = 0;
    memset(pinfo->vaddr,0x00,amlge2d.src_size);
    for (i=0;i<amlge2d.src_size;) {
        pinfo->vaddr[i] = color & 0xff;
        pinfo->vaddr[i+1] = (color & 0xff00)>>8;
        pinfo->vaddr[i+2] = (color & 0xff0000)>>16;
        pinfo->vaddr[i+3] = (color & 0xff000000)>>24;
        i+=4;
    }
}
int aml_ge2d_init(void)
{
    int ret = -1;
    fd_ge2d = ge2d_open();
    if (fd_ge2d < 0)
        return ge2d_fail;
    ret = CMEM_init();
    if (ret < 0)
        return ge2d_fail;
    return ge2d_success;
}


void aml_ge2d_exit(void)
{
    if (fd_ge2d >= 0)
        ge2d_close(fd_ge2d);
    CMEM_exit();
}

void aml_ge2d_get_cap(void)
{
    int ret = -1;
    int cap_mask = 0;
    ret = ioctl(fd_ge2d, GE2D_GET_CAP, &cap_mask);
    if (ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        cap_attr = -1;
    }
    cap_attr = cap_mask;
}

void aml_ge2d_mem_free(aml_ge2d_info_t *pge2dinfo)
{
    if (amlge2d.src_size)
        CMEM_free(&cmemParm_src);
    if (amlge2d.src2_size)
        CMEM_free(&cmemParm_src2);
    if (amlge2d.dst_size)
        CMEM_free(&cmemParm_dst);

    if (pge2dinfo->src_info[0].vaddr)
        munmap(pge2dinfo->src_info[0].vaddr, amlge2d.src_size);
    if (pge2dinfo->src_info[1].vaddr)
        munmap(pge2dinfo->src_info[1].vaddr, amlge2d.src2_size);
    if (pge2dinfo->dst_info.vaddr)
        munmap(pge2dinfo->dst_info.vaddr, amlge2d.dst_size);

    if (pge2dinfo->src_info[0].shared_fd !=-1) {
        close(pge2dinfo->src_info[0].shared_fd);
        pge2dinfo->src_info[0].shared_fd = -1;
    }
    if (pge2dinfo->src_info[1].shared_fd !=-1) {
        close(pge2dinfo->src_info[1].shared_fd);
        pge2dinfo->src_info[1].shared_fd = -1;
    }
    if (pge2dinfo->dst_info.shared_fd !=-1) {
        close(pge2dinfo->dst_info.shared_fd);
        pge2dinfo->dst_info.shared_fd = -1;
    }

    D_GE2D("aml_ge2d_mem_free!\n");
}

int aml_ge2d_mem_alloc(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    unsigned int input_image_width  = 0, input2_image_width = 0, output_image_width = 0;
    unsigned int nbytes = 0;

    if (GE2D_CANVAS_ALLOC == pge2dinfo->src_info[0].memtype) {
        input_image_width  = pge2dinfo->src_info[0].canvas_w;
        if ((pge2dinfo->src_info[0].format == PIXEL_FORMAT_RGBA_8888) ||
            (pge2dinfo->src_info[0].format == PIXEL_FORMAT_BGRA_8888) ||
            (pge2dinfo->src_info[0].format == PIXEL_FORMAT_RGBX_8888))
            amlge2d.src_size = CANVAS_ALIGNED(input_image_width * pge2dinfo->src_info[0].canvas_h * 4);
        else if (pge2dinfo->src_info[0].format == PIXEL_FORMAT_RGB_565)
            amlge2d.src_size = CANVAS_ALIGNED(input_image_width * pge2dinfo->src_info[0].canvas_h * 2);
        else if (pge2dinfo->src_info[0].format == PIXEL_FORMAT_RGB_888)
            amlge2d.src_size = CANVAS_ALIGNED(input_image_width * pge2dinfo->src_info[0].canvas_h * 3);
        else if ((pge2dinfo->src_info[0].format == PIXEL_FORMAT_YCrCb_420_SP) ||
            (pge2dinfo->src_info[0].format == PIXEL_FORMAT_YV12))
            amlge2d.src_size = CANVAS_ALIGNED(input_image_width * pge2dinfo->src_info[0].canvas_h * 3 / 2);
        else if ((pge2dinfo->src_info[0].format == PIXEL_FORMAT_YCbCr_422_SP) ||
            (pge2dinfo->src_info[0].format == PIXEL_FORMAT_YCbCr_422_I))
            amlge2d.src_size = CANVAS_ALIGNED(input_image_width * pge2dinfo->src_info[0].canvas_h * 2);
        else if (pge2dinfo->src_info[0].format == PIXEL_FORMAT_Y8)
            amlge2d.src_size = CANVAS_ALIGNED(input_image_width * pge2dinfo->src_info[0].canvas_h);
        else {
            E_GE2D("format not support now\n");
            goto exit;
        }
    }

    if (GE2D_CANVAS_ALLOC == pge2dinfo->src_info[1].memtype) {
        input2_image_width  = pge2dinfo->src_info[1].canvas_w;
        if ((pge2dinfo->src_info[1].format == PIXEL_FORMAT_RGBA_8888) ||
            (pge2dinfo->src_info[1].format == PIXEL_FORMAT_BGRA_8888) ||
            (pge2dinfo->src_info[1].format == PIXEL_FORMAT_RGBX_8888))
            amlge2d.src2_size = CANVAS_ALIGNED(input2_image_width * pge2dinfo->src_info[1].canvas_h * 4);
        else if (pge2dinfo->src_info[1].format == PIXEL_FORMAT_RGB_565)
            amlge2d.src2_size = CANVAS_ALIGNED(input2_image_width * pge2dinfo->src_info[1].canvas_h * 2);
        else if (pge2dinfo->src_info[1].format == PIXEL_FORMAT_RGB_888)
            amlge2d.src2_size = CANVAS_ALIGNED(input2_image_width * pge2dinfo->src_info[1].canvas_h * 3);
        else if ((pge2dinfo->src_info[1].format == PIXEL_FORMAT_YCrCb_420_SP) ||
            (pge2dinfo->src_info[1].format == PIXEL_FORMAT_YV12))
            amlge2d.src2_size = CANVAS_ALIGNED(input2_image_width * pge2dinfo->src_info[1].canvas_h * 3 / 2);
        else if ((pge2dinfo->src_info[1].format == PIXEL_FORMAT_YCbCr_422_SP) ||
            (pge2dinfo->src_info[1].format == PIXEL_FORMAT_YCbCr_422_I))
            amlge2d.src2_size = CANVAS_ALIGNED(input2_image_width * pge2dinfo->src_info[1].canvas_h * 2);
        else if (pge2dinfo->src_info[1].format == PIXEL_FORMAT_Y8)
            amlge2d.src2_size = CANVAS_ALIGNED(input2_image_width * pge2dinfo->src_info[1].canvas_h);
        else {
            E_GE2D("format not support now\n");
            goto exit;
        }
    }

    if (GE2D_CANVAS_ALLOC == pge2dinfo->dst_info.memtype) {
        output_image_width = pge2dinfo->dst_info.canvas_w;
        if ((pge2dinfo->dst_info.format == PIXEL_FORMAT_RGBA_8888) ||
            (pge2dinfo->dst_info.format == PIXEL_FORMAT_BGRA_8888) ||
            (pge2dinfo->dst_info.format == PIXEL_FORMAT_RGBX_8888))
            amlge2d.dst_size = CANVAS_ALIGNED(output_image_width * pge2dinfo->dst_info.canvas_h * 4);
        else if (pge2dinfo->dst_info.format == PIXEL_FORMAT_RGB_565)
            amlge2d.dst_size = CANVAS_ALIGNED(output_image_width * pge2dinfo->dst_info.canvas_h * 2);
        else if (pge2dinfo->dst_info.format == PIXEL_FORMAT_RGB_888)
            amlge2d.dst_size = CANVAS_ALIGNED(output_image_width * pge2dinfo->dst_info.canvas_h * 3);
        else if ((pge2dinfo->dst_info.format == PIXEL_FORMAT_YCrCb_420_SP) ||
            (pge2dinfo->dst_info.format == PIXEL_FORMAT_YV12))
            amlge2d.dst_size = CANVAS_ALIGNED(output_image_width * pge2dinfo->dst_info.canvas_h * 3 / 2);
        else if ((pge2dinfo->dst_info.format == PIXEL_FORMAT_YCbCr_422_SP) ||
            (pge2dinfo->dst_info.format == PIXEL_FORMAT_YCbCr_422_I))
            amlge2d.dst_size = CANVAS_ALIGNED(output_image_width * pge2dinfo->dst_info.canvas_h * 2);
        else if (pge2dinfo->dst_info.format == PIXEL_FORMAT_Y8)
            amlge2d.dst_size = CANVAS_ALIGNED(output_image_width * pge2dinfo->dst_info.canvas_h);

        else {
            E_GE2D("format not support now\n");
            goto exit;
        }
    }
    if (amlge2d.src_size) {
        ret = CMEM_alloc(amlge2d.src_size, &cmemParm_src, false);
        if (ret < 0) {
            E_GE2D("Not enough memory\n");
            CMEM_free(&cmemParm_src);
            return ge2d_fail;
        }
        pge2dinfo->src_info[0].shared_fd = cmemParm_src.mImageFd;
        pge2dinfo->src_info[0].vaddr = (char*)mmap( NULL, amlge2d.src_size,
            PROT_READ | PROT_WRITE, MAP_SHARED, cmemParm_src.mImageFd, 0);

        if (!pge2dinfo->src_info[0].vaddr) {
            E_GE2D("mmap failed,Not enough memory\n");
            goto exit;
        }
        fill_data(&pge2dinfo->src_info[0],0x00000000);
    }

    if (amlge2d.src2_size) {
        ret = CMEM_alloc(amlge2d.src2_size, &cmemParm_src2, false);
        if (ret < 0) {
            E_GE2D("Not enough memory\n");
            CMEM_free(&cmemParm_src2);
            return ge2d_fail;
        }
        pge2dinfo->src_info[1].shared_fd = cmemParm_src2.mImageFd;
        pge2dinfo->src_info[1].vaddr = (char*)mmap( NULL, amlge2d.src2_size,
            PROT_READ | PROT_WRITE, MAP_SHARED, cmemParm_src2.mImageFd, 0);
        if (!pge2dinfo->src_info[1].vaddr) {
            E_GE2D("mmap failed,Not enough memory\n");
            goto exit;
        }
    }


    if (amlge2d.dst_size) {
        ret = CMEM_alloc(amlge2d.dst_size, &cmemParm_dst, true);
        if (ret < 0) {
            E_GE2D("Not enough memory\n");
            goto exit;
        }
        pge2dinfo->dst_info.shared_fd = cmemParm_dst.mImageFd;
        pge2dinfo->dst_info.vaddr = (char*)mmap( NULL, amlge2d.dst_size,
            PROT_READ | PROT_WRITE, MAP_SHARED, cmemParm_dst.mImageFd, 0);

        if (!pge2dinfo->dst_info.vaddr) {
            E_GE2D("mmap failed,Not enough memory\n");
            goto exit;
        }
    }

    D_GE2D("aml_ge2d_mem_alloc: src_info[0].w=%d,src_info[1].w=%d,dst_info.w=%d\n",
        input_image_width,input2_image_width,output_image_width);
    D_GE2D("aml_ge2d_mem_alloc: src_info[0].h=%d,dst_info.h=%d\n",
        pge2dinfo->src_info[0].canvas_h,pge2dinfo->dst_info.canvas_h);
    D_GE2D("aml_ge2d_mem_alloc: src_info[0].format=%d,dst_info.format=%d\n",
        pge2dinfo->src_info[0].format,pge2dinfo->dst_info.format);
    D_GE2D("src_info[0].size=%d,src_info[1].size=%d,dst_info.size=%d\n",
        amlge2d.src_size,amlge2d.src2_size,amlge2d.dst_size);
    return ge2d_success;
exit:
    if (amlge2d.src_size)
        CMEM_free(&cmemParm_src);
    if (amlge2d.src2_size)
        CMEM_free(&cmemParm_src2);

    if (pge2dinfo->src_info[0].vaddr)
        munmap(pge2dinfo->src_info[0].vaddr, amlge2d.src_size);
    if (pge2dinfo->src_info[1].vaddr)
        munmap(pge2dinfo->src_info[1].vaddr, amlge2d.src2_size);
    if (amlge2d.dst_size)
        CMEM_free(&cmemParm_dst);
    if (pge2dinfo->dst_info.vaddr)
        munmap(pge2dinfo->dst_info.vaddr, amlge2d.dst_size);
    return ret;
}


int aml_ge2d_process(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    if (fd_ge2d >= 0)
        ret = ge2d_process(fd_ge2d,pge2dinfo);
    return ret;
}
int  aml_ge2d_invalid_cache(aml_ge2d_info_t *pge2dinfo)
{
    if (pge2dinfo && pge2dinfo->dst_info.shared_fd != -1) {
        CMEM_invalid_cache(pge2dinfo->dst_info.shared_fd);
    } else {
        E_GE2D("aml_ge2d_invalid err!\n");
        return -1;
    }
    return 0;
}


