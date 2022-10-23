/*****************************************************************************
 **
 **  Name:           av.c
 **
 **  Description:    Bluetooth audio video application
 **
 **  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "bsa_api.h"

#include "gki.h"
#include "uipc.h"

#include "av.h"

/*
 * Defines
 */
#define APP_MAX_AUDIO_BUF 256


/*
 * global variable
 * */

static BOOLEAN          av_open_rcv = FALSE;
static tBSA_STATUS      av_open_status;

static BOOLEAN          av_close_rcv = FALSE;

static BOOLEAN          av_start_rcv = FALSE;
static tBSA_STATUS      av_start_status;

static BOOLEAN          av_stop_rcv = FALSE;


/*******************************************************************************
 **
 ** Function         av_cback
 **
 ** Description      This function is the AV callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void av_cback(tBSA_AV_EVT event, tBSA_AV_MSG *p_data)
{
    switch (event) {
    case BSA_AV_OPEN_EVT:
        printf("av_cback: BSA_AV_OPEN_EVT status 0x%x\n", p_data->open.status);
        av_open_status = p_data->open.status;
        av_open_rcv = TRUE;
        break;

    case BSA_AV_CLOSE_EVT:
        printf("av_cback: BSA_AV_CLOSE_EVT status 0x%x\n", p_data->close.status);
        av_close_rcv = TRUE;
        break;

    case BSA_AV_START_EVT:
        printf("av_cback: BSA_AV_START_EVT status 0x%x\n", p_data->start.status);
        av_start_status = p_data->start.status;
        av_start_rcv = TRUE;
        break;

    case BSA_AV_STOP_EVT:
        printf("av_cback: BSA_AV_STOP_EVT\n");
        av_stop_rcv = TRUE;
        break;

    case BSA_AV_RC_OPEN_EVT:
        printf("av_cback: BSA_AV_RC_OPEN_EVT status 0x%x\n",
                p_data->rc_open.status);
        break;

    case BSA_AV_RC_CLOSE_EVT:
        printf("av_cback: BSA_AV_RC_CLOSE_EVT handle 0x%x\n",
                p_data->rc_close.rc_handle);
        break;

    case BSA_AV_REMOTE_CMD_EVT:
        printf("av_cback: BSA_AV_REMOTE_CMD_EVT handle 0x%x id:%d state:%d\n",
                p_data->remote_cmd.rc_handle, p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state);
        break;

    case BSA_AV_REMOTE_RSP_EVT:
        printf("av_cback: BSA_AV_REMOTE_RSP_EVT handle 0x%x\n", p_data->remote_rsp.rc_handle);
        break;

    case BSA_AV_META_MSG_EVT:
        printf("av_cback: BSA_AV_META_MSG_EVT handle 0x%x\n", p_data->meta_msg.rc_handle);
        break;

    default:
        printf("av_cback: unknown msg %d\n", event);
        break;
    }
}


/*******************************************************************************
 **
 ** Function         av_read_tone
 **
 ** Description      fill up a PCM buffer
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void av_read_tone(short * pOut)
{
    short sinwave[32] = { 0, 488, 957, 1389, 1768, 2079, 2310, 2452, 2500, 2452, 2310, 2079, 1768,
                    1389, 957, 488, 0, -488, -957, -1389, -1768, -2079, -2310, -2452, -2500, -2452,
                    -2310, -2079, -1768, -1389, -957, -488 };
    static int StaticIndex = 0;
    int Index;

    for (Index = 0; Index < (APP_MAX_AUDIO_BUF / 2); Index++)
    {
        /* Stereo for this demo */
        pOut[Index * 2] = sinwave[StaticIndex % 32];
        pOut[Index * 2 + 1] = sinwave[StaticIndex % 32];
        StaticIndex++;
    }
}

