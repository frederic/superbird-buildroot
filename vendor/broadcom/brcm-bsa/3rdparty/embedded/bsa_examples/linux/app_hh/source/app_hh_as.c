/*****************************************************************************
 **
 **  Name:           app_hh_as.c
 **
 **  Description:    Bluetooth audio streaming through HID channel
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

#include "bsa_api.h"

#include "gki_int.h"

#include "bsa_trace.h"
#include "app_xml_param.h"
#include "app_utils.h"
#include "app_thread.h"
#include "app_mutex.h"
#include "app_xml_utils.h"
#include "app_dm.h"
#include "app_hh.h"
#include "app_hh_as.h"
#include "sbc_encoder.h"

/*
 * Defines
 */

#ifndef APP_HH_AS_DEBUG
#define APP_HH_AS_DEBUG FALSE
#endif

#ifndef APP_HH_WAV_FILE_NAME
#define APP_HH_WAV_FILE_NAME    "hh_stream.wav"
#endif


static const short sinwaves[2][64] = {{
         0,    488,    957,   1389,   1768,  2079,  2310,  2452,
      2500,   2452,   2310,   2079,   1768,  1389,   957,   488,
         0,   -488,   -957,  -1389,  -1768, -2079, -2310, -2452,
     -2500,  -2452,  -2310,  -2079,  -1768, -1389,  -957,  -488,
         0,    488,    957,   1389,   1768,  2079,  2310,  2452,
      2500,   2452,   2310,   2079,   1768,  1389,   957,   488,
         0,   -488,   -957,  -1389,  -1768, -2079, -2310, -2452,
     -2500,  -2452,  -2310,  -2079,  -1768, -1389,  -957,  -488},{

         0,    244,    488,    722,    957,  1173,  1389,   1578,
      1768,   1923,   2079,   2194,   2310,  2381,  2452,   2476,
      2500,   2476,   2452,   2381,   2310,  2194,  2079,   1923,
      1768,   1578,   1389,   1173,    957,   722,   488,    244,
         0,   -244,   -488,   -722,   -957, -1173, -1389,  -1578,
     -1768,  -1923,  -2079,  -2194,  -2310, -2381, -2452,  -2476,
     -2500,  -2476,  -2452,  -2381,  -2310, -2194, -2079,  -1923,
     -1768,  -1578,  -1389,  -1173,   -957,  -722,  -488,   -244 }};


/*
 * Global variables
 */
tAPP_THREAD app_hh_as_tx_thread_struct;

/* local application control block (current status) */
tAPP_HH_AS_AUDIO_CB app_hh_as_cb;

/* Play states */
enum
{
    APP_HH_AS_PLAY_STOPPED,
    APP_HH_AS_PLAY_STARTED,
    APP_HH_AS_PLAY_PAUSED,
    APP_HH_AS_PLAY_STOPPING
};

/*
 * Local definitions & functions
 */


#define APP_HH_AS_SBC_SYNC_WORD       0x9C
#define APP_HH_AS_SBC_CRC_IDX         3
#define APP_HH_AS_SBC_USE_MASK        0x64
#define APP_HH_AS_SBC_SYNC_MASK       0x10
#define APP_HH_AS_SBC_CIDX            0
#define APP_HH_AS_SBC_LIDX            1
#define APP_HH_AS_SBC_CH_M_BITS       0xC /* channel mode bits: 0: mono; 1 ch */
#define APP_HH_AS_SBC_SUBBAND_BIT     0x1 /* num of subband bit: 0:4; 1: 8 */

#define APP_HH_AS_SBC_GET_IDX(sc) (((sc) & 0x3) + (((sc) & 0x30) >> 2))

typedef struct
{
    UINT8   use;
    UINT8   idx;
} tAPP_HH_AS_SBC_FR_CB;

typedef struct
{
    tAPP_HH_AS_SBC_FR_CB  fr[2];
    UINT8           index;
    UINT8           base;
} tAPP_HH_AS_SBC_DS_CB;

static tAPP_HH_AS_SBC_DS_CB app_hh_ad_sbc_ds_cb;

