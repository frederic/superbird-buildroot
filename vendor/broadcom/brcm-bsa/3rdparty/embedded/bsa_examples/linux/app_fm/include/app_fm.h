/*****************************************************************************
 **
 **  Name:           app_fm.h
 **
 **  Description:    Contains application functions Bluetooth FM Profile
 **
 **
 **  Copyright (c) 2006, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#ifndef APP_FM_H
#define APP_FM_H

#include "bsa_fm_api.h"
#include "bsa_rds_api.h"

#ifndef APP_FM_C
#define _EXT_
#else
#define _EXT_ extern
#endif

#ifndef BTUI_FM_TEST
#define BTUI_FM_TEST            FALSE
#endif

/* BTAPP FM state */
#define APP_FM_NULL_ST             0x00
#define APP_FM_ACTIVE_ST           0x01

#define APP_FM_GET_FREQ(x)     (UINT16)((x + 0.01) *10)
#define APP_FM_CVT_FREQ(x)     (float) (x/10.0)
#define APP_FM_MAX_FREQ        APP_FM_GET_FREQ(108.0)
#define APP_FM_MIN_FREQ        APP_FM_GET_FREQ(87.5)
#define APP_FM_DEFAULT_STEP    1

#define APP_FM_MAX_STAT_LST    15

#define BSA_FM_SCO_OFF  0
#define BSA_FM_SCO_ON   1

enum
{
    APP_FM_SCAN_OFF,
    APP_FM_SCAN_ON,
    APP_FM_SCAN_FULL_ON
};
typedef UINT8  tAPP_FM_SCAN_STAT;

typedef struct
{
    BOOLEAN             af_avail;
    BOOLEAN             af_on;
    UINT16              pi_code;
    tBSA_FM_AF_LIST     af_list;
}tAPP_FM_AF_CB;


typedef void (tAPP_FM_CBACK)(tBSA_FM_EVT event, tBSA_FM *p_data);
typedef void (tAPP_FM_RDS_CBACK)(tBSA_FM_RDS_EVT event, UINT8 *p_data);

typedef struct
{
    BOOLEAN                 enabled;
    BOOLEAN                 audio_quality_enabled;
    BOOLEAN                 rds_on;
    BOOLEAN                 sco_on;
    BOOLEAN                 sco_pending;
    BOOLEAN                 fm_pending;
    tBSA_FM_AUDIO_MODE      mode;
    UINT16                     service_mask;
    UINT16                     cur_freq;
    UINT16                     last_freq;
    UINT16                     stat_lst[APP_FM_MAX_STAT_LST];
    UINT8                       stat_num;
    tAPP_FM_SCAN_STAT     scan_stat;
    BOOLEAN                     audio_muted;
    UINT8                       nb_station_found;
    UINT8                       rssi_threshold;
    tBSA_FM_SCAN_MODE           scn_mode;
    tAPP_FM_CBACK               *app_fm_cb;
} tAPP_FM_CB;

typedef struct
{
    BOOLEAN                 ps_on;
    BOOLEAN                 pty_on;
    BOOLEAN                 ptyn_on;
    BOOLEAN                 rt_on;
    UINT8*                    prg_service;
    UINT8*                    prg_type;
    UINT8*                    prg_type_name;
    UINT8*                    radio_txt;
    tAPP_FM_AF_CB   af_cb;
    tAPP_FM_RDS_CBACK       * app_fm_cb;
} tAPP_RDS_CB;

extern tAPP_FM_CB app_fm_cb;
extern tAPP_RDS_CB app_rds_cb;

