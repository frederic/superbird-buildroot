/*****************************************************************************
**
**  Name:           app_pbc.h
**
**  Description:    Bluetooth Phonebook client application
**
**  Copyright (c)   2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef APP_PBC_H_
#define APP_PBC_H_

/* Callback to application for connection events */
typedef void (tPbcCallback)(tBSA_PBC_EVT event, tBSA_PBC_MSG *p_data);

/*******************************************************************************
**
** Function         app_pbc_enable
**
** Description      Example of function to enable PBC functionality
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_pbc_enable(tPbcCallback pcb);

/*******************************************************************************
**
** Function         app_pbc_disable
**
** Description      Example of function to stop PBC functionality
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_pbc_disable(void);

/*******************************************************************************
**
** Function         app_pbc_open
**
** Description      Example of function to open up a connection to peer
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_pbc_open(BD_ADDR bda);

/*******************************************************************************
**
** Function         app_pbc_close
**
** Description      Function to close PBC connection
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_pbc_close(void);

/*******************************************************************************
**
** Function         app_pbc_abort
**
** Description      Example of function to abort current actions
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_pbc_abort(void);

/*******************************************************************************
**
** Function         app_pbc_cancel
**
** Description      cancel connections
**
** Returns          0 if success -1 if failure
*******************************************************************************/
tBSA_STATUS app_pbc_cancel(void);

/*******************************************************************************
**
** Function         app_pbc_get_list_vcards
**
** Description      Example of function to perform a GET - List VCards operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_pbc_get_list_vcards(const char* string_dir, tBSA_PBC_ORDER  order,
    tBSA_PBC_ATTR  attr, const char* searchvalue, UINT16 max_list_count,
    BOOLEAN is_reset_missed_calls, tBSA_PBC_FILTER_MASK selector, tBSA_PBC_OP selector_op);

/*******************************************************************************
**
** Function         app_pbc_get_vcard
**
** Description      Example of function to perform a get VCard operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_pbc_get_vcard(const char* file_name);

/*******************************************************************************
**
** Function         app_pbc_get_phonebook
**
** Description      Example of function to perform a get phonebook operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_pbc_get_phonebook(const char* file_name,
    BOOLEAN is_reset_missed_calls, tBSA_PBC_FILTER_MASK selector, tBSA_PBC_OP selector_op);

/*******************************************************************************
**
** Function         app_pbc_set_chdir
**
** Description      Example of function to perform a set current folder operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/

tBSA_STATUS app_pbc_set_chdir(const char *string_dir);

/*******************************************************************************
**
** Function         pbc_is_open_pending
**
** Description      Check if pbc Open is pending
**
** Parameters
**
** Returns          TRUE if open is pending, FALSE otherwise
**
*******************************************************************************/
BOOLEAN pbc_is_open_pending();

/*******************************************************************************
**
** Function         pbc_set_open_pending
**
** Description      Set pbc open pending
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void pbc_set_open_pending(BOOLEAN bopenpend);

/*******************************************************************************
**
** Function         pbc_is_open
**
** Description      Check if pbc is open
**
** Parameters
**
** Returns          TRUE if pbc is open, FALSE otherwise. Return BDA of the connected device
**
*******************************************************************************/
BOOLEAN pbc_is_open(BD_ADDR *bda);


/*******************************************************************************
**
** Function         pbc_last_open_status
**
** Description      Get the last pbc open status
**
** Parameters
**
** Returns          open status
**
*******************************************************************************/
tBSA_STATUS pbc_last_open_status();



#endif /* APP_PBC_H_ */
