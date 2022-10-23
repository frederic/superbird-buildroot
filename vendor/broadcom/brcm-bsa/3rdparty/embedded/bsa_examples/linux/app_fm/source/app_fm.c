/****************************************************************************
 **
 **  Name:          app_fm.c
 **
 **  Description:   Contains application functions for FM Radio.
 **
 **
 **  Copyright (c) 2003-2006, Broadcom Corp., All Rights Reserved.
 **  Widcomm Bluetooth Core. Proprietary and confidential.
 ******************************************************************************/

#include "bt_target.h"

#define APP_FM_C

#include "gki.h"
#include "bsa_api.h"
#include "app_fm.h"
#include "bsa_fm_api.h"
#include "bsa_fm_hwhdr.h"
#include "bsa_rds_api.h"

static char * app_fm_status[10] =
{
    "BSA_FM_OK",
    "BSA_FM_SCAN_RSSI_LOW",
    "BSA_FM_SCAN_FAIL",
    "BSA_FM_SCAN_ABORT",
    "BSA_FM_RDS_CONT",
    "BSA_FM_ERR"
    "BSA_FM_UNSPT_ERR",
    "BSA_FM_FLAG_TOUT_ERR",
    "BSA_FM_FREQ_ERR",
    "BSA_FM_VCMD_ERR"
};

char * app_fm_am[4]=
{
    "BSA_FM_AUTO_MODE",       /* auto mode, blending by default */
    "BSA_FM_STEREO_MODE",     /* manual stereo switch */
    "BSA_FM_MONO_MODE",       /* manual mono switch */
    "BSA_FM_SWITCH_MODE"       /* manual stereo blending */
};

const UINT8 rssi_thresh_audio[4] =
{
    85, /* "Auto"   */
    70, /* "Stereo" */
    85, /* "Mono"   */
    85  /* "Switch"  */
};

#define APP_IDLE  0
#define APP_SCAN 1

#ifndef APP_FM_RESULT_HIGH_WM
#define APP_FM_RESULT_HIGH_WM 25 /* water mark that will triger the decrease of the rssi threshold */
#endif

#ifndef APP_FM_RESULT_LOW_WM
#define APP_FM_RESULT_LOW_WM 2 /* Water mark that will triger the increase of the rssi threshold */
#endif


#define APP_FM_RSSI_THRESH_MAX 115
#define APP_FM_RSSI_THRESH_MIN  70

#define APP_FM_RSSI_THRESH_STEP   2

#ifndef APP_FM_DEFAULT_RSSI_THRESH
#define APP_FM_DEFAULT_RSSI_THRESH 78
#endif

tAPP_FM_CB app_fm_cb;
tAPP_RDS_CB app_rds_cb;

/*****************************************************************************
 **  Local Function prototypes
 *****************************************************************************/
static void app_fm_cback(tBSA_FM_EVT event, tBSA_FM *p_data);
static BOOLEAN app_fm_refresh_rssi_threshold(void);
void defualt_cb(tBSA_FM_EVT event, tBSA_FM *p_data){}
void default_rds_cb(tBSA_FM_EVT event, UINT8*p_data){}
void app_fm_cback(tBSA_FM_EVT event, tBSA_FM *p_data);
void app_fm_rds_cback(UINT16 event, UINT8*p_data);

void app_fm_init(tAPP_FM_CBACK app_cb, tAPP_FM_RDS_CBACK rds_cb)
{
    memset(&app_fm_cb,0,sizeof(app_fm_cb));
    memset(&app_rds_cb,0,sizeof(app_rds_cb));
    if (app_cb)
        app_fm_cb.app_fm_cb = app_cb;
    else
        app_fm_cb.app_fm_cb = defualt_cb;

    if (rds_cb)
        app_rds_cb.app_fm_cb = rds_cb;
    else
        app_rds_cb.app_fm_cb = default_rds_cb;
}

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
void app_fm_enable( tBSA_FM_FUNC_MASK func_mask )
{
    tBSA_FM_ENABLE p_req;
    APPL_TRACE_API1("[FM] app_fm_enable(func_mask=x%x)", func_mask );
    BSA_FmEnableInit(&p_req);
    p_req.func_mask = func_mask;
    p_req.p_cback = app_fm_cback;
    p_req.p_rds_cback = app_fm_rds_cback;
    p_req.app_id = 0;
    app_fm_cb.rssi_threshold =  APP_FM_DEFAULT_RSSI_THRESH;

    BSA_FmEnable( &p_req );
}


