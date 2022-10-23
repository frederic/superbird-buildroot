/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "drm.h"
#include "demux.h"
#include "v4l2-dec.h"

static char* media_file;
static sem_t wait_for_end;
int global_plane_id;
char mode_str[16];
unsigned int vfresh;
int g_dw_mode = 16;
static int secure_mode = 0;
int ffmpeg_log = 0;

static void usage(int argc, char **argv)
{
        printf(  "Usage: %s [options]\n\n"
                 "Version 0.1\n"
                 "Options:\n"
                 "-d | --double_write  double write mode.\n"
                 "                     0: no DW compressed only\n"
                 "                     1: DW with 1:1 ratio\n"
                 "                     2: DW with 1:4 down sample\n"
                 "                     4: DW with 1:2 down sample\n"
                 "                     16: DW only\n"
                 "-f | --file name     media file\n"
                 "-h | --help          Print this message\n"
                 "-p | --plane=id      select display plane. 26[pri] 28[overlay 1] 30[overlay 2] 32[video]\n"
                 "-m | --mode str      set display mode. such as 3840x2160 or 3840x2160-60\n"
                 "                                               1920x1080 or 1920x1080-60\n"
                 "-l | --log           enable more ffmpeg demux log.\n"
                 "-s | --secure        secure video path.\n"
                 "",
                 argv[0]);
}

static const char short_options[] = "d:hf:p:m:ls";

static const struct option
long_options[] = {
        { "double_write", required_argument, NULL, 'd' },
        { "file", required_argument, NULL, 'f' },
        { "help",   no_argument,       NULL, 'h' },
        { "plane",  required_argument, NULL, 'p' },
        { "mode", required_argument, NULL, 'm' },
        { "log",  no_argument, NULL, 'l' },
        { "secure",  no_argument, NULL, 's' },
        { 0, 0, 0, 0 }
};

void decode_finish()
{
    sem_post(&wait_for_end);
}

int start_decoder(struct dmx_v_data *v_data)
{
    int ret;
    ret = v4l2_dec_init(v_data->type, secure_mode, decode_finish);
    if (ret) {
        printf("FATAL: start_decoder error:%d\n",ret);
        exit(1);
    }
    return 0;
}

static int parse_para(int argc, char *argv[])
{
    char *p;
    unsigned int len;
    for (;;) {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                      short_options, long_options, &idx);

        if (-1 == c)
            break;

        switch (c) {
            case 0: /* getopt_long() flag */
                break;

            case 'l':
                ffmpeg_log = 1;
                break;

            case 'd':
                g_dw_mode = atoi(optarg);
                if (g_dw_mode != 0 &&
                        g_dw_mode != 1 &&
                        g_dw_mode != 2 &&
                        g_dw_mode != 4 &&
                        g_dw_mode != 16) {
                    printf("invalide dw_mode %d\n", g_dw_mode);
                    exit(1);
                }
                break;

            case 'f':
                media_file = strdup(optarg);
                break;

            case 'h':
                usage(argc, argv);
                return -1;

            case 'p':
                global_plane_id = atoi(optarg);
                break;

            case 's':
#ifndef CONFIG_SECMEM
                printf("secure mode can not be enabled, check compile option\n");
                exit(1);
#endif
                secure_mode = 1;
                break;

            case 'm':
                p = strchr(optarg, '-');
                if (p == NULL) {
                    len	= strlen(optarg);
                } else {
                    vfresh = strtoul(p + 1, NULL, 0);
                    len = p - optarg;
                }

                strncpy(mode_str, optarg, len);
                printf("mode:%s, vfresh:%d.\n", mode_str, vfresh);
                break;

            default:
                usage(argc, argv);
                return -1;
        }
    }
    return 0;
}

static void sig_handler(int signo)
{
    dump_v4l2_decode_state();
    decode_finish();
}

int main(int argc, char *argv[])
{
    int rc;

    if (parse_para(argc, argv))
        goto error;

    if (!media_file) {
        printf("no file name assigned\n");
        goto error;
    }

    sem_init(&wait_for_end, 0, 0);

    display_engine_register_cb(capture_buffer_recycle);
    rc = display_engine_start(secure_mode);
    if (rc < 0) {
        printf("Unable to start display engine\n");
        goto error;
    }
    printf("\ndisplay started\n");

    struct dmx_cb cb = {
        .write = v4l2_dec_write_es,
        .frame_done = v4l2_dec_frame_done,
        .meta_done = start_decoder,
        .eos = v4l2_dec_eos,
    };

    signal(SIGINT, sig_handler);

    rc = demux_init(media_file, &cb);
    if (rc) {
        printf("demux_init fail %d\n",rc);
        goto error;
    }
    printf("\ndemux started\n");

    sem_wait(&wait_for_end);

    dmx_destroy();
    v4l2_dec_destroy();
    display_engine_stop();
    sem_destroy(&wait_for_end);

    return 0;
error:
	return 1;
}
