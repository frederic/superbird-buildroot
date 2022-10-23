/*****************************************************************************
 **
 **  Name:           app_avk.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2009-2016, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 **  Copyright (C) 2017-2018 Cypress Semiconductor Corporation
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef PCM_ALSA_OPEN_BLOCKING
#include <pthread.h>
#endif
#include "buildcfg.h"

#include "bsa_api.h"

#include "gki.h"
#include "uipc.h"

#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_wav.h"

#ifdef PCM_ALSA
#include "alsa/asoundlib.h"
#define APP_AVK_ASLA_DEV "default"
#endif

#include "app_avk.h"
/*
 * Defines
 */

#define APP_XML_REM_DEVICES_FILE_PATH "./bt_devices.xml"

#define APP_AVK_SOUND_FILE "test_avk"

#ifndef BSA_AVK_SECURITY
#define BSA_AVK_SECURITY    BSA_SEC_AUTHORIZATION
#endif
#ifndef BSA_AVK_FEATURES
#define BSA_AVK_FEATURES    (BSA_AVK_FEAT_DELAY_RPT)
#endif

#ifndef BSA_RC_FEATURES
#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
#define BSA_RC_FEATURES    (BSA_RC_FEAT_RCCT|BSA_RC_FEAT_RCTG|\
                             BSA_RC_FEAT_VENDOR|BSA_RC_FEAT_METADATA|\
                             BSA_RC_FEAT_BROWSE|BSA_RC_FEAT_COVER_ART)
#else
#define BSA_RC_FEATURES    (BSA_RC_FEAT_RCCT|BSA_RC_FEAT_RCTG|\
                             BSA_RC_FEAT_VENDOR|BSA_RC_FEAT_METADATA|\
                             BSA_RC_FEAT_BROWSE)
#endif
#endif


#ifndef BSA_AVK_DUMP_RX_DATA
#define BSA_AVK_DUMP_RX_DATA FALSE
#endif

#ifndef BSA_AVK_AAC_SUPPORTED
#define BSA_AVK_AAC_SUPPORTED FALSE
#endif

/* bitmask of events that BSA client is interested in registering for notifications */
tBSA_AVK_REG_NOTIFICATIONS reg_notifications =
        (1 << (BSA_AVK_RC_EVT_PLAY_STATUS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_TRACK_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_TRACK_REACHED_END - 1)) |
        (1 << (BSA_AVK_RC_EVT_TRACK_REACHED_START - 1)) |
        (1 << (BSA_AVK_RC_EVT_PLAY_POS_CHANGED - 1)) |
        (1 << (BSA_AVK_RC_EVT_BATTERY_STATUS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_SYSTEM_STATUS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_APP_SETTING_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_NOW_PLAYING_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_AVAL_PLAYERS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_ADDR_PLAYER_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_UIDS_CHANGE - 1));

/*
 * global variable
 */

tAPP_AVK_CB app_avk_cb;

#ifdef PCM_ALSA
static char *alsa_device = "default"; /* ALSA playback device */
#ifdef PCM_ALSA_OPEN_BLOCKING
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
#endif /* PCM_ALSA */

/*
 * Local functions
 */
static void app_avk_close_wave_file(tAPP_AVK_CONNECTION *connection);
static void app_avk_create_wave_file(void);
static void app_avk_uipc_cback(BT_HDR *p_msg);

#ifdef PCM_ALSA_OPEN_BLOCKING
static tAPP_AVK_CONNECTION* pStreamingConn = NULL;
#endif

/*******************************************************************************
 **
 ** Function         app_avk_get_label
 **
 ** Description      get transaction label (used to distinguish transactions)
 **
 ** Returns          UINT8
 **
 *******************************************************************************/
UINT8 app_avk_get_label()
{
    if(app_avk_cb.label >= 15)
        app_avk_cb.label = 0;
    return app_avk_cb.label++;
}

/*******************************************************************************
 **
 ** Function         app_avk_create_wave_file
 **
 ** Description     create a new wave file
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_create_wave_file(void)
{
    int file_index = 0;
    int fd;
    char filename[200];

#ifdef PCM_ALSA
    return;
#endif

    /* If a wav file is currently open => close it */
    if (app_avk_cb.fd != -1)
    {
        tAPP_AVK_CONNECTION dummy;
        memset (&dummy, 0, sizeof(tAPP_AVK_CONNECTION));
        app_avk_close_wave_file(&dummy);
    }

    do
    {
        snprintf(filename, sizeof(filename), "%s-%d.wav", APP_AVK_SOUND_FILE, file_index);
        filename[sizeof(filename)-1] = '\0';
        fd = app_wav_create_file(filename, O_EXCL);
        file_index++;
    } while (fd < 0);

    app_avk_cb.fd = fd;
}
/*******************************************************************************
 **
 ** Function         app_avk_create_aac_file
 **
 ** Description     create a new wave file
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_create_aac_file(void)
{
    int file_index = 0;
    int fd;
    char filename[200];

    /* If a wav file is currently open => close it */
    if (app_avk_cb.fd != -1)
    {
//        close(app_avk_cb.fd);
//        app_avk_cb.fd = -1;
        return;
    }

    do
    {
        int flags = O_EXCL | O_RDWR | O_CREAT | O_TRUNC;

        snprintf(filename, sizeof(filename), "%s-%d.aac", APP_AVK_SOUND_FILE, file_index);
        filename[sizeof(filename)-1] = '\0';

        /* Create file in read/write mode, reset the length field */
        fd = open(filename, flags, 0666);
        if (fd < 0)
        {
                APP_ERROR1("open(%s) failed: %d", filename, errno);
            }

        file_index++;
    } while ((fd < 0) && (file_index<100));

    app_avk_cb.fd_codec_type = BSA_AVK_CODEC_M24;
    app_avk_cb.fd = fd;
}

/*******************************************************************************
 **
 ** Function         app_avk_close_wave_file
 **
 ** Description     proper header and close the wave file
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_close_wave_file(tAPP_AVK_CONNECTION *connection)
{
    tAPP_WAV_FILE_FORMAT format;

#ifdef PCM_ALSA
    return;
#endif

    if (app_avk_cb.fd == -1)
    {
        printf("app_avk_close_wave_file: no file to close\n");
        return;
    }

    format.bits_per_sample = connection->bit_per_sample;
    format.nb_channels = connection->num_channel;
    format.sample_rate = connection->sample_rate;

    app_wav_close_file(app_avk_cb.fd, &format);

    app_avk_cb.fd = -1;
}

/*******************************************************************************
**
** Function         app_avk_end
**
** Description      This function is used to close AV
**
** Returns          void
**
*******************************************************************************/
void app_avk_end(void)
{
    tBSA_AVK_DISABLE disable_param;
    tBSA_AVK_DEREGISTER deregister_param;

    /* deregister avk */
    BSA_AvkDeregisterInit(&deregister_param);
    BSA_AvkDeregister(&deregister_param);

    /* disable avk */
    BSA_AvkDisableInit(&disable_param);
    BSA_AvkDisable(&disable_param);
}


