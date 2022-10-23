/*****************************************************************************
**
**  Name:           app_ble_tvselect_server.c
**
**  Description:    Bluetooth BLE TVSELECT Server main application
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "app_ble.h"
#include "app_thread.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_dm.h"
#include "app_ble_client.h"
#include "app_ble_server.h"
#include "app_ble_client_db.h"
#include "app_ble_tvselect_client.h"
#include "app_ble_tvselect_server.h"
#include "app_tvselect.h"             /*for LT Address*/

/*
 * Global Variables
*/
tAPP_BLE_TVSELECT_SERVER_CB app_ble_tvselect_server_cb;
tBSA_DM_BLE_ADV_CONFIG adv_conf;
tAPP_BLE_TVSELECT_ADVERTISE_DATA adv_meta_data;

/*include DataType Flags in advertising packet*/
#define INCLUDE_DATATYPE_FLAGS_IN_ADVERTISING_DATA

/*******************************************************************************
**
** Function        app_ble_tvselect_server_init_metadata
**
** Description     initialize TVSelect server metadata elements
**
** Parameters      None
**
** Returns         None
**
*******************************************************************************/
void app_ble_tvselect_server_init_metadata(void)
{
    int choice;
    UINT8 initial_channel_name[8] = {'C','N','N',' ','N','e','w','s'};

    memset(&adv_meta_data, 0, sizeof(tAPP_BLE_TVSELECT_ADVERTISE_DATA));

    /*state*/
    adv_meta_data.state = BAV_CP_EVT_STATE_STREAMING;

    /* SW Version */
    adv_meta_data.major_ver[0] = (APP_BLE_TVSELECT_SW_MAJOR_VERSION >> 8) & 0xff;
    adv_meta_data.major_ver[1] = APP_BLE_TVSELECT_SW_MAJOR_VERSION & 0xff;
    adv_meta_data.minor_ver[0] = (APP_BLE_TVSELECT_SW_MINOR_VERSION >> 8) & 0xff;
    adv_meta_data.minor_ver[1] = APP_BLE_TVSELECT_SW_MINOR_VERSION & 0xff;

    /*service uuid*/
    adv_meta_data.service_uuid = APP_BLE_TVSELECT_SERVER_SERV_UUID;

    /*get the LT Address for Advertising*/
    APP_INFO0("Enter LT Address");
    choice = app_get_choice("LT Addr");
    if((0 < choice) && (choice <= APP_AV_BCST_LT_ADDR_MAX))
    {
        APP_DEBUG1("LT Address %d entered", choice);
        adv_meta_data.lt_addr = choice;
    }
    else
    {
        APP_DEBUG0("Wrong LT Address");
        return;
    }

    /*initial channel name*/
    memset(&adv_meta_data.channel_name, 0, BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX);
    memcpy(&adv_meta_data.channel_name, initial_channel_name, sizeof(initial_channel_name));
    adv_meta_data.channel_name_len = sizeof(initial_channel_name);
}

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
int app_ble_tvselect_server_start(void)
{
    UINT8 *p;

    /* clear control block, advertising data */
    memset(&app_ble_tvselect_server_cb, 0, sizeof(app_ble_tvselect_server_cb));
    memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));

    /*initialize TVSelect Server metadata elements*/
    app_ble_tvselect_server_init_metadata();

    /* register BLE server app */
    app_ble_server_register(APP_BLE_TVSELECT_SERVER_APP_UUID, app_ble_tvselect_server_profile_cback);
    GKI_delay(1000);

    /* create a BLE service */
    app_ble_tvselect_server_create_service();
    GKI_delay(1000);

    /* add a characteristic */
    app_ble_tvselect_server_add_char();
    GKI_delay(1000);

    /* start service */
    app_ble_tvselect_server_start_service();
    GKI_delay(1000);

    /* make discoverable & connectable */
    app_dm_set_ble_visibility(TRUE, TRUE);

    /* Start TV Select BLE Advertising*/
    /* Payload = 37Bytes (= Advertiser Address(6) + Flag(3) + Service Data(28) ) */
#ifdef INCLUDE_DATATYPE_FLAGS_IN_ADVERTISING_DATA
    /*DataType = Flag + Service Data*/
    adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS|BSA_DM_BLE_AD_BIT_SERVICE_DATA;
    /* Flag (AD Type: 0x01)*/
    adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
