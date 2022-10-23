/*****************************************************************************
**
**  Name:           hci_cmd.h
**
**  Description:    Bluetooth HCI functions
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

/*******************************************************************************
 **
 ** Function         hci_send_init_param
 **
 ** Description      This function is used to initialize the HCI params
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
extern int hci_send_init_param(tBSA_TM_HCI_CMD *hci_param);

/*******************************************************************************
 **
 ** Function         hci_send
 **
 ** Description      This function is used to send a HCI Command
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
extern int hci_send(tBSA_TM_HCI_CMD *hci_param);

