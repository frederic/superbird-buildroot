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
#include "include/ge2d_port.h"
#include "include/ge2d_com.h"
#include "include/dmabuf.h"
#include <IONmem.h>
#include "include/aml_ge2d.h"
#include "kernel-headers/linux/ge2d.h"

#define CANVAS_ALIGNED(x)    (((x) + 31) & ~31)

static void ge2d_calculate_buffer_size(const buffer_info_t* buffer,
                                       unsigned int* size_out)
{
    unsigned int mask = ~MATRIX_CUSTOM;
    unsigned int image_width = buffer->canvas_w;

    switch (buffer->format & mask) {
        case PIXEL_FORMAT_RGBA_8888:
        case PIXEL_FORMAT_BGRA_8888:
        case PIXEL_FORMAT_RGBX_8888:
            size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h * 4);
            break;
        case PIXEL_FORMAT_RGB_565:
            size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h * 2);
            break;
        case PIXEL_FORMAT_RGB_888:
        case PIXEL_FORMAT_BGR_888:
            size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h * 3);
            break;
        case PIXEL_FORMAT_YCrCb_420_SP:
        case PIXEL_FORMAT_YCbCr_420_SP_NV12:
            if (buffer->plane_number == 1)
                size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h * 3 / 2);
            else if (buffer->plane_number == 2) {
                size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h);
                size_out[1] = CANVAS_ALIGNED(image_width * buffer->canvas_h / 2);
            }
            break;
          case PIXEL_FORMAT_YV12:
            if (buffer->plane_number == 1)
                size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h * 3 / 2);
            else if (buffer->plane_number == 3) {
                size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h);
                size_out[1] = CANVAS_ALIGNED(image_width * buffer->canvas_h / 4);
                size_out[2] = CANVAS_ALIGNED(image_width * buffer->canvas_h / 4);
            }
            break;
          case PIXEL_FORMAT_YCbCr_422_SP:
            if (buffer->plane_number == 1)
                size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h * 2);
            else if (buffer->plane_number == 2) {
                size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h);
                size_out[1] = CANVAS_ALIGNED(image_width * buffer->canvas_h);
            }
            break;
        case PIXEL_FORMAT_Y8:
            size_out[0] = CANVAS_ALIGNED(image_width * buffer->canvas_h);
            break;
        default:
            E_GE2D("%s,%d,format not support now\n",__func__, __LINE__);
            break;
    }
}

static void ge2d_setinfo_size(aml_ge2d_t *pge2d)
{
    if (GE2D_CANVAS_ALLOC == pge2d->ge2dinfo.src_info[0].memtype) {
        ge2d_calculate_buffer_size(&pge2d->ge2dinfo.src_info[0], pge2d->src_size);
    }

    if (GE2D_CANVAS_ALLOC == pge2d->ge2dinfo.src_info[1].memtype) {
        ge2d_calculate_buffer_size(&pge2d->ge2dinfo.src_info[1], pge2d->src2_size);
    }

    if (GE2D_CANVAS_ALLOC == pge2d->ge2dinfo.dst_info.memtype) {
        ge2d_calculate_buffer_size(&pge2d->ge2dinfo.dst_info, pge2d->dst_size);
    }
}

