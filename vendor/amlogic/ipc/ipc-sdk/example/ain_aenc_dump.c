#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aml_ipc_sdk.h"

struct cmd_val {
    char *ain_device;
    int ain_samplerate;
    int ain_channel;
    char *enc_codec;
    char *out_file;
};

static volatile int bexit = 0;

void signal_handle(int sig) { bexit = 1; }

AmlStatus on_frame(struct AmlIPCFrame *frm, void *param) {
    fwrite(frm->data, 1, frm->size, (FILE *)param);
    return AML_STATUS_OK;
}

int parse_cmd(int argc, char *argv[], struct cmd_val *val) {
    int ch;
    while ((ch = getopt(argc, argv, "hd:r:n:e:f:")) != -1) {
        switch (ch) {
        case 'h':
            printf("-----------------------------------------------------\n");
            printf("-d : device, (default)\n");
            printf("-r : samplerate\n");
            printf("-n : number of audio channel\n");
            printf("-e : audio encoder codec, aac, g711-alaw, g711-ulaw, g726\n");
            printf("-f : output file\n");
            printf("-h : print this help message\n");
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
        case 'e':
            val->enc_codec = optarg;
            break;
        case 'f':
            val->out_file = optarg;
            break;
        default:
            printf("use '%s -h' to print help message\n", argv[0]);
            return -1;
        }
    }
    printf("audioIn device: %s\nnaudioIn samplerate: "
           "%d\naudioIn number channel: %d\nencoder: "
           "%s\noutput file: %s\n",
           val->ain_device, val->ain_samplerate, val->ain_channel, val->enc_codec, val->out_file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("use '%s -h' to print help message\n", argv[0]);
        exit(1);
    }

    struct cmd_val val = {"default", 48000, 2, "aac", "aac.dat"};

    if (parse_cmd(argc, argv, &val) < 0) {
        exit(1);
    }

    FILE *dump_fp = fopen(val.out_file, "w");
    if (dump_fp == NULL) {
        printf("file open failed\n");
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

    struct AmlIPCComponent *comp = NULL;

    if (strcmp(val.enc_codec, "aac") == 0) {
        comp = (void *)aml_ipc_fdkaac_new(AML_ACODEC_AAC, AML_AAC_LC);
    } else if (strcmp(val.enc_codec, "g726") == 0) {
        comp = (void *)aml_ipc_g726_new(AML_ACODEC_ADPCM_G726, AML_G726_16K, AML_G726_LE);
    } else if (strcmp(val.enc_codec, "g711-ulaw") == 0) {
        comp = (void *)aml_ipc_g711_new(AML_ACODEC_PCM_ULAW);
    } else if (strcmp(val.enc_codec, "g711-alaw") == 0) {
        comp = (void *)aml_ipc_g711_new(AML_ACODEC_PCM_ALAW);
    } else {
        printf("!!! encoder error !!!\n");
        exit(1);
    }
    aml_ipc_bind(AML_IPC_COMPONENT(ain), AML_AIN_OUTPUT, comp, AML_ACODEC_INPUT);
    aml_ipc_add_frame_hook(comp, AML_ACODEC_OUTPUT, dump_fp, on_frame);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(ain), comp, NULL);
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
    aml_ipc_start(comp);
    aml_ipc_start(AML_IPC_COMPONENT(ain));
#endif

    while (!bexit) {
        usleep(1000 * 500);
    }

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    aml_obj_release(AML_OBJECT(pipeline));
#else
    aml_ipc_stop(comp);
    aml_ipc_stop(AML_IPC_COMPONENT(ain));
    aml_obj_release(AML_OBJECT(comp));
    aml_obj_release(AML_OBJECT(ain));
#endif

    fclose(dump_fp);
    return 0;
}
