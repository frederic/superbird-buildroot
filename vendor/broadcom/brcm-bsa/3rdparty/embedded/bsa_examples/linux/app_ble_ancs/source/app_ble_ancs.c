/*****************************************************************************
**
**  Name:           app_ble_ancs.c
**
**  Description:    BLE ANCS Notification Consumer application
**
**  Copyright (c) 2016, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "app_ble.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_tm_vse.h"
#include "app_dm.h"
#include "app_ble_client.h"
#include "app_ble_client_db.h"
#include "app_ble_ancs.h"

#include "app_thread.h"

/*
 * Global Variables
 */
/* BLE ANCS app UUID */
#define APP_BLE_ANCS_APP_UUID      0x9996
static tBT_UUID app_ble_ancs_app_uuid = {2, {APP_BLE_ANCS_APP_UUID}};

/*7905F431-B5CE-4E99-A40F-4B1E122D00D0*/
const char ANCS_SERVICE[]             = {0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79};

/*Notification Source: UUID 9FBF120D-6301-42D9-8C58-25E699A21DBD (notifiable)*/
const char ANCS_NOTIFICATION_SOURCE[] = {0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C, 0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F};

/* Control Point: UUID 69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9 (writeable with response) */
const char ANCS_CONTROL_POINT[]       = {0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98, 0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69};

/* Data Source: UUID 22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB (notifiable) */
const char ANCS_DATA_SOURCE[]         = {0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE, 0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22};

BOOLEAN g_bRegDataSrcNotfnPending = FALSE;
BOOLEAN g_bDeRegDataSrcNotfnPending = FALSE;

static char *EventId[] =
{
    "Added",
    "Modified",
    "Removed",
    "Unknown"
};

#define ANCS_CATEGORY_ID_MAX    12
static char *CategoryId[] =
{
    "Other",
    "IncomingCall",
    "MissedCall",
    "Voicemail",
    "Social",
    "Schedule",
    "Email",
    "News",
    "HealthAndFitness",
    "BusinessAndFinance",
    "Location",
    "Entertainment",
    "Unknown"
};

static char *NotificationAttributeID[] =
{
    "AppIdentifier",
    "Title",
    "Subtitle",
    "Message",
    "MessageSize",
    "Date",
    "PositiveActLabel",
    "NegativeActLabel",
    "Unknown"
};


#define ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES     0
#define ANCS_COMMAND_ID_GET_APP_ATTRIBUTES              1
#define ANCS_COMMAND_ID_PERFORM_NOTIFICATION_ACTION     2

#define ANCS_EVENT_ID_NOTIFICATION_ADDED                0
#define ANCS_EVENT_ID_NOTIFICATION_MODIFIED             1
#define ANCS_EVENT_ID_NOTIFICATION_REMOVED              2
#define ANCS_EVENT_ID_MAX                               3

/* Definitions for attributes we are not interested in are commented out */
/* #define ANCS_NOTIFICATION_ATTR_ID_APP_ID                0 */
#define ANCS_NOTIFICATION_ATTR_ID_TITLE                 1
/* #define ANCS_NOTIFICATION_ATTR_ID_SUBTITLE              2 */
#define ANCS_NOTIFICATION_ATTR_ID_MESSAGE               3
#define ANCS_NOTIFICATION_ATTR_ID_MESSAGE_SIZE          4
/* #define ANCS_NOTIFICATION_ATTR_ID_DATE                  5 */
#define ANCS_NOTIFICATION_ATTR_ID_POSITIVE_ACTION_LABEL 6
#define ANCS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL 7
#define ANCS_NOTIFICATION_ATTR_ID_MAX                   8

#define ANCS_EVENT_FLAG_SILENT                          (1 << 0)
#define ANCS_EVENT_FLAG_IMPORTANT                       (1 << 1)
#define ANCS_EVENT_FLAG_PREEXISTING                     (1 << 2)
#define ANCS_EVENT_FLAG_POSITIVE_ACTION                 (1 << 3)
#define ANCS_EVENT_FLAG_NEGATIVE_ACTION                 (1 << 4)

