#ifndef __AUDIO_FORMAT_PARSE_H__
#define __AUDIO_FORMAT_PARSE_H__

#include "audio_core.h"
#include <tinyalsa/asoundlib.h>

/*IEC61937 package presamble Pc value 0-4bit */
enum IEC61937_PC_Value {
    IEC61937_NULL               = 0x00,          ///< NULL data
    IEC61937_AC3                = 0x01,          ///< AC-3 data
    IEC61937_DTS1               = 0x0B,          ///< DTS type I   (512 samples)
    IEC61937_DTS2               = 0x0C,          ///< DTS type II  (1024 samples)
    IEC61937_DTS3               = 0x0D,          ///< DTS type III (2048 samples)
    IEC61937_DTSHD              = 0x11,          ///< DTS HD data
    IEC61937_EAC3               = 0x15,          ///< E-AC-3 data
    IEC61937_TRUEHD             = 0x16,          ///< TrueHD data
    IEC61937_PAUSE              = 0x03,          ///< Pause
};

enum audio_type {
    LPCM = 0,
    AC3,
    EAC3,
    DTS,
    DTSCD,
    DTSHD,
    TRUEHD,
    PAUSE,
    MUTE,
};

#define PARSER_DEFAULT_PERIOD_SIZE  (1024)

/*Period of data burst in IEC60958 frames*/
#define AC3_PERIOD_SIZE  (6144)
#define EAC3_PERIOD_SIZE (24576)
#define THD_PERIOD_SIZE  (61440)

#define DTS1_PERIOD_SIZE (2048)
#define DTS2_PERIOD_SIZE (4096)
#define DTS3_PERIOD_SIZE (8192)
/*min DTSHD Period 2048; max DTSHD Period 65536*/
#define DTSHD_PERIOD_SIZE (2048)

#define IEC61937_CHECK_SIZE 32768   // for 48K 2ch, it is about 170ms


enum input_source {
    LINEIN = 0,
    ATV,
    HDMIIN,
    SPDIFIN,
};

typedef struct audio_type_parse {
    struct pcm_config config_in;
    struct pcm *in;
    unsigned int card;
    unsigned int device;
    unsigned int flags;

    int period_bytes;
    char *parse_buffer;

    int audio_type;
    int cur_audio_type;

    audio_channel_mask_t audio_ch_mask;

    int read_bytes;
    int package_size;

    int running_flag;
    int state;
    int parsed_size;
    int iec_check_size;
    int type_preset; /*-1, NoneAudio, 0: PCM, 1: bitstream*/
} audio_type_parse_t;

int creat_pthread_for_audio_type_parse(
    pthread_t *audio_type_parse_ThreadID,
    void **status,
    int input_sr, int input_ch);
void exit_pthread_for_audio_type_parse(
    pthread_t audio_type_parse_ThreadID,
    void **status);

/*
 *@brief convert the audio type to android audio format
 */
audio_format_t andio_type_convert_to_android_audio_format_t(int codec_type);


/*
 *@brief convert android audio format to the audio type
 */
int android_audio_format_t_convert_to_andio_type(audio_format_t format);
/*
 *@brief get current android audio fromat from audio parser thread
 */
audio_format_t audio_parse_get_audio_type(audio_type_parse_t *status);
/*
 *@brief get current audio channel mask from audio parser thread
 */
audio_channel_mask_t audio_parse_get_audio_channel_mask(audio_type_parse_t *status);
/*
 *@brief gget current audio fromat from audio parser thread
 */
int audio_parse_get_audio_type_direct(audio_type_parse_t *status);
/*
 *@brief get current audio type from buffer data
 * input params:
 *          void *buffer : scan data address
            size_t bytes : scan data size
            int *package_size : if data is IEC71937 format, get the IEC61937 size
            audio_channel_mask_t *cur_ch_mask : if format is DDP, return its channel mask
            int *raw_size : raw data size in payload
            int *offset : IEC61937 offset in 'buffer'
 *
 * return value:
 *          0/1/2/3~: current audio type defined by enum audio_type
 */
int audio_type_parse(void *buffer, size_t bytes, int *package_size, audio_channel_mask_t *cur_ch_mask, int *raw_size, int *offset);
/*
 *@brief create audio parse handle for none-thread case
 * input params:
 *          void **status: store the handle
            int iec_check_size : how many size to check to detect the pcm case
            int type_preset: if we know its type,-1, NoneAudio, 0: PCM, 1: bitstream
 *
 * return value:
 *          0 : success
 *          -1: failed
 */

int creat_audio_type_parse(void **status, int iec_check_size, int type_preset);

void release_audio_type_parse(void **status);

void feeddata_audio_type_parse(void **status, char * input, int size);

/*
 *@brief get current audio type from buffer data
 * input params:
 *          char *buffer: iec61937 data buffer address
            size_t bytes: iec61937 data buffer length
            size_t *raw_deficiency: raw data deficiency last time
            char *raw_buf: raw data buffer address
            size_t *raw_wt: raw data buffer write pointer
            size_t raw_max_bytes: raw data buffer length
            int *raw_size: raw audio valid size in IEC61937 packet
            int *offset : IEC61937 offset in 'buffer'
            int *got_format: it value is defined by iec61937 pc
 *
 * return value:
 *          0: if the audio format is suitable for dobly atmos(dd/ddp/truehd/mat)
               and this time valid data(<=min(raw_deficiency,input_bytes)) could store to the raw_buf.
               suppose this cases is sucess!
 *          1; if this time valid data(<=min(raw_size , (raw_max_bytes - *raw_wt))) is large than valid space of raw_buf.
                suppose this is fail
 */
int decode_IEC61937_to_raw_data(char *buffer
                                , size_t bytes
                                , char *raw_buf
                                , size_t *raw_wt
                                , size_t raw_max_bytes
                                , size_t *raw_deficiency
                                , int *raw_size
                                , int *offset
                                , int *got_format);
/*
 *@brief get current audio type from buffer data
 * input params:
 *          char *input_data: iec61937 data buffer address
            size_t input_bytes: iec61937 data buffer length
            size_t *raw_deficiency: raw data deficiency last time
            char *raw_buf: raw data buffer address
            size_t *raw_wt: raw data buffer write pointer
            size_t raw_max_bytes: raw data buffer length
 *
 * return value:
 *          val(>= 0): if *raw_deficiency is zero return zero;
               if this time valid data(<=min(raw_deficiency,input_bytes)) could store to the raw_buf.
               suppose these two cases are sucess!
 *          -1; if this time valid data(<=min(raw_deficiency,input_bytes)) is large than valid space of raw_buf.
                suppose this is fail
 */
int fill_in_the_remaining_data(char *input_data, size_t input_bytes, size_t *raw_deficiency, char *raw_buf, size_t *raw_wt, size_t raw_max_bytes);

#endif
