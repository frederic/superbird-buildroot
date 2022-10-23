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
#include <stdbool.h>
#include <IONmem.h>

#include "gdc_api.h"

#define   FILE_NAME_GDC "/dev/gdc"

int gdc_create_ctx(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1;

	ctx->gdc_client = open(FILE_NAME_GDC, O_RDWR | O_SYNC);

	if (ctx->gdc_client < 0) {
		E_GDC("gdc open failed error=%d, %s",
			errno, strerror(errno));
		return -1;
	}
	ret = ion_mem_init();
	if (ret < 0) {
		E_GDC("ionmem init failed\n");
		return -1;
	}

	ctx->ion_fd = ret;

	return 0;
}

int gdc_destroy_ctx(struct gdc_usr_ctx_s *ctx)
{
	int i;

	if (ctx->c_buff != NULL) {
		munmap(ctx->c_buff, ctx->c_len);
		ctx->c_buff = NULL;
		ctx->c_len = 0;
	}
	if (ctx->gs_ex.config_buffer.shared_fd > 0) {
		D_GDC("close config_fd\n");
		close(ctx->gs_ex.config_buffer.shared_fd);
	}

	for (i = 0; i < ctx->plane_number; i++) {
		if (ctx->i_buff[i] != NULL) {
			munmap(ctx->i_buff[i], ctx->i_len[i]);
			ctx->i_buff[i] = NULL;
			ctx->i_len[i] = 0;
		}

		if (i == 0) {
			if (ctx->gs_ex.input_buffer.shared_fd > 0) {
				D_GDC("close in_fd=%d\n",
					ctx->gs_ex.input_buffer.shared_fd);
				close(ctx->gs_ex.input_buffer.shared_fd);
			}
		} else if (i == 1) {
			if (ctx->gs_ex.input_buffer.uv_base_fd > 0) {
				D_GDC("close in_fd=%d\n",
					ctx->gs_ex.input_buffer.uv_base_fd);
				close(ctx->gs_ex.input_buffer.uv_base_fd);
			}
		} else if (i == 2) {
			if (ctx->gs_ex.input_buffer.v_base_fd > 0) {
				D_GDC("close in_fd=%d\n",
					ctx->gs_ex.input_buffer.v_base_fd);
				close(ctx->gs_ex.input_buffer.v_base_fd);
			}
		}

		if (ctx->o_buff[i] != NULL) {
			munmap(ctx->o_buff[i], ctx->o_len[i]);
			ctx->o_buff[i] = NULL;
			ctx->o_len[i] = 0;
		}
		if (i == 0) {
			if (ctx->gs_ex.output_buffer.shared_fd > 0) {
				D_GDC("close out_fd=%d\n",
					ctx->gs_ex.output_buffer.shared_fd);
				close(ctx->gs_ex.output_buffer.shared_fd);
			}
		} else if (i == 1) {
			if (ctx->gs_ex.output_buffer.uv_base_fd > 0) {
				D_GDC("close out_fd=%d\n",
					ctx->gs_ex.output_buffer.uv_base_fd);
				close(ctx->gs_ex.output_buffer.uv_base_fd);
			}
		} else if (i == 2) {
			if (ctx->gs_ex.output_buffer.v_base_fd > 0) {
				D_GDC("close out_fd=%d\n",
					ctx->gs_ex.output_buffer.v_base_fd);
				close(ctx->gs_ex.output_buffer.v_base_fd);
			}
		}
	}
	if (ctx->gdc_client >= 0) {
		close(ctx->gdc_client);
		ctx->gdc_client = -1;
	}

	if (ctx->ion_fd >= 0) {
		ion_mem_exit(ctx->ion_fd);
		ctx->ion_fd = -1;
	}

	return 0;
}

