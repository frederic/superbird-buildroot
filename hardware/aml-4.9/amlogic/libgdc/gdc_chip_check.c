/*
 *   Copyright 2018 Amlogic, Inc
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
#include <test_data.h>
#include <IONmem.h>

#include "gdc_api.h"
#define TEST_NUM 3

struct gdc_param {
	uint32_t i_width;
	uint32_t i_height;
	uint32_t o_width;
	uint32_t o_height;
	uint32_t format;
};

int compare_data(char *data1, char *data2, int size)
{
	int i = 0, err_hit = 0;
	int thresh_hold = 1;

	if (!data1 || !data2 || !size) {
		E_GDC("cmp data, para err\n");
		return -1;
	}
	for (i = 0; i < size; i++) {
		if (*data1 != *data2) {
			E_GDC("data1=%x,data2=%x\n",*data1,*data2);
			err_hit++;
		}
		data1++;
		data2++;
		if (err_hit > thresh_hold) {
			E_GDC("bad chip found,err_hit=%d\n",err_hit);
			return -1;
		}
	}
	return 0;
}

static int gdc_set_config_param(struct gdc_usr_ctx_s *ctx,
				char *config, int len)
{
	if (config == NULL || ctx == NULL || ctx->c_buff == NULL) {
		E_GDC("Error input param\n");
		return -1;
	}

	memcpy(ctx->c_buff, config, len);

	return 0;
}

static int gdc_set_input_image(struct gdc_usr_ctx_s *ctx,
				   char *input, int len)
{
	int i;

	if (input == NULL || ctx == NULL) {
		E_GDC("Error input param\n");
		return -1;
	}

	for (i = 0; i < ctx->plane_number; i++) {
		if (ctx->i_buff[i] == NULL || ctx->i_len[i] == 0) {
			D_GDC("Error input param, plane_id=%d\n", i);
			return -1;
		}
		memcpy(ctx->i_buff[i], input, ctx->i_len[i]);
		input += ctx->i_len[i];
	}

	return 0;
}

#ifdef DUMP_OUTPUT
static void save_imgae(struct gdc_usr_ctx_s *ctx, char *file_name)
{
	FILE *fp = NULL;
	int i;

	if (ctx == NULL || file_name == NULL) {
		E_GDC("%s:wrong paras\n", __func__);
		return;
	}
	fp = fopen(file_name, "wb");
	if (fp == NULL) {
		E_GDC("%s:Error open file\n", __func__);
		return;
	}
	for (i = 0; i < ctx->plane_number; i++) {
		if (ctx->o_buff[i] == NULL || ctx->o_len[i] == 0) {
			E_GDC("%s:Error input param\n", __func__);
			break;
		}
		D_GDC("gdc: 0x%2x, 0x%2x,0x%2x,0x%2x, 0x%2x,0x%2x,0x%2x,0x%2x\n",
			ctx->o_buff[i][0],
			ctx->o_buff[i][1],
			ctx->o_buff[i][2],
			ctx->o_buff[i][3],
			ctx->o_buff[i][4],
			ctx->o_buff[i][5],
			ctx->o_buff[i][6],
			ctx->o_buff[i][7]);

		fwrite(ctx->o_buff[i], ctx->o_len[i], 1, fp);
	}
	fclose(fp);
}
#endif

static int gdc_init_cfg(struct gdc_usr_ctx_s *ctx, struct gdc_param *tparm,
				char *f_name)
{
	struct gdc_settings_ex *gdc_gs = NULL;
	int ret = -1;
	uint32_t format = 0;
	uint32_t i_width = 0;
	uint32_t i_height = 0;
	uint32_t o_width = 0;
	uint32_t o_height = 0;
	uint32_t i_y_stride = 0;
	uint32_t i_c_stride = 0;
	uint32_t o_y_stride = 0;
	uint32_t o_c_stride = 0;
	uint32_t i_len = 0;
	uint32_t o_len = 0;
	uint32_t c_len = 0;
	int plane_number = 1;
	gdc_alloc_buffer_t buf;

	if (ctx == NULL || tparm == NULL || f_name == NULL) {
		E_GDC("Error invalid input param\n");
		return ret;
	}

	plane_number = ctx->plane_number;
	i_width = tparm->i_width;
	i_height = tparm->i_height;
	o_width = tparm->o_width;
	o_height = tparm->o_height;

	format = tparm->format;

	i_y_stride = AXI_WORD_ALIGN(i_width);
	o_y_stride = AXI_WORD_ALIGN(o_width);

	if (format == NV12 || format == YUV444_P || format == RGB444_P) {
		i_c_stride = AXI_WORD_ALIGN(i_width);
		o_c_stride = AXI_WORD_ALIGN(o_width);
	} else if (format == YV12) {
		i_c_stride = AXI_WORD_ALIGN(i_width) / 2;
		o_c_stride = AXI_WORD_ALIGN(o_width) / 2;
	} else if (format == Y_GREY) {
		i_c_stride = 0;
		o_c_stride = 0;
	} else {
		E_GDC("Error unknow format\n");
		return ret;
	}

	gdc_gs = &ctx->gs_ex;

	gdc_gs->gdc_config.input_width = i_width;
	gdc_gs->gdc_config.input_height = i_height;
	gdc_gs->gdc_config.input_y_stride = i_y_stride;
	gdc_gs->gdc_config.input_c_stride = i_c_stride;
	gdc_gs->gdc_config.output_width = o_width;
	gdc_gs->gdc_config.output_height = o_height;
	gdc_gs->gdc_config.output_y_stride = o_y_stride;
	gdc_gs->gdc_config.output_c_stride = o_c_stride;
	gdc_gs->gdc_config.format = format;
	gdc_gs->magic = sizeof(*gdc_gs);

	buf.format = format;

	ret = gdc_create_ctx(ctx);
	if (ret < 0)
		return -1;

	if (!ctx->custom_fw) {
		c_len = sizeof(config_data);
		if (c_len <= 0) {
			gdc_destroy_ctx(ctx);
			E_GDC("Error gdc config file size\n");
			return ret;
		}

		buf.plane_number = 1;
		buf.len[0] = c_len;
		ret = gdc_alloc_buffer(ctx, CONFIG_BUFF_TYPE, &buf, false);
		if (ret < 0) {
			gdc_destroy_ctx(ctx);
			E_GDC("Error alloc gdc cfg buff\n");
			return ret;
		}

		ret = gdc_set_config_param(ctx, f_name, c_len);
		if (ret < 0) {
			gdc_destroy_ctx(ctx);
			E_GDC("Error cfg gdc param buff\n");
			return ret;
		}

		gdc_gs->gdc_config.config_size = c_len / 4;
	}
	buf.plane_number = plane_number;
	if ((plane_number == 1) || (format == Y_GREY)) {
		if (format == RGB444_P || format == YUV444_P)
			i_len = i_y_stride * i_height * 3;
		else if (format == NV12 || format == YV12)
			i_len = i_y_stride * i_height * 3 / 2;
		else if (format == Y_GREY)
			i_len = i_y_stride * i_height;
		buf.plane_number = 1;
		buf.len[0] = i_len;
	} else if ((plane_number == 2) && (format == NV12)) {
		buf.len[0] = i_y_stride * i_height;
		buf.len[1] = i_y_stride * i_height / 2;
	} else if ((plane_number == 3) &&
		(format == YV12 ||
		(format == YUV444_P) ||
		(format == RGB444_P))) {
		buf.len[0] = i_y_stride * i_height;
		if (format == YV12) {
			buf.len[1] = i_y_stride * i_height / 4;
			buf.len[2] = i_y_stride * i_height / 4;
		} else if ((format == YUV444_P) ||
			(format == RGB444_P)) {
			buf.len[1] = i_y_stride * i_height;
			buf.len[2] = i_y_stride * i_height;
		}
	}
	ret = gdc_alloc_buffer(ctx, INPUT_BUFF_TYPE, &buf, false);
	if (ret < 0) {
		gdc_destroy_ctx(ctx);
		E_GDC("Error alloc gdc input buff\n");
		return ret;
	}

	buf.plane_number = plane_number;
	if ((plane_number == 1) || (format == Y_GREY)) {
		if (format == RGB444_P || format == YUV444_P)
			o_len = o_y_stride * o_height * 3;
		else if (format == NV12 || format == YV12)
			o_len = o_y_stride * o_height * 3 / 2;
		else if (format == Y_GREY)
			o_len = o_y_stride * o_height;
		buf.plane_number = 1;
		buf.len[0] = o_len;
	} else if ((plane_number == 2) && (format == NV12)) {
		buf.len[0] = o_y_stride * o_height;
		buf.len[1] = o_y_stride * o_height / 2;
	} else if ((plane_number == 3) &&
		(format == YV12 ||
		(format == YUV444_P) ||
		(format == RGB444_P))) {
		buf.len[0] = o_y_stride * o_height;
		if (format == YV12) {
			buf.len[1] = o_y_stride * o_height / 4;
			buf.len[2] = o_y_stride * o_height / 4;
		} else if ((format == YUV444_P) ||
			(format == RGB444_P)) {
			buf.len[1] = o_y_stride * o_height;
			buf.len[2] = o_y_stride * o_height;
		}
	}

	ret = gdc_alloc_buffer(ctx, OUTPUT_BUFF_TYPE, &buf, true);
	if (ret < 0) {
		gdc_destroy_ctx(ctx);
		E_GDC("Error alloc gdc input buff\n");
		return ret;
	}
	return ret;
}

static int check_plane_number(int plane_number, int format)
{
	int ret = -1;

	if (plane_number == 1) {
		ret = 0;
	} else if (plane_number == 2) {
		if (format == NV12)
		ret = 0;
	} else if (plane_number == 3) {
		if (format == YV12 ||
			(format == YUV444_P)  ||
			(format == RGB444_P))
		ret = 0;
	}
	return ret;
}

int main(int argc, char* argv[]) {
	int c;
	int ret = 0;
	struct gdc_usr_ctx_s ctx;
	uint32_t format;
	uint32_t in_width;
	uint32_t in_height;
	uint32_t out_width;
	uint32_t out_height;
	int plane_number, mem_type;
	char *output_cmp = (char *)output_golden;
	struct gdc_param g_param;
	int i = 0, len = 0, test_cnt = 0, err_cnt = 0;
	int is_custom_fw;

	/* set params */
	is_custom_fw = 0;
	format = 1;
	plane_number = 1;
	in_width = 640;
	in_height = 480;
	out_width = 640;
	out_height = 480;
	mem_type = 0;
	/* set params end */

	ret = check_plane_number(plane_number, format);
	if (ret < 0) {
		E_GDC("Error plane_number=[%d]\n", plane_number);
		return -1;
	}
	g_param.i_width = in_width;
	g_param.i_height = in_height;
	g_param.o_width = out_width;
	g_param.o_height = out_height;
	g_param.format = format;

	memset(&ctx, 0, sizeof(ctx));
	ctx.custom_fw = is_custom_fw;
	ctx.mem_type = mem_type;
	ctx.plane_number = plane_number;

	ret = gdc_init_cfg(&ctx, &g_param, (char *)config_data);
	if (ret < 0) {
		E_GDC("Error gdc init\n");
		gdc_destroy_ctx(&ctx);
		return -1;
	}

	len = sizeof(input_image);

	printf("Start GDC chip check...\n");
	for (test_cnt = 0; test_cnt < TEST_NUM; test_cnt++) {
		gdc_set_input_image(&ctx, (char *)input_image, len);
		ret = gdc_process(&ctx);
		if (ret < 0) {
			E_GDC("ioctl failed\n");
			gdc_destroy_ctx(&ctx);
			return ret;
		}

		ion_mem_invalid_cache(ctx.ion_fd,
			ctx.gs_ex.output_buffer.shared_fd);
		ret = compare_data(output_cmp, ctx.o_buff[i],
				ctx.o_len[i]);
		if (ret < 0)
			err_cnt++;
	}
	if (err_cnt >= TEST_NUM)
		printf("===gdc_slt_test:failed===\n");
	else
		printf("===gdc_slt_test:pass===\n");

#ifdef DUMP_OUTPUT
	{
		char *output_file = "save_output_file.raw";
		save_imgae(&ctx, output_file);
		D_GDC("Success save output image\n");
	}
#endif
	gdc_destroy_ctx(&ctx);

	return 0;
}
