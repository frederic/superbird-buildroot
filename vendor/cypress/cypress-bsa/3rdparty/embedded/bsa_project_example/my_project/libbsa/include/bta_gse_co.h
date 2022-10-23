/*****************************************************************************
**
**  Name:           bta_gse_co.h
**
**  Description:    This is the interface file for the Generic PIM Server Equipment
**                  (GSE) subsystem call-out functions.
**
**  Copyright (c) 2014-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GSE_CO_H
#define BTA_GSE_CO_H

#include "bta_api.h"
#include "bta_gp_def.h"
#include "bta_gse_api.h"

enum
{
    BTA_GSE_CO_RCV_STATUS_COMPLETE = 0,
    BTA_GSE_CO_RCV_STATUS_FRACTIONED,
    BTA_GSE_CO_RCV_STATUS_NOTIFICATION,
    BTA_GSE_CO_RCV_STATUS_MAX
};

typedef UINT8 tBTA_GSE_CO_RCV_STATUS;

typedef struct
{
    UINT16                   obj_list_size;
    UINT8                    gse_time_len;
    char                     gse_time[BTA_GP_DATETIME_SIZE+1];
} tBTA_GSE_CO_OBJ_LIST_INFO;


typedef struct
{
    void                    *refdata;            /* holder for OS specific data used to get next msg entry */
    tBTA_GP_CTN_OL_MASK     parameter_mask;
    tBTA_GSE_CO_RCV_STATUS  reception_status;
    tBTA_GP_OBJ_HANDLE      obj_handle;
    tBTA_GP_OBJ_TYPE        cal_type;
    char                    update[BTA_GP_DATETIME_SIZE+1];
    char                    summary[BTA_GP_SUMMARY_SIZE];
    char                    start_time[BTA_GP_DATETIME_SIZE+1];
    char                    end_time[BTA_GP_DATETIME_SIZE+1];
    char                    ori_name[BTA_GP_NAME_LEN+1];
    char                    ori_address[BTA_GP_NAME_LEN+1];
    UINT32                  obj_size;
    char                    high_priority[BTA_GP_PRIORITY_SIZE+1];
    char                    pstatus[BTA_GP_PARTSTAT_SIZE+1];
    char                    alarm_status[BTA_GP_ALARM_STATUS_SIZE+1];
    BOOLEAN                 send_status;
    BOOLEAN                 recurrent;

} tBTA_GSE_CO_OBJ_LIST_ENTRY;

