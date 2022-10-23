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

#ifndef _AUDIO_HW_H_
#define _AUDIO_HW_H_

#include "resampler.h"
#include "audio_core.h"
#include <list.h>

/* kill and replace kernel-specific types */
#ifndef __user
#define __user
#endif
#ifndef __force
#define __force
#endif

#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>

/* ALSA cards for AML */
#define CARD_AMLOGIC_BOARD 0
#define CARD_AMLOGIC_DEFAULT CARD_AMLOGIC_BOARD
/* ALSA ports for AML */
#if (ENABLE_HUITONG == 0)
#define PORT_MM 1
#endif

#include "audio_hwsync.h"
#include "audio_post_process.h"
#include "aml_hw_profile.h"
#include "aml_audio_stream.h"
#include "aml_hw_mixer.h"
#include "audio_eq_drc_compensation.h"
#include "aml_dcv_dec_api.h"
#include "aml_dca_dec_api.h"
#include "aml_audio_types_def.h"
#include "audio_format_parse.h"
#include "aml_alsa_mixer.h"
#include "aml_sample_conv.h"
#include "aml_channel_map.h"
#include "aml_datmos_api.h"
#include "aml_audio_ease.h"
#include "aml_audio_resample_manager.h"

/* number of frames per period */
/*
 * change DEFAULT_PERIOD_SIZE from 1024 to 512 for passing CTS
 * test case test4_1MeasurePeakRms(android.media.cts.VisualizerTest)
 */
#ifndef DEFAULT_PLAYBACK_PERIOD_SIZE
#define DEFAULT_PLAYBACK_PERIOD_SIZE 512
#endif

#ifndef DEFAULT_CAPTURE_PERIOD_SIZE
#define DEFAULT_CAPTURE_PERIOD_SIZE  512
#endif

/* number of ICE61937 format frames per period */
#define DEFAULT_IEC_SIZE 6144

/* number of periods for low power playback */
#define PLAYBACK_PERIOD_COUNT 4
/* number of periods for capture */
#define CAPTURE_PERIOD_COUNT 4

#include "aml_audio_ms12.h"

/* minimum sleep time in out_write() when write threshold is not reached */
#define MIN_WRITE_SLEEP_US 5000

#define RESAMPLER_BUFFER_FRAMES (DEFAULT_PLAYBACK_PERIOD_SIZE * 6)
#define RESAMPLER_BUFFER_SIZE (4 * RESAMPLER_BUFFER_FRAMES)

static unsigned int DEFAULT_OUT_SAMPLING_RATE = 48000;

/* sampling rate when using MM low power port */
#define MM_LOW_POWER_SAMPLING_RATE 44100
/* sampling rate when using MM full power port */
#define MM_FULL_POWER_SAMPLING_RATE 48000
/* sampling rate when using VX port for narrow band */
#define VX_NB_SAMPLING_RATE 8000

#define AUDIO_PARAMETER_STREAM_EQ "audioeffect_eq"
#define AUDIO_PARAMETER_STREAM_SRS "audioeffect_srs_param"
#define AUDIO_PARAMETER_STREAM_SRS_GAIN "audioeffect_srs_gain"
#define AUDIO_PARAMETER_STREAM_SRS_SWITCH "audioeffect_srs_switch"

/* Get a new HW synchronization source identifier.
 * Return a valid source (positive integer) or AUDIO_HW_SYNC_INVALID if an error occurs
 * or no HW sync is available. */
#define AUDIO_PARAMETER_HW_AV_SYNC "hw_av_sync"

#define AUDIO_PARAMETER_HW_AV_EAC3_SYNC "HwAvSyncEAC3Supported"

#define DDP_FRAME_SIZE      768
#define EAC3_MULTIPLIER 4

#define DEFAULT_CHANNEL_NUM 2//STEREO
#define DEFAULT_BITWIDTH 16//16bits_per_sample

/*
enum {
    TYPE_PCM = 0,
    TYPE_AC3 = 2,
    TYPE_DTS = 3,
    TYPE_EAC3 = 4,
    TYPE_DTS_HD = 5 ,
    TYPE_MULTI_PCM = 6,
    TYPE_TRUE_HD = 7,
    TYPE_DTS_HD_MA = 8,//should not used after we unify DTS-HD&DTS-HD MA
    TYPE_PCM_HIGH_SR = 9,
};
*/

#define AML_HAL_MIXER_BUF_SIZE  64*1024

#define SYSTEM_APP_SOUND_MIXING_ON 1
#define SYSTEM_APP_SOUND_MIXING_OFF 0

