/*
 * Sample Dolby Audio for Wireless Speakers (DAWS) ALSA external plugin
 *
 * Copyright (c) 2018 by Amlogic
 */

#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include <stdio.h>
#include <dlfcn.h>
#include "snd_convert.h"
#include "aml_ringbuffer.h"

/*enable dump data for debug*/
#define DUMP_ENABLE  1

#define PLUG_OUTPUT_DEFAULT_CH                        2
#define MAX_DAWS_SUPPORTED_OUT_CHANNELS   8
#define MAX_DAWS_SUPPORTED_IN_CHANNELS      2
#define MAX_DAWS_SUPPORTED_FORMATS             3
#define MAX_DAWS_SUPPORTED_SAMPLE_SIZE      512

typedef struct {
    snd_pcm_extplug_t ext;
    unsigned short output_channels;
    int init_enabled;
    int passthrough_enable;
    int16_t *pcm_in_buffer;
    int16_t *pcm_out_buffer;
    snd_pcm_uframes_t bufread;
    ring_buffer_t in_ringbuffer;
    ring_buffer_t out_ringbuffer;

    /*daws lib interface */
    int (*daws_effect_init)(unsigned, unsigned int, unsigned int);
    void (*daws_effect_close)();
    int (*daws_effect_process)(void *, snd_pcm_uframes_t, void *, snd_pcm_uframes_t *);
    void *fd ;
#ifdef DUMP_ENABLE
    int dump_enable;
    FILE* input_fp;
    FILE* output_fp;
    const char* input_fn;
    const char* output_fn;
#endif
} daws_plug_info;

static inline void *area_addr(const snd_pcm_channel_area_t *area,
                              snd_pcm_uframes_t offset)
{
    unsigned int bitofs = area->first + area->step * offset;
    return (char *) area->addr + bitofs / 8;
}

