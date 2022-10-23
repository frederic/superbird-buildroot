/*
 *   Copyright 2013 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "test_dma_api.h"

int create_ctx(struct usr_ctx_s *ctx)
{
    memset(ctx, 0, sizeof(struct usr_ctx_s));
    ctx->fd[0] = ctx->fd[1] = ctx->fd[2] = -1;

    ctx->gdc_client = open("/dev/gdc", O_RDWR | O_SYNC);

    if (ctx->gdc_client < 0) {
        printf("gdc open failed error=%d, %s",
                errno, strerror(errno));
        return -1;
    }

    return 0;
}

int destroy_ctx(struct usr_ctx_s *ctx)
{
    if (ctx->i_buff[0] != NULL) {
        munmap(ctx->i_buff[0], ctx->i_len[0]);
        ctx->i_buff[0] = NULL;
        ctx->i_len[0] = 0;
    }
    if (ctx->i_buff[1] != NULL) {
        munmap(ctx->i_buff[1], ctx->i_len[1]);
        ctx->i_buff[1] = NULL;
        ctx->i_len[1] = 0;
    }
    if (ctx->i_buff[2] != NULL) {
        munmap(ctx->i_buff[2], ctx->i_len[2]);
        ctx->i_buff[2] = NULL;
        ctx->i_len[2] = 0;
    }

    if (ctx->o_buff != NULL) {
        munmap(ctx->o_buff, ctx->o_len);
        ctx->o_buff = NULL;
        ctx->o_len = 0;
    }

    if (ctx->c_buff != NULL) {
        munmap(ctx->c_buff, ctx->c_len);
        ctx->c_buff = NULL;
        ctx->c_len = 0;
    }

    if (ctx->gdc_client >= 0) {
        close(ctx->gdc_client);
        ctx->gdc_client = -1;
    }

    return 0;
}

static int _alloc_dma_buffer(int fd, unsigned int dir, unsigned int len)
{
    int ret = -1;
    struct dmabuf_req_s buf_cfg;

    memset(&buf_cfg, 0, sizeof(buf_cfg));

    buf_cfg.dma_dir = dir;
    buf_cfg.len = len;

    ret = ioctl(fd, GDC_REQUEST_DMA_BUFF, &buf_cfg);
    if (ret < 0) {
        printf("GDC_REQUEST_DMA_BUFF %s failed: %s\n", __func__, strerror(ret));
        return ret;
    }
    return buf_cfg.index;
}

static int _get_dma_buffer_fd(int fd, int index)
{
    struct dmabuf_exp_s ex_buf;

    ex_buf.index = index;
    ex_buf.flags = O_RDWR;
    ex_buf.fd = -1;

    if (ioctl(fd, GDC_EXP_DMA_BUFF, &ex_buf))  {
        printf("failed get dma buf fd\n");
        return -1;
    }
    printf("dma buffer export, fd=%d\n", ex_buf.fd);

    return ex_buf.fd;
}

static int _free_dma_buffer(int fd, int index)
{
    printf("_gdc_free_dma_buffer: index=%d\n", index);
    if (ioctl(fd, GDC_FREE_DMA_BUFF, &index))  {
        printf("failed free dma buf fd\n");
        return -1;
    }
    return 0;
}

int alloc_dma_buffer (struct usr_ctx_s *ctx,
        uint32_t type, size_t len)
{
    int dir = 0, index = -1, dma_fd = -1;
    int i = 0;

    if (type == OUTPUT_BUFF_TYPE)
        dir = DMA_FROM_DEVICE;
    else
        dir = DMA_TO_DEVICE;

    index = _alloc_dma_buffer(ctx->gdc_client, dir,len);
    if (index < 0)
        return -1;

    /* get  dma fd*/
    dma_fd = _get_dma_buffer_fd(ctx->gdc_client, index);
    if (dma_fd < 0)
    return -1;
    printf("gdc_alloc_dma_buffer: fd=%d\n", dma_fd);
    /* after alloc, dmabuffer free can be called, it just dec refcount */
    _free_dma_buffer(ctx->gdc_client, index);

    switch (type) {
        case INPUT_BUFF_TYPE:
            i = ctx->inbuf_num;
            ctx->i_len[i] = len;
            ctx->i_buff[i] = (char *)mmap(NULL, len,
                PROT_READ | PROT_WRITE,
                MAP_SHARED, dma_fd, 0);
            if (ctx->i_buff[i] == MAP_FAILED) {
                ctx->i_buff[i] = NULL;
                printf("Failed to alloc i_buff:%s\n", strerror(errno));
            }
        ctx->fd[i] = dma_fd;
        ctx->inbuf_num++;
        break;
        default:
            printf("Error no such buff type\n");
            break;
    }
    return dma_fd;
}

int sync_cpu(struct usr_ctx_s *ctx)
{
    int ret = 0;
    int shared_fd = -1;

    if (ctx->i_buff[0]) {
        shared_fd = ctx->fd[0];
        ret = ioctl(ctx->gdc_client, GDC_SYNC_CPU, &shared_fd);
        if (ret < 0) {
            printf("gdc_sync_cpu ioctl failed\n");
            return ret;
        }
    }

    if (ctx->i_buff[1]) {
        shared_fd = ctx->fd[1];
        ret = ioctl(ctx->gdc_client, GDC_SYNC_CPU, &shared_fd);
        if (ret < 0) {
            printf("gdc_sync_cpu ioctl failed\n");
            return ret;
        }
    }

    if (ctx->i_buff[2]) {
        shared_fd = ctx->fd[2];
        ret = ioctl(ctx->gdc_client, GDC_SYNC_CPU, &shared_fd);
        if (ret < 0) {
            printf("gdc_sync_cpu ioctl failed\n");
            return ret;
        }
    }
    return 0;
}
