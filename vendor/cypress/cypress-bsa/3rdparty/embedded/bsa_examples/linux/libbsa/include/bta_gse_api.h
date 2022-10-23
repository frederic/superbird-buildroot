/*****************************************************************************
**
**  Name:           bta_gse_api.h
**
**  Description:    This is the public interface file of the API for the Generic PIM
**                  Access server subsystem of BTA, Broadcom's Bluetooth
**                  application layer for mobile phones.
**
**
**  Copyright (c) 2014-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GSE_API_H
#define BTA_GSE_API_H

#include "bta_api.h"
#include "bta_gp_def.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/
/* Extra Debug Code */
#ifndef BTA_GSE_DEBUG
#define BTA_GSE_DEBUG           TRUE
#endif

#define BTA_GSE_LEN_EMAIL_URI       256
#define BTA_GSE_LEN_LAST_UPDATE     30

/* Set Notification Registration response types */
enum
{
    BTA_GSE_SET_NOTIF_REG_ALLOW = 0,    /* Allow the notification registration request*/
    BTA_GSE_SET_NOTIF_REG_FORBID        /* Disallow the notification registration request */
};
typedef UINT8 tBTA_GSE_SET_NOTIF_REG_TYPE;

/* Update Inbox response types */
enum
{
    BTA_GSE_SYNC_INST_ALLOW = 0,    /* Allow sync account */
    BTA_GSE_SYNC_INST_FORBID        /* Disallow sync account */
};
typedef UINT8 tBTA_GSE_SYNC_ACC_TYPE;

/* Update Inbox response types */
enum
{
    BTA_GSE_FORWARD_OBJ_ALLOW = 0,    /* Allow sync account */
    BTA_GSE_FORWARD_OBJ_FORBID        /* Disallow sync account */
};
typedef UINT8 tBTA_GSE_FORWARD_OBJ_TYPE;

/* Maximun number of session in one GSE instance control block */
#define BTA_GSE_MAX_NUM_SESS        2

/* Maximun number of instance in one GSE control block */
#define BTA_GSE_MAX_NUM_INST        6

/* Maximun number of instance in one GSE control block */
#define BTA_GSE_INVALID_INST_ID     BTA_GSE_MAX_NUM_INST + 1; /* Inst_id starts from 1 */

/* Maximun number of notificatopm control block in one GSE service control block */
#define BTA_GSE_MAX_NUM_NOTIF       3

#define BTA_GSE_NAME_LENGTH         32

#define BTA_GSE_PATH_LEN            294

typedef struct
{
    UINT16                  version;
    tBTA_SEC                sec_mask;
    tBTA_GP_SUP_FEA_MASK    features;
    tBTA_GP_INST_ID         pas_inst_id;
    char                    service_name[BTA_GSE_NAME_LENGTH];
    char                    root_path[BTA_GSE_PATH_LEN];

} tBTA_GSE_START_INFO;

typedef struct
{
    UINT16  max_file_len;                       /* Maximum size file name */
    UINT16  max_path_len;                       /* Maximum path length (includes appended file name) */
    char    path_separator;                     /* 0x2f ('/'), or 0x5c ('\') */
    UINT16  max_name_len;                       /* maximum folder name length */
    char    emailuri[BTA_GSE_LEN_EMAIL_URI];    /* email URL*/
    char    lastupdate[BTA_GSE_LEN_LAST_UPDATE];/* last update */
    INT32   obx_rsp_tout;                       /* maximum amount of time to wait for obx rsp */

} tBTA_GSE_CFG;

extern tBTA_GSE_CFG * p_bta_gse_cfg;

/* GSE callback function evnets */
enum
{
    BTA_GSE_ENABLE_EVT              =0,
    BTA_GSE_DISABLE_EVT,
    BTA_GSE_ACCESS_EVT,
    BTA_GSE_OPER_CMPL_EVT,
    BTA_GSE_PA_OPEN_EVT,
    BTA_GSE_PA_CLOSE_EVT,
    BTA_GSE_SET_NOTIF_REG_REQ_EVT,
    BTA_GSE_SYNC_ACC_EVT,
    BTA_GSE_FORWARD_OBJ_EVT,
    BTA_GSE_NOTIF_REG_EVT,
    BTA_GSE_GET_OBJ_IN_PROG_EVT,
    BTA_GSE_PUSH_OBJ_IN_PROG_EVT,
    BTA_GSE_SEND_NOTIF_EVT,
    BTA_GSE_PN_OPEN_EVT,
    BTA_GSE_PN_CLOSE_EVT,
    BTA_GSE_PA_START_EVT,
    BTA_GSE_PA_STOP_EVT
};
typedef UINT8 tBTA_GSE_EVT;