/******************************************************************************
**
** Function         app_hh_as_SbcChkFrInit
**
** Description      check if need to init the descramble control block.
**
** Returns          nothing.
******************************************************************************/
static void app_hh_as_SbcChkFrInit(UINT8 *p_pkt)
{
    UINT8   fmt;
    UINT8   num_chnl = 1;
    UINT8   num_subband = 4;

    if((p_pkt[0] & APP_HH_AS_SBC_SYNC_MASK) == 0)
    {
        app_hh_as_cb.use_desc = TRUE;
        fmt = p_pkt[1];
        p_pkt[0] |= APP_HH_AS_SBC_SYNC_MASK;
        memset(&app_hh_ad_sbc_ds_cb, 0, sizeof(tAPP_HH_AS_SBC_DS_CB));
        if(fmt & APP_HH_AS_SBC_CH_M_BITS)
            num_chnl = 2;
        if(fmt & APP_HH_AS_SBC_SUBBAND_BIT)
            num_subband = 8;
        app_hh_ad_sbc_ds_cb.base = 6 + num_chnl*num_subband/2;
    }
}

/******************************************************************************
**
** Function         app_hh_as_SbcDescramble
**
** Description      descramble the packet.
**
** Returns          nothing.
******************************************************************************/
static void app_hh_as_SbcDescramble(UINT8 *p_pkt, UINT16 len)
{
    tAPP_HH_AS_SBC_FR_CB *p_cur, *p_last;
    UINT32   idx, tmp, tmp2;

    if(app_hh_as_cb.use_desc)
    {
        /* c2l */
        p_last  = &app_hh_ad_sbc_ds_cb.fr[APP_HH_AS_SBC_LIDX];
        p_cur   = &app_hh_ad_sbc_ds_cb.fr[APP_HH_AS_SBC_CIDX];
        p_last->idx = p_cur->idx;
        p_last->use = p_cur->use;
        /* getc */
        p_cur->use = p_pkt[APP_HH_AS_SBC_CRC_IDX] & APP_HH_AS_SBC_USE_MASK;
        p_cur->idx = APP_HH_AS_SBC_GET_IDX(p_pkt[APP_HH_AS_SBC_CRC_IDX]);
        app_hh_ad_sbc_ds_cb.index = (p_cur->use)?APP_HH_AS_SBC_CIDX:APP_HH_AS_SBC_LIDX;

        /* descramble */
        idx = app_hh_ad_sbc_ds_cb.fr[app_hh_ad_sbc_ds_cb.index].idx;
        if(idx > 0)
        {
            p_pkt = &p_pkt[app_hh_ad_sbc_ds_cb.base];
            if((idx&1) && (len > (app_hh_ad_sbc_ds_cb.base+(idx<<1))))
            {
                tmp2        = (idx<<1);
                tmp         = p_pkt[idx];
                p_pkt[idx]  = p_pkt[tmp2];
                p_pkt[tmp2]  = tmp;
            }
            else
            {
                tmp2        = p_pkt[idx];
                tmp         = (tmp2>>3)+(tmp2<<5);
                p_pkt[idx]  = (UINT8)tmp;
            }
        }
    }
}

/*******************************************************************************
 **
 ** Function         app_hh_as_add_handle
 **
 ** Description      Add handle to streaming list
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
static int app_hh_as_add_handle(UINT8 handle)
{
    UINT8 hid_index = 0;
    APP_DEBUG1("app_hh_as_add_handle %d", handle);

    for (hid_index = 0; hid_index < APP_HH_AS_MAX_HIDD ; hid_index++)
    {
        if (app_hh_as_cb.handle[hid_index] == BSA_HH_INVALID_HANDLE)
        {
            app_hh_as_cb.handle[hid_index] = handle;
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_hh_as_del_handle
 **
 ** Description      Delete handle from streaming list
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
static int app_hh_as_del_handle(UINT8 handle)
{
    UINT8 hid_index = 0;
    APP_DEBUG1("app_hh_as_del_handle %d", handle);

    for (hid_index = 0; hid_index < APP_HH_AS_MAX_HIDD ; hid_index++)
    {
        if (app_hh_as_cb.handle[hid_index] == handle)
        {
            app_hh_as_cb.handle[hid_index] = BSA_HH_INVALID_HANDLE;
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_hh_as_read_tone
 **
 ** Description      read tone signal
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_as_read_tone(short * pOut, int nb_bytes, UINT8 sinus_type, UINT8 *p_sinus_index)
{
    int index;
    UINT8 sinus_index = *p_sinus_index;

    /* Generate a standard PCM stereo interlaced sinewave */
    for (index = 0; index < (nb_bytes / 4); index++)
    {
        pOut[index * 2] = sinwaves[sinus_type][sinus_index % 64];
        pOut[index * 2 + 1] = sinwaves[sinus_type][sinus_index % 64];
        sinus_index++;
    }
    *p_sinus_index = sinus_index;
}


