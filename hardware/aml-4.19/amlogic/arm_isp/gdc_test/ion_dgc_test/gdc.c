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
#include "include/ion/ion.h"
#include "kernel-headers/linux/ion.h"
#include <errno.h>

#include "include/gdc/gdc_api.h"

int gdc_create_ctx(struct gdc_usr_ctx_s *ctx)
{
    ctx->gdc_client = open("/dev/gdc", O_RDONLY | O_CLOEXEC);

    if (ctx->gdc_client < 0)
        printf("ion open failed error=%d, %s",
                errno, strerror(errno));

    ctx->ion_client = ion_open();

    if (ctx->ion_client < 0)
        printf("ion open failed error=%d, %s",
                errno, strerror(errno));
    return 0;
}

int gdc_destroy_ctx(struct gdc_usr_ctx_s *ctx)
{

    if (ctx->gdc_client >= 0) {
        close(ctx->gdc_client);
        ctx->gdc_client = -1;
    }

    if (ctx->ion_client >= 0) {
        close(ctx->ion_client);
        ctx->ion_client = -1;
    }

    return 0;
}

int gdc_init(struct gdc_usr_ctx_s *ctx, struct gdc_settings *gs)
{
    memcpy (&ctx->gs, gs, sizeof(*gs));
    return 0;
}

int gdc_alloc_dma_buffer (struct gdc_usr_ctx_s *ctx,
        int *share_fd, size_t len)
{
    int ret;
    int ion_client = ctx->ion_client;
    size_t align = 4096;
    int heap_mask =  1 << ION_HEAP_TYPE_DMA;
    int alloc_flags = 0;
    ion_user_handle_t handle;

    ret = ion_alloc(ion_client, len, align, heap_mask, alloc_flags, &handle);

    if (ret)
        printf("%s failed: %s\n", __func__, strerror(ret));

    ret = ion_share(ion_client, handle, share_fd);
    if (ret)
        printf("share failed %s\n", strerror(errno));

    //free this handle as share_fd
    //could hold this reference.
    ion_free(ion_client, handle);

    return ret;
}
int gdc_process(struct gdc_usr_ctx_s *ctx,
        int in_buf_fd, int out_buf_fd)
{
    struct gdc_settings *gs = &ctx->gs;
    int ret = 0;

    gs->in_fd = in_buf_fd;
    gs->out_fd = out_buf_fd;
    gs->magic = sizeof(gs);

    ret = ioctl(ctx->gdc_client, GDC_PROCESS, gs);
    if (ret < 0)
        printf("GDC_PROCESS ioctl failed\n");

    return 0;
}
