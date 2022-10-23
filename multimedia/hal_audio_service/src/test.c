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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "data.h"
#include "audio_if.h"

#define TV_AUDIO_OUTPUT

#ifdef TV_AUDIO_OUTPUT
static struct audio_port_config source_;
static struct audio_port_config sink_;
static audio_patch_handle_t patch_h_;

static void test_patch(audio_hw_device_t *device)
{
    int ret;
    /* create mixer to device patch */
    source_.id = 1;
    source_.role = AUDIO_PORT_ROLE_SOURCE;
    source_.type = AUDIO_PORT_TYPE_MIX;
    source_.config_mask = AUDIO_PORT_CONFIG_SAMPLE_RATE |
        AUDIO_PORT_CONFIG_FORMAT;
    source_.sample_rate = 48000;
    source_.format = AUDIO_FORMAT_PCM_16_BIT;

    sink_.id = 2;
    sink_.role = AUDIO_PORT_ROLE_SINK;
    sink_.type = AUDIO_PORT_TYPE_DEVICE;
    sink_.config_mask = AUDIO_PORT_CONFIG_SAMPLE_RATE |
        AUDIO_PORT_CONFIG_FORMAT;
    sink_.sample_rate = 48000;
    sink_.format = AUDIO_FORMAT_PCM_16_BIT;
    sink_.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    printf("create mix --> speaker patch...");
    ret = device->create_audio_patch(device, 1, &source_, 1, &sink_, &patch_h_);
    if (ret) {
        printf("fail ret:%d\n",ret);
    } else {
        printf("success\n");
    }
}

static void destroy_patch(audio_hw_device_t *device)
{
    if (patch_h_) {
        int ret;
        printf("destroy patch...");
        ret = device->release_audio_patch(device, patch_h_);
        if (ret) {
            printf("fail ret:%d\n",ret);
        } else {
            printf("success\n");
        }
    }
}
#endif
static void test_vol(audio_hw_device_t *device, int gain)
{
    int ret;
    float vol = 0;
    ret = device->get_master_volume(device, &vol);
    if (ret) {
        printf("get_master_volume fail\n");
    } else {
        printf("cur master vol:%f\n", vol);
    }
    ret = device->set_master_volume(device, 0.5f);
    if (ret) {
        printf("set_master_volume fail\n");
    } else {
        printf("set master vol 0.5\n");
        device->set_master_volume(device, vol);
    }

#ifdef  TV_AUDIO_OUTPUT
    {
        struct audio_port_config config;

        memset(&config, 0, sizeof(config));
        config.id = 2;
        config.role = AUDIO_PORT_ROLE_SINK;
        config.type = AUDIO_PORT_TYPE_DEVICE;
        config.config_mask = AUDIO_PORT_CONFIG_GAIN;
        /* audio_hal use dB * 100 to keep the accuracy */
        config.gain.values[0] = gain * 100;
        printf("set gain to %ddB...\n", gain);
        ret = device->set_audio_port_config(device, &config);
        if (ret) {
            printf("fail\n");
        } else {
            printf("success\n");
        }
    }
#endif
}

static int test_stream(struct audio_stream_out *stream)
{
    int ret;
    int i;
    printf("%s %d, ch_mask:%x\n",
            __func__, __LINE__,
            stream->common.get_channels(&stream->common));
    printf("%s %d, format:%x\n",
            __func__, __LINE__,
            stream->common.get_format(&stream->common));
    printf("%s %d, sr:%d\n",
            __func__, __LINE__,
            stream->common.get_sample_rate(&stream->common));
    ret = stream->common.standby(&stream->common);
    if (ret) {
        printf("%s %d, ret:%x\n",
                __func__, __LINE__, ret);
        return -1;
    }

    ret = stream->set_volume(stream, 1.0f, 1.0f);
    if (ret) {
        printf("%s %d, ret:%x\n", __func__, __LINE__, ret);
        return -1;
    }

    stream->resume(stream);

    for (i = 0; i < 20; i++) {
        ssize_t s = stream->write(stream, wav_data, sizeof(wav_data));
        printf("%s %d, write %d bytes\n", __func__, __LINE__, s);
        uint32_t latency = stream->get_latency(stream);
        printf("%s %d, write %d latency\n", __func__, __LINE__, latency);
    }

    int pos = 0;
    ret = stream->get_render_position(stream, &pos);
    if (ret) {
        printf("%s %d, ret:%x\n", __func__, __LINE__, ret);
        return -1;
    }
    printf("%s %d, write %d pos:%d\n", __func__, __LINE__, pos);

    ret = stream->pause(stream);
    if (ret) {
        printf("%s %d, ret:%x\n", __func__, __LINE__, ret);
    }

    return 0;
}

static void test_output_stream(audio_hw_device_t *device)
{
    int ret;
    struct audio_config config;
    struct audio_stream_out *stream;

    memset(&config, 0, sizeof(config));
    config.sample_rate = 48000;
    config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    config.format = AUDIO_FORMAT_PCM_16_BIT;

    printf("open output speaker...\n");
    ret = device->open_output_stream(device,
            0, AUDIO_DEVICE_OUT_SPEAKER,
            AUDIO_OUTPUT_FLAG_PRIMARY, &config,
            &stream, NULL);
    if (ret) {
        printf("fail\n");
        return;
    } else {
        printf("success\n");
    }
    test_stream(stream);
    printf("close output speaker...\n");
    device->close_output_stream(device, stream);

#if 0
    sleep(1);

    printf("open output HDMI...\n");
    ret = device->open_output_stream(device,
            0, AUDIO_DEVICE_OUT_HDMI,
            AUDIO_OUTPUT_FLAG_PRIMARY, &config,
            &stream, NULL);
    if (ret) {
        printf("fail\n");
        return;
    } else {
        printf("success\n");
    }
    test_stream(stream);
    printf("close output hdmi...\n");
    device->close_output_stream(device, stream);
#endif
}

static void test_input_stream(audio_hw_device_t *device)
{
    int ret;
    size_t s;
    struct audio_config config;

    memset(&config, 0, sizeof(config));
    config.sample_rate = 48000;
    config.channel_mask = AUDIO_CHANNEL_IN_MONO;
    config.format = AUDIO_FORMAT_PCM_16_BIT;

    s = device->get_input_buffer_size(device, &config);
    printf("%s %d size:%d\n", __func__, __LINE__, s);
    if (s < 0) {
        printf("%s %d wrong size:%d\n", __func__, __LINE__, s);
        return;
    }
}

int main(int argc, const char **argv)
{
    int ret;
    audio_hw_device_t *device;

    ret = audio_hw_load_interface(&device);
    if (ret) {
        printf("%s %d error:%d\n", __func__, __LINE__, ret);
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

#ifdef TV_AUDIO_OUTPUT
    test_patch(device);
#endif
    test_vol(device, (argc > 1) ? atoi(argv[1]) : 0);
    test_output_stream(device);
    test_input_stream(device);

#ifdef TV_AUDIO_OUTPUT
    destroy_patch(device);
#endif
exit:
    device->common.close(&device->common);
    audio_hw_unload_interface(device);
    return 0;
}
