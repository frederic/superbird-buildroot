#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include "atomic.h"
#include "log.h"
#include "properties.h"
//#include <cutils/str_parms.h>
#include <errno.h>
#include <fcntl.h>
#include "hardware.h"
#include <inttypes.h>
#include <linux/ioctl.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "audio_core.h"
#include <time.h>
//#include <utils/Timers.h>

#if ANDROID_PLATFORM_SDK_VERSION >= 25  // 8.0
#include <system/audio-base.h>
#endif

#include "audio_hal.h"

#include <aml_android_utils.h>
#include <aml_data_utils.h>

#include "aml_audio_stream.h"
#include "aml_data_utils.h"
#include "aml_dump_debug.h"
#include "aml_hw_profile.h"
#include "audio_hw.h"
#include "audio_hw_dtv.h"
#include "audio_hw_profile.h"
#include "audio_hw_utils.h"
//#include "dtv_patch_out.h"
#include "aml_audio_resampler.h"
#include "aml_audio_parser.h"

#include "audio_hw_ms12.h"
#include "dolby_lib_api.h"

#define TSYNC_PCRSCR "/sys/class/tsync/pts_pcrscr"
#define TSYNC_EVENT "/sys/class/tsync/event"
#define TSYNC_APTS "/sys/class/tsync/pts_audio"
#define TSYNC_VPTS "/sys/class/tsync/pts_video"

#define PATCH_PERIOD_COUNT 4
//#define SYSTIME_CORRECTION_THRESHOLD (90000 * 10 / 100)
#define AUDIO_PTS_DISCONTINUE_THRESHOLD (90000 * 5)

//{reference from " /amcodec/include/amports/aformat.h"
#define ACODEC_FMT_NULL -1
#define ACODEC_FMT_MPEG 0
#define ACODEC_FMT_PCM_S16LE 1
#define ACODEC_FMT_AAC 2
#define ACODEC_FMT_AC3 3
#define ACODEC_FMT_ALAW 4
#define ACODEC_FMT_MULAW 5
#define ACODEC_FMT_DTS 6
#define ACODEC_FMT_PCM_S16BE 7
#define ACODEC_FMT_FLAC 8
#define ACODEC_FMT_COOK 9
#define ACODEC_FMT_PCM_U8 10
#define ACODEC_FMT_ADPCM 11
#define ACODEC_FMT_AMR 12
#define ACODEC_FMT_RAAC 13
#define ACODEC_FMT_WMA 14
#define ACODEC_FMT_WMAPRO 15
#define ACODEC_FMT_PCM_BLURAY 16
#define ACODEC_FMT_ALAC 17
#define ACODEC_FMT_VORBIS 18
#define ACODEC_FMT_AAC_LATM 19
#define ACODEC_FMT_APE 20
#define ACODEC_FMT_EAC3 21
#define ACODEC_FMT_WIFIDISPLAY 22
#define ACODEC_FMT_DRA 23
#define ACODEC_FMT_TRUEHD 25
#define ACODEC_FMT_MPEG1                                                       \
  26 // AFORMAT_MPEG-->mp3,AFORMAT_MPEG1-->mp1,AFROMAT_MPEG2-->mp2
#define ACODEC_FMT_MPEG2 27
#define ACODEC_FMT_WMAVOI 28

//}



//{ RAW_OUTPUT_FORMAT
#define PCM 0  /*AUDIO_FORMAT_PCM_16_BIT*/
#define DD 4   /*AUDIO_FORMAT_AC3*/
#define AUTO 5 /*choose by sink capability/source format/Digital format*/

//}


#define DTV_DECODER_PTS_LOOKUP_PATH "/sys/class/tsync/apts_lookup"

pthread_mutex_t dtv_patch_mutex = PTHREAD_MUTEX_INITIALIZER;
struct cmd_list {
    struct cmd_list *next;
    int cmd;
    int cmd_num;
    int used;
};

struct cmd_list dtv_cmd_list = {
    .next = NULL,
    .cmd = -1,
    .cmd_num = 0,
    .used = 1,
};

static int create_dtv_output_stream_thread(struct aml_audio_patch *patch);
static int release_dtv_output_stream_thread(struct aml_audio_patch *patch);

struct cmd_list cmd_array[16];  // max cache 16 cmd;

static unsigned long decoder_apts_lookup(unsigned int offset)
{
    unsigned int pts = 0;
    int ret;
    char buff[32];
    memset(buff, 0, 32);
    snprintf(buff, 32, "%d", offset);

    aml_sysfs_set_str(DTV_DECODER_PTS_LOOKUP_PATH, buff);
    ret = aml_sysfs_get_str(DTV_DECODER_PTS_LOOKUP_PATH, buff, sizeof(buff));

    if (ret > 0) {
        ret = sscanf(buff, "0x%x\n", &pts);
    }
    // ALOGI("decoder_apts_lookup get the pts is %x\n", pts);
    if (pts == (unsigned int) - 1) {
        pts = 0;
    }

    // adec_print("adec_apts_lookup get the pts is %lx\n", pts);

    return (unsigned long)pts;
}