static void ge2d_mem_free(aml_ge2d_t *pge2d)
{
    int i;

    for (i = 0; i < pge2d->ge2dinfo.src_info[0].plane_number; i++) {
        if (pge2d->src_size[i] && (pge2d->ge2dinfo.src_info[0].mem_alloc_type == AML_GE2D_MEM_ION)) {
            free((IONMEM_AllocParams*)pge2d->cmemParm_src[i]);
            pge2d->cmemParm_src[i] = NULL;
        }
        if (pge2d->ge2dinfo.src_info[0].vaddr[i])
            munmap(pge2d->ge2dinfo.src_info[0].vaddr[i], pge2d->src_size[i]);
        if (pge2d->ge2dinfo.src_info[0].shared_fd[i] != -1) {
            D_GE2D("src close shared_fd -- %d\n",
                pge2d->ge2dinfo.src_info[0].shared_fd[i]);
            close(pge2d->ge2dinfo.src_info[0].shared_fd[i]);
            pge2d->ge2dinfo.src_info[0].shared_fd[i] = -1;
        }
    }

    for (i = 0; i < pge2d->ge2dinfo.src_info[1].plane_number; i++) {
        if (pge2d->src2_size[i] && (pge2d->ge2dinfo.src_info[1].mem_alloc_type == AML_GE2D_MEM_ION)) {
            free((IONMEM_AllocParams*)pge2d->cmemParm_src2[i]);
            pge2d->cmemParm_src2[i] = NULL;
        }
        if (pge2d->ge2dinfo.src_info[1].vaddr[i])
            munmap(pge2d->ge2dinfo.src_info[1].vaddr[i], pge2d->src2_size[i]);
        if (pge2d->ge2dinfo.src_info[1].shared_fd[i] != -1) {
            D_GE2D("src1 close shared_fd -- %d\n",
                pge2d->ge2dinfo.src_info[1].shared_fd[i]);
            close(pge2d->ge2dinfo.src_info[1].shared_fd[i]);
            pge2d->ge2dinfo.src_info[1].shared_fd[i] = -1;
        }
    }

    for (i = 0; i < pge2d->ge2dinfo.dst_info.plane_number; i++) {
        if (pge2d->dst_size[i] && (pge2d->ge2dinfo.dst_info.mem_alloc_type == AML_GE2D_MEM_ION)) {
            free((IONMEM_AllocParams*)pge2d->cmemParm_dst[i]);
            pge2d->cmemParm_dst[i] = NULL;
        }
        if (pge2d->ge2dinfo.dst_info.vaddr[i])
            munmap(pge2d->ge2dinfo.dst_info.vaddr[i], pge2d->dst_size[i]);
        if (pge2d->ge2dinfo.dst_info.shared_fd[i] != -1) {
            D_GE2D("dst close shared_fd -- %d\n",
                pge2d->ge2dinfo.dst_info.shared_fd[i]);
            close(pge2d->ge2dinfo.dst_info.shared_fd[i]);
            pge2d->ge2dinfo.dst_info.shared_fd[i] = -1;
        }
    }

    D_GE2D("ge2d_mem_free!\n");
}

int aml_ge2d_get_cap(int fd_ge2d)
{
    return ge2d_get_cap(fd_ge2d);
}

int aml_ge2d_init(aml_ge2d_t *pge2d)
{
    int ret = -1, fd_ge2d, ion_fd, i;

    for (i = 0; i < MAX_PLANE; i++) {
        pge2d->ge2dinfo.src_info[0].shared_fd[i] = -1;
        pge2d->ge2dinfo.src_info[1].shared_fd[i] = -1;
        pge2d->ge2dinfo.dst_info.shared_fd[i] = -1;
    }
    fd_ge2d = ge2d_open();
    if (fd_ge2d < 0)
        return ge2d_fail;
    ion_fd = ion_mem_init();
    if (ion_fd < 0)
        return ge2d_fail;

    pge2d->ge2dinfo.ge2d_fd = fd_ge2d;
    pge2d->ge2dinfo.ion_fd = ion_fd;
    pge2d->ge2dinfo.cap_attr = aml_ge2d_get_cap(fd_ge2d);

    return ge2d_success;
}

void aml_ge2d_exit(aml_ge2d_t *pge2d)
{
    if (pge2d->ge2dinfo.ge2d_fd >= 0)
        ge2d_close(pge2d->ge2dinfo.ge2d_fd);
    if (pge2d->ge2dinfo.ion_fd >= 0)
        ion_mem_exit(pge2d->ge2dinfo.ion_fd);
}