#else
    /*DataType = Service Data only*/
    adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_SERVICE_DATA;
#endif

    /* Service Data (AD Type: 0x16) */
    adv_conf.service_data_len = BSA_DM_BLE_AD_SERVICE_DATA_LEN-1; /*1 = length*/
    adv_conf.service_data_uuid.len = sizeof(adv_meta_data.service_uuid);
    adv_conf.service_data_uuid.uu.uuid16 = adv_meta_data.service_uuid;

    p = adv_conf.service_data_val;
    UINT8_TO_STREAM(p, adv_meta_data.lt_addr);         /*Byte4  LT Addr */
    UINT16_TO_STREAM(p, adv_meta_data.attr_data);      /*Byte5-6: attribute data*/
    ARRAY_TO_STREAM(p, adv_meta_data.channel_name, adv_meta_data.channel_name_len);

    app_dm_set_ble_adv_data(&adv_conf);

    return 0;
}

/*******************************************************************************
**
** Function        app_ble_tvselect_server_update_advertise_packet
**
** Description     start TVSELECT Server
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
void app_ble_tvselect_server_update_advertise_packet(UINT16 metatype, UINT8* metadata, UINT16 len)
{
    UINT8 *p;

    APP_DEBUG1("metadata length = %d", len);

    if (len <= 0)
    {
        APP_ERROR0("wrong metadata, len: 0. No Update AD Packet!");
        return;
    }

    /*save new metadata*/
    switch (metatype)
    {
        case TV_METADATA_CH_NAME:
            if (memcmp(&adv_meta_data.channel_name, metadata, len)==0 && (adv_meta_data.channel_name_len==len))
            {
                APP_DEBUG0("no change - TV_METADATA_CH_NAME");
            }
            else  /*update*/
            {
                memset(&adv_meta_data.channel_name, 0, BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX);
                memcpy(&adv_meta_data.channel_name, metadata, len);
                adv_meta_data.channel_name_len = len;
            }
        break;

        case TV_METADATA_CH_ICON:
            if (memcmp(&adv_meta_data.channel_icon, metadata, len)==0 && (adv_meta_data.channel_icon_len==len))
            {
                APP_DEBUG0("no change - TV_METADATA_CH_ICON");
            }
            else  /*update*/
            {
                memset(&adv_meta_data.channel_icon, 0, BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX);
                memcpy(&adv_meta_data.channel_icon, metadata, len);
                adv_meta_data.channel_icon_len = len;
            }
        break;

        case TV_METADATA_PGM_NAME:
            if (memcmp(&adv_meta_data.media_name, metadata, len)==0 && (adv_meta_data.media_name_len==len))
            {
                APP_DEBUG0("no change - TV_METADATA_PGM_NAME");
            }
            else  /*update*/
            {
                memset(&adv_meta_data.media_name, 0, BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX);
                memcpy(&adv_meta_data.media_name, metadata, len);
                adv_meta_data.media_name_len = len;
            }
        break;

        case TV_METADATA_PGM_ICON:
            if (memcmp(&adv_meta_data.media_icon, metadata, len)==0 && (adv_meta_data.media_icon_len==len))
            {
                APP_DEBUG0("no change - TV_METADATA_PGM_ICON");
            }
            else  /*update*/
            {
                memset(&adv_meta_data.media_icon, 0, BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX);
                memcpy(&adv_meta_data.media_icon, metadata, len);
                adv_meta_data.media_icon_len = len;
            }
        break;

        default:
        break;
    }

    /*initialize attr_data to 0*/
    adv_meta_data.attr_data = 0;

    /*completed - 1: completed, 0: not completed*/
    if (len <= BSA_DM_BLE_AD_SERVICE_DATA_VAL_LEN)
        adv_meta_data.attr_data |= TV_METADATA_COMPLETED_MSK; /*completed*/
    else
        adv_meta_data.attr_data &= ~TV_METADATA_COMPLETED_MSK; /*not completed*/

    /*metadata type*/
    adv_meta_data.attr_data &= ~TV_METADATA_CHANGE_MSK;
    adv_meta_data.attr_data |= (metatype & TV_METADATA_CHANGE_MSK);

    /*update advertisement transaction id to allow clients to detect changes in content */
    adv_meta_data.unique_id++;
    if ((adv_meta_data.unique_id & TV_METADATA_UNIQUE_ID_MSK) == 0)
        adv_meta_data.unique_id = 0;
    TV_METADATA_UPDATE_UNIQUE_ID_VALUE(adv_meta_data.attr_data, adv_meta_data.unique_id);
    APP_DEBUG1("attr_data: 0x%4x", adv_meta_data.attr_data);

    /*length of service data only. 1: length byte itself*/
    adv_conf.service_data_len = BSA_DM_BLE_AD_SERVICE_DATA_LEN-1;

    /*service uuid*/
    adv_conf.service_data_uuid.len = LEN_UUID_16;
    adv_conf.service_data_uuid.uu.uuid16 = adv_meta_data.service_uuid;

    p = adv_conf.service_data_val;
    UINT8_TO_STREAM(p, adv_meta_data.lt_addr);         /*Byte4  LT Addr */
    UINT16_TO_STREAM(p, adv_meta_data.attr_data);      /*Byte5-6: attribute data*/

    memset(p, 0, BSA_DM_BLE_AD_SERVICE_DATA_VAL_LEN);
    if (len > BSA_DM_BLE_AD_SERVICE_DATA_VAL_LEN)
        len = BSA_DM_BLE_AD_SERVICE_DATA_VAL_LEN;      /*max bytes*/
    APP_DEBUG1("AD data len: %d", len);

    ARRAY_TO_STREAM(p, metadata, len);                 /*Byte7~: meta data */

    /*set advertise data*/
    app_dm_set_ble_adv_data(&adv_conf);
}

