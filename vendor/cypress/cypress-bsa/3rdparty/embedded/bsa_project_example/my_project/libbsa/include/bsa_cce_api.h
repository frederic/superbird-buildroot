/*****************************************************************************
**
**  Name:           bsa_cce_api.h
**
**  Description:    This is the public interface file for CTN client part of
**                  the Bluetooth simplified API
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BSA_CCE_API_H
#define BSA_CCE_API_H

#include "uipc.h"

/* for tBSA_STATUS */
#include "bsa_status.h"

/*****************************************************************************
**  Constants and Type Definitions
*****************************************************************************/

/* CTN client status code   */
#define BSA_CCE_STATUS_OK               0
#define BSA_CCE_STATUS_FAIL             1   /* Used to pass all other errors */
#define BSA_CCE_STATUS_ABORTED          2
#define BSA_CCE_STATUS_NO_RESOURCE      3
#define BSA_CCE_STATUS_EACCES           4
#define BSA_CCE_STATUS_ENOTEMPTY        5
#define BSA_CCE_STATUS_EOF              6
#define BSA_CCE_STATUS_EODIR            7
#define BSA_CCE_STATUS_ENOSPACE         8   /* Returned in bta_fs_ci_open if no room */
#define BSA_CCE_STATUS_DUPLICATE_ID     9
#define BSA_CCE_STATUS_ID_NOT_FOUND     10
#define BSA_CCE_STATUS_FULL             11  /* reach the max packet size */
#define BSA_CCE_STATUS_UNSUP_FEATURES   12  /* feature is not suppored on the peer device */
#define BSA_CCE_STATUS_SDP_FAIL         20

typedef UINT8 tBSA_CCE_STATUS;


typedef UINT8   tBSA_CCE_INST_ID;

typedef UINT8   tBSA_CCE_HANDLE;
typedef UINT8   tBSA_CNS_HANDLE;

/* Parameter Mask for Object-Listing   */
#define BSA_CCE_OL_MASK_ATTACHMENT          (1<<0)
#define BSA_CCE_OL_MASK_SUMMARY             (1<<1)
#define BSA_CCE_OL_MASK_ENDTIME             (1<<2)
#define BSA_CCE_OL_MASK_ORI_NAME            (1<<3)
#define BSA_CCE_OL_MASK_ORI_ADDR            (1<<4)
#define BSA_CCE_OL_MASK_PRIORITY            (1<<5)
#define BSA_CCE_OL_MASK_PSTATUS             (1<<6)
#define BSA_CCE_OL_MASK_ALARM_STATUS        (1<<7)
#define BSA_CCE_OL_MASK_SEND_STATUS         (1<<8)
#define BSA_CCE_OL_MASK_RECURRENT           (1<<9)

typedef UINT32 tBSA_CCE_OL_MASK;

/* CTN object type */
enum
{
    BSA_CCE_OBJ_TYPE_CALENDAR = 0,
    BSA_CCE_OBJ_TYPE_TASKS,
    BSA_CCE_OBJ_TYPE_NOTES
};
typedef UINT8 tBSA_CCE_OBJ_TYPE;

#define BSA_CCE_DATETIME_SIZE   25   /* In the format of YEAR MONTH DATE T HOURS MINUTES SECONDS "<20120822T100000+02:00>" */

#define BSA_CCE_OBJ_HANDLE_SIZE 16

#define BSA_CCE_RECIPIENT_SIZE  256

typedef UINT8 tBSA_CCE_OBJ_HANDLE[BSA_CCE_OBJ_HANDLE_SIZE];

/* Attachment type */
#define BSA_CCE_ATTACHMENT_ON       0x00
#define BSA_CCE_ATTACHMENT_OFF      0x01
#define BSA_CCE_ATTACHMENT_SELECT   0x10

typedef UINT8 tBSA_CCE_ATTACHMENT;