static int _gdc_alloc_dma_buffer(int fd, unsigned int dir,
						unsigned int len)
{
	int ret = -1;
	struct gdc_dmabuf_req_s buf_cfg;

	memset(&buf_cfg, 0, sizeof(buf_cfg));

	buf_cfg.dma_dir = dir;
	buf_cfg.len = len;

	ret = ioctl(fd, GDC_REQUEST_DMA_BUFF, &buf_cfg);
	if (ret < 0) {
		E_GDC("GDC_REQUEST_DMA_BUFF %s failed: %s, fd=%d\n", __func__,
			strerror(ret), fd);
		return ret;
	}
	return buf_cfg.index;
}

static int _gdc_get_dma_buffer_fd(int fd, int index)
{
	struct gdc_dmabuf_exp_s ex_buf;

	ex_buf.index = index;
	ex_buf.flags = O_RDWR;
	ex_buf.fd = -1;

	if (ioctl(fd, GDC_EXP_DMA_BUFF, &ex_buf)) {
		E_GDC("failed get dma buf fd\n");
		return -1;
	}
	D_GDC("dma buffer export, fd=%d\n", ex_buf.fd);

	return ex_buf.fd;
}

static int _gdc_free_dma_buffer(int fd, int index)
{
	if (ioctl(fd, GDC_FREE_DMA_BUFF, &index))  {
		E_GDC("failed free dma buf fd\n");
		return -1;
	}
	return 0;
}

static int set_buf_fd(gdc_alloc_buffer_t *buf, gdc_buffer_info_t *buf_info,
				int buf_fd, int plane_id)
{
	int ret = 0;

	D_GDC("buf_fd=%d, plane_id=%d, buf->format=%d\n", buf_fd,
				plane_id, buf->format);
	switch (buf->format) {
	case NV12:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			if (plane_id == 0)
				buf_info->y_base_fd = buf_fd;
			else if(plane_id == 1)
				buf_info->uv_base_fd = buf_fd;
			else {
				E_GDC("plane id(%d) error\n", plane_id);
				ret = -1;
			}
		}
		break;
	case YV12:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			if (plane_id == 0)
				buf_info->y_base_fd = buf_fd;
			else if (plane_id == 1)
				buf_info->u_base_fd = buf_fd;
			else if (plane_id == 2)
				buf_info->v_base_fd = buf_fd;
			else {
				E_GDC("plane id(%d) error\n", plane_id);
				ret = -1;
			}
		}
		break;
	case Y_GREY:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			E_GDC("plane id(%d) error\n", plane_id);
			ret = -1;
		}
		break;
	case YUV444_P:
	case RGB444_P:
		if (buf->plane_number == 1) {
			buf_info->shared_fd = buf_fd;
		} else {
			if (plane_id == 0)
			buf_info->y_base_fd = buf_fd;
			else if (plane_id == 1)
				buf_info->u_base_fd = buf_fd;
			else if (plane_id == 2)
				buf_info->v_base_fd = buf_fd;
			else {
				E_GDC("plane id(%d) error\n", plane_id);
				ret = -1;
			}
		}
		break;
	default:
		return -1;
		break;
	}
	D_GDC("buf_info->shared_fd=%d\n",buf_info->shared_fd);
	D_GDC("buf_info->y_base_fd=%d\n",buf_info->y_base_fd);
	D_GDC("buf_info->uv_base_fd=%d\n",buf_info->uv_base_fd);
	return ret;
}

int gdc_alloc_buffer (struct gdc_usr_ctx_s *ctx, uint32_t type,
			struct gdc_alloc_buffer_s *buf, bool cache_flag)
{
	int dir = 0, index = -1, buf_fd[MAX_PLANE] = {-1};
	struct gdc_dmabuf_req_s buf_cfg;
	struct gdc_settings_ex *gs_ex;
	int i;

	gs_ex = &(ctx->gs_ex);
	memset(&buf_cfg, 0, sizeof(buf_cfg));
	if (type == OUTPUT_BUFF_TYPE)
		dir = DMA_FROM_DEVICE;
	else
		dir = DMA_TO_DEVICE;

