#include <stdio.h>
#include <stdlib.h>
#include <audio_if.h>

#define SPEAKER_DELAY_CMD "hal_param_speaker_delay_time_ms="

int main(int argc, char **argv)
{
    audio_hw_device_t *device;
    int delay = 0, ret;
    char cmd[256];

    if (argc < 2) {
        printf("Usage: hal_set_speaker_delay <delay in ms>\n");
        return -1;
    }

    delay = atoi(argv[1]);
    if ((delay < 0) || (delay >= 1000)) {
        printf("Delay out of range\n");
        return -2;
    }
    snprintf(cmd, sizeof(cmd), "%s%d", SPEAKER_DELAY_CMD, delay);

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