/* Status indicator */
#define BSA_CCE_STATUS_IND_PSTATUS      0x00    /* pstatus */
#define BSA_CCE_STATUS_IND_ALARM_STS    0x01    /* alarm status */
#define BSA_CCE_STATUS_IND_SEND_STS     0x02    /* send status */
#define BSA_CCE_STATUS_IND_DELETE_STS   0x03    /* delete status */

typedef UINT8 tBSA_CCE_STATUS_IND;

/* Status value associated with status indicator */
#define BSA_CCE_STATUS_VAL_NO           0x00    /* no for alarmstatus, sendstatus and deletestatus */
#define BSA_CCE_STATUS_VAL_YES          0x01    /* yes for alarmstatus, sendstatus */
#define BSA_CCE_STATUS_VAL_POSTPONE     0x02    /* postpone for alarmstatus */
#define BSA_CCE_STATUS_VAL_TENTATIVE    0x03    /* tentative for pstatus */
#define BSA_CCE_STATUS_VAL_NEEDS_ACTION 0x04    /* needs-action for pstatus */
#define BSA_CCE_STATUS_VAL_ACCEPTED     0x05    /* accepted for pstatus */
#define BSA_CCE_STATUS_VAL_DECLINED     0x06    /* declined for pstatus */
#define BSA_CCE_STATUS_VAL_DELEGATED    0x07    /* delegated for pstatus */
#define BSA_CCE_STATUS_VAL_COMPLETED    0x08    /* completed for pstatus */
#define BSA_CCE_STATUS_VAL_IN_PROGRESS  0x09    /* in-progress for pstatus */

typedef UINT8 tBSA_CCE_STATUS_VAL;

#define BSA_CCE_POSTPONE_VAL_IGNORE     0xFFFF  /* this value is used when status ind is not alarm_status */

typedef UINT32 tBSA_CCE_POSTPONE_VAL;           /* Indicate the number of minutes the alarm shall be postponed */

#define BSA_CCE_MAX_SERVICE_NAME        35

#define BSA_CCE_MAX_PATH                260

/* Supported features */
#define BSA_CCE_SUP_FEA_ACC_MAN         0x00000001
#define BSA_CCE_SUP_FEA_NOTIF           0x00000002
#define BSA_CCE_SUP_FEA_BROWSING        0x00000004
#define BSA_CCE_SUP_FEA_DOWNLOADING     0x00000008
#define BSA_CCE_SUP_FEA_UPLOADING       0x00000010
#define BSA_CCE_SUP_FEA_DELETE          0x00000020
#define BSA_CCE_SUP_FEA_FORWARD         0x00000040

typedef UINT32 tBSA_CCE_SUP_FEA_MASK;

/* Notification status */
enum
{
    BSA_CCE_NOTIF_OFF = 0,
    BSA_CCE_NOTIF_ON
};
typedef UINT8 tBSA_CCE_NOTIF_STATUS;

/* Acoustic alarm status */
enum
{
    BSA_CCE_ACOUSTIC_ALARM_OFF = 0,
    BSA_CCE_ACOUSTIC_ALARM_ON
};
typedef UINT8 tBSA_CCE_ACOUSTIC_ALARM_STATUS;

#define BSA_CCE_MAX_ACCOUNT_NAME        17

#define BSA_CCE_MAX_NUM_ACCOUNT         6

#define BSA_CCE_MAX_ACCOUNT_INFO_LEN    200

typedef char tBSA_CCE_ACCOUNT_INFO[BSA_CCE_MAX_ACCOUNT_INFO_LEN];

typedef UINT8 tBSA_BD_NAME[24];

