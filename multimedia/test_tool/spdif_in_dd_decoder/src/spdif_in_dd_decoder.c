/*
 *  this tool can read from spdif (ac3 format), and decode the ac3 data, then playback to hw:0,2 interface.
 *  input:  48k 16bit 2ch (spdif, ac3 only)
 *  output: 48k 16bit 2ch pcm
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <dlfcn.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <amconfigutils.h>
#define PROPERTY_VALUE_MAX 127
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 500*1024
#define AUDIO_EXTRA_DATA_SIZE   (8192)
enum IEC61937_PC_Value {
    IEC61937_AC3                = 0x01,          ///< AC-3 data
    IEC61937_DTS1               = 0x0B,          ///< DTS type I   (512 samples)
    IEC61937_DTS2               = 0x0C,          ///< DTS type II  (1024 samples)
    IEC61937_DTS3               = 0x0D,          ///< DTS type III (2048 samples)
    IEC61937_DTSHD              = 0x11,          ///< DTS HD data
    IEC61937_EAC3               = 0x15,          ///< E-AC-3 data
    IEC61937_TRUEHD             = 0x16,          ///< TrueHD data
    IEC61937_PAUSE              = 0x03,          ///< Pause
};
typedef struct _audio_info {
    int bitrate;
    int samplerate;
    int channels;
    int file_profile;
    int decoded_nb_frames;
    int dropped_nb_frames;
    int error_nb_frames;
} AudioInfo;

/* audio decoder operation*/
typedef struct audio_decoder_operations audio_decoder_operations_t;
struct audio_decoder_operations {
    const char * name;
    int nAudioDecoderType;
    int nInBufSize;
    int nOutBufSize;
    int (*init)(audio_decoder_operations_t *);
    int (*decode)(audio_decoder_operations_t *, char *outbuf, int *outlen, char *inbuf, int inlen);
    int (*release)(audio_decoder_operations_t *);
    int (*getinfo)(audio_decoder_operations_t *, AudioInfo *pAudioInfo);
    void * priv_data;//point to audec
    void * priv_dec_data;//decoder private data
    void *pdecoder; // decoder instance
    int channels;
    unsigned long pts;
    int samplerate;
    int bps;
    int extradata_size;      ///< extra data size
    char extradata[AUDIO_EXTRA_DATA_SIZE];
    int NchOriginal;
    int nInAssocBufSize;//associate data size
    int lfepresent;
};

#define SPDIF_RATE     48000
#define SPDIF_CHANNEL  2
#define SPDIF_FORMATE  SND_PCM_FORMAT_S16_LE
#define PCM_RATE       48000
#define PCM_CHANNEL    2
#define PCM_FORMATE    SND_PCM_FORMAT_S16_LE

#define SPDIF_FRAME    6144
#define READSIZE       6144/4
#define READ_DD_SIZE   1792

/* global data */
static snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
static snd_pcm_t *handle_write;
static snd_pcm_t *handle_read;
static u_char *audiobuf_in = NULL;
static char pcm_buf_out[AVCODEC_MAX_AUDIO_FRAME_SIZE] = {0};//max frame size out buf
static char pcm_buf_mute[SPDIF_FRAME * 4 * 4] = {0};

static unsigned period_time = 0;
static unsigned buffer_time = 0;
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t buffer_frames = 0;
static int start_delay = 1;
static int stop_delay = 0;

static void thread_start(void *arg){
    writei_func(handle_write, pcm_buf_out, SPDIF_FRAME * 2);
}
 int amsysfs_set_sysfs_int(const char *path, int val)
{
    int fd;
    int bytes;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        printf("unable to open file %s", path);
            return -1;
    }
}
 static void dump_pcm_bin(char *path, char *buf, int size)
 {
     FILE *fp = fopen(path, "ab+");
     if (fp != NULL) {
         fwrite(buf, 1, size, fp);
         fclose(fp);
     }
 }

