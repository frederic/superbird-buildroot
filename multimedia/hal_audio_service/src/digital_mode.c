#include <stdio.h>
#include <stdlib.h>
#include <audio_if.h>

#define DIGITAL_MODE_PCM  0
#define DIGITAL_MODE_DD   4
#define DIGITAL_MODE_AUTO 5

#define DIGITAL_MODE_CMD "hdmi_mode="

int main(int argc, char **argv)
{
    audio_hw_device_t *device;
    int mode, ret;
    char cmd[256];

    if (argc < 2) {
        printf("Usage: digital_mode <mode>\n");
        return -1;
    }

    mode = atoi(argv[1]);
    if ((mode != DIGITAL_MODE_PCM) &&
        (mode != DIGITAL_MODE_DD) &&
        (mode != DIGITAL_MODE_AUTO)) {
        printf("Invalid mode\n");
        return -2;
    }
    snprintf(cmd, sizeof(cmd), "%s%d", DIGITAL_MODE_CMD, mode);

    ret = audio_hw_load_interface(&device);
    if (ret) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return ret;
    }

    ret = device->init_check(device);
    if (ret) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return -1;
    }

    ret = device->set_parameters(device, cmd);
    printf("set_parameters returns %d\n", ret);

    audio_hw_unload_interface(device);

    return ret;
}

