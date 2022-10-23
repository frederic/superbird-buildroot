/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <linux/dma-buf.h>
#include <sys/ioctl.h>

#include "bs_drm.h"

#define HANDLE_EINTR(x)                                                  \
	({                                                               \
		int eintr_wrapper_counter = 0;                           \
		int eintr_wrapper_result;                                \
		do {                                                     \
			eintr_wrapper_result = (x);                      \
		} while (eintr_wrapper_result == -1 && errno == EINTR && \
			 eintr_wrapper_counter++ < 100);                 \
		eintr_wrapper_result;                                    \
	})

struct bs_map_info {
	size_t plane_index;
	void *ptr;
	void *map_data;
};

typedef void *(*bs_map_t)(struct bs_mapper *mapper, struct gbm_bo *bo, size_t plane,
			  struct bs_map_info *info);
typedef void (*bs_unmap_t)(struct gbm_bo *bo, struct bs_map_info *info);

struct bs_mapper {
	bs_map_t map_plane_fn;
	bs_unmap_t unmap_plane_fn;
	int device_fd;
};

static void *dma_buf_map(struct bs_mapper *mapper, struct gbm_bo *bo, size_t plane,
			 struct bs_map_info *info)
{
	int drm_prime_fd = gbm_bo_get_plane_fd(bo, plane);
	uint32_t handle = gbm_bo_get_plane_handle(bo, plane).u32;
	size_t length = 0;

	for (size_t p = 0; p <= plane; p++) {
		if (gbm_bo_get_plane_handle(bo, p).u32 == handle)
			length += gbm_bo_get_plane_size(bo, p);
	}

	void *ptr = mmap(NULL, length, (PROT_READ | PROT_WRITE), MAP_SHARED, drm_prime_fd, 0);
	if (ptr == MAP_FAILED) {
		bs_debug_error("dma-buf mmap returned MAP_FAILED: %d", errno);
		ptr = MAP_FAILED;
	} else {
		ptr += gbm_bo_get_plane_offset(bo, plane);
	}

	struct dma_buf_sync sync_start = { 0 };
	sync_start.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
	int ret = HANDLE_EINTR(ioctl(drm_prime_fd, DMA_BUF_IOCTL_SYNC, &sync_start));
	if (ret)
		bs_debug_error("DMA_BUF_IOCTL_SYNC failed");

	close(drm_prime_fd);
	return ptr;
}

static void dma_buf_unmap(struct gbm_bo *bo, struct bs_map_info *info)
{
	struct dma_buf_sync sync_end = { 0 };
	sync_end.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
	int drm_prime_fd = gbm_bo_get_plane_fd(bo, info->plane_index);
	int ret = HANDLE_EINTR(ioctl(drm_prime_fd, DMA_BUF_IOCTL_SYNC, &sync_end));
	close(drm_prime_fd);
	if (ret)
		bs_debug_error("DMA_BUF_IOCTL_SYNC failed");

	void *addr = info->ptr;
	addr -= gbm_bo_get_plane_offset(bo, info->plane_index);
	ret = munmap(addr, gbm_bo_get_plane_size(bo, info->plane_index));
	if (ret)
		bs_debug_error("dma-buf unmap failed.");
}

static void *gem_map(struct bs_mapper *mapper, struct gbm_bo *bo, size_t plane,
		     struct bs_map_info *info)
{
	uint32_t w = gbm_bo_get_width(bo);
	uint32_t h = gbm_bo_get_height(bo);
	uint32_t stride;
	void *ptr =
	    gbm_bo_map(bo, 0, 0, w, h, GBM_BO_TRANSFER_READ_WRITE, &stride, &info->map_data, plane);
	return ptr;
}

static void gem_unmap(struct gbm_bo *bo, struct bs_map_info *info)
{
	gbm_bo_unmap(bo, info->map_data);
}

static void *dumb_map(struct bs_mapper *mapper, struct gbm_bo *bo, size_t plane,
		      struct bs_map_info *info)
{
	if (plane) {
		bs_debug_error("dump map supports only single plane buffer.");
		return MAP_FAILED;
	}

	int prime_fd = gbm_bo_get_fd(bo);
	uint32_t size = gbm_bo_get_plane_size(bo, plane);

	uint32_t bo_handle;
	int ret = drmPrimeFDToHandle(mapper->device_fd, prime_fd, &bo_handle);
	if (ret) {
		bs_debug_error("dump map failed.");
		return MAP_FAILED;
	}

	struct drm_mode_map_dumb mmap_arg = { 0 };
	mmap_arg.handle = bo_handle;

	ret = drmIoctl(mapper->device_fd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
	if (ret) {
		bs_debug_error("failed DRM_IOCTL_MODE_MAP_DUMB: %d", ret);
		return MAP_FAILED;
	}

	if (mmap_arg.offset == 0) {
		bs_debug_error("DRM_IOCTL_MODE_MAP_DUMB returned 0 offset");
		return MAP_FAILED;
	}

	void *ptr = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED, mapper->device_fd,
			 mmap_arg.offset);

	if (ptr == MAP_FAILED) {
		bs_debug_error("mmap returned MAP_FAILED: %d", errno);
		return MAP_FAILED;
	}

	close(prime_fd);
	return ptr;
}

static void dumb_unmap(struct gbm_bo *bo, struct bs_map_info *info)
{
	int ret = munmap(info->ptr, gbm_bo_get_plane_size(bo, info->plane_index));
	if (ret)
		bs_debug_error("dump unmap failed.");
}

struct bs_mapper *bs_mapper_dma_buf_new()
{
	struct bs_mapper *mapper = calloc(1, sizeof(struct bs_mapper));
	assert(mapper);
	mapper->map_plane_fn = dma_buf_map;
	mapper->unmap_plane_fn = dma_buf_unmap;
	mapper->device_fd = -1;
	return mapper;
}

struct bs_mapper *bs_mapper_gem_new()
{
	struct bs_mapper *mapper = calloc(1, sizeof(struct bs_mapper));
	assert(mapper);
	mapper->map_plane_fn = gem_map;
	mapper->unmap_plane_fn = gem_unmap;
	mapper->device_fd = -1;
	return mapper;
}

struct bs_mapper *bs_mapper_dumb_new(int device_fd)
{
	assert(device_fd >= 0);
	struct bs_mapper *mapper = calloc(1, sizeof(struct bs_mapper));
	assert(mapper);
	mapper->map_plane_fn = dumb_map;
	mapper->unmap_plane_fn = dumb_unmap;
	mapper->device_fd = dup(device_fd);
	assert(mapper->device_fd >= 0);
	return mapper;
}

void bs_mapper_destroy(struct bs_mapper *mapper)
{
	assert(mapper);
	if (mapper->device_fd >= 0)
		close(mapper->device_fd);

	free(mapper);
}

void *bs_mapper_map(struct bs_mapper *mapper, struct gbm_bo *bo, size_t plane, void **map_data)
{
	assert(mapper);
	assert(bo);
	assert(map_data);
	struct bs_map_info *info = calloc(1, sizeof(struct bs_map_info));
	info->plane_index = plane;
	void *ptr = mapper->map_plane_fn(mapper, bo, plane, info);
	if (ptr == MAP_FAILED) {
		free(info);
		return MAP_FAILED;
	}

	info->ptr = ptr;
	*map_data = info;
	return info->ptr;
}

void bs_mapper_unmap(struct bs_mapper *mapper, struct gbm_bo *bo, void *map_data)
{
	struct bs_map_info *info = map_data;
	assert(info);
	mapper->unmap_plane_fn(bo, info);
	free(info);
}
