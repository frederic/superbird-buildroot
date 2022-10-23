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
#include "include/ion/ion.h"
#include "kernel-headers/linux/ion.h"
#include <errno.h>

#include "include/gdc/gdc_api.h"
#define DEBUG_IO 1

#if DEBUG_IO
#include <sys/time.h>
#endif

//gdc configuration example
#define WORD_SIZE 16
#define WORD_MASK (~(WORD_SIZE - 1))
#define AXI_WORD_ALIGN(size) ((size + WORD_SIZE - 1) & WORD_MASK)

int gdc_open()
{
    int fd = open("/dev/gdc", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        printf("open /dev/gdc failed!\n");
    return fd;
}

int main(int argc, char* argv[]) {
    int i;
    int c;
    int ret = 0;
    enum tests {
        ALLOC_TEST = 0, MAP_TEST, SHARE_TEST,
    };

    struct gdc_usr_ctx_s ctx;

    int ion_client = -1;
    int gdc_fd = -1;
    uint32_t format;
    uint32_t in_width = 1920;
    uint32_t in_height = 1920;
    uint32_t out_width = 1920;
    uint32_t out_height = 1920;
    uint32_t in_y_stride = 0, in_c_stride = 0, out_y_stride = 0, out_c_stride = 0;
    uint32_t height = 1080;
    char *input_file = NULL;
    char *output_file = "out.bin";
    char *config_file = "config.bin";

    int config_fd;
    int in_fd;
    int out_fd;
    struct gdc_settings gs;
    size_t len = 0;
    size_t input_file_len = 0;
    size_t output_file_len = 0;
    size_t config_file_len = 0;

    unsigned char *tmp_ptr;
	FILE *fp;

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
        case 'h':
            height = atol(optarg);
            break;
        case 'i':
            input_file = optarg;
            break;
        case 'o':
            output_file = optarg;
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

    if (0 == in_y_stride) {
        in_y_stride = AXI_WORD_ALIGN(in_width);
        out_y_stride = AXI_WORD_ALIGN(out_width);
        if (format == NV12 || format == YUV444_P || format == RGB444_P) {
            in_c_stride = AXI_WORD_ALIGN(in_width);
            out_c_stride = AXI_WORD_ALIGN(out_width);
        } else if (format == YV12) {
            in_c_stride = AXI_WORD_ALIGN(in_width) / 2;
            out_c_stride = AXI_WORD_ALIGN(out_width) / 2;
        } else if (format == Y_GREY) {
            in_c_stride = 0;
            out_c_stride = 0;
        } else {
            printf("unknow format");
            return -EINVAL;
        }
    }

    printf("i_y_stride: %d, i_c_stride: %d, o_y_stride %d, o_c_stride %d\n",
                 in_y_stride, in_c_stride, out_y_stride, out_c_stride);

    ret = gdc_create_ctx(&ctx);

    fp = fopen(input_file, "rb");
    if (!fp)
        printf("fopen %s fail\n", input_file);
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    input_file_len = len;
    printf("input file len =%zu, %d?\n", len, 1920*1080*3/2);
    rewind(fp);

    ret = gdc_alloc_dma_buffer(&ctx, &in_fd, input_file_len);
    if (ret < 0) printf("alloc fail ret=%d, len=%zd\n", ret, input_file_len);
    tmp_ptr = (unsigned char *) mmap (NULL,
                input_file_len, PROT_READ | PROT_WRITE, MAP_SHARED, in_fd, 0);
    ret = fread (tmp_ptr, input_file_len, 1, fp);
    if (ret <= 0) {
        printf("fread %s failed %d\n", input_file, ret);
        return ret;
    }

    munmap(tmp_ptr, input_file_len);
    fclose(fp);
    fp = NULL;

    ion_sync_fd(ctx.ion_client, in_fd);


    // allocate an output buffer the same length as the input buffer.
    //len = out_width * out_height * 3 / 2;
    if (NV12 == format) {
        len = out_y_stride * out_height + out_c_stride * out_height/2;
    } else  if (YV12 == format) {
        len = out_y_stride * out_height + out_c_stride * out_height;
    }
    printf("output check %d, %zu\n",
            out_width * out_height * 3 / 2, len);
    output_file_len = len;
    ret = gdc_alloc_dma_buffer(&ctx, &out_fd, len);
    if (ret < 0) {
        printf("alloc fail ret=%d, len=%zd\n", ret, len);
        return ret;
    }

    fp = fopen(config_file, "rb");
    if (!fp)
        printf("fopen %s fail", config_file);

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    config_file_len = len;
    printf("configuration file len =%zu 7372?\n", config_file_len);

    //rewind to the configuration file's start;
    fseek(fp, 0, SEEK_SET);
///
    ret = gdc_alloc_dma_buffer(&ctx, &config_fd, config_file_len);
    if (ret < 0) {
        printf("alloc fail ret=%d, len=%zd\n", ret, config_file_len);
        return ret;
    }
    tmp_ptr = (unsigned char *) mmap (NULL,
                config_file_len, PROT_READ | PROT_WRITE, MAP_SHARED, config_fd, 0);
    if (ret) {
        printf("config_fd map failed\n");
        return -1;
    }

    ret = fread (tmp_ptr, config_file_len, 1, fp);
    if (ret <= 0) {
        printf("fread config file %s fail,ret=%d, errno=%d,%s\n",
                config_file, ret, errno, strerror(errno));
        return ret;
    }

    munmap(tmp_ptr, config_file_len);
    ion_sync_fd(ctx.ion_client, out_fd);

    fclose (fp);
    fp = NULL;

#if DEBUG_IO
	struct timeval time_IO1, time_IO2;
	int bw_IO;
	gettimeofday(&time_IO1, NULL);
#endif
    memset(&gs, 0, sizeof(gs));
    gs.base_gdc = 0;
    //gs.get_frame_buffer = get_frame_buffer_callback;
    gs.current_addr = gs.buffer_addr;

    //set the gdc config
    gs.gdc_config.config_addr = config_fd;
    gs.gdc_config.config_size = config_file_len / 4; //size of configuration in 4bytes

    gs.gdc_config.input_width = in_width;
    gs.gdc_config.input_height = in_height;
    gs.gdc_config.input_y_stride = in_y_stride;
    gs.gdc_config.input_c_stride = in_c_stride;
    gs.gdc_config.output_width = out_width;
    gs.gdc_config.output_height = out_height;
    gs.gdc_config.output_y_stride = out_y_stride;
    gs.gdc_config.output_c_stride = out_c_stride;
    gs.gdc_config.format = format;
    gs.in_fd = in_fd;
    gs.out_fd = out_fd;
    gs.magic = sizeof(gs);

    ret = gdc_init(&ctx, &gs);

    //cal the 1000 frame's time spend.
    for (i = 0; i < 1; i++) {
        ret = gdc_process(&ctx, in_fd, out_fd);
        if (ret < 0)
            printf("ioctl failed\n");
    }

#if DEBUG_IO
	gettimeofday(&time_IO2, NULL);
	bw_IO = time_IO2.tv_sec - time_IO1.tv_sec;
	bw_IO = bw_IO*1000000 + time_IO2.tv_usec -time_IO1.tv_usec;
	printf("gdc process spend %d us\n", bw_IO);
#endif


    //output buffer len the same as the input file len
    len = output_file_len;
    ion_sync_fd(ctx.ion_client, out_fd);
    tmp_ptr = (unsigned char *) mmap (NULL,
                len, PROT_READ | PROT_WRITE, MAP_SHARED, out_fd, 0);

    fp = fopen(output_file, "wb");
    if (!fp) {
        printf("fopen output file %s fail", output_file);
        return ret;
    }

    ret = fwrite (tmp_ptr, len, 1, fp);
    if (ret <= 0)
        printf("fread church 1080p_y fail");

    fclose(fp);

    munmap(tmp_ptr, len);

    close(in_fd);
    close(config_fd);
    close(out_fd);
    gdc_destroy_ctx(&ctx);

    return 0;
}