/*******************************************************************************
**
** Function        bta_gse_co_sync_account
**
** Description     Sync account
**
** Parameters       service_class - Service class UUID
**                  session_id  - session ID
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_co_sync_account(UINT16 service_class,
                                            tBTA_GP_SESS_HANDLE session_id);


/*******************************************************************************
**
** Function         bta_gse_co_forward_obj
**
** Description      Forward object
**
** Parameters       service_class - Service class UUID
**                  inst_id  - instance ID.
**                  obj_handle - object handle
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_co_forward_obj(UINT16 service_class, tBTA_GP_INST_ID inst_id,
                                           tBTA_GP_SESS_HANDLE session_id,
                                           tBTA_GP_OBJ_HANDLE obj_handle,
                                           UINT8 *p_desc_hdr);


/*******************************************************************************
**
** Function         bta_gse_co_get_obj_list_info
**
** Description      This function is called to get a object list information for
**                  the specified folder
**
** Parameters       service_class - service class UUId
                    session_id  - session ID
**                  p_name - points to the folder for getting the
**                           object list information
**                  p_filter_param - filter parameters for getting the object
**                                list information
**                  p_info(input/output) - Points to the object listing information
**                  evt - event that be passed into the call-in function.
**
** Returns          void
**
**                  Note: Upon completion of the request, the status is passed
**                        in the bta_gse_ci_get_obj_list_info() call-in function.
**                        BTA_GP_STATUS_OK is returned when p_entry is valid
**                        BTA_GP_STATUS_FAIL is returned if an error occurred
**
*******************************************************************************/
BTA_API extern void bta_gse_co_get_obj_list_info(UINT16 service_class,
                                    tBTA_GP_SESS_HANDLE  session_id,
                                    char *p_name,
                                    tBTA_GP_OL_FILTER_PARAM * p_filter_param,
                                    tBTA_GSE_CO_OBJ_LIST_INFO * p_info,
                                    UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_co_get_obj_list_entry
**
** Description      This function is called to retrieve a object list entry for
**                  the specified folder.  The obj information should be filled by
**                  application into the location specified by p_entry.
**
** Parameters       service_class - service class UUId
**                  session_id - session ID
**                  p_name - points to the folder for getting the
**                           object list entry
**                  p_filter_param - filter parameters for getting object list
**                  first_item - TRUE if first get, FALSE if next msg
**                  p_entry(input/output)  - Points to current entry data
**                  evt - event that must be passed into the call-in function.
**
** Returns          void
**
**                  Note: Upon completion of the request, the status is passed
**                        in the bta_mse_ci_get_msg_list_entry() call-in function.
**                        BTA_GP_STATUS_OK is returned when p_entry is valid,
**                        BTA_GP_STATUS_EODIR is returned when no more entries [finished]
**                        BTA_GP_STATUS_FAIL is returned if an error occurred
*******************************************************************************/
BTA_API extern void bta_gse_co_get_obj_list_entry(UINT16 service_class,
                                                  tBTA_GP_SESS_HANDLE session_id,
                                                  char *p_name,
                                                  tBTA_GP_OL_FILTER_PARAM *p_filter_param,
                                                  BOOLEAN first_item,
                                                  tBTA_GSE_CO_OBJ_LIST_ENTRY *p_entry,
                                                  UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_co_get_obj_open
**
** Description      This function is called to open a object for the
**                  specified object handle.
**
** Parameters       service_class - service class UUId
**                  session_id - session ID
**                  p_param  - points to the parameters for the get object operation
**                  evt - event that be passed into the call-in function.
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_co_get_obj_open(UINT16 service_class,
                                            tBTA_GP_SESS_HANDLE  session_id,
                                            tBTA_GP_GET_OBJ_PARAM *p_param,
                                            UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_co_get_obj_read
**
** Description      This function is called to read the content for the
**                  specified object handle.
**
** Parameters       service_class - service class UUId
**                  session_id - session ID
**                  is_first_get - TRUE if the first read
**                  fd  - file descriptor
**                  p_buf - pointer to the memory to save the content read
**                  nbytes - number of bytes available to read
**                  p_obj_handle - pointer to the name of the handle
**                  evt - event that be passed into the call-in function.
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_gse_co_get_obj_read(UINT16 service_class,
                                            tBTA_GP_SESS_HANDLE session_id,
                                            BOOLEAN is_first_get,
                                            int fd,
                                            UINT8 *p_buf, UINT16 nbytes,
                                            char *p_obj_handle,
                                            UINT16 evt);


/*******************************************************************************
**
** Function         bta_gse_co_push_obj
**
** Description      This function is called by BTA to save the pushed object
**                  from the peer device
**
** Parameters       service_class - service class UUId
**                  session_id - session ID
**                  p_param - points to parameters for object upload
**                  msg_len - length of the object uploaded
**                  p_msg - points to the address to save the uploaded object
**                  first_pkt - TRUE first push message packet
**                  multi_pkt_status -
**                       BTA_GP_MPKT_STATUS_MORE - need to get more packets
**                       BTA_GP_MPKT_STATUS_LAST - last packet of
**                                                 the bMessage to be uploaded
**                  evt - event that be passed into the call-in function.
**
**
** Returns          void
**
**                  Note: Upon completion of the request, bta_gse_ci_push_obj() is
**                        called with the buffer of data, along with the number
**                        of bytes read into the buffer, and a status.
**
*******************************************************************************/
BTA_API extern void bta_gse_co_push_obj(UINT16 service_class, tBTA_GP_SESS_HANDLE session_id,
                                        tBTA_GP_PUSH_OBJ_PARAM *p_param, UINT16 msg_len,
                                        UINT8 *p_msg, BOOLEAN first_pkt,
                                        tBTA_GP_MPKT_STATUS mpkt_status,
                                        UINT16 evt);

/*******************************************************************************
**
** Function         bta_gse_co_set_obj_delete_status
**
** Description      This function is called to delete an object
**
** Parameters       service_class - service class UUId
**                  session_id - session ID
**                  handle - object handle
**                  status value - 1- yes, 0 = No
**                  evt_id - event that be passed into the call-in function.
**
** Returns          void
**
**                  Note: Upon completion of the request, the status is passed
**                        in the bta_gse_ci_set_obj_delete_status() call-in function.
**                        BTA_GP_STATUS_OK is returned when the delete operation is successful
**                        BTA_GP_STATUS_FAIL is returned if an error occurred
*******************************************************************************/
BTA_API extern void bta_gse_co_set_obj_delete_status(UINT16 service_class,
                                                     tBTA_GP_INST_ID inst_id,
                                                     tBTA_GP_SESS_HANDLE session_id,
                                                     tBTA_GP_OBJ_HANDLE handle,
                                                     tBTA_GP_STATUS_VAL status_value,
                                                     UINT16 evt_id);


/*******************************************************************************
**
** Function         bta_gse_co_set_obj_status
**
** Description      This function is called to set an object status
**
** Parameters       service_class - service class UUId
**                  session_id - session ID
**                  handle - object handle
**                  status type - one of the tBTA_GP_STATUS_IND_TYPE
**                  status value - 1- yes, 0 = No
**                  evt_id - event that be passed into the call-in function.
**
** Returns          void
**
**                  Note: Upon completion of the request, the status is passed
**                        in the bta_gse_ci_set_obj_delete_status() call-in function.
**                        BTA_GP_STATUS_OK is returned when the delete operation is successful
**                        BTA_GP_STATUS_FAIL is returned if an error occurred
*******************************************************************************/
BTA_API extern void bta_gse_co_set_obj_status(UINT16 service_class, tBTA_GP_INST_ID inst_id,
                                              tBTA_GP_SESS_HANDLE  session_id,
                                              tBTA_GP_OBJ_HANDLE handle,
                                              tBTA_GP_STATUS_IND_TYPE status_type,
                                              tBTA_GP_STATUS_VAL status_value,
                                              tBTA_GP_POSTPONE_VAL post_value,
                                              UINT16 evt_id);
#endif /* BTA_GSE_CO_H */
