/*****************************************************************************
**
**  Name:           app_ble_tvselect_server.h
**
**  Description:    Bluetooth BLE TVSELECT Server include file
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_BLE_TVSELECT_SERVER_H
#define APP_BLE_TVSELECT_SERVER_H

#include "bsa_api.h"
#include "bsa_ble_api.h"

#include "app_ble.h"
#include "app_ble_client_db.h"
#include "app_thread.h"

/* TVSELECT SW Version */
#define APP_BLE_TVSELECT_SW_MAJOR_VERSION          0x0000
#define APP_BLE_TVSELECT_SW_MINOR_VERSION          0x0001

/* TVSELECT Helper UUID Definitions */
#define APP_BLE_TVSELECT_SERVER_SERV_UUID          0x8000
#define APP_BLE_TVSELECT_SERVER_APP_UUID           0x8010
#define APP_BLE_TVSELECT_SERVER_CHAR_UUID          0x8011
#define APP_BLE_TVSELECT_SERVER_NUM_OF_HANDLES     10
#define APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER      1
#define APP_BLE_TVSELECT_SERVER_ATTR_NUM           1

/* commands */
#define CMD_BAV_CP_VERSION       0x01
#define CMD_BAV_CP_STATE         0x02
#define CMD_BAV_CP_CHANNEL_NAME  0x03
#define CMD_BAV_CP_CHANNEL_ICON  0x04
#define CMD_BAV_CP_MEDIA_NAME    0x05
#define CMD_BAV_CP_MEDIA_ICON    0x06

/* commands status */
#define BAV_CP_EVT_SUCCESS       0x00
#define BAV_CP_EVT_ERROR         0x02
#define BAV_CP_EVT_UNSUPPORTED   0x03
#define BAV_CP_EVT_UNAVAILABLE   0xff

/* server states */
#define BAV_CP_EVT_STATE_IDLE      0x00
#define BAV_CP_EVT_STATE_STREAMING 0x01
#define BAV_CP_EVT_STATE_UNKNOWN   0xff

/* events */
#define BAV_EVT_VERSION       0x01
#define BAV_EVT_MEDIA_CHANGED 0x02
#define BAV_EVT_CHANNEL_NAME  0x03
#define BAV_EVT_CHANNEL_ICON  0x04
#define BAV_EVT_MEDIA_INFO    0x05
#define BAV_EVT_MEDIA_ICON    0x06

/*Attribute Byte 0*/
#define TV_METADATA_COMPLETED_MSK         0x0001    /*b0*/
#define TV_METADATA_CH_NAME               0x0002    /*b1*/
#define TV_METADATA_CH_ICON               0x0004    /*b2*/
#define TV_METADATA_PGM_NAME              0x0008    /*b3*/
#define TV_METADATA_PGM_ICON              0x0010    /*b4*/
#define TV_METADATA_CHANGE_MSK            0x001e    /*b1-4*/

/*Attribute Byte 1*/
#define TV_METADATA_UNIQUE_ID_MSK         0x0f      /*4 bits value*/
#define TV_METADATA_UPDATE_UNIQUE_ID_VALUE(attr_data, id_value)   \
{                                                                 \
    attr_data &= ~(TV_METADATA_UNIQUE_ID_MSK << 12);              \
    attr_data |= (id_value << 12);                                \
}
#define TV_METADATA_UNIQUE_ID_INITIAL_VALUE     0x0f    /*initial value when start*/

typedef struct
{
    tAPP_THREAD  resp_thread;
    BOOLEAN      notification_send;
    UINT16       conn_id;
    UINT16       attr_id;
} tAPP_BLE_TVSELECT_SERVER_CB;

/* TVSelect  message type definitions*/
#define TVSELECT_TYPE_COMMAND   1
#define TVSELECT_TYPE_EVENT     2

/*advertisement packet length: 31 bytes max*/
#define BSA_DM_BLE_AD_FLAG_LEN                     3   /*BLE Advertisement flag length */
#define BSA_DM_BLE_AD_SERVICE_DATA_LEN             28  /*BLE Advertisement service data length */

/*BLE Advertisement service data (28bytes)*/
/*7 bytes = 1+1+2+1+2 (= length + data type length (0x16) + uuid + lt_addr + attr)*/
#define BSA_DM_BLE_AD_SERVICE_DATA_HEADER_LEN      7

/*value length (21bytes) */
#define BSA_DM_BLE_AD_SERVICE_DATA_VAL_LEN BSA_DM_BLE_AD_SERVICE_DATA_LEN-BSA_DM_BLE_AD_SERVICE_DATA_HEADER_LEN

#define BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX BSA_BLE_MAX_ATTR_LEN

typedef struct
{
    UINT16       service_uuid;
    UINT8        lt_addr;                               /* LT ADDR Server is using for broadcasting for audio*/
    UINT16       attr_data;
    UINT8        unique_id;
    UINT8        service_data_val[BSA_DM_BLE_AD_SERVICE_DATA_VAL_LEN];

    UINT8        state;
    UINT8        major_ver[2];
    UINT8        minor_ver[2];
    UINT8        channel_name[BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX];
    UINT16       channel_name_len;
    UINT8        channel_icon[BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX];
    UINT16       channel_icon_len;
    UINT8        media_name[BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX];
    UINT16       media_name_len;
    UINT8        media_icon[BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX];
    UINT16       media_icon_len;
} tAPP_BLE_TVSELECT_ADVERTISE_DATA;

/*******************************************************************************
**
** Function        app_ble_tvselect_server_start
**
** Description     start TVSELECT Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_server_start(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_stop
**
** Description     stop responder
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_server_stop(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_deregister
**
** Description     Deregister server app
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_server_deregister(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_create_service
**
** Description     create service
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_server_create_service(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_start_service
**
** Description     Start Service
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_server_start_service(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_add_char
**
** Description     Add character to service
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_server_add_char(void);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_send_response
**
** Description     Send response to initiator
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
int app_ble_tvselect_server_send_response(UINT8 *p, int len);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_update_advertise_packet
**
** Description     update advertising packet data
**
** Parameters
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
void app_ble_tvselect_server_update_advertise_packet(UINT16 metatype, UINT8* metadata, UINT16 len);

/*******************************************************************************
**
** Function        app_ble_tvselect_server_change_metadata_chname
**
** Description     update advertising packet data - channel name
**
** Parameters
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
void app_ble_tvselect_server_change_metadata_chname();

/*******************************************************************************
**
** Function        app_ble_tvselect_server_change_metadata_chname
**
** Description     update advertising packet data - channel icon
**
** Parameters
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
void app_ble_tvselect_server_change_metadata_chicon();

/*******************************************************************************
**
** Function        app_ble_tvselect_server_change_metadata_chname
**
** Description     update advertising packet data - channel icon
**
** Parameters
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
void app_ble_tvselect_server_change_metadata_medianame();

/*******************************************************************************
**
** Function        app_ble_tvselect_server_change_metadata_mediaicon
**
** Description     update advertising packet data - program icon
**
** Parameters
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
void app_ble_tvselect_server_change_metadata_mediaicon();

/*******************************************************************************
**
** Function         app_ble_tvselect_server_profile_cback
**
** Description      TVSELECT Server callback
**
** Returns          void
**
*******************************************************************************/
void app_ble_tvselect_server_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);

#endif