struct aml_hal_mixer {
    unsigned char start_buf[AML_HAL_MIXER_BUF_SIZE];
    unsigned int wp;
    unsigned int rp;
    unsigned int buf_size;
    /* flag to check if need cache some data before write to mix */
    unsigned char need_cache_flag;
    pthread_mutex_t lock;
};

enum arc_hdmi_format {
    _LPCM = 1,
    _AC3,
    _MPEG1,
    _MP3,
    _MPEG2,
    _AAC,
    _DTS,
    _ATRAC,
    _ONE_BIT_AUDIO,
    _DDP,
    _DTSHD,
    _MAT,
    _DST,
    _WMAPRO
};

struct format_desc {
    enum arc_hdmi_format fmt;
    bool is_support;
    unsigned int max_channels;
    /*
     * bit:    6     5     4    3    2    1    0
     * rate: 192  176.4   96  88.2  48  44.1   32
     */
    unsigned int sample_rate_mask;
    unsigned int max_bit_rate;
    /* only used by dd+ format */
    bool   atmos_supported;
};

struct aml_arc_hdmi_desc {
    unsigned int avr_port;
    struct format_desc pcm_fmt;
    struct format_desc dts_fmt;
    struct format_desc dd_fmt;
    struct format_desc ddp_fmt;
};

enum patch_src_assortion {
    SRC_DTV,
    SRC_ATV,
    SRC_LINEIN,
    SRC_HDMIIN,
    SRC_SPDIFIN,
    SRC_OTHER,
    SRC_INVAL
};

enum OUT_PORT {
    OUTPORT_SPEAKER = 0,
    OUTPORT_HDMI_ARC,
    OUTPORT_HDMI,
    OUTPORT_SPDIF,
    OUTPORT_AUX_LINE,
    OUTPORT_HEADPHONE,
    OUTPORT_MAX
};

enum IN_PORT {
    INPORT_TUNER = 0,
    INPORT_HDMIIN,
    INPORT_SPDIF,
    INPORT_LINEIN,
    INPORT_MAX
};

struct audio_patch_set {
    struct listnode list;
    struct audio_patch audio_patch;
};


#define MAX_STREAM_NUM   5
#define HDMI_ARC_MAX_FORMAT  20
struct aml_audio_device {
    struct audio_hw_device hw_device;
    /* see note below on mutex acquisition order */
    pthread_mutex_t lock;
    pthread_mutex_t pcm_write_lock;
    /*
    if dolby ms12 is enabled, the ms12 thread will change the
    stream information depending on the main input format.
    we need protect the risk and this case.
    */
    pthread_mutex_t trans_lock;
    int mode;
    audio_devices_t in_device;
    audio_devices_t out_device;
    int in_call;
    struct aml_stream_in *active_input;
    struct aml_stream_out *active_output[MAX_STREAM_NUM];
    unsigned char active_output_count;
    bool mic_mute;
    bool speaker_mute;
    unsigned int card;
    struct audio_route *ar;
    struct echo_reference_itfe *echo_reference;
    bool low_power;
    struct aml_stream_out *hwsync_output;
    struct aml_hal_mixer hal_mixer;
    struct pcm *pcm;
    bool pcm_paused;
    unsigned hdmi_arc_ad[HDMI_ARC_MAX_FORMAT];
    bool hi_pcm_mode;
    bool audio_patching;
    /* audio configuration for dolby HDMI/SPDIF output */
    int hdmi_format;
    int spdif_format;
    int hdmi_is_pth_active;
    int disable_pcm_mixing;
    /* mute/unmute for vchip  lock control */
    bool parental_control_av_mute;
    int routing;
    struct audio_config output_config;
    struct aml_arc_hdmi_desc hdmi_descs;
    int arc_hdmi_updated;
    struct aml_native_postprocess native_postprocess;
    /* to classify audio patch sources */
    enum patch_src_assortion patch_src;
    /* for port config infos */
    float sink_gain[OUTPORT_MAX];
    float speaker_volume;
    enum OUT_PORT active_outport;
    float src_gain[INPORT_MAX];
    enum IN_PORT active_inport;
    /* message to handle usecase changes */
    bool usecase_changed;
    uint32_t usecase_masks;
    struct aml_stream_out *active_outputs[STREAM_USECASE_MAX];
    struct aml_audio_patch *audio_patch;
    /* indicates atv to mixer patch, no need HAL patching  */
    bool tuner2mix_patch;
    /* Now only two pcm handle supported: I2S, SPDIF */
    pthread_mutex_t alsa_pcm_lock;
    struct pcm *pcm_handle[ALSA_DEVICE_CNT];
    int pcm_refs[ALSA_DEVICE_CNT];
    bool is_paused[ALSA_DEVICE_CNT];
    struct aml_hw_mixer hw_mixer;
    audio_format_t sink_format;
    audio_format_t optical_format;
    volatile int32_t next_unique_ID;
    /* list head for audio_patch */
    struct listnode patch_list;
    void *temp_buf;
    int temp_buf_size;
    int temp_buf_pos;
    bool dual_decoder_support;
    bool dual_spdifenc_inited;

