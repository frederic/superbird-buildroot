/*****************************************************************************
**
**  Name:           vsc.h
**
**  Description:    Bluetooth VSC functions
**
**  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

/*******************************************************************************
 **
 ** Function         vsc_send_init_param
 **
 ** Description      This function is used to initialize the VSC params
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
extern int vsc_send_init_param(tBSA_TM_VSC *vsc_param);

/*******************************************************************************
 **
 ** Function         vsc_send
 **
 ** Description      This function is used to send a Vendor Specific Command
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
extern int vsc_send(tBSA_TM_VSC *vsc_param);

