/*****************************************************************************
 **
 **  Name:           app_hd_as.c
 **
 **  Description:    Bluetooth HID Device application
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "app_hd.h"
#include "app_hd_as.h"
#include "app_utils.h"
#include "app_wav.h"

static tAPP_HD_AS_AUDIO_CB app_hd_as_audio_cb;


static void app_hd_as_create_wave_file(void);

#ifndef APP_HD_AS_DEBUG
#define APP_HD_AS_DEBUG FLASE
#endif

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
void app_hd_as_audio_handler(tBSA_HD_DATA_MSG *p_report_data)
{
    UINT8* p_data;
    UINT8 report_id;

    p_data = p_report_data->data;
    STREAM_TO_UINT8(report_id, p_data);
    APP_INFO1("app_hd_audio_handler report id:0x%x", report_id);

    switch(report_id)
    {
      case APP_HD_AS_AUDIO_REPORT_ID:
         app_hd_as_decode_audio(p_report_data->data, p_report_data->len);
         break;

      default:
         APP_ERROR1("Unsupported report ID %d",report_id);
         break;
    }
}


/*******************************************************************************
 **
 ** Function         app_hh_sbc_dec_dump
 **
 ** Description      This function is called by the SBC decoder to dump out the
 **                  decoded data
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static UINT16 app_hd_as_sbc_dec_dump(SINT16 *s16PacketPtr, UINT16 u16NumberOfPCM)
{
    UINT16 pcm_copy_len = 0;
    UINT8 *p = (UINT8*) s16PacketPtr;
    ssize_t write_len;

    pcm_copy_len = u16NumberOfPCM * sizeof(SINT16);

#if APP_HD_AS_DEBUG
    APP_DEBUG1("app_hd_as_sbc_dec_dump (pcm16 length:%d), length:%d",
        u16NumberOfPCM, pcm_copy_len);
#endif

    memcpy(&app_hd_as_audio_cb.outFrm, p, pcm_copy_len);

#if APP_HD_AS_DEBUG
    APP_DUMP("PCM data: ", p, pcm_copy_len);
#endif
    if (app_hd_as_audio_cb.fd != -1)
    {
        write_len = write(app_hd_as_audio_cb.fd, p, pcm_copy_len);
        if (write_len != pcm_copy_len)
        {
            APP_ERROR0("write error!");
        }
    }
    else
    {
        app_hd_as_create_wave_file();
    }

    return(pcm_copy_len);
}

/*******************************************************************************
 **
 ** Function         app_hd_as_sbc_dec_fill
 **
 ** Description      This function is called by the SBC decoder to fill the
 **                  buffer with data to decode
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static UINT16 app_hd_as_sbc_dec_fill(UINT8 *u8PacketPtr, UINT16 u16NumberOfBytes)
{
    UINT16 u16RetVal = 0;

    memcpy(u8PacketPtr, &app_hd_as_audio_cb.inFrm, APP_HD_AS_NBYTES_PER_FRAME);

#if APP_HD_AS_DEBUG
    printf("app_hd_as_sbc_dec_fill  u16NumOfBytes:%d, audio_size:%d\n",
            u16NumberOfBytes, app_hd_as_audio_cb.audio_size);
    APP_DUMP("data: ", u8PacketPtr, APP_HD_AS_NBYTES_PER_FRAME);
#endif

    u16RetVal = APP_HD_AS_NBYTES_PER_FRAME;

    return(u16RetVal);
}

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
void app_hd_as_decode_audio(UINT8 *p_data, UINT16 len)
{
    UINT8 index =0;
    UINT8 *p_curr_data;
    UINT16 length = 0;
    p_curr_data = p_data;
    SBC_DEC_PARAMS *dcd;

    length = len - 4;
    dcd = &app_hd_as_audio_cb.decoder;

    /* Check report ID */
    if (p_data[0] == APP_HD_AS_AUDIO_REPORT_ID)
    {
        if (p_data[1] == APP_HD_AS_AUDIO_DATA)
        {
            APP_DEBUG1("sequence num:%d, number of SBC:%d\n", p_data[2], p_data[3]);

            p_curr_data += 4; /* skip report ID, seq num, num of SBC */

            APP_DEBUG1("HID audio data length:%d\n", length);
            for (index = 0 ; length > 0 ; index++)
            {
                if (length >= APP_HD_AS_NBYTES_PER_FRAME)
                {
                    memset(&app_hd_as_audio_cb.inFrm, 0, APP_HD_AS_NBYTES_PER_FRAME);
                    memcpy(&app_hd_as_audio_cb.inFrm, p_curr_data, APP_HD_AS_NBYTES_PER_FRAME);
                    if (app_hd_as_audio_cb.DecoderInitDone != TRUE)
                    {
                        /* Init decoder for mSBC - this section may not be needed */
                        dcd->samplingFreq = APP_HD_AS_SAMPLING_FREQ;
                        dcd->channelMode = APP_HD_AS_CHANNEL_MODE;
                        dcd->numOfSubBands = APP_HD_AS_NUM_OF_SUBBANDS; /* 4 or 8 */
                        dcd->numOfBlocks = APP_HD_AS_NUM_OF_BLOCKS; /* 4, 8, 12, 16 or 15 for mSBC */
                        dcd->allocationMethod = APP_HD_AS_ALLOC_METHOD; /* loudness or SNR */

                        /* Init decoder memory and callbacks */
                        dcd->s32StaticMem = app_hd_as_audio_cb.static_mem;
                        dcd->s32ScartchMem = app_hd_as_audio_cb.scratch_mem;
                        dcd->EmptyBufferFP = app_hd_as_sbc_dec_dump;
                        dcd->FillBufferFP = app_hd_as_sbc_dec_fill;

                        printf("Calling SBC_Decoder_Init \n");
                        if((app_hd_as_audio_cb.sbc_status = SBC_Decoder_Init(&app_hd_as_audio_cb.decoder)) != SBC_SUCCESS)
                        {
                            APP_ERROR1("ERROR SBC Decoder Init failure:%d", app_hd_as_audio_cb.sbc_status);
                            return;
                        }
                        app_hd_as_create_wave_file();
                        app_hd_as_audio_cb.DecoderInitDone = TRUE;
                    }
                    printf("Calling SBC_Decoder \n");
                    if((app_hd_as_audio_cb.sbc_status = SBC_Decoder(&app_hd_as_audio_cb.decoder)) != SBC_SUCCESS)
                    {
                        APP_ERROR1("ERROR sbc decoder failure:%d", app_hd_as_audio_cb.sbc_status);
                    }
                    p_curr_data += APP_HD_AS_NBYTES_PER_FRAME;
                    length -= APP_HD_AS_NBYTES_PER_FRAME;
                }
                else if ((length > 0) && (length < APP_HD_AS_NBYTES_PER_FRAME))
                {
                    APP_ERROR1("wrong SBC length:%d", length);
                    length = 0;
                }
                else
                {

                }
            }
        }
    }
}


