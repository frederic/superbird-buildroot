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
#include <sys/time.h>
#include <pthread.h>

#include "gdc_api.h"

struct gdc_param {
	uint32_t i_width;
	uint32_t i_height;
	uint32_t o_width;
	uint32_t o_height;
	uint32_t format;
};

static int get_file_size(char *f_name)
{
	int f_size = -1;
	FILE *fp = NULL;

	if (f_name == NULL) {
		E_GDC("Error file name\n");
		return f_size;
	}

	fp = fopen(f_name, "rb");
	if (fp == NULL) {
		E_GDC("Error open file %s\n", f_name);
		return f_size;
	}

	fseek(fp, 0, SEEK_END);

	f_size = ftell(fp);

	fclose(fp);

	D_GDC("%s: size %d\n", f_name, f_size);

	return f_size;
}

static int gdc_set_config_param(struct gdc_usr_ctx_s *ctx,
				char *f_name, int len)
{
	FILE *fp = NULL;
	int r_size = -1;

	if (f_name == NULL || ctx == NULL || ctx->c_buff == NULL) {
		E_GDC("Error input param\n");
		return r_size;
	}

	fp = fopen(f_name, "rb");
	if (fp == NULL) {
		E_GDC("Error open file %s\n", f_name);
		return -1;
	}

	r_size = fread(ctx->c_buff, len, 1, fp);
	if (r_size <= 0)
		E_GDC("Failed to read file %s\n", f_name);

	fclose(fp);

	return r_size;
}

static int gdc_set_input_image(struct gdc_usr_ctx_s *ctx,
					char *f_name, int len)
{
	FILE *fp = NULL;
	int r_size = -1;
	int i;

	if (f_name == NULL || ctx == NULL) {
		E_GDC("Error input param\n");
		return r_size;
	}

	fp = fopen(f_name, "rb");
	if (fp == NULL) {
		E_GDC("Error open file %s\n", f_name);
		return -1;
	}

	for (i = 0; i < ctx->plane_number; i++) {
		if (ctx->i_buff[i] == NULL || ctx->i_len[i] == 0) {
			D_GDC("Error input param, plane_id=%d\n", i);
			return r_size;
		}
		r_size = fread(ctx->i_buff[i], ctx->i_len[i], 1, fp);
		if (r_size <= 0) {
			E_GDC("Failed to read file %s\n", f_name);
			return -1;
		}
	}
	fclose(fp);

	return r_size;
}

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
		c_len = get_file_size(f_name);
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