static void init_cmd_list(void)
{
    pthread_mutex_lock(&dtv_patch_mutex);
    dtv_cmd_list.next = NULL;
    dtv_cmd_list.cmd = -1;
    dtv_cmd_list.cmd_num = 0;
    dtv_cmd_list.used = 0;
    memset(cmd_array, 0, sizeof(cmd_array));
    pthread_mutex_unlock(&dtv_patch_mutex);
}

static struct cmd_list *cmd_array_get(void)
{
    int index = 0;
    pthread_mutex_lock(&dtv_patch_mutex);
    for (index = 0; index < 16; index++) {
        if (cmd_array[index].used == 0) {
            break;
        }
    }

    if (index == 16) {
        pthread_mutex_unlock(&dtv_patch_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&dtv_patch_mutex);
    return &cmd_array[index];
}
static void cmd_array_put(struct cmd_list *list)
{
    pthread_mutex_lock(&dtv_patch_mutex);
    list->used = 0;
    pthread_mutex_unlock(&dtv_patch_mutex);
}

static void _add_cmd_to_tail(struct cmd_list *node)
{
    struct cmd_list *list = &dtv_cmd_list;
    pthread_mutex_lock(&dtv_patch_mutex);
    while (list->next != NULL) {
        list = list->next;
    }
    list->next = node;
    dtv_cmd_list.cmd_num++;
    pthread_mutex_unlock(&dtv_patch_mutex);
}

int dtv_patch_add_cmd(int cmd)
{
    struct cmd_list *list = cmd_array_get();
    if (list == NULL) {
        ALOGI("can't get cmd list, add by live \n");
        return -1;
    }
    ALOGI("add by live dtv_patch_add_cmd the cmd is %d \n", cmd);
    list->cmd = cmd;
    list->next = NULL;
    list->used = 1;

    _add_cmd_to_tail(list);
    return 0;
}

int dtv_patch_get_cmd(void)
{
    int cmd = AUDIO_DTV_PATCH_CMD_NUM;
    struct cmd_list *list = NULL;
    ALOGI("enter dtv_patch_get_cmd funciton now\n");
    pthread_mutex_lock(&dtv_patch_mutex);
    list = dtv_cmd_list.next;
    dtv_cmd_list.next = list->next;
    cmd = list->cmd;
    pthread_mutex_unlock(&dtv_patch_mutex);
    cmd_array_put(list);
    ALOGI("leave dtv_patch_get_cmd the cmd is %d \n", cmd);
    return cmd;
}
int dtv_patch_cmd_is_empty(void)
{
    pthread_mutex_lock(&dtv_patch_mutex);
    if (dtv_cmd_list.next == NULL) {
        pthread_mutex_unlock(&dtv_patch_mutex);
        return 1;
    }
    pthread_mutex_unlock(&dtv_patch_mutex);
    return 0;
}
static int dtv_patch_buffer_space(void *args)
{
    int left = 0;
    struct aml_audio_patch *patch = (struct aml_audio_patch *)args;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    left = get_buffer_write_space(ringbuffer);
    return left;
}

unsigned long dtv_hal_get_pts(struct aml_audio_patch *patch)
{
    unsigned long val, offset;
    unsigned long pts;
    int data_width, channels, samplerate;
    unsigned long long frame_nums;
    unsigned long delay_pts;
    char value[PROPERTY_VALUE_MAX];

    channels = 2;
    samplerate = 48;
    data_width = 2;

    offset = patch->decoder_offset;

    // when first  look up apts,set offset 0
    if (!patch->first_apts_lookup_over) {
        offset = 0;
    }
    offset = decoder_apts_lookup(offset);

    pts = offset;
    if (!patch->first_apts_lookup_over) {
        patch->last_valid_pts = pts;
        patch->first_apts_lookup_over = 1;
        return pts;
    }

    if (pts == 0) {
        if (patch->last_valid_pts) {
            pts = patch->last_valid_pts;
        }
        frame_nums =
            (patch->outlen_after_last_validpts / (data_width * channels));
        pts += (frame_nums * 90 / samplerate);

        // ALOGI("decode_offset:%d out_pcm:%d   pts:%lx,audec->last_valid_pts %lx\n",
        //       patch->decoder_offset, patch->outlen_after_last_validpts, pts,
        //     patch->last_valid_pts);
        return pts;
    }

    val = pts;
    if (pts < patch->last_valid_pts)
        ALOGI("==== error the pts is %lx the last checkout pts is %lx\n", pts,
              patch->last_valid_pts);
    patch->last_valid_pts = pts;
    patch->outlen_after_last_validpts = 0;
    //ALOGI("====get pts:%lx offset:%d\n", val, patch->decoder_offset);
    return val;
}

void process_ac3_sync(struct aml_audio_patch *patch, unsigned long pts,
                      unsigned int lantcy)
{

    int channel_count = 2;
    int bytewidth = 2;
    int symbol = 48;
    char tempbuf[128];
    unsigned int pcrpts;

    unsigned long cur_out_pts;

    if (patch->dtv_first_apts_flag == 0) {
        sprintf(tempbuf, "AUDIO_START:0x%x", (unsigned int)pts);
        ALOGI("dtv set tsync -> %s", tempbuf);
        if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
            ALOGE("set AUDIO_START failed \n");
        }
        patch->dtv_first_apts_flag = 1;
    } else {
        unsigned int pts_diff;
        cur_out_pts = pts - lantcy * 90;
        get_sysfs_uint(TSYNC_PCRSCR, &pcrpts);
        //  ALOGI("The iniput pts is %lx pcrpts %x  "
        //      "cur_outpts %lx\n",
        //    pts,  pcrpts, cur_out_pts);

        if (pcrpts > cur_out_pts) {
            pts_diff = pcrpts - cur_out_pts;
            // ALOGI(" the pts %lx  diff is %d pcm_lantcy is %d\n", pts, pts_diff,
            //      lantcy);
        } else {
            pts_diff = pcrpts - cur_out_pts;
            //   ALOGI(" the pts is %lx the pcrpts is %x pcm_lantcy is %d\n",
            //   pts, pcrpts, lantcy);
            //    ALOGI("the pts diff is %d\n", (int)pts_diff);
        }

        if (pts_diff < SYSTIME_CORRECTION_THRESHOLD) {
            // now the av is syncd ,so do nothing;
        } else if (pts_diff > SYSTIME_CORRECTION_THRESHOLD &&
                   pts_diff < AUDIO_PTS_DISCONTINUE_THRESHOLD) {
            sprintf(tempbuf, "0x%lx", (unsigned long)cur_out_pts);
            //ALOGI("reset the apts to %lx \n", cur_out_pts);

            sysfs_set_sysfs_str(TSYNC_APTS, tempbuf);
        } else {
            sprintf(tempbuf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx", (unsigned long)pts);

            if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
                ALOGI("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
            }
        }
    }
}

void process_pts_sync(unsigned int pcm_lancty, struct aml_audio_patch *patch,
                      unsigned int rbuf_level)
{
    int channel_count = popcount(patch->chanmask);
    int bytewidth = 2;
    int sysmbol = 48;
    char tempbuf[128];
    unsigned int pcrpts;
    unsigned int calc_len = 0;
    unsigned long pts = 0;
    unsigned long cache_pts = 0;
    unsigned long cur_out_pts = 0;

    pts = dtv_patch_get_pts();

    if (pts == (unsigned long) - 1) {
        return ;
    }

    if (patch->dtv_first_apts_flag == 0) {
        sprintf(tempbuf, "AUDIO_START:0x%x", (unsigned int)pts);
        ALOGI("dtv set tsync -> %s", tempbuf);
        if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
            ALOGE("set AUDIO_START failed \n");
        }
        patch->dtv_first_apts_flag = 1;
    } else {
#if 1
        unsigned int pts_diff;
        calc_len = (unsigned int)rbuf_level;
        cache_pts = (calc_len * 90) / (sysmbol * channel_count * bytewidth);
        cur_out_pts = pts - cache_pts;
        cur_out_pts = cur_out_pts - pcm_lancty * 90;

        get_sysfs_uint(TSYNC_PCRSCR, &pcrpts);
        // ALOGI("The iniput pts is %lx and the cache pts is %lx  abuf_level "
        //       "%d pcrpts %x  "
        //       "cur_outpts %lx\n",
        //       pts, cache_pts, rbuf_level, pcrpts, cur_out_pts);

        if (pcrpts > cur_out_pts) {
            pts_diff = pcrpts - cur_out_pts;
            // ALOGI(" the pts %lx  diff is %d pcm_lantcy is %d\n", pts, pts_diff,
            //       pcm_lancty);
        } else {
            pts_diff = pcrpts - cur_out_pts;
            // ALOGI(" the pts is %lx the pcrpts is %x pcm_lantcy is %d the "
            //       "cache_pts is %lx ",
            //       pts, pcrpts, pcm_lancty, cache_pts);
            // ALOGI("the pts diff is %d\n", (int)pts_diff);
        }

        if (pts_diff < SYSTIME_CORRECTION_THRESHOLD) {
            // now the av is syncd ,so do nothing;
        } else if (pts_diff > SYSTIME_CORRECTION_THRESHOLD &&
                   pts_diff < AUDIO_PTS_DISCONTINUE_THRESHOLD) {
            sprintf(tempbuf, "0x%lx", (unsigned long)cur_out_pts);
            // ALOGI("reset the apts to %lx \n", cur_out_pts);

            sysfs_set_sysfs_str(TSYNC_APTS, tempbuf);
        } else {
            sprintf(tempbuf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx", (unsigned long)pts);

            if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
                ALOGI("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
            }
        }
#endif
    }
}

extern uint32_t out_get_latency(const struct audio_stream_out *stream);

static int dtv_patch_pcm_wirte(unsigned char *pcm_data, int size,
                               int symbolrate, int channel, void *args)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)args;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    int left,need_resample;
    int write_size,return_size;
    unsigned char *write_buf;
    int16_t tmpbuf[OUTPUT_BUFFER_SIZE];
    write_buf = pcm_data;
    if (pcm_data == NULL || size == 0) {
        return 0;
    }

    patch->sample_rate = symbolrate;
    patch->chanmask  = channel;
    if (patch->sample_rate != 48000)
        need_resample = 1;
    else
        need_resample = 0;
    left = get_buffer_write_space(ringbuffer);

    if (left <= 0) {
        return 0;
    }
   if (need_resample == 0 &&  patch->chanmask == 1) {
         if (left >= 2 * size) {
             write_size = size;
          } else {
             write_size = left / 2;
          }
    } else if (need_resample == 1 && patch->chanmask == 2) {
          if ((unsigned int)left >=  size * 48000 / patch->sample_rate) {
              write_size = size;
          } else {
               return 0;
          }

    } else if (need_resample == 1 && patch->chanmask == 1) {
          if ((unsigned int)left >=  2 * size * 48000 /patch->sample_rate) {
              write_size = size;
          } else {
              return 0;
          }
    } else {
        if (left >= size) {
            write_size = size;
        } else {
            write_size = left;
        }
    }

    return_size = write_size;

    if ((patch->aformat != AUDIO_FORMAT_E_AC3 &&
          patch->aformat != AUDIO_FORMAT_AC3 &&
          patch->aformat != AUDIO_FORMAT_DTS) ) {
          if (patch->chanmask == 1) {
             int16_t *buf = (int16_t *)write_buf;
             int i = 0 ,samples_num;
             samples_num = write_size /(patch->chanmask * sizeof(int16_t));
             for (;i < samples_num; i++) {
                tmpbuf[2 * (samples_num - i) - 1] = buf[samples_num - i - 1];
                tmpbuf[2 * (samples_num - i) - 2] = buf[samples_num - i - 1];
             }
             write_size = write_size * 2;
             write_buf = (unsigned char *)tmpbuf;
          }
          if (need_resample == 1) {
              if (patch->dtv_resample.input_sr != patch->sample_rate) {
                           patch->dtv_resample.input_sr = patch->sample_rate;
                           patch->dtv_resample.output_sr = 48000;
                           patch->dtv_resample.channels = 2;
                           resampler_init (&patch->dtv_resample);
               }
               if (!patch->resample_outbuf) {
                  patch->resample_outbuf = (unsigned char*) malloc (OUTPUT_BUFFER_SIZE * 3);
                  if (!patch->resample_outbuf) {
                      ALOGE ("malloc buffer failed\n");
                      return -1;
                  }
                  memset(patch->resample_outbuf, 0, OUTPUT_BUFFER_SIZE * 3);
               }
               int out_frame = write_size >> 2;
               out_frame = resample_process (&patch->dtv_resample, out_frame,
                     (int16_t *) write_buf, (int16_t *) patch->resample_outbuf);
               write_size = out_frame << 2;
               write_buf = patch->resample_outbuf;
          }
    }
    ring_buffer_write(ringbuffer, (unsigned char *)write_buf, write_size,
                      UNCOVER_WRITE);
    if (aml_getprop_bool("media.audiohal.outdump")) {
            FILE *fp1 = fopen("/data/audio_dtv.pcm", "a+");
            if (fp1) {
                int flen = fwrite((char *)write_buf, 1, write_size, fp1);
                ALOGI("%s buffer %p size %zu\n", __FUNCTION__, write_buf, write_size);
                fclose(fp1);
            }
    }
    // ALOGI("[%s]ring_buffer_write now wirte %d to ringbuffer\
    //  now\n",
    //       __FUNCTION__, write_size);
    patch->dtv_pcm_writed += return_size;

    pthread_cond_signal(&patch->cond);
    return return_size;
}

