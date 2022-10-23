/*****************************************************************************
**
**  Name:           bta_gp_def.h
**
**  Description:    This file contains the common definitions for the Generic
**                  PIM access profile (GPP) related SW modules
**
**  Copyright (c) 2014-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GP_DEF_H
#define BTA_GP_DEF_H

#include "obx_api.h"
#include "bta_api.h"
#include "btm_api.h"
#include "bta_sys.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#ifndef BTA_GP_INST_INFO_MAX_LEN
#define BTA_GP_INST_INFO_MAX_LEN    200
#endif

#define BTA_GP_FILE_PATH_SIZE       305

#define BTA_GP_MAX_MAX_LIST_COUNT   1024    /* According to GPP spec */

#define  BTA_GP_OBJ_HANDLE_SIZE     16      /* 16 UINT8, One UINT8 is 2 byte, 128/8 = 16 */

#define BTA_GP_128BIT_HEX_STR_SIZE  (BTA_GP_OBJ_HANDLE_SIZE*2+1)

#define BTA_GP_INVALID_HANDLE       0xFF

typedef UINT8  tBTA_GP_OBJ_HANDLE[BTA_GP_OBJ_HANDLE_SIZE];

typedef tOBX_HANDLE     tBTA_GP_SESS_HANDLE;

#define BTA_GP_DATETIME_SIZE        25   /* In the format of YEAR MONTH DATE T HOURS MINUTES SECONDS "<20120822T100000+02:00>" */
#define BTA_GP_SUMMARY_SIZE         256  /* 255 according to CTN 1.0 */
#define BTA_GP_PRIORITY_SIZE        10   /* Either "<high>", "<Normal>" or "<low>" */
#define BTA_GP_PARTSTAT_SIZE        20   /* Longest "<NEEDS-ACTION>" */
#define BTA_GP_ALARM_STATUS_SIZE    7    /* either "<on>" or "<off>" */
#define BTA_GP_NAME_LEN             256  /* 255 according to CTN 1.0 */


/* Notification Type */
enum
{
    BTA_GP_NOTIF_TYPE_NEW_MSG = 0,
    BTA_GP_NOTIF_TYPE_OBJECT_UPDATE,
    BTA_GP_NOTIF_TYPE_ALARM,
    BTA_GP_NOTIF_TYPE_SENDING_SUCCESS,
    BTA_GP_NOTIF_TYPE_SENDING_FAILURE,
    BTA_GP_NOTIF_TYPE_DELIVERY_SUCCESS,
    BTA_GP_NOTIF_TYPE_DELIVERY_FAILURE,
    BTA_GP_NOTIF_TYPE_MEMORY_FULL,
    BTA_GP_NOTIF_TYPE_MEMORY_AVAILABLE,
    BTA_GP_NOTIF_TYPE_OBJECT_DELETED,
    BTA_GP_NOTIF_TYPE_MAX
};

typedef UINT8 tBTA_GP_NOTIF_TYPE;

/* Parameter Mask for Object-Listing   */
#define BTA_GP_CTN_OL_MASK_ATTACHMENT       (1<<0)
#define BTA_GP_CTN_OL_MASK_SUMMARY          (1<<1)
#define BTA_GP_CTN_OL_MASK_ENDTIME          (1<<2)
#define BTA_GP_CTN_OL_MASK_ORI_NAME         (1<<3)
#define BTA_GP_CTN_OL_MASK_ORI_ADDR         (1<<4)
#define BTA_GP_CTN_OL_MASK_PRIORITY         (1<<5)
#define BTA_GP_CTN_OL_MASK_PSTATUS          (1<<6)
#define BTA_GP_CTN_OL_MASK_ALARM_STATUS     (1<<7)
#define BTA_GP_CTN_OL_MASK_SEND_STATUS      (1<<8)
#define BTA_GP_CTN_OL_MASK_RECURRENT        (1<<9)

typedef UINT32 tBTA_GP_CTN_OL_MASK;

