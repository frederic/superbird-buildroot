/*****************************************************************************
 **
 **  Name:           app_fts.h
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

#include "app_mgt.h"
#include "app_utils.h"
#include "app_xml_param.h"

#ifndef __APP_FTS_H__
#define __APP_FTS_H__

#ifdef  __cplusplus
extern "C"
{
#endif


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
void app_fts_access(tBSA_FTS_MSG *p_data);

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
void app_fts_auto_access(BOOLEAN enabled);

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
void app_fts_auth(tBSA_FTS_MSG *p_data);


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
void app_fts_cback(tBSA_FTS_EVT event, tBSA_FTS_MSG *p_data);

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
tBSA_STATUS app_start_fts(char * p_root_path);


#ifdef  __cplusplus
}
#endif

#endif /* __APP_FTS_H__ */