static int dtv_patch_raw_wirte(unsigned char *raw_data, int size, void *args)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)args;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    int left;
    int write_size;
    if (raw_data == NULL) {
        return 0;
    }

    if (size == 0) {
        return 0;
    }

    left = get_buffer_write_space(ringbuffer);
    if (left > size) {
        write_size = size;
    } else {
        write_size = left;
    }

    ring_buffer_write(ringbuffer, (unsigned char *)raw_data, write_size,
                      UNCOVER_WRITE);
    return write_size;
}
static int raw_dump_fd = -1;
void dump_raw_buffer(const void *data_buf, int size)
{
    ALOGI("enter the dump_raw_buffer save %d len data now\n", size);
    if (raw_dump_fd < 0) {
        if (access("/data/raw.es", 0) == 0) {
            raw_dump_fd = open("/data/raw.es", O_RDWR);
            if (raw_dump_fd < 0) {
                ALOGE("%s, Open device file \"%s\" error: %s.\n", __FUNCTION__,
                      "/data/raw.es", strerror(errno));
            }
        } else {
            raw_dump_fd = open("/data/raw.es", O_RDWR);
            if (raw_dump_fd < 0) {
                ALOGE("%s, Create device file \"%s\" error: %s.\n", __FUNCTION__,
                      "/data/raw.es", strerror(errno));
            }
        }
    }

    if (raw_dump_fd >= 0) {
        write(raw_dump_fd, data_buf, size);
    }
    return;
}

