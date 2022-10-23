/*****************************************************************************
**
**  Name:           bta_gce_api.h
**
**  Description:    This is the public interface file of the API for the Generic PIM
**                  Access client subsystem of BTA, Broadcom's Bluetooth
**                  application layer for mobile phones.
**
**
**  Copyright (c) 2014-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GCE_API_H
#define BTA_GCE_API_H

#include "bta_api.h"
#include "bta_gp_def.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
/* Extra Debug Code */
#ifndef BTA_GCE_DEBUG
#define BTA_GCE_DEBUG           TRUE
#endif

typedef UINT8           tBTA_GCE_HANDLE;

/* Maximun number of notif server control block */
#define BTA_GCE_MAX_NUM_NOTIF_SERV          2

/* Maximun number of instances control block */
#define BTA_GCE_MAX_NUM_INST                6

/* Maximun number of session per notif server */
#define BTA_GCE_MAX_NUM_SESS_PER_NOTIF_SERV 2

/* GCE callback function evnets */
enum
{
    BTA_GCE_ENABLE_EVT              =0,
    BTA_GCE_DISABLE_EVT,
    BTA_GCE_DISCOVER_EVT,
    BTA_GCE_OPEN_EVT,
    BTA_GCE_CLOSE_EVT,
    BTA_GCE_GET_INST_INFO_EVT,
    BTA_GCE_OBEX_GET_RSP_EVT,
    BTA_GCE_OBEX_PUT_RSP_EVT,
    BTA_GCE_SYNC_ACC_EVT,
    BTA_GCE_GET_LISTING_EVT,
    BTA_GCE_GET_OBJ_EVT,
    BTA_GCE_GET_OBJ_PROG_EVT,
    BTA_GCE_PUSH_OBJ_EVT,
    BTA_GCE_PUSH_OBJ_PROG_EVT,
    BTA_GCE_FORWARD_OBJ_EVT,
    BTA_GCE_SET_OBJ_STATUS_EVT,
    BTA_GCE_NOTIF_REG_EVT,


    BTA_GCE_PN_START_EVT,
    BTA_GCE_PN_STOP_EVT,
    BTA_GCE_PN_OPEN_EVT,
    BTA_GCE_PN_CLOSE_EVT,
    BTA_GCE_PN_NOTIF_EVT,

    BTA_GCE_INVALID_EVT
};
typedef UINT8 tBTA_GCE_EVT;

#define BTA_GCE_PATH_TYPE_OBJ      0
#define BTA_GCE_PATH_TYPE_EVT_RPT  1
#define BTA_GCE_PATH_TYPE_UPLOAD   2
typedef UINT8 tBTA_GCE_PATH_TYPE;

typedef struct
{
    char            name[BTA_GP_MAX_NAME_LEN_PER_LINE];
    tBTA_GP_INST_ID pas_inst_id;
    UINT16          version;
    UINT16          psm;
    UINT32          peer_features;

} tBTA_GCE_PAS_REC;

/* Structure associated with BTA_GCE_DISCOVER_EVT */
typedef struct
{
    UINT16              status;
    BD_ADDR             bd_addr;
    UINT8               num_pas_srv;
    tBTA_GCE_PAS_REC    rec[BTA_GCE_MAX_NUM_INST];
} tBTA_GCE_DISCOVER;

/* Structure associated with BTA_GCE_OBEX_PUT_RSP_EVT
   and BTA_GCE_OBEX_GET_RSP_EVT                        */
typedef struct
{
    tBTA_GCE_HANDLE         gce_handle;
    tBTA_GP_INST_ID         instance_id;
    UINT8                   rsp_code;
} tBTA_GCE_OBEX_RSP;

/* Structure associated with BTA_GCE_API_PA_OPEN_EVT or
   BTA_GCE_API_PA_CLOSE_EVT                            */
typedef struct
{
    tBTA_GP_STATUS          status;
    tBTA_GP_INST_ID         inst_id;
    tBTM_BD_NAME            dev_name;    /* Name of device, "" if unknown */
    BD_ADDR                 bd_addr;     /* Address of device */
    tBTA_GCE_HANDLE         gce_handle;
    tBTA_GP_SUP_FEA_MASK    peer_feature;
} tBTA_GCE_PA_OPEN_CLOSE;

/* Structure associated with BTA_GCE_GET_INST_INFO_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;
    tBTA_GP_INST_ID     inst_id;
    tBTA_GP_INST_INFO   inst_info;
    tOBX_RSP_CODE       obx_rsp_code;       /* obex response code */
    tBTA_GCE_HANDLE     gce_handle;
} tBTA_GCE_PA_GET_INST_INFO;

typedef union
{
    UINT16          obj_list_size;
    UINT8           ltime[BTA_GP_DATETIME_SIZE+1];
}tBTA_GCE_LIST_APP_PARAM;