static int parse_custom_fw(struct gdc_usr_ctx_s *ctx, char *config_file)
{
	int ret = -1;
	struct fw_input_info_s *in = &ctx->gs_with_fw.fw_info.fw_input_info;
	struct fw_output_info_s *out = &ctx->gs_with_fw.fw_info.fw_output_info;
	union transform_u *trans =
				&ctx->gs_with_fw.fw_info.fw_output_info.trans;
	char gdc_format[8] = {};
	char custom_name[64] = {};

	ret = sscanf(config_file, "equisolid-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%[^-]-%[^_]_%[^_]_%d-%[^.].bin",
		&in->with, &in->height,
		&in->fov, &in->diameter,
		&in->offsetX, &in->offsetY,
		&out->offsetX, &out->offsetY,
		&out->width, &out->height,
		&out->pan, &out->tilt, out->zoom,
		trans->fw_equisolid.strengthX,
		trans->fw_equisolid.strengthY,
		&trans->fw_equisolid.rotation,
		gdc_format);
	ctx->gs_with_fw.fw_info.fw_type = EQUISOLID;

	if (ret > 0)
		D_GDC("equisolid-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%s-%s_%s_%d-%s.bin\n",
			in->with, in->height,
			in->fov, in->diameter,
			in->offsetX, in->offsetY,
			out->offsetX, out->offsetY,
			out->width, out->height,
			out->pan, out->tilt, out->zoom,
			trans->fw_equisolid.strengthX,
			trans->fw_equisolid.strengthY,
			trans->fw_equisolid.rotation,
			gdc_format);

	if (ret <= 0) {
		ret = sscanf(config_file, "cylinder-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%[^-]-%[^_]_%d-%[^.].bin",
			&in->with, &in->height,
			&in->fov, &in->diameter,
			&in->offsetX, &in->offsetY,
			&out->offsetX, &out->offsetY,
			&out->width, &out->height,
			&out->pan, &out->tilt, out->zoom,
			trans->fw_cylinder.strength,
			&trans->fw_cylinder.rotation,
			gdc_format);
		ctx->gs_with_fw.fw_info.fw_type = CYLINDER;

		if (ret > 0)
			D_GDC("cindif: cylinder-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%s-%s_%d-%s.bin\n",
				in->with, in->height,
				in->fov, in->diameter,
				in->offsetX, in->offsetY,
				out->offsetX, out->offsetY,
				out->width, out->height,
				out->pan, out->tilt, out->zoom,
				trans->fw_cylinder.strength,
				trans->fw_cylinder.rotation,
				gdc_format);
	}

	if (ret <= 0) {
		ret = sscanf(config_file, "equidistant-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%[^-]-%[^_]_%d_%d_%d_%d_%d_%d_%d-%[^.].bin",
			&in->with, &in->height,
			&in->fov, &in->diameter,
			&in->offsetX, &in->offsetY,
			&out->offsetX, &out->offsetY,
			&out->width, &out->height,
			&out->pan, &out->tilt, out->zoom,
			trans->fw_equidistant.azimuth,
			&trans->fw_equidistant.elevation,
			&trans->fw_equidistant.rotation,
			&trans->fw_equidistant.fov_width,
			&trans->fw_equidistant.fov_height,
			(int *)&trans->fw_equidistant.keep_ratio,
			&trans->fw_equidistant.cylindricityX,
			&trans->fw_equidistant.cylindricityY,
			gdc_format);
		ctx->gs_with_fw.fw_info.fw_type = EQUIDISTANT;

		if (ret > 0)
			D_GDC("equidistant-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%s-%s_%d_%d_%d_%d_%d_%d_%d-%s.bin\n",
				in->with, in->height,
				in->fov, in->diameter,
				in->offsetX, in->offsetY,
				out->offsetX, out->offsetY,
				out->width, out->height,
				out->pan, out->tilt, out->zoom,
				trans->fw_equidistant.azimuth,
				trans->fw_equidistant.elevation,
				trans->fw_equidistant.rotation,
				trans->fw_equidistant.fov_width,
				trans->fw_equidistant.fov_height,
				trans->fw_equidistant.keep_ratio,
				trans->fw_equidistant.cylindricityX,
				trans->fw_equidistant.cylindricityY,
				gdc_format);
	}
	if (ret <= 0) {
		trans->fw_custom.fw_name = custom_name;
		ret = sscanf(config_file, "custom-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%[^-]-%[^-]-%[^.].bin",
			&in->with, &in->height,
			&in->fov, &in->diameter,
			&in->offsetX, &in->offsetY,
			&out->offsetX, &out->offsetY,
			&out->width, &out->height,
			&out->pan, &out->tilt, out->zoom,
			trans->fw_custom.fw_name,
			gdc_format);
		ctx->gs_with_fw.fw_info.fw_type = CUSTOM;

		if (ret > 0)
			D_GDC("custom-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%s-%s-%s.bin\n",
				in->with, in->height,
				in->fov, in->diameter,
				in->offsetX, in->offsetY,
				out->offsetX, out->offsetY,
				out->width, out->height,
				out->pan, out->tilt, out->zoom,
				trans->fw_custom.fw_name,
				gdc_format);
	}
	if (ret <= 0) {
		ret = sscanf(config_file, "affine-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%[^-]-%d-%[^.].bin",
			&in->with, &in->height,
			&in->fov, &in->diameter,
			&in->offsetX, &in->offsetY,
			&out->offsetX, &out->offsetY,
			&out->width, &out->height,
			&out->pan, &out->tilt, out->zoom,
			&trans->fw_affine.rotation, gdc_format);

	ctx->gs_with_fw.fw_info.fw_type = AFFINE;

	if (ret > 0)
		D_GDC("affine-%d_%d_%d_%d_%d_%d-%d_%d_%d_%d-%d_%d_%s-%d-%s.bin\n",
			in->with, in->height,
			in->fov, in->diameter,
			in->offsetX, in->offsetY,
			out->offsetX, out->offsetY,
			out->width, out->height,
			out->pan, out->tilt, out->zoom,
			trans->fw_affine.rotation, gdc_format);
	}
	if (ret <= 0) {
		ctx->gs_with_fw.fw_info.fw_name = config_file;
		ctx->gs_with_fw.fw_info.fw_type = FW_TYPE_MAX;
	}


	return 0;
}

