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

#ifndef _AML_AUDIO_STREAM_H_
#define _AML_AUDIO_STREAM_H_

#include "audio_core.h"
#include <pthread.h>
#include "aml_ringbuffer.h"
#include "aml_audio_resampler.h"
//#include "audio_hw.h"
#include "aml_callback_api.h"
#include "aml_spdif_in.h"


#ifndef __unused
#define __unused
#endif


typedef enum stream_usecase {
    STREAM_PCM_NORMAL = 0,
    STREAM_PCM_DIRECT,
    STREAM_PCM_HWSYNC,
    STREAM_RAW_DIRECT,
    STREAM_RAW_HWSYNC,
    STREAM_PCM_PATCH,
    STREAM_RAW_PATCH,
    STREAM_USECASE_MAX,
    STREAM_USECASE_INVAL = -1
} stream_usecase_t;


#define RAW_USECASE_MASK ((1<<STREAM_RAW_DIRECT) | (1<<STREAM_RAW_HWSYNC) | (1<<STREAM_RAW_PATCH))

typedef uint32_t usecase_mask_t;

enum stream_status {
    STREAM_STANDBY = 0,
    STREAM_HW_WRITING,
    STREAM_MIXING,
    STREAM_PAUSED
};


/*
 *@brief get this value by adev_set_parameters(), command is "hdmi_format"
 */
enum digital_format {
    PCM = 0,
    DD = 4,
    AUTO = 5
};

static inline bool is_main_write_usecase(stream_usecase_t usecase)
{
    return usecase > 0;
}

static inline bool is_digital_raw_format(audio_format_t format)
{
    switch (format) {
    case AUDIO_FORMAT_AC3:
    case AUDIO_FORMAT_E_AC3:
    case AUDIO_FORMAT_DTS:
    case AUDIO_FORMAT_DTS_HD:
    case AUDIO_FORMAT_DOLBY_TRUEHD:
    case AUDIO_FORMAT_IEC61937:
        return true;
    default:
        return false;
    }
}

static inline stream_usecase_t attr_to_usecase(uint32_t devices __unused,
        audio_format_t format, uint32_t flags)
{
    // hwsync case
    if ((flags & AUDIO_OUTPUT_FLAG_HW_AV_SYNC) && (format != AUDIO_FORMAT_IEC61937)) {
        if (audio_is_linear_pcm(format)) {
            return STREAM_PCM_HWSYNC;
        }

        if (is_digital_raw_format(format)) {
            return STREAM_RAW_HWSYNC;
        }

        return STREAM_USECASE_INVAL;
    }

    // non hwsync cases
    if (/*devices == AUDIO_DEVICE_OUT_HDMI ||*/
        is_digital_raw_format(format)) {
        return STREAM_RAW_DIRECT;
    }
    //multi-channel LPCM or hi-res LPCM
    else if ((flags & AUDIO_OUTPUT_FLAG_DIRECT) && audio_is_linear_pcm(format)) {
        return STREAM_PCM_DIRECT;
    } else {
        return STREAM_PCM_NORMAL;
    }
}

static inline stream_usecase_t convert_usecase_mask_to_stream_usecase(usecase_mask_t mask)
{
    int i = 0;
    for (i = 0; i < STREAM_USECASE_MAX; i++) {
        if ((1 << i) & mask) {
            break;
        }
    }
    //ALOGI("%s mask %#x i %d", __func__, mask, i);
    if (i >= STREAM_USECASE_MAX) {
        return STREAM_USECASE_INVAL;
    } else {
        return (stream_usecase_t)i;
    }
}

typedef enum alsa_device {
    I2S_DEVICE = 0,
    DIGITAL_DEVICE,
    TDM_DEVICE,
    ALSA_DEVICE_CNT
} alsa_device_t;

#define OUTPUT_LAYER


typedef enum output_device {
    PCM_OUTPUT_DEVICE = 0,
    RAW_OUTPUT_DEVICE,
    OUTPUT_DEVICE_CNT
} output_device_t;

