/*****************************************************************************
 **
 **  Name:           app_tvwakeup.h
 **
 **  Description:    Host WakeUp utility functions for applications
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_TVWAKEUP_H
#define APP_TVWAKEUP_H

/* Self sufficiency */
//#include "data_types.h"
#include "bsa_api.h"
#include "app_utils.h"

/*
 * Definitions
 */

/* Up to 3 Record (per profile) */
#ifndef APP_TVWAKEUP_RECORD_NB_MAX
#define APP_TVWAKEUP_RECORD_NB_MAX         3
#endif

/* Records are up to 10 bytes */
#ifndef APP_TVWAKEUP_RECORD_LEN_MAX
#define APP_TVWAKEUP_RECORD_LEN_MAX        10
#endif

/* Host WakeUp Profile definitions (HID only for the moment) */
typedef enum
{
    APP_TVWAKEUP_PROFILE_ID_HID = 0, /* HID */
} tAPP_TVWAKEUP_PROFILE_ID;


typedef struct
{
    UINT8 len;                  /* Record length */
    /* Record data */
    UINT8 data[APP_TVWAKEUP_RECORD_LEN_MAX];
} tAPP_TVWAKEUP_RECORD;

typedef struct
{
    tAPP_TVWAKEUP_PROFILE_ID id;   /* Id of the Profile */
    UINT8 records_nb;           /* Number of Records */
    /* Records list */
    tAPP_TVWAKEUP_RECORD records[APP_TVWAKEUP_RECORD_NB_MAX];
} tAPP_TVWAKEUP_PROFILE;

/*******************************************************************************
 **
 ** Function        app_tvwakeup_add_send
 **
 ** Description     Send an TV WakeUp Add Device command to the local BT Chip
 **
 ** Parameters      TV WakeUp configuration for a given peer device
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_tvwakeup_add_send(BD_ADDR bdaddr, LINK_KEY link_key,
        UINT8 *p_class_of_device, tAPP_TVWAKEUP_PROFILE *p_profile,
        UINT8 profile_nb);

/*******************************************************************************
 **
 ** Function        app_tvwakeup_remove_send
 **
 ** Description     Send an TV WakeUp Remove Device command to the local BT Chip
 **
 ** Parameters      BT Address of the peer device to remove
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_tvwakeup_remove_send(BD_ADDR bdaddr);

/*******************************************************************************
 **
 ** Function        app_tvwakeup_clear_send
 **
 ** Description     Send an TV WakeUp Clear Device command to the local BT Chip
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_tvwakeup_clear_send(void);

#endif /* APP_TVWAKEUP_H */
