/*****************************************************************************
 **
 **  Name:           app_hl_db.c
 **
 **  Description:    Bluetooth Health Database utility functions
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bsa_api.h"

#include "bsa_trace.h"
#include "app_utils.h"
#include "app_hl.h"
#include "app_hl_db.h"
#include "app_hl_xml.h"


/* Structure to hold Database data */
typedef struct
{
    tAPP_HL_APP_CFG app_cfg[APP_HL_LOCAL_APP_MAX];
} tAPP_HL_DB_CB;


tAPP_HL_DB_CB app_hl_db_cb;

/*
 * Local functions
 */
UINT8 app_hl_db_get_app_index(tBSA_HL_APP_HANDLE app_handle, BOOLEAN allocate);
void app_hl_db_clean_up(void);


/*******************************************************************************
 **
 ** Function         app_hl_db_mdl_add
 **
 ** Description      Function used to add one MDL configuration
 **
 ** Parameters       app_handle: Application Handle
 **                  mdl_index: MDL Index
 **                  p_app_cfg: The MDL configuration
 **
 ** Returns          status: -1:error, 0:ok
 **
 **                  note: this function automatically:
 **                        - reads the database from file
 **                        - update it in memory
 **                        - write it back to file
 **
 *******************************************************************************/
int app_hl_db_mdl_add(tBSA_HL_APP_HANDLE app_handle, UINT8 mdl_index,
        tBSA_HL_MDL_CFG *p_mdl_cfg)
{
    tBSA_HL_MDL_CFG *p_mdl_cfg_dst;
    UINT8 app_index;

    APP_DEBUG1("app_handle:%d mdl_index:%d", app_handle, mdl_index);

    /* Check parameters */
    if (mdl_index >= BSA_HL_NUM_MDL_CFGS)
    {
        APP_ERROR1("Bad mdl_index:%d", mdl_index);
        return -1;
    }
    if (p_mdl_cfg == NULL)
    {
        APP_ERROR0("app_hl_db_mdl_add p_mdl_cfg is NULL");
        return -1;
    }

    /* Read the database (to have a fresh view) */
    if (app_hl_xml_read(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to read the XML database");
        return -1;
    }

    /* Get App_index from app_handle. Allocate one if not found */
    app_index = app_hl_db_get_app_index(app_handle, TRUE);

    /* If not found and cannot allocate */
    if (app_index >= APP_HL_LOCAL_APP_MAX)
    {
        APP_ERROR0("Unable allocate new app_handle in XML database");
        return -1;
    }

    /* Get pointer on the MDL CFG to add/update */
    p_mdl_cfg_dst = &app_hl_db_cb.app_cfg[app_index].mdl_cfg[mdl_index];
    if (p_mdl_cfg_dst->active)
    {
        APP_DEBUG1("mdl_index:%d of app_handle:%d was already in use", mdl_index, app_handle);
    }

    /* Copy the MDL CFG in the database */
    memcpy(p_mdl_cfg_dst, p_mdl_cfg, sizeof(*p_mdl_cfg_dst));

    /* clean up database. In no MDL CFG active for an app_handle, free it */
    app_hl_db_clean_up();

    /* Update the database */
    if (app_hl_xml_write(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to write the XML database");
        return -1;
    }

    return -1;
}

/*******************************************************************************
 **
 ** Function         app_hl_db_get_by_app_hdl
 **
 ** Description      Function used to get all MDL configurations for an App Handle
 **
 ** Parameters       app_handle: Application Handle
 **                  p_app_cfg: MDL configuration buffer (table of structures)
 **                  nb_entries: number of element of the table
 **
 ** Returns          status: -1:error
 **                           0:ok but no MDL active
 **                           1:ok and at least one MDL active
 **
 **                  note: this function automatically:
 **                        - reads the database from file
 **                        - return all the MDL config for the matching App Handle
 **
 *******************************************************************************/
int app_hl_db_get_by_app_hdl(tBSA_HL_APP_HANDLE app_handle, tBSA_HL_MDL_CFG *p_mdl_cfg)
{
    tBSA_HL_MDL_CFG *p_mdl_cfg_src;
    UINT8 mdl_index;
    UINT8 app_index;

    APP_DEBUG1("app_handle:%d", app_handle);

    /* Check parameters */
    if (p_mdl_cfg == NULL)
    {
        APP_ERROR0("app_hl_db_mdl_add p_mdl_cfg is NULL");
        return -1;
    }

    /* clear mdl_cfg table  */
    memset(p_mdl_cfg, 0, sizeof(tBSA_HL_MDL_CFG) * BSA_HL_NUM_MDL_CFGS);

    /* Read the database (to have a fresh view) */
    if (app_hl_xml_read(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to read the XML database");
        return -1;
    }

    /* Get App_index matching app_handle. */
    app_index = app_hl_db_get_app_index(app_handle, FALSE);
    if (app_index >= APP_HL_LOCAL_APP_MAX)
    {
        APP_DEBUG1("app_handle:%d not found", app_handle);
        return -1;
    }

    /* Get pointer on the first MDL CFG */
    p_mdl_cfg_src = &app_hl_db_cb.app_cfg[app_index].mdl_cfg[0];

    /* Copy all the MDL CFG from the datebase */
    memcpy(p_mdl_cfg, p_mdl_cfg_src, sizeof(tBSA_HL_MDL_CFG) * BSA_HL_NUM_MDL_CFGS);

    /* Check if at least one MDL Configuration exists */
    for (mdl_index = 0 ; mdl_index < BSA_HL_NUM_MDL_CFGS ; mdl_index++, p_mdl_cfg_src++)
    {
        if (p_mdl_cfg->active)
        {
            APP_DEBUG1("mdl_index:%d is active", mdl_index);
            return 1;
        }
    }
    APP_DEBUG0("no active mdl found");
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_db_mdl_remove_by_index
 **
 ** Description      Function used to delete one MDL for an App Handle
 **
 ** Parameters       app_handle: Application Handle
 **                  mdl_index: MDL Index to delete
 **
 ** Returns          status: -1:error, 0:ok
 **
 **                  note: this function automatically:
 **                        - reads the database
 **                        - update it in memory
 **                        - write it back to file
 **
 *******************************************************************************/
int app_hl_db_mdl_remove_by_index(tBSA_HL_APP_HANDLE app_handle, UINT8 mdl_index)
{
    tBSA_HL_MDL_CFG *p_mdl_cfg;
    UINT8 app_index;

    APP_DEBUG1("app_handle:%d mdl_index:%d", app_handle, mdl_index);

    /* Check parameters */
    if (mdl_index >= BSA_HL_NUM_MDL_CFGS)
    {
        APP_ERROR1("Bad mdl_index:%d", mdl_index);
        return -1;
    }

    /* Read the database (to have a fresh view) */
    if (app_hl_xml_read(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to read the XML database");
        return -1;
    }

    /* Get App_index matching app_handle. */
    app_index = app_hl_db_get_app_index(app_handle, FALSE);
    if (app_index >= APP_HL_LOCAL_APP_MAX)
    {
        APP_DEBUG1("app_handle:%d not found", app_handle);
        return -1;
    }

    /* Get pointer on this MDL CFG */
    p_mdl_cfg = &app_hl_db_cb.app_cfg[app_index].mdl_cfg[mdl_index];

    if (p_mdl_cfg->active == FALSE)
    {
        APP_DEBUG0("This Mdl Index was not active");
    }

    /* clean this MDL CFG in the datebase */
    memset(p_mdl_cfg, 0, sizeof(*p_mdl_cfg));

    /* clean up database. In no MDL CFG active for an app_handle, free it */
    app_hl_db_clean_up();

    /* Update the database */
    if (app_hl_xml_write(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to write the XML database");
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_db_mdl_remove_by_bdaddr
 **
 ** Description      Function used to delete one or all MDL configurations matching
 **                  a BdAddr (for every Application Handles.
 **                  This function can be used when a Peer Device is suppressed
 **
 ** Parameters       bd_addr: BdAddr to erase
 **
 ** Returns          status: -1:error, 0:ok
 **
 **                  note: this function automatically:
 **                        - reads the database
 **                        - update it in memory
 **                        - write it back to file
 **
 *******************************************************************************/
int app_hl_db_mdl_remove_by_bdaddr(BD_ADDR bd_addr)
{
    tBSA_HL_MDL_CFG *p_mdl_cfg;
    UINT8 app_index;
    UINT8 mdl_index;

    APP_DEBUG1("BdAddr:%02X-%02X-%02X-%02X-%02X-%02X",
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    /* Read the database (to have a fresh view) */
    if (app_hl_xml_read(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to read the XML database");
        return -1;
    }

    /* For every Application in device database */
    for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
    {
        /* If this application handle is in use */
        if (app_hl_db_cb.app_cfg[app_index].app_handle != BSA_HL_BAD_APP_HANDLE)
        {
            /* For every MDL of this application */
            p_mdl_cfg = &app_hl_db_cb.app_cfg[app_index].mdl_cfg[0];
            for (mdl_index = 0; mdl_index < BSA_HL_NUM_MDL_CFGS; mdl_index++, p_mdl_cfg++)
            {
                /* If this MDL CFG is in use */
                if (p_mdl_cfg->active)
                {
                    /* If the BdAddr of this MDL CFG matches the BdAddr */
                    if (bdcmp(p_mdl_cfg->peer_bd_addr, bd_addr) == 0)
                    {
                        /* Clear it */
                        APP_DEBUG1("Delete app_index:%d mdl_index:%d", app_index, mdl_index);
                        memset(p_mdl_cfg, 0, sizeof(*p_mdl_cfg));
                    }
                }
            }
        }
    }

    /* clean up database. In no MDL CFG active for an app_handle, free it */
    app_hl_db_clean_up();

    /* Update the database */
    if (app_hl_xml_write(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to write the XML database");
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_db_display
 **
 ** Description      Function used to display all the HL database (debug)
 **
 ** Parameters       None
 **
 ** Returns          status: -1:error, 0:ok
 **
 *******************************************************************************/
int app_hl_db_display(void)
{
    tBSA_HL_MDL_CFG *p_mdl_cfg;
    UINT8 app_index;
    UINT8 mdl_index;

    APP_DEBUG0("");
    /* Read the database (to have a fresh view) */
    if (app_hl_xml_read(app_hl_db_cb.app_cfg) < 0)
    {
        APP_ERROR0("Unable to read the XML database");
        return -1;
    }

    /* For every Application in device database */
    for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
    {
        /* If this application handle is in use */
        if (app_hl_db_cb.app_cfg[app_index].app_handle != BSA_HL_BAD_APP_HANDLE)
        {
            APP_INFO1("app_handle:%d", app_hl_db_cb.app_cfg[app_index].app_handle);

            /* For every MDL of this application */
            p_mdl_cfg = &app_hl_db_cb.app_cfg[app_index].mdl_cfg[0];
            for (mdl_index = 0; mdl_index < BSA_HL_NUM_MDL_CFGS; mdl_index++, p_mdl_cfg++)
            {
                /* If this MDL CFG is in use */
                if (p_mdl_cfg->active)
                {
                    APP_INFO1("\tmdl_id:%d tmld_index:%d BdAddr:%02X-%02X-%02X-%02X-%02X-%02X local_mdep_id:%d local_mdep_role:%d",
                            p_mdl_cfg->mdl_id, mdl_index,
                            p_mdl_cfg->peer_bd_addr[0], p_mdl_cfg->peer_bd_addr[1],
                            p_mdl_cfg->peer_bd_addr[2], p_mdl_cfg->peer_bd_addr[3],
                            p_mdl_cfg->peer_bd_addr[4], p_mdl_cfg->peer_bd_addr[5],
                            p_mdl_cfg->local_mdep_id, p_mdl_cfg->local_mdep_role);
                    APP_INFO1("\t\tdch_mode:%d fcs:%d mtu:%d time:%d",
                            p_mdl_cfg->dch_mode, p_mdl_cfg->fcs,
                            p_mdl_cfg->mtu, p_mdl_cfg->time);
                }
            }
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_hl_db_get_app_index
 **
 ** Description      Function used to get an app_index from an app_handle
 **
 ** Parameters       app_handle: app_handle to look for
 **                  allocate: indicate if one must be allocated if not found
 **
 ** Returns          status: 0:ok APP_HL_LOCAL_APP_MAX:error
 **
 *******************************************************************************/
UINT8 app_hl_db_get_app_index(tBSA_HL_APP_HANDLE app_handle, BOOLEAN allocate)
{
    UINT8 app_index;

    /* look for this app_handle in the database */
    for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
    {
        if (app_hl_db_cb.app_cfg[app_index].app_handle == app_handle)
        {
            return app_index;
        }
    }
    /* If not found */
    if (allocate)
    {
        /* Try to allocate one */
        for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
        {
            if (app_hl_db_cb.app_cfg[app_index].app_handle == BSA_HL_BAD_APP_HANDLE)
            {
                app_hl_db_cb.app_cfg[app_index].app_handle = app_handle;
                return app_index;
            }
        }
    }
    return APP_HL_LOCAL_APP_MAX;
}

/*******************************************************************************
 **
 ** Function         app_hl_db_clean_up
 **
 ** Description      Function used to clean up database.
 **                  In no MDL CFG active for an app_handle, free it
 **
 ** Parameters       none
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_hl_db_clean_up(void)
{
    UINT8 app_index;
    UINT8 mdl_index;
    BOOLEAN one_active;
    tBSA_HL_MDL_CFG *p_mdl_cfg;

    APP_DEBUG0("");

    /* For every Application in device database */
    for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
    {
        /* If this application handle is in use */
        if (app_hl_db_cb.app_cfg[app_index].app_handle != BSA_HL_BAD_APP_HANDLE)
        {
            one_active = FALSE;
            /* For every MDL of this application */
            p_mdl_cfg = &app_hl_db_cb.app_cfg[app_index].mdl_cfg[0];
            for (mdl_index = 0; mdl_index < BSA_HL_NUM_MDL_CFGS; mdl_index++, p_mdl_cfg++)
            {
                /* If this MDL CFG is in use */
                if (p_mdl_cfg->active)
                {
                    one_active = TRUE;
                    break;
                }
            }
            /* If no MDL CFG active => mark this application as unused */
            if (one_active == FALSE)
            {
                app_hl_db_cb.app_cfg[app_index].app_handle = BSA_HL_BAD_APP_HANDLE;
            }
        }
    }
}

