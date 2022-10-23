/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef DMABUF_H
#define DMABUF_H
#if defined (__cplusplus)
extern "C" {
#endif

enum {
    GE2D_BUF_INPUT1,
    GE2D_BUF_INPUT2,
    GE2D_BUF_OUTPUT,
};

int dmabuf_alloc(int ge2d_fd, int type, unsigned int len);
int dmabuf_sync_for_device(int ge2d_fd, int dma_fd);
int dmabuf_sync_for_cpu(int ge2d_fd, int dma_fd);
#if defined (__cplusplus)
}
#endif

#endif