typedef struct
{
    tBTA_GP_INST_ID             inst_id;
    tBTA_GP_NOTIF_TYPE          notif_type;
    tBTA_GP_OBJ_HANDLE          handle;
    tBTA_GP_OBJ_TYPE            cal_type;
    char                        summary[BTA_GP_SUMMARY_SIZE+1];
    char                        update[BTA_GP_DATETIME_SIZE+1];  /* In the format of YEAR MONTH DATA T HOURS MINUTES SECONDS "<20120822T100000+02:00>" */
    char                        originator_name[BTA_GP_NAME_LEN+1];
    char                        originator_address[BTA_GP_NAME_LEN+1];
    BD_ADDR                     except_bd_addr;

}tBTA_GSE_SEND_MSG_NOTIF_PARAM;


/* Access event operation types */
#define BTA_GSE_OPER_NONE               0
#define BTA_GSE_OPER_SETPATH            1
#define BTA_GSE_OPER_GET_OBJ_LIST       2
#define BTA_GSE_OPER_GET_OBJ            3
#define BTA_GSE_OPER_SET_OBJ_STATUS     4
#define BTA_GSE_OPER_DEL_OBJ            5
#define BTA_GSE_OPER_PUSH_OBJ           6
#define BTA_GSE_OPER_SET_NOTIF_REG      7
#define BTA_GSE_OPER_SYNC_ACC           8
#define BTA_GSE_OPER_FORWARD_OBJ        9

typedef UINT8 tBTA_GSE_OPER;


/* Structure associated with BTA_GSE_PA_START_EVT or
   BTA_GSE_PA_STOP_EVT                               */
typedef struct
{
    tBTA_GP_STATUS      status;
    UINT8               handle;
    tBTA_GP_INST_ID     inst_id;
} tBTA_GSE_START_STOP;

/* Structure associated with BTA_GSE_PA_OPEN_EVT */
typedef struct
{
    tBTA_GP_INST_ID     instance_id;        /* PAS instance ID: one PAS
                                               instance can support multiple
                                               sessions */
    tBTA_GP_SESS_HANDLE session_id;         /* PAS Session ID, all session based
                                               operation will need to use this ID */
    tBTM_BD_NAME        dev_name;           /* Name of device, "" if unknown */
    BD_ADDR             bd_addr;            /* Address of device */

} tBTA_GSE_PA_OPEN;

/* Structure associated with BTA_GSE_PA_CLOSE_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;             /* status */
    tBTA_GP_INST_ID     instance_id;        /* PAS instance ID: one PAS
                                               instance can support multiple
                                               sessions */
    tBTA_GP_SESS_HANDLE session_id;         /* PAS Session ID, all session based
                                               operation will need to use this ID */
    tBTM_BD_NAME        dev_name;           /* Name of device, "" if unknown */
    BD_ADDR             bd_addr;            /* Address of device */

} tBTA_GSE_PA_CLOSE;

/* Structure associated with BTA_GSE_SYNC_ACC_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;             /* status */
    tBTA_GP_SESS_HANDLE session_id;         /* PAS Session ID, all session based
                                               operation will need to use this ID */
} tBTA_GSE_PA_SYNC_ACC;

/* Structure associated with BTA_GSE_FORWARD_OBJ_EVT */
typedef struct
{
    tBTA_GP_STATUS      status;             /* status */
    tBTA_GP_SESS_HANDLE session_id;         /* PAS Session ID, all session based
                                               operation will need to use this ID */
    UINT8               *p_desc_hdr;        /* Description header */
    tBTA_GP_OBJ_HANDLE  obj_handle;
} tBTA_GSE_PA_FORWARD_OBJ;