/*******************************************************************************
**
** Function        app_ble_tvselect_server_change_metadata_chname
**
** Description     Example code for TV application to use to update the metadata channel name
**
** Parameters      None
**
** Returns         None
**
*******************************************************************************/
void app_ble_tvselect_server_change_metadata_chname()
{
    int choice;
    UINT16 len;
    UINT8 channel_name[BSA_BLE_SE_WRITE_MAX];

    /*sample channel name - to be provided from tv application*/
    #define sample_channel_name "sample ch name - CNN"
    #define sample_channel_name_long "sample ch name long - CNN Breaking News: \
        The Internet of Things (IoT) market will grow rapidly for the next six years."

    APP_INFO0("    1: Completed");
    APP_INFO0("    2: Not Completed");
    choice = app_get_choice("Select Completed Type");
    if(choice == 1)
    {
        APP_DEBUG0("Completed - metadata channel name");
        len = sizeof(sample_channel_name);
        strncpy((char*)channel_name, sample_channel_name, len);
    }
    else if (choice == 2)
    {
        APP_DEBUG0("Not Completed - metadata channel name");
        len = sizeof(sample_channel_name_long);
        strncpy((char*)channel_name, sample_channel_name_long, len);
    }
    else
    {
        APP_ERROR1("Wrong selection! = %d", choice);
        return;
    }

    app_ble_tvselect_server_update_advertise_packet(TV_METADATA_CH_NAME, channel_name, len);
}

void app_ble_tvselect_server_change_metadata_chicon()
{
    int choice;
    UINT16 len;
    UINT8 channel_icon[BSA_BLE_SE_WRITE_MAX];

    /*sample channel_icon*/
    #define sample_channel_icon "sample channel icon"
    #define sample_channel_icon_long "sample channel icon long data - \
        to be provided from tv application"

    APP_INFO0("    1: Completed");
    APP_INFO0("    2: Not Completed");
    choice = app_get_choice("Select Completed Type");
    if(choice == 1)
    {
        APP_DEBUG0("Completed - metadata channel icon");
        len = sizeof(sample_channel_icon);
        strncpy((char*)channel_icon, sample_channel_icon, len);
    }
    else if (choice == 2)
    {
        APP_DEBUG0("Not Completed - metadata channel icon");
        len = sizeof(sample_channel_icon_long);
        strncpy((char*)channel_icon, sample_channel_icon_long, len);
    }
    else
    {
        APP_ERROR1("Wrong selection! = %d", choice);
        return;
    }

    app_ble_tvselect_server_update_advertise_packet(TV_METADATA_CH_ICON, channel_icon, len);
}

