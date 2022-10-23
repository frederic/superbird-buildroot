#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aml_ipc_sdk.h"

struct cmd_val {
    char *ain_device;
    int ain_samplerate;
    int ain_channel;
    char *out_file;
    char *aout_device;
};

static volatile int bexit = 0;

void signal_handle(int sig) { bexit = 1; }

AmlStatus ain_hook(struct AmlIPCFrame *frm, void *param) {
    fwrite(frm->data, 1, frm->size, (FILE *)param);
    return AML_STATUS_HOOK_CONTINUE;
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
    int ch;
    while ((ch = getopt(argc, argv, "hd:r:n:f:o:")) != -1) {
        switch (ch) {
        case 'h':
            printf("-----------------------------------------------------\n");
            printf("-d        : audioIn device, (default)\n");
            printf("-r        : audioIn samplerate\n");
            printf("-n        : number of audioIn channel\n");
            printf("-o        : audioOut device, (default)\n");
            printf("-f        : output file\n");
            printf("-h        : print this help message\n");
            printf("-----------------------------------------------------\n");
            return -1;
            break;
        case 'd':
            val->ain_device = optarg;
            break;
        case 'r':
            val->ain_samplerate = atoi(optarg);
            break;
        case 'n':
            val->ain_channel = atoi(optarg);
            break;
        case 'f':
            val->out_file = optarg;
            break;
        case 'o':
            val->aout_device = optarg;
            break;
        default:
            printf("use '%s -h' to print help message\n", argv[0]);
            return -1;
        }
    }
    printf("audioIn device: %s\naudioIn samplerate: "
           "%d\naudioIn number channel: %d\naudioOut device: %s\noutput file: %s\n",
           val->ain_device, val->ain_samplerate, val->ain_channel, val->aout_device, val->out_file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("use '%s -h' to print help message\n", argv[0]);
        exit(1);
    }

    struct cmd_val val = {"default", 48000, 2, "ain_aout.pcm", "default"};

    if (parse_cmd(argc, argv, &val) < 0) {
        exit(1);
    }

    if (strcmp(val.aout_device, "default") != 0) {
        printf("!!! audioOut device error !!!\n");
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
    aml_ipc_ain_set_device(ain, val.ain_device);
    aml_ipc_ain_set_codec(ain, AML_ACODEC_PCM_S16LE);
    aml_ipc_ain_set_rate(ain, val.ain_samplerate);
    aml_ipc_ain_set_channel(ain, val.ain_channel);

    struct AmlIPCAOut *aout = aml_ipc_aout_new();
    aml_ipc_aout_set_device(aout, val.aout_device);
    aml_ipc_bind(AML_IPC_COMPONENT(ain), AML_DEFAULT_CHANNEL, AML_IPC_COMPONENT(aout),
                 AML_DEFAULT_CHANNEL);
    aml_ipc_add_frame_hook(AML_IPC_COMPONENT(ain), AML_AIN_OUTPUT, dump_fp, ain_hook);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(ain), aout, NULL);
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
    aml_ipc_start(AML_IPC_COMPONENT(aout));
    aml_ipc_start(AML_IPC_COMPONENT(ain));
#endif

    while (!bexit) {
        usleep(1000 * 500);
    }

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    aml_obj_release(AML_OBJECT(pipeline));
#else
    aml_ipc_stop(AML_IPC_COMPONENT(aout));
    aml_ipc_stop(AML_IPC_COMPONENT(ain));
    aml_obj_release(AML_OBJECT(aout));
    aml_obj_release(AML_OBJECT(ain));
#endif

    fclose(dump_fp);
    return 0;
}