/* BSA CTN Client callback events */
typedef enum
{
    BSA_CCE_ENABLE_EVT,
    BSA_CCE_DISABLE_EVT,
    BSA_CCE_GET_ACCOUNTS_EVT,
    BSA_CCE_OPEN_EVT,
    BSA_CCE_CLOSE_EVT,
    BSA_CCE_GET_LISTING_EVT,
    BSA_CCE_GET_OBJECT_EVT,
    BSA_CCE_SET_STATUS_EVT,
    BSA_CCE_PUSH_DATA_REQ_EVT,
    BSA_CCE_PUSH_OBJECT_EVT,
    BSA_CCE_FORWARD_OBJECT_EVT,
    BSA_CCE_GET_ACCOUNT_INFO_EVT,
    BSA_CCE_SYNC_ACCOUNT_EVT,
    BSA_CCE_NOTIF_REG_EVT,
    BSA_CCE_CNS_START_EVT,
    BSA_CCE_CNS_STOP_EVT,
    BSA_CCE_CNS_OPEN_EVT,
    BSA_CCE_CNS_CLOSE_EVT,
    BSA_CCE_CNS_NOTIF_EVT
} tBSA_CCE_EVT;

typedef struct
{
    char                    name[BSA_CCE_MAX_ACCOUNT_NAME];
    tBSA_CCE_INST_ID        inst_id;
    UINT16                  version;
    UINT16                  psm;
    tBSA_CCE_SUP_FEA_MASK   peer_features;
} tBSA_CCE_ACCOUNT_REC;

typedef struct
{
    tBSA_CCE_STATUS         status;
    BD_ADDR                 bd_addr;
    UINT8                   num_rec;
    tBSA_CCE_ACCOUNT_REC    rec[BSA_CCE_MAX_NUM_ACCOUNT];
} tBSA_CCE_GET_ACCOUNTS_MSG;

typedef struct
{
    tBSA_CCE_STATUS         status;
    tBSA_CCE_INST_ID        inst_id;
    tBSA_BD_NAME            dev_name;    /* Name of device, "" if unknown */
    BD_ADDR                 bd_addr;
    tBSA_CCE_HANDLE         cce_handle;
    tBSA_CCE_SUP_FEA_MASK   peer_feature;
} tBSA_CCE_OPEN_CLOSE_MSG;

typedef union
{
    UINT16      list_size;
    UINT8       ltime[BSA_CCE_DATETIME_SIZE+1]; /* "yyyymmddTHHMMSS+HH:MM", or "" if none */
}tBSA_CCE_LIST_APP_PARAM;

typedef struct
{
    tBSA_CCE_STATUS         status;
    UINT8                   obx_rsp_code;       /* obex response code */
    tBSA_CCE_HANDLE         cce_handle;
    UINT8                   *p_data;
    UINT16                  data_len;
    tBSA_CCE_LIST_APP_PARAM param;              /* for BSA_CCE_GET_LISTING_EVT only */
    BOOLEAN                 is_final;
} tBSA_CCE_GET_MSG;

typedef struct
{
    tBSA_CCE_STATUS         status;
    UINT8                   obx_rsp_code;       /* obex response code */
    tBSA_CCE_HANDLE         cce_handle;
    tBSA_CCE_OBJ_HANDLE     obj_handle;         /* for BSA_CCE_PUSH_OBJECT_EVT only */
} tBSA_CCE_SET_MSG;

typedef struct
{
    tBSA_CCE_HANDLE         cce_handle;
    UINT16                  nbytes;
} tBSA_CCE_PUSH_DATA_REQ_MSG;

typedef struct
{
    tBSA_CCE_STATUS         status;
    UINT8                   obx_rsp_code;       /* obex response code */
    tBSA_CCE_INST_ID        inst_id;
    tBSA_CCE_ACCOUNT_INFO   account_info;
    tBSA_CCE_HANDLE         cce_handle;
} tBSA_CCE_GET_ACCOUNT_INFO_MSG;

typedef struct
{
    tBSA_CCE_STATUS         status;
    tBSA_CNS_HANDLE         cns_handle;
} tBSA_CCE_CNS_START_STOP_MSG;

typedef struct
{
    tBSA_CCE_STATUS         status;
    tBSA_BD_NAME            dev_name;           /* Name of device, "" if unknown */
    BD_ADDR                 bd_addr;
    UINT16                  cns_sess_handle;
} tBSA_CCE_CNS_OPEN_CLOSE_MSG;