extern int do_output_standby_l(struct audio_stream *stream);
extern void adev_close_output_stream_new(struct audio_hw_device *dev,
        struct audio_stream_out *stream);
extern int adev_open_output_stream_new(struct audio_hw_device *dev,
                                       audio_io_handle_t handle __unused,
                                       audio_devices_t devices,
                                       audio_output_flags_t flags,
                                       struct audio_config *config,
                                       struct audio_stream_out **stream_out,
                                       const char *address __unused);
ssize_t out_write_new(struct audio_stream_out *stream, const void *buffer,
                      size_t bytes);

void *audio_dtv_patch_output_threadloop(void *data)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)data;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    struct audio_stream_out *stream_out = NULL;
    struct aml_stream_out *aml_out = NULL;
    struct audio_config stream_config;
    struct timespec ts;
    int write_bytes = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    int ret;

    ALOGI("++%s live ", __FUNCTION__);
    // FIXME: get actual configs
    stream_config.sample_rate = 48000;
    stream_config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    stream_config.format = AUDIO_FORMAT_PCM_16_BIT;
    /*
    may we just exit from a direct active stream playback
    still here.we need remove to standby to new playback
    */
    pthread_mutex_lock(&aml_dev->lock);
    aml_out = direct_active(aml_dev);
    if (aml_out) {
        ALOGI("%s live stream %p active,need standby aml_out->usecase:%d ",
              __func__, aml_out, aml_out->usecase);
        pthread_mutex_lock(&aml_out->lock);
        do_output_standby_l((struct audio_stream *)aml_out);
        pthread_mutex_unlock(&aml_out->lock);
        if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
            get_dolby_ms12_cleanup(&aml_dev->ms12);
        }
        if (aml_dev->need_remove_conti_mode == true) {
            ALOGI("%s,conntinous mode still there,release ms12 here", __func__);
            aml_dev->need_remove_conti_mode = false;
            aml_dev->continuous_audio_mode = 0;
        }
    } else {
        ALOGI("++%s live cant get the aml_out now!!!\n ", __FUNCTION__);
    }
    aml_dev->mix_init_flag = false;
    pthread_mutex_unlock(&aml_dev->lock);
    ret = adev_open_output_stream_new(patch->dev, 0,
                                      AUDIO_DEVICE_OUT_SPEAKER, // devices_t
                                      AUDIO_OUTPUT_FLAG_DIRECT, // flags
                                      &stream_config, &stream_out, NULL);
    if (ret < 0) {
        ALOGE("live open output stream fail, ret = %d", ret);
        goto exit_open;
    }

    ALOGI("++%s live create a output stream success now!!!\n ", __FUNCTION__);

    patch->out_buf_size = write_bytes * EAC3_MULTIPLIER;
    patch->out_buf = calloc(1, patch->out_buf_size);
    if (!patch->out_buf) {
        ret = -ENOMEM;
        goto exit_outbuf;
    }
    memset(&patch->dtv_resample, 0, sizeof(struct resample_para));
    patch->resample_outbuf = NULL;
    ALOGI("++%s live start output pcm now patch->output_thread_exit %d!!!\n ",
          __FUNCTION__, patch->output_thread_exit);

    prctl(PR_SET_NAME, (unsigned long)"audio_output_patch");

    while (!patch->output_thread_exit) {
        pthread_mutex_lock(&(patch->dtv_output_mutex));
        int period_mul =
            (patch->aformat == AUDIO_FORMAT_E_AC3) ? EAC3_MULTIPLIER : 1;
        if ((patch->aformat == AUDIO_FORMAT_AC3) ||
            (patch->aformat == AUDIO_FORMAT_E_AC3)) {
            int remain_size = 0;
            int avail = get_buffer_read_space(ringbuffer);
            if (avail > 0) {
                if (avail > (int)patch->out_buf_size) {
                    avail = (int)patch->out_buf_size;
                    if (avail > 512) {
                        avail = 512;
                    }
                } else {
                    avail = 512;
                }
                ret = ring_buffer_read(
                          ringbuffer, (unsigned char *)patch->out_buf, avail);
                if (ret == 0) {
                    pthread_mutex_unlock(&(patch->dtv_output_mutex));
                    usleep(10000);
                    ALOGE("%s(), live ring_buffer read 0 data!", __func__);
                    continue;
                }
                {
                    unsigned long pts = 0;
                    aml_out = direct_active(aml_dev);
                    unsigned int lancty = out_get_latency(&(aml_out->stream));
                    pts = decoder_apts_lookup(patch->decoder_offset);
                    process_ac3_sync(patch, pts, lancty);
                }
                {
                    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream_out;
                    if (aml_out->hal_internal_format != patch->aformat) {
                        aml_out->hal_format = aml_out->hal_internal_format = patch->aformat;
                        get_sink_format(stream_out);
                    }
                }

                remain_size = aml_dev->ddp.remain_size;
                ret = out_write_new(stream_out, patch->out_buf, ret);
                patch->outlen_after_last_validpts += aml_dev->ddp.outlen_pcm;

                patch->decoder_offset +=
                    remain_size + ret - aml_dev->ddp.remain_size;
                patch->dtv_pcm_readed += ret;
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
            } else {
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(50000);
            }

        } else if (patch->aformat == AUDIO_FORMAT_DTS) {
            int remain_size = 0;
            int avail = get_buffer_read_space(ringbuffer);
            if (avail > 0) {
                if (avail > (int)patch->out_buf_size) {
                    avail = (int)patch->out_buf_size;
                    if (avail > 1024) {
                        avail = 1024;
                    }
                } else {
                    avail = 1024;
                }
                ret = ring_buffer_read(ringbuffer, (unsigned char *)patch->out_buf,
                                       avail);
                if (ret == 0) {
                    pthread_mutex_unlock(&(patch->dtv_output_mutex));
                    usleep(10000);
                    ALOGE("%s(), live ring_buffer read 0 data!", __func__);
                    continue;
                }
                {
                    unsigned long pts = 0;
                    aml_out = direct_active(aml_dev);
                    unsigned int lancty = out_get_latency(&(aml_out->stream));
                    pts = decoder_apts_lookup(patch->decoder_offset);
                    process_ac3_sync(patch, pts, lancty);
                }
                remain_size = aml_dev->dts_hd.remain_size;
                ret = out_write_new(stream_out, patch->out_buf, ret);
                patch->outlen_after_last_validpts += aml_dev->dts_hd.outlen_pcm;

                patch->decoder_offset +=
                    remain_size + ret - aml_dev->dts_hd.remain_size;
                patch->dtv_pcm_readed += ret;
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
            } else {
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(20000);
            }

        } else {
            int avail = get_buffer_read_space(ringbuffer);

            if (avail > 0) {
                if (avail > (int)patch->out_buf_size) {
                    avail = (int)patch->out_buf_size;
                }

                ret = ring_buffer_read(ringbuffer, (unsigned char *)patch->out_buf,
                                       avail);
                if (ret == 0) {
                    pthread_mutex_unlock(&(patch->dtv_output_mutex));
                    usleep(10000);
                    ALOGE("%s(), live ring_buffer read 0 data!", __func__);
                    continue;
                }

                {
                    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream_out;
                    if (aml_out->hal_internal_format != patch->aformat) {
                        aml_out->hal_internal_format = patch->aformat;
                        get_sink_format(stream_out);
                    }
                }

                ret = out_write_new(stream_out, patch->out_buf, ret);
                {
                    aml_out = direct_active(aml_dev);
                    int pcm_lantcy = out_get_latency(&(aml_out->stream));
                    int abuf_level = get_buffer_read_space(ringbuffer);
                    process_pts_sync(pcm_lantcy, patch, abuf_level);
                }
                // ALOGE("[%s]wirte pcm %d data to pcm!\n", __FUNCTION__, ret);
                patch->dtv_pcm_readed += ret;
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
            } else {
                pthread_mutex_unlock(&(patch->dtv_output_mutex));
                usleep(10000);
            }
        }
    }

    free(patch->out_buf);
