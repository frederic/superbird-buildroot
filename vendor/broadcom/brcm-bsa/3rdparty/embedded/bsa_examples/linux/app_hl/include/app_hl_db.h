/*****************************************************************************
 **
 **  Name:           app_hl_db.h
 **
 **  Description:    Application Bluetooth Health Database
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_HL_DB_H
#define APP_HL_DB_H

/*
 * Global Variables
 */
typedef struct
{
    tBSA_HL_APP_HANDLE app_handle; /* APP Handle */
    tBSA_HL_MDL_CFG mdl_cfg[BSA_HL_NUM_MDL_CFGS]; /* MDLs configuration */
} tAPP_HL_APP_CFG;


/*******************************************************************************
 **
 ** Function         app_hl_db_mdl_add
 **
 ** Description      Function used to add one MDL configuration
 **
 ** Parameters       app_handle: Application Handle
 **                  mdl_index: MDL Index
 **                  p_app_cfg: The MDL configuration
 **
 ** Returns          status: -1:error, 0:ok
 **
 **                  note: this function automatically:
 **                        - reads the database from file
 **                        - update it in memory
 **                        - write it back to file
 **
 *******************************************************************************/
int app_hl_db_mdl_add(tBSA_HL_APP_HANDLE app_handle, UINT8 mdl_index,
        tBSA_HL_MDL_CFG *p_mdl_cfg);

/*******************************************************************************
 **
 ** Function         app_hl_db_get_by_app_hdl
 **
 ** Description      Function used to get all MDL configurations for an App Handle
 **
 ** Parameters       app_handle: Application Handle
 **                  p_app_cfg: MDL configuration buffer (table of structures)
 **
 ** Returns          status: -1:error, 0:ok
 **
 **                  note: this function automatically:
 **                        - reads the database from file
 **                        - return all the MDL config for the matching App Handle
 **
 *******************************************************************************/

int app_hl_db_get_by_app_hdl(tBSA_HL_APP_HANDLE app_handle, tBSA_HL_MDL_CFG *p_mdl_cfg);

/*******************************************************************************
 **
 ** Function         app_hl_db_mdl_remove_by_index
 **
 ** Description      Function used to delete one MDL for an App Handle
 **
 ** Parameters       app_handle: Application Handle
 **                  mdl_index: MDL Index to delete
 **
 ** Returns          status: -1:error, 0:ok
 **
 **                  note: this function automatically:
 **                        - reads the database
 **                        - update it in memory
 **                        - write it back to file
 **
 *******************************************************************************/
int app_hl_db_mdl_remove_by_index(tBSA_HL_APP_HANDLE app_handle, UINT8 mdl_index);

/*******************************************************************************
 **
 ** Function         app_hl_db_mdl_remove_by_bdaddr
 **
 ** Description      Function used to delete one or all MDL configurations matching
 **                  a BdAddr (for every Application Handles.
 **                  This function can be used when a Peer Device is suppressed
 **
 ** Parameters       bd_addr: BdAddr to erase
 **
 ** Returns          status: -1:error, 0:ok
 **
 **                  note: this function automatically:
 **                        - reads the database
 **                        - update it in memory
 **                        - write it back to file
 **
 *******************************************************************************/
int app_hl_db_mdl_remove_by_bdaddr(BD_ADDR bd_addr);

/*******************************************************************************
 **
 ** Function         app_hl_db_display
 **
 ** Description      Function used to display all the HL database (debug)
 **
 ** Parameters       None
 **
 ** Returns          status: -1:error, 0:ok
 **
 *******************************************************************************/
int app_hl_db_display(void);

#endif