typedef struct
{
    tBSA_CCE_STATUS         status;
    UINT16                  cns_sess_handle;
    UINT8                   inst_id;
    UINT8                   *p_data;
    UINT16                  data_len;
    BOOLEAN                 is_final;
} tBSA_CCE_CNS_NOTIF_MSG;

/* Data for all CCE events */
typedef union
{
    tBSA_CCE_STATUS                 status;
    tBSA_CCE_GET_ACCOUNTS_MSG       get_accounts;
    tBSA_CCE_OPEN_CLOSE_MSG         open;
    tBSA_CCE_OPEN_CLOSE_MSG         close;
    tBSA_CCE_GET_MSG                get_listing;
    tBSA_CCE_GET_MSG                get_object;
    tBSA_CCE_SET_MSG                set_status;
    tBSA_CCE_PUSH_DATA_REQ_MSG      push_data_req;
    tBSA_CCE_SET_MSG                push_object;
    tBSA_CCE_SET_MSG                forward_object;
    tBSA_CCE_GET_ACCOUNT_INFO_MSG   get_account_info;
    tBSA_CCE_SET_MSG                sync_account;
    tBSA_CCE_SET_MSG                notif_reg;
    tBSA_CCE_CNS_START_STOP_MSG     cns_start;
    tBSA_CCE_CNS_START_STOP_MSG     cns_stop;
    tBSA_CCE_CNS_OPEN_CLOSE_MSG     cns_open;
    tBSA_CCE_CNS_OPEN_CLOSE_MSG     cns_close;
    tBSA_CCE_CNS_NOTIF_MSG          cns_notif;
} tBSA_CCE_MSG;

/* BSA PBC callback function */
typedef void (tBSA_CCE_CBACK)(tBSA_CCE_EVT event, tBSA_CCE_MSG *p_data);

/*
* Structures used to pass parameters to BSA API functions
*/

typedef struct
{
    tBSA_CCE_CBACK  *p_cback;
} tBSA_CCE_ENABLE;

typedef struct
{
    int dummy;
} tBSA_CCE_DISABLE;

typedef struct
{
    BD_ADDR         bd_addr;
} tBSA_CCE_GET_ACCOUNTS;

typedef struct
{
    BD_ADDR             bd_addr;
    tBSA_CCE_INST_ID    instance_id;
    tBSA_SEC_AUTH       sec_mask;
} tBSA_CCE_OPEN;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
} tBSA_CCE_CLOSE;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
    tBSA_CCE_OBJ_TYPE   obj_type;
    tBSA_CCE_OL_MASK    parameter_mask;
    UINT16              max_list_cnt;
    UINT16              list_start_offset;
    char                period_begin[BSA_CCE_DATETIME_SIZE+1];   /* "yyyymmddTHHMMSS+HH:MM", or "" if none */
    char                period_end[BSA_CCE_DATETIME_SIZE+1];     /* "yyyymmddTHHMMSS+HH:MM", or "" if none */
} tBSA_CCE_GET_LISTING;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
    tBSA_CCE_OBJ_HANDLE obj_handle;
    BOOLEAN             recurrent;
    tBSA_CCE_ATTACHMENT attachment;
    UINT8               attachment_id;
} tBSA_CCE_GET_OBJECT;