/*******************************************************************************
 **
 ** Function         app_fm_enable
 **
 ** Description    Enable FM
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_enable( tBSA_FM_FUNC_MASK func_mask );

/*******************************************************************************
 **
 ** Function         app_fm_disable
 **
 ** Description      Disables PBS
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_disable(void);

/*******************************************************************************
 **
 ** Function         app_fm_is_enabled
 **
 ** Description    return the current state of FM
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ BOOLEAN app_fm_is_enabled(void);


/*******************************************************************************
 **
 ** Function         app_fm_is_rds_on
 **
 ** Description    return the state of RDS (TRUE if ON, FALSE if OFF)
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ BOOLEAN app_fm_is_rds_on(void);

/*******************************************************************************
 **
 ** Function         app_fm_is_fm_sco_on
 **
 ** Description    return the state of fm over sco
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ BOOLEAN app_fm_is_fm_sco_on(void);

/*******************************************************************************
 **
 ** Function       app_fm_is_audio_quality_on
 **
 ** Description    return the state of audio quality status
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ BOOLEAN app_fm_is_audio_quality_on(void);

/*******************************************************************************
 **
 ** Function       app_fm_is_fm_audio_muted
 **
 ** Description    return the state of the audio mute state
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ BOOLEAN app_fm_is_fm_audio_muted(void);

/*******************************************************************************
 **
 ** Function         app_fm_is_af_on
 **
 ** Description    return the state of Alternative Frequency
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ BOOLEAN app_fm_is_af_on(void);


/*******************************************************************************
 **
 ** Function         app_fm_get_current_audio_mode
 **
 ** Description    return the current audio mode
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ tBSA_FM_AUDIO_MODE app_fm_get_current_audio_mode(void);

/*******************************************************************************
 **
 ** Function        app_fm_read_rds
 **
 ** Description   Reads the RDS information of the current FM radio station
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_read_rds(void);

/*******************************************************************************
 **
 ** Function       app_fm_tune_frequency
 **
 ** Description   Sets the FM receiver to a specific frequency.
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_tune_freq( UINT16 freq );

/*******************************************************************************
 **
 ** Function         app_fm_tune_freq
 **
 ** Description      Connects to the selected device
 **
 ** Returns          void
 *******************************************************************************/
_EXT_ void app_fm_set_volume(UINT16 vol);

/*******************************************************************************
 **
 ** Function       app_fm_search_frequency
 **
 ** Description   Searches the radio band for good channels.
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_search_freq(tBSA_FM_SCAN_MODE scn_mode);

/*******************************************************************************
 **
 ** Function         app_fm_rds_search
 **
 ** Description
 **
 **
 ** Returns          void
 *******************************************************************************/
_EXT_ void app_fm_rds_search(tBSA_FM_SCAN_MODE scn_mode, tBSA_FM_SCH_RDS_COND rds_cond);


/*******************************************************************************
 **
 ** Function       app_fm_menu_set_audio_mode
 **
 ** Description   Manually sets the FM radio control parameter for mono/stereo/blend mode.
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_set_audio_mode(tBSA_FM_AUDIO_MODE mode);


/*******************************************************************************
 **
 ** Function       app_fm_set_rds
 **
 ** Description   Turn RDS and/or the AF algorithm ON/OFF.
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_set_rds_mode(BOOLEAN rds_on, BOOLEAN af_on);

/*******************************************************************************
 **
 ** Function         app_fm_set_region
 **
 ** Description      Sets the region
 **
 **
 ** Returns          void
 *******************************************************************************/
_EXT_ void app_fm_set_region(tBSA_FM_REGION_CODE region);

/*******************************************************************************
 **
 ** Function         app_fm_abort
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
_EXT_ void app_fm_abort(void);

/*******************************************************************************
 **
 ** Function         app_fm_route_audio
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
_EXT_ void app_fm_route_audio(tBSA_FM_AUDIO_PATH path);

/*******************************************************************************
 **
 ** Function       app_fm_set_audio_quality
 **
 ** Description   Set the audio quality status
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_set_audio_quality( BOOLEAN turn_on);

/*******************************************************************************
 **
 ** Function       app_fm_config_deemphasis
 **
 ** Description   Set the emphasis/deemphasis time constant
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_set_deemphasis(tBSA_FM_DEEMPHA_TIME interval);

/*******************************************************************************
 **
 ** Function         app_fm_rds_cback
 **
 ** Description     FM Callback function.  Handles all FM events
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
_EXT_ void app_fm_rdsp_cback(tBSA_RDS_EVT event, tBSA_FM_RDS *p_data, UINT8 app_id);

#endif
