/*****************************************************************************
 **
 **  Name:           app_av.h
 **
 **  Description:    Bluetooth Audio/Video Streaming application
 **
 **  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/* idempotency */
#ifndef APP_AV_H_
#define APP_AV_H_

/* for timespec */
#include <time.h>
/* for tBSA_AV_MEDIA_FEEDINGS */
#include "bsa_av_api.h"

/* for various types */
#include "data_types.h"

/* Number of simultaneous A2DP connections supported */
#define APP_AV_MAX_CONNECTIONS 2

/* RC commands */
enum RC_COMMANDS
{
    APP_AV_IDLE,
    APP_AV_START,
    APP_AV_STOP,
    APP_AV_PAUSE,
    APP_AV_FORWARD,
    APP_AV_BACKWARD
};

typedef struct
{
    int timeout;
    struct timespec timestamp;
} tAPP_AV_DELAY;

typedef struct
{
    BOOLEAN is_blocking;
    /* feeding interval in us */
    int period;
    /* feeding interval requested by the user */
    int asked_period;
    int length;
} tAPP_AV_UIPC;


/* Play states */
enum
{
    APP_AV_PLAY_STOPPED,
    APP_AV_PLAY_STARTED,
    APP_AV_PLAY_PAUSED,
    APP_AV_PLAY_STOPPING
};

/*
 * Types
 */
typedef struct
{
    /* Indicate if connection is registered */
    BOOLEAN is_registered;
    /* Handle of the connection */
    tBSA_AV_HNDL handle;
    /* Indicate that connection is open */
    BOOLEAN is_open;
    /* BD ADDRESS of the opened connection */
    BD_ADDR bd_addr;
    /* Indicate that this device has RC buttons */
    BOOLEAN is_rc;
    /* Handle of the remote connection of the device */
    tBSA_AV_RC_HNDL rc_handle;
    /* Latest Delay reported */
    UINT16 delay;
} tAPP_AV_CONNECTION;

/*******************************************************************************
 **
 ** Function         app_av_init
 **
 ** Description      Init AV application
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_init(BOOLEAN boot);

/*******************************************************************************
 **
 ** Function         app_av_end
 **
 ** Description      This function is used to close application
 **
 ** Returns          0 success
 **
 *******************************************************************************/
int app_av_end(void);

/*******************************************************************************
 **
 ** Function         app_av_register
 **
 ** Description      Register a new AV source point
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_register(void);

/*******************************************************************************
 **
 ** Function         app_av_deregister
 **
 ** Description      DeRegister an AV source point
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_deregister(int index);

/*******************************************************************************
 **
 ** Function         app_av_open
 **
 ** Description      Function to open AV connection
 **
 ** Parameters       BD_ADDR of the deivce to connect (if null, user will be prompted)
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_open(BD_ADDR *bda /* = NULL */);

/*******************************************************************************
 **
 ** Function         app_av_close
 **
 ** Description      Function to close AV connection
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_close(void);

/*******************************************************************************
 **
 ** Function         app_av_set_tone_sample_frequency
 **
 ** Description      Change the tone sampling frequency before start
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_av_set_tone_sample_frequency(UINT16 sample_freq);

/*******************************************************************************
 **
 ** Function         app_av_toggle_tone
 **
 ** Description      Toggle the tone type
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_av_toggle_tone(void);

/*******************************************************************************
 **
 ** Function         app_av_play_resume
 **
 ** Description      Resume playing the last stream (after a pause)
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_resume(void);

/*******************************************************************************
 **
 ** Function         app_av_play_tone
 **
 ** Description      Example of function to play a tone
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_tone(void);

/*******************************************************************************
 **
 ** Function         app_av_play_from_avk
 **
 ** Description      Example of function to play audio from avk
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_from_avk();

/*******************************************************************************
 **
 ** Function         app_av_play_file
 **
 ** Description      Example of function to play a file
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_file(void);

/*******************************************************************************
 **
 ** Function         app_av_play_playlist
 **
 ** Description      Example of start to play a play list
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_playlist(UINT8 command);

/*******************************************************************************
 **
 ** Function         app_av_play_mic
 **
 ** Description      Example of function to play a microphone input
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_mic(void);

/*******************************************************************************
 **
 ** Function         app_av_stop
 **
 ** Description      Stop playing
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_stop(void);

/*******************************************************************************
 **
 ** Function         app_av_pause
 **
 ** Description      Pause playing
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_pause(void);

/*******************************************************************************
 **
 ** Function         app_av_resume
 **
 ** Description      Resume playing
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_resume(void);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_command
 **
 ** Description      Example of Send a RemoteControl command
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_command(int index, int command);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_absolute_volume_vd_command
 **
 ** Description      Example of Send a Vendor Specific command
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_absolute_volume_vd_command(int index, int volume);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_get_element_attributes_vd_command
 **
 ** Description      Example of Send a Vendor Specific command
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_get_element_attributes_vd_command(int index);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_get_element_attributes_vd_response
 **
 ** Description      Example of Send response to Vendor Specific command request
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_get_element_attributes_vd_response(int index);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_get_element_attributes_meta_response
 **
 ** Description      Example of Send response to get element attributes meta command request
 **                  BSA_AvMetaRsp currently supports following requests
 **                  GetElementAttributes - BSA_AVRC_PDU_GET_ELEMENT_ATTR and
 **                  GetPlayStatus - BSA_AVRC_PDU_GET_PLAY_STATUS
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_get_element_attributes_meta_response(int index);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_play_status_meta_response
 **
 ** Description      Example of Send response to get play status command request
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_play_status_meta_response(int index);

/*******************************************************************************
 **
 ** Function         app_av_rc_close
 **
 ** Description      Example to close RC handle
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_close(int index);

/*******************************************************************************
 **
 ** Function         app_av_display_connections
 **
 ** Description      This function displays the connections
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_av_display_connections(void);

/*******************************************************************************
 **
 ** Function         app_av_ask_uipc_config
 **
 ** Description      This function is used to configure the UIPC channel
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_av_ask_uipc_config(void);

/*******************************************************************************
 **
 ** Function         app_av_test_sec_codec
 **
 ** Description      This function is used to test SEC codec
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_av_test_sec_codec(BOOLEAN);

/*******************************************************************************
 **
 ** Function         app_av_read_tone
 **
 ** Description      fill up a PCM buffer
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_av_read_tone(short * pOut, int nb_bytes, UINT8 sinus_type, UINT8 *p_sinus_index);

/*******************************************************************************
 **
 ** Function         app_av_compute_uipc_param
 **
 ** Description      This function computes the UIPC length/period
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_av_compute_uipc_param(tAPP_AV_UIPC *p_uipc_cfg, tBSA_AV_MEDIA_FEEDINGS *p_media_feeding);

/*******************************************************************************
 **
 ** Function         app_av_wait_delay
 **
 ** Description      This function wait for a compensated amount of time such
 **                  that the average interval between app_av_wait_delay is exactly
 **                  app_av_cb.uipc_cfg.period
 **
 ** Parameters:      count : number cycle of app_av_cb.uipc_cfg.period time to wait
 **                  p_delay: structure used by app_av_wait_delay to store timing
 **                  information
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_av_wait_delay(int count, tAPP_AV_DELAY *p_delay, tAPP_AV_UIPC *p_uipc_cfg);

/*******************************************************************************
 **
 ** Function         app_av_display_playlist
 **
 ** Description      Display the play list
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_av_display_playlist(void);

/*******************************************************************************
 **
 ** Function         app_av_get_soundfile_path
 **
 ** Description      Get a sound file path from index
 **
 ** Returns          void
 **
 *******************************************************************************/