/* Bit mask for status indicator */
#define BTA_GP_STATUS_IND_TYPE_PSTATUS      0x00    /* pstatus */
#define BTA_GP_STATUS_IND_TYPE_ALARM_STS    0x01    /* alarm status */
#define BTA_GP_STATUS_IND_TYPE_SEND_STS     0x02    /* send status */
#define BTA_GP_STATUS_IND_TYPE_DELETE_STS   0x03    /* delete status */

typedef UINT8 tBTA_GP_STATUS_IND_TYPE;

/* Bit mask for status value associated with status indicator */
#define BTA_GP_STATUS_VAL_NO            0x00    /* no for alarmstatus, sendstatus and deletestatus */
#define BTA_GP_STATUS_VAL_YES           0x01    /* yes for alarmstatus, sendstatus */
#define BTA_GP_STATUS_VAL_POSTPONE      0x02    /* postpone for alarmstatus */
#define BTA_GP_STATUS_VAL_TENTATIVE     0x03    /* tentative for pstatus */
#define BTA_GP_STATUS_VAL_NEEDS_ACTION  0x04    /* needs-action for pstatus */
#define BTA_GP_STATUS_VAL_ACCEPTED      0x05    /* accepted for pstatus */
#define BTA_GP_STATUS_VAL_DECLINED      0x06    /* declined for pstatus */
#define BTA_GP_STATUS_VAL_DELEGATED     0x07    /* delegated for pstatus */
#define BTA_GP_STATUS_VAL_COMPLETED     0x08    /* completed for pstatus */
#define BTA_GP_STATUS_VAL_IN_PROGRESS   0x09    /* in-progress for pstatus */

typedef UINT8 tBTA_GP_STATUS_VAL;


#define BTA_GP_POSTPONE_VAL_IGNORE      0xFFFF  /* this value is used when status ind is not alarm_status */
typedef UINT32 tBTA_GP_POSTPONE_VAL;            /* Indicate the number of minutes the alarm shall be postponed */

/* get or push object multi-packet status */
enum
{
    BTA_GP_MPKT_STATUS_MORE = 0,
    BTA_GP_MPKT_STATUS_LAST,
    BTA_GP_MPKT_MAX
};

typedef UINT8 tBTA_GP_MPKT_STATUS;

/* Calendar type see SDP supported message type */
#define BTA_GP_OBJ_TYPE_CALENDAR         (1<<0)
#define BTA_GP_OBJ_TYPE_TASK             (1<<1)
#define BTA_GP_OBJ_TYPE_NOTE             (1<<2)

typedef UINT32 tBTA_GP_OBJ_TYPE;

/* Access response types */
enum
{
    BTA_GP_ACCESS_TYPE_ALLOW = 0,    /* Allow the requested operation */
    BTA_GP_ACCESS_TYPE_FORBID        /* Disallow the requested operation */
};

typedef UINT8 tBTA_GP_ACCESS_TYPE;

/* notification status */
enum
{
    BTA_GP_NOTIF_OFF = 0,
    BTA_GP_NOTIF_ON,
    BTA_GP_NOTIF_MAX

};
typedef UINT8 tBTA_GP_NOTIF_STATUS;

/* Acoustic alarm status */
enum
{
    BTA_GP_ALARM_OFF = 0,
    BTA_GP_ALARM_ON,
    BTA_GP_ALARM_MAX

};
typedef UINT8 tBTA_GP_ACOUSTIC_ALARM_STATUS;

#define BTA_GP_RETRY_OFF        0
#define BTA_GP_RETRY_ON         1
#define BTA_GP_RETRY_UNKNOWN    0xff
typedef UINT8   tBTA_GP_RETRY_TYPE;


#define BTA_GP_TRANSP_OFF        0
#define BTA_GP_TRANSP_ON         1
#define BTA_GP_TRANSP_UNKNOWN    0xff
typedef UINT8   tBTA_GP_TRANSP_TYPE;

/* enum for charset used in GetMEssage */
enum
{
    BTA_GP_CHARSET_NATIVE = 0,
    BTA_GP_CHARSET_UTF_8  = 1,
    BTA_GP_CHARSET_UNKNOWN,
    BTA_GP_CHARSET_MAX
};
typedef UINT8 tBTA_GP_CHARSET;

