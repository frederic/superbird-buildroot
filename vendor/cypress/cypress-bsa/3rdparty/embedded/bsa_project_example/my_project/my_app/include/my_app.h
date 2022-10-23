/*****************************************************************************
**
**  Name:           my_app.h
**
**  Description:    Bluetooth Composite Application include file
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef MY_APP_H
#define MY_APP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "gki.h"
#include "uipc.h"
#include "bsa_api.h"
#include "bsa_trace.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_sec.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_link.h"
#include "app_manager.h"
#include "app_services.h"
#include "app_mgt.h"
#include "app_av.h"
#include "app_avk.h"
#include "app_dm.h"
#include "app_hh.h"
#include "app_hl.h"
#include "app_dg.h"
#include "app_tm.h"
#include "app_tm_vse.h"
#include "app_thread.h"
#include "app_headless.h"
#ifdef APP_BLE_INCLUDED
#include "app_ble.h"
#include "app_ble_client.h"
#include "app_ble_server.h"
#endif
#include "app_xml_param.h"


/*
 * Global Variables
 */

/*******************************************************************************
 **
 ** Function         my_app_init
 **
 ** Description      BSA Application initialization function
 **
 ** Returns          int
 **
 *******************************************************************************/
int my_app_init(void);

/*******************************************************************************
 **
 ** Function         my_app_start
 **
 ** Description      BSA Application function
 **
 ** Returns          int
 **
 *******************************************************************************/
int my_app_start(void);

/*******************************************************************************
 **
 ** Function        my_app_exit
 **
 ** Description     BSA Application exit function
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int my_app_exit(void);

/*******************************************************************************
 **
 ** Function        my_app_mgt_callback
 **
 ** Description     This callback function is called in case of server
 **                 disconnection (e.g. server crashes)
 **
 ** Parameters      event: Management event
 **                 p_data: parameters
 **
 ** Returns         BOOLEAN: True if the generic callback must return (do not handle the event)
 **                          False if the generic callback must handle the event
 **
 *******************************************************************************/
BOOLEAN my_app_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data);

#endif