/*******************************************************************************
 **
 ** Function         app_fm_disable
 **
 ** Description      Disable FM.
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_disable(void)
{
    memset(&app_fm_cb, 0, sizeof(tAPP_FM_CB));
    memset(&app_rds_cb, 0, sizeof(tAPP_RDS_CB));

#if APP_FM_DEBUG==TRUE
    APPL_TRACE_API0("[FM] app_fm_disable()" );
#endif

    BSA_FmDisable();
}


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
BOOLEAN app_fm_is_enabled(void)
{
    return app_fm_cb.enabled;
}

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
BOOLEAN app_fm_is_rds_on(void)
{
    return app_fm_cb.rds_on;
}

/*******************************************************************************
 **
 ** Function         app_fm_is_fm_sco_on
 **
 ** Description    return the state of RDS (TRUE if ON, FALSE if OFF)
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
BOOLEAN app_fm_is_fm_sco_on(void)
{
    return app_fm_cb.sco_on;
}

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
BOOLEAN app_fm_is_audio_quality_on(void)
{
    return app_fm_cb.audio_quality_enabled;
}

/*******************************************************************************
 **
 ** Function       app_fm_is_fm_audio_muted
 **
 ** Description    return the state of audio
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
BOOLEAN app_fm_is_fm_audio_muted(void)
{
    return app_fm_cb.audio_muted;
}

/*******************************************************************************
 **
 ** Function         app_fm_tune_freq
 **
 ** Description      Connects to the selected device
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_tune_freq(UINT16 freq)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_DEBUG1("[FM] app_fm_tune_freq(%d)", freq );
#endif
    app_rds_cb.ps_on = app_rds_cb.pty_on = app_rds_cb.ptyn_on = app_rds_cb.rt_on = FALSE;
    BSA_FmTuneFreq(freq);
}

/*******************************************************************************
 **
 ** Function         app_fm_set_volume
 **
 ** Description      Set volume level.
 **
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_set_volume(UINT16 vol)
{
    BSA_FmVolumeControl(vol);
}

/*******************************************************************************
 **
 ** Function         app_fm_set_region
 **
 ** Description      Sets the region
 **
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_set_region(tBSA_FM_REGION_CODE region)
{
    BSA_FmSetRegion(region);
}

/*******************************************************************************
 **
 ** Function         app_fm_search_freq
 **
 ** Description
 **
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_search_freq(tBSA_FM_SCAN_MODE scn_mode)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_DEBUG2("[FM] app_fm_search_frequency - mode = x%x rssi threshold %d", scn_mode, app_fm_cb.rssi_threshold );
#endif
    tBSA_FM_MSGID_SRCH_CMD_REQ req;
    BSA_FmSearchFreqInit(&req);

    app_fm_cb.nb_station_found = 0;

    app_fm_cb.scn_mode = scn_mode;

    app_rds_cb.ps_on = app_rds_cb.pty_on = app_rds_cb.ptyn_on = app_rds_cb.rt_on = FALSE;
    req.scn_mode = scn_mode;
    req.rssi_thresh = app_fm_cb.rssi_threshold;
    req.p_rds_cond = NULL;
    BSA_FmSearchFreq(&req);
}


/*******************************************************************************
 **
 ** Function         app_fm_rds_search
 **
 ** Description
 **
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_rds_search( tBSA_FM_SCAN_MODE scn_mode, tBSA_FM_SCH_RDS_COND rds_cond )
{
    tBSA_FM_MSGID_SRCH_CMD_REQ req;

#if APP_FM_DEBUG==TRUE
   APPL_TRACE_DEBUG3("[FM] app_fm_rds_search( scn_mode = x%x, rds_cond_type = x%x, rds_cond_val = x%x",
                    scn_mode, rds_cond.cond_type, rds_cond.cond_val );
#endif

 #if 0
   if (scn_mode == BSA_FM_SCAN_FULL)
    {
        memset(&btui_fm_db, 0, sizeof(tBTUI_FM_DB));
    }
#endif

   BSA_FmSearchFreqInit(&req);

   app_fm_cb.nb_station_found = 0;

   app_fm_cb.scn_mode = scn_mode;

   app_rds_cb.ps_on = app_rds_cb.pty_on = app_rds_cb.ptyn_on = app_rds_cb.rt_on = FALSE;

    req.scn_mode = scn_mode;
    req.rssi_thresh = rssi_thresh_audio[app_fm_cb.mode];
    req.p_rds_cond = &rds_cond;
    BSA_FmSearchFreq(&req);

    app_rds_cb.ps_on = app_rds_cb.pty_on = app_rds_cb.ptyn_on = app_rds_cb.rt_on = FALSE;

    return;
}


/*******************************************************************************
 **
 ** Function         app_fm_set_rds_mode
 **
 ** Description      Turn on/off RDS/AF mode.
 **
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_set_rds_mode(BOOLEAN rds_on, BOOLEAN af_on)
{
#if APP_FM_DEBUG==TRUE
   APPL_TRACE_DEBUG2("[FM] app_fm_set_rds_mode( rds_on=%2.2d, af_on=%2.2d )", rds_on, af_on );
#endif

    tBSA_FM_MSGID_SET_RDS_CMD_REQ req;

    BSA_FmSetRDSModeInit(&req);
    req.rds_on = rds_on;
    req.af_on = af_on;

    if (af_on && app_rds_cb.af_cb.af_avail)
    {
        req.af_struct.pi_code = app_rds_cb.af_cb.pi_code;
        memcpy(&(req.af_struct.af_list), &app_rds_cb.af_cb.af_list, sizeof(tBSA_FM_AF_LIST));
    }

    /* enable RDS data polling with once every 3 seconds */
    BSA_FmSetRDSMode(&req);
}