exit_outbuf:
    adev_close_output_stream_new(dev, stream_out);
exit_open:
    ALOGI("--%s live ", __FUNCTION__);
    return ((void *)0);
}

static void patch_thread_get_cmd(int *cmd)
{
    if (dtv_patch_cmd_is_empty() == 1) {
        *cmd = AUDIO_DTV_PATCH_CMD_NULL;
    } else {
        *cmd = dtv_patch_get_cmd();
    }
}

static void *audio_dtv_patch_process_threadloop(void *data)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)data;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    ring_buffer_t *ringbuffer = &(patch->aml_ringbuffer);
    struct audio_stream_in *stream_in = NULL;
    struct audio_config stream_config;
    // FIXME: add calc for read_bytes;
    int read_bytes = DEFAULT_CAPTURE_PERIOD_SIZE * CAPTURE_PERIOD_COUNT;
    int ret = 0, retry = 0;
    audio_format_t cur_aformat;
    unsigned int adec_handle;
    int cmd = AUDIO_DTV_PATCH_CMD_NUM;
    struct dolby_ddp_dec *ddp_dec = &(aml_dev->ddp);
    struct dca_dts_dec *dts_dec = &(aml_dev->dts_hd);
    patch->sample_rate = stream_config.sample_rate = 48000;
    patch->chanmask = stream_config.channel_mask = AUDIO_CHANNEL_IN_STEREO;
    patch->aformat = stream_config.format = AUDIO_FORMAT_PCM_16_BIT;

    ALOGI("++%s live \n", __FUNCTION__);
    patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_INIT;

    while (!patch->input_thread_exit) {
        pthread_mutex_lock(&patch->dtv_input_mutex);
        switch (patch->dtv_decoder_state) {
        case AUDIO_DTV_PATCH_DECODER_STATE_INIT: {
            ALOGI("++%s live now  open the audio decoder now !\n", __FUNCTION__);
            dtv_patch_input_open(&adec_handle, dtv_patch_pcm_wirte,
                                 dtv_patch_buffer_space, patch);
            patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_START;
        }
        break;
        case AUDIO_DTV_PATCH_DECODER_STATE_START:
            patch_thread_get_cmd(&cmd);
            if (cmd == AUDIO_DTV_PATCH_CMD_NULL) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }

            if (cmd == AUDIO_DTV_PATCH_CMD_START) {
                dtv_patch_input_start(adec_handle, patch->dtv_aformat,
                                      patch->dtv_has_video);
                ALOGI("++%s live now  start the audio decoder now !\n",
                      __FUNCTION__);
                patch->dtv_first_apts_flag = 0;
                if (patch->dtv_aformat == ACODEC_FMT_AC3) {
                    patch->aformat = AUDIO_FORMAT_AC3;
                    dcv_decoder_init_patch(ddp_dec);
                    ddp_dec->is_iec61937  = false;
                    patch->decoder_offset = 0;
                } else if (patch->dtv_aformat == ACODEC_FMT_EAC3) {
                    patch->aformat = AUDIO_FORMAT_E_AC3;
                    dcv_decoder_init_patch(ddp_dec);
                    ddp_dec->is_iec61937  = false;
                    patch->decoder_offset = 0;
                } else if (patch->dtv_aformat == ACODEC_FMT_DTS) {
                    patch->aformat = AUDIO_FORMAT_DTS;
                    dca_decoder_init_patch(dts_dec);
                    patch->decoder_offset = 0;
                }
                patch->dtv_pcm_readed = patch->dtv_pcm_writed = 0;
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_RUNING;
            } else {
                ALOGI("++%s line %d  live state unsupport state %d cmd %d !\n",
                      __FUNCTION__, __LINE__, patch->dtv_decoder_state, cmd);
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }
            break;
        case AUDIO_DTV_PATCH_DECODER_STATE_RUNING:
            patch_thread_get_cmd(&cmd);
            if (cmd == AUDIO_DTV_PATCH_CMD_NULL) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }
            if (cmd == AUDIO_DTV_PATCH_CMD_PAUSE) {
                ALOGI("++%s live now start  pause  the audio decoder now \n",
                      __FUNCTION__);
                dtv_patch_input_pause(adec_handle);
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_PAUSE;
                ALOGI("++%s live now end  pause  the audio decoder now \n",
                      __FUNCTION__);
            } else if (cmd == AUDIO_DTV_PATCH_CMD_STOP) {
                ALOGI("++%s live now  stop  the audio decoder now \n", __FUNCTION__);
                dtv_patch_input_stop(adec_handle);

                patch->sample_rate = stream_config.sample_rate = 48000;
                patch->chanmask = stream_config.channel_mask = AUDIO_CHANNEL_IN_STEREO;
                patch->aformat = stream_config.format = AUDIO_FORMAT_PCM_16_BIT;
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_INIT;
            } else {
                ALOGI("++%s line %d  live state unsupport state %d cmd %d !\n",
                      __FUNCTION__, __LINE__, patch->dtv_decoder_state, cmd);
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }

            break;

        case AUDIO_DTV_PATCH_DECODER_STATE_PAUSE:
            patch_thread_get_cmd(&cmd);
            if (cmd == AUDIO_DTV_PATCH_CMD_NULL) {
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;
            }
            if (cmd == AUDIO_DTV_PATCH_CMD_RESUME) {
                dtv_patch_input_resume(adec_handle);
                patch->dtv_decoder_state = AUDIO_DTV_PATCH_DECODER_STATE_RUNING;
            } else {
                ALOGI("++%s line %d  live state unsupport state %d cmd %d !\n",
                      __FUNCTION__, __LINE__, patch->dtv_decoder_state, cmd);
                pthread_mutex_unlock(&patch->dtv_input_mutex);
                usleep(50000);
                continue;

            }
            break;
        default:
            pthread_mutex_unlock(&patch->dtv_input_mutex);
            usleep(50000);
            continue;
            break;
        }
        pthread_mutex_unlock(&patch->dtv_input_mutex);
    }
    ALOGI("++%s now  live  release  the audio decoder", __FUNCTION__);
    dtv_patch_input_stop(adec_handle);
    pthread_exit(NULL);
}

