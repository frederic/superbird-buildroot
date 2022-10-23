/*****************************************************************************
 **
 **  Name:           app_hh_ble_audio_sink.c
 **
 **  Description:    
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include "app_hh_ble_audio_sink.h"

/*
 * Definitions
 */
#define mSBC_HEADER_SYNCWORD            0xAD

#define APP_HH_NBYTES_PER_FRAME         57
#define APP_HH_NSAMPLES_PER_FRAME_MSBC  120
#define APP_HH_MSBC_DEBUG               FALSE

/* Sequence codes of consecutive BLE audio reports maintain the */
/* following order:  08, 38, C8, F8 */
#define APP_HH_1ST_SEQ_CODE 0x08
#define APP_HH_2ND_SEQ_CODE 0x38
#define APP_HH_3RD_SEQ_CODE 0xC8
#define APP_HH_4TH_SEQ_CODE 0xF8

typedef struct
{
    UINT8 inFrm[APP_HH_NBYTES_PER_FRAME + 1];
    UINT8 outFrm[APP_HH_NSAMPLES_PER_FRAME_MSBC*2];
    UINT16 bytesInFrm;
    BOOLEAN DecoderInitDone;
    SBC_DEC_PARAMS decoder;
    SINT32 scratch_mem[240+256+128+8+8];
    /* static_mem for SBC_VECTOR_VX_ON_16BITS==FALSE case */
    SINT32 static_mem[DEC_VX_BUFFER_SIZE + (SBC_MAX_PACKET_LENGTH)];
    SINT16 sbc_status;
    UINT8 rpt_seq_num;
    UINT16 pkt_err_cnt;
    UINT16 pkt_cnt;
    BOOLEAN startFrm;
    BOOLEAN startSeq;
    UINT8 prev_seq_byte;
} tAPP_HH_BLE_AUDIO_SINK_CB;

/*
 * Globale variables
 */
static tAPP_HH_BLE_AUDIO_SINK_CB app_hh_ble_audio_sink_cb;

/*
 * Local functions
 */
static UINT16 app_hh_ble_msbc_dec_dump(SINT16 *s16PacketPtr, UINT16 u16NumberOfPCM);
static UINT16 app_hh_ble_msbc_dec_fill(UINT8 *u8PacketPtr, UINT16 u16NumberOfBytes);

/*******************************************************************************
 **
 ** Function         app_hh_ble_audio_sink_init
 **
 ** Description      Initialize HID BLE Audio Sink
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hh_ble_audio_sink_init(void)
{
    /* Reset BLE AUDIO Control Block */
    memset(&app_hh_ble_audio_sink_cb, 0, sizeof(app_hh_ble_audio_sink_cb));
    app_hh_ble_audio_sink_cb.prev_seq_byte = APP_HH_4TH_SEQ_CODE;
    return 0;
}
/*******************************************************************************
 **
 ** Function        app_hh_ble_audio_sink_handler
 **
 ** Description     BLE Audio sink Handler
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_hh_ble_audio_sink_handler(tAPP_HH_DEVICE *p_hh_dev,
        tBSA_HH_REPORT_DATA *p_report_data)
{
    UINT8* p_data;
    UINT8 report_id;
    UINT8 audio_cmd;

    p_data = p_report_data->data;
    STREAM_TO_UINT8(report_id, p_data);
    switch(report_id)
    {
    case APP_HH_AUDIO_START_STOP_REPORT_ID:
        STREAM_TO_UINT8(audio_cmd, p_data);
        switch(audio_cmd)
        {
        case APP_HH_AUDIO_RC_MIC_START_REQ:
            /* Send solicited start */
            if (app_hh_audio_mic_start(p_hh_dev, 1) == 0)
            {
                app_hh_ble_audio_sink_seq_init();
            }
            break;

        case APP_HH_AUDIO_RC_MIC_STOP_REQ:
            /* Send solicited stop */
            app_hh_audio_mic_stop(p_hh_dev, 1);
            APP_DEBUG1("BLE AudioSink Packet Received:%d NbSeqErr:%d",
                    app_hh_ble_audio_sink_cb.pkt_cnt, app_hh_ble_audio_sink_cb.pkt_err_cnt);
            break;

        default:
            APP_ERROR1("Unsupported audio command %d",audio_cmd);
            break;
        }
        break;

    case APP_HH_AUDIO_REPORT_ID:
        app_hh_ble_msbc_decode(p_hh_dev, p_report_data->data, p_report_data->length);
        break;

    default:
        APP_ERROR1("Unsupported report ID %d",report_id);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_hh_ble_msbc_dec_dump
 **
 ** Description      This function is called by the mSBC decoder to dump out the
 **                  decoded data
 **
 ** Returns          void
 **
 *******************************************************************************/