	for (i = 0; i < buf->plane_number; i++) {
		buf_cfg.len = buf->len[i];
		buf_cfg.dma_dir = dir;

		if (ctx->mem_type == AML_GDC_MEM_ION) {
			int ret = -1;
			IONMEM_AllocParams ion_alloc_params;

			ret = ion_mem_alloc(ctx->ion_fd, buf->len[i], &ion_alloc_params,
						cache_flag);
			if (ret < 0) {
				E_GDC("%s,%d,Not enough memory\n",__func__,
					__LINE__);
				return -1;
			}
			buf_fd[i] = ion_alloc_params.mImageFd;
			D_GDC("gdc_alloc_buffer: ion_fd=%d\n", buf_fd[i]);
		} else if (ctx->mem_type == AML_GDC_MEM_DMABUF) {
			index = _gdc_alloc_dma_buffer(ctx->gdc_client, dir,
							buf_cfg.len);
			if (index < 0)
				return -1;

			/* get  dma fd*/
			buf_fd[i] = _gdc_get_dma_buffer_fd(ctx->gdc_client,
								index);
			if (buf_fd[i] < 0)
				return -1;
			D_GDC("gdc_alloc_buffer: dma_fd=%d\n", buf_fd[i]);
			/* after alloc, dmabuffer free can be called, it just dec refcount */
			_gdc_free_dma_buffer(ctx->gdc_client, index);
		} else {
			E_GDC("gdc_alloc_buffer: wrong mem_type\n");
			return -1;
		}

		switch (type) {
		case INPUT_BUFF_TYPE:
			ctx->i_len[i] = buf->len[i];
			ctx->i_buff[i] = mmap(NULL, buf->len[i],
				PROT_READ | PROT_WRITE,
				MAP_SHARED, buf_fd[i], 0);
			if (ctx->i_buff[i] == MAP_FAILED) {
				ctx->i_buff[i] = NULL;
				E_GDC("Failed to alloc i_buff:%s\n",
					strerror(errno));
			}
			set_buf_fd(buf, &(gs_ex->input_buffer) ,buf_fd[i], i);
			gs_ex->input_buffer.plane_number = buf->plane_number;
			gs_ex->input_buffer.mem_alloc_type = ctx->mem_type;
			break;
		case OUTPUT_BUFF_TYPE:
			ctx->o_len[i] = buf->len[i];
			ctx->o_buff[i] = mmap(NULL, buf->len[i],
				PROT_READ | PROT_WRITE,
				MAP_SHARED, buf_fd[i], 0);
			if (ctx->o_buff[i] == MAP_FAILED) {
				ctx->o_buff[i] = NULL;
				E_GDC("Failed to alloc o_buff:%s\n",
						strerror(errno));
			}
			set_buf_fd(buf, &(gs_ex->output_buffer),buf_fd[i], i);
			gs_ex->output_buffer.plane_number = buf->plane_number;
			gs_ex->output_buffer.mem_alloc_type = ctx->mem_type;
			break;
		case CONFIG_BUFF_TYPE:
			if (i > 0)
				break;
			ctx->c_len = buf->len[i];
			ctx->c_buff = mmap(NULL, buf->len[i],
				PROT_READ | PROT_WRITE,
				MAP_SHARED, buf_fd[i], 0);
			if (ctx->c_buff == MAP_FAILED) {
				ctx->c_buff = NULL;
				E_GDC("Failed to alloc c_buff:%s\n",
						strerror(errno));
			}
			gs_ex->config_buffer.shared_fd = buf_fd[i];
			gs_ex->config_buffer.plane_number = 1;
			gs_ex->config_buffer.mem_alloc_type = ctx->mem_type;
			break;
		default:
			E_GDC("Error no such buff type\n");
			break;
		}
	}
	return 0;
}