/* Structure for in progress related event*/
typedef struct
{
    UINT32              obj_size;   /* Total size of object 0 if unknow*/
    tBTA_GP_SESS_HANDLE session_id;
    UINT16              bytes;      /* Number of bytes read or written since last progress event */
} tBTA_GP_IN_PROG;


#define BTA_GP_ATTACHMENT_VALUE_ON                0x00
#define BTA_GP_ATTACHMENT_VALUE_OFF               0x01
#define BTA_GP_ATTACHMENT_VALUE_SELECT            0x10

typedef UINT8 tBTA_GP_ATTACHMENT_VALUE;

#define BTA_GP_NOTIF_STS_ON         0x01
#define BTA_GP_NOTIF_STS_OFF        0x00


#define BTA_GP_CAS_TARGET_UUID       "\x11\x5F\x1E\xC2\x29\x22\x11\xE4\xB6\x5D\xA6\xC5\xE4\xD2\x2F\xB7" /* CTN Access Service */
#define BTA_GP_CNS_TARGET_UUID       "\x45\xD6\x8D\x34\x29\x22\x11\xE4\xB6\x5D\xA6\xC5\xE4\xD2\x2F\xB7" /* CTN Notification Service */
#define BTA_GP_TARGET_UUID_LENGTH   16

#define BTA_GP_VERSION_1_0          0x0100      /* GPP 1.0 */

#define BTA_GP_MAX_NAME_LEN_PER_LINE    17

typedef char tBTA_GP_INST_INFO[BTA_GP_INST_INFO_MAX_LEN];


typedef UINT8           tBTA_GP_INST_ID;


#define BTA_GP_SUP_FEA_ACC_MAN      0x00000001
#define BTA_GP_SUP_FEA_NOTIF        0x00000002
#define BTA_GP_SUP_FEA_BROWSING     0x00000004
#define BTA_GP_SUP_FEA_DOWNLOADING  0x00000008
#define BTA_GP_SUP_FEA_UPLOADING    0x00000010
#define BTA_GP_SUP_FEA_DELETE       0x00000020
#define BTA_GP_SUP_FEA_FORWARD      0x00000040

typedef UINT32          tBTA_GP_SUP_FEA_MASK;


/* GPP tag ID in application parameters header definition */
/* CTN tag ID */
#define BTA_GP_APH_ACOUSTIC_ALARM   0x01    /* Acoustic Alarm status 1 byte: 0, 1*/
#define BTA_GP_APH_ATTACH           0x02    /* Attachment 1 byte: 0, 1 , 2 */
#define BTA_GP_APH_SEND             0x03    /* Send 1 byte: 0, 1 */
#define BTA_GP_APH_FILTER_PRD_BEGIN 0x04    /* Filter Period Begin  variable */
#define BTA_GP_APH_FILTER_PRD_END   0x05    /* Filter Period End    variable */
#define BTA_GP_APH_PARAM_MASK       0x06    /* Parameter mask    2 bytes  */
#define BTA_GP_APH_STS_INDCTR       0x07    /* Status Indicator    1 byte  */
#define BTA_GP_APH_STS_VALUE        0x08    /* Status Value    1 byte: 0, 1  */
#define BTA_GP_APH_POST_VALUE       0x09    /* Postpone Value 4 byte */
#define BTA_GP_APH_EMAIL_URI        0x0a    /* E-mail URI variable */
#define BTA_GP_APH_GPP_TIME         0x0b    /* GPPTime variable  */
#define BTA_GP_APH_RECURRENT        0x0c    /* Recurrent 1 byte  */
#define BTA_GP_APH_ATTACH_ID        0x0d    /* Attachment ID 1 byte: 0, 1 , 2 */
#define BTA_GP_APH_LAST_UPDATE      0x0e    /* String about last update variable */