static UINT16 app_hh_ble_msbc_dec_dump(SINT16 *s16PacketPtr, UINT16 u16NumberOfPCM)
{
    UINT16 pcm_copy_len = 0;
    UINT8 *p = (UINT8*) s16PacketPtr;

    memcpy(app_hh_ble_audio_sink_cb.outFrm, p, APP_HH_NSAMPLES_PER_FRAME_MSBC*2);

    pcm_copy_len = u16NumberOfPCM;

#if defined(APP_HH_MSBC_DEBUG) && (APP_HH_MSBC_DEBUG == TRUE)
    APP_TRACE1("app_hh_ble_msbc_dec_dump (pcm16 = %d)\n", u16NumberOfPCM);
    APP_DUMP("data: ", p, 240);
#endif

    return(pcm_copy_len);
}

/*******************************************************************************
 **
 ** Function         app_hh_ble_msbc_dec_fill
 **
 ** Description      This function is called by the mSBC decoder to fill the
 **                  buffer with data to decode
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static UINT16 app_hh_ble_msbc_dec_fill(UINT8 *u8PacketPtr, UINT16 u16NumberOfBytes)
{
    UINT16 u16RetVal = 0;

    memcpy(u8PacketPtr, app_hh_ble_audio_sink_cb.inFrm, APP_HH_NBYTES_PER_FRAME);

#if defined(APP_HH_MSBC_DEBUG) && (APP_HH_MSBC_DEBUG == TRUE)
    APP_TRACE1("app_hh_ble_msbc_dec_fill - u16NumOfBytes = %d\n", u16NumberOfBytes);
    APP_DUMP("data: ", u8PacketPtr, 57);
#endif

    u16RetVal = APP_HH_NBYTES_PER_FRAME;

    return(u16RetVal);
}

/*******************************************************************************
 **
 ** Function         app_hh_ble_msbc_decode
 **
 ** Description      This function processes HID reports containing mSBC encoded
 **                  data.  It re-assembles the mSBC frame from 3 HID reports and
 **                  then decodes the mSBC frame and write the PCM output to a file.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_ble_msbc_decode(tAPP_HH_DEVICE *p_hh_dev, UINT8 *p_data, UINT16 length)
{
    ssize_t dummy;
#if !defined(APP_BLE2_BRCM_INCLUDED) || (APP_BLE2_BRCM_INCLUDED == FALSE)
    UINT8 copySize;
#endif

    /* Check report ID */
    if (p_data[0] != APP_HH_AUDIO_REPORT_ID)
    {
        return;
    }

    /* One more Voice ReportId received */
    app_hh_ble_audio_sink_cb.pkt_cnt++;

    if (app_hh_ble_audio_sink_cb.DecoderInitDone == FALSE)
    {
        /* Init decoder for mSBC - this section may not be needed */
        app_hh_ble_audio_sink_cb.decoder.samplingFreq = SBC_sf16000;
        app_hh_ble_audio_sink_cb.decoder.channelMode = SBC_MONO;
        app_hh_ble_audio_sink_cb.decoder.numOfSubBands = 8; /* 4 or 8 */
        app_hh_ble_audio_sink_cb.decoder.numOfChannels = 1;
        app_hh_ble_audio_sink_cb.decoder.numOfBlocks = 15; /* 4, 8, 12, 16 or 15 for mSBC */
        app_hh_ble_audio_sink_cb.decoder.allocationMethod = SBC_LOUDNESS; /* loudness or SNR */
        app_hh_ble_audio_sink_cb.decoder.bitPool = 26; /* 16*numOfSb for mono */

        /* Select mSBC mode */
        app_hh_ble_audio_sink_cb.decoder.decodeMode = SBC_DEC_MODE_MSBC;

        /* Init decoder memory and callbacks */
        app_hh_ble_audio_sink_cb.decoder.s32StaticMem = app_hh_ble_audio_sink_cb.static_mem;
        app_hh_ble_audio_sink_cb.decoder.s32ScartchMem = app_hh_ble_audio_sink_cb.scratch_mem;
        app_hh_ble_audio_sink_cb.decoder.EmptyBufferFP = app_hh_ble_msbc_dec_dump;
        app_hh_ble_audio_sink_cb.decoder.FillBufferFP = app_hh_ble_msbc_dec_fill;
    }

