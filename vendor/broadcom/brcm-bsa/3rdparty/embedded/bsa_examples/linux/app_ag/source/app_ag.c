/*****************************************************************************
 **
 **  Name:           app_ag.c
 **
 **  Description:    Bluetooth AG application
 **
 **  Copyright (c) 2010-2016, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <unistd.h>
#include <fcntl.h>

#include "app_ag.h"
#include "app_utils.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_thread.h"

#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
#include "alsa/asoundlib.h"
#endif
#endif /* PCM_ALSA */
/*
 * Defines
 */
#define APP_AG_SAMPLE_RATE      8000      /* AG Voice sample rate is always 8KHz */
#define APP_AG_BITS_PER_SAMPLE  16        /* AG Voice sample size is 16 bits */
#define APP_AG_CHANNEL_NB       1         /* AG Voice sample in mono */

#define APP_AG_FEATURES  (BSA_AG_FEAT_ECNR | BSA_AG_FEAT_3WAY | \
                          BSA_AG_FEAT_VREC | BSA_AG_FEAT_ECS |  \
                          BSA_AG_FEAT_ECC | BSA_AG_FEAT_CODEC)

#define APP_AG_XML_REM_DEVICES_FILE_PATH "./bt_devices.xml"
//#define APP_MAX_AUDIO_BUF 240

/*
 * Global variables
 */
tAPP_AG_CB app_ag_cb;

const char hsp_serv_name[] = "BSA Headset";
const char hfp_serv_name[] = "BSA Handsfree";

#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
static char *alsa_device = "default"; /* ALSA playback device */
static snd_pcm_t *alsa_handle_playback = NULL;
static snd_pcm_t *alsa_handle_capture = NULL;
static BOOLEAN alsa_capture_opened = FALSE;
static BOOLEAN alsa_playback_opened = FALSE;
#endif
#endif /* PCM_ALSA */

/*
 * Local functions
 */
void app_ag_cback(tBSA_AG_EVT event, tBSA_AG_MSG *p_data);
char *app_ag_status_2_str(tBSA_STATUS status);
void app_ag_sco_uipc_cback(BT_HDR *p_buf);

#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
int app_ag_open_alsa_duplex(void);
int app_ag_close_alsa_duplex(void);
#endif
#endif /* PCM_ALSA */

static tBSA_AG_CBACK *s_pCallback = NULL;

/*******************************************************************************
 **
 ** Function         app_ag_read_xml_remote_devices
 **
 ** Description      This function is used to read the XML Bluetooth remote device file
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_ag_read_xml_remote_devices(void)
{
    int status;
    int index;

    for (index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
    {
        app_xml_remote_devices_db[index].in_use = FALSE;
    }

    status = app_xml_read_db(APP_AG_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    if (status < 0)
    {
        APP_ERROR1("app_xml_read_db failed (%d)", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function         app_ag_get_index_by_handle
**
** Description      Find a connection index by its handle
**
** Returns          Index
*******************************************************************************/
static UINT8 app_ag_get_index_by_handle(UINT16 handle)
{
    UINT8 index;

    for (index=0; index < APP_AG_MAX_NUM_CONN; index++)
    {
        if (app_ag_cb.hndl[index] == handle)
        {
            break;
        }
    }

    return index;
}

/*******************************************************************************
 **
 ** Function         app_ag_enable
 **
 ** Description      enable mono AG service
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_enable(void)
{
    tBSA_STATUS status;
    tBSA_AG_ENABLE param;

    APP_DEBUG0("Entering");

    status = BSA_AgEnableInit(&param);
    param.p_cback = app_ag_cback;
    status = BSA_AgEnable(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgEnable failed(%d)", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_ag_disable
 **
 ** Description      disable mono AG service
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_disable(void)
{
    tBSA_STATUS status;
    tBSA_AG_DISABLE param;
    int index ;

    APP_DEBUG0("Entering");

    status = BSA_AgDisableInit(&param);
    status = BSA_AgDisable(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgDisable failed(%d)", status);
    }
    else
    {
        /* Ensure deregister steps have been done */
        for (index=0; index < APP_AG_MAX_NUM_CONN; index++)
        {
            if ((app_ag_cb.sco_route == BSA_SCO_ROUTE_HCI) &&
                (app_ag_cb.uipc_channel[index] != UIPC_CH_ID_BAD))
            {
                UIPC_Close(app_ag_cb.uipc_channel[index]);
                app_ag_cb.uipc_channel[index] = UIPC_CH_ID_BAD;
            }

            if (app_ag_cb.hndl[index] != BSA_AG_BAD_HANDLE)
            {
                app_ag_cb.hndl[index] = BSA_AG_BAD_HANDLE;
            }
        }
    }


    return status;
}

