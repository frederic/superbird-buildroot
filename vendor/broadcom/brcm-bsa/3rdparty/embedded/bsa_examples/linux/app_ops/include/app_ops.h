/*****************************************************************************
 **
 **  Name:           app_ops.h
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/* idempotency */
#ifndef APP_OPS_H
#define APP_OPS_H

/* self-sufficiency */
#include "bsa_status.h"
#include "bsa_op_api.h"

/*******************************************************************************
 **
 ** Function         app_ops_auto_accept
 **
 ** Description      Example of a function to set the BSA to automatically accept
 **                  an access request
 **
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ops_auto_accept(void);

/*******************************************************************************
 **
 ** Function         app_access_resp_ops
 **
 ** Description      Example of a function to respond to an access request on
 **                  the OPP Server
 **
 ** Parameters      OPS Message
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_access_resp_ops(tBSA_OPS_MSG *ops_access_req_param);

/*******************************************************************************
 **
 ** Function         app_ops_cback
 **
 ** Description      Example of OPS callback function
 **
 ** Parameters      event and message
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ops_cback(tBSA_OPS_EVT event, tBSA_OPS_MSG *p_data);

/*******************************************************************************
 **
 ** Function         app_start_ops
 **
 ** Description      Example of function to start OPP Server application
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_start_ops(void);

/*******************************************************************************
 **
 ** Function         app_stop_ops
 **
 ** Description      Example of function to sttop OPP Server application
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_stop_ops(void);

/*******************************************************************************
 **
 ** Function         app_ops_disconnect
 **
 ** Description      Example of function to disconnect current device
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ops_disconnect(void);

#endif

