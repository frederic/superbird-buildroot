/*
 * This example demostrate how to push data to IPC component
 * it read pcm data from a file and send it to aout
 */

#include <signal.h>
#include <unistd.h>
#include "aml_ipc_sdk.h"

static volatile int bexit = 0;
void signal_handle(int sig) { bexit = 1; }

static struct AmlIPCFrame *read_pcm_frame(FILE *fp, int rate, int nch, int sample_to_read) {
    static int64_t total_samples = 0LL;
    if (feof(fp))
    {
        bexit =1;
        return NULL;
    }
    struct AmlIPCAudioFrame *frm = AML_OBJ_NEW(AmlIPCAudioFrame);
    frm->format.codec = AML_ACODEC_PCM_S16LE;
    frm->format.sample_width = 2*8;
    frm->format.sample_rate = rate;
    frm->format.num_channel = nch;
    AML_IPC_FRAME(frm)->pts_us = total_samples * 1000000LL / rate;
    int data_size = sample_to_read * nch * frm->format.sample_width / 8;
    AML_IPC_FRAME(frm)->data = malloc(data_size);
    int size = fread(AML_IPC_FRAME(frm)->data, 1, data_size, fp);
    AML_IPC_FRAME(frm)->size = size;
    total_samples += size / nch * 8 / frm->format.sample_width;
    return AML_IPC_FRAME(frm);
}

int main(int argc, const char *argv[]) {
    FILE *fp;
    if ((argc != 3) || ((fp = fopen(argv[2], "rb")) == NULL)) {
        printf("USAGE: %s out_dev pcmfile\n", argv[0]);
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

    struct AmlIPCAOut *aout = aml_ipc_aout_new();

    aml_obj_set_properties(AML_OBJECT(aout), "device", argv[1], NULL);
    aml_ipc_start(AML_IPC_COMPONENT(aout));

    while (!bexit) {
        struct AmlIPCFrame *frame = read_pcm_frame(fp, 48000, 2, 1024);
        if (frame) {
            AmlStatus status = aml_ipc_send_frame(AML_IPC_COMPONENT(aout), AML_AOUT_INPUT, frame);
            if (status != AML_STATUS_OK)
                break;
        } else {
            usleep(1000 * 10);
        }
    }
    fclose(fp);
    aml_ipc_stop(AML_IPC_COMPONENT(aout));
    aml_obj_release(AML_OBJECT(aout));
    return 0;
}
