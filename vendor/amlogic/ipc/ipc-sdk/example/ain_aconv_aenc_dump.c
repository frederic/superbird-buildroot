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
    int enc_samplerate;
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
    while ((ch = getopt(argc, argv, "hd:r:n:e:t:f:")) != -1) {
        switch (ch) {
        case 'h':
            printf("-----------------------------------------------------\n");
            printf("-d        : device, (default)\n");
            printf("-r        : samplerate\n");
            printf("-n        : number of audio channel\n");
            printf("-e        : encoder codec, aac, g711-alaw, g711-ulaw, g726\n");
            printf("-t        : encoder bitrate, g726:16000, 24000, 32000, 40000\n");
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
        case 'e':
            val->enc_codec = optarg;
            break;
        case 't':
            val->enc_samplerate = atoi(optarg);
            break;
        case 'f':
            val->out_file = optarg;
            break;
        default:
            printf("use '%s -h' to print help message\n", argv[0]);
            return -1;
        }
    }
    printf("audioIn device: %s\naudioIn samplerate: "
           "%d\naudioIn number channel: %d\nencoder codec: "
           "%s\nencoder samplerate: %d\noutput file: %s\n",
           val->ain_device, val->ain_samplerate, val->ain_channel, val->enc_codec,
           val->enc_samplerate, val->out_file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("use '%s -h' to print help message\n", argv[0]);
        exit(1);
    }

    struct cmd_val val = {"default", 48000, 2, "aac", 128000, "ain_aconv_aenc-aac.dat"};

    if (parse_cmd(argc, argv, &val) < 0) {
        exit(1);
    }

    FILE *dump_fp = fopen(val.out_file, "w");
    if (dump_fp == NULL) {
        printf("file open failed\n");
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

    struct AmlIPCAudioFormat fmt = {AML_ACODEC_PCM_S16LE, 48000, 16, 2};
    struct AmlIPCAConvert *aconv = aml_ipc_aconv_new(&fmt);
    aml_ipc_aconv_set_channel_mapping(aconv, "01");
    aml_ipc_bind(AML_IPC_COMPONENT(ain), AML_AIN_OUTPUT, AML_IPC_COMPONENT(aconv),
                 AML_ACONVERT_INPUT);

    struct AmlIPCComponent *comp = NULL;

    if (strcmp(val.enc_codec, "aac") == 0) {
        comp = (void *)aml_ipc_fdkaac_new(AML_ACODEC_AAC, AML_AAC_LC);
        aml_ipc_fdkaac_set_bitrate((struct AmlIPCACodecFDK *)comp, val.enc_samplerate);
    } else if (strcmp(val.enc_codec, "g726") == 0) {
        if (val.enc_samplerate == 16000) {
            comp = (void *)aml_ipc_g726_new(AML_ACODEC_ADPCM_G726, AML_G726_16K, AML_G726_LE);
        } else if (val.enc_samplerate == 24000) {
            comp = (void *)aml_ipc_g726_new(AML_ACODEC_ADPCM_G726, AML_G726_24K, AML_G726_LE);
        } else if (val.enc_samplerate == 32000) {
            comp = (void *)aml_ipc_g726_new(AML_ACODEC_ADPCM_G726, AML_G726_32K, AML_G726_LE);
        } else if (val.enc_samplerate == 40000) {
            comp = (void *)aml_ipc_g726_new(AML_ACODEC_ADPCM_G726, AML_G726_40K, AML_G726_LE);
        } else {
            printf("!!! g726 samplerate error !!!\n");
            exit(1);
        }
    } else if (strcmp(val.enc_codec, "g711-ulaw") == 0) {
        comp = (void *)aml_ipc_g711_new(AML_ACODEC_PCM_ULAW);
    } else if (strcmp(val.enc_codec, "g711-alaw") == 0) {
        comp = (void *)aml_ipc_g711_new(AML_ACODEC_PCM_ALAW);
    } else {
        printf("!!! encoder error !!!\n");
        exit(1);
    }

    aml_ipc_bind(AML_IPC_COMPONENT(aconv), AML_ACONVERT_OUTPUT, comp, AML_ACODEC_INPUT);
    aml_ipc_add_frame_hook(comp, AML_ACODEC_OUTPUT, dump_fp, on_frame);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(ain), aconv, comp, NULL);
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
    aml_ipc_start(comp);
    aml_ipc_start(AML_IPC_COMPONENT(aconv));
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
    aml_ipc_stop(AML_IPC_COMPONENT(aconv));
    aml_ipc_stop(AML_IPC_COMPONENT(ain));
    aml_obj_release(AML_OBJECT(comp));
    aml_obj_release(AML_OBJECT(aconv));
    aml_obj_release(AML_OBJECT(ain));
#endif

    fclose(dump_fp);
    return 0;
}