void aml_ge2d_mem_free_ion(aml_ge2d_t *pge2d)
{
    int i;

    for (i = 0; i < pge2d->ge2dinfo.src_info[0].plane_number; i++) {
        if (pge2d->src_size[i]) {
            free((IONMEM_AllocParams*)pge2d->cmemParm_src[i]);
            pge2d->cmemParm_src[i] = NULL;
        }
        if (pge2d->ge2dinfo.src_info[0].vaddr[i])
            munmap(pge2d->ge2dinfo.src_info[0].vaddr[i], pge2d->src_size[i]);
    }

    for (i = 0; i < pge2d->ge2dinfo.src_info[1].plane_number; i++) {
        if (pge2d->src2_size[i]) {
            free((IONMEM_AllocParams*)pge2d->cmemParm_src2[i]);
            pge2d->cmemParm_src2[i] = NULL;
        }
        if (pge2d->ge2dinfo.src_info[1].vaddr[i])
            munmap(pge2d->ge2dinfo.src_info[1].vaddr[i], pge2d->src2_size[i]);
    }

    for (i = 0; i < pge2d->ge2dinfo.dst_info.plane_number; i++) {
        if (pge2d->dst_size[i]) {
            free((IONMEM_AllocParams*)pge2d->cmemParm_dst[i]);
            pge2d->cmemParm_dst[i] = NULL;
        }
        if (pge2d->ge2dinfo.dst_info.vaddr[i])
            munmap(pge2d->ge2dinfo.dst_info.vaddr[i], pge2d->dst_size[i]);
    }
}

