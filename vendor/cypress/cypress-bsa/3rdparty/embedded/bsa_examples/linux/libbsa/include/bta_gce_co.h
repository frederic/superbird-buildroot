/*****************************************************************************
**
**  Name:           bta_gce_co.h
**
**  Description:    This is the interface file for the Generic PIM Client Equipment
**                  (GCE) subsystem call-out functions.
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GCE_CO_H
#define BTA_GCE_CO_H

#include "bta_api.h"
#include "bta_gp_def.h"
#include "bta_gse_api.h"
#include "bta_gce_api.h"
#include "bta_gp_co.h"

/*****************************************************************************
**  Constants and Data Types
*****************************************************************************/
/**************************
**  Common Definitions
***************************/
#define BTA_GCE_LEN_UNKNOWN      GOEP_LEN_UNKNOWN

/*****************************************************************************
**  Function Declarations
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_gce_co_open
**
** Description      This function is executed by BTA when a file is opened.
**                  The phone uses this function to open
**                  a file for reading or writing.
**
** Parameters       service_class - Service class UUID
                    p_path  - Fully qualified path and file name.
**                  oflags  - permissions and mode (see constants above)
**                  size    - size of file to put (0 if unavailable or not applicable)
**                  evt     - event that must be passed into the call-in function.
**                  gce_handle - GCE handle identifies GCE instance control block
**
** Returns          void
**
**                  Note: Upon completion of the request, a file descriptor (int),
**                        if successful, and an error code (tBTA_FS_CO_STATUS)
**                        are returned in the call-in function, bta_gce_ci_open().
**
*******************************************************************************/
BTA_API extern void bta_gce_co_open(UINT16 service_uuid, const char *p_path, int oflags,
                                    UINT32 size, UINT16 evt,
                                    tBTA_GCE_HANDLE gce_handle);

/*******************************************************************************
**
** Function         bta_gce_co_write_obj
**
** Description      This function is executed by BTA when a file is opened to write.
**                  The phone uses this function to open
**                  a file for reading or writing.
**
** Parameters       service_class - Service class UUID
**                  fd      - File descriptor to write to.
**                  obj_size- File of the object
**                  p_buf   - Memory address from the obex packet to write from
**                  is_last_packet  - If it is the last write.
**                  evt     - event that must be passed into the call-in function.
**                  gce_handle - GCE handle identifies GCE instance control block
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gce_co_write_obj(UINT16 service_uuid, int fd, UINT32 obj_size,
                                         UINT8 *p_msg, BOOLEAN is_last_packet,
                                         UINT16 evt, tBTA_GCE_HANDLE gce_handle);

/*******************************************************************************
**
** Function         bta_gce_co_read_push_obj
**
** Description      This function is used to read file to be pushed to the peer device
**
** Parameters       service_class - Service class UUID
                    nbytes - number of byte to read
**                  p_buf  - memory location to save the read data
**                  evt     - event that must be passed into the call-in function.
**                  gce_handle - GCE handle identifies GCE instance control block
**
** Returns          void
**
**
*******************************************************************************/
BTA_API extern void bta_gce_co_read_push_obj(UINT16 service_uuid, int fd, UINT32 buffer_size,
                                             UINT8 *p_buffer, UINT16 evt,
                                             tBTA_GCE_HANDLE gce_handle);

/*******************************************************************************
**
** Function         bta_gce_co_get_obj_path
**
** Description      This function is used to get the path for the specified
**                  obj handle
**
** Parameters       service_class - Service class UUID
**                  handle_str - handle string
**                  p_path  - (output) Fully qualified path and file name to store
**                            the object
**                  peer_bd_addr - peer_bd_addr used to identify where the file is from
**
** Returns          BOOELAN TRUE - operation is successful
**
**
*******************************************************************************/
BTA_API extern BOOLEAN bta_gce_co_get_obj_path(UINT16 service_uuid, char *handle_str,
                                               char *p_path, BD_ADDR peer_bd_addr);

#endif /* BTA_GCE_CO_H */
