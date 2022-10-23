/*****************************************************************************
**
**  Name:           my_app_menu.h
**
**  Description:    Bluetooth Composite Application Menu include file
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef MY_APP_MAIN_H
#define MY_APP_MAIN_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "gki.h"
#include "uipc.h"
#include "bsa_api.h"
#include "bsa_trace.h"
#include "app_xml_param.h"

#include "app_disc.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_sec.h"
#include "app_dm.h"
#include "app_3d.h"
#include "app_3dtv.h"

#include "app_av.h"
#include "app_avk.h"
#include "app_mgt.h"
#include "app_hh.h"
#include "app_hl.h"
#include "app_dg.h"
#include "app_tm.h"
#include "app_tm_vse.h"
#include "app_tm_vsc.h"
#include "app_headless.h"
#include "my_app.h"
#ifdef APP_BLE_INCLUDED
#include "app_ble.h"
#include "app_ble_client.h"
#include "app_ble_server.h"
#endif

/*
 * Global Variables
 */


/*
 * Function declaration
 */

#endif