int dtv_in_read(struct aml_audio_device *dev, unsigned char *buf, int bytes)
{
    int ret = 0;
    if (dev == NULL || buf == NULL || bytes == 0) {
        ALOGI("[%s] pls check the input parameters \n", __FUNCTION__);
    }

    struct aml_audio_patch *patch = dev->audio_patch;

    if (patch->aformat == AUDIO_FORMAT_AC3 ||
        patch->aformat == AUDIO_FORMAT_E_AC3) {
        if (patch->dtv_decoder_ready == 0) {
            memset(buf, 0, bytes);
        } else {
            ret =
                ring_buffer_read(&patch->aml_ringbuffer, (unsigned char *)buf, bytes);
        }
    } else if (patch->aformat == AUDIO_FORMAT_DTS) {
        if (patch->dtv_decoder_ready == 0) {
        } else {
        }
    } else {
    }
    return 0;
}

#define AVSYNC_SAMPLE_INTERVAL (50)
#define AVSYNC_SAMPLE_MAX_CNT (10)

static int create_dtv_output_stream_thread(struct aml_audio_patch *patch)
{
    int ret = 0;
    ALOGI("++%s   ---- %d\n", __FUNCTION__, patch->ouput_thread_created);

    if (patch->ouput_thread_created == 0) {
        pthread_mutex_init(&patch->dtv_output_mutex, NULL);
        ret = pthread_create(&(patch->audio_output_threadID), NULL,
                             audio_dtv_patch_output_threadloop, patch);
        if (ret != 0) {
            ALOGE("%s, Create output thread fail!\n", __FUNCTION__);
            pthread_mutex_destroy(&patch->dtv_output_mutex);
            return -1;
        }
        patch->output_thread_exit = 0;
        patch->ouput_thread_created = 1;
    }
    ALOGI("--%s", __FUNCTION__);
    return 0;
}