/* Structure associated with BTA_GSE_SET_NOTIF_REG_REQ_EVT */
typedef struct
{
    tBTA_GP_SESS_HANDLE session_id;         /* PAS Session ID, all session based
                                               operation will need to use this ID */
     BD_ADDR            bd_addr;            /* remote device's BD address*/
} tBTA_GSE_SET_NOTIF_REG_REQ;

/* Structure associated with BTA_GSE_ACCESS_EVT */
typedef struct
{
    tBTA_GP_SESS_HANDLE         session_id;     /* Session ID */
    tBTA_GSE_OPER               oper;           /* operation */
    char                        *p_name;        /* folder name  */
    tBTM_BD_NAME                dev_name;       /* Name of device, "" if unknown */
    BD_ADDR                     bd_addr;        /* Address of device */
    tBTA_GP_OBJ_HANDLE          handle;         /* object handle */
    tBTA_GP_STATUS_VAL          delete_sts;     /* Delete status */
    tBTA_GP_STATUS_IND_TYPE     set_sts_type;   /* Set object status type */
    tBTA_GP_STATUS_VAL          set_sts_val;    /* Value associated with set object */
} tBTA_GSE_ACCESS;


/* Structure associated with BTA_GSE_NOTIF_REG_EVT */

typedef struct
{
    tBTA_GP_STATUS                  status;         /* status */
    tBTA_GP_INST_ID                 instance_id;    /* PAS Session ID */
    tBTA_GP_NOTIF_STATUS            notif_status;   /* Notification status */
    tBTA_GP_ACOUSTIC_ALARM_STATUS   alarm_status;   /* Alarm status */
    BD_ADDR                         bd_addr;        /* remote device's BD address*/
}tBTA_GSE_NOTIF_REG;

/* Structure associated with BTA_GSE_OPER_CMPL_EVT */
typedef struct
{
    UINT32              obj_size;       /* Total size of object 0 if unknow*/
    tBTA_GP_SESS_HANDLE session_id;     /* Session ID */
    tBTA_GSE_OPER       operation;      /* operation type */
    tBTA_GP_STATUS      status;         /* status */
} tBTA_GSE_OPER_CMPL;

typedef struct
{
    tBTM_BD_NAME    dev_name;               /* Name of peer device, "" if unknown */
    BD_ADDR         bd_addr;                /* Address of peer device */
    tBTA_GP_INST_ID first_instance_id;      /* PN connection can be used for more
                                              than one Instance IDs.
                                              first_instance_id is the Instance Id of a
                                              PAS service GCE first registers to receive the
                                              message notification */
} tBTA_GSE_PN_OPEN;

/* Structure associated with BTA_MSE_SEND_NOTIF_EVT */
typedef struct
{
    tBTA_GP_STATUS          status;         /* status */
    tBTA_GP_INST_ID         inst_id;        /* instance ID */
    BD_ADDR                 bd_addr;        /* Peer BD address */
} tBTA_GSE_PN_SEND_NOTIF;

typedef struct
{
    tBTM_BD_NAME    dev_name;               /* Name of peer device, "" if unknown */
    BD_ADDR         bd_addr;                /* Address of peer device */
} tBTA_GSE_PN_CLOSE;

/* Data associated with call back events */
typedef union
{
    tBTA_GP_STATUS              status;
    tBTA_GSE_START_STOP         start;
    tBTA_GSE_START_STOP         stop;
    tBTA_GSE_PA_OPEN            pa_open;
    tBTA_GSE_PA_CLOSE           pa_close;
    tBTA_GSE_PA_SYNC_ACC        pa_sync_acc;
    tBTA_GSE_PA_FORWARD_OBJ     pa_forward_obj;
    tBTA_GSE_SET_NOTIF_REG_REQ  set_notif_reg;
    tBTA_GSE_ACCESS             access;
    tBTA_GSE_NOTIF_REG          notif_reg;
    tBTA_GSE_OPER_CMPL          oper_cmpl;
    tBTA_GP_IN_PROG             get_obj_in_prog;
    tBTA_GP_IN_PROG             push_obj_in_prog;

    tBTA_GSE_PN_OPEN            pn_open;
    tBTA_GSE_PN_SEND_NOTIF      pn_send_notif;
    tBTA_GSE_PN_CLOSE           pn_close;
} tBTA_GSE;