/*******************************************************************************
 **
 ** Function         app_hd_as_create_wave_file
 **
 ** Description      create a new wave file
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_hd_as_create_wave_file(void)
{
    int file_index = 0;
    int fd;
    char filename[200];

    /* If a wav file is currently open => close it */
    if (app_hd_as_audio_cb.fd != -1)
    {
        app_hd_as_close_wave_file();
    }

    do
    {
        snprintf(filename, sizeof(filename), "%s-%d.wav", APP_HD_AS_SOUND_FILE, file_index);
        filename[sizeof(filename)-1] = '\0';
        fd = app_wav_create_file(filename, O_EXCL);
        file_index++;
    } while (fd < 0);

    app_hd_as_audio_cb.fd = fd;
}

/*******************************************************************************
 **
 ** Function         app_hd_as_close_wave_file
 **
 ** Description     proper header and close the wave file
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_as_close_wave_file(void)
{
    printf("app_hd_as_close_wave_file\n");
    app_hd_as_audio_cb.fd = -1;
    return 0;
}

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
int app_hd_send_start_streaming(void)
{
    int status;
    tBSA_HD_SEND hd_send_param;

    APP_INFO0("app_hd_send_start_streaming");

    BSA_HdSendInit(&hd_send_param);
    hd_send_param.key_type = BSA_HD_CUSTOMER_DATA;
    hd_send_param.param.customer.data_len = 2;

    hd_send_param.param.customer.data[0] = APP_HD_AS_AUDIO_REPORT_ID;
    hd_send_param.param.customer.data[1] = APP_HD_AS_AUDIO_START_CMD;

    status = BSA_HdSend(&hd_send_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HdSend fail status:%d", status);
        return -1;
    }
    return status;
}

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
int app_hd_send_stop_streaming(void)
{
    int status;
    tBSA_HD_SEND hd_send_param;

    APP_INFO0("app_hd_send_stop_streaming");

    BSA_HdSendInit(&hd_send_param);
    hd_send_param.key_type = BSA_HD_CUSTOMER_DATA;
    hd_send_param.param.customer.data_len = 2;

    hd_send_param.param.customer.data[0] = APP_HD_AS_AUDIO_REPORT_ID;
    hd_send_param.param.customer.data[1] = APP_HD_AS_AUDIO_STOP_CMD;

    status = BSA_HdSend(&hd_send_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HdSend fail status:%d", status);
        return -1;
    }
    return status;
}