/* following is the list of notification attributes that we are going
to request.  Compile out attribute of no interest*/
UINT8  ancs_client_notification_attribute[] =
{
    /* ANCS_NOTIFICATION_ATTR_ID_APP_ID, */
    ANCS_NOTIFICATION_ATTR_ID_TITLE,
    /* ANCS_NOTIFICATION_ATTR_ID_SUBTITLE, */
    ANCS_NOTIFICATION_ATTR_ID_MESSAGE_SIZE,
    ANCS_NOTIFICATION_ATTR_ID_MESSAGE,
    /* ANCS_NOTIFICATION_ATTR_ID_DATE, */
    ANCS_NOTIFICATION_ATTR_ID_POSITIVE_ACTION_LABEL,
    ANCS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL,
    0
};
/* Maximum length we are going to request.  The values are valid for
title subtitle and message.  The number of elements should match number
of elements in the ancs_client_notification_attribute above*/
 UINT8  ancs_client_notification_attribute_length[] =
{
    /* 0, */
    20,
    /* 20, */
    0,
    255,
    /* 0, */
    0,
    0,
    0
};

/* ANCS event description */
typedef struct
{
    void      *p_next;
    UINT32    notification_uid;
    UINT8     command;
    UINT8     flags;
    UINT8     category;
    UINT8     title[20];
    UINT8     message[255];
    UINT8     positive_action_label[10];
    UINT8     negative_action_label[10];
} tANCS_EVENT;


typedef struct
{
    UINT8           state;
    UINT8           notification_attribute_inx;
    tANCS_EVENT     *p_first_event;
    UINT16          data_left_to_read;
    UINT16          data_source_buffer_offset;
    UINT8           data_source_buffer[256];

    int             thread_client_num;
    UINT32          thread_uid;
    BOOLEAN         bNotfnThreadInitialized;
    tAPP_THREAD     notfn_thread;

}tBSA_ANCS_CLCB;

tBSA_ANCS_CLCB      ancs_clcb;

/*
 * Local functions
 */
static int app_ble_ancs_register_notification(int client);
static int app_ble_ancs_deregister_notification(int client);
static int app_ble_ancs_configure_notification_source(int client, BOOLEAN start);
void app_ble_ancs_client_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);

static int app_ble_ancs_configure_data_source(int client, BOOLEAN start);

/*
 * BLE common functions
 */

/*******************************************************************************
 **
 ** Function        app_ble_ancs_main
 **
 ** Description     main function of ANCS consumer
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_ancs_main(void)
{
    int ret_value;

    /* register ANCS app */
    ret_value = app_ble_ancs_register();
    if(ret_value < 0)
    {
        APP_ERROR0("app_ble_ancs_register returns error");
    }
    return 0;
}


/*******************************************************************************
**
** Function         bsa_ancs_enable_advertisment
**
** Description      Include ANCS UUID enable the ANCS Consumer.
**
**
** Returns          void
*******************************************************************************/

#define BTM_BLE_ADVERT_TYPE_NAME_COMPLETE 0x09

static void bsa_ancs_enable_advertisment(void)
{
    tBSA_DM_BLE_ADV_PARAM adv_param;
    tBSA_DM_BLE_AD_MASK data_mask;
    tBSA_DM_SET_CONFIG bt_config;
        tBSA_STATUS bsa_status;
    UINT16  adv_int_min = 0x100;           /* minimum adv interval */
    UINT16  adv_int_max = 0x1000;          /* maximum adv interval */
    char device_name[] =  "APNC";          /* Apple Notification Consumer */

    UINT16 service_uuid_16_data[] = {
        0x1812,                    /* 16-bit Service UUID for 'Human Interface Device'  */
        0x1803,                    /* 16-bit Service UUID for 'Link Loss'               */
        0x1802,                    /* 16-bit Service UUID for 'Immediate Alert'         */
    };

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Inquiry Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_ADV_CONFIG_MASK;

    /* Use services flag to show above services if required on the peer device */
    data_mask = BSA_DM_BLE_AD_BIT_FLAGS /*| BSA_DM_BLE_AD_BIT_SERVICE*/ | BSA_DM_BLE_AD_BIT_PROPRIETARY | BSA_DM_BLE_AD_BIT_APPEARANCE | BSA_DM_BLE_AD_BIT_SERVICE_128SOL;


    bt_config.adv_config.num_service = 3;
    memcpy(bt_config.adv_config.uuid_val, service_uuid_16_data, bt_config.adv_config.num_service  * sizeof(UINT16));

    bt_config.adv_config.len = 4;
    bt_config.adv_config.flag = BSA_DM_BLE_ADV_FLAG_MASK;
    bt_config.adv_config.adv_data_mask = data_mask;
    bt_config.adv_config.appearance_data = 193; /* Watch*/
    bt_config.adv_config.is_scan_rsp = FALSE;

    memcpy(bt_config.adv_config.sol_service_128b.uuid128, ANCS_SERVICE, 16);
    bt_config.adv_config.sol_service_128b.list_cmpl = FALSE;

    memcpy(bt_config.adv_config.p_val, device_name, bt_config.adv_config.len);

    bt_config.adv_config.proprietary.num_elem = 1;
    bt_config.adv_config.proprietary.elem[0].adv_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    bt_config.adv_config.proprietary.elem[0].len = strlen(device_name);
    strcpy(bt_config.adv_config.proprietary.elem[0].val, device_name);

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return;
    }

    /* Set ADV params */
    memset(&adv_param, 0, sizeof(tBSA_DM_BLE_ADV_PARAM));
    adv_param.adv_int_min = adv_int_min;
    adv_param.adv_int_max = adv_int_max;
    app_dm_set_ble_adv_param(&adv_param);

    /* Set visible and connectable */
    app_dm_set_ble_visibility(TRUE, TRUE);
}