    /* Dolby MS12 lib variable start */
    struct dolby_ms12_desc ms12;
    //bool dolby_ms12_status; //not used any more?
    struct pcm_config ms12_config;
    int mixing_level;
    bool associate_audio_mixing_enable;
    bool need_reset_for_dual_decoder;
    /* Dolby MS12 lib variable end */

    /*used for ac3 eac3 decoder*/
    int digital_raw;
    int dcv_output_ch;
    /**
     * enum eDolbyLibType
     * DolbyDcvLib  = dcv dec lib   , libHwAudio_dcvdec.so
     * DolbyMS12Lib = dolby MS12 lib, libdolbyms12.so
     */
    int dolby_lib_type;
    int dolby_lib_type_last;
    /*used for dts decoder*/
    //struct dca_dts_dec dts_hd;
    int is_dtscd;

    bool bHDMIARCon;
    bool bHDMIConnected;

    /**
     * buffer pointer whose data output to headphone
     * buffer size equal to efect_buf_size
     */
    void *hp_output_buf;
    void *effect_buf;
    size_t effect_buf_size;
    /* ringbuffer size lvl for tuning latency between spk and spdif */
    size_t spk_tuning_lvl;
    /* ringbuffer for tuning latency total buf size */
    size_t spk_tuning_buf_size;
    ring_buffer_t spk_tuning_rbuf;
    bool mix_init_flag;
    struct eq_drc_data eq_data;
    /*used for high pricision A/V from amlogic amadec decoder*/
    unsigned first_apts;
    /*
    first apts flag for alsa hardware prepare,true,need set apts to hw.
    by default it is false as we do not need set the first apts in normal use case.
    */
    bool first_apts_flag;
    size_t frame_trigger_thred;
    struct aml_audio_parser *aml_parser;
    int continuous_audio_mode;
    /*
    we need later release continuous_audio_mode when direct audio is active.
    */
    bool need_remove_conti_mode;
    int debug_flag;
    int bypass_enable;
    float dts_post_gain;
    bool spdif_encoder_init_flag;
    /*atsc has video in program*/
    bool is_has_video;
    struct aml_stream_out *ms12_out;
    bool ms12_ott_enable;
    bool ms12_main1_dolby_dummy;
    /*amlogic soft ware noise gate fot analog TV source*/
    void* aml_ng_handle;
    int aml_ng_enable;
    float aml_ng_level;
    int aml_ng_attrack_time;
    int aml_ng_release_time;
    int system_app_mixing_status;
    int audio_type;
    int audio_latency;  // currently work on pulse audio
    int capture_device;
    /*below item is used for HDMI input setting*/
    int capture_ch;
    int capture_samplerate;
    int capture_audiotype;  /*-1, NoneAudio, 0: PCM, 1: bitstream */


#ifdef DATMOS
    struct aml_datmos_param datmos_param;
    bool datmos_enable;
    int is_truehd_within_mat;
    int is_dolby_atmos;
    int audio_sample_rate;//audio source samplerate
    int audio_channels;
    audio_format_t decode_format;
    int bm_init;
    int lowerpass_corner;//lowerpass corner for bass management
    bool bm_enable;
    unsigned int dec_params_update_mask;
#endif

    volume_info_t volume_info;
    ch_coef_info_t ch_coef_info; // used for BM process

    void * level_handle;
    /*delay parameter*/
    int delay_max;   // the max audio delay time ms
    int delay_time;  // audio delay time ms
    void * delay_handle;
    void * aml_audio_config; /*cjson handle*/
};

