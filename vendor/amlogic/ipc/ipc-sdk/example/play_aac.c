/*
 * This example demostrate how to push data to IPC component
 * it read aac data from a file and send it to decoder and then aout
 */

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "aml_ipc_sdk.h"

static volatile int bexit = 0;
void signal_handle(int sig) { bexit = 1; }

/*
 * https://wiki.multimedia.cx/index.php/ADTS
 * AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ)
 *
 */
static struct AmlIPCFrame *read_aac_frame(FILE *fp) {
    if (feof(fp))
        return NULL;
    uint8_t header[7];
    if (fread(header, 1, sizeof(header), fp) != sizeof(header)) {
        return NULL;
    }
    if (header[0] != 0xff || (header[1] & 0b11110110) != 0b11110000) {
        AML_LOGE("wrong aac header %02x %02x", header[0], header[1]);
        return NULL;
    }
    //int mpeg_ver = (header[1] & 0b00001000) ? 2 : 4;
    //int has_crc = (header[1] & 0x01) ? 0 : 1;
    int profile = (header[2] >> 6) + 1;
    int freq = (header[2] >> 2) & 0x0f;
    int channel = ((header[2] & 0x01) << 2) | (header[3] >> 6);
    int frmlen = ((header[3] & 0x03) << 11) | (header[4] << 3) | (header[5] >> 5);
    static int frequency_tbl[13] = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                    22050, 16000, 12000, 11025, 8000,  7350};
    struct AmlIPCAudioAACFrame *frm = AML_OBJ_NEW(AmlIPCAudioAACFrame);
    frm->aac_aot = profile;
    AML_IPC_AUDIO_FRAME(frm)->format.codec = AML_ACODEC_AAC;
    AML_IPC_AUDIO_FRAME(frm)->format.sample_rate = frequency_tbl[freq];
    AML_IPC_AUDIO_FRAME(frm)->format.num_channel = channel;
    AML_IPC_FRAME(frm)->data = malloc(frmlen);
    memcpy(AML_IPC_FRAME(frm)->data, header, sizeof(header));
    int size = fread(AML_IPC_FRAME(frm)->data + sizeof(header), 1, frmlen - sizeof(header), fp);
    AML_IPC_FRAME(frm)->size = size + sizeof(header);
    return AML_IPC_FRAME(frm);
}

int main(int argc, const char *argv[]) {
    FILE *fp;
    if ((argc != 2) || ((fp = fopen(argv[1], "rb")) == NULL)) {
        printf("USAGE: %s aacfile\n", argv[0]);
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

    struct AmlIPCACodecFDK *adec = aml_ipc_fdkaac_new(AML_ACODEC_PCM_S16LE, AML_AAC_LC);
    aml_ipc_fdkaac_set_adts_header(adec, 1);
    struct AmlIPCAOut *aout = aml_ipc_aout_new();
    aml_ipc_bind(AML_IPC_COMPONENT(adec), AML_ACODEC_OUTPUT, AML_IPC_COMPONENT(aout),
                 AML_AOUT_INPUT);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
  struct AmlIPCPipeline *pipeline = aml_ipc_pipeline_new();
  aml_ipc_pipeline_add_many(pipeline, AML_IPC_COMPONENT(aout), adec, NULL);
  aml_ipc_start(AML_IPC_COMPONENT(pipeline));
#else
    aml_ipc_start(AML_IPC_COMPONENT(aout));
    aml_ipc_start(AML_IPC_COMPONENT(adec));
#endif

    while (!bexit) {
        struct AmlIPCFrame *frame = read_aac_frame(fp);
        if (frame) {
            AmlStatus status = aml_ipc_send_frame(AML_IPC_COMPONENT(adec), AML_ACODEC_INPUT, frame);
            if (status != AML_STATUS_OK)
                break;
        } else {
            usleep(1000 * 10);
        }
    }
    fclose(fp);

#if defined(IPC_EXAMPLE_USE_PIPELINE)
  aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
  aml_obj_release(AML_OBJECT(pipeline));
#else
    aml_ipc_stop(AML_IPC_COMPONENT(aout));
    aml_ipc_stop(AML_IPC_COMPONENT(adec));

    aml_obj_release(AML_OBJECT(aout));
    aml_obj_release(AML_OBJECT(adec));
#endif
    return 0;
}