typedef struct
{
    tBSA_CCE_HANDLE         cce_handle;
    tBSA_CCE_OBJ_HANDLE     obj_handle;
    tBSA_CCE_STATUS_IND     status_ind;
    tBSA_CCE_STATUS_VAL     status_val;
    tBSA_CCE_POSTPONE_VAL   postpone_val;
} tBSA_CCE_SET_STATUS;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
    tBSA_CCE_OBJ_TYPE   obj_type;
    BOOLEAN             send;
    char                path[BSA_CCE_MAX_PATH];
} tBSA_CCE_PUSH_OBJECT;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
    UINT8               *p_data;
    UINT16              data_len;
    BOOLEAN             is_final;
} tBSA_CCE_PUSH_OBJECT_DATA;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
    tBSA_CCE_OBJ_HANDLE obj_handle;
    char                recipients[BSA_CCE_RECIPIENT_SIZE];
} tBSA_CCE_FORWARD_OBJECT;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
    tBSA_CCE_INST_ID    instance_id;
} tBSA_CCE_GET_ACCOUNT_INFO;

typedef struct
{
    tBSA_CCE_HANDLE     cce_handle;
} tBSA_CCE_SYNC_ACCOUNT;

typedef struct
{
    char                    service_name[BSA_CCE_MAX_SERVICE_NAME + 1];
    tBSA_SEC_AUTH           sec_mask;
    tBSA_CCE_SUP_FEA_MASK   features;
} tBSA_CCE_CNS_START;

typedef struct
{
    tBSA_CNS_HANDLE     cns_handle;
} tBSA_CCE_CNS_STOP;

typedef struct
{
    tBSA_CCE_HANDLE                 cce_handle;
    tBSA_CCE_NOTIF_STATUS           notif_status;
    tBSA_CCE_ACOUSTIC_ALARM_STATUS  alarm_status;
} tBSA_CCE_NOTIF_REG;


/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         BSA_CceEnableInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceEnableInit(tBSA_CCE_ENABLE *p_enable);