/* Structure associated with BTA_GCE_GET_LISTING_EVT  */
typedef struct
{
    UINT8                   *p_data;
    tBTA_GCE_LIST_APP_PARAM *p_param;
    tBTA_GCE_HANDLE         gce_handle;
    UINT16                  len; /* 0 if object not included */
    BOOLEAN                 is_final; /* TRUE - no more pkt to come */
    tBTA_GP_STATUS          status;
    tOBX_RSP_CODE           obx_rsp_code;       /* obex response code */
} tBTA_GCE_PA_OBJ_LIST;

/* Structure associated with BTA_GCE_GET_OBJ_EVT  */
typedef struct
{
    UINT8                   *p_data;
    tBTA_GCE_HANDLE         gce_handle;
    UINT16                  len; /* 0 if object not included */
    BOOLEAN                 is_final; /* TRUE - no more pkt to come */
    tBTA_GP_STATUS          status;
    tOBX_RSP_CODE           obx_rsp_code;       /* obex response code */
} tBTA_GCE_PA_GET_OBJ;

/* Structure associated with BTA_GCE_GET_OBJ_PROG_EVT  */
typedef struct
{
    UINT32                  read_size;
    tBTA_GCE_HANDLE         handle;
} tBTA_GCE_PA_GET_OBJ_PROG;

/* Structure associated with BTA_GCE_PUSH_OBJ_EVT  */
typedef struct
{
    UINT8                   *p_data;
    tBTA_GCE_HANDLE         gce_handle;
    tBTA_GP_STATUS          status;
    tOBX_RSP_CODE           obx_rsp_code;       /* obex response code */
    tBTA_GP_OBJ_HANDLE      obj_handle;
} tBTA_GCE_PA_PUSH_OBJ;

/* Structure associated with BTA_GCE_PUSH_OBJ_PROG_EVT  */
typedef struct
{
    UINT32                  read_size;
    UINT32                  obj_size;
    tBTA_GCE_HANDLE         gce_handle;
} tBTA_GCE_PA_PUSH_OBJ_PROG;

/* Structure associated with BTA_GCE_FORWARD_OBJ_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;
    tOBX_RSP_CODE       obx_rsp_code;       /* obex response code */
    tBTA_GCE_HANDLE     gce_handle;
    tBTA_GP_INST_ID     inst_id;
} tBTA_GCE_PA_FORWARD_OBJ;

/* Structure associated with BTA_GCE_SET_OBJ_STATUS_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;
    tOBX_RSP_CODE       obx_rsp_code;       /* obex response code */
    tBTA_GCE_HANDLE     gce_handle;
    tBTA_GP_INST_ID     inst_id;
} tBTA_GCE_PA_SET_OBJ_STATUS;

/* Structure associated with BTA_GCE_SYNC_ACC_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;
    tOBX_RSP_CODE       obx_rsp_code;       /* obex response code */
    tBTA_GCE_HANDLE     gce_handle;
} tBTA_GCE_PA_SYNC_ACC;

/* Structure associated with BTA_GCE_NOTIF_REG_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;
    tOBX_RSP_CODE       obx_rsp_code;       /* obex response code */
    tBTA_GCE_HANDLE     gce_handle;
    tBTA_GP_INST_ID     inst_id;
} tBTA_GCE_PA_NOTIF_REG;


/* Structure associated with BTA_GCE_PN_START_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;
    tBTA_GCE_HANDLE     pn_handle;
} tBTA_GCE_PN_START_STOP;

/* Structure associated with BTA_GCE_PN_OPEN_EVT or
   BTA_GCE_PN_CLOSE_EVT                            */
typedef struct
{
    tBTA_GP_STATUS      status;
    tBTM_BD_NAME        dev_name;    /* Name of device, "" if unknown */
    BD_ADDR             bd_addr;     /* Address of device */
    tOBX_RSP_CODE       obx_rsp_code;/* obex response code */
    UINT16              pn_sess_handle;

} tBTA_GCE_PN_OPEN_CLOSE;

/* Structure associated with BTA_GCE_PN_NOTIF_EVT  */
typedef struct
{
    UINT8               *p_object;          /* event report data */
    UINT16              pn_sess_handle;
    UINT16              len;                /* length of the event report */
    BOOLEAN             final;
    tBTA_GP_STATUS      status;
    UINT8               inst_id;            /* MAS instance ID */
    tOBX_RSP_CODE       obx_rsp_code;       /* obex response code */
} tBTA_GCE_PN_NOTIF;


