#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include "audio_hal.h"

//#define MEM_CHECK
#ifdef MEM_CHECK
#include <mcheck.h>
#endif


int format_changed_callback(audio_callback_info_t *info, void * args)
{
    printf("%s format=0x%x\n", __func__, *(audio_format_t*)(args));

    return 0;
}



int spdif_source(audio_hw_device_t *dev)
{
    int rc;

    unsigned int num_sources = 0;
    unsigned int num_sinks = 0;
    struct audio_port_config sources;
    struct audio_port_config sinks;
    audio_patch_handle_t halPatch = AUDIO_PATCH_HANDLE_NONE;
    int cnt = 0;



    dev->set_parameters(dev, "audio_latency=20");

    /*spdif_source iec61937_align is 1*/
    dev->set_parameters(dev, "iec61937_align=1");

    /* create the audio patch*/
    memset(&sources, 0 , sizeof(struct audio_port_config));
    num_sources = 1;
    num_sinks = 1;
    sources.id = 1;
    sources.role = AUDIO_PORT_ROLE_SOURCE;
    sources.type = AUDIO_PORT_TYPE_DEVICE;
    sources.ext.device.type = AUDIO_DEVICE_IN_SPDIF;

    memset(&sinks, 0 , sizeof(struct audio_port_config));
    sinks.id = 2;
    sinks.role = AUDIO_PORT_ROLE_SINK;
    sinks.type = AUDIO_PORT_TYPE_DEVICE;
    sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev, num_sources, &sources, num_sinks, &sinks, &halPatch);
    printf("audio patch handle=0x%x\n", halPatch);
    if (rc) {
        printf("create audio patch failed =%d\n", rc);
        return -1;
    }


    audio_callback_info_t  callback_info;
    callback_info.id = getpid();
    callback_info.type = AML_AUDIO_CALLBACK_FORMATCHANGED;
    audio_callback_func_t  callback_func = format_changed_callback;

    dev->install_callback_audio_patch(dev, halPatch, &callback_info, callback_func);


    while (cnt <= 20) {
        sleep(1);
        cnt++;
        printf("test spdif running =%d\n", cnt);
    }
    dev->remove_callback_audio_patch(dev, halPatch, &callback_info);

    printf("release patch func=%p\n", dev->release_audio_patch);
    rc = dev->release_audio_patch(dev, halPatch);
    if (rc) {
        printf("release audio patch failed =%d\n", rc);
        return -1;
    }


    return 0;
}

int linein_source(audio_hw_device_t *dev)
{
    int rc;

    unsigned int num_sources = 0;
    unsigned int num_sinks = 0;
    struct audio_port_config sources;
    struct audio_port_config sinks;
    audio_patch_handle_t halPatch = AUDIO_PATCH_HANDLE_NONE;
    int cnt = 0;



    /* create the audio patch*/
    memset(&sources, 0 , sizeof(struct audio_port_config));
    num_sources = 1;
    num_sinks = 1;
    sources.id = 1;
    sources.role = AUDIO_PORT_ROLE_SOURCE;
    sources.type = AUDIO_PORT_TYPE_DEVICE;
    sources.ext.device.type = AUDIO_DEVICE_IN_LINE;

    memset(&sinks, 0 , sizeof(struct audio_port_config));
    sinks.id = 2;
    sinks.role = AUDIO_PORT_ROLE_SINK;
    sinks.type = AUDIO_PORT_TYPE_DEVICE;
    sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev, num_sources, &sources, num_sinks, &sinks, &halPatch);
    printf("audio patch handle=0x%x\n", halPatch);
    if (rc) {
        printf("create audio patch failed =%d\n", rc);
        return -1;
    }

    while (cnt <= 20) {
        sleep(1);
        cnt++;
        printf("test linein running =%d\n", cnt);
    }
    printf("release patch\n");
    rc = dev->release_audio_patch(dev, halPatch);
    if (rc) {
        printf("release audio patch failed =%d\n", rc);
        return -1;
    }


    return 0;
}