/*******************************************************************************
**
** Function         app_avk_handle_start
**
** Description      This function is the AVK start handler function
**                  Configures PCM Driver with the stream settings
** Returns          void
**
*******************************************************************************/
static void app_avk_handle_start(tBSA_AVK_MSG *p_data, tAPP_AVK_CONNECTION *connection)
{
    char *avk_format_display[12] = {"SBC", "M12", "M24", "??", "ATRAC", "PCM", "APT-X", "SEC"};
#ifdef PCM_ALSA
    int status;
    snd_pcm_format_t format;
#endif

    if (p_data->start_streaming.media_receiving.format < (sizeof(avk_format_display)/sizeof(avk_format_display[0])))
    {
        APP_INFO1("AVK start: format %s", avk_format_display[p_data->start_streaming.media_receiving.format]);
    }
    else
    {
        APP_INFO1("AVK start: format code (%u) unknown", p_data->start_streaming.media_receiving.format);
    }

    connection->format = p_data->start_streaming.media_receiving.format;

    if (connection->format == BSA_AVK_CODEC_PCM)
    {
        app_avk_create_wave_file();/* create and open a wave file */
        connection->sample_rate = p_data->start_streaming.media_receiving.cfg.pcm.sampling_freq;
        connection->num_channel = p_data->start_streaming.media_receiving.cfg.pcm.num_channel;
        connection->bit_per_sample = p_data->start_streaming.media_receiving.cfg.pcm.bit_per_sample;
        printf("Sampling rate:%d, number of channel:%d bit per sample:%d\n",
            p_data->start_streaming.media_receiving.cfg.pcm.sampling_freq,
            p_data->start_streaming.media_receiving.cfg.pcm.num_channel,
            p_data->start_streaming.media_receiving.cfg.pcm.bit_per_sample);
#ifdef PCM_ALSA
        /* If ALSA PCM driver was already open => close it */
        if (app_avk_cb.alsa_handle != NULL)
        {
#ifdef PCM_ALSA_OPEN_BLOCKING
            pthread_mutex_lock(&mutex);
#endif
            snd_pcm_close(app_avk_cb.alsa_handle);
            app_avk_cb.alsa_handle = NULL;
#ifdef PCM_ALSA_OPEN_BLOCKING
            pthread_mutex_unlock(&mutex);
#endif
        }

        /* Open ALSA driver */
#ifdef PCM_ALSA_OPEN_BLOCKING
        /* Configure as blocking */
        status = snd_pcm_open(&(app_avk_cb.alsa_handle), alsa_device,
            SND_PCM_STREAM_PLAYBACK, 0);
#else
        status = snd_pcm_open(&(app_avk_cb.alsa_handle), alsa_device,
            SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
#endif
        if (status < 0)
        {
            APP_ERROR1("snd_pcm_open failed: %s", snd_strerror(status));

        }
        else
        {
            APP_DEBUG0("ALSA driver opened");
            if (connection->bit_per_sample == 8)
                format = SND_PCM_FORMAT_U8;
            else
                format = SND_PCM_FORMAT_S16_LE;
            /* Configure ALSA driver with PCM parameters */
            status = snd_pcm_set_params(app_avk_cb.alsa_handle, format,
                SND_PCM_ACCESS_RW_INTERLEAVED, connection->num_channel,
                connection->sample_rate, 1, 500000);/* 0.5sec */
            if (status < 0)
            {
                APP_ERROR1("snd_pcm_set_params failed: %s", snd_strerror(status));
                exit(1);
            }
        }
#endif
    }
    else if (connection->format == BSA_AVK_CODEC_M24)
    {
        /* create and open an aac file to dump data to */
        app_avk_create_aac_file();

        /* Initialize the decoder with the format information here */
    }

}

/*******************************************************************************
**
** Function         app_avk_select_streaming_device
**
** Description      This function is used to select streaming device
**                  when mutiple AV devices are streaming simultaneously
** Returns          void
**
*******************************************************************************/
void app_avk_select_streaming_device(BD_ADDR bd_addr)
{
    tBSA_AVK_SELECT_STREAM_DEV sel_str_dev;
    int status;

    APP_DEBUG0("app_avk_select_streaming_device");

    BSA_AvkSelectStreamDevInit(&sel_str_dev);
    bdcpy(sel_str_dev.bd_addr, bd_addr);
    status = BSA_AvkSelectStreamDev(&sel_str_dev);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to select streaming device %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_cback
 **
 ** Description      This function is the AVK callback function
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_cback(tBSA_AVK_EVT event, tBSA_AVK_MSG *p_data)
{
    tAPP_AVK_CONNECTION *connection = NULL;
    tBSA_AVK_REM_RSP RemRsp;
    int i, j;

    switch (event)
    {
    case BSA_AVK_REGISTER_EVT:
        if(p_data->reg_evt.status == BSA_SUCCESS && app_avk_cb.uipc_audio_channel == UIPC_CH_ID_BAD)
        {
            /* Save UIPC channel */
            app_avk_cb.uipc_audio_channel = p_data->reg_evt.uipc_channel;
            /* open the UIPC channel to receive the pcm */
            if (UIPC_Open(p_data->reg_evt.uipc_channel, app_avk_uipc_cback) == FALSE)
            {
                APP_ERROR0("Unable to open UIPC channel");
                break;
            }
            app_avk_cb.fd = -1;
        }
        break;
    case BSA_AVK_OPEN_EVT:
        APP_DEBUG1("BSA_AVK_OPEN_EVT status 0x%x", p_data->sig_chnl_open.status);

        if (p_data->sig_chnl_open.status == BSA_SUCCESS)
        {
            connection = app_avk_add_connection(p_data->sig_chnl_open.bd_addr);

            if(connection == NULL)
            {
                APP_DEBUG0("BSA_AVK_OPEN_EVT cannot allocate connection cb");
                break;
            }

            connection->ccb_handle = p_data->sig_chnl_open.ccb_handle;
            connection->is_open = TRUE;
            connection->is_streaming_chl_open = FALSE;
        }

        app_avk_cb.open_pending = FALSE;
        memset(app_avk_cb.open_pending_bda, 0, sizeof(BD_ADDR));

        printf("AVK connected to device %02X:%02X:%02X:%02X:%02X:%02X\n",
            p_data->sig_chnl_open.bd_addr[0], p_data->sig_chnl_open.bd_addr[1],p_data->sig_chnl_open.bd_addr[2],
            p_data->sig_chnl_open.bd_addr[3], p_data->sig_chnl_open.bd_addr[4],p_data->sig_chnl_open.bd_addr[5]);
        break;

    case BSA_AVK_CLOSE_EVT:
        /* Close event, reason BTA_AVK_CLOSE_STR_CLOSED or BTA_AVK_CLOSE_CONN_LOSS*/
        APP_DEBUG1("BSA_AVK_CLOSE_EVT status 0x%x, handle %d", p_data->sig_chnl_close.status, p_data->sig_chnl_close.ccb_handle);
        connection = app_avk_find_connection_by_bd_addr(p_data->sig_chnl_close.bd_addr);

        if(connection == NULL)
        {
            APP_DEBUG1("BSA_AVK_CLOSE_EVT unknown handle %d", p_data->sig_chnl_close.ccb_handle);
            break;
        }

        connection->is_open = FALSE;
        app_avk_cb.open_pending = FALSE;
        memset(app_avk_cb.open_pending_bda, 0, sizeof(BD_ADDR));
        if (connection->is_started_streaming == TRUE)
        {
            connection->is_started_streaming = FALSE;

            if (connection->format == BSA_AVK_CODEC_M24)
            {
                close(app_avk_cb.fd);
                app_avk_cb.fd  = -1;
            }
            else
                app_avk_close_wave_file(connection);
        }
        app_avk_reset_connection(connection->bda_connected);
        break;

    case BSA_AVK_STR_OPEN_EVT:
        APP_DEBUG1("BSA_AVK_STR_OPEN_EVT status 0x%x", p_data->stream_chnl_open.status);

        connection = app_avk_find_connection_by_av_handle(p_data->stream_chnl_open.ccb_handle);
        if(connection == NULL)
        {
            break;
        }
        connection->is_streaming_chl_open = TRUE;
        connection->cur_psc_mask = p_data->stream_chnl_open.cur_psc_mask;
        connection->avdt_version = p_data->stream_chnl_open.avdt_version;
        APP_DEBUG1("BSA_AVK_STR_OPEN_EVT cur_psc_mask 0x%x, avdt_version: 0x%x",
                    p_data->stream_chnl_open.cur_psc_mask, p_data->stream_chnl_open.avdt_version);

        break;

    case BSA_AVK_STR_CLOSE_EVT:
        APP_DEBUG1("BSA_AVK_STR_CLOSE_EVT streaming chn closed handle: %d ", p_data->stream_chnl_close.ccb_handle);
        connection = app_avk_find_connection_by_bd_addr(p_data->stream_chnl_close.bd_addr);
        if(connection == NULL)
        {
            break;
        }

        connection->is_streaming_chl_open = FALSE;
        break;

        case BSA_AVK_DELAY_RPT_RSP_EVT:
            APP_DEBUG1("BSA_AVK_DELAY_RPT_RSP_EVT status 0x%x", p_data->delay_rpt_rsp.status);
            APP_DEBUG1("device %02X:%02X:%02X:%02X:%02X:%02X\n",
                p_data->delay_rpt_rsp.bd_addr[0], p_data->delay_rpt_rsp.bd_addr[1],p_data->delay_rpt_rsp.bd_addr[2],
                p_data->delay_rpt_rsp.bd_addr[3], p_data->delay_rpt_rsp.bd_addr[4],p_data->delay_rpt_rsp.bd_addr[5]);
        break;

    case BSA_AVK_START_EVT:
        APP_DEBUG1("BSA_AVK_START_EVT status 0x%x", p_data->start_streaming.status);

        /* We got START_EVT for a new device that is streaming but server discards the data
            because another stream is already active */
        if(p_data->start_streaming.discarded)
        {
            connection = app_avk_find_connection_by_bd_addr(p_data->start_streaming.bd_addr);
            if(connection)
            {
                connection->is_started_streaming = TRUE;
                connection->is_streaming_chl_open = TRUE;
            }

            break;
        }
        /* got Start event and device is streaming */
        else
        {
            app_avk_cb.pStreamingConn = NULL;

            connection = app_avk_find_connection_by_bd_addr(p_data->start_streaming.bd_addr);

            if(connection == NULL)
            {
                break;
            }

            connection->is_started_streaming = TRUE;
            connection->is_streaming_chl_open = TRUE;
            app_avk_handle_start(p_data, connection);
            app_avk_cb.pStreamingConn = connection;
        }

        break;



    case BSA_AVK_STOP_EVT:
        APP_DEBUG1("BSA_AVK_STOP_EVT handle: %d  Suspended: %d", p_data->stop_streaming.ccb_handle, p_data->stop_streaming.suspended);

        /* Stream was suspended */
        if(p_data->stop_streaming.suspended)
        {
            connection = app_avk_find_connection_by_bd_addr(p_data->stop_streaming.bd_addr);
            if(connection == NULL)
            {
                break;
            }

            connection->is_started_streaming = FALSE;
        }
        /* stream was closed */
        else
        {
            connection = app_avk_find_connection_by_av_handle(p_data->stop_streaming.ccb_handle);
            if(connection == NULL)
            {
                break;
            }

            connection->is_started_streaming = FALSE;

            if (connection->format == BSA_AVK_CODEC_M24)
            {
                close(app_avk_cb.fd);
                app_avk_cb.fd  = -1;
            }
            else
                app_avk_close_wave_file(connection);

        }

        break;

    case BSA_AVK_RC_OPEN_EVT:

        if(p_data->rc_open.status == BSA_SUCCESS)
        {
            connection = app_avk_add_connection(p_data->rc_open.bd_addr);
            if(connection == NULL)
            {
                APP_DEBUG0("BSA_AVK_RC_OPEN_EVT could not allocate connection");
                break;
            }

            APP_DEBUG1("BSA_AVK_RC_OPEN_EVT peer_feature=0x%x rc_handle=%d",
                p_data->rc_open.peer_features, p_data->rc_open.rc_handle);
            connection->rc_handle = p_data->rc_open.rc_handle;
            connection->peer_features =  p_data->rc_open.peer_features;
            connection->peer_version = p_data->rc_open.peer_version;
            connection->is_rc_open = TRUE;
        }
        break;

    case BSA_AVK_RC_PEER_OPEN_EVT:
        connection = app_avk_find_connection_by_rc_handle(p_data->rc_open.rc_handle);
        if(connection == NULL)
        {
            APP_DEBUG1("BSA_AVK_RC_PEER_OPEN_EVT could not find connection handle %d", p_data->rc_open.rc_handle);
            break;
        }

        APP_DEBUG1("BSA_AVK_RC_PEER_OPEN_EVT peer_feature=0x%x rc_handle=%d",
            p_data->rc_open.peer_features, p_data->rc_open.rc_handle);

        connection->peer_features =  p_data->rc_open.peer_features;
        connection->peer_version = p_data->rc_open.peer_version;
        connection->is_rc_open = TRUE;
        break;

    case BSA_AVK_RC_CLOSE_EVT:
        APP_DEBUG0("BSA_AVK_RC_CLOSE_EVT");
        connection = app_avk_find_connection_by_rc_handle(p_data->rc_close.rc_handle);
        if(connection == NULL)
        {
            break;
        }

        connection->is_rc_open = FALSE;
        break;

    case BSA_AVK_REMOTE_RSP_EVT:
        APP_DEBUG0("BSA_AVK_REMOTE_RSP_EVT");
        break;

    case BSA_AVK_REMOTE_CMD_EVT:
        APP_DEBUG0("BSA_AVK_REMOTE_CMD_EVT");
        APP_DEBUG1(" label:0x%x", p_data->remote_cmd.label);
        APP_DEBUG1(" op_id:0x%x", p_data->remote_cmd.op_id);
        APP_DEBUG1(" label:0x%x", p_data->remote_cmd.label);
        APP_DEBUG0(" avrc header");
        APP_DEBUG1("   ctype:0x%x", p_data->remote_cmd.hdr.ctype);
        APP_DEBUG1("   subunit_type:0x%x", p_data->remote_cmd.hdr.subunit_type);
        APP_DEBUG1("   subunit_id:0x%x", p_data->remote_cmd.hdr.subunit_id);
        APP_DEBUG1("   opcode:0x%x", p_data->remote_cmd.hdr.opcode);
        APP_DEBUG1(" len:0x%x", p_data->remote_cmd.len);
        APP_DUMP("data", p_data->remote_cmd.data, p_data->remote_cmd.len);

        BSA_AvkRemoteRspInit(&RemRsp);
        RemRsp.avrc_rsp = BSA_AVK_RSP_ACCEPT;
        RemRsp.label = p_data->remote_cmd.label;
        RemRsp.op_id = p_data->remote_cmd.op_id;
        RemRsp.key_state = p_data->remote_cmd.key_state;
        RemRsp.handle = p_data->remote_cmd.rc_handle;
        RemRsp.length = 0;
        BSA_AvkRemoteRsp(&RemRsp);
        break;

    case BSA_AVK_VENDOR_CMD_EVT:
        APP_DEBUG0("BSA_AVK_VENDOR_CMD_EVT");
        APP_DEBUG1(" code:0x%x", p_data->vendor_cmd.code);
        APP_DEBUG1(" company_id:0x%x", p_data->vendor_cmd.company_id);
        APP_DEBUG1(" label:0x%x", p_data->vendor_cmd.label);
        APP_DEBUG1(" len:0x%x", p_data->vendor_cmd.len);
#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
        APP_DUMP("A2DP Data", p_data->vendor_cmd.data, p_data->vendor_cmd.len);
#endif
        break;

    case BSA_AVK_VENDOR_RSP_EVT:
        APP_DEBUG0("BSA_AVK_VENDOR_RSP_EVT");
        APP_DEBUG1(" code:0x%x", p_data->vendor_rsp.code);
        APP_DEBUG1(" company_id:0x%x", p_data->vendor_rsp.company_id);
        APP_DEBUG1(" label:0x%x", p_data->vendor_rsp.label);
        APP_DEBUG1(" len:0x%x", p_data->vendor_rsp.len);
#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
        APP_DUMP("A2DP Data", p_data->vendor_rsp.data, p_data->vendor_rsp.len);
#endif
        break;

    case BSA_AVK_CP_INFO_EVT:
        APP_DEBUG0("BSA_AVK_CP_INFO_EVT");
        if(p_data->cp_info.id == BSA_AVK_CP_SCMS_T_ID)
        {
            switch(p_data->cp_info.info.scmst_flag)
            {
                case BSA_AVK_CP_SCMS_COPY_NEVER:
                    APP_INFO1(" content protection:0x%x - COPY NEVER", p_data->cp_info.info.scmst_flag);
                    break;
                case BSA_AVK_CP_SCMS_COPY_ONCE:
                    APP_INFO1(" content protection:0x%x - COPY ONCE", p_data->cp_info.info.scmst_flag);
                    break;
                case BSA_AVK_CP_SCMS_COPY_FREE:
                case (BSA_AVK_CP_SCMS_COPY_FREE+1):
                    APP_INFO1(" content protection:0x%x - COPY FREE", p_data->cp_info.info.scmst_flag);
                    break;
                default:
                    APP_ERROR1(" content protection:0x%x - UNKNOWN VALUE", p_data->cp_info.info.scmst_flag);
                    break;
            }
        }
        else
        {
            APP_INFO0("No content protection");
        }
        break;

    case BSA_AVK_REGISTER_NOTIFICATION_EVT:
        APP_DEBUG1("BSA_AVK_REGISTER_NOTIFICATION_EVT handle:%d", p_data->reg_notif_rsp.handle);
        APP_DEBUG1("event_id:0x%x, opcode:0x%x, pdu:0x%x\n",
                p_data->reg_notif_rsp.rsp.event_id,
                p_data->reg_notif_rsp.rsp.opcode,
                p_data->reg_notif_rsp.rsp.pdu);

        if (p_data->reg_notif_rsp.rsp.event_id == AVRC_EVT_VOLUME_CHANGE)
        {
            APP_INFO1("Volume changed :0x%x", p_data->reg_notif_rsp.rsp.param.volume);
        }

        if (p_data->reg_notif_rsp.rsp.event_id == AVRC_EVT_PLAY_STATUS_CHANGE)
        {
            switch(p_data->reg_notif_rsp.rsp.param.play_status)
            {
                case AVRC_PLAYSTATE_PLAYING:
                     APP_INFO0("Play Status Playing");
                     break;
                case AVRC_PLAYSTATE_STOPPED:
                case AVRC_PLAYSTATE_PAUSED:
                     APP_INFO0("Play Status Stopped");
                     break;
                default:
                     APP_INFO1("Play Status Playing : %02x",
                         p_data->reg_notif_rsp.rsp.param.play_status);
                     break;
            }
        }
        break;

    case BSA_AVK_LIST_PLAYER_APP_ATTR_EVT:
        APP_DEBUG0("BSA_AVK_LIST_PLAYER_APP_ATTR_EVT");
        APP_INFO1("handle:0x%x", p_data->list_app_attr.handle);
        APP_INFO1("pdu:0x%x, status:0x%x, opcode:0x%x, num_attr:%d",
                p_data->list_app_attr.rsp.pdu, p_data->list_app_attr.rsp.status,
                p_data->list_app_attr.rsp.opcode, p_data->list_app_attr.rsp.num_attr);
        for(i = 0 ; i < p_data->list_app_attr.rsp.num_attr ; i++)
        {
            APP_INFO1("attr:0x%x", p_data->list_app_attr.rsp.attrs[i]);
        }
        break;

    case BSA_AVK_LIST_PLAYER_APP_VALUES_EVT:
        APP_DEBUG0("BSA_AVK_LIST_PLAYER_APP_VALUES_EVT");
        APP_INFO1("handle:0x%x", p_data->list_app_values.handle);
        APP_INFO1("pdu:0x%x, status:0x%x, opcode:0x%x, num_attr:%d",
                p_data->list_app_values.rsp.pdu, p_data->list_app_values.rsp.status,
                p_data->list_app_values.rsp.opcode, p_data->list_app_values.rsp.num_val);
        for(i = 0 ; i < p_data->list_app_values.rsp.num_val ; i++)
        {
            APP_INFO1("attr:0x%x", p_data->list_app_values.rsp.vals[i]);
        }
        break;

    case BSA_AVK_SET_PLAYER_APP_VALUE_EVT:
        APP_DEBUG0("BSA_AVK_SET_PLAYER_APP_VALUE_EVT");
        break;

    case BSA_AVK_GET_PLAYER_APP_VALUE_EVT:
        APP_DEBUG0("BSA_AVK_GET_PLAYER_APP_VALUE_EVT");
        break;

    case BSA_AVK_GET_ELEM_ATTR_EVT:
        APP_DEBUG0("BSA_AVK_GET_ELEM_ATTR_EVT");
        APP_INFO1("status:0x%x, num_attr:0x%x, handle:0x%x",
                p_data->elem_attr.status,
                p_data->elem_attr.num_attr,
                p_data->elem_attr.handle);
        if(p_data->elem_attr.num_attr <= BSA_AVK_ELEMENT_ATTR_MAX)
        {
            for(i = 0 ; i < p_data->elem_attr.num_attr ; i++)
            {
                APP_INFO1("attr_id:0x%x", p_data->elem_attr.attr_entry[i].attr_id);
                APP_INFO1("name:%s", p_data->elem_attr.attr_entry[i].name.data);
            }
        }
        break;

    case BSA_AVK_GET_PLAY_STATUS_EVT:
        APP_DEBUG0("BSA_AVK_GET_PLAY_STATUS_EVT");
        break;
    case BSA_AVK_SET_ADDRESSED_PLAYER_EVT:
        APP_DEBUG0("BSA_AVK_SET_ADDRESSED_PLAYER_EVT");
        break;

    case BSA_AVK_SET_BROWSED_PLAYER_EVT:
        APP_DEBUG0("BSA_AVK_SET_BROWSED_PLAYER_EVT");
        APP_INFO1("status:0x%x, uid_counter:%d, folder_depth:%d, final:%d, handle:0x%x",
                p_data->br_player.status,
                p_data->br_player.uid_counter,
                p_data->br_player.folder_depth,
                p_data->br_player.final,
                p_data->br_player.handle);
        if(p_data->br_player.folder.str_len)
        {
            APP_INFO1("folder:%s", p_data->br_player.folder.data);
        }
        break;

    case BSA_AVK_GET_FOLDER_ITEMS_EVT:
        APP_DEBUG0("BSA_AVK_GET_FOLDER_ITEMS_EVT");
        APP_INFO1("status:0x%x, uid_counter:%d, item_count:%d, final:%d, handle:0x%x",
                p_data->get_items.status,
                p_data->get_items.uid_counter,
                p_data->get_items.item_count,
                p_data->get_items.final,
                p_data->get_items.handle);
        APP_INFO1("item type:0x%x", p_data->get_items.item.item_type);

        switch(p_data->get_items.item.item_type)
        {
        case AVRC_ITEM_NONE:
            APP_INFO0("AVRC_ITEM_NONE");
            break;

        case AVRC_ITEM_PLAYER:
            APP_INFO0("AVRC_ITEM_PLAYER");
            APP_INFO1("player_id 0x%x", p_data->get_items.item.u.player.player_id);
            APP_INFO1("major_type 0x%x", p_data->get_items.item.u.player.major_type);
            APP_INFO1("sub_type 0x%x", p_data->get_items.item.u.player.sub_type);
            APP_INFO1("play_status 0x%x", p_data->get_items.item.u.player.play_status);
            for(j= 0 ; j < AVRC_FEATURE_MASK_SIZE ; j++)
            {
                APP_INFO1("feature mask %d : 0x%x", j, p_data->get_items.item.u.player.features[j]);
            }
            APP_INFO1("name : %s", p_data->get_items.item.u.player.name.data);
            break;

        case AVRC_ITEM_FOLDER:
            APP_INFO0("AVRC_ITEM_FOLDER");
            APP_INFO0("UID values");
            APP_INFO1(" 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                        p_data->get_items.item.u.folder.uid[0],
                        p_data->get_items.item.u.folder.uid[1],
                        p_data->get_items.item.u.folder.uid[2],
                        p_data->get_items.item.u.folder.uid[3],
                        p_data->get_items.item.u.folder.uid[4],
                        p_data->get_items.item.u.folder.uid[5],
                        p_data->get_items.item.u.folder.uid[6],
                        p_data->get_items.item.u.folder.uid[7]);
            APP_INFO1("name:%s", p_data->get_items.item.u.folder.name.data);
            APP_INFO1("playable:%d", p_data->get_items.item.u.folder.playable);
            APP_INFO1("type:0x%x", p_data->get_items.item.u.folder.type);
            break;

        case AVRC_ITEM_MEDIA:
            APP_INFO0("AVRC_ITEM_MEDIA");
            APP_INFO0("UID values");
            APP_INFO1(" 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                        p_data->get_items.item.u.media.uid[0],
                        p_data->get_items.item.u.media.uid[1],
                        p_data->get_items.item.u.media.uid[2],
                        p_data->get_items.item.u.media.uid[3],
                        p_data->get_items.item.u.media.uid[4],
                        p_data->get_items.item.u.media.uid[5],
                        p_data->get_items.item.u.media.uid[6],
                        p_data->get_items.item.u.media.uid[7]);
            APP_INFO1("attr_count:0x%x", p_data->get_items.item.u.media.attr_count);
            APP_INFO1("name:%s", p_data->get_items.item.u.media.name.data);
            APP_INFO1("type:0x%x", p_data->get_items.item.u.media.type);
            for(j = 0 ; j < p_data->get_items.item.u.media.attr_count ; j ++)
            {
                APP_INFO1("attr_id : 0x%x", p_data->get_items.item.u.media.attr_entry[j].attr_id);
                APP_INFO1("name : %s", p_data->get_items.item.u.media.attr_entry[j].name.data);
            }
            break;

        default:
            APP_INFO1("wrong type : 0x%x", p_data->get_items.item.item_type);
            break;
        }
        break;

    case BSA_AVK_CHANGE_PATH_EVT:
        APP_DEBUG0("BSA_AVK_CHANGE_PATH_EVT");
        APP_INFO1("status:0x%x, pdu:%d, opcode:0x%x, num_items:%d, handle:0x%x",
                p_data->chg_path.rsp.status,
                p_data->chg_path.rsp.pdu,
                p_data->chg_path.rsp.opcode,
                p_data->chg_path.rsp.num_items,
                p_data->chg_path.handle);
        break;

    case BSA_AVK_GET_ITEM_ATTR_EVT:
        APP_DEBUG0("BSA_AVK_GET_ITEM_ATTR_EVT");
        APP_INFO1("status:0x%x, rsp-handle:0x%x, num_attr:%d, handle:0x%x",
                p_data->item_attr.rsp.status,
                p_data->item_attr.rsp.handle,
                p_data->item_attr.rsp.num_attr,
                p_data->item_attr.handle);
        for(i = 0; i < p_data->item_attr.rsp.num_attr; i++)
        {
            APP_INFO1("attr_id:0x%x", p_data->item_attr.rsp.attr_entry[i].attr_id);
            APP_INFO1("name:%s", p_data->item_attr.rsp.attr_entry[i].name.data);
        }
        break;

    case BSA_AVK_PLAY_ITEM_EVT:
        APP_DEBUG0("BSA_AVK_PLAY_ITEM_EVT");
        APP_INFO1("status:0x%x, pdu:%d, opcode:0x%x, handle:0x%x",
                p_data->play_item.rsp.status,
                p_data->play_item.rsp.pdu,
                p_data->play_item.rsp.opcode,
                p_data->play_item.handle);
        break;

    case BSA_AVK_ADD_TO_NOW_PLAYING_EVT:
        APP_DEBUG0("BSA_AVK_ADD_TO_NOW_PLAYING_EVT");
        break;

    case BSA_AVK_SET_ABS_VOL_CMD_EVT:
        APP_DEBUG0("BSA_AVK_SET_ABS_VOL_CMD_EVT");
        connection = app_avk_find_connection_by_rc_handle(p_data->abs_volume.handle);
        if(connection != NULL)
        {
            APP_INFO1("ADSOLUTE Volume : %d", p_data->abs_volume.abs_volume_cmd.volume);
            /* Peer requested change in volume. Make the change and send response with new system volume. BSA is TG role in AVK */
            if (p_data->abs_volume.abs_volume_cmd.volume <= BSA_MAX_ABS_VOLUME)
            {
                app_avk_cb.volume = p_data->abs_volume.abs_volume_cmd.volume;
            }

            /* Change the code below based on which interface audio is going out to. */
            /*char buffer[100];
            sprintf(buffer, "amixer -q cset name='Headphone Playback Volume' '%d'", app_avk_cb.volume);
            system(buffer);*/
            app_avk_set_abs_vol_rsp(app_avk_cb.volume, p_data->abs_volume.handle, p_data->abs_volume.label);;
        }
        else
        {
            APP_ERROR1("not changing volume connection 0x%x m_bAbsVolumeSupported %d", connection, connection->m_bAbsVolumeSupported);
        }
        break;


    case BSA_AVK_REG_NOTIFICATION_CMD_EVT:
        APP_DEBUG0("BSA_AVK_REG_NOTIFICATION_CMD_EVT");
        if (p_data->reg_notif_cmd.reg_notif_cmd.event_id == AVRC_EVT_VOLUME_CHANGE)
        {
            connection = app_avk_find_connection_by_rc_handle(p_data->reg_notif_cmd.handle);
            if(connection != NULL)
            {
                connection->m_bAbsVolumeSupported = TRUE;
                connection->volChangeLabel = p_data->reg_notif_cmd.label;

                /* Peer requested registration for vol change event. Send response with current system volume. BSA is TG role in AVK */
                app_avk_reg_notfn_rsp(app_avk_cb.volume,
                                  p_data->reg_notif_cmd.handle,
                                  p_data->reg_notif_cmd.label,
                                  p_data->reg_notif_cmd.reg_notif_cmd.event_id,
                                  BSA_AVK_RSP_INTERIM);
            }
        }
        break;

#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
    case BSA_AVK_CAC_ENABLE_EVT:
        APP_DEBUG1("BSA_AVK_CAC_ENABLE_EVT status 0x%x", p_data->cac_enable_evt.status);
        break;

    case BSA_AVK_CAC_OPEN_EVT:
        APP_DEBUG1("BSA_AVK_CAC_OPEN_EVT status 0x%x", p_data->cac_open_evt.status);
        APP_DEBUG1("Device Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            p_data->cac_open_evt.bd_addr[0], p_data->cac_open_evt.bd_addr[1], p_data->cac_open_evt.bd_addr[2],
            p_data->cac_open_evt.bd_addr[3], p_data->cac_open_evt.bd_addr[4], p_data->cac_open_evt.bd_addr[5]);
        break;

    case BSA_AVK_CAC_CLOSE_EVT:
        APP_DEBUG1("BSA_AVK_CAC_CLOSE_EVT status 0x%x", p_data->cac_close_evt.status);
        APP_DEBUG1("Device Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            p_data->cac_close_evt.bd_addr[0], p_data->cac_close_evt.bd_addr[1], p_data->cac_close_evt.bd_addr[2],
            p_data->cac_close_evt.bd_addr[3], p_data->cac_close_evt.bd_addr[4], p_data->cac_close_evt.bd_addr[5]);
        break;

    case BSA_AVK_CAC_PROPERTIES_EVT:
        APP_DEBUG1("BSA_AVK_CAC_PROPERTIES_EVT status 0x%x", p_data->cac_prop_evt.status);
        if(p_data->cac_prop_evt.status == BSA_SUCCESS)
        {
            APP_DEBUG1("Native: size = %d", p_data->cac_prop_evt.native.size);
            APP_DEBUG1("Native: pixel w = %d, h = %d, w2 = %d, h2 = %d", p_data->cac_prop_evt.native.pixel.w,
                p_data->cac_prop_evt.native.pixel.h, p_data->cac_prop_evt.native.pixel.w2,
                p_data->cac_prop_evt.native.pixel.h2);
            APP_DEBUG1("Native: encoding = %s", p_data->cac_prop_evt.native.encoding);
            APP_DEBUG1("Actual number of items in variant = %d", p_data->cac_prop_evt.num_variant);
            if(p_data->cac_prop_evt.num_variant)
            {
                int idx;
                char *transform_display[] = {"NONE", "STRETCH", "FILL", "CROP"};
                for(idx = 0; idx < p_data->cac_prop_evt.num_variant; idx++)
                {
                    APP_DEBUG1("Variant[%d]: size = %d", idx, p_data->cac_prop_evt.variant[idx].size);
                    APP_DEBUG1("Variant[%d]: pixel w = %d, h = %d, w2 = %d, h2 = %d", idx,
                        p_data->cac_prop_evt.variant[idx].pixel.w, p_data->cac_prop_evt.variant[idx].pixel.h,
                        p_data->cac_prop_evt.variant[idx].pixel.w2, p_data->cac_prop_evt.variant[idx].pixel.h2);
                    APP_DEBUG1("Variant[%d]: encoding = %s", idx, p_data->cac_prop_evt.variant->encoding);
                    APP_DEBUG1("Variant[%d]: transform = %s", idx, transform_display[p_data->cac_prop_evt.variant->transform]);
                }
            }
            APP_DEBUG1("Image Handle = %s", p_data->cac_prop_evt.handle);
            APP_DEBUG1("Friendly Name = %s", p_data->cac_prop_evt.f_name);
        }
        break;

    case BSA_AVK_CAC_PROGRESS_EVT:
        APP_DEBUG1("BSA_AVK_CAC_PROGRESS_EVT, Number Of Bytes Read Or Written %d", p_data->cac_prog_evt.bytes);
        break;

    case BSA_AVK_CAC_GET_IMAGE_EVT:
        APP_DEBUG1("BSA_AVK_CAC_GET_IMAGE_EVT status 0x%x", p_data->cac_get_img_evt.status);
        break;

    case BSA_AVK_CAC_GET_THUMB_EVT:
        APP_DEBUG1("BSA_AVK_CAC_GET_THUMB_EVT status 0x%x", p_data->cac_get_thumb_evt.status);
        break;

    case BSA_AVK_CAC_ABORT_EVT:
        APP_DEBUG1("BSA_AVK_CAC_ABORT_EVT status 0x%x", p_data->cac_abort_evt.status);
        if(p_data->cac_abort_evt.p_file != NULL)
            APP_DEBUG1("File Name: %s", p_data->cac_abort_evt.p_file);
        break;

    case BSA_AVK_CAC_ERROR_EVT:
        APP_DEBUG1("BSA_AVK_CAC_ERROR_EVT 0x%x", p_data->cac_error_evt.status);
        break;
#endif

    default:
        APP_ERROR1("Unsupported event %d", event);
        break;
    }

    /* forward the callback to registered applications */
    if(app_avk_cb.p_Callback)
        app_avk_cb.p_Callback(event, p_data);
}

/*******************************************************************************
 **
 ** Function         app_avk_open
 **
 ** Description      Example of function to open AVK connection
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_open(void)
{
    tBSA_STATUS status;
    int choice;
    BOOLEAN connect = FALSE;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_AVK_OPEN open_param;
    tAPP_AVK_CONNECTION *connection = NULL;

    if (app_avk_cb.open_pending)
    {
        APP_ERROR0("already trying to connect");
        return;
    }

    printf("Bluetooth AVK Open menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
    choice = app_get_choice("Select source");

    switch(choice)
    {
    case 0:
        /* Read the XML file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        choice = app_get_choice("Select device");
        if ((choice >= 0) && (choice < APP_NUM_ELEMENTS(app_xml_remote_devices_db)))
        {
            if (app_xml_remote_devices_db[choice].in_use != FALSE)
            {
                bdcpy(bd_addr, app_xml_remote_devices_db[choice].bd_addr);
                p_name = app_xml_remote_devices_db[choice].name;
                connect = TRUE;
            }
            else
            {
                APP_ERROR0("Device entry not in use");
            }
        }
        else
        {
            APP_ERROR0("Unsupported device number");
        }
        break;
    case 1:
        app_disc_display_devices();
        choice = app_get_choice("Select device");
        if ((choice >= 0) && (choice < APP_NUM_ELEMENTS(app_discovery_cb.devs)))
        {
            if (app_discovery_cb.devs[choice].in_use != FALSE)
            {
                bdcpy(bd_addr, app_discovery_cb.devs[choice].device.bd_addr);
                p_name = app_discovery_cb.devs[choice].device.name;
                connect = TRUE;
            }
            else
            {
                APP_ERROR0("Device entry not in use");
            }
        }
        else
        {
            APP_ERROR0("Unsupported device number");
        }
        break;
    default:
        APP_ERROR0("Unsupported choice");
        break;
    }

    if (connect != FALSE)
    {
        /* Open AVK stream */
        printf("Connecting to AV device:%s \n", p_name);

        app_avk_cb.open_pending = TRUE;
        memcpy(app_avk_cb.open_pending_bda, bd_addr, sizeof(BD_ADDR));

        BSA_AvkOpenInit(&open_param);
        memcpy((char *) (open_param.bd_addr), bd_addr, sizeof(BD_ADDR));

        open_param.sec_mask = BSA_SEC_NONE;
        status = BSA_AvkOpen(&open_param);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("Unable to connect to device %02X:%02X:%02X:%02X:%02X:%02X with status %d",
                    open_param.bd_addr[0], open_param.bd_addr[1], open_param.bd_addr[2],
                    open_param.bd_addr[3], open_param.bd_addr[4], open_param.bd_addr[5], status);

            app_avk_cb.open_pending = FALSE;
            memset(app_avk_cb.open_pending_bda, 0, sizeof(BD_ADDR));
        }
        else
        {
            /* this is an active wait for demo purpose */
            printf("waiting for AV connection to open\n");

            while (app_avk_cb.open_pending == TRUE);

            connection = app_avk_find_connection_by_bd_addr(open_param.bd_addr);
            if(connection == NULL || connection->is_open == FALSE)
            {
                printf("failure opening AV connection  \n");
            }
            else
            {
                /* XML Database update should be done upon reception of AV OPEN event */
                APP_DEBUG1("Connected to AV device:%s", p_name);
                /* Read the Remote device xml file to have a fresh view */
                app_read_xml_remote_devices();

                /* Add AV service for this devices in XML database */
                app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr,
                    BSA_A2DP_SERVICE_MASK | BSA_AVRCP_SERVICE_MASK);

                app_xml_update_name_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr, p_name);

                /* Update database => write to disk */
                if (app_write_xml_remote_devices() < 0)
                {
                    APP_ERROR0("Failed to store remote devices database");
                }
            }
        }
    }

}

