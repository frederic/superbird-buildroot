/*****************************************************************************
 **
 **  Name:           app_hd_as.h
 **
 **  Description:    Bluetooth HID Device application
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef __APP_HD_AS_H__
#define __APP_HD_AS_H__

#include "sbc_decoder.h"


#define APP_HD_AS_CHANNEL_MODE        SBC_STEREO
#define APP_HD_AS_NUM_OF_SUBBANDS     SBC_SUB_BANDS_8
#define APP_HD_AS_NUM_OF_BLOCKS       SBC_BLOCK_3
#define APP_HD_AS_ALLOC_METHOD        SBC_LOUDNESS
#define APP_HD_AS_SAMPLING_FREQ       SBC_sf48000

#define APP_HD_AS_SOUND_FILE      "test_hd_as"

#define APP_HD_AS_AUDIO_REPORT_ID             0xF9

#define APP_HD_AS_AUDIO_DATA    0x01
#define APP_HD_AS_AUDIO_START_CMD  0x0E
#define APP_HD_AS_AUDIO_STOP_CMD   0x0F

#define APP_HD_AS_AUDIO_RTP_ID_OFFSET     0
#define APP_HD_AS_AUDIO_FORMAT_OFFSET     1
#define APP_HD_AS_AUDIO_SEQ_OFFSET        2
#define APP_HD_AS_AUDIO_NUM_SBC_OFFSET    3


#if 1
#define APP_HD_AS_NBYTES_PER_FRAME 64
#else
#define APP_HD_AS_NBYTES_PER_FRAME 82
#endif


#define APP_HD_AS_NSAMPLES_PER_FRAME_SBC 256
#define APP_HD_AS_MSBC_DEBUG TRUE


typedef struct
{
    UINT8 inFrm[APP_HD_AS_NBYTES_PER_FRAME + 1];
    UINT8 outFrm[APP_HD_AS_NSAMPLES_PER_FRAME_SBC*2];
    UINT16 bytesInFrm;
    BOOLEAN DecoderInitDone;
    SBC_DEC_PARAMS decoder;
    SINT32 scratch_mem[240+256+128+8+8];
    /* static_mem for SBC_VECTOR_VX_ON_16BITS==FALSE case */
    SINT32 static_mem[DEC_VX_BUFFER_SIZE + (SBC_MAX_PACKET_LENGTH)];
    SINT16 sbc_status;
    UINT8 rpt_seq_num;
    UINT16 pkt_err_cnt;
    BOOLEAN startFrm;
    BOOLEAN startSeq;
    UINT8 prev_seq_byte;
    int fd;
    UINT16 audio_size;
} tAPP_HD_AS_AUDIO_CB;


/*******************************************************************************
 **
 ** Function         app_hd_as_audio_handler
 **
 ** Description      handle audio data from HID host
 **
 ** Parameters       p_report_data : HID report
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hd_as_audio_handler(tBSA_HD_DATA_MSG *p_report_data);


/*******************************************************************************
 **
 ** Function         app_hd_as_decode_audio
 **
 ** Description      This function decode HID reports having SBC data
 **
 ** Parameters       p_data : pointer of HID report
 **                  len : length of HID report
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hd_as_decode_audio(UINT8 *p_data, UINT16 len);

/*******************************************************************************
 **
 ** Function         app_hd_as_close_wave_file
 **
 ** Description     proper header and close the wave file
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_as_close_wave_file(void);

/*******************************************************************************
 **
 ** Function        app_hd_send_start_streaming
 **
 ** Description     Send start streaming command
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_start_streaming(void);

/*******************************************************************************
 **
 ** Function        app_hd_send_stop_streaming
 **
 ** Description     Send stop streaming command
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_stop_streaming(void);

#endif