/*******************************************************************************
 **
 ** Function         av_play_tone_init_param
 **
 ** Description      initializes play tone param
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int av_play_tone_init_param(tAV_PLAY_TONE_PARAM *av_play_tone_param)
{
    printf("av_play_tone_init_param\n");
    bzero(av_play_tone_param, sizeof(tAV_PLAY_TONE_PARAM));
    return 0;
}

/*******************************************************************************
 **
 ** Function         av_play_tone
 **
 ** Description      play tone
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int av_play_tone(tAV_PLAY_TONE_PARAM *av_play_tone_param)
{
    tBSA_MGT_CLOSE bsa_close_param;
    tBSA_AV_ENABLE bsa_av_enable_param;
    tBSA_AV_DISABLE bsa_av_disable_param;
    tBSA_AV_REGISTER bsa_av_register_param;
    tBSA_AV_DEREGISTER bsa_av_deregister_param;
    tBSA_AV_OPEN bsa_av_open_param;
    tBSA_AV_CLOSE bsa_av_close_param;
    tBSA_AV_START bsa_av_start_param;
    tBSA_AV_STOP bsa_av_stop_param;
    tUIPC_CH_ID bsa_av_uipc_channel;
    tBSA_STATUS status;
    UINT32 index = 0;
    tBSA_AV_HNDL av_handle;
    short app_audio_buf[APP_MAX_AUDIO_BUF];

    printf("av_play_tone\n");

    /* Enable AV */
    BSA_AvEnableInit(&bsa_av_enable_param);
    bsa_av_enable_param.p_cback = av_cback;
    bsa_av_enable_param.features = BSA_AV_FEAT_RCTG;
    status = BSA_AvEnable(&bsa_av_enable_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "Unable to enable AV service \n");
        goto av_mgt_close;
    }

    /* Register an audio AV end point */
    BSA_AvRegisterInit(&bsa_av_register_param);
    bsa_av_register_param.channel = BSA_AV_CHNL_AUDIO;
    status = BSA_AvRegister(&bsa_av_register_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "Unable to register AV source status 0x%x\n", status);
        goto av_mgt_close;
    }
    /* Save register data */
    av_handle = bsa_av_register_param.handle;
    bsa_av_uipc_channel = bsa_av_register_param.uipc_channel;

    /* Open UIPC channel */
    printf("Opening UIPC Channel\n");
    if (UIPC_Open(bsa_av_uipc_channel, NULL) == FALSE)
    {
        fprintf(stderr, "Unable to open AV UIPC channel status:%d\n", status);
        goto av_mgt_close;
    }

    /* Open AV connection to peer device */
    BSA_AvOpenInit(&bsa_av_open_param);
    memcpy((char *) (bsa_av_open_param.bd_addr), av_play_tone_param->bd_addr, sizeof(BD_ADDR));
    bsa_av_open_param.handle = av_handle;
    bsa_av_open_param.use_rc = TRUE;
    status = BSA_AvOpen(&bsa_av_open_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "Unable to connect HID device status:%d\n", status);
        goto av_mgt_close;
    }

    /* This is an active wait for demo purpose */
    while(av_open_rcv == FALSE);

    if (av_open_status != BSA_SUCCESS)
    {
        fprintf(stderr, "Unable to connect HID device status:%d\n", av_open_status);
        goto av_mgt_close;
    }

    /* Start Av stream */
    BSA_AvStartInit(&bsa_av_start_param);
    bsa_av_start_param.media_feeding.format = BSA_AV_CODEC_PCM;
    /* Supported sampling frequencies: */
    /* 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100 and 48000 */
    bsa_av_start_param.media_feeding.cfg.pcm.sampling_freq = 44100;
    /* Supported number of channel: 1 (mono) or 2 (stereo) */
    bsa_av_start_param.media_feeding.cfg.pcm.num_channel = 2;
    /* Supported bits per samples: 8 (unsigned) or 16 (signed) */
    bsa_av_start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    status = BSA_AvStart(&bsa_av_start_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "Unable to start AV 0x%x \n", status);
        goto av_mgt_close;
    }

    /* This is an active wait for demo purpose */
    printf("waiting for av connection to start \n");
    while (av_start_rcv == FALSE);

    if (av_start_status != BSA_SUCCESS)
    {
        printf("failure starting av connection : status %d \n", av_start_status);
        return status;
    }

    printf("sending audio tone:\n");

    /* Send 60 sec of tone */
    while (index++ * APP_MAX_AUDIO_BUF < (2*44100 * 60 * sizeof(INT16)))
    {
        /* Print something once per sec (for info) */
        if (0 == (index % ((44100 * 2 * sizeof(INT16)) / APP_MAX_AUDIO_BUF)))
            printf("- samples %u \n", index++ * APP_MAX_AUDIO_BUF / 4);

        /* Send a sinwave signal */
        av_read_tone(app_audio_buf);
        if (TRUE != UIPC_Send(bsa_av_uipc_channel, 0, (UINT8 *) app_audio_buf,
                APP_MAX_AUDIO_BUF*sizeof(short)))
        {
            printf("error in UIPC send could not send data \n");
            break;
        }
    }

    printf("nb byte sent %u, i.e. %u sec of audio\n",
            index *APP_MAX_AUDIO_BUF, index * APP_MAX_AUDIO_BUF / (4 * 44100));

    /* Stop the AV stream */
    status = BSA_AvStopInit(&bsa_av_stop_param);
    status = BSA_AvStop(&bsa_av_stop_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "Unable to Stop AV status:0x%x \n", status);
        goto av_mgt_close;
    }
    /* This is an active wait for demo purpose */
    /* Wait AV stop event */
    while(av_stop_rcv == FALSE);

    /* Close av connection */
    status = BSA_AvCloseInit(&bsa_av_close_param);
    bsa_av_close_param.handle = av_handle;
    status = BSA_AvClose(&bsa_av_close_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "Unable to Close AV status:0x%x \n", status);
        goto av_mgt_close;
    }
    /* This is an active wait for demo purpose */
    /* Wait AV close event */
    while(av_close_rcv == FALSE);

    /* Dlose UIPC channel */
    printf("Closing UIPC Channel\n");
    UIPC_Close(bsa_av_uipc_channel);

    /* Deregister av */
    status = BSA_AvDeregisterInit(&bsa_av_deregister_param);
    bsa_av_deregister_param.handle = bsa_av_register_param.handle;
    status = BSA_AvDeregister(&bsa_av_deregister_param);

    /* Disable av */
    status = BSA_AvDisableInit(&bsa_av_disable_param);
    status = BSA_AvDisable(&bsa_av_disable_param);

av_mgt_close:

    /* Close BSA before exiting (to release resources) */
    BSA_MgtCloseInit(&bsa_close_param);
    BSA_MgtClose(&bsa_close_param);

    exit(0);
}

