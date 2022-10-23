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

#include "gdc_api.h"
#define DEBUG_IO 0

#if DEBUG_IO
#include <sys/time.h>
#endif

struct gdc_param {
    uint32_t i_width;
    uint32_t i_height;
    uint32_t o_width;
    uint32_t o_height;
    uint32_t format;
};


int get_file_size(char *f_name)
{
    int f_size = -1;
    FILE *fp = NULL;

    if (f_name == NULL) {
        printf("Error file name\n");
        return f_size;
    }

    fp = fopen(f_name, "rb");
    if (fp == NULL) {
        printf("Error open file %s\n", f_name);
        return f_size;
    }

    fseek(fp, 0, SEEK_END);

    f_size = ftell(fp);

    fclose(fp);

    printf("%s: size %d\n", f_name, f_size);

    return f_size;
}

int gdc_set_config_param(struct gdc_usr_ctx_s *ctx,
                                    char *f_name, int len)
{
    FILE *fp = NULL;
    int r_size = -1;

    if (f_name == NULL || ctx == NULL || ctx->c_buff == NULL) {
        printf("Error input param\n");
        return r_size;
    }

    fp = fopen(f_name, "rb");
    if (fp == NULL) {
        printf("Error open file %s\n", f_name);
        return -1;
    }

    r_size = fread(ctx->c_buff, len, 1, fp);
    if (r_size <= 0) {
        printf("Failed to read file %s\n", f_name);
    }

    fclose(fp);

    return r_size;
}

int gdc_set_input_image(struct gdc_usr_ctx_s *ctx,
                                    char *f_name, int len)
{
    FILE *fp = NULL;
    int r_size = -1;

    if (f_name == NULL || ctx == NULL || ctx->i_buff == NULL) {
        printf("Error input param\n");
        return r_size;
    }

    fp = fopen(f_name, "rb");
    if (fp == NULL) {
        printf("Error open file %s\n", f_name);
        return -1;
    }

    r_size = fread(ctx->i_buff, len, 1, fp);
    if (r_size <= 0) {
        printf("Failed to read file %s\n", f_name);
	return -1;
    }

    fclose(fp);

    return r_size;
}

void save_imgae(char *buff, unsigned long size, int flag, int count)
{
    char name[60] = {'\0'};
    FILE *fp = NULL;

    if (buff == NULL || size == 0) {
        printf("%s:Error input param\n", __func__);
        return;
    }

    sprintf(name, "/media/gdc_%d_dump-%d.raw", flag, count);

    fp = fopen(name, "wb");
    if (fp == NULL) {
        printf("%s:Error open file\n", __func__);
        return;
    }

    fwrite(buff, size, 1, fp);

    fclose(fp);
}


int gdc_init_cfg(struct gdc_usr_ctx_s *ctx, struct gdc_param *tparm, char *f_name)
{
    struct gdc_settings *gdc_gs = NULL;
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

    if (ctx == NULL || tparm == NULL || f_name == NULL) {
        printf("Error invalid input param\n");
        return ret;
    }

    memset(ctx, 0, sizeof(*ctx));

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
        printf("Error unknow format\n");
        return ret;
    }

    gdc_gs = &ctx->gs;

    gdc_gs->base_gdc = 0;

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

    gdc_create_ctx(ctx);

    c_len = get_file_size(f_name);
    if (c_len <= 0) {
        gdc_destroy_ctx(ctx);
        printf("Error gdc config file size\n");
        return ret;
    }

    ret = gdc_alloc_dma_buffer(ctx, CONFIG_BUFF_TYPE, c_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc cfg buff\n");
        return ret;
    }

    ret = gdc_set_config_param(ctx, f_name, c_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error cfg gdc param buff\n");
        return ret;
    }

    gdc_gs->gdc_config.config_size = c_len / 4;

    if (format == RGB444_P || format == YUV444_P)
        i_len = i_y_stride * i_height * 3;
    else
        i_len = i_y_stride * i_height * 2;

    ret = gdc_alloc_dma_buffer(ctx, INPUT_BUFF_TYPE, i_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc input buff\n");
        return ret;
    }

    if (format == RGB444_P || format == YUV444_P)
        o_len = o_y_stride * o_height * 3;
    else if (format == NV12 || format == YV12)
        o_len = o_y_stride * o_height * 3 / 2;
    else if (format == Y_GREY)
         o_len = o_y_stride * o_height;

    ret = gdc_alloc_dma_buffer(ctx, OUTPUT_BUFF_TYPE, o_len);
    if (ret < 0) {
        gdc_destroy_ctx(ctx);
        printf("Error alloc gdc input buff\n");
        return ret;
    }

    return ret;
}

int main(int argc, char* argv[]) {
    int c;
    int ret = 0;
    struct gdc_usr_ctx_s ctx;
    uint32_t format;
    uint32_t in_width = 1920;
    uint32_t in_height = 1920;
    uint32_t out_width = 1920;
    uint32_t out_height = 1920;
    uint32_t in_y_stride = 0, in_c_stride = 0, out_y_stride = 0, out_c_stride = 0;
    char *input_file = NULL;
    char *config_file = "config.bin";
    struct gdc_param g_param;
    int len = 0;

    while (1) {
        static struct option opts[] = {
            {"config_file", required_argument, 0, 'f'},
            {"format", required_argument, 0, 'f'},
            {"height", required_argument, 0, 'h'},
            {"input_file", required_argument, 0, 'i'},
            {"output_file", optional_argument, 0, 'o'},
            {"stride", optional_argument, 0, 's'},
            {"width", required_argument, 0, 'w'},
        };
        int i = 0;
        c = getopt_long(argc, argv, "c:f:h:i:os:w:", opts, &i);
        if (c == -1)
            break;

        switch (c) {
        case 'c':
            config_file = optarg;
            break;
        case 'f':
            format = atol(optarg);
            break;
        case 'i':
            input_file = optarg;
            break;
        case 's':
            sscanf(optarg, "%dx%d-%dx%d",
                    &in_y_stride, &in_c_stride, &out_y_stride, &out_c_stride);
            printf("parse stride, in: y-%d uv-%d, out:y-%d uv-%d\n",
                    in_y_stride, in_c_stride, out_y_stride, out_c_stride);
            break;
        case 'w':
            sscanf(optarg, "%dx%d-%dx%d",
                    &in_width, &in_height, &out_width, &out_height);
            printf("parse wxh, in: %dx%d, out:%dx%d\n",
                    in_width, in_height, out_width, out_height);
            break;
        }
    }

    g_param.i_width = in_width;
    g_param.i_height = in_height;
    g_param.o_width = out_width;
    g_param.o_height = out_height;
    g_param.format = format;

    memset(&ctx, 0, sizeof(ctx));

    ret = gdc_init_cfg(&ctx, &g_param, config_file);
    if (ret < 0) {
	printf("Error gdc init\n");
	gdc_destroy_ctx(&ctx);
	return -1;
    }

    len = get_file_size(input_file);

    gdc_set_input_image(&ctx, input_file, len);

    ret = gdc_process(&ctx);
    if (ret < 0) {
        printf("ioctl failed\n");
	gdc_destroy_ctx(&ctx);
	return ret;
    }

    save_imgae(ctx.o_buff, ctx.o_len, format, 1);
    gdc_destroy_ctx(&ctx);

    printf("Success save output image\n");

    return 0;
}