/* Data associated with call back events */
typedef union
{
    tBTA_GP_STATUS              status;

    tBTA_GCE_DISCOVER           discover;
    tBTA_GCE_OBEX_RSP           pa_put_rsp;
    tBTA_GCE_OBEX_RSP           pa_get_rsp;
    tBTA_GCE_PA_OPEN_CLOSE      pa_open;
    tBTA_GCE_PA_OPEN_CLOSE      pa_close;
    tBTA_GCE_PA_GET_INST_INFO   pa_get_inst_info;
    tBTA_GCE_PA_OBJ_LIST        pa_get_list;
    tBTA_GCE_PA_GET_OBJ         pa_get_obj;
    tBTA_GCE_PA_GET_OBJ_PROG    pa_get_obj_prog;
    tBTA_GCE_PA_PUSH_OBJ        pa_push_obj;
    tBTA_GCE_PA_PUSH_OBJ_PROG   pa_push_obj_prog;
    tBTA_GCE_PA_FORWARD_OBJ     pa_forward_obj;
    tBTA_GCE_PA_SET_OBJ_STATUS  pa_set_obj_status;
    tBTA_GCE_PA_SYNC_ACC        pa_sync_acc;

    tBTA_GCE_PA_NOTIF_REG       pa_notif_reg;

    tBTA_GCE_PN_START_STOP      pn_start;
    tBTA_GCE_PN_START_STOP      pn_stop;
    tBTA_GCE_PN_OPEN_CLOSE      pn_open_close;
    tBTA_GCE_PN_NOTIF           pn_notif;

} tBTA_GCE;

typedef struct
{
    UINT16  max_file_len;           /* Maximum size file name */
    UINT16  max_path_len;           /* Maximum path length (includes appended file name) */
    char    path_separator;         /* 0x2f ('/'), or 0x5c ('\') */
    UINT16  max_name_len;           /* maximum folder name length */
    INT32   obx_rsp_tout;           /* maximum amount of time to wait for obx rsp */

} tBTA_GCE_CFG;

extern tBTA_GCE_CFG * p_bta_gce_cfg;

/* GCE callback function */
typedef void tBTA_GCE_CBACK(tBTA_GCE_EVT event, tBTA_GCE *p_data);

/* GCE discover callback function */
typedef void tBTA_GCE_SDP_CBACK(tBTA_GCE *p_data);


/*****************************************************************************
**  External Function Declarations
*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/**************************
**  API Functions
***************************/

