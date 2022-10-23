/*****************************************************************************
 **
 **  Name:           app_av_bcst.c
 **
 **  Description:    Bluetooth Broadcast Audio Streaming application
 **
 **  Copyright (c) Amlogc
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
/* for memcpy etc... */
#include <string.h>
/* for open and open parameters */
#include <fcntl.h>
/* for lseek, read and close */
#include <unistd.h>
/* for EINTR */
#include <errno.h>
#include <pthread.h>

#include "bsa_api.h"

#include "gki_int.h"
#include "uipc.h"

#include "app_dm.h"
#include "app_av.h"
#include "app_av_bcst.h"
#include "app_xml_param.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_wav.h"
#include "app_playlist.h"
#include "app_mutex.h"
#include "app_thread.h"
#include "app_xml_utils.h"
#include "app_tvselect.h" /*for LT Address*/

#ifdef APP_3D_INCLUDED
#include "app_3d.h"
#endif

#ifdef PCM_ALSA
#include "app_alsa.h"
#endif

/*
 * Defines
 */

/* Number of simultaneous Boradcast A2DP connections supported */
#define APP_AV_BCST_CONNECTIONS_MAX 2

#ifndef APP_AV_BCST_STAT_PERIOD
#define APP_AV_BCST_STAT_PERIOD     5
#endif

/* LT Addr for AV Broadcasting */
#ifndef APP_AV_BCST_LT_ADDR
#define APP_AV_BCST_LT_ADDR 2
#endif

/*
 * Types
 */
typedef enum
{
    APP_AV_BCST_PLAYTYPE_TONE,   /* Play tone */
    APP_AV_BCST_PLAYTYPE_FILE,   /* Play file */
    APP_AV_BCST_PLAYTYPE_MIC     /* Play Microphone */
} tAPP_AV_BCST_PLAYTYPE;

/* Size of the audio buffer use to store the PCM to send to BSA */
#define APP_AV_BCST_AUDIO_BUF_MAX 4000 /* Number of UNIT16 */


typedef struct
{
    BOOLEAN in_use; /* Indicate if this Broadcast Stream is in_use */
    tBSA_AV_HNDL handle; /* Handle of the Broadcast Stream */
    volatile BOOLEAN is_started; /* Indicates if this Broadcast Stream is started */
    tUIPC_CH_ID uipc_channel; /* Contains the UIPC channel of this stream */
    UINT8 lt_addr; /* LtAddr of this AV Broadcast Stream */

    tAPP_THREAD tx_thread;

    tAPP_AV_BCST_PLAYTYPE play_type; /* Tone, file or microphone */

    /* Information about currently played Tone */
    UINT8 sinus_index; /* Information about the current tone generation */
    UINT8 sinus_type; /* Information about the current tone generation */
    UINT16 tone_sample_freq; /* Tone generation sampling frequency */

    /* Information about currently played File */
    int file_index;
    char file_name[1000];

    tBSA_AV_MEDIA_FEEDINGS media_feeding; /* Feeding configuration */
    tAPP_AV_UIPC uipc_config; /* UIPC configuration */
    tAPP_AV_DELAY uipc_delay; /* UIPC Delay (for non blocking mode */

    /* PCM audio buffer */
    short audio_buf[APP_AV_BCST_AUDIO_BUF_MAX];
} tAPP_AV_BCST_STREAM;

typedef struct
{
#ifdef APP_3D_INCLUDED
    UINT8 audio_bit_mask; /* Audio bit mask sent (part of 3D beacon) */
#endif
    /* Array of Stream instances */
    tAPP_AV_BCST_STREAM stream[APP_AV_BCST_CONNECTIONS_MAX];
} tAPP_AV_BCST_CB;

/*
 * Global variables
 */

/* local application control block (current status) */

tAPP_AV_BCST_CB app_av_bcst_cb;

/*
 * Local functions
 */
static tAPP_AV_BCST_STREAM *app_av_bcst_stream_alloc(void);
static void app_av_bcst_stream_free(tAPP_AV_BCST_STREAM *p_stream);
tAPP_AV_BCST_STREAM *app_av_bcst_stream_get_by_handle(tBSA_AV_HNDL handle);
tAPP_AV_BCST_STREAM *app_av_bcst_stream_get_by_uipc(tUIPC_CH_ID uipc_channel);

static void app_av_bcst_thread(void);
static void app_av_bcst_thread_exit(tAPP_AV_BCST_STREAM *p_stream);
static void app_av_bcst_print_statistic (tBSA_AV_HNDL handle, UINT32 *p_byte_sends,
        struct timespec *p_timestamp_begin, struct timespec *p_timestamp_end);

static int app_av_bcst_uipc_reconfig(tAPP_AV_BCST_STREAM *p_stream);
static void app_av_bcst_delay_start(tAPP_AV_BCST_STREAM *p_stream);

#ifdef APP_3D_INCLUDED
static BOOLEAN app_av_bcst_is_audio_registered(void);
static BOOLEAN app_av_bcst_is_audio_started(void);
#endif /* APP_3D_INCLUDED */

/*******************************************************************************
 **
 ** Function         app_av_bcst_init
 **
 ** Description      Initialize Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_init(void)
{
    int stream;

    memset(&app_av_bcst_cb, 0, sizeof(app_av_bcst_cb));

    /* The first think to do it to find which Stream this thread is related to */
    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++)
    {
        /* Default UIPC configuration blocking */
        app_av_bcst_cb.stream[stream].uipc_config.is_blocking = TRUE;
        app_av_bcst_cb.stream[stream].uipc_config.length = sizeof(UINT16)  * APP_AV_BCST_AUDIO_BUF_MAX;

    }

   return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_end
 **
 ** Description      Stop Broadcast AV Stream application
 **
 ** Returns          Void
 **
 *******************************************************************************/
