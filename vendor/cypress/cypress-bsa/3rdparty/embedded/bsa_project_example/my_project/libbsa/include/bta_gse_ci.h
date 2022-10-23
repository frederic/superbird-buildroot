/*****************************************************************************
**
**  Name:           bta_gse_ci.h
**
**  Description:    This is the interface file for the Generic PIM Server Equipment
**                  (GSE) subsystem call-in functions.
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GSE_CI_H
#define BTA_GSE_CI_H

#include "bta_api.h"
#include "bta_gp_def.h"
#include "bta_gse_api.h"
#include "bta_gse_co.h"

/*******************************************************************************
**
** Function         bta_gse_ci_get_obj_list_info
**
** Description      This function is called in response to the
**                  bta_gse_co_get_obj_list_info call-out function.
**
** Parameters       session_id - session ID
**                  status - BTA_MA_STATUS_OK operation is successful.
**                           BTA_MA_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_ci_get_obj_list_info(tBTA_GP_SESS_HANDLE  session_id,
                                                 tBTA_GP_STATUS status,
                                                 UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_ci_get_obj_list_entry
**
** Description      This function is called in response to the
**                  bta_gse_co_get_obj_list_entry call-out function.
**
** Parameters       session_id - session ID
**                  status - BTA_GP_STATUS_OK operation is successful.
**                           BTA_GP_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_ci_get_obj_list_entry(tBTA_GP_SESS_HANDLE session_id,
                                                 tBTA_GP_STATUS status,
                                                 UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_ci_get_obj_open
**
** Description      This function is called in response to the
**                  bta_gse_co_get_obj_open call-out function.
**
** Parameters       session_id - session ID
**                  fd - file descriptor to the file to open
**                  file_size - file size
**                  status - BTA_GP_STATUS_OK if p_msg points to a valid bmessage.
**                           BTA_GP_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_ci_get_obj_open(tBTA_GP_SESS_HANDLE  session_id,
                                            int fd, UINT32 file_size,
                                            tBTA_GP_STATUS status,
                                            UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_ci_get_obj_read
**
** Description      This function is called in response to the
**                  bta_mse_co_get_msg call-out function.
**
** Parameters       session_id - session ID
**                  fd - file descriptor to the file to read
**                  num_bytes_read - file size
**                  status - BTA_GP_STATUS_OK if p_msg points to a valid bmessage.
**                           BTA_GP_STATUS_FAIL if any errors have occurred.
**                  final  - whether this is the final read
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_ci_get_obj_read(tBTA_GP_SESS_HANDLE  session_id,
                                            int fd, UINT16 num_bytes_read,
                                            tBTA_GP_STATUS status, BOOLEAN final,
                                            UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_ci_set_obj_delete_status
**
** Description      This function is called in response to the
**                  bta_gse_co_set_obj_delete_status call-out function.
**
** Parameters       session_id - session ID
**                  status - BTA_GP_STATUS_OK if p_msg points to a valid bmessage.
**                           BTA_GP_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_ci_set_obj_delete_status(tBTA_GP_SESS_HANDLE  session_id,
                                                     tBTA_GP_STATUS status,
                                                     UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_ci_set_obj_status
**
** Description      This function is called in response to the
**                  bta_gse_co_set_obj_status call-out function.
**
** Parameters       session_id - session ID
**                  status - BTA_GP_STATUS_OK if p_msg points to a valid bmessage.
**                           BTA_GP_STATUS_FAIL if any errors have occurred.
**                  evt    - evt from the call-out function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_ci_set_obj_status(tBTA_GP_SESS_HANDLE  session_id,
                                              tBTA_GP_STATUS status,
                                              UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_ci_push_obj
**
** Description      This function is called in response to the
**                  bta_gse_co_push_obj call-out function.
**
** Parameters       session_id - session ID
**                  status - BTA_GP_STATUS_OK if p_msg points to a valid bmessage.
**                           BTA_GP_STATUS_FAIL if any errors have occurred.
**                  final  - whether this is the final push
**                  p_handle - name of the object to push
**                  evt    - evt from the call-out function
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_ci_push_obj(tBTA_GP_SESS_HANDLE  session_id,
                                        tBTA_GP_STATUS status,
                                        BOOLEAN final,
                                        char *p_handle,
                                        UINT16 evt);

#endif /* BTA_GSE_CI_H */