/*******************************************************************************
 **
 ** Function         app_hh_as_toggle_tone
 **
 ** Description      Toggle the tone type
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_hh_as_toggle_tone(void)
{
    if (app_hh_as_cb.sinus_type == 0)
        app_hh_as_cb.sinus_type = 1;
    else
        app_hh_as_cb.sinus_type = 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_as_configure_audio_interval
 **
 ** Description      configure interval of HID packet including audio data
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_hh_as_configure_audio_interval()
{
    UINT32 byte_per_sec;
    float pcm_size;
    UINT16 packet_interval;

    packet_interval = app_get_choice("interval(20ms ~ 100ms)");
    if ((packet_interval < 20) || (packet_interval > 100))
    {
        APP_ERROR1("invalid interval:%d", packet_interval);
        packet_interval = 40;
    }

    byte_per_sec = (16/8) * 2 * 48000;
    APP_DEBUG1("Byte per second :%d", byte_per_sec);
    pcm_size = (float)byte_per_sec * ((float)packet_interval/1000);
    app_hh_as_cb.pcm_read_size = pcm_size;
    app_hh_as_cb.pcm_read_size = APP_HH_AS_PCM_BLOCK_SIZE * (app_hh_as_cb.pcm_read_size / APP_HH_AS_PCM_BLOCK_SIZE);
    APP_DEBUG1("packet size :%f :%d", pcm_size, app_hh_as_cb.pcm_read_size);

    app_hh_as_cb.interval = (app_hh_as_cb.pcm_read_size * 1000000) / byte_per_sec;
    APP_DEBUG1("Calculated HID packet interval: %d us", app_hh_as_cb.interval);
}

int app_hh_as_play_mic(void)
{
#ifdef PCM_ALSA
    int status;
    tAPP_ALSA_CAPTURE_OPEN asla_capture;

    /* open and configure ALSA capture device */
    app_alsa_capture_open_init(&asla_capture);

    asla_capture.access = APP_ALSA_PCM_ACCESS_RW_INTERLEAVED;
    asla_capture.blocking = FALSE; /* Non Blocking */
    asla_capture.format = APP_ALSA_PCM_FORMAT_S16_LE; /* Signed 16 bits Little Endian*/
    asla_capture.sample_rate = 48000; /* 48KHz */
    asla_capture.stereo = TRUE; /* Stereo */
    status = app_alsa_capture_open(&asla_capture);
    if (status < 0)
    {
        APP_ERROR1("app_alsa_capture_open fail:%d", status);
        return -1;
    }
    APP_DEBUG0("ALSA driver opened in capture mode (Microphone/LineIn)");

    return 0;
#else
    APP_ERROR0("Microphone input not supported on this target (see PCM_ALSA in makefile)");
    return -1;
#endif
}
/*******************************************************************************
 **
 ** Function           app_hh_as_tx_thread
 **
 ** Description        Thread in charge of feeding PCM data to HID channel
 **
 ** Returns            nothing
 **
 *******************************************************************************/