static int daws_plugin_init(snd_pcm_extplug_t *ext)
{
    daws_plug_info *daws = (daws_plug_info *)ext;
    int ret;

    if (daws->init_enabled == 1)
        return 0;

    daws->fd = dlopen("/tmp/ds/0x21_0x1234_0x1d.so", RTLD_LAZY);
    if (daws->fd != NULL) {
        daws->daws_effect_init = dlsym(daws->fd, "dolby_audio_wireless_speakers_init");
        if (daws->daws_effect_init == NULL) {
            SNDERR("cant find lib interface %s\n", dlerror());
            daws->passthrough_enable = 1;
        }
        daws->daws_effect_process = dlsym(daws->fd, "dolby_audio_wireless_speakers_process");
        if (daws->daws_effect_process == NULL) {
            SNDERR("cant find lib interface %s\n", dlerror());
            daws->passthrough_enable = 1;
        }
        daws->daws_effect_close = dlsym(daws->fd, "dolby_audio_wireless_speakers_close");
        if (daws->daws_effect_close == NULL) {
            SNDERR("cant find lib interface %s\n", dlerror());
            daws->passthrough_enable = 1;
        }
        daws->passthrough_enable = 0;
    } else {
        printf("cant find decoder lib %s\n", dlerror());
        daws->passthrough_enable = 1;
    }

    if (daws->passthrough_enable == 0) {
        ret = daws->daws_effect_init(ext->rate, ext->channels, daws->output_channels);
        if (ret < 0) {
            SNDERR("daws effect init failed! passthrough enabled\n");
            daws->passthrough_enable = 1;
        }
    }
    if (daws->passthrough_enable == 1) {
        if (daws->fd != NULL) {
            dlclose(daws->fd);
            daws->fd = NULL;
        }
    }

    daws->pcm_in_buffer = (int16_t *)malloc(sizeof(int16_t) * MAX_DAWS_SUPPORTED_IN_CHANNELS * MAX_DAWS_SUPPORTED_SAMPLE_SIZE * 2);
    if (daws->pcm_in_buffer == NULL) {
        SNDERR("malloc failed for input buffer");
        return -ENOENT;
    }
    daws->pcm_out_buffer = (int16_t *)malloc(sizeof(int16_t) * MAX_DAWS_SUPPORTED_OUT_CHANNELS * MAX_DAWS_SUPPORTED_SAMPLE_SIZE);
    if (daws->pcm_out_buffer == NULL) {
        SNDERR("malloc failed for output buffer");
        return -ENOENT;
    }
    memset(daws->pcm_in_buffer, 0, sizeof(int16_t) * MAX_DAWS_SUPPORTED_IN_CHANNELS * MAX_DAWS_SUPPORTED_SAMPLE_SIZE);
    memset(daws->pcm_out_buffer, 0, sizeof(int16_t) * MAX_DAWS_SUPPORTED_OUT_CHANNELS * MAX_DAWS_SUPPORTED_SAMPLE_SIZE);
    ring_buffer_init(&(daws->in_ringbuffer), sizeof(int16_t) * MAX_DAWS_SUPPORTED_IN_CHANNELS * MAX_DAWS_SUPPORTED_SAMPLE_SIZE * 2);
    ring_buffer_init(&(daws->out_ringbuffer), sizeof(int16_t) * MAX_DAWS_SUPPORTED_OUT_CHANNELS * MAX_DAWS_SUPPORTED_SAMPLE_SIZE * 2);

    daws->bufread = 0;
    /*write 512 samples in case the first transfer samples < 512*/
    ring_buffer_write(&(daws->out_ringbuffer), daws->pcm_out_buffer, MAX_DAWS_SUPPORTED_SAMPLE_SIZE * daws->output_channels * 2, UNCOVER_WRITE);

#ifdef DUMP_ENABLE
    if (daws->dump_enable ) {
        if (daws->input_fn == NULL || daws->output_fn == NULL ) {
            SNDERR("Use proper Input-Output file name");
            return -EINVAL;
        }
        daws->input_fp = fopen(daws->input_fn, "wb");
        if (daws->input_fp == NULL) {
            SNDERR("File open failed for Input File");
            free(( char*)daws->input_fn);
            return -ENOENT;
        }
        daws->output_fp = fopen(daws->output_fn, "wb");
        if (daws->output_fp == NULL) {
            SNDERR("File open failed for output File");
            free(( char*)daws->output_fn);
            return -ENOENT;
        }
    }
#endif

    daws->init_enabled = 1;
    printf("plug input channels = %d, output channels = %d\n",ext->channels, daws->output_channels);
    printf("plugin init success!\n");
    return 0;
}

static int daws_plugin_close(snd_pcm_extplug_t *ext)
{
    daws_plug_info *daws = (daws_plug_info *)ext;

    if (daws->pcm_in_buffer != NULL ) {
        free(daws->pcm_in_buffer);
        daws->pcm_in_buffer = NULL;
    }
    if (daws->pcm_out_buffer != NULL ) {
        free(daws->pcm_out_buffer);
        daws->pcm_out_buffer = NULL;
    }

    if (daws->passthrough_enable == 0) {
        daws->daws_effect_close();
        if (daws->fd != NULL) {
            dlclose(daws->fd);
            daws->fd = NULL;
        }
    }

    if (daws->in_ringbuffer.start_addr != NULL) {
        ring_buffer_release(&(daws->in_ringbuffer));
    }
    if (daws->in_ringbuffer.start_addr != NULL) {
        ring_buffer_release(&(daws->out_ringbuffer));
    }

#ifdef DUMP_ENABLE
    if (daws->dump_enable) {
        if (   (daws->input_fn != NULL && daws->output_fn != NULL)) {
            printf("dump plug input data to : %s \n", daws->input_fn);
            printf("dump plug output data to : %s \n", daws->output_fn);
        }
        if (daws->input_fp != NULL) {
            fclose(daws->input_fp);
            daws->input_fp = NULL;
        }
        if (daws->output_fp != NULL) {
            fclose(daws->output_fp);
            daws->output_fp = NULL;
        }
    }
#endif
    daws->bufread = 0;
    daws->init_enabled = 0;
    printf("daws plugin closed\n");
    return 0;
}

