/*****************************************************************************
 **
 **  Name:           app_hl_xml.c
 **
 **  Description:    This module contains utility functions to access Health
 **                  saved parameters
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/

#ifndef APP_HL_XML_H
#define APP_HL_XML_H


/*******************************************************************************
 **
 ** Function        app_hl_xml_init
 **
 ** Description     Initialize XML parameter system (nothing for the moment)
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_hl_xml_init(void);

/*******************************************************************************
 **
 ** Function        app_hl_xml_read
 **
 ** Description     Load the MDL data base from  XML file
 **
 ** Parameters      p_app_cfg: Application Configuration buffer
 **
 ** Returns         0 if ok, otherwise -1
 **
 *******************************************************************************/
int app_hl_xml_read(tAPP_HL_APP_CFG *p_app_cfg);

/*******************************************************************************
 **
 ** Function        app_hl_xml_write
 **
 ** Description     Writes the database file in file system or XML, creates one if it doesn't exist
 **
 ** Parameters      p_app_cfg: Application Configuration to save to file
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hl_xml_write(tAPP_HL_APP_CFG *p_app_cfg);

#endif