void app_ble_tvselect_server_change_metadata_medianame()
{
    int choice;
    UINT16 len;
    UINT8 media_name[BSA_BLE_SE_WRITE_MAX];

    /*sample media_name*/
    #define sample_media_name "Kangnam Style"
    #define sample_media_name_long "sample media name long - \
        Gangnam Style is a Korean neologism that refers to a lifestyle associated \
        with the Gangnam District of Seoul. The song and its accompanying music video \
        went viral in August 2012 and have influenced popular culture worldwide since then."

    APP_INFO0("    1: Completed");
    APP_INFO0("    2: Not Completed");
    choice = app_get_choice("Select Completed Type");
    if(choice == 1)
    {
        APP_DEBUG0("Completed - metadata media name");
        len = sizeof(sample_media_name);
        strncpy((char*)media_name, sample_media_name, len);
    }
    else if (choice == 2)
    {
        APP_DEBUG0("Not Completed - metadata media name");
        len = sizeof(sample_media_name_long);
        if (len > BSA_BLE_SE_WRITE_MAX) len = BSA_BLE_SE_WRITE_MAX-1;
        strncpy((char*)media_name, sample_media_name_long, len);
    }
    else
    {
        APP_ERROR1("Wrong selection! = %d", choice);
        return;
    }

    app_ble_tvselect_server_update_advertise_packet(TV_METADATA_PGM_NAME, media_name, len);
}