/* GSE callback function */
typedef void tBTA_GSE_CBACK(tBTA_GSE_EVT event, tBTA_GSE *p_data);

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
** Function         BTA_GseEnable
**
** Description      Enable the GSE subsystems.  This function must be
**                  called before any other functions in the GSE API are called.
**                  When the enable operation is completed the callback function
**                  will be called with an BTA_GSE_ENABLE_EVT event.
**
** Parameters       p_cback - GSE event call back function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseEnable(tBTA_GSE_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_GseDisable
**
** Description      This function disables the GSE subsystem. If the client is
**                  currently connected to a peer device, the connection will be
**
** Parameters       void
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseDisable(void);

/*******************************************************************************
**
** Function         BTA_GseStart
**
** Description      This function starts the generic PIM access server.
**
**
** Parameters       app_id: Differrent instants belonging to the same service
**                          should use the same app_id. app_id starts with 1
**                  service_class: Service class UUID
**                  p_info: Pointer to the service information
**                  p_cback: Pointer to the callback function
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseStart(UINT8 app_id, UINT16 service_class,
                                 tBTA_GSE_START_INFO *p_info,
                                 tBTA_GSE_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_GseSyncAccountRsp
**
** Description      Send a response for the sync account request
**
**
** Parameters       session_id  - PAS session ID
**                  sync_rsp    - sync account is allowed or not
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseSyncAccountRsp(tBTA_GP_SESS_HANDLE session_id,
                                          tBTA_GSE_SYNC_ACC_TYPE sync_rsp);

/*******************************************************************************
**
** Function         BTA_GseForwardObjRsp
**
** Description      Send a response for the forward object request
**
**
** Parameters       session_id  - PAS session ID
**                  forward_rsp - Forward type BTA_GSE_FORWARD_OBJ_ALLOW |
**                                BTA_GSE_FORWARD_OBJ_FORBID
**                  obj_handle  - Object handle to forward
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseForwardObjRsp(tBTA_GP_SESS_HANDLE session_id,
                                         tBTA_GSE_FORWARD_OBJ_TYPE forward_rsp,
                                         tBTA_GP_OBJ_HANDLE obj_handle);

/*******************************************************************************
**
** Function         BTA_GseSetNotifRegRsp
**
** Description      Send a response for set notification registration
**
**
** Parameters       session_id  - PAS session ID
**                  set_notif_reg_type - set notitifcation registration type
**                                       BTA_GSE_SET_NOTIF_REG_ALLOW |
**                                       BTA_GSE_SET_NOTIF_REG_FORBID
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseSetNotifRegRsp(tBTA_GP_SESS_HANDLE session_id,
                                          tBTA_GSE_SET_NOTIF_REG_TYPE set_notif_reg_type);


/*******************************************************************************
**
** Function         BTA_GseSendNotif
**
** Description      Send a notification report to all GCEs registered with
**                  the specified PAS instance ID
**
** Parameters       The following parameters are in the BTA_GSE_API_SEND_NOTIF_TO_ALL_EVT
**                  inst_id             - PAS Instance ID
**                  notif_type          - notification type
**                  handle              - object handle
**                  cal_type            - object type (calendar | tasks | notes)
**                  summary             - notification summary
**                  update              - last update
**                  originator_name     - originator name
**                  originator_address  - originator address
**                  except_bd_addr      - except to the GCE on this BD Address.
**                                        (Note: notification will be not sent to
**                                        this BD Addreess)
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseSendNotif(tBTA_GSE_SEND_MSG_NOTIF_PARAM *p_param);

/*******************************************************************************
**
** Function         BTA_GseAccessRsp
**
** Description      Send a response for the access request
**
** Parameters       session_id  - session ID
**                  oper        - operation type
**                  access      - Access is allowed or not
**                  p_path      - pointer to a path if if the operation
**                                involves accessing a folder
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_GseAccessRsp(tBTA_GP_SESS_HANDLE session_id, tBTA_GSE_OPER oper,
                                     tBTA_GP_ACCESS_TYPE access, char *p_name);

#ifdef __cplusplus
}
#endif



#endif /* BTA_GSE_API_H */