/*******************************************************************************
 **
 ** Function         app_fm_abort
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_abort(void)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_DEBUG0("[FM] app_fm_abort()");
#endif
    BSA_FmSearchAbort();
}

/*******************************************************************************
 **
 ** Function         app_fm_set_audio_mode
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_set_audio_mode(tBSA_FM_AUDIO_MODE mode)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_DEBUG1("[FM] app_fm_set_audio_mode - requested mode= %s", app_fm_am[mode]);
#endif
   BSA_FmSetAudioMode(mode);
}

/*******************************************************************************
 **
 ** Function         app_fm_read_rds
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_read_rds(void)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_DEBUG0("[FM] app_fm_read_rds()");
#endif
    BSA_FmReadRDS();
}
/*******************************************************************************
 **
 ** Function         app_fm_route_audio
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_route_audio(tBSA_FM_AUDIO_PATH path)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_DEBUG1("[FM] app_fm_route_audio(path=x%x)", path);
#endif
    BSA_FmConfigAudioPath(path);
}

/*******************************************************************************
 **
 ** Function         app_fm_set_deemphasis
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_set_deemphasis(tBSA_FM_DEEMPHA_TIME interval)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_DEBUG1("[FM] app_fm_set_deemphasis time interval = %d", interval );
#endif
    BSA_FmConfigDeemphasis(interval);
}

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
void app_fm_set_audio_quality( BOOLEAN turn_on)
{
    APPL_TRACE_API1("[FM] app_fm_set_audio_quality( turn_on = %2.2d )", turn_on);

    app_fm_cb.audio_quality_enabled = turn_on;
    BSA_FmReadAudioQuality (turn_on);

}

/*******************************************************************************
**
** Function         app_fm_mute_audio
**
** Description      Mute/unmute FM audio
**
** Returns          void
*******************************************************************************/
void app_fm_mute_audio(BOOLEAN mute)
{
    APPL_TRACE_API1("[FM] app_fm_mute_audio mute = %d )", mute);
    BSA_FmMute(mute);
}