static snd_pcm_sframes_t
daws_plugin_transfer(snd_pcm_extplug_t *ext,
                     const snd_pcm_channel_area_t *dst_areas,
                     snd_pcm_uframes_t dst_offset,
                     const snd_pcm_channel_area_t *src_areas,
                     snd_pcm_uframes_t src_offset,
                     snd_pcm_uframes_t size)
{
    daws_plug_info *daws = (daws_plug_info *)ext;

    int size_bytes =  snd_pcm_frames_to_bytes (ext->pcm, size);
    short *src = area_addr(src_areas, src_offset);
    short *dst = area_addr(dst_areas, dst_offset);

#ifdef DUMP_ENABLE
    if (daws->dump_enable) {
        fwrite(src, 1, size_bytes, daws->input_fp);
    }
#endif

    if (daws->passthrough_enable == 0) {
        /*convert to 16bit pcm buffer*/
        convert_to_s16(src_areas, src_offset, ext->channels, size, ext->format, daws->pcm_in_buffer);

        ring_buffer_write(&(daws->in_ringbuffer), daws->pcm_in_buffer, size * ext->channels * 2, UNCOVER_WRITE);
        daws->bufread += size;

        /* Load two frames of 256 bytes each totalling about 512 samples */
        while (daws->bufread >= MAX_DAWS_SUPPORTED_SAMPLE_SIZE) {

            /*read 16bit 512 samples*/
            ring_buffer_read(&(daws->in_ringbuffer), daws->pcm_in_buffer, MAX_DAWS_SUPPORTED_SAMPLE_SIZE * ext->channels * 2);

            /*daws process: pcm_in_buffer -> pcm_out_buffer, size = 512 */
            snd_pcm_uframes_t out_size;
            daws->daws_effect_process(daws->pcm_in_buffer, MAX_DAWS_SUPPORTED_SAMPLE_SIZE, daws->pcm_out_buffer, &out_size);

            ring_buffer_write(&(daws->out_ringbuffer), daws->pcm_out_buffer, MAX_DAWS_SUPPORTED_SAMPLE_SIZE * daws->output_channels * 2, UNCOVER_WRITE);

            daws->bufread -= MAX_DAWS_SUPPORTED_SAMPLE_SIZE;
        }

        ring_buffer_read(&(daws->out_ringbuffer), daws->pcm_out_buffer, size * daws->output_channels * 2);
        /*convert to 16bit dst areas*/
        convert_to_area(dst_areas, dst_offset, daws->output_channels, size, SND_PCM_FORMAT_S16, daws->pcm_out_buffer);

    } else {
        /*passthrough data from in to out*/
        convert_process(src_areas, src_offset,
                        dst_areas, dst_offset,
                        ext->channels,
                        ext->channels,
                        size, ext->format);
    }

#ifdef DUMP_ENABLE
    if (daws->dump_enable) {
        short *dst = area_addr(dst_areas, dst_offset);
        int  size_bytes = size * daws->output_channels * 2;
        fwrite(dst, 1, size_bytes , daws->output_fp);
    }
#endif

    return size;
}

static const snd_pcm_extplug_callback_t daws_plugin_callback = {
    .init = daws_plugin_init,
    .close = daws_plugin_close,
    .transfer = daws_plugin_transfer,
};