typedef enum input_device {
    PCM_INPUT_DEVICE = 0,   /*we treat all the input as PCM*/
    INPUT_DEVICE_CNT
} input_device_t;

typedef enum info_type {
    PCMOUTPUT_CONFIG_INFO,   // refer to aml_stream_config
    OUTPUT_INFO_STATUS,      // running or xrun etc..
    OUTPUT_INFO_DELAYFRAME,  // the delay frames
} info_type_t;

typedef enum output_state {
    OUTPUT_IDLE,
    OUTPUT_OPENED,
    OUTPUT_STARTED,
    OUTPUT_RUNNING,
    OUTPUT_PAUSED,
    OUTPUT_STOPED,
    OUTPUT_CLOSED,
} output_state_t;

typedef struct aml_stream_config {
    unsigned int channels;
    unsigned int rate;
    audio_format_t format;
    unsigned int framesize;
    unsigned int latency; // the value is ms
    unsigned int start_threshold_coef;

} aml_stream_config_t;

typedef struct hdmi_config {
    int port_id;
} hdmi_config_t;

typedef struct aml_device_config {
    audio_devices_t device;
    union {
        hdmi_config_t hdmi_config;
    };
} aml_device_config_t;


typedef union output_info {
    aml_stream_config_t config_info;
    output_state_t      output_state;
    int                 delay_frame;

} output_info_t;
#define AML_MAX_CHANNELS 16
typedef enum channel_id {
    CHANNEL_BASE = 1,
    CHANNEL_LEFT_FRONT = CHANNEL_BASE,
    CHANNEL_RIGHT_FRONT,
    CHANNEL_CENTER,
    CHANNEL_LFE,
    CHANNEL_LEFT_SURROUND,
    CHANNEL_RIGHT_SURROUND,
    CHANNEL_LEFT_REAR_SURROUND,
    CHANNEL_RIGHT_REAR_SURROUND,
    CHANNEL_LEFT_TOP_FRONT,
    CHANNEL_RIGHT_TOP_FRONT,
    CHANNEL_LEFT_TOP_MIDDLE,
    CHANNEL_RIGHT_TOP_MIDDLE,
    CHANNEL_LEFT_TOP_REAR,
    CHANNEL_RIGHT_TOP_REAR,
    CHANNEL_LEFT_DOLBY_ENABLE,
    CHANNEL_RIGHT_DOLBY_ENABLE,
    CHANNEL_LEFT_SUR_DOLBY_ENABLE,
    CHANNEL_RIGHT_SUR_DOLBY_ENABLE,
    CHANNEL_LEFT_REAR_SUR_DOLBY_ENABLE,
    CHANNEL_RIGHT_REAR_SUR_DOLBY_ENABLE,
} channel_id_t;

typedef enum channel_order_type {
    CHANNEL_ORDER_DOLBY,
    CHANNEL_ORDER_DTS,
    CHANNEL_ORDER_HDMIPCM,
}channel_order_type_t;

struct ch_name_pair {
    char name[16];
    channel_id_t ch_id;
};


typedef struct channel_item {
    channel_id_t ch_id;  // left, right ***
    int present;    /* whether the channel is present*/
    int order;      /* what is the channel order */
} channel_item_t;

typedef struct channel_info {
    channel_item_t channel_items[AML_MAX_CHANNELS];
} channel_info_t;

typedef struct ch_volume {
    channel_id_t ch_id;
    int          bOn;
    float        volume;
} ch_volume_t;

typedef struct volume_info {
    float master_vol;
    float pre_scale;
    ch_volume_t volume_item[AML_MAX_CHANNELS];
} volume_info_t;

typedef struct ch_coef {
    channel_id_t ch_id;
    float        coef;
} ch_coef_t;

typedef struct coef_info {
    float bgo_coef;
    float lfe_post_scale;
    ch_coef_t coef_item[AML_MAX_CHANNELS];
} ch_coef_info_t;


enum sample_bitwidth {
    SAMPLE_8BITS =  8,
    SAMPLE_16BITS = 16,
    SAMPLE_24BITS = 24,
    SAMPLE_32BITS = 32,
};

