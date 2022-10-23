/*****************************************************************************
 **
 **  Name:           app_hh_as.h
 **
 **  Description:    Bluetooth audio streaming through HID channel
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_HH_AS_H
#define APP_HH_AS_H

#include "sbc_encoder.h"


#define APP_HH_AS_CHANNEL_MODE        SBC_STEREO
#define APP_HH_AS_NUM_OF_SUBBANDS     SUB_BANDS_8
#define APP_HH_AS_NUM_OF_BLOCKS       SBC_BLOCK_3
#define APP_HH_AS_ALLOC_METHOD        SBC_LOUDNESS
#define APP_HH_AS_SAMPLING_FREQ       SBC_sf48000
#define APP_HH_AS_PCM_BLOCK_SIZE      512

#define APP_HH_AS_AUDIO_REPORT_ID             0xF9

#define APP_HH_AS_AUDIO_RTP_ID_OFFSET     0
#define APP_HH_AS_AUDIO_FORMAT_OFFSET     1
#define APP_HH_AS_AUDIO_SEQ_OFFSET        2
#define APP_HH_AS_AUDIO_NUM_SBC_OFFSET    3

#define APP_HH_AS_AUDIO_DATA       0x01
#define APP_HH_AS_REQ_AUDIO_START  0x04
#define APP_HH_AS_REQ_AUDIO_STOP   0x05

#define APP_HH_AS_AUDIO_START_CMD  0x0E
#define APP_HH_AS_AUDIO_STOP_CMD   0x0F

#if 1
/* in order to meet 26 bitpool size
 * PCM 256 bytes -> SBC 64 bytes */
#define APP_HH_AS_BITRATE             195
#else
/* in order to meet 35 bitpool size
 * PCM 256 bytes -> SBC 82 bytes */
#define APP_HH_AS_BITRATE             250
#endif

#define APP_HH_AS_TX_MTU_SIZE         700

/* Size of the audio buffer use to store the PCM to send to HID channel */
#define APP_HH_AS_MAX_AUDIO_BUF_MAX 10000


/* Play states (we use define instead of enum to allow Makefile to use them) */
#define APP_HH_AS_PLAYTYPE_TONE    0   /* Play tone */
#define APP_HH_AS_PLAYTYPE_FILE    1   /* Play file */
#define APP_HH_AS_PLAYTYPE_MIC     2   /* ALSA capture */

#define APP_HH_AS_MAX_HIDD         3   /* Max number of HID Devices for audio streaming */

/* local application control block (current status) */
typedef struct
{
    /* Information about the current tone generation */
    UINT8 sinus_index;
    UINT8 sinus_type;

    UINT16 tone_sample_freq;     /* Tone generation sampling frequency */
    volatile UINT8 play_state;
    UINT8 play_type;
    UINT8 seq_number;
    UINT8 num_of_scb_frame;
    short audio_buf[APP_HH_AS_MAX_AUDIO_BUF_MAX];   /* PCM audio buffer */
    UINT8 sbc_buf[APP_HH_AS_MAX_AUDIO_BUF_MAX];   /* SBC audio buffer */
    tBSA_HH_HANDLE handle[APP_HH_AS_MAX_HIDD];
    BOOLEAN EncoderInitDone;
    SBC_ENC_PARAMS encoder;

    long interval;          /* HID audio packet interval(us), 20,000us or 40,000us */
    UINT16 pcm_read_size;   /* pcm data length to read in every interval */
    UINT16 TxAaMtuSize;
    UINT32 timestamp;
    BOOLEAN use_desc;
    char file_name[1000];
} tAPP_HH_AS_AUDIO_CB;


/*******************************************************************************
 **
 ** Function         app_hh_as_read_tone
 **
 ** Description      read tone signal
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_as_read_tone(short * pOut, int nb_bytes, UINT8 sinus_type, UINT8 *p_sinus_index);


/*******************************************************************************
 **
 ** Function         app_hh_as_toggle_tone
 **
 ** Description      Toggle the tone type
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_hh_as_toggle_tone(void);


/*******************************************************************************
 **
 ** Function         app_hh_as_configure_audio_interval
 **
 ** Description      configure interval of HID packet including audio data
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_hh_as_configure_audio_interval();




/*******************************************************************************
 **
 ** Function         app_hh_as_init
 **
 ** Description      Init HID Audio streaming
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_init(void);

/*******************************************************************************
 **
 ** Function         app_hh_as_select_sound
 **
 ** Description      Select sound source
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_select_sound(void);

/*******************************************************************************
 **
 ** Function         app_hh_as_start_streaming
 **
 ** Description      Start streaming to RCU
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_start_streaming(UINT8 handle);

/*******************************************************************************
 **
 ** Function         app_hh_as_stop_streaming
 **
 ** Description      Stop streaming to RCU
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_stop_streaming(UINT8 handle);

/*******************************************************************************
 **
 ** Function         app_hh_as_check_connection
 **
 ** Description      Check HID connection is opened
 **
 ** Returns          true if opened, fail is not opened
 **
 *******************************************************************************/
BOOLEAN app_hh_as_check_connection(tBSA_HH_HANDLE handle);


/*******************************************************************************
 **
 ** Function        app_hh_send_audio_int
 **
 ** Description     Example of HH Send audio report on Interrupt Channel
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_as_send_audio(tBSA_HH_HANDLE handle, int pcm_length, SINT16 *pcm_data);

/*******************************************************************************
 **
 ** Function         app_hh_as_enc_init
 **
 ** Description      This function initialize encoder.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_as_enc_init(void);

/*******************************************************************************
 **
 ** Function         app_hh_as_encode_audio_data
 **
 ** Description      This function encodes audio data to SBC data.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_as_encode_audio_data(SINT16 *p_pcm_data, UINT16 pcm_len);

/*******************************************************************************
 **
 ** Function         app_hh_as_audio_cmd_handler
 **
 ** Description
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_as_audio_cmd_handler(tBSA_HH_REPORT *p_report);

/*******************************************************************************
 **
 ** Function        app_hh_as_notif_start_streaming
 **
 ** Description     Send notification to start audio streaming
 **
 ** Parameters      handle
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_as_notif_start_streaming(tBSA_HH_HANDLE handle);

/*******************************************************************************
 **
 ** Function        app_hh_as_notif_stop_streaming
 **
 ** Description     Send notification to stop audio streaming
 **
 ** Parameters      handle
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_as_notif_stop_streaming(tBSA_HH_HANDLE handle);

#endif