SND_PCM_PLUGIN_DEFINE_FUNC(daws)
{

    snd_config_iterator_t i, next;
    int err;
    daws_plug_info *daws_info;
    snd_config_t *slave = NULL;
    unsigned int out_channels = PLUG_OUTPUT_DEFAULT_CH;
    char *ifname = NULL, *ofname = NULL;
    int dump_enable = 0;

    static const unsigned int in_channels[MAX_DAWS_SUPPORTED_IN_CHANNELS] = {
        1
        , 2
    };
    static const unsigned int in_formats[MAX_DAWS_SUPPORTED_FORMATS] = {
        SND_PCM_FORMAT_S32
        , SND_PCM_FORMAT_S24
        , SND_PCM_FORMAT_S16
    };

    snd_config_for_each(i, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(i);
        const char *id;
        if (snd_config_get_id(n, &id) < 0)
            continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0)
            continue;
        if (strcmp(id, "slave") == 0) {
            slave = n;
            continue;
        }
        if (strcmp(id, "channels") == 0) {
            unsigned int val;
            err = snd_config_get_integer(n, &val);
            if (err < 0) {
                SNDERR("Invalid value for %s", id);
                return err;
            }
            if (val == 0 || val > 8) {
                SNDERR("out put channels[%d] is out of range", val);
            }
            out_channels = val;
            continue;
        }
        if (strcmp(id, "dump_enable") == 0) {
            int val;
            val = snd_config_get_bool(n);
            if (val < 0) {
                SNDERR("Invalid value for %s", id);
                return val;
            }
            dump_enable = val;
            continue;
        }
        if (strcmp(id, "dumpin_name") == 0) {
            err = snd_config_get_string(n, &ifname);
            if (err < 0) {
                SNDERR("Invalid value for %s", id);
                return err;
            }
            continue;
        }
        if (strcmp(id, "dumpout_name") == 0) {
            err = snd_config_get_string(n, &ofname);
            if (err < 0) {
                SNDERR("Invalid value for %s", id);
                return err;
            }
            continue;
        }
        SNDERR("Unknown field %s", id);
        return -EINVAL;
    }
    if (! slave) {
        SNDERR("No slave defined for daws plugin");
        return -EINVAL;
    }
    daws_info = calloc(1, sizeof(*daws_info));
    if (daws_info == NULL)
        return -ENOMEM;
    daws_info->ext.version = SND_PCM_EXTPLUG_VERSION;
    daws_info->ext.name = "Dolby Audio for Wireless Speakers Plugin";
    daws_info->ext.callback = &daws_plugin_callback;
    daws_info->ext.private_data = daws_info;

    /*create plugin, must get slave name*/
    err = snd_pcm_extplug_create(&daws_info->ext, name, root, slave, stream, mode);
    if (err < 0) {
        free(daws_info);
        return err;
    }

    /*set slave out put channels*/
    err = snd_pcm_extplug_set_slave_param_minmax (&daws_info->ext
            , SND_PCM_EXTPLUG_HW_CHANNELS
            , out_channels
            , out_channels
                                                 );
    if (err < 0) {
        free(daws_info);
        return err;
    }
    daws_info->output_channels = (unsigned short )out_channels;

    /*set in put channels : 1 or 2*/
    err = snd_pcm_extplug_set_param_list (&daws_info->ext
                                          , SND_PCM_EXTPLUG_HW_CHANNELS
                                          , MAX_DAWS_SUPPORTED_IN_CHANNELS
                                          , in_channels
                                         );
    if (err < 0) {
        free(daws_info);
        return err;
    }

    /*set in put format: 16/24/32bit */
    err = snd_pcm_extplug_set_param_list (&daws_info->ext
                                          , SND_PCM_EXTPLUG_HW_FORMAT
                                          , MAX_DAWS_SUPPORTED_FORMATS
                                          , in_formats
                                         );
    if (err < 0) {
        free(daws_info);
        return err;
    }

    /*set out put format: 16bit only*/
    err = snd_pcm_extplug_set_slave_param (&daws_info->ext
                                           , SND_PCM_EXTPLUG_HW_FORMAT
                                           , SND_PCM_FORMAT_S16
                                          );
    if (err < 0) {
        free(daws_info);
        return err;
    }

#ifdef DUMP_ENABLE
    daws_info->dump_enable = dump_enable;
    if (daws_info->dump_enable) {
        if (!ifname || strlen (ifname) == 0 ) {
            SNDERR("Input file name is not defined");
            return -EINVAL;
        }
        if (!ofname || strlen (ofname) == 0 ) {
            SNDERR("Output file name is not defined");
            return -EINVAL;
        }
        if (!strcmp(ifname, ofname) ) {
            SNDERR("Input and Output Dump File names are same");
            return -EINVAL;
        }
        daws_info->input_fn = strdup (ifname);
        daws_info->output_fn = strdup (ofname);
    }
    daws_info->input_fp = NULL;
    daws_info->output_fp = NULL;
#endif

    *pcmp = daws_info->ext.pcm;
    return 0;
}


SND_PCM_PLUGIN_SYMBOL(daws);