int aml_ge2d_mem_alloc_ion(aml_ge2d_t *pge2d)
{
    int ret = -1, i;
    unsigned int nbytes = 0;

    ge2d_setinfo_size(pge2d);

    for (i = 0; i < pge2d->ge2dinfo.src_info[0].plane_number; i++) {
        if (pge2d->src_size[i]) {
            pge2d->cmemParm_src[i] = malloc(sizeof(IONMEM_AllocParams));
            ret = ion_mem_alloc(pge2d->ge2dinfo.ion_fd, pge2d->src_size[i], pge2d->cmemParm_src[i], false);
            if (ret < 0) {
                E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
            pge2d->ge2dinfo.src_info[0].shared_fd[i] = ((IONMEM_AllocParams*)pge2d->cmemParm_src[i])->mImageFd;
            pge2d->ge2dinfo.src_info[0].vaddr[i] = (char*)mmap( NULL, pge2d->src_size[i],
                PROT_READ | PROT_WRITE, MAP_SHARED, ((IONMEM_AllocParams*)pge2d->cmemParm_src[i])->mImageFd, 0);

            if (!pge2d->ge2dinfo.src_info[0].vaddr[i]) {
                E_GE2D("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
        }
    }

    for (i = 0; i < pge2d->ge2dinfo.src_info[1].plane_number; i++) {
        if (pge2d->src2_size[i]) {
            pge2d->cmemParm_src2[i] = malloc(sizeof(IONMEM_AllocParams));
            ret = ion_mem_alloc(pge2d->ge2dinfo.ion_fd, pge2d->src2_size[i], pge2d->cmemParm_src2[i], false);
            if (ret < 0) {
                E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
            pge2d->ge2dinfo.src_info[1].shared_fd[i] = ((IONMEM_AllocParams*)pge2d->cmemParm_src2[i])->mImageFd;
            pge2d->ge2dinfo.src_info[1].vaddr[i] = (char*)mmap( NULL, pge2d->src2_size[i],
                PROT_READ | PROT_WRITE, MAP_SHARED, ((IONMEM_AllocParams*)pge2d->cmemParm_src2[i])->mImageFd, 0);
            if (!pge2d->ge2dinfo.src_info[1].vaddr[i]) {
                E_GE2D("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
        }
    }

    for (i = 0; i < pge2d->ge2dinfo.dst_info.plane_number; i++) {
        if (pge2d->dst_size[i]) {
            pge2d->cmemParm_dst[i] = malloc(sizeof(IONMEM_AllocParams));
            ret = ion_mem_alloc(pge2d->ge2dinfo.ion_fd, pge2d->dst_size[i], pge2d->cmemParm_dst[i], true);
            if (ret < 0) {
                E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
            pge2d->ge2dinfo.dst_info.shared_fd[i] = ((IONMEM_AllocParams*)pge2d->cmemParm_dst[i])->mImageFd;
            pge2d->ge2dinfo.dst_info.vaddr[i] = (char*)mmap( NULL, pge2d->dst_size[i],
                PROT_READ | PROT_WRITE, MAP_SHARED, ((IONMEM_AllocParams*)pge2d->cmemParm_dst[i])->mImageFd, 0);
            if (!pge2d->ge2dinfo.dst_info.vaddr[i]) {
                E_GE2D("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
        }
        D_GE2D("src_info[0].size=%d,src_info[1].size=%d,dst_info.size=%d\n",
        pge2d->src_size[i],pge2d->src2_size[i],pge2d->dst_size[i]);
    }
    D_GE2D("aml_ge2d_mem_alloc: src_info[0].h=%d,dst_info.h=%d\n",
        pge2d->ge2dinfo.src_info[0].canvas_h,pge2d->ge2dinfo.dst_info.canvas_h);
    D_GE2D("aml_ge2d_mem_alloc: src_info[0].format=%d,dst_info.format=%d\n",
        pge2d->ge2dinfo.src_info[0].format,pge2d->ge2dinfo.dst_info.format);

    return ge2d_success;
exit:
    aml_ge2d_mem_free_ion(pge2d);
    return ret;
}

void aml_ge2d_mem_free(aml_ge2d_t *pge2d)
{
    ge2d_mem_free(pge2d);
}

int aml_ge2d_mem_alloc(aml_ge2d_t *pge2d)
{
    int ret = -1, i;
    int dma_fd = -1;
    unsigned int nbytes = 0;

    ge2d_setinfo_size(pge2d);
    for (i = 0; i < pge2d->ge2dinfo.src_info[0].plane_number; i++) {
        if (pge2d->src_size[i]) {
            if (pge2d->ge2dinfo.src_info[0].mem_alloc_type == AML_GE2D_MEM_ION) {
                pge2d->cmemParm_src[i] = malloc(sizeof(IONMEM_AllocParams));
            ret = ion_mem_alloc(pge2d->ge2dinfo.ion_fd, pge2d->src_size[i], pge2d->cmemParm_src[i], false);
                if (ret < 0) {
                    E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                    goto exit;
                }
                dma_fd = ((IONMEM_AllocParams*)pge2d->cmemParm_src[i])->mImageFd;
            } else if (pge2d->ge2dinfo.src_info[i].mem_alloc_type == AML_GE2D_MEM_DMABUF) {
                dma_fd = dmabuf_alloc(pge2d->ge2dinfo.ge2d_fd, GE2D_BUF_INPUT1,	pge2d->src_size[i]);
                if (dma_fd < 0) {
                    E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                    goto exit;
                }
            }
            pge2d->ge2dinfo.src_info[0].shared_fd[i] = dma_fd;
            pge2d->ge2dinfo.src_info[0].vaddr[i] = (char*)mmap( NULL, pge2d->src_size[i],
                PROT_READ | PROT_WRITE, MAP_SHARED, dma_fd, 0);

            if (!pge2d->ge2dinfo.src_info[0].vaddr[i]) {
                E_GE2D("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
        }
    }

    for (i = 0; i < pge2d->ge2dinfo.src_info[1].plane_number; i++) {
        if (pge2d->src2_size[i]) {
            if (pge2d->ge2dinfo.src_info[1].mem_alloc_type == AML_GE2D_MEM_ION) {
                pge2d->cmemParm_src2[i] = malloc(sizeof(IONMEM_AllocParams));
            ret = ion_mem_alloc(pge2d->ge2dinfo.ion_fd, pge2d->src2_size[i], pge2d->cmemParm_src2[i], false);
                if (ret < 0) {
                    E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                    goto exit;
                }
                dma_fd = ((IONMEM_AllocParams*)pge2d->cmemParm_src2[i])->mImageFd;
            } else if (pge2d->ge2dinfo.src_info[1].mem_alloc_type == AML_GE2D_MEM_DMABUF) {
                dma_fd = dmabuf_alloc(pge2d->ge2dinfo.ge2d_fd, GE2D_BUF_INPUT2, pge2d->src2_size[i]);
                if (dma_fd < 0) {
                    E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                    goto exit;
                }
            }
            pge2d->ge2dinfo.src_info[1].shared_fd[i] = dma_fd;
            pge2d->ge2dinfo.src_info[1].vaddr[i] = (char*)mmap( NULL, pge2d->src2_size[i],
                PROT_READ | PROT_WRITE, MAP_SHARED, dma_fd, 0);
            if (!pge2d->ge2dinfo.src_info[1].vaddr[i]) {
                E_GE2D("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
        }
    }

    for (i = 0; i < pge2d->ge2dinfo.dst_info.plane_number; i++) {
        if (pge2d->dst_size[i]) {
            if (pge2d->ge2dinfo.dst_info.mem_alloc_type == AML_GE2D_MEM_ION) {
                pge2d->cmemParm_dst[i] = malloc(sizeof(IONMEM_AllocParams));
                ret = ion_mem_alloc(pge2d->ge2dinfo.ion_fd, pge2d->dst_size[i], pge2d->cmemParm_dst[i], true);
                if (ret < 0) {
                    E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                    goto exit;
                }
                dma_fd = ((IONMEM_AllocParams*)pge2d->cmemParm_dst[i])->mImageFd;
            } else if (pge2d->ge2dinfo.dst_info.mem_alloc_type == AML_GE2D_MEM_DMABUF) {
                dma_fd = dmabuf_alloc(pge2d->ge2dinfo.ge2d_fd, GE2D_BUF_OUTPUT, pge2d->dst_size[i]);
                if (dma_fd < 0) {
                    E_GE2D("%s,%d,Not enough memory\n",__func__, __LINE__);
                    goto exit;
                }
            }
            pge2d->ge2dinfo.dst_info.shared_fd[i] = dma_fd;
            pge2d->ge2dinfo.dst_info.vaddr[i] = (char*)mmap( NULL, pge2d->dst_size[i],
                PROT_READ | PROT_WRITE, MAP_SHARED, dma_fd, 0);
            //memset(pge2d->ge2dinfo.dst_info.vaddr[i], 0x0, pge2d->dst_size[i]);
            if (!pge2d->ge2dinfo.dst_info.vaddr[i]) {
                E_GE2D("%s,%d,mmap failed,Not enough memory\n",__func__, __LINE__);
                goto exit;
            }
        }
        D_GE2D("src_info[0].size=%d,src_info[1].size=%d,dst_info.size=%d\n",
            pge2d->src_size[i],pge2d->src2_size[i],pge2d->dst_size[i]);
    }
    D_GE2D("aml_ge2d_mem_alloc: src_info[0].h=%d,dst_info.h=%d\n",
        pge2d->ge2dinfo.src_info[0].canvas_h,pge2d->ge2dinfo.dst_info.canvas_h);
    D_GE2D("aml_ge2d_mem_alloc: src_info[0].format=%d,dst_info.format=%d\n",
        pge2d->ge2dinfo.src_info[0].format,pge2d->ge2dinfo.dst_info.format);

    return ge2d_success;
exit:
    ge2d_mem_free(pge2d);
    return ret;
}

int aml_ge2d_process(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    if (pge2dinfo->ge2d_fd >= 0)
        ret = ge2d_process(pge2dinfo->ge2d_fd, pge2dinfo);
    return ret;
}

int aml_ge2d_process_ion(aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    if (pge2dinfo->ge2d_fd >= 0)
        ret = ge2d_process_ion(pge2dinfo->ge2d_fd, pge2dinfo);
    return ret;
}

int  aml_ge2d_invalid_cache(aml_ge2d_info_t *pge2dinfo)
{
    int i, shared_fd = -1;

    if (pge2dinfo) {
        for (i = 0; i < pge2dinfo->dst_info.plane_number; i++) {
            shared_fd = pge2dinfo->dst_info.shared_fd[i];
            if (shared_fd != -1)
                ion_mem_invalid_cache(pge2dinfo->ion_fd, shared_fd);
        }
    } else {
        E_GE2D("aml_ge2d_invalid err!\n");
        return -1;
    }

    return 0;
}