/*******************************************************************************
 **
 ** Function         app_fm_clear_af_info
 **
 ** Description      Reset AF information if a new frequency is set.
 **
 **
 ** Returns          void
 *******************************************************************************/
static void app_fm_clear_af_info(void)
{
    BOOLEAN af_on = app_rds_cb.af_cb.af_on;

    memset(&app_rds_cb.af_cb, 0, sizeof(tAPP_FM_AF_CB));

    if ((app_rds_cb.af_cb.af_on = af_on) == TRUE)
        app_fm_set_rds_mode(app_fm_cb.rds_on, af_on);

}

/*******************************************************************************
 **
 ** Function         app_fm_refresh_rssi_threshold
 **
 ** Description     recalculate the rssi threshold for the next scan.
 **
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_fm_refresh_rssi_threshold(void)
{
    BOOLEAN rssi_updated = FALSE;

    APPL_TRACE_DEBUG2("app_fm_refresh_rssi_threshold rssi_threshold %d, NbStationFound %d", \
        app_fm_cb.rssi_threshold, app_fm_cb.nb_station_found);

    if(app_fm_cb.nb_station_found <= APP_FM_RESULT_LOW_WM)
    {
        app_fm_cb.rssi_threshold += APP_FM_RSSI_THRESH_STEP;
        rssi_updated = TRUE;
    }

    if(app_fm_cb.nb_station_found >= APP_FM_RESULT_HIGH_WM)
    {
        app_fm_cb.rssi_threshold -= APP_FM_RSSI_THRESH_STEP;
        rssi_updated = TRUE;
    }

    if(app_fm_cb.rssi_threshold < APP_FM_RSSI_THRESH_MIN)
    {
        app_fm_cb.rssi_threshold = APP_FM_RSSI_THRESH_MIN;
    }

    if(app_fm_cb.rssi_threshold > APP_FM_RSSI_THRESH_MAX)
    {
        app_fm_cb.rssi_threshold = APP_FM_RSSI_THRESH_MAX;
    }

    APPL_TRACE_DEBUG1("btapp fm_refresh_rssi_threshold new rssi_threshold %d", app_fm_cb.rssi_threshold);

    return rssi_updated;
}



/*******************************************************************************
 **
 ** Function         app_fm_cback
 **
 ** Description      process the events from FM
 **
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_cback(tBSA_FM_EVT event, tBSA_FM *p_data)
{
    BOOLEAN rds_change = FALSE;
    tBSA_FM_MSGID_SRCH_CMD_REQ req;

#if APP_FM_DEBUG==TRUE
    APPL_TRACE_EVENT1("app_fm_cback event:%d", event );
#endif
    switch(event)
    {
    case BSA_FM_ENABLE_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_EVENT1("[FM] BSA_FM_ENABLE_EVT: status= %s", app_fm_status[p_data->status]);
#endif
        if (p_data->status == BSA_FM_OK)
        {
            app_fm_cb.enabled = TRUE;
        }

        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_DISABLE_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("[FM] BSA_FM_DISABLE_EVT status= %s", app_fm_status[p_data->status]);
#endif
        if (p_data->status == BSA_FM_OK)
        {
            app_fm_cb.enabled = FALSE;
        }
        if (app_fm_cb.app_fm_cb)
            app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_TUNE_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_EVENT2("[FM] BSA_FM_TUNE_EVT status= %s, frequency %d",
                          app_fm_status[p_data->status], p_data->chnl_info.freq*10);
#endif

        app_fm_clear_af_info();

        if (BSA_FM_OK == p_data->status)
            app_fm_cb.last_freq = p_data->chnl_info.freq;

        if (app_fm_cb.rds_on)
        {
            BSA_FmRDSResetDecoder(0);
        }

        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_SEARCH_CMPL_EVT:
#if APP_FM_DEBUG==TRUE
    if (BSA_FM_FAST_SCAN ==  app_fm_cb.scn_mode)
    {
            APPL_TRACE_DEBUG3("[FM] BSA_FM_SEARCH_CMPL_EVT: Total Channel found: %d STOP AT: [%d] status = [%s]!",
                              app_fm_cb.nb_station_found, p_data->chnl_info.freq, app_fm_status[p_data->chnl_info.status]);
    }
    else
    {
            APPL_TRACE_DEBUG2("[FM] BSA_FM_SEARCH_CMPL_EVT:  STOP AT: [%d] status = [%s]!", p_data->chnl_info.freq, app_fm_status[p_data->chnl_info.status]);
    }
#endif

    if ((BSA_FM_FAST_SCAN ==  app_fm_cb.scn_mode) && (p_data->status  != BSA_FM_SCAN_ABORT))
    {
        if (app_fm_refresh_rssi_threshold())
        {
            BSA_FmSearchFreqInit(&req);
            app_fm_cb.nb_station_found = 0;
            req.scn_mode = app_fm_cb.scn_mode;
            req.rssi_thresh =  app_fm_cb.rssi_threshold;
            req.p_rds_cond = NULL;
            BSA_FmSearchFreq(&req);
        }
        else
        {
            app_fm_cb.cur_freq = p_data->chnl_info.freq;
            app_fm_cb.scn_mode = BSA_FM_SCAN_NONE;
            app_fm_cb.app_fm_cb(event, p_data);
        }
    }
    else
    {

        app_fm_cb.scn_mode = BSA_FM_SCAN_NONE;
        app_fm_cb.app_fm_cb(event, p_data);
    }

#if 0
        if (btui_fm_cb.rds_on)
        {
            BSA_FmRDSResetDecoder(BTUI_RDS_APP_ID);
        }
#endif
        break;

    case BSA_FM_SEARCH_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_EVENT2("[FM] BSA_FM_SEARCH_EVT  scan rssi = %d, freq = %d", p_data->scan_data.rssi, p_data->scan_data.freq);
#endif
        if ((app_fm_cb.scn_mode != BSA_FM_SCAN_DOWN) && (app_fm_cb.scn_mode != BSA_FM_SCAN_UP))
        {
            app_fm_cb.nb_station_found ++;
        }
        else
        {
            app_fm_cb.cur_freq = p_data->scan_data.freq;
        }

        app_fm_clear_af_info();
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_AUD_MODE_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_EVENT2("[FM] Audio mode evt - Status= %s, audio mode %s", app_fm_status[p_data->status], app_fm_am[p_data->mode_info.audio_mode]);
#endif
        app_fm_cb.mode = p_data->mode_info.audio_mode;
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_AUD_DATA_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_EVENT4("[FM] BSA_FM_AUD_DATA_EVT Audio Data Live Updating: rssi = %d stereo [%s] blend [%s] mono [%s]",
                          p_data->audio_data.rssi,
                          (p_data->audio_data.audio_mode & BSA_FM_STEREO_ACTIVE) ? "ON" : "OFF",
                          (p_data->audio_data.audio_mode & BSA_FM_BLEND_ACTIVE) ? "ON" : "OFF",
                          (p_data->audio_data.audio_mode & BSA_FM_MONO_ACTIVE) ? "ON" : "OFF");
#endif
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_AUD_PATH_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_EVENT2("[FM] BSA_FM_AUD_PATH_EVT status= %s, freq= %.1f ",
                          app_fm_status[p_data->status], APP_FM_CVT_FREQ(app_fm_cb.cur_freq) );
#endif
        if (BSA_FM_OK == p_data->status)
        {
            if (BSA_FM_SCO_OFF == app_fm_cb.sco_on)
                app_fm_cb.sco_on = BSA_FM_SCO_ON;
            else
                app_fm_cb.sco_on = BSA_FM_SCO_OFF;

            if (app_fm_cb.cur_freq >= APP_FM_MIN_FREQ && app_fm_cb.cur_freq <= APP_FM_MAX_FREQ)
                app_fm_tune_freq(app_fm_cb.cur_freq);
        }
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_RDS_MODE_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_EVENT3("[FM] BSA_FM_RDS_MODE_EVT status=%s, af_on=%2.2d, rds_on=%2.2d",
                          app_fm_status[p_data->status], p_data->rds_mode.af_on,p_data->rds_mode.rds_on );
#endif
        app_rds_cb.af_cb.af_on  = p_data->rds_mode.af_on;
        if (app_fm_cb.rds_on != p_data->rds_mode.rds_on)
        {
            rds_change = TRUE;
            app_fm_cb.rds_on       = p_data->rds_mode.rds_on;
            app_rds_cb.ps_on		= app_rds_cb.pty_on = app_rds_cb.ptyn_on = app_rds_cb.rt_on = FALSE;
        }
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_SET_DEEMPH_EVT:
        APPL_TRACE_EVENT2("BSA_FM_SET_DEEMPH_EVT status = %d deemphasis = %s",
                          p_data->deemphasis.status, (p_data->deemphasis.time_const == 0)?"50 us" :"75 us");
        break;

    case BSA_FM_MUTE_AUD_EVT:
        APPL_TRACE_EVENT2("BSA_FM_MUTE_AUD_EVT status = %d is_mute = %d",
                          p_data->mute_stat.status, p_data->mute_stat.is_mute);
        if (p_data->mute_stat.status == BSA_FM_OK)
            app_fm_cb.audio_muted = p_data->mute_stat.is_mute;
        app_fm_cb.app_fm_cb(event, p_data);
        break;
    case BSA_FM_RDS_TYPE_EVT:
        APPL_TRACE_EVENT0("BSA_FM_RDS_TYPE_EVT");
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_VOLUME_EVT:
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_FM_SET_REGION_EVT:
        app_fm_cb.app_fm_cb(event, p_data);
        break;

    default:
        APPL_TRACE_WARNING1("[FM] app_fm_cback::Unknown event:x%d", event );
    }
}

/*******************************************************************************
 **
 ** Function         app_fm_rdsp_cback
 **
 ** Description      RDS decoder call back function.
 **
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_rds_cback(UINT16 event, UINT8*p_data)
{
#if APP_FM_DEBUG==TRUE
    APPL_TRACE_EVENT1("[FM] app_fm_hdl_rds_evt(%d)", event );
#endif

    switch(event)
    {
    case BSA_RDS_PI_EVT:
        app_rds_cb.af_cb.pi_code = *(UINT16 *)p_data;
        break;

    case BSA_RDS_AF_EVT:
        app_rds_cb.af_cb.af_avail = TRUE;
        memcpy(&app_rds_cb.af_cb.af_list, (tBSA_FM_AF_LIST *)p_data, sizeof(tBSA_FM_AF_LIST));
        /* when a new AF arrives, update AF list by calling set RDS mode */
        if (app_rds_cb.af_cb.af_on)
            app_fm_set_rds_mode(TRUE, TRUE);

        break;

    case BSA_RDS_PS_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("**************BSA_RDS_PS_EVT PS [%s]", (char*)p_data);