struct aml_stream_out {
    struct audio_stream_out stream;
    /* see note below on mutex acquisition order */
    pthread_mutex_t lock;
    /* config which set to ALSA device */
    struct pcm_config config;
    /* channel mask exposed to AudioFlinger. */
    audio_channel_mask_t hal_channel_mask;
    /* format mask exposed to AudioFlinger. */
    audio_format_t hal_format;
    /* samplerate exposed to AudioFlinger. */
    unsigned int hal_rate;
    audio_output_flags_t flags;
    audio_devices_t out_device;
    struct pcm *pcm;
    struct resampler_itfe *resampler;
    char *buffer;
    size_t buffer_frames;
    bool standby;
    struct aml_audio_device *dev;
    int write_threshold;
    bool low_power;
    unsigned multich;
    int codec_type;
    uint64_t frame_write_sum;
    uint64_t frame_skip_sum;
    uint64_t last_frames_postion;
    uint64_t spdif_enc_init_frame_write_sum;
    int skip_frame;
    int32_t *tmp_buffer_8ch;
    size_t tmp_buffer_8ch_size;
    int is_tv_platform;
    void *audioeffect_tmp_buffer;
    bool pause_status;
    bool hw_sync_mode;
    float volume_l;
    float volume_r;
    int last_codec_type;
    /**
     * as raw audio framesize  is 1 computed by audio_stream_out_frame_size
     * we need divide more when we got 61937 audio package
     */
    int raw_61937_frame_size;
    /* recorded for wraparound print info */
    unsigned last_dsp_frame;
    //audio_hwsync_t hwsync;
    audio_hwsync_t *hwsync;
    struct timespec timestamp;
    stream_usecase_t usecase;
    uint32_t dev_usecase_masks;
    /**
     * flag indicates that this stream need do mixing
     * int is_in_mixing: 1;
     * Normal pcm may not hold alsa pcm device.
     */
    int is_normal_pcm;
    unsigned int card;
    alsa_device_t device;
    ssize_t (*write)(struct audio_stream_out *stream, const void *buffer, size_t bytes);
    enum stream_status status;
    audio_format_t hal_internal_format;
    bool dual_output_flag;
    uint64_t input_bytes_size;
    uint64_t continuous_audio_offset;
    bool hwsync_pcm_config;
    bool hwsync_raw_config;
    bool direct_raw_config;
    bool is_device_differ_with_ms12;
    uint64_t total_write_size;
    int  ddp_frame_size;
    int dropped_size;
    unsigned long long mute_bytes;
    bool is_get_mute_bytes;
    size_t frame_deficiency;
    bool normal_pcm_mixing_config;
    bool flag_dtschecked;

    void *output_handle[OUTPUT_DEVICE_CNT];    /*store output handle*/

    aml_dec_t *aml_dec;                        /*store the decoder handle*/
    aml_sample_conv_t * sample_convert;        /*store the sample convert handle*/
    aml_channel_map_t * channel_map;           /*store the channel map handle*/
    aml_audio_resample_t *resample_handle;     /*store the resample handle*/
    aml_dec_config_t    dec_config;            /*store the special decoder config info*/
};

typedef ssize_t (*write_func)(struct audio_stream_out *stream, const void *buffer, size_t bytes);

#define MAX_PREPROCESSORS 3 /* maximum one AGC + one NS + one AEC per input stream */
struct aml_stream_in {
    struct audio_stream_in stream;
    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm *pcm;
    int device;
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;
    int16_t *buffer;
    size_t frames_in;
    unsigned int requested_rate;
    uint32_t main_channels;
    bool standby;
    int source;
    struct echo_reference_itfe *echo_reference;
    bool need_echo_reference;
    effect_handle_t preprocessors[MAX_PREPROCESSORS];
    int num_preprocessors;
    int16_t *proc_buf;
    size_t proc_buf_size;
    size_t proc_frames_in;
    int16_t *ref_buf;
    size_t ref_buf_size;
    size_t ref_frames_in;
    int read_status;
    /* HW parser audio format */
    int spdif_fmt_hw;
    /* SW parser audio format */
    audio_format_t spdif_fmt_sw;
    struct timespec mute_start_ts;
    int mute_flag;
    int mute_log_cntr;
    struct aml_audio_device *dev;

    void *input_handle[INPUT_DEVICE_CNT];    /*store input handle*/
};
typedef  int (*do_standby_func)(struct aml_stream_out *out);
typedef  int (*do_startup_func)(struct aml_stream_out *out);