static inline unsigned long myclock()
{
	struct timeval tv;

	gettimeofday (&tv, NULL);

	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static uint32_t format;
static uint32_t in_width = 1920;
static uint32_t in_height = 1920;
static uint32_t out_width = 1920;
static uint32_t out_height = 1920;
static uint32_t in_y_stride = 0, in_c_stride = 0;
static uint32_t out_y_stride = 0, out_c_stride = 0;
static int num = 1, plane_number = 1, mem_type = 1;
static char *input_file = "input_file";
static char *output_file = "output_file";
static char *config_file = "config.bin";
static int is_custom_fw;
static int num_thread = 1;
static int num_process_per_thread = 1;

#define THREADS_MAX_NUM (64)

void *main_run(void *arg)
{
	int ret = 0, i = 0, j = 0, len = 0;
	struct gdc_usr_ctx_s ctx;
	struct gdc_param g_param;
	unsigned long stime;

	ret = check_plane_number(plane_number, format);
	if (ret < 0) {
		E_GDC("Error plane_number=[%d]\n", plane_number);
		return NULL;
	}

	g_param.i_width = in_width;
	g_param.i_height = in_height;
	g_param.o_width = out_width;
	g_param.o_height = out_height;
	g_param.format = format;

	for (i = 0; i < num_process_per_thread; i++) {
		printf("ThreadIdx -- %d, run time -- %d\n", *(int *)arg, i);
		memset(&ctx, 0, sizeof(ctx));
		ctx.custom_fw = is_custom_fw;
		ctx.mem_type = mem_type;
		ctx.plane_number = plane_number;

		ret = gdc_init_cfg(&ctx, &g_param, config_file);
		if (ret < 0) {
			E_GDC("Error gdc init\n");
			gdc_destroy_ctx(&ctx);
			return NULL;
		}

		len = get_file_size(input_file);

		gdc_set_input_image(&ctx, input_file, len);

		stime = myclock();

		for (j = 0; j < num; j++) {
			if (!ctx.custom_fw) {
				ret = gdc_process(&ctx);
				if (ret < 0) {
					E_GDC("ioctl failed\n");
					gdc_destroy_ctx(&ctx);
					return NULL;
				}
			} else {
				struct gdc_settings_with_fw *gdc_gw = &ctx.gs_with_fw;
				struct gdc_settings_ex *gdc_gs = &ctx.gs_ex;
				memcpy(&gdc_gw->input_buffer, &gdc_gs->input_buffer,
					sizeof(gdc_buffer_info_t));
				memcpy(&gdc_gw->output_buffer, &gdc_gs->output_buffer,
					sizeof(gdc_buffer_info_t));
				memcpy(&gdc_gw->gdc_config, &gdc_gs->gdc_config,
					sizeof(gdc_config_t));
				parse_custom_fw(&ctx, config_file);
				ret = gdc_process_with_builtin_fw(&ctx);
				if (ret < 0) {
					E_GDC("ioctl failed\n");
					gdc_destroy_ctx(&ctx);
					return NULL;
				}
			}
		}
		printf("time=%ld ms\n", myclock() - stime);

		save_imgae(&ctx, output_file);
		gdc_destroy_ctx(&ctx);
	}
	return NULL;
}

static void print_usage(void)
{
	int i;
	printf ("Usage: gdc_test [options]\n\n");
	printf ("Options:\n\n");
	printf ("  -h                      Print usage information.\n");
	printf ("  -c <string>             config file name.\n");
	printf ("  -t <num>                use custom_fw or not.\n");
	printf ("  -f <num>                format. 1:NV12,2:YV12,3:Y_GREY,4:YUV444_P,5:RGB444_P,\n");
	printf ("  -i <string>             input file name.\n");
	printf ("  -o <string>             output file name.\n");
	printf ("  -p <num>                image plane num.\n");
	printf ("  -w <width x height>     image width x height.\n");
	printf ("  -n <num>                num of process time.\n");
	printf ("  -m <num>                memtype. 0:ION,1:DMABUF.\n");
	printf ("  -l <num>                num of threads.\n");
	printf ("  -r <num>                run time for every thread.\n");
	printf ("\n");
}

int main(int argc, char **argv)
{
	int ret = -1, i, c;

	pthread_t thread[THREADS_MAX_NUM];
	int threadindex[THREADS_MAX_NUM];

	if (num_thread > THREADS_MAX_NUM) {
		E_GDC("num of thread greater than THREADS_MAX_NUM\n");
		return -1;
	}
	memset(&thread, 0, sizeof(thread));

	/* parse command line */
	while (1) {
		static struct option opts[] = {
			{"help", no_argument, 0, 'h'},
			{"config_file", required_argument, 0, 'c'},
			{"custom_fw", required_argument, 0, 't'},
			{"format", required_argument, 0, 'f'},
			{"input_file", required_argument, 0, 'i'},
			{"output_file", required_argument, 0, 'o'},
			{"plane_num", required_argument, 0, 'p'},
			{"stride", required_argument, 0, 's'},
			{"width x height", required_argument, 0, 'w'},
			{"num_of_iter", required_argument, 0, 'n'},
			{"memory_type", required_argument, 0, 'm'},
			{"num of threads", required_argument, 0, 'l'},
			{"run circles in thread", required_argument, 0, 'r'}
		};
		int i = 0;
		c = getopt_long(argc, argv, "hc:t:f:h:i:o:p:s:w:n:m:l:r:", opts, &i);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_usage();
			return 0;
		case 'c':
			is_custom_fw = 0;
			config_file = optarg;
			D_GDC("config_file: %s\n", config_file);
			break;
		case 't':
			is_custom_fw = 1;
			config_file = optarg;
			D_GDC("custom_fw: %s\n", config_file);
			break;
		case 'f':
			format = atol(optarg);
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			D_GDC("output_file: %s\n", output_file);
			break;
		case 'p':
			plane_number = atol(optarg);
			break;
		case 's':
			sscanf(optarg, "%dx%d-%dx%d",
				&in_y_stride, &in_c_stride,
				&out_y_stride, &out_c_stride);
			D_GDC("parse stride, in: y-%d uv-%d, out:y-%d uv-%d\n",
			in_y_stride, in_c_stride, out_y_stride, out_c_stride);
			break;
		case 'w':
			sscanf(optarg, "%dx%d-%dx%d",
				&in_width, &in_height, &out_width, &out_height);
			D_GDC("parse wxh, in: %dx%d, out:%dx%d\n",
				in_width, in_height, out_width, out_height);
			break;
		case 'n':
			num = atol(optarg);
			break;
		case 'm':
			mem_type = atol(optarg);
			break;
		case 'l':
			num_thread = atol(optarg);
			break;
		case 'r':
			num_process_per_thread = atol(optarg);
			break;
		}
	}

	for (int i = 0; i < num_thread; i++) {
		threadindex[i] = i;
		int res = pthread_create(&(thread[i]), NULL, main_run, (void *)&threadindex[i]);
		if (res != 0) {
			E_GDC("integral thread %d creation failed!", i);
			return -1;
		}
	}
	for (int i = 0; i < num_thread; i++)
		pthread_join(thread[i], NULL);

	return 0;
}