#endif
        app_rds_cb.ps_on = TRUE;
        //app_rds_cb.prg_service = p_data->p_data;
        break;

    case BSA_RDS_TP_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("**************BSA_RDS_TP_EVT TA_TP [%02x]", *(UINT8*)p_data);
#endif
      //  app_fm_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_PTY_EVT:
        app_rds_cb.pty_on = TRUE;
        //app_rds_cb.prg_type = p_data->p_data;
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("**************BSA_RDS_PTY_EVT PTY [%02x]", p_data);
#endif
        app_rds_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_PTYN_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("**************BSA_RDS_TP_EVT PTYN [%s]", (char*)p_data);
#endif
        app_rds_cb.ptyn_on = TRUE;
//        app_rds_cb.prg_type_name = p_data->p_data;
        app_rds_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_RT_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("**************BSA_RDS_RT_EVT [%s]", (char*)p_data);
#endif
        app_rds_cb.rt_on = TRUE;
  //      app_rds_cb.radio_txt = p_data->p_data;
        app_rds_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_DI_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("**************BSA_RDS_DI_EVT [0x%02x]", p_data);
#endif
        break;

    case BSA_RDS_CT_EVT:
#if APP_FM_DEBUG==TRUE
    {
        tBSA_RDS_CT *pct = (tBSA_RDS_CT*)p_data;
        APPL_TRACE_DEBUG4("**************BSA_RDS_CT_EVT day [%d] hour[0x%02x] minute[0x%02x] offset = %02x",
                          pct->day, pct->hour, pct->minute, pct->offset);
    }
