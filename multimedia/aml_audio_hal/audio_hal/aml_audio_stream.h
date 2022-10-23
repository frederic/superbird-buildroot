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

#include <compiler.h>
#include <system/audio.h>
#include <pthread.h>
#include "aml_ringbuffer.h"
#include "audio_hw.h"
#include "audio_hw_profile.h"

#define RAW_USECASE_MASK ((1<<STREAM_RAW_DIRECT) | (1<<STREAM_RAW_HWSYNC) | (1<<STREAM_RAW_PATCH))

typedef uint32_t usecase_mask_t;

/* sync with tinymix before TXL */
enum input_source {
    SRC_NA = -1,
    LINEIN  = 0,
    ATV     = 1,
    HDMIIN  = 2,
    SPDIFIN = 3,
    ARCIN   = 4,
};

/* sync with tinymix after auge */
enum auge_input_source {
    TDMIN_A = 0,
    TDMIN_B = 1,
    TDMIN_C = 2,
    SPDIFIN_AUGE = 3,
    PDMIN = 4,
    FRATV = 5,
    TDMIN_LB    = 6,
    LOOPBACK_A  = 7,
    FRHDMIRX    = 8,
    LOOPBACK_B  = 9,
    SPDIFIN_LB  = 10,
    EARCRX_DMAC = 11,
    RESERVED_0  = 12,
    RESERVED_1  = 13,
    RESERVED_2  = 14,
    VAD     = 15,
};

/*
 *@brief get this value by adev_set_parameters(), command is "hdmi_format"
 */
enum digital_format {
    PCM = 0,
    DD = 4,
    AUTO = 5
};

/**\brief Audio output mode*/
typedef enum {
    AM_AOUT_OUTPUT_STEREO,     /**< Stereo output*/
    AM_AOUT_OUTPUT_DUAL_LEFT,  /**< Left audio output to dual channel*/
    AM_AOUT_OUTPUT_DUAL_RIGHT, /**< Right audio output to dual channel*/
    AM_AOUT_OUTPUT_SWAP,        /**< Swap left and right channel*/
    AM_AOUT_OUTPUT_LRMIX       /**< mix left and right channel*/
} AM_AOUT_OutputMode_t;

enum sample_bitwidth {
    SAMPLE_8BITS =  8,
    SAMPLE_16BITS = 16,
    SAMPLE_24BITS = 24,
    SAMPLE_32BITS = 32,
};

typedef enum hdmiin_audio_packet {
    AUDIO_PACKET_NONE,
    AUDIO_PACKET_AUDS,
    AUDIO_PACKET_OBA,
    AUDIO_PACKET_DST,
    AUDIO_PACKET_HBR,
    AUDIO_PACKET_OBM,
    AUDIO_PACKET_MAS
} hdmiin_audio_packet_t;

static inline bool is_main_write_usecase(stream_usecase_t usecase)
{
    return usecase > 0;
}

static inline bool is_digital_raw_format(audio_format_t format)
{
    switch (format) {
    case AUDIO_FORMAT_AC3:
    case AUDIO_FORMAT_E_AC3:
    case AUDIO_FORMAT_MAT:
    case AUDIO_FORMAT_AC4:
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
    ALOGI("%s mask %#x i %d", __func__, mask, i);
    if (i >= STREAM_USECASE_MAX) {
        return STREAM_USECASE_INVAL;
    } else {
        return (stream_usecase_t)i;
    }
}

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

static inline alsa_device_t usecase_device_adapter_with_ms12(alsa_device_t usecase_device, audio_format_t output_format)
{
    ALOGI("%s usecase_device %d output_format %#x", __func__, usecase_device, output_format);
    switch (usecase_device) {
    case DIGITAL_DEVICE:
    case I2S_DEVICE:
        if ((output_format == AUDIO_FORMAT_AC3) || (output_format == AUDIO_FORMAT_E_AC3) || (output_format == AUDIO_FORMAT_MAT) ) {
            return DIGITAL_DEVICE;
        } else {
            return I2S_DEVICE;
        }
    default:
        return I2S_DEVICE;
    }
}

typedef void (*dtv_avsync_process_cb)(struct aml_audio_patch* patch,struct aml_stream_out* stream_out);

struct aml_audio_patch {
    struct audio_hw_device *dev;
    ring_buffer_t aml_ringbuffer;
    ring_buffer_t dtvin_ringbuffer;
    ring_buffer_t assoc_ringbuffer;
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
    int dtvin_buffer_inited;
    int assoc_buffer_inited;
    int input_thread_exit;
    int output_thread_exit;
    int parse_thread_exit;
    void *audio_parse_para;
    audio_devices_t input_src;
    audio_format_t  aformat;
    int  sample_rate;
    audio_channel_mask_t chanmask;
    audio_channel_mask_t in_chanmask;
    int in_sample_rate;
    audio_format_t in_format;

    audio_devices_t output_src;
    bool is_dtv_src;
    audio_channel_mask_t out_chanmask;
    int out_sample_rate;
    audio_format_t out_format;
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
    unsigned char dtv_NchOriginal;
    unsigned char dtv_lfepresent;
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
    unsigned int dtv_output_clock;
    unsigned int dtv_default_i2s_clock;
    unsigned int dtv_default_spdif_clock;
    unsigned int spdif_format_set;
    int spdif_step_clk;
    int i2s_step_clk;
    int dtv_audio_mode;
    int dtv_apts_lookup;
    int dtv_audio_tune;
    int pll_state;
    unsigned int last_apts;
    unsigned int last_pcrpts;
    dtv_avsync_process_cb avsync_callback;
    pthread_mutex_t dtv_output_mutex;
    pthread_mutex_t dtv_input_mutex;
    pthread_mutex_t assoc_mutex;
    /*end dtv play*/
    // correspond to audio_patch:: audio_patch_handle_t id;
    // patch unique ID
    int patch_hdl;
    struct resample_para dtv_resample;
    unsigned char *resample_outbuf;
    AM_AOUT_OutputMode_t   mode;
    unsigned char avsync_adelay;
    unsigned char avsync_tuned;
    unsigned char avsync_drop;
    void *drop_buf;
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
int set_audio_source(struct aml_mixer_handle *mixer_handle,
		enum input_source audio_source, bool is_auge);
int get_HW_resample(struct aml_mixer_handle *mixer_handle);
int enable_HW_resample(struct aml_mixer_handle *mixer_handle, int enable_sr);
bool Stop_watch(struct timespec start_ts, int64_t time);
bool signal_status_check(audio_devices_t in_device, int *mute_time,
                         struct audio_stream_in *stream);
int set_spdifin_pao(struct aml_mixer_handle *mixer_handle, int enable);
int get_hdmiin_samplerate(struct aml_mixer_handle *mixer_handle);
hdmiin_audio_packet_t get_hdmiin_audio_packet(struct aml_mixer_handle *mixer_handle);
int get_hdmiin_channel(struct aml_mixer_handle *mixer_handle);

/*
 *@brief clean the tmp_buffer_8ch&audioeffect_tmp_buffer and release audio stream
 */
void  release_audio_stream(struct audio_stream_out *stream);
/*@brief check the AV audio stability by HW register */
bool is_av_in_stable_hw(struct audio_stream_in *stream);

int set_stream_dual_output(struct audio_stream_out *stream, bool en);
int update_stream_dual_output(struct audio_stream_out *stream);
bool is_dual_output_stream(struct audio_stream_out *stream);

int get_spdifin_samplerate(struct aml_mixer_handle *mixer_handle);
unsigned int inport_to_device(enum IN_PORT inport);

#endif /* _AML_AUDIO_STREAM_H_ */
