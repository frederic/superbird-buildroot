/*****************************************************************************
**
**  Name:               app_ag.h
**
**  Description:        This is the header file for the Headset application
**
**  Copyright (c) 2009-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef APP_AG_H
#define APP_AG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "data_types.h"
#include "bsa_sec_api.h"
#include "uipc.h"
#include "bsa_ag_api.h"
#include "app_wav.h"


#ifndef BSA_SCO_ROUTE_DEFAULT
#define BSA_SCO_ROUTE_DEFAULT BSA_SCO_ROUTE_HCI
#endif

#define APP_AG_MAX_NUM_CONN    1
#define APP_AG_SCO_IN_SOUND_FILE    "./test_files/ag/sco_ag_in.wav"
#define APP_AG_SCO_OUT_SOUND_FILE   "./test_files/ag/sco_ag_out.wav"

/* control block (not needed to be stored in NVRAM) */
typedef struct
{
    UINT16                  hndl[APP_AG_MAX_NUM_CONN];
    int                     fd_sco_in_file;
    int                     fd_sco_out_file;
    tAPP_WAV_FILE_FORMAT    audio_format;
    BOOLEAN                 opened;
    BOOLEAN                 voice_opened;
    BOOLEAN                 call_active;
    UINT8                   sco_route;
    tUIPC_CH_ID             uipc_channel[APP_AG_MAX_NUM_CONN];
    UINT8                   pcmbuf[240*2];
    BOOLEAN                 stop_play;
} tAPP_AG_CB;

extern tAPP_AG_CB app_ag_cb;

#ifdef  __cplusplus
}
#endif

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
void app_ag_init(void);

/*******************************************************************************
 **
 ** Function         app_ag_open_rec_file
 **
 ** Description     Open recording file
 **
 ** Parameters      filename to open
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_open_rec_file(char * filename);

/*******************************************************************************
 **
 ** Function         app_ag_close_rec_file
 **
 ** Description     Close recording file
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_close_rec_file(void);

/*******************************************************************************
 **
 ** Function         app_ag_status_2_str
 **
 ** Description      convert a tBSA_STATUS to a string of char
 **
 ** Parameters
 **             status: status code to convert
 **
 ** Returns
 **             converted string
 **
 *******************************************************************************/
char * app_ag_status_2_str(tBSA_STATUS status);

/*******************************************************************************
 **
 ** Function         app_ag_enable
 **
 ** Description      enable mono AG service
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_enable(void);

/*******************************************************************************
 **
 ** Function         app_ag_disable
 **
 ** Description      disable mono AG service
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_disable(void);

/*******************************************************************************
 **
 ** Function         app_ag_register
 **
 ** Description      Register one AG instance
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_register(UINT8 sco_route);

/*******************************************************************************
 **
 ** Function         app_ag_deregister
 **
 ** Description      deregister all AG instance
 **
 ** Returns          BSA_SUCCESS success error code for failure
 *******************************************************************************/
tBSA_STATUS app_ag_deregister(void);

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
int app_ag_open(BD_ADDR *bd_addr_in /*= NULL*/);

/*******************************************************************************
 **
 ** Function         app_ag_close
 **
 ** Description      Closes mono headset connections
 **
 ** Returns          TRUE or FALSE for success or failure
 *******************************************************************************/
tBSA_STATUS app_ag_close(void);

/*******************************************************************************
 **
 ** Function         app_ag_open_audio
 **
 ** Description      open ag sco link
 **
 ** Returns          TRUE or FALSE for success or failure
 *******************************************************************************/
void app_ag_open_audio(void);

/*******************************************************************************
 **
 ** Function         app_ag_close_audio
 **
 ** Description      close ag sco link
 **
 ** Returns          TRUE or FALSE for success or failure
 *******************************************************************************/
void app_ag_close_audio(void);

/*******************************************************************************
 **
 ** Function         app_ag_play_file
 **
 ** Description      Play SCO data from file to SCO OUT channel
 **
 ** Parameters
 **
 ** Returns          BSA_STATUS/ BSA error code or -1 in case of error
 **
 *******************************************************************************/
int app_ag_play_file(void);

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
int app_ag_stop_play_file(void);

/*******************************************************************************
 **
 ** Function        app_ag_close_playing_file
 **
 ** Description     Close playing file
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_ag_close_playing_file(void);

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
void app_ag_pickupcall(UINT16 handle);

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
void app_ag_hangupcall(UINT16 handle);

/*******************************************************************************
 **
 ** Function         app_ag_start
 **
 ** Description      Example of function to start the AG application
 **
 ** Parameters
 **
 ** Returns          BSA_SUCCESS/BSA error code
 **
 *******************************************************************************/
tBSA_STATUS app_ag_start(UINT8 sco_route);

/*******************************************************************************
 **
 ** Function         app_ag_stop
 **
 ** Description      Example of function to start the AG application
 **
 ** Parameters
 **
 ** Returns          BSA_SUCCESS/BSA error code
 **
 *******************************************************************************/
tBSA_STATUS app_ag_stop(void);

/*******************************************************************************
 **
 ** Function         app_ag_end
 **
 ** Description      This function is used to close AG
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ag_end(void);

/*******************************************************************************
 **
 ** Function         app_ag_set_cback
 **
 ** Description      Set application callback for third party app
 **
 **
 *******************************************************************************/
void app_ag_set_cback(tBSA_AG_CBACK pcb);

#endif /* APP_AG_H_ */