/*******************************************************************************
**
** Function         btui_ancs_disable_advertisment
**
** Description      Disable advertisment for ANCS consumer service
**
** Returns          void
**
*******************************************************************************/
void app_ble_ancs_disable_advertisment()
{
    /* Set NOT visible and NOT connectable */
    app_dm_set_ble_visibility(FALSE, FALSE);

}

/*******************************************************************************
 **
 ** Function        app_ble_ancs_register
 **
 ** Description     Register ANCS consumer
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_ancs_register(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_REGISTER ble_appreg_param;
    int index;

    index = app_ble_client_find_free_space();
    if( index < 0)
    {
        APP_ERROR0("app_ble_client_register no more space");
        return -1;
    }
    status = BSA_BleClAppRegisterInit(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppRegisterInit failed status = %d", status);
    }

    ble_appreg_param.uuid = app_ble_ancs_app_uuid;
    ble_appreg_param.p_cback = app_ble_ancs_client_profile_cback;

    status = BSA_BleClAppRegister(&ble_appreg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppRegister failed status = %d", status);
        return -1;
    }

    app_ble_cb.ble_client[index].enabled = TRUE;
    app_ble_cb.ble_client[index].client_if = ble_appreg_param.client_if;

    /* Eanble advertisement for the service */
    bsa_ancs_enable_advertisment();

    g_bRegDataSrcNotfnPending = FALSE;
    g_bDeRegDataSrcNotfnPending  = FALSE;


    ancs_clcb.thread_client_num = 0;
    ancs_clcb.thread_uid = 0;
    ancs_clcb.bNotfnThreadInitialized = FALSE;
    ancs_clcb.notfn_thread = NULL;

    return 0;
}

/*******************************************************************************
 **
 ** Function           app_ble_ancs_get_attributes_thread
 **
 ** Description        Thread to get ANCS attributes
 **
 ** Returns            void
 **
 *******************************************************************************/
static void app_ble_ancs_get_attributes_thread(void)
{
    UINT8 buf[8 + 10];
    UINT8 *p_write = (UINT8 *)buf;
    UINT8 *p_command = p_write;
    UINT8 len;
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT16 client_num;
    UINT32 uid;
    int i = 0;

    client_num = ancs_clcb.thread_client_num;
    uid = ancs_clcb.thread_uid;

    APPL_TRACE_DEBUG1("app_ble_ancs_get_attributes_thread( client_num = %d...) Called\n", client_num);

    while(app_ble_cb.ble_client[client_num].write_pending && i < 3)
    {
        APPL_TRACE_DEBUG1("app_ble_ancs_get_attributes_thread waiting : write pending! %d", i);
        sleep(1); /* wait for 3 seconds */
        i++;
    }

    if(app_ble_cb.ble_client[client_num].write_pending)
    {
        APP_ERROR0("app_ble_ancs_get_attributes_thread failed : write pending!");

        pthread_exit(NULL);
        return;/* Abort */
    }

    /* Allocating a buffer to send the write request */
    memset(buf, 0, sizeof(buf));

    *p_command++ = ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES;
    *p_command++ = uid & 0xff;
    *p_command++ = (uid >> 8) & 0xff;
    *p_command++ = (uid >> 16) & 0xff;
    *p_command++ = (uid >> 24) & 0xff;

    *p_command++ = ancs_client_notification_attribute[ancs_clcb.notification_attribute_inx];
    if (ancs_client_notification_attribute_length[ancs_clcb.notification_attribute_inx] != 0)
    {
        *p_command++ = ancs_client_notification_attribute_length[ancs_clcb.notification_attribute_inx] & 0xff;
        *p_command++ = (((UINT16)ancs_client_notification_attribute_length[ancs_clcb.notification_attribute_inx]) >> 8) & 0xff;
    }

    len      = (UINT8)(p_command - p_write);

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
        return;
    }

    ble_write_param.char_id.char_id.uuid.len=16;
    memcpy(ble_write_param.char_id.char_id.uuid.uu.uuid128, ANCS_CONTROL_POINT, 16);

    ble_write_param.char_id.srvc_id.id.uuid.len=16;
    memcpy(ble_write_param.char_id.srvc_id.id.uuid.uu.uuid128, ANCS_SERVICE, 16);

    ble_write_param.char_id.srvc_id.is_primary = 1;

    ble_write_param.len = len;
    memcpy(ble_write_param.value, buf, len);

    ble_write_param.descr = FALSE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_MITM;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client_num].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client_num].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
    }

    ancs_clcb.bNotfnThreadInitialized = FALSE;

    pthread_exit(NULL);
}


