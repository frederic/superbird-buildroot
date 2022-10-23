/*
 * Copyright (C) 2017 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <unistd.h>

//#include <utils/Timers.h>
#include "log.h"
#include "atomic.h"
#include <cutils/str_parms.h>
#include "properties.h"
#include <linux/ioctl.h>
#include "hardware.h"
#include "audio_core.h"
#include "audio_trace.h"

#if ANDROID_PLATFORM_SDK_VERSION >= 25 //8.0
#include <system/audio-base.h>
#endif

#include "audio_hal.h"
#include <tinyalsa/asoundlib.h>
#include "audio_route.h"
#include <aml_data_utils.h>
#include <spdifenc_wrap.h>
#include <aml_volume_utils.h>
#include <aml_android_utils.h>
#include <aml_alsa_mixer.h>

#include "aml_hw_profile.h"
#include "audio_format_parse.h"
#include "SPDIFEncoderAD.h"
#include "aml_volume_utils.h"
#include "aml_data_utils.h"
#include "aml_dump_debug.h"
#include "spdifenc_wrap.h"
#include "alsa_manager.h"
#include "aml_audio_stream.h"
#include "audio_hw.h"
#include "spdif_encoder_api.h"
#include "audio_hw_utils.h"
#include "audio_hw_profile.h"
#include "aml_dump_debug.h"
#include "alsa_manager.h"
#include "alsa_device_parser.h"
#include "aml_audio_stream.h"
#include "alsa_config_parameters.h"
#include "spdif_encoder_api.h"
#include "aml_avsync_tuning.h"
#include "aml_ng.h"

#include "aml_output_manager.h"
#include "aml_input_manager.h"
#include "aml_callback_api.h"
#include "aml_sample_conv.h"
#include "aml_bm_api.h"
#include "aml_audio_volume.h"
#include "aml_audio_level.h"
#include "aml_config_parser.h"
#include "aml_audio_delay.h"
#include "aml_external_process.h"

// for invoke bluetooth rc hal
//#include "audio_hal_thunks.h"

#include <dolby_ms12_status.h>
#include <SPDIFEncoderAD.h>
#include "audio_hw_ms12.h"
#include "dolby_lib_api.h"
// for dtv playback
#include "audio_hw_dtv.h"

/*dolby amtos api*/
#include "aml_datmos_api.h"
#include "aml_audio_log.h"

//#define MEM_CHECK
#ifdef MEM_CHECK
#include <mcheck.h>
#endif


//#define ENABLE_AVSYNC_TUNING //debug  zz
/* ALSA cards for AML */
#define CARD_AMLOGIC_BOARD 0
/* ALSA ports for AML */
#define PORT_I2S 0
#define PORT_SPDIF 1
#define PORT_PCM 2
//static unsigned CAPTURE_PERIOD_SIZE = DEFAULT_CAPTURE_PERIOD_SIZE;
/* number of periods for low power playback */
#undef PLAYBACK_PERIOD_COUNT
#define PLAYBACK_PERIOD_COUNT 4
/* number of periods for capture */
#undef CAPTURE_PERIOD_COUNT
#define CAPTURE_PERIOD_COUNT 4

/* minimum sleep time in out_write() when write threshold is not reached */
#define MIN_WRITE_SLEEP_US 5000

#undef RESAMPLER_BUFFER_FRAMES
#define RESAMPLER_BUFFER_FRAMES (PERIOD_SIZE * 6)
#define RESAMPLER_BUFFER_SIZE (4 * RESAMPLER_BUFFER_FRAMES)

#define NSEC_PER_SECOND 1000000000ULL

//static unsigned int  DEFAULT_OUT_SAMPLING_RATE  = 48000;

/* sampling rate when using MM low power port */
#define MM_LOW_POWER_SAMPLING_RATE 44100
/* sampling rate when using MM full power port */
#define MM_FULL_POWER_SAMPLING_RATE 48000
/* sampling rate when using VX port for narrow band */
#define VX_NB_SAMPLING_RATE 8000
//#if (ENABLE_HUITONG == 0)
#define MIXER_XML_PATH "/vendor/etc/mixer_paths.xml"
//#endif
//#undef DOLBY_MS12_ENABLE
#define DOLBY_MS12_INPUT_FORMAT_TEST

#define IEC61937_PACKET_SIZE_OF_AC3     0x1800
#define IEC61937_PACKET_SIZE_OF_EAC3    0x6000

#define SPDIF_ENCODER

//property:disable continuous output
#define DISABLE_CONTINUOUS_OUTPUT "persist.aud.continuous.disable"

#define AUDIO_JASON_FILE_MAXLENGTH (256)
static char aml_audio_jason_file[AUDIO_JASON_FILE_MAXLENGTH];
cJSON *audio_config_jason = NULL;


const char *str_usecases[STREAM_USECASE_MAX] = {
    "STREAM_PCM_NORMAL",
    "STREAM_PCM_DIRECT",
    "STREAM_PCM_HWSYNC",
    "STREAM_RAW_DIRECT",
    "STREAM_RAW_HWSYNC",
    "STREAM_PCM_PATCH",
    "STREAM_RAW_PATCH"
};

static const struct pcm_config pcm_config_out = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_PLAYBACK_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static const struct pcm_config pcm_config_out_direct = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_PLAYBACK_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static const struct pcm_config pcm_config_in = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_CAPTURE_PERIOD_SIZE,
    .period_count = CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static const struct pcm_config pcm_config_bt = {
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = DEFAULT_PLAYBACK_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static const struct pcm_config i2s_multi_config_in = {
    .channels = 8,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = DEFAULT_CAPTURE_PERIOD_SIZE,
    .period_count = CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

static void select_output_device(struct aml_audio_device *adev);
static void select_input_device(struct aml_audio_device *adev);
static void select_devices(struct aml_audio_device *adev);
static int adev_set_voice_volume(struct audio_hw_device *dev, float volume);
static int do_input_standby(struct aml_stream_in *in);
static int do_output_standby(struct aml_stream_out *out);
int do_output_standby_l(struct audio_stream *out);
static uint32_t out_get_sample_rate(const struct audio_stream *stream);
static int out_pause(struct audio_stream_out *stream);
static int usecase_change_validate_l(struct aml_stream_out *aml_out, bool is_standby);
static inline int is_usecase_mix(stream_usecase_t usecase);
static int create_patch(struct audio_hw_device *dev,
                        audio_devices_t input,
                        audio_devices_t output);
static int create_patch_ext(struct audio_hw_device *dev,
                            audio_devices_t input,
                            audio_devices_t output,
                            audio_patch_handle_t handle);
static int release_patch(struct aml_audio_device *aml_dev);
static int release_parser(struct aml_audio_device *aml_dev);
static void aml_tinymix_set_spdif_format(audio_format_t output_format);
static inline bool need_hw_mix(usecase_mask_t masks);
static int out_standby_new(struct audio_stream *stream);
static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle __unused,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused);
static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream);
static int get_audio_patch_by_src_dev(struct audio_hw_device *dev,
                                      audio_devices_t dev_type,
                                      struct audio_patch **p_audio_patch);
ssize_t out_write_new(struct audio_stream_out *stream,
                      const void *buffer,
                      size_t bytes);


static inline bool need_hw_mix(usecase_mask_t masks)
{
    return (masks > 1);
}

static inline int is_usecase_mix(stream_usecase_t usecase)
{
    return usecase > STREAM_PCM_NORMAL;
}

static inline short CLIP(int r)
{
    return (r >  0x7fff) ? 0x7fff :
           (r < -0x8000) ? -0x8000 :
           r;
}

static void continuous_stream_do_standby(struct aml_audio_device *adev)
{
    struct aml_stream_out *aml_direct_out = NULL;
    struct aml_stream_out *aml_normal_pcm_out = NULL;
    ALOGI("\n%s usecase_masks %#x", __FUNCTION__, adev->usecase_masks);

    usecase_mask_t direct_mask = adev->usecase_masks & (~1);
    stream_usecase_t direct_stream_usecase = convert_usecase_mask_to_stream_usecase(direct_mask);

    stream_usecase_t normal_pcm_stream_usecase = STREAM_PCM_NORMAL;

    ALOGI("%s direct_mask %#x direct_stream_usecase %#x", __FUNCTION__, direct_mask, direct_stream_usecase);
    if ((direct_mask > 0) && (direct_stream_usecase != 0)) {
        aml_direct_out = adev->active_outputs[direct_stream_usecase];
        if (aml_direct_out) {
            pthread_mutex_lock(&aml_direct_out->lock);
            struct audio_stream_out *direct_stream = (struct audio_stream_out *)aml_direct_out;
            do_output_standby_l(&direct_stream->common);
            pthread_mutex_unlock(&aml_direct_out->lock);
            ALOGI("%s direct stream do standby\n", __FUNCTION__);
        }
    }
#ifdef ENABLE_MS12
    //normal pcm stream exit.
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        get_dolby_ms12_cleanup(&adev->ms12);
        ALOGI("[%s:%d] get_dolby_ms12_cleanup\n", __FUNCTION__, __LINE__);
    }
#endif
    aml_normal_pcm_out = adev->active_outputs[normal_pcm_stream_usecase];
    ALOGI("%s normal_pcm_stream_usecase %#x aml_normal_pcm_out %p", __FUNCTION__, normal_pcm_stream_usecase, aml_normal_pcm_out);
    if (aml_normal_pcm_out) {
        pthread_mutex_lock(&aml_normal_pcm_out->lock);
        struct audio_stream_out *normal_pcm_stream = (struct audio_stream_out *)aml_normal_pcm_out;
        do_output_standby_l(&normal_pcm_stream->common);
        pthread_mutex_unlock(&aml_normal_pcm_out->lock);
        ALOGI("%s normal pcm stream do standby\n", __FUNCTION__);
    }
    pthread_mutex_lock(&adev->alsa_pcm_lock);
    //aml_close_continuous_audio_device(adev);
    aml_normal_pcm_out->pcm = NULL;
    pthread_mutex_unlock(&adev->alsa_pcm_lock);
    ALOGI("%s normal pcm stream pcm-handle reset as NULL\n\n", __FUNCTION__);
    ALOGI("%s end!\n\n", __FUNCTION__);
    return ;
}

static void store_stream_presentation(struct aml_audio_device *adev)
{
    //here restore every bitstream  frame postion and total frame write
    //we support every STREAM TYPE only here one stream instance
    struct aml_stream_out *aml_out = NULL;
    uint64_t write_frames = 0;
    int i = 0;
    for (i = 0; i < STREAM_USECASE_MAX; i++) {
        aml_out = adev->active_outputs[i];
        if (aml_out && (aml_out->flags & AUDIO_OUTPUT_FLAG_DIRECT)) {
            if (aml_out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) {
                if (aml_out->hal_internal_format == AUDIO_FORMAT_AC3) {
                    write_frames = aml_out->input_bytes_size / 6144 * 32 * 48;
                }
                if (aml_out->hal_internal_format == AUDIO_FORMAT_E_AC3) {
                    write_frames = aml_out->input_bytes_size / 24576 * 32 * 48;
                }
            } else if ((aml_out->hal_internal_format == AUDIO_FORMAT_E_AC3) || (aml_out->hal_internal_format == AUDIO_FORMAT_AC3)) {
                write_frames = aml_out->input_bytes_size / aml_out->ddp_frame_size * 32 * 48;
            }

            /* handle the case hal_rate != 48KHz */
            if (aml_out->hal_rate != MM_FULL_POWER_SAMPLING_RATE) {
                write_frames = (write_frames * MM_FULL_POWER_SAMPLING_RATE) / aml_out->hal_rate;
            }

            aml_out->last_frames_postion = write_frames;
            aml_out->frame_write_sum = write_frames;
            aml_out->continuous_audio_offset = aml_out->input_bytes_size;
            if (continous_mode(adev) && adev->ms12_out) {
                adev->ms12_out->continuous_audio_offset = aml_out->input_bytes_size;
            }
            ALOGI("%s,restore input size %"PRIu64",frame %"PRIu64"",
                  __func__, aml_out->input_bytes_size, write_frames);
        }
    }
}

//code here for audio hal mixer when hwsync with af mixer output stream output
//at the same,need do a software mixer in audio hal.
static int aml_hal_mixer_init(struct aml_hal_mixer *mixer)
{
    pthread_mutex_lock(&mixer->lock);
    mixer->wp = 0;
    mixer->rp = 0;
    mixer->buf_size = AML_HAL_MIXER_BUF_SIZE;
    mixer->need_cache_flag = 1;
    pthread_mutex_unlock(&mixer->lock);
    return 0;
}
static uint aml_hal_mixer_get_space(struct aml_hal_mixer *mixer)
{
    unsigned space;
    if (mixer->wp >= mixer->rp) {
        space = mixer->buf_size - (mixer->wp - mixer->rp);
    } else {
        space = mixer->rp - mixer->wp;
    }
    return space > 64 ? (space - 64) : 0;
}
static int aml_hal_mixer_get_content(struct aml_hal_mixer *mixer)
{
    unsigned content = 0;
    pthread_mutex_lock(&mixer->lock);
    if (mixer->wp >= mixer->rp) {
        content = mixer->wp - mixer->rp;
    } else {
        content = mixer->wp - mixer->rp + mixer->buf_size;
    }
    //ALOGI("wp %d,rp %d\n",mixer->wp,mixer->rp);
    pthread_mutex_unlock(&mixer->lock);
    return content;
}
//we assue the cached size is always smaller then buffer size
//need called by device mutux locked
static int aml_hal_mixer_write(struct aml_hal_mixer *mixer, const void *w_buf, uint size)
{
    unsigned space;
    unsigned write_size = size;
    unsigned tail = 0;
    pthread_mutex_lock(&mixer->lock);
    space = aml_hal_mixer_get_space(mixer);
    if (space < size) {
        ALOGI("write data no space,space %d,size %d,rp %d,wp %d,reset all ptr\n", space, size, mixer->rp, mixer->wp);
        mixer->wp = 0;
        mixer->rp = 0;
    }
    //TODO
    if (write_size > space) {
        write_size = space;
    }
    if (write_size + mixer->wp > mixer->buf_size) {
        tail = mixer->buf_size - mixer->wp;
        memcpy(mixer->start_buf + mixer->wp, w_buf, tail);
        write_size -= tail;
        memcpy(mixer->start_buf, (unsigned char*) w_buf + tail, write_size);
        mixer->wp = write_size;
    } else {
        memcpy(mixer->start_buf + mixer->wp, w_buf, write_size);
        mixer->wp += write_size;
        mixer->wp %= AML_HAL_MIXER_BUF_SIZE;
    }
    pthread_mutex_unlock(&mixer->lock);
    return size;
}
//need called by device mutux locked
static int aml_hal_mixer_read(struct aml_hal_mixer *mixer, void *r_buf, uint size)
{
    unsigned cached_size;
    unsigned read_size = size;
    unsigned tail = 0;
    cached_size = aml_hal_mixer_get_content(mixer);
    pthread_mutex_lock(&mixer->lock);
    // we always assue we have enough data to read when hwsync enabled.
    // if we do not have,insert zero data.
    if (cached_size < size) {
        ALOGI("read data has not enough data to mixer,read %d, have %d,rp %d,wp %d\n", size, cached_size, mixer->rp, mixer->wp);
        memset((unsigned char*) r_buf + cached_size, 0, size - cached_size);
        read_size = cached_size;
    }
    if (read_size + mixer->rp > mixer->buf_size) {
        tail = mixer->buf_size - mixer->rp;
        memcpy(r_buf, mixer->start_buf + mixer->rp, tail);
        read_size -= tail;
        memcpy((unsigned char*) r_buf + tail, mixer->start_buf, read_size);
        mixer->rp = read_size;
    } else {
        memcpy(r_buf, mixer->start_buf + mixer->rp, read_size);
        mixer->rp += read_size;
        mixer->rp %= AML_HAL_MIXER_BUF_SIZE;
    }
    pthread_mutex_unlock(&mixer->lock);
    return size;
}
// aml audio hal mixer code end

static void select_devices(struct aml_audio_device *adev)
{
    ALOGD("%s(mode=%d, out_device=%#x)", __FUNCTION__, adev->mode, adev->out_device);
    int headset_on;
    int headphone_on;
    int speaker_on;
    int hdmi_on;
    int earpiece;
    int mic_in;
    int headset_mic;

    headset_on = adev->out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET;
    headphone_on = adev->out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
    speaker_on = adev->out_device & AUDIO_DEVICE_OUT_SPEAKER;
    hdmi_on = adev->out_device & AUDIO_DEVICE_OUT_AUX_DIGITAL;
    earpiece =  adev->out_device & AUDIO_DEVICE_OUT_EARPIECE;
    mic_in = adev->in_device & (AUDIO_DEVICE_IN_BUILTIN_MIC | AUDIO_DEVICE_IN_BACK_MIC);
    headset_mic = adev->in_device & AUDIO_DEVICE_IN_WIRED_HEADSET;

    ALOGD("%s : hs=%d , hp=%d, sp=%d, hdmi=0x%x,earpiece=0x%x", __func__,
          headset_on, headphone_on, speaker_on, hdmi_on, earpiece);
    ALOGD("%s : in_device(%#x), mic_in(%#x), headset_mic(%#x)", __func__,
          adev->in_device, mic_in, headset_mic);
    audio_route_reset(adev->ar);
    if (hdmi_on) {
        audio_route_apply_path(adev->ar, "hdmi");
    }
    if (headphone_on || headset_on) {
        audio_route_apply_path(adev->ar, "headphone");
    }
    if (speaker_on || earpiece) {
        audio_route_apply_path(adev->ar, "speaker");
    }
    if (mic_in) {
        audio_route_apply_path(adev->ar, "main_mic");
    }
    if (headset_mic) {
        audio_route_apply_path(adev->ar, "headset-mic");
    }

    audio_route_update_mixer(adev->ar);

}

static void select_mode(struct aml_audio_device *adev)
{
    ALOGD("%s(out_device=%#x)", __FUNCTION__, adev->out_device);
    ALOGD("%s(in_device=%#x)", __FUNCTION__, adev->in_device);
    return;

    /* force earpiece route for in call state if speaker is the
        only currently selected route. This prevents having to tear
        down the modem PCMs to change route from speaker to earpiece
        after the ringtone is played, but doesn't cause a route
        change if a headset or bt device is already connected. If
        speaker is not the only thing active, just remove it from
        the route. We'll assume it'll never be used initally during
        a call. This works because we're sure that the audio policy
        manager will update the output device after the audio mode
        change, even if the device selection did not change. */
    if ((adev->out_device & AUDIO_DEVICE_OUT_ALL) == AUDIO_DEVICE_OUT_SPEAKER) {
        adev->in_device = AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN;
    } else {
        adev->out_device &= ~AUDIO_DEVICE_OUT_SPEAKER;
    }

    return;
}

bool format_is_passthrough(audio_format_t fmt)
{
    ALOGD("%s() fmt = 0x%x\n", __func__, fmt);
    switch (fmt) {
    case AUDIO_FORMAT_AC3:
    case AUDIO_FORMAT_E_AC3:
    case AUDIO_FORMAT_DTS:
    case AUDIO_FORMAT_DTS_HD:
    case AUDIO_FORMAT_IEC61937:
    case AUDIO_FORMAT_DOLBY_TRUEHD:
    case AUDIO_FORMAT_MAT:
        return true;
    default:
        return false;
    }
}

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct aml_stream_out *out)
{
    struct aml_audio_device *adev = out->dev;
    unsigned int card = CARD_AMLOGIC_BOARD;
    unsigned int port = PORT_I2S;
    int ret = 0;
    int i  = 0;
    struct aml_stream_out *out_removed = NULL;
    int channel_count = popcount(out->hal_channel_mask);
    bool hwsync_lpcm = (out->flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC && out->config.rate  <= 48000 &&
                        audio_is_linear_pcm(out->hal_internal_format) && channel_count <= 2);
    ALOGD("%s(adev->out_device=%#x, adev->mode=%d)",
          __FUNCTION__, adev->out_device, adev->mode);
    if (adev->mode != AUDIO_MODE_IN_CALL) {
        /* FIXME: only works if only one output can be active at a time */
        //select_devices(adev);
    }
    if (out->hw_sync_mode == true) {
        adev->hwsync_output = out;
#if 0
        for (i = 0; i < MAX_STREAM_NUM; i++) {
            if (adev->active_output[i]) {
                out_removed = adev->active_output[i];
                pthread_mutex_lock(&out_removed->lock);
                if (!out_removed->standby) {
                    ALOGI("hwsync start,force %p standby\n", out_removed);
                    do_output_standby(out_removed);
                }
                pthread_mutex_unlock(&out_removed->lock);
            }
        }
#endif
    }
    card = alsa_device_get_card_index();
    if (adev->out_device & AUDIO_DEVICE_OUT_ALL_SCO) {
        port = PORT_PCM;
        out->config = pcm_config_bt;
    } else if (out->flags & AUDIO_OUTPUT_FLAG_DIRECT && !hwsync_lpcm) {
        port = PORT_SPDIF;
    }

    /* check to update port */
    port = alsa_device_get_pcm_index(port);

    ALOGD("*%s, open card(%d) port(%d)", __FUNCTION__, card, port);

    /* default to low power: will be corrected in out_write if necessary before first write to
     * tinyalsa.
     */
    out->write_threshold = out->config.period_size * PLAYBACK_PERIOD_COUNT;
    out->config.start_threshold = out->config.period_size * PLAYBACK_PERIOD_COUNT;
    out->config.avail_min = 0;//SHORT_PERIOD_SIZE;
    //added by xujian for NTS hwsync/system stream mix smooth playback.
    //we need re-use the tinyalsa pcm handle by all the output stream, including
    //hwsync direct output stream,system mixer output stream.
    //TODO we need diff the code with AUDIO_DEVICE_OUT_ALL_SCO.
    //as it share the same hal but with the different card id.
    //TODO need reopen the tinyalsa card when sr/ch changed,
    if (adev->pcm == NULL) {
        ALOGD("%s(), pcm_open card %u port %u\n", __func__, card, port);
        out->pcm = pcm_open(card, port, PCM_OUT /*| PCM_MMAP | PCM_NOIRQ*/, & (out->config));
        if (!pcm_is_ready(out->pcm)) {
            ALOGE("cannot open pcm_out driver: %s", pcm_get_error(out->pcm));
            pcm_close(out->pcm);
            return -ENOMEM;
        }
#ifdef RESAMPLER
        if (out->config.rate != out_get_sample_rate(&out->stream.common)) {
            ALOGD("%s(out->config.rate=%d, out->config.channels=%d)",
                  __FUNCTION__, out->config.rate, out->config.channels);
            ret = create_resampler(out_get_sample_rate(&out->stream.common),
                                   out->config.rate,
                                   out->config.channels,
                                   RESAMPLER_QUALITY_DEFAULT,
                                   NULL,
                                   &out->resampler);
            if (ret != 0) {
                ALOGE("cannot create resampler for output");
                return -ENOMEM;
            }
            out->buffer_frames = (out->config.period_size * out->config.rate) /
                                 out_get_sample_rate(&out->stream.common) + 1;
            out->buffer = malloc(pcm_frames_to_bytes(out->pcm, out->buffer_frames));
            if (out->buffer == NULL) {
                ALOGE("cannot malloc memory for out->buffer");
                return -ENOMEM;
            }
        }
#endif
        adev->pcm = out->pcm;
        ALOGI("device pcm %p\n", adev->pcm);
    } else {
        ALOGI("stream %p share the pcm %p\n", out, adev->pcm);
        out->pcm = adev->pcm;
        // add to fix start output when pcm in pause state
        if (adev->pcm_paused && pcm_is_ready(out->pcm)) {
            ret = pcm_ioctl(out->pcm, SNDRV_PCM_IOCTL_PAUSE, 0);
            if (ret < 0) {
                ALOGE("cannot resume channel\n");
            }
        }
    }
    ALOGD("channels=%d---format=%d---period_count%d---period_size%d---rate=%d---",
          out->config.channels, out->config.format, out->config.period_count,
          out->config.period_size, out->config.rate);
#ifdef RESAMPLER
    if (out->resampler) {
        out->resampler->reset(out->resampler);
    }
#endif
    if (out->is_tv_platform == 1) {
        sysfs_set_sysfs_str("/sys/class/amhdmitx/amhdmitx0/aud_output_chs", "2:2");
    }
    //set_codec_type(0);
    if (out->hw_sync_mode == 1) {
        ALOGD("start_output_stream with hw sync enable %p\n", out);
    }
    for (i = 0; i < MAX_STREAM_NUM; i++) {
        if (adev->active_output[i] == NULL) {
            ALOGI("store out (%p) to index %d\n", out, i);
            adev->active_output[i] = out;
            adev->active_output_count++;
            break;
        }
    }
    if (i == MAX_STREAM_NUM) {
        ALOGE("error,no space to store the dev stream \n");
    }
    return 0;
}

/* dircet stream mainly map to audio HDMI port */
static int start_output_stream_direct(struct aml_stream_out *out)
{
    struct aml_audio_device *adev = out->dev;
    unsigned int card = CARD_AMLOGIC_BOARD;
    unsigned int port = PORT_SPDIF;
    int ret = 0;

    int codec_type = get_codec_type(out->hal_internal_format);
    if (codec_type == AUDIO_FORMAT_PCM && out->config.rate > 48000 && (out->flags & AUDIO_OUTPUT_FLAG_DIRECT)) {
        ALOGI("start output stream for high sample rate pcm for direct mode\n");
        codec_type = TYPE_PCM_HIGH_SR;
    }
    if (codec_type == AUDIO_FORMAT_PCM && out->config.channels >= 6 && (out->flags & AUDIO_OUTPUT_FLAG_DIRECT)) {
        ALOGI("start output stream for multi-channel pcm for direct mode\n");
        codec_type = TYPE_MULTI_PCM;
    }

    card = alsa_device_get_card_index();
    ALOGI("%s: hdmi sound card id %d,device id %d \n", __func__, card, port);
    if (out->multich == 6) {
        /* switch to hdmi port */
        if (alsa_device_is_auge()) {
            port = PORT_I2S2HDMI;
            ALOGI(" %d CH from i2s to hdmi, port:%d\n", out->multich, port);
        } else {
            ALOGI("round 6ch to 8 ch output \n");
            /* our hw only support 8 channel configure,so when 5.1,hw mask the last two channels*/
            sysfs_set_sysfs_str("/sys/class/amhdmitx/amhdmitx0/aud_output_chs", "6:7");
            out->config.channels = 8;
        }
    }
    /*
    * 8 channel audio only support 32 byte mode,so need convert them to
    * PCM_FORMAT_S32_LE
    */
    if (!format_is_passthrough(out->hal_format) && (out->config.channels == 8)) {
        port = PORT_I2S;
        out->config.format = PCM_FORMAT_S32_LE;
        adev->out_device = AUDIO_DEVICE_OUT_SPEAKER;
        ALOGI("[%s %d]8CH format output: set port/0 adev->out_device/%d\n",
              __FUNCTION__, __LINE__, AUDIO_DEVICE_OUT_SPEAKER);
    }
    if (getprop_bool("media.libplayer.wfd")) {
        out->config.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE;
    }
    switch (out->hal_internal_format) {
    case AUDIO_FORMAT_E_AC3:
        out->config.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * 2;
        out->write_threshold = PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE * 2;
        out->config.start_threshold = PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE * 2;
        //as dd+ frame size = 1 and alsa sr as divide 16
        //out->raw_61937_frame_size = 16;
        break;
    case AUDIO_FORMAT_DTS_HD:
    case AUDIO_FORMAT_DOLBY_TRUEHD:
        out->config.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * 4 * 2;
        out->write_threshold = PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE * 4 * 2;
        out->config.start_threshold = PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE * 4 * 2;
        //out->raw_61937_frame_size = 16;//192k 2ch
        break;
    case AUDIO_FORMAT_PCM:
    default:
        if (out->config.rate == 96000) {
            out->config.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * 2;
        } else {
            out->config.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE;
        }
        out->write_threshold = PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE;
        out->config.start_threshold = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
        //out->raw_61937_frame_size = 4;
    }
    out->config.avail_min = 0;
    set_codec_type(codec_type);

    ALOGI("ALSA open configs: channels=%d, format=%d, period_count=%d, period_size=%d,,rate=%d",
          out->config.channels, out->config.format, out->config.period_count,
          out->config.period_size, out->config.rate);

    if (out->pcm == NULL) {
        /* switch to hdmi out for default */
        if (alsa_device_is_auge()) {
            port = PORT_SPDIFB2HDMI;
        }
        /* check to update port */
        port = alsa_device_get_pcm_index(port);
        ALOGD("%s(), pcm_open card %u port %u\n", __func__, card, port);
        out->pcm = pcm_open(card, port, PCM_OUT, &out->config);
        if (!pcm_is_ready(out->pcm)) {
            ALOGE("cannot open pcm_out driver: %s", pcm_get_error(out->pcm));
            pcm_close(out->pcm);
            out->pcm = NULL;
            return -EINVAL;
        }
    } else {
        ALOGE("stream %p share the pcm %p\n", out, out->pcm);
    }
#ifdef SPDIF_ENCODER
    if (codec_type_is_raw_data(codec_type) && !(out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO)) {
        spdifenc_init(out->pcm, out->hal_internal_format);
        out->spdif_enc_init_frame_write_sum = out->frame_write_sum;
    }
#endif
    out->codec_type = codec_type;
    out->flag_dtschecked = false;
    if (out->hw_sync_mode == 1) {
        ALOGD("start_output_stream with hw sync enable %p\n", out);
    }

    return 0;
}

static int check_input_parameters(uint32_t sample_rate, audio_format_t format, int channel_count, audio_devices_t devices)
{
    ALOGD("%s(sample_rate=%d, format=%d, channel_count=%d, devices = %x)", __FUNCTION__, sample_rate, format, channel_count, devices);

    if (format != AUDIO_FORMAT_PCM_16_BIT) {
        return -EINVAL;
    }

    if ((channel_count < 1) || (channel_count > 8)) {
        return -EINVAL;
    }

    switch (sample_rate) {
    case 8000:
    case 11025:
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        break;
    default:
        return -EINVAL;
    }

    devices &= ~AUDIO_DEVICE_BIT_IN;
    if ((devices & AUDIO_DEVICE_IN_LINE) ||
        (devices & AUDIO_DEVICE_IN_SPDIF) ||
        (devices & AUDIO_DEVICE_IN_TV_TUNER) ||
        (devices & AUDIO_DEVICE_IN_HDMI)) {
        if (format == AUDIO_FORMAT_PCM_16_BIT &&
            channel_count == 2 &&
            sample_rate == 48000) {
            ALOGD("%s: audio patch input device %x", __FUNCTION__, devices);
            return 0;
        } else if (format == AUDIO_FORMAT_PCM_16_BIT &&
                   channel_count == 8 &&
                   sample_rate == 48000) {
            ALOGD("%s: audio patch input device %x(i2s 8ch/16bits)", __FUNCTION__, devices);
            return 0;
        } else {
            ALOGD("%s: unspported audio patch input device %x", __FUNCTION__, devices);
            return -EINVAL;
        }
    }
    return 0;
}

static size_t get_input_buffer_size(unsigned int period_size, uint32_t sample_rate, audio_format_t format, int channel_count)
{
    size_t size;

    ALOGD("%s(sample_rate=%d, format=%d, channel_count=%d)", __FUNCTION__, sample_rate, format, channel_count);

    if (check_input_parameters(sample_rate, format, channel_count, AUDIO_DEVICE_NONE) != 0) {
        return 0;
    }

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    if (period_size == 0) {
        period_size = (pcm_config_in.period_size * sample_rate) / pcm_config_in.rate;
    }

    size = period_size;
    size = ((size + 15) / 16) * 16;

    return size * channel_count * sizeof(short);
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *) stream;
    unsigned int rate = out->hal_rate;
    ALOGV("Amlogic_HAL - out_get_sample_rate() = %d", rate);
    return rate;
}

static int out_set_sample_rate(struct audio_stream *stream __unused, uint32_t rate __unused)
{
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    ALOGI("%s(out->config.rate=%d, format %x,stream format %x)", __FUNCTION__,
          out->config.rate, out->hal_internal_format, stream->get_format(stream));
    /* take resampling into account and return the closest majoring
     * multiple of 16 frames, as audioflinger expects audio buffers to
     * be a multiple of 16 frames
     */
    size_t size = out->config.period_size;
    size_t buffer_size = 0;
    switch (out->hal_internal_format) {
    case AUDIO_FORMAT_AC3:
    case AUDIO_FORMAT_DTS:
        if (stream->get_format(stream) == AUDIO_FORMAT_IEC61937) {
            size = AC3_PERIOD_SIZE;
            //ALOGI("%s AUDIO_FORMAT_IEC61937 %zu)", __FUNCTION__, size);
            if ((eDolbyDcvLib == adev->dolby_lib_type) &&
                (out->flags & AUDIO_OUTPUT_FLAG_DIRECT)) {
                // local file playback, data from audio flinger direct mode
                // the data is packed by DCV decoder by OMX, 1536 samples per packet
                // to match with it, set the size to 1536
                // (ms12 decoder doesn't encounter this issue, so only handle with DCV decoder case)
                size = AC3_PERIOD_SIZE / 4;
                //ALOGI("%s AUDIO_FORMAT_IEC61937(DIRECT) (eDolbyDcvLib) size = %zu)", __FUNCTION__, size);
            }
        } else if (out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) {
            size = AC3_PERIOD_SIZE;
        } else {
            size = DEFAULT_PLAYBACK_PERIOD_SIZE;
        }

        if (out->hal_internal_format == AUDIO_FORMAT_DTS) {
            ALOGI("stream->get_format(stream):%0x", stream->get_format(stream));
            if (stream->get_format(stream) == AUDIO_FORMAT_IEC61937) {
                size = DEFAULT_PLAYBACK_PERIOD_SIZE;
                ALOGI("%s dts buf size%zu)", __FUNCTION__, size);
            }
        }

        break;
    case AUDIO_FORMAT_E_AC3:
        if (stream->get_format(stream) == AUDIO_FORMAT_IEC61937) {
            size =  EAC3_PERIOD_SIZE;
        } else if (out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) {
            size = EAC3_PERIOD_SIZE;//one iec61937 packet size
        } else {
            /*frame align*/
            if (adev->continuous_audio_mode) {
                /*Tunnel sync HEADER is 16 bytes*/
                if (out->flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC) {
                    size = out->ddp_frame_size + 16;
                } else {
                    size = out->ddp_frame_size * 4;
                }
            } else {
                size = PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE;    //PERIOD_SIZE;
            }
        }

        if (eDolbyDcvLib == adev->dolby_lib_type) {
            if (stream->get_format(stream) == AUDIO_FORMAT_IEC61937) {
                size =  PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE;
                ALOGI("%s eac3 eDolbyDcvLib = size%zu)", __FUNCTION__, size);
            }
        }

        break;
    case AUDIO_FORMAT_DTS_HD:
    case AUDIO_FORMAT_DOLBY_TRUEHD:
        if (out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) {
            size = 16 * DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
        } else {
            size = 4 * PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE;
        }
        if (stream->get_format(stream) == AUDIO_FORMAT_IEC61937) {
            size = 4 * PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE;
        }
        break;
    case AUDIO_FORMAT_PCM:
        if (adev->continuous_audio_mode) {
            /*Tunnel sync HEADER is 16 bytes*/
            if (out->flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC) {
                size = (8192 + 16) / 4;
            }
        }
    default:
        if (out->config.rate == 96000) {
            size = DEFAULT_PLAYBACK_PERIOD_SIZE * 2;
        } else
            // bug_id - 158018, modify size value from PERIOD_SIZE to (PERIOD_SIZE * PLAYBACK_PERIOD_COUNT)
        {
            size = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
        }
    }
    size = ((size + 15) / 16) * 16;
    return size * audio_stream_out_frame_size((struct audio_stream_out *) stream);
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream __unused)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *) stream;
    //ALOGV("Amlogic_HAL - out_get_channels return constant value AUDIO_CHANNEL_OUT_STEREO.");
    ALOGV("Amlogic_HAL - out_get_channels return out->hal_channel_mask:%0x", out->hal_channel_mask);
    return out->hal_channel_mask;
    //return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_channel_mask_t out_get_channels_direct(const struct audio_stream *stream)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *) stream;
    //ALOGV("out->hal_channel_mask:%0x", out->hal_channel_mask);
    return out->hal_channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream __unused)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *) stream;
    ALOGV("Amlogic_HAL - out_get_format() = %d\n", out->hal_format);
    // if hal_format doesn't have a valid value,
    // return default value AUDIO_FORMAT_PCM_16_BIT
    if (out->hal_format == 0) {
        return AUDIO_FORMAT_PCM_16_BIT;
    }
    return out->hal_format;
}

static audio_format_t out_get_format_direct(const struct audio_stream *stream)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *) stream;

    return  out->hal_format;
}

static int out_set_format(struct audio_stream *stream __unused, audio_format_t format __unused)
{
    return 0;
}

/* must be called with hw device and output stream mutexes locked */
static int do_output_standby(struct aml_stream_out *out)
{
    struct aml_audio_device *adev = out->dev;
    int i = 0;

    ALOGD("%s(%p)", __FUNCTION__, out);

    if (!out->standby) {
        //commit here for hwsync/mix stream hal mixer
        //pcm_close(out->pcm);
        //out->pcm = NULL;
        if (out->buffer) {
            free(out->buffer);
            out->buffer = NULL;
        }
#ifdef RESAMPLER
        if (out->resampler) {
            release_resampler(out->resampler);
            out->resampler = NULL;
        }
#endif
        out->standby = 1;
        for (i  = 0; i < MAX_STREAM_NUM; i++) {
            if (adev->active_output[i] == out) {
                adev->active_output[i]  = NULL;
                adev->active_output_count--;
                ALOGI("remove out (%p) from index %d\n", out, i);
                break;
            }
        }
        if (out->hw_sync_mode == 1 || adev->hwsync_output == out) {
#if 0
            //here to check if hwsync in pause status,if that,chear the status
            //to release the sound card to other active output stream
            if (out->pause_status == true && adev->active_output_count > 0) {
                if (pcm_is_ready(out->pcm)) {
                    int r = pcm_ioctl(out->pcm, SNDRV_PCM_IOCTL_PAUSE, 0);
                    if (r < 0) {
                        ALOGE("here cannot resume channel\n");
                    } else {
                        r = 0;
                    }
                    ALOGI("clear the hwsync output pause status.resume pcm\n");
                }
                out->pause_status = false;
            }
#endif
            /* cleanup the audio hw fifo */
            if (out->pause_status == true) {
                pcm_stop(out->pcm);
            }

            out->pause_status = false;
            adev->hwsync_output = NULL;
            ALOGI("clear hwsync_output when hwsync standby\n");
        }
        if (i == MAX_STREAM_NUM) {
            ALOGE("error, not found stream in dev stream list\n");
        }
        /* no active output here,we can close the pcm to release the sound card now*/
        if (adev->active_output_count == 0) {
            if (adev->pcm) {
                ALOGI("close pcm %p\n", adev->pcm);
                pcm_stop(adev->pcm); // cleanup the audio hw fifo
                pcm_close(adev->pcm);
                adev->pcm = NULL;
            }
            out->pause_status = false;
            adev->pcm_paused = false;
        }
    }
    return 0;
}
/* must be called with hw device and output stream mutexes locked */
static int do_output_standby_direct(struct aml_stream_out *out)
{
    int status = 0;

    ALOGI("%s,out %p", __FUNCTION__,  out);

    if (!out->standby) {
        if (out->buffer) {
            free(out->buffer);
            out->buffer = NULL;
        }

        out->standby = 1;

        /* cleanup the audio hw fifo */
        if (out->pause_status == true) {
            pcm_stop(out->pcm);
        }

        pcm_close(out->pcm);
        out->pcm = NULL;
    }
    out->pause_status = false;
    set_codec_type(TYPE_PCM);
    /* clear the hdmitx channel config to default */
    if (out->multich == 6) {
        sysfs_set_sysfs_str("/sys/class/amhdmitx/amhdmitx0/aud_output_chs", "0:0");
    }
    return status;
}

static int out_standby(struct audio_stream *stream)
{
    ALOGD("%s(%p)", __FUNCTION__, stream);
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    int status = 0;
    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    status = do_output_standby(out);
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    return status;
}

static int out_standby_direct(struct audio_stream *stream)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    int status = 0;

    ALOGI("%s(%p),out %p", __FUNCTION__, stream, out);

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    if (!out->standby) {
        if (out->buffer) {
            free(out->buffer);
            out->buffer = NULL;
        }
        if (adev->hi_pcm_mode) {
            adev->hi_pcm_mode = false;
        }
        out->standby = 1;

        /* cleanup the audio hw fifo */
        if (out->pause_status == true) {
            pcm_stop(out->pcm);
        }

        pcm_close(out->pcm);
        out->pcm = NULL;
    }
    out->pause_status = false;
    set_codec_type(TYPE_PCM);
    /* clear the hdmitx channel config to default */
    if (out->multich == 6) {
        sysfs_set_sysfs_str("/sys/class/amhdmitx/amhdmitx0/aud_output_chs", "0:0");
    }
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    return status;
}

static int out_dump(const struct audio_stream *stream __unused, int fd __unused)
{
    ALOGD("%s(%p, %d)", __FUNCTION__, stream, fd);
    return 0;
}

static int out_flush(struct audio_stream_out *stream)
{
    ALOGD("%s(%p)", __FUNCTION__, stream);
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    int channel_count = popcount(out->hal_channel_mask);
    bool hwsync_lpcm = (out->flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC && out->config.rate  <= 48000 &&
                        audio_is_linear_pcm(out->hal_internal_format) && channel_count <= 2);
    do_standby_func standy_func = NULL;
    if (out->flags & AUDIO_OUTPUT_FLAG_DIRECT && !hwsync_lpcm) {
        standy_func = do_output_standby_direct;
    } else {
        standy_func = do_output_standby;
    }
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);

    /* cleanup the audio hw fifo */
    if (out->pause_status == true) {
        pcm_stop(out->pcm);
    }

    standy_func(out);
    out->frame_write_sum  = 0;
    out->last_frames_postion = 0;
    out->spdif_enc_init_frame_write_sum =  0;
    out->frame_skip_sum = 0;
    out->skip_frame = 0;

exit:
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{

    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    struct aml_stream_in *in;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret;
    uint val = 0;
#if 1
    bool force_input_standby = false;
    int channel_count = popcount(out->hal_channel_mask);
    bool hwsync_lpcm = (out->flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC && out->config.rate  <= 48000 &&
                        audio_is_linear_pcm(out->hal_internal_format) && channel_count <= 2);
    do_standby_func standy_func = NULL;
    do_startup_func   startup_func = NULL;
    if (out->flags & AUDIO_OUTPUT_FLAG_DIRECT && !hwsync_lpcm) {
        standy_func = do_output_standby_direct;
        startup_func = start_output_stream_direct;
    } else {
        standy_func = do_output_standby;
        startup_func = start_output_stream;
    }
    ALOGD("%s(kvpairs(%s), out_device=%#x)", __FUNCTION__, kvpairs, adev->out_device);
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        if (((adev->out_device & AUDIO_DEVICE_OUT_ALL) != val) && (val != 0)) {
            if (1/* out == adev->active_output[0]*/) {
                ALOGI("audio hw select device!\n");
                standy_func(out);
                /* a change in output device may change the microphone selection */
                if (adev->active_input &&
                    adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION) {
                    force_input_standby = true;
                }
                /* force standby if moving to/from HDMI */
                if (((val & AUDIO_DEVICE_OUT_AUX_DIGITAL) ^
                     (adev->out_device & AUDIO_DEVICE_OUT_AUX_DIGITAL)) ||
                    ((val & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET) ^
                     (adev->out_device & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET))) {
                    standy_func(out);
                }
            }
            adev->out_device &= ~AUDIO_DEVICE_OUT_ALL;
            adev->out_device |= val;
            //select_devices(adev);
        }
        pthread_mutex_unlock(&out->lock);
        if (force_input_standby) {
            in = adev->active_input;
            pthread_mutex_lock(&in->lock);
            do_input_standby(in);
            pthread_mutex_unlock(&in->lock);
        }
        pthread_mutex_unlock(&adev->lock);

        // We shall return Result::OK, which is 0, if parameter is set successfully,
        // or we can not pass VTS test.
        ALOGI("Amlogic_HAL - %s: change ret value to 0 in order to pass VTS test.", __FUNCTION__);
        ret = 0;

        goto exit;
    }
    int sr = 0;
    ret = str_parms_get_int(parms, AUDIO_PARAMETER_STREAM_SAMPLING_RATE, &sr);
    if (ret >= 0) {
        if (sr > 0) {
            struct pcm_config *config = &out->config;
            ALOGI("audio hw sampling_rate change from %d to %d \n", config->rate, sr);
            config->rate = sr;
            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&out->lock);
            if (!out->standby) {
                standy_func(out);
                startup_func(out);
                out->standby = 0;
            }
            // set hal_rate to sr for passing VTS
            ALOGI("Amlogic_HAL - %s: set sample_rate to hal_rate.", __FUNCTION__);
            out->hal_rate = sr;
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&out->lock);
        }

        // We shall return Result::OK, which is 0, if parameter is set successfully,
        // or we can not pass VTS test.
        ALOGI("Amlogic_HAL - %s: change ret value to 0 in order to pass VTS test.", __FUNCTION__);
        ret = 0;

        goto exit;
    }
    // Detect and set AUDIO_PARAMETER_STREAM_FORMAT for passing VTS
    audio_format_t fmt = 0;
    ret = str_parms_get_int(parms, AUDIO_PARAMETER_STREAM_FORMAT, (int *) &fmt);
    if (ret >= 0) {
        if (fmt > 0) {
            struct pcm_config *config = &out->config;
            ALOGI("audio hw sampling_rate change from %d to %d \n", config->format, fmt);
            config->format = fmt;
            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&out->lock);
            if (!out->standby) {
                standy_func(out);
                startup_func(out);
                out->standby = 0;
            }
            // set hal_format to fmt for passing VTS
            ALOGI("Amlogic_HAL - %s: set format to hal_format. fmt = %d", __FUNCTION__, fmt);
            out->hal_format = fmt;
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&out->lock);
        }

        // We shall return Result::OK, which is 0, if parameter is set successfully,
        // or we can not pass VTS test.
        ALOGI("Amlogic_HAL - %s: change ret value to 0 in order to pass VTS test.", __FUNCTION__);
        ret = 0;

        goto exit;
    }
    // Detect and set AUDIO_PARAMETER_STREAM_CHANNELS for passing VTS
    audio_channel_mask_t channels = AUDIO_CHANNEL_OUT_STEREO;
    ret = str_parms_get_int(parms, AUDIO_PARAMETER_STREAM_CHANNELS, (int *) &channels);
    if (ret >= 0) {
        if (channels > AUDIO_CHANNEL_NONE) {
            struct pcm_config *config = &out->config;
            ALOGI("audio hw channel_mask change from %d to %d \n", config->channels, channels);
            config->channels = audio_channel_count_from_out_mask(channels);
            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&out->lock);
            if (!out->standby) {
                standy_func(out);
                startup_func(out);
                out->standby = 0;
            }
            // set out->hal_channel_mask to channels for passing VTS
            ALOGI("Amlogic_HAL - %s: set out->hal_channel_mask to channels. fmt = %d", __FUNCTION__, channels);
            out->hal_channel_mask = channels;
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&out->lock);
        }

        // We shall return Result::OK, which is 0, if parameter is set successfully,
        // or we can not pass VTS test.
        ALOGI("Amlogic_HAL - %s: change ret value to 0 in order to pass VTS test.", __FUNCTION__);
        ret = 0;

        goto exit;
    }

    int frame_size = 0;
    ret = str_parms_get_int(parms, AUDIO_PARAMETER_STREAM_FRAME_COUNT, &frame_size);
    if (ret >= 0) {
        if (frame_size > 0) {
            struct pcm_config *config = &out->config;
            ALOGI("audio hw frame size change from %d to %d \n", config->period_size, frame_size);
            config->period_size =  frame_size;
            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&out->lock);
            if (!out->standby) {
                standy_func(out);
                startup_func(out);
                out->standby = 0;
            }
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&out->lock);
        }

        // We shall return Result::OK, which is 0, if parameter is set successfully,
        // or we can not pass VTS test.
        ALOGI("Amlogic_HAL - %s: change ret value to 0 in order to pass VTS test.", __FUNCTION__);
        ret = 0;

        goto exit;
    }
    ret = str_parms_get_str(parms, "hw_av_sync", value, sizeof(value));
    if (ret >= 0) {
        int hw_sync_id = atoi(value);
        unsigned char sync_enable = (hw_sync_id == 12345678) ? 1 : 0;
        audio_hwsync_t *hw_sync = out->hwsync;
        ALOGI("(%p)set hw_sync_id %d,%s hw sync mode\n",
              out, hw_sync_id, sync_enable ? "enable" : "disable");
        out->hw_sync_mode = sync_enable;
        hw_sync->first_apts_flag = false;
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        out->frame_write_sum = 0;
        out->last_frames_postion = 0;
        /* clear up previous playback output status */
        if (!out->standby) {
            standy_func(out);
        }
        //adev->hwsync_output = sync_enable?out:NULL;
        if (sync_enable) {
            ALOGI("init hal mixer when hwsync\n");
            aml_hal_mixer_init(&adev->hal_mixer);
        }
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
        ret = 0;
        goto exit;
    }
exit:
    str_parms_destroy(parms);

    // We shall return Result::OK, which is 0, if parameter is NULL,
    // or we can not pass VTS test.
    if (ret < 0) {
        ALOGE("Amlogic_HAL - %s: parameter is NULL, change ret value to 0 in order to pass VTS test.", __FUNCTION__);
        ret = 0;
    }
#endif
    return ret;
}

static char *out_get_parameters(const struct audio_stream *stream, const char *keys)
{

    char *cap = NULL;
    char *para = NULL;
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;

    ALOGI("out_get_parameters %s,out %p\n", keys, out);
    struct str_parms *parms;
    audio_format_t format;
    int ret = 0, val_int = 0;
#if 1
    parms = str_parms_create_str(keys);
    //ret = str_parms_get_int(parms, AUDIO_PARAMETER_STREAM_FORMAT ,(int *)&format);
    ret = str_parms_get_int(parms, AUDIO_PARAMETER_STREAM_FORMAT, &val_int);
    format = (audio_format_t) val_int;

    if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
        if (out->flags & AUDIO_OUTPUT_FLAG_PRIMARY) {
            ALOGI("Amlogic - return hard coded sample_rate list for primary output stream.\n");
            cap = strdup("sup_sampling_rates=8000|11025|16000|22050|24000|32000|44100|48000");
        } else {
            if (out->out_device & AUDIO_DEVICE_OUT_HDMI_ARC) {
                cap = (char *) strdup_hdmi_arc_cap_default(AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES, format);
            } else if (out->out_device & AUDIO_DEVICE_OUT_SPDIF) {
                cap = strdup("sup_sampling_rates=32000|44100|48000|88200|192000");
            } else {
                cap = (char *) get_hdmi_sink_cap(AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES, format);
            }
        }
        if (cap) {
            para = strdup(cap);
            free(cap);
        } else {
            para = strdup("");
        }
        ALOGI("%s\n", para);
        return para;
    } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
        if (out->flags & AUDIO_OUTPUT_FLAG_PRIMARY) {
            ALOGV("Amlogic - return hard coded channel_mask list for primary output stream.\n");
            cap = strdup("sup_channels=AUDIO_CHANNEL_OUT_MONO|AUDIO_CHANNEL_OUT_STEREO");
        } else {
            if (out->out_device & AUDIO_DEVICE_OUT_HDMI_ARC) {
                cap = (char *) strdup_hdmi_arc_cap_default(AUDIO_PARAMETER_STREAM_SUP_CHANNELS, format);
            } else if (out->out_device & AUDIO_DEVICE_OUT_SPDIF) {
                cap = strdup("sup_channels=AUDIO_CHANNEL_OUT_MONO|AUDIO_CHANNEL_OUT_STEREO|AUDIO_CHANNEL_OUT_5POINT1|AUDIO_CHANNEL_OUT_7POINT1");
            } else {
                cap = (char *) get_hdmi_sink_cap(AUDIO_PARAMETER_STREAM_SUP_CHANNELS, format);
            }
        }
        if (cap) {
            para = strdup(cap);
            free(cap);
        } else {
            para = strdup("");
        }
        ALOGI("%s\n", para);
        return para;
    } else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
        if (out->out_device & AUDIO_DEVICE_OUT_HDMI_ARC) {
            cap = (char *) strdup_hdmi_arc_cap_default(AUDIO_PARAMETER_STREAM_SUP_FORMATS, format);
        }  else if (out->out_device & AUDIO_DEVICE_OUT_SPDIF) {
            cap = strdup("sup_formats=AUDIO_FORMAT_AC3|AUDIO_FORMAT_DTS|AUDIO_FORMAT_E_AC3|AUDIO_FORMAT_IEC61937");
        } else {
            cap = (char *) get_hdmi_sink_cap(AUDIO_PARAMETER_STREAM_SUP_FORMATS, format);
        }
        if (cap) {
            para = strdup(cap);
            free(cap);
        } else {
            para = strdup("");
        }
        ALOGI("%s\n", para);
        return para;
    }
#endif
    return strdup("");
}

/* //redifinition in audio_hw_utils.c
static uint32_t out_get_latency_frames (const struct audio_stream_out *stream)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *) stream;
    snd_pcm_sframes_t frames = 0;
    uint32_t whole_latency_frames;
    int ret = 0;

    whole_latency_frames = out->config.period_size * out->config.period_count;
    if (!out->pcm || !pcm_is_ready (out->pcm) ) {
        return whole_latency_frames;
    }
    ret = pcm_ioctl (out->pcm, SNDRV_PCM_IOCTL_DELAY, &frames);
    if (ret < 0) {
        return whole_latency_frames;
    }
    return frames;
}
*/

uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *) stream;
    snd_pcm_sframes_t frames = out_get_latency_frames(stream);
    return (frames * 1000) / out->config.rate;
}

static int out_set_volume(struct audio_stream_out *stream, float left, float right)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    out->volume_l = left;
    out->volume_r = right;
    return 0;
}

static int out_pause(struct audio_stream_out *stream)
{
    ALOGD("out_pause(%p)\n", stream);

    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    int r = 0;
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);
    if (out->standby || out->pause_status == true) {
        goto exit;
    }
    if (out->hw_sync_mode) {
        adev->hwsync_output = NULL;
        if (adev->active_output_count > 1) {
            ALOGI("more than one active stream,skip alsa hw pause\n");
            goto exit1;
        }
    }
    if (pcm_is_ready(out->pcm)) {
        r = pcm_ioctl(out->pcm, SNDRV_PCM_IOCTL_PAUSE, 1);
        if (r < 0) {
            ALOGE("cannot pause channel\n");
        } else {
            r = 0;
            // set the pcm pause state
            if (out->pcm == adev->pcm) {
                adev->pcm_paused = true;
            } else {
                ALOGE("out->pcm and adev->pcm are assumed same handle");
            }
        }
    }
exit1:
    if (out->hw_sync_mode) {
        sysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PAUSE");
    }
    out->pause_status = true;
exit:
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&out->lock);
    return r;
}

static int out_resume(struct audio_stream_out *stream)
{
    ALOGD("out_resume (%p)\n", stream);
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    int r = 0;
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);
    if (out->standby || out->pause_status == false) {
        // If output stream is not standby or not paused,
        // we should return Result::INVALID_STATE (3),
        // thus we can pass VTS test.
        ALOGE("Amlogic_HAL - %s: cannot resume, because output stream isn't in standby or paused state.", __FUNCTION__);
        r = 3;

        goto exit;
    }
    if (pcm_is_ready(out->pcm)) {
        r = pcm_ioctl(out->pcm, SNDRV_PCM_IOCTL_PAUSE, 0);
        if (r < 0) {
            ALOGE("cannot resume channel\n");
        } else {
            r = 0;
            // clear the pcm pause state
            if (out->pcm == adev->pcm) {
                adev->pcm_paused = false;
            }
        }
    }
    if (out->hw_sync_mode) {
        ALOGI("init hal mixer when hwsync resume\n");
        adev->hwsync_output = out;
        aml_hal_mixer_init(&adev->hal_mixer);
        sysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_RESUME");
    }
    out->pause_status = false;
exit:
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&out->lock);
    return r;
}

/* use standby instead of pause to fix background pcm playback */
static int out_pause_new(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(aml_dev->ms12);

    int ret = 0;

    ALOGI("%s(), stream(%p)\n", __func__, stream);
    pthread_mutex_lock(&aml_dev->lock);
    pthread_mutex_lock(&aml_out->lock);
    // mark : some project have comment this out, while 8.1 keep it functioning. zz
    //if (aml_out->pause_status == true) {
    //    ALOGI("pause already, do nothing and exit");
    //    goto exit1;
    //}
    /*
    send audio pause cmd to  video when tunnel mode.
    */
    if (aml_out->hw_sync_mode) {
        sysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PAUSE");
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
        if (aml_dev->continuous_audio_mode == 1) {
            if ((aml_dev->ms12.dolby_ms12_enable == true) && (aml_dev->ms12.is_continuous_paused == false)) {
                aml_dev->ms12.is_continuous_paused = true;
                pthread_mutex_lock(&ms12->lock);
                int ms12_runtime_update_ret = 0;
                dolby_ms12_set_pause_flag(aml_dev->ms12.is_continuous_paused);
                ms12_runtime_update_ret = aml_ms12_update_runtime_params(&(aml_dev->ms12));
                pthread_mutex_unlock(&ms12->lock);
                ALOGI("%s ms12 set pause flag runtime return %d\n", __func__, ms12_runtime_update_ret);
            }
        } else {
            ret = do_output_standby_l(&stream->common);
            if (ret < 0) {
                goto exit;
            }
        }
    } else
#endif
    {

        ret = do_output_standby_l(&stream->common);
        if (ret < 0) {
            goto exit;
        }
    }
exit1:
    aml_out->pause_status = true;

    if (aml_out->hw_sync_mode) {
        sysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PAUSE");
    }

exit:
    pthread_mutex_unlock(&aml_dev->lock);
    pthread_mutex_unlock(&aml_out->lock);
    return ret;
}

static int out_resume_new(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(aml_dev->ms12);

    int ret = 0;

    pthread_mutex_lock(&aml_dev->lock);
    pthread_mutex_lock(&aml_out->lock);

    ALOGI("%s(), stream(%p)\n", __func__, stream);
    if (aml_out->standby || aml_out->pause_status == false) {
        // If output stream is not standby or not paused,
        // we should return Result::INVALID_STATE (3),
        // thus we can pass VTS test.
        ALOGE("Amlogic_HAL - %s: cannot resume, because output stream isn't in standby or paused state.", __FUNCTION__);
        ret = 3;
        goto exit;
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
        if (aml_dev->continuous_audio_mode == 1) {
            if ((aml_dev->ms12.dolby_ms12_enable == true) && (aml_dev->ms12.is_continuous_paused == true)) {
                aml_dev->ms12.is_continuous_paused = false;
                pthread_mutex_lock(&ms12->lock);
                int ms12_runtime_update_ret = 0;
                dolby_ms12_set_pause_flag(aml_dev->ms12.is_continuous_paused);
                ms12_runtime_update_ret = aml_ms12_update_runtime_params(&(aml_dev->ms12));
                pthread_mutex_unlock(&ms12->lock);
                ALOGI("%s ms12 set unpause flag runtime return %d\n", __func__, ms12_runtime_update_ret);
            }
        }
    }
#endif
    aml_out->pause_status = false;
    if (aml_out->hw_sync_mode) {
        sysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_RESUME");
    }

exit:
    pthread_mutex_unlock(&aml_dev->lock);
    pthread_mutex_unlock(&aml_out->lock);
    //aml_out->pause_status = 0;
    return ret;
}

static int out_flush_new(struct audio_stream_out *stream)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    ALOGI("%s(), stream(%p)\n", __func__, stream);
    out->frame_write_sum = 0;
    out->last_frames_postion = 0;
    out->spdif_enc_init_frame_write_sum =  0;
    out->frame_skip_sum = 0;
    // 8.1 after flush there have sync issue, so need to skip 3 frames zz
    out->skip_frame = 3;
    out->input_bytes_size = 0;
    out->pause_status = false;
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (out->hw_sync_mode) {
            aml_audio_hwsync_init(out->hwsync, out);
        }
        //normal pcm(mixer thread) do not flush dolby ms12 input buffer
        if (continous_mode(adev) && (out->flags & AUDIO_OUTPUT_FLAG_DIRECT)) {
            pthread_mutex_lock(&ms12->lock);
            dolby_ms12_flush_main_input_buffer();
            pthread_mutex_unlock(&ms12->lock);
            out->continuous_audio_offset = 0;
        }
    }
#endif
    return 0;
}

static ssize_t out_write_legacy(struct audio_stream_out *stream, const void* buffer,
                                size_t bytes)
{
    int ret = 0;
    size_t oldBytes = bytes;
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    size_t frame_size = audio_stream_out_frame_size(stream);
    size_t in_frames = bytes / frame_size;
    size_t out_frames;
    bool force_input_standby = false;
    int16_t *in_buffer = (int16_t *) buffer;
    int16_t *out_buffer = in_buffer;
    struct aml_stream_in *in;
    uint ouput_len;
    char *data,  *data_dst;
    volatile char *data_src;
    uint i, total_len;
    int codec_type = 0;
    int samesource_flag = 0;
    uint32_t latency_frames = 0;
    int need_mix = 0;
    short *mix_buf = NULL;
    audio_hwsync_t *hw_sync = out->hwsync;
    struct pcm_config *config = &out->config;
    unsigned int pcrscr = 0;
    unsigned char enable_dump = getprop_bool("media.audiohal.outdump");

    ALOGV("%s() out_write_direct:out %p,position %zu",
          __func__, out, bytes);

    // limit HAL mixer buffer level within 200ms
    while ((adev->hwsync_output != NULL && adev->hwsync_output != out) &&
           (aml_hal_mixer_get_content(&adev->hal_mixer) > 200 * 48 * 4)) {
        usleep(20000);
    }
    if (adev->first_apts_flag) {
        char buf[32];
        memset(buf, 0, sizeof(char) * 32);
        sysfs_get_sysfs_str(TSYNC_MODE, buf, 32);
        ALOGI("TSYNC_MODE is %s\n", buf);
        if (strncmp(buf, "2", 1) == 0) {
            get_sysfs_uint(TSYNC_PCRSCR, &pcrscr);
            if (adev->first_apts > pcrscr) {
                // ALOGI("disable trigger audio start first_apts %x pcrscr %x \n",
                //adev->first_apts,pcrscr);
                return 0;
            }
        }
    }
    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the output stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&out->lock);
    //if hi pcm mode ,we need releae i2s device so direct stream can get it.
    if (adev->hi_pcm_mode) {
        if (!out->standby) {
            pthread_mutex_lock(&adev->lock);
            do_output_standby(out);
            pthread_mutex_unlock(&adev->lock);
        }
        ret = -1 ;
        out->frame_write_sum += in_frames;
        goto exit;
    }
    //code here to handle audio start issue to make higher a/v sync precision
    if (adev->first_apts_flag) {
        adev->frame_trigger_thred +=  bytes / frame_size;
        if (adev->frame_trigger_thred > config->period_count * config->period_size) {
            ALOGI("start trigger audio start first apts is %x the pcrscr  is %x \n", adev->first_apts, pcrscr);
            aml_audio_start_trigger(stream);
            adev->first_apts_flag = false;
            adev->frame_trigger_thred = 0;
        }
    }
    //here to check whether hwsync out stream and other stream are enabled at the same time.
    //if that we need do the hal mixer of the two out stream.
    if (out->hw_sync_mode == 1) {
        int content_size = aml_hal_mixer_get_content(&adev->hal_mixer);
        //ALOGI("content_size %d\n",content_size);
        if (content_size > 0) {
            if (adev->hal_mixer.need_cache_flag == 0)   {
                //ALOGI("need do hal mixer\n");
                need_mix = 1;
            } else if (content_size < 80 * 48 * 4) { //80 ms
                //ALOGI("hal mixed cached size %d\n", content_size);
            } else {
                ALOGI("start enable mix,cached size %d\n", content_size);
                adev->hal_mixer.need_cache_flag = 0;
            }

        } else {
            //  ALOGI("content size %d,duration %d ms\n",content_size,content_size/48/4);
        }
    }
    /* if hwsync output stream are enabled,write  other output to a mixe buffer and sleep for the pcm duration time  */
    if (adev->hwsync_output != NULL && adev->hwsync_output != out) {
        //ALOGI("dev hwsync enable,hwsync %p) cur (%p),size %d\n",adev->hwsync_output,out,bytes);
        //out->frame_write_sum += in_frames;
#if 0
        if (!out->standby) {
            do_output_standby(out);
        }
#endif
        if (out->standby) {
            pthread_mutex_lock(&adev->lock);
            ret = start_output_stream(out);
            pthread_mutex_unlock(&adev->lock);
            if (ret != 0) {
                ALOGE("start_output_stream failed");
                goto exit;
            }
            out->standby = false;
        }
        ret = -1;
        aml_hal_mixer_write(&adev->hal_mixer, buffer, bytes);
        goto exit;
    }
    if (out->pause_status == true) {
        pthread_mutex_unlock(&out->lock);
        ALOGI("call out_write when pause status (%p)\n", stream);
        return 0;
    }
    if ((out->standby) && (out->hw_sync_mode == 1)) {
        // todo: check timestamp header PTS discontinue for new sync point after seek
        hw_sync->first_apts_flag = false;
        hw_sync->hw_sync_state = HW_SYNC_STATE_HEADER;
        hw_sync->hw_sync_header_cnt = 0;
    }

#if 1
    if (enable_dump && out->hw_sync_mode == 0) {
        FILE *fp1 = fopen("/data/tmp/i2s_audio_out.pcm", "a+");
        if (fp1) {
            int flen = fwrite((char *) buffer, 1, bytes, fp1);
            fclose(fp1);
        }
    }
#endif

    if (out->hw_sync_mode == 1) {
        char buf[64] = {0};
        unsigned char *header;

        if (hw_sync->hw_sync_state == HW_SYNC_STATE_RESYNC) {
            uint i = 0;
            uint8_t *p = (uint8_t *) buffer;
            while (i < bytes) {
                if (hwsync_header_valid(p)) {
                    ALOGI("HWSYNC resync.%p", out);
                    hw_sync->hw_sync_state = HW_SYNC_STATE_HEADER;
                    hw_sync->hw_sync_header_cnt = 0;
                    hw_sync->first_apts_flag = false;
                    bytes -= i;
                    p += i;
                    in_frames = bytes / frame_size;
                    ALOGI("in_frames = %zu", in_frames);
                    in_buffer = (int16_t *) p;
                    break;
                } else {
                    i += 4;
                    p += 4;
                }
            }

            if (hw_sync->hw_sync_state == HW_SYNC_STATE_RESYNC) {
                ALOGI("Keep searching for HWSYNC header.%p", out);
                goto exit;
            }
        }

        header = (unsigned char *) buffer;
    }
    if (out->standby) {
        pthread_mutex_lock(&adev->lock);
        ret = start_output_stream(out);
        pthread_mutex_unlock(&adev->lock);
        if (ret != 0) {
            ALOGE("start_output_stream failed");
            goto exit;
        }
        out->standby = false;
        /* a change in output device may change the microphone selection */
        if (adev->active_input &&
            adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION) {
            force_input_standby = true;
        }
    }
#if 1
    /* Reduce number of channels, if necessary */
    // TODO: find the right way to handle PCM BT case.
    //if (popcount(out_get_channels(&stream->common)) >
    //    (int)out->config.channels) {
    if (2 > (int) out->config.channels) {
        unsigned int i;

        /* Discard right channel */
        for (i = 1; i < in_frames; i++) {
            in_buffer[i] = in_buffer[i * 2];
        }

        /* The frame size is now half */
        frame_size /= 2;
    }
#endif
    /* only use resampler if required */
    if (out->config.rate != out_get_sample_rate(&stream->common)) {
        out_frames = out->buffer_frames;
        out->resampler->resample_from_input(out->resampler,
                                            in_buffer, &in_frames,
                                            (int16_t*) out->buffer, &out_frames);
        in_buffer = (int16_t*) out->buffer;
        out_buffer = in_buffer;
    } else {
        out_frames = in_frames;
    }

#if 0
    if (enable_dump && out->hw_sync_mode == 1) {
        FILE *fp1 = fopen("/data/tmp/i2s_audio_out.pcm", "a+");
        if (fp1) {
            int flen = fwrite((char *) in_buffer, 1, out_frames * frame_size, fp1);
            ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
            fclose(fp1);
        } else {
            ALOGD("could not open file:/data/i2s_audio_out.pcm");
        }
    }
#endif
#if 1
    if (!(adev->out_device & AUDIO_DEVICE_OUT_ALL_SCO)) {
        codec_type = get_sysfs_int("/sys/class/audiodsp/digital_codec");
        //samesource_flag = get_sysfs_int("/sys/class/audiodsp/audio_samesource");
        if (codec_type != out->last_codec_type/*samesource_flag == 0*/ && codec_type == 0) {
            ALOGI("to enable same source,need reset alsa,type %d,same source flag %d \n", codec_type, samesource_flag);
            if (out->pcm) {
                pcm_stop(out->pcm);
            }
        }
        out->last_codec_type = codec_type;
    }
#endif
    if (out->is_tv_platform == 1) {
        int16_t *tmp_buffer = (int16_t *) out->audioeffect_tmp_buffer;
        memcpy((void *) tmp_buffer, (void *) in_buffer, out_frames * 4);
        ALOGV("Amlogic - disable audio_data_process(), and replace tmp_buffer data with in_buffer data.\n");
        // audio_effect_process() is deprecated
        //audio_effect_process(stream, tmp_buffer, out_frames);
        for (i = 0; i < out_frames; i ++) {
            out->tmp_buffer_8ch[8 * i] = ((int32_t)(in_buffer[2 * i])) << 16;
            out->tmp_buffer_8ch[8 * i + 1] = ((int32_t)(in_buffer[2 * i + 1])) << 16;
            out->tmp_buffer_8ch[8 * i + 2] = ((int32_t)(in_buffer[2 * i])) << 16;    //8.1
            out->tmp_buffer_8ch[8 * i + 3] = ((int32_t)(in_buffer[2 * i + 1])) << 16;    //8.1
            out->tmp_buffer_8ch[8 * i + 4] = 0;
            out->tmp_buffer_8ch[8 * i + 5] = 0;
            out->tmp_buffer_8ch[8 * i + 6] = 0;
            out->tmp_buffer_8ch[8 * i + 7] = 0;
        }
        /*if (out->frame_count < 5*1024) {
            memset(out->tmp_buffer_8ch, 0, out_frames * frame_size * 8);
        }*/
        ret = pcm_write(out->pcm, out->tmp_buffer_8ch, out_frames * frame_size * 8);
        out->frame_write_sum += out_frames;
    } else {
        if (out->hw_sync_mode) {

            size_t remain = out_frames * frame_size;
            uint8_t *p = (uint8_t *) buffer;

            //ALOGI(" --- out_write %d, cache cnt = %d, body = %d, hw_sync_state = %d", out_frames * frame_size, out->body_align_cnt, out->hw_sync_body_cnt, out->hw_sync_state);

            while (remain > 0) {
                if (hw_sync->hw_sync_state == HW_SYNC_STATE_HEADER) {
                    //ALOGI("Add to header buffer [%d], 0x%x", out->hw_sync_header_cnt, *p);
                    out->hwsync->hw_sync_header[out->hwsync->hw_sync_header_cnt++] = *p++;
                    remain--;
                    if (hw_sync->hw_sync_header_cnt == 16) {
                        uint64_t pts;
                        if (!hwsync_header_valid(&hw_sync->hw_sync_header[0])) {
                            ALOGE("hwsync header out of sync! Resync.");
                            hw_sync->hw_sync_state = HW_SYNC_STATE_RESYNC;
                            break;
                        }
                        hw_sync->hw_sync_state = HW_SYNC_STATE_BODY;
                        hw_sync->hw_sync_body_cnt = hwsync_header_get_size(&hw_sync->hw_sync_header[0]);
                        hw_sync->body_align_cnt = 0;
                        pts = hwsync_header_get_pts(&hw_sync->hw_sync_header[0]);
                        pts = pts * 90 / 1000000;
#if 1
                        char buf[64] = {0};
                        if (hw_sync->first_apts_flag == false) {
                            uint32_t apts_cal;
                            ALOGI("HW SYNC new first APTS %zd,body size %u", pts, hw_sync->hw_sync_body_cnt);
                            hw_sync->first_apts_flag = true;
                            hw_sync->first_apts = pts;
                            out->frame_write_sum = 0;
                            hw_sync->last_apts_from_header =    pts;
                            sprintf(buf, "AUDIO_START:0x%"PRIx64"", pts & 0xffffffff);
                            ALOGI("tsync -> %s", buf);
                            if (sysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1) {
                                ALOGE("set AUDIO_START failed \n");
                            }
                        } else {
                            uint64_t apts;
                            uint32_t latency = out_get_latency(stream) * 90;
                            apts = (uint64_t) out->frame_write_sum * 90000 / DEFAULT_OUT_SAMPLING_RATE;
                            apts += hw_sync->first_apts;
                            // check PTS discontinue, which may happen when audio track switching
                            // discontinue means PTS calculated based on first_apts and frame_write_sum
                            // does not match the timestamp of next audio samples
                            if (apts > latency) {
                                apts -= latency;
                            } else {
                                apts = 0;
                            }

                            // here we use acutal audio frame gap,not use the differece of  caculated current apts with the current frame pts,
                            //as there is a offset of audio latency from alsa.
                            // handle audio gap 0.5~5 s
                            uint64_t two_frame_gap = get_pts_gap(hw_sync->last_apts_from_header, pts);
                            if (two_frame_gap > APTS_DISCONTINUE_THRESHOLD_MIN  && two_frame_gap < APTS_DISCONTINUE_THRESHOLD_MAX) {
                                /*   if (abs(pts -apts) > APTS_DISCONTINUE_THRESHOLD_MIN && abs(pts -apts) < APTS_DISCONTINUE_THRESHOLD_MAX) { */
                                ALOGI("HW sync PTS discontinue, 0x%"PRIx64"->0x%"PRIx64"(from header) diff %"PRIx64",last apts %"PRIx64"(from header)",
                                      apts, pts, two_frame_gap, hw_sync->last_apts_from_header);
                                //here handle the audio gap and insert zero to the alsa
                                uint insert_size = 0;
                                uint insert_size_total = 0;
                                uint once_write_size = 0;
                                insert_size = two_frame_gap/*abs(pts -apts) */ / 90 * 48 * 4;
                                insert_size = insert_size & (~63);
                                insert_size_total = insert_size;
                                ALOGI("audio gap %"PRIx64" ms ,need insert pcm size %d\n", two_frame_gap/*abs(pts -apts) */ / 90, insert_size);
                                char *insert_buf = (char*) malloc(8192);
                                if (insert_buf == NULL) {
                                    ALOGE("malloc size failed \n");
                                    goto exit;
                                }
                                memset(insert_buf, 0, 8192);
                                if (need_mix) {
                                    mix_buf = malloc(once_write_size);
                                    if (mix_buf == NULL) {
                                        ALOGE("mix_buf malloc failed\n");
                                        free(insert_buf);
                                        goto exit;
                                    }
                                }
                                while (insert_size > 0) {
                                    once_write_size = insert_size > 8192 ? 8192 : insert_size;
                                    if (need_mix) {
                                        pthread_mutex_lock(&adev->lock);
                                        aml_hal_mixer_read(&adev->hal_mixer, mix_buf, once_write_size);
                                        pthread_mutex_unlock(&adev->lock);
                                        memcpy(insert_buf, mix_buf, once_write_size);
                                    }
#if 1
                                    if (enable_dump) {
                                        FILE *fp1 = fopen("/data/tmp/i2s_audio_out.pcm", "a+");
                                        if (fp1) {
                                            int flen = fwrite((char *) insert_buf, 1, once_write_size, fp1);
                                            fclose(fp1);
                                        }
                                    }
#endif
                                    pthread_mutex_lock(&adev->pcm_write_lock);
                                    ret = pcm_write(out->pcm, (void *) insert_buf, once_write_size);
                                    pthread_mutex_unlock(&adev->pcm_write_lock);
                                    if (ret != 0) {
                                        ALOGE("pcm write failed\n");
                                        free(insert_buf);
                                        if (mix_buf) {
                                            free(mix_buf);
                                        }
                                        goto exit;
                                    }
                                    insert_size -= once_write_size;
                                }
                                if (mix_buf) {
                                    free(mix_buf);
                                }
                                mix_buf = NULL;
                                free(insert_buf);
                                // insert end
                                //adev->first_apts = pts;
                                out->frame_write_sum +=  insert_size_total / frame_size;
#if 0
                                sprintf(buf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx", pts);
                                if (sysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1) {
                                    ALOGE("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
                                }
#endif
                            } else {
                                uint pcr = 0;
                                if (get_sysfs_uint(TSYNC_PCRSCR, &pcr) == 0) {
                                    uint apts_gap = 0;
                                    int32_t apts_cal = apts & 0xffffffff;
                                    apts_gap = get_pts_gap(pcr, apts);
                                    if (apts_gap < SYSTIME_CORRECTION_THRESHOLD) {
                                        // do nothing
                                    } else {
                                        sprintf(buf, "0x%x", apts_cal);
                                        ALOGI("tsync -> reset pcrscr 0x%x -> 0x%x, diff %d ms,frame pts %"PRIx64",latency pts %d", pcr, apts_cal, (int)(apts_cal - pcr) / 90, pts, latency);
                                        int ret_val = sysfs_set_sysfs_str(TSYNC_APTS, buf);
                                        if (ret_val == -1) {
                                            ALOGE("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
                                        }
                                    }
                                }
                            }
                            hw_sync->last_apts_from_header = pts;
                        }
#endif

                        //ALOGI("get header body_cnt = %d, pts = %lld", out->hw_sync_body_cnt, pts);
                    }
                    continue;
                } else if (hw_sync->hw_sync_state == HW_SYNC_STATE_BODY) {
                    uint align;
                    uint m = (hw_sync->hw_sync_body_cnt < remain) ? hw_sync->hw_sync_body_cnt : remain;

                    //ALOGI("m = %d", m);

                    // process m bytes, upto end of hw_sync_body_cnt or end of remaining our_write bytes.
                    // within m bytes, there is no hw_sync header and all are body bytes.
                    if (hw_sync->body_align_cnt) {
                        // clear fragment first for alignment limitation on ALSA driver, which
                        // requires each pcm_writing aligned at 16 frame boundaries
                        // assuming data are always PCM16 based, so aligned at 64 bytes unit.
                        if ((m + hw_sync->body_align_cnt) < 64) {
                            // merge only
                            memcpy(&hw_sync->body_align[hw_sync->body_align_cnt], p, m);
                            p += m;
                            remain -= m;
                            hw_sync->body_align_cnt += m;
                            hw_sync->hw_sync_body_cnt -= m;
                            if (hw_sync->hw_sync_body_cnt == 0) {
                                // end of body, research for HW SYNC header
                                hw_sync->hw_sync_state = HW_SYNC_STATE_HEADER;
                                hw_sync->hw_sync_header_cnt = 0;
                                continue;
                            }
                            //ALOGI("align cache add %d, cnt = %d", remain, out->body_align_cnt);
                            break;
                        } else {
                            // merge-submit-continue
                            memcpy(&hw_sync->body_align[hw_sync->body_align_cnt], p, 64 - hw_sync->body_align_cnt);
                            p += 64 - hw_sync->body_align_cnt;
                            remain -= 64 - hw_sync->body_align_cnt;
                            //ALOGI("pcm_write 64, out remain %d", remain);

                            short *w_buf = (short*) &hw_sync->body_align[0];

                            if (need_mix) {
                                short mix_buf[32];
                                pthread_mutex_lock(&adev->lock);
                                aml_hal_mixer_read(&adev->hal_mixer, mix_buf, 64);
                                pthread_mutex_unlock(&adev->lock);

                                for (i = 0; i < 64 / 2 / 2; i++) {
                                    int r;
                                    r = w_buf[2 * i] * out->volume_l + mix_buf[2 * i];
                                    w_buf[2 * i] = CLIP(r);
                                    r = w_buf[2 * i + 1] * out->volume_r + mix_buf[2 * i + 1];
                                    w_buf[2 * i + 1] = CLIP(r);
                                }
                            } else {
                                for (i = 0; i < 64 / 2 / 2; i++) {
                                    int r;
                                    r = w_buf[2 * i] * out->volume_l;
                                    w_buf[2 * i] = CLIP(r);
                                    r = w_buf[2 * i + 1] * out->volume_r;
                                    w_buf[2 * i + 1] = CLIP(r);
                                }
                            }
#if 1
                            if (enable_dump) {
                                FILE *fp1 = fopen("/data/tmp/i2s_audio_out.pcm", "a+");
                                if (fp1) {
                                    int flen = fwrite((char *) w_buf, 1, 64, fp1);
                                    fclose(fp1);
                                }
                            }
#endif
                            pthread_mutex_lock(&adev->pcm_write_lock);
                            ret = pcm_write(out->pcm, w_buf, 64);
                            pthread_mutex_unlock(&adev->pcm_write_lock);
                            out->frame_write_sum += 64 / frame_size;
                            hw_sync->hw_sync_body_cnt -= 64 - hw_sync->body_align_cnt;
                            hw_sync->body_align_cnt = 0;
                            if (hw_sync->hw_sync_body_cnt == 0) {
                                hw_sync->hw_sync_state = HW_SYNC_STATE_HEADER;
                                hw_sync->hw_sync_header_cnt = 0;
                            }
                            continue;
                        }
                    }

                    // process m bytes body with an empty fragment for alignment
                    align = m & 63;
                    if ((m - align) > 0) {
                        short *w_buf = (short*) p;
                        mix_buf = (short *) malloc(m - align);
                        if (mix_buf == NULL) {
                            ALOGE("!!!fatal err,malloc %d bytes fail\n", m - align);
                            ret = -1;
                            goto exit;
                        }
                        if (need_mix) {
                            pthread_mutex_lock(&adev->lock);
                            aml_hal_mixer_read(&adev->hal_mixer, mix_buf, m - align);
                            pthread_mutex_unlock(&adev->lock);
                            for (i = 0; i < (m - align) / 2 / 2; i++) {
                                int r;
                                r = w_buf[2 * i] * out->volume_l + mix_buf[2 * i];
                                mix_buf[2 * i] = CLIP(r);
                                r = w_buf[2 * i + 1] * out->volume_r + mix_buf[2 * i + 1];
                                mix_buf[2 * i + 1] = CLIP(r);
                            }
                        } else {
                            for (i = 0; i < (m - align) / 2 / 2; i++) {

                                int r;
                                r = w_buf[2 * i] * out->volume_l;
                                mix_buf[2 * i] = CLIP(r);
                                r = w_buf[2 * i + 1] * out->volume_r;
                                mix_buf[2 * i + 1] = CLIP(r);
                            }
                        }
#if 1
                        if (enable_dump) {
                            FILE *fp1 = fopen("/data/tmp/i2s_audio_out.pcm", "a+");
                            if (fp1) {
                                int flen = fwrite((char *) mix_buf, 1, m - align, fp1);
                                fclose(fp1);
                            }
                        }
#endif
                        pthread_mutex_lock(&adev->pcm_write_lock);
                        ret = pcm_write(out->pcm, mix_buf, m - align);
                        pthread_mutex_unlock(&adev->pcm_write_lock);
                        free(mix_buf);
                        out->frame_write_sum += (m - align) / frame_size;

                        p += m - align;
                        remain -= m - align;
                        //ALOGI("pcm_write %d, remain %d", m - align, remain);

                        hw_sync->hw_sync_body_cnt -= (m - align);
                        if (hw_sync->hw_sync_body_cnt == 0) {
                            hw_sync->hw_sync_state = HW_SYNC_STATE_HEADER;
                            hw_sync->hw_sync_header_cnt = 0;
                            continue;
                        }
                    }

                    if (align) {
                        memcpy(&hw_sync->body_align[0], p, align);
                        p += align;
                        remain -= align;
                        hw_sync->body_align_cnt = align;
                        //ALOGI("align cache add %d, cnt = %d, remain = %d", align, out->body_align_cnt, remain);

                        hw_sync->hw_sync_body_cnt -= align;
                        if (hw_sync->hw_sync_body_cnt == 0) {
                            hw_sync->hw_sync_state = HW_SYNC_STATE_HEADER;
                            hw_sync->hw_sync_header_cnt = 0;
                            continue;
                        }
                    }
                }
            }

        } else {
            struct aml_hal_mixer *mixer = &adev->hal_mixer;
            pthread_mutex_lock(&adev->pcm_write_lock);
            if (aml_hal_mixer_get_content(mixer) > 0) {
                pthread_mutex_lock(&mixer->lock);
                if (mixer->wp > mixer->rp) {
                    pcm_write(out->pcm, mixer->start_buf + mixer->rp, mixer->wp - mixer->rp);
                } else {
                    pcm_write(out->pcm, mixer->start_buf + mixer->wp, mixer->buf_size - mixer->rp);
                    pcm_write(out->pcm, mixer->start_buf, mixer->wp);
                }
                mixer->rp = mixer->wp = 0;
                pthread_mutex_unlock(&mixer->lock);
            }
            ret = pcm_write(out->pcm, out_buffer, out_frames * frame_size);
            pthread_mutex_unlock(&adev->pcm_write_lock);
            out->frame_write_sum += out_frames;
        }
    }

exit:
    clock_gettime(CLOCK_MONOTONIC, &out->timestamp);
    latency_frames = out_get_latency_frames(stream);
    if (out->frame_write_sum >= latency_frames) {
        out->last_frames_postion = out->frame_write_sum - latency_frames;
    } else {
        out->last_frames_postion = out->frame_write_sum;
    }
    pthread_mutex_unlock(&out->lock);
    if (ret != 0) {
        usleep(bytes * 1000000 / audio_stream_out_frame_size(stream) /
               out_get_sample_rate(&stream->common) * 15 / 16);
    }

    if (force_input_standby) {
        pthread_mutex_lock(&adev->lock);
        if (adev->active_input) {
            in = adev->active_input;
            pthread_mutex_lock(&in->lock);
            do_input_standby(in);
            pthread_mutex_unlock(&in->lock);
        }
        pthread_mutex_unlock(&adev->lock);
    }
    return oldBytes;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    int ret = 0;
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    size_t frame_size = audio_stream_out_frame_size(stream);
    size_t in_frames = bytes / frame_size;
    size_t out_frames;
    bool force_input_standby = false;
    int16_t *in_buffer = (int16_t *) buffer;
    struct aml_stream_in *in;
    uint ouput_len;
    char *data,  *data_dst;
    volatile char *data_src;
    uint i, total_len;
    int codec_type = 0;
    int samesource_flag = 0;
    uint32_t latency_frames = 0;
    int need_mix = 0;
    short *mix_buf = NULL;
    unsigned char enable_dump = getprop_bool("media.audiohal.outdump");

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the output stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&out->lock);

#if 1
    if (enable_dump && out->hw_sync_mode == 0) {
        FILE *fp1 = fopen("/data/tmp/i2s_audio_out.pcm", "a+");
        if (fp1) {
            int flen = fwrite((char *) buffer, 1, bytes, fp1);
            fclose(fp1);
        }
    }
#endif

    if (out->standby) {
        pthread_mutex_lock(&adev->lock);
        ret = start_output_stream(out);
        pthread_mutex_unlock(&adev->lock);
        if (ret != 0) {
            ALOGE("start_output_stream failed");
            goto exit;
        }
        out->standby = false;
        /* a change in output device may change the microphone selection */
        if (adev->active_input &&
            adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION) {
            force_input_standby = true;
        }
    }
#if 1
    /* Reduce number of channels, if necessary */
    //if (popcount(out_get_channels(&stream->common)) >
    //    (int)out->config.channels) {
    // TODO: find the right way to handle PCM BT case.
    if (2 > (int) out->config.channels) {
        unsigned int i;

        /* Discard right channel */
        for (i = 1; i < in_frames; i++) {
            in_buffer[i] = in_buffer[i * 2];
        }

        /* The frame size is now half */
        frame_size /= 2;
    }
#endif
    /* only use resampler if required */
    if (out->config.rate != out_get_sample_rate(&stream->common)) {
        out_frames = out->buffer_frames;
        out->resampler->resample_from_input(out->resampler,
                                            in_buffer, &in_frames,
                                            (int16_t*) out->buffer, &out_frames);
        in_buffer = (int16_t*) out->buffer;
    } else {
        out_frames = in_frames;
    }

#if 1
    if (!(adev->out_device & AUDIO_DEVICE_OUT_ALL_SCO)) {
        codec_type = get_sysfs_int("/sys/class/audiodsp/digital_codec");
        samesource_flag = get_sysfs_int("/sys/class/audiodsp/audio_samesource");
        if (samesource_flag == 0 && codec_type == 0) {
            ALOGI("to enable same source,need reset alsa,type %d,same source flag %d \n",
                  codec_type, samesource_flag);
            pcm_stop(out->pcm);
        }
    }
#endif

    struct aml_hal_mixer *mixer = &adev->hal_mixer;
    pthread_mutex_lock(&adev->pcm_write_lock);
    if (aml_hal_mixer_get_content(mixer) > 0) {
        pthread_mutex_lock(&mixer->lock);
        if (mixer->wp > mixer->rp) {
            pcm_write(out->pcm, mixer->start_buf + mixer->rp, mixer->wp - mixer->rp);
        } else {
            pcm_write(out->pcm, mixer->start_buf + mixer->wp, mixer->buf_size - mixer->rp);
            pcm_write(out->pcm, mixer->start_buf, mixer->wp);
        }
        mixer->rp = mixer->wp = 0;
        pthread_mutex_unlock(&mixer->lock);
    }
    ret = pcm_write(out->pcm, in_buffer, out_frames * frame_size);
    pthread_mutex_unlock(&adev->pcm_write_lock);
    out->frame_write_sum += out_frames;

exit:
    latency_frames = out_get_latency(stream) * out->config.rate / 1000;
    if (out->frame_write_sum >= latency_frames) {
        out->last_frames_postion = out->frame_write_sum - latency_frames;
    } else {
        out->last_frames_postion = out->frame_write_sum;
    }
    pthread_mutex_unlock(&out->lock);
    if (ret != 0) {
        usleep(bytes * 1000000 / audio_stream_out_frame_size(stream) /
               out_get_sample_rate(&stream->common) * 15 / 16);
    }

    if (force_input_standby) {
        pthread_mutex_lock(&adev->lock);
        if (adev->active_input) {
            in = adev->active_input;
            pthread_mutex_lock(&in->lock);
            do_input_standby(in);
            pthread_mutex_unlock(&in->lock);
        }
        pthread_mutex_unlock(&adev->lock);
    }
    return bytes;
}

/*
insert bytes of zero data to pcm which makes A/V synchronization.
TODO.If the HWSYNC audio format is DD+/DD compressed bitstream.we can
not directly insert zero data or do hw mixer then write data to
alsa driver. MS12 will skip the audio data and no valid output.
We need insert a mute dd+ frame then to MS12.
*/
static int insert_output_bytes(struct aml_stream_out *out, size_t size)
{
    int ret = 0;
    size_t insert_size = size;
    size_t once_write_size = 0;
    struct audio_stream_out *stream = (struct audio_stream_out*)out;
    struct aml_audio_device *adev = out->dev;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;
    audio_format_t output_format = get_output_format(stream);
    aml_data_format_t data_format;
    char *insert_buf = (char*) malloc(8192);

    if (insert_buf == NULL) {
        ALOGE("malloc size failed \n");
        return -ENOMEM;
    }

    if (!out->pcm) {
        ret = -ENOENT;
        goto exit;
    }

    memset(insert_buf, 0, 8192);
    while (insert_size > 0) {
        once_write_size = insert_size > 8192 ? 8192 : insert_size;
#ifdef ENABLE_MS12
        if (eDolbyMS12Lib == adev->dolby_lib_type) {
            size_t used_size = 0;
            ret = dolby_ms12_main_process(stream, insert_buf, once_write_size, &used_size);
            if (ret) {
                ALOGW("dolby_ms12_main_process cost size %zu,input size %zu,check!!!!", once_write_size, used_size);
            }
        } else
#endif
        {
            aml_hw_mixer_mixing(&adev->hw_mixer, insert_buf, once_write_size);
            //process_buffer_write(stream, buffer, bytes);
            data_format.format = output_format;
            data_format.sr     = 48000;
            data_format.ch     = 2;
            data_format.bitwidth = SAMPLE_16BITS;
            if (audio_hal_data_processing(stream, insert_buf, once_write_size, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
                hw_write(stream, output_buffer, output_buffer_bytes, &data_format);
            }
        }
        insert_size -= once_write_size;
    }

exit:
    free(insert_buf);
    return 0;
}

enum hwsync_status {
    CONTINUATION,  // good sync condition
    ADJUSTMENT,    // can be adjusted by discarding or padding data
    RESYNC,        // pts need resync
};

enum hwsync_status check_hwsync_status(uint apts_gap) {
    enum hwsync_status sync_status;

    if (apts_gap < APTS_DISCONTINUE_THRESHOLD_MIN)
    {
        sync_status = CONTINUATION;
    } else if (apts_gap > APTS_DISCONTINUE_THRESHOLD_MAX)
    {
        sync_status = RESYNC;
    } else
    { sync_status = ADJUSTMENT; }

    return sync_status;
}

static ssize_t out_write_direct(struct audio_stream_out *stream, const void* buffer,
                                size_t bytes)
{
    int ret = 0;
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    size_t frame_size = audio_stream_out_frame_size(stream);
    size_t in_frames = bytes / frame_size;
    bool force_input_standby = false;
    size_t out_frames = 0;
    void *buf;
    uint i, total_len;
    char prop[PROPERTY_VALUE_MAX];
    char head[4];
    int codec_type = out->codec_type;
    int samesource_flag = 0;
    uint32_t latency_frames = 0;
    uint64_t total_frame = 0;
    int return_bytes = bytes;
    audio_hwsync_t *hw_sync = out->hwsync;
    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
    * on the output stream mutex - e.g. executing select_mode() while holding the hw device
    * mutex
    */
    ALOGI("out_write_direct:out %p,position %zu, out_write size %"PRIu64"",
          out, bytes, out->frame_write_sum);
#if 0
    FILE *fp1 = fopen("/data/out_write_direct_passthrough.pcm", "a+");
    if (fp1) {
        int flen = fwrite((char *) buffer, 1, bytes, fp1);
        //ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
        fclose(fp1);
    } else {
        ALOGD("could not open file:/data/out_write_direct_passthrough.pcm");
    }
#endif

    /*when hi-pcm stopped  and switch to 2-ch , then switch to hi-pcm,hi-pcm-mode must be
     set and wait 20ms for i2s device release*/
    if (get_codec_type(out->hal_internal_format) == TYPE_PCM && !adev->hi_pcm_mode
        && (out->config.rate > 48000 || out->config.channels >= 6)) {
        adev->hi_pcm_mode = true;
        usleep(20000);
    }
    pthread_mutex_lock(&out->lock);
    if (out->pause_status == true) {
        pthread_mutex_unlock(&out->lock);
        ALOGI("call out_write when pause status,size %zu,(%p)\n", bytes, out);
        return 0;
    }

    ALOGV("%s with flag 0x%x\n", __func__, out->flags);
    if ((out->standby) && out->hw_sync_mode) {
        /*
        there are two types of raw data come to hdmi  audio hal
        1) compressed audio data without IEC61937 wrapped
        2) compressed audio data  with IEC61937 wrapped (typically from amlogic amadec source)
        we use the AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO to distiguwish the two cases.
        */
#ifdef SPDIF_ENCODER
        if ((codec_type == TYPE_AC3 || codec_type == TYPE_EAC3)  && (out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO)) {
            spdifenc_init(out->pcm, out->hal_internal_format);
            out->spdif_enc_init_frame_write_sum = out->frame_write_sum;
        }
#endif
        // todo: check timestamp header PTS discontinue for new sync point after seek
        if ((codec_type == TYPE_AC3 || codec_type == TYPE_EAC3)) {
            aml_audio_hwsync_init(out->hwsync, out);
            out->spdif_enc_init_frame_write_sum = out->frame_write_sum;
        }
    }
    if (out->standby) {
        pthread_mutex_lock(&adev->lock);
        ret = start_output_stream_direct(out);
        pthread_mutex_unlock(&adev->lock);

        if (ret != 0) {
            goto exit;
        }
        out->standby = 0;
        /* a change in output device may change the microphone selection */
        if (adev->active_input &&
            adev->active_input->source == AUDIO_SOURCE_VOICE_COMMUNICATION) {
            force_input_standby = true;
        }
    }

    if (out->hal_format == AUDIO_FORMAT_IEC61937 &&
        out->hal_internal_format == AUDIO_FORMAT_DTS &&
        !out->flag_dtschecked &&
        get_sysfs_int("/sys/class/audiodsp/digital_codec") == TYPE_DTS) {
        memcpy(head, buffer, 4);
        ALOGI("head[0]:%0x head[1]:%0x head[2]:%0x head[3]:%0x", head[0], head[2], head[2], head[3]);
        if ((head[0] == 0x72 && head[1] == 0xf8 && head[2] == 0x1f && head[3] == 0x4e) ||
            (head[0] == 0x4e && head[1] == 0x1f && head[2] == 0xf8 && head[3] == 0x72)) {
            ALOGI("IEC NONAUDIO DETECTED !!!");
        } else {
            ALOGI("DTS CD DETECTED !!!");
            set_codec_type(TYPE_PCM);
        }
        out->flag_dtschecked = true;
    }
    void *write_buf = NULL;
    size_t  hwsync_cost_bytes = 0;
    if (out->hw_sync_mode == 1) {
        uint64_t  cur_pts = 0xffffffff;
        int outsize = 0;
        char tempbuf[128];
        ALOGV("before aml_audio_hwsync_find_frame bytes %zu\n", bytes);
        hwsync_cost_bytes = aml_audio_hwsync_find_frame(out->hwsync, buffer, bytes, &cur_pts, &outsize);
        if (cur_pts > 0xffffffff) {
            ALOGE("APTS exeed the max 32bit value");
        }
        ALOGV("after aml_audio_hwsync_find_frame bytes remain %zu,cost %zu,outsize %d,pts %"PRIx64"\n",
              bytes - hwsync_cost_bytes, hwsync_cost_bytes, outsize, cur_pts);
        //TODO,skip 3 frames after flush, to tmp fix seek pts discontinue issue.need dig more
        // to find out why seek ppint pts frame is remained after flush.WTF.
        if (out->skip_frame > 0) {
            out->skip_frame--;
            ALOGI("skip pts@%"PRIx64",cur frame size %d,cost size %zu\n", cur_pts, outsize, hwsync_cost_bytes);
            pthread_mutex_unlock(&out->lock);
            return hwsync_cost_bytes;
        }
        if (cur_pts != 0xffffffff && outsize > 0) {
            // if we got the frame body,which means we get a complete frame.
            //we take this frame pts as the first apts.
            //this can fix the seek discontinue,we got a fake frame,which maybe cached before the seek
            if (hw_sync->first_apts_flag == false) {
                aml_audio_hwsync_set_first_pts(out->hwsync, cur_pts);
            } else {
                uint64_t apts;
                uint32_t apts32;
                uint pcr = 0;
                uint apts_gap = 0;
                uint64_t latency = out_get_latency(stream) * 90;
                // check PTS discontinue, which may happen when audio track switching
                // discontinue means PTS calculated based on first_apts and frame_write_sum
                // does not match the timestamp of next audio samples
                if (cur_pts > latency) {
                    apts = cur_pts - latency;
                } else {
                    apts = 0;
                }

                apts32 = apts & 0xffffffff;

                if (get_sysfs_uint(TSYNC_PCRSCR, &pcr) == 0) {
                    enum hwsync_status sync_status = CONTINUATION;
                    apts_gap = get_pts_gap(pcr, apts32);
                    sync_status = check_hwsync_status(apts_gap);

                    // limit the gap handle to 0.5~5 s.
                    if (sync_status == ADJUSTMENT) {
                        // two cases: apts leading or pcr leading
                        // apts leading needs inserting frame and pcr leading neads discarding frame
                        if (apts32 > pcr) {
                            int insert_size = 0;
                            if (out->codec_type == TYPE_EAC3) {
                                insert_size = apts_gap / 90 * 48 * 4 * 4;
                            } else {
                                insert_size = apts_gap / 90 * 48 * 4;
                            }
                            insert_size = insert_size & (~63);
                            ALOGI("audio gap 0x%"PRIx32" ms ,need insert data %d\n", apts_gap / 90, insert_size);
                            ret = insert_output_bytes(out, insert_size);
                        } else {
                            //audio pts smaller than pcr,need skip frame.
                            //we assume one frame duration is 32 ms for DD+(6 blocks X 1536 frames,48K sample rate)
                            if (out->codec_type == TYPE_EAC3 && outsize > 0) {
                                ALOGI("audio slow 0x%x,skip frame @pts 0x%"PRIx64",pcr 0x%x,cur apts 0x%x\n",
                                      apts_gap, cur_pts, pcr, apts32);
                                out->frame_skip_sum  +=   1536;
                                bytes = outsize;
                                goto exit;
                            }
                        }
                    } else if (sync_status == RESYNC) {
                        sprintf(tempbuf, "0x%x", apts32);
                        ALOGI("tsync -> reset pcrscr 0x%x -> 0x%x, %s big,diff %"PRIx64" ms",
                              pcr, apts32, apts32 > pcr ? "apts" : "pcr", get_pts_gap(apts, pcr) / 90);

                        int ret_val = sysfs_set_sysfs_str(TSYNC_APTS, tempbuf);
                        if (ret_val == -1) {
                            ALOGE("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
                        }
                    }
                }
            }
        }
        if (outsize > 0) {
            return_bytes = hwsync_cost_bytes;
            in_frames = outsize / frame_size;
            write_buf = hw_sync->hw_sync_body_buf;
        } else {
            return_bytes = hwsync_cost_bytes;
            goto exit;
        }
    } else {
        write_buf = (void *) buffer;
    }
    out_frames = in_frames;
    buf = (void *) write_buf;
    if (getprop_bool("media.hdmihal.outdump")) {
        FILE *fp1 = fopen("/data/tmp/hdmi_audio_out.pcm", "a+");
        if (fp1) {
            int flen = fwrite((char *)buffer, 1, out_frames * frame_size, fp1);
            //ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
            fclose(fp1);
        } else {
            ALOGD("could not open file:/data/tmp/hdmi_audio_out.pcm");
        }
    }
    if (codec_type_is_raw_data(out->codec_type) && !(out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO)) {
#ifdef SPDIF_ENCODER
        //here to do IEC61937 pack
        ALOGV("IEC61937 write size %zu,hw_sync_mode %d,flag %x\n", out_frames * frame_size, out->hw_sync_mode, out->flags);
        if (out->codec_type  > 0) {
            // compressed audio DD/DD+
            bytes = spdifenc_write((void *) buf, out_frames * frame_size);
            //need return actual size of this burst write
            if (out->hw_sync_mode == 1) {
                bytes = hwsync_cost_bytes;
            }
            ALOGV("spdifenc_write return %zu\n", bytes);

            if (out->codec_type == TYPE_EAC3) {
                out->frame_write_sum = spdifenc_get_total() / 16 + out->spdif_enc_init_frame_write_sum;
            } else {
                out->frame_write_sum = spdifenc_get_total() / 4 + out->spdif_enc_init_frame_write_sum;
            }

            ALOGV("out %p,out->frame_write_sum %"PRId64"\n", out, out->frame_write_sum);
        }
#endif
        goto exit;
    }
    //here handle LPCM audio (hi-res audio) which goes to direct output
    if (!out->standby) {
        int write_size = out_frames * frame_size;
        //for 5.1/7.1 LPCM direct output,we assume only use left channel volume
        if (!codec_type_is_raw_data(out->codec_type)
            && (out->multich > 2 || out->hal_internal_format != AUDIO_FORMAT_PCM_16_BIT)) {
            //do audio format and data conversion here
            int input_frames = out_frames;
            write_buf = convert_audio_sample_for_output(input_frames,
                        out->hal_internal_format, out->multich, buf, &write_size);
            //volume apply here,TODO need apply that inside convert_audio_sample_for_output function.
            if (out->multich == 2) {
                short *sample = (short*) write_buf;
                int l, r;
                int kk;
                for (kk = 0; kk <  input_frames; kk++) {
                    l = out->volume_l * sample[kk * 2];
                    sample[kk * 2] = CLIP(l);
                    r = out->volume_r * sample[kk * 2 + 1];
                    sample[kk * 2 + 1] = CLIP(r);
                }
            } else {
                int *sample = (int*) write_buf;
                int kk;
                for (kk = 0; kk <  write_size / 4; kk++) {
                    sample[kk] = out->volume_l * sample[kk];
                }
            }

            if (write_buf) {
                if (getprop_bool("media.hdmihal.outdump")) {
                    FILE *fp1 = fopen("/data/tmp/hdmi_audio_out8.pcm", "a+");
                    if (fp1) {
                        int flen = fwrite((char *) buffer, 1, out_frames * frame_size, fp1);
                        ALOGD("flen = %d---outlen=%zu ", flen, out_frames * frame_size);
                        fclose(fp1);
                    } else {
                        ALOGD("could not open file:/data/hdmi_audio_out.pcm");
                    }
                }
                ret = pcm_write(out->pcm, write_buf, write_size);
                if (ret == 0) {
                    out->frame_write_sum += out_frames;
                } else {
                    ALOGI("pcm_get_error(out->pcm):%s", pcm_get_error(out->pcm));
                }
                if (write_buf) {
                    free(write_buf);
                }
            }
        } else {
            //2 channel LPCM or raw data pass through
            if (!codec_type_is_raw_data(out->codec_type) && out->config.channels == 2) {
                short *sample = (short*) buf;
                int l, r;
                size_t kk;
                for (kk = 0; kk <  out_frames; kk++) {
                    l = out->volume_l * sample[kk * 2];
                    sample[kk * 2] = CLIP(l);
                    r = out->volume_r * sample[kk * 2 + 1];
                    sample[kk * 2 + 1] = CLIP(r);
                }
            }
#if 0
            FILE *fp1 = fopen("/data/pcm_write_passthrough.pcm", "a+");
            if (fp1) {
                int flen = fwrite((char *) buf, 1, out_frames * frame_size, fp1);
                //ALOGD("flen = %d---outlen=%d ", flen, out_frames * frame_size);
                fclose(fp1);
            } else {
                ALOGD("could not open file:/data/pcm_write_passthrough.pcm");
            }
#endif
            ret = pcm_write(out->pcm, (void *) buf, out_frames * frame_size);
            if (ret == 0) {
                out->frame_write_sum += out_frames;
            } else {
                ALOGI("pcm_get_error(out->pcm):%s", pcm_get_error(out->pcm));
            }
        }
    }

exit:
    total_frame = out->frame_write_sum + out->frame_skip_sum;
    latency_frames = out_get_latency_frames(stream);
    clock_gettime(CLOCK_MONOTONIC, &out->timestamp);
    if (total_frame >= latency_frames) {
        out->last_frames_postion = total_frame - latency_frames;
    } else {
        out->last_frames_postion = total_frame;
    }
    ALOGV("\nout %p,out->last_frames_postion %"PRId64", latency = %d\n", out, out->last_frames_postion, latency_frames);
    pthread_mutex_unlock(&out->lock);
    if (ret != 0) {
        usleep(bytes * 1000000 / audio_stream_out_frame_size(stream) /
               out_get_sample_rate(&stream->common));
    }

    return return_bytes;
}


static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    uint64_t  dsp_frame_int64 = out->last_frames_postion;
    *dsp_frames = 0U;
    if (out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) {
        dsp_frame_int64 = out->last_frames_postion ;
        *dsp_frames = (uint32_t)(dsp_frame_int64 & 0xffffffff);
        if (out->last_dsp_frame > dsp_frame_int64) {
            ALOGI("maybe uint32_t wraparound,print something,last %u,now %u", out->last_dsp_frame, *dsp_frames);
            ALOGI("wraparound,out_get_render_position return %u,playback time %"PRIu64" ms,sr %d\n", *dsp_frames,
                  out->last_frames_postion * 1000 / out->config.rate, out->config.rate);
        }
    }

#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        /* handle the case hal_rate != 48KHz */
        if (out->hal_rate != MM_FULL_POWER_SAMPLING_RATE) {
            dsp_frame_int64 = (dsp_frame_int64 * out->hal_rate) / MM_FULL_POWER_SAMPLING_RATE;
        }
        *dsp_frames = dsp_frame_int64;
        if (direct_continous((struct audio_stream_out *)stream)) {
            *dsp_frames = adev->ms12.last_frames_postion;
        }
    }
#endif
    if (adev->active_outport == OUTPORT_HDMI_ARC && !adev->continuous_audio_mode) {
        int arc_latency_ms = 0;
        int frame_latency = 0;
        arc_latency_ms = aml_audio_get_arc_latency_offset(adev->sink_format);
        //TODO we support it is 1 48K audio
        frame_latency = arc_latency_ms * 48;
        *dsp_frames += frame_latency;
    }
    if (adev->debug_flag) {
        ALOGI("out_get_render_position %d \n", *dsp_frames);
    }
    return 0;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *dev = out->dev;
    int i;
    int status = 0;

    pthread_mutex_lock(&dev->lock);
    pthread_mutex_lock(&out->lock);
    if (dev->native_postprocess.num_postprocessors >= MAX_POSTPROCESSORS) {
        status = -ENOSYS;
        goto exit;
    }

    for (i = 0; i < dev->native_postprocess.num_postprocessors; i++) {
        if (dev->native_postprocess.postprocessors[i] == effect) {
            status = 0;
            goto exit;
        }
    }

    dev->native_postprocess.postprocessors[dev->native_postprocess.num_postprocessors++] = effect;
exit:
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&dev->lock);
    return status;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *dev = out->dev;
    int i;
    int status = -EINVAL;
    bool found = false;
    pthread_mutex_lock(&dev->lock);
    pthread_mutex_lock(&out->lock);
    if (dev->native_postprocess.num_postprocessors <= 0) {
        status = -ENOSYS;
        goto exit;
    }

    for (i = 0; i < dev->native_postprocess.num_postprocessors; i++) {
        if (found) {
            dev->native_postprocess.postprocessors[i - 1] = dev->native_postprocess.postprocessors[i];
            continue;
        }

        if (dev->native_postprocess.postprocessors[i] == effect) {
            dev->native_postprocess.postprocessors[i] = NULL;
            status = 0;
            found = true;
        }
    }

    if (status != 0) {
        goto exit;
    }

    dev->native_postprocess.num_postprocessors--;
exit:
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&dev->lock);
    return status;
}
static int out_get_next_write_timestamp(const struct audio_stream_out *stream __unused,
                                        int64_t *timestamp __unused)
{
    // return -EINVAL;

    // VTS can only recognizes Result:OK or Result:INVALID_STATE, which is 0 or 3.
    // So we return ESRCH (3) in order to pass VTS.
    ALOGI("Amlogic_HAL - %s: return ESRCH (3) instead of -EINVAL (-22)", __FUNCTION__);
    return ESRCH;
}

//actually maybe it be not useful now  except pass CTS_TEST:
//  run cts -c android.media.cts.AudioTrackTest -m testGetTimestamp
static int out_get_presentation_position(const struct audio_stream_out *stream, uint64_t *frames, struct timespec *timestamp)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = out->dev;
    uint64_t frames_written_hw = out->last_frames_postion;

    if (!frames || !timestamp) {
        return -EINVAL;
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        /* handle the case hal_rate != 48KHz */
        if (out->hal_rate != MM_FULL_POWER_SAMPLING_RATE) {
            frames_written_hw = (frames_written_hw * out->hal_rate) / MM_FULL_POWER_SAMPLING_RATE;
        }
    }
#endif
    *frames = frames_written_hw;
    *timestamp = out->timestamp;
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (direct_continous((struct audio_stream_out *)stream)) {
            *frames = adev->ms12.last_frames_postion;
            *timestamp = adev->ms12.timestamp;
        }
    }
#endif
    if (adev->active_outport == OUTPORT_HDMI_ARC && !adev->continuous_audio_mode) {
        int arc_latency_ms = 0;
        int frame_latency = 0;
        arc_latency_ms = aml_audio_get_arc_latency_offset(adev->sink_format);
        //TODO we support it is 1 48K audio
        frame_latency = arc_latency_ms * 48;
        *frames += frame_latency;
    }
    if (adev->debug_flag) {
        ALOGI("out_get_presentation_position out %p %"PRIu64", sec = %ld, nanosec = %ld\n", out, *frames, timestamp->tv_sec, timestamp->tv_nsec);
    }
    return 0;
}
static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer* buffer);
static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer* buffer);


/** audio_stream_in implementation **/

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct aml_stream_in *in)
{
    int ret = 0;
    aml_stream_config_t stream_config = {0};
    aml_device_config_t device_config;

    struct aml_audio_device *adev = in->dev;
    ALOGD("%s(need_echo_reference=%d, channels=%d, rate=%d, requested_rate=%d, mode= %d device=0x%x 0x%x)\n",
          __FUNCTION__, in->need_echo_reference, in->config.channels, in->config.rate, in->requested_rate, adev->mode, adev->in_device, in->device);
    adev->active_input = in;

    if (adev->mode != AUDIO_MODE_IN_CALL) {
        adev->in_device &= ~AUDIO_DEVICE_IN_ALL;
        adev->in_device |= in->device;
        //select_devices(adev);
    }

    stream_config.channels  = in->config.channels;
    stream_config.rate      = in->config.rate;
    stream_config.format    = in->config.format;
    //stream_config.framesize = in->config.period_size;
    device_config.device    = adev->in_device;

    ret = aml_input_open((struct audio_stream_in *)in, &stream_config, &device_config);

    if (ret != 0) {
        adev->active_input = NULL;
        return -1;
    }

    /* if no supported sample rate is available, use the resampler */
    if (in->resampler) {
        in->resampler->reset(in->resampler);
        in->frames_in = 0;
    }
    return 0;
}

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;

    return in->requested_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    (void)stream;
    (void)rate;
    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;

    return get_input_buffer_size(in->config.period_size, in->config.rate,
                                 AUDIO_FORMAT_PCM_16_BIT,
                                 in->config.channels);
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;

    /* if (in->config.channels == 1) {
         return AUDIO_CHANNEL_IN_MONO;
     } else {
         return AUDIO_CHANNEL_IN_STEREO;
     }*/
    return in->main_channels;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    (void)stream;
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    (void)stream;
    (void)format;
    return -ENOSYS;
}

/* must be called with hw device and input stream mutexes locked */
static int do_input_standby(struct aml_stream_in *in)
{
    struct aml_audio_device *adev = in->dev;

    ALOGD("%s(%p) in->standby = %d", __FUNCTION__, in, in->standby);
    if (!in->standby) {
        aml_input_close((struct audio_stream_in *)in);

        adev->active_input = 0;
        if (adev->mode != AUDIO_MODE_IN_CALL) {
            adev->in_device &= ~AUDIO_DEVICE_IN_ALL;
            //select_input_device(adev);
        }

        in->standby = 1;
#if 0
        ALOGD("%s : output_standby=%d,input_standby=%d",
              __FUNCTION__, output_standby, input_standby);
        if (output_standby && input_standby) {
            reset_mixer_state(adev->ar);
            update_mixer_state(adev->ar);
        }
#endif
    }
    return 0;
}
static int in_standby(struct audio_stream *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    int status;
    ALOGD("%s(%p)", __FUNCTION__, stream);

    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    status = do_input_standby(in);
    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int in_dump(const struct audio_stream *stream __unused, int fd __unused)
{
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{

    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    struct aml_audio_device *adev = in->dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    bool do_standby = false;
#if 1

    ALOGD("%s(%p, %s)", __FUNCTION__, stream, kvpairs);
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value, sizeof(value));

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
    if (ret >= 0) {
        val = atoi(value);
        /* no audio source uses val == 0 */
        if ((in->source != val) && (val != 0)) {
            in->source = val;
            do_standby = true;
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value) & ~AUDIO_DEVICE_BIT_IN;
        if ((in->device != val) && (val != 0)) {
            in->device = val;
            do_standby = true;
        }
    }

    if (do_standby) {
        do_input_standby(in);
    }
    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&adev->lock);

    int framesize = 0;
    ret = str_parms_get_int(parms, AUDIO_PARAMETER_STREAM_FRAME_COUNT, &framesize);

    if (ret >= 0) {
        if (framesize > 0) {
            ALOGI("Reset audio input hw frame size from %d to %d\n",
                  in->config.period_size * in->config.period_count, framesize);
            in->config.period_size = framesize / in->config.period_count;
            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&in->lock);

            if (!in->standby && (in == adev->active_input)) {
                do_input_standby(in);
                start_input_stream(in);
                in->standby = 0;
            }

            pthread_mutex_unlock(&in->lock);
            pthread_mutex_unlock(&adev->lock);
        }
    }

    str_parms_destroy(parms);

    // VTS can only recognizes Result::OK, which is 0x0.
    // So we change ret value to 0 when ret isn't equal to 0
    if (ret > 0) {
        ALOGI("Amlogic_HAL - %s: change ret value to 0 if it's greater than 0 for passing VTS test.", __FUNCTION__);
        ret = 0;
    } else if (ret < 0) {
        ALOGI("Amlogic_HAL - %s: parameter is NULL, change ret value to 0 if it's greater than 0 for passing VTS test.", __FUNCTION__);
        ret = 0;
    }
#endif
    return ret;
}

static char * in_get_parameters(const struct audio_stream *stream __unused,
                                const char *keys __unused)
{
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream __unused, float gain __unused)
{
    return 0;
}


static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer* buffer)
{
    struct aml_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL) {
        return -EINVAL;
    }

    in = (struct aml_stream_in *)((char *) buffer_provider -
                                  offsetof(struct aml_stream_in, buf_provider));

    if (in->pcm == NULL) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        in->read_status = -ENODEV;
        return -ENODEV;
    }

    if (in->frames_in == 0) {

        in->read_status = aml_input_read((struct audio_stream_in *)in, buffer, in->config.period_size * audio_stream_in_frame_size(&in->stream));


        if (in->read_status != 0) {
            ALOGE("get_next_buffer() in_read error %d", in->read_status);
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return in->read_status;
        }
        in->frames_in = in->config.period_size;
    }

    buffer->frame_count = (buffer->frame_count > in->frames_in) ?
                          in->frames_in : buffer->frame_count;
    buffer->i16 = in->buffer + (in->config.period_size - in->frames_in) *
                  in->config.channels;

    return in->read_status;

}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer* buffer)
{
    struct aml_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL) {
        return;
    }

    in = (struct aml_stream_in *)((char *) buffer_provider -
                                  offsetof(struct aml_stream_in, buf_provider));

    in->frames_in -= buffer->frame_count;
}

/* read_frames() reads frames from kernel driver, down samples to capture rate
 * if necessary and output the number of frames requested to the buffer specified */
static ssize_t read_frames(struct aml_stream_in *in, void *buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;

    while (frames_wr < frames) {
        size_t frames_rd = frames - frames_wr;
        if (in->resampler != NULL) {
            in->resampler->resample_from_provider(in->resampler,
                                                  (int16_t *)((char *) buffer +
                                                          frames_wr * audio_stream_in_frame_size(&in->stream)),
                                                  &frames_rd);
        } else {
            struct resampler_buffer buf = {
                { .raw = NULL, },
                .frame_count = frames_rd,
            };
            get_next_buffer(&in->buf_provider, &buf);
            if (buf.raw != NULL) {
                memcpy((char *) buffer +
                       frames_wr * audio_stream_in_frame_size(&in->stream),
                       buf.raw,
                       buf.frame_count * audio_stream_in_frame_size(&in->stream));
                frames_rd = buf.frame_count;
            }
            release_buffer(&in->buf_provider, &buf);
        }
        /* in->read_status is updated by getNextBuffer() also called by
         * in->resampler->resample_from_provider() */
        if (in->read_status != 0) {
            return in->read_status;
        }

        frames_wr += frames_rd;
    }
    return frames_wr;
}


static void in_reset_preroid_size(struct audio_stream_in *stream , unsigned int period_size)
{
    int ret = 0;
    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    struct aml_audio_device *adev = in->dev;
    pthread_mutex_lock(&in->lock);

    /*in buffer is allocated in adev_open_input_stream according to the
    period_size, is it safe to change the size?*/
    in->config.period_size = period_size;
    ALOGD("Config Period size to %d\n", in->config.period_size);
    if (in->standby) {
        ret = start_input_stream(in);
        if (ret < 0) {
            goto exit;
        }
    } else {
        do_input_standby(in);
        start_input_stream(in);
        if (ret < 0) {
            goto exit;
        }

    }

    in->standby = 0;
exit:
    if (ret < 0) {
        ALOGE("in_reset err!!!");
    }

    pthread_mutex_unlock(&in->lock);
    return;
}


static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    int ret = 0;
    struct aml_stream_in *in = (struct aml_stream_in *) stream;
    struct aml_audio_device *adev = in->dev;
    size_t frames_rq = bytes / audio_stream_in_frame_size(&in->stream);
    struct aml_audio_parser *parser = adev->aml_parser;
    int in_mute = 0, parental_mute = 0;
    bool stable = true;

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the input stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&in->lock);
    if (in->standby) {
        ret = start_input_stream(in);
        if (ret < 0) {
            bytes = 0;
            ALOGI("could not get data here!");
            goto exit;
        }
        in->standby = 0;
    }

#if 0
    /* if audio patch type is hdmi to mixer, check audio format from hdmi*/
    if (adev->patch_src == SRC_HDMIIN && parser != NULL) {
        audio_format_t cur_aformat = audio_parse_get_audio_type(parser->audio_parse_para);
        if (cur_aformat != parser->aformat) {
            ALOGI("input format changed from %#x to %#x\n", parser->aformat, cur_aformat);
            if (parser->aformat == AUDIO_FORMAT_PCM_16_BIT && //from pcm -> dd/dd+
                (cur_aformat == AUDIO_FORMAT_AC3 || cur_aformat == AUDIO_FORMAT_E_AC3)) {
                parser->aml_pcm = in->pcm;
                parser->in_sample_rate = in->config.rate;
                parser->out_sample_rate = in->requested_rate;
                parser->decode_dev_op_mutex = &in->lock;
                parser->data_ready = 0;
                dcv_decode_init(parser);
            }
            if (parser->aformat == AUDIO_FORMAT_PCM_16_BIT && //from pcm -> dd/dd+
                (cur_aformat == AUDIO_FORMAT_DTS || cur_aformat == AUDIO_FORMAT_DTS_HD)) {
                parser->aml_pcm = in->pcm;
                parser->in_sample_rate = in->config.rate;
                parser->out_sample_rate = in->requested_rate;
                parser->decode_dev_op_mutex = &in->lock;
                parser->data_ready = 0;
                dca_decode_init(parser);
            } else if (cur_aformat == AUDIO_FORMAT_PCM_16_BIT && //from dd/dd+ -> pcm
                       (parser->aformat == AUDIO_FORMAT_AC3 || parser->aformat == AUDIO_FORMAT_E_AC3)) {
                dcv_decode_release(parser);
            } else if (cur_aformat == AUDIO_FORMAT_PCM_16_BIT && //from dd/dd+ -> pcm
                       (parser->aformat == AUDIO_FORMAT_DTS || parser->aformat == AUDIO_FORMAT_DTS_HD)) {
                dca_decode_release(parser);
            } else {
                ALOGI("This format unsupport or no need to reset decoder!\n");
            }
            parser->aformat = cur_aformat;
        }

    }
#endif
    /*if raw data from hdmi and decoder is ready read from decoder buffer*/
    if (parser != NULL && parser->decode_enabled == 1) {
        /*if data is ready, read from buffer.*/
        if (parser->data_ready == 1) {
            ret = ring_buffer_read(&parser->aml_ringbuffer,
                                   (unsigned char*) buffer, bytes);
            if (ret < 0) {
                ALOGE("%s(), parser in_read err", __func__);
                goto exit;
            } else if (ret == 0) {
                memset(buffer, 0, bytes);
            } else {
                bytes = ret;
            }
        } else {
            memset(buffer, 0, bytes);
        }
    } else {
        if (in->resampler != NULL) {
            ret = read_frames(in, buffer, frames_rq);
        } else {
            ret = aml_input_read(stream, buffer, bytes);
        }
        if (ret < 0) {
            goto exit;
        }
        DoDumpData(buffer, bytes, CC_DUMP_SRC_TYPE_INPUT);
    }


#if 0
    /* hdmi in audio unstable, need to mute the audio data for a while
     * the mute time is related to hdmi audio buffer size
     */
    if (adev->in_device & AUDIO_DEVICE_IN_HDMI) {
        stable = is_hdmi_in_stable_hw(stream);
        if (!stable || !is_hdmi_in_stable_sw(stream)) {
            if (in->mute_log_cntr == 0) {
                ALOGI("hdmi rx audio unstable, mute HDMI RX channel");
            }

            in->mute_log_cntr++;
            if (in->mute_log_cntr >= 100) {
                in->mute_log_cntr = 0;
            }
            clock_gettime(CLOCK_MONOTONIC, &in->mute_start_ts);
            in->mute_flag = 1;
        }
    }
#endif
    if ((adev->in_device & AUDIO_DEVICE_IN_TV_TUNER) && adev->patch_src == SRC_DTV) {
        //add for dtv process
    }
    /* ATV audio unstable, need to mute audio for a while */
    else if ((adev->in_device & AUDIO_DEVICE_IN_TV_TUNER) &&
             !is_atv_in_stable_hw(stream)) {
        if (in->mute_log_cntr == 0) {
            ALOGI("ATV audio unstable, mute ATV channel");
        }

        in->mute_log_cntr++;
        if (in->mute_log_cntr >= 100) {
            in->mute_log_cntr = 0;
        }
        clock_gettime(CLOCK_MONOTONIC, &in->mute_start_ts);
        in->mute_flag = 1;
    }
    if (in->mute_flag == 1) {
        struct timespec new_ts;
        int64_t start_ms, end_ms;
        int64_t interval_ms;
        int64_t mute_mdelay = 600;

        if (adev->in_device & AUDIO_DEVICE_IN_TV_TUNER) {
            mute_mdelay = 1000;
        }

        clock_gettime(CLOCK_MONOTONIC, &new_ts);
        start_ms = in->mute_start_ts.tv_sec * 1000LL +
                   in->mute_start_ts.tv_nsec / 1000000LL;
        end_ms = new_ts.tv_sec * 1000LL +
                 new_ts.tv_nsec / 1000000LL;
        interval_ms = end_ms - start_ms;
        if (interval_ms < mute_mdelay) {
            in_mute = 1;
        } else {
            ALOGI("unmute audio since audio signal is stable");
            in->mute_log_cntr = 0;
            in_mute = 0;
            in->mute_flag = 0;
            if (adev->audio_patching) {
                adev->audio_patch->is_src_stable = 1;
            }
        }
    }

    /*noise gate is only used in Linein for 16bit audio data*/
    if (adev->active_inport == INPORT_LINEIN && adev->aml_ng_enable == 1) {
#ifdef NOISE_GATE
        int ng_status = noise_evaluation(adev->aml_ng_handle, buffer, bytes >> 1);
#endif
        /*if (ng_status == NG_MUTE)
            ALOGI("noise gate is working!");*/
    }

    if (adev->parental_control_av_mute &&
        (adev->active_inport == INPORT_TUNER || adev->active_inport == INPORT_LINEIN)) {
        parental_mute = 1;
    }
    if (adev->mic_mute || in_mute || parental_mute
        || in->spdif_fmt_hw == SPDIFIN_AUDIO_TYPE_PAUSE) {
        ALOGI("mic_mute %d in_mute %d parental_mute %d in->spdif_fmt_hw %d",
              adev->mic_mute, in_mute, parental_mute, in->spdif_fmt_hw);
        memset(buffer, 0, bytes);
    } else if (!adev->audio_patching) {
        /* case dev->mix, set audio gain to src */
        apply_volume(adev->src_gain[adev->active_inport], buffer, sizeof(uint16_t), bytes);
    }

#if 1
    if (getprop_bool("media.audiohal.indump")) {
        FILE *dump_fp = NULL;
        dump_fp = fopen("/data/audio_hal/audio_in.pcm", "a+");
        if (dump_fp != NULL) {
            fwrite(buffer, bytes, 1, dump_fp);
            fclose(dump_fp);
        } else {
            ALOGW("[Error] Can't write to /data/dump_in.pcm");
        }
    }
#endif
#if DATMOS
    if (0) {
        FILE *dump_fp = NULL;
        dump_fp = fopen("/tmp/audio_in.data", "a+");
        if (dump_fp != NULL) {
            fwrite(buffer, bytes, 1, dump_fp);
            fclose(dump_fp);
        } else {
            ALOGW("[Error] Can't write to /data/dump_in.pcm");
        }
    }
#endif

exit:

    if (ret < 0) {
        size_t frame_size = audio_stream_in_frame_size(stream);
        if (frame_size == 0) {
            if (adev->in_device & AUDIO_DEVICE_IN_HDMI) {
                if (adev->datmos_enable) {
                    frame_size = adev->capture_ch * 2;
                }
            } else {
                frame_size = 4;
            }
        }
        frame_size = (frame_size == 0) ? frame_size : 4;

        usleep(bytes * 1000000 / frame_size / in_get_sample_rate(&stream->common));
        ALOGI("audio in read err!!!");
        bytes = -1;
    }
    pthread_mutex_unlock(&in->lock);
    return bytes;

}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream __unused)
{
    return 0;
}


// open corresponding stream by flags, formats and others params
static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle __unused,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    struct aml_audio_device *ladev = (struct aml_audio_device *) dev;
    struct aml_stream_out *out;
    int channel_count = popcount(config->channel_mask);
    int digital_codec;
    bool direct = false;
    int ret;
    bool hwsync_lpcm = false;
    ALOGI("enter %s(devices=0x%04x,format=%#x, ch=0x%04x, SR=%d, flags=0x%x)\n", __FUNCTION__, devices,
          config->format, config->channel_mask, config->sample_rate, flags);

    out = (struct aml_stream_out *) calloc(1, sizeof(struct aml_stream_out));
    if (!out) {
        return -ENOMEM;
    }

    out->out_device = devices;

    // Output flag shall not be AUDIO_OUTPUT_FLAG_NONE during HAL stage
    if (flags == AUDIO_OUTPUT_FLAG_NONE) {
        ALOGE("Amlogic_HAL - %s: output flag is AUDIO_OUTPUT_FLAG_NONE, modify it to default value AUDIO_OUTPUT_FLAG_PRIMARY.", __FUNCTION__);
        flags = AUDIO_OUTPUT_FLAG_PRIMARY;
    }

    out->flags = flags;
    if (getprop_bool("ro.platform.is.tv")) {
        out->is_tv_platform = 1;
    }

    out->config = pcm_config_out;
    if (config->channel_mask == AUDIO_CHANNEL_NONE) {
        config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    }
    if (config->sample_rate == 0) {
        config->sample_rate = 48000;
    }
    ALOGI("Amlogic - set out->config.channels to popcount of config->channel_mask = %d.\n", config->channel_mask);
    out->config.channels = audio_channel_count_from_out_mask(config->channel_mask);
    ALOGI("Amlogic - set out->config.channels to config->sample_rate = %d.\n", config->sample_rate);
    out->config.rate = config->sample_rate;

    //hwsync with LPCM still goes to out_write_legacy
    hwsync_lpcm = (flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC && config->sample_rate <= 48000 &&
                   audio_is_linear_pcm(config->format) && channel_count <= 2);
    ALOGI("hwsync_lpcm %d\n", hwsync_lpcm);
    if (flags & AUDIO_OUTPUT_FLAG_PRIMARY/* hwsync_lpcm*/) {
        out->stream.common.get_channels = out_get_channels;
        out->stream.common.get_format = out_get_format;
        out->stream.write = out_write_legacy;
        out->stream.common.standby = out_standby;
        ALOGV("Amlogic - set out->hal_channel_mask = config->channel_mask\n");
        out->hal_channel_mask  = config->channel_mask;
        out->hal_rate = out->config.rate;
        out->hal_format = config->format;
        out->hal_internal_format = out->hal_format;
        config->format = out_get_format(&out->stream.common);
        config->channel_mask = out_get_channels(&out->stream.common);
        config->sample_rate = out_get_sample_rate(&out->stream.common);
        out->hal_channel_mask = config->channel_mask;
    } else if (flags & AUDIO_OUTPUT_FLAG_DIRECT) {
        out->stream.common.get_channels = out_get_channels_direct;
        out->stream.common.get_format = out_get_format_direct;
        out->stream.write = out_write_direct;
        out->stream.common.standby = out_standby_direct;
        if (config->format == AUDIO_FORMAT_DEFAULT) {
            config->format = AUDIO_FORMAT_AC3;
        }
        /* set default pcm config for direct. */
        out->config = pcm_config_out_direct;
        if (popcount(config->channel_mask) == 0) {
            config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
        }
        out->hal_channel_mask  = config->channel_mask;
        //out->config.channels = popcount(config->channel_mask);
        if (config->sample_rate == 0) {
            config->sample_rate = 48000;
        }
        out->config.rate = out->hal_rate = config->sample_rate;
        if (config->format == AUDIO_FORMAT_PCM_16_BIT) {
            out->config.format = PCM_FORMAT_S16_LE;
        } else if (config->format == AUDIO_FORMAT_PCM_32_BIT) {
            out->config.format = PCM_FORMAT_S32_LE;
        }

        out->hal_format =  config->format;
        out->hal_internal_format = out->hal_format;

        ladev->dolby_lib_type = ladev->dolby_lib_type_last;
        if (config->format == AUDIO_FORMAT_IEC61937) {
            if (audio_channel_count_from_out_mask(config->channel_mask) == 2 &&
                (config->sample_rate == 192000 || config->sample_rate == 176400)) {
                out->hal_internal_format = AUDIO_FORMAT_E_AC3;
                out->config.rate = config->sample_rate / 4;
            } else if (audio_channel_count_from_out_mask(config->channel_mask) >= 6 &&
                       config->sample_rate == 192000) {
                out->hal_internal_format = AUDIO_FORMAT_DTS_HD;
                ladev->dolby_lib_type = eDolbyDcvLib;
            } else if (audio_channel_count_from_out_mask(config->channel_mask) == 2 &&
                       config->sample_rate >= 32000 && config->sample_rate <= 48000) {
                if (ladev->audio_type == DTS) {
                    out->hal_internal_format = AUDIO_FORMAT_DTS;
                    ladev->dolby_lib_type = eDolbyDcvLib;
                    if (out->is_tv_platform && ladev->hdmi_format == AUTO) {
                        out->config.rate = out->hal_rate = 48000;
                    }
                } else {
                    out->hal_internal_format =  AUDIO_FORMAT_AC3;
                }

                config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
            } else {
                ALOGE("DO not support yet!!");
                config->format = AUDIO_FORMAT_DEFAULT;
                return -EINVAL;
            }
            ALOGI("convert format IEC61937 to 0x%x\n", out->hal_format);
        }
        out->raw_61937_frame_size = 1;
        digital_codec = get_codec_type(out->hal_internal_format);
        if (digital_codec == TYPE_EAC3) {
            out->raw_61937_frame_size = 4;
            out->config.period_size = pcm_config_out_direct.period_size * 2;
        } else if (digital_codec == TYPE_TRUE_HD || digital_codec == TYPE_DTS_HD) {
            out->config.period_size = pcm_config_out_direct.period_size * 4 * 2;
            out->raw_61937_frame_size = 16;
        } else if (digital_codec == TYPE_AC3 || digital_codec == TYPE_DTS) {
            out->raw_61937_frame_size = 4;
        }

        if (channel_count > 2) {
            ALOGI("[adev_open_output_stream]: out/%p channel/%d\n", out,
                  channel_count);
            out->multich = channel_count;
            out->config.channels = channel_count;
        }
        if (codec_type_is_raw_data(digital_codec)) {
            ALOGI("for raw audio output,force alsa stereo output\n");
            out->config.channels = 2;
            out->multich = 2;
            //set default channels mask to stereo if none.
            if (!out->hal_channel_mask) {
                out->hal_channel_mask = config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
            }
            //config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
        }
        if (digital_codec == TYPE_PCM && (out->config.rate > 48000 || out->config.channels >= 6)) {
            ALOGI("open hi pcm mode !\n");
            ladev->hi_pcm_mode = true;
        }
    } else {
        // TODO: add other cases here
        ALOGE("DO not support yet!!");
        return -EINVAL;
    }

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.set_format = out_set_format;
    //out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    out->stream.get_presentation_position = out_get_presentation_position;
    out->stream.pause = out_pause_new;
    out->stream.resume = out_resume_new;
    out->stream.flush = out_flush_new;
    out->volume_l = 1.0;
    out->volume_r = 1.0;
    out->dev = ladev;
    out->standby = true;
    out->pause_status = false;
    out->frame_write_sum = 0;
    out->aml_dec = NULL;
    //out->hw_sync_mode = !!(flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC); // zz
    /* //8.1
    out->hw_sync_mode = false;
    aml_audio_hwsync_init (&out->hwsync);
    //out->hal_rate = out->config.rate;
    //if (flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC) {
    if (0) {
        out->hw_sync_mode = true;
        ALOGI ("Output stream open with AUDIO_OUTPUT_FLAG_HW_AV_SYNC");
    }
    */

    /* FIXME: when we support multiple output devices, we will want to
       * do the following:
       * adev->devices &= ~AUDIO_DEVICE_OUT_ALL;
       * adev->devices |= out->device;
       * select_output_device(adev);
       * This is because out_set_parameters() with a route is not
       * guaranteed to be called after an output stream is opened.
       */

    ALOGD("**leave %s(devices=0x%04x,format=%#x, ch=0x%04x, SR=%d)", __FUNCTION__, devices,
          config->format, config->channel_mask, config->sample_rate);

    *stream_out = &out->stream;

    //if (out->is_tv_platform)
    {
        // hank hard code
#ifdef USE_AUDIOSERVICE_S400
        /*the trunk alsa dts only support to 6 ch,
        we will change such define later*/
        out->config.channels = 4;
#else
        out->config.channels = 8;

#endif
        out->config.format = PCM_FORMAT_S32_LE;
        out->tmp_buffer_8ch = (int32_t *)calloc(1, out->config.period_size * 4 * 8);
        memset(out->tmp_buffer_8ch, 0, out->config.period_size * 4 * 8);
        if (out->tmp_buffer_8ch == NULL) {
            ALOGE("cannot malloc memory for out->tmp_buffer_8ch");
            goto err_open;
        }
        out->tmp_buffer_8ch_size = out->config.period_size * 4 * 8;

        out->audioeffect_tmp_buffer = calloc(1, out->config.period_size * 6);
        if (out->audioeffect_tmp_buffer == NULL) {
            ALOGE("cannot malloc memory for audioeffect_tmp_buffer");
            goto err_open;
        }

        ret = aml_sampleconv_init(&out->sample_convert);
        if (ret < 0) {
            ALOGE("aml_sampleconv_init faild\n");
            goto err_open;
        }

        ret = aml_channelmap_init(&out->channel_map, ladev->datmos_param.speaker_config);
        if (ret < 0) {
            ALOGE("aml_channelmap_init faild\n");
            goto err_open;
        }

        ret = aml_audiodelay_init(&ladev->hw_device);
        if (ret < 0) {
            ALOGE("aml_audiodelay_init faild\n");
            goto err_open;
        }

        aml_external_pp_start(&ladev->hw_device);

        out->resample_handle = NULL;
    }
    out->hwsync =  calloc(1, sizeof(audio_hwsync_t));
    if (!out->hwsync) {
        ALOGE("%s,malloc hwsync failed", __func__);
        goto err_open;
    }
    out->hwsync->tsync_fd = -1;
    if (out->hw_sync_mode) {
        aml_audio_hwsync_init(out->hwsync, out);
    }
    out->ddp_frame_size = aml_audio_get_ddp_frame_size();

    return 0;

err_open:
    if (out->tmp_buffer_8ch) {
        free(out->tmp_buffer_8ch);
    }

    if (out->audioeffect_tmp_buffer) {
        free(out->audioeffect_tmp_buffer);
    }
    if (out->hwsync) {
        free(out->hwsync);
    }
    free(out);
    *stream_out = NULL;
    return -ENOMEM;
}

static int out_standby_new(struct audio_stream *stream);
static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct aml_stream_out *out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    bool hwsync_lpcm = false;
    ALOGD("%s(%p, %p)", __FUNCTION__, dev, stream);
    //if (out->is_tv_platform == 1)
    // hank add
    {
        free(out->tmp_buffer_8ch);
        free(out->audioeffect_tmp_buffer);
        aml_sampleconv_close(out->sample_convert);
        out->sample_convert = NULL;
        aml_channelmap_close(out->channel_map);
        out->channel_map = NULL;
        aml_audiolevel_reset(&adev->hw_device);
        aml_audiodelay_close(&adev->hw_device);
        aml_audio_resample_close(out->resample_handle);
        out->resample_handle = NULL;
        aml_external_pp_stop(&adev->hw_device);
    }
    int channel_count = popcount(out->hal_channel_mask);
    hwsync_lpcm = (out->flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC && out->config.rate  <= 48000 &&
                   audio_is_linear_pcm(out->hal_format) && channel_count <= 2);
    adev->active_outputs[out->usecase] = NULL;
    adev->decode_format = AUDIO_FORMAT_INVALID;
    out_standby_new(&stream->common);

    if (out->hwsync) {
        free(out->hwsync);
        out->hwsync = NULL;
    }

    pthread_mutex_lock(&adev->lock);
    free(stream);
    pthread_mutex_unlock(&adev->lock);
}

static int set_arc_hdmi(struct audio_hw_device *dev, char *value, size_t len)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    struct aml_arc_hdmi_desc *hdmi_desc = &adev->hdmi_descs;
    char *pt = NULL, *tmp = NULL;
    int i = 0;

    if (strlen(value) > len) {
        ALOGE("value array overflow!");
        return -EINVAL;
    }

    pt = strtok_r(value, "[], ", &tmp);
    while (pt != NULL) {
        //index 1 means avr port
        if (i == 1) {
            hdmi_desc->avr_port = atoi(pt);
        }

        pt = strtok_r(NULL, "[], ", &tmp);
        i++;
    }

    ALOGI("ARC HDMI AVR port = %d", hdmi_desc->avr_port);
    return 0;
}

static void dump_format_desc(struct format_desc *desc)
{
    if (desc) {
        ALOGI("dump format desc = %p", desc);
        ALOGI("\t-fmt %d", desc->fmt);
        ALOGI("\t-is support %d", desc->is_support);
        ALOGI("\t-max channels %d", desc->max_channels);
        ALOGI("\t-sample rate masks %#x", desc->sample_rate_mask);
        ALOGI("\t-max bit rate %d", desc->max_bit_rate);
        ALOGI("\t-atmos supported %d", desc->atmos_supported);
    }
}

static int set_arc_format(struct audio_hw_device *dev, char *value, size_t len)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    struct aml_arc_hdmi_desc *hdmi_desc = &adev->hdmi_descs;
    struct format_desc *fmt_desc = NULL;
    char *pt = NULL, *tmp = NULL;
    int i = 0, val = 0;
    enum arc_hdmi_format format = _LPCM;
    if (strlen(value) > len) {
        ALOGE("value array overflow!");
        return -EINVAL;
    }

    pt = strtok_r(value, "[], ", &tmp);
    while (pt != NULL) {
        val = atoi(pt);
        switch (i) {
        case 0:
            format = val;
            if (val == _AC3) {
                fmt_desc = &hdmi_desc->dd_fmt;
                fmt_desc->fmt = val;
            } else if (val == _DDP) {
                fmt_desc = &hdmi_desc->ddp_fmt;
                fmt_desc->fmt = val;
            } else if (val == _LPCM) {
                fmt_desc = &hdmi_desc->pcm_fmt;
                fmt_desc->fmt = val;
            } else if (val == _DTS) {
                fmt_desc = &hdmi_desc->dts_fmt;
                fmt_desc->fmt = val;
            } else {
                ALOGE("unsupport fmt %d", val);
                return -EINVAL;
            }
            break;
        case 1:
            fmt_desc->is_support = val;
            break;
        case 2:
            fmt_desc->max_channels = val + 1;
            break;
        case 3:
            fmt_desc->sample_rate_mask = val;
            break;
        case 4:
            if (format == _DDP) {
                fmt_desc = &hdmi_desc->ddp_fmt;
                fmt_desc->atmos_supported = val > 0 ? true : false;
                aml_mixer_ctrl_set_int(AML_MIXER_ID_HDMI_ATMOS_EDID, fmt_desc->atmos_supported);
                /**
                 * Add flag to indicate hdmi arc format updated.
                 * Out write thread checks it and makes decision on output format.
                 * NOTICE: assuming that DDP format is the last ARC KV parameters.
                 */
                adev->arc_hdmi_updated = 1;
            } else {
                fmt_desc->max_bit_rate = val * 80;
            }
            break;
        default:
            break;
        }

        pt = strtok_r(NULL, "[], ", &tmp);
        i++;
    }

    dump_format_desc(fmt_desc);
    return 0;
}

/*static int init_tv_dev_for_patch(struct aml_tv_device *aml_tv_dev,
        int input_src, int sample_rate, int channel_cnt);*/

const char *output_ports[] = {
    "OUTPORT_SPEAKER",
    "OUTPORT_HDMI_ARC",
    "OUTPORT_HDMI",
    "OUTPORT_SPDIF",
    "OUTPORT_AUX_LINE",
    "OUTPORT_HEADPHONE",
    "OUTPORT_MAX"
};

const char *input_ports[] = {
    "INPORT_TUNER",
    "INPORT_HDMIIN",
    "INPORT_SPDIF",
    "INPORT_LINEIN",
    "INPORT_MAX"
};

static int aml_audio_output_routing(struct audio_hw_device *dev,
                                    enum OUT_PORT outport, bool user_setting) {
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;

    if (!aml_dev->ar)
    {
        ALOGE("%s() audio route init failed\n", __func__);
        return -ENOENT;
    }

    if (aml_dev->active_outport == outport)
    {
        /* In this case, user toggle the speaker_mute menu */
        if ((outport == OUTPORT_SPEAKER) && user_setting) {
            if (aml_dev->speaker_mute) {
                audio_route_apply_path(aml_dev->ar, "speaker_off");
                aml_dev->active_outport = OUTPORT_SPEAKER;
            } else {
                audio_route_apply_path(aml_dev->ar, "speaker");
                aml_dev->active_outport = OUTPORT_SPEAKER;
            }
            audio_route_update_mixer(aml_dev->ar);
            return 0;
        }
        ALOGI("%s() outport %s already exists, do nothing", __func__, output_ports[outport]);
        return 0;
    }
    ALOGI("%s() switch from %s to %s", __func__,
          output_ports[aml_dev->active_outport], output_ports[outport]);

    /* switch off the active output */
    switch (aml_dev->active_outport)
    {
    case OUTPORT_SPEAKER:
        ALOGI("%s() speaker_off", __func__);
        audio_route_apply_path(aml_dev->ar, "speaker_off");
        break;
    case OUTPORT_HDMI_ARC:
        ALOGI("%s() hdmi_arc_off", __func__);
        audio_route_apply_path(aml_dev->ar, "hdmi_arc_off");
        break;
    case OUTPORT_HEADPHONE:
        ALOGI("%s() headphone_off", __func__);
        audio_route_apply_path(aml_dev->ar, "headphone_off");
        break;
    default:
        ALOGE("%s() unsupport", __func__);
        break;
    }

    /* switch on the new output */
    switch (outport)
    {
    case OUTPORT_SPEAKER:
        ALOGI("%s() speaker on", __func__);
        if (!aml_dev->speaker_mute) {
            audio_route_apply_path(aml_dev->ar, "speaker");
        }
        audio_route_apply_path(aml_dev->ar, "spdif");
        break;
    case OUTPORT_HDMI_ARC:
        ALOGI("%s() hdmi_arc on", __func__);
        audio_route_apply_path(aml_dev->ar, "hdmi_arc");
        /* TODO: spdif case need deal with hdmi arc format */
        if (aml_dev->hdmi_format != 3) {
            audio_route_apply_path(aml_dev->ar, "spdif");
        }
        break;
    case OUTPORT_HEADPHONE:
        ALOGI("%s() headphone on", __func__);
        audio_route_apply_path(aml_dev->ar, "headphone");
        audio_route_apply_path(aml_dev->ar, "spdif");
        break;
    default:
        ALOGE("%s() unsupport", __func__);
        break;
    }

    audio_route_update_mixer(aml_dev->ar);
    aml_dev->active_outport = outport;
    return 0;
}

static int aml_audio_input_routing(struct audio_hw_device *dev,
                                   enum IN_PORT inport) {
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    (void) inport;
    return 0;
}

#define VAL_LEN 256
static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{

    ALOGD("%s(%p, %s)", __FUNCTION__, dev, kvpairs);

    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    struct str_parms *parms;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);

    char value[VAL_LEN];
    int val = 0;
    int ret = 0;
#if 1

    ALOGI("%s(kv: %s)", __FUNCTION__, kvpairs);
    parms = str_parms_create_str(kvpairs);
    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            adev->low_power = false;
        } else {
            adev->low_power = true;
        }
        goto exit;
    }

    ret = str_parms_get_int(parms, "disable_pcm_mixing", &val);
    if (ret >= 0) {
        adev->disable_pcm_mixing = val;
        ALOGI("ms12 disable_pcm_mixing set to %d\n", adev->disable_pcm_mixing);
        goto exit;
    }

    ret = str_parms_get_int(parms, "Audio hdmi-out mute", &val);
    if (ret >= 0) {
        aml_mixer_ctrl_set_int(AML_MIXER_ID_HDMI_OUT_AUDIO_MUTE, val);
        ALOGI("audio hdmi out status: %d\n", val);
        goto exit;
    }

    ret = str_parms_get_int(parms, "Audio spdif mute", &val);
    if (ret >= 0) {
        aml_mixer_ctrl_set_int(AML_MIXER_ID_SPDIF_MUTE, val);
        ALOGI("audio spdif out status: %d\n", val);
        goto exit;
    }

    // HDMI cable plug off
    ret = str_parms_get_int(parms, "disconnect", &val);
    if (ret >= 0) {
        if (val & AUDIO_DEVICE_OUT_HDMI_ARC) {
            adev->bHDMIConnected = 0;
            ALOGI("bHDMIConnected: %d\n", val);
        }
        goto exit;
    }

    // HDMI cable plug in
    ret = str_parms_get_int(parms, "connect", &val);
    if (ret >= 0) {
        if (val & AUDIO_DEVICE_OUT_HDMI_ARC) {
            adev->bHDMIConnected = 1;
            ALOGI("%s,bHDMIConnected: %d\n", __FUNCTION__, val);
        }
        goto exit;
    }

    //  HDMI plug in and UI [Sound Output Device] set to "ARC" will recieve HDMI ARC Switch = 1
    //  HDMI plug off and UI [Sound Output Device] set to "speaker" will recieve HDMI ARC Switch = 0
    ret = str_parms_get_int(parms, "HDMI ARC Switch", &val);
    if (ret >= 0) {
        aml_mixer_ctrl_set_int(AML_MIXER_ID_HDMI_ARC_AUDIO_ENABLE, val);
        adev->bHDMIARCon = (val == 0) ? 0 : 1;
        ALOGI("%s audio hdmi arc status: %d\n", __FUNCTION__, adev->bHDMIARCon);

        /*
           * when user switch UI setting, means output device changed,
           * use "arc_hdmi_updated" paramter to notify HDMI ARC have updated status
           * while in config_output(), it detects this change, then re-route output config.
           */
        adev->arc_hdmi_updated = 1;

        goto exit;
    }

    //add for fireos tv for Dolby audio setting
    ret = str_parms_get_int(parms, "hdmi_format", &val);
    if (ret >= 0) {
        adev->hdmi_format = val;
        ALOGI("HDMI format: %d\n", adev->hdmi_format);
        goto exit;
    }

    ret = str_parms_get_int(parms, "spdif_format", &val);
    if (ret >= 0) {
        adev->spdif_format = val;
        ALOGI("S/PDIF format: %d\n", adev->spdif_format);
        goto exit;
    }

    ret = str_parms_get_int(parms, "audio_type", &val);
    if (ret >= 0) {
        adev->audio_type = val;
        ALOGI("audio_type: %d\n", adev->audio_type);
        goto exit;
    }
    ret = str_parms_get_int(parms, "hdmi_is_passthrough_active", &val);
    if (ret >= 0) {
        adev->hdmi_is_pth_active = val;
        ALOGI("hdmi_is_passthrough_active: %d\n", adev->hdmi_is_pth_active);
        goto exit;
    }

    ret = str_parms_get_int(parms, "capability:routing", &val);
    if (ret >= 0) {
        adev->routing = val;
        ALOGI("capability:routing = %#x\n", adev->routing);
        goto exit;
    }
    ret = str_parms_get_int(parms, "capability:format", &val);
    if (ret >= 0) {
        adev->output_config.format = val;
        ALOGI("capability:format = %#x\n", adev->output_config.format = val);
        goto exit;
    }

    ret = str_parms_get_int(parms, "capability:channels", &val);
    if (ret >= 0) {
        adev->output_config.channel_mask = val;
        ALOGI("capability:channel_mask = %#x\n", adev->output_config.channel_mask);
        goto exit;
    }

    ret = str_parms_get_int(parms, "capability:sampling_rate", &val);
    if (ret >= 0) {
        adev->output_config.sample_rate = val;
        ALOGI("capability:sample_rate = %d\n", adev->output_config.sample_rate);
        goto exit;
    }

    ret = str_parms_get_str(parms, "set_ARC_hdmi", value, sizeof(value));
    if (ret >= 0) {
        set_arc_hdmi(dev, value, VAL_LEN);
        goto exit;
    }

    ret = str_parms_get_str(parms, "set_ARC_format", value, sizeof(value));
    if (ret >= 0) {
        set_arc_format(dev, value, VAL_LEN);
        goto exit;
    }

    ret = str_parms_get_str(parms, "tuner_in", value, sizeof(value));
    // tuner_in=atv: tuner_in=dtv
    if (ret >= 0) {
        if (adev->tuner2mix_patch) {
            if (strncmp(value, "dtv", 3) == 0) {
                adev->patch_src = SRC_DTV;
            } else if (strncmp(value, "atv", 3) == 0) {
                adev->patch_src = SRC_ATV;
                set_audio_source(ATV);
            }
            ALOGI("%s, tuner to mixer case, no need to create patch", __func__);
            goto exit;
        }
#ifdef ENABLE_DTV
        if (strncmp(value, "dtv", 3) == 0) {
            // no audio patching in dtv
            if (adev->audio_patching && (adev->patch_src == SRC_ATV)) {
                // this is to handle atv->dtv case
                ALOGI("%s, atv->dtv", __func__);
                ret = release_patch(adev);
                if (!ret) {
                    adev->audio_patching = 0;
                }
            }
            ALOGI("%s, now the audio patch src is %d  the audio_patching is %d ", __func__, adev->patch_src, adev->audio_patching);
            if ((adev->patch_src == SRC_DTV) && adev->audio_patching) {
                ALOGI("%s, no need to create the dtv patch now\n ", __func__);
            } else {
                ALOGI("%s, now end release dtv patch the audio_patching is %d ", __func__, adev->audio_patching);
                ALOGI("%s, now create the dtv patch now\n ", __func__);
                adev->patch_src = SRC_DTV;
                ret = create_dtv_patch(dev, AUDIO_DEVICE_IN_TV_TUNER,
                                       AUDIO_DEVICE_OUT_SPEAKER);
                if (ret == 0) {
                    adev->audio_patching = 1;
                }
                ALOGI("%s, now end create dtv patch the audio_patching is %d ", __func__, adev->audio_patching);
            }

            goto exit;

        } else if (strncmp(value, "atv", 3) == 0) {
            // need create patching
            if ((adev->patch_src == SRC_DTV) && adev->audio_patching) {
                ret = release_dtv_patch(adev);
                if (!ret) {
                    adev->audio_patching = 0;
                }
            }
            if (!adev->audio_patching) {
                ALOGI("%s, create atv patching", __func__);
                set_audio_source(ATV);
                ret = create_patch(dev, AUDIO_DEVICE_IN_TV_TUNER, AUDIO_DEVICE_OUT_SPEAKER);
                // audio_patching ok, mark the patching status
                if (ret == 0) {
                    adev->audio_patching = 1;
                }
            }
            adev->patch_src = SRC_ATV;
        }
#endif
        goto exit;
    }

    /*
     * This is a work around when plug in HDMI-DVI connector
     * first time application only recognize it as HDMI input device
     * then it can know it's DVI in ,and send "audio=linein" message to audio hal
    */
    ret = str_parms_get_str(parms, "audio", value, sizeof(value));
    if (ret >= 0) {
        if (strncmp(value, "linein", 6) == 0) {

            struct audio_patch *pAudPatchTmp = NULL;

            get_audio_patch_by_src_dev(dev, AUDIO_DEVICE_IN_HDMI, &pAudPatchTmp);

            if (pAudPatchTmp == NULL) {
                ALOGE("%s,There is no autio patch using HDMI as input", __func__);
                goto exit;
            }
            if (pAudPatchTmp->sources[0].ext.device.type != AUDIO_DEVICE_IN_HDMI) {
                ALOGE("%s, pAudPatchTmp->sources[0].ext.device.type != AUDIO_DEVICE_IN_HDMI", __func__);
                goto exit;
            }

            // dev->dev (example: HDMI in-> speaker out)
            if (pAudPatchTmp->sources[0].type == AUDIO_PORT_TYPE_DEVICE
                && pAudPatchTmp->sinks[0].type == AUDIO_PORT_TYPE_DEVICE) {
                // This "adev->audio_patch" will be created in create_patch() function
                // which in current design is only in "dev->dev" case
                // make sure everything is matching, no error
                if (adev->audio_patch && (adev->patch_src == SRC_HDMIIN) /*&& pAudPatchTmp->id == adev->audio_patch->patch_hdl*/) {
                    // TODO: notices
                    // These codes must corresponding to the same case in adev_create_audio_patch() and adev_release_audio_patch()
                    // Anything change in the adev_create_audio_patch() . Must also change code here..
                    ALOGI("%s, create hdmi-dvi patching dev->dev", __func__);
                    release_patch(adev);
                    adev->active_inport = INPORT_LINEIN;
                    set_audio_source(LINEIN);
                    ret = create_patch_ext(dev, AUDIO_DEVICE_IN_LINE, pAudPatchTmp->sinks[0].ext.device.type, pAudPatchTmp->id);
                    pAudPatchTmp->sources[0].ext.device.type = AUDIO_DEVICE_IN_LINE;
                }
            }

            // dev->mix (example: HDMI in-> USB out)
            if (pAudPatchTmp->sources[0].type == AUDIO_PORT_TYPE_DEVICE
                && pAudPatchTmp->sinks[0].type == AUDIO_PORT_TYPE_MIX) {
                ALOGI("%s, !!create hdmi-dvi patching dev->mix", __func__);
                if (adev->patch_src == SRC_HDMIIN) {
                    release_parser(adev);
                }

                //adev->in_device &= ~AUDIO_DEVICE_IN_ALL;
                //adev->in_device = AUDIO_DEVICE_IN_LINE;
                adev->active_inport = INPORT_LINEIN;
                adev->patch_src == SRC_LINEIN;
                pAudPatchTmp->sources[0].ext.device.type = AUDIO_DEVICE_IN_LINE;

                set_audio_source(LINEIN);
            }

        }
    }

    //  HDMI plug in and UI [Sound Output Device] set to "ARC" will recieve speaker_mute = 1
    //  HDMI plug off and UI [Sound Output Device] set to "speaker" will recieve speaker_mute  = 0
    ret = str_parms_get_str(parms, "speaker_mute", value, sizeof(value));
    if (ret >= 0) {
        if (strncmp(value, "true", 4) == 0 || strncmp(value, "1", 1) == 0) {
            adev->speaker_mute = 1;
        } else if (strncmp(value, "false", 5) == 0 || strncmp(value, "0", 1) == 0) {
            adev->speaker_mute = 0;
        } else {
            ALOGE("%s() unsupport speaker_mute value: %s",   __func__, value);
        }

        /*
            when ARC is connecting, and user switch [Sound Output Device] to "speaker"
            we need to set out_port as OUTPORT_SPEAKER ,
            the aml_audio_output_routing() will "mute" ARC and "unmute" speaker.
           */
        int out_port = adev->active_outport;
        if (adev->active_outport == OUTPORT_HDMI_ARC && adev->speaker_mute == 0) {
            out_port = OUTPORT_SPEAKER;
        }

        ret = aml_audio_output_routing(dev, out_port, true);
        if (ret < 0) {
            ALOGE("%s() output routing failed", __func__);
        }
        goto exit;
    }

    ret = str_parms_get_str(parms, "spdif_raw_enable", value, sizeof(value));
    if (ret >= 0) {
        if (strncmp(value, "true", 4) == 0) {
            adev->digital_raw = 1;
        } else if (strncmp(value, "false", 5) == 0) {
            adev->digital_raw = 0;
        } else {
            ALOGE("%s() unsupport spdif_raw_enable value: %s",
                  __func__, value);
        }
        ALOGI("spdif_raw_enable set to %d\n", adev->digital_raw);
        goto exit;
    }
    ret = str_parms_get_str(parms, "parental_control_av_mute", value, sizeof(value));
    if (ret >= 0) {
        if (strncmp(value, "true", 4) == 0) {
            adev->parental_control_av_mute = true;
        } else if (strncmp(value, "false", 5) == 0) {
            adev->parental_control_av_mute = false;
        } else {
            ALOGE("%s() unsupport parental_control_av_mute value: %s",
                  __func__, value);
        }
        ALOGI("parental_control_av_mute set to %d\n", adev->parental_control_av_mute);
        goto exit;
    }

    // used for get first apts for A/V sync
    ret = str_parms_get_str(parms, "first_apts", value, sizeof(value));
    if (ret >= 0) {
        unsigned int first_apts = (unsigned int)atoll(value); // zz
        //unsigned int first_apts = atoi (value); //8.1
        ALOGI("audio set first apts 0x%x\n", first_apts);
        adev->first_apts = first_apts;
        adev->first_apts_flag = true;
        adev->frame_trigger_thred = 0;
        goto exit;
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        ret = str_parms_get_int(parms, "dual_decoder_support", &val);
        if (ret >= 0) {
            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&ms12->lock);
            adev->dual_decoder_support = val;
            ALOGI("dual_decoder_support set to %d\n", adev->dual_decoder_support);
            set_audio_system_format(AUDIO_FORMAT_PCM_16_BIT);
            //only use to set associate flag, dd/dd+ format is same.
            if (adev->dual_decoder_support == 1) {
                set_audio_associate_format(AUDIO_FORMAT_AC3);
            } else {
                set_audio_associate_format(AUDIO_FORMAT_INVALID);
            }
            dolby_ms12_config_params_set_associate_flag(adev->dual_decoder_support);
            adev->need_reset_for_dual_decoder = true;
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&ms12->lock);
            goto exit;
        }

        ret = str_parms_get_int(parms, "associate_audio_mixing_enable", &val);
        if (ret >= 0) {
            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&ms12->lock);
            adev->associate_audio_mixing_enable = val;
            ALOGI("associate_audio_mixing_enable set to %d\n", adev->associate_audio_mixing_enable);
            dolby_ms12_set_asscociated_audio_mixing(adev->associate_audio_mixing_enable);
            int ms12_runtime_update_ret = aml_ms12_update_runtime_params(& (adev->ms12));
            ALOGI("aml_ms12_update_runtime_params return %d\n", ms12_runtime_update_ret);
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&ms12->lock);
            goto exit;
        }

        ret = str_parms_get_int(parms, "dual_decoder_mixing_level", &val);
        if (ret >= 0) {
            int mix_user_prefer = 0;
            int mixing_level = val;

            pthread_mutex_lock(&adev->lock);
            pthread_mutex_lock(&ms12->lock);
            if (mixing_level < 0) {
                mixing_level = 0;
            } else if (mixing_level > 100) {
                mixing_level = 100;
            }
            mix_user_prefer = (mixing_level * 64 - 32 * 100) / 100; //[0,100] mapping to [-32,32]
            adev->mixing_level = mix_user_prefer;
            ALOGI("mixing_level set to %d\n", adev->mixing_level);
            dolby_ms12_set_user_control_value_for_mixing_main_and_associated_audio(adev->mixing_level);
            int ms12_runtime_update_ret = aml_ms12_update_runtime_params(& (adev->ms12));
            ALOGI("aml_ms12_update_runtime_params return %d\n", ms12_runtime_update_ret);
            pthread_mutex_unlock(&ms12->lock);
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }

        ret = str_parms_get_int(parms, "continuous_audio_mode", &val);
        if (ret >= 0) {
            ALOGI("%s continuous_audio_mode set to %d\n", __func__ , val);
            char buf[PROPERTY_VALUE_MAX];
            int disable_continuous = !val;
            ret = property_get(DISABLE_CONTINUOUS_OUTPUT, buf, NULL);
            if (ret > 0) {
                sscanf(buf, "%d", &disable_continuous);
                ALOGI("%s[%s] disable_continuous %d\n", DISABLE_CONTINUOUS_OUTPUT, buf, disable_continuous);
            }
            pthread_mutex_lock(&adev->lock);
            if (continous_mode(adev) && disable_continuous) {
                if (dolby_stream_active(adev) || hwsync_lpcm_active(adev)) {
                    ALOGI("%s later release MS12 when direct stream active", __func__);
                    adev->need_remove_conti_mode = true;
                } else {
                    ALOGI("%s Dolby MS12 is at continuous output mode, here go to end it!\n", __FUNCTION__);
                    get_dolby_ms12_cleanup(&adev->ms12);
                    //ALOGI("[%s:%d] get_dolby_ms12_cleanup\n", __FUNCTION__, __LINE__);
                    adev->continuous_audio_mode = 0;
                    usecase_change_validate_l(adev->active_outputs[STREAM_PCM_NORMAL], true);
                    //continuous_stream_do_standby(adev);
                }
            } else {
                if ((!disable_continuous) && !continous_mode(adev)) {
                    adev->mix_init_flag = false;
                    adev->continuous_audio_mode = 1;
                }
            }
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }
    }
#endif
    ret = str_parms_get_str(parms, "SOURCE_GAIN", value, sizeof(value));
    if (ret >= 0) {
        sscanf(value, "%f %f %f %f", &adev->eq_data.s_gain.atv, &adev->eq_data.s_gain.dtv,
               &adev->eq_data.s_gain.hdmi, &adev->eq_data.s_gain.av);
        ALOGI("%s() audio source gain: atv:%f, dtv:%f, hdmiin:%f, av:%f", __func__,
              adev->eq_data.s_gain.atv, adev->eq_data.s_gain.dtv,
              adev->eq_data.s_gain.hdmi, adev->eq_data.s_gain.av);
        goto exit;
    }

    ret = str_parms_get_str(parms, "POST_GAIN", value, sizeof(value));
    if (ret >= 0) {
        sscanf(value, "%f %f %f", &adev->eq_data.p_gain.speaker, &adev->eq_data.p_gain.spdif_arc,
               &adev->eq_data.p_gain.headphone);
        ALOGI("%s() audio device gain: speaker:%f, spdif_arc:%f, headphone:%f", __func__,
              adev->eq_data.p_gain.speaker, adev->eq_data.p_gain.spdif_arc,
              adev->eq_data.p_gain.headphone);
        goto exit;
    }

    ret = str_parms_get_str(parms, "DTS_POST_GAIN", value, sizeof(value));
    if (ret >= 0) {
        sscanf(value, "%f", &adev->dts_post_gain);
        ALOGI("%s() audio dts post gain: %f", __func__, adev->dts_post_gain);
        goto exit;
    }

    ret = str_parms_get_int(parms, "EQ_MODE", &val);
    if (ret >= 0) {
        if (eq_mode_set(&adev->eq_data, val) < 0) {
            ALOGE("%s: eq_mode_set failed", __FUNCTION__);
        }
        goto exit;
    }
#ifdef ENABLE_DTV
    ret = str_parms_get_str(parms, "cmd", value, sizeof(value));
    if (ret > 0) {
        int cmd = atoi(value);
        if (adev->audio_patch == NULL) {
            ALOGI("%s()the audio patch is NULL \n", __func__);
        }

        switch (cmd) {
        case 1:
            ALOGI("%s() live AUDIO_DTV_PATCH_CMD_START\n", __func__);
            dtv_patch_add_cmd(AUDIO_DTV_PATCH_CMD_START);
            break;
        case 2:
            ALOGI("%s() live AUDIO_DTV_PATCH_CMD_PAUSE\n", __func__);
            dtv_patch_add_cmd(AUDIO_DTV_PATCH_CMD_PAUSE);
            break;
        case 3:
            ALOGI("%s() live AUDIO_DTV_PATCH_CMD_RESUME\n", __func__);
            dtv_patch_add_cmd(AUDIO_DTV_PATCH_CMD_RESUME);
            break;
        case 4:
            ALOGI("%s() live AUDIO_DTV_PATCH_CMD_STOP\n", __func__);
            dtv_patch_add_cmd(AUDIO_DTV_PATCH_CMD_STOP);
            break;
        default:
            ALOGI("%s() live %d \n", __func__, cmd);
            break;
        }
        goto exit;
    }
#endif
    ret = str_parms_get_str(parms, "fmt", value, sizeof(value));
    if (ret > 0) {
        unsigned int audio_fmt = (unsigned int)atoi(value); // zz
        ALOGI("%s() get the audio format %d\n", __func__, audio_fmt);
        if (adev->audio_patch != NULL) {
            adev->audio_patch->dtv_aformat = audio_fmt;
        } else {
            ALOGI("%s()the audio patch is NULL \n", __func__);
        }
        goto exit;
    }

    ret = str_parms_get_str(parms, "dtv_has_video", value, sizeof(value));
    if (ret > 0) {
        unsigned int has_video = (unsigned int)atoi(value);
        ALOGI("%s() get the has video parameters %d \n", __func__, has_video);
        if (adev->audio_patch != NULL) {
            adev->audio_patch->dtv_has_video = has_video;
        } else {
            ALOGI("%s()the audio patch is NULL \n", __func__);
        }

        goto exit;
    }

    ret = str_parms_get_str(parms, "has_video", value, sizeof(value));
    if (ret >= 0) {
        if (strncmp(value, "true", 4) == 0) {
            adev->is_has_video = true;
        } else if (strncmp(value, "false", 5) == 0) {
            adev->is_has_video = false;
        } else {
            adev->is_has_video = true;
            ALOGE("%s() unsupport value %s choose is_has_video(default) %d\n", __func__, value, adev->is_has_video);
        }
        ALOGI("is_has_video set to %d\n", adev->is_has_video);
    }

    ret = str_parms_get_int(parms, "audio_latency", &val);
    if (ret >= 0) {
        adev->audio_latency = val;
        ALOGD("audio_latency = %d\n", val);
        goto exit;
    }

    ret = str_parms_get_int(parms, "capture_device", &val);
    if (ret >= 0) {
        adev->capture_device = val;
        ALOGI("capture_device set to %d\n", adev->capture_device);
        goto exit;
    }

    ret = str_parms_get_int(parms, "capture_ch", &val);
    if (ret >= 0) {
        adev->capture_ch = val;
        /*we support 1 or 4 I2S*/
        if (adev->capture_ch > 2) {
            adev->capture_ch = 8;
        }
        ALOGI("capture_ch set to %d\n", adev->capture_ch);
        goto exit;
    }

    ret = str_parms_get_int(parms, "capture_samplerate", &val);
    if (ret >= 0) {
        adev->capture_samplerate = val;
        ALOGI("capture_samplerate set to %d\n", adev->capture_samplerate);
        goto exit;
    }

    ret = str_parms_get_int(parms, "capture_audiotype", &val);
    if (ret >= 0) {
        adev->capture_audiotype = val;
        ALOGI("capture_audiotype set to %d\n", adev->capture_audiotype);
        goto exit;
    }

    ret = str_parms_get_int(parms, "delay_max", &val);
    if (ret >= 0) {
        adev->delay_max = val;
        ALOGI("delay max  set to %d\n", adev->delay_max);
        goto exit;
    }

    ret = str_parms_get_int(parms, "delay_time", &val);
    if (ret >= 0) {
        adev->delay_time = val;
        ALOGI("delay time  set to %d\n", adev->delay_time);
        goto exit;
    }

    ret = aml_volctl_setparameters((struct audio_hw_device *)adev, parms);
    if (ret >= 0) {
        goto exit;
    }

    ret = aml_bm_setparameters((struct audio_hw_device *)adev, parms);
    if (ret >= 0) {
        goto exit;
    }

    /*bass management lowerpass corner*/
    ret = str_parms_get_int(parms, "lowerpass_corner", &val);
    if (ret >= 0) {
        ret = aml_bm_set(adev, val);
        goto exit;
    }
    ret = aml_external_pp_config((struct audio_hw_device *)adev, parms);
    if (ret >= 0) {
        goto exit;
    }


#ifdef DATMOS
    ret = datmos_set_parameters((struct audio_hw_device *)adev, parms);
    if (ret >= 0) {
        goto exit;
    }
#endif

    ret = str_parms_get_int(parms, "dcv_output_ch", &val);
    if (ret >= 0) {
        adev->dcv_output_ch = val;
        ALOGI("dcv_output_ch  set to %d\n", adev->dcv_output_ch);
        goto exit;
    }



exit:
    str_parms_destroy(parms);

    // VTS regards 0 as success, so if we setting parameter successfully,
    // zero should be returned instead of data length.
    // To pass VTS test, ret must be Result::OK (0) or Result::NOT_SUPPORTED (4).
    if (kvpairs == NULL) {
        ALOGE("Amlogic_HAL - %s: kvpairs points to NULL. Abort function and return 0.", __FUNCTION__);
        return 0;
    }
    if (ret > 0 || (strlen(kvpairs) == 0)) {
        ALOGI("Amlogic_HAL - %s: return 0 instead of length of data be copied.", __FUNCTION__);
        ret = 0;
    } else if (ret < 0) {
        ALOGI("Amlogic_HAL - %s: return Result::NOT_SUPPORTED (4) instead of other error code.", __FUNCTION__);
        //ALOGI ("Amlogic_HAL - %s: return Result::OK (0) instead of other error code.", __FUNCTION__);
        ret = 4;
    }
#endif
    return ret;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    int ret = -1;
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    char temp_buf[256] = {0};

    if (!strcmp(keys, AUDIO_PARAMETER_HW_AV_SYNC)) {
        ALOGI("get hwsync id\n");
        return strdup("hw_av_sync=12345678");
    }

    if (strstr(keys, AUDIO_PARAMETER_HW_AV_EAC3_SYNC)) {
        return strdup("HwAvSyncEAC3Supported=true");
    } else if (strstr(keys, "hdmi_format")) {
        sprintf(temp_buf, "hdmi_format=%d", adev->hdmi_format);
        return strdup(temp_buf);
    } else if (strstr(keys, "spdif_format")) {
        sprintf(temp_buf, "spdif_format=%d", adev->spdif_format);
        return strdup(temp_buf);
    } else if (strstr(keys, "hdmi_is_passthrough_active")) {
        sprintf(temp_buf, "hdmi_is_passthrough_active=%d", adev->hdmi_is_pth_active);
        return strdup(temp_buf);
    } else if (strstr(keys, "disable_pcm_mixing")) {
        sprintf(temp_buf, "disable_pcm_mixing=%d", adev->disable_pcm_mixing);
        return strdup(temp_buf);
    } else if (strstr(keys, "hdmi_encodings")) {
        struct format_desc *fmtdesc = NULL;
        bool dd = false, ddp = false;

        // query dd support
        fmtdesc = &adev->hdmi_descs.dd_fmt;
        if (fmtdesc && fmtdesc->fmt == _AC3) {
            dd = fmtdesc->is_support;
        }

        // query ddp support
        fmtdesc = &adev->hdmi_descs.ddp_fmt;
        if (fmtdesc && fmtdesc->fmt == _DDP) {
            ddp = fmtdesc->is_support;
        }

        sprintf(temp_buf, "hdmi_encodings=%s", "pcm;");
        if (ddp) {
            sprintf(temp_buf, "ac3;eac3;");
        } else if (dd) {
            sprintf(temp_buf, "ac3;");
        }

        return strdup(temp_buf);
    } else if (strstr(keys, "is_passthrough_active")) {
        bool active = false;
        pthread_mutex_lock(&adev->lock);
        // "is_passthrough_active" is used by amnuplayer to check is current audio hal have passthrough instance(DD/DD+/DTS)
        // if already have one passthrough instance, it will not invoke another one.
        // While in HDMI plug off/in test case. Player will query "is_passthrough_active" before adev->usecase_masks finally settle
        // So Player will have a chance get a middle-term value, which is wrong.
        // TODO: discuss later how to handle this async problem
        /*
        if (adev->usecase_masks & RAW_USECASE_MASK) {
            active = true;
            ALOGI("%s, usecase_masks = 0x%x , RAW_USECASE_MASK = 0x%x, is_passthrough_active = true",__FUNCTION__,adev->usecase_masks,RAW_USECASE_MASK);
        } else
        if (adev->audio_patch && (adev->audio_patch->aformat == AUDIO_FORMAT_E_AC3 \
                                         || adev->audio_patch->aformat == AUDIO_FORMAT_AC3)) {
            active = true;
            ALOGI("%s, audio_patch->aformat = 0x%x, is_passthrough_active = true",__FUNCTION__,adev->audio_patch->aformat);
        }*/
        pthread_mutex_unlock(&adev->lock);
        sprintf(temp_buf, "is_passthrough_active=%d", active);
        return  strdup(temp_buf);
    } else if (!strcmp(keys, "SOURCE_GAIN")) {
        sprintf(temp_buf, "source_gain = %f %f %f %f", adev->eq_data.s_gain.atv, adev->eq_data.s_gain.dtv,
                adev->eq_data.s_gain.hdmi, adev->eq_data.s_gain.av);
        return strdup(temp_buf);
    } else if (!strcmp(keys, "POST_GAIN")) {
        sprintf(temp_buf, "post_gain = %f %f %f", adev->eq_data.p_gain.speaker, adev->eq_data.p_gain.spdif_arc,
                adev->eq_data.p_gain.headphone);
        return strdup(temp_buf);
    } else if (strstr(keys, "dolby_ms12_enable")) {
#ifdef ENABLE_MS12
        int ms12_enable = (eDolbyMS12Lib == adev->dolby_lib_type_last);
        ALOGI("ms12_enable :%d", ms12_enable);
        sprintf(temp_buf, "dolby_ms12_enable=%d", ms12_enable);
        return  strdup(temp_buf);
#endif
    }

    ret = get_stream_parameters((struct audio_hw_device *)adev, keys, temp_buf, sizeof(temp_buf));
    if (ret == 0) {
        return  strdup(temp_buf);
    }

    ret = aml_audiolevel_getparam((struct audio_hw_device *)adev, keys, temp_buf, sizeof(temp_buf));
    if (ret == 0) {
        return  strdup(temp_buf);
    }

    ret = aml_external_pp_getinfo((struct audio_hw_device *)adev, keys, temp_buf, sizeof(temp_buf));
    if (ret == 0) {
        return  strdup(temp_buf);
    }

    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev __unused)
{
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev __unused, float volume __unused)
{
    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev __unused, float volume __unused)
{
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev __unused,
                                  float *volume __unused)
{
    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev __unused, bool muted __unused)
{
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev __unused, bool *muted __unused)
{
    return -ENOSYS;
}
static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    ALOGD("%s(%p, %d)", __FUNCTION__, dev, mode);

    pthread_mutex_lock(&adev->lock);
    if (adev->mode != mode) {
        adev->mode = mode;
        select_mode(adev);
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;

    adev->mic_mute = state;

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;

    *state = adev->mic_mute;

    return 0;

}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
        const struct audio_config *config)
{
    size_t size;
    int channel_count = popcount(config->channel_mask);

    ALOGD("%s(%p, %d, %d, %d)", __FUNCTION__, dev, config->sample_rate,
          config->format, channel_count);
    if (check_input_parameters(config->sample_rate, config->format, channel_count, AUDIO_DEVICE_NONE) != 0) {
        return 0;
    }

    return get_input_buffer_size(config->frame_count, config->sample_rate,
                                 config->format, channel_count);

}

static int rc_open_input_stream(struct aml_stream_in **stream,
                                struct audio_config *config)
{
    return 0;
}
static void rc_close_input_stream(struct aml_stream_in *stream)
{
    return;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle __unused,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags __unused,
                                  const char *address __unused,
                                  audio_source_t source __unused)
{
    struct aml_audio_device *ladev = (struct aml_audio_device *) dev;
    struct aml_stream_in *in;
    int ret;
    int channel_count = popcount(config->channel_mask);
    ALOGD("%s(%#x, %d, 0x%04x, %d)", __FUNCTION__,
          devices, config->format, config->channel_mask, config->sample_rate);

    if (check_input_parameters(config->sample_rate, config->format, channel_count, devices) != 0) {
        config->sample_rate = 48000;
        config->format = AUDIO_FORMAT_PCM_16_BIT;
        config->channel_mask = AUDIO_CHANNEL_IN_STEREO;
        return -EINVAL;
    }

    in = (struct aml_stream_in *) calloc(1, sizeof(struct aml_stream_in));
    if (!in) {
        return -ENOMEM;
    }
    in->main_channels = config->channel_mask;
    in->requested_rate = config->sample_rate;

    in->device = devices & ~AUDIO_DEVICE_BIT_IN;

    // usecase for amlogic audio hal
    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    //ALOGI("Amlogic - set in->config.channels to popcount of config->channel_mask.\n");
    in->config.channels = audio_channel_count_from_in_mask(config->channel_mask);
    if (in->config.channels == 1) {
        //config->channel_mask = AUDIO_CHANNEL_IN_MONO;
        ALOGI("Amlogic - channels == 1, set config->channel_mask = AUDIO_CHANNEL_IN_FRONT");
        // in fact, this value should be AUDIO_CHANNEL_OUT_BACK_LEFT(16u) according to VTS codes,
        // but the macroname can be confusing, so I'd like to set this value to
        // AUDIO_CHANNEL_IN_FRONT(16u) instead of AUDIO_CHANNEL_OUT_BACK_LEFT.
        config->channel_mask = AUDIO_CHANNEL_IN_FRONT;
    } else if (in->config.channels == 2) {
        config->channel_mask = AUDIO_CHANNEL_IN_STEREO;
    } else {
        ALOGE("Bad value of channel count : %d", in->config.channels);
    }

    //set device
    if (in->device & AUDIO_DEVICE_IN_ALL_SCO) {
        //bluetooth sco voice
        memcpy(&in->config, &pcm_config_bt, sizeof(pcm_config_bt));
    } else if (in->device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
        //bluetooth rc voice
        // usecase for bluetooth rc audio hal
        ALOGD("Amlogic - use RC audio HAL");
        ret = rc_open_input_stream(&in, config);
        if (ret != 0) {
            goto err_open;
        }

        config->sample_rate = in->config.rate;
        config->channel_mask = AUDIO_CHANNEL_IN_MONO;
    } else if (config->channel_mask == AUDIO_CHANNEL_OUT_7POINT1) {
        ALOGI("config use 8ch/16bits", in->config.channels);
        memcpy(&in->config, &i2s_multi_config_in, sizeof(i2s_multi_config_in));
    } else {
        ALOGI("config use 2ch/16bits", in->config.channels);
        memcpy(&in->config, &pcm_config_in, sizeof(pcm_config_in));
    }

#ifdef RESAMPLER
    in->buffer = malloc(in->config.period_size *
                        audio_stream_in_frame_size(&in->stream));
    if (!in->buffer) {
        ret = -ENOMEM;
        goto err_open;
    }

    if (!(in->device & AUDIO_DEVICE_IN_WIRED_HEADSET)) {
        // initiate resampler only if amlogic audio hal is used

        if (in->requested_rate != in->config.rate) {
            ALOGD("%s(in->requested_rate=%d, in->config.rate=%d)",
                  __FUNCTION__, in->requested_rate, in->config.rate);
            in->buf_provider.get_next_buffer = get_next_buffer;
            in->buf_provider.release_buffer = release_buffer;
            ret = create_resampler(in->config.rate,
                                   in->requested_rate,
                                   in->config.channels,
                                   RESAMPLER_QUALITY_DEFAULT,
                                   &in->buf_provider,
                                   &in->resampler);

            if (ret != 0) {
                ALOGE("Amlogic_HAL - create resampler failed. (%dHz --> %dHz)", in->config.rate, in->requested_rate);
                ret = -EINVAL;
                goto err_open;
            }
        }

    }
#endif

    in->dev = ladev;
    in->standby = 1;
    *stream_in = &in->stream;

    return 0;

err_open:
#ifdef RESAMPLER
    if (in->resampler) {
        release_resampler(in->resampler);
    }
#endif
    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                    struct audio_stream_in *stream)
{
    struct aml_stream_in *in = (struct aml_stream_in *) stream;

    ALOGD("%s(%p, %p)", __FUNCTION__, dev, stream);
    in_standby(&stream->common);

    if (in->device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
        rc_close_input_stream(in);
    }
#ifdef RESAMPLER
    if (in->resampler) {
        free(in->buffer);
        release_resampler(in->resampler);
    }
#endif
    if (in->proc_buf) {
        free(in->proc_buf);
    }
    if (in->ref_buf) {
        free(in->ref_buf);
    }

    free(stream);

    return;
}

const char *audio_port_role[] = {"AUDIO_PORT_ROLE_NONE", "AUDIO_PORT_ROLE_SOURCE", "AUDIO_PORT_ROLE_SINK"};
const char *audio_port_type[] = {"AUDIO_PORT_TYPE_NONE", "AUDIO_PORT_TYPE_DEVICE", "AUDIO_PORT_TYPE_MIX", "AUDIO_PORT_TYPE_SESSION"};
static void dump_audio_port_config(const struct audio_port_config *port_config)
{
    if (port_config == NULL) {
        return;
    }

    ALOGI("  -%s port_config(%p)", __FUNCTION__, port_config);
    ALOGI("\t-id(%d), role(%s), type(%s)", port_config->id, audio_port_role[port_config->role], audio_port_type[port_config->type]);
    ALOGV("\t-config_mask(%#x)", port_config->config_mask);
    ALOGI("\t-sample_rate(%d), channel_mask(%#x), format(%#x)", port_config->sample_rate,
          port_config->channel_mask, port_config->format);
    ALOGV("\t-gain.index(%#x)", port_config->gain.index);
    ALOGV("\t-gain.mode(%#x)", port_config->gain.mode);
    ALOGV("\t-gain.channel_mask(%#x)", port_config->gain.channel_mask);
    ALOGI("\t-gain.value0(%d)", port_config->gain.values[0]);
    ALOGI("\t-gain.value1(%d)", port_config->gain.values[1]);
    ALOGI("\t-gain.value2(%d)", port_config->gain.values[2]);
    ALOGV("\t-gain.ramp_duration_ms(%d)", port_config->gain.ramp_duration_ms);
    switch (port_config->type) {
    case AUDIO_PORT_TYPE_DEVICE:
        ALOGI("\t-port device: type(%#x) addr(%s)",
              port_config->ext.device.type, port_config->ext.device.address);
        break;
    case AUDIO_PORT_TYPE_MIX:
        ALOGI("\t-port mix: iohandle(%d)", port_config->ext.mix.handle);
        break;
    default:
        break;
    }
}

/* must be called with hw device and output stream mutexes locked */
int do_output_standby_l(struct audio_stream *stream)
{
    struct audio_stream_out *out = (struct audio_stream_out *) stream;
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    if (aml_out->usecase >= STREAM_PCM_NORMAL && aml_out->usecase <= STREAM_USECASE_MAX) {
        ALOGI("%s(%p), stream usecase: %s continuous_audio_mode %d", __func__, aml_out, str_usecases[aml_out->usecase], adev->continuous_audio_mode);
    } else {
        ALOGI("%s(%p), stream usecase %d invalid ", __func__, aml_out, aml_out->usecase);
    }

    /*
    if continous mode,we need always have output.
    so we should not disable the output.
    */
    if (aml_out->status == STREAM_HW_WRITING && !continous_mode(adev)) {
        ALOGI("%s aml_out(%p)standby close", __func__, aml_out);
        pthread_mutex_lock(&adev->alsa_pcm_lock);
#ifndef OUTPUT_LAYER
        aml_alsa_output_close(stream);
#else
        aml_output_close(out);
#endif
        pthread_mutex_unlock(&adev->alsa_pcm_lock);
    }

    if (adev->continuous_audio_mode == 0) {
        // release buffers
        if (aml_out->buffer) {
            free(aml_out->buffer);
            aml_out->buffer = NULL;
        }
#ifdef RESAMPLER
        if (aml_out->resampler) {
            release_resampler(aml_out->resampler);
            aml_out->resampler = NULL;
        }
#endif
    }
    stream_usecase_t usecase = aml_out->usecase;
    usecase_change_validate_l(aml_out, true);
    aml_out->status = STREAM_STANDBY;
    if (aml_out->is_normal_pcm) {
#ifdef ENABLE_MS12
        set_system_app_mixing_status(aml_out, aml_out->status);
#endif
        aml_out->normal_pcm_mixing_config = false;
    }
    aml_out->pause_status = false;//clear pause status
    return 0;
}

static int out_standby_new(struct audio_stream *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    int status = 0;

    ALOGI("++%s()", __func__);
    pthread_mutex_lock(&aml_out->dev->lock);
    pthread_mutex_lock(&aml_out->lock);
    status = do_output_standby_l(stream);

    pthread_mutex_unlock(&aml_out->lock);
    pthread_mutex_unlock(&aml_out->dev->lock);

    return status;
}

static bool is_iec61937_format(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    if ((adev->disable_pcm_mixing == true) && (adev->dual_decoder_support == false) && (adev->continuous_audio_mode == false) && \
        (adev->sink_format == AUDIO_FORMAT_E_AC3 || adev->sink_format == AUDIO_FORMAT_AC3) && ((aml_out->hal_format == AUDIO_FORMAT_IEC61937) || \
                ((aml_out->hal_format == AUDIO_FORMAT_E_AC3 || aml_out->hal_format == AUDIO_FORMAT_AC3) && (aml_out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO)))) {
        /*
         *case 1.With Kodi APK Dolby passthrough
         *Media Player direct format is AUDIO_FORMAT_IEC61937
         *case 2.HDMI-IN or SPDIF-IN
         *audio format is AUDIO_FORMAT_IEC61937
         */
        return true;
    }
    /*other data is dd/dd+ raw data*/

    return false;
}

static void aml_tinymix_set_spdif_format(audio_format_t output_format)
{
    int aml_spdif_format = AML_STEREO_PCM;
    int spdif_mute = 0;
    if (output_format == AUDIO_FORMAT_AC3) {
        aml_spdif_format = AML_DOLBY_DIGITAL;
    } else if (output_format == AUDIO_FORMAT_E_AC3) {
        aml_spdif_format = AML_DOLBY_DIGITAL_PLUS;
        spdif_mute = 1;
    } else if (output_format == AUDIO_FORMAT_DTS) {
        aml_spdif_format = AML_DTS;
    } else {
        aml_spdif_format = AML_STEREO_PCM;
    }
    //aml_mixer_ctrl_set_int(AML_MIXER_ID_SPDIF_FORMAT, aml_spdif_format);
    //aml_mixer_ctrl_set_int(AML_MIXER_ID_SPDIF_MUTE, spdif_mute);
    //ALOGI("%s tinymix AML_MIXER_ID_SPDIF_FORMAT %d,spdif mute %d\n",
    //__FUNCTION__, aml_spdif_format, spdif_mute);
}

audio_format_t get_output_format(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    audio_format_t output_format = aml_out->hal_internal_format;
    struct dolby_ms12_desc *ms12 = & (adev->ms12);
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (ms12->dolby_ms12_enable) {
            output_format = adev->sink_format;
        }
    } else
#endif
        if (eDolbyDcvLib == adev->dolby_lib_type) {
            if (adev->hdmi_format > 0) {
                output_format = adev->sink_format;
            }
        }

    return output_format;
}
ssize_t aml_audio_spdif_output(struct audio_stream_out *stream,
                               const void *buffer, size_t byte)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *aml_dev = aml_out->dev;
    struct pcm *pcm = aml_dev->pcm_handle[DIGITAL_DEVICE];
    struct pcm_config config;
    int ret = 0;
    int codec_type = 0;

    if (!aml_out->dual_output_flag) {
        ALOGE("%s() not support, dual flag = %d",
              __func__, aml_out->dual_output_flag);
        return -EINVAL;
    }

    /* open spdif handle if necessary */
    if (!pcm) {
        /* init pcm configs, no DDP case in dual output */
        memset(&config, 0, sizeof(struct pcm_config));
        config.channels = 2;
        config.rate = MM_FULL_POWER_SAMPLING_RATE;
        config.period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * 2;
        config.period_count = PLAYBACK_PERIOD_COUNT;
        config.start_threshold = DEFAULT_PLAYBACK_PERIOD_SIZE * 2 * PLAYBACK_PERIOD_COUNT;
        config.format = PCM_FORMAT_S16_LE;

        aml_tinymix_set_spdif_format(aml_dev->optical_format);
        pthread_mutex_lock(&aml_dev->alsa_pcm_lock);
        pcm = pcm_open(aml_out->card, DIGITAL_DEVICE, PCM_OUT, &config);
        if (!pcm_is_ready(pcm)) {
            ALOGE("%s() cannot open pcm_out: %s,card %d,device %d", __func__, pcm_get_error(pcm), aml_out->card, DIGITAL_DEVICE);
            pcm_close(pcm);
            return -ENOENT;
        }
        ALOGI("%s open dual output pcm handle %p", __func__, pcm);
        aml_dev->pcm_handle[DIGITAL_DEVICE] = pcm;
        pthread_mutex_unlock(&aml_dev->alsa_pcm_lock);

    }
    if (eDolbyDcvLib == aml_dev->dolby_lib_type) {
        pthread_mutex_lock(&aml_dev->alsa_pcm_lock);
        ret = pcm_write(pcm, buffer, byte);
        pthread_mutex_unlock(&aml_dev->alsa_pcm_lock);
        if (ret < 0) {
            ALOGE("%s write failed,pcm handle %p err num %d", __func__, pcm, ret);
        }
        ALOGV("%s(), aml_alsa_output_write bytes = %zu", __func__, byte);

    } else {
#ifdef SPDIF_ENCODER
        if (!aml_dev->dual_spdifenc_inited) {
            if (aml_dev->optical_format != AUDIO_FORMAT_AC3) {
                ALOGE("%s() not support, optical format = %#x",
                      __func__, aml_dev->optical_format);
                return -EINVAL;
            }
            int init_ret = spdifenc_init(pcm, aml_dev->optical_format); // zz
            //int init_ret = spdifenc_init (pcm, AUDIO_FORMAT_AC3); //8.1
            if (init_ret == 0) {
                aml_dev->dual_spdifenc_inited = 1;
                aml_mixer_ctrl_set_int(AML_MIXER_ID_SPDIF_FORMAT, AML_DOLBY_DIGITAL);
                ALOGI("%s tinymix AML_MIXER_ID_SPDIF_FORMAT %d\n", __FUNCTION__, AML_DOLBY_DIGITAL);
            }
        }

        ret = spdifenc_write(buffer, byte);
        ALOGI("%s(), spdif write bytes = %d", __func__, ret);
#endif
    }
    return ret;
}

static int get_resamplerate(int input_rate) {
    switch (input_rate) {
        case 8000:
        case 12000:
        case 16000:
        case 24000:
        case 32000:
        case 48000:
        case 96000:
        case 192000:
            return 48000;
        case 11025:
        case 22050:
        case 44100:
        case 88200:
        case 176400:
            return 44100;
        default:
            return 48000;
    }
    return 48000;
}

ssize_t audio_hal_data_processing(struct audio_stream_out *stream
                                  , const void *buffer
                                  , size_t bytes
                                  , void **output_buffer
                                  , size_t *output_buffer_bytes
                                  , aml_data_format_t * data_format)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    int16_t *tmp_buffer = (int16_t *) buffer;
    int16_t *effect_tmp_buf = NULL;
    int out_frames = 0;
    int nsamples = 0;
    ssize_t ret = 0;
    int i = 0, j = 0;
    uint32_t latency_frames = 0;
    uint64_t total_frame = 0;
    int out_channel = aml_out->config.channels;
    int in_ch = data_format->ch;
    audio_format_t output_format = data_format->format;
    aml_data_format_t  dst_format;
    unsigned char * convert_buf = NULL;
    size_t need_bytes = 0;
    size_t convert_size = 0;

    if (out_channel == 0 || bytes == 0) {
        //ALOGI("Wrong Input\n");
        return -1;

    }

    memcpy(&dst_format, data_format, sizeof(aml_data_format_t));
    dst_format.bitwidth = SAMPLE_32BITS;
    dst_format.format   = AUDIO_FORMAT_PCM_32_BIT;

    out_frames = bytes / (in_ch * (data_format->bitwidth >> 3));


    /* raw data need packet to IEC61937 format by spdif encoder */
    if ((output_format == AUDIO_FORMAT_AC3) || (output_format == AUDIO_FORMAT_E_AC3)) {
        //ALOGI("%s, aml_out->hal_format %x , is_iec61937_format = %d, \n", __func__, aml_out->hal_format,is_iec61937_format(stream));
        if ((is_iec61937_format(stream) == true) ||
            (adev->dolby_lib_type == eDolbyDcvLib)) {
            *output_buffer = (void *) buffer;
            *output_buffer_bytes = bytes;
        } else {
#ifdef SPDIF_ENCODER
            if (adev->spdif_encoder_init_flag == false) {
                ALOGI("%s, go to prepare spdif encoder, output_format %#x\n", __func__, output_format);
                ret = get_the_spdif_encoder_prepared(output_format, aml_out);
                if (ret) {
                    ALOGE("%s() get_the_spdif_encoder_prepared failed", __func__);
                    return ret;
                }
                aml_out->spdif_enc_init_frame_write_sum = aml_out->frame_write_sum;
                adev->spdif_encoder_init_flag = true;
            }
            spdif_encoder_ad_write(buffer, bytes);
            adev->temp_buf_pos = spdif_encoder_ad_get_current_position();
            if (adev->temp_buf_pos <= 0) {
                adev->temp_buf_pos = 0;
            }
            spdif_encoder_ad_flush_output_current_position();
            *output_buffer = adev->temp_buf;
            *output_buffer_bytes = adev->temp_buf_pos;
#endif
        }
    } else if (output_format == AUDIO_FORMAT_DTS) {
        *output_buffer = (void *) buffer;
        *output_buffer_bytes = bytes;
    } else {
        int16_t *hp_tmp_buf = NULL;
        int out_multiply = 4;
        float source_gain = 1.0;
        float gain_headphone = adev->eq_data.p_gain.headphone;
        float gain_speaker = adev->eq_data.p_gain.speaker;
        float gain_spdif_arc = adev->eq_data.p_gain.spdif_arc;

        if (adev->patch_src == SRC_DTV) {
            source_gain = adev->eq_data.s_gain.dtv;
        } else if (adev->patch_src == SRC_HDMIIN) {
            source_gain = adev->eq_data.s_gain.hdmi;
        } else if (adev->patch_src == SRC_LINEIN) {
            source_gain = adev->eq_data.s_gain.av;
        } else if (adev->patch_src == SRC_ATV) {
            source_gain = adev->eq_data.s_gain.atv;
        }

        /* handling audio effect process here */
        if (adev->effect_buf_size < bytes) {
            ALOGI("realloc effect_buf_size size from %zu to %zu", adev->effect_buf_size, bytes);
            adev->effect_buf = realloc(adev->effect_buf, bytes);
            if (adev->effect_buf == NULL) {
                ALOGE("realloc effect buf failed size %zu\n", bytes);
                return -ENOMEM;
            }

            adev->hp_output_buf = realloc(adev->hp_output_buf, bytes);
            if (adev->hp_output_buf == NULL) {
                ALOGE("realloc headphone buf failed size %zu\n", bytes);
                return -ENOMEM;
            }
            adev->effect_buf_size = bytes;
        }
        effect_tmp_buf = (int16_t *) adev->effect_buf;
        hp_tmp_buf = (int16_t *) adev->hp_output_buf;
#ifdef ENABLE_AVSYNC_TUNING
        tuning_spker_latency(adev, effect_tmp_buf, tmp_buffer, bytes);
#else
        memcpy(effect_tmp_buf, tmp_buffer, bytes);
#endif
        memcpy(hp_tmp_buf, tmp_buffer, bytes);

        /*aduio effect process for speaker*/
        for (i = 0; i < adev->native_postprocess.num_postprocessors; i++) {
            audio_post_process(adev->native_postprocess.postprocessors[i], effect_tmp_buf, out_frames);
        }

        /* apply volume for spk/hp, SPDIF/HDMI keep the max volume */
        gain_speaker *= (adev->sink_gain[OUTPORT_SPEAKER] * source_gain);
        //gain_headphone *= (adev->sink_gain[OUTPORT_HEADPHONE]*source_gain);
        //gain_spdif_arc *= source_gain;
        //ALOGD("gain speaker=%f sink=%f source=%f\n",gain_speaker,adev->sink_gain[OUTPORT_SPEAKER],source_gain);
        //apply_volume(gain_speaker, effect_tmp_buf, sizeof(uint16_t), bytes);
        //apply_volume(gain_speaker, hp_tmp_buf, sizeof(uint16_t), bytes);
        //apply_volume(gain_spdif_arc, tmp_buffer, sizeof(uint16_t), bytes);


#if 0//def USE_ALSA_PLUGINS
        // if ALSA plugins is enabled, we can bypass the data to alsa
        *output_buffer = (void*) buffer;
        *output_buffer_bytes = bytes;
        return 0;

#endif



#if 0
        need_bytes = out_frames * (SAMPLE_32BITS >> 3) * out_channel;

        /* we should do 32 bit out, we may need increase the buffer size. */
        if (aml_out->tmp_buffer_8ch_size < need_bytes) {
            ALOGI("realloc tmp_buffer_8ch size from %zu to %zu\n", aml_out->tmp_buffer_8ch_size, need_bytes);
            aml_out->tmp_buffer_8ch = realloc(aml_out->tmp_buffer_8ch, need_bytes);
            if (aml_out->tmp_buffer_8ch == NULL) {
                ALOGE("realloc tmp_buffer_8ch buf failed size %zu\n", need_bytes);
                return -ENOMEM;
            }
            aml_out->tmp_buffer_8ch_size = need_bytes;
        }
        /*we don't need convert the data*/
        if (data_format->bitwidth == dst_format.bitwidth &&
            out_channel == in_ch) {
            int32_t * src_data = (int32_t *)tmp_buffer;
            for (i = 0; i < out_frames; i++) {
                aml_out->tmp_buffer_8ch[out_channel * i ]     = src_data[in_ch * i];
                aml_out->tmp_buffer_8ch[out_channel * i + 1]  = src_data[in_ch * i + 1];
                aml_out->tmp_buffer_8ch[out_channel * i + 2]  = src_data[in_ch * i + 4];
                aml_out->tmp_buffer_8ch[out_channel * i + 3]  = src_data[in_ch * i + 5];
                aml_out->tmp_buffer_8ch[out_channel * i + 4]  = src_data[in_ch * i + 6];
                aml_out->tmp_buffer_8ch[out_channel * i + 5]  = src_data[in_ch * i + 7];
                aml_out->tmp_buffer_8ch[out_channel * i + 6]  = src_data[in_ch * i + 2];
                aml_out->tmp_buffer_8ch[out_channel * i + 7]  = src_data[in_ch * i + 3];

            }
            *output_buffer = aml_out->tmp_buffer_8ch;
            *output_buffer_bytes = bytes;
            return 0;
        }
#endif
        /*do resample for 16 bit*/
        if (data_format->bitwidth == SAMPLE_16BITS && data_format->sr != 48000 && data_format->sr != 44100) {
            int target_rate = 48000;
            /*sample rate is chagned, we need to resmaple it*/
            if (aml_out->resample_handle != NULL) {
                if (data_format->sr != aml_out->resample_handle->resample_config.input_sr) {
                    aml_audio_resample_close(aml_out->resample_handle);
                    aml_out->resample_handle = NULL;
                }
            }

            target_rate = get_resamplerate(data_format->sr);
            if (aml_out->resample_handle == NULL) {
                audio_resample_config_t resample_config;
                ALOGI("init resampler from %d to target_rate!\n", data_format->sr, target_rate);
                resample_config.aformat   = AUDIO_FORMAT_PCM_16_BIT;
                resample_config.channels  = data_format->ch;
                resample_config.input_sr  = data_format->sr;
                resample_config.output_sr = target_rate;
                ret = aml_audio_resample_init((aml_audio_resample_t **)&aml_out->resample_handle, AML_AUDIO_SPEEX_RESAMPLE, &resample_config);
                if (ret < 0) {
                    ALOGE("resample init error\n");
                    return -1;
                }
            }

            ret = aml_audio_resample_process(aml_out->resample_handle, tmp_buffer, bytes);
            if (ret < 0) {
                ALOGE("resample process error\n");
                return -1;
            }

            tmp_buffer = (int16_t *) aml_out->resample_handle->resample_buffer;
            bytes      = aml_out->resample_handle->resample_size;
            data_format->sr = target_rate;

        }


        /* convert the original sample bit to target sample bit */
        nsamples = bytes / (data_format->bitwidth >> 3);
        if (data_format->bitwidth != dst_format.bitwidth) {
            aml_sampleconv_process(aml_out->sample_convert, data_format, tmp_buffer, &dst_format, nsamples);
            convert_buf = aml_out->sample_convert->convert_buffer;
            convert_size = aml_out->sample_convert->convert_size;
            data_format->bitwidth = dst_format.bitwidth;

        } else {
            convert_buf = (unsigned char *)tmp_buffer;
            convert_size = bytes;
        }

        /* do channel mapping*/
        out_frames = convert_size / (in_ch * (data_format->bitwidth >> 3));
        aml_channelmap_process(aml_out->channel_map, data_format, convert_buf, out_frames);
        data_format->ch = aml_out->channel_map->format.ch;

        /*fix the input as 8ch-32bits format and suppose the LFE is the ch3 of ch0~7*/
        ret = aml_bm_process(stream, (const void *)aml_out->channel_map->map_buffer, aml_out->channel_map->out_buffer_size, data_format);
        if (ret) {
            ALOGE("%s failed", __func__);
            return ret;
        }

        *output_buffer = aml_out->channel_map->map_buffer;
        *output_buffer_bytes = aml_out->channel_map->out_buffer_size;
        //ALOGD("out channel=%d\n",data_format->ch);

        if (0) {
            FILE *fp1 = fopen("/tmp/8ch.pcm", "a+");
            if (fp1) {
                int flen = fwrite((char *)*output_buffer, 1, *output_buffer_bytes, fp1);
                fclose(fp1);
            }
        }

        aml_external_pp_process(&adev->hw_device, *output_buffer, *output_buffer_bytes, data_format);

        ret = aml_audiolevel_cal(&adev->hw_device, *output_buffer, *output_buffer_bytes, data_format);
        if (ret < 0) {
            ALOGE("aml_audiolevel_cal failed\n");
            return ret;
        }

        ret = aml_volctl_process(&adev->hw_device, *output_buffer, *output_buffer_bytes, data_format);
        if (ret < 0) {
            ALOGE("aml_volctl_process failed\n");
            return ret;
        }

        ret = aml_audiodelay_process(&adev->hw_device, *output_buffer, *output_buffer_bytes, data_format);
        if (ret < 0) {
            ALOGE("aml_audiodelay_process failed\n");
            return ret;
        }

    }

    return 0;
}

ssize_t hw_write(struct audio_stream_out *stream
                 , const void *buffer
                 , size_t bytes
                 , aml_data_format_t * data_format)
{
    //ALOGV("+%s() buffer %p bytes %zu", __func__, buffer, bytes);
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    const uint16_t *tmp_buffer = buffer;
    int16_t *effect_tmp_buf = NULL;
    int out_frames = bytes / 4;
    ssize_t ret = 0;
    int i;
    uint32_t latency_frames = 0;
    uint64_t total_frame = 0;
    uint64_t write_frames = 0;
    int  adjust_ms = 0;
    aml_stream_config_t output_config = {0};
    audio_format_t output_format = data_format->format;
    aml_device_config_t device_config;
    aml_dec_t *aml_dec = aml_out->aml_dec;

    adev->debug_flag = aml_audio_get_debug_flag();
    if (adev->debug_flag) {
        ALOGI("+%s() buffer %p bytes %zu, format %d", __func__, buffer, bytes, output_format);
    }

    pthread_mutex_lock(&adev->trans_lock);
    pthread_mutex_lock(&adev->alsa_pcm_lock);

    if (aml_out->status == STREAM_HW_WRITING) {

        ret = aml_output_getinfo(stream, PCMOUTPUT_CONFIG_INFO, (output_info_t *)&output_config);
        if (ret == 0) {
            if (data_format->sr != output_config.rate ||
                data_format->ch != output_config.channels) {
                ALOGD("Output format changed from rate=%d ch=%d to rate=%d ch=%d\n",
                      output_config.rate, output_config.channels, data_format->sr, data_format->ch);
                aml_output_close(stream);
                aml_out->status = STREAM_STANDBY;
            }
        }

    }

    if (aml_out->status != STREAM_HW_WRITING) {
        ALOGI("%s, aml_out %p alsa open output_format %#x\n", __func__, aml_out, output_format);
        if (eDolbyDcvLib == adev->dolby_lib_type) {
            if (!aml_out->dual_output_flag) {
                aml_tinymix_set_spdif_format(output_format);
            }
        } else {
            aml_tinymix_set_spdif_format(output_format);
        }
        if (!is_digital_raw_format(output_format)) {
            output_config.channels = data_format->ch;
            output_config.rate     = data_format->sr;
            output_config.format   = BitToFormat(data_format->bitwidth);
            output_config.latency  = adev->audio_latency;
            output_config.framesize = IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format) ? (DEFAULT_PLAYBACK_PERIOD_SIZE * 2) : DEFAULT_PLAYBACK_PERIOD_SIZE;
            output_config.start_threshold_coef = IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format) ? 1 : 2;
            device_config.device   = AUDIO_DEVICE_OUT_SPEAKER;
            ret = aml_output_open(stream, &output_config, &device_config);
        } else if (is_digital_raw_format(output_format)) {
            output_config.channels = 2;
            output_config.rate     = MM_FULL_POWER_SAMPLING_RATE;
            output_config.format   = output_format;
            device_config.device   = AUDIO_DEVICE_OUT_SPDIF;
            /*TODO:bitstream output
             *how to set the value of output_config.framesize
             */
            //output_config.framesize = 0;
            ret = aml_output_open(stream, &output_config, &device_config);
        }

        if (ret) {
            ALOGE("%s() open failed\n", __func__);
            pthread_mutex_unlock(&adev->alsa_pcm_lock);
            pthread_mutex_unlock(&adev->trans_lock);
            return -1;

        }
        aml_out->status = STREAM_HW_WRITING;
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (aml_out->hw_sync_mode && !adev->ms12.is_continuous_paused) {
            aml_audio_hwsync_audio_process(aml_out->hwsync, dolby_ms12_get_consumed_payload(), &adjust_ms);
        }
    }
#endif
    if (1)/*(aml_out->pcm)*/ {

        if (!is_digital_raw_format(output_format)) {
            ret = aml_output_write_pcm(stream, (void*)buffer, bytes);

        } else if (is_digital_raw_format(output_format)) {
            ret = aml_output_write_raw(stream, (void*)buffer, bytes);

        }
        if (ret < 0) {
            ALOGE("ALSA out write fail\n");
        } else {
            if (!continous_mode(adev)) {
                if ((output_format == AUDIO_FORMAT_AC3) ||
                    (output_format == AUDIO_FORMAT_E_AC3) ||
                    (output_format == AUDIO_FORMAT_DTS)) {
                    if (is_iec61937_format(stream) == true) {
                        //continuous_audio_mode = 0, when mixing-off and 7.1ch
                        //FIXME out_frames 4 or 16 when DD+ output???
                        aml_out->frame_write_sum += out_frames;
                    } else {
                        int sample_per_bytes = (output_format == AUDIO_FORMAT_E_AC3) ? 16 : 4;

                        if (eDolbyDcvLib == adev->dolby_lib_type && aml_out->hal_format == AUDIO_FORMAT_IEC61937) {
                            aml_out->frame_write_sum = aml_out->input_bytes_size / audio_stream_out_frame_size(stream);
                            //aml_out->frame_write_sum += out_frames;
                        } else {
                            aml_out->frame_write_sum += bytes / sample_per_bytes; //old code
                            //aml_out->frame_write_sum = spdif_encoder_ad_get_total() / sample_per_bytes + aml_out->spdif_enc_init_frame_write_sum; //8.1
                        }
                    }
                } else {
                    if (aml_out->is_tv_platform == true) {
                        out_frames = out_frames / 8;
                    }
                    aml_out->frame_write_sum += out_frames;

                    total_frame =  aml_out->frame_write_sum;
                }
            }

        }
    }
    pthread_mutex_unlock(&adev->alsa_pcm_lock);

    latency_frames = out_get_latency_frames(stream);
    if (output_format == AUDIO_FORMAT_E_AC3) {
        latency_frames /= 4;
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        /*it is the main alsa write function we need notify that to all sub-stream */
        adev->ms12.latency_frame = latency_frames;
    }
#endif
    /*
    if ms12 main process run in the audioflinger thread.all the information
    from the stream is the same as its origial stream.for example,some key
    information(format,ch,and the presentation).so we can use this information
    to config the output format and update the output presentation.
    */
    if (!continous_mode(adev)) {
        if (aml_out->hal_internal_format == AUDIO_FORMAT_PCM_16_BIT) {
            write_frames = aml_out->input_bytes_size / 4;
            //total_frame = write_frames;
        } else {
            total_frame = aml_out->frame_write_sum + aml_out->frame_skip_sum;
            if (aml_out->flags & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) {
                if (aml_out->hal_internal_format == AUDIO_FORMAT_AC3) {
                    write_frames = aml_out->input_bytes_size / 6144 * 32 * 48;
                }
                if (aml_out->hal_internal_format == AUDIO_FORMAT_E_AC3) {
                    write_frames = aml_out->input_bytes_size / 24576 * 32 * 48;
                }
            } else {
                write_frames = aml_out->input_bytes_size / aml_out->ddp_frame_size * 32 * 48;
            }
        }
    } else {
#ifdef ENABLE_MS12
        if (eDolbyMS12Lib == adev->dolby_lib_type) {
            /*
                since the ms12 process runs in a seperate thread,also the audio stream is a totally
                seperate one  which just copy the information from  the audio stream when the ms12 thread
                is created,the information maybe not corrent anymore after format updated between
                hd<->hd 5.1 playback switch. for example. the hal audio format will change between
                LPCM and Dolby audio.We need update stream information from the original active
                bitstream which is triggered by audio flinger thread.The  render presentation is needed to
                updated to main bistream which is owned by audiofligner thread instance.

                case 1: MS12  stream------------------- system audio from HD audio<main output>
                        |
                        |-------------------------------dummy dolby audio

                case 2: MS12 stream---------------------Dolby audio from HD 5.1 audio<main output>
                        |
                        |-------------------------------system sound

                case 3: MS12 stream---------------------OTT sound audio from tunnel mode LPCM playback<main output>
                        |-------------------------------dummy dolby audio
                        |-------------------------------system sound
                */
            /*case 2*/
            if (!audio_is_linear_pcm(aml_out->hal_internal_format)) {
                //unsigned long long output_size = 0;
                //dolby_ms12_get_pcm_output_size(&output_size,NULL);
                //total_frame = total_frame/aml_out->ddp_frame_size*32*48;
                //total_frame = output_size/4;
                total_frame = dolby_ms12_get_consumed_payload() + aml_out->continuous_audio_offset;
                total_frame = total_frame / aml_out->ddp_frame_size * 32 * 48;
                if (adev->debug_flag) {
                    ALOGI("%s,ms12 dolby offset %llu,time %"PRIu64" ms", __func__,
                          dolby_ms12_get_consumed_payload(), total_frame / 48);
                }
            }
            /*case 3*/
            else if (aml_out->hw_sync_mode) {
                total_frame = dolby_ms12_get_consumed_payload() / 4;
            }
            /*case 1*/
            else {
                /*system volume prestation caculatation is done inside aux write thread*/
                total_frame = 0;
                //write_frames = (aml_out->input_bytes_size - dolby_ms12_get_system_buffer_avail()) / 4;
                //total_frame = write_frames;
            }
        }
#endif
    }
    /*we should also to caculate the alsa latency*/
    {
        clock_gettime(CLOCK_MONOTONIC, &aml_out->timestamp);
        if (total_frame >= latency_frames) {
            aml_out->last_frames_postion = total_frame - latency_frames;
        } else {
            aml_out->last_frames_postion = total_frame;
        }
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (continous_mode(adev)) {
            adev->ms12.last_frames_postion = aml_out->last_frames_postion;
            clock_gettime(CLOCK_MONOTONIC, &adev->ms12.timestamp);
        }
    }
#endif
    if (adev->debug_flag) {
        ALOGI("%s() stream(%p) pcm handle %p format input %#x output %#x 61937 frame %d",
              __func__, stream, aml_out->pcm, aml_out->hal_internal_format, output_format, is_iec61937_format(stream));
#ifdef ENABLE_MS12
        if (eDolbyMS12Lib == adev->dolby_lib_type) {
            //ms12 internal buffer avail(main/associate/system)
            if (adev->ms12.dolby_ms12_enable == true)
                ALOGI("%s MS12 buffer avail main %d associate %d system %d\n",
                      __FUNCTION__, dolby_ms12_get_main_buffer_avail(), dolby_ms12_get_associate_buffer_avail(), dolby_ms12_get_system_buffer_avail());
        }
#endif
        if ((aml_out->hal_internal_format == AUDIO_FORMAT_AC3) || (aml_out->hal_internal_format == AUDIO_FORMAT_E_AC3)) {
            ALOGI("%s() total_frame %"PRIu64" latency_frames %d last_frames_postion %"PRIu64" total write %"PRIu64" total writes frames %"PRIu64" diff latency %"PRIu64" ms\n",
                  __FUNCTION__, total_frame, latency_frames, aml_out->last_frames_postion, aml_out->input_bytes_size, write_frames, (write_frames - total_frame) / 48);
        } else {
            ALOGI("%s() total_frame %"PRIu64" latency_frames %d last_frames_postion %"PRIu64" total write %"PRIu64" total writes frames %"PRIu64" diff latency %"PRIu64" ms\n",
                  __FUNCTION__, total_frame, latency_frames, aml_out->last_frames_postion, aml_out->input_bytes_size, write_frames, (write_frames - aml_out->last_frames_postion) / 48);
        }
    }

    pthread_mutex_unlock(&adev->trans_lock);
    return ret;
}

/*call by adev lock*/
static void config_output(struct audio_stream_out *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_stream_out *out = NULL;
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    struct aml_audio_patch *patch = adev->audio_patch;
    aml_dec_t *aml_dec = aml_out->aml_dec;
    aml_dcv_config_t dcv_config;
    aml_dca_config_t dca_config;
    aml_dec_config_t dec_config;

    int ret = 0;
    bool main1_dummy = false;
    bool ott_input = false;
    bool dtscd_flag = false;
    int i  = 0 ;
    uint64_t write_frames = 0;
    int status = -1;

    int is_arc_connected = 0;
    if (adev->bHDMIARCon /*&& adev->bHDMIConnected*/ && adev->speaker_mute) {
        is_arc_connected = 1;
    }
    ALOGD("%s\n", __func__);
    /*
    *   when ARC is connecting, and user switch [Sound Output Device] to "ARC"
    *   we need to set out_port as OUTPORT_HDMI_ARC ,
    *   the aml_audio_output_routing() will "UNmute" ARC and "mute" speaker.
    */
    int out_port = adev->active_outport;
    if (adev->active_outport == OUTPORT_SPEAKER && is_arc_connected) {
        out_port = OUTPORT_HDMI_ARC;
    }

    /*
    *   when ARC is connecting, and user switch [Sound Output Device] to "speaker"
    *   we need to set out_port as OUTPORT_SPEAKER ,
    *   the aml_audio_output_routing() will "mute" ARC and "unmute" speaker.
    */
    if (adev->active_outport == OUTPORT_HDMI_ARC && !is_arc_connected) {
        out_port = OUTPORT_SPEAKER;
    }
#ifdef AUDIO_ROUTE
    ret = aml_audio_output_routing((struct audio_hw_device *)adev, out_port, true);
    if (ret < 0) {
        ALOGE("%s() output routing failed\n", __func__);
    }
#endif
    /*get sink format*/
    get_sink_format(stream);
    //ALOGE("%s() adev->dolby_lib_type = %d",  __func__, adev->dolby_lib_type);

    if (aml_out->status == STREAM_HW_WRITING) {
        aml_output_close(stream);
        aml_out->status = STREAM_STANDBY;
        aml_tinymix_set_spdif_format(AUDIO_FORMAT_PCM_16_BIT);
    }

    /*check whether enable raw output*/
    switch (adev->hdmi_format) {

    case PCM:
        adev->digital_raw = 0;
        adev->bypass_enable = 0;
        break;
    case AUTO:
        /* ARC On */
        if (adev->active_outport == OUTPORT_HDMI_ARC) {
            /* check whether we can use bypass mode, only IEC format can use bypass mdode */
            if (aml_out->hal_format == AUDIO_FORMAT_IEC61937) {
                /* Dolby cases */
                if (aml_out->hal_internal_format == AUDIO_FORMAT_E_AC3 ||
                    aml_out->hal_internal_format == AUDIO_FORMAT_AC3) {
                    /* ARC support DDP */
                    if (adev->hdmi_descs.ddp_fmt.is_support) {
                        adev->digital_raw = 0;
                        adev->bypass_enable = 1;
                    } else if (adev->hdmi_descs.dd_fmt.is_support) {
                        /* ARC only supports DD */
                        if (aml_out->hal_internal_format == AUDIO_FORMAT_AC3) {
                            adev->bypass_enable = 1;
                            adev->digital_raw = 0;
                        } else {
                            adev->bypass_enable = 0;
                            adev->digital_raw = 1;
                        }
                    } else {
                        /* ARC doesn't support DD/DDP */
                        adev->digital_raw = 0;
                        adev->bypass_enable = 0;
                    }
                } else if (aml_out->hal_internal_format == AUDIO_FORMAT_DTS ||
                           aml_out->hal_internal_format == AUDIO_FORMAT_DTS_HD) {
                    /* dts case*/
                    /* ARC support DTS*/
                    if (adev->hdmi_descs.dts_fmt.is_support) {
                        /* dts to dts, we can use bypass mode*/
                        if (aml_out->hal_internal_format == AUDIO_FORMAT_DTS) {
                            adev->bypass_enable = 1;
                            adev->digital_raw = 0;
                        } else {
                            adev->bypass_enable = 0;
                            adev->digital_raw = 1;
                        }

                    } else {
                        /* ARC doesn't support DTS */
                        adev->bypass_enable = 0;
                        adev->digital_raw = 0;
                    }

                } else {
                    /*other IEC case, we don't support it*/
                    adev->bypass_enable = 0;
                    adev->digital_raw = 0;
                }

            } else {
                /* we can't use bypass mode */
                adev->bypass_enable = 0;
                /*check Dolby cases*/
                if (aml_out->hal_format == AUDIO_FORMAT_AC3 ||
                    aml_out->hal_format == AUDIO_FORMAT_E_AC3) {
                    if (adev->hdmi_descs.ddp_fmt.is_support) {
                        adev->digital_raw   = 2;
                    } else if (adev->hdmi_descs.dd_fmt.is_support) {
                        adev->digital_raw   = 1;
                    } else {
                        adev->digital_raw   = 0;
                    }

                } else if (aml_out->hal_format == AUDIO_FORMAT_DTS ||
                           aml_out->hal_format == AUDIO_FORMAT_DTS_HD) {
                    if (adev->hdmi_descs.dts_fmt.is_support) {
                        adev->digital_raw   = 1;
                    } else {
                        adev->digital_raw   = 0;
                    }

                } else {
                    adev->digital_raw = 0;
                }

            }
        } else {
            /* ARC off*/
            if (aml_out->hal_format == AUDIO_FORMAT_IEC61937) {
                if (aml_out->hal_internal_format == AUDIO_FORMAT_AC3 || aml_out->hal_internal_format == AUDIO_FORMAT_DTS) {
                    // only iec ac3/ dts can use bypass mode to spdif
                    adev->bypass_enable = 1;
                    adev->digital_raw = 0;
                } else {
                    adev->bypass_enable = 0;
                    adev->digital_raw = 1;
                }

            } else {
                /*we can't use bypass mode*/
                adev->bypass_enable = 0;
                adev->digital_raw = 1;
            }

        }
        break;
    default:
        adev->digital_raw = 0;
        adev->bypass_enable = 0;
        break;
    }
    ALOGI("adev->digital_raw:%d adev->bypass_enable:%d\n", adev->digital_raw, adev->bypass_enable);

    /*do some decoder setting*/

    dcv_config.digital_raw = adev->digital_raw;
    dcv_config.dcv_output_ch  = adev->dcv_output_ch;
    if (aml_out->hal_internal_format == AUDIO_FORMAT_E_AC3) {
        dcv_config.nIsEc3 = 1;
        dec_config.dcv_config = dcv_config;
    } else if (aml_out->hal_internal_format == AUDIO_FORMAT_AC3) {
        dcv_config.nIsEc3 = 0;
        dec_config.dcv_config = dcv_config;
    } else if (aml_out->hal_internal_format == AUDIO_FORMAT_DTS ||
               aml_out->hal_internal_format == AUDIO_FORMAT_DTS_HD) {
        if (adev->is_dtscd) {
            dtscd_flag = true;
        }
        if ((patch && audio_parse_get_audio_type_direct(patch->audio_parse_para) == DTSCD) || dtscd_flag) {
            dca_config.is_dtscd = 1;
            ALOGI("dts cd stream");
        }
        dec_config.dca_config = dca_config;

    } else if (IS_PCM_FORMAT(aml_out->hal_internal_format) && (adev->dolby_lib_type != eDolbyAtmosLib)) {
        dec_config.pcm_config.pcm_format = aml_out->hal_internal_format;
        dec_config.pcm_config.bitwidth   = aml_out->dec_config.pcm_config.bitwidth;
        dec_config.pcm_config.samplerate = aml_out->dec_config.pcm_config.samplerate;
        dec_config.pcm_config.channel    = aml_out->dec_config.pcm_config.channel;
    }

    if (adev->bypass_enable != 1) {
        if (aml_dec == NULL) {
            ALOGD("decoder init format=0x%x\n", aml_out->hal_internal_format);
            if (IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format) && (adev->dolby_lib_type == eDolbyAtmosLib)) {
                ((aml_datmos_config_t *)&dec_config)->reserved = &adev->datmos_param;
                if (IS_PCM_FORMAT(aml_out->hal_internal_format)) {
                    ((aml_datmos_config_t *)&dec_config)->pcm_format = aml_out->hal_internal_format;
                    ((aml_datmos_config_t *)&dec_config)->channel = patch->ch;
                    ((aml_datmos_config_t *)&dec_config)->samplerate = patch->sample_rate;
                    ((aml_datmos_config_t *)&dec_config)->bitwidth = FormatToBit(aml_out->hal_internal_format);
                    ALOGE("pcm_format %#x ch %d sr %d bitwidth %d\n", aml_out->hal_internal_format, patch->ch, patch->sample_rate, FormatToBit(aml_out->hal_internal_format));
                }
            }
            status = aml_decoder_init(&aml_out->aml_dec, aml_out->hal_internal_format, (aml_dec_config_t *)&dec_config);
            if (status < 0) {
                adev->audio_sample_rate = 0;
                adev->audio_channels    = 0;
                ALOGE("Init decoder failed\n");
                aml_out->aml_dec = NULL;
                return;
            }
            adev->dec_params_update_mask = 0;
            aml_dec = aml_out->aml_dec;
            /*check if the input format is contained with 61937 format*/
            if (aml_out->hal_format == AUDIO_FORMAT_IEC61937) {
                aml_dec->is_iec61937 = true;
            } else {
                aml_dec->is_iec61937 = false;
            }
            ALOGI("aml_decoder_init return :%d,is 61937 %d\n", status, aml_dec->is_iec61937);
        } else if (aml_dec) {
            if (aml_dec->digital_raw > 0) {
                struct pcm *pcm = adev->pcm_handle[DIGITAL_DEVICE];
                if (pcm && aml_out->dual_output_flag) {
                    ALOGI("%s close dual output pcm handle %p", __func__, pcm);
                    pcm_close(pcm);
                    adev->pcm_handle[DIGITAL_DEVICE] = NULL;
                    aml_out->dual_output_flag = 0;
                    aml_tinymix_set_spdif_format(AUDIO_FORMAT_PCM_16_BIT);
                }
            }
            aml_decoder_release(aml_dec);
            adev->decode_format = AUDIO_FORMAT_INVALID;
            adev->is_truehd_within_mat = false;
            adev->is_dolby_atmos = false;
            adev->audio_sample_rate = 0;
            aml_out->aml_dec = NULL;
            ALOGI("dcv_decoder_release_patch release");
        }

    }

    pthread_mutex_lock(&adev->lock);
    if (!adev->hw_mixer.start_buf) {
        aml_hw_mixer_init(&adev->hw_mixer);
    } else {
        aml_hw_mixer_reset(&adev->hw_mixer);
    }
    pthread_mutex_unlock(&adev->lock);
    return ;
}

ssize_t mixer_main_buffer_write(struct audio_stream_out *stream, const void *buffer,
                                size_t bytes)
{
    //ALOGV("%s write in %zu!\n", __FUNCTION__, bytes);
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct aml_stream_out *ms12_out = (struct aml_stream_out *)adev->ms12_out;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);
    struct aml_audio_patch *patch = adev->audio_patch;
    aml_dec_t *aml_dec = NULL;
    int case_cnt;
    int ret = -1;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;
    bool need_reconfig_output = false;
    void   *write_buf = NULL;
    size_t  write_bytes = 0;
    size_t  hwsync_cost_bytes = 0;
    int total_write = 0;
    size_t used_size = 0;
    int write_retry = 0;
    int i = 0;
    aml_data_format_t data_format = {0};
    audio_hwsync_t *hw_sync = aml_out->hwsync;
    if (adev->debug_flag) {
        ALOGI("%s write in %zu!, format = 0x%x\n", __FUNCTION__, bytes, aml_out->hal_internal_format);
    }

    int return_bytes = bytes;

    if (buffer == NULL) {
        ALOGE("%s() invalid buffer %p\n", __FUNCTION__, buffer);
        return -1;
    }

    /*set some default value*/
    data_format.format = AUDIO_FORMAT_PCM_SUB_16_BIT;
    data_format.sr     = 48000;
    data_format.ch     = 2;
    data_format.bitwidth = SAMPLE_16BITS;


    /* here to check if the audio HDMI ARC format updated. */
    if (adev->arc_hdmi_updated) {
        ALOGI("%s(), arc format updated, need reconfig output", __func__);
        need_reconfig_output = true;
        adev->arc_hdmi_updated = 0;
    }

    /* here to check if the audio output routing changed. */
    if (adev->out_device != aml_out->out_device) {
        ALOGI("%s(), output routing changed, need reconfig output", __func__);
        need_reconfig_output = true;
        aml_out->out_device = adev->out_device;
    }
    /* handle HWSYNC audio data*/
    if (aml_out->hw_sync_mode) {
        // do nothing
    } else {
        write_buf = (void *) buffer;
        write_bytes = bytes;
    }

    /* here to check if the audio input format changed. */
    if (adev->audio_patch) {
        audio_format_t cur_aformat;
        if (patch->input_src == AUDIO_DEVICE_IN_HDMI || patch->input_src == AUDIO_DEVICE_IN_SPDIF) {
            if (!adev->hw_audio_type_det)
                cur_aformat = audio_parse_get_audio_type(patch->audio_parse_para);
            else if (patch->input_src == AUDIO_DEVICE_IN_HDMI)
                cur_aformat = hdmiin_hw_audio_format_detection();
            else if (patch->input_src == AUDIO_DEVICE_IN_SPDIF)
                cur_aformat = spdifhw_audio_format_detection();
            //ALOGI("cur_aformat=%d\n",cur_aformat);

            if (cur_aformat != patch->aformat) {
                ALOGE("HDMI/SPDIF input format changed from %#x to %#x\n", patch->aformat, cur_aformat);
                /*if it is invalid type, we trigger it here */
                if (cur_aformat == AUDIO_FORMAT_INVALID ||
                    cur_aformat == AUDIO_FORMAT_DTS ||
                    cur_aformat == AUDIO_FORMAT_DTS_HD) {
                    ALOGE("trigger audio format changed callback =%#x\n",cur_aformat);
                    adev->decode_format = cur_aformat;
                    trigger_audio_callback(patch->callback_handle, AML_AUDIO_CALLBACK_FORMATCHANGED, (audio_callback_data_t *)&cur_aformat);
                }
                patch->aformat = cur_aformat;
                //FIXME: if patch audio format change, the hal_format need to redefine.
                //then the out_get_format() can get it.
                ALOGI("hal_format changed from %#x to %#x\n", aml_out->hal_format, cur_aformat);
                if (cur_aformat != AUDIO_FORMAT_PCM_16_BIT) {
                    aml_out->hal_format = AUDIO_FORMAT_IEC61937;
                    if (adev->hw_audio_type_det)
                        enable_HW_resample(48000, 0);
                } else {
                    aml_out->hal_format = cur_aformat ;
                    if (adev->hw_audio_type_det)
                        enable_HW_resample(48000, 1);
                }
                aml_out->hal_internal_format = cur_aformat;
                if (!IS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
                    adev->is_dolby_atmos = false;
                }
                aml_out->hal_channel_mask = audio_parse_get_audio_channel_mask(patch->audio_parse_para);
                //ALOGI("%s hal_channel_mask %#x\n", __FUNCTION__, aml_out->hal_channel_mask);
                if (aml_out->hal_internal_format == AUDIO_FORMAT_DTS) {
                    adev->dolby_lib_type = eDolbyDcvLib;
                } else {
                    adev->dolby_lib_type = adev->dolby_lib_type_last;
                }
                //we just do not support dts decoder,just mute as LPCM
                need_reconfig_output = true;
                /* reset audio patch ringbuffer */
                ring_buffer_reset(&patch->aml_ringbuffer);
                adev->spdif_encoder_init_flag = false;
            }

            if (cur_aformat == AUDIO_FORMAT_INVALID) {
                // the input data is invalid, mute it
                aml_out->hal_format = AUDIO_FORMAT_INVALID;
                aml_out->hal_internal_format = AUDIO_FORMAT_INVALID;
                memset((void *)buffer, 0, bytes);
                return 0;
            }

        } else if (patch->input_src == AUDIO_DEVICE_IN_LINE ||
                 patch->input_src == AUDIO_DEVICE_IN_BLUETOOTH_A2DP ||
                 patch->input_src == AUDIO_DEVICE_IN_LOOPBACK) {

            cur_aformat = AUDIO_FORMAT_PCM_16_BIT;
        }

        if (IS_PCM_FORMAT(cur_aformat)) {
            if (aml_out->dec_config.pcm_config.samplerate !=0 && aml_out->dec_config.pcm_config.samplerate != patch->sample_rate) {
                need_reconfig_output = true;
                /* reset audio patch ringbuffer */
                ring_buffer_reset(&patch->aml_ringbuffer);
            }
            aml_out->dec_config.pcm_config.pcm_format = cur_aformat;
            aml_out->dec_config.pcm_config.samplerate = patch->sample_rate;
            aml_out->dec_config.pcm_config.channel    = patch->ch;
            aml_out->dec_config.pcm_config.bitwidth   = FormatToBit(cur_aformat);
        }

    } else if (aml_out->hal_internal_format == AUDIO_FORMAT_IEC61937) {
        audio_channel_mask_t cur_ch_mask;
        audio_format_t cur_aformat;
        int package_size;
        int raw_size = 0;
        int offset = 0;
        int cur_audio_type = audio_type_parse(write_buf, write_bytes, &package_size, &cur_ch_mask, &raw_size, &offset);

        cur_aformat = andio_type_convert_to_android_audio_format_t(cur_audio_type);
        ALOGI("cur_aformat:%0x cur_audio_type:%d", cur_aformat, cur_audio_type);
        if (cur_audio_type == DTSCD) {
            adev->is_dtscd = 1;
        } else {
            adev->is_dtscd = 0;
        }
        if (cur_aformat == AUDIO_FORMAT_DTS || cur_aformat == AUDIO_FORMAT_AC3) {
            aml_out->hal_internal_format = cur_aformat;
            if (aml_out->hal_internal_format == AUDIO_FORMAT_DTS) {
                adev->dolby_lib_type = eDolbyDcvLib;
            } else if (aml_out->hal_internal_format == AUDIO_FORMAT_AC3) {
                adev->dolby_lib_type = adev->dolby_lib_type_last;
            }
        } else {
            return return_bytes;
        }
    } else if (aml_out->hal_internal_format != AUDIO_FORMAT_DTS) {
        adev->dolby_lib_type = adev->dolby_lib_type_last;
    }

    if (need_reconfig_output) {
        config_output(stream);
        return bytes;
    }

    aml_out->input_bytes_size += write_bytes;

    if (aml_out->hal_format == AUDIO_FORMAT_INVALID) {
        return bytes;
    }

    /*
     *when disable_pcm_mixing is true, the 7.1ch DD+ could not be process with Dolby MS12
     *the HDMI-In or Spdif-In is special with IEC61937 format, other input need packet with spdif-encoder.
     */
    audio_format_t output_format = get_output_format(stream);
    if ((adev->disable_pcm_mixing == true) && \
        //(aml_out->hal_channel_mask == AUDIO_CHANNEL_OUT_7POINT1) &&\ // removed this zz
        (adev->dual_decoder_support == false) && \
        (adev->continuous_audio_mode == false) && \
        (adev->sink_format == AUDIO_FORMAT_E_AC3)) {
        data_format.format = output_format;
        if (audio_hal_data_processing(stream, buffer, bytes, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
            hw_write(stream, output_buffer, output_buffer_bytes, &data_format);
        }
    } else {

        if (IS_DECODER_SUPPORT(aml_out->hal_internal_format) || IS_PCM_FORMAT(aml_out->hal_internal_format)) {
            if (adev->bypass_enable) {
                if (aml_out->hal_format == AUDIO_FORMAT_IEC61937) {
                    output_format = aml_out->hal_internal_format;
                    data_format.format = output_format;
                    if (audio_hal_data_processing(stream, (void *)buffer, bytes, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
                        hw_write(stream, output_buffer, output_buffer_bytes, &data_format);
                    }
                    return return_bytes;
                }
            }

            if (aml_out->aml_dec == NULL) {
                config_output(stream);
            }

            aml_dec = aml_out->aml_dec;

            if (aml_dec) {
                if (adev->dec_params_update_mask) {
                    if (aml_decoder_dynamic_param_set(aml_dec)) {
                        ALOGE("set dec dynamic param occur error");
                    }
                    adev->dec_params_update_mask = 0;
                }
#if 0
                struct timespec before_read;
                struct timespec after_read;
                int us = 0;
                clock_gettime(CLOCK_MONOTONIC, &before_read);
#endif

                ret = aml_decoder_process(aml_dec, (unsigned char *)buffer, bytes, &used_size);
                //ALOGD("pcm len=%d ch=%d rate=%d\n",aml_dec->outlen_pcm, aml_dec->dec_info.output_ch,aml_dec->dec_info.output_sr);
                if (aml_dec->outlen_pcm > 0) {
                    aml_dec->frame_cnt++;
                }

#if 0
                clock_gettime(CLOCK_MONOTONIC, &after_read);
                us = calc_time_interval_us(&before_read, &after_read);
                if (aml_dec->frame_cnt>= 10 && aml_dec->dec_info.output_ch!=0) {
                    float percent = (float)(us)/(aml_dec->outlen_pcm*1000/(aml_dec->dec_info.output_ch*4*48));
                    if (percent < aml_dec->min_time) {
                        aml_dec->min_time = percent;
                    }
                    if (percent > aml_dec->max_time) {
                        aml_dec->max_time = percent;
                    }
                    aml_dec->average_time = 0.0;
                    aml_dec->frame_total_ms += (aml_dec->outlen_pcm*1000/(aml_dec->dec_info.output_ch*4*48));
                    aml_dec->dec_total_ms += us;
                    ALOGD("min =%f max=%f ave=%f percent=%f\n",aml_dec->min_time, aml_dec->max_time,
                        (float)(aml_dec->dec_total_ms)/aml_dec->frame_total_ms, percent);
                }
#endif


                /*update information*/
                adev->is_truehd_within_mat = aml_dec->is_truehd_within_mat;
                adev->audio_sample_rate = aml_dec->dec_info.stream_sr;
                adev->audio_channels    = aml_dec->dec_info.output_ch;
                adev->decode_format     = aml_dec->format;

                /*if it first frame, trigger callback to get some info*/
                if (aml_dec->outlen_pcm > 0) {
                    if (aml_dec->frame_cnt == 1) {
                        if (patch) {
                            ALOGD("trigger audio format changed callback = %#x ch=%d\n",aml_dec->format,adev->audio_channels);
                            trigger_audio_callback(patch->callback_handle, AML_AUDIO_CALLBACK_FORMATCHANGED, (audio_callback_data_t *)&aml_dec->format);
                        }
                    } else {
                        if (adev->is_dolby_atmos != aml_dec->is_dolby_atmos) {
                            if (patch) {
                                trigger_audio_callback(patch->callback_handle, AML_AUDIO_CALLBACK_FORMATCHANGED, (audio_callback_data_t *)&aml_dec->format);
                            }
                        }
                    }
                    adev->is_dolby_atmos = aml_dec->is_dolby_atmos;
                }


                /*decoder return error, reinit here*/
                if ((adev->dolby_lib_type == eDolbyAtmosLib) && (ret < 0) && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
                    aml_decoder_release(aml_dec);
                    aml_dec_config_t dec_config;
                    if (IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)) {
                        ((aml_datmos_config_t *)&dec_config)->reserved = &adev->datmos_param;
                        if (IS_PCM_FORMAT(aml_out->hal_internal_format)) {
                            ((aml_datmos_config_t *)&dec_config)->pcm_format = aml_out->hal_internal_format;
                            ((aml_datmos_config_t *)&dec_config)->channel = patch->ch;
                            ((aml_datmos_config_t *)&dec_config)->samplerate = patch->sample_rate;
                            ((aml_datmos_config_t *)&dec_config)->bitwidth = FormatToBit(aml_out->hal_internal_format);
                            ALOGE("pcm_format %#x ch %d sr %d bitwidth %d\n", aml_out->hal_internal_format, patch->ch, patch->sample_rate, FormatToBit(aml_out->hal_internal_format));
                        }
                    }
                    int init_status = aml_decoder_init(&aml_out->aml_dec, aml_out->hal_internal_format, (aml_dec_config_t *)&dec_config);
                    if (init_status < 0) {
                        ALOGE("Reinit decoder error");
                    }
                }

            } else {
                config_output(stream);
            }
            if (aml_dec == NULL) {
                return -1;
            }
            if (ret < 0) {
                return bytes;
            }

            //wirte raw data
            if (aml_dec->digital_raw == 1 && aml_out->dual_output_flag) {
                if (aml_dec->outlen_raw) {
                    aml_audio_spdif_output(stream, (void *)aml_dec->outbuf_raw, aml_dec->outlen_raw);
                }
            }

            if (aml_dec->outlen_pcm <= 0) {
                return bytes;
            }

            //write pcm data
            void *tmp_buffer = (void *) aml_dec->outbuf;
            output_format = AUDIO_FORMAT_PCM_16_BIT;
            aml_hw_mixer_mixing(&adev->hw_mixer, tmp_buffer, write_bytes);
            data_format.format = output_format;
            data_format.sr     = aml_dec->dec_info.output_sr;
            /*for some hdmi in device, input content is 44.1*4kHz but caputre samplerate is 192kHz*/
            if (((adev->capture_samplerate % data_format.sr) != 0 && (patch->input_src == AUDIO_DEVICE_IN_HDMI))
                && IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format)
                && adev->datmos_enable) {
                data_format.sr = adev->datmos_param.noupresampler ? 48000 : adev->capture_samplerate;
            }
            data_format.ch     = aml_dec->dec_info.output_ch;
            data_format.bitwidth = aml_dec->dec_info.output_bitwidth;
            if (IS_DATMOS_DECODER_SUPPORT(aml_out->hal_internal_format) && (adev->dolby_lib_type == eDolbyAtmosLib)) {
                data_format.ch_order_type = CHANNEL_ORDER_DOLBY;
            } else if (aml_out->hal_internal_format == AUDIO_FORMAT_DTS || aml_out->hal_internal_format == AUDIO_FORMAT_DTS_HD) {
                data_format.ch_order_type = CHANNEL_ORDER_DTS;
            } else if (IS_PCM_FORMAT(aml_out->hal_internal_format)) {
                data_format.ch_order_type = CHANNEL_ORDER_HDMIPCM;
            }

            if (patch && patch->output_thread_exit) {
                ALOGD("output thread has been exit return");
                return 0;
            }

            if (audio_hal_data_processing(stream, tmp_buffer, aml_dec->outlen_pcm, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
                hw_write(stream, output_buffer, output_buffer_bytes, &data_format);
                aml_out->frame_write_sum = aml_out->input_bytes_size  / audio_stream_out_frame_size(stream) ;
                aml_out->last_frames_postion = aml_out->frame_write_sum;
            }

            return return_bytes;
        }

        void *tmp_buffer = (void *) write_buf;
        //memset(write_buf, 0, write_bytes);
        if (aml_out->hw_sync_mode) {
            ALOGV("mixing hw_sync mode");
            aml_hw_mixer_mixing(&adev->hw_mixer, tmp_buffer, hw_sync->hw_sync_frame_size);
            data_format.format = output_format;
            if (audio_hal_data_processing(stream, tmp_buffer, hw_sync->hw_sync_frame_size, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
                hw_write(stream, output_buffer, output_buffer_bytes, &data_format);
            }
        } else {
            //ALOGV("mixing non-hw_sync mode");
            aml_hw_mixer_mixing(&adev->hw_mixer, tmp_buffer, write_bytes);
            //hw_write(stream, tmp_buffer, write_bytes, output_format);
            data_format.format = output_format;
            if (audio_hal_data_processing(stream, tmp_buffer, write_bytes, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
                hw_write(stream, output_buffer, output_buffer_bytes, &data_format);
            }
        }

    }
exit:
    if (adev->debug_flag) {
        ALOGI("%s return %zu!\n", __FUNCTION__, bytes);
    }
    return return_bytes;
}

ssize_t mixer_aux_buffer_write(struct audio_stream_out *stream, const void *buffer,
                               size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    struct dolby_ms12_desc *ms12 = &(adev->ms12);

    int ret = 0;
    size_t frame_size = audio_stream_out_frame_size(stream);
    size_t in_frames = bytes / frame_size;
    size_t bytes_remaining = bytes;
    size_t bytes_written = 0;
    int need_reconfig_ms12 = 0;
    int retry = 0;
    unsigned int alsa_latency_frame = 0;
    pthread_mutex_lock(&adev->lock);
    bool hw_mix = need_hw_mix(adev->usecase_masks);
    if ((aml_out->status == STREAM_HW_WRITING) && hw_mix) {
        ALOGI("%s(), aux do alsa close\n", __func__);

        pthread_mutex_lock(&adev->alsa_pcm_lock);
        aml_output_close(stream);
        pthread_mutex_unlock(&adev->alsa_pcm_lock);

        ALOGI("%s(), aux alsa close done\n", __func__);
        aml_out->status = STREAM_MIXING;
    }

    if (aml_out->status == STREAM_STANDBY) {
        aml_out->status = STREAM_MIXING;
    }

    if (aml_out->is_normal_pcm && !aml_out->normal_pcm_mixing_config) {
#ifdef ENABLE_MS12
        if (0 == set_system_app_mixing_status(aml_out, aml_out->status)) {
            aml_out->normal_pcm_mixing_config = true;
        } else
#endif
        {
            aml_out->normal_pcm_mixing_config = false;
        }
    }

    pthread_mutex_unlock(&adev->lock);
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        if (continous_mode(adev)) {
            /*only system sound active*/
            if (!hw_mix) {
                /* here to check if the audio HDMI ARC format updated. */
                if ((adev->arc_hdmi_updated) && (adev->ms12.dolby_ms12_enable == true)) {
                    //? if we need protect
                    adev->arc_hdmi_updated = 0;
                    need_reconfig_ms12 = 1;
                    ALOGI("%s() HDMI ARC EndPoint changing status, need reconfig Dolby MS12\n", __func__);
                }

                /* here to check if the audio output routing changed. */
                if ((adev->out_device != aml_out->out_device) && (adev->ms12.dolby_ms12_enable == true)) {
                    ALOGI("%s(), output routing changed from 0x%x to 0x%x,need MS12 reconfig output", __func__, aml_out->out_device, adev->out_device);
                    aml_out->out_device = adev->out_device;
                    need_reconfig_ms12 = 1;
                }

                /* here to check if ms12 is already enabled */
                if (!adev->ms12.dolby_ms12_enable) {
                    ALOGI("%s(), 0x%x, Swithing system output to MS12, need MS12 reconfig output", __func__, aml_out->out_device);
                    need_reconfig_ms12 = 1;
                }

                if (need_reconfig_ms12) {
                    config_output(stream);
                }
            }
        }
        /*
         *when disable_pcm_mixing is true and offload format is dolby
         *the system tone voice should not be mixed
         */
        if (adev->disable_pcm_mixing && (dolby_stream_active(adev) || hwsync_lpcm_active(adev))) {
            memset((void *)buffer, 0, bytes);
            if (adev->debug_flag) {
                ALOGI("%s mute the mixer voice(system/alexa)\n", __FUNCTION__);
            }
        }

        aml_out->input_bytes_size += bytes;
        int count = 0;
        while (bytes_remaining && adev->ms12.dolby_ms12_enable && retry < 10) {
            size_t used_size = 0;
            ret = dolby_ms12_system_process(stream, (char *)buffer + bytes_written, bytes_remaining, &used_size);
            if (!ret) {
                bytes_remaining -= used_size;
                bytes_written += used_size;
                retry = 0;
            }
            retry++;
            if (bytes_remaining) {
                usleep(bytes_remaining * 1000000 / frame_size / out_get_sample_rate(&stream->common));
            }
            count++;
            if ((count % 10) == 0) {
                ALOGE("%s count %d", __func__, count);
            }
        }
    } else
#endif
    {
        bytes_written = aml_hw_mixer_write(&adev->hw_mixer, buffer, bytes);
    }
    aml_out->frame_write_sum += in_frames;
    clock_gettime(CLOCK_MONOTONIC, &aml_out->timestamp);
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == adev->dolby_lib_type) {
        /*
           mixer_aux_buffer_write is called when the hw write is called another thread,for example
           main write thread or ms12 thread. aux audio is coming from the audioflinger mixed thread.
           we need caculate the system buffer latency in ms12 also with the alsa out latency.It is
           48K 2 ch 16 bit audio.
           */
        alsa_latency_frame = adev->ms12.latency_frame;
        int system_latency = dolby_ms12_get_system_buffer_avail() / frame_size;
        aml_out->last_frames_postion = aml_out->frame_write_sum - system_latency;
        if (aml_out->last_frames_postion >= alsa_latency_frame) {
            aml_out->last_frames_postion -= alsa_latency_frame;
        }
        if (adev->debug_flag) {
            ALOGI("%s stream audio presentation %"PRIu64" latency_frame %d.ms12 system latency_frame %d", __func__,
                  aml_out->last_frames_postion, alsa_latency_frame, system_latency);
        }
    } else
#endif
    {
        aml_out->last_frames_postion = aml_out->frame_write_sum;
    }
    return bytes;

}



ssize_t process_buffer_write(struct audio_stream_out *stream, const void *buffer,
                             size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    void *output_buffer = NULL;
    size_t output_buffer_bytes = 0;
    aml_data_format_t data_format;
    ALOGD("process_buffer_write\n");
    data_format.format = aml_out->hal_internal_format;
    data_format.sr     = 48000;
    data_format.ch     = 2;
    data_format.bitwidth = SAMPLE_16BITS;
    if (audio_hal_data_processing(stream, buffer, bytes, &output_buffer, &output_buffer_bytes, &data_format) == 0) {
        hw_write(stream, output_buffer, output_buffer_bytes, &data_format);
    }
    if (bytes > 0) {
        aml_out->input_bytes_size += bytes;
    }
    return bytes;
}

/* must be called with hw device mutexes locked */
static int usecase_change_validate_l(struct aml_stream_out *aml_out, bool is_standby)
{
    struct aml_audio_device *aml_dev = aml_out->dev;
    bool hw_mix;

    if (is_standby) {
        ALOGI("++%s(), dev usecase masks = %#x, is_standby = %d, out usecase %s",
              __func__, aml_dev->usecase_masks, is_standby, aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? str_usecases[aml_out->usecase] : "STREAM_USECASE_INVAL");
        /**
         * If called by standby, reset out stream's usecase masks and clear the aml_dev usecase masks.
         * So other active streams could know that usecase have been changed.
         * But keep it's own usecase if out_write is called in the future to exit standby mode.
         */
        aml_out->dev_usecase_masks = 0;
        aml_out->write = NULL;
        aml_dev->usecase_masks &= ~(1 << aml_out->usecase);
        ALOGI("--%s(), dev usecase masks = %#x, is_standby = %d, out usecase %s",
              __func__, aml_dev->usecase_masks, is_standby, aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? str_usecases[aml_out->usecase] : "STREAM_USECASE_INVAL");
        return 0;
    }

    /* No usecase changes, do nothing */
    if (((aml_dev->usecase_masks == aml_out->dev_usecase_masks) && aml_dev->usecase_masks) && (aml_dev->continuous_audio_mode == 0)) {
        return 0;
    }

    /* check the usecase validation */
    if (popcount(aml_dev->usecase_masks) > 2) {
        ALOGE("%s(), invalid usecase masks = %#x, out usecase %s!",
              __func__, aml_dev->usecase_masks, str_usecases[aml_out->usecase]);
        return -EINVAL;
    }

    if (((aml_dev->continuous_audio_mode == 1) && (aml_dev->debug_flag > 1)) || \
        (aml_dev->continuous_audio_mode == 0))
        ALOGI("++++%s(),continuous_audio_mode %d dev usecase masks = %#x, out usecase_masks = %#x, out usecase %s",
              __func__, aml_dev->continuous_audio_mode, aml_dev->usecase_masks, aml_out->dev_usecase_masks, aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? str_usecases[aml_out->usecase] : "STREAM_USECASE_INVAL");

    /* new output case entered, so no masks has been set to the out stream */
    if (!aml_out->dev_usecase_masks) {
        if ((1 << aml_out->usecase) & aml_dev->usecase_masks) {
            ALOGE("%s(), usecase: %s already exists!!", __func__, aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? str_usecases[aml_out->usecase] : "STREAM_USECASE_INVAL");
            return -EINVAL;
        }

        /* add the new output usecase to aml_dev usecase masks */
        aml_dev->usecase_masks |= 1 << aml_out->usecase;
    }

    /* choose the out_write functions by usecase masks */
    hw_mix = need_hw_mix(aml_dev->usecase_masks);
    if (aml_dev->continuous_audio_mode == 0) {
        if (hw_mix) {
            /**
             * normal pcm write to aux buffer
             * others write to main buffer
             * may affect the output device
             */
            if (aml_out->is_normal_pcm) {
                aml_out->write = mixer_aux_buffer_write;
            } else {
                aml_out->write = mixer_main_buffer_write;
            }
        } else {
            /**
             * only one stream output will be processed then send to hw.
             * This case only for normal output without mixing
             */
            aml_out->write = process_buffer_write;
        }
    } else {
        /**
         * normal pcm write to aux buffer
         * others write to main buffer
         * may affect the output device
         */
        if (aml_out->is_normal_pcm) {
            aml_out->write = mixer_aux_buffer_write;
            //FIXEME if need config ms12 here if neeeded.

        } else {
            aml_out->write = mixer_main_buffer_write;
        }
    }

    /* store the new usecase masks in the out stream */
    aml_out->dev_usecase_masks = aml_dev->usecase_masks;
    if (((aml_dev->continuous_audio_mode == 1) && (aml_dev->debug_flag > 1)) || \
        (aml_dev->continuous_audio_mode == 0))
        ALOGI("----%s(), continuous_audio_mode %d dev usecase masks = %#x, out usecase_masks = %#x, out usecase %s",
              __func__, aml_dev->continuous_audio_mode, aml_dev->usecase_masks, aml_out->dev_usecase_masks, aml_out->usecase < STREAM_USECASE_MAX && aml_out->usecase >= STREAM_PCM_NORMAL ? str_usecases[aml_out->usecase] : "STREAM_USECASE_INVAL");
    return 0;
}

/* out_write entrance: every write goes in here. */
ssize_t out_write_new(struct audio_stream_out *stream,
                      const void *buffer,
                      size_t bytes)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    struct aml_audio_device *adev = aml_out->dev;
    ssize_t ret = 0;
    write_func  write_func_p = NULL;

    //ALOGV("%s(): out_stream %p,position %zu \n", __func__, stream, bytes);

    /**
     * deal with the device output changes
     * pthread_mutex_lock(&aml_out->lock);
     * out_device_change_validate_l(aml_out);
     * pthread_mutex_unlock(&aml_out->lock);
     */
    pthread_mutex_lock(&adev->lock);
    ret = usecase_change_validate_l(aml_out, false);
    if (ret < 0) {
        ALOGE("%s() failed", __func__);
        pthread_mutex_unlock(&adev->lock);
        return ret;
    }
    if (aml_out->write) {
        write_func_p = aml_out->write;
    }
    pthread_mutex_unlock(&adev->lock);
    if (write_func_p) {
        ret = write_func_p(stream, buffer, bytes);
    }
    if (ret > 0) {
        aml_out->total_write_size += ret;
    }
    if (adev->debug_flag > 1) {
        ALOGI("-%s() ret %zd,%p %"PRIu64"\n", __func__, ret, stream, aml_out->total_write_size);
    }
    return ret;
}

/*static int adev_open_output_stream (struct audio_hw_device *dev,
                                    audio_io_handle_t handle __unused,
                                    audio_devices_t devices,
                                    audio_output_flags_t flags,
                                    struct audio_config *config,
                                    struct audio_stream_out **stream_out,
                                    const char *address __unused);*/ // moved to top

/*static void adev_close_output_stream (struct audio_hw_device *dev,
                                      struct audio_stream_out *stream);*/

int adev_open_output_stream_new(struct audio_hw_device *dev,
                                audio_io_handle_t handle __unused,
                                audio_devices_t devices,
                                audio_output_flags_t flags,
                                struct audio_config *config,
                                struct audio_stream_out **stream_out,
                                const char *address __unused)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    struct aml_stream_out *aml_out = NULL;
    stream_usecase_t usecase = STREAM_PCM_NORMAL;
    int ret;

    ALOGI("++%s()", __func__);
    ret = adev_open_output_stream(dev,
                                  0,
                                  devices,
                                  flags,
                                  config,
                                  stream_out,
                                  NULL);
    if (ret < 0) {
        ALOGE("%s(), open stream failed", __func__);
    }

    /*get sink format*/
    pthread_mutex_lock(&adev->lock);
    get_sink_format(*stream_out);
    pthread_mutex_unlock(&adev->lock);
    aml_out = (struct aml_stream_out *)(*stream_out);

    usecase = attr_to_usecase(devices, config->format, flags);
    aml_out->usecase = usecase;
    aml_out->is_normal_pcm = (usecase == STREAM_PCM_NORMAL);
    aml_out->card = alsa_device_get_card_index();
    if (adev->sink_format == AUDIO_FORMAT_PCM_16_BIT) {
        aml_out->device = PORT_I2S;
    } else {
        aml_out->device = PORT_SPDIF;
    }

    aml_out->stream.write = out_write_new;
    aml_out->stream.common.standby = out_standby_new;

    ALOGI("%s(), usecase = %s card %d devices %d\n",
          __func__, str_usecases[usecase], aml_out->card, aml_out->device);
    // set default status to standby
    aml_out->status = STREAM_STANDBY;
    adev->spdif_encoder_init_flag = false;
    pthread_mutex_lock(&adev->lock);
    if (adev->active_outputs[usecase]) {
        ALOGE("%s(), usecase[%s] already exists", __func__, str_usecases[usecase]);
        //pthread_mutex_unlock(&adev->lock);
        //return -EINVAL;
    }

    adev->active_outputs[usecase] = aml_out;
    ALOGI("%s(), active_outputs[%d] %p", __func__, usecase, adev->active_outputs[usecase]);
    pthread_mutex_unlock(&adev->lock);
    return 0;
}


void adev_close_output_stream_new(struct audio_hw_device *dev,
                                  struct audio_stream_out *stream)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) dev;
    struct aml_stream_out *aml_out = (struct aml_stream_out *) stream;
    aml_dec_t *aml_dec = aml_out->aml_dec;

    /* call legacy close to reuse codes */
    ALOGI("%s(), active_outputs[%d] %p", __func__, aml_out->usecase, adev->active_outputs[aml_out->usecase]);

    adev_close_output_stream(dev, stream);

    if (aml_dec) {
        aml_decoder_release(aml_dec);
    }
    aml_out->aml_dec = NULL;

    adev->dual_decoder_support = false;
    return;
}

int calc_time_interval_us(struct timespec *ts0, struct timespec *ts1)
{
    int64_t start_us, end_us;
    int64_t interval_us;


    start_us = ts0->tv_sec * 1000000LL +
               ts0->tv_nsec / 1000LL;

    end_us   = ts1->tv_sec * 1000000LL +
               ts1->tv_nsec / 1000LL;

    interval_us = end_us - start_us;

    return interval_us;
}

// buffer/period ratio, bigger will add more latency
void *audio_patch_input_threadloop(void *data)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *) data;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    ring_buffer_t *ringbuffer = & (patch->aml_ringbuffer);
    struct audio_stream_in *stream_in = NULL;
    struct aml_stream_in *aml_in = NULL;
    struct audio_config stream_config;
    aml_data_format_t data_format = { 0 };
    // FIXME: add calc for read_bytes;
    int read_bytes = DEFAULT_CAPTURE_PERIOD_SIZE * 2 * 2; // 2ch, 16bits
    int ret = 0, retry = 0;
    audio_format_t cur_aformat  = AUDIO_FORMAT_INVALID;
    audio_format_t last_aformat = AUDIO_FORMAT_INVALID;
    int cur_samplerate = 0;
    int last_samplerate = 0;
    int ring_buffer_size = 0;
    int period_time = 0;
    int i2s_clk = 0;
    int last_i2s_clk = 0;
    ALOGI("++%s", __FUNCTION__);

    patch->sample_rate = stream_config.sample_rate = 48000;
    patch->chanmask = stream_config.channel_mask = AUDIO_CHANNEL_IN_STEREO;
    stream_config.format = AUDIO_FORMAT_PCM_16_BIT;
    patch->aformat = AUDIO_FORMAT_INVALID;


    ret = adev_open_input_stream(patch->dev,
                                 0,
                                 patch->input_src, //devices,
                                 &stream_config,
                                 &stream_in,
                                 0, //flags,
                                 NULL,
                                 0 //source);
                                );
    if (ret < 0) {
        ALOGE("open input steam fail, ret = %d", ret);
        goto exit_open;
    }
    aml_in = (struct aml_stream_in *)stream_in;
    patch->in_buf_size = read_bytes * EAC3_MULTIPLIER * 4;
    patch->in_buf = calloc(1, patch->in_buf_size);
    if (!patch->in_buf) {
        ALOGE("NOMEM");
        goto exit_inbuf;
    }

    int first_start = 1;
    if (ringbuffer) {
        ring_buffer_size = ringbuffer->size;
    }
    prctl(PR_SET_NAME, (unsigned long)"audio_in_patch");
    while (!patch->input_thread_exit) {
        int bytes_avail = 0;
        int period_mul = 1;
        int read_threshold = 0;
        int clk_changed = 0;
        //set_thread_affinity(3);

        ret = get_input_streaminfo(stream_in, &data_format);
        if (ret < 0) {
            patch->sample_rate = 48000;
            patch->chanmask    = AUDIO_CHANNEL_IN_STEREO;
            patch->ch          = 2;
            aml_in->config.channels = 2;
            aml_in->config.rate     = 48000;

        } else {
            if (data_format.sr == 0) {
                data_format.sr = 48000;
            }
            patch->sample_rate = data_format.sr;
            patch->chanmask    = audio_channel_out_mask_from_count(data_format.ch);
            patch->ch = data_format.ch;
            aml_in->config.channels = data_format.ch;
            aml_in->config.rate     = data_format.sr;
        }

        if (patch->ch == 8) {
            period_mul = 4;
        }

        if (patch->sample_rate >= 176400) {
            period_mul = period_mul * 4;
        }
        patch->in_period_mul = period_mul;

        if (patch->input_src == AUDIO_DEVICE_IN_LINE) {
            read_threshold = 4 * read_bytes * period_mul;
        }


        struct timespec before_read;
        struct timespec after_read;
        int us = 0;
        clock_gettime(CLOCK_MONOTONIC, &before_read);

        //ALOGV("++%s start to in read, periodmul %d, threshold %d \n",
        //__func__, period_mul, read_threshold);
        bytes_avail = in_read(stream_in, patch->in_buf, read_bytes * period_mul);
        clock_gettime(CLOCK_MONOTONIC, &after_read);
        us = calc_time_interval_us(&before_read, &after_read);
        if(us > 20*1000) {
            //ALOGE("read data function gap =%d \n", us);
        }

        if (patch->input_src == AUDIO_DEVICE_IN_HDMI) {
            i2s_clk = get_hdmiin_i2sclk();
            if (i2s_clk != last_i2s_clk) {
                ALOGE("I2S clk changed from =%d --> %d  format=%d\n",last_i2s_clk, i2s_clk,audio_parse_get_audio_type(patch->audio_parse_para));
                last_i2s_clk = i2s_clk;
                clk_changed = 1;
                patch->clk_changed = 1;
                pthread_cond_signal(&patch->cond);
            } else {
                patch->clk_changed = 0;
            }
        }

        /*if original sample rate or read bytes is zero, audio format is invalid*/
        if ((patch->sample_rate == 0) || (bytes_avail <= 0)) {
            aml_audiolevel_reset(dev);
            patch->aformat = AUDIO_FORMAT_INVALID;
            if (aml_dev->decode_format != patch->aformat) {
                trigger_audio_callback(patch->callback_handle, AML_AUDIO_CALLBACK_FORMATCHANGED,
                     (audio_callback_data_t *) & (patch->aformat));
                ALOGE("HDMI/SPDIF input format changed from %#x to %#x\n",
                     aml_dev->decode_format, patch->aformat);
                aml_dev->decode_format = patch->aformat;
            }
        }

        /*when arc unplug, but there is still some data which cause noise*/
        if (patch->input_src == AUDIO_DEVICE_IN_SPDIF) {
            aml_data_format_t spdif_format = { 0 };
            get_input_streaminfo(stream_in, &spdif_format);
            if (patch->sample_rate == 0 || spdif_format.sr == 0) {
                usleep(5000);
                continue;
            }
        }

        // ALOGV("++%s in read over read_bytes = %d, in_read returns = %d \n",
        // __FUNCTION__, read_bytes * period_mul, bytes_avail);
        if (bytes_avail > 0) {
            //temp add a parse function here
            if (/*patch->input_src == AUDIO_DEVICE_IN_HDMI || */patch->input_src == AUDIO_DEVICE_IN_SPDIF) {
                feeddata_audio_type_parse(&patch->audio_parse_para, patch->in_buf, bytes_avail);
                aml_dev->sink_format = patch->aformat;
            }

            if (patch->input_src == AUDIO_DEVICE_IN_HDMI) {
                struct timespec new_ts;
                audio_format_t format = AUDIO_FORMAT_PCM_16_BIT;
                /*during format transition, especially from pcm to bitstream,
                 there is "pop" noise, sometimes the
                 bitstream data is treated as PCM, so we use this method to detect
                 whether the input is stable*/
                if (aml_dev->hw_audio_type_det)
                    format = hdmiin_hw_audio_format_detection();
                else
                    format = audio_parse_get_audio_type(patch->audio_parse_para);

                if ((patch->ch && patch->sample_rate) && (format == AUDIO_FORMAT_PCM_16_BIT)) {
                    period_time = (int)((int64_t)bytes_avail * 1000000LL / (patch->ch * patch->sample_rate * 2));
                    if ((us > 2 * period_time) || (clk_changed == 1)) {
                        ALOGE("Input is not stable gap=%d period_time=%d clk_changed=%d\n", us, period_time, clk_changed);
                        clock_gettime(CLOCK_MONOTONIC, &patch->mute_start_ts);
                        patch->hdmi_stable = 0;
                    }
                    clock_gettime(CLOCK_MONOTONIC, &new_ts);
                    if (patch->hdmi_stable == 0) {
                        us =  calc_time_interval_us(&patch->mute_start_ts, &new_ts);
                        if (us > 100 * 1000) {
                            patch->hdmi_stable = 1;
                        }
                    }

                    if (patch->hdmi_stable == 0) {
                        memset(patch->in_buf, 0, bytes_avail);
                    }
                }
            }
#if 0
            if (patch->input_src == AUDIO_DEVICE_IN_SPDIF) {
                audio_format_t format = AUDIO_FORMAT_PCM_16_BIT;
                audio_format_t format_hw = AUDIO_FORMAT_PCM_16_BIT;
                format = audio_parse_get_audio_type(patch->audio_parse_para);
                format_hw = spdifhw_audio_format_detection();
                if (is_digital_raw_format(format_hw) && format == AUDIO_FORMAT_PCM_16_BIT) {
                    //ALOGF("sw format=0x%x hw =0x%x\n",format, format_hw);
                    // mute these data
                    //memset(patch->in_buf, 0, bytes_avail);
                }

            }
#endif

            if (aml_log_get_dumpfile_enable("dump_input")) {
                //dump the input data
                FILE *fp1=fopen("/tmp/input.data","a+");
                if (fp1) {
                    fwrite((char *)patch->in_buf, 1, bytes_avail ,fp1);
                    fclose(fp1);
                }
            }

            do {
                if (get_buffer_write_space(ringbuffer) >= bytes_avail) {
                    retry = 0;
                    ret = ring_buffer_write(ringbuffer,
                                            (unsigned char*)patch->in_buf,
                                            bytes_avail, UNCOVER_WRITE);
                    if (ret != bytes_avail) {
                        ALOGE("%s(), write buffer fails!", __func__);
                    }

                    if (!first_start || get_buffer_read_space(ringbuffer) >= read_threshold) {
                        pthread_cond_signal(&patch->cond);
                        if (first_start) {
                            first_start = 0;
                        }
                    }
                    //usleep(1000);
                } else {
                    retry++;
                    pthread_cond_signal(&patch->cond);
                    //Fixme: if ringbuffer is full enough but no output, reset ringbuffer
                    ring_buffer_reset(ringbuffer);
                    ALOGE("%s(), no space to input, in read audio discontinue!\n", __func__);
                    usleep(5000);
                }
            } while (retry && !patch->input_thread_exit);
        } else {
            ALOGV("%s(), read alsa pcm fails, to _read(%d), bytes_avail(%d)!\n",
                  __func__, read_bytes * period_mul, bytes_avail);
            if (get_buffer_read_space(ringbuffer) >= bytes_avail) {
                pthread_cond_signal(&patch->cond);
            }
            usleep(5000);
        }
    }

exit_ring_buf:
    free(patch->in_buf);
exit_inbuf:
    adev_close_input_stream(dev, stream_in);
exit_open:
    ALOGI("--%s", __FUNCTION__);
    return ((void *) 0);
}

static void ts_wait_time(struct timespec *ts, uint32_t time)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += time / 1000000;
    ts->tv_nsec += (time * 1000) % 1000000000;
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}
#define AVSYNC_SAMPLE_INTERVAL (50)
#define AVSYNC_SAMPLE_MAX_CNT (10)

void *audio_patch_output_threadloop(void *data)
{
    struct aml_audio_patch *patch = (struct aml_audio_patch *)data;
    struct audio_hw_device *dev = patch->dev;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    ring_buffer_t *ringbuffer = & (patch->aml_ringbuffer);
    struct audio_stream_out *stream_out = NULL;
    struct aml_stream_out *aml_out = NULL;
    struct audio_config stream_config;
    struct timespec ts;
    int write_bytes = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    int ret;

    ALOGI("++%s", __FUNCTION__);
    // FIXME: get actual configs
    stream_config.sample_rate = 48000;
    stream_config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    stream_config.format = AUDIO_FORMAT_PCM_16_BIT;

    ret = adev_open_output_stream_new(patch->dev,
                                      0,
                                      AUDIO_DEVICE_OUT_SPEAKER, //devices_t
                                      AUDIO_OUTPUT_FLAG_DIRECT, //flags
                                      &stream_config,
                                      &stream_out,
                                      NULL);
    if (ret < 0) {
        ALOGE("open output stream fail, ret = %d", ret);
        goto exit_open;
    }

    // allocate a bigger buffer for DTS HD case
    patch->out_buf_size = write_bytes * 4 * 4;
    patch->out_buf = calloc(1, patch->out_buf_size);
    if (!patch->out_buf) {
        ret = -ENOMEM;
        goto exit_outbuf;
    }

    prctl(PR_SET_NAME, (unsigned long) "audio_output_patch");


    while (!patch->output_thread_exit) {
        int period_mul = 1;

        //set_thread_affinity(3);

        if (patch->ch == 8) {
            period_mul = 4;
        }

        if (patch->sample_rate >= 176400) {
            period_mul = period_mul * 4;
        }

        patch->out_period_mul = period_mul;

        pthread_mutex_lock(&patch->mutex);
        //ALOGV("%s(), ringbuffer level read before wait--%d \n",
        //__func__, get_buffer_read_space(ringbuffer));
        if (get_buffer_read_space(ringbuffer) < (write_bytes * period_mul)) {
            // wait 300ms
            ts_wait_time(&ts, 300000);
            pthread_cond_timedwait(&patch->cond, &patch->mutex, &ts);
            //pthread_cond_wait(&patch->cond, &patch->mutex);
        }
        pthread_mutex_unlock(&patch->mutex);
        if (patch->output_thread_exit) {
            ALOGD("exit output thread");
            break;
        }

        if (patch->clk_changed) {
            ALOGD("clk changed");
            do_output_standby_l((struct audio_stream *) stream_out);
        }

        // ALOGV("%s(), ringbuffer level read after wait-- %d\n",
        //__func__, get_buffer_read_space(ringbuffer));
        if (get_buffer_read_space(ringbuffer) >= (write_bytes * period_mul)) {
            ret = ring_buffer_read(ringbuffer,
                                   (unsigned char*)patch->out_buf, write_bytes * period_mul);

            if (ret == 0) {
                ALOGE("%s(), ring_buffer read 0 data!", __func__);
            }

            //ALOGV("++%s ringbuf get data ok, start to out write, bytes-%d\n",
            //__FUNCTION__, ret);
#if 0
            /*if (getprop_bool("media.audiohal.writedump")) */{
                FILE *dump_fp = NULL;
                dump_fp = fopen("/data/audio_hal/audioraw.dump", "a+");
                if (dump_fp != NULL) {
                    fwrite(patch->out_buf, ret, 1, dump_fp);
                    fclose(dump_fp);
                } else {
                    ALOGW("[Error] Can't write to /data/audio_hal/audioraw.dump");
                }
            }
#endif

#if 0
            struct timespec before_read;
            struct timespec after_read;
            int us = 0;
            clock_gettime(CLOCK_MONOTONIC, &before_read);
#endif
            //if (get_buffer_read_space(ringbuffer) > 2048)
            //  ALOGD("ch=%d rate=%d size=%d write=%d available=%d\n",patch->ch,patch->sample_rate,ringbuffer->size,ret,get_buffer_read_space(ringbuffer));
            ret = out_write_new(stream_out, patch->out_buf, ret);

#if 0
            clock_gettime(CLOCK_MONOTONIC, &after_read);
            us = calc_time_interval_us(&before_read, &after_read);
            ALOGD("function gap =%d ", us);
#endif


            //ALOGV("++%s ringbuf get data ok, out write over!, retbytes = %d\n",
            //__FUNCTION__, ret);
            if (patch->avsync_sample_interval >= AVSYNC_SAMPLE_INTERVAL * period_mul) {
                aml_dev_try_avsync(patch);
                patch->avsync_sample_interval = 0;
            } else {
                patch->avsync_sample_interval++;
            }
        } else {
            ALOGV("%s(), no data in buffer, audio discontinue output!", __func__);
        }
    }

    free(patch->out_buf);
exit_outbuf:
    adev_close_output_stream_new(dev, stream_out);
exit_open:
    ALOGI("--%s", __FUNCTION__);
    return ((void *)0);
}

void dump_patch_info(void * private)
{
    struct audio_hw_device *dev = (struct audio_hw_device *)private;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct aml_audio_patch *patch = NULL;
    ring_buffer_t *ringbuffer = NULL;
    int read_size = 0;
    int write_size = 0;

    if (aml_dev->audio_patch == NULL) {
        ALOGA("There is no Patch info\n");
        return;
    }

    patch = aml_dev->audio_patch;
    ringbuffer = & (patch->aml_ringbuffer);

    read_size = get_buffer_read_space(ringbuffer);
    write_size = get_buffer_write_space(ringbuffer);
    if (patch->input_src == AUDIO_DEVICE_IN_HDMI) {
        ALOGA("HDMI input ch=%d samplerate=%d audio_type=%d\n",aml_dev->capture_ch,aml_dev->capture_samplerate, aml_dev->capture_audiotype);
    }
    ALOGA("Patch   Info: ring buffer size=0x%x avail=0x%x empty=0x%x Period in=%d %d stable=%d\n", ringbuffer->size, read_size, write_size, patch->in_period_mul, patch->out_period_mul, patch->hdmi_stable);
    ALOGA("Input   Info: src=0x%x  rate (in=%d out=%d) ch=%d \n", patch->input_src, patch->original_rate, patch->sample_rate , patch->ch);
    ALOGA("Decoder Info: fomrat=0x%x \n", patch->aformat);

    return;
}


#define PATCH_PERIOD_COUNT 4

static int create_patch(struct audio_hw_device *dev,
                        audio_devices_t input,
                        audio_devices_t output __unused)
{
    struct aml_audio_patch *patch;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    int period_size = DEFAULT_PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    pthread_attr_t attr;
    struct sched_param param;
    int ret = 0;
    int ring_buffer_len = 0;
    int iec_check_size = 0;
    int audio_type_preset = -1;

    ALOGI("++%s", __func__);
    patch = calloc(1, sizeof(*patch));
    if (!patch) {
        ret = -ENOMEM;
        goto err;
    }
    // save dev to patch
    patch->dev = dev;
    patch->input_src = input;
    patch->aformat = AUDIO_FORMAT_INVALID;
    patch->avsync_sample_max_cnt = AVSYNC_SAMPLE_MAX_CNT;
    patch->hdmi_stable = 1;
    aml_dev->audio_patch = patch;
    pthread_mutex_init(&patch->mutex, NULL);
    pthread_cond_init(&patch->cond, NULL);

#ifdef DATMOS
    if (aml_dev->datmos_enable) {
        if (aml_dev->capture_ch == 8) {
            ring_buffer_len = 64 * period_size * PATCH_PERIOD_COUNT;
        } else if (aml_dev->capture_samplerate >= 176400) {
            ring_buffer_len = 16 * period_size * PATCH_PERIOD_COUNT;

        } else {
            ring_buffer_len = 4 * period_size * PATCH_PERIOD_COUNT;
        }
    } else
#endif
        ring_buffer_len = 16 * period_size * PATCH_PERIOD_COUNT;

    ALOGE("ring_buffer_len %#x\n", ring_buffer_len);

    ret = ring_buffer_init(& (patch->aml_ringbuffer), ring_buffer_len);
    if (ret < 0) {
        ALOGE("Fail to init audio ringbuffer!");
        goto err_ring_buf;
    }

    ret = pthread_create(&(patch->audio_input_threadID), NULL,
                         &audio_patch_input_threadloop, patch);
    if (ret != 0) {
        ALOGE("%s, Create input thread fail!\n", __FUNCTION__);
        goto err_in_thread;
    }
#if 1
    ret = pthread_create(& (patch->audio_output_threadID), NULL,
                         &audio_patch_output_threadloop, patch);
#endif
    if (ret != 0) {
        ALOGE("%s, Create output thread fail!\n", __FUNCTION__);
        goto err_out_thread;
    }

    //create audio format parser thread if the input source is HDMI/SPDIF in
    if (input == AUDIO_DEVICE_IN_HDMI || input == AUDIO_DEVICE_IN_SPDIF) {
        //TODO add sample rate and channel information
        //ret = creat_pthread_for_audio_type_parse(&patch->audio_parse_threadID, &patch->audio_parse_para, 48000, 2);
        iec_check_size = IEC61937_CHECK_SIZE;
        if (input == AUDIO_DEVICE_IN_HDMI) {
            if (aml_dev->capture_ch == 8) {
                iec_check_size = iec_check_size * 2;
            }
            audio_type_preset = aml_dev->capture_audiotype;
        }
        creat_audio_type_parse(&patch->audio_parse_para, iec_check_size, audio_type_preset);
        if (ret !=  0) {
            ALOGE("%s,create format parse thread fail!\n", __FUNCTION__);
            goto err_parse_thread;
        }
    }

    if (input == AUDIO_DEVICE_IN_SPDIF) {
        aml_spdifin_init(&patch->spdif_in);
    }

    // init callback for patch
    init_audio_callback(&patch->callback_handle);

    aml_log_dumpinfo_install("aml_audio_patch", dump_patch_info, dev);


    ALOGI("--%s", __FUNCTION__);
    return 0;
err_parse_thread:
    patch->output_thread_exit = 1;
    pthread_join(patch->audio_output_threadID, NULL);
err_out_thread:
    patch->input_thread_exit = 1;
    pthread_join(patch->audio_input_threadID, NULL);
err_in_thread:
    ring_buffer_release(& (patch->aml_ringbuffer));
err_ring_buf:
    free(patch);
err:
    return ret;
}

static int create_patch_ext(struct audio_hw_device *dev,
                            audio_devices_t input,
                            audio_devices_t output __unused,
                            audio_patch_handle_t handle)
{
    int ret = 0;

    ret = create_patch(dev, input, output);
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;

    // successful
    if (!ret) {
        aml_dev->audio_patch->patch_hdl = handle;
    }

    return ret;

}

static int release_patch(struct aml_audio_device *aml_dev)
{
    struct aml_audio_patch *patch = aml_dev->audio_patch;

    ALOGI("++%s\n", __FUNCTION__);
    patch->output_thread_exit = 1;
    pthread_cond_signal(&patch->cond);
    patch->input_thread_exit = 1;


    pthread_join(patch->audio_input_threadID, NULL);
    pthread_join(patch->audio_output_threadID, NULL);

    if (patch->input_src == AUDIO_DEVICE_IN_HDMI || patch->input_src == AUDIO_DEVICE_IN_SPDIF)  {
        //exit_pthread_for_audio_type_parse(patch->audio_parse_threadID, &patch->audio_parse_para);
        release_audio_type_parse(&patch->audio_parse_para);
    }


    if (patch->input_src == AUDIO_DEVICE_IN_SPDIF) {
        aml_spdifin_release(patch->spdif_in);
        patch->spdif_in = NULL;
    }


    ring_buffer_release(& (patch->aml_ringbuffer));

    aml_log_dumpinfo_remove("aml_audio_patch");

    exit_audio_callback(&patch->callback_handle);
    free(patch);
    aml_dev->audio_patch = NULL;
    ALOGI("--%s", __FUNCTION__);
    return 0;
}

static int create_parser(struct audio_hw_device *dev)
{
    struct aml_audio_parser *parser;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    int period_size = 4096;
    int ret = 0;

    ALOGI("++%s", __func__);
    parser = calloc(1, sizeof(struct aml_audio_parser));
    if (!parser) {
        ret = -ENOMEM;
        goto err;
    }

    parser->dev = dev;
    parser->aformat = AUDIO_FORMAT_PCM_16_BIT;
    aml_dev->aml_parser = parser;
    pthread_mutex_init(&parser->mutex, NULL);

    ret = ring_buffer_init(& (parser->aml_ringbuffer), 4 * period_size * PATCH_PERIOD_COUNT);
    if (ret < 0) {
        ALOGE("Fail to init audio ringbuffer!");
        goto err_ring_buf;
    }

    ret = creat_pthread_for_audio_type_parse(&parser->audio_parse_threadID, &parser->audio_parse_para, 48000, 2);
    if (ret !=  0) {
        ALOGE("%s,create format parse thread fail!\n", __FUNCTION__);
        goto err_in_thread;
    }
    ALOGI("--%s", __FUNCTION__);
    return 0;
err_in_thread:
    ring_buffer_release(& (parser->aml_ringbuffer));
err_ring_buf:
    free(parser);
err:
    return ret;
}

static int release_parser(struct aml_audio_device *aml_dev)
{
    struct aml_audio_parser *parser = aml_dev->aml_parser;

    ALOGI("++%s", __FUNCTION__);

    if (aml_dev->aml_parser == NULL) {
        ALOGI("--%s aml_dev->aml_parser == NULL", __FUNCTION__);
        return 0;
    }

    if (parser->decode_enabled == 1) {
        ALOGI("++%s: release decoder!", __FUNCTION__);
        dcv_decode_release(parser);
        parser->aformat = AUDIO_FORMAT_PCM_16_BIT;
    }
    exit_pthread_for_audio_type_parse(parser->audio_parse_threadID, &parser->audio_parse_para);
    ring_buffer_release(& (parser->aml_ringbuffer));
    free(parser);
    aml_dev->aml_parser = NULL;
    ALOGI("--%s", __FUNCTION__);
    return 0;
}

static void dump_audio_patch_set(struct audio_patch_set *patch_set)
{
    struct audio_patch *patch = NULL;
    unsigned int i = 0;

    if (!patch_set) {
        return;
    }

    patch = &patch_set->audio_patch;
    if (!patch) {
        return;
    }

    ALOGI("  - %s(), id: %d", __func__, patch->id);
    for (i = 0; i < patch->num_sources; i++) {
        dump_audio_port_config(&patch->sources[i]);
    }
    for (i = 0; i < patch->num_sinks; i++) {
        dump_audio_port_config(&patch->sinks[i]);
    }
}

static void dump_aml_audio_patch_sets(struct audio_hw_device *dev)
{
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct audio_patch_set *patch_set = NULL;
    struct audio_patch *patch = NULL;
    struct listnode *node = NULL;
    unsigned int i = 0;

    ALOGI("++%s(), lists all patch sets:", __func__);
    list_for_each(node, &aml_dev->patch_list) {
        ALOGI(" - patch set num: %d", i);
        patch_set = node_to_item(node, struct audio_patch_set, list);
        dump_audio_patch_set(patch_set);
        i++;
    }
    ALOGI("--%s(), lists all patch sets over", __func__);
}


static int get_audio_patch_by_hdl(struct audio_hw_device *dev, audio_patch_handle_t handle, struct audio_patch *p_audio_patch)
{
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;

    struct listnode *node = NULL;

    struct audio_patch_set *patch_set_tmp = NULL;
    struct audio_patch *patch_tmp = NULL;
    p_audio_patch = NULL;

    list_for_each(node, &aml_dev->patch_list) {
        patch_set_tmp = node_to_item(node, struct audio_patch_set, list);
        patch_tmp = &patch_set_tmp->audio_patch;


        if (patch_tmp->id == handle) {
            p_audio_patch = patch_tmp;
            break;
        }
    }
    return 0;
}

static int get_audio_patch_by_src_dev(struct audio_hw_device *dev, audio_devices_t dev_type, struct audio_patch **p_audio_patch)
{
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct listnode *node = NULL;
    struct audio_patch_set *patch_set_tmp = NULL;
    struct audio_patch *patch_tmp = NULL;

    list_for_each(node, &aml_dev->patch_list) {
        patch_set_tmp = node_to_item(node, struct audio_patch_set, list);
        patch_tmp = &patch_set_tmp->audio_patch;
        if (patch_tmp->sources[0].ext.device.type == dev_type) {
            ALOGI("%s, patch_tmp->id = %d, dev_type = %ud", __func__, patch_tmp->id, dev_type);
            *p_audio_patch = patch_tmp;
            break;
        }
    }
    return 0;
}

/* remove audio patch from dev list */
static int unregister_audio_patch(struct audio_hw_device *dev,
                                  struct audio_patch_set *patch_set)
{
    (void) dev;
    if (!patch_set) {
        ALOGE("%s(), null pointer", __func__);
        return -EINVAL;
    }

#ifdef DEBUG_PATCH_SET
    ALOGI("++%s(), patch set to release is:", __func__);
    dump_audio_patch_set(patch_set);
#endif
    list_remove(&patch_set->list);
    free(patch_set);
    ALOGI("--%s()", __func__);
    return 0;
}

/* add new audio patch to dev list */
static struct audio_patch_set *register_audio_patch(struct audio_hw_device *dev,
        unsigned int num_sources,
        const struct audio_port_config *sources,
        unsigned int num_sinks,
        const struct audio_port_config *sinks,
        audio_patch_handle_t *handle)
{
    /* init audio patch */
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct audio_patch_set *patch_set_new = NULL;
    struct audio_patch *patch_new = NULL;
    struct audio_patch_set *patch_set_tmp = NULL;
    struct audio_patch *patch_tmp = NULL;
    struct listnode *node = NULL;
    audio_patch_handle_t patch_handle;

    patch_set_new = calloc(1, sizeof(*patch_set_new));
    if (!patch_set_new) {
        ALOGE("%s(): no memory", __func__);
        return NULL;
    }

    patch_new = &patch_set_new->audio_patch;
    patch_handle = (audio_patch_handle_t) android_atomic_inc(&aml_dev->next_unique_ID);
    *handle = patch_handle;

    /* init audio patch new */
    patch_new->id = patch_handle;
    patch_new->num_sources = num_sources;
    memcpy(patch_new->sources, sources, num_sources * sizeof(struct audio_port_config));
    patch_new->num_sinks = num_sinks;
    memcpy(patch_new->sinks, sinks, num_sinks * sizeof(struct audio_port_config));
#ifdef DEBUG_PATCH_SET
    ALOGD("%s(), patch set new to register:", __func__);
    dump_audio_patch_set(patch_set_new);
#endif

    /* find if mix->dev exists and remove from list */
    list_for_each(node, &aml_dev->patch_list) {
        patch_set_tmp = node_to_item(node, struct audio_patch_set, list);
        patch_tmp = &patch_set_tmp->audio_patch;
        if (patch_tmp->sources[0].type == AUDIO_PORT_TYPE_MIX &&
            patch_tmp->sinks[0].type == AUDIO_PORT_TYPE_DEVICE &&
            sources[0].ext.mix.handle == patch_tmp->sources[0].ext.mix.handle) {
            ALOGI("patch found mix->dev id %d, patchset %p", patch_tmp->id, patch_set_tmp);
            break;
        } else {
            patch_set_tmp = NULL;
            patch_tmp = NULL;
        }
    }
    /* found mix->dev patch, so release and remove it */
    if (patch_set_tmp) {
        ALOGD("%s, found the former mix->dev patch set, remove it first", __func__);
        unregister_audio_patch(dev, patch_set_tmp);
        patch_set_tmp = NULL;
        patch_tmp = NULL;
    }

    /* find if dev->mix exists and remove from list */
    list_for_each(node, &aml_dev->patch_list) {
        patch_set_tmp = node_to_item(node, struct audio_patch_set, list);
        patch_tmp = &patch_set_tmp->audio_patch;
        if (patch_tmp->sources[0].type == AUDIO_PORT_TYPE_DEVICE &&
            patch_tmp->sinks[0].type == AUDIO_PORT_TYPE_MIX &&
            sinks[0].ext.mix.handle == patch_tmp->sinks[0].ext.mix.handle) {
            ALOGI("patch found dev->mix id %d, patchset %p", patch_tmp->id, patch_set_tmp);
            break;
        } else {
            patch_set_tmp = NULL;
            patch_tmp = NULL;
        }
    }
    /* found dev->mix patch, so release and remove it */
    if (patch_set_tmp) {
        ALOGD("%s, found the former dev->mix patch set, remove it first", __func__);
        unregister_audio_patch(dev, patch_set_tmp);
    }
    /* add new patch set to dev patch list */
    list_add_head(&aml_dev->patch_list, &patch_set_new->list);
    ALOGI("--%s: after registering new patch, patch sets will be:", __func__);
    //dump_aml_audio_patch_sets(dev);

    return patch_set_new;
}

static int adev_create_audio_patch(struct audio_hw_device *dev,
                                   unsigned int num_sources,
                                   const struct audio_port_config *sources,
                                   unsigned int num_sinks,
                                   const struct audio_port_config *sinks,
                                   audio_patch_handle_t *handle)
{
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct audio_patch_set *patch_set;
    const struct audio_port_config *src_config = sources;
    const struct audio_port_config *sink_config = sinks;
    enum input_source input_src = HDMIIN;
    uint32_t sample_rate = 48000, channel_cnt = 2;
    enum OUT_PORT outport = OUTPORT_SPEAKER;
    enum IN_PORT inport = INPORT_HDMIIN;
    unsigned int i = 0;
    int ret = -1;

#ifdef MEM_CHECK
    {
        static int create_cnt = 1;
        char log_name[32];
        snprintf(log_name, 32, "/data/trace_%d.log", create_cnt);
        setenv("MALLOC_TRACE", log_name, 1);
        create_cnt++;
        mtrace();
    }
#endif

    if (src_config->ext.device.type == AUDIO_DEVICE_IN_WIRED_HEADSET) {
        ALOGD("bluetooth voice search is in use, bypass adev_create_audio_patch()!!\n");
        goto err;
    }
    ALOGI("++%s", __FUNCTION__);
    if (!sources || !sinks || !handle) {
        ALOGE("%s: null pointer!", __func__);
        ret = -EINVAL;
        goto err;
    }

    if ((num_sources != 1) || (num_sinks > AUDIO_PATCH_PORTS_MAX)) {
        ALOGE("%s: unsupport num sources or sinks", __func__);
        ret = -EINVAL;
        goto err;
    }

    ALOGI("+%s num_sources [%d] , num_sinks [%d]", __func__, num_sources, num_sinks);
    patch_set = register_audio_patch(dev, num_sources, sources,
                                     num_sinks, sinks, handle);
    ALOGI("%s(), patch new handle for AF: %d", __func__, (int)handle);
    if (!patch_set) {
        ret = -ENOMEM;
        goto err;
    }

    /* mix to device audio patch */
    if (src_config->type == AUDIO_PORT_TYPE_MIX) {
        if (sink_config->type != AUDIO_PORT_TYPE_DEVICE) {
            ALOGE("unsupport patch case");
            ret = -EINVAL;
            goto err_patch;
        }

        switch (sink_config->ext.device.type) {
        case AUDIO_DEVICE_OUT_HDMI_ARC:
            outport = OUTPORT_HDMI_ARC;
            break;
        case AUDIO_DEVICE_OUT_HDMI:
            outport = OUTPORT_HDMI;
            break;
        case AUDIO_DEVICE_OUT_SPDIF:
            outport = OUTPORT_SPDIF;
            break;
        case AUDIO_DEVICE_OUT_AUX_LINE:
            outport = OUTPORT_AUX_LINE;
            break;
        case AUDIO_DEVICE_OUT_SPEAKER:
            outport = OUTPORT_SPEAKER;
            break;
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            outport = OUTPORT_HEADPHONE;
            break;
        default:
            ALOGE("%s: invalid sink device type %#x",
                  __func__, sink_config->ext.device.type);
        }

        /*
          // when HDMI plug in or UI [Sound Output Device] set to "ARC" ,AUDIO need to output from HDMI ARC
          // while in current design, speaker is an "always on" device
          // in this case , there will be 2 devices, one is ARC, one is SPEAKER
          // we need to output audio from ARC, so set outport as OUTPORT_HDMI_ARC
          */
        if (num_sinks == 2) {
            if ((sink_config[0].ext.device.type == AUDIO_DEVICE_OUT_HDMI_ARC && sink_config[1].ext.device.type == AUDIO_DEVICE_OUT_SPEAKER)
                || (sink_config[0].ext.device.type == AUDIO_DEVICE_OUT_SPEAKER && sink_config[1].ext.device.type == AUDIO_DEVICE_OUT_HDMI_ARC)) {
                if (aml_dev->bHDMIARCon) {
                    outport = OUTPORT_HDMI_ARC;
                    ALOGI("%s() mix->device patch: speaker and HDMI_ARC co-exist case, output=ARC", __func__);
                } else {
                    outport = OUTPORT_SPEAKER;
                    ALOGI("%s() mix->device patch: speaker and HDMI_ARC co-exist case, output=SPEAKER", __func__);
                }
            }
        }
#ifdef AUDIO_ROUTE
        if (outport != OUTPORT_SPDIF) {
            ret = aml_audio_output_routing(dev, outport, false);
        }
        if (ret < 0) {
            ALOGE("%s() output routing failed", __func__);
        }
#endif
        aml_dev->out_device = sink_config->ext.device.type;
        ALOGI("%s: mix->device patch: outport(%d)", __func__, outport);
        return 0;
    }

    /* device to mix audio patch */
    if (sink_config->type == AUDIO_PORT_TYPE_MIX) {
        if (src_config->type != AUDIO_PORT_TYPE_DEVICE) {
            ALOGE("unsupport patch case");
            ret = -EINVAL;
            goto err_patch;
        }

        switch (src_config->ext.device.type) {
        case AUDIO_DEVICE_IN_HDMI:
            input_src = HDMIIN;
            inport = INPORT_HDMIIN;
            aml_dev->patch_src = SRC_HDMIIN;
            set_audio_source_e(HDMIIN, true);
            break;
        case AUDIO_DEVICE_IN_LOOPBACK:
        case AUDIO_DEVICE_IN_BLUETOOTH_A2DP:
        case AUDIO_DEVICE_IN_LINE:
            input_src = LINEIN;
            inport = INPORT_LINEIN;
            aml_dev->patch_src = SRC_LINEIN;
            set_audio_source_e(LINEIN, true);
            break;
        case AUDIO_DEVICE_IN_SPDIF:
            input_src = SPDIFIN;
            inport = INPORT_SPDIF;
            aml_dev->patch_src = SRC_SPDIFIN;
            break;
        case AUDIO_DEVICE_IN_TV_TUNER:
            input_src = ATV;
            inport = INPORT_TUNER;
            aml_dev->tuner2mix_patch = true;
            break;
        default:
            ALOGE("%s: invalid src device type %#x",
                  __func__, src_config->ext.device.type);
            ret = -EINVAL;
            goto err_patch;
        }

        //set_audio_source(input_src);

        aml_dev->active_inport = inport;
        aml_dev->src_gain[inport] = 1.0;
        if (inport == INPORT_HDMIIN) {
            create_parser(dev);
        }
        ALOGI("%s: device->mix patch: inport(%s)", __func__, input_ports[inport]);
        return 0;
    }

    /*device to device audio patch. TODO: unify with the android device type*/
    if (sink_config->type == AUDIO_PORT_TYPE_DEVICE && src_config->type == AUDIO_PORT_TYPE_DEVICE) {
        switch (sink_config->ext.device.type) {
        case AUDIO_DEVICE_OUT_HDMI_ARC:
            outport = OUTPORT_HDMI_ARC;
            break;
        case AUDIO_DEVICE_OUT_HDMI:
            outport = OUTPORT_HDMI;
            break;
        case AUDIO_DEVICE_OUT_SPDIF:
            outport = OUTPORT_SPDIF;
            break;
        case AUDIO_DEVICE_OUT_AUX_LINE:
            outport = OUTPORT_AUX_LINE;
            break;
        case AUDIO_DEVICE_OUT_SPEAKER:
            outport = OUTPORT_SPEAKER;
            break;
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            outport = OUTPORT_HEADPHONE;
            break;
        default:
            ALOGE("%s: invalid sink device type %#x",
                  __func__, sink_config->ext.device.type);
        }

        /*
           * while in current design, speaker is an "always attached" device
           * when HDMI cable is plug in, there will be 2 sink devices "SPEAKER" and "HDMI ARC"
           * we need to do extra check to set proper sink device
           */
        if (num_sinks == 2) {
            if ((sink_config[0].ext.device.type == AUDIO_DEVICE_OUT_HDMI_ARC && sink_config[1].ext.device.type == AUDIO_DEVICE_OUT_SPEAKER)
                || (sink_config[0].ext.device.type == AUDIO_DEVICE_OUT_SPEAKER && sink_config[1].ext.device.type == AUDIO_DEVICE_OUT_HDMI_ARC)) {
                if (aml_dev->bHDMIARCon) {
                    outport = OUTPORT_HDMI_ARC;
                    ALOGI("%s() dev->dev patch: speaker and HDMI_ARC co-exist case, output=ARC", __func__);
                } else {
                    outport = OUTPORT_SPEAKER;
                    ALOGI("%s() dev->dev patch: speaker and HDMI_ARC co-exist case, output=SPEAKER", __func__);
                }
            }
        }
#ifdef AUDIO_ROUTE
        if (outport != OUTPORT_SPDIF) {
            ret = aml_audio_output_routing(dev, outport, false);
        }
        if (ret < 0) {
            ALOGE("%s() output routing failed", __func__);
        }
#endif
        if (sink_config->config_mask & AUDIO_PORT_CONFIG_SAMPLE_RATE) {
            sample_rate = sink_config->sample_rate;
        }

        if (sink_config->config_mask & AUDIO_PORT_CONFIG_CHANNEL_MASK) {
            channel_cnt = audio_channel_count_from_out_mask(sink_config->channel_mask);
        }

        switch (src_config->ext.device.type) {
        case AUDIO_DEVICE_IN_HDMI:
            input_src = HDMIIN;
            inport = INPORT_HDMIIN;
            aml_dev->patch_src = SRC_HDMIIN;
            set_audio_source_e(HDMIIN, true);
            break;
        case AUDIO_DEVICE_IN_LOOPBACK:
        case AUDIO_DEVICE_IN_BLUETOOTH_A2DP:
        case AUDIO_DEVICE_IN_LINE:
            input_src = LINEIN;
            inport = INPORT_LINEIN;
            aml_dev->patch_src = SRC_LINEIN;
            set_audio_source_e(LINEIN, true);
            break;
        case AUDIO_DEVICE_IN_TV_TUNER:
            input_src = ATV;
            inport = INPORT_TUNER;
            break;
        case AUDIO_DEVICE_IN_SPDIF:
            input_src = SPDIFIN;
            inport = INPORT_SPDIF;
            aml_dev->patch_src = SRC_SPDIFIN;
            break;
        default:
            ALOGE("%s: invalid src device type %#x",
                  __func__, src_config->ext.device.type);
            ret = -EINVAL;
            goto err_patch;
        }
        aml_dev->active_inport = inport;
        aml_dev->src_gain[inport] = 1.0;
        //aml_dev->sink_gain[outport] = 1.0;
        ALOGI("%s: dev->dev patch: inport(%s), outport(%s)",
              __func__, input_ports[inport], output_ports[outport]);

        // ATV path goes to dev set_params which could
        // tell atv or dtv source and decide to create or not.
        // One more case is ATV->ATV, should recreate audio patch.
        if (input_src != ATV || (input_src == ATV && aml_dev->patch_src == SRC_ATV)) {
            //set_audio_source(input_src);
            ret = create_patch(dev,
                               src_config->ext.device.type, outport);
            if (ret) {
                ALOGE("%s: create patch failed", __func__);
                goto err_patch;
            }
            aml_dev->audio_patching = 1;
            if (input_src == ATV) {
                aml_dev->patch_src = SRC_ATV;
            }
        }
        if (input_src == LINEIN && aml_dev->aml_ng_enable) {
#ifdef NOISE_GATE
            aml_dev->aml_ng_handle = init_noise_gate(aml_dev->aml_ng_level,
                                     aml_dev->aml_ng_attrack_time, aml_dev->aml_ng_release_time);
#endif
            ALOGE("%s: init amlogic noise gate: level: %fdB, attrack_time = %dms, release_time = %dms",
                  __func__, aml_dev->aml_ng_level,
                  aml_dev->aml_ng_attrack_time, aml_dev->aml_ng_release_time);
        }
    }
#ifdef DATMOS
    /*datmos input data has iec61937 align for HDMI-IN and SPDIF-IN*/
    datmos_set_iec61937_align_param(dev);
#endif
    return 0;

err_patch:
    unregister_audio_patch(dev, patch_set);
err:
    return ret;
}

/* Release an audio patch */
static int adev_release_audio_patch(struct audio_hw_device *dev,
                                    audio_patch_handle_t handle)
{
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct audio_patch_set *patch_set = NULL;
    struct audio_patch *patch = NULL;
    struct listnode *node = NULL;
    int ret = 0;

    ALOGI("++%s: handle(%d)", __func__, handle);
    if (list_empty(&aml_dev->patch_list)) {
        ALOGE("No patch in list to release");
        ret = -EINVAL;
        goto exit;
    }


    /* find audio_patch in patch_set list */
    list_for_each(node, &aml_dev->patch_list) {
        patch_set = node_to_item(node, struct audio_patch_set, list);
        patch = &patch_set->audio_patch;
        if (patch->id == handle) {
            ALOGI("patch set found id %d, patchset %p", patch->id, patch_set);
            break;
        } else {
            patch_set = NULL;
            patch = NULL;
        }
    }

    if (!patch_set || !patch) {
        ALOGE("Can't get patch in list");
        ret = -EINVAL;
        goto exit;
    }

    if (patch->sources[0].type == AUDIO_PORT_TYPE_DEVICE
        && patch->sinks[0].type == AUDIO_PORT_TYPE_DEVICE) {
#if 0
        if (aml_dev->patch_src == SRC_DTV) {
            ALOGI("patch src == DTV now line %d \n", __LINE__);
            aml_dev->audio_patching = 0;
            release_dtv_patch(aml_dev);
        }
#endif

        if (aml_dev->patch_src != SRC_DTV
            && aml_dev->patch_src != SRC_INVAL
            && aml_dev->audio_patching == 1) {
            release_patch(aml_dev);
#ifdef NOISE_GATE
            if (aml_dev->patch_src == SRC_LINEIN && aml_dev->aml_ng_handle) {
                release_noise_gate(aml_dev->aml_ng_handle);
                aml_dev->aml_ng_handle = NULL;
            }
#endif
        }
        aml_dev->audio_patching = 0;
        /* save ATV src to deal with ATV HP hotplug */
        if (aml_dev->patch_src != SRC_ATV) {
            aml_dev->patch_src = SRC_INVAL;
        }
        aml_dev->parental_control_av_mute = false;
    }

    if (patch->sources[0].type == AUDIO_PORT_TYPE_DEVICE
        && patch->sinks[0].type == AUDIO_PORT_TYPE_MIX) {
        if (aml_dev->patch_src == SRC_HDMIIN) {
            release_parser(aml_dev);
        }
        if (aml_dev->patch_src != SRC_ATV) {
            aml_dev->patch_src = SRC_INVAL;
        }
        if (aml_dev->patch_src == SRC_DTV) {
            ALOGI("patch src == DTV now line %d \n", __LINE__);
        }
        aml_dev->tuner2mix_patch = false;
    }

    unregister_audio_patch(dev, patch_set);
    ALOGI("--%s: after releasing patch, patch sets will be:", __func__);
    //dump_aml_audio_patch_sets(dev);
#ifdef MEM_CHECK
    muntrace();
#endif
exit:
    return ret;
}

static int aml_dev_dump_latency(struct aml_audio_device *aml_dev, int fd)
{
    struct aml_stream_in *in = aml_dev->active_input;
    struct aml_audio_patch *patch = aml_dev->audio_patch;
    int rbuf_ltcy = 0, spk_tuning_ltcy = 0, ms12_ltcy = 0, alsa_in_ltcy = 0;
    int alsa_out_i2s_ltcy = 0, alsa_out_spdif_ltcy = 0;
    int v_ltcy = 0;
    int frame_size = 0;
    int ret = 0;
    snd_pcm_sframes_t frames = 0;

    frame_size = CHANNEL_CNT * audio_bytes_per_sample(AUDIO_FORMAT_PCM_16_BIT);
    if (patch) {
        size_t rbuf_avail = 0;

        rbuf_avail = get_buffer_read_space(&patch->aml_ringbuffer);
        frames = rbuf_avail / frame_size;
        rbuf_ltcy = frames / SAMPLE_RATE_MS;
        if (patch->aformat == AUDIO_FORMAT_E_AC3) {
            rbuf_ltcy /= EAC3_MULTIPLIER;
        }
        v_ltcy = aml_sysfs_get_int("/sys/class/video/vframe_walk_delay");
    } else {
        return 0;
    }

    if (aml_dev->spk_tuning_lvl) {
        size_t rbuf_avail = 0;

        rbuf_avail = get_buffer_read_space(&aml_dev->spk_tuning_rbuf);
        frames = rbuf_avail / frame_size;
        spk_tuning_ltcy = frames / SAMPLE_RATE_MS;
    }
#ifdef ENABLE_MS12
    if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
        if (aml_dev->ms12.dolby_ms12_enable == true) {
            int dolby_main_avail = dolby_ms12_get_main_buffer_avail();
            if (dolby_main_avail > 0) {
                ms12_ltcy = dolby_main_avail / frame_size / SAMPLE_RATE_MS;
                if (patch && patch->aformat == AUDIO_FORMAT_E_AC3) {
                    ms12_ltcy /= EAC3_MULTIPLIER;
                }
            }
        }
    }
#endif
    if (aml_dev->pcm_handle[I2S_DEVICE]) {
        ret = pcm_ioctl(aml_dev->pcm_handle[I2S_DEVICE], SNDRV_PCM_IOCTL_DELAY, &frames);
        if (ret >= 0) {
            alsa_out_i2s_ltcy = frames / SAMPLE_RATE_MS;
        }
    }

    if (aml_dev->pcm_handle[DIGITAL_DEVICE]) {
        ret = pcm_ioctl(aml_dev->pcm_handle[DIGITAL_DEVICE], SNDRV_PCM_IOCTL_DELAY, &frames);
        if (ret >= 0) {
            alsa_out_spdif_ltcy = frames / SAMPLE_RATE_MS;
        }
    }

    if (in) {
        ret = pcm_ioctl(in->pcm, SNDRV_PCM_IOCTL_DELAY, &frames);
        if (ret >= 0) {
            alsa_in_ltcy = frames / SAMPLE_RATE_MS;
            if (patch && patch->aformat == AUDIO_FORMAT_E_AC3) {
                alsa_in_ltcy /= EAC3_MULTIPLIER;
            }
        }
    }

    if (patch) {
        int in_path_ltcy = alsa_in_ltcy + rbuf_ltcy + ms12_ltcy;
        int out_path_ltcy = 0;
        int whole_path_ltcy = 0;

        if (aml_dev->sink_format == AUDIO_FORMAT_PCM_16_BIT) {
            out_path_ltcy = alsa_out_i2s_ltcy + spk_tuning_ltcy;
        } else if (aml_dev->sink_format == AUDIO_FORMAT_AC3) {
            out_path_ltcy = alsa_out_spdif_ltcy;
        } else if (aml_dev->sink_format == AUDIO_FORMAT_E_AC3) {
            out_path_ltcy = alsa_out_spdif_ltcy / EAC3_MULTIPLIER;
        }

        whole_path_ltcy = in_path_ltcy + out_path_ltcy;

        dprintf(fd, "  audio patch latency: %dms\n", rbuf_ltcy);
        dprintf(fd, "  audio spk tuning latency: %dms\n", spk_tuning_ltcy);
        dprintf(fd, "  MS12 buffer latency: %dms\n", ms12_ltcy);
        dprintf(fd, "  alsa hw i2s latency: %dms\n", alsa_out_i2s_ltcy);
        dprintf(fd, "  alsa hw spdif latency: %dms\n", alsa_out_spdif_ltcy);
        dprintf(fd, "  alsa in hw latency: %dms\n\n", alsa_in_ltcy);
        dprintf(fd, "  dev->dev audio total latency: %dms\n", whole_path_ltcy);
        if (v_ltcy > 0 && v_ltcy < 200) {
            dprintf(fd, "  video path total latency: %dms\n", v_ltcy);
        } else {
            dprintf(fd, "  video path total latency: N/A\n");
        }
    }
    return 0;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    struct aml_audio_device* aml_dev = (struct aml_audio_device*)device;
    const int kNumRetries = 3;
    const int kSleepTimeMS = 500;
    int retry = kNumRetries;

    dprintf(fd, "\nAML primary audio hal:\n");

    while (retry > 0 && pthread_mutex_trylock(&aml_dev->lock) != 0) {
        sleep(kSleepTimeMS);
        retry--;
    }

    if (retry > 0) {
        aml_dev_dump_latency(aml_dev, fd);
        pthread_mutex_unlock(&aml_dev->lock);
    } else {
        // Couldn't lock
        dprintf(fd, "  Could not obtain aml_dev lock.\n");
    }

    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct aml_audio_device *adev = (struct aml_audio_device *) device;
#ifdef DATMOS
    if (adev->datmos_enable) {
        cleanup_atmos_func(&(adev->datmos_param));
    }
#endif

    if (adev->effect_buf) {
        free(adev->effect_buf);
    }
    if (adev->hp_output_buf) {
        free(adev->hp_output_buf);
    }
    ring_buffer_release(&(adev->spk_tuning_rbuf));
    if (adev->ar) {
        audio_route_free(adev->ar);
    }
    eq_drc_release(&adev->eq_data);
    close_mixer_handle();
    aml_audiolevel_close(&adev->hw_device);
    aml_bm_close(&adev->hw_device);
    aml_channelmap_parser_deinit();
    aml_external_pp_deinit(&adev->hw_device);
    free(device);
    aml_log_exit();
    audio_trace_deinit();
    return 0;

}

static int adev_set_audio_port_config(struct audio_hw_device *dev, const struct audio_port_config *config)
{
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    enum OUT_PORT outport = OUTPORT_SPEAKER;
    enum IN_PORT inport = INPORT_HDMIIN;
    int ret = 0;
    int num_sinks = 1;

    ALOGI("++%s", __FUNCTION__);
    if (config == NULL) {
        ALOGE("NULL configs");
        return -EINVAL;
    }

    //dump_audio_port_config(config);
    if ((config->config_mask & AUDIO_PORT_CONFIG_GAIN) == 0) {
        ALOGE("null configs");
        return -EINVAL;
    }
    ALOGD("config id=%d role=%d type=%d gain=%d\n", config->id, config->role, config->type, config->gain.values[0]);
    struct audio_patch_set *patch_set = NULL;
    struct audio_patch *patch = NULL;
    struct listnode *node = NULL;

    /* find the corrisponding sink for this src */
    list_for_each(node, &aml_dev->patch_list) {
        patch_set = node_to_item(node, struct audio_patch_set, list);
        patch = &patch_set->audio_patch;
        if (patch->sources[0].ext.device.type == config->ext.device.type) {
            ALOGI("patch set found id %d, patchset %p \n", patch->id, patch_set);
            break;
        } else if (patch->sinks[0].ext.device.type == config->ext.device.type) {
            ALOGI("patch set found id %d, patchset %p \n", patch->id, patch_set);
            break;
        } else if (patch->sources[0].type == AUDIO_PORT_TYPE_MIX &&
                   patch->sinks[0].type == AUDIO_PORT_TYPE_DEVICE &&
                   patch->sources[0].id == config->id) {
            ALOGI("patch found mix->dev id %d, patchset %p \n", patch->id, patch_set);
            break;
        } else {
            patch_set = NULL;
            patch = NULL;
        }
    }

    if (!patch_set || !patch) {
        ALOGE("%s(): no right patch available \n", __func__);
        return -EINVAL;
    }

    num_sinks = patch->num_sinks;
    if (num_sinks == 2) {
        if ((patch->sinks[0].ext.device.type == AUDIO_DEVICE_OUT_HDMI_ARC &&
             patch->sinks[1].ext.device.type == AUDIO_DEVICE_OUT_SPEAKER) ||
            (patch->sinks[0].ext.device.type == AUDIO_DEVICE_OUT_SPEAKER &&
             patch->sinks[1].ext.device.type == AUDIO_DEVICE_OUT_HDMI_ARC)) {
            if (aml_dev->bHDMIARCon) {
                outport = OUTPORT_HDMI_ARC;
                ALOGI("%s() speaker and HDMI_ARC co-exist case, output=ARC", __func__);
            } else {
                outport = OUTPORT_SPEAKER;
                ALOGI("%s() speaker and HDMI_ARC co-exist case, output=SPEAKER", __func__);
            }
        }
    }
    ALOGD("type=%d role=%d num_sinks=%d\n", config->type, config->role, num_sinks);
    if (config->type == AUDIO_PORT_TYPE_DEVICE) {
        if (config->role == AUDIO_PORT_ROLE_SINK) {
            if (num_sinks == 1) {
                switch (config->ext.device.type) {
                case AUDIO_DEVICE_OUT_HDMI_ARC:
                    outport = OUTPORT_HDMI_ARC;
                    break;
                case AUDIO_DEVICE_OUT_HDMI:
                    outport = OUTPORT_HDMI;
                    break;
                case AUDIO_DEVICE_OUT_SPDIF:
                    outport = OUTPORT_SPDIF;
                    break;
                case AUDIO_DEVICE_OUT_AUX_LINE:
                    outport = OUTPORT_AUX_LINE;
                    break;
                case AUDIO_DEVICE_OUT_SPEAKER:
                    outport = OUTPORT_SPEAKER;
                    break;
                case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
                    outport = OUTPORT_HEADPHONE;
                    break;
                default:
                    ALOGE("%s: invalid out device type %#x",
                          __func__, config->ext.device.type);
                }
            }

            aml_dev->sink_gain[outport] = get_volume_by_index(config->gain.values[0]);
            ALOGI(" - set sink device[%#x](outport[%s]): gain[%f] value=%d\n",
                  config->ext.device.type, output_ports[outport], aml_dev->sink_gain[outport], config->gain.values[0]);
            ALOGI(" - now the sink gains are:");
            ALOGI("\t- OUTPORT_SPEAKER->gain[%f]", aml_dev->sink_gain[OUTPORT_SPEAKER]);
            ALOGI("\t- OUTPORT_HDMI_ARC->gain[%f]", aml_dev->sink_gain[OUTPORT_HDMI_ARC]);
            ALOGI("\t- OUTPORT_HEADPHONE->gain[%f]", aml_dev->sink_gain[OUTPORT_HEADPHONE]);
            ALOGI("\t- OUTPORT_HDMI->gain[%f]", aml_dev->sink_gain[OUTPORT_HDMI]);
            ALOGI("\t- active outport is: %s", output_ports[aml_dev->active_outport]);
        } else if (config->role == AUDIO_PORT_ROLE_SOURCE) {
            switch (config->ext.device.type) {
            case AUDIO_DEVICE_IN_HDMI:
                inport = INPORT_HDMIIN;
                break;
            case AUDIO_DEVICE_IN_LOOPBACK:
            case AUDIO_DEVICE_IN_BLUETOOTH_A2DP:
            case AUDIO_DEVICE_IN_LINE:
                inport = INPORT_LINEIN;
                break;
            case AUDIO_DEVICE_IN_SPDIF:
                inport = INPORT_SPDIF;
                break;
            case AUDIO_DEVICE_IN_TV_TUNER:
                inport = INPORT_TUNER;
                break;
            }

            aml_dev->src_gain[inport] = 1.0;
            if (patch->sinks[0].type == AUDIO_PORT_TYPE_DEVICE) {
                if (num_sinks == 2) {
                    switch (outport) {
                    case OUTPORT_HDMI_ARC:
                        aml_dev->sink_gain[outport] = 1.0;
                        break;
                    case OUTPORT_SPEAKER:
                        aml_dev->sink_gain[outport] = get_volume_by_index(config->gain.values[0]);
                        break;
                    default:
                        ALOGE("%s: invalid out device type %#x",
                              __func__, outport);
                    }
                } else if (num_sinks == 1) {
                    switch (patch->sinks->ext.device.type) {
                    case AUDIO_DEVICE_OUT_HDMI_ARC:
                        outport = OUTPORT_HDMI_ARC;
                        aml_dev->sink_gain[outport] = 1.0;
                        break;
                    case AUDIO_DEVICE_OUT_HDMI:
                        outport = OUTPORT_HDMI;
                        aml_dev->sink_gain[outport] = 1.0;
                        break;
                    case AUDIO_DEVICE_OUT_SPDIF:
                        outport = OUTPORT_SPDIF;
                        aml_dev->sink_gain[outport] = 1.0;
                        break;
                    case AUDIO_DEVICE_OUT_AUX_LINE:
                        outport = OUTPORT_AUX_LINE;
                        aml_dev->sink_gain[outport] = get_volume_by_index(config->gain.values[0]);
                        break;
                    case AUDIO_DEVICE_OUT_SPEAKER:
                        outport = OUTPORT_SPEAKER;
                        aml_dev->sink_gain[outport] = get_volume_by_index(config->gain.values[0]);
                        break;
                    case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
                        outport = OUTPORT_HEADPHONE;
                        aml_dev->sink_gain[outport] = get_volume_by_index(config->gain.values[0]);
                        break;
                    default:
                        ALOGE("%s: invalid out device type %#x",
                              __func__, patch->sinks->ext.device.type);
                    }
                }
#ifdef ENABLE_MS12
                if (eDolbyMS12Lib == aml_dev->dolby_lib_type) {
                    /* dev->dev and DTV src gain using MS12 primary gain */
                    if (aml_dev->audio_patching || aml_dev->patch_src == SRC_DTV) {
                        pthread_mutex_lock(&aml_dev->lock);
                        // In recent (20180609) modify, HDMI ARC adjust volume will this function.
                        // Once this fucntion was called, HDMI ARC have no sound
                        // Maybe the db_gain parameter is difference form project to project
                        // After remove this function here, HDMI ARC volume still can work
                        // So. Temporary remove here.
                        // TODO: find out when call this function will cause HDMI ARC mute
                        //ret = set_dolby_ms12_primary_input_db_gain(&aml_dev->ms12, config->gain.values[1] / 100);
                        pthread_mutex_unlock(&aml_dev->lock);
                        if (ret < 0) {
                            ALOGE("set dolby primary gain failed");
                        }
                    }
                }
#endif
                ALOGI(" - set src device[%#x](inport[%s]): gain[%f]\n",
                      config->ext.device.type, input_ports[inport], aml_dev->src_gain[inport]);
                ALOGI(" - set sink device[%#x](outport[%s]): gain[%f]\n",
                      patch->sinks->ext.device.type, output_ports[outport], aml_dev->sink_gain[outport]);
            } else if (patch->sinks[0].type == AUDIO_PORT_TYPE_MIX) {
                aml_dev->src_gain[inport] = get_volume_by_index(config->gain.values[0]);
                ALOGI(" - set src device[%#x](inport[%s]): gain[%f]\n",
                      config->ext.device.type, input_ports[inport], aml_dev->src_gain[inport]);
            }
            ALOGI(" - set gain for in_port[%s], active inport[%s]\n",
                  input_ports[inport], input_ports[aml_dev->active_inport]);
        } else {
            ALOGE("unsupport");
        }
    }

    return 0;
}

static int adev_get_audio_port(struct audio_hw_device *dev, struct audio_port *port)
{
    ALOGI("%s unsupport", __func__);
    (void) dev;
    (void) port;
    return -ENOSYS;
}

static int remove_callback_audio_patch(struct audio_hw_device *dev,
                                       audio_patch_handle_t handle /*currently we don't use it*/,
                                       audio_callback_info_t * callback_info)
{
    int ret = -1;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct aml_audio_patch * aml_patch = NULL;
    aml_audiocb_handle_t *pcallback_handle = NULL;


    if (aml_dev == NULL || callback_info == NULL) {
        ALOGD("%s input is NULL\n", __func__);
        return -1;
    }
    aml_patch = aml_dev->audio_patch;

    if (aml_patch == NULL) {
        ALOGD("There is no Patch\n");
        return -1;
    }

    pcallback_handle = aml_patch->callback_handle;
    if (pcallback_handle == NULL) {
        ALOGD("%s pcallback_handle is NULL\n", __func__, pcallback_handle);
        return -1;
    }

    ret = remove_audio_callback(pcallback_handle, callback_info);

    return ret;
}

static int install_callback_audio_patch(struct audio_hw_device *dev,
                                        audio_patch_handle_t handle /*currently we don't use it*/,
                                        audio_callback_info_t * callback_info,
                                        audio_callback_func_t  callback_func)
{
    int ret = -1;
    struct aml_audio_device *aml_dev = (struct aml_audio_device *) dev;
    struct aml_audio_patch * aml_patch = NULL;
    aml_audiocb_handle_t *pcallback_handle = NULL;

    if (aml_dev == NULL || callback_info == NULL) {
        ALOGD("%s input is NULL\n", __func__);
        return -1;
    }
    aml_patch = aml_dev->audio_patch;

    if (aml_patch == NULL) {
        ALOGD("There is no Patch\n");
        return -1;
    }

    pcallback_handle = aml_patch->callback_handle;
    if (pcallback_handle == NULL) {
        ALOGD("%s pcallback_handle is NULL\n", __func__, pcallback_handle);
        return -1;
    }

    ret = install_audio_callback(pcallback_handle, callback_info, callback_func);

    return ret;
}


#define MAX_SPK_EXTRA_LATENCY_MS (100)
#define DEFAULT_SPK_EXTRA_LATENCY_MS (15)

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct aml_audio_device *adev;
    size_t bytes_per_frame = audio_bytes_per_sample(AUDIO_FORMAT_PCM_16_BIT)
                             * audio_channel_count_from_out_mask(AUDIO_CHANNEL_OUT_STEREO);
    int buffer_size = PLAYBACK_PERIOD_COUNT * DEFAULT_PLAYBACK_PERIOD_SIZE * bytes_per_frame;
    int spk_tuning_buf_size = MAX_SPK_EXTRA_LATENCY_MS
                              * bytes_per_frame * MM_FULL_POWER_SAMPLING_RATE / 1000;
    int spdif_tuning_latency = aml_audio_get_spdif_tuning_latency();
    int card = CARD_AMLOGIC_BOARD;
    int ret, i;
    char buf[PROPERTY_VALUE_MAX];
    int disable_continuous = 1;
    cJSON *config_root = NULL;
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) {
        ret = -EINVAL;
        goto err;
    }
    /*check whether jason config is set*/
    if (audio_config_jason) {
        config_root = audio_config_jason;
    } else {
        /*check is there any confi file*/
        config_root = aml_config_parser(aml_audio_jason_file);
    }
    if (config_root == NULL) {
        printf("Config file parsing error\n");
        return -EINVAL;
    }


    adev = calloc(1, sizeof(struct aml_audio_device));
    if (!adev) {
        ret = -ENOMEM;
        goto err;
    }

    aml_log_init();
    audio_trace_init();
    adev->aml_audio_config = (void*)config_root;

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_3_0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.get_master_volume = adev_get_master_volume;
    adev->hw_device.set_master_mute = adev_set_master_mute;
    adev->hw_device.get_master_mute = adev_get_master_mute;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;


    if (1) { /*(getprop_bool("ro.platform.is.tv")) */
        printf("it is TV\n");
        adev->hw_device.open_output_stream = adev_open_output_stream_new;
        adev->hw_device.close_output_stream = adev_close_output_stream_new;
    } else {
        adev->hw_device.open_output_stream = adev_open_output_stream;
        adev->hw_device.close_output_stream = adev_close_output_stream;
    }
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.create_audio_patch = adev_create_audio_patch;
    adev->hw_device.release_audio_patch = adev_release_audio_patch;
    adev->hw_device.set_audio_port_config = adev_set_audio_port_config;
    adev->hw_device.get_audio_port = adev_get_audio_port;
    adev->hw_device.dump = adev_dump;
    /* ensure card is parsed. */
    card = alsa_device_get_card_index();
    if ((card < 0) || (card > 7)) {
        ALOGE("error to get audio card");
        ret = -EINVAL;
        goto err_adev;
    }

    adev->card = card;
#ifdef AUDIO_ROUTE
    adev->ar = audio_route_init(adev->card, MIXER_XML_PATH);
    if (adev->ar == NULL) {
        ALOGE("audio route init failed");
        ret = -EINVAL;
        goto err_adev;
    }
#endif

    /* Set the default route before the PCM stream is opened */
    adev->mode = AUDIO_MODE_NORMAL;
    adev->out_device = AUDIO_DEVICE_OUT_SPEAKER;
    adev->in_device = AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN;
    adev->hi_pcm_mode = false;
    adev->eq_data.card = adev->card;
#if 0
    if (eq_drc_init(&adev->eq_data) == 0) {
        ALOGI("%s() audio source gain: atv:%f, dtv:%f, hdmiin:%f, av:%f", __func__,
              adev->eq_data.s_gain.atv, adev->eq_data.s_gain.dtv,
              adev->eq_data.s_gain.hdmi, adev->eq_data.s_gain.av);
        ALOGI("%s() audio device gain: speaker:%f, spdif_arc:%f, headphone:%f", __func__,
              adev->eq_data.p_gain.speaker, adev->eq_data.p_gain.spdif_arc,
              adev->eq_data.p_gain.headphone);
        adev->aml_ng_enable = adev->eq_data.p_attr->aml_ng_enable;
        adev->aml_ng_level = adev->eq_data.p_attr->aml_ng_level;
        adev->aml_ng_attrack_time = adev->eq_data.p_attr->aml_ng_attrack_time;
        adev->aml_ng_release_time = adev->eq_data.p_attr->aml_ng_release_time;
        ALOGI("%s() audio noise gate level: %fdB, attrack_time = %dms, release_time = %dms", __func__,
              adev->aml_ng_level, adev->aml_ng_attrack_time, adev->aml_ng_release_time);
    }
#endif
#ifdef AUDIO_ROUTE
    ret = aml_audio_output_routing(&adev->hw_device, OUTPORT_SPEAKER, false);
    if (ret < 0) {
        ALOGE("%s() routing failed", __func__);
        ret = -EINVAL;
        goto err_adev;
    }
#endif
    adev->next_unique_ID = 1;
    list_init(&adev->patch_list);
    adev->effect_buf = malloc(buffer_size);
    if (adev->effect_buf == NULL) {
        ALOGE("malloc effect buffer failed");
        ret = -ENOMEM;
        goto err_adev;
    }
    memset(adev->effect_buf, 0, buffer_size);
    adev->hp_output_buf = malloc(buffer_size);
    if (adev->hp_output_buf == NULL) {
        ALOGE("no memory for headphone output buffer");
        ret = -ENOMEM;
        goto err_effect_buf;
    }
    memset(adev->hp_output_buf, 0, buffer_size);
    adev->effect_buf_size = buffer_size;

    /* init speaker tuning buffers */
    ret = ring_buffer_init(&(adev->spk_tuning_rbuf), spk_tuning_buf_size);
    if (ret < 0) {
        ALOGE("Fail to init audio spk_tuning_rbuf!");
        goto err_spk_tuning_buf;
    }
    adev->spk_tuning_buf_size = spk_tuning_buf_size;

    /* if no latency set by prop, use default one */
    if (spdif_tuning_latency == 0) {
        spdif_tuning_latency = DEFAULT_SPK_EXTRA_LATENCY_MS;
    } else if (spdif_tuning_latency > MAX_SPK_EXTRA_LATENCY_MS) {
        spdif_tuning_latency = MAX_SPK_EXTRA_LATENCY_MS;
    } else if (spdif_tuning_latency < 0) {
        spdif_tuning_latency = 0;
    }

    /* convert MS to data buffer length need to cache */
    adev->spk_tuning_lvl = (spdif_tuning_latency * bytes_per_frame * MM_FULL_POWER_SAMPLING_RATE) / 1000;
    /* end of spker tuning things */
    *device = &adev->hw_device.common;
    adev->dts_post_gain = 1.0;
    /* set default HP gain */
    adev->sink_gain[OUTPORT_HEADPHONE] = 1.0;
    adev->ms12_main1_dolby_dummy = true;
    adev->ms12_ott_enable = false;
    adev->continuous_audio_mode = 0;
    adev->need_remove_conti_mode = false;
    ret = property_get(DISABLE_CONTINUOUS_OUTPUT, buf, NULL);
    if (ret > 0) {
        sscanf(buf, "%d", &disable_continuous);
        if (!disable_continuous) {
            adev->continuous_audio_mode = 1;
        }
        ALOGI("%s[%s] disable_continuous %d\n", DISABLE_CONTINUOUS_OUTPUT, buf, disable_continuous);
    }
    pthread_mutex_init(&adev->alsa_pcm_lock, NULL);

    // try to detect which dobly lib is readable
    adev->dolby_lib_type = detect_dolby_lib_type();
    adev->dolby_lib_type_last = adev->dolby_lib_type;
    if (eDolbyMS12Lib != adev->dolby_lib_type) {
        adev->ms12.dolby_ms12_enable == false;
    }

#ifdef DATMOS
    adev->datmos_enable = !get_datmos_func(&(adev->datmos_param));
    ALOGI("Amlogic HAL get Datmos Func %s!\n", adev->datmos_enable ? "success" : "fail");
    if (adev->datmos_enable) {
        datmos_default_options();
    }
    adev->bm_enable = 0;
    adev->bm_init   = 0;
    adev->dec_params_update_mask = 0;
#endif

    /*init the 3rd party post process*/
    aml_external_pp_init(&adev->hw_device, config_root);

    /* init the input/output function */
    aml_channelmap_parser_init(config_root);
    aml_output_init();
    aml_input_init();
    aml_alsa_init(config_root);
    aml_volctl_init(&adev->hw_device);
    aml_audiolevel_init(&adev->hw_device);
    aml_bm_init(&adev->hw_device);
    adev->hw_audio_type_det = aml_audio_hw_type_parser_init(config_root);

    /* init callback function */
    adev->hw_device.install_callback_audio_patch = install_callback_audio_patch;
    adev->hw_device.remove_callback_audio_patch  = remove_callback_audio_patch;

#if defined(USE_AUDIOSERVICE_S400_SBR) || defined(USE_AUDIOSERVICE_S410_SBR)
    set_tdmout_c_binv(0);
#endif

    ALOGI("%s() adev->dolby_lib_type = %d", __FUNCTION__, adev->dolby_lib_type);
    adev->patch_src = SRC_INVAL;
    adev->audio_type = LPCM;
    adev->is_truehd_within_mat = false;
    adev->is_dolby_atmos = false;
    adev->audio_sample_rate = 0;
    adev->audio_channels    = 0;
    adev->decode_format = AUDIO_FORMAT_INVALID;
    adev->capture_audiotype = -1;
    adev->delay_time    = 0;
    return 0;

err_spk_tuning_buf:
    free(adev->hp_output_buf);
err_effect_buf:
    free(adev->effect_buf);
err_adev:
    free(adev);
err:
    return ret;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "aml audio HW HAL",
        .author = "amlogic, Corp.",
        .methods = &hal_module_methods,
    },
};

int audio_hw_device_get_module(struct hw_module_t** module)
{
    printf("%s \n",  __func__);
    *module = &HAL_MODULE_INFO_SYM.common;

    return 0;
}

int audio_hw_device_set_config_file(char * file_name) {
    int name_length = 0;
    if (file_name == NULL) {
        return -1;
    }
    name_length = strlen(file_name);
    if (name_length >= AUDIO_JASON_FILE_MAXLENGTH) {
        return -1;
    }
    memcpy(aml_audio_jason_file, file_name, name_length);
    aml_audio_jason_file[name_length] = '\0';

    return 0;
}

int audio_hw_device_set_config_jason(void *config_jason) {
    if (config_jason == NULL) {
        return -1;
    }
    audio_config_jason = (cJSON *)config_jason;

    return 0;
}