int gdc_process(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1;
	struct gdc_settings_ex *gs_ex = &ctx->gs_ex;

	ret = gdc_sync_for_device(ctx);
	if (ret < 0)
		return ret;

	ret = ioctl(ctx->gdc_client, GDC_PROCESS_EX, gs_ex);
	if (ret < 0) {
		E_GDC("GDC_RUN ioctl failed\n");
		return ret;
	}

	ret = gdc_sync_for_cpu(ctx);
	if (ret < 0)
		return ret;

	return 0;
}

int gdc_process_with_builtin_fw(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1;
	struct gdc_settings_with_fw *gs_with_fw = &ctx->gs_with_fw;

	ret = gdc_sync_for_device(ctx);
	if (ret < 0)
		return ret;

	ret = ioctl(ctx->gdc_client, GDC_PROCESS_WITH_FW, gs_with_fw);
	if (ret < 0) {
		E_GDC("GDC_PROCESS_WITH_FW ioctl failed\n");
		return ret;
	}
	ret = gdc_sync_for_cpu(ctx);
	if (ret < 0)
		return ret;

	return 0;
}

int gdc_sync_for_device(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1, i;
	int shared_fd[MAX_PLANE];
	int plane_number = 0;
	struct gdc_settings_ex *gs_ex = &ctx->gs_ex;

	if (gs_ex->input_buffer.mem_alloc_type == AML_GDC_MEM_DMABUF) {
		plane_number = gs_ex->input_buffer.plane_number;
		for (i = 0; i < plane_number; i++) {
			if (i == 0)
				shared_fd[i] = gs_ex->input_buffer.shared_fd;
			else if (i == 1)
				shared_fd[i] = gs_ex->input_buffer.uv_base_fd;
			else if (i == 2)
				shared_fd[i] = gs_ex->input_buffer.v_base_fd;
			ret = ioctl(ctx->gdc_client, GDC_SYNC_DEVICE,
					&shared_fd[i]);
			if (ret < 0) {
				E_GDC("gdc_sync_for_device ioctl failed\n");
				return ret;
			}
		}
	}
	if (!ctx->custom_fw &&
		gs_ex->config_buffer.mem_alloc_type == AML_GDC_MEM_DMABUF) {
		shared_fd[0] = gs_ex->config_buffer.shared_fd;
		ret = ioctl(ctx->gdc_client, GDC_SYNC_DEVICE,
				&shared_fd[0]);
		if (ret < 0) {
			E_GDC("gdc_sync_for_device ioctl failed\n");
			return ret;
		}
	}

	return 0;
}

int gdc_sync_for_cpu(struct gdc_usr_ctx_s *ctx)
{
	int ret = -1, i;
	int shared_fd[MAX_PLANE];
	int plane_number = 0;
	struct gdc_settings_ex *gs_ex = &ctx->gs_ex;

	if (gs_ex->output_buffer.mem_alloc_type == AML_GDC_MEM_DMABUF) {
		plane_number = gs_ex->output_buffer.plane_number;
		for (i = 0; i < plane_number; i++) {
			if (i == 0)
				shared_fd[i] = gs_ex->output_buffer.shared_fd;
			else if (i == 1)
				shared_fd[i] = gs_ex->output_buffer.uv_base_fd;
			else if (i == 2)
				shared_fd[i] = gs_ex->output_buffer.v_base_fd;
			ret = ioctl(ctx->gdc_client, GDC_SYNC_CPU,
					&shared_fd[i]);
			if (ret < 0) {
				E_GDC("gdc_sync_for_cpu ioctl failed\n");
				return ret;
			}
		}
	} else {
		plane_number = gs_ex->output_buffer.plane_number;
		for (i = 0; i < plane_number; i++) {
			if (i == 0)
				ion_mem_invalid_cache(ctx->ion_fd,
						gs_ex->output_buffer.shared_fd);
			else if (i == 1)
				ion_mem_invalid_cache(ctx->ion_fd,
						gs_ex->output_buffer.uv_base_fd);
			else if (i == 2)
				ion_mem_invalid_cache(ctx->ion_fd,
						gs_ex->output_buffer.v_base_fd);
		}
	}

	return 0;
}
