/*****************************************************************************
 **
 **  Name:           app_opc.h
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/* idempotency */
#ifndef APP_OPC_H
#define APP_OPC_H

/* self-sufficiency */
#include "bsa_status.h"

/*******************************************************************************
 **
 ** Function         app_opc_start
 **
 ** Description      Example of function to start OPP Client application
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_start(void);

/*******************************************************************************
 **
 ** Function         app_opc_stop
 **
 ** Description      Example of function to sttop OPP Server application
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_stop(void);

/*******************************************************************************
 **
 ** Function         app_opc_disconnect
 **
 ** Description      Example of function to disconnect current device
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_disconnect(void);

/*******************************************************************************
 **
 ** Function         app_opc_push_vc
 **
 ** Description      Example of function to push a v card to current peer device
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_push_vc(void);

/*******************************************************************************
 **
 ** Function         app_opc_push_file
 **
 ** Description      Example of function to push a file to current peer device
 **
 ** Parameters      file name
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_push_file(char * p_file_name);

/*******************************************************************************
 **
 ** Function         app_opc_exchange_vc
 **
 ** Description      Example of function to exhcange a vcard with current peer device
 **
 ** Parameters      file name
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_exchange_vc(void);

/*******************************************************************************
 **
 ** Function         app_opc_pull_vc
 **
 ** Description      Example of function to exchange a vcard with current peer device
 **
 ** Parameters      void
 **
 ** Returns          BSA_SUCCESS success error code for failure
 **
 *******************************************************************************/
tBSA_STATUS app_opc_pull_vc(void);


#endif

