/*****************************************************************************
**
**  Name:               app_ftc.h
**
**  Description:        This is the header file for the FTC application
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bsa_api.h"

#ifndef APP_FTC_H
#define APP_FTC_H

/*******************************************************************************
 **
 ** Function         app_ftc_open
 **
 ** Description      Example of function to exhcange a vcard with current peer device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_open(void);

/*******************************************************************************
 **
 ** Function         app_ftc_put_file
 **
 ** Description      Example of function to put a file in the
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_put_file(char * p_file_name);

/*******************************************************************************
 **
 ** Function         app_ftc_get_file
 **
 ** Description      Example of function to get a file from peer current dir.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_get_file(char * p_local_file_name, char * p_remote_file_name);

/*******************************************************************************
 **
 ** Function         app_ftc_cd
 **
 ** Description      Example of function to get a file from peer current dir.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_cd(void);

/*******************************************************************************
 **
 ** Function         app_ftc_mkdir
 **
 ** Description      Example of function to get a file from peer current dir.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_mkdir(void);

/*******************************************************************************
 **
 ** Function         app_ftc_close
 **
 ** Description      Example of function to disconnect current device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_close(void);

/*******************************************************************************
 **
 ** Function         app_ftc_list_dir
 **
 ** Description      Example of function to perform a get directory listing request
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_list_dir(char * p_dir, BOOLEAN is_xml);

/*******************************************************************************
 **
 ** Function         app_ftc_rm_file
 **
 ** Description      Example of function to perform a file remove operation on the server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_rm_file(char * p_file_name);


/*******************************************************************************
 **
 ** Function         app_start_ftc
 **
 ** Description      Example of function to start OPP Client application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_start_ftc(void);

/*******************************************************************************
 **
 ** Function         app_stop_ftc
 **
 ** Description      Example of function to sttop OPP Server application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_stop_ftc(void);

/*******************************************************************************
 **
 ** Function         app_ftc_cp_file
 **
 ** Description      Example of function to perform a file copy on the server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_cp_file(char * p_dst_file_name, char * p_src_file_name);

/*******************************************************************************
 **
 ** Function         app_ftc_mv_file
 **
 ** Description      Example of function to perform a file move operation on the server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_mv_file(char * p_dst_file_name, char * p_src_file_name);

#endif /* APP_FTC_H_ */