/*******************************************************************************
**
** Function         ancs_client_send_next_get_notification_attributes_command
**
** Description      Send command to the phone to get notification attributes.
**
** Returns          void
**
*******************************************************************************/
void ancs_client_send_next_get_notification_attributes_command(UINT16 client_num, UINT32 uid)
{
    int status = 0;
    if(ancs_clcb.bNotfnThreadInitialized)
    {
        APP_ERROR0("ancs_client_send_next_get_notification_attributes_command Thread already exists CANNOT GET ATTRIBUTES");
        return;
    }

    ancs_clcb.thread_client_num = client_num;
    ancs_clcb.thread_uid = uid;


    /* create thread to send notification */
    status = app_create_thread(app_ble_ancs_get_attributes_thread, 0, 0, &ancs_clcb.notfn_thread);
    if (status < 0)
    {
        APP_ERROR1("app_create_thread failed: %d", status);
        return -1;
    }
    else
        ancs_clcb.bNotfnThreadInitialized = TRUE;
}

/*******************************************************************************
**
** Function         ancs_perform_action
**
** Description      Send command to the phone to perform specified action
**
** Returns          void
**
*******************************************************************************/
void ancs_perform_action(UINT16 client_num, UINT32 uid, UINT32 action_id )
{
    UINT8 buf[8+ 10];
    UINT8 *p_write = (UINT8 *)buf;
    UINT8 *p_command = p_write;
    UINT8 len;
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    APPL_TRACE_DEBUG3( "%s uid:%d action:%d\n", __FUNCTION__, uid, action_id );

    APPL_TRACE_DEBUG1("ancs_perform_action( client_num = %d...) Called\n", client_num);

    /* Allocating a buffer to send the write request */
    memset(buf, 0, sizeof(buf));

    *p_command++ = ANCS_COMMAND_ID_PERFORM_NOTIFICATION_ACTION;
    *p_command++ = uid & 0xff;
    *p_command++ = (uid >> 8) & 0xff;
    *p_command++ = (uid >> 16) & 0xff;
    *p_command++ = (uid >> 24) & 0xff;

    *p_command++ = action_id;

    len      = (UINT8)(p_command - p_write);

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
        return;
    }

    ble_write_param.char_id.char_id.uuid.len=16;
    memcpy(ble_write_param.char_id.char_id.uuid.uu.uuid128, ANCS_CONTROL_POINT, 16);

    ble_write_param.char_id.srvc_id.id.uuid.len=16;
    memcpy(ble_write_param.char_id.srvc_id.id.uuid.uu.uuid128, ANCS_SERVICE, 16);

    ble_write_param.char_id.srvc_id.is_primary = 1;

    ble_write_param.len = len;
    memcpy(ble_write_param.value, buf, len);

    ble_write_param.descr = FALSE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_MITM;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client_num].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client_num].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
    }
}