enum sample_endian {
    SAMPLE_LITTLE_ENDIAN,
    SAMPLE_BIG_ENDIAN,
};

typedef struct aml_data_format {
    audio_format_t format;
    int sr ;   /**  samplerate*/
    int ch ;   /**  channels*/
    int bitwidth; /**the sample bit width, refer to sample_bitwidth*/
    int endian;   /*refer to sample_endian*/
    int ch_layout; /*the channel layout*/
    channel_order_type_t ch_order_type;  /*the ch layout type*/
    channel_info_t channel_info; /*the channel detailed present&order info*/

} aml_data_format_t;

typedef enum ease_type{
    EaseLinear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    EaseMax
} ease_type_t;


typedef struct ease_setting{
    float target_volume;    /*the vol range is 0.0~1.0*/
    int duration;           /*the value is ms*/
    ease_type_t ease_type;  /*ease type*/
} ease_setting_t;


static inline alsa_device_t usecase_to_device(stream_usecase_t usecase)
{
    switch (usecase) {
    case STREAM_PCM_NORMAL:
    case STREAM_PCM_DIRECT:
    case STREAM_PCM_HWSYNC:
    case STREAM_PCM_PATCH:
        return I2S_DEVICE;
    case STREAM_RAW_PATCH:
    case STREAM_RAW_DIRECT:
    case STREAM_RAW_HWSYNC:
        return DIGITAL_DEVICE;
    default:
        return I2S_DEVICE;
    }
}

//#ifdef DOLBY_MS12_ENABLE
static inline alsa_device_t usecase_device_adapter_with_ms12(alsa_device_t usecase_device, audio_format_t output_format)
{
    ALOGI("%s usecase_device %d output_format %#x", __func__, usecase_device, output_format);
    switch (usecase_device) {
    case DIGITAL_DEVICE:
    case I2S_DEVICE:
        if ((output_format == AUDIO_FORMAT_AC3) || (output_format == AUDIO_FORMAT_E_AC3)) {
            return DIGITAL_DEVICE;
        } else {
            return I2S_DEVICE;
        }
    default:
        return I2S_DEVICE;
    }
}
//#endif

static inline audio_format_t BitToFormat(int bitwidth)
{
    audio_format_t format;
    switch (bitwidth) {
    case SAMPLE_8BITS:
        format = AUDIO_FORMAT_PCM_8_BIT;
        break;
    case SAMPLE_16BITS:
        format = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case SAMPLE_24BITS:
        /*24 bit store in 32 bits*/
        format = AUDIO_FORMAT_PCM_32_BIT;
        break;
    case SAMPLE_32BITS:
        format = AUDIO_FORMAT_PCM_32_BIT;
        break;
    default:
        format =  AUDIO_FORMAT_PCM_16_BIT;

    }
    return format;
}

static inline  int FormatToBit(audio_format_t format)
{
    int bitwidth = 0;
    switch (format) {
    case AUDIO_FORMAT_PCM_8_BIT:
        bitwidth = SAMPLE_8BITS;
        break;
    case AUDIO_FORMAT_PCM_16_BIT:
        bitwidth = SAMPLE_16BITS;
        break;
    case AUDIO_FORMAT_PCM_8_24_BIT:
        /*24 bit store in 32 bits*/
        bitwidth = SAMPLE_32BITS;
        break;
    case AUDIO_FORMAT_PCM_32_BIT:
        bitwidth = SAMPLE_32BITS;
        break;
    default:
        bitwidth = SAMPLE_16BITS;
    }
    return bitwidth;
}