/*******************************************************************************
 **
 ** Function         app_avk_close
 **
 ** Description      Function to close AVK connection
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_close(BD_ADDR bda)
{
    tBSA_STATUS status;
    tBSA_AVK_CLOSE bsa_avk_close_param;
    tAPP_AVK_CONNECTION *connection = NULL;

    /* Close AVK connection */
    BSA_AvkCloseInit(&bsa_avk_close_param);

    connection = app_avk_find_connection_by_bd_addr(bda);

    if(connection == NULL)
    {
        APP_ERROR0("Unable to Close AVK connection , invalid BDA");
        return;
    }

    bsa_avk_close_param.ccb_handle = connection->ccb_handle;
    bsa_avk_close_param.rc_handle = connection->rc_handle;

    status = BSA_AvkClose(&bsa_avk_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Close AVK connection with status %d", status);
    }
    else
    {
        app_avk_reset_connection(bda);
    }

}

/*******************************************************************************
 **
 ** Function         app_avk_close_all
 **
 ** Description      Function to close all AVK connections
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_close_all()
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if (app_avk_cb.connections[index].in_use == TRUE)
        {
            app_avk_close(app_avk_cb.connections[index].bda_connected);
        }
    }
}

/*******************************************************************************
**
** Function         app_avk_open_rc
**
** Description      Function to opens avrc controller connection. AVK should be open before opening rc.
**
** Returns          void
**
*******************************************************************************/
void app_avk_open_rc(BD_ADDR bd_addr)
{
    tBSA_STATUS status;
    tBSA_AVK_OPEN bsa_avk_open_param;

    /* open avrc connection */
    BSA_AvkOpenRcInit(&bsa_avk_open_param);
    bdcpy(bsa_avk_open_param.bd_addr, bd_addr);
    status = BSA_AvkOpenRc(&bsa_avk_open_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to open arvc connection with status %d", status);
    }

}

