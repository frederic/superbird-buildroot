/*****************************************************************************
 **
 **  Name:           app_dg.h
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2011-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_DG_H
#define APP_DG_H

/*******************************************************************************
 **
 ** Function         app_dg_init
 **
 ** Description      Init DG application
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_init(void);

/*******************************************************************************
 **
 ** Function         app_dg_con_free_all
 **
 ** Description      Function to free all DG connection structure
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_con_free_all(void);

/*******************************************************************************
 **
 ** Function         app_dg_start
 **
 ** Description      Start DG application
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_start(void);

/*******************************************************************************
 **
 ** Function         app_dg_stop
 **
 ** Description      Stop DG application
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_stop(void);

/*******************************************************************************
 **
 ** Function         app_dg_open
 **
 ** Description      Example of function to open a DG connection
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_dg_open();

/*******************************************************************************
 **
 ** Function         app_dg_open_ex
 **
 ** Description      Example of function to open a DG connection for multiple
 **                  or single connections.
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_open_ex(int single_conn_only);

/*******************************************************************************
 **
 ** Function         app_dg_close
 **
 ** Description      Example of function to close a DG connection
 **
 ** Returns          int
 **
 *******************************************************************************/
int  app_dg_close(int connection);

/*******************************************************************************
 **
 ** Function         app_dg_listen
 **
 ** Description      Example of function to start an SPP server
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_listen(void);

/*******************************************************************************
 **
 ** Function         app_dg_shutdown
 **
 ** Description      Example of function to stop an SPP server
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_dg_shutdown(int connection);

/*******************************************************************************
 **
 ** Function         app_dg_read
 **
 ** Description      Example of function to read a data gateway port
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_read(int connection);


/*******************************************************************************
 **
 ** Function         app_dg_send_file
 **
 ** Description      Example of function to send a file
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_send_file(char * p_file_name, int connection);


/*******************************************************************************
 **
 ** Function         app_dg_send_data
 **
 ** Description      Example of function to send data
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_send_data(int connection);


/*******************************************************************************
 **
 ** Function         app_dg_find_service
 **
 ** Description      Example of function to find custom bluetooth service
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_dg_find_service(void);

/*******************************************************************************
 **
 ** Function         app_dg_loopback_toggle
 **
 ** Description      This function is used to stop DG
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_loopback_toggle(void);

/*******************************************************************************
 **
 ** Function         app_dg_con_display
 **
 ** Description      Function to display DG connection status
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_dg_con_display(void);

#endif

