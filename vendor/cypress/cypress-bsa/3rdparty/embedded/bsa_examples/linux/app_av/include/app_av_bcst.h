/*****************************************************************************
 **
 **  Name:           app_av_bcst.h
 **
 **  Description:    Bluetooth Broadcast Audio Streaming application
 **
 **  Copyright (c) 2012-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_AV_BCST_H
#define APP_AV_BCST_H


/*******************************************************************************
 **
 ** Function         app_av_bcst_stream_display
 **
 ** Description      This function displays a Broadcast AV stream
 **
 ** Returns          Number of stream in use
 **
 *******************************************************************************/
int app_av_bcst_stream_display(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_init
 **
 ** Description      Initialize Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_init(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_end
 **
 ** Description      Stop Broadcast AV Stream application
 **
 ** Returns          Void
 **
 *******************************************************************************/
void app_av_bcst_end(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_register
 **
 ** Description      Register a new Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_register(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_deregister
 **
 ** Description      Register a new Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_deregister(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_tone
 **
 ** Description      Start to play a Tone on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_tone(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_file
 **
 ** Description      Start to play a File on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_file(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_mic
 **
 ** Description      Start to play a Microphone Input to Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_mic(void);

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
int app_av_bcst_play_loopback_input(void);
#endif

/*******************************************************************************
 **
 ** Function         app_av_bcst_play_mm
 **
 ** Description      Start to play MM (Kernel Audio) on Broadcast AV Stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_play_mm(void);

/*******************************************************************************
 **
 ** Function         app_av_stop_current
 **
 ** Description      Stop playing current stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_bcst_stop(void);

/*******************************************************************************
 **
 ** Function         app_av_bcst_start_event_hdlr
 **
 ** Description      Handle AV Broadcast Start event
 **
 ** Returns          Void
 **
 *******************************************************************************/
void app_av_bcst_start_event_hdlr(tBSA_AV_START_MSG *p_msg);

/*******************************************************************************
 **
 ** Function         app_av_bcst_stop_event_hdlr
 **
 ** Description      Handle AV Broadcast Stop event
 **
 ** Returns          Void
 **
 *******************************************************************************/
void app_av_bcst_stop_event_hdlr(tBSA_AV_STOP_MSG *p_msg);

/*******************************************************************************
 **
 ** Function         app_av_bcst_ask_uipc_config
 **
 ** Description      This function is used to configure a Broadcast UIPC channel
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_av_bcst_ask_uipc_config(void);

#endif /* APP_AV_BCST_H */
