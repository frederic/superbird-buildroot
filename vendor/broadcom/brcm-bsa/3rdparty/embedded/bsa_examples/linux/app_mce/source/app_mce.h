/*****************************************************************************
**
**  Name:           app_mce.h
**
**  Description:    Bluetooth Message Access Profile client application
**
**  Copyright (c)   2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef APP_MCE_H_
#define APP_MCE_H_

/* Callback to application for connection events */
typedef void (tMceCallback)(tBSA_MCE_EVT event, tBSA_MCE_MSG *p_data);

/*******************************************************************************
**
** Function         app_mce_enable
**
** Description      Example of function to enable MCE functionality
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mce_enable(tMceCallback pcb);

/*******************************************************************************
**
** Function         app_mce_disable
**
** Description      Example of function to stop MCE functionality
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_disable(void);

/*******************************************************************************
**
** Function         app_mce_abort
**
** Description      Example of function to abort current operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_abort(UINT8 instance_id);

/*******************************************************************************
**
** Function         app_mce_open
**
** Description      Example of function to open up a connection to peer
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_open(BD_ADDR bda, UINT8 instance_id);

/*******************************************************************************
**
** Function         app_mce_close
**
** Description      Function to close MCE connection
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_close(UINT8 instance_id);

/*******************************************************************************
**
** Function         app_mce_close_all
**
** Description      Function to close all MCE connections
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_close_all();

/*******************************************************************************
**
** Function         app_mce_cancel
**
** Description      cancel connections
**
** Returns          0 if success -1 if failure
*******************************************************************************/
int app_mce_cancel(UINT8 instance_id);

/*******************************************************************************
**
** Function         app_mce_get_mas_instances
**
** Description      Example of function to get all available MAS instances on server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_mas_instances(BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         app_mce_get_mas_ins_info
**
** Description      Get MAS info for the specified MAS instance id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_mas_ins_info(UINT8 instance_id);

/*******************************************************************************
**
** Function         app_mce_get_msg
**
** Description      Get the message for specified handle from MAS server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_get_msg(UINT8 instance_id, const char* phandlestr, BOOLEAN attachment);


/*******************************************************************************
**
** Function         app_mce_push_msg
**
** Description      Example of function to push a message to MAS
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_mce_push_msg(UINT8 instance_id, const char* folder);

/*******************************************************************************
**
** Function         app_mce_get_msg_list
**
** Description      Get message listing from the specified MAS server
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_get_msg_list(UINT8 instance_id, const char* pfolder,
         tBSA_MCE_MSG_LIST_FILTER_PARAM* p_filter_param);


/*******************************************************************************
**
** Function         app_mce_mn_start
**
** Description      Get MAS info for the specified MAS instance id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_mn_start(void);


/*******************************************************************************
**
** Function         app_mce_mn_stop
**
** Description      Example of function to stop notification service
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_mn_stop(void);


/*******************************************************************************
**
** Function         app_mce_notif_reg
**
** Description      Set the Message Notification status to On or OFF on the MSE.
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_notif_reg(UINT8 instance_id, BOOLEAN bReg);


/*******************************************************************************
**
** Function         app_mce_upd_inbox
**
** Description      Example of function to send update inbox request to MAS
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_upd_inbox(UINT8 instance_id);


/*******************************************************************************
**
** Function         app_mce_get_folder_list
**
** Description      Get Folder listing at the current level on existing MAS connection
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_get_folder_list(UINT8 instance_id, UINT16 max_list_count, UINT16 start_offset);

/*******************************************************************************
**
** Function         app_mce_set_folder
**
** Description      Example of function to perform a set current folder operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_set_folder(UINT8 instance_id, const char* p_string_dir, int direction_flag);


/*******************************************************************************
**
** Function         app_mce_set_msg_sts
**
** Description      Example of function to perform a set message status operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_mce_set_msg_sts(UINT8 instance_id, const char* phandlestr, int status_indicator,
        int status_value);

/*******************************************************************************
**
** Function         mce_is_connected
**
** Description      Check if an mce instance is connected
**
** Parameters
**
** Returns          TRUE if connected, FALSE otherwise.
**
*******************************************************************************/
BOOLEAN mce_is_connected(BD_ADDR bda, UINT8 instance_id);

/*******************************************************************************
**
** Function         mce_is_open_pending
**
** Description      Check if mce Open is pending
**
** Parameters
**
** Returns          TRUE if open is pending, FALSE otherwise
**
*******************************************************************************/
BOOLEAN mce_is_open_pending(UINT8 instance_id);

/*******************************************************************************
**
** Function         mce_is_open
**
** Description      Check if mce is open
**
** Parameters
**
** Returns          TRUE if mce is open, FALSE otherwise. Return BDA of the connected device
**
*******************************************************************************/
BOOLEAN mce_is_open(BD_ADDR *bda);


/*******************************************************************************
**
** Function         mce_last_open_status
**
** Description      Get the last mce open status
**
** Parameters
**
** Returns          open status
**
*******************************************************************************/
tBSA_STATUS mce_last_open_status();

/*******************************************************************************
**
** Function         mce_is_mns_started
**
** Description      Is MNS service started
**
** Parameters
**
** Returns          true if mns service has been started
**
*******************************************************************************/
tBSA_STATUS mce_is_mns_started();

#endif /* APP_MCE_H_ */