/*******************************************************************************
**
** Function         app_avk_close_rc
**
** Description      Function to closes avrc controller connection.
**
** Returns          void
**
*******************************************************************************/
void app_avk_close_rc(UINT8 rc_handle)
{
    tBSA_STATUS status;
    tBSA_AVK_CLOSE bsa_avk_close_param;

    /* close avrc connection */
    BSA_AvkCloseRcInit(&bsa_avk_close_param);

    bsa_avk_close_param.rc_handle = rc_handle;

    status = BSA_AvkCloseRc(&bsa_avk_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to close arvc connection with status %d", status);
    }

}

/*******************************************************************************
**
** Function         app_avk_close_str
**
** Description      Function to close an A2DP Steam connection
**
** Returns          void
**
*******************************************************************************/
void app_avk_close_str(UINT8 ccb_handle)
{
    tBSA_STATUS status;
    tBSA_AVK_CLOSE_STR bsa_avk_close_str_param;

    /* close avrc connection */
    BSA_AvkCloseStrInit(&bsa_avk_close_str_param);

    bsa_avk_close_str_param.ccb_handle = ccb_handle;

    status = BSA_AvkCloseStr(&bsa_avk_close_str_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to close arvc connection with status %d", status);
    }

}

/*******************************************************************************
**
** Function        app_avk_register
**
** Description     Example of function to register an avk endpoint
**
** Returns         void
**
*******************************************************************************/
int app_avk_register(void)
{
    tBSA_STATUS status;
    UINT8 index;
    tBSA_AVK_REGISTER bsa_avk_register_param;

    /* register an audio AVK end point */
    BSA_AvkRegisterInit(&bsa_avk_register_param);

    bsa_avk_register_param.media_sup_format.audio.pcm_supported = TRUE;
    bsa_avk_register_param.media_sup_format.audio.sec_supported = TRUE;
#if (defined(BSA_AVK_AAC_SUPPORTED) && (BSA_AVK_AAC_SUPPORTED==TRUE))
    /* Enable AAC support in the app */
    bsa_avk_register_param.media_sup_format.audio.aac_supported = TRUE;
#endif
    strncpy(bsa_avk_register_param.service_name, "BSA Audio Service",
        BSA_AVK_SERVICE_NAME_LEN_MAX);

    bsa_avk_register_param.reg_notifications = reg_notifications;

    for(index = 0; index < BSA_MAX_AVK_CONNECTIONS; index++)
    {
        status = BSA_AvkRegister(&bsa_avk_register_param);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("Unable to register an AV sink with status %d", status);

            if(BSA_ERROR_SRV_AVK_AV_REGISTERED == status)
            {
                APP_ERROR0("AV was registered before AVK, AVK should be registered before AV");
            }
            return -1;
        }
    }

    return 0;

}

