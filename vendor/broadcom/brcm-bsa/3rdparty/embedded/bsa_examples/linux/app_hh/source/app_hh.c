/*****************************************************************************
 **
 **  Name:           app_hh.c
 **
 **  Description:    Bluetooth HID Host application
 **
 **  Copyright (c) 2009-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
/* for *printf */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "app_hh.h"

#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_link.h"
#include "app_hh_db.h"
#include "app_hh_main.h"
#include "bsa_trace.h"

#if defined(APP_HH_AUDIO_STREAMING_INCLUDED) && (APP_HH_AUDIO_STREAMING_INCLUDED == TRUE)
#include "app_hh_as.h"
#endif

#if defined(APP_HH_BLE_AUDIO_INCLUDED) && (APP_HH_BLE_AUDIO_INCLUDED == TRUE)
#include "app_hh_ble_audio_sink.h"
#endif

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
#include "app_bthid.h"
#endif

#if defined(APP_HH_OTA_FW_DL_INCLUDED) && (APP_HH_OTA_FW_DL_INCLUDED == TRUE)
#include "app_hh_otafwdl.h"
#endif

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
#include "app_ble_client_db.h"
#endif

#ifndef APP_HH_SEC_DEFAULT
#define APP_HH_SEC_DEFAULT BSA_SEC_NONE
#endif

#ifdef APP_HH_UIPC_CBACK
extern void app_hh_uipc_handler(tBSA_HH_UIPC_REPORT *p_uipc_report);
#endif

//#define APP_TRACE_NODEBUG

/*
 * Globals
 */
tAPP_HH_CB app_hh_cb;

/*
 * Local functions
 */
static tAPP_HH_DEVICE *app_hh_pdev_alloc(const BD_ADDR bda);
static tAPP_HH_DEVICE *app_hh_add_dev(tAPP_XML_REM_DEVICE *p_xml_dev);

#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
static void app_hh_audio_handler(tAPP_HH_DEVICE *p_hh_dev, tBSA_HH_REPORT_DATA *p_report_data);
#endif




#if defined(BSA_HH_USE_DEPRECATED) && (BSA_HH_USE_DEPRECATED == TRUE)
/*******************************************************************************
 ** DEPRECATED FUNCTION, SHOULD USE app_hh_pdev_find_by_handle INSTEAD
 ** Function         app_hh_dev_find_by_handle
 **
 ** Description      Find HH known device by handle
 **
 ** Parameters       handle : HID handle to look for
 **
 ** Returns          -1 if not found, index in table otherwise
 ** DEPRECATED FUNCTION, SHOULD USE app_hh_pdev_find_by_handle INSTEAD
 *******************************************************************************/
int app_hh_dev_find_by_handle(tBSA_HH_HANDLE handle)
{
    int index;

    for (index = 0; index < APP_HH_DEVICE_MAX; index++)
    {
        if ((app_hh_cb.devices[index].info_mask & APP_HH_DEV_USED) &&
            (app_hh_cb.devices[index].handle == handle))
        {
            return index;
        }
    }
    return -1;
}
/*******************************************************************************
 **
 ** Function         app_hh_dev_find_by_bdaddr
 **
 ** Description      Find HH known device by its BD address
 **
 ** Parameters       bda : BD address of the device to look for
 **
 ** Returns          -1 if not found, index in table otherwise
 **
 *******************************************************************************/