static void app_hh_as_tx_thread(void)
{
    int nb_bytes = 0;
    int file_desc = -1;
    UINT8 hid_index = 0;
    tAPP_WAV_FILE_FORMAT wav_format;
    struct timespec tx_time;

    APP_DEBUG0("app_hh_as_tx_thread:Starting thread");
    while(1)
    {
        if (app_hh_as_cb.play_state == APP_HH_AS_PLAY_STOPPED)
        {
            GKI_delay(1000); /* 1s delay */
        }
        else
        {
            if (app_hh_as_cb.play_type == APP_HH_AS_PLAYTYPE_TONE)
            {
                APP_DEBUG0("Playing tone");
            }
            else if (app_hh_as_cb.play_type == APP_HH_AS_PLAYTYPE_FILE)
            {
                strncpy(app_hh_as_cb.file_name, APP_HH_WAV_FILE_NAME,
                        sizeof(APP_HH_WAV_FILE_NAME));
                APP_DEBUG1("Playing file %s", app_hh_as_cb.file_name);

                file_desc = app_wav_open_file(app_hh_as_cb.file_name, &wav_format);
                if (file_desc < 0)
                {
                    APP_ERROR1("open(%s) failed(%d) -> Stop playing",
                        app_hh_as_cb.file_name, errno);
                    app_hh_as_cb.play_state = APP_HH_AS_PLAY_STOPPED;
                }
                APP_DEBUG1("wave format sample_rate:%d, num_channel:%d, bit_per_sample:%d",
                    wav_format.sample_rate, wav_format.nb_channels,
                    wav_format.bits_per_sample);
            }

            do
            {
                switch (app_hh_as_cb.play_type)
                {
                case APP_HH_AS_PLAYTYPE_TONE:
                    nb_bytes = app_hh_as_cb.pcm_read_size;
                    app_hh_as_read_tone(app_hh_as_cb.audio_buf,
                                    app_hh_as_cb.pcm_read_size,
                                    app_hh_as_cb.sinus_type,
                                    &app_hh_as_cb.sinus_index);
                    break;

                case APP_HH_AS_PLAYTYPE_FILE:
                    nb_bytes = app_wav_read_data(file_desc, &wav_format,
                                             (char *)app_hh_as_cb.audio_buf,
                                             app_hh_as_cb.pcm_read_size);
                    if (app_hh_as_cb.pcm_read_size != nb_bytes)
                    {
                        APP_ERROR1("read wav file try:%d result:%d",
                            app_hh_as_cb.pcm_read_size, nb_bytes);
                    }
                    break;

                case APP_HH_AS_PLAYTYPE_MIC:
#ifdef PCM_ALSA
                    nb_bytes = app_alsa_capture_read((char *)app_hh_as_cb.audio_buf,
                        app_hh_as_cb.pcm_read_size);
                    APP_DEBUG1("app_alsa_capture_read :%d", nb_bytes);
                    if (nb_bytes < 0)
                    {
                        APP_ERROR1("app_alsa_capture_read fail:%d", nb_bytes);
                    }
#else
                    nb_bytes = 0;
#endif
                    break;

                default:
                    APP_ERROR0("Unsupported play_type");
                    break;
                }

                /* Get the current time */
                clock_gettime(CLOCK_MONOTONIC, &tx_time);

                /* check HID connection */
                for (hid_index = 0; hid_index < APP_HH_AS_MAX_HIDD;hid_index++)
                {
                    if(app_hh_as_cb.play_state == APP_HH_AS_PLAY_STARTED)
                    {
                        if (app_hh_as_check_connection(app_hh_as_cb.handle[hid_index]))
                        {
                            /* Send the samples to the HID channel */
                            if (app_hh_as_send_audio(app_hh_as_cb.handle[hid_index], nb_bytes,
                                (SINT16 *)app_hh_as_cb.audio_buf)!=BSA_SUCCESS)
                            {
                                APP_ERROR0("Could not send Audio data to hid channel!!");
                                app_hh_as_cb.handle[hid_index] = BSA_HH_INVALID_HANDLE;
                            }
                        }
                        else
                        {
                            app_hh_as_cb.handle[hid_index] = BSA_HH_INVALID_HANDLE;
                        }
                    }
                }

                /* calculate next tx time */
                tx_time.tv_nsec += (app_hh_as_cb.interval * 1000);
                if (tx_time.tv_nsec > 1000000000)
                {
                    tx_time.tv_sec += 1;
                    tx_time.tv_nsec -= 1000000000;
                }
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tx_time, NULL);

            } while (app_hh_as_cb.play_state != APP_HH_AS_PLAY_STOPPED);
        }
    }
    APP_DEBUG0("Exit");
    pthread_exit(NULL);
}