static int release_dtv_output_stream_thread(struct aml_audio_patch *patch)
{
    int ret = 0;
    ALOGI("++%s   ---- %d\n", __FUNCTION__, patch->ouput_thread_created);

    if (patch->ouput_thread_created == 1) {
        pthread_mutex_lock(&patch->dtv_output_mutex);
        patch->output_thread_exit = 1;
        pthread_mutex_unlock(&patch->dtv_output_mutex);
        pthread_join(patch->audio_output_threadID, NULL);
        pthread_mutex_destroy(&patch->dtv_output_mutex);

        patch->ouput_thread_created = 0;
    }
    ALOGI("--%s", __FUNCTION__);

    return 0;
}

int create_dtv_patch(struct audio_hw_device *dev, audio_devices_t input,
                     audio_devices_t output __unused)
{
    struct aml_audio_patch *patch;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *)dev;
    int period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    pthread_attr_t attr;
    struct sched_param param;

    int ret = 0;
    // ALOGI("++%s live period_size %d\n", __func__, period_size);
    init_cmd_list();
    pthread_mutex_lock(&dtv_patch_mutex);
    patch = calloc(1, sizeof(*patch));
    if (!patch) {
        ret = -1;
        goto err;
    }

    memset(cmd_array, 0, sizeof(cmd_array));
    // save dev to patch
    patch->dev = dev;
    patch->input_src = input;
    patch->aformat = AUDIO_FORMAT_PCM_16_BIT;
    patch->avsync_sample_max_cnt = AVSYNC_SAMPLE_MAX_CNT;

    patch->output_thread_exit = 0;
    patch->input_thread_exit = 0;
    aml_dev->audio_patch = patch;
    pthread_mutex_init(&patch->mutex, NULL);
    pthread_cond_init(&patch->cond, NULL);
    pthread_mutex_init(&patch->dtv_input_mutex, NULL);

    ret = ring_buffer_init(&(patch->aml_ringbuffer),
                           4 * period_size * PATCH_PERIOD_COUNT * 10);
    if (ret < 0) {
        ALOGE("Fail to init audio ringbuffer!");
        goto err_ring_buf;
    }

    ret = pthread_create(&(patch->audio_input_threadID), NULL,
                         audio_dtv_patch_process_threadloop, patch);
    if (ret != 0) {
        ALOGE("%s, Create process thread fail!\n", __FUNCTION__);
        goto err_in_thread;
    }
    create_dtv_output_stream_thread(patch);
    pthread_mutex_unlock(&dtv_patch_mutex);

    ALOGI("--%s", __FUNCTION__);
    return 0;