char *app_av_get_soundfile_path(int file_index);

/*******************************************************************************
 **
 ** Function         app_av_change_cp
 **
 ** Description      This function is used to Change the Content Protection
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_change_cp(void);

/*******************************************************************************
 **
 ** Function         app_av_get_current_cp_desc
 **
 ** Description      Get the current Content Protection Description
 **
 ** Returns          0Content Protection Description
 **
 *******************************************************************************/
char *app_av_get_current_cp_desc(void);

/*******************************************************************************
 **
 ** Function         app_av_set_busy_level
 **
 ** Description      Change busy level
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_set_busy_level(UINT8 level);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_get_element_attributes_meta_response
 **
 ** Description      Example of Send response to get element attributes meta command request
 **                  BSA_AvMetaRsp currently supports following requests
 **                  GetElementAttributes - BSA_AVRC_PDU_GET_ELEMENT_ATTR and
 **                  GetPlayStatus - BSA_AVRC_PDU_GET_PLAY_STATUS
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_get_element_attributes_meta_response(int index);

/*******************************************************************************
 **
 ** Function         app_av_rc_send_play_status_meta_response
 **
 ** Description      Example of Send response to get play status command request
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_play_status_meta_response(int index);

/*******************************************************************************
 **
 ** Function         app_av_rc_set_addr_player_meta_response
 **
 ** Description      Example of set addressed player response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_set_addr_player_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg);

/*******************************************************************************
 **
 ** Function         app_av_rc_get_folder_items
 **
 ** Description      Example of get folder items response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_get_folder_items(int index, tBSA_AV_META_MSG_MSG *pMetaMsg);

/*******************************************************************************
 **
 ** Function         app_av_rc_register_notifications
 **
 ** Description      Example of register for notifications
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/

int app_av_rc_register_notifications(int index, tBSA_AV_META_MSG_MSG *pMetaMsg);

/*******************************************************************************
**
** Function         app_av_rc_complete_notification
**
** Description      Send event notification that was registered before with the
**                  given response code
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_complete_notification(UINT8 event_id);

/*******************************************************************************
**
** Function         app_av_rc_change_play_status
**
** Description      Change the play status. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_change_play_status(UINT8 new_play_status);

/*******************************************************************************
**
** Function         app_av_rc_change_track
**
** Description      Change the track number. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_change_track();

/*******************************************************************************
**
** Function         app_av_rc_addressed_player_change
**
** Description      Change the addressed player. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_addressed_player_change(UINT16 addr_player);

/*******************************************************************************
**
** Function         app_av_rc_settings_change
**
** Description      Change the player setting value. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_settings_change(UINT8 setting, UINT8 value);

/*******************************************************************************
 **
 ** Function         app_av_get_play_state
 **
 ** Description      Get the current play state
 **
 **
 *******************************************************************************/
UINT8 app_av_get_play_state();

/*******************************************************************************
 **
 ** Function         app_av_get_play_type
 **
 ** Description      Get the current play type
 **
 **
 *******************************************************************************/
UINT8 app_av_get_play_type();

/*******************************************************************************
 **
 ** Function         app_av_rc_set_browsed_player_meta_response
 **
 ** Description      Example of browsed addressed player response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_set_browsed_player_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg);

/*******************************************************************************
 **
 ** Function         app_av_rc_change_path_meta_response
 **
 ** Description      Example of change path response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_change_path_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg);

/*******************************************************************************
 **
 ** Function         app_av_rc_get_item_attr_meta_response
 **
 ** Description      Example of GetItemAttr reponse
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_get_item_attr_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg);

/*******************************************************************************
 **
 ** Function         app_av_rc_play_item_meta_response
 **
 ** Description      Example of play item response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_play_item_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg);

#endif /* APP_AV_H_ */