/*******************************************************************************
 **
 ** Function         app_ag_register
 **
 ** Description      Register one AG instance
 **
 ** Parameters       SCO route path(PCM or HCI)
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_register(UINT8 sco_route)
{
    tBSA_STATUS status;
    tBSA_AG_REGISTER param;

    UINT8 index;

    APP_DEBUG0("Entering");

    for (index=0; index < APP_AG_MAX_NUM_CONN; index++)
    {
        if (app_ag_cb.hndl[index] == BSA_AG_BAD_HANDLE)
        {
            break;
        }
    }

    if (index == APP_AG_MAX_NUM_CONN)
    {
        APP_ERROR1("Can not register more than %d AG",APP_AG_MAX_NUM_CONN);
        return BSA_ERROR_CLI_BUSY;
    }

    BSA_AgRegisterInit(&param);

    /* prepare parameters */
    param.services = BSA_HSP_SERVICE_MASK | BSA_HFP_SERVICE_MASK;
    param.sec_mask = BSA_SEC_NONE;
    param.features = APP_AG_FEATURES;
    strncpy(param.service_name[0], hsp_serv_name, sizeof(param.service_name[0]));
    param.service_name[0][sizeof(param.service_name[0]) - 1] = '\0';
    strncpy(param.service_name[1], hfp_serv_name, sizeof(param.service_name[1]));
    param.service_name[1][sizeof(param.service_name[1]) - 1] = '\0';

    /* SCO routing options:  BSA_SCO_ROUTE_HCI or BSA_SCO_ROUTE_PCM */
    param.sco_route = sco_route;
    /* save SCO routing option to match with the Audio open request */
    app_ag_cb.sco_route = param.sco_route;

    status = BSA_AgRegister(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgRegister failed(%d)", status);
    }
    else
    {
        app_ag_cb.hndl[index] = param.hndl;
        app_ag_cb.uipc_channel[index] = param.uipc_channel;
        APP_DEBUG1("Register complete handle %d, status %d/%s", app_ag_cb.hndl[index],
                status, app_ag_status_2_str(status));
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_ag_deregister
 **
 ** Description      deregister all AG instance
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_deregister(void)
{
    tBSA_STATUS status = BSA_SUCCESS;
    tBSA_AG_DEREGISTER param;
    UINT8 index;

    APP_DEBUG0("Entering");

    BSA_AgDeregisterInit(&param);

    for (index=0; index < APP_AG_MAX_NUM_CONN; index++)
    {
        if (app_ag_cb.hndl[index] != BSA_AG_BAD_HANDLE)
        {
            param.hndl = app_ag_cb.hndl[index];

            status = BSA_AgDeregister(&param);
            if (status != BSA_SUCCESS)
            {
                APP_ERROR1("BSA_AgDeregister failed(%d)", status);
                return status;
            }
            else
            {
                if (app_ag_cb.uipc_channel[index] != UIPC_CH_ID_BAD)
                {
                    if(app_ag_cb.voice_opened)
                    {
                        UIPC_Close(app_ag_cb.uipc_channel[index]);
                        app_ag_cb.voice_opened = FALSE;
                    }
                    app_ag_cb.uipc_channel[index] = UIPC_CH_ID_BAD;
                }
                app_ag_cb.hndl[index] = BSA_AG_BAD_HANDLE;
            }
        }
    }

    return BSA_SUCCESS;
}

/*******************************************************************************
 **
 ** Function         app_ag_open
 **
 ** Description      Establishes mono headset connections
 **
 ** Parameters       BD_ADDR of the deivce to connect (if null, user will be prompted)
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
int app_ag_open(BD_ADDR *bd_addr_in /*= NULL*/)
{
    tBSA_STATUS status = 0;
    BD_ADDR bd_addr;
    int device_index;

    tBSA_AG_OPEN param;

    if(bd_addr_in == NULL)
    {
        APP_DEBUG0("Entering");

        APP_INFO0("Bluetooth AG menu:");
        APP_INFO0("    0 Device from XML database (already paired)");
        APP_INFO0("    1 Device found in last discovery");
        device_index = app_get_choice("Select source");
        /* Devices from XML database */
        if (device_index == 0)
        {
            /* Read the Remote device xml file to have a fresh view */
            app_ag_read_xml_remote_devices();

            app_xml_display_devices(app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));
            device_index = app_get_choice("Select device");
            if ((device_index >= 0) &&
                (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
                (app_xml_remote_devices_db[device_index].in_use != FALSE))
            {
                bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
            }
            else
            {
                APP_ERROR1("Bad Device Index: %d", device_index);
                return -1;
            }
        }
        /* Devices from Discovery */
        else
        {
            app_disc_display_devices();
            device_index = app_get_choice("Select device");
            if ((device_index >= 0) &&
                (device_index < APP_DISC_NB_DEVICES) &&
                (app_discovery_cb.devs[device_index].in_use != FALSE))
            {
                bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
            }
            else
            {
                APP_ERROR1("Bad Device Index: %d", device_index);
                return -1;
            }
        }
    }
    else
    {
        bdcpy(bd_addr, *bd_addr_in);
    }

    BSA_AgOpenInit(&param);

    bdcpy(param.bd_addr, bd_addr);
    param.hndl = app_ag_cb.hndl[0];
    status = BSA_AgOpen(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgOpen failed (%d)", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ag_close
 **
 ** Description      Closes mono headset connections
 **
 ** Returns          TRUE or FALSE for success or failure
 *******************************************************************************/
tBSA_STATUS app_ag_close(void)
{
    tBSA_STATUS status;
    tBSA_AG_CLOSE param;

    APP_DEBUG0("Entering");

    if (app_ag_cb.opened == FALSE)
    {
        status = BSA_ERROR_CLI_NOT_CONNECTED;
        APP_ERROR0("app_ag_close failed, no AG connection");
        return status;
    }

    status = BSA_AgCloseInit(&param);
    param.hndl = app_ag_cb.hndl[0];
    status = BSA_AgClose(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgClose failed(%d)", status);
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_ag_open_audio
 **
 ** Description      open ag sco link
 **
 ** Returns          TRUE or FALSE for success or failure
 *******************************************************************************/
void app_ag_open_audio(void)
{
    tBSA_STATUS status;
    tBSA_AG_AUDIO_OPEN param;

    APP_DEBUG0("Entering");

    if (app_ag_cb.opened == FALSE)
    {
        APP_ERROR0("AG connection must be established first");
        return;
    }

    status = BSA_AgAudioOpenInit(&param);
    param.hndl = app_ag_cb.hndl[0];
    param.sco_route = app_ag_cb.sco_route;
    status = BSA_AgAudioOpen(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgAudioOpen failed (%d)", status);
    }

}

/*******************************************************************************
 **
 ** Function         app_ag_close_audio
 **
 ** Description      close ag sco link
 **
 ** Returns          TRUE or FALSE for success or failure
 *******************************************************************************/
void app_ag_close_audio(void)
{
    tBSA_STATUS status;
    tBSA_AG_AUDIO_CLOSE param;

    APP_DEBUG0("Entering");

    status = BSA_AgAudioCloseInit(&param);
    param.hndl = app_ag_cb.hndl[0];
    status = BSA_AgAudioClose(&param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgAudioClose failed: %d", status);
    }
}

/*******************************************************************************
 **
 ** Function        app_ag_close_rec_file
 **
 ** Description     Close recording file
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_ag_close_rec_file(void)
{
    if (app_ag_cb.fd_sco_in_file >=0)
    {
        app_wav_close_file(app_ag_cb.fd_sco_in_file, &(app_ag_cb.audio_format));
        app_ag_cb.fd_sco_in_file = -1;
    }
}

/*******************************************************************************
 **
 ** Function        app_ag_close_playing_file
 **
 ** Description     Close playing file
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_ag_close_playing_file(void)
{
    if (app_ag_cb.fd_sco_out_file >=0)
    {
        close(app_ag_cb.fd_sco_out_file);
        app_ag_cb.fd_sco_out_file = -1;
    }
}

/*******************************************************************************
 **
 ** Function        app_ag_open_rec_file
 **
 ** Description     Open recording file
 **
 ** Parameters      filename to open
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_ag_open_rec_file(char *filename)
{
    app_ag_cb.fd_sco_in_file = app_wav_create_file(filename, O_EXCL);

    app_ag_cb.audio_format.bits_per_sample = APP_AG_BITS_PER_SAMPLE;
    app_ag_cb.audio_format.nb_channels = APP_AG_CHANNEL_NB;
    app_ag_cb.audio_format.sample_rate = APP_AG_SAMPLE_RATE;

}

/*******************************************************************************
 **
 ** Function         app_ag_write_to_file
 **
 ** Description      write SCO IN data to file
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_ag_write_to_file(UINT8 *p_buf, int size)
{
    if (app_ag_cb.fd_sco_in_file < 0)
    {
        return 0;
    }
    return write(app_ag_cb.fd_sco_in_file, p_buf, size);

}

/*******************************************************************************
 **
 ** Function         app_ag_sco_out_read_file
 **
 ** Description      fill up a PCM buffer
 **
 ** Parameters
 **
 ** Returns          Number of bytes read (0 if error or end of file)
 **
 *******************************************************************************/
int app_ag_sco_out_read_file(void)
{
    tAPP_WAV_FILE_FORMAT format;
    int nb_bytes = 0;
    int status;

    /* Check if file is already open */
    if ((app_ag_cb.fd_sco_out_file < 0))
    {
        /* Open only if audio is opened */
        if (app_ag_cb.voice_opened)
        {
            app_ag_cb.fd_sco_out_file = app_wav_open_file(APP_AG_SCO_OUT_SOUND_FILE, &format);
        }

        if (app_ag_cb.fd_sco_out_file < 0)
        {
            APP_ERROR1("Could not open file: %s", APP_AG_SCO_OUT_SOUND_FILE);
            return 0;
        }
    }

    nb_bytes = read(app_ag_cb.fd_sco_out_file, app_ag_cb.pcmbuf, sizeof(app_ag_cb.pcmbuf));

    if (nb_bytes <= 0)
    {
        APP_INFO0("End of SCO file reached");
        status = close(app_ag_cb.fd_sco_out_file);
        if (status < 0)
        {
            APP_ERROR1("closed failed: %d", status);
        }
        app_ag_cb.fd_sco_out_file = -1;
    }

    return nb_bytes;
}

#ifndef PCM_ALSA_AG
/*******************************************************************************
 **
 ** Function         app_ag_play_file_thread
 **
 ** Description      Play SCO data from file to SCO OUT channel
 **
 ** Parameters
 **
 ** Returns          None
 **
 *******************************************************************************/
static void  app_ag_play_file_thread(void)
{
    int status = -1;
    int nb_bytes = 0;
    tAPP_WAV_FILE_FORMAT wav_format;
    int index;

    APP_DEBUG0("Entering");

    if (!app_ag_cb.voice_opened)
    {
        APP_ERROR0("Could not play file, voice connection not opened");
        return;
    }

    if ((app_wav_format(APP_AG_SCO_OUT_SOUND_FILE, &wav_format) == 0) &&
        (wav_format.bits_per_sample == APP_AG_BITS_PER_SAMPLE)        &&
        (wav_format.nb_channels     == APP_AG_CHANNEL_NB)             &&
        (wav_format.sample_rate     == APP_AG_SAMPLE_RATE))
    {
        status = 0;
    }
    if (status != 0)
    {
        APP_ERROR1("'%s' does not exist or is not right format (%d/%d/%d)",
                APP_AG_SCO_OUT_SOUND_FILE, APP_AG_SAMPLE_RATE, APP_AG_CHANNEL_NB, APP_AG_BITS_PER_SAMPLE);
        return;
    }

    for (index = 0; index < APP_AG_MAX_NUM_CONN; index++)
    {
        /* Play on the first channel id */
        if (app_ag_cb.uipc_channel[index] != UIPC_CH_ID_BAD)
            break;
    }

    if (index == APP_AG_MAX_NUM_CONN)
    {
        return;
    }

    do
    {
        if ((nb_bytes = app_ag_sco_out_read_file()) > 0)
        {
            if (!UIPC_Send(app_ag_cb.uipc_channel[index], 0, app_ag_cb.pcmbuf, nb_bytes))
            {
                APP_ERROR1("UIPC_Send(%d) failed", app_ag_cb.uipc_channel[index]);
            }
        }
        else if (nb_bytes < 0)
        {
            status = -1;
            nb_bytes = 0;
        }

    } while ((nb_bytes != 0) && app_ag_cb.voice_opened && !app_ag_cb.stop_play);

    if (app_ag_cb.fd_sco_out_file >= 0)
    {
        status = close(app_ag_cb.fd_sco_out_file);
        if (status < 0)
        {
            APP_ERROR1("close failed: %d", status);
        }
        app_ag_cb.fd_sco_out_file = -1;
    }

    if (status == 0 && !app_ag_cb.stop_play )
    {
        sleep(5);
        APP_INFO0("Finished reading audio file...all data sent to SCO");
    }
}

#endif

/*******************************************************************************
 **
 ** Function         app_ag_play_file
 **
 ** Description      Play SCO data from file to SCO OUT channel
 **
 ** Parameters
 **
 ** Returns          0 or -1 in case of error
 **
 *******************************************************************************/
int app_ag_play_file(void)
{
#ifndef PCM_ALSA_DISABLE_AG
    APP_ERROR0("Cannot play file when PCM_ALSA is defined");
    return -1;
#else
    tAPP_THREAD app_ag_play_file_struct;

    app_ag_cb.stop_play = FALSE;
    return app_create_thread(app_ag_play_file_thread, 0, 0, &app_ag_play_file_struct);
#endif
}

/*******************************************************************************
 **
 ** Function         app_ag_stop_play_file
 **
 ** Description      Stop playing SCO data from file
 **
 ** Parameters
 **
 ** Returns          0 or -1 in case of error
 **
 *******************************************************************************/
int app_ag_stop_play_file(void)
{
    app_ag_cb.stop_play = TRUE;
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_ag_pickupcall
 **
 ** Description      Pickup a call
 **
 ** Parameters
 **         handle: connection handle
 **
  ** Returns          void
 **
 *******************************************************************************/
void app_ag_pickupcall(UINT16 handle)
{
    tBSA_AG_RES bsa_ag_res;

    /* send response to open the SCO link */
    bsa_ag_res.hndl = handle;
    bsa_ag_res.result = BTA_AG_IN_CALL_CONN_RES;
    bsa_ag_res.data.audio_handle = handle;
    /* ATOK was already sent by BTA */
    BSA_AgResult(&bsa_ag_res);
    app_ag_cb.call_active = TRUE;
}

/*******************************************************************************
 **
 ** Function         app_ag_hangupcall
 **
 ** Description      Hangup a call
 **
 ** Parameters
 **         handle: connection handle
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_hangupcall(UINT16 handle)
{
    tBSA_AG_RES bsa_ag_res;

    /* send response to open the SCO link */
    bsa_ag_res.hndl = handle;
    bsa_ag_res.result = BTA_AG_END_CALL_RES;
    BSA_AgResult(&bsa_ag_res);
    app_ag_cb.call_active = FALSE;
}

/*******************************************************************************
 **
 ** Function         app_ag_sco_uipc_cback
 **
 ** Description     uipc audio call back function.
 **
 ** Parameters
 **
 **             BT_HDR *p_buf: Pointer on BT header
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_sco_uipc_cback(BT_HDR *p_buf)
{
    UINT8 *pp ;
    UINT16 pkt_len ;
#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
    snd_pcm_sframes_t alsa_frames;
    snd_pcm_sframes_t alsa_frames_expected;
#endif
#endif /* PCM_ALSA */

    if (p_buf == NULL)
    {
        return;
    }

    pp = (UINT8 *) (p_buf + 1);
    pkt_len = p_buf->len;

    /* write to file if open */
    app_ag_write_to_file(pp, pkt_len);

#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
    /* Compute number of PCM samples (contained in pkt_len->len bytes) */
    /* Divide by the number of channel */
    alsa_frames_expected = pkt_len / APP_AG_CHANNEL_NB;
    alsa_frames_expected /= 2; /* 16 bits samples */

    if (alsa_playback_opened != FALSE)
    {
        /*
         * Send PCM samples to ALSA/asound driver (local sound card)
         */
        alsa_frames = snd_pcm_writei(alsa_handle_playback, pp, alsa_frames_expected);
        if (alsa_frames < 0)
        {
            APP_DEBUG1("snd_pcm_recover %d", (int)alsa_frames);
            alsa_frames = snd_pcm_recover(alsa_handle_playback, alsa_frames, 0);
        }
        if (alsa_frames < 0)
        {
            APP_ERROR1("snd_pcm_writei failed: %s", snd_strerror(alsa_frames));
        }
        if (alsa_frames > 0 && alsa_frames < alsa_frames_expected)
        {
            APP_ERROR1("Short write (expected %d, wrote %d)",
                    (int)alsa_frames_expected, (int)alsa_frames);
        }
    }

    if (alsa_capture_opened != FALSE)
    {
        /*
         * Read PCM samples from ALSA/asound driver (local sound card)
         */
        alsa_frames = snd_pcm_readi(alsa_handle_capture, app_ag_cb.pcmbuf, alsa_frames_expected);
        if (alsa_frames < 0)
        {
            APP_ERROR1("snd_pcm_readi returns: %d", (int)alsa_frames);
        }
        else if ((alsa_frames > 0) &&
                 (alsa_frames < alsa_frames_expected))
        {
            APP_ERROR1("Short read (expected %i, wrote %i)", (int)alsa_frames_expected, (int)alsa_frames);
        }
        /* Send them to UIPC (to Headet) */
        if (TRUE != UIPC_Send(app_ag_cb.uipc_channel[0], 0, app_ag_cb.pcmbuf, alsa_frames * 2))
        {
            APP_ERROR0("UIPC_Send failed");
        }
    }
#endif
#endif /* PCM_ALSA */

    GKI_freebuf(p_buf);

}

/*******************************************************************************
 **
 ** Function         app_ag_status_2_str
 **
 ** Description      convert a tBSA_STATUS to a string of char
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
char *app_ag_status_2_str(tBSA_STATUS status)
{
    switch (status)
    {
        case BSA_SUCCESS                    : return "success";
        case BSA_ERROR_SRV_AG_OFFSET        : return "offset";
        case BSA_ERROR_SRV_AG_FAIL_SDP      : return "sdp failed";
        case BSA_ERROR_SRV_AG_FAIL_RFCOMM   : return "Open failed due to RFCOMM";
        case BSA_ERROR_SRV_AG_FAIL_RESOURCES: return "bsa ag out of resources";
        case BSA_ERROR_SRV_AG_BUSY          : return "bsa server busy";
        case BSA_ERROR_SRV_NYI              : return "Not Yet Implemented Feature Error";
        default                             : return "unknown error";
    }
}

/*******************************************************************************
 **
 ** Function         app_ag_cback
 **
 ** Description      Example of AG callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_cback(tBSA_AG_EVT event, tBSA_AG_MSG *p_data)
{
    UINT16 handle = 0;
    UINT8 index = 0;
    tBSA_AG_RES bsa_ag_res;

    if (p_data == NULL)
    {
        APP_ERROR0("NULL data pointer");
        return;
    }

    handle = p_data->hdr.hndl;
    index = app_ag_get_index_by_handle(handle);

    APP_DEBUG1("event:%d for handle: %d, index: %d", event, handle, index);
    switch (event)
    {
    case BSA_AG_OPEN_EVT: /* RFCOM level connection */
        APP_DEBUG1("BSA_AG_OPEN_EVT: s=%d(%s)", p_data->open.status, app_ag_status_2_str(p_data->open.status));
        APP_DEBUG1("%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->open.bd_addr[0], p_data->open.bd_addr[1], p_data->open.bd_addr[2],
                p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
        if (p_data->open.status == BSA_SUCCESS)
        {
            app_ag_cb.opened = TRUE;
        }
        else
        {
            app_ag_cb.opened = FALSE;
        }

        break;
    case BSA_AG_CONN_EVT: /* Service level connection */
        APP_DEBUG1("BSA_AG_CONN_EVT: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->conn.bd_addr[0], p_data->conn.bd_addr[1], p_data->conn.bd_addr[2],
                p_data->conn.bd_addr[3], p_data->conn.bd_addr[4], p_data->conn.bd_addr[5]);
        break;

    case BSA_AG_CLOSE_EVT: /* Connection Closed (for info)*/
        APP_DEBUG1("BSA_AG_CLOSE_EVT: s=%d(%s)", p_data->close.status, app_ag_status_2_str(p_data->close.status));
        app_ag_cb.opened = FALSE;
        break;

    case BSA_AG_AUDIO_OPEN_EVT: /* Audio Open Event */
        APP_DEBUG1("BSA_AG_AUDIO_OPEN_EVT: s=%d(%s)", p_data->hdr.status, app_ag_status_2_str(p_data->hdr.status));

        if(index >= APP_AG_MAX_NUM_CONN)
            break;

        if (app_ag_cb.sco_route == BSA_SCO_ROUTE_HCI &&
           app_ag_cb.uipc_channel[index] != UIPC_CH_ID_BAD &&
           !app_ag_cb.voice_opened)
        {
            /* Open UIPC channel for TX channel ID */
            if (UIPC_Open(app_ag_cb.uipc_channel[index], app_ag_sco_uipc_cback) != TRUE)
            {
                APP_ERROR1("UIPC_Open(%d) failed", app_ag_cb.uipc_channel[index]);
                break;
            }
            app_ag_cb.voice_opened = TRUE;
            UIPC_Ioctl(app_ag_cb.uipc_channel[index],UIPC_REG_CBACK,(void*) app_ag_sco_uipc_cback);
        }

#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
        app_ag_open_alsa_duplex();
#endif
#endif /* PCM_ALSA */
        break;

    case BSA_AG_AUDIO_CLOSE_EVT: /* Audio Close event */
        APP_DEBUG1("BSA_AG_AUDIO_CLOSE_EVT: s=%d(%s)", p_data->hdr.status, app_ag_status_2_str(p_data->hdr.status));

        if(index >= APP_AG_MAX_NUM_CONN)
            break;

        if(app_ag_cb.uipc_channel[index] != UIPC_CH_ID_BAD &&
           app_ag_cb.voice_opened)
        {
            UIPC_Close(app_ag_cb.uipc_channel[index]);
            app_ag_cb.voice_opened = FALSE;
        }

#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
        app_ag_close_alsa_duplex();
#endif
#endif /* PCM_ALSA */
        break;

    case BSA_AG_WBS_EVT: /* WBS (for info)*/
        APP_DEBUG1("BSA_AG_WBS_EVT: s=%d(%s)", p_data->hdr.status, app_ag_status_2_str(p_data->hdr.status));
        break;

    case BSA_AG_SPK_EVT:
        APP_DEBUG1("BSA_AG_SPK_EVT %d", p_data->val.num);
        break;
    case BSA_AG_MIC_EVT:
        APP_DEBUG1("BSA_AG_MIC_EVT %d", p_data->val.num);
        break;

    case BSA_AG_AT_CKPD_EVT:
        APP_DEBUG0("BSA_AG_MIC_EVT");
        break;

    case BSA_AG_AT_A_EVT: /* ATA*/
        APP_DEBUG0("BSA_AG_AT_A_EVT");
        /* pick up the phone */
        app_ag_pickupcall(handle);
        break;

    case BSA_AG_AT_D_EVT: /* ATD */
        APP_DEBUG1("BSA_AG_AT_D_EVT: %s", p_data->val.str);
        break;

    case BSA_AG_AT_CHLD_EVT: /* AT+CHDL*/
        APP_DEBUG0("BSA_AG_AT_CHLD_EVT");
        break;

    case BSA_AG_AT_CHUP_EVT: /* AT+CHUP*/
        APP_DEBUG0("BSA_AG_AT_CHUP_EVT");
        /* hang up the phone */
        app_ag_hangupcall(handle);
        break;

    case BSA_AG_AT_CIND_EVT: /* AT+CIND?*/
        APP_DEBUG0("BSA_AG_AT_CIND_EVT");
        /* send back the appropriate AT commands */
        bsa_ag_res.hndl = handle;
        bsa_ag_res.result = BTA_AG_CIND_RES;
        strncpy(bsa_ag_res.data.str,app_ag_cb.call_active?"1":"0",
            sizeof(bsa_ag_res.data.str)-1);
        strncat(bsa_ag_res.data.str, ",0,0,6,0,5,0,7",
            (sizeof(bsa_ag_res.data.str)-1) - strlen(bsa_ag_res.data.str));
        bsa_ag_res.data.str[sizeof(bsa_ag_res.data.str)-1] = 0;
        BSA_AgResult(&bsa_ag_res);
        break;
    case BSA_AG_AT_VTS_EVT :
        /* Transmit DTMF tone */
        APP_DEBUG1("BSA_AG_AT_VTS_EVT: %s", p_data->val.str);
        break;
    case BSA_AG_AT_BINP_EVT :
        /* Retrieve number from voice tag */
        APP_DEBUG0("BSA_AG_AT_BINP_EVT");
        break;
    case BSA_AG_AT_BLDN_EVT :
        /* Place call to last dialed number */
        APP_DEBUG0("BSA_AG_AT_BLDN_EVT");
        break;
    case BSA_AG_AT_BVRA_EVT :
        /* Enable/disable voice recognition */
        APP_DEBUG1("BSA_AG_AT_BVRA_EVT %d", p_data->val.num);
        break;
    case BSA_AG_AT_NREC_EVT :
        /* Disable echo canceling */
        APP_DEBUG0("BSA_AG_AT_NREC_EVT");
        break;
    case BSA_AG_AT_CNUM_EVT :
        /* Retrieve subscriber number */
        APP_DEBUG0("BSA_AG_AT_CNUM_EVT");
        break;
    case BSA_AG_AT_BTRH_EVT :
        /* CCAP-style incoming call hold */
        APP_DEBUG0("BSA_AG_AT_BTRH_EVT");
        break;
    case BSA_AG_AT_CLCC_EVT: /* AT+CLCC*/
        APP_DEBUG0("BSA_AG_AT_CLCC_EVT");
        /* send back the appropriate AT commands */
        bsa_ag_res.hndl = handle;
        bsa_ag_res.result = BTA_AG_CLCC_RES;
        if (app_ag_cb.call_active)
        {
            /* dummy string: call id 1, mobile terminated, active, voice, noMP */
            strncpy(bsa_ag_res.data.str, "1,1,0,0, 0",sizeof(bsa_ag_res.data.str)-1);
            bsa_ag_res.data.str[sizeof(bsa_ag_res.data.str)-1] = 0;
        }
        bsa_ag_res.data.ok_flag = BTA_AG_OK_DONE;
        BSA_AgResult(&bsa_ag_res);
        break;

    case BSA_AG_AT_COPS_EVT: /* AT+COPS?*/
        APP_DEBUG0("BSA_AG_AT_COPS_EVT");
        /* send back the appropriate AT commands */
        bsa_ag_res.hndl = handle;
        bsa_ag_res.result = BTA_AG_COPS_RES;
        strncpy(bsa_ag_res.data.str, "0, 0, \"BSA Net\"",sizeof(bsa_ag_res.data.str)-1);
        bsa_ag_res.data.str[sizeof(bsa_ag_res.data.str)-1] = 0;
        bsa_ag_res.data.ok_flag = BTA_AG_OK_DONE;
        BSA_AgResult(&bsa_ag_res);
        break;

    case BSA_AG_AT_UNAT_EVT :
        /* Unknown AT command */
        APP_DEBUG0("BSA_AG_AT_UNAT_EVT");
        break;
    case BSA_AG_AT_CBC_EVT :
        /* Battery Level report from HF */
        APP_DEBUG0("BSA_AG_AT_CBC_EVT");
        break;
    case BSA_AG_AT_BAC_EVT :
        /* Codec select */
        APP_DEBUG0("BSA_AG_AT_BAC_EVT");
        break;
    case BSA_AG_AT_BCS_EVT :
        /* Codec select */
        APP_DEBUG0("BSA_AG_AT_BCS_EVT");
        break;

    default:
        APP_ERROR1("app_ag_cback unknown event:%d", event);
        break;
    }

    if(s_pCallback)
        s_pCallback(event, p_data);
}

#ifdef PCM_ALSA
#ifndef PCM_ALSA_DISABLE_AG
/*******************************************************************************
 **
 ** Function         app_ag_open_alsa_duplex
 **
 ** Description      function to open ALSA driver
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_ag_open_alsa_duplex(void)
{
    int status;

    /* If ALSA PCM driver was already open => close it */
    if (alsa_handle_playback != NULL)
    {
        snd_pcm_close(alsa_handle_playback);
        alsa_handle_playback = NULL;
    }

    APP_DEBUG0("Opening Alsa/Asound audio driver Playback");
    /* Open ALSA driver */
    status = snd_pcm_open(&alsa_handle_playback, alsa_device,
            SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if (status < 0)
    {
        APP_ERROR1("snd_pcm_open failed: %s", snd_strerror(status));
        return status;
    }
    else
    {
        /* Configure ALSA driver with PCM parameters */
        status = snd_pcm_set_params(alsa_handle_playback,
                SND_PCM_FORMAT_S16_LE,
                SND_PCM_ACCESS_RW_INTERLEAVED,
                APP_AG_CHANNEL_NB,
                APP_AG_SAMPLE_RATE,
                1, /* SW resample */
                500000);/* 500msec */
        if (status < 0)
        {
            APP_ERROR1("snd_pcm_set_params failed: %s", snd_strerror(status));
            return status;
        }
    }
    alsa_playback_opened = TRUE;

    /* If ALSA PCM driver was already open => close it */
    if (alsa_handle_capture != NULL)
    {
        snd_pcm_close(alsa_handle_capture);
        alsa_handle_capture = NULL;
    }
    APP_DEBUG0("Opening Alsa/Asound audio driver Capture");

    status = snd_pcm_open(&alsa_handle_capture, alsa_device,
            SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (status < 0)
    {
        APP_ERROR1("snd_pcm_open failed: %s", snd_strerror(status));
        return status;
    }
    else
    {
        /* Configure ALSA driver with PCM parameters */
        status = snd_pcm_set_params(alsa_handle_capture,
                SND_PCM_FORMAT_S16_LE,
                SND_PCM_ACCESS_RW_INTERLEAVED,
                APP_AG_CHANNEL_NB,
                APP_AG_SAMPLE_RATE,
                1, /* SW resample */
                500000);/* 500msec */
        if (status < 0)
        {
            APP_ERROR1("snd_pcm_set_params failed: %s", snd_strerror(status));
            return status;
        }
    }
    alsa_capture_opened = TRUE;

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_ag_close_alsa_duplex
 **
 ** Description      function to close ALSA driver
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_ag_close_alsa_duplex(void)
{
    if (alsa_handle_playback != NULL)
    {
        snd_pcm_close(alsa_handle_playback);
        alsa_handle_playback = NULL;
        alsa_playback_opened = FALSE;
    }
    if (alsa_handle_capture != NULL)
    {
        snd_pcm_close(alsa_handle_capture);
        alsa_handle_capture = NULL;
        alsa_capture_opened = FALSE;
    }
    return 0;
}
#endif
#endif /* PCM_ALSA */


/*******************************************************************************
 **
 ** Function         app_ag_start
 **
 ** Description      Example of function to start the AG application
 **
 ** Parameters       SCO route path(PCM or HCI)
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ag_start(UINT8 sco_route)
{
    tBSA_STATUS status;

    status = app_ag_enable();
    if (status != BSA_SUCCESS)
    {
        return status;
    }

    status = app_ag_register(sco_route);

    return status;
}

/*******************************************************************************
 **
 ** Function         app_ag_stop
 **
 ** Description      Example of function to start the AG application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ag_stop(void)
{
    tBSA_STATUS status = BSA_SUCCESS;
    tBSA_AG_DISABLE disable_param;
    tBSA_AG_DEREGISTER deregister_param;

    APP_DEBUG0("Deregister");

    /* prepare parameters */
    BSA_AgDeregisterInit(&deregister_param);
    deregister_param.hndl = app_ag_cb.hndl[0];
    status = BSA_AgDeregister(&deregister_param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgDeregister failed: %d(%s)", status, app_ag_status_2_str(status));
        return status;
    }

    /* prepare parameters */
    BSA_AgDisableInit(&disable_param);
    status = BSA_AgDisable(&disable_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AgDisable failed: %d(%s)", status, app_ag_status_2_str(status));
        return status;
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ag_init
 **
 ** Description      Init Headset application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_init(void)
{
    int index;

    memset(&app_ag_cb, 0, sizeof(app_ag_cb));

    /* Ensure the file descriptor are properly initialized*/
    app_ag_cb.fd_sco_in_file = -1;
    app_ag_cb.fd_sco_out_file = -1;

    for (index=0; index < APP_AG_MAX_NUM_CONN; index++)
    {
        app_ag_cb.uipc_channel[index] = UIPC_CH_ID_BAD;
        app_ag_cb.hndl[index] = BSA_AG_BAD_HANDLE;
    }
}

/*******************************************************************************
 **
 ** Function         app_ag_end
 **
 ** Description      This function is used to close AG
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_end(void)
{
    int index;

    /* Close UIPC channel */
    for (index = 0; index < APP_AG_MAX_NUM_CONN; index++)
    {
        if (app_ag_cb.uipc_channel[index] != UIPC_CH_ID_BAD)
        {
            APP_DEBUG0("Closing UIPC Channel");
            UIPC_Close(app_ag_cb.uipc_channel[index]);
            app_ag_cb.uipc_channel[index]= UIPC_CH_ID_BAD;
        }

    }

    app_ag_stop();
}

/*******************************************************************************
 **
 ** Function         app_ag_set_cback
 **
 ** Description      Set application callback for third party app
 **
 **
 *******************************************************************************/
void app_ag_set_cback(tBSA_AG_CBACK pcb)
{
    /* register callback */
    s_pCallback = pcb;
}