err_parse_thread:
err_out_thread:
    patch->input_thread_exit = 1;
    pthread_join(patch->audio_input_threadID, NULL);
err_in_thread:
    ring_buffer_release(&(patch->aml_ringbuffer));
err_ring_buf:
    free(patch);
err:
    pthread_mutex_unlock(&dtv_patch_mutex);
    return ret;
}

int release_dtv_patch(struct aml_audio_device *aml_dev)
{
    if (aml_dev == NULL) {
        ALOGI("[%s]release the dtv patch failed  aml_dev == NULL\n", __FUNCTION__);
        return -1;
    }
    struct aml_audio_patch *patch = aml_dev->audio_patch;

    ALOGI("++%s live\n", __FUNCTION__);
    pthread_mutex_lock(&dtv_patch_mutex);
    if (patch == NULL) {
        ALOGI("release the dtv patch failed  patch == NULL\n");
        pthread_mutex_unlock(&dtv_patch_mutex);
        return -1;
    }



    pthread_mutex_lock(&patch->dtv_input_mutex);
    patch->input_thread_exit = 1;
    pthread_mutex_unlock(&patch->dtv_input_mutex);

    pthread_join(patch->audio_input_threadID, NULL);


    pthread_mutex_destroy(&patch->dtv_input_mutex);
    release_dtv_output_stream_thread(patch);

    ring_buffer_release(&(patch->aml_ringbuffer));
    free(patch);
    aml_dev->audio_patch = NULL;
    ALOGI("--%s", __FUNCTION__);
    pthread_mutex_unlock(&dtv_patch_mutex);

    return 0;
}