static int set_params_alsa(snd_pcm_t *handle, int rate, int channels)
{
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t bufsize;
    snd_pcm_uframes_t period_size;
    int err;
    size_t n;
    snd_pcm_uframes_t start_threshold, stop_threshold;
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) {
        printf("Broken configuration for this PCM: no configurations available");
        return err;
    }
    err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        printf("Access type not available");
        return err;
    }
    err = snd_pcm_hw_params_set_format(handle,hwparams,SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        printf("Sample format non available");
        return err;
    }
    err = snd_pcm_hw_params_set_channels(handle, hwparams, channels);
    if (err < 0) {
        printf("Channels count non available");
        return err;
    }
    err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rate, 0);
    assert(err >= 0);
    err = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0);
    assert(err >= 0);
    if (buffer_time > 500000)
    buffer_time = 500000;
    if (buffer_time > 0)
    period_time = buffer_time / 4;
    else
    period_frames = buffer_frames / 4;
    if (period_time > 0)
    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, 0);
    else
    err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &period_frames, 0);
    assert(err >= 0);
    if (buffer_time > 0) {
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, 0);
    } else {
    err = snd_pcm_hw_params_set_buffer_size_near(handle, hwparams, &buffer_frames);
    }
    assert(err >= 0);
    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) {
        printf("Unable to install hw hwparams:");
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    if (err < 0) {
        printf("Unable to get buffersize \n");
        return err;
    }
    snd_pcm_hw_params_get_period_size(hwparams, &period_size, 0);
    if (period_size == bufsize)
        printf("Can't use period equal to buffer size (%lu == %lu)\n", period_size, bufsize);
    else
        printf("period_size = %d\n",period_size);
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        printf("??Unable to get sw-parameters\n");
        return err;
    }
    n = period_size;
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, n);
    n = bufsize;
    if (start_delay <= 0) {
      start_threshold = n + (double) rate * start_delay / 1000000;
    } else
    start_threshold = (double) rate * start_delay / 1000000;
    if (start_threshold < 1)
        start_threshold = 1;
    if (start_threshold > n)
        start_threshold = n;
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_threshold);
    assert(err >= 0);
    stop_threshold = bufsize + (double) rate * stop_delay / 1000000;
    err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, stop_threshold);
    assert(err >= 0);
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        printf("Unable to get sw-parameters\n");
        return err;
    }
    return 0;
}

