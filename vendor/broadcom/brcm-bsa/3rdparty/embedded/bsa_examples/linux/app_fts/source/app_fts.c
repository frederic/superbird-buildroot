/*****************************************************************************
 **
 **  Name:           app_fts.c
 **
 **  Description:    Bluetooth File Transfer Server application
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
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

#include "app_fts.h"

#include "app_mgt.h"
#include "app_utils.h"
#include "app_xml_param.h"

/*
 * Defines
 */

/*
 * Global Variables
 */

/*******************************************************************************
 **
 ** Function         app_fts_access
 **
 ** Description      Respond to an Access Request
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_fts_access(tBSA_FTS_MSG *p_data)
{
    tBSA_FTS_ACCESS access_cmd;

    if (p_data->access.resp_required)
    {
        fprintf(stdout, "app_fts_access - accepted evt\n");
        BSA_FtsAccessInit(&access_cmd);
        access_cmd.oper = p_data->access.oper;
        access_cmd.access = BSA_FT_ACCESS_ALLOW;
        access_cmd.access_mask = 0;

        strncpy(access_cmd.p_name, p_data->access.name, sizeof(access_cmd.p_name)-1);

        printf("%s\n", access_cmd.p_name);

        BSA_FtsAccess(&access_cmd);
    }
    else
    {
        fprintf(stdout, "app_fts_access - autoaccepted evt\n");
    }
}

/*******************************************************************************
 *
 **
 ** Function         app_fts_auto_access
 **
 ** Description      Set the auto access feature
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_fts_auto_access(BOOLEAN enabled)
{
    tBSA_FTS_ACCESS access_cmd;

    BSA_FtsAccessInit(&access_cmd);

    if (enabled)
    {
        fprintf(stdout, "app_fts_auto_access - enabled\n");
        access_cmd.access_mask = 0xFF;
        BSA_FtsAccess(&access_cmd);
    }
    else
    {
        fprintf(stdout, "app_fts_auto_access - disabled\n");
        access_cmd.access_mask = 0;
        BSA_FtsAccess(&access_cmd);
    }

}

/*******************************************************************************
 **
 ** Function         app_fts_auth
 **
 ** Description      Respond to an Authorization Request
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_fts_auth(tBSA_FTS_MSG *p_data)
{
    tBSA_FTS_AUTH_RSP auth_cmd;

    BSA_FtsAuthRspInit(&auth_cmd);

    strncpy(auth_cmd.password, "0000", sizeof(auth_cmd.password)-1);
    strncpy(auth_cmd.userid, (char *) p_data->auth.userid, sizeof(auth_cmd.userid)-1);

    BSA_FtsAuthRsp(&auth_cmd);

}

/*******************************************************************************
 **
 ** Function         app_fts_cback
 **
 ** Description      Example of FTS callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_fts_cback(tBSA_FTS_EVT event, tBSA_FTS_MSG *p_data)
{
    fprintf(stderr, "app_fts_cback  event:%d\n", event);
    switch (event)
    {

    case BSA_FTS_OPEN_EVT: /* Connection Open (for info) */
        fprintf(stdout, "BSA_FTS_OPEN_EVT:Remote bdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
                p_data->bd_addr[0], p_data->bd_addr[1], p_data->bd_addr[2], p_data->bd_addr[3],
                p_data->bd_addr[4], p_data->bd_addr[5]);
        break;

    case BSA_FTS_CLOSE_EVT: /* Connection Closed (for info)*/
        fprintf(stdout, "BSA_FTS_CLOSE_EVT\n");
        break;

    case BSA_FTS_AUTH_EVT: /* Authorization */
        fprintf(stdout, "BSA_FTS_AUTH_EVT\n");
        app_fts_auth(p_data);
        break;

    case BSA_FTS_ACCESS_EVT: /* Access requested Event */
        fprintf(stdout, "BSA_FTS_ACCESS_EVT\n");
        app_fts_access(p_data);
        break;

    case BSA_FTS_PROGRESS_EVT: /* Access requested Event */
        fprintf(stdout, "BSA_FTS_PROGRESS_EVT\n");
        break;

    case BSA_FTS_PUT_CMPL_EVT: /* Access requested Event */
        fprintf(stdout, "FTS_PUT_CMPL_EVT\n");
        break;

    case BSA_FTS_GET_CMPL_EVT: /* Access requested Event */
        fprintf(stdout, "BSA_FTS_GET_CMPL_EVT\n");
        break;

    case BSA_FTS_DEL_CMPL_EVT: /* Access requested Event */
        fprintf(stdout, "BSA_FTS_DEL_CMPL_EVT\n");
        break;

    default:
        fprintf(stderr, "app_fts_cback unknown event:%d\n", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_start_fts
 **
 ** Description      Example of function to start File Transfer server application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_start_fts(char * p_root_path)
{
    tBSA_STATUS status = BSA_SUCCESS;
    tBSA_FTS_ENABLE enable_param;

    fprintf(stdout, "app_start_fts\n");

    /* prepare parameters */
    BSA_FtsEnableInit(&enable_param);

    enable_param.sec_mask = BSA_SEC_NONE;
    strncpy(enable_param.service_name, "FT Server", sizeof(enable_param.service_name)-1);
    if( (p_root_path != NULL) && (strlen(p_root_path) <= sizeof(enable_param.root_path)) )
    {
        strncpy(enable_param.root_path,p_root_path, sizeof(enable_param.root_path)-1);
    }
    else
    {
        strncpy(enable_param.root_path, "./fts_root", sizeof(enable_param.root_path)-1);
    }

    enable_param.enable_authen = FALSE;
    strncpy(enable_param.realm, "guest", sizeof(enable_param.realm)-1);
    enable_param.p_cback = app_fts_cback;

    status = BSA_FtsEnable(&enable_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_start_fts: Unable to enable FTS status:%d\n", status);
    }

    fprintf(stdout, "app_start_fts done\n");

    return status;
}