/*******************************************************************************
 **
 ** Function         app_hh_as_init
 **
 ** Description      Init HID Audio streaming
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_init(void)
{
    int status, index;

    /* Initialize the control structure */
    memset(&app_hh_as_cb, 0, sizeof(app_hh_as_cb));
    app_hh_as_cb.tone_sample_freq = 48000;

    app_hh_as_enc_init();
    /*
     * Create and init Thread
     */
    status = app_create_thread(app_hh_as_tx_thread, 0, 0, &app_hh_as_tx_thread_struct);
    if (status < 0)
    {
        APP_ERROR1("app_create_thread failed: %d", status);
        return -1;
    }

    app_hh_as_cb.play_state = APP_HH_AS_PLAY_STOPPED;
    app_hh_as_cb.interval = 18667;
    app_hh_as_cb.pcm_read_size = 3584;
    app_hh_as_cb.play_type = APP_HH_AS_PLAYTYPE_TONE;

    for (index = 0; index < APP_HH_AS_MAX_HIDD ; index++)
    {
        app_hh_as_cb.handle[index] = BSA_HH_INVALID_HANDLE;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hh_as_select_sound
 **
 ** Description      Select sound source
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_select_sound(void)
{
    UINT8 select;

    APP_INFO0("Select sound source");
    APP_INFO0("0:tone signal, 1:wav file");
#ifdef PCM_ALSA
    APP_INFO0("2:alsa capture(MIC)");
#endif
    select = app_get_choice("number");

    if (select == 0)
    {
        app_hh_as_cb.play_type = APP_HH_AS_PLAYTYPE_TONE;
    }
    else if (select == 1)
    {
        app_hh_as_cb.play_type = APP_HH_AS_PLAYTYPE_FILE;
    }
#ifdef PCM_ALSA
    else if (select == 2)
    {
        app_hh_as_cb.play_type = APP_HH_AS_PLAYTYPE_MIC;
        app_hh_as_play_mic();
    }
#endif
    else
    {
        APP_ERROR1("wrong input %d", select);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_as_start_streaming
 **
 ** Description      Start streaming to RCU
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_start_streaming(UINT8 handle)
{
    int status;
#ifdef PCM_ALSA
    int nb_bytes;
#endif
    APP_INFO0("Start HID audio streaming");

    status = app_hh_as_add_handle(handle);

    if (status < 0)
    {
        APP_ERROR0("cannot start streaming!");
        return status;
    }

    app_hh_as_cb.play_state = APP_HH_AS_PLAY_STARTED;

#ifdef PCM_ALSA
    if(app_hh_as_cb.play_type == APP_HH_AS_PLAYTYPE_MIC)
    {
        do
        {
        /* clear ALSA buffer */
            nb_bytes = app_alsa_capture_read((char *)app_hh_as_cb.audio_buf,
                                app_hh_as_cb.pcm_read_size);
        }while (nb_bytes>0);
    }
#endif

    return status;
}


/*******************************************************************************
 **
 ** Function         app_hh_as_stop_streaming
 **
 ** Description      Stop streaming to RCU
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_as_stop_streaming(UINT8 handle)
{
    int status;

    APP_INFO0("Stop HID audio streaming");
    status = app_hh_as_del_handle(handle);
    if (status < 0)
    {
        APP_ERROR0("cannot stop streaming!");
        return status;
    }

    app_hh_as_cb.play_state = APP_HH_AS_PLAY_STOPPED;
    app_hh_as_cb.seq_number = 0;
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_as_check_connection
 **
 ** Description      Check HID connection is opened
 **
 ** Returns          true if opened, fail is not opened
 **
 *******************************************************************************/
BOOLEAN app_hh_as_check_connection(tBSA_HH_HANDLE handle)
{
    tAPP_HH_DEVICE *p_dev;

    if (handle == BSA_HH_INVALID_HANDLE)
        return FALSE;

    p_dev = app_hh_pdev_find_by_handle(handle);
    if (p_dev == NULL)
    {
        return FALSE;
    }

    if (p_dev->info_mask & APP_HH_DEV_OPENED)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*******************************************************************************
 **
 ** Function        app_hh_as_send_audio
 **
 ** Description     Example of HH Send audio report on Interrupt Channel
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_as_send_audio(tBSA_HH_HANDLE handle, int pcm_length, SINT16 *pcm_data)
{
    int status;
    int index = 0;
    tBSA_HH_SEND_DATA hh_send_data_param;
    UINT8 *hid_data;

    BSA_HhSendDataInit(&hh_send_data_param);
    hh_send_data_param.handle = handle;
    hid_data = hh_send_data_param.data.data;
    *(hid_data + APP_HH_AS_AUDIO_RTP_ID_OFFSET) = (UINT8)APP_HH_AS_AUDIO_REPORT_ID;
    *(hid_data + APP_HH_AS_AUDIO_FORMAT_OFFSET) = APP_HH_AS_AUDIO_DATA;
    *(hid_data + APP_HH_AS_AUDIO_SEQ_OFFSET) = app_hh_as_cb.seq_number++;
    hh_send_data_param.data.length = 4;
    hid_data += 4;

    /* encode PCM data */
    for ( index = 0 ; pcm_length > 0 ; index++ )
    {
        if (pcm_length >=  APP_HH_AS_PCM_BLOCK_SIZE)
        {
            app_hh_as_encode_audio_data(pcm_data, APP_HH_AS_PCM_BLOCK_SIZE);
            hh_send_data_param.data.length += app_hh_as_cb.encoder.u16PacketLength;
            if (hh_send_data_param.data.length > BSA_HH_DATA_SIZE_MAX)
            {
                APP_ERROR1("Audio report is too big!!:%d", hh_send_data_param.data.length);
                return -1;
            }
            memcpy(hid_data, &app_hh_as_cb.sbc_buf, app_hh_as_cb.encoder.u16PacketLength);
            hid_data += app_hh_as_cb.encoder.u16PacketLength;
            pcm_data += (APP_HH_AS_PCM_BLOCK_SIZE/2);
            pcm_length -= APP_HH_AS_PCM_BLOCK_SIZE;
        }
        else if ((pcm_length >0) && (pcm_length < APP_HH_AS_PCM_BLOCK_SIZE))
        {
            APP_ERROR1("Wrong PCM length:%d", pcm_length);
            return -1;
        }
    }
    hh_send_data_param.data.data[APP_HH_AS_AUDIO_NUM_SBC_OFFSET] = index; /* number of SBC frame */

#if APP_HH_AS_DEBUG
    APP_DEBUG1("HID audio data len:%d", hh_send_data_param.data.length);
    APP_DUMP("HID audio data", hh_send_data_param.data.data, hh_send_data_param.data.length);
#endif

    status = BSA_HhSendData(&hh_send_data_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSendData fail status:%d", status);
        return -1;
    }

    return status;
}


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
void app_hh_as_enc_init(void)
{
    app_hh_as_cb.timestamp = 0;

    app_hh_as_cb.encoder.s16ChannelMode = APP_HH_AS_CHANNEL_MODE;
    app_hh_as_cb.encoder.s16NumOfSubBands = APP_HH_AS_NUM_OF_SUBBANDS;
    app_hh_as_cb.encoder.s16NumOfBlocks = APP_HH_AS_NUM_OF_BLOCKS;
    app_hh_as_cb.encoder.s16AllocationMethod = APP_HH_AS_ALLOC_METHOD;
    app_hh_as_cb.encoder.s16SamplingFreq = APP_HH_AS_SAMPLING_FREQ;
    app_hh_as_cb.encoder.u16BitRate = APP_HH_AS_BITRATE;
    app_hh_as_cb.encoder.s16NumOfChannels = 2;
    app_hh_as_cb.TxAaMtuSize = APP_HH_AS_TX_MTU_SIZE;
    APPL_TRACE_WARNING6("app_hh_as_enc_init ch mode %d, nbsubd %d, nb blk %d, alloc method %d, bit rate %d, Smp freq %d",
            app_hh_as_cb.encoder.s16ChannelMode,
            app_hh_as_cb.encoder.s16NumOfSubBands,
            app_hh_as_cb.encoder.s16NumOfBlocks,
            app_hh_as_cb.encoder.s16AllocationMethod,
            app_hh_as_cb.encoder.u16BitRate,
            app_hh_as_cb.encoder.s16SamplingFreq);

    /* Reset entirely the SBC encoder */
    SBC_Encoder_Init(&(app_hh_as_cb.encoder));
    APP_DEBUG1("app_hh_as_enc_init bit pool %d", app_hh_as_cb.encoder.s16BitPool);
    app_hh_as_cb.EncoderInitDone = TRUE;
}


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
void app_hh_as_encode_audio_data(SINT16 *p_pcm_data, UINT16 pcm_len)
{
    UINT16 blocm_x_subband;

    if(app_hh_as_cb.EncoderInitDone == FALSE)
    {
        app_hh_as_enc_init();
    }

    blocm_x_subband = app_hh_as_cb.encoder.s16NumOfSubBands *
            app_hh_as_cb.encoder.s16NumOfBlocks;

    /* Write @ of allocated buffer in encoder.pu8Packet */
    app_hh_as_cb.encoder.pu8Packet = &(app_hh_as_cb.sbc_buf[0]);

    /* Fill allocated buffer with 0 */
    memset(app_hh_as_cb.encoder.as16PcmBuffer, 0,
            blocm_x_subband * app_hh_as_cb.encoder.s16NumOfChannels);
    memcpy((UINT8 *)app_hh_as_cb.encoder.as16PcmBuffer, (UINT8 *)p_pcm_data, pcm_len);
    SBC_Encoder(&app_hh_as_cb.encoder);
    app_hh_as_SbcChkFrInit(app_hh_as_cb.encoder.pu8Packet);
    app_hh_as_SbcDescramble(app_hh_as_cb.encoder.pu8Packet, app_hh_as_cb.encoder.u16PacketLength);
}


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
void app_hh_as_audio_cmd_handler(tBSA_HH_REPORT *p_report)
{
    UINT8 cmd;

    cmd = p_report->report_data.data[APP_HH_AS_AUDIO_FORMAT_OFFSET];

    switch(cmd)
    {
        case APP_HH_AS_AUDIO_START_CMD:
            APP_DEBUG0("Received start streaming cmd!");
            app_hh_as_start_streaming(p_report->handle);
            break;

        case APP_HH_AS_AUDIO_STOP_CMD:
            APP_DEBUG0("Received stop streaming cmd!");
            app_hh_as_stop_streaming(p_report->handle);
            break;

     default:
         APP_ERROR1("Unsupported audio streaming command 0x%x", cmd);
         break;
    }
}

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
int app_hh_as_notif_start_streaming(tBSA_HH_HANDLE handle)
{
    int status;
    tBSA_HH_SEND_DATA hh_send_data_param;

    APP_INFO0("app_hh_as_send_start_streaming_cmd");

    BSA_HhSendDataInit(&hh_send_data_param);
    hh_send_data_param.handle = handle;
    hh_send_data_param.data.data[0] = (UINT8)APP_HH_AS_AUDIO_REPORT_ID;
    hh_send_data_param.data.data[1] = (UINT8)APP_HH_AS_REQ_AUDIO_START;
    hh_send_data_param.data.length = 2;

    status = BSA_HhSendData(&hh_send_data_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSendData fail status:%d", status);
        return -1;
    }

    return status;
}

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
int app_hh_as_notif_stop_streaming(tBSA_HH_HANDLE handle)
{
    int status;
    tBSA_HH_SEND_DATA hh_send_data_param;

    APP_INFO0("app_hh_as_send_start_streaming_cmd");

    BSA_HhSendDataInit(&hh_send_data_param);
    hh_send_data_param.handle = handle;
    hh_send_data_param.data.data[0] = (UINT8)APP_HH_AS_AUDIO_REPORT_ID;
    hh_send_data_param.data.data[1] = (UINT8)APP_HH_AS_REQ_AUDIO_STOP;
    hh_send_data_param.data.length = 2;

    status = BSA_HhSendData(&hh_send_data_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSendData fail status:%d", status);
        return -1;
    }

    return status;
}
