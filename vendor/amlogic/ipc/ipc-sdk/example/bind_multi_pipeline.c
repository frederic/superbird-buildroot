/*
 * This example demostrate binding multiple input channels to a single output channel
 * Not like the example bind_multi.c, this example use a pipeline to manage these components
 */

#include "aml_ipc_sdk.h"
#include <signal.h>
#include <unistd.h>

static AmlStatus dump_data(struct AmlIPCFrame *frm, void *param) {
    fwrite(frm->data, 1, frm->size, (FILE *)param);
    return AML_STATUS_OK;
}

static volatile int bexit = 0;
void signal_handle(int sig) { bexit = 1; }

int main(int argc, const char *argv[]) {

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

    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();

    struct AmlIPCAIn *ain = aml_ipc_ain_new();
    aml_ipc_pipeline_add(pipeline, AML_IPC_COMPONENT(ain));
    // create audio encoders
    struct {
        struct AmlIPCComponent *comp;
        FILE *fp;
    } recivers[] = {{(void *)aml_ipc_g711_new(AML_ACODEC_PCM_ULAW), fopen("ulaw.dat", "w")},
                    {(void *)aml_ipc_g711_new(AML_ACODEC_PCM_ALAW), fopen("alaw.dat", "w")},
                    {(void *)aml_ipc_g726_new(AML_ACODEC_ADPCM_G726, AML_G726_16K, AML_G726_BE),
                     fopen("g726_16.dat", "w")},
                    {(void *)aml_ipc_g726_new(AML_ACODEC_ADPCM_G726, AML_G726_24K, AML_G726_BE),
                     fopen("g726_24.dat", "w")}};

    for (int i = 0; i < sizeof(recivers) / sizeof(recivers[0]); ++i) {
        // bind all encoders to the same channel
        aml_ipc_bind(AML_IPC_COMPONENT(ain), AML_AIN_OUTPUT, recivers[i].comp, AML_ACODEC_INPUT);
        // use hook to dump the data
        aml_ipc_add_frame_hook(recivers[i].comp, AML_ACODEC_OUTPUT, recivers[i].fp, dump_data);
        // add encoder to pipeline
        aml_ipc_pipeline_add(pipeline, recivers[i].comp);
    }
    // start the pipeline
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));

    while (!bexit) {
        usleep(1000 * 10);
    }

    // stop the pipeline
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    // release the pipeline
    aml_obj_release(AML_OBJECT(pipeline));
    // close the dump file
    for (int i = 0; i < sizeof(recivers) / sizeof(recivers[0]); ++i) {
        fclose(recivers[i].fp);
    }
    return 0;
}

