/*
 * This example demostrate changging setting and restart the pipeline
 */

#include "aml_ipc_sdk.h"
#include <signal.h>
#include <unistd.h>

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

    struct AmlIPCISP *isp = aml_ipc_isp_new("/dev/video0", 4, 0, 0);
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_NV12, 1920, 1080, 30};
    aml_ipc_isp_set_capture_format(isp, AML_ISP_FR, &fmt);

    struct AmlIPCVenc *enc = aml_ipc_venc_new(AML_VCODEC_H264);

    struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
    aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(isp), enc, NULL);
    aml_ipc_bind(AML_IPC_COMPONENT(isp), AML_ISP_FR, AML_IPC_COMPONENT(enc), AML_VENC_INPUT);
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
    sleep(3);
    // pause the pipeline and config new parameters
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    fmt.fps = 25;
    aml_ipc_isp_set_capture_format(isp, AML_ISP_FR, &fmt);
    aml_obj_set_properties(AML_OBJECT(enc), "codec", AML_VCODEC_H265, NULL);
    // now restart the pipeline with new configuration
    aml_ipc_start(AML_IPC_COMPONENT(pipeline));
    sleep(3);
    aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
    aml_obj_release(AML_OBJECT(pipeline));

    return 0;
}