/*******************************************************************************
**
** Function         BSA_CceEnable
**
** Description      This function enables CTN client and registers CTN with
**                  lower layers.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceEnable(tBSA_CCE_ENABLE *p_enable);

/*******************************************************************************
**
** Function         BSA_CceDisableInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceDisableInit(tBSA_CCE_DISABLE *p_disable);

/*******************************************************************************
**
** Function         BSA_CceDisable
**
** Description      This function is called when the host is about power down.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceDisable(tBSA_CCE_DISABLE *p_disable);

/*******************************************************************************
**
** Function         BSA_CceGetAccountsInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetAccountsInit(tBSA_CCE_GET_ACCOUNTS *p_get);

/*******************************************************************************
**
** Function         BSA_CceGetAccounts
**
** Description      Discover CTN accounts on CSE.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetAccounts(tBSA_CCE_GET_ACCOUNTS *p_get);

/*******************************************************************************
**
** Function         BSA_CceOpenInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceOpenInit(tBSA_CCE_OPEN *p_open);

/*******************************************************************************
**
** Function         BSA_CceOpen
**
** Description      Open a connection to a CSE.
**
**                  When the connection is open the callback function
**                  will be called with a BSA_CCE_OPEN_EVT.  If the connection
**                  fails or otherwise is closed the callback function will be
**                  called with a BSA_CCE_CLOSE_EVT.
**
**                  Note: CCE always enable (BSA_SEC_AUTHENTICATE | BSA_SEC_ENCRYPT)
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceOpen(tBSA_CCE_OPEN *p_open);

/*******************************************************************************
**
** Function         BSA_CceCloseInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceCloseInit(tBSA_CCE_CLOSE *p_close);

/*******************************************************************************
**
** Function         BSA_CceClose
**
** Description      Close the current connection to the server. Aborts any
**                  active CTN transfer.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceClose(tBSA_CCE_CLOSE *p_close);

/*******************************************************************************
**
** Function         BSA_CceGetListingInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetListingInit(tBSA_CCE_GET_LISTING *p_get);

/*******************************************************************************
**
** Function         BSA_CceGetListing
**
** Description      Get CTN object listing.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetListing(tBSA_CCE_GET_LISTING *p_get);

/*******************************************************************************
**
** Function         BSA_CceGetObjectInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetObjectInit(tBSA_CCE_GET_OBJECT *p_get);

/*******************************************************************************
**
** Function         BSA_CceGetObject
**
** Description      Get a CTN object from CSE.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetObject(tBSA_CCE_GET_OBJECT *p_get);

/*******************************************************************************
**
** Function         BSA_CceSetStatusInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceSetStatusInit(tBSA_CCE_SET_STATUS *p_set);

/*******************************************************************************
**
** Function         BSA_CceSetStatus
**
** Description      Modify the status of a CTN object on CSE.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceSetStatus(tBSA_CCE_SET_STATUS *p_set);

/*******************************************************************************
**
** Function         BSA_CcePushObjectInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CcePushObjectInit(tBSA_CCE_PUSH_OBJECT *p_push);

/*******************************************************************************
**
** Function         BSA_CcePushObject
**
** Description      Start to push a CTN object to CSE.  Data will be sent in BSA_CcePushObjectData()
**                  when stack send back BSA_CCE_PUSH_DATA_REQ_EVT.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CcePushObject(tBSA_CCE_PUSH_OBJECT *p_push);

/*******************************************************************************
**
** Function         BSA_CcePushObjectDataInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CcePushObjectDataInit(tBSA_CCE_PUSH_OBJECT_DATA *p_data);

/*******************************************************************************
**
** Function         BSA_CcePushObjectData
**
** Description      Send CTN object data.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CcePushObjectData(tBSA_CCE_PUSH_OBJECT_DATA *p_data);

/*******************************************************************************
**
** Function         BSA_CceForwardObjectInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceForwardObjectInit(tBSA_CCE_FORWARD_OBJECT *p_forward);

/*******************************************************************************
**
** Function         BSA_CceForwardObject
**
** Description      Forward a CTN object on CSE to one or more recipients.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceForwardObject(tBSA_CCE_FORWARD_OBJECT *p_forward);

/*******************************************************************************
**
** Function         BSA_CceGetAccountInfoInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetAccountInfoInit(tBSA_CCE_GET_ACCOUNT_INFO *p_get);

/*******************************************************************************
**
** Function         BSA_CceGetAccountInfo
**
** Description      Get readable information about a CTN account from CSE.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceGetAccountInfo(tBSA_CCE_GET_ACCOUNT_INFO *p_get);

/*******************************************************************************
**
** Function         BSA_CceSyncAccountInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceSyncAccountInit(tBSA_CCE_SYNC_ACCOUNT *p_sync);

/*******************************************************************************
**
** Function         BSA_CceSyncAccount
**
** Description      Synchronize the CSE account with an external server.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceSyncAccount(tBSA_CCE_SYNC_ACCOUNT *p_sync);

/*******************************************************************************
**
** Function         BSA_CceCnsStartInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceCnsStartInit(tBSA_CCE_CNS_START *p_start);

/*******************************************************************************
**
** Function         BSA_CceCnsStart
**
** Description      Start the CTN Notification service server.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceCnsStart(tBSA_CCE_CNS_START *p_start);

/*******************************************************************************
**
** Function         BSA_CceCnsStopInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceCnsStopInit(tBSA_CCE_CNS_STOP *p_stop);

/*******************************************************************************
**
** Function         BSA_CceCnsStop
**
** Description      Stop the CTN Notification service server.  If the server is currently
**                  connected to a peer device the connection will be closed.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceCnsStop(tBSA_CCE_CNS_STOP *p_stop);

/*******************************************************************************
**
** Function         BSA_CceNotifRegInit
**
** Description      Initialize structure containing API parameters with default values
**
** Parameters       Pointer on structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceNotifRegInit(tBSA_CCE_NOTIF_REG *p_reg);

/*******************************************************************************
**
** Function         BSA_CceNotifReg
**
** Description      Register for notifications indicating changes in CSE's object repository.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_CceNotifReg(tBSA_CCE_NOTIF_REG *p_reg);

#ifdef __cplusplus
}
#endif

#endif