/* Generic GPP tag ID*/
#define BTA_GP_APH_MAX_LIST_COUNT   0x41    /* MaxListCount     2 bytes     0x0000 to 0xFFFF */
#define BTA_GP_APH_START_STOFF      0x42    /* StartOffset      2 bytes     0x0000 to 0xFFFF */
#define BTA_GP_APH_NOTIF_STATUS     0x43    /* Notification Status 1 byte */
#define BTA_GP_APH_INST_ID          0x44    /* GPP instance ID 1 byte  0 ... 255 */
#define BTA_GP_APH_LST_SIZE         0x46    /* Listing Size    2 bytes  */

#define BTA_GP_BODY_FILLER_BYTE     0x30    /* GPP body filter byte */

#define BTA_GP_APH_INST_ID_LEN      1

/* CTN type header */
#define BTA_GP_HDR_TYPE_CTN_NOTIF_REG   "x-bt/CTN-NotificationRegistration"
#define BTA_GP_HDR_TYPE_EVENT_RPT       "x-bt/CTN-EventReport"
#define BTA_GP_HDR_TYPE_LIST            "x-bt/CTN-Listing"
#define BTA_GP_HDR_TYPE_CALENDAR        "x-bt/Calendar"
#define BTA_GP_HDR_TYPE_CALENDAR_STATUS "x-bt/CalendarStatus"
#define BTA_GP_HDR_TYPE_FORWARD         "x-bt/CTN-forward"
#define BTA_GP_HDR_TYPE_SYNC            "x-bt/Update"
#define BTA_GP_HDR_TYPE_INST_INFO       "x-bt/InstanceDescription"


#define BTA_GP_STATUS_OK                0
#define BTA_GP_STATUS_FAIL              1   /* Used to pass all other errors */
#define BTA_GP_STATUS_ABORTED           2
#define BTA_GP_STATUS_NO_RESOURCE       3
#define BTA_GP_STATUS_EACCES            4
#define BTA_GP_STATUS_ENOTEMPTY         5
#define BTA_GP_STATUS_EOF               6
#define BTA_GP_STATUS_EODIR             7
#define BTA_GP_STATUS_ENOSPACE          8   /* Returned in bta_fs_ci_open if no room */
#define BTA_GP_STATUS_DUPLICATE_ID      9
#define BTA_GP_STATUS_ID_NOT_FOUND      10
#define BTA_GP_STATUS_FULL              11  /* reach the max packet size */
#define BTA_GP_STATUS_UNSUP_FEATURES    12  /* feature is not suppored on the peer device */

#define BTA_GP_STATUS_SDP_FAIL          20

typedef UINT8 tBTA_GP_STATUS;


/* Calendar type see SDP supported calendar type */
#define BTA_GP_CAL_TYPE_EVENT           (1<<0)
#define BTA_GP_CAL_TYPE_TASK            (1<<1)
#define BTA_GP_CAL_TYPE_NOTE            (1<<2)
typedef UINT8 tBTA_GP_CAL_TYPE;



typedef struct
{
    char                    *p_folder;  /* current or child folder
                                         for current folder set
                                         *p_folder = ""
                                      */
    char                    *p_path; /* for GCE use only*/
    tBTA_GP_STATUS_VAL      is_send;
    char                    *p_obj_type;
} tBTA_GP_PUSH_OBJ_PARAM;


typedef struct
{
    tBTA_GP_ATTACHMENT_VALUE    attachment;
    tBTA_GP_OBJ_HANDLE          obj_handle;
    BOOLEAN                     recurrent;
    UINT8                       attachment_id;
    char                        *p_obj_type;
} tBTA_GP_GET_OBJ_PARAM;

typedef struct
{

    tBTA_GP_CTN_OL_MASK     parameter_mask;
    UINT16                  max_list_cnt;
    UINT16                  list_start_offset;
    char                    period_begin[BTA_GP_DATETIME_SIZE+1];   /* "yyyymmddTHHMMSS+HH:MM", or "" if none */
    char                    period_end[BTA_GP_DATETIME_SIZE+1];     /* "yyyymmddTHHMMSS+HH:MM", or "" if none */
} tBTA_GP_CTN_OL_FILTER_PARAM;

typedef union
{
    tBTA_GP_CTN_OL_FILTER_PARAM param;
} tBTA_GP_OL_FILTER_PARAM;


#endif /* BTA_GP_DEF_H */
