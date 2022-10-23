/*****************************************************************************
 **
 **  Name:           app_hh_audio.h
 **
 **  Description:    APP HID Audio definitions
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#ifndef _APP_HH_AUDIO_H_
#define _APP_HH_AUDIO_H_

#ifdef PCM_ALSA
#include "app_alsa.h"
#endif

/* SBC Header Syncword (first byte of every SBC frame) */
#define SBC_HEADER_SYNCWORD                 0x9C

/* Audio Report ID (control & data) */
#ifndef APP_HH_AUDIO_REPORT_ID
#define APP_HH_AUDIO_REPORT_ID              0xF7
#endif

#ifndef APP_HH_AUDIO_START_STOP_REPORT_ID
#define APP_HH_AUDIO_START_STOP_REPORT_ID   0xF8
#endif

/* audio format offset */
#define APP_HH_AUDIO_FORMAT_OFFSET          1

#define APP_HH_AUDIO_DATA                   0x01

#define APP_HH_AUDIO_MAX_REPORT_SIZE        16

/* From Host to HID device */
#define APP_HH_AUDIO_MIC_START              0x02
#define APP_HH_AUDIO_MIC_STOP               0x03
#define APP_HH_AUDIO_SPK_START              0x04
#define APP_HH_AUDIO_SPK_STOP               0x05
#define APP_HH_AUDIO_CODEC_READ_REQ         0x06
#define APP_HH_AUDIO_CODEC_WRITE_REQ        0x08
#define APP_HH_AUDIO_CALL_START             0x10
#define APP_HH_AUDIO_CALL_STOP              0x11
#define APP_HH_AUDIO_MODE_READ_REQ          0x0A

/* From HID device to Host */
#define APP_HH_AUDIO_RC_MIC_START_REQ       0x0C
#define APP_HH_AUDIO_RC_MIC_STOP_REQ        0x0D
#define APP_HH_AUDIO_RC_SPK_START           0x0E
#define APP_HH_AUDIO_RC_SPK_STOP            0x0F
#define APP_HH_AUDIO_CODEC_READ_ACK         0x07
#define APP_HH_AUDIO_CODEC_WRITE_ACK        0x09
#define APP_HH_AUDIO_MODE_READ_ACK          0x0B
#define APP_HH_AUDIO_RC_CALL_START          0x12
#define APP_HH_AUDIO_RC_CALL_STOP           0x13

#define APP_HH_AUDIO_SAMP_FREQ_ID           0x00
#define APP_HH_AUDIO_SAMP_FORMAT_ID         0x01
#define APP_HH_AUDIO_PGA_GAIN_ID            0x02
#define APP_HH_AUDIO_BITS_PER_SAMP_ID       0x03
#define APP_HH_AUDIO_HP_FILTER_SET_ID       0x04

#define APP_HH_AUDIO_MINIMAL_CODEC_CONFIG   ( (1 << APP_HH_AUDIO_SAMP_FREQ_ID)| \
                                              (1 << APP_HH_AUDIO_BITS_PER_SAMP_ID) )

#define APP_HH_AUDIO_SAMP_FREQ_8K           0x20
#define APP_HH_AUDIO_SAMP_FREQ_16K          0x40
#define APP_HH_AUDIO_SAMP_FREQ_22K5         0x5A
#define APP_HH_AUDIO_SAMP_FREQ_32K          0x80
#define APP_HH_AUDIO_SAMP_FREQ_48K          0xC0

typedef struct tAPP_HH_AUDIO_DATA_HDR
{
    UINT16 seq_num;
    UINT16 timestamp;
    UINT16 bufferState;
    UINT16 reserved;
    UINT16 datacnt;
} tAPP_HH_AUDIO_DATA_HDR;

#endif /* _APP_HH_AUDIO_H_ */