int loopback_source(audio_hw_device_t *dev)
{
    int rc;

    unsigned int num_sources = 0;
    unsigned int num_sinks = 0;
    struct audio_port_config sources;
    struct audio_port_config sinks;
    audio_patch_handle_t halPatch = AUDIO_PATCH_HANDLE_NONE;
    int cnt = 0;

    dev->set_parameters(dev, "capture_samplerate=48000");
    dev->set_parameters(dev, "capture_ch=2");

    /*loopback_source pcm format, iec61937_align is 0*/
    dev->set_parameters(dev, "iec61937_align=0");


    /* create the audio patch*/
    memset(&sources, 0 , sizeof(struct audio_port_config));
    num_sources = 1;
    num_sinks = 1;
    sources.id = 1;
    sources.role = AUDIO_PORT_ROLE_SOURCE;
    sources.type = AUDIO_PORT_TYPE_DEVICE;
    sources.ext.device.type = AUDIO_DEVICE_IN_LOOPBACK;

    memset(&sinks, 0 , sizeof(struct audio_port_config));
    sinks.id = 2;
    sinks.role = AUDIO_PORT_ROLE_SINK;
    sinks.type = AUDIO_PORT_TYPE_DEVICE;
    sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev, num_sources, &sources, num_sinks, &sinks, &halPatch);
    printf("audio patch handle=0x%x\n", halPatch);
    if (rc) {
        printf("create audio patch failed =%d\n", rc);
        return -1;
    }

    while (cnt <= 20) {
        sleep(1);
        cnt++;
        printf("test loopback in running =%d\n", cnt);
    }
    printf("release patch\n");
    rc = dev->release_audio_patch(dev, halPatch);
    if (rc) {
        printf("release audio patch failed =%d\n", rc);
        return -1;
    }


    return 0;
}

#define MAX_DECODER_MAT_FRAME_LENGTH 61424
#define MAX_DECODER_DDP_FRAME_LENGTH 0X1800
#define MAX_DECODER_THD_FRAME_LENGTH 8190

char temp_buf[MAX_DECODER_MAT_FRAME_LENGTH];

static void byte_swap(unsigned char * input_buf, int size) {
    unsigned char * buf = (unsigned char *) input_buf;
    unsigned char data = 0;
    int i = 0;
    for (i = 0; i < size; i += 2) {
        data = buf[i];
        buf[i] = buf[i + 1];
        buf[i + 1] = data;
    }

    return;
}

int media_source(audio_hw_device_t *dev, audio_format_t  format, char * file_name)
{
    int rc = 0;
    audio_io_handle_t handle;
    audio_stream_out_t *stream_out = NULL;
    struct audio_config config;
    unsigned int num_sources = 0;
    unsigned int num_sinks = 0;
    struct audio_port_config sources;
    struct audio_port_config sinks;
    audio_patch_handle_t halPatch = AUDIO_PATCH_HANDLE_NONE;
    FILE *fp_input = NULL;
    int length = 512;
    int endian_detect = 0;
    int need_swap = 0;

    int read_size = 0;

    /*media_source iec61937_align is 0*/
    dev->set_parameters(dev, "iec61937_align=0");

    if (format == AUDIO_FORMAT_AC3) {
        length = 512;
    } else if (format == AUDIO_FORMAT_MAT) {
        length = MAX_DECODER_MAT_FRAME_LENGTH;
    } else if (format == AUDIO_FORMAT_DOLBY_TRUEHD) {
        length = MAX_DECODER_THD_FRAME_LENGTH;
    } else if (format == AUDIO_FORMAT_DTS) {
        length = 512;
    } else {
        return 0;
    }

    config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    config.sample_rate  = 48000;
    config.format       = format;

    /* open the output stream */
    rc = dev->open_output_stream(dev,
                                 handle,
                                 AUDIO_DEVICE_OUT_SPEAKER,
                                 AUDIO_OUTPUT_FLAG_DIRECT,
                                 &config,
                                 &stream_out,
                                 NULL);
    if (rc) {
        printf("open output stream failed\n");
        return -1;

    }

    /* create the media audio patch */
    memset(&sources, 0 , sizeof(struct audio_port_config));
    num_sources = 1;
    num_sinks = 1;
    sources.id = 1;
    sources.role = AUDIO_PORT_ROLE_SOURCE;
    sources.type = AUDIO_PORT_TYPE_MIX;


    memset(&sinks, 0 , sizeof(struct audio_port_config));
    sinks.id = 2;
    sinks.role = AUDIO_PORT_ROLE_SINK;
    sinks.type = AUDIO_PORT_TYPE_DEVICE;
    sinks.ext.device.type = AUDIO_DEVICE_OUT_SPEAKER;

    rc = dev->create_audio_patch(dev, num_sources, &sources, num_sinks, &sinks, &halPatch);
    printf("audio patch handle=0x%x\n", halPatch);
    if (rc) {
        printf("create audio patch failed =%d\n", rc);
        return -1;
    }

    printf("file name=%s", file_name);
    fp_input = fopen(file_name, "r+");
    if (fp_input == NULL) {
        printf("open input file failed\n");

    } else {
        /* write data into outputstream*/
        while (1) {
            read_size = fread(temp_buf, 1, length, fp_input);
            if (format == AUDIO_FORMAT_MAT && endian_detect == 0) {
                if (temp_buf[0] == 0x07 && temp_buf[1] == 0x9e) {
                    need_swap = 1;
                }
                endian_detect = 1;
            }
            if (read_size <= 0) {
                printf("read error\n");
                fclose(fp_input);
                break;
            }
            /*only do swap for mat*/
            if (need_swap) {
                byte_swap(temp_buf, read_size);
            }

            //printf("read data=%d\n",read_size);
            stream_out->write(stream_out, temp_buf, read_size);
            //printf("after write data\n");

        }
    }

    stream_out->common.standby(&stream_out->common);

    dev->close_output_stream(dev, stream_out);

    /* release the audio patch*/
    rc = dev->release_audio_patch(dev, halPatch);
    if (rc) {
        printf("release audio patch failed =%d\n", rc);
        return -1;
    }



    return 0;
}