/*******************************************************************************
**
** Function         ancs_client_process_notification_source
**
** Description      Process Notification Source messages from the phone.
**                  If it is a new or modified notification start reading attributes.
**
** Returns          void
**
*******************************************************************************/
void ancs_client_process_notification_source(UINT16 client_num, UINT8 *data, int len)
{
    tANCS_EVENT *p_ancs_event;

    APPL_TRACE_DEBUG1("ancs_client_process_notification_source( client_num = %d...) Called\n", client_num);

    /* Skip all pre-existing events */
    if (data[1] & ANCS_EVENT_FLAG_PREEXISTING)
    {
        APPL_TRACE_DEBUG1("skipped preexisting event UID:%d\n", data[4] + (data[5] << 8) + (data[6] << 16) + (data[7] << 24));
        return;
    }

    switch (data[0])
    {
    case ANCS_EVENT_ID_NOTIFICATION_ADDED:
    case ANCS_EVENT_ID_NOTIFICATION_MODIFIED:
    case ANCS_EVENT_ID_NOTIFICATION_REMOVED:
        break;

    default:
        APPL_TRACE_DEBUG1("unknown command:%d\n", data[0]);
        return;
    }

    if ((p_ancs_event = (tANCS_EVENT *) GKI_getbuf(sizeof(tANCS_EVENT))) == NULL)
    {
        APPL_TRACE_DEBUG0("Failed to get buf\n");
        return;
    }
    memset (p_ancs_event, 0, sizeof(tANCS_EVENT));

    p_ancs_event->notification_uid = data[4] + (data[5] << 8) + (data[6] << 16) + (data[7] << 24);


    APPL_TRACE_DEBUG2("notification command:%d, uuid:%d\n", data[0], p_ancs_event->notification_uid);

    p_ancs_event->command  = data[0];
    p_ancs_event->flags    = data[1];
    p_ancs_event->category = data[2];

    /* if we do not need to get details, and if there is nothing in the queue, can ship it out now */
    if ((p_ancs_event->command == ANCS_EVENT_ID_NOTIFICATION_REMOVED) && (ancs_clcb.p_first_event == NULL))
    {

        APPL_TRACE_DEBUG5 ("ANCS Notification EventID:%s EventFlags:%04x CategoryID:%s CategoryCount:%d UID:%04x",
            (UINT32)(data[0] < ANCS_EVENT_ID_MAX) ? EventId[data[0]] : EventId[ANCS_EVENT_ID_MAX],
            data[1],
            (UINT32)(data[2] < ANCS_CATEGORY_ID_MAX) ? CategoryId[data[2]] : CategoryId[ANCS_CATEGORY_ID_MAX],
            data[3],
            p_ancs_event->notification_uid);

        GKI_freebuf(p_ancs_event);
        return;
    }

    /* enqueue new event at the end of the queue */
    if (ancs_clcb.p_first_event == NULL)
        ancs_clcb.p_first_event = p_ancs_event;
    else
    {
        tANCS_EVENT *p;
        for (p = ancs_clcb.p_first_event; p->p_next != NULL; p = p->p_next)
            ;
        p->p_next = p_ancs_event;
    }

    if ((p_ancs_event->command == ANCS_EVENT_ID_NOTIFICATION_ADDED) || (p_ancs_event->command == ANCS_EVENT_ID_NOTIFICATION_MODIFIED))
    {
        /* if we are currently in process of dealing with another event just return */
        if (ancs_clcb.p_first_event == p_ancs_event)
        {
            ancs_client_send_next_get_notification_attributes_command(client_num, p_ancs_event->notification_uid);
        }
        else
        {
            APPL_TRACE_DEBUG0("will retrieve details later\n");
        }
    }
}

/*******************************************************************************
**
** Function         ancs_display_event_details
**
** Description      Displays event details.
**
** Returns          void
**
*******************************************************************************/
void ancs_display_event_details(tANCS_EVENT* p_ancs_event)
{
    APPL_TRACE_DEBUG4("ANCS notification UID:%d command:%d category:%d flags:%04x\n", \
        p_ancs_event->notification_uid, p_ancs_event->command, p_ancs_event->category, p_ancs_event->flags);
    APPL_TRACE_DEBUG4("ANCS notification %s %s %s %s\n", p_ancs_event->title, p_ancs_event->message, \
        p_ancs_event->positive_action_label, p_ancs_event->negative_action_label);

    GKI_freebuf(p_ancs_event);
}