int alsa_init(void)
{
    char sound_card_dev[10] = {0};
    int sound_card_id = 0;
    int sound_dev_id = 0;
    int err;
    if (amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_raw", 0 ) < 0 )
        return -1;
    if (amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec", 0 )< 0 )
        return -1;
/*open spdif */
    sound_card_id = 0;
    sound_dev_id = 4;
    sprintf(sound_card_dev, "hw:%d,%d", sound_card_id, sound_dev_id);
    err = snd_pcm_open(&handle_read, sound_card_dev, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        printf("audio open read error: %s\n", snd_strerror(err));
        return -1;
    }
    start_delay = 1;
    err = set_params_alsa(handle_read, SPDIF_RATE, SPDIF_CHANNEL);
    if (err < 0) {
        printf("set_spdif_alsa err\n");
        return -1;
    }
/*open tdm*/
    sound_card_id =0;
    sound_dev_id = 2;
    sprintf(sound_card_dev, "hw:%d,%d", sound_card_id, sound_dev_id);
    err = snd_pcm_open(&handle_write, sound_card_dev, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if (err < 0) {
        printf("audio open write error: %s\n", snd_strerror(err));
        return -1;
    }
    start_delay = 0;
    printf("defaut alsa out rate :%d channel num:%d !!!\n",PCM_RATE,PCM_CHANNEL);
    set_params_alsa(handle_write, PCM_RATE, PCM_CHANNEL);
    return 0;
}
static size_t pcm_write(snd_pcm_t *handle_write, u_char * data, size_t count){
    int err = 0;
    while (count > 0) {
        err = writei_func(handle_write, data, count);

        if (err == -EINTR) {
            err = 0;
        }
        if (err == -ESTRPIPE) {
            while ((err = snd_pcm_resume(handle_write)) == -EAGAIN) {
                usleep(1000);
            }
        }

        if (err < 0) {
            printf("xun in\n");
            if ((err = snd_pcm_prepare(handle_write)) < 0) {
                //result = 0;
                //goto  done;
                break;
            }
        }

        if (err > 0) {
            //result += r;
            count -= err;
            data += err * 4;
        }
    }

}

int find_audio_lib(audio_decoder_operations_t *adec_ops,char *name)
{
    void *fd = NULL;
    int err;
    //fd = dlopen("/usr/lib/libdcv.so", RTLD_LAZY);
    fd = dlopen(name, RTLD_LAZY);
    if (fd != NULL) {
        printf("dlopen_success!\n");
        adec_ops->init    = dlsym(fd, "audio_dec_init");
        adec_ops->decode  = dlsym(fd, "audio_dec_decode");
        adec_ops->release = dlsym(fd, "audio_dec_release");
        adec_ops->getinfo = dlsym(fd, "audio_dec_getinfo");
        printf("lib_init_success!\n");
    } else {
        printf("cant find decoder lib %s\n", dlerror());
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int err,ret;
    int SyncFlag =0;
    int PthFlag = 0;
    short read_dd_size = READ_DD_SIZE; /*read buffer*/
    short pc;
    short framesize;
    int decode_dd_size;
    int audio_type = -1;
    char *outbuf = pcm_buf_out;
    int outlen;
    int pthread_id=1;
    unsigned char value[PROPERTY_VALUE_MAX];
    unsigned char ptr_head[33] = { 0 };
    unsigned char name[100] = { 0 };
    audio_decoder_operations_t *adec_ops;
    AudioInfo  g_AudioInfo = {0};
    memset(&g_AudioInfo, 0, sizeof(AudioInfo));
    audiobuf_in = (u_char *)malloc(SPDIF_FRAME * 4);
    adec_ops = malloc(sizeof(audio_decoder_operations_t));
    writei_func = snd_pcm_writei;
    readi_func = snd_pcm_readi;
    if (alsa_init() < 0 )
        return -1;
    //if (find_audio_lib(adec_ops,name) < 0 )
      //  return -1;
    //adec_ops->init(adec_ops);
    printf("init success!\n");
    readi_func(handle_read, &ptr_head[0], 8);
    while (1) {
        /*find 61937 header*/
        while (!SyncFlag) {
            int i;
            for (i = 0; i <= 22; i++) {
                if ((ptr_head[i] == 0x72 && ptr_head[i + 1] == 0xf8) && ptr_head[i + 2] == 0x1f && ptr_head[i + 3] == 0x4e) {
                     printf(" find 61937 header, i =%d\n",i);
                     pc = *((short *)(ptr_head + i + 4));
                     printf("pc:%0x \n",pc);
                     if (pc == IEC61937_AC3) {
                         sprintf(name, "%s","/usr/lib/libdcv.so");
                         read_dd_size = SPDIF_FRAME;
                         printf("detected DD IEC data !\n");
                         ret = property_get("media.multich.support.info", value, NULL);
                         if (ret > 0) {
                            if (strcasecmp(value,"hdmi6") == 0) {
                                printf("reconfig alsa out ,rate:%d channel num:%d",PCM_RATE,3 * PCM_CHANNEL);
                                err =  set_params_alsa(handle_write, PCM_RATE, 3 * PCM_CHANNEL);
                                if (err < 0) {
                                    printf("set_spdif_alsa err\n");
                                    return -1;
                                 }
                            }
                         }
                     } else if (pc == IEC61937_DTS1 || pc == IEC61937_DTS2 || pc == IEC61937_DTS3){
                          sprintf(name, "%s","/usr/lib/libdtscore.so");
                          read_dd_size = 512 * 4;
                          printf("detected DTS IEC data !\n");
                          //ret = property_get("media.multich.support.info", value, NULL);
                           if (property_get("media.dts.multichpcm", value, NULL) > 0 && !strcmp(value, "1")) {
                               printf("reconfig alsa out ,rate:%d channel num:%d",PCM_RATE,3 * PCM_CHANNEL);
                               err = set_params_alsa(handle_write, PCM_RATE, 3 * PCM_CHANNEL);
                               if (err < 0) {
                                    printf("set_spdif_alsa err\n");
                                    return -1;
                                }
                           }
                     }
                     if (audio_type == -1 || audio_type != pc) {
                        if (find_audio_lib(adec_ops,name) < 0 )
                          return -1;
                        adec_ops->init(adec_ops);
                     }
                     audio_type = pc;
                     if (i%4 != 0) {
                         printf("err: read data with every frames, the header address can not be read, please open the tool again\n");
                         return 0;
                     }
                     if (ptr_head[i] == 0x72 && ptr_head[i + 1] == 0xf8)
                         framesize = (ptr_head[i + 7] << 8 |  ptr_head[i + 6]) >> 3;
                     else
                         framesize = (ptr_head[i + 6] << 8 |  ptr_head[i + 7]) >> 3;
                     printf("framesize:%d read_dd_size:%d \n", framesize,read_dd_size);
                     memcpy((unsigned char*) audiobuf_in , &ptr_head[i], 32 - i);
                     readi_func(handle_read, (unsigned char*) audiobuf_in + 32 - i, SPDIF_FRAME -( 32 - i)/4);
                     //pthread_create(&pthread_id,NULL,thread_start, NULL);
                     SyncFlag = 1;
                     PthFlag = 1;
                     break;
                 }
            }
            if (SyncFlag != 1) {
                ptr_head[0] = ptr_head[28];
                ptr_head[1] = ptr_head[29];
                ptr_head[2] = ptr_head[30];
                ptr_head[3] = ptr_head[31];
                readi_func(handle_read,&ptr_head[4], 7);
                writei_func(handle_write, &ptr_head[4], 7);
            }
        }

        /*read -> decoder -> write*/
        while (1) {
            readi_func(handle_read, (unsigned char*) audiobuf_in , read_dd_size/4);
            if ((audiobuf_in[8] ==0x0b && audiobuf_in[9] ==0x77) ||
               (audiobuf_in[8] ==0x77 && audiobuf_in[9] ==0x0b) ||
                ((audiobuf_in[8] ==0x7f && audiobuf_in[9] ==0xfe && audiobuf_in[10] ==0x80 && audiobuf_in[11] ==0x01)||
                 (audiobuf_in[8] ==0xfe && audiobuf_in[9] ==0x7f && audiobuf_in[10] ==0x01 && audiobuf_in[11] ==0x80))
                ) {
                    decode_dd_size = adec_ops->decode(adec_ops, outbuf, &outlen, (char *)(audiobuf_in + 8), framesize);
                    //printf("decode_dd_size:%d outlen:%d\n",decode_dd_size,outlen);

                    if (outlen > AVCODEC_MAX_AUDIO_FRAME_SIZE)
                        printf("fatal error,out buffer overwriten,out len %d,actual %d", outlen, AVCODEC_MAX_AUDIO_FRAME_SIZE);
                    if (outlen > 0) {
                        //to get frame samplerate and channelnum
                        adec_ops->getinfo(adec_ops, &g_AudioInfo);
                        //printf("src channel:%d sink channels:%d sink samplerate:%d \n",adec_ops->NchOriginal,g_AudioInfo.channels,g_AudioInfo.samplerate);
                        dump_pcm_bin("/data/out_pcm.raw", outbuf, outlen);
                        //writei_func(handle_write, (char *)outbuf, outlen /4);
                    }
                    int count = outlen /4;
                    unsigned char *data = outbuf;
                    pcm_write(handle_write, outbuf, count);
                } else {
                SyncFlag = 0;
                writei_func(handle_write, pcm_buf_mute, READSIZE);
                readi_func(handle_read, &ptr_head[0], 8);
                writei_func(handle_write, pcm_buf_mute, 8);
                break;
            }
        }
    }
    return 0;
}
