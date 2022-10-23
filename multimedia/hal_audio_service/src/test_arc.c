
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

//#include</linux/amlogic/cec_common.h>
#include "hdmi_tx_cec_20.h"
#include "data.h"
#include "audio_if.h"


#define DEV_CEC			"/dev/cec"
static int      Cec_fd = 0;
#define HDR 0
#define OPCODE 1
#define OPERAND 2

#define CEC_MSG_HDR_BROADCAST 0x0F
#define CEC_MSG_HDR 0x05
#define INITIATE_ARC                     0XC0
#define	REPORT_ARC_INITIATED             0XC1
#define REPORT_ARC_TERMINATED            0XC2
#define	REQUEST_ARC_INITIATION           0XC3
#define	REQUEST_ARC_TERMINATION          0XC4
#define	TERMINATE_ARC                    0XC5
char CEC_MSG_ARC_INITIATED[]={CEC_MSG_HDR,REPORT_ARC_INITIATED};

static pthread_t CEC_Event_Thread;
struct pollfd polling;
static bool arc_not_initialised = true;
static char cecmsg[16] ;
static int stop =0;
static char msg_to_send[16];

/* Audio stuffs */


#define TV_AUDIO_OUTPUT

#ifdef TV_AUDIO_OUTPUT
static struct audio_port_config source_;
static struct audio_port_config sink_;
static audio_patch_handle_t patch_h_;

static void test_patch(audio_hw_device_t *device)
{
    int ret;
    /* create mixer to device patch */
    source_.id = 1;
    source_.role = AUDIO_PORT_ROLE_SOURCE;
    source_.type = AUDIO_PORT_TYPE_MIX;
    source_.config_mask = AUDIO_PORT_CONFIG_SAMPLE_RATE |
        AUDIO_PORT_CONFIG_FORMAT;
    source_.sample_rate = 48000;
    source_.format = AUDIO_FORMAT_PCM_16_BIT;

    sink_.id = 2;
    sink_.role = AUDIO_PORT_ROLE_SINK;
    sink_.type = AUDIO_PORT_TYPE_DEVICE;
    sink_.config_mask = AUDIO_PORT_CONFIG_SAMPLE_RATE |
        AUDIO_PORT_CONFIG_FORMAT;
    sink_.sample_rate = 48000;
    sink_.format = AUDIO_FORMAT_PCM_16_BIT;
    sink_.ext.device.type = AUDIO_DEVICE_OUT_HDMI_ARC;

    printf("create mix --> ARC patch...");
    ret = device->create_audio_patch(device, 1, &source_, 1, &sink_, &patch_h_);
    if (ret) {
        printf("fail ret:%d\n",ret);
    } else {
        printf("success\n");
    }
}

static void destroy_patch(audio_hw_device_t *device)
{
    if (patch_h_) {
        int ret;
        printf("destroy patch...");
        ret = device->release_audio_patch(device, patch_h_);
        if (ret) {
            printf("fail ret:%d\n",ret);
        } else {
            printf("success\n");
        }
    }
}
#endif
static void test_vol(audio_hw_device_t *device, int gain)
{
    int ret;
    float vol = 0;
    ret = device->get_master_volume(device, &vol);
    if (ret) {
        printf("get_master_volume fail\n");
    } else {
        printf("cur master vol:%f\n", vol);
    }
    ret = device->set_master_volume(device, 0.5f);
    if (ret) {
        printf("set_master_volume fail\n");
    } else {
        printf("set master vol 0.5\n");
        device->set_master_volume(device, vol);
    }

#ifdef  TV_AUDIO_OUTPUT
    {
        struct audio_port_config config;

        memset(&config, 0, sizeof(config));
        config.id = 2;
        config.role = AUDIO_PORT_ROLE_SINK;
        config.type = AUDIO_PORT_TYPE_DEVICE;
        config.config_mask = AUDIO_PORT_CONFIG_GAIN;
        /* audio_hal use dB * 100 to keep the accuracy */
        config.gain.values[0] = gain * 100;
        printf("set gain to %ddB...\n", gain);
        ret = device->set_audio_port_config(device, &config);
        if (ret) {
            printf("fail\n");
        } else {
            printf("success\n");
        }
    }
#endif
}

