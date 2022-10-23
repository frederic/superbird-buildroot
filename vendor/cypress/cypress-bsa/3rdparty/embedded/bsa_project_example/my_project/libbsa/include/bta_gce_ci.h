/*****************************************************************************
**
**  Name:           bta_gce_ci.h
**
**  Description:    This is the interface file for the Genric PIM Client Equipment
**                  (GCE) subsystem call-in functions.
**
**  Copyright (c) 2009 - 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GCE_CI_H
#define BTA_GCE_CI_H

#include "bta_api.h"
#include "bta_gp_def.h"
#include "bta_gse_api.h"
#include "bta_gce_api.h"
#include "bta_fs_ci.h"

/*******************************************************************************
**
** Function         bta_gce_ci_write_obj
**
** Description      This function is a response for the bta_gce_ci_write_obj
**
**
** Parameters       STATUS - indicate the write operation is successful or not
**                  evt    - event that waspassed into the call-out function.
**                  gce_handle - Handle identify instance control block
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gce_ci_write_obj( tBTA_GP_STATUS status,
                                          UINT16 evt, tBTA_GCE_HANDLE gce_handle);

/*******************************************************************************
**
** Function         bta_gce_ci_open
**
** Description      This function sends an event to BTA indicating the callout has
**                  finished opening a file for reading or writing.
**
** Parameters       fd - file descriptor passed to the stack in the
**                       bta_fs_ci_open call-in function.
**                  status - BTA_FS_CO_OK if file was opened in mode specified
**                                          in the call-out function.
**                           BTA_FS_CO_EACCES if the file exists, but contains
**                                          the wrong access permissions.
**                           BTA_FS_CO_FAIL if any other error has occurred.
**                  file_size - The total size of the file
**                  evt - Used Internally by BTA -> MUST be same value passed
**                       in call-out function.
**                  gce_handle - Handle identify instance control block
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gce_ci_open(int fd, UINT16 status, UINT32 file_size, UINT16 evt,
                                    tBTA_GCE_HANDLE gce_handle);

/*******************************************************************************
**
** Function         bta_gce_ci_read_push_obj
**
** Description      This function is a response for the bta_gce_co_read_push_obj
**
**
** Parameters       status - indicate the read operation is successful or not
**                  num_bytes_read - size of the bytes filled
**                  evt    - event that waspassed into the call-out function.
**                  gce_handle - Handle identify instance control block
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gce_ci_read_push_obj( tBTA_GP_STATUS status,
                                              UINT32 num_bytes_read,
                                              UINT16 evt, tBTA_GCE_HANDLE gce_handle);


#endif /* BTA_GCE_CI_H */




