#include <stdio.h>
#include <audio_if.h>

#define MS12_RUNTIME_CMD \
    "ms12_runtime="
#define DAP_DISABLE_SURROUND_DECODER_CMD \
    "-dap_surround_decoder_enable 0"
#define DAP_BOOST_TREBLE \
    "-dap_graphic_eq 1,2,8000,16000,500,500"
#define DAP_GAIN \
    "-dap_gains -1024"
#define SPACE \
    " "

int main()
{
    audio_hw_device_t *device;
    int ret = audio_hw_load_interface(&device);

    if (ret) {
        fprintf(stderr, "audio_hw_load_interface failed: %d\n", ret);
        return ret;
    }

    printf("hw version: %x\n", device->common.version);
    printf("hal api version: %x\n", device->common.module->hal_api_version);
    printf("module id: %s\n", device->common.module->id);
    printf("module name: %s\n", device->common.module->name);

    int inited = device->init_check(device);
    if (inited) {
        printf("device not inited, quit\n");
        audio_hw_unload_interface(device);
        return -1;
    }

#if 1
    ret = device->set_parameters(device,
        MS12_RUNTIME_CMD
        DAP_DISABLE_SURROUND_DECODER_CMD
        SPACE
        DAP_BOOST_TREBLE);
#else
    ret = device->set_parameters(device,
        MS12_RUNTIME_CMD
        DAP_GAIN);
#endif

    audio_hw_unload_interface(device);

    return ret;
}

