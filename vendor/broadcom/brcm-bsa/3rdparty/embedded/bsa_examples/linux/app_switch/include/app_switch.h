/*****************************************************************************
 **
 **  Name:           app_switch.h
 **
 **  Description:    Switching between  av-ag to avk-hf
 **
 **  Copyright (c) 2009-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/* idempotency */
#ifndef APP_SWITCH_H_
#define APP_SWITCH_H_

/* for timespec */
#include <time.h>

#include "bsa_av_api.h"
#include "bsa_avk_api.h"
#include "bsa_hs_api.h"
#include "bsa_ag_api.h"

/* for various types */
#include "data_types.h"

/* role */
typedef enum
{
    APP_ILDE_MODE = 0,  /* Idle mode */
    APP_AV_AG_ROLE,     /* AV, AG Role */
    APP_AVK_HF_ROLE     /* AVK, HF Role */
} tAPP_SWITCH_DEVICE_ROLE;

typedef struct
{
    tAPP_SWITCH_DEVICE_ROLE current_role;
} tAPP_SWITCH_CB;

/*
 * Global Variables
 */
extern tAPP_SWITCH_CB app_switch_cb;

/*******************************************************************************
 **
 ** Function         app_switch_av_ag_enable
 **
 ** Description      Enables av, ag and disables avk, Hf
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_av_ag_enable(BOOLEAN bBoot);

/*******************************************************************************
 **
 ** Function         app_switch_av_ag_connect
 **
 ** Description      Connects to avk, hf device
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_av_ag_connect(void);

/*******************************************************************************
 **
 ** Function         app_switch_avk_hf_enable
 **
 ** Description      Enables Avk, Hf and disables Av source, Ag .
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_avk_hf_enable(void);

/*******************************************************************************
 **
 ** Function         app_switch_avk_hf_connect
 **
 ** Description      Connects to Av source and Ag .
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_switch_avk_hf_connect(void);

#endif
