/*****************************************************************************
 **
 **  Name:           app_hd.h
 **
 **  Description:    Bluetooth HID Device application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef __APP_HD_H__
#define __APP_HD_H__

/* for BSA functions and types */
#include "bsa_api.h"
/* for tAPP_XML_REM_DEVICE */
#include "app_xml_param.h"

/*
 * Definitions
 */

/* APP HH callback functions */
typedef BOOLEAN (tAPP_HD_EVENT_CBACK)(tBSA_HD_EVT event, tBSA_HD_MSG *p_data);


typedef struct
{
    tBSA_SEC_AUTH sec_mask_in;
    tBSA_SEC_AUTH sec_mask_out;
    tAPP_HD_EVENT_CBACK *p_event_cback;
} tAPP_HD_CB;

/*
 * Global variables
 */
extern tAPP_HD_CB app_hd_cb;

/*
 * Interface functions
 */
/*******************************************************************************
 **
 ** Function         app_hd_cback
 **
 ** Description      Example of call back function of HID Device application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hd_cback(tBSA_HD_EVT event, tBSA_HD_MSG *p_data);

/*******************************************************************************
 **
 ** Function         app_hd_start
 **
 ** Description      Start HID Device
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_start(void);

/*******************************************************************************
 **
 ** Function         app_hd_listen
 **
 ** Description      Listen HID Host
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_listen(void);

/*******************************************************************************
 **
 ** Function         app_hd_connect
 **
 ** Description      Example of function to connect to HID host
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_connect(void);


/*******************************************************************************
 **
 ** Function         app_hd_disconnect
 **
 ** Description      Example of HD Close
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_disconnect(void);


/*******************************************************************************
 **
 ** Function        app_hd_send_regular_key
 **
 ** Description     Example of HD Send Regular key
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_regular_key(void);


/*******************************************************************************
 **
 ** Function        app_hd_send_special_key
 **
 ** Description     Example of HD Send Special key
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_special_key(void);


/*******************************************************************************
 **
 ** Function        app_hd_send_mouse_data
 **
 ** Description     Example of HD Send mouse data
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_mouse_data(void);


/*******************************************************************************
 **
 ** Function        app_hd_send_customer_data
 **
 ** Description     Example of HD Send customer data
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hd_send_customer_data(void);


/*******************************************************************************
 **
 ** Function         app_hd_disable
 **
 ** Description      Disable HID Device
 **
 ** Parameters
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hd_disable(void);


/*******************************************************************************
 **
 ** Function         app_hd_init
 **
 ** Description      Initialize HD
 **
 ** Parameters       boot: indicate if the management path must be created
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_init(void);


/*******************************************************************************
 **
 ** Function         app_hd_exit
 **
 ** Description      Exit HD application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_hd_exit(void);


/*******************************************************************************
** Function         app_hd_set_cod
**
** Description      Sets the COD to HID device
**
** Parameters
**
** Returns          status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_hd_set_cod();

#endif
