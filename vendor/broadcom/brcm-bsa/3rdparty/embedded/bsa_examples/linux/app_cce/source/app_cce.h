/*****************************************************************************
**
**  Name:           app_cce.h
**
**  Description:    Bluetooth CTN client application
**
**  Copyright (c)   2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef APP_CCE_H_
#define APP_CCE_H_

/*****************************************************************************
**  Constants and Type Definitions
*****************************************************************************/

typedef struct
{
    tBTM_BD_NAME            dev_name;
    BD_ADDR                 bd_addr;
    UINT8                   instance_id;
    UINT8                   cce_handle;
} tAPP_CCE_CONN;

/* Callback to application for CCE events */
typedef void (tCceCallback)(tBSA_CCE_EVT event, tBSA_CCE_MSG *p_data);

/*******************************************************************************
**
** Function         app_cce_enable
**
** Description      Enable CCE functionality
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_enable(tCceCallback *pcb);

/*******************************************************************************
**
** Function         app_cce_disable
**
** Description      Stop CCE functionality
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_disable();

/*******************************************************************************
**
** Function         app_cce_get_accounts
**
** Description      Get all available accounts on a CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_accounts(BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         app_cce_open
**
** Description      Open a connection to a CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_open(BD_ADDR bda, UINT8 instance_id);

/*******************************************************************************
**
** Function         app_cce_close
**
** Description      Close a CTN connection
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_close(UINT8 cce_handle);

/*******************************************************************************
**
** Function         app_cce_close_all
**
** Description      Close all CTN connections
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_close_all();

/*******************************************************************************
**
** Function         app_cce_get_obj_list
**
** Description      Get object listing from the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_obj_list(UINT8 cce_handle, UINT8 obj_type);

/*******************************************************************************
**
** Function         app_cce_get_obj
**
** Description      Get an object from the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_obj(UINT8 cce_handle, char *obj_handle, BOOLEAN recurrent);

/*******************************************************************************
**
** Function         app_cce_set_obj_status
**
** Description      Set status of an object on the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_set_obj_status(UINT8 cce_handle, char *obj_handle,
        UINT8 indicator, UINT8 value, UINT32 postpone_val);

/*******************************************************************************
**
** Function         app_cce_push_obj
**
** Description      Push an object to the CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_push_obj(UINT8 cce_handle, UINT8 obj_type, BOOLEAN send,
        char *file_name);

/*******************************************************************************
**
** Function         app_cce_forward_obj
**
** Description      Forward an object to one or more additional recipients
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_forward_obj(UINT8 cce_handle, char *obj_handle,
        char *recipients);

/*******************************************************************************
**
** Function         app_cce_get_account_info
**
** Description      Get CAS info for the specified CAS instance ID
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_get_account_info(UINT8 cce_handle, UINT8 instance_id);

/*******************************************************************************
**
** Function         app_cce_sync_account
**
** Description      Synchronize CTN server account with an external server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_sync_account(UINT8 cce_handle);

/*******************************************************************************
**
** Function         app_cce_cns_start
**
** Description      Start CTN notification service
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_cns_start(char *service_name);

/*******************************************************************************
**
** Function         app_cce_cns_stop
**
** Description      Stop CTN notification service
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_cns_stop();

/*******************************************************************************
**
** Function         app_cce_notif_reg
**
** Description      Register for notification indicating changes in CTN server
**
** Returns          BSA status
**
*******************************************************************************/
tBSA_STATUS app_cce_notif_reg(UINT8 cce_handle, BOOLEAN notif_on, BOOLEAN alarm_on);

/*******************************************************************************
**
** Function         app_cce_get_connections
**
** Description      Get all open connections
**
** Parameter        p_num : returns number of connections
**
** Returns          Pointer to connections
**
*******************************************************************************/
tAPP_CCE_CONN *app_cce_get_connections(UINT8 *p_num);

#endif /* APP_CCE_H_ */