void app_ble_tvselect_server_change_metadata_mediaicon()
{
    int choice;
    UINT16 len;
    UINT8 media_icon[BSA_BLE_SE_WRITE_MAX];

    /*sample media_icon*/
    #define sample_media_icon "sample media icon"
    #define sample_media_icon_long "sample media icon long data - Oppa Is Just My Style"

    APP_INFO0("    1: Completed");
    APP_INFO0("    2: Not Completed");
    choice = app_get_choice("Select Completed Type");
    if(choice == 1)
    {
        APP_DEBUG0("Completed - metadata media icon");
        len = sizeof(sample_media_icon);
        strncpy((char*)media_icon, sample_media_icon, len);
    }
    else if (choice == 2)
    {
        APP_DEBUG0("Not Completed - metadata media icon");
        len = sizeof(sample_media_icon_long);
        strncpy((char*)media_icon, sample_media_icon_long, len);
    }
    else
    {
        APP_ERROR1("Wrong selection! = %d", choice);
        return;
    }

    app_ble_tvselect_server_update_advertise_packet(TV_METADATA_PGM_ICON, media_icon, len);
}

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
int app_ble_tvselect_server_create_service(void)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_CREATE ble_create_param;

    status = BSA_BleSeCreateServiceInit(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeCreateServiceInit failed status = %d", status);
        return -1;
    }

    ble_create_param.service_uuid.uu.uuid16 = APP_BLE_TVSELECT_SERVER_SERV_UUID;
    ble_create_param.service_uuid.len = LEN_UUID_16;
    ble_create_param.server_if = app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].server_if;
    ble_create_param.num_handle = APP_BLE_TVSELECT_SERVER_NUM_OF_HANDLES;
    ble_create_param.is_primary = TRUE;

    app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].attr[APP_BLE_TVSELECT_SERVER_ATTR_NUM-1].wait_flag = TRUE;

    status = BSA_BleSeCreateService(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeCreateService failed status = %d", status);
        app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].attr[APP_BLE_TVSELECT_SERVER_ATTR_NUM-1].wait_flag = FALSE;
        return -1;
    }

    /* store information on control block */
    app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].attr[APP_BLE_TVSELECT_SERVER_ATTR_NUM-1].attr_UUID.len = 2;
    app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].attr[APP_BLE_TVSELECT_SERVER_ATTR_NUM-1].attr_UUID.uu.uuid16 = APP_BLE_TVSELECT_SERVER_SERV_UUID;
    app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].attr[APP_BLE_TVSELECT_SERVER_ATTR_NUM-1].is_pri = ble_create_param.is_primary;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_tvselect_server_stop
 **
 ** Description     stop tvselect_server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_tvselect_server_stop(void)
{
    /* make undiscoverable & unconnectable */
    app_dm_set_ble_visibility(FALSE, FALSE);

    app_ble_tvselect_server_deregister();

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_tvselect_server_send_response
 **
 ** Description     Send response to client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_tvselect_server_send_response(UINT8 *p, int len)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_SENDIND ble_sendind_param;
    int num;

    num = 0;
    if (app_ble_cb.ble_server[num].enabled != TRUE)
    {
        APP_ERROR1("Server was not registered! = %d", num);
        return -1;
    }

    status = BSA_BleSeSendIndInit(&ble_sendind_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeSendIndInit failed status = %d", status);
        return -1;
    }

    APP_DEBUG1("response data len: %d", len);

    ble_sendind_param.conn_id = app_ble_tvselect_server_cb.conn_id;
    ble_sendind_param.attr_id = app_ble_tvselect_server_cb.attr_id;
    ble_sendind_param.data_len = len;
    memcpy(ble_sendind_param.value, p, len);
    ble_sendind_param.need_confirm = FALSE;

    status = BSA_BleSeSendInd(&ble_sendind_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeSendInd failed status = %d", status);
        return -1;
    }

    return 0;
}

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
int app_ble_tvselect_server_deregister(void)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_DEREGISTER ble_deregister_param;

    APP_INFO0("Bluetooth BLE deregister menu:");
    if (app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].enabled != TRUE)
    {
        APP_ERROR1("Server was not registered! = %d", APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1);
        return -1;
    }

    status = BSA_BleSeAppDeregisterInit(&ble_deregister_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppDeregisterInit failed status = %d", status);
        return -1;
    }

    ble_deregister_param.server_if = app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].server_if;

    status = BSA_BleSeAppDeregister(&ble_deregister_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppDeregister failed status = %d", status);
        return -1;
    }

    return 0;
}

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
int app_ble_tvselect_server_start_service(void)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_START ble_start_param;

    status = BSA_BleSeStartServiceInit(&ble_start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeStartServiceInit failed status = %d", status);
        return -1;
    }

    ble_start_param.service_id = app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].attr[APP_BLE_TVSELECT_SERVER_ATTR_NUM-1].service_id;
    ble_start_param.sup_transport = BSA_BLE_GATT_TRANSPORT_LE_BR_EDR;

    APP_INFO1("service_id:%d, num:%d", ble_start_param.service_id, APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1);

    status = BSA_BleSeStartService(&ble_start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeStartService failed status = %d", status);
        return -1;
    }
    return 0;
}

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
int app_ble_tvselect_server_add_char(void)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_ADDCHAR ble_addchar_param;

    status = BSA_BleSeAddCharInit(&ble_addchar_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAddCharInit failed status = %d", status);
        return -1;
    }

    ble_addchar_param.service_id = app_ble_cb.ble_server[APP_BLE_TVSELECT_SERVER_NUM_OF_SERVER-1].attr[APP_BLE_TVSELECT_SERVER_ATTR_NUM-1].service_id;
    ble_addchar_param.is_descr = FALSE;
    ble_addchar_param.char_uuid.len = LEN_UUID_16;
    ble_addchar_param.char_uuid.uu.uuid16 = APP_BLE_TVSELECT_SERVER_CHAR_UUID;
    ble_addchar_param.perm = BSA_GATT_PERM_READ|BSA_GATT_PERM_WRITE;
    ble_addchar_param.property = BSA_GATT_CHAR_PROP_BIT_NOTIFY |
                                 BSA_GATT_CHAR_PROP_BIT_WRITE |
                                 BSA_GATT_CHAR_PROP_BIT_INDICATE;

    APP_INFO1("app_ble_tvselect_add_char service_id:%d", ble_addchar_param.service_id);

    status = BSA_BleSeAddChar(&ble_addchar_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAddChar failed status = %d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function         app_ble_tvselect_server_profile_cback
**
** Description      TVSELECT Server callback
**
** Returns          void
**
*******************************************************************************/
void app_ble_tvselect_server_handle_commands (UINT8 *p_data)
{
    UINT8 response[BSA_DM_BLE_AD_SERVICE_DATA_VAL_MAX] = {0, };
    UINT8 *p = response;
    UINT16 len;

    switch (*(p_data+1))
    {
        case CMD_BAV_CP_VERSION:
            APP_DEBUG0("command CMD_BAV_CP_VERSION received");
            UINT8_TO_STREAM(p, TVSELECT_TYPE_EVENT);           /*type: response*/
            UINT8_TO_STREAM(p, CMD_BAV_CP_VERSION);            /*opcode*/
            UINT16_TO_STREAM(p, sizeof(adv_meta_data.major_ver)+sizeof(adv_meta_data.minor_ver)); /*param length*/
            *p++ = adv_meta_data.major_ver[0];
            *p++ = adv_meta_data.major_ver[1];
            *p++ = adv_meta_data.minor_ver[0];
            *p++ = adv_meta_data.minor_ver[1];
            len = 8;

            app_ble_tvselect_server_send_response(response, len);
        break;

        case CMD_BAV_CP_STATE:
            APP_DEBUG0("command CMD_BAV_CP_STATE received");
            UINT8_TO_STREAM(p, TVSELECT_TYPE_EVENT);           /*type: response*/
            UINT8_TO_STREAM(p, CMD_BAV_CP_STATE);              /*opcode*/
            UINT16_TO_STREAM(p, sizeof(adv_meta_data.state));   /*param length*/
            *p++ = adv_meta_data.state;
            len = 5;

            app_ble_tvselect_server_send_response(response, len);
        break;

        case CMD_BAV_CP_CHANNEL_NAME:
            APP_DEBUG0("command CMD_BAV_CP_CHANNEL_NAME received");
            UINT8_TO_STREAM(p, TVSELECT_TYPE_EVENT);           /*type: response*/
            UINT8_TO_STREAM(p, CMD_BAV_CP_CHANNEL_NAME);       /*opcode*/
            UINT16_TO_STREAM(p, adv_meta_data.channel_name_len);   /*param length*/
            ARRAY_TO_STREAM(p, adv_meta_data.channel_name, adv_meta_data.channel_name_len);
            len = adv_meta_data.channel_name_len+4;

            APP_DEBUG1("len:%d", len);
            APP_DUMP("ch name", response, len);
            app_ble_tvselect_server_send_response(response, len);
        break;

        case CMD_BAV_CP_CHANNEL_ICON:
            APP_DEBUG0("command CMD_BAV_CP_CHANNEL_ICON received");
            UINT8_TO_STREAM(p, TVSELECT_TYPE_EVENT);           /*type: response*/
            UINT8_TO_STREAM(p, CMD_BAV_CP_CHANNEL_ICON);       /*opcode*/
            UINT16_TO_STREAM(p, adv_meta_data.channel_icon_len);   /*param length*/
            ARRAY_TO_STREAM(p, adv_meta_data.channel_icon, adv_meta_data.channel_icon_len);
            len = adv_meta_data.channel_icon_len+4;

            APP_DEBUG1("len:%d", len);
            APP_DUMP("ch icon", response, len);
            app_ble_tvselect_server_send_response(response, len);
        break;

        case CMD_BAV_CP_MEDIA_NAME:
            APP_DEBUG0("command CMD_BAV_CP_MEDIA_INFO received");
            UINT8_TO_STREAM(p, TVSELECT_TYPE_EVENT);           /*type: response*/
            UINT8_TO_STREAM(p, CMD_BAV_CP_MEDIA_NAME);         /*opcode*/
            UINT16_TO_STREAM(p, adv_meta_data.media_name_len);   /*param length*/
            ARRAY_TO_STREAM(p,  adv_meta_data.media_name, adv_meta_data.media_name_len);
            len = adv_meta_data.media_name_len+4;

            APP_DEBUG1("len:%d", len);
            APP_DUMP("media name", response, len);
            app_ble_tvselect_server_send_response(response, len);
        break;

        case CMD_BAV_CP_MEDIA_ICON:
            APP_DEBUG0("command CMD_BAV_CP_MEDIA_ICON received");
            UINT8_TO_STREAM(p, TVSELECT_TYPE_EVENT);           /*type: response*/
            UINT8_TO_STREAM(p, CMD_BAV_CP_MEDIA_ICON);         /*opcode*/
            UINT16_TO_STREAM(p, adv_meta_data.media_icon_len);   /*param length*/
            ARRAY_TO_STREAM(p, adv_meta_data.media_icon, adv_meta_data.media_icon_len);
            len = adv_meta_data.media_icon_len+4;

            APP_DEBUG1("len:%d", len);
            APP_DUMP("media icon", response, len);
            app_ble_tvselect_server_send_response(response, len);
        break;

        default:
            APP_DEBUG0("command unknown received");
         break;
    }
}

/*******************************************************************************
**
** Function         app_ble_tvselect_server_profile_cback
**
** Description      TVSELECT Server callback
**
** Returns          void
**
*******************************************************************************/
void app_ble_tvselect_server_profile_cback(tBSA_BLE_EVT event, tBSA_BLE_MSG *p_data)
{
    int num, attr_index;

    APP_DEBUG1("event = %d ", event);

    switch (event)
    {
    case BSA_BLE_SE_DEREGISTER_EVT:
        APP_INFO1("BSA_BLE_SE_DEREGISTER_EVT server_if:%d status:%d",
            p_data->ser_deregister.server_if, p_data->ser_deregister.status);
        num = app_ble_server_find_index_by_interface(p_data->ser_deregister.server_if);
        if (num < 0)
        {
            APP_ERROR0("no deregister pending!!");
            break;
        }

        app_ble_cb.ble_server[num].server_if = BSA_BLE_INVALID_IF;
        app_ble_cb.ble_server[num].enabled = FALSE;
        break;

    case BSA_BLE_SE_CREATE_EVT:
        APP_INFO1("BSA_BLE_SE_CREATE_EVT server_if:%d status:%d service_id:%d",
            p_data->ser_create.server_if, p_data->ser_create.status, p_data->ser_create.service_id);

        num = app_ble_server_find_index_by_interface(p_data->ser_create.server_if);

        /* search interface number */
        if(num < 0)
        {
            APP_ERROR0("no interface!!");
            break;
        }

        /* search attribute number */
        for (attr_index = 0 ; attr_index < BSA_BLE_ATTRIBUTE_MAX ; attr_index++)
        {
            if (app_ble_cb.ble_server[num].attr[attr_index].wait_flag == TRUE)
            {
                APP_INFO1("BSA_BLE_SE_CREATE_EVT if_num:%d, attr_num:%d", num, attr_index);
                if (p_data->ser_create.status == BSA_SUCCESS)
                {
                    app_ble_cb.ble_server[num].attr[attr_index].service_id = p_data->ser_create.service_id;
                    app_ble_cb.ble_server[num].attr[attr_index].wait_flag = FALSE;
                    break;
                }
                else  /* if CREATE fail */
                {
                    memset(&app_ble_cb.ble_server[num].attr[attr_index], 0, sizeof(tAPP_BLE_ATTRIBUTE));
                    break;
                }
            }
        }
        if (attr_index >= BSA_BLE_ATTRIBUTE_MAX)
        {
            APP_ERROR0("BSA_BLE_SE_CREATE_EVT no waiting!!");
            break;
        }
        break;

    case BSA_BLE_SE_ADDCHAR_EVT:
        APP_INFO1("BSA_BLE_SE_ADDCHAR_EVT status:%d", p_data->ser_addchar.status);
        if (p_data->ser_addchar.status == BSA_SUCCESS)
        {
            app_ble_tvselect_server_cb.attr_id = p_data->ser_addchar.attr_id;
            APP_INFO1("attr_id:0x%x", p_data->ser_addchar.attr_id);
        }
        break;

    case BSA_BLE_SE_START_EVT:
        APP_INFO1("BSA_BLE_SE_START_EVT status:%d", p_data->ser_start.status);
        break;

    case BSA_BLE_SE_READ_EVT:
        APP_INFO1("BSA_BLE_SE_READ_EVT status:%d", p_data->ser_read.status);
        break;

    case BSA_BLE_SE_WRITE_EVT:
        APP_INFO1("BSA_BLE_SE_WRITE_EVT status:%d", p_data->ser_write.status);
        APP_DUMP("Write value", p_data->ser_write.value, p_data->ser_write.len);
        APP_INFO1("BSA_BLE_SE_WRITE_EVT trans_id:%d, conn_id:%d, handle:%d",
            p_data->ser_write.trans_id, p_data->ser_write.conn_id, p_data->ser_write.handle);

        if (*p_data->ser_write.value == TVSELECT_TYPE_COMMAND)
            app_ble_tvselect_server_handle_commands (p_data->ser_write.value);
        else
            APP_DEBUG0("BSA_BLE_SE_WRITE_EVT wrong command type!");
        break;

    case BSA_BLE_SE_OPEN_EVT:
        APP_INFO1("BSA_BLE_SE_OPEN_EVT status:%d", p_data->ser_open.reason);
        if (p_data->ser_open.reason == BSA_SUCCESS)
        {
            app_ble_tvselect_server_cb.conn_id = p_data->ser_open.conn_id;
            APP_INFO1("conn_id:0x%x", p_data->ser_open.conn_id);
        }
        break;

    case BSA_BLE_SE_CLOSE_EVT:
        APP_INFO1("BSA_BLE_SE_CLOSE_EVT status:%d", p_data->ser_close.reason);
        break;

    default:
        break;
    }
}