void app_av_bcst_end(void)
{
    int stream;
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS bsa_status;
    tBSA_AV_STOP stop_param;
    tBSA_AV_DEREGISTER deregister_param;

    p_stream = &app_av_bcst_cb.stream[0];
    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++, p_stream++)
    {
        if (!p_stream->in_use)
        {
            continue;                       /* Unused Stream. Check next */
        }
        if (p_stream->is_started)
        {
            /* Stop this Stream */
            APP_INFO1("Stopping Broadcast AV Handle:%d", p_stream->handle);
            BSA_AvStopInit(&stop_param);
            stop_param.pause = FALSE;
            stop_param.uipc_channel = p_stream->uipc_channel;
            bsa_status = BSA_AvStop(&stop_param);
            if (bsa_status != BSA_SUCCESS)
            {
                APP_ERROR1("BSA_AvStop failed: %d", bsa_status);
                continue;
            }
            /* Wait until stop has completed */
            while (p_stream->is_started)
            {
                GKI_delay(10);
            }
            /* Deregister this Stream */
            APP_INFO1("Deregistering Broadcast AV Handle:%d", p_stream->handle);
            BSA_AvDeregisterInit(&deregister_param);
            deregister_param.handle = p_stream->handle;
            bsa_status = BSA_AvDeregister(&deregister_param);
            if (bsa_status != BSA_SUCCESS)
            {
                APP_ERROR1("BSA_AvDeregister failed: %d", bsa_status);
                continue;
            }

            /* Free the App's Stream */
            app_av_bcst_stream_free(p_stream);
        }
    }
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_register
 **
 ** Description      Register a new Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_register(void)
{
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS status;
    tBSA_AV_REGISTER register_param;

    /* Allocate an AV Broadcast Stream */
    p_stream = app_av_bcst_stream_alloc();
    if (p_stream == NULL)
    {
        APP_ERROR0("All available AV Broadcast Stream already registered");
        return -1;
    }

#ifndef APP_AV_BCST_DYNAMIC_LTADDR
    /* Register an audio AV source point */
    BSA_AvRegisterInit(&register_param);
    register_param.channel = BSA_AV_CHNL_AUDIO_BCST;
    register_param.lt_addr = p_stream->lt_addr;
    APP_INFO1("Registering lt addr %d", p_stream->lt_addr);
    status = BSA_AvRegister(&register_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvRegister failed: %d", status);
        app_av_bcst_stream_free(p_stream);
        return -1;
    }
#else
    /* Register an audio AV source point */
    BSA_AvRegisterInit(&register_param);
    register_param.channel = BSA_AV_CHNL_AUDIO_BCST;
    for (register_param.lt_addr = 1; register_param.lt_addr <= APP_AV_BCST_LT_ADDR_MAX; register_param.lt_addr++)
    {
        APP_INFO1("Registering lt addr %d", register_param.lt_addr);
        status = BSA_AvRegister(&register_param);
        if (status == BSA_SUCCESS)
        {
            APP_DEBUG1("BSA_AvRegister Success. lt_addr: %d", register_param.lt_addr);
            /*update lt_addr from dynamic allocation*/
            p_stream->lt_addr = register_param.lt_addr;
            break;
        }
        else
        {
            APP_DEBUG1("BSA_AvRegister Failed. lt_addr: %d", register_param.lt_addr);
        }
    }

    if ((status != BSA_SUCCESS) && (register_param.lt_addr == APP_AV_BCST_LT_ADDR_MAX))
    {
        APP_ERROR0("BSA_AvRegister Failed! No LT Addr available.");
        app_av_bcst_stream_free(p_stream);
        return -1;
    }
#endif

    APP_INFO1("Broadcast AV Handle:%d UIPC:%d Registered",
            register_param.handle, register_param.uipc_channel);

    /* Save the Stream handle and the UIPC Channel */
    p_stream->handle = register_param.handle;
    p_stream->uipc_channel = register_param.uipc_channel;

    /* For test purpose, each channel will stream a different sinus wave */
    if (p_stream->uipc_channel == UIPC_CH_ID_BAV_1)
        p_stream->sinus_type = 0;
    else
        p_stream->sinus_type = 1;

#ifdef APP_3D_INCLUDED
    /* If audio Bit Used bit is not set (i.e. this is the first registration) */
    if ((app_av_bcst_cb.audio_bit_mask & APP_3D_AUDIO_USED) == 0)
    {
        /* Set it and send it to the BT controller */
        app_av_bcst_cb.audio_bit_mask |= APP_3D_AUDIO_USED;
        app_3d_set_audio_bits(app_av_bcst_cb.audio_bit_mask);
    }
#endif /* APP_3D_INCLUDED */

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_deregister
 **
 ** Description      Register a new Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_deregister(void)
{
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS status;
    tBSA_AV_DEREGISTER deregister_param;
    int choice;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to deregister");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    /* Deregister */
    BSA_AvDeregisterInit(&deregister_param);
    deregister_param.handle = p_stream->handle;
    status = BSA_AvDeregister(&deregister_param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvDeregister failed: %d", status);
        return -1;
    }

    /* To prevent Thread to write in UIPC */
    p_stream->is_started = FALSE;

    /* Flush UIPC channel to unblock the Tx thread which may be blocked */
    UIPC_Ioctl(p_stream->uipc_channel, UIPC_REQ_TX_FLUSH, NULL);

    app_av_bcst_stream_free(p_stream);

#ifdef APP_3D_INCLUDED
    /* If no more Audio Stream Registered */
    if (app_av_bcst_is_audio_registered() == FALSE)
    {
        /* Reset Audio Bit On bit and send it to the BT controller */
        app_av_bcst_cb.audio_bit_mask &= ~APP_3D_AUDIO_USED;
        app_3d_set_audio_bits(app_av_bcst_cb.audio_bit_mask);
    }
#endif /* APP_3D_INCLUDED */

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_tone
 **
 ** Description      Start to play a Tone on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_tone(void)
{
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS status;
    tBSA_AV_START start_param;
    int choice;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to Start playing a Tone");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    if (p_stream->is_started)
    {
        APP_ERROR1("Bad Stream:%d already started", (tBSA_AV_HNDL)choice);
        return -1;
    }

    /* reset the current tone */
    p_stream->sinus_index = 0;
    p_stream->tone_sample_freq = 48000;

    /* Start */
    BSA_AvStartInit(&start_param);
    start_param.uipc_channel = p_stream->uipc_channel;
    start_param.media_feeding.format = BSA_AV_CODEC_PCM;
    start_param.media_feeding.cfg.pcm.sampling_freq = p_stream->tone_sample_freq;
    start_param.media_feeding.cfg.pcm.num_channel = 2;
    start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    start_param.feeding_mode = BSA_AV_FEEDING_ASYNCHRONOUS;
    start_param.latency = 0; /* convert us to ms, synchronous feeding mode only*/

    p_stream->play_type = APP_AV_BCST_PLAYTYPE_TONE;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_file
 **
 ** Description      Start to play a File on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_file(void)
{
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS status;
    tBSA_AV_START start_param;
    int choice;
    int max_file_index;
    tAPP_WAV_FILE_FORMAT wav_format;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to Start playing a File");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    if (p_stream->is_started)
    {
        APP_ERROR1("Bad Stream:%d already started", (tBSA_AV_HNDL)choice);
        return -1;
    }

    /* Print the files available */
    max_file_index = app_av_display_playlist();

    choice = app_get_choice("Select file");
    if ((choice < 0 ) || (choice >= max_file_index))
    {
        APP_ERROR1("File index out of bounds:%d", choice);
        return -1;
    }
    if (app_wav_format(app_av_get_soundfile_path(choice), &wav_format) < 0)
    {
        APP_ERROR1("Unable to extract WAV format from:%s", app_av_get_soundfile_path(choice));
        return -1;
    }
    else
    {
        APP_INFO1("%d :%s", (int)choice, app_av_get_soundfile_path(choice));
        APP_INFO1("    codec(%s) ch(%d) bits(%d) rate(%d)", (wav_format.codec==BSA_AV_CODEC_PCM)?"PCM":"apt-X",
                wav_format.nb_channels, wav_format.bits_per_sample, (int)wav_format.sample_rate);
    }

    if (wav_format.codec != BSA_AV_CODEC_PCM)
    {
        APP_ERROR0("Only PCM files can be streamed");
        return -1;
    }

    p_stream->play_type = APP_AV_BCST_PLAYTYPE_FILE;
    p_stream->file_index = choice;
    strncpy(p_stream->file_name, app_av_get_soundfile_path(choice), sizeof(p_stream->file_name)-1);
    p_stream->file_name[sizeof(p_stream->file_name)-1] = '\0';

    /* start AV stream */
    BSA_AvStartInit(&start_param);

    start_param.media_feeding.format = wav_format.codec;
    start_param.media_feeding.cfg.pcm.sampling_freq = wav_format.sample_rate;
    start_param.media_feeding.cfg.pcm.num_channel = wav_format.nb_channels;
    start_param.media_feeding.cfg.pcm.bit_per_sample = wav_format.bits_per_sample;

    start_param.uipc_channel = p_stream->uipc_channel;

    start_param.feeding_mode = BSA_AV_FEEDING_ASYNCHRONOUS;
    start_param.latency = 0;

    status = BSA_AvStart(&start_param);
    if (status == BSA_ERROR_SRV_AV_FEEDING_NOT_SUPPORTED)
    {
        APP_ERROR0("BSA_AvStart failed because file encoding is not supported by remote devices");
        return status;
    }
    else if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed: %d", status);
        return status;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_mic
 **
 ** Description      Start to play a Microphone on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_mic(void)
{
#ifdef PCM_ALSA
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS status;
    tBSA_AV_START start_param;
    int choice;
    tAPP_ALSA_CAPTURE_OPEN alsa_capture;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to Start playing from Microphone");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    if (p_stream->is_started)
    {
        APP_ERROR1("Bad Stream:%d already started", (tBSA_AV_HNDL)choice);
        return -1;
    }

    /* open and configure ALSA capture device */
    app_alsa_capture_open_init(&alsa_capture);

    alsa_capture.access = APP_ALSA_PCM_ACCESS_RW_INTERLEAVED;
    alsa_capture.blocking = TRUE; /* Blocking */
    alsa_capture.format = APP_ALSA_PCM_FORMAT_S16_LE; /* Signed 16 bits Little Endian*/
    alsa_capture.sample_rate = 48000; /* 48KHz */
    alsa_capture.stereo = TRUE; /* Stereo */
    if (app_alsa_capture_open(&alsa_capture) < 0)
    {
        APP_ERROR0("app_alsa_capture_open fail:%d");
        return -1;
    }
    APP_DEBUG0("ALSA driver opened in capture mode (Microphone/LineIn)");

    /* Start */
    BSA_AvStartInit(&start_param);
    start_param.uipc_channel = p_stream->uipc_channel;
    start_param.media_feeding.format = BSA_AV_CODEC_PCM;
    start_param.media_feeding.cfg.pcm.sampling_freq = 48000;
    start_param.media_feeding.cfg.pcm.num_channel = 2;
    start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    start_param.feeding_mode = BSA_AV_FEEDING_ASYNCHRONOUS;
    start_param.latency = 0; /* convert us to ms, synchronous feeding mode only*/

    p_stream->play_type = APP_AV_BCST_PLAYTYPE_MIC;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed:%d", status);
        return -1;
    }

    return 0;
#else
    APP_ERROR0("Application not compiled with ALSA");
    return -1;
#endif
}

#ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED
/*******************************************************************************
 **
 ** Function         app_av_bcst_play_loopback_input
 **
 ** Description      Start to play a loopback input on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_loopback_input(void)
{
#ifdef PCM_ALSA
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS status;
    tBSA_AV_START start_param;
    int choice;
    tAPP_ALSA_CAPTURE_OPEN alsa_capture;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to Start playing");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    if (p_stream->is_started)
    {
        APP_ERROR1("Bad Stream:%d already started", (tBSA_AV_HNDL)choice);
        return -1;
    }

#if 0 //48000Hz only
    choice = app_get_choice("Select rate to stream at (default 48000)");

    if (choice == 0)
        choice = 48000;

    if ((choice != 44100) && (choice != 48000))
    {
        APP_ERROR1("Unsupported rate (%d)", (tBSA_AV_HNDL)choice);
        return -1;
    }
#endif

    /* open and configure ALSA capture device */
    app_alsa_capture_open_init(&alsa_capture);

    alsa_capture.access = APP_ALSA_PCM_ACCESS_RW_INTERLEAVED;
    alsa_capture.blocking = TRUE; /* Blocking */
    alsa_capture.format = APP_ALSA_PCM_FORMAT_S16_LE; /* Signed 16 bits Little Endian*/
    alsa_capture.sample_rate = 48000; //choice
    alsa_capture.stereo = TRUE; /* Stereo */
    alsa_capture.latency = 0;

    if (app_alsa_capture_loopback_open(&alsa_capture) < 0)
    {
        APP_ERROR0("app_alsa_capture_open fail:%d");
        return -1;
    }
    APP_DEBUG0("ALSA driver opened in capture mode (Microphone/LineIn)");

    /* Start */
    BSA_AvStartInit(&start_param);
    start_param.uipc_channel = p_stream->uipc_channel;
    start_param.media_feeding.format = BSA_AV_CODEC_PCM;
    start_param.media_feeding.cfg.pcm.sampling_freq =  48000; //choice
    start_param.media_feeding.cfg.pcm.num_channel = 2;
    start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    start_param.feeding_mode = BSA_AV_FEEDING_ASYNCHRONOUS;
    start_param.latency = 0; /* convert us to ms, synchronous feeding mode only*/

    p_stream->play_type = APP_AV_BCST_PLAYTYPE_MIC;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed:%d", status);
        return -1;
    }

    return 0;
#else
    APP_ERROR0("Application not compiled with ALSA");
    return -1;
#endif
}
#endif /* #ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED */

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_mm
 **
 ** Description      Start to play MM (Kernel Audio) on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_mm(void)
{
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS status;
    tBSA_AV_START start_param;
    int choice;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to Start playing from Kernel Audio");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    if (p_stream->is_started)
    {
        APP_ERROR1("Bad Stream:%d already started", (tBSA_AV_HNDL)choice);
        return -1;
    }

    /* Start */
    BSA_AvStartInit(&start_param);
    start_param.uipc_channel = p_stream->uipc_channel;
    start_param.media_feeding.format = BSA_AV_CODEC_PCM;
    start_param.media_feeding.cfg.pcm.sampling_freq = 48000;
    start_param.media_feeding.cfg.pcm.num_channel = 2;
    start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    start_param.feeding_mode = BSA_AV_FEEDING_ASYNCHRONOUS;
    start_param.latency = 0; /* convert us to ms, synchronous feeding mode only*/

    p_stream->play_type = APP_AV_BCST_PLAYTYPE_MIC;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_stop_current
 **
 ** Description      Stop playing current stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_stop(void)
{

    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS bsa_status;
    tBSA_AV_STOP stop_param;
    int choice;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to Stop");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    if (p_stream->is_started == FALSE)
    {
        APP_ERROR1("Bad Stream:%d already Stopped", (tBSA_AV_HNDL)choice);
        return -1;
    }
    /* Stop the AV stream */
    bsa_status = BSA_AvStopInit(&stop_param);
    stop_param.pause = FALSE;
    stop_param.uipc_channel = p_stream->uipc_channel;
    bsa_status = BSA_AvStop(&stop_param);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStop failed: %d", bsa_status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_start_event_hdlr
 **
 ** Description      Handle AV Broadcast Start event
 **
 ** Returns          Void
 **
 *******************************************************************************/
void app_av_bcst_start_event_hdlr(tBSA_AV_START_MSG *p_msg)
{
    tAPP_AV_BCST_STREAM *p_stream;
    tBSA_STATUS bsa_status;
    tBSA_AV_STOP stop_param;

    p_stream = app_av_bcst_stream_get_by_uipc(p_msg->uipc_channel);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad UIPC Channel:%d", p_msg->uipc_channel);
        return;
    }

    if (p_msg->status == BSA_SUCCESS)
    {
        p_stream->is_started = TRUE;

        switch (p_msg->media_feeding.format)
        {
            case BSA_AV_CODEC_PCM:
                APP_INFO1("    Start PCM feeding: freq=%d, channels=%d, bits=%d",
                        p_msg->media_feeding.cfg.pcm.sampling_freq,
                        p_msg->media_feeding.cfg.pcm.num_channel,
                        p_msg->media_feeding.cfg.pcm.bit_per_sample);

                /* Save the feeding information */
                p_stream->media_feeding = p_msg->media_feeding;

                if (app_dm_get_dual_stack_mode() == BSA_DM_DUAL_STACK_MODE_BSA)
                {
                    /* Start the Feeding thread associated with this stream */
                    if (app_create_thread(app_av_bcst_thread, 0, 0, &p_stream->tx_thread) < 0)
                    {
                        APP_ERROR0("app_create_thread failed");
                        return;
                    }
                }
                else if (app_dm_get_dual_stack_mode() == BSA_DM_DUAL_STACK_MODE_MM)
                {
                    APP_INFO0("Stack is not in BSA mode. Do not Start AV thread");
                }

#ifdef APP_3D_INCLUDED
                /* If Audio Bit On bit is not set (i.e. first stream started) */
                if ((app_av_bcst_cb.audio_bit_mask & APP_3D_AUDIO_ON) == 0)
                {
                    /* Set it and send it to the BT controller */
                    app_av_bcst_cb.audio_bit_mask |= APP_3D_AUDIO_ON;
                    app_3d_set_audio_bits(app_av_bcst_cb.audio_bit_mask);
                }
#endif /* APP_3D_INCLUDED */
                break;

            default:
                APP_ERROR1("Unsupported feeding format code: %d",
                        p_stream->media_feeding.format);

                /* Stop the AV stream */
                bsa_status = BSA_AvStopInit(&stop_param);
                stop_param.pause = FALSE;
                stop_param.uipc_channel = p_stream->uipc_channel;
                bsa_status = BSA_AvStop(&stop_param);
                if (bsa_status != BSA_SUCCESS)
                {
                    APP_ERROR1("BSA_AvStop failed:%d", bsa_status);
                    return;
                }
                break;
        }
    }
}


/*******************************************************************************
 **
 ** Function         app_av_bcst_uipc_reconfig
 **
 ** Description      This function is used to reconfigure a Broadcast UIPC channel
 **
 ** Returns          status
 **
 *******************************************************************************/
static int app_av_bcst_uipc_reconfig(tAPP_AV_BCST_STREAM *p_stream)
{
    UINT32 uipc_mode;

    app_av_compute_uipc_param(&p_stream->uipc_config, &p_stream->media_feeding);

    if (p_stream->uipc_config.is_blocking)
    {
        uipc_mode = UIPC_WRITE_BLOCK;
    }
    else
    {
        uipc_mode = UIPC_WRITE_NONBLOCK;
    }
    /* Setting the mode will also flush the buffer */
    if (!UIPC_Ioctl(p_stream->uipc_channel, uipc_mode, NULL))
    {
        APP_ERROR0("Failed setting the UIPC (non) blocking");
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_stop_event_hdlr
 **
 ** Description      Handle AV Broadcast Stop event
 **
 ** Returns          Void
 **
 *******************************************************************************/
void app_av_bcst_stop_event_hdlr(tBSA_AV_STOP_MSG *p_msg)
{
    tAPP_AV_BCST_STREAM *p_stream;

    p_stream = app_av_bcst_stream_get_by_uipc(p_msg->uipc_channel);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad UIPC Channel:%d", p_msg->uipc_channel);
        return;
    }

    p_stream->is_started = FALSE;

    /* Flush UIPC channel */
    UIPC_Ioctl(p_stream->uipc_channel, UIPC_REQ_TX_FLUSH, NULL);

#ifdef APP_3D_INCLUDED
    /* If no Audio Stream Started (i.e. last Stream stopped) */
    if (app_av_bcst_is_audio_started() == FALSE)
    {
        /* Reset Audio Bit On bit and send it to the BT controller */
        app_av_bcst_cb.audio_bit_mask &= ~APP_3D_AUDIO_ON;
        app_3d_set_audio_bits(app_av_bcst_cb.audio_bit_mask);
    }
#endif /* APP_3D_INCLUDED */
}

/*******************************************************************************
 **
 ** Function           app_av_bcst_thread
 **
 ** Description        Thread in charge of feeding UIPC channel
 **
 ** Returns            nothing
 **
 *******************************************************************************/
static void app_av_bcst_thread(void)
{
    int stream_idx;
    tAPP_AV_BCST_STREAM *p_stream = &app_av_bcst_cb.stream[0];
    int nb_bytes = 0;
    tBSA_AV_HNDL handle;
    struct timespec timestamp_begin;
    struct timespec timestamp_end;
    UINT32 byte_sends;
    int file_desc = -1;
    tAPP_WAV_FILE_FORMAT wav_format;

    /* The first think to do it to find which Stream this thread is related to */
    for (stream_idx = 0 ; stream_idx < APP_AV_BCST_CONNECTIONS_MAX ; stream_idx++, p_stream++)
    {
        /* Compare the ThreadId of each stream with the current ThreadId */
        if ((p_stream->in_use) && (pthread_equal(p_stream->tx_thread, pthread_self())))
        {
            /* Thread found */
            break;
        }
    }
    if (stream_idx >= APP_AV_BCST_CONNECTIONS_MAX)
    {
        APP_ERROR0("app_av_bcst_thread no Thread reference found. Dying.");
        app_av_bcst_thread_exit(p_stream);
        return;
    }

    handle = p_stream->handle; /* Save the Handle */

    APP_INFO1("app_av_bcst_thread Started stream:%d", handle);

    /* Open the UIPC channel associated with this stream */
    if (UIPC_Open(p_stream->uipc_channel, NULL) == FALSE)
    {
        APP_ERROR0("UIPC_Open failed");
        app_av_bcst_thread_exit(p_stream);
        return;
    }

    /* Reconfigure the UIPC to adapt to the feeding mode */
    if (app_av_bcst_uipc_reconfig(p_stream) < 0)
    {
        APP_ERROR0("app_av_bcst_uipc_reconfig failed");
        app_av_bcst_thread_exit(p_stream);
        return;
    }

    if (p_stream->uipc_config.is_blocking)
    {
        APP_DEBUG1("UIPC Blocking mode Length:%d", p_stream->uipc_config.length);
    }
    else
    {
        APP_DEBUG1("UIPC Non Blocking mode Length:%d Period:%d(us)",
                p_stream->uipc_config.length, p_stream->uipc_config.period);
    }

    /* Start the timer management */
    app_av_bcst_delay_start(p_stream);

    switch (p_stream->play_type)
    {
    case APP_AV_BCST_PLAYTYPE_TONE:
        APP_DEBUG1("Stream=%d Playing Tone", p_stream->handle);
        break;

    case APP_AV_BCST_PLAYTYPE_FILE:
        APP_DEBUG1("Stream=%d Playing File %s", p_stream->handle, p_stream->file_name);
        file_desc = app_wav_open_file(p_stream->file_name, &wav_format);
        if (file_desc < 0)
        {
            APP_ERROR1("Open(%s) failed(%d)", p_stream->file_name, errno);
            app_av_bcst_thread_exit(p_stream);
            return;
        }
        break;

    case APP_AV_BCST_PLAYTYPE_MIC:
#ifdef PCM_ALSA
        APP_DEBUG1("Stream=%d Playing Microphone", p_stream->handle);
#else
        APP_ERROR0("APP_ALSA not compiled");
        app_av_bcst_thread_exit(p_stream);
        return;
#endif
        break;

    default:
        APP_ERROR1("play_type:%d not implemented", p_stream->play_type);
        app_av_bcst_thread_exit(p_stream);
        return;
        break;
    }

    /* Get the initial time (for statistic) */
    clock_gettime(CLOCK_MONOTONIC, &timestamp_begin);
    byte_sends = 0;

    do
    {
        switch (p_stream->play_type)
        {
        case APP_AV_BCST_PLAYTYPE_TONE:
            nb_bytes =  p_stream->uipc_config.length;
            app_av_read_tone(p_stream->audio_buf, nb_bytes,
                    p_stream->sinus_type, &p_stream->sinus_index);
            break;

        case APP_AV_BCST_PLAYTYPE_FILE:
            if (file_desc < 0)
                break;
            nb_bytes = app_wav_read_data(file_desc, &wav_format,
                    (char *)p_stream->audio_buf, p_stream->uipc_config.length);
            if (nb_bytes < 0)
            {
                nb_bytes = 0;
                break;
            }
            else if (nb_bytes < p_stream->uipc_config.length)
            {
                if (file_desc >= 0 )
                    close(file_desc);

                file_desc = app_wav_open_file(p_stream->file_name, &wav_format);
                if (file_desc < 0)
                {
                    APP_ERROR1("Open(%s) failed(%d)", p_stream->file_name, errno);
                    continue;
                }
            }
            break;

        case APP_AV_BCST_PLAYTYPE_MIC:
#ifdef PCM_ALSA
            nb_bytes = app_alsa_capture_read((char *)p_stream->audio_buf,
                    p_stream->uipc_config.length);

            if (nb_bytes == 0)
            {
                /*
                 * let's simulate that 4 bytes (16bits stereo) have been
                 * to do not exit the while loop */
                nb_bytes = 4;
            }
            else if (nb_bytes < 0)
            {
                APP_ERROR1("app_alsa_capture_read fail:%d", nb_bytes);
            }
#else
            nb_bytes = 0;
#endif
            break;

        default:
            APP_ERROR1("play_type:%d not implemented", p_stream->play_type);
            break;
        }

        if (nb_bytes <= 0)
        {
            APP_DEBUG0("No more samples");
            /* todo: stop? */
        }
        else
        {
            /* Send the samples to the AV channel */
            if (UIPC_Send(p_stream->uipc_channel, 0, (UINT8 *)p_stream->audio_buf, nb_bytes) == FALSE)
            {
                APP_ERROR0("UIPC_Send failed");
            }

            /* If the UIPC is in non blocking mode */
            if (p_stream->uipc_config.is_blocking == FALSE)
            {
                APP_INFO1("sleep %d", p_stream->uipc_delay);
                /* Sleep for the duration requested (between UIPC_Send) */
                app_av_wait_delay(1, &p_stream->uipc_delay, &p_stream->uipc_config);
            }

            /* Print some statistic information (PCM throughput) */
            clock_gettime(CLOCK_MONOTONIC, &timestamp_end);
            byte_sends += nb_bytes;
            app_av_bcst_print_statistic(p_stream->handle,&byte_sends,
                    &timestamp_begin, &timestamp_end);

        }
    } while(nb_bytes && (p_stream->is_started));

    if (p_stream->play_type == APP_AV_BCST_PLAYTYPE_FILE)
    {
        if (file_desc >= 0 )
            close(file_desc);
    }
#ifdef PCM_ALSA
    else if (p_stream->play_type == APP_AV_BCST_PLAYTYPE_MIC)
    {
        app_alsa_capture_close();
    }
#endif

    app_av_bcst_thread_exit(p_stream);
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_thread_exit
 **
 ** Description      This function is called when the Tx thread exits
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_av_bcst_thread_exit(tAPP_AV_BCST_STREAM *p_stream)
{
    APP_DEBUG1("Thread Stopped Stream:%d", p_stream->handle);

    UIPC_Close(p_stream->uipc_channel);
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_delay_start
 **
 ** Description      This function starts the delay management
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_av_bcst_delay_start(tAPP_AV_BCST_STREAM *p_stream)
{
    /* Get the initial time and period */
    clock_gettime(CLOCK_MONOTONIC, &(p_stream->uipc_delay.timestamp));
    p_stream->uipc_delay.timeout = p_stream->uipc_config.period;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_print_statistic
 **
 ** Description      This function prints some Throughput statistic
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_av_bcst_print_statistic (tBSA_AV_HNDL handle, UINT32 *p_byte_sends,
        struct timespec *p_timestamp_begin, struct timespec *p_timestamp_end)
{
#if (APP_AV_BCST_STAT_PERIOD > 0)
    float delta;
    int diff;

    if ((p_timestamp_end->tv_sec - p_timestamp_begin->tv_sec) >= APP_AV_BCST_STAT_PERIOD)
    {
        diff = p_timestamp_end->tv_nsec - p_timestamp_begin->tv_nsec;
        delta = (float)diff / (1000 * 1000 * 1000);
        //delta /= 1000; /* micro-sec */

        diff = (p_timestamp_end->tv_sec - p_timestamp_begin->tv_sec);
        delta += (float)diff;
        APP_INFO1("Stream:%d PCM throughput=%d B/s", handle, (int)((float)*p_byte_sends/(float)delta));
        *p_timestamp_begin = *p_timestamp_end;
        *p_byte_sends = 0;
    }
#endif
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_stream_alloc
 **
 ** Description      This function allocates a Broadcast AV stream
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
static tAPP_AV_BCST_STREAM *app_av_bcst_stream_alloc(void)
{
    int stream;
    BOOLEAN is_blocking;
    int length;

    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++)
    {
        if (app_av_bcst_cb.stream[stream].in_use == FALSE)
        {
            is_blocking = app_av_bcst_cb.stream[stream].uipc_config.is_blocking;
            length = app_av_bcst_cb.stream[stream].uipc_config.length;
            memset (&app_av_bcst_cb.stream[stream], 0, sizeof(app_av_bcst_cb.stream[stream]));
            app_av_bcst_cb.stream[stream].uipc_config.is_blocking = is_blocking;
            app_av_bcst_cb.stream[stream].uipc_config.length = length;
            app_av_bcst_cb.stream[stream].in_use = TRUE;
            app_av_bcst_cb.stream[stream].lt_addr = stream + APP_AV_BCST_LT_ADDR;
            return &app_av_bcst_cb.stream[stream];
        }
    }
    APP_ERROR0("no more resources");
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_stream_free
 **
 ** Description      This function frees a Broadcast AV stream
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_av_bcst_stream_free(tAPP_AV_BCST_STREAM *p_stream)
{
    if (p_stream == NULL)
    {
        APP_ERROR0("null stream");
        return;
    }
    if (p_stream->in_use == FALSE)
    {
        APP_ERROR0("stream not allocated");
    }

    p_stream->in_use = FALSE;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_stream_get_by_handle
 **
 ** Description      This function find a Broadcast AV stream by Handle
 **
 ** Returns          None
 **
 *******************************************************************************/
tAPP_AV_BCST_STREAM *app_av_bcst_stream_get_by_handle(tBSA_AV_HNDL handle)
{
    int stream;
    tAPP_AV_BCST_STREAM *p_stream = &app_av_bcst_cb.stream[0];

    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++, p_stream++)
    {
        if ((p_stream->in_use) &&
            (p_stream->handle == handle))
        {
            return p_stream;
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_stream_get_by_uipc
 **
 ** Description      This function find a Broadcast AV stream by UIPC Channel
 **
 ** Returns          None
 **
 *******************************************************************************/
tAPP_AV_BCST_STREAM *app_av_bcst_stream_get_by_uipc(tUIPC_CH_ID uipc_channel)
{
    int stream;
    tAPP_AV_BCST_STREAM *p_stream = &app_av_bcst_cb.stream[0];

    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++, p_stream++)
    {
        if ((p_stream->in_use) &&
            (p_stream->uipc_channel == uipc_channel))
        {
            return p_stream;
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_stream_display
 **
 ** Description      This function displays a Broadcast AV stream
 **
 ** Returns          Number of stream in use
 **
 *******************************************************************************/
int app_av_bcst_stream_display(void)
{
    int stream;
    tAPP_AV_BCST_STREAM *p_stream = &app_av_bcst_cb.stream[0];
    int nb_in_use = 0;

    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++, p_stream++)
    {
        if (p_stream->in_use)
        {
            APP_INFO1("Stream:%d LtAddr:%d Started:%d UIPC:%d",
                    p_stream->handle, p_stream->lt_addr,
                    p_stream->is_started, p_stream->uipc_channel);
            nb_in_use++;
        }
    }
    return nb_in_use;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_ask_uipc_config
 **
 ** Description      This function is used to configure a Broadcast UIPC channel
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_av_bcst_ask_uipc_config(void)
{
    int choice;
    tAPP_AV_UIPC uipc_cfg;
    UINT32 uipc_mode = UIPC_WRITE_BLOCK;
    tAPP_AV_BCST_STREAM *p_stream;

    /* Display Streams */
    app_av_bcst_stream_display();

    choice = app_get_choice("Stream to configure");

    p_stream = app_av_bcst_stream_get_by_handle((tBSA_AV_HNDL)choice);
    if (p_stream == NULL)
    {
        APP_ERROR1("Bad Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    if (p_stream->is_started)
    {
        APP_ERROR1("Cannot configure Started Stream:%d", (tBSA_AV_HNDL)choice);
        return -1;
    }

    /* Check if the media feeding configuration has ever been received */
    if ((p_stream->media_feeding.cfg.pcm.bit_per_sample == 0) ||
        (p_stream->media_feeding.cfg.pcm.num_channel == 0) ||
        (p_stream->media_feeding.cfg.pcm.sampling_freq == 0))
    {
        APP_INFO0("Broadcast AV not yet started => let's use default values as feeding example (PCM, 48kHz, 16bits, Stereo)");

        p_stream->media_feeding.format = BSA_AV_CODEC_PCM;
        p_stream->media_feeding.cfg.pcm.sampling_freq = 48000;
        p_stream->media_feeding.cfg.pcm.bit_per_sample = 16;
        p_stream->media_feeding.cfg.pcm.num_channel = 2;
    }

    /* Set the default mode : blocking */
    uipc_cfg.is_blocking = TRUE;
    uipc_cfg.asked_period = 0;
    uipc_cfg.period = 0;
    uipc_cfg.length = APP_AV_BCST_AUDIO_BUF_MAX;

    APP_INFO0("UIPC blocking/non blocking mode:");
    APP_INFO0("    0 Blocking mode (default)");
    APP_INFO0("    1 Non blocking mode");
    choice = app_get_choice("");
    if (choice == 0)
    {
        uipc_cfg.is_blocking = TRUE;
        APP_INFO0("UIPC Blocking mode selected");
    }
    else if (choice == 1)
    {
        APP_INFO0("UIPC Non Blocking mode selected");
        uipc_cfg.is_blocking = FALSE;
        uipc_mode = UIPC_WRITE_NONBLOCK;
        choice = app_get_choice("Enter Period (in micro)");
        if (choice < 0)
        {
            APP_ERROR1("Bad period:%d", choice);
            return -1;
        }
        uipc_cfg.asked_period = choice;
    }
    else
    {
        APP_ERROR1("Bad UIPC mode:%d", choice);
        return -1;
    }

    app_av_compute_uipc_param(&uipc_cfg, &p_stream->media_feeding);

    if (!UIPC_Ioctl(p_stream->uipc_channel, uipc_mode, NULL))
    {
        APP_ERROR0("Failed setting the UIPC (non) blocking");
        APP_DEBUG0("It will be configured during next AV stop/start");
    }

    /* Copy the computed parameters in UIPC configuration */
    memcpy(&p_stream->uipc_config, &uipc_cfg, sizeof(p_stream->uipc_config));

    return 0;

}

#ifdef APP_3D_INCLUDED
/*******************************************************************************
 **
 ** Function         app_av_bcst_is_audio_registered
 **
 ** Description      This function checks if at least one Audio Stream is registered
 **
 ** Returns          TRUE if at least one Audio Stream in registered
 **
 *******************************************************************************/
BOOLEAN app_av_bcst_is_audio_registered(void)
{
    int stream;
    tAPP_AV_BCST_STREAM *p_stream = &app_av_bcst_cb.stream[0];

    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++, p_stream++)
    {
        /* If this stream is In Use (registered) */
        if (p_stream->in_use)
        {
            return TRUE;
        }
    }
    /* No Stream registered */
    return FALSE;
}

/*******************************************************************************
 **
 ** Function         app_av_bcst_is_audio_started
 **
 ** Description      This function checks if at least one Audio Stream is Started
 **
 ** Returns          TRUE if at least one Audio Stream in registered
 **
 *******************************************************************************/
BOOLEAN app_av_bcst_is_audio_started(void)
{
    int stream;
    tAPP_AV_BCST_STREAM *p_stream = &app_av_bcst_cb.stream[0];

    for (stream = 0 ; stream < APP_AV_BCST_CONNECTIONS_MAX ; stream++, p_stream++)
    {
        /* If this stream is In Use (registered) and Started */
        if ((p_stream->in_use) &&
            (p_stream->is_started))
        {
            return TRUE;
        }
    }
    /* No Stream Started */
    return FALSE;
}

#endif /* APP_3D_INCLUDED */