#endif
        break;

    case BSA_RDS_PIN_EVT:
#if APP_FM_DEBUG==TRUE
    {
        tBSA_RDS_PIN *ppin = (tBSA_RDS_PIN*)p_data;
        APPL_TRACE_DEBUG3("**************BSA_RDS_PIN_EVT day [%02x] hour[0x%02x] minute[0x%02x] ",
                          ppin->day, ppin->hour, ppin->minute);
    }
#endif
        break;

    case BSA_RDS_MS_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG1("**************BSA_RDS_MS_EVT M_S on [%s]",
                          (*((tBSA_RDS_M_S*)p_data) == BSA_RDS_SPEECH )? "Speech" : "Music");
#endif
        break;

    case BSA_RDS_TDC_EVT:
#if APP_FM_DEBUG==TRUE
    {
        tBSA_RDS_TDC_DATA * ptdc = (tBSA_RDS_TDC_DATA *)p_data;
        APPL_TRACE_DEBUG2("***************BSA_RDS_TDC_EVT: channel number = %02x tdc length = %02x", ptdc->chnl_num, ptdc->len);
        APPL_TRACE_DEBUG4("TDC: data = [%02x] [%02x] [%02x] [%02x]", ptdc->tdc_seg[0],
                          ptdc->tdc_seg[1], ptdc->tdc_seg[2], ptdc->tdc_seg[3]);
    }
