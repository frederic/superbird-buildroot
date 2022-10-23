/*****************************************************************************
 **
 **  Name:           app_pbs.h
 **
 **  Description:    Bluetooth PBAP application for BSA
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#ifndef __APP_PBS_H__
#define __APP_PBS_H__

extern tBSA_PBS_ACCESS_TYPE pbap_access_flag;

/*******************************************************************************
 **
 ** Function         app_pbs_cback
 **
 ** Description      Example of PBS callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_pbs_cback(tBSA_PBS_EVT event, tBSA_PBS_MSG *p_data);

/*******************************************************************************
 **
 ** Function         app_stop_pbs
 **
 ** Description      Example of function to start PBAP server application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_stop_pbs(void);

/*******************************************************************************
 **
 ** Function         app_start_pbs
 **
 ** Description      Example of function to start PBAP server application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_start_pbs(void);

#ifdef  __cplusplus
extern "C"
{
#endif


#ifdef  __cplusplus
}
#endif

#endif /* __APP_PBS_H__ */