int app_hh_dev_find_by_bdaddr(const BD_ADDR bda)
{
    int index;

    for (index = 0 ; index < APP_NUM_ELEMENTS(app_hh_cb.devices); index++)
    {
        if ((app_hh_cb.devices[index].info_mask & APP_HH_DEV_USED) &&
            (bdcmp(bda, app_hh_cb.devices[index].bd_addr) == 0))
        {
            return index;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function         app_hh_dev_look_for_free
 **
 ** Description      Find a free element
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_dev_look_for_free(void)
{
    int index;

    for (index = 0 ; index < APP_HH_DEVICE_MAX; index++)
    {
        if ((app_hh_cb.devices[index].info_mask & APP_HH_DEV_USED) == 0)
        {
            APP_DEBUG1("returns:%d", index);
            return index;
        }
    }
    return -1;
}
#endif

/*******************************************************************************
 **
 ** Function         app_hh_pdev_find_by_handle
 **
 ** Description      Find HH known device by handle
 **
 ** Parameters       handle : HID handle to look for
 **
 ** Returns          Pointer to HH device structure (NULL if not found)
 **
 *******************************************************************************/
tAPP_HH_DEVICE *app_hh_pdev_find_by_handle(tBSA_HH_HANDLE handle)
{
    int cnt;

    for (cnt = 0; cnt < APP_NUM_ELEMENTS(app_hh_cb.devices); cnt++)
    {
        if ((app_hh_cb.devices[cnt].info_mask & APP_HH_DEV_USED) &&
            (app_hh_cb.devices[cnt].handle == handle))
        {
            return &app_hh_cb.devices[cnt];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hh_pdev_find_by_bdaddr
 **
 ** Description      Find HH known device by its BD address
 **
 ** Parameters       bda : BD address of the device to look for
 **
 ** Returns          Pointer to HH device structure (NULL if not found)
 **
 *******************************************************************************/
tAPP_HH_DEVICE *app_hh_pdev_find_by_bdaddr(const BD_ADDR bda)
{
    int cnt;

    for (cnt = 0 ; cnt < APP_NUM_ELEMENTS(app_hh_cb.devices); cnt++)
    {
        if ((app_hh_cb.devices[cnt].info_mask & APP_HH_DEV_USED) &&
            (bdcmp(bda, app_hh_cb.devices[cnt].bd_addr) == 0))
        {
            return &app_hh_cb.devices[cnt];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hh_pdev_alloc
 **
 ** Description      Allocate a new HH device
 **
 ** Parameters       bda : BD address of the new device to allocate
 **
 ** Returns          Pointer to HH device structure (NULL if not found)
 **
 *******************************************************************************/
static tAPP_HH_DEVICE *app_hh_pdev_alloc(const BD_ADDR bda)
{
    int cnt;
    tAPP_HH_DEVICE *p_hh_dev;

    for (cnt = 0 ; cnt < APP_NUM_ELEMENTS(app_hh_cb.devices); cnt++)
    {
        if ((app_hh_cb.devices[cnt].info_mask & APP_HH_DEV_USED) == 0)
        {
            p_hh_dev = &app_hh_cb.devices[cnt];
            /* Mark device used */
            p_hh_dev->info_mask = APP_HH_DEV_USED;
            bdcpy(p_hh_dev->bd_addr, bda);
            p_hh_dev->handle = BSA_HH_INVALID_HANDLE;
            return p_hh_dev;
        }
    }
    APP_ERROR0("Failed allocating new HH element");
    return NULL;
}

#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
/*******************************************************************************
 **
 ** Function         app_hh_audio_find_active
 **
 ** Description      Find HH device which started audio
 **
 ** Parameters       None
 **
 ** Returns          Pointer to HH device structure (NULL if not found)
 **
 *******************************************************************************/
tAPP_HH_DEVICE *app_hh_audio_find_active(void)
{
    int cnt;

    for (cnt = 0 ; cnt < APP_NUM_ELEMENTS(app_hh_cb.devices); cnt++)
    {
        if ((app_hh_cb.devices[cnt].info_mask & APP_HH_DEV_USED) &&
            (app_hh_cb.devices[cnt].audio.is_audio_active))
        {
            return &app_hh_cb.devices[cnt];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_hh_audio_send
 **
 ** Description      Send an audio report
 **
 ** Parameters       handle: HID handle of the device
 **                  p_data: data content of the report (not including report ID)
 **                  length: length of the data
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_hh_audio_send(tBSA_HH_HANDLE handle, UINT8 *p_data, UINT8 length)
{
    tBSA_HH_SET hh_set_param;
    int status;
#if defined(APP_HH_BLE_AUDIO_INCLUDED) && (APP_HH_BLE_AUDIO_INCLUDED == TRUE)
    tAPP_HH_DEVICE *p_hh_dev;
#endif

    BSA_HhSetInit(&hh_set_param);

    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = BSA_HH_RPTT_FEATURE;

    hh_set_param.param.set_report.report.length = length+1;
    hh_set_param.param.set_report.report.data[0] = APP_HH_AUDIO_REPORT_ID;
#if defined(APP_HH_BLE_AUDIO_INCLUDED) && (APP_HH_BLE_AUDIO_INCLUDED == TRUE)
    /* For BLE audio use different report id */
    p_hh_dev = app_hh_pdev_find_by_handle(handle);
    if(p_hh_dev != NULL)
    {
        if(p_hh_dev->audio.is_ble_audio_device == TRUE)
            hh_set_param.param.set_report.report.data[0] = APP_HH_AUDIO_START_STOP_REPORT_ID;
    }
    else
        APP_ERROR0("app_hh_audio_send error - device not found by handle");
#endif
    memcpy(&hh_set_param.param.set_report.report.data[1], p_data, length);

    scru_dump_hex(hh_set_param.param.set_report.report.data, "Audio RC command",
            length + 1, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);

    status = BSA_HhSet(&hh_set_param);

    if(status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet fail status:%d", status);
        return (-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_audio_mic_start
 **
 ** Description      Send start microphone command to device
 **
 ** Parameters       p_hh_dev: pointer to HID device
 **                  mode: audio mode selection (1=solicited, 0=unsolicited)
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_hh_audio_mic_start(tAPP_HH_DEVICE *p_hh_dev, UINT8 mode)
{
    UINT8 report[7], *p_data;
#ifdef PCM_ALSA
    tAPP_ALSA_PLAYBACK_OPEN alsa_open;
#else
    char file_name[50];
#endif

    APP_DEBUG1("p=%p m=%d", p_hh_dev, mode);

    /* Check if there is already a device using Audio */
    if (app_hh_audio_find_active())
    {
        APP_ERROR0("Audio is already active.");
        return -1;
    }

#ifdef PCM_ALSA
     app_alsa_init();
     app_alsa_playback_open_init(&alsa_open);

     alsa_open.blocking = FALSE;
     alsa_open.format = APP_ALSA_PCM_FORMAT_S16_LE; /* PCM Format */
     alsa_open.access = APP_ALSA_PCM_ACCESS_RW_INTERLEAVED; /* PCM access */
     if (p_hh_dev->audio.format.nb_channels == 1)
     {
         alsa_open.stereo = FALSE; /* Mono or Stereo */
     }
     else
     {
         alsa_open.stereo = TRUE;
     }

     /* Sampling rate (e.g. 8000 for 8KHz) */
     alsa_open.sample_rate = p_hh_dev->audio.format.sample_rate;

     app_alsa_playback_open(&alsa_open);
#else
     snprintf(file_name, sizeof(file_name), APP_HH_VOICE_FILE_NAME_FORMAT, p_hh_dev->handle);
     file_name[sizeof(file_name)-1] = '\0';
     p_hh_dev->audio.fd = app_wav_create_file(file_name, 0);
#endif /* PCM_ALSA */

    p_data = &report[0];
    UINT8_TO_STREAM(p_data, APP_HH_AUDIO_MIC_START); /* Report format */
    UINT8_TO_STREAM(p_data, 0); /* channel */
    UINT8_TO_STREAM(p_data, mode); /* reserved: audio mode */
    UINT16_TO_STREAM(p_data, 0); /* reserved */
    UINT16_TO_STREAM(p_data, 0); /* data count */

    if (app_hh_audio_send(p_hh_dev->handle, &report[0], sizeof(report)) != 0)
    {
#ifdef PCM_ALSA
        app_alsa_playback_close();
#else
        app_wav_close_file(p_hh_dev->audio.fd, &(p_hh_dev->audio.format));
        p_hh_dev->audio.fd = -1;
#endif
        return -1;
    }

    /* Set Priority to HIGH for HID connection */
    app_hh_send_set_prio(p_hh_dev->handle,p_hh_dev->bd_addr, L2CAP_PRIORITY_HIGH, L2CAP_DIRECTION_DATA_SINK);

    p_hh_dev->audio.is_audio_active = TRUE;

#if defined(APP_HH_AUDIO_STATS_ENABLED) && (APP_HH_AUDIO_STATS_ENABLED == TRUE)
    p_hh_dev->audio.lastseqnum = 0;
    p_hh_dev->audio.firstseqnum = 0xFFFFFFFF;
    p_hh_dev->audio.missseqnum = 0;
#endif

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_audio_mic_stop
 **
 ** Description      Send stop microphone command to device
 **
 ** Parameters       p_hh_dev: pointer to HID device
 **                  mode: audio mode selection (1=solicited, 0=unsolicited)
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_hh_audio_mic_stop(tAPP_HH_DEVICE *p_hh_dev, UINT8 mode)
{
    UINT8 report[7], *p_data;

    APP_DEBUG1("p=%p m=%d", p_hh_dev, mode);

    if (p_hh_dev->audio.is_audio_active == FALSE)
    {
        APP_ERROR0("Audio is not active.");
        return -1;
    }

#ifdef PCM_ALSA
     app_alsa_playback_close();
#else
     app_wav_close_file(p_hh_dev->audio.fd, &(p_hh_dev->audio.format));
     p_hh_dev->audio.fd = -1;
#endif

     p_hh_dev->audio.is_audio_active = FALSE;

#if defined(APP_HH_AUDIO_STATS_ENABLED) && (APP_HH_AUDIO_STATS_ENABLED == TRUE)
     APP_INFO0("HH  voice statistics");
     APP_INFO1("First sequence number: %u", p_hh_dev->audio.firstseqnum);
     APP_INFO1("Last sequence number: %u", p_hh_dev->audio.lastseqnum);
     APP_INFO1("Missing sequences: %u", p_hh_dev->audio.missseqnum);
#endif

     /* Send APP_HH_AUDIO_MIC_STOP */
     p_data = &report[0];
     UINT8_TO_STREAM(p_data, APP_HH_AUDIO_MIC_STOP); /* Report format */
     UINT8_TO_STREAM(p_data, 0); /* channel */
     UINT8_TO_STREAM(p_data, mode); /* reserved: audio mode */
     UINT16_TO_STREAM(p_data, 0); /* reserved */
     UINT16_TO_STREAM(p_data, 0); /* data count */
     if (app_hh_audio_send(p_hh_dev->handle, &report[0], sizeof(report)) != 0)
     {
         return -1;
     }
     /* Set Priority to NORMAL for HID connection */
     app_hh_send_set_prio(p_hh_dev->handle,p_hh_dev->bd_addr, L2CAP_PRIORITY_NORMAL, L2CAP_DIRECTION_DATA_SINK);
     return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_audio_handler
 **
 ** Description
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_hh_audio_handler(tAPP_HH_DEVICE *p_hh_dev, tBSA_HH_REPORT_DATA *p_report_data)
{
    int hh_voice_size;
    tAPP_HH_AUDIO_DATA_HDR audiodata_hdr;
    UINT8 audio_report[APP_HH_AUDIO_MAX_REPORT_SIZE];
    UINT8* p_data;
    UINT8 audio_format;
    UINT8 codec_setting_id;
    UINT16 codec_setting_len;
    UINT8 sample_freq;
    UINT8 bits_per_samp;

    p_data = &p_report_data->data[APP_HH_AUDIO_FORMAT_OFFSET];
    STREAM_TO_UINT8(audio_format, p_data);

    switch(audio_format)
    {
     case APP_HH_AUDIO_RC_MIC_START_REQ:
         /* Send solicited start */
         app_hh_audio_mic_start(p_hh_dev, 1);
         break;

     case APP_HH_AUDIO_DATA:
         if (p_hh_dev->audio.is_audio_active)
         {
             STREAM_TO_UINT16(audiodata_hdr.seq_num, p_data);
             STREAM_TO_UINT16(audiodata_hdr.timestamp, p_data);
             STREAM_TO_UINT16(audiodata_hdr.bufferState, p_data);
             STREAM_TO_UINT16(audiodata_hdr.reserved, p_data);
             STREAM_TO_UINT16(audiodata_hdr.datacnt, p_data);

#if defined(APP_HH_AUDIO_STATS_ENABLED) && (APP_HH_AUDIO_STATS_ENABLED == TRUE)
             if (p_hh_dev->audio.firstseqnum == 0xFFFFFFFF)
             {
                 p_hh_dev->audio.firstseqnum = audiodata_hdr.seq_num;
             }
             else
             {
                 p_hh_dev->audio.missseqnum += audiodata_hdr.seq_num - (p_hh_dev->audio.lastseqnum + 1);
             }
             p_hh_dev->audio.lastseqnum = audiodata_hdr.seq_num;
#endif

             hh_voice_size = audiodata_hdr.datacnt;
             APP_DEBUG1("voice size %d", hh_voice_size);

             hh_voice_size = hh_voice_size * (p_hh_dev->audio.format.bits_per_sample / 8) ;
#ifdef PCM_ALSA
             app_alsa_playback_write(p_data, hh_voice_size);
#else
             if (write(p_hh_dev->audio.fd, p_data, hh_voice_size) < 0)
             {
                 APP_ERROR1("write(%d) failed", p_hh_dev->audio.fd);
             }
#endif
         }
         break;

     case APP_HH_AUDIO_RC_MIC_STOP_REQ:
         /* Send solicited stop */
         app_hh_audio_mic_stop(p_hh_dev, 1);
         break;

     case APP_HH_AUDIO_CODEC_READ_ACK:
         p_data++ ; /* channel */
         STREAM_TO_UINT8(codec_setting_id, p_data); /* setting ID */
         STREAM_TO_UINT16(codec_setting_len, p_data); /* data count */

         switch (codec_setting_id)
         {
             case APP_HH_AUDIO_SAMP_FREQ_ID:
                 /* Size is 2: 1 byte for sample_freq + 1 reserved byte*/
                 if(codec_setting_len == 2)
                 {
                     STREAM_TO_UINT8(sample_freq, p_data);
                     switch (sample_freq)
                     {
                         case APP_HH_AUDIO_SAMP_FREQ_8K:
                         case APP_HH_AUDIO_SAMP_FREQ_16K:
                         case APP_HH_AUDIO_SAMP_FREQ_22K5:
                         case APP_HH_AUDIO_SAMP_FREQ_32K:
                         case APP_HH_AUDIO_SAMP_FREQ_48K:
                             p_hh_dev->audio.format.sample_rate = sample_freq * 250 ;
                             break;

                         default:
                             APP_ERROR1("Unsupported frequency (%d) reported in codec settings", sample_freq);
                             return;
                             break;
                      }

                     APP_DEBUG1("Sample rate = %u", p_hh_dev->audio.format.sample_rate);

                     p_data = &audio_report[0];

                     /* Read number of bits per samples */
                     UINT8_TO_STREAM(p_data, APP_HH_AUDIO_CODEC_READ_REQ); /* Report format */
                     UINT8_TO_STREAM(p_data, 0); /* channel */
                     UINT8_TO_STREAM(p_data, APP_HH_AUDIO_BITS_PER_SAMP_ID); /* Setting Id = bits per sample */
                     UINT16_TO_STREAM(p_data, 0); /* data count */
                     APP_DEBUG0("Send APP_HH_AUDIO_BITS_PER_SAMP_ID");
                     app_hh_audio_send(p_hh_dev->handle, &audio_report[0], 5);
                 }
                 else
                 {
                     APP_DEBUG1("Wrong codec setting length %d",codec_setting_len);
                 }
                 break;

             case APP_HH_AUDIO_BITS_PER_SAMP_ID:
                 if (codec_setting_len == 2)
                 {
                     STREAM_TO_UINT8(bits_per_samp, p_data);

                     p_hh_dev->audio.format.bits_per_sample = bits_per_samp;
                     APP_DEBUG1("BitPerSample = %u", p_hh_dev->audio.format.bits_per_sample);
                 }
                 else
                 {
                     APP_DEBUG1("Wrong codec setting length %d", codec_setting_len);
                 }
                 break;

             case APP_HH_AUDIO_SAMP_FORMAT_ID:
             case APP_HH_AUDIO_PGA_GAIN_ID:
             case APP_HH_AUDIO_HP_FILTER_SET_ID:
                 APP_DEBUG1("Setting [%d] not yet implemented", codec_setting_id);
                 break;

             default:
                 APP_ERROR1("Wrong setting ID",codec_setting_id);
                 return;
                 break;
         }

         /*
          * If minimal configuration was not already achieved, then set the bit and
          * check if we achieved it
          */
         if (!APP_BITS_SET(p_hh_dev->audio.codec_setting_bits, APP_HH_AUDIO_MINIMAL_CODEC_CONFIG))
         {
             /* Set bit field of codec settings that has been read */
             p_hh_dev->audio.codec_setting_bits |= (1 << codec_setting_id);
             /* if we have read the minimal set of codec settings then we can start audio */
             if (APP_BITS_SET(p_hh_dev->audio.codec_setting_bits, APP_HH_AUDIO_MINIMAL_CODEC_CONFIG))
             {
                 app_hh_audio_mic_start(p_hh_dev, 1);
             }
         }
         break;

     default:
         APP_ERROR1("Unsupported audio format %d", audio_format);
         break;
    }
}
#endif

/*******************************************************************************
 **
 ** Function         app_hh_uipc_cback
 **
 ** Description      Process Report data
 **
 ** Parameters       p_msg : HID report
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_uipc_cback(BT_HDR *p_msg)
{
    tBSA_HH_UIPC_REPORT *p_uipc_report = (tBSA_HH_UIPC_REPORT *)p_msg;
    BOOLEAN exit_generic_cback= FALSE;

    /* Since in some circumstances, this variable is not used, remove warnings */
    (void)p_uipc_report;

#if ( defined (APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)) || \
    ( defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE))
    tAPP_HH_DEVICE *p_hh_dev = app_hh_pdev_find_by_handle(p_uipc_report->report.handle);
#endif

    /* if a specific UIPC callback function is defined */
    if (app_hh_cb.p_uipc_cback)
    {
        exit_generic_cback = app_hh_cb.p_uipc_cback(p_msg);
        if (exit_generic_cback != FALSE)
        {
            GKI_freebuf(p_msg);
            return;
        }
    }

    APP_DEBUG1("app_hh_uipc_cback called Handle:%d BdAddr:%02X:%02X:%02X:%02X:%02X:%02X",
           p_uipc_report->report.handle,
           p_uipc_report->report.bd_addr[0], p_uipc_report->report.bd_addr[1],
           p_uipc_report->report.bd_addr[2], p_uipc_report->report.bd_addr[3],
           p_uipc_report->report.bd_addr[4], p_uipc_report->report.bd_addr[5]);
    APP_DEBUG1("\tMode:%d SubClass:0x%x CtryCode:%d len:%d event:%d",
            p_uipc_report->report.mode,
            p_uipc_report->report.sub_class,
            p_uipc_report->report.ctry_code,
            p_uipc_report->report.report_data.length,
            p_uipc_report->hdr.event);

#ifndef APP_TRACE_NODEBUG
    if (p_uipc_report->report.report_data.length > 32)
    {
        scru_dump_hex(p_uipc_report->report.report_data.data, "Report (truncated)",
                32, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
    }
    else
    {
        scru_dump_hex(p_uipc_report->report.report_data.data, "Report",
                p_uipc_report->report.report_data.length, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
    }
#endif

#if defined(APP_HH_AUDIO_STREAMING_INCLUDED) && (APP_HH_AUDIO_STREAMING_INCLUDED == TRUE)
    if ((p_hh_dev != NULL) &&
        (p_uipc_report->report.report_data.data[0] == APP_HH_AS_AUDIO_REPORT_ID))
    {
        app_hh_as_audio_cmd_handler(&p_uipc_report->report);
        GKI_freebuf(p_msg);
        return;
    }
#endif
#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
    if ((p_hh_dev != NULL) && (p_hh_dev->audio.is_audio_device) &&
        (p_uipc_report->report.report_data.data[0] == APP_HH_AUDIO_REPORT_ID))
    {
        app_hh_audio_handler(p_hh_dev, &p_uipc_report->report.report_data);
        GKI_freebuf(p_msg);
        return;
    }
#if defined(APP_HH_BLE_AUDIO_INCLUDED) && (APP_HH_BLE_AUDIO_INCLUDED == TRUE)
    if ((p_hh_dev != NULL) && (p_hh_dev->audio.is_ble_audio_device) &&
        ((p_uipc_report->report.report_data.data[0] == APP_HH_AUDIO_REPORT_ID) ||
          (p_uipc_report->report.report_data.data[0] == APP_HH_AUDIO_START_STOP_REPORT_ID)))
    {
        app_hh_ble_audio_sink_handler(p_hh_dev, &p_uipc_report->report.report_data);
        GKI_freebuf(p_msg);
        return;
    }
#endif
#endif

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    if ((p_hh_dev != NULL) && (p_hh_dev->bthid_fd != -1) )
    {
        app_bthid_report_write(p_hh_dev->bthid_fd,
            p_uipc_report->report.report_data.data,
            p_uipc_report->report.report_data.length);
    }
    else
    {
        APP_ERROR1("bthid not opened for HH handle:%d", p_uipc_report->report.handle);
    }
#endif

#ifdef APP_HH_UIPC_CBACK
    app_hh_uipc_handler(p_uipc_report);
#endif

    GKI_freebuf(p_msg);
}

/*******************************************************************************
 **
 ** Function         app_hh_cback
 **
 ** Description      Example of function to start HID Host application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_cback(tBSA_HH_EVT event, tBSA_HH_MSG *p_data)
{
    int index;
    tAPP_HH_DB_ELEMENT *p_hh_db_elmt;
    tAPP_HH_DEVICE *p_hh_dev;
    tAPP_XML_REM_DEVICE *p_xml_dev;
    tBSA_HH_CLOSE hh_close_param;

    /* If a specific callback function is defined */
    if (app_hh_cb.p_event_cback)
    {
        if (app_hh_cb.p_event_cback(event, p_data))
        {
            return;
        }
    }

    switch (event)
    {
    case BSA_HH_OPEN_EVT:
        APP_INFO1("BSA_HH_OPEN_EVT: status:%d, handle:%d mode:%s (%d)",
                p_data->open.status, p_data->open.handle,
                p_data->open.mode==BTA_HH_PROTO_RPT_MODE?"Report":"Boot",
                        p_data->open.mode);
        APP_INFO1("BdAddr :%02X-%02X-%02X-%02X-%02X-%02X",
                p_data->open.bd_addr[0], p_data->open.bd_addr[1], p_data->open.bd_addr[2],
                p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);

        if (p_data->open.status == BSA_SUCCESS)
        {
            if (p_data->open.handle == app_hh_cb.mip_handle)
            {
                app_hh_cb.mip_device_nb++;
                APP_INFO1("%d 3DG MIP device connected", app_hh_cb.mip_device_nb);
                break;
            }

            /* Check if the device is already in our internal database */
            p_hh_dev = app_hh_pdev_find_by_handle(p_data->open.handle);
            if (p_hh_dev == NULL)
            {
                /* Add the device in the tables */
                p_hh_dev = app_hh_pdev_alloc(p_data->open.bd_addr);
                if (p_hh_dev == NULL)
                {
                    APP_ERROR0("Failed allocating HH element");
                    BSA_HhCloseInit(&hh_close_param);
                    hh_close_param.handle = p_data->open.handle;
                    BSA_HhClose(&hh_close_param);
                    break;
                }

                /* For safety, remove any previously existing HH db element */
                app_hh_db_remove_by_bda(p_data->open.bd_addr);

                p_hh_db_elmt = app_hh_db_alloc_element();
                if (p_hh_db_elmt != NULL)
                {
                    bdcpy(p_hh_db_elmt->bd_addr, p_data->open.bd_addr);
                    app_hh_db_add_element(p_hh_db_elmt);
                }
                else
                {
                    APP_ERROR0("Failed allocating HH database element");
                    BSA_HhCloseInit(&hh_close_param);
                    hh_close_param.handle = p_data->open.handle;
                    BSA_HhClose(&hh_close_param);
                    break;
                }
                /* Copy the BRR info */
                p_hh_db_elmt->brr_size = sizeof(tBSA_HH_BRR_CFG);
                p_hh_db_elmt->brr = malloc(p_hh_db_elmt->brr_size);
                if (p_hh_db_elmt->brr != NULL)
                {
                    memcpy(p_hh_db_elmt->brr, &p_data->open.brr_cfg, p_hh_db_elmt->brr_size);
                }
                else
                {
                    p_hh_db_elmt->brr_size = 0;
                }

                p_hh_db_elmt->sub_class = p_data->open.sub_class;
                p_hh_db_elmt->attr_mask = p_data->open.attr_mask;

                /* Save the database */
                app_hh_db_save();

                /* Fetch the descriptor */
                app_hh_get_dscpinfo(p_data->open.handle);
            }

            /* Save the HH Handle */
            p_hh_dev->handle = p_data->open.handle;

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
            p_hh_dev->le_hid = p_data->open.le_hid;
#endif
            /* Mark device opened */
            p_hh_dev->info_mask |= APP_HH_DEV_OPENED;
        }
        break;

    /* coverity[MISSING BREAK] False-positive: Unplug event is handled as a close event */
    case BSA_HH_VC_UNPLUG_EVT:
        APP_INFO0("BSA_HH_VC_UNPLUG_EVT is handled like a close event, redirecting");

    case BSA_HH_CLOSE_EVT:
        APP_INFO1("BSA_HH_CLOSE_EVT: status:%d  handle:%d",
                p_data->close.status, p_data->close.handle);

        if (p_data->close.handle == app_hh_cb.mip_handle)
        {
            if (app_hh_cb.mip_device_nb > 0)
            {
                app_hh_cb.mip_device_nb--;
            }
            APP_INFO1("%d 3DG MIP device connected", app_hh_cb.mip_device_nb);
            break;
        }

        p_hh_dev = app_hh_pdev_find_by_handle(p_data->close.handle);
        if (p_hh_dev != NULL)
        {
#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
#ifndef PCM_ALSA
            if (p_hh_dev->audio.fd != -1)
            {
                app_wav_close_file(p_hh_dev->audio.fd, &(p_hh_dev->audio.format));
                p_hh_dev->audio.fd = -1;
            }
#endif
            p_hh_dev->audio.is_audio_active = FALSE;
#endif

            p_hh_dev->info_mask &= ~APP_HH_DEV_OPENED;
        }
        app_hh_display_status();
        break;

    case BSA_HH_GET_DSCPINFO_EVT:
        APP_INFO1("BSA_HH_GET_DSCPINFO_EVT: status:%d handle:%d",
                p_data->dscpinfo.status, p_data->dscpinfo.handle);

        if (p_data->dscpinfo.status == BSA_SUCCESS)
        {
            APP_INFO1("DscpInfo: VID/PID (hex) = %04X/%04X",
                    p_data->dscpinfo.peerinfo.vendor_id,
                    p_data->dscpinfo.peerinfo.product_id);
            APP_INFO1("DscpInfo: version = %d", p_data->dscpinfo.peerinfo.version);
            APP_INFO1("DscpInfo: ssr_max_latency = %d ssr_min_tout = %d",
                    p_data->dscpinfo.peerinfo.ssr_max_latency,
                    p_data->dscpinfo.peerinfo.ssr_min_tout);
            APP_INFO1("DscpInfo: supervision_tout = %d", p_data->dscpinfo.peerinfo.supervision_tout);
            APP_INFO1("DscpInfo (descriptor len:%d):", p_data->dscpinfo.dscpdata.length);
            scru_dump_hex(p_data->dscpinfo.dscpdata.data, NULL,
                    p_data->dscpinfo.dscpdata.length, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);

            /* Check if the device is already in our internal database */
            p_hh_dev = app_hh_pdev_find_by_handle(p_data->dscpinfo.handle);
            if (p_hh_dev)
            {
                APP_INFO0("Update Remote Device XML file");
                app_read_xml_remote_devices();
                app_xml_update_pidvid_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_hh_dev->bd_addr,
                        p_data->dscpinfo.peerinfo.product_id,
                        p_data->dscpinfo.peerinfo.vendor_id);
                app_xml_update_version_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_hh_dev->bd_addr,
                        p_data->dscpinfo.peerinfo.version);
                app_write_xml_remote_devices();

                /* Find the pointer to the DB element or allocate one */
                p_hh_db_elmt = app_hh_db_find_by_bda(p_hh_dev->bd_addr);
                if (p_hh_db_elmt != NULL)
                {
                    /* Free the previous descriptor if there is one */
                    if (p_hh_db_elmt->descriptor != NULL)
                    {
                        free(p_hh_db_elmt->descriptor);
                        p_hh_db_elmt->descriptor = NULL;
                    }
                    /* Save the HH descriptor information */
                    p_hh_db_elmt->ssr_max_latency = p_data->dscpinfo.peerinfo.ssr_max_latency;
                    p_hh_db_elmt->ssr_min_tout = p_data->dscpinfo.peerinfo.ssr_min_tout;
                    p_hh_db_elmt->supervision_tout = p_data->dscpinfo.peerinfo.supervision_tout;
                    p_hh_db_elmt->descriptor_size = p_data->dscpinfo.dscpdata.length;
                    if (p_hh_db_elmt->descriptor_size)
                    {
                        /* Copy the descriptor content */
                        p_hh_db_elmt->descriptor = malloc(p_hh_db_elmt->descriptor_size);
                        if (p_hh_db_elmt->descriptor  != NULL)
                        {
                            memcpy(p_hh_db_elmt->descriptor, p_data->dscpinfo.dscpdata.data, p_hh_db_elmt->descriptor_size);
                        }
                        else
                        {
                            p_hh_db_elmt->descriptor_size = 0;
                        }
                    }

                    /* Save the database */
                    app_hh_db_save();

                    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
                    {
                        p_xml_dev = &app_xml_remote_devices_db[index];
                        if ((p_xml_dev->in_use != FALSE) &&
                            (bdcmp(p_xml_dev->bd_addr, p_hh_db_elmt->bd_addr) == 0))
                        {
                            /* Let's add this device to Server HH database to allow it to reconnect */
                            APP_INFO1("Adding HID Device:%s", p_xml_dev->name);
                            app_hh_add_dev(p_xml_dev);
                            break;
                        }
                    }
                }
                else
                {
                    APP_ERROR0("BSA_HH_GET_DSCPINFO_EVT: app_hh_db_find_by_bda failed");
                    BSA_HhCloseInit(&hh_close_param);
                    hh_close_param.handle = p_data->dscpinfo.handle;
                    BSA_HhClose(&hh_close_param);
                }
            }
        }
        break;

    case BSA_HH_GET_REPORT_EVT:
        APP_INFO1("BSA_HH_GET_REPORT_EVT: status:%d handle:%d len:%d",
                p_data->get_report.status, p_data->get_report.handle,
                p_data->get_report.report.length);

        if (p_data->get_report.report.length > 0)
        {
            APP_INFO1("Report ID: [%02x]",p_data->get_report.report.data[0]);

            scru_dump_hex(&p_data->get_report.report.data[1], "DATA",
                p_data->get_report.report.length-1, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG);
        }
        break;

    case BSA_HH_SET_REPORT_EVT:
        APP_INFO1("BSA_HH_SET_REPORT_EVT: status:%d handle:%d",
                p_data->set_report.status, p_data->set_report.handle);
        break;

    case BSA_HH_GET_PROTO_EVT:
        APP_INFO1("BSA_HH_GET_PROTO_EVT: status:%d handle:%d mode:%d",
                p_data->get_proto.status, p_data->get_proto.handle,
                p_data->get_proto.mode);

        if (p_data->get_proto.status == BSA_SUCCESS)
        {
            /* Boot mode setting here is required to pass HH PTS test HOS_HID_BV_09_C*/
            app_hh_change_proto_mode(p_data->get_proto.handle,
                                     BSA_HH_PROTO_BOOT_MODE);
        }

        break;

    case BSA_HH_SET_PROTO_EVT:
        APP_INFO1("BSA_HH_SET_PROTO_EVT: status:%d, handle:%d",
                p_data->set_proto.status, p_data->set_proto.handle);
        break;

    case BSA_HH_GET_IDLE_EVT:
        APP_INFO1("BSA_HH_GET_IDLE_EVT: status:%d, handle:%d, idle:%d",
                p_data->get_idle.status, p_data->get_idle.handle, p_data->get_idle.idle);
        break;

    case BSA_HH_SET_IDLE_EVT:
        APP_INFO1("BSA_HH_SET_IDLE_EVT: status:%d, handle:%d",
                p_data->set_idle.status, p_data->set_idle.handle);
        break;

    case BSA_HH_MIP_START_EVT:
        APP_INFO0("BSA_HH_MIP_START_EVT");
        app_hh_cb.mip_handle = p_data->mip_start.handle;
        break;

    case BSA_HH_MIP_STOP_EVT:
        APP_INFO0("BSA_HH_MIP_STOP_EVT");
        app_hh_cb.mip_handle = p_data->mip_stop.handle;
        break;

    default:
        APP_ERROR1("bad event:%d", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_hh_add_dev
 **
 ** Description      Add an HID device in the internal tables of BSA
 **
 ** Parameters       index: index of the device in the remote entries XML table
 **
 ** Returns          Pointer to HH device structure (NULL if failed)
 **
 *******************************************************************************/
static tAPP_HH_DEVICE *app_hh_add_dev(tAPP_XML_REM_DEVICE *p_xml_dev)
{
    tAPP_HH_DEVICE *p_hh_dev;
    DEV_CLASS class_of_device;
    UINT8 *p_cod = &class_of_device[0];
    UINT8 major;
    UINT8 minor;
    UINT16 services;
    int status;
    tBSA_HH_ADD_DEV hh_add_dev_param;
    tAPP_HH_DB_ELEMENT *p_hh_db_elmt;
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    BOOLEAN desc_present = FALSE;
#endif

    /* Find the HH DB element */
    p_hh_db_elmt = app_hh_db_find_by_bda(p_xml_dev->bd_addr);
    if (p_hh_db_elmt == NULL)
    {
        APP_ERROR0("Trying to add an HH device that is not in the HH database");
        return NULL;
    }

    /* Check if device is already in internal tables */
    p_hh_dev = app_hh_pdev_find_by_bdaddr(p_xml_dev->bd_addr);
    if (p_hh_dev == NULL)
    {
        /* Add the device in the tables */
        p_hh_dev = app_hh_pdev_alloc(p_xml_dev->bd_addr);
        if (p_hh_dev == NULL)
        {
            return NULL;
        }
    }

    /* Add device in HH tables */
    BSA_HhAddDevInit(&hh_add_dev_param);

    bdcpy(hh_add_dev_param.bd_addr, p_hh_db_elmt->bd_addr);

    APP_DEBUG1("Attribute mask = %x - sub_class = %x",p_hh_db_elmt->attr_mask,p_hh_db_elmt->sub_class);
    hh_add_dev_param.attr_mask = p_hh_db_elmt->attr_mask;
    hh_add_dev_param.sub_class = p_hh_db_elmt->sub_class;
    hh_add_dev_param.peerinfo.ssr_max_latency = p_hh_db_elmt->ssr_max_latency;
    hh_add_dev_param.peerinfo.ssr_min_tout = p_hh_db_elmt->ssr_min_tout;
    hh_add_dev_param.peerinfo.supervision_tout = p_hh_db_elmt->supervision_tout;
    hh_add_dev_param.attr_mask |= HID_SUP_TOUT_AVLBL;

    hh_add_dev_param.peerinfo.vendor_id = p_xml_dev->vid;
    hh_add_dev_param.peerinfo.product_id = p_xml_dev->pid;
    hh_add_dev_param.peerinfo.version = p_xml_dev->version;

    /* Copy the descriptor if available */
    if ((p_hh_db_elmt->descriptor_size != 0) &&
        (p_hh_db_elmt->descriptor_size <= sizeof(hh_add_dev_param.dscp_data.data)) &&
        (p_hh_db_elmt->descriptor != NULL))
    {
        hh_add_dev_param.dscp_data.length = p_hh_db_elmt->descriptor_size;
        memcpy(hh_add_dev_param.dscp_data.data, p_hh_db_elmt->descriptor,
                p_hh_db_elmt->descriptor_size);
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
        desc_present = TRUE;
#endif
    }

    /* If there is a callback to add specific information */
    if (app_hh_cb.p_add_dev_cback != NULL)
    {
        app_hh_cb.p_add_dev_cback(&hh_add_dev_param, p_xml_dev);
    }

    status = BSA_HhAddDev(&hh_add_dev_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to add HID device:%d", status);
        app_hh_db_remove_by_bda(p_xml_dev->bd_addr);
        p_hh_dev->info_mask = 0;
        p_xml_dev->trusted_services &= ~BSA_HID_SERVICE_MASK;
        app_write_xml_remote_devices();
        return NULL;
    }

    /* Save the HH handle in the internal table */
    p_hh_dev->handle = hh_add_dev_param.handle;

    /* Add HID to security trusted services */
    p_xml_dev->trusted_services |= BSA_HID_SERVICE_MASK;
    app_write_xml_remote_devices();
    APP_INFO1("Services = x%x", p_xml_dev->trusted_services);

    /* Extract Minor and Major numbers from COD */
    p_cod = p_xml_dev->class_of_device;
    BTM_COD_MINOR_CLASS(minor, p_cod);
    BTM_COD_MAJOR_CLASS(major, p_cod);
    APP_INFO1("COD:%02X-%02X-%02X => %s", p_cod[0], p_cod[1], p_cod[2], app_get_cod_string(p_cod));

    /* Check if the COD means RemoteControl */
    if ((major == BTM_COD_MAJOR_PERIPHERAL) &&
        ((minor & ~BTM_COD_MINOR_COMBO) == BTM_COD_MINOR_REMOTE_CONTROL))
    {
        BTM_COD_SERVICE_CLASS(services, p_cod);
        APP_INFO0("This device is a RemoteControl");
#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
         /* If Service audio or Capturing bits set, this is a Voice capable RC */
         if (services & (BTM_COD_SERVICE_AUDIO | BTM_COD_SERVICE_CAPTURING))
         {
             p_hh_dev->audio.is_audio_active = FALSE;
             p_hh_dev->audio.is_audio_device = TRUE;
         }
         else
         {
             p_hh_dev->audio.is_audio_device = FALSE;
         }
#endif
    }
#if defined(APP_HH_BLE_AUDIO_INCLUDED) && (APP_HH_BLE_AUDIO_INCLUDED == TRUE)
    if ((major == 0) && (minor == 0) && (p_xml_dev->device_type == BT_DEVICE_TYPE_BLE))
    {
        APP_INFO0("This device is a BLE RemoteControl");
        p_hh_dev->audio.is_audio_active = FALSE;
        p_hh_dev->audio.is_ble_audio_device = TRUE;
    }
#endif

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    if (p_hh_dev->bthid_fd != -1)
    {
        app_bthid_close(p_hh_dev->bthid_fd);
    }

    /* Open BTHID driver */
    p_hh_dev->bthid_fd = app_bthid_open();
    if (p_hh_dev->bthid_fd < 0)
    {
        APP_ERROR0("BTHID open failed");
    }
    else
    {
        app_bthid_config(p_hh_dev->bthid_fd, "BSA sample apps HID device",
                p_xml_dev->vid, p_xml_dev->pid, p_xml_dev->version, 0);
        if (desc_present)
        {
            /* Send HID descriptor to the kernel (via bthid module) */
            app_bthid_desc_write(p_hh_dev->bthid_fd,
                                (unsigned char *)p_hh_db_elmt->descriptor,
                                p_hh_db_elmt->descriptor_size);
        }
    }
#endif

    return p_hh_dev;
}
/*******************************************************************************
 **
 ** Function         app_hh_start
 **
 ** Description      Start HID Host
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_start(void)
{
    int status;
    int index;
    tBSA_HH_ENABLE enable_param;
    tAPP_XML_REM_DEVICE *p_xml_dev;

    APP_INFO0("app_hh_start");

    BSA_HhEnableInit(&enable_param);
    enable_param.sec_mask = APP_HH_SEC_DEFAULT;
    app_hh_cb.sec_mask_in = enable_param.sec_mask;

    enable_param.p_cback = app_hh_cback;

    status = BSA_HhEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to enable HID Host status:%d", status);
        return -1;
    }

    app_hh_cb.uipc_channel = enable_param.uipc_channel; /* Save UIPC channel */
    UIPC_Open(app_hh_cb.uipc_channel, app_hh_uipc_cback);  /* Open UIPC channel */

    for (index = 0 ; index < APP_HH_DEVICE_MAX; index++)
    {
        app_hh_cb.devices[index].info_mask = 0;
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
        app_hh_cb.devices[index].bthid_fd = -1;
#endif
    }


    /* Add every HID devices found in remote device database */
    /* They will be able to connect to our device */
    APP_INFO0("Add all HID devices found in database (if any)");
    app_read_xml_remote_devices();
    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        p_xml_dev = &app_xml_remote_devices_db[index];
        if ((p_xml_dev->in_use != FALSE) &&
            ((p_xml_dev->trusted_services & BSA_HID_SERVICE_MASK) != 0))
        {
            APP_INFO1("Adding HID Device:%s BdAddr:%02X:%02X:%02X:%02X:%02X:%02X",
                    p_xml_dev->name,
                    p_xml_dev->bd_addr[0], p_xml_dev->bd_addr[1], p_xml_dev->bd_addr[2],
                    p_xml_dev->bd_addr[3], p_xml_dev->bd_addr[4], p_xml_dev->bd_addr[5]);
            app_hh_add_dev(p_xml_dev);
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_get_dscpinfo
 **
 ** Description      Example of function to connect DHCP Info of an HID device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_get_dscpinfo(UINT8 handle)
{
    int status;
    tBSA_HH_GET get_dscpinfo;

    APP_INFO0("Get HID DSCP Info");
    if (handle == BSA_HH_INVALID_HANDLE)
    {
        /* display the known HID devices */
        if (app_hh_display_status() == 0)
        {
            APP_INFO0("No HID devices");
            return -1;
        }
        handle = app_get_choice("Enter HID handle");
    }
    BSA_HhGetInit(&get_dscpinfo);
    get_dscpinfo.request = BSA_HH_DSCP_REQ;
    get_dscpinfo.handle = handle;
    status = BSA_HhGet(&get_dscpinfo);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhGet: Unable to get DSCP Info status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_connect
 **
 ** Description      Example of function to connect to HID device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_connect(tBSA_HH_PROTO_MODE mode)
{
    int status = -1;
    tBSA_STATUS bsastatus;
    int device_index;
    BOOLEAN connect = FALSE;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_HH_OPEN hh_open_param;
    DEV_CLASS class_of_device;
    int choice, index;

    APP_INFO0("Bluetooth HID connect menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    device_index = app_get_choice("Select source");

    /* Devices from XML database */
    if (device_index == 0)
    {
        /* Read the XML file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
            p_name = app_xml_remote_devices_db[device_index].name;
            class_of_device[0] = app_xml_remote_devices_db[device_index].class_of_device[0];
            class_of_device[1] = app_xml_remote_devices_db[device_index].class_of_device[1];
            class_of_device[2] = app_xml_remote_devices_db[device_index].class_of_device[2];
            connect = TRUE;
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_disc_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_DISC_NB_DEVICES) &&
            (app_discovery_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
            p_name = app_discovery_cb.devs[device_index].device.name;
            class_of_device[0] = app_discovery_cb.devs[device_index].device.class_of_device[0];
            class_of_device[1] = app_discovery_cb.devs[device_index].device.class_of_device[1];
            class_of_device[2] = app_discovery_cb.devs[device_index].device.class_of_device[2];
            connect = TRUE;
        }
    }
    else
    {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
        return -1;
    }

    if (!connect)
    {
        APP_ERROR1("Bad Device index:%d", device_index);
        return -1;
    }
    BSA_HhOpenInit(&hh_open_param);
    bdcpy(hh_open_param.bd_addr, bd_addr);
    hh_open_param.mode = mode;
    APP_INFO0("Security?");
    APP_INFO0("   NONE: 0");
    APP_INFO0("   Authentication: 1");
    APP_INFO0("   Encryption+Authentication: 3");
    APP_INFO0("   Authorization: 4");
    APP_INFO0("   Encryption+Authentication+Authorization: 7");

    choice = app_get_choice("Enter Security mask");
    if ((choice & ~7) == 0)
    {
        hh_open_param.sec_mask = BTA_SEC_NONE;
        if (choice & 1)
        {
            hh_open_param.sec_mask |= BSA_SEC_AUTHENTICATION;
        }
        if (choice & 2)
        {
            hh_open_param.sec_mask |= BSA_SEC_ENCRYPTION;
        }
        if (choice & 4)
        {
            hh_open_param.sec_mask |= BSA_SEC_AUTHORIZATION;
        }
    }
    else
    {
        APP_ERROR1("Bad Security Mask selected :%d", choice);
        return -1;
    }
    app_hh_cb.sec_mask_out = hh_open_param.sec_mask;

    bsastatus = BSA_HhOpen(&hh_open_param);
    if (bsastatus != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhOpen failed: %d", bsastatus);
    }
    else
    {
        APP_DEBUG1("Connecting to HID device:%s", p_name);

        /* XML Database update (Service) will be done upon reception of HH OPEN event */
        /* Read the Remote device XML file to have a fresh view */
        if (app_read_xml_remote_devices() < 0)
        {
            /*
             * Must reset the hh device database if bt_devices.xml does not exist or
             * the descriptor will not be submitted to the kernel (issues seen if user
             * deletes bt_devices.xml but keeps bt_hh_devices.xml)
             */
             app_hh_db_remove_db();
        }

        app_xml_update_name_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr, p_name);

        app_xml_update_cod_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr, class_of_device);

        /* Check if the name in the inquiry responses database */
        for (index = 0; index < APP_NUM_ELEMENTS(app_discovery_cb.devs); index++)
        {
            if (!bdcmp(app_discovery_cb.devs[index].device.bd_addr, bd_addr))
            {
                app_xml_update_name_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr,
                        app_discovery_cb.devs[index].device.name);

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE) /* BLE Connection from DB */
                app_xml_update_ble_addr_type_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr,
                        app_discovery_cb.devs[index].device.ble_addr_type);

                app_xml_update_device_type_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr,
                        app_discovery_cb.devs[index].device.device_type);
#endif

                break;
            }
        }
        /* Update database => write on disk */
        app_write_xml_remote_devices();
    }

    return status;
}

/*******************************************************************************
 **
 ** Function        app_hh_open_tbfc
 **
 ** Description     Example of function to connect to HID device using TBFC Page
 **
 ** Parameters      None
 **
 ** Returns         void
 **
 *******************************************************************************/
int app_hh_open_tbfc(void)
{
    int status = -1;
    int device_index;
    BOOLEAN connect = FALSE;
    BD_ADDR bd_addr;
    UINT8 *p_name;
    tBSA_HH_OPEN hh_open_param;
    DEV_CLASS class_of_device;

    APP_INFO0("Bluetooth HID TBFC open menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    device_index = app_get_choice("Select source");

    /* Devices from XML database */
    if (device_index == 0)
    {
        /* Read the xml file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
            p_name = app_xml_remote_devices_db[device_index].name;
            class_of_device[0] = app_xml_remote_devices_db[device_index].class_of_device[0];
            class_of_device[1] = app_xml_remote_devices_db[device_index].class_of_device[1];
            class_of_device[2] = app_xml_remote_devices_db[device_index].class_of_device[2];
            connect = TRUE;
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_disc_display_devices();
        device_index = app_get_choice("Select device");
        if ((device_index >= 0) &&
            (device_index < APP_DISC_NB_DEVICES) &&
            (app_discovery_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
            p_name = app_discovery_cb.devs[device_index].device.name;
            class_of_device[0] = app_discovery_cb.devs[device_index].device.class_of_device[0];
            class_of_device[1] = app_discovery_cb.devs[device_index].device.class_of_device[1];
            class_of_device[2] = app_discovery_cb.devs[device_index].device.class_of_device[2];
            connect = TRUE;
        }
    }
    else
    {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
    }

    if (connect != FALSE)
    {
        BSA_HhOpenInit(&hh_open_param);
        bdcpy(hh_open_param.bd_addr, bd_addr);
        hh_open_param.mode = BSA_HH_PROTO_RPT_MODE;
        hh_open_param.sec_mask = BSA_SEC_AUTHENTICATION;
        app_hh_cb.sec_mask_out = hh_open_param.sec_mask;
        hh_open_param.brcm_mask = BSA_HH_BRCM_TBFC_PAGE;

        status = BSA_HhOpen(&hh_open_param);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("Unable to connect HID device status:%d", status);
        }
        else
        {
            APP_DEBUG1("Connecting to HID device:%s", p_name);

            /* XML Database update (Service) will be done upon reception of HH OPEN event */
            /* Read the Remote device XML file to have a fresh view */
            app_read_xml_remote_devices();

            app_xml_update_name_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr, p_name);

            app_xml_update_cod_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr, class_of_device);

            /* Update database => write on disk */
            app_write_xml_remote_devices();
        }
    }
    else
    {
        APP_ERROR1("Bad Device index:%d", device_index);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_disconnect
 **
 ** Description      Example of HH Close
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_disconnect(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_CLOSE hh_close_param;

    APP_INFO0("Bluetooth HID Disconnect");

    /* Show connected HID devices */
    app_hh_display_status();

    handle = app_get_choice("Enter HID Handle");

    BSA_HhCloseInit(&hh_close_param);
    hh_close_param.handle = handle;
    status = BSA_HhClose(&hh_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to disconnect HID device status:%d", status);
        return -1;
    }
    else
    {
        APP_DEBUG0("Disconnecting from HID device...");
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_idle
 **
 ** Description      Example of HH Set Idle
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_set_idle(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;

    APP_INFO0("Bluetooth HID Set Idle");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_IDLE_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.idle = 0;

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_get_idle
 **
 ** Description      Example of HH get idle
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_get_idle(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;

    APP_INFO0("Bluetooth HID Get Idle");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_IDLE_REQ;
    hh_get_param.handle = handle;

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhGet fail status:%d", status);
    }

    return status;
}


/*******************************************************************************
 **
 ** Function         app_hh_change_proto_mode
 **
 ** Description      Example of HH change protocol mode
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_change_proto_mode(tBSA_HH_HANDLE handle, tBSA_HH_PROTO_MODE mode)
{
    int status;
    tBSA_HH_SET hh_set_param;

    APP_INFO0("Bluetooth HID Change Proto Mode");
    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_PROTO_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.mode = mode;

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_proto_mode
 **
 ** Description      Example of HH set protocol mode
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_set_proto_mode(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;
    int choice;

    APP_INFO0("Bluetooth HID Set Protocol mode");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_PROTO_REQ;
    hh_set_param.handle = handle;
    APP_INFO0("    Select Protocol Mode:");
    APP_INFO0("        0 Report Mode");
    APP_INFO0("        1 Boot Mode");
    choice = app_get_choice("Protocol Mode");
    if(choice == 0)
        hh_set_param.param.mode = BSA_HH_PROTO_RPT_MODE;
    else if (choice == 1)
        hh_set_param.param.mode = BSA_HH_PROTO_BOOT_MODE;
    else{
        APP_ERROR0("Invalid choice");
        return -1;
    }

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_get_proto_mode
 **
 ** Description      Example of HH get protocol mode
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_hh_get_proto_mode(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;

    APP_INFO0("Bluetooth HID Get Proto Mode");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_PROTO_REQ;
    hh_get_param.handle = handle;

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhGet fail status:%d", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_hh_enable_mip
 **
 ** Description      Example of MIP feature enabling
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_enable_mip(void)
{
    int status;
    tBSA_HH_ADD_DEV hh_add_dev_param;

    APP_INFO0("app_hh_enable_mip");

    BSA_HhAddDevInit(&hh_add_dev_param);

    hh_add_dev_param.enable_mip = TRUE;

    status = BSA_HhAddDev(&hh_add_dev_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhAddDev: Unable to enable MIP feature status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_disable_mip
 **
 ** Description      Example of MIP feature enabling
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_disable_mip(void)
{
    int status;
    tBSA_HH_REMOVE_DEV hh_remove_dev_param;

    APP_INFO0("app_hh_enable_mip");

    BSA_HhRemoveDevInit(&hh_remove_dev_param);

    hh_remove_dev_param.disable_mip = TRUE;

    status = BSA_HhRemoveDev(&hh_remove_dev_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhRemoveDev: Unable to disable MIP feature:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_report
 **
 ** Description      Example of HH Set Report
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_set_report(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;
    UINT16 report_type, report_id;

    app_hh_display_status();

    APP_INFO0("Bluetooth HID Set Report");
    handle = app_get_choice("Enter HID Handle");
    report_type = app_get_choice("Report Type(0=RESRV,1=INPUT,2=OUTPUT,3=FEATURE)");
    report_id = app_get_choice("Enter report id");

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = report_type;

    status = app_get_hex_data("Report data (e.g. 01 AB 54 ..)",
            &hh_set_param.param.set_report.report.data[1],  BSA_HH_REPORT_SIZE_MAX - 1);
    if (status < 0)
    {
        APP_ERROR0("incorrect Data");
        return -1;
    }
    hh_set_param.param.set_report.report.length = (UINT16)status + 1;
    hh_set_param.param.set_report.report.data[0] = report_id; /* Report Id = 0x01 */
    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet fail status:%d", status);
        return -1;
    }

    return 0;
}



/*******************************************************************************
 **
 ** Function         app_hh_send_vc_unplug
 **
 ** Description      Example of HH Send Control type
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_send_vc_unplug(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SEND_CTRL param;

    app_hh_display_status();

    APP_INFO0("Bluetooth HID Send Virtual Cable Unplug");
    handle = app_get_choice("Select HID Handle");

    BSA_HhSendCtrlInit(&param);
    param.ctrl_type = BSA_HH_CTRL_VIRTUAL_CABLE_UNPLUG;
    param.handle = handle;
    status = BSA_HhSendCtrl(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSendCtrl fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_send_hid_ctrl
 **
 ** Description      Example of HH Send HID Control
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_send_hid_ctrl(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SEND_CTRL param;
    UINT8 ctrl_type;

    app_hh_display_status();

    APP_INFO0("Bluetooth send HID Control");
    handle = app_get_choice("Select HID Handle");

    BSA_HhSendCtrlInit(&param);
    ctrl_type = app_get_choice("Message type(3:suspend, 4:exit_suspend, 5:virtual cable unplug");
    param.ctrl_type = ctrl_type;
    param.handle = handle;
    status = BSA_HhSendCtrl(&param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSendCtrl fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_send_ctrl_data
 **
 ** Description     Example of HH Send Report on control Channel (any size/id)
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_send_ctrl_data(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SET hh_set_param;
    int report_id;
    int report_type;

    app_hh_display_status();

    APP_INFO0("Bluetooth HID Send Control Data");
    handle = app_get_choice("Enter HID Handle");

    report_type = app_get_choice("Report Type(0=RESRV,1=INPUT,2=OUTPUT,3=FEATURE)");
    if ((report_type < 0) || (report_type > BSA_HH_RPTT_FEATURE))
    {
        APP_ERROR1("Bad Report Type:0x%x", report_type);
        return -1;
    }

    report_id = app_get_choice("ReportId");
    if ((report_id < 0) || (report_id > 0xFF))
    {
        APP_ERROR1("Bad report Id:0x%x", report_id);
        return -1;
    }

    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = (UINT8)report_type;

    status = app_get_hex_data("Report data (e.g. 01 AB 54 ..)",
            &hh_set_param.param.set_report.report.data[1],  BSA_HH_REPORT_SIZE_MAX - 1);
    if (status < 0)
    {
        APP_ERROR0("incorrect Data");
        return -1;
    }
    hh_set_param.param.set_report.report.length = (UINT16)status + 1;
    hh_set_param.param.set_report.report.data[0] = report_id; /* Report Id = 0x01 */

    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet fail status:%d", status);
        return -1;
    }

    return status;
}

/*******************************************************************************
 **
 ** Function        app_hh_send_int_data
 **
 ** Description     Example of HH Send Report on Interrupt Channel (any size/id)
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_send_int_data(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    int report_id;
    int length;
    int index;
    tBSA_HH_SEND_DATA hh_send_data_param;

    app_hh_display_status();

    APP_INFO0("Bluetooth HID Send Interrupt Data");
    handle = app_get_choice("Enter HID Handle");
    report_id = app_get_choice("ReportId");

    if ((report_id < 0) || (report_id > 0xFF))
    {
        APP_ERROR1("Bad report Id:0x%x", report_id);
        return -1;
    }

    length = app_get_choice("Data Length (including report ID)");
    if ((length < 1) || (length >= BSA_HH_DATA_SIZE_MAX))
    {
        APP_ERROR1("Bad length Id:0x%x", length);
        return -1;
    }

    BSA_HhSendDataInit(&hh_send_data_param);
    hh_send_data_param.handle = handle;

    hh_send_data_param.data.length = (UINT16)length;
    hh_send_data_param.data.data[0] = (UINT8)report_id;

    for (index = 1 ; index < length ; index++)
    {
        hh_send_data_param.data.data[index] = (UINT8)index-1;
    }
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
 ** Function        app_hh_get_report
 **
 ** Description     Example of HH Get Report
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_get_report(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;
    UINT8 report_type;

    app_hh_display_status();

    APP_INFO0("Bluetooth HID Get Report");
    handle = app_get_choice("Enter HID Handle");
    report_type = app_get_choice("Report Type(0=RESRV,1=INPUT,2=OUTPUT,3=FEATURE)");
    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_REPORT_REQ;
    hh_get_param.handle = handle;
    hh_get_param.param.get_report.report_type = report_type;

    hh_get_param.param.get_report.report_id = app_get_choice("Enter Report ID");

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhGet fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_get_report_no_size
 **
 ** Description     Example of HH Get Report
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hh_get_report_no_size(void)
{
    int status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_GET hh_get_param;

    app_hh_display_status();

    APP_INFO0("Bluetooth HID Get Report w/o size info");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhGetInit(&hh_get_param);

    hh_get_param.request = BSA_HH_REPORT_NO_SIZE_REQ;
    hh_get_param.handle = handle;
    hh_get_param.param.get_report.report_type = BSA_HH_RPTT_INPUT;

    hh_get_param.param.get_report.report_id = app_get_choice("Enter Report ID");

    status = BSA_HhGet(&hh_get_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhGet fail status:%d", status);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hh_set_ucd_brr_mode
 **
 ** Description      Example to set UCD/BRR mode (send data)
 **
 ** Parameters
 **
 ** Returns          status (0 if successful, error code otherwise)
 **
 *******************************************************************************/
int app_hh_set_ucd_brr_mode(void)
{
    tBSA_STATUS status;
    tBSA_HH_HANDLE handle;
    tBSA_HH_SEND_DATA hh_send_data_param;

    app_hh_display_status();

    APP_INFO0("Bluetooth HID Send data (UCD mode)");
    handle = app_get_choice("Enter HID Handle");

    BSA_HhSendDataInit(&hh_send_data_param);
    hh_send_data_param.handle = handle;
    hh_send_data_param.data.data[0] = 0xFF;  /* Report Id = 0xFF */
    hh_send_data_param.data.data[1] = 0x00;  /* Report data */
    hh_send_data_param.data.length = 2;      /* Report Length */

    status = BSA_HhSendData(&hh_send_data_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSendData fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_cfg_hid_ucd
 **
 ** Description      Configure the UCD flags over HID
 **
 ** Parameters       handle: handle to send to (if BSA_HH_INVALID_HANDLE, ask)
 **                  command: HID UCD command (if 0, ask)
 **                  flags: HID UCD flags (if 0, ask)
 **
 ** Returns          status (0 if successful, error code otherwise)
 **
 *******************************************************************************/
int app_hh_cfg_hid_ucd(UINT8 handle, UINT8 command, UINT8 flags)
{
    tBSA_STATUS status;
    tBSA_HH_SET hh_set_param;
    APP_INFO0("Set HID UCD mode");

    if (handle == BSA_HH_INVALID_HANDLE)
    {
        app_hh_display_status();
        handle = app_get_choice("Enter HID handle");
    }

    if (!command)
    {
        command = app_get_choice("Enter HID UCD command (1=Set, 2=Clear, 3=Exec)");
    }

    if (!flags)
    {
        flags = app_get_choice("Enter flags to set (1=BRR, 2=UCD, 4=Multicast)");
    }


    BSA_HhSetInit(&hh_set_param);
    hh_set_param.request = BSA_HH_REPORT_REQ;
    hh_set_param.handle = handle;
    hh_set_param.param.set_report.report_type = BTA_HH_RPTT_FEATURE;
    hh_set_param.param.set_report.report.length = 3;
    hh_set_param.param.set_report.report.data[0] = 0xCC; /* Report Id = 0xCC */
    hh_set_param.param.set_report.report.data[1] = command; /* Set */
    hh_set_param.param.set_report.report.data[2] = flags; /* UCD */
    status = BSA_HhSet(&hh_set_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhSet fail status:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_init
 **
 ** Description      Initialize HH
 **
 ** Parameters       boot: indicate if the management path must be created
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_init(void)
{
    int index;
    tAPP_HH_DEVICE *p_hh_dev;

    /* Reset HH Control Block */
    memset(&app_hh_cb, 0, sizeof(app_hh_cb));

    for (index = 0 ; index < APP_NUM_ELEMENTS(app_hh_cb.devices); index++)
    {
        p_hh_dev = &app_hh_cb.devices[index];
        p_hh_dev->handle = 0; /* to remove warning message */
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
        p_hh_dev->bthid_fd = -1;
#endif
#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
        /* Force the config as a 1st approach */
        p_hh_dev->audio.format.nb_channels = 1;
        p_hh_dev->audio.format.sample_rate = 16000;
        p_hh_dev->audio.format.bits_per_sample = 16;
#ifndef PCM_ALSA
        p_hh_dev->audio.fd = -1;
#endif
#endif
    }

#if defined(APP_HH_OTA_FW_DL_INCLUDED) && (APP_HH_OTA_FW_DL_INCLUDED == TRUE)
    /* Default OTAFWU write size (for backward compatibility) - 256 matches max Hex record */
    app_hh_cb.ota_fw_dl.otafwu_writesize = APP_HH_OTAFWDL_HIDFWU_DEFAULT_WRITESIZE;
#endif

#if defined(APP_HH_BLE_AUDIO_INCLUDED) && (APP_HH_BLE_AUDIO_INCLUDED == TRUE)
    app_hh_ble_audio_sink_init();
#endif
    app_hh_cb.mip_device_nb = 0;
    app_hh_cb.mip_handle = BSA_HH_MIP_INVALID_HANDLE;

#if defined(APP_HH_AUDIO_STREAMING_INCLUDED) && (APP_HH_AUDIO_STREAMING_INCLUDED == TRUE)
    app_hh_as_init();
#endif

    app_hh_db_init();

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_exit
 **
 ** Description      Exit HH application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_exit(void)
{
    tBSA_HH_DISABLE dis_param;

    UIPC_Close(app_hh_cb.uipc_channel);  /* Close UIPC channel */
    app_hh_cb.uipc_channel = UIPC_CH_ID_BAD;
    app_hh_cb.mip_device_nb = 0;
    app_hh_cb.mip_handle = BSA_HH_MIP_INVALID_HANDLE;

    BSA_HhDisableInit(&dis_param);
    BSA_HhDisable(&dis_param);

    app_hh_db_exit();

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_3dsg_mode
 **
 ** Description      Set 3DSG mode for one device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_set_3dsg_mode(BOOLEAN enable, int req_handle, int vsync_period)
{
    tBSA_HH_SET app_hh_set;
    tBSA_HH_HANDLE handle;
    tBSA_STATUS status;

    if (req_handle  == -1)
    {
        /* Show connected HID devices */
        app_hh_display_status();
        APP_INFO1("Bluetooth HID Set 3DSG mode enable:%d", enable);
        handle = app_get_choice("Enter HID Handle");
    }
    else
    {
        handle = (tBSA_HH_HANDLE)req_handle;
    }

    BSA_HhSetInit(&app_hh_set);
    app_hh_set.handle = handle;
    if (enable)
    {
        app_hh_set.request = BSA_HH_3DSG_ON_REQ;
        APP_INFO1("These parameters are for %d ms VSync Period", vsync_period);
        app_hh_set.param.offset_3dsg.left_open = 0;
        app_hh_set.param.offset_3dsg.left_close = vsync_period / 2 - 1;
        app_hh_set.param.offset_3dsg.right_open = vsync_period / 2;
        app_hh_set.param.offset_3dsg.right_close = vsync_period - 1;
        APP_INFO1("Enabling 3DSG with LO:%d LC:%d RO:%d RC:%d",
                app_hh_set.param.offset_3dsg.left_open,
                app_hh_set.param.offset_3dsg.left_close,
                app_hh_set.param.offset_3dsg.right_open,
                app_hh_set.param.offset_3dsg.right_close);
    }
    else
    {
        app_hh_set.request = BSA_HH_3DSG_OFF_REQ;
        APP_INFO0("Disabling 3DSG");
    }
    status = BSA_HhSet(&app_hh_set);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Failed to control 3DSG status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_enable_3dsg_mode_manual
 **
 ** Description      Set 3DSG mode for one device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hh_enable_3dsg_mode_manual(void)
{
    tBSA_HH_SET app_hh_set;
    tBSA_HH_HANDLE handle;
    tBSA_STATUS status;
    UINT16 left_open;
    UINT16 left_close;
    UINT16 right_open;
    UINT16 right_close;

    APP_INFO0("Set 3DSG mode (manual Offsets/Delay)");
    handle = app_get_choice("Enter HID Handle");
    left_open = app_get_choice("Enter Left open");
    left_close = app_get_choice("Enter Left Close");
    right_open = app_get_choice("Enter Right Open");
    right_close = app_get_choice("Enter Right Close");

    BSA_HhSetInit(&app_hh_set);
    app_hh_set.handle = handle;
    app_hh_set.request = BSA_HH_3DSG_ON_REQ;
    app_hh_set.param.offset_3dsg.left_open = left_open;
    app_hh_set.param.offset_3dsg.left_close = left_close;
    app_hh_set.param.offset_3dsg.right_open = right_open;
    app_hh_set.param.offset_3dsg.right_close = right_close;

    APP_INFO1("Enabling 3DSG with LO:%d LC:%d RO:%d RC:%d",
            app_hh_set.param.offset_3dsg.left_open,
            app_hh_set.param.offset_3dsg.left_close,
            app_hh_set.param.offset_3dsg.right_open,
            app_hh_set.param.offset_3dsg.right_close);

    status = BSA_HhSet(&app_hh_set);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Failed to Enable 3DSG status:%d", status);
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_hh_display_status
 **
 ** Description      Display HID Device status
 **
 ** Parameters
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hh_display_status(void)
{
    tAPP_HH_DEVICE *p_dev;
    int index, nb_devices = 0;

    APP_INFO0("Known HID device(s):");

    /* Display status of every HH connection */
    p_dev = &app_hh_cb.devices[0];
    for (index = 0; index < APP_NUM_ELEMENTS(app_hh_cb.devices); index++, p_dev++)
    {
        if (p_dev->info_mask & APP_HH_DEV_USED)
        {
            nb_devices++;
            APP_INFO1("Handle:%d Index:%d BdAddr:%02X:%02X:%02X:%02X:%02X:%02X (%s)",
                p_dev->handle, index,
                p_dev->bd_addr[0], p_dev->bd_addr[1], p_dev->bd_addr[2],
                p_dev->bd_addr[3], p_dev->bd_addr[4], p_dev->bd_addr[5],
                (p_dev->info_mask & APP_HH_DEV_OPENED)?"OPENED":"CLOSED");
        }
    }

    if (nb_devices == 0)
    {
        APP_INFO0("No HID Connection");
    }

    return nb_devices;
}

/*******************************************************************************
 **
 ** Function         app_hh_remove_dev
 **
 ** Description      Remove a device
 **
 ** Parameters
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hh_remove_dev(void)
{
    int dev_index;
    int hh_dev_index;
    int status = 0;
    tBSA_HH_REMOVE_DEV hh_remove;
    tBSA_SEC_REMOVE_DEV sec_remove;
    tAPP_HH_DEVICE *p_hh_dev;
    tAPP_XML_REM_DEVICE *p_xml_dev;
    BD_ADDR bd_addr;
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    tAPP_BLE_CLIENT_DB_ELEMENT *p_blecl_db_elmt;
#endif

    /* Example to Remove an HID Device (Server, Sec and HH Databases) */
    APP_INFO0("Remove HH Device");

    /* display the known HID devices */
    if (app_hh_display_status() == 0)
    {
        APP_INFO0("No HID devices");
        return -1;
    }

    hh_dev_index = app_get_choice("Enter the index of the device to remove");

    /* Check hid device index */
    if ((hh_dev_index < 0) ||
        (hh_dev_index >= APP_HH_DEVICE_MAX))
    {
        APP_INFO0("Bad HH Device index");
        return -1;
    }
    p_hh_dev = &app_hh_cb.devices[hh_dev_index];

    /* Check hid device */
    if ((p_hh_dev->info_mask & APP_HH_DEV_USED) == 0)
    {
        APP_INFO0("HH Device not in use");
        return -1;
    }

    /* Save the BdAddr for future use */
    bdcpy(bd_addr, p_hh_dev->bd_addr);

    /* Remove this device from BSA's HH database */
    BSA_HhRemoveDevInit(&hh_remove);
    bdcpy(hh_remove.bd_addr, bd_addr);

    status = BSA_HhRemoveDev(&hh_remove);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_HhRemoveDev failed: %d",status);
        return -1;
    }

#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    if (p_hh_dev->bthid_fd != -1)
    {
        app_bthid_close(p_hh_dev->bthid_fd);
        p_hh_dev->bthid_fd = -1;
    }
#endif
    /* Remove the device from HH application's memory */
    p_hh_dev->info_mask = 0;

    /* Remove the device from the HH application's database */
    app_hh_db_remove_by_bda(bd_addr);

    /* Remove the device from Security database (BSA Server) */
    BSA_SecRemoveDeviceInit(&sec_remove);
    bdcpy(sec_remove.bd_addr, bd_addr);
    status = BSA_SecRemoveDevice(&sec_remove);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_SecRemoveDevice failed: %d",status);
        /* No return on purpose (to erase it from XML database) */
    }

    /* Remove the device from Security database (XML) */
    app_read_xml_remote_devices();

    for (dev_index = 0 ; dev_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; dev_index++)
    {
        p_xml_dev = &app_xml_remote_devices_db[dev_index];
        if ((p_xml_dev->in_use) &&
            (bdcmp(p_xml_dev->bd_addr, bd_addr) == 0))
        {
            /* Mark this device as unused */
            p_xml_dev->in_use = FALSE;
            /* Update XML database */
            app_write_xml_remote_devices();
        }
    }

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    APP_INFO0("Try to remove device from BLE DB");

    /* search BLE DB */
    p_blecl_db_elmt = app_ble_client_db_find_by_bda(bd_addr);
    if(p_blecl_db_elmt != NULL)
    {
        APP_DEBUG0("Device present in BLE DB, remove it.");
        app_ble_client_db_clear_by_bda(bd_addr);
        app_ble_client_db_save();
    }
    else
    {
        APP_DEBUG0("Device Not present in BLE DB");
    }
#endif

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_send_set_prio
 **
 ** Description      Send set priority for the HID audio
 **
 ** Parameters       handle: HID handle of the device
 **                  bd_addr: BDADDR of RC
 **                  priority: priority value requested for channel
 **                  direction: direction of audio data
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_hh_send_set_prio(tBSA_HH_HANDLE handle, BD_ADDR bd_addr, UINT8 priority, UINT8 direction)
{
    int status = 0;
    tBSA_HH_SET app_hh_set;

    /* Send Set Priority Ext command for HID audio*/
    APP_INFO1("app_hh_send_set_prio priority:%d direction:%d", priority, direction);

    BSA_HhSetInit(&app_hh_set);
    app_hh_set.handle = handle;
    app_hh_set.request = BSA_HH_SET_PRIO_EXT_REQ;
    bdcpy(app_hh_set.param.prio.bd_addr, bd_addr);
    app_hh_set.param.prio.priority = priority;
    app_hh_set.param.prio.direction = direction;

    status = BSA_HhSet(&app_hh_set);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Failed to set priority status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hh_set_prio_audio
 **
 ** Description      Set priority for the HID audio
 **
 ** Parameters
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hh_set_prio_audio(void)
{
    int hh_dev_index;
    tAPP_HH_DEVICE *p_hh_dev;
    UINT8 priority;
    UINT8 direction;

    /* Example to Set Priority for HID audio*/
    APP_INFO0("Set Priority Ext (for HID audio)");

    /* display the known HID devices */
    if (app_hh_display_status() == 0)
    {
        APP_INFO0("No HID devices");
        return -1;
    }

    hh_dev_index = app_get_choice("Enter the index of the device");

    /* Check hid device index */
    if ((hh_dev_index < 0) ||
        (hh_dev_index >= APP_HH_DEVICE_MAX))
    {
        APP_INFO0("Bad HH Device index");
        return -1;
    }
    p_hh_dev = &app_hh_cb.devices[hh_dev_index];

    /* Check hid device */
    if ((p_hh_dev->info_mask & APP_HH_DEV_USED) == 0)
    {
        APP_INFO0("HH Device not in use");
        return -1;
    }

    priority = app_get_choice("Enter priority (0=Normal, 1=High)");
    if(priority > 0)
    {
       priority = L2CAP_PRIORITY_HIGH;
       APP_INFO1("High priority selected - priority =%d",priority);
    }
    else
    {
       priority = L2CAP_PRIORITY_NORMAL;
       APP_INFO1("Normal priority selected - priority =%d",priority);
    }

    direction = app_get_choice("Enter Audio direction (0=Source, 1=Sink)");
    if(direction > 0)
    {
       direction = L2CAP_DIRECTION_DATA_SINK;
       APP_INFO1("Audio Sink selected - direction =%d",direction);
    }
    else
    {
       direction = L2CAP_DIRECTION_DATA_SOURCE;
       APP_INFO1("Audio Source selected - direction =%d",direction);
    }

    app_hh_send_set_prio(p_hh_dev->handle,p_hh_dev->bd_addr, priority, direction);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_event_cback_register
 **
 ** Description     Register a callback function
 **
 ** Parameters      p_funct: Callback function
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_hh_event_cback_register(tAPP_HH_EVENT_CBACK *p_funct)
{
    app_hh_cb.p_event_cback = p_funct;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_uipc_cback_register
 **
 ** Description     Register a callback function
 **
 ** Parameters      p_funct: Callback function
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_hh_uipc_cback_register(tAPP_HH_CUSTOM_UIPC_CBACK *p_funct)
{
    app_hh_cb.p_uipc_cback = p_funct;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_add_dev_cback_register
 **
 ** Description     Register a callback function
 **
 ** Parameters      p_funct: Callback function
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_hh_add_dev_cback_register(tAPP_HH_ADD_DEV_CBACK *p_funct)
{
    app_hh_cb.p_add_dev_cback = p_funct;
    return 0;
}