#define WRITE_UNIT 4096
#define min(a, b) ((a) < (b) ? (a) : (b))
static int test_stream(struct audio_stream_out *stream)
{
    int i, ret;
    int len;
    unsigned char *data;

    ret = stream->set_volume(stream, 1.0f, 1.0f);
    if (ret) {
        fprintf(stderr, "%s %d, ret:%x\n", __func__, __LINE__, ret);
        return -1;
    }

    printf("Start output to audio HAL\n");
    for (i = 0; i < 10; i++) {
        data = wav_data;
        len = sizeof(wav_data);
        while (len > 0) {
            ssize_t s = stream->write(stream, data, min(len, WRITE_UNIT));
            if (s < 0) {
                fprintf(stderr, "stream writing error %d\n", s);
                break;
            }

            len -= s;
            data += s;
        }
    }

    return 0;
}

#define HDMI_ARC_OUTPUT_DD

static void test_output_stream(audio_hw_device_t *device)
{
    int ret;
    struct audio_config config;
    struct audio_stream_out *stream;
    char cmd[32];

    memset(&config, 0, sizeof(config));
    config.sample_rate = 48000;
    config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    config.format = AUDIO_FORMAT_PCM_16_BIT;

    printf("Tell audio HAL HDMI ARC is connected\n");
    ret = device->set_parameters(device, "connect=0x40000");
    if (ret) {
        fprintf(stderr, "set_parameters HDMI connected failed\n");
    }

#ifdef HDMI_ARC_OUTPUT_DD
    printf("Set HDMI ARC output format to AC3\n");
    ret = device->set_parameters(device, "hdmi_format=4");
    if (ret) {
        fprintf(stderr, "set_parameters failed\n");
    }
#else
    printf("Set HDMI ARC output format to PCM\n");
    ret = device->set_parameters(device, "hdmi_format=0");
    if (ret) {
        fprintf(stderr, "set_parameters failed\n");
    }
#endif

#ifdef HDMI_ARC_OUTPUT_DD
    /*
     * The following set_parameters calling sequence is needed
     * to meet the audio HAL internal checking for ARC output:
     * 1. "HDMI ARC Switch" key is used for UI control switch to enable
     *    ARC output.
     * 2. "connect" key is used to tell audio HAL the connection status
     * 3. "speaker_mute" is also checked to make sure ARC output is
     *    effective.
     */
    printf("Report UI settings for HDMI ARC enabled\n");
    ret = device->set_parameters(device, "HDMI ARC Switch=1");
    if (ret) {
        fprintf(stderr, "set_parameters failed\n");
    }

    printf("Report HDMI ARC is connected\n");
    snprintf(cmd, sizeof(cmd), "connect=%d", AUDIO_DEVICE_OUT_HDMI_ARC);
    ret = device->set_parameters(device, cmd);
    if (ret) {
        fprintf(stderr, "set_parameters failed\n");
    }

    printf("Mute speaker output\n");
    ret = device->set_parameters(device, "speaker_mute=1");
    if (ret) {
        fprintf(stderr, "set_parameters failed\n");
    }
#endif

    printf("open output to HDMI_ARC with direct mode...\n");
    ret = device->open_output_stream(device,
            0, AUDIO_DEVICE_OUT_HDMI_ARC,
            AUDIO_OUTPUT_FLAG_PRIMARY | AUDIO_OUTPUT_FLAG_DIRECT, &config,
            &stream, NULL);
    if (ret) {
        printf("fail\n");
        return;
    } else {
        printf("success\n");
    }

    test_stream(stream);

    printf("close output speaker...\n");
    device->close_output_stream(device, stream);
}

void process_cec_msg(char * cec_msg,int len)
{
    int i=0;

    switch (cec_msg[OPCODE]) {
       case INITIATE_ARC :
           printf("Got the Initiate ARC message Amplifeir requesting to start ARC\n");
           arc_not_initialised = false;
           write(Cec_fd,CEC_MSG_ARC_INITIATED,2);
           printf("Sending the ARC initiated message to the Amplifier\n");
           stop =1;
           break;

       case CEC_OC_GIVE_PHYSICAL_ADDRESS :
           printf("CEC_OC_GIVE_PHYSICAL_ADDRESS\n");
           msg_to_send[0] = CEC_MSG_HDR_BROADCAST;
           msg_to_send[1] = CEC_OC_REPORT_PHYSICAL_ADDRESS;
           msg_to_send[2] = 0x00;
           msg_to_send[3] = 0x00;
           msg_to_send[4] = 0x00;
           write(Cec_fd,msg_to_send,5);
           printf("Sending CEC_OC_REPORT_PHYSICAL_ADDRESS\n");
           break;
       default:
          {
               printf("CEC msg is ");
               for(i=0;i< len;i++) {
                   printf(" 0x%x ",cec_msg[i]);
               }
          }
          printf("\n###################");
          break;
    }
}