/*******************************************************************************
**
** Function         ancs_client_process_event_attribute
**
** Description      Process additional message attributes. The header file
**                  defines which attributes we are asking for.
**
** Returns          void
**
*******************************************************************************/
void ancs_client_process_event_attribute(UINT16 client_num, UINT8  *data, int len)
{
    UINT8      type = data[0];
    UINT16     length = data[1] + (((UINT16) data[2]) >> 8);
    UINT8      *p_event_data = &data[3];
    tANCS_EVENT *p_event = ancs_clcb.p_first_event;

    ancs_clcb.data_left_to_read         = 0;
    ancs_clcb.data_source_buffer_offset = 0;

    switch(type)
    {
#ifdef ANCS_NOTIFICATION_ATTR_ID_APP_ID
    case ANCS_NOTIFICATION_ATTR_ID_APP_ID:
        break;
#endif
#ifdef ANCS_NOTIFICATION_ATTR_ID_TITLE
    case ANCS_NOTIFICATION_ATTR_ID_TITLE:
        memcpy(p_event->title, p_event_data, (length < sizeof(p_event->title) ? length : sizeof(p_event->title)));
        break;
#endif
#ifdef ANCS_NOTIFICATION_ATTR_ID_SUBTITLE
    case ANCS_NOTIFICATION_ATTR_ID_SUBTITLE:
        break;
#endif
#ifdef ANCS_NOTIFICATION_ATTR_ID_MESSAGE_SIZE
    case ANCS_NOTIFICATION_ATTR_ID_MESSAGE_SIZE:
        break;
#endif
#ifdef ANCS_NOTIFICATION_ATTR_ID_MESSAGE
    case ANCS_NOTIFICATION_ATTR_ID_MESSAGE:
        memcpy(p_event->message, p_event_data, (length < sizeof(p_event->message) ? length : sizeof(p_event->message)));
        break;
#endif
#ifdef ANCS_NOTIFICATION_ATTR_ID_DATE
    case ANCS_NOTIFICATION_ATTR_ID_DATE:
        break;
#endif
#ifdef ANCS_NOTIFICATION_ATTR_ID_POSITIVE_ACTION_LABEL
    case ANCS_NOTIFICATION_ATTR_ID_POSITIVE_ACTION_LABEL:
        memcpy(p_event->positive_action_label, p_event_data, (length < sizeof(p_event->positive_action_label) ? length : sizeof(p_event->positive_action_label)));
        break;
#endif
#ifdef ANCS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL
    case ANCS_NOTIFICATION_ATTR_ID_NEGATIVE_ACTION_LABEL:
        memcpy(p_event->negative_action_label, p_event_data, (length < sizeof(p_event->negative_action_label) ? length : sizeof(p_event->negative_action_label)));
        break;
#endif
    }

    /* if we are not done with attributes, request the next one */
    if (ancs_client_notification_attribute[++ancs_clcb.notification_attribute_inx] != 0)
    {
        ancs_client_send_next_get_notification_attributes_command(client_num, p_event->notification_uid);
    }
    else
    {
        /* Done with attributes for current event */
        ancs_clcb.notification_attribute_inx = 0;

        p_event = ancs_clcb.p_first_event;
        ancs_clcb.p_first_event = p_event->p_next;

        /* ship current event to the application */
        ancs_display_event_details(p_event);

        /* if next event in the queue is "Removed" ship it out right away */
        while (ancs_clcb.p_first_event != NULL)
        {
            if (ancs_clcb.p_first_event->command == ANCS_EVENT_ID_NOTIFICATION_REMOVED)
            {
                p_event = ancs_clcb.p_first_event;
                ancs_clcb.p_first_event = p_event->p_next;

                ancs_display_event_details(p_event);
            }
            else
            {
                /* start reading attributes for the next message */
                ancs_client_send_next_get_notification_attributes_command(client_num, ancs_clcb.p_first_event->notification_uid);
                break;
            }
        }
    }
}

/*******************************************************************************
**
** Function         ancs_client_process_data_source
**
** Description      Process Data Source messages from the phone.
**                  This can be new or continuation of the previous message
**
** Returns          void
**
*******************************************************************************/
void ancs_client_process_data_source(UINT16 client_num, UINT8  *data, int len)
{
    UINT8      attr_id;
    UINT8      attr_len;

    APPL_TRACE_DEBUG2("Data source left to read:%d len:%d\n", ancs_clcb.data_left_to_read, len);

    /* check if this is a continuation of the previous message */
    if (ancs_clcb.data_left_to_read)
    {
        memcpy(&ancs_clcb.data_source_buffer[ancs_clcb.data_source_buffer_offset], data, len);
        ancs_clcb.data_source_buffer_offset += len;
        ancs_clcb.data_left_to_read -= len;
        if (ancs_clcb.data_left_to_read <= 0)
        {
            ancs_client_process_event_attribute(client_num, &ancs_clcb.data_source_buffer[5], ancs_clcb.data_source_buffer_offset - 5);
        }
    }
    else
    {
        /* start of the new message */
        attr_id  = data[5];
        attr_len = data[6] + (data[7] << 8);
        APPL_TRACE_DEBUG2("ANCS Data Notification Attribute:%04x len %d\n", attr_id, attr_len);
        if (attr_len <= len - 8)
        {
            ancs_client_process_event_attribute(client_num, (UINT8 *)&data[5], len - 5);
        }
        else
        {
            /* whole message did not fit into the message, phone should send addition data */
            memcpy(&ancs_clcb.data_source_buffer[0], data, len);
            ancs_clcb.data_source_buffer_offset = len;
            ancs_clcb.data_left_to_read = attr_len - len + 8;
        }
    }
}

