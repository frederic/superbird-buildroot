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

#include "gdc_api.h"

int gdc_create_ctx(struct gdc_usr_ctx_s *ctx)
{
    ctx->gdc_client = open("/dev/gdc", O_RDWR);

    if (ctx->gdc_client < 0)
        printf("gdc open failed error=%d, %s",
                errno, strerror(errno));

    return 0;
}

int gdc_destroy_ctx(struct gdc_usr_ctx_s *ctx)
{
	if (ctx->i_buff != NULL) {
		munmap(ctx->i_buff, ctx->i_len);
		ctx->i_buff = NULL;
		ctx->i_len = 0;
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

int gdc_init(struct gdc_usr_ctx_s *ctx, struct gdc_settings *gs)
{
    memcpy (&ctx->gs, gs, sizeof(*gs));
    return 0;
}

int gdc_alloc_dma_buffer (struct gdc_usr_ctx_s *ctx,
        uint32_t type, size_t len)
{
	int ret = -1;
	struct gdc_buf_cfg buf_cfg;

	memset(&buf_cfg, 0, sizeof(buf_cfg));

	buf_cfg.type = type;
	buf_cfg.len = len;

	ret = ioctl(ctx->gdc_client, GDC_REQUEST_BUFF, &buf_cfg);
    if (ret < 0) {
        printf("%s failed: %s\n", __func__, strerror(ret));
		return ret;
    }

	switch (type) {
		case INPUT_BUFF_TYPE:
			ctx->i_len = len;
			ctx->i_buff = mmap(NULL, len,
								PROT_READ | PROT_WRITE,
								MAP_SHARED, ctx->gdc_client, 0);
			if (ctx->i_buff == MAP_FAILED) {
				ctx->i_buff = NULL;
				printf("Failed to alloc i_buff:%s\n", strerror(errno));
			}
		break;
		case OUTPUT_BUFF_TYPE:
			ctx->o_len = len;
			ctx->o_buff = mmap(NULL, len,
								PROT_READ | PROT_WRITE,
								MAP_SHARED, ctx->gdc_client, 0);
			if (ctx->o_buff == MAP_FAILED) {
				ctx->o_buff = NULL;
				printf("Failed to alloc o_buff:%s\n", strerror(errno));
			}
		break;
		case CONFIG_BUFF_TYPE:
			ctx->c_len = len;
			ctx->c_buff = mmap(NULL, len,
								PROT_READ | PROT_WRITE,
								MAP_SHARED, ctx->gdc_client, 0);
			if (ctx->c_buff == MAP_FAILED) {
				ctx->c_buff = NULL;
				printf("Failed to alloc c_buff:%s\n", strerror(errno));
			}
		break;

		default:
			printf("Error no such buff type\n");
			break;
	}

    return ret;
}

int gdc_process(struct gdc_usr_ctx_s *ctx)
{
    int ret = 0;
    struct gdc_settings *gs = &ctx->gs;

    ret = ioctl(ctx->gdc_client, GDC_RUN, gs);
    if (ret < 0)
        printf("GDC_PROCESS ioctl failed\n");

    return ret;
}

int gdc_handle(struct gdc_usr_ctx_s *ctx)
{
    int ret = 0;
    struct gdc_settings *gs = &ctx->gs;

    ret = ioctl(ctx->gdc_client, GDC_HANDLE, gs);
    if (ret < 0)
        printf("GDC_HANDLE ioctl failed\n");

    return ret;
}