/*******************************************************************************
**
** Function         BTA_GceEnable
**
** Description      Enable the GCE subsystems.  This function must be
**                  called before any other functions in the GCE API are called.
**                  When the enable operation is completed the callback function
**                  will be called with an BTA_GCE_ENABLE_EVT event.
**
** Parameters       p_cback - GCE event call back function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceEnable(tBTA_GCE_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_GceDisable
**
** Description      This function disables the GCE subsystem. If the client is
**                  currently connected to a peer device, the connection will be
**
** Parameters       void
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceDisable(void);

/*******************************************************************************
**
** Function         BTA_GceDiscover
**
** Description      This function discovers PIM access service on the peer device.
**
**
** Parameters       bd_addr: Peer bd addr
**                  uuid: profile UUID upper layer is looking for
**                  p_sdp_cback: SDP callback
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceDiscover(BD_ADDR bd_addr, UINT16 uuid,
                                    tBTA_GCE_SDP_CBACK *p_sdp_cback);


/*******************************************************************************
**
** Function         BTA_GceOpen
**
** Description      This function opens PIM access conection to the peer device.
**
**
** Parameters       app_id: application ID
                    bd_addr: Peer bd addr
**                  instance_id: Peer instance ID to connect to
**                  sec_mask: Security mask
**                  p_cback: callback functions
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceOpen(UINT8 app_id, UINT16 uuid, BD_ADDR bd_addr,
                                tBTA_GP_INST_ID instance_id, tBTA_SEC sec_mask,
                                tBTA_GCE_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_GceClose
**
** Description      This function closes PIM access conection to the peer device.
**
**
** Parameters       gce_handle: Handle identify instance control block
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceClose(tBTA_GCE_HANDLE gce_handle);



/*******************************************************************************
**
** Function         BTA_GceObjectListing
**
** Description      This function is used by the GCE to retrieve the listing of
**                  objects on the GSE.
**
**
** Parameters       gce_handle: Handle identify instance control block
**                  p_param: Parameters related to get object lising
**                  p_folder: Name of the folder to do object listing
**                  p_obj_type: Object listing type
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceObjectListing(tBTA_GCE_HANDLE gce_handle,
                                         tBTA_GP_OL_FILTER_PARAM *p_param,
                                         char *p_folder, char *p_obj_type);
/*******************************************************************************
**
** Function         BTA_GceNotifReg
**
** Description      This function sets the PIM notification status to ON or OFF
**                  on the GSE. PIM notification service must be enabled by
**                  calling BTA_GceNotifStart first.
**
**
**
** Parameters       gce_handle: Handle identify instance control block
**                  notif_status: Set to ON or OFF
**                  alarm_status: Used to switch acoustic alarm
**
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceNotifReg(tBTA_GCE_HANDLE gce_handle,
                                    tBTA_GP_NOTIF_STATUS notif_status,
                                    tBTA_GP_ACOUSTIC_ALARM_STATUS alarm_status);

/*******************************************************************************
**
** Function         BTA_GceGetObj
**
** Description      This function is used by the GCE to retrieve the objects
**                  on the GSE.
**
**
** Parameters       gce_handle: Handle identify instance control block
**                  p_param: Parameters related to get object
**                  p_obj_type: get object type
**
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceGetObj(tBTA_GCE_HANDLE gce_handle,
                                  tBTA_GP_GET_OBJ_PARAM *p_param, char *p_obj_type);

/*******************************************************************************
**
** Function         BTA_GceForwardObj
**
** Description      This function is used by the GCE to forward object on the GSE
**
**
** Parameters       gce_handle: Handle identify instance control block
**                  obj_handle: object handle
**                  p_obj_type: forward object type
**                  p_description: Description info related recipient
**
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceForwardObj(tBTA_GCE_HANDLE gce_handle,
                                      tBTA_GP_OBJ_HANDLE obj_handle,
                                      char *p_obj_type, char *p_description);

/*******************************************************************************
**
** Function         BTA_GcePushObj
**
** Description      This function is used by the GCE to putsh an object yo the GSE
**
**
** Parameters       gce_handle: Handle identify instance control block
**                  is_send: Indicate whehter the object uploaded shall be sent to
**                           participant or not
**                  p_folder: Folder name the object to be uploaded to
**                  p_path : Path to the object to be upload
**                  p_obj_type: push object type
**
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GcePushObj(tBTA_GCE_HANDLE gce_handle,
                                   tBTA_GP_STATUS_VAL is_send,
                                   char *p_folder, char *p_path,
                                   char *p_obj_type);

/*******************************************************************************
**
** Function         BTA_GceSetObjStatus
**
** Description      This function is used by the GCE to set object status on the GSE
**
**
** Parameters       gce_handle: Handle identify instance control block
**                  obj_handle: Object handle
**                  p_obj_type: push object type
**                  status_type: Set status type
**                  status_val: Status value associated with the type
**                  postpone_val: Indicate the number of minutes the alarm shall be postponed
**
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceSetObjStatus(tBTA_GCE_HANDLE gce_handle, tBTA_GP_OBJ_HANDLE obj_handle,
                       char *p_obj_type, tBTA_GP_STATUS_IND_TYPE status_type,
                       tBTA_GP_STATUS_VAL status_val, tBTA_GP_POSTPONE_VAL postpone_val);

/*******************************************************************************
**
** Function         BTA_GceGetInstanceInfo
**
** Description      This function may be used by the PIMCE to retrieve instance
**                  information on the PIMSE
**
**
**
** Parameters       gce_handle: Handle identify instance control block
**                  instance_id: ID of the instance info to retrieve
**                  p_inst_type: Instance type
**
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceGetInstanceInfo(tBTA_GCE_HANDLE gce_handle,
                                           tBTA_GP_INST_ID instance_id,
                                           char *p_inst_type);

/*******************************************************************************
**
** Function         BTA_GceSyncAccount
**
** Description      This function allows the GCE to initiate a synchronization
**                  of the corresponding account on the GSE with an external
**                  server
**
**
**
** Parameters       first_conn_gce_handle: The first connected gce handle
**                  p_acc_type : Sync account type
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GceSyncAccount(tBTA_GCE_HANDLE first_conn_gce_handle,
                                        char *p_acc_type);

/*******************************************************************************
**
** Function         BTA_GcePnStart
**
** Description      Start the Generic PIM Notification service server.
**                  When the Start operation is complete the callback function
**                  will be called with an BTA_GCE_PN_START_EVT event.
**                  Note: GCE always enables (BTA_SEC_AUTHENTICATE | BTA_SEC_ENCRYPT)
**
**  Parameters      uuid: service class UUID
**                  version: Notification version number
**                  sec_mask - The security setting for the message access server.
**                  p_service_name - The name of the Notification service, in SDP.
**                                   Maximum length is 35 bytes.
**                  notif_features - Notification supported features
**                  p_cback - Notification callback
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GcePnStart(UINT16 uuid, UINT16 version, tBTA_SEC sec_mask,
                                   const char *p_service_name,
                                   tBTA_GP_SUP_FEA_MASK notif_features,
                                   tBTA_GCE_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_GcePnStop
**
** Description      Stop the Generic PIM Access service server.  If the server
**                  is currently connected to a peer device the connection will
**                  be closed.
**
**  Parameters      pn_handle: Handle identifies the notification control block
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GcePnStop(UINT8 pn_handle);

#ifdef __cplusplus
}
#endif

#endif /* BTA_GCE_API_H */