/*******************************************************************************
 **
 ** Function        app_avk_deregister
 **
 ** Description     Example of function to deregister an avk endpoint
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_avk_deregister(void)
{
    tBSA_STATUS status;
    tBSA_AVK_DEREGISTER req;

    /* register an audio AVK end point */
    BSA_AvkDeregisterInit(&req);
    status = BSA_AvkDeregister(&req);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to deregister an AV sink with status %d", status);
    }

#ifdef TODO
    if (app_avk_cb.format == BSA_AVK_CODEC_M24)
    {
        close(app_avk_cb.fd);
        app_avk_cb.fd  = -1;
    }
    else
        app_avk_close_wave_file();
#endif

    /* close the UIPC channel receiving the pcm */
    tUIPC_CH_ID chn = app_avk_cb.uipc_audio_channel;
    app_avk_cb.uipc_audio_channel = UIPC_CH_ID_BAD;
    UIPC_Close(chn);
}

/*******************************************************************************
**
** Function        app_avk_uipc_cback
**
** Description     uipc audio call back function.
**
** Parameters      pointer on a buffer containing PCM sample with a BT_HDR header
**
** Returns          void
**
*******************************************************************************/
static void app_avk_uipc_cback(BT_HDR *p_msg)
{
#ifdef PCM_ALSA
    snd_pcm_sframes_t alsa_frames;
    snd_pcm_sframes_t alsa_frames_to_send;
#endif
    UINT8 *p_buffer;
    int dummy;
    tAPP_AVK_CONNECTION *connection = NULL;

    if (p_msg == NULL)
    {
        return;
    }

    p_buffer = ((UINT8 *)(p_msg + 1)) + p_msg->offset;

    connection = app_avk_find_streaming_connection();

    if(!connection)
    {
        if(app_avk_cb.uipc_audio_channel != UIPC_CH_ID_BAD)
        APP_ERROR0("Unable to find connection!!!!!!");
        GKI_freebuf(p_msg);
        return;
    }

    if (connection->format == BSA_AVK_CODEC_M24)
    {
        /* Invoke AAC decoder here for the current buffer.
            decode the AAC file that is created */
        if (app_avk_cb.fd != -1)
        {
            dummy = write(app_avk_cb.fd, p_buffer, p_msg->len);
            (void)dummy;
        }

        GKI_freebuf(p_msg);
        return;
    }

    if (connection->format == BSA_AVK_CODEC_PCM &&
            app_avk_cb.fd_codec_type != BSA_AVK_CODEC_M24)
    {
        /* Invoke AAC decoder here for the current buffer.
            decode the AAC file that is created */
        if (app_avk_cb.fd != -1)
        {
#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
            APP_DEBUG1("app_avk_uipc_cback Writing Data 0x%x, len = %d", p_buffer, p_msg->len);
#endif
            dummy = write(app_avk_cb.fd, p_buffer, p_msg->len);
            (void)dummy;
        }

#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
        APP_DUMP("A2DP Data", p_buffer, p_msg->len);
#endif
    }

    if(connection->num_channel == 0)
    {
        APP_ERROR0("connection->num_channel == 0");
        GKI_freebuf(p_msg);
        return;
    }

#ifdef PCM_ALSA
    if (app_avk_cb.alsa_handle != NULL && p_buffer)
    {
        /* Compute number of PCM samples (contained in p_msg->len bytes) */
        alsa_frames_to_send = p_msg->len / connection->num_channel;
        if (connection->bit_per_sample == 16)
            alsa_frames_to_send /= 2;

#ifdef PCM_ALSA_OPEN_BLOCKING
        while (alsa_frames_to_send > 0)
        {
            pthread_mutex_lock(&mutex);
            if (!app_avk_cb.alsa_handle)
            {
                pthread_mutex_unlock(&mutex);
                break;
            }

            alsa_frames = snd_pcm_writei(app_avk_cb.alsa_handle, p_buffer, alsa_frames_to_send);
            if (alsa_frames < 0)
            {
                if (alsa_frames == -EPIPE)
                {
                    APP_DEBUG0("ALSA: underrun in frame");
                    snd_pcm_prepare(app_avk_cb.alsa_handle);
                    alsa_frames = 0;
                }
                else if (alsa_frames == -EBADFD)
                {
                    APP_DEBUG0("ALSA: retry");
                    alsa_frames = 0;
                }
                else
                {
                    APP_DEBUG1("ALSA: snd_pcm_writei err %d (%s)", (int) alsa_frames, snd_strerror(alsa_frames));
                    pthread_mutex_unlock(&mutex);
                    break;
                }
            }

            pthread_mutex_unlock(&mutex);
            alsa_frames_to_send -= alsa_frames;
        }
        if (alsa_frames_to_send)
            APP_DEBUG1("ALSA: short write (discarded %li)", alsa_frames_to_send);
    }
#else
        alsa_frames = snd_pcm_writei(app_avk_cb.alsa_handle, p_buffer, alsa_frames_to_send);
        if (alsa_frames < 0)
        {
            alsa_frames = snd_pcm_recover(app_avk_cb.alsa_handle, alsa_frames, 0);
        }
        if (alsa_frames < 0)
        {
            APP_DEBUG1("app_avk_uipc_cback snd_pcm_writei failed %s", snd_strerror(alsa_frames));
        }
        if (alsa_frames > 0 && alsa_frames < alsa_frames_to_send)
            APP_DEBUG1("app_avk_uipc_cback Short write (expected %li, wrote %li)",
                (long) alsa_frames_to_send, alsa_frames);
    }
    else
        APP_DEBUG0("app_avk_uipc_cback snd_pcm_writei failed FINALLY !!");
#endif /* PCM_ALSA_OPEN_BLOCKING */
#endif /* PCM_ALSA */
    GKI_freebuf(p_msg);
}

/*******************************************************************************
**
** Function         app_avk_init
**
** Description      Init Manager application
**
** Parameters       Application callback (if null, default will be used)
**
** Returns          0 if successful, error code otherwise
**
*******************************************************************************/
int app_avk_init(tBSA_AVK_CBACK pcb)
{
    tBSA_AVK_ENABLE bsa_avk_enable_param;
    tBSA_STATUS status;

    /* Initialize the control structure */
    memset(&app_avk_cb, 0, sizeof(app_avk_cb));
    app_avk_cb.uipc_audio_channel = UIPC_CH_ID_BAD;

    app_avk_cb.fd = -1;

    /* register callback */
    app_avk_cb.p_Callback = pcb;

    /* set sytem vol at 50% */
    app_avk_cb.volume = (UINT8)((BSA_MAX_ABS_VOLUME - BSA_MIN_ABS_VOLUME)>>1);

    /* get hold on the AVK resource, synchronous mode */
    BSA_AvkEnableInit(&bsa_avk_enable_param);

    bsa_avk_enable_param.sec_mask = BSA_AVK_SECURITY;
    bsa_avk_enable_param.features = BSA_AVK_FEATURES;
    bsa_avk_enable_param.rc_features = BSA_RC_FEATURES;
    bsa_avk_enable_param.p_cback = app_avk_cback;

    status = BSA_AvkEnable(&bsa_avk_enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable an AV sink with status %d", status);

        if(BSA_ERROR_SRV_AVK_AV_REGISTERED == status)
        {
            APP_ERROR0("AV was registered before AVK, AVK should be registered before AV");
        }

        return -1;
    }

    return BSA_SUCCESS;

}

/*******************************************************************************
**
** Function         app_avk_rc_send_command
**
** Description      Example of Send a RemoteControl command
**
** Returns          void
**
*******************************************************************************/
void app_avk_rc_send_command(UINT8 key_state, UINT8 command, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_REM_CMD bsa_avk_rc_cmd;

    /* Send remote control command */
    status = BSA_AvkRemoteCmdInit(&bsa_avk_rc_cmd);
    bsa_avk_rc_cmd.rc_handle = rc_handle;
    bsa_avk_rc_cmd.key_state = (tBSA_AVK_STATE)key_state;
    bsa_avk_rc_cmd.rc_id = (tBSA_AVK_RC)command;
    bsa_avk_rc_cmd.label = app_avk_get_label();

    status = BSA_AvkRemoteCmd(&bsa_avk_rc_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send AV RC Command %d", status);
    }
}

/*******************************************************************************
**
** Function         app_avk_rc_send_click
**
** Description      Send press and release states of a command
**
** Returns          void
**
*******************************************************************************/
void app_avk_rc_send_click(UINT8 command, UINT8 rc_handle)
{
    app_avk_rc_send_command(BSA_AV_STATE_PRESS, command, rc_handle);
    GKI_delay(300);
    app_avk_rc_send_command(BSA_AV_STATE_RELEASE, command, rc_handle);
}