#if defined(APP_HH_MSBC_DEBUG) && (APP_HH_MSBC_DEBUG == TRUE)
    APP_DUMP("data: ", p_data, len);
#endif

#if defined(APP_BLE2_BRCM_INCLUDED) && (APP_BLE2_BRCM_INCLUDED == TRUE)
    /*
     * For BLE2 the Audio Report format is:
     * data[0]: 0xF7; data[1]: SequenceNumber; data[2]: mSBC SyncWord, etc
     */
    if((p_data[2] == mSBC_HEADER_SYNCWORD) &&
       (p_data[3] == 0x00) && (p_data[4] == 0x00))
    {
        UINT16 mscb_data_len;
        UINT8 *p_mscb_data;

        printf("Got BLE2 mSBC Packet length=%d\n", length);

        /* Check Sequence number */
        if (p_data[1] != (UINT8)(app_hh_ble_audio_sink_cb.prev_seq_byte + 1))
        {
            app_hh_ble_audio_sink_cb.pkt_err_cnt++;
            APP_ERROR1("Received SeqNumber:%d Expecting:%d NbSeqErrors:%d\n",
                    p_data[1], (UINT8)(app_hh_ble_audio_sink_cb.prev_seq_byte + 1),
                    app_hh_ble_audio_sink_cb.pkt_err_cnt);
        }
        /* Update Sequence Number */
        app_hh_ble_audio_sink_cb.prev_seq_byte = p_data[1];

        /* Skip the HID Report and Sequence number */
        mscb_data_len = length - 2;
        p_mscb_data = p_data + 2;

        while (mscb_data_len >= APP_HH_NBYTES_PER_FRAME)
        {
            /* Copy one mSBC frame in the mSBC Input Buffer */
            memcpy(&app_hh_ble_audio_sink_cb.inFrm[0], p_mscb_data, APP_HH_NBYTES_PER_FRAME);

            /* Initialize mSBC (if needed) */
            if(app_hh_ble_audio_sink_cb.DecoderInitDone == FALSE)
            {
                printf("Calling SBC_Decoder_Init \n");
                app_hh_ble_audio_sink_cb.sbc_status = SBC_Decoder_Init(&app_hh_ble_audio_sink_cb.decoder);
                if(app_hh_ble_audio_sink_cb.sbc_status != SBC_SUCCESS)
                {
                    APP_ERROR1("ERROR SBC Decoder Init failure:%d", app_hh_ble_audio_sink_cb.sbc_status);
                    exit(0);
                }
                app_hh_ble_audio_sink_cb.DecoderInitDone = TRUE;
            }

            /* Decoding mSBC Frames */
            printf("Calling SBC_Decoder \n");
            app_hh_ble_audio_sink_cb.sbc_status = SBC_Decoder(&app_hh_ble_audio_sink_cb.decoder);
            if(app_hh_ble_audio_sink_cb.sbc_status != SBC_SUCCESS)
            {
                APP_ERROR1("sbc decoder failure:%d", app_hh_ble_audio_sink_cb.sbc_status);
                exit(0);
            }

#ifdef PCM_ALSA
            app_alsa_playback_write(app_hh_ble_audio_sink_cb.outFrm,
                    APP_HH_NSAMPLES_PER_FRAME_MSBC * 2);
#else /* PCM_ALSA */
            /* write the decoded data to file */
            dummy = write(p_hh_dev->audio.fd, app_hh_ble_audio_sink_cb.outFrm,
                    APP_HH_NSAMPLES_PER_FRAME_MSBC * 2);
            (void)dummy;
#endif /* !PCM_ALSA */
            app_hh_ble_audio_sink_cb.bytesInFrm = 0;
            /* Jump to next mSBC frame */
            mscb_data_len -= APP_HH_NBYTES_PER_FRAME;
            p_mscb_data += APP_HH_NBYTES_PER_FRAME;
        }
    }