/*******************************************************************************
**
** Function         app_ble_ancs_client_profile_cback
**
** Description      BLE Client Profile callback.
**
** Returns          void
**
*******************************************************************************/
void app_ble_ancs_client_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    int client_num;
    int status;
    tBSA_BLE_CL_INDCONF ind_conf;

    switch (event)
    {
    case BSA_BLE_CL_WRITE_EVT:
        APP_INFO1("BSA_BLE_CL_WRITE_EVT status:%d", p_data->cli_write.status);
        client_num = app_ble_client_find_index_by_conn_id(p_data->cli_write.conn_id);
        if (client_num >= 0)
        {
            app_ble_cb.ble_client[client_num].write_pending = FALSE;

            if(g_bRegDataSrcNotfnPending)
            {
                g_bRegDataSrcNotfnPending = FALSE;
                app_ble_ancs_configure_data_source(client_num, TRUE);
            }

            if(g_bDeRegDataSrcNotfnPending)
            {
                g_bDeRegDataSrcNotfnPending = FALSE;
                app_ble_ancs_configure_data_source(client_num, FALSE);
            }
        }
        break;

    case BSA_BLE_CL_NOTIF_EVT:
        APP_INFO1("BSA_BLE_CL_NOTIF_EVT BDA :%02X:%02X:%02X:%02X:%02X:%02X",
                    p_data->cli_notif.bda[0], p_data->cli_notif.bda[1],
                    p_data->cli_notif.bda[2], p_data->cli_notif.bda[3],
                    p_data->cli_notif.bda[4], p_data->cli_notif.bda[5]);
        APP_INFO1("conn_id:0x%x, svrc_id:0x%04x, char_id:0x%04x, descr_type:0x%04x, len:%d, is_notify:%d",
                    p_data->cli_notif.conn_id,
                    p_data->cli_notif.char_id.srvc_id.id.uuid.uu.uuid16,
                    p_data->cli_notif.char_id.char_id.uuid.uu.uuid16,
                    p_data->cli_notif.descr_type.uuid.uu.uuid16, p_data->cli_notif.len,
                    p_data->cli_notif.is_notify);
        APP_DUMP("data", p_data->cli_notif.value, p_data->cli_notif.len);

        client_num = app_ble_client_find_index_by_conn_id(p_data->cli_notif.conn_id);

        if(p_data->cli_notif.is_notify != TRUE)
        {
            /* this is indication, and need to send confirm */
            APP_INFO0("Receive Indication! send Indication Confirmation!");
            status = BSA_BleClIndConfInit(&ind_conf);
            if (status < 0)
            {
                APP_ERROR1("BSA_BleClIndConfInit failed: %d", status);
                break;
            }
            ind_conf.conn_id = p_data->cli_notif.conn_id;
            memcpy(&(ind_conf.char_id), &(p_data->cli_notif.char_id), sizeof(tBTA_GATTC_CHAR_ID));
            status = BSA_BleClIndConf(&ind_conf);
            if (status < 0)
            {
                APP_ERROR1("BSA_BleClIndConf failed: %d", status);
                break;
            }
        }

        if (!memcmp(p_data->cli_notif.char_id.char_id.uuid.uu.uuid128, ANCS_NOTIFICATION_SOURCE, 16))
        {
            APPL_TRACE_DEBUG0("received ancs Notification Source ");
            ancs_client_process_notification_source(client_num, p_data->cli_notif.value, p_data->cli_notif.len);
        }
        else if (!memcmp(p_data->cli_notif.char_id.char_id.uuid.uu.uuid128, ANCS_DATA_SOURCE, 16))
        {
            APPL_TRACE_DEBUG0("received ancs Data Source ");
            ancs_client_process_data_source(client_num, p_data->cli_notif.value, p_data->cli_notif.len);
        }
        else
        {
            APPL_TRACE_DEBUG0("received UNKNOWN ANCS Notification");
        }
        break;

    default:
         app_ble_client_profile_cback(event, p_data);
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_ble_ancs_register_notifications
 **
 ** Description     Register for ANCS notifications
 **
 ** Parameters      bRegister: TRUE/FALSE
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_ancs_register_notifications(BOOLEAN bRegister)
{
    int client_num;

    APP_INFO0("Select Client:");
    app_ble_client_display(0);
    client_num = app_get_choice("Select");

    if(client_num < 0)
    {
        APP_ERROR0("app_ble_ancs_register_notifications : Invalid client number entered");
        return -1;
    }

    if(bRegister)
    {
        app_ble_ancs_register_notification(client_num);
        app_ble_ancs_configure_notification_source(client_num, TRUE);
        g_bRegDataSrcNotfnPending= TRUE;
    }
    else /* stop monitoring */
    {
        app_ble_ancs_configure_notification_source(client_num, FALSE);
        g_bDeRegDataSrcNotfnPending = TRUE;
        app_ble_ancs_deregister_notification(client_num);
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function        app_ble_ancs_register_notification
 **
 ** Description     This is the register function to receive a notification
 **
 ** Parameters      client identifier
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_ancs_register_notification(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 16;
    memcpy(ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid128, ANCS_SERVICE, 16);
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 16;
    memcpy(ble_notireg_param.notification_id.char_id.uuid.uu.uuid128, ANCS_NOTIFICATION_SOURCE, 16);

    status = BSA_BleClNotifRegister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegister Data Source Notification failed status = %d", status);
        return -1;
    }

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status = %d", status);
    }

    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 16;
    memcpy(ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid128, ANCS_SERVICE, 16);
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 16;
    memcpy(ble_notireg_param.notification_id.char_id.uuid.uu.uuid128, ANCS_DATA_SOURCE, 16);

    status = BSA_BleClNotifRegister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegister Data Source Notification failed status = %d", status);
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function        app_ble_ancs_deregister_notification
 **
 ** Description     This is the deregister function to receive a notification
 **
 ** Parameters      client identifier
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_ancs_deregister_notification(int client)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFDEREG ble_notireg_param;

    status = BSA_BleClNotifDeregisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifDeregisterInit failed status = %d", status);
    }

    /* register all notification */
    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client].client_if;

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 16;
    memcpy(ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid128, ANCS_SERVICE, 16);
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;

    ble_notireg_param.notification_id.char_id.uuid.len = 16;
    memcpy(ble_notireg_param.notification_id.char_id.uuid.uu.uuid128, ANCS_NOTIFICATION_SOURCE, 16);

    status = BSA_BleClNotifDeregister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifDeregister failed status = %d", status);
        return -1;
    }

    ble_notireg_param.notification_id.char_id.uuid.len = 16;
    memcpy(ble_notireg_param.notification_id.char_id.uuid.uu.uuid128, ANCS_DATA_SOURCE, 16);

    status = BSA_BleClNotifDeregister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifDeregister failed status = %d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_ancs_configure_notification_source
 **
 ** Description     Enables / Disables Notification-Source notifications
 **
 ** Parameters      client identifier and TRUE / FALSE
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_ancs_configure_notification_source(int client, BOOLEAN start)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    APPL_TRACE_DEBUG1("app_ble_ancs_configure_notification_source( client_num = %d...) Called\n", client);

    if(app_ble_cb.ble_client[client].write_pending)
    {
        APP_ERROR0("app_ble_ancs_configure_notification_source failed : write pending!");
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=16;
    memcpy(ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid128, ANCS_SERVICE, 16);

    ble_write_param.descr_id.char_id.char_id.uuid.len=16;
    memcpy(ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid128, ANCS_NOTIFICATION_SOURCE, 16);

    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.len = 2;
    if(start)
        ble_write_param.value[0] = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG_ENABLE_NOTI;
    ble_write_param.value[1] = 0;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_MITM;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
**
** Function         app_ble_ancs_configure_data_source
**
** Description      Enables / Disables Data Source notification
**
** Parameters       client identifier and TRUE / FALSE
**
** Returns          status: 0 if success / -1 otherwise
**
*******************************************************************************/
static int app_ble_ancs_configure_data_source(int client, BOOLEAN start)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    APPL_TRACE_DEBUG1("app_ble_ancs_configure_data_source( client_num = %d...) Called\n", client);

    if(app_ble_cb.ble_client[client].write_pending)
    {
        APP_ERROR0("app_ble_ancs_configure_data_source failed : write pending!");
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    ble_write_param.descr_id.char_id.srvc_id.id.uuid.len=16;
    memcpy(ble_write_param.descr_id.char_id.srvc_id.id.uuid.uu.uuid128, ANCS_SERVICE, 16);

    ble_write_param.descr_id.char_id.char_id.uuid.len=16;
    memcpy(ble_write_param.descr_id.char_id.char_id.uuid.uu.uuid128, ANCS_DATA_SOURCE, 16);

    ble_write_param.descr_id.char_id.srvc_id.is_primary = 1;
    ble_write_param.descr_id.descr_id.uuid.uu.uuid16 = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.descr_id.descr_id.uuid.len = 2;
    ble_write_param.len = 2;
    if(start)
        ble_write_param.value[0] = BSA_BLE_GATT_UUID_CHAR_CLIENT_CONFIG_ENABLE_NOTI;
    ble_write_param.value[1] = 0;
    ble_write_param.descr = TRUE;
    ble_write_param.conn_id = app_ble_cb.ble_client[client].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_MITM;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[client].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[client].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }

    return 0;
}
