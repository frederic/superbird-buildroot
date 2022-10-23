/*
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "audio_if.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define WRITE_UNIT 4096

#define FORMAT_PCM16    0
#define FORMAT_PCM32    1
#define FORMAT_DD       2
#define FORMAT_MAT      3
#define FORMAT_IEC61937 4
#define FORMAT_AC4      5
#define FORMAT_MAX      6

static int format_tab[] = {AUDIO_FORMAT_PCM_16_BIT, AUDIO_FORMAT_PCM_32_BIT, AUDIO_FORMAT_AC3, AUDIO_FORMAT_MAT, AUDIO_FORMAT_IEC61937, AUDIO_FORMAT_AC4};
static const char *format_str[] = {
    "PCM_16", "PCM_32", "DOLBY DD/DD+", "DOLBY MAT", "IEC_61937", "AC4"
};

static int format_is_pcm(int format)
{
    return (format == FORMAT_PCM16) || (format == FORMAT_PCM32);
}

static unsigned char *fmap(const char *fn, int *size, int *fd)
{
    int fd_r, r;
    struct stat st;
    unsigned char *p;

    fd_r = open(fn, O_RDWR);

    if (fd_r < 0)
        return NULL;

    *fd = fd_r;
    r = fstat(fd_r, &st);
    *size = st.st_size;
    return mmap(0, st.st_size, PROT_READ|PROT_EXEC, MAP_SHARED, fd_r, 0);
}

static void funmap(unsigned char *p, int size, int fd)
{
    if (p && size > 0)
        munmap(p, size);
    if (fd >= 0)
        close(fd);
}

static int test_stream(struct audio_stream_out *stream, unsigned char *buf, int size)
{
    int ret;
    int len = size;
    unsigned char *data = buf;

    ret = stream->common.standby(&stream->common);
    if (ret) {
        fprintf(stderr, "%s %d, ret:%x\n",
                __func__, __LINE__, ret);
        return -1;
    }

    ret = stream->set_volume(stream, 1.0f, 1.0f);
    if (ret) {
        fprintf(stderr, "%s %d, ret:%x\n", __func__, __LINE__, ret);
        return -1;
    }

    while (len > 0) {
        ssize_t s = stream->write(stream, data, min(len, WRITE_UNIT));
        if (s < 0) {
            fprintf(stderr, "stream writing error %d\n", s);
            break;
        }

        len -= s;
        data += s;
    }

    return 0;
}

static void test_output_stream(audio_hw_device_t *device, unsigned char *buf, int size, struct audio_config *config)
{
    struct audio_stream_out *stream;
    int ret;

    printf("open output speaker...\n");
    ret = device->open_output_stream(device,
            0, AUDIO_DEVICE_OUT_SPEAKER,
            AUDIO_OUTPUT_FLAG_PRIMARY, config,
            &stream, NULL);
    if (ret) {
        printf("fail\n");
        return;
    } else {
        printf("success\n");
    }

    test_stream(stream, buf, size);

    printf("close output speaker...\n");
    device->close_output_stream(device, stream);
}

int main(int argc, char **argv)
{
    audio_hw_device_t *device;
    int ret, c = -1, format = FORMAT_MAX, ch = 0, sr = 0;
    struct audio_config config;
    const char *fn = NULL;
    int size = 0;
    unsigned char *buf;
    int fd = -1;

    if (argc == 1) {
        printf("Usage: halplay -f <format> -c <channel number> -r <sample rate> <filename>\n");
        return 0;
    }

    while ((c = getopt(argc, argv, "f:c:r:")) != -1) {
        switch (c) {
            case 'f':
                format = atoi(optarg);
                break;
            case 'c':
                ch = atoi(optarg);
                break;
            case 'r':
                sr = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Error in an argument.\n");
                return -1;
            default:
                return -1;
        }
    }

    if (optind < argc) {
        fn = argv[optind];
    }

    if (!fn) {
        fprintf(stderr, "No file name specified\n");
        return -1;
    }

    if ((format < 0) || (format >= FORMAT_MAX)) {
        int i;
        fprintf(stderr, "Wrong format, valid format:\n");
        for (i = 0; i < FORMAT_MAX; i++)
            fprintf(stderr, "\t%d: %s\n", i, format_str[i]);
        return -1;
    }

    if (format_is_pcm(format)) {
        if ((ch < 1) || (ch > 8)) {
            fprintf(stderr, "Wrong channel number, valid range [1-8]\n");
            return -1;
        }
        if ((sr != 32000) && (sr != 44100) && (sr != 48000)) {
            fprintf(stderr, "Invalid sample rate, valid options [32000, 44100, 48000]\n");
            return -1;
        }
        if ((ch != 1) && (ch != 2) && (ch != 6) && (ch != 8)) {
            fprintf(stderr, "Invalid channel number, valid options [1, 2, 6, 8]\n");
            return -1;
        }
    }

    ret = audio_hw_load_interface(&device);
    if (ret) {
        fprintf(stderr, "%s %d error:%d\n", __func__, __LINE__, ret);
        return ret;
    }
    printf("hw version: %x\n", device->common.version);
    printf("hal api version: %x\n", device->common.module->hal_api_version);
    printf("module id: %s\n", device->common.module->id);
    printf("module name: %s\n", device->common.module->name);

    if (device->get_supported_devices) {
        uint32_t support_dev = 0;
        support_dev = device->get_supported_devices(device);
        printf("supported device: %x\n", support_dev);
    }

    int inited = device->init_check(device);
    if (inited) {
        printf("device not inited, quit\n");
        goto exit;
    }

    buf = fmap(fn, &size, &fd);
    if (!buf) {
        fprintf(stderr, "Error, cannot open input file\n");
        goto exit;
    }

    /* set audio config */
    memset(&config, 0, sizeof(config));

    if (format_is_pcm(format)) {
        config.sample_rate = sr;
        switch (ch) {
            case 1:
                config.channel_mask = AUDIO_CHANNEL_OUT_MONO;
                break;
            case 2:
                config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
                break;
            case 6:
                config.channel_mask = AUDIO_CHANNEL_OUT_5POINT1;
                break;
            case 8:
                config.channel_mask = AUDIO_CHANNEL_OUT_7POINT1;
                break;
            default:
                config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
                break;
        }
    } else {
        config.sample_rate = 48000;
        config.channel_mask = AUDIO_CHANNEL_OUT_5POINT1;
    }
    config.format = format_tab[format];

    test_output_stream(device, buf, size, &config);

    funmap(buf, size, fd);

exit:
    audio_hw_unload_interface(device);
    return 0;
}