struct aml_audio_patch {
    struct audio_hw_device *dev;
    ring_buffer_t aml_ringbuffer;
    pthread_t audio_input_threadID;
    pthread_t audio_output_threadID;
    pthread_t audio_parse_threadID;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    void *in_buf;
    size_t in_buf_size;
    void *out_buf;
    size_t out_buf_size;
    void *out_tmpbuf;
    size_t out_tmpbuf_size;
    int input_thread_exit;
    int output_thread_exit;
    int parse_thread_exit;
    void *audio_parse_para;
    audio_devices_t input_src;
    audio_format_t  aformat;
    unsigned int  sample_rate;   // it is used for output
    unsigned int  original_rate; // it is the input rate, used for app info
    unsigned int  ch;
    unsigned int in_period_mul;
    unsigned int out_period_mul;
    audio_channel_mask_t chanmask;
    int hdmi_stable;
    int clk_changed;
    struct timespec mute_start_ts;
#if 0
    struct ring_buffer
        struct thread_read
            struct thread_write
                struct audio_mixer;
    void *output_process_buf;
    void *mixed_buf;
#endif

    /* for AVSYNC tuning */
    int is_src_stable;
    int av_diffs;
    int do_tune;
    int avsync_sample_accumed;
    int avsync_sample_max_cnt;
    int avsync_sample_interval;
    /* end of AVSYNC tuning */

    /*for dtv play parameters */
    int dtv_aformat;
    int dtv_has_video;
    int dtv_decoder_state;
    int dtv_decoder_cmd;
    int dtv_first_apts_flag;
    unsigned int dtv_first_apts;
    unsigned int dtv_pcm_writed;
    unsigned int dtv_pcm_readed;
    unsigned int dtv_decoder_ready;
    unsigned int ouput_thread_created;
    unsigned int decoder_offset ;
    unsigned int outlen_after_last_validpts;
    unsigned long last_valid_pts;
    unsigned int first_apts_lookup_over;
    int dtv_symple_rate;
    int dtv_pcm_channel;
    pthread_mutex_t dtv_output_mutex;
    pthread_mutex_t dtv_input_mutex;
    /*end dtv play*/
    // correspond to audio_patch:: audio_patch_handle_t id;
    // patch unique ID
    int patch_hdl;
    struct resample_para dtv_resample;
    unsigned char *resample_outbuf;


    /** callback info*/
    aml_audiocb_handle_t * callback_handle;
    aml_spdif_in_t       * spdif_in;
};

struct audio_stream_out;
struct audio_stream_in;

stream_usecase_t convert_usecase_mask_to_stream_usecase(usecase_mask_t mask);

/*
 *@brief get sink format by logic min(source format / digital format / sink capability)
 * For Speaker/Headphone output, sink format keep PCM-16bits
 * For optical output, min(dd, source format, digital format)
 * For HDMI_ARC output
 *      1.digital format is PCM, sink format is PCM-16bits
 *      2.digital format is dd, sink format is min (source format,  AUDIO_FORMAT_AC3)
 *      3.digital format is auto, sink format is min (source format, digital format)
 */
void get_sink_format(struct audio_stream_out *stream);

/*@brief check the hdmi rx audio stability by HW register */
bool is_hdmi_in_stable_hw(struct audio_stream_in *stream);
/*@brief check the hdmix rx audio format stability by SW parser */
bool is_hdmi_in_stable_sw(struct audio_stream_in *stream);
/*@brief check the ATV audio stability by HW register */
bool is_atv_in_stable_hw(struct audio_stream_in *stream);
int set_audio_source(int audio_source);
int enable_HW_resample(int sr, int enable);
int set_spdifin_pao(int enable);
int get_hdmiin_samplerate(void);
int get_spdifin_samplerate(void);
int set_audio_inskew(int value);
int set_tdmout_c_binv(int binv);

/*
 *@brief clean the tmp_buffer_8ch&audioeffect_tmp_buffer and release audio stream
 */
void  release_audio_stream(struct audio_stream_out *stream);
/*@brief check the AV audio stability by HW register */
bool is_av_in_stable_hw(struct audio_stream_in *stream);

int get_input_streaminfo(struct audio_stream_in *stream, aml_data_format_t *data_format);

int get_stream_parameters(struct audio_hw_device *dev, const char *keys, char *temp_buf, size_t temp_buf_size);
int spdifhw_audio_format_detection();

int get_hdmiin_i2sclk(void);

#endif /* _AML_AUDIO_STREAM_H_ */
