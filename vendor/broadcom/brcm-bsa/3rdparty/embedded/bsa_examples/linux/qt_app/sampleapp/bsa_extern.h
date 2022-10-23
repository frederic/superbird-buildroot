#ifndef BSA_EXTERN_H
#define BSA_EXTERN_H


extern "C"
{
#include "gki.h"
#include "uipc.h"
#include "bsa_api.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_dm.h"
#include "app_services.h"
#include "app_mgt.h"
#include "app_avk.h"
#include "app_hs.h"
#include "app_pbc.h"
#include "app_mce.h"
#include "app_av.h"
#include "app_ag.h"

    /*
    * Global variables
    */

    BD_ADDR                 app_sec_db_addr;    /* BdAddr of peer device requesting SP */


    typedef void (tSecurity_callback)(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data);
    tAPP_XML_REM_DEVICE app_xml_remote_devices_db[APP_MAX_NB_REMOTE_STORED_DEVICES];

    int app_mgr_open(char *uipc_path);
    int app_mgr_config(tSecurity_callback cb);
    void app_mgr_close(void);

    tBSA_HS_CONN_CB *app_hs_get_default_conn();
    int app_mgr_sp_cfm_reply(BOOLEAN accept, BD_ADDR bd_addr);

    typedef struct
    {
        int nb_devices;
        tBSA_DISC_CBACK *p_disc_cback;
    }tAPP_DISC_CB;

    tAPP_DISC_CB app_disc_cb;

    typedef void ( tAvkCallback)(tBSA_AVK_EVT event, tBSA_AVK_MSG *p_data);
    typedef void ( tAvCallback)(tBSA_AV_EVT event, tBSA_AV_MSG *p_data);

    tAPP_AV_CONNECTION *app_av_find_connection_by_handle(tBSA_AV_HNDL handle);
    char *app_av_get_notification_string(UINT8 command);
    BOOLEAN app_av_is_event_registered(UINT8 event_id);
    void app_av_set_cback(tAvCallback pcb);
    int app_init_playlist(char *path);
}

#endif // BSA_EXTERN_H
