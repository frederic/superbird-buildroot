/*****************************************************************************
**
**  Name:           app_nsa.h
**
**  Description:    Bluetooth NSA include file
**
**  Copyright (c) 2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_NSA_H
#define APP_NSA_H

#include "bsa_api.h"

/*
 * Definitions
 */

#ifndef APP_NSA_IF_MAX
#define APP_NSA_IF_MAX  1   /* Maximum number of NSA Interface */
#endif

typedef struct
{
    BOOLEAN         in_use;         /* Interface allocated ? */
    BD_ADDR         bd_addr;        /* BdAddr of the HID device */
    UINT8           out_report_id;  /* ReportId used to encapsulate NCI Tx packet */
    UINT8           in_report_id;   /* ReportId used to encapsulate NCI Rx packet */
    tBSA_NSA_PORT   port;           /* NSA port allocated */
} tAPP_NSA_IF;

typedef struct
{
    tAPP_NSA_IF interface[APP_NSA_IF_MAX];
} tAPP_NSA_CB;

/*
 * Global Variables
 */

extern tAPP_NSA_CB app_nsa_cb;

/*******************************************************************************
 **
 ** Function         app_nsa_init
 **
 ** Description      NSA initialization function
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_nsa_init(void);

/*******************************************************************************
 **
 ** Function        app_nsa_exit
 **
 ** Description     This is the exit function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_nsa_exit(void);

/*******************************************************************************
 **
 ** Function         app_nsa_add_if
 **
 ** Description      Add NSA Interface
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_nsa_add_if(void);

/*******************************************************************************
 **
 ** Function         app_nsa_remove_if
 **
 ** Description      Remove NSA Interface
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_nsa_remove_if(void);

#endif /* APP_NSA_H */