/*******************************************************************************
**
** Function         app_avk_play_start
**
** Description      Example of start steam play
**
** Returns          void
**
*******************************************************************************/
void app_avk_play_start(UINT8 rc_handle)
{
    tAPP_AVK_CONNECTION *connection = NULL;
    connection = app_avk_find_connection_by_rc_handle(rc_handle);
    if(connection == NULL)
    {
        APP_DEBUG1("app_avk_play_start could not find connection handle %d", rc_handle);
        return;
    }

    if ((connection->peer_features & BSA_RC_FEAT_RCTG) && connection->is_rc_open)
    {
        app_avk_rc_send_click(BSA_AVK_RC_PLAY, rc_handle);
    }
    else
    {
        APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
            (connection->peer_features & BSA_RC_FEAT_RCTG), connection->is_rc_open);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_play_stop
 **
 ** Description      Example of stop steam play
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_stop(UINT8 rc_handle)
{
    tAPP_AVK_CONNECTION *connection = NULL;
    connection = app_avk_find_connection_by_rc_handle(rc_handle);
    if(connection == NULL)
    {
        APP_DEBUG1("app_avk_play_stop could not find connection handle %d", rc_handle);
        return;
    }

    if ((connection->peer_features & BSA_RC_FEAT_RCTG) && connection->is_rc_open)
    {
        app_avk_rc_send_click(BSA_AVK_RC_STOP, rc_handle);
    }
    else
    {
        APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
            (connection->peer_features & BSA_RC_FEAT_RCTG), connection->is_rc_open);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_play_pause
 **
 ** Description      Example of pause steam pause
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_pause(UINT8 rc_handle)
{
    tAPP_AVK_CONNECTION *connection = NULL;
    connection = app_avk_find_connection_by_rc_handle(rc_handle);
    if(connection == NULL)
    {
        APP_DEBUG1("app_avk_play_pause could not find connection handle %d", rc_handle);
        return;
    }

    if ((connection->peer_features & BSA_RC_FEAT_RCTG) && connection->is_rc_open)
    {
        app_avk_rc_send_click(BSA_AVK_RC_PAUSE, rc_handle);
    }
    else
    {
        APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
                    (connection->peer_features & BSA_RC_FEAT_RCTG), connection->is_rc_open);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_play_next_track
 **
 ** Description      Example of play next track
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_next_track(UINT8 rc_handle)
{
    tAPP_AVK_CONNECTION *connection = NULL;
    connection = app_avk_find_connection_by_rc_handle(rc_handle);
    if(connection == NULL)
    {
        APP_DEBUG1("app_avk_play_pause could not find connection handle %d", rc_handle);
        return;
    }

    if ((connection->peer_features & BSA_RC_FEAT_RCTG) && connection->is_rc_open)
    {
        app_avk_rc_send_click(BSA_AVK_RC_FORWARD, rc_handle);
    }
    else
    {
        APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
                    (connection->peer_features & BSA_RC_FEAT_RCTG), connection->is_rc_open);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_play_previous_track
 **
 ** Description      Example of play previous track
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_previous_track(UINT8 rc_handle)
{
    tAPP_AVK_CONNECTION *connection = NULL;
    connection = app_avk_find_connection_by_rc_handle(rc_handle);
    if(connection == NULL)
    {
        APP_DEBUG1("app_avk_play_pause could not find connection handle %d", rc_handle);
        return;
    }

    if ((connection->peer_features & BSA_RC_FEAT_RCTG) && connection->is_rc_open)
    {
        app_avk_rc_send_click(BSA_AVK_RC_BACKWARD, rc_handle);
    }
    else
    {
        APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
                    (connection->peer_features & BSA_RC_FEAT_RCTG), connection->is_rc_open);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_cmd
 **
 ** Description      Example of function to open AVK connection
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_cmd(UINT8 rc_handle)
{
    int choice;
    int value;

    do
    {
        printf("Bluetooth AVK AVRC CMD menu:\n");
        printf("    0 play\n");
        printf("    1 stop\n");
        printf("    2 pause\n");
        printf("    3 forward\n");
        printf("    4 backward\n");
        printf("    5 angle\n");
        printf("    6 rewind key_press\n");
        printf("    7 rewind key_release\n");
        printf("    8 fast forward key_press\n");
        printf("    9 fast forward key_release\n");
        printf("    10 eject key press\n");
        printf("    11 eject key release\n");
        printf("    12 subpicture\n");
        printf("    13 0 key\n");
        printf("    14 1 key\n");
        printf("    15 2 key\n");
        printf("    16 3 key\n");
        printf("    17 4 key\n");
        printf("    18 5 key\n");
        printf("    19 mute key\n");
        printf("    20 raw value\n");
        printf("    99 exit\n");

        choice = app_get_choice("Select source");

        switch(choice)
        {
            case 0:
                app_avk_rc_send_click(BSA_AVK_RC_PLAY, rc_handle);
                break;

            case 1:
                app_avk_rc_send_click(BSA_AVK_RC_STOP, rc_handle);
                break;

            case 2:
                app_avk_rc_send_click(BSA_AVK_RC_PAUSE, rc_handle);
                break;

            case 3:
                app_avk_rc_send_click(BSA_AVK_RC_FORWARD, rc_handle);
                break;

            case 4:
                app_avk_rc_send_click(BSA_AVK_RC_BACKWARD, rc_handle);
                break;

            case 5:
                app_avk_rc_send_click(BSA_AVK_RC_ANGLE, rc_handle);
                break;

            case 6:
                app_avk_rc_send_command(BSA_AV_STATE_PRESS, BSA_AVK_RC_REWIND, rc_handle);
                break;

            case 7:
                app_avk_rc_send_command(BSA_AV_STATE_RELEASE, BSA_AVK_RC_REWIND, rc_handle);
                break;

            case 8:
                app_avk_rc_send_command(BSA_AV_STATE_PRESS, BSA_AVK_RC_FAST_FOR, rc_handle);
                break;

            case 9:
                app_avk_rc_send_command(BSA_AV_STATE_RELEASE, BSA_AVK_RC_FAST_FOR, rc_handle);
                break;

            case 10:
                app_avk_rc_send_command(BSA_AV_STATE_PRESS, BSA_AVK_RC_EJECT, rc_handle);
                break;

            case 11:
                app_avk_rc_send_command(BSA_AV_STATE_RELEASE, BSA_AVK_RC_EJECT, rc_handle);
                break;

            case 12:
                app_avk_rc_send_click(BSA_AVK_RC_SUBPICT, rc_handle);
                break;

            case 13:
                app_avk_rc_send_click(BSA_AVK_RC_0, rc_handle);
                break;

            case 14:
                app_avk_rc_send_click(BSA_AVK_RC_1, rc_handle);
                break;

            case 15:
                app_avk_rc_send_click(BSA_AVK_RC_2, rc_handle);
                break;

            case 16:
                app_avk_rc_send_click(BSA_AVK_RC_3, rc_handle);
                break;

            case 17:
                app_avk_rc_send_click(BSA_AVK_RC_4, rc_handle);
                break;

            case 18:
                app_avk_rc_send_click(BSA_AVK_RC_5, rc_handle);
                break;

            case 19:
                app_avk_rc_send_click(BSA_AVK_RC_MUTE, rc_handle);
                break;

            case 20:
                value = app_get_choice("value to press");
                app_avk_rc_send_click(value, rc_handle);
                break;

            default:
                APP_ERROR0("Unsupported choice");
            break;
        }
    } while (choice != 99);
}


/*******************************************************************************
 **
 ** Function         app_avk_send_get_element_attributes_cmd
 **
 ** Description      Example of function to send Vendor Dependent Command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_get_element_attributes_cmd(UINT8 rc_handle)
{
    int status;
    tBSA_AVK_VEN_CMD bsa_avk_vendor_cmd;

    /* Send remote control command */
    status = BSA_AvkVendorCmdInit(&bsa_avk_vendor_cmd);

    bsa_avk_vendor_cmd.rc_handle = rc_handle;
    bsa_avk_vendor_cmd.ctype = BSA_AVK_CMD_STATUS;
    bsa_avk_vendor_cmd.data[0] = BSA_AVK_RC_VD_GET_ELEMENT_ATTR;
    bsa_avk_vendor_cmd.data[1] = 0; /* reserved & packet type */
    bsa_avk_vendor_cmd.data[2] = 0; /* length high*/
    bsa_avk_vendor_cmd.data[3] = 0x11; /* length low*/

    /* data 4 ~ 11 are 0, which means "identifier 0x0 PLAYING" */

    bsa_avk_vendor_cmd.data[12] = 2; /* num of attributes */

    /* data 13 ~ 16 are 0x1, which means "attribute ID 1 : Title of media" */
    bsa_avk_vendor_cmd.data[16] = 0x1;

    /* data 17 ~ 20 are 0x7, which means "attribute ID 2 : Playing Time" */
    bsa_avk_vendor_cmd.data[20] = 0x7;

    bsa_avk_vendor_cmd.length = 21;
    bsa_avk_vendor_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkVendorCmd(&bsa_avk_vendor_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send AV Vendor Command %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_send_set_absolute_volume_cmd
 **
 ** Description      Example of function to send Vendor Dependent Command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_set_absolute_volume_cmd(UINT8 vol, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_VEN_CMD bsa_avk_vendor_cmd;

    /* Send remote control command */
    status = BSA_AvkVendorCmdInit(&bsa_avk_vendor_cmd);

    bsa_avk_vendor_cmd.rc_handle = rc_handle;
    bsa_avk_vendor_cmd.ctype = BSA_AVK_CMD_CTRL;
    bsa_avk_vendor_cmd.subunit_type = AVRC_SUB_PANEL;
    bsa_avk_vendor_cmd.data[0] = BSA_AVK_RC_VD_SET_ABSOLUTE_VOLUME;
    bsa_avk_vendor_cmd.data[1] = 0; /* reserved & packet type */
    bsa_avk_vendor_cmd.data[2] = 0; /* length high*/
    bsa_avk_vendor_cmd.data[3] = 0x1; /* length low*/

    /* data 4 : absolute volume(7 bit, 6 ~ 0 ) */

    bsa_avk_vendor_cmd.data[4] = vol & 0x3F;
    bsa_avk_vendor_cmd.length = 5;
    bsa_avk_vendor_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkVendorCmd(&bsa_avk_vendor_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send AV Vendor Command %d", status);
    }
}

/*******************************************************************************
**
** Function         avk_is_open_pending
**
** Description      Check if AVK Open is pending
**
** Parameters
**
** Returns          TRUE if open is pending, FALSE otherwise
**
*******************************************************************************/
BOOLEAN avk_is_open_pending()
{
    return app_avk_cb.open_pending;
}

/*******************************************************************************
**
** Function         avk_set_open_pending
**
** Description      Set AVK open pending
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void avk_set_open_pending(BOOLEAN bopenpend)
{
    app_avk_cb.open_pending = bopenpend;
}

/*******************************************************************************
**
** Function         avk_is_open
**
** Description      Check if AVK is open
**
** Parameters       BDA of device to check
**
** Returns          TRUE if AVK is open, FALSE otherwise.
**
*******************************************************************************/
BOOLEAN avk_is_open(BD_ADDR bda)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if ((app_avk_cb.connections[index].in_use == TRUE) &&
                (app_avk_cb.connections[index].is_open == TRUE))
    {
            if(bdcmp(bda, app_avk_cb.connections[index].bda_connected) == 0)
                return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         avk_is_any_open
**
** Description      Check if any AVK is open
**
** Parameters
**
** Returns          TRUE if AVK is open, FALSE otherwise. Return BDA of the connected device
**
*******************************************************************************/
BOOLEAN avk_is_any_open(BD_ADDR bda /* out */)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if ((app_avk_cb.connections[index].in_use == TRUE) &&
                (app_avk_cb.connections[index].is_open == TRUE))
    {
            bdcpy(bda, app_avk_cb.connections[index].bda_connected);
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         avk_is_rc_open
**
** Description      Check if AVRC is open
**
** Parameters
**
** Returns          TRUE if AVRC is open, FALSE otherwise.
**
*******************************************************************************/
BOOLEAN avk_is_rc_open(BD_ADDR bda)
{
    tAPP_AVK_CONNECTION *connection = app_avk_find_connection_by_bd_addr(bda);

    if(connection == NULL)
        return FALSE;

    return connection->is_rc_open;
}

/*******************************************************************************
**
** Function         app_avk_cancel
**
** Description      Example of cancel connection command
**
** Returns          void
**
*******************************************************************************/
void app_avk_cancel(BD_ADDR bda)
{
    tBSA_STATUS status;
    tBSA_AVK_CANCEL_CMD bsa_avk_cancel_cmd;

    /* Send remote control command */
    status = BSA_AvkCancelCmdInit(&bsa_avk_cancel_cmd);

    bdcpy(bsa_avk_cancel_cmd.bda, bda);

    status = BSA_AvkCancelCmd(&bsa_avk_cancel_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_cancel_command %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_element_attr_command
 **
 ** Description      Example of get_element_attr_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_element_attr_command(UINT8 rc_handle)
{
    int status;
    tBSA_AVK_GET_ELEMENT_ATTR bsa_avk_get_elem_attr_cmd;
#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
    UINT32 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
                      AVRC_MEDIA_ATTR_ID_ARTIST,
                      AVRC_MEDIA_ATTR_ID_ALBUM,
                      AVRC_MEDIA_ATTR_ID_TRACK_NUM,
                      AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
                      AVRC_MEDIA_ATTR_ID_GENRE,
                      AVRC_MEDIA_ATTR_ID_PLAYING_TIME,
                      AVRC_MEDIA_ATTR_ID_COVER_ART};

    UINT8 num_attr  = 8;
#else
    UINT32 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
                      AVRC_MEDIA_ATTR_ID_ARTIST,
                      AVRC_MEDIA_ATTR_ID_ALBUM,
                      AVRC_MEDIA_ATTR_ID_TRACK_NUM,
                      AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
                      AVRC_MEDIA_ATTR_ID_GENRE,
                      AVRC_MEDIA_ATTR_ID_PLAYING_TIME};

    UINT8 num_attr  = 7;
#endif
    /* Send command */
    status = BSA_AvkGetElementAttrCmdInit(&bsa_avk_get_elem_attr_cmd);
    bsa_avk_get_elem_attr_cmd.rc_handle = rc_handle;
    bsa_avk_get_elem_attr_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

    bsa_avk_get_elem_attr_cmd.num_attr = num_attr;
    memcpy(bsa_avk_get_elem_attr_cmd.attrs, attrs, sizeof(attrs));

    status = BSA_AvkGetElementAttrCmd(&bsa_avk_get_elem_attr_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_get_element_attr_command %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_list_player_attr_command
 **
 ** Description      Example of app_avk_rc_list_player_attr_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_list_player_attr_command(UINT8 rc_handle)
{
    int status;
    tBSA_AVK_LIST_PLAYER_ATTR bsa_avk_list_player_attr_cmd;

    /* Send command */
    status = BSA_AvkListPlayerAttrCmdInit(&bsa_avk_list_player_attr_cmd);
    bsa_avk_list_player_attr_cmd.rc_handle = rc_handle;
    bsa_avk_list_player_attr_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkListPlayerAttrCmd(&bsa_avk_list_player_attr_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_list_player_attr_command %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_list_player_attr_value_command
 **
 ** Description      Example of app_avk_rc_list_player_attr_value_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_list_player_attr_value_command(UINT8 attr, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_LIST_PLAYER_VALUES bsa_avk_list_player_attr_cmd;

    /* Send command */
    status = BSA_AvkListPlayerValuesCmdInit(&bsa_avk_list_player_attr_cmd);
    bsa_avk_list_player_attr_cmd.rc_handle = rc_handle;
    bsa_avk_list_player_attr_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */
    bsa_avk_list_player_attr_cmd.attr = attr;

    status = BSA_AvkListPlayerValuesCmd(&bsa_avk_list_player_attr_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_list_player_attr_command %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_player_value_command
 **
 ** Description      Example of get_player_value_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_player_value_command(UINT8 *attrs, UINT8 num_attr, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_GET_PLAYER_VALUE bsa_avk_get_player_val_cmd;

    /* Send command */
    status = BSA_AvkGetPlayerValueCmdInit(&bsa_avk_get_player_val_cmd);
    bsa_avk_get_player_val_cmd.rc_handle = rc_handle;
    bsa_avk_get_player_val_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */
    bsa_avk_get_player_val_cmd.num_attr = num_attr;

    memcpy(bsa_avk_get_player_val_cmd.attrs, attrs, sizeof(UINT8) * num_attr);

    status = BSA_AvkGetPlayerValueCmd(&bsa_avk_get_player_val_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_get_player_value_command %d", status);
    }
}



/*******************************************************************************
 **
 ** Function         app_avk_rc_set_player_value_command
 **
 ** Description      Example of set_player_value_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_set_player_value_command(UINT8 num_attr, UINT8 *attr_ids, UINT8 * attr_val, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_SET_PLAYER_VALUE bsa_avk_set_player_val_cmd;

    /* Send command */
    status = BSA_AvkSetPlayerValueCmdInit(&bsa_avk_set_player_val_cmd);
    bsa_avk_set_player_val_cmd.rc_handle = rc_handle;
    bsa_avk_set_player_val_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */
    bsa_avk_set_player_val_cmd.num_attr = num_attr;

    memcpy(bsa_avk_set_player_val_cmd.player_attr, attr_ids, sizeof(UINT8) * num_attr);
    memcpy(bsa_avk_set_player_val_cmd.player_value, attr_val, sizeof(UINT8) * num_attr);

    status = BSA_AvkSetPlayerValueCmd(&bsa_avk_set_player_val_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_set_player_value_command %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_play_status_command
 **
 ** Description      Example of get_play_status
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_play_status_command(UINT8 rc_handle)
{
    int status;
    tBSA_AVK_GET_PLAY_STATUS bsa_avk_play_status_cmd;

    /* Send command */
    status = BSA_AvkGetPlayStatusCmdInit(&bsa_avk_play_status_cmd);
    bsa_avk_play_status_cmd.rc_handle = rc_handle;
    bsa_avk_play_status_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkGetPlayStatusCmd(&bsa_avk_play_status_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_get_play_status_command %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_set_browsed_player_command
 **
 ** Description      Example of set_browsed_player
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_set_browsed_player_command(UINT16  player_id, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_SET_BROWSED_PLAYER bsa_avk_set_browsed_player;

    /* Send command */
    status = BSA_AvkSetBrowsedPlayerCmdInit(&bsa_avk_set_browsed_player);
    bsa_avk_set_browsed_player.player_id = player_id;
    bsa_avk_set_browsed_player.rc_handle = rc_handle;
    bsa_avk_set_browsed_player.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkSetBrowsedPlayerCmd(&bsa_avk_set_browsed_player);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_set_browsed_player_command %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_set_addressed_player_command
 **
 ** Description      Example of set_addressed_player
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_set_addressed_player_command(UINT16  player_id, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_SET_ADDR_PLAYER bsa_avk_set_player;

    /* Send command */
    status = BSA_AvkSetAddressedPlayerCmdInit(&bsa_avk_set_player);
    bsa_avk_set_player.player_id = player_id;
    bsa_avk_set_player.rc_handle = rc_handle;
    bsa_avk_set_player.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkSetAddressedPlayerCmd(&bsa_avk_set_player);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_set_addressed_player_command %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_change_path_command
 **
 ** Description      Example of change_path
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_change_path_command(UINT16   uid_counter, UINT8  direction, tAVRC_UID folder_uid, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_CHG_PATH bsa_change_path;

    /* Send command */
    status = BSA_AvkChangePathCmdInit(&bsa_change_path);

    bsa_change_path.rc_handle = rc_handle;
    bsa_change_path.label = app_avk_get_label(); /* Just used to distinguish commands */

    bsa_change_path.uid_counter = uid_counter;
    bsa_change_path.direction = direction;
    if(folder_uid)
    {
        memcpy(bsa_change_path.folder_uid, folder_uid, sizeof(tAVRC_UID));
    }
    status = BSA_AvkChangePathCmd(&bsa_change_path);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_change_path_command %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_folder_items
 **
 ** Description      Example of get_folder_items
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_folder_items(UINT8  scope, UINT32  start_item, UINT32  end_item, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_GET_FOLDER_ITEMS bsa_get_folder_items;

#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
    UINT32 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
                      AVRC_MEDIA_ATTR_ID_ARTIST,
                      AVRC_MEDIA_ATTR_ID_ALBUM,
                      AVRC_MEDIA_ATTR_ID_TRACK_NUM,
                      AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
                      AVRC_MEDIA_ATTR_ID_GENRE,
                      AVRC_MEDIA_ATTR_ID_PLAYING_TIME,
                      AVRC_MEDIA_ATTR_ID_COVER_ART};

    UINT8 num_attr  = 8;
#else
    UINT32 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
                      AVRC_MEDIA_ATTR_ID_ARTIST,
                      AVRC_MEDIA_ATTR_ID_ALBUM,
                      AVRC_MEDIA_ATTR_ID_TRACK_NUM,
                      AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
                      AVRC_MEDIA_ATTR_ID_GENRE,
                      AVRC_MEDIA_ATTR_ID_PLAYING_TIME};

    UINT8 num_attr  = 7;
#endif

    /* Send command */
    status = BSA_AvkGetFolderItemsCmdInit(&bsa_get_folder_items);

    bsa_get_folder_items.rc_handle = rc_handle;
    bsa_get_folder_items.label = app_avk_get_label(); /* Just used to distinguish commands */

    bsa_get_folder_items.scope = scope;
    bsa_get_folder_items.start_item = start_item;
    bsa_get_folder_items.end_item = end_item;

    if(AVRC_SCOPE_PLAYER_LIST != scope)
    {
        bsa_get_folder_items.attr_count = num_attr;
        memcpy(bsa_get_folder_items.attrs, attrs, sizeof(attrs));
    }


    status = BSA_AvkGetFolderItemsCmd(&bsa_get_folder_items);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_get_folder_items %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_get_items_attr
 **
 ** Description      Example of get_items_attr
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_items_attr(UINT8  scope, tAVRC_UID  uid, UINT16  uid_counter, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_GET_ITEMS_ATTR bsa_get_items_attr;

#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
    UINT32 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
                      AVRC_MEDIA_ATTR_ID_ARTIST,
                      AVRC_MEDIA_ATTR_ID_ALBUM,
                      AVRC_MEDIA_ATTR_ID_TRACK_NUM,
                      AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
                      AVRC_MEDIA_ATTR_ID_GENRE,
                      AVRC_MEDIA_ATTR_ID_PLAYING_TIME,
                      AVRC_MEDIA_ATTR_ID_COVER_ART};

    UINT8 num_attr  = 8;
#else
    UINT32 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
                      AVRC_MEDIA_ATTR_ID_ARTIST,
                      AVRC_MEDIA_ATTR_ID_ALBUM,
                      AVRC_MEDIA_ATTR_ID_TRACK_NUM,
                      AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
                      AVRC_MEDIA_ATTR_ID_GENRE,
                      AVRC_MEDIA_ATTR_ID_PLAYING_TIME};

    UINT8 num_attr  = 7;
#endif


    /* Send command */
    status = BSA_AvkGetItemsAttrCmdInit(&bsa_get_items_attr);

    bsa_get_items_attr.rc_handle = rc_handle;
    bsa_get_items_attr.label = app_avk_get_label(); /* Just used to distinguish commands */

    bsa_get_items_attr.scope = scope;
    memcpy(bsa_get_items_attr.uid, uid, sizeof(tAVRC_UID));
    bsa_get_items_attr.uid_counter = uid_counter;
    bsa_get_items_attr.attr_count = num_attr;
    memcpy(bsa_get_items_attr.attrs, attrs, sizeof(attrs));

    status = BSA_AvkGetItemsAttrCmd(&bsa_get_items_attr);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_get_items_attr %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_play_item
 **
 ** Description      Example of play_item
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_play_item(UINT8  scope, tAVRC_UID  uid, UINT16  uid_counter, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_PLAY_ITEM bsa_play_item;


    /* Send command */
    status = BSA_AvkPlayItemCmdInit(&bsa_play_item);

    bsa_play_item.rc_handle = rc_handle;
    bsa_play_item.label = app_avk_get_label(); /* Just used to distinguish commands */

    bsa_play_item.scope = scope;
    memcpy(bsa_play_item.uid, uid, sizeof(tAVRC_UID));
    bsa_play_item.uid_counter = uid_counter;

    status = BSA_AvkPlayItemCmd(&bsa_play_item);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_play_item %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_add_to_play
 **
 ** Description      Example of add_to_play
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_add_to_play(UINT8  scope, tAVRC_UID  uid, UINT16  uid_counter, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_ADD_TO_PLAY bsa_add_play;


    /* Send command */
    status = BSA_AvkAddToPlayCmdInit(&bsa_add_play);

    bsa_add_play.rc_handle = rc_handle;
    bsa_add_play.label = app_avk_get_label(); /* Just used to distinguish commands */

    bsa_add_play.scope = scope;
    memcpy(bsa_add_play.uid, uid, sizeof(tAVRC_UID));
    bsa_add_play.uid_counter = uid_counter;

    status = BSA_AvkAddToPlayCmd(&bsa_add_play);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_rc_add_to_play %d", status);
    }
}

#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
/*******************************************************************************
 **
 ** Function         app_avk_cac_get_prop
 **
 ** Description      Example of cover art get property
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_cac_get_prop(tBSA_AVK_RC_CAC_IMG_HDL img_handle, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_CAC_GET_PROP bsa_get_prop;

    /* Send command */
    status = BSA_AvkCacGetPropInit(&bsa_get_prop);

    bsa_get_prop.rc_handle = rc_handle;
    memcpy(bsa_get_prop.img_handle, img_handle, sizeof(tBSA_AVK_RC_CAC_IMG_HDL));

    status = BSA_AvkCacGetProp(&bsa_get_prop);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_cac_get_prop %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_cac_get_img
 **
 ** Description      Example of cover art get image
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_cac_get_img(tBSA_AVK_RC_CAC_IMG_HDL img_handle, tBSA_AVK_RC_CAC_IMAGE_DESC *desc,
                UINT16 path_len, UINT8 *path, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_CAC_GET_IMG bsa_get_img;

    /* Send command */
    status = BSA_AvkCacGetImgInit(&bsa_get_img);

    bsa_get_img.rc_handle = rc_handle;
    memcpy(bsa_get_img.img_handle, img_handle, sizeof(tBSA_AVK_RC_CAC_IMG_HDL));
    if(desc == NULL)
    {
        memset(&bsa_get_img.desc, 0, sizeof(tBSA_AVK_RC_CAC_IMAGE_DESC));
    }
    else
    {
        memcpy(&bsa_get_img.desc, desc, sizeof(tBSA_AVK_RC_CAC_IMAGE_DESC));
    }
    bsa_get_img.path_len = path_len;
    memcpy(bsa_get_img.path, path, BSA_AVK_CAC_MAX_PATH_LEN);


    status = BSA_AvkCacGetImg(&bsa_get_img);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_cac_get_prop %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_cac_get_thumb
 **
 ** Description      Example of cover art get thumbnail
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_cac_get_thumb(tBSA_AVK_RC_CAC_IMG_HDL img_handle, UINT16 path_len, UINT8 *path,
                           UINT8 rc_handle)
{
    int status;
    tBSA_AVK_CAC_GET_THUMB bsa_get_thumb;

    /* Send command */
    status = BSA_AvkCacGetThumbInit(&bsa_get_thumb);

    bsa_get_thumb.rc_handle = rc_handle;
    memcpy(bsa_get_thumb.img_handle, img_handle, sizeof(tBSA_AVK_RC_CAC_IMG_HDL));
    bsa_get_thumb.path_len = path_len;
    memcpy(bsa_get_thumb.path, path, BSA_AVK_CAC_MAX_PATH_LEN);

    status = BSA_AvkCacGetThumb(&bsa_get_thumb);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_cac_get_prop %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_cac_abort
 **
 ** Description      Example of cover art abort
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_cac_abort(UINT8 rc_handle)
{
    int status;
    tBSA_AVK_CAC_ABORT bsa_abort;

    /* Send command */
    status = BSA_AvkCacAbortInit(&bsa_abort);
    bsa_abort.rc_handle = rc_handle;
    status = BSA_AvkCacAbort(&bsa_abort);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send app_avk_cac_abort %d", status);
    }
}

#endif

/*******************************************************************************
 **
 ** Function         app_avk_set_abs_vol_rsp
 **
 ** Description      This function sends abs vol response
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_avk_set_abs_vol_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label)
{
    int status;
    tBSA_AVK_SET_ABS_VOLUME_RSP abs_vol;
    BSA_AvkSetAbsVolumeRspInit(&abs_vol);

    abs_vol.label = label;
    abs_vol.rc_handle = rc_handle;
    abs_vol.volume = volume;
    status = BSA_AvkSetAbsVolumeRsp(&abs_vol);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to send app_avk_set_abs_vol_rsp %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_reg_notfn_rsp
 **
 ** Description      This function sends reg notfn response (BSA as TG role in AVK)
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_avk_reg_notfn_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label, UINT8 event, tBSA_AVK_CODE code)
{
    int status;
    tBSA_AVK_REG_NOTIF_RSP reg_notf;
    BSA_AvkRegNotifRspInit(&reg_notf);

    reg_notf.reg_notf.param.volume  = volume;
    reg_notf.reg_notf.event_id      = event;
    reg_notf.rc_handle              = rc_handle;
    reg_notf.label                  = label;
    reg_notf.code                   = code;

    status = BSA_AvkRegNotifRsp(&reg_notf);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to send app_avk_reg_notfn_rsp %d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_send_get_capabilities
 **
 ** Description      Sample code for attaining capability for events
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_get_capabilities(UINT8 rc_handle)
{
    int status;
    tBSA_AVK_VEN_CMD bsa_avk_vendor_cmd;

    /* Send remote control command */
    status = BSA_AvkVendorCmdInit(&bsa_avk_vendor_cmd);

    bsa_avk_vendor_cmd.rc_handle = rc_handle;
    bsa_avk_vendor_cmd.ctype = BSA_AVK_CMD_STATUS;
    bsa_avk_vendor_cmd.subunit_type = AVRC_SUB_PANEL;
    bsa_avk_vendor_cmd.subunit_id = 0;
    bsa_avk_vendor_cmd.data[0] = BSA_AVK_RC_VD_GET_CAPABILITIES;
    bsa_avk_vendor_cmd.data[1] = 0; /* reserved & packet type */
    bsa_avk_vendor_cmd.data[2] = 0; /* length high*/
    bsa_avk_vendor_cmd.data[3] = 1; /* length low*/
    bsa_avk_vendor_cmd.data[4] = 0x03; /* for event*/

    bsa_avk_vendor_cmd.length = 5;
    bsa_avk_vendor_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkVendorCmd(&bsa_avk_vendor_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send AV Vendor Command %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_send_register_notification
 **
 ** Description      Sample code for attaining capability for events
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_register_notification(int evt, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_VEN_CMD bsa_avk_vendor_cmd;

    /* Send remote control command */
    status = BSA_AvkVendorCmdInit(&bsa_avk_vendor_cmd);

    bsa_avk_vendor_cmd.rc_handle = rc_handle;
    bsa_avk_vendor_cmd.ctype = BSA_AVK_CMD_NOTIF;
    bsa_avk_vendor_cmd.subunit_type = AVRC_SUB_PANEL;
    bsa_avk_vendor_cmd.subunit_id = 0;
    bsa_avk_vendor_cmd.data[0] = BSA_AVK_RC_VD_REGISTER_NOTIFICATION;
    bsa_avk_vendor_cmd.data[1] = 0; /* reserved & packet type */
    bsa_avk_vendor_cmd.data[2] = 0; /* length high*/
    bsa_avk_vendor_cmd.data[3] = 5; /* length low*/
    bsa_avk_vendor_cmd.data[4] = evt; /* length low*/

    bsa_avk_vendor_cmd.length = 9;
    bsa_avk_vendor_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

    status = BSA_AvkVendorCmd(&bsa_avk_vendor_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send AV Vendor Command %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_add_connection
 **
 ** Description      This function adds a connection
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_add_connection(BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if (bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) == 0)
        {
            app_avk_cb.connections[index].in_use = TRUE;
            return &app_avk_cb.connections[index];
        }

        if (app_avk_cb.connections[index].in_use == FALSE)
        {
            bdcpy(app_avk_cb.connections[index].bda_connected, bd_addr);
            app_avk_cb.connections[index].in_use = TRUE;
            app_avk_cb.connections[index].index = index;
            return &app_avk_cb.connections[index];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_avk_reset_connection
 **
 ** Description      This function resets a connection
 **
 **
 *******************************************************************************/
void app_avk_reset_connection(BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if (bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) == 0)
        {
            app_avk_cb.connections[index].in_use = FALSE;

            app_avk_cb.connections[index].m_uiAddressedPlayer = 0;
            app_avk_cb.connections[index].m_uidCounterAddrPlayer = 0;
            app_avk_cb.connections[index].m_uid_counter = 0;
            app_avk_cb.connections[index].m_bDeviceSupportBrowse = FALSE;
            app_avk_cb.connections[index].m_uiBrowsedPlayer = 0;
            app_avk_cb.connections[index].m_iCurrentFolderItemCount = 0;
            app_avk_cb.connections[index].m_bAbsVolumeSupported = FALSE;
            app_avk_cb.connections[index].is_streaming_chl_open = FALSE;
            app_avk_cb.connections[index].is_started_streaming = FALSE;
            break;
        }
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_av_handle
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_av_handle(UINT8 handle)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if (app_avk_cb.connections[index].ccb_handle == handle)
            return &app_avk_cb.connections[index];
    }
    return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_rc_handle
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_rc_handle(UINT8 handle)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if (app_avk_cb.connections[index].rc_handle == handle)
            return &app_avk_cb.connections[index];
    }
    return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_bd_addr
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_bd_addr(BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if ((bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) == 0) &&
                (app_avk_cb.connections[index].in_use == TRUE))
            return &app_avk_cb.connections[index];
    }
    return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_find_streaming_connection
 **
 ** Description      This function finds the connection structure that is streaming
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_streaming_connection()
{
    int index;

    if(app_avk_cb.pStreamingConn &&
            app_avk_cb.pStreamingConn->is_streaming_chl_open &&
            app_avk_cb.pStreamingConn->is_started_streaming)
    {
        return app_avk_cb.pStreamingConn;
    }

    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if (app_avk_cb.connections[index].is_streaming_chl_open == TRUE &&
                app_avk_cb.connections[index].is_started_streaming == TRUE )
            return &app_avk_cb.connections[index];
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_avk_num_connections
 **
 ** Description      This function number of connections
 **
 ** Returns          number of connections
 **
 *******************************************************************************/
int app_avk_num_connections()
{
    int iCount = 0;
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if (app_avk_cb.connections[index].in_use == TRUE)
            iCount++;
    }
    return iCount;
}

/*******************************************************************************
 **
 ** Function         app_avk_display_connections
 **
 ** Description      This function displays the connections
 **
 ** Returns          status
 **
 *******************************************************************************/
void app_avk_display_connections(void)
{
    int index;
    tAPP_AVK_CONNECTION *conn = NULL;

    int num_conn = app_avk_num_connections();

    if(num_conn == 0)
    {
        APP_INFO0("    No connections");
        return;
    }

    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        conn = app_avk_find_connection_by_index(index);
        if(conn->in_use)
        {
            APP_INFO1("Connection index %d:%02X:%02X:%02X:%02X:%02X:%02X",
                index,
                conn->bda_connected[0], conn->bda_connected[1],
                conn->bda_connected[2], conn->bda_connected[3],
                conn->bda_connected[4], conn->bda_connected[5]);
        }
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_index
 **
 ** Description      This function finds the connection structure by its index
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_index(int index)
{
    if(index < APP_AVK_MAX_CONNECTIONS)
        return &app_avk_cb.connections[index];

    return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_pause_other_streams
 **
 ** Description      This function pauses other streams when a new stream start
 **                  is received from device identified by bd_addr
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_avk_pause_other_streams(BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
    {
        if ((bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) != 0) &&
                app_avk_cb.connections[index].in_use  &&
                app_avk_cb.connections[index].is_streaming_chl_open)
        {
            app_avk_cb.connections[index].is_started_streaming = FALSE;
        }
    }
}


/*******************************************************************************
 **
 ** Function         app_avk_send_delay_report
 **
 ** Description      Sample code to send delay report
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_delay_report(BD_ADDR bd_addr, UINT16 delay)
{
    tBSA_AVK_DELAY_REPORT report;
    int status;

    APP_DEBUG1("send delay report:%d", delay);

    BSA_AvkDelayReportInit(&report);
    bdcpy(report.bd_addr, bd_addr);
    report.delay = delay;
    status = BSA_AvkDelayReport(&report);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send delay report %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_avk_send_abort_req
 **
 ** Description      Sample code to send an abort request
 **                  This is used to pass PTS TC for AVDTP ABORT
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_abort_req(BD_ADDR bd_addr)
{
    tBSA_AVK_ABORT_REQ abort_req;
    int status;

    APP_DEBUG0("app_avk_send_abort_req");

    BSA_AvkAbortReqInit(&abort_req);
    bdcpy(abort_req.bd_addr, bd_addr);
    status = BSA_AvkAbortReq(&abort_req);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to send an abort request %d", status);
    }
}

