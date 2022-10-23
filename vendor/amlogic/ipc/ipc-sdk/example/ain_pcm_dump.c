#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aml_ipc_sdk.h"

struct cmd_val {
    char *device;
    int samplerate;
    int channel;
    char *out_file;
};

static volatile int bexit = 0;

void signal_handle(int sig) { bexit = 1; }

AmlStatus ain_hook(struct AmlIPCFrame *frm, void *param) {
    fwrite(frm->data, 1, frm->size, (FILE *)param);
    return AML_STATUS_OK;
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
    int ch;
    while ((ch = getopt(argc, argv, "hd:r:n:f:")) != -1) {
        switch (ch) {
        case 'h':
            printf("-----------------------------------------------------\n");
            printf("-d        : device, (default)\n");
            printf("-r        : samplerate\n");
            printf("-n        : number of audio channel\n");
            printf("-f        : output file\n");
            printf("-h        : print this help message\n");
            printf("-----------------------------------------------------\n");
            return -1;
            break;
        case 'd':
            val->device = optarg;
            break;
        case 'r':
            val->samplerate = atoi(optarg);
            break;
        case 'n':
            val->channel = atoi(optarg);
            break;
        case 'f':
            val->out_file = optarg;
            break;
        default:
            printf("use '%s -h' to print help message\n", argv[0]);
            return -1;
        }
    }
    printf("device: %s\nsamplerate: %d\nnumber channel: %d\noutput file: %s\n", val->device,
           val->samplerate, val->channel, val->out_file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("use '%s -h' to print help message\n", argv[0]);
        exit(1);
    }

    struct cmd_val val = {"default", 48000, 2, "ain.pcm"};

    if (parse_cmd(argc, argv, &val) < 0) {
        exit(1);
    }

    FILE *dump_fp = fopen(val.out_file, "w");
    if (dump_fp == NULL) {
        printf("!!! file open failed !!!\n");
        exit(1);
    }

    aml_ipc_sdk_init();
    const char *debuglevel = getenv("AML_DEBUG");
    if (!debuglevel)
        debuglevel = ".*:3";
    aml_ipc_log_set_from_string(debuglevel);
    const char *tracelevel = getenv("AML_TRACE");
    if (!tracelevel)
        tracelevel = ".*:3";
    aml_ipc_trace_set_from_string(tracelevel);

    signal(SIGINT, signal_handle);

    struct AmlIPCAIn *ain = aml_ipc_ain_new();
    aml_ipc_ain_set_device(ain, val.device);
    aml_ipc_ain_set_codec(ain, AML_ACODEC_PCM_S16LE);
    aml_ipc_ain_set_rate(ain, val.samplerate);
    aml_ipc_ain_set_channel(ain, val.channel);
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(ain), AML_AIN_OUTPUT, dump_fp, ain_hook);

    aml_ipc_start(AML_IPC_COMPONENT(ain));

    while (!bexit) {
        usleep(1000 * 500);
    }

    aml_ipc_stop(AML_IPC_COMPONENT(ain));
    aml_obj_release(AML_OBJECT(ain));

    fclose(dump_fp);
    return 0;
}