int main(int argc, char *argv[])
{
    audio_hw_device_t *dev;
    hw_module_t *mod = NULL;
    int rc;
    int cnt = 0;
    char * point = NULL;
#ifdef MEM_CHECK
    mtrace();
#endif
    if (argc != 2 && argc != 3 && argc != 4) {
        printf("cmd should be: \n");
        printf("************************\n");
        printf("test spdifin \n");
        printf("test linein \n");
        printf("test loopback \n");
        printf("test mediain ac3, dts, mat, mlp  input file\n");
        printf("************************\n");


        return -1;
    }
    /* get the audio module*/
    rc = audio_hw_device_get_module(&mod);
    printf("audio_hw_device_get_module rc=%d mod addr=%p\n", rc, mod);
    if (rc) {
        printf("audio_hw_device_get_module failed\n");
        return -1;

    }

    /* open the devce*/
    rc = audio_hw_device_open(mod, &dev);
    printf("audio_hw_device_open rc=%d dev=%p\n", rc, dev);
    if (rc) {
        printf("audio_hw_device_open failed\n");
        return -1;
    }

    dev->set_parameters(dev, "speakers=lr:c:lfe:lrs:lre");

    if (strcmp(argv[1], "spdifin") == 0) {
        spdif_source(dev);
    } else if (strcmp(argv[1], "linein") == 0) {
        linein_source(dev);
    } else if (strcmp(argv[1], "loopback") == 0) {
        loopback_source(dev);
    }else if (strcmp(argv[1], "mediain") == 0) {
        audio_format_t  format = AUDIO_FORMAT_INVALID;
        if (argc != 4) {
            goto exit;
        }
        if (strcmp(argv[2], "ac3") == 0) {
            format = AUDIO_FORMAT_AC3;
        } else if (strcmp(argv[2], "mat") == 0) {
            format = AUDIO_FORMAT_MAT;
        } else if (strcmp(argv[2], "mlp") == 0) {
            format = AUDIO_FORMAT_DOLBY_TRUEHD;
        } else if (strcmp(argv[2], "dts") == 0) {
            format = AUDIO_FORMAT_DTS;
        }
        media_source(dev, format, argv[3]);
    }else if (strcmp(argv[1], "all") == 0){
        //run all the case
        spdif_source(dev);
        linein_source(dev);
    }

#if 0
    while (cnt <= 20) {
        sleep(1);
        cnt++;
        printf("waiting exit =%d\n", cnt);
    }
#endif

exit:
    /* close the audio device*/
    rc = audio_hw_device_close(dev);
    printf("audio_hw_device_close rc=%d\n", rc);
#ifdef MEM_CHECK
    muntrace();
#endif
    return 0;
}


