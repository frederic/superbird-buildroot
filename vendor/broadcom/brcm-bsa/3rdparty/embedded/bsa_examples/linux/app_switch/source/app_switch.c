/*****************************************************************************
 **
 **  Name:           app_switch.c
 **
 **  Description:    AV/AG to AVK/HF Switching functions
 **
 **  Copyright (c) 2009-2014 Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
/* for memcpy etc... */
#include <string.h>
/* for open and open parameters */
#include <fcntl.h>
/* for lseek, read and close */
#include <unistd.h>
/* for EINTR */
#include <errno.h>

#include "bsa_api.h"

#include "gki_int.h"
#include "uipc.h"

#include "app_switch.h"
#include "app_xml_param.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_wav.h"
#include "app_playlist.h"
#include "app_mutex.h"
#include "app_thread.h"
#include "app_xml_utils.h"
#include "app_dm.h"

#if (defined(BSA_AG_INCLUDED) && BSA_AG_INCLUDED==TRUE)
#include "app_ag.h"
#endif
#if (defined(BSA_HS_INCLUDED) && BSA_HS_INCLUDED==TRUE)
#include "app_hs.h"
#endif
#if (defined(BSA_AV_INCLUDED) && BSA_AV_INCLUDED==TRUE)
#include "app_av.h"
#endif
#if (defined(BSA_AVK_INCLUDED) && BSA_AVK_INCLUDED==TRUE)
#include "app_avk.h"
#endif


tAPP_SWITCH_CB app_switch_cb;

/*******************************************************************************
 **
 ** Function         app_switch_av_ag_enable
 **
 ** Description
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_av_ag_enable(BOOLEAN bBoot)
{
   app_hs_stop();
   app_avk_end();
   app_av_init(bBoot);
   app_av_register();
   app_ag_init();
   app_ag_start(app_ag_cb.sco_route);
   app_switch_cb.current_role = APP_AV_AG_ROLE;
}
/*******************************************************************************
 **
 ** Function         app_switch_av_ag_connect
 **
 ** Description
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_av_ag_connect(void)
{
   app_ag_open(NULL);
   app_av_open(NULL);
}
/*******************************************************************************
 **
 ** Function         app_switch_avk_hf_enable
 **
 ** Description
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_avk_hf_enable(void)
{
   app_ag_deregister();
   app_ag_disable();
   app_av_end();
   app_hs_init();
   app_hs_enable();
   app_hs_register();
   app_avk_init(NULL);
   app_avk_register();
   app_switch_cb.current_role = APP_AVK_HF_ROLE;
}
/*******************************************************************************
 **
 ** Function         app_avk_hf_connect
 **
 ** Description
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_avk_hf_connect(void)
{
   app_hs_open(NULL);
   app_avk_open();
}