inline int continous_mode(struct aml_audio_device *adev)
{
    return adev->continuous_audio_mode;
}
inline bool direct_continous(struct audio_stream_out *stream)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    if ((out->flags & AUDIO_OUTPUT_FLAG_DIRECT) && adev->continuous_audio_mode) {
        return true;
    } else {
        return false;
    }
}
inline bool primary_continous(struct audio_stream_out *stream)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    if ((out->flags & AUDIO_OUTPUT_FLAG_PRIMARY) && adev->continuous_audio_mode) {
        return true;
    } else {
        return false;
    }
}
/* called when adev locked */
inline int dolby_stream_active(struct aml_audio_device *adev)
{
    int i = 0;
    int is_dolby = 0;
    struct aml_stream_out *out = NULL;
    for (i = 0 ; i < STREAM_USECASE_MAX; i++) {
        out = adev->active_outputs[i];
        if (out && (out->hal_internal_format == AUDIO_FORMAT_AC3 || out->hal_internal_format == AUDIO_FORMAT_E_AC3)) {
            is_dolby = 1;
            break;
        }
    }
    return is_dolby;
}
/* called when adev locked */
inline int hwsync_lpcm_active(struct aml_audio_device *adev)
{
    int i = 0;
    int is_hwsync_lpcm = 0;
    struct aml_stream_out *out = NULL;
    for (i = 0 ; i < STREAM_USECASE_MAX; i++) {
        out = adev->active_outputs[i];
        if (out && audio_is_linear_pcm(out->hal_internal_format) && (out->flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC)) {
            is_hwsync_lpcm = 1;
            break;
        }
    }
    return is_hwsync_lpcm;
}

inline struct aml_stream_out *direct_active(struct aml_audio_device *adev)
{
    int i = 0;
    struct aml_stream_out *out = NULL;
    for (i = 0 ; i < STREAM_USECASE_MAX; i++) {
        out = adev->active_outputs[i];
        if (out && (out->flags & AUDIO_OUTPUT_FLAG_DIRECT)) {
            return out;
        }
    }
    return NULL;
}

/*
 *@brief get_output_format get the output format always return the "sink_format" of adev
 */
audio_format_t get_output_format(struct audio_stream_out *stream);
void *audio_patch_output_threadloop(void *data);

ssize_t aml_audio_spdif_output(struct audio_stream_out *stream,
                               const void *buffer, size_t bytes);

/*
 *@brief audio_hal_data_processing
 * format:
 *    if pcm-16bits-stereo, add audio effect process, and mapping to 8ch
 *    if raw data, packet it to IEC61937 format with spdif encoder
 *    if IEC61937 format, write them to hardware
 * return
 *    0, success
 *    -1, fail
 */
ssize_t audio_hal_data_processing(struct audio_stream_out *stream
                                  , const void *input_buffer
                                  , size_t input_buffer_bytes
                                  , void **output_buffer
                                  , size_t *output_buffer_bytes
                                  , aml_data_format_t * data_format);


/*
 *@brief hw_write the api to write the data to audio hardware
 */
ssize_t hw_write(struct audio_stream_out *stream
                 , const void *buffer
                 , size_t bytes
                 , aml_data_format_t * data_format);


#define IS_DATMOS_DECODER_SUPPORT(format) ((format == AUDIO_FORMAT_AC3) || \
                                            (format == AUDIO_FORMAT_E_AC3) || \
                                            (format == AUDIO_FORMAT_DOLBY_TRUEHD) || \
                                            (format == AUDIO_FORMAT_PCM_8_BIT) || \
                                            (format == AUDIO_FORMAT_PCM_16_BIT) || \
                                            (format == AUDIO_FORMAT_PCM_8_24_BIT) || \
                                            (format == AUDIO_FORMAT_PCM_32_BIT))

#define EAC3_MULTIPLIER 4
#define TRUEHD_MULTIPLIER 16

#define IS_PCM_FORMAT(format)       ((format == AUDIO_FORMAT_PCM_8_BIT) || \
                                    (format == AUDIO_FORMAT_PCM_16_BIT) || \
                                    (format == AUDIO_FORMAT_PCM_8_24_BIT) || \
                                    (format == AUDIO_FORMAT_PCM_32_BIT))



#ifdef DATMOS
#define IS_DECODER_SUPPORT(format)   ((format == AUDIO_FORMAT_DTS) || \
                                    (format == AUDIO_FORMAT_DTS_HD) || \
                                    (format == AUDIO_FORMAT_AC3) || \
                                    (format == AUDIO_FORMAT_E_AC3) || \
                                    (format == AUDIO_FORMAT_DOLBY_TRUEHD) || \
                                    (format == AUDIO_FORMAT_PCM_8_BIT) || \
                                    (format == AUDIO_FORMAT_PCM_16_BIT) || \
                                    (format == AUDIO_FORMAT_PCM_8_24_BIT) || \
                                    (format == AUDIO_FORMAT_PCM_32_BIT))
#else
#define IS_DECODER_SUPPORT(format)   ((format == AUDIO_FORMAT_DTS) || \
                                    (format == AUDIO_FORMAT_DTS_HD) || \
                                    (format == AUDIO_FORMAT_AC3) || \
                                    (format == AUDIO_FORMAT_E_AC3))
#endif
#endif