#else /* APP_BLE2_BRCM_INCLUDED == TRUE */

    if ((p_data[3] == mSBC_HEADER_SYNCWORD) && (p_data[4] == 0x00) && (p_data[5] == 0x00))
    {
        printf("Got start frame\n");
        app_hh_ble_audio_sink_cb.startFrm = TRUE;
        if((p_data[2] == APP_HH_1ST_SEQ_CODE)&&(app_hh_ble_audio_sink_cb.startSeq == FALSE))
        {
            printf("Got start sequence\n");
            app_hh_ble_audio_sink_cb.startSeq = TRUE;
        }

        /* Check sequence code for missing packets */
        /* Correct sequence: 08, 38, C8, F8 */
        if (app_hh_ble_audio_sink_cb.startSeq == TRUE)
        {
            switch(p_data[2])
            {
            case APP_HH_1ST_SEQ_CODE:
                if(app_hh_ble_audio_sink_cb.prev_seq_byte != APP_HH_4TH_SEQ_CODE)
                {
                    printf("ERROR, missed packet prior to 0x%x!!!\n", p_data[2]);
                    app_hh_ble_audio_sink_cb.pkt_err_cnt++;
                }
                break;
            case APP_HH_2ND_SEQ_CODE:
                if(app_hh_ble_audio_sink_cb.prev_seq_byte != APP_HH_1ST_SEQ_CODE)
                {
                    printf("ERROR, missed packet prior to 0x%x!!!\n", p_data[2]);
                    app_hh_ble_audio_sink_cb.pkt_err_cnt++;
                }
                break;
            case APP_HH_3RD_SEQ_CODE:
                if(app_hh_ble_audio_sink_cb.prev_seq_byte != APP_HH_2ND_SEQ_CODE)
                {
                    printf("ERROR, missed packet prior to 0x%x!!!\n", p_data[2]);
                    app_hh_ble_audio_sink_cb.pkt_err_cnt++;
                }
                break;
            case APP_HH_4TH_SEQ_CODE:
                if(app_hh_ble_audio_sink_cb.prev_seq_byte != APP_HH_3RD_SEQ_CODE)
                {
                    printf("ERROR, missed packet prior to 0x%x!!!\n", p_data[2]);
                    app_hh_ble_audio_sink_cb.pkt_err_cnt++;
                }
                break;
            default:
                printf("bad sequence code\n");
                app_hh_ble_audio_sink_cb.startSeq = FALSE;
                break;
            }

            /* save seq number */
            app_hh_ble_audio_sink_cb.prev_seq_byte = p_data[2];

        }
    }

    if ((app_hh_ble_audio_sink_cb.startFrm == TRUE) && (app_hh_ble_audio_sink_cb.startSeq == TRUE))
    {
        app_hh_ble_audio_sink_cb.rpt_seq_num++;
        if (app_hh_ble_audio_sink_cb.rpt_seq_num == 1)
        {
            copySize = 18;
            memcpy(&app_hh_ble_audio_sink_cb.inFrm[app_hh_ble_audio_sink_cb.bytesInFrm], &p_data[3], copySize);
            app_hh_ble_audio_sink_cb.bytesInFrm += copySize;
        }
        else if(app_hh_ble_audio_sink_cb.rpt_seq_num == 2)
        {
            copySize = 20;
            memcpy(&app_hh_ble_audio_sink_cb.inFrm[app_hh_ble_audio_sink_cb.bytesInFrm], &p_data[1], copySize);
            app_hh_ble_audio_sink_cb.bytesInFrm += copySize;
        }
        else if(app_hh_ble_audio_sink_cb.rpt_seq_num == 3)
        {
            copySize = 19;
            memcpy(&app_hh_ble_audio_sink_cb.inFrm[app_hh_ble_audio_sink_cb.bytesInFrm], &p_data[1], copySize);
            app_hh_ble_audio_sink_cb.bytesInFrm += copySize;
        }
        else
        {
            printf("ERROR: Too many reports in frame\n");
            app_hh_ble_audio_sink_cb.bytesInFrm = 0;
            app_hh_ble_audio_sink_cb.rpt_seq_num = 0;
            app_hh_ble_audio_sink_cb.startFrm = FALSE;
        }

        if (app_hh_ble_audio_sink_cb.bytesInFrm >= APP_HH_NBYTES_PER_FRAME)
        {
#if defined(APP_HH_MSBC_DEBUG) && (APP_HH_MSBC_DEBUG == TRUE)
            APP_DUMP("full frame of data: ", app_hh_ble_audio_sink_cb.inFrm, 58);
#endif

            if (app_hh_ble_audio_sink_cb.DecoderInitDone == FALSE)
            {
                printf("Calling SBC_Decoder_Init \n");
                if ((app_hh_ble_audio_sink_cb.sbc_status = SBC_Decoder_Init(&app_hh_ble_audio_sink_cb.decoder)) != SBC_SUCCESS)
                {
                    APP_ERROR1("ERROR SBC Decoder Init failure:%d", app_hh_ble_audio_sink_cb.sbc_status);
                    exit(0);
                }
                app_hh_ble_audio_sink_cb.DecoderInitDone = TRUE;
            }
            printf("Calling SBC_Decoder \n");
            if ((app_hh_ble_audio_sink_cb.sbc_status = SBC_Decoder(&app_hh_ble_audio_sink_cb.decoder)) != SBC_SUCCESS)
            {
                APP_ERROR1("ERROR sbc decoder failure:%d", app_hh_ble_audio_sink_cb.sbc_status);
                exit(0);
            }

#ifdef PCM_ALSA
            app_alsa_playback_write(app_hh_ble_audio_sink_cb.outFrm, APP_HH_NSAMPLES_PER_FRAME_MSBC*2);
#else
            dummy = write(p_hh_dev->audio.fd, app_hh_ble_audio_sink_cb.outFrm,
                    APP_HH_NSAMPLES_PER_FRAME_MSBC * 2);
            (void)dummy;
#endif
            app_hh_ble_audio_sink_cb.bytesInFrm = 0;
            app_hh_ble_audio_sink_cb.rpt_seq_num = 0;
            app_hh_ble_audio_sink_cb.startFrm = FALSE;
        }
    }
#endif /* APP_BLE2_BRCM_INCLUDED ==  FALSE */
}

/*******************************************************************************
 **
 ** Function         app_hh_ble_audio_sink_seq_init
 **
 ** Description      This function initializes Audio sink sequence number and
 **                  some error counters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_ble_audio_sink_seq_init(void)
{
#if defined(APP_BLE2_BRCM_INCLUDED) && (APP_BLE2_BRCM_INCLUDED == TRUE)
    app_hh_ble_audio_sink_cb.prev_seq_byte = 0xFF;
#else
    app_hh_ble_audio_sink_cb.prev_seq_byte = APP_HH_4TH_SEQ_CODE;
#endif
    app_hh_ble_audio_sink_cb.pkt_err_cnt = 0;   /* Reset Packet Error Counter */
    app_hh_ble_audio_sink_cb.pkt_cnt = 0;       /* Reset Packet Counter */
}