#endif
        app_rds_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_ODA_EVT:
        app_rds_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_SLC_EVT:
#if APP_FM_DEBUG==TRUE
    {
        tBSA_RDS_SLC_DATA * pslc = (tBSA_RDS_SLC_DATA*)p_data;
        APPL_TRACE_DEBUG1("***************BSA_RDS_SLC_EVT: slc code = %02x ", pslc->slc_type);
    }
#endif
        app_rds_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_EWS_EVT:
#if APP_FM_DEBUG==TRUE
    {
        tBSA_RDS_SEG_DATA * pseg = (tBSA_RDS_SEG_DATA * )p_data;
        APPL_TRACE_DEBUG3("***************BSA_RDS_EWS_EVT 5bit= %02x 16bit = %04x 16bits 2 = %04x",
                          pseg->data_5b, pseg->data_16b_1, pseg->  data_16b_2);
    }
#endif
        app_rds_cb.app_fm_cb(event, p_data);
        break;

    case BSA_RDS_STATS_EVT:
#if APP_FM_DEBUG==TRUE
        APPL_TRACE_DEBUG3("***************BSA_RDS_STATS_EVT");
#endif
        break;

    default:
        APPL_TRACE_WARNING1("[FM] app_fm_rdsp_cback(Unknown event:x%d)", event );
        break;
    }
    app_rds_cb.app_fm_cb(event, p_data);

}

#undef APP_FM_C