void* CEC_event_read_fn (void *data)
{
    int timeout = -1;
    unsigned int mask = 0;
    int read_len =0;

    polling.fd= Cec_fd;
    polling.events = POLLIN;

    while (!stop) { 
        mask = poll(&polling,1,timeout);

        if (mask & POLLIN || mask & POLLRDNORM) {
            read_len= read(Cec_fd, &cecmsg, 1);
            process_cec_msg(cecmsg,read_len);
        }
    }
}

static void sighandler(int sig)
{
    printf("Interrupted by signal %d\n", sig);
    stop= 1;
    printf("sighandler! Done\n");
}

int main(int argc, char *argv[]) 
{
    char msg[16];
    int ret=0;
    audio_hw_device_t *device;
    int test_earc = 0;

    if (argc > 1) {
        test_earc = atoi(argv[1]);
        if (test_earc) {
            printf("Test EARC output, CEC communicatin will be skipped\n");
        }
    }

    if (!test_earc) {
        Cec_fd = open (DEV_CEC, O_RDWR);

        if (Cec_fd < 0) {
            printf ("%s CEC_device opening returned %d",DEV_CEC);
            return -1;
        }

        ret = pthread_create(&CEC_Event_Thread, NULL, CEC_event_read_fn, NULL);
        if (ret) {
            printf("Unable to create decoder event thread\n");
        }

        /* setup the signal handler for CTRL-C */
        signal(SIGINT, sighandler);
        /*kill -USR1 pid */
        signal(SIGUSR1, sighandler);

        ioctl(Cec_fd, CEC_IOC_SET_OPTION_SYS_CTRL, 0x8);
	
        ioctl(Cec_fd, CEC_IOC_ADD_LOGICAL_ADDR, 0x0);
        msg[HDR]=CEC_MSG_HDR;
        msg[OPCODE]=REQUEST_ARC_INITIATION;
        write(Cec_fd,msg,2);
        printf("sending ARC initialisation \n");
    }

    ret = audio_hw_load_interface(&device);
    if (ret) {
        printf("%s %d error:%d\n", __func__, __LINE__, ret);
        return ret;
    }
    printf("hw version: %x\n", device->common.version);
    printf("hal api version: %x\n", device->common.module->hal_api_version);
    printf("module id: %s\n", device->common.module->id);
    printf("module name: %s\n", device->common.module->name);

    if (device->get_supported_devices) {
        uint32_t support_dev = 0;
        support_dev = device->get_supported_devices(device);
        printf("supported device: %x\n", support_dev);
    }

    int inited = device->init_check(device);
    if (inited) {
        printf("device not inited, quit\n");
        goto exit;
    }

    if (!test_earc) {
        while (!stop) {
            if (arc_not_initialised) {
                printf("TV sending Request ARC initialisation to Amplifier \n");
                write(Cec_fd,msg,2);
            }
            sleep(1);
        }
    }

    sleep(2);

    #ifdef TV_AUDIO_OUTPUT
    test_patch(device);
    #endif

    test_output_stream(device);
	  
    if (!test_earc) {
        ret = pthread_join(CEC_Event_Thread, NULL);
        if (ret != 0) {
            printf("CEC_Event_Thread returned error\n");
        }

        ioctl(Cec_fd, CEC_IOC_CLR_LOGICAL_ADDR, 0x0);
        ioctl(Cec_fd, CEC_IOC_SET_OPTION_SYS_CTRL, 0x0);
	
        close(Cec_fd);
    }

    /* Audio clean up */
#ifdef TV_AUDIO_OUTPUT
    destroy_patch(device);
#endif

exit:
    device->common.close(&device->common);
    audio_hw_unload_interface(device);
    return 0;
	
}



