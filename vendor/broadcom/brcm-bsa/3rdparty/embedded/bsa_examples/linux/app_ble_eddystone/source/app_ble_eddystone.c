/*****************************************************************************
**
**  Name:           app_ble_eddystone.c
**
**  Description:    Bluetooth BLE eddystone advertiser application
*                   Implements Google Eddysone spec
*                   https://github.com/google/eddystone/blob/master/protocol-specification.md
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdlib.h>
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"

#include "app_dm.h"
#include "app_ble_eddystone.h"

/* Whether to use muti-adv feature with eddystone advertisement */
/* This feature needs FW support */
// #define APP_BLE_EDDYSTONE_MULTI_ADV

/*
* Defines
*/
/* max adv data length */
#define APP_BLE_EDDYSTONE_MAX_ADV_DATA_LEN        25
/* BLE EDDYSTONE Helper UUID Definitions */

#define APP_BLE_EDDYSTONE_APP_CLIENT_UUID          0x9891

#define APP_BLE_EDDYSTONE_APP_ATTR_NUM           1
#define APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER      1
#define APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT      1
#define APP_BLE_EDDYSTONE_CLIENT_INFO_DATA_LEN   18

#define  ADVERTISE_TX_POWER_ULTRA_LOW 0
#define  ADVERTISE_TX_POWER_LOW 1
#define  ADVERTISE_TX_POWER_MEDIUM 2
#define  ADVERTISE_TX_POWER_HIGH 3

#define  APP_BLE_EDDYSTONE_TX_DBM_HIGH   -16
#define  APP_BLE_EDDYSTONE_TX_DBM_MEDIUM -26
#define  APP_BLE_EDDYSTONE_TX_DBM_LOW    -35
#define  APP_BLE_EDDYSTONE_TX_ULTRA_LOW  -59

#define  APP_BLE_EDDYSTONE_ADV_INT_MIN 0x100
#define  APP_BLE_EDDYSTONE_ADV_INT_MAX 0x1000

/* values from Eddystone specification */
/* https://github.com/google/eddystone/blob/master/protocol-specification.md */
#define APP_BLE_EDDYSTON_FRAME_TYPE_UID   0x00
#define APP_BLE_EDDYSTON_FRAME_TYPE_URL   0x10
#define APP_BLE_EDDYSTON_FRAME_TYPE_TLM   0x20

#define APP_BLE_EDDYSTONE_UUID16         0xFEAA

#define APP_BLE_EDDYSTONE_SERVICE_DAT_LEN  20

#define APP_BLE_EDDYSTONE_UID_NAMESPACE_LEN       10
#define APP_BLE_EDDYSTONE_UID_INSTANCE_ID_LEN     6

#define APP_BLE_EDDYSTONE_URL_DAT_LEN  17

#define APP_BLE_EDDYSTONE_TLM_VERSION     0x00
#define APP_BLE_EDDYSTONE_TLM_VBATT_LEN     2
#define APP_BLE_EDDYSTONE_TLM_TEMP_LEN      2
#define APP_BLE_EDDYSTONE_TLM_ADV_CNT_LEN   4
#define APP_BLE_EDDYSTONE_TLM_SEC_CNT_LEN   4

UINT8 eddystone_uuid[16]= {0x00, 0x00, 0xFE, 0xAA, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

UINT8 eddystone_service_uuid[16]=          {0xee, 0x0c, 0x20, 0x80, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};

UINT8 eddystone_char_lock_state_uuid[16]=  {0xee, 0x0c, 0x20, 0x81, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_lock_uuid[16]=        {0xee, 0x0c, 0x20, 0x82, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_unlock_uuid[16]=      {0xee, 0x0c, 0x20, 0x83, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_uri_data_uuid[16]=    {0xee, 0x0c, 0x20, 0x84, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_uri_flags_uuid[16]=   {0xee, 0x0c, 0x20, 0x85, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_ad_tx_pwr_uuid[16]=   {0xee, 0x0c, 0x20, 0x86, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_tx_pwr_mode_uuid[16]= {0xee, 0x0c, 0x20, 0x87, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_beacon_per_uuid[16]=  {0xee, 0x0c, 0x20, 0x88, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};
UINT8 eddystone_char_reset_uuid[16]=       {0xee, 0x0c, 0x20, 0x88, 0x87, 0x86, 0x40, 0xba, 0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8};

/* END values from Eddystone specification */


/*
 * Global Variables
 */
char app_ble_eddystone_write_data[APP_BLE_EDDYSTONE_CLIENT_INFO_DATA_LEN] = "www.broadcom";
char app_ble_eddystone_received_data[APP_BLE_EDDYSTONE_CLIENT_INFO_DATA_LEN]; //Todo: This received data/URL needs to be passed to STB

UINT8 eddystone_adv_data[APP_BLE_EDDYSTONE_MAX_ADV_DATA_LEN] =
    {0x4C, 0x00, 0x02, 0x15,0xe2, 0xc5, 0x6d, 0xb5, 0xdf, 0xfb, 0x48, 0xd2, 0xb0, 0x60, 0xd0, 0xf5, 0xa7, 0x10, 0x96, 0xe0, 0x00, 0x00, 0x00, 0x00,0xC5};

tAPP_DISCOVERY_CB app_ble_eddystone_init_disc_cb;

UINT8 app_ble_eddystone_index = 1;
BOOLEAN app_ble_eddystone_uid_in_use = FALSE;
BOOLEAN app_ble_eddystone_url_in_use = FALSE;
BOOLEAN app_ble_eddystone_tlm_in_use = FALSE;

tBSA_DM_GET_CONFIG bt_config;
/*
 * Local functions
 */
static void app_ble_eddystone_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data);
static BOOLEAN app_ble_eddystone_find_vs_eir(UINT8 *p_eir);
static int app_ble_eddystone_le_client_close(void);
static int app_ble_eddystone_le_client_deregister(void);
static UINT8 app_ble_eddystone_power_level_to_byte_value(UINT8 txPowerLevel);
static int app_ble_eddystone_disable_adv(UINT8 inst_id);
static int app_ble_eddystone_set_adv_params(UINT8 inst_id);

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_init
 **
 ** Description     init
 **
 ** Parameters      None
 **
 **
 *******************************************************************************/
void app_ble_eddystone_init(void)
{
    /*
     * Get Bluetooth configuration
     */
    BSA_DmGetConfigInit(&bt_config);
    BSA_DmGetConfig(&bt_config);
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_uid_adv
 **
 ** Description     start eddystone UID advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_uid_adv(void)
{
    UINT8  inst_id = 0;

    UINT8 txval = 0;

    UINT8 tx_power_level =  ADVERTISE_TX_POWER_LOW; /* app defined sample data */

    UINT8 frame_type = APP_BLE_EDDYSTON_FRAME_TYPE_UID;

    UINT8 service_data[APP_BLE_EDDYSTONE_SERVICE_DAT_LEN];

    /* namespace and instatance sample data */
    UINT8 namesp[APP_BLE_EDDYSTONE_UID_NAMESPACE_LEN] = { 0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0x0 };
    UINT8 inst[APP_BLE_EDDYSTONE_UID_INSTANCE_ID_LEN] = { 0xA,0xB,0xC,0xD,0xE,0xF};

    tBSA_DM_BLE_ADV_CONFIG adv_conf;

    APP_INFO0("app_ble_eddystone_start_eddystone_uid_adv");

    if(app_ble_eddystone_uid_in_use)
    {
        APP_INFO0("already advertising");
        return 0;
    }

    app_ble_eddystone_uid_in_use = TRUE;

#ifdef APP_BLE_EDDYSTONE_MULTI_ADV
    inst_id = app_ble_eddystone_index++;
#endif

    memset(service_data, 0, APP_BLE_EDDYSTONE_SERVICE_DAT_LEN);

    txval = app_ble_eddystone_power_level_to_byte_value(tx_power_level);

    /* the service data is populated as per Eddystone UID spec */
    /* https://github.com/google/eddystone/tree/master/eddystone-uid */
    memcpy(&service_data[0], &frame_type, sizeof(UINT8));
    memcpy(&service_data[1], &txval, sizeof(UINT8));

    memcpy(&service_data[2], &namesp, APP_BLE_EDDYSTONE_UID_NAMESPACE_LEN);
    memcpy(&service_data[12], &inst, APP_BLE_EDDYSTONE_UID_INSTANCE_ID_LEN);

    memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));

    adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
    adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_SERVICE | BSA_DM_BLE_AD_BIT_SERVICE_DATA | BSA_DM_BLE_AD_BIT_FLAGS ;
    adv_conf.inst_id = inst_id;

    adv_conf.num_service = 1;
    adv_conf.uuid_val[0] = APP_BLE_EDDYSTONE_UUID16;

    adv_conf.service_data_len = APP_BLE_EDDYSTONE_SERVICE_DAT_LEN;
    memcpy(&adv_conf.service_data_val, &service_data, APP_BLE_EDDYSTONE_SERVICE_DAT_LEN);
    adv_conf.service_data_uuid.len = 2;
    adv_conf.service_data_uuid.uu.uuid16 = APP_BLE_EDDYSTONE_UUID16;

    /* set adv params */
    app_ble_eddystone_set_adv_params(inst_id);

    /* start advertising */
    app_dm_set_ble_adv_data(&adv_conf);

    /* Set visible, not connectable */
    app_dm_set_ble_visibility(TRUE, FALSE);


    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_url_adv
 **
 ** Description     start eddystone URL advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_url_adv(void)
{
    UINT8  inst_id = 0;

    UINT8 tx_power_level =  ADVERTISE_TX_POWER_LOW; /* app defined sample data */

    UINT8 txval = app_ble_eddystone_power_level_to_byte_value(tx_power_level);

    UINT8 service_data[APP_BLE_EDDYSTONE_SERVICE_DAT_LEN];

    UINT8 frame_type = APP_BLE_EDDYSTON_FRAME_TYPE_URL;

    UINT8 urlscheme = 0x03; /* prefix https:// */ /* app defined sample data */

    char szURL[] = "goo.gl/S6zT6P"; /* app defined sample data */

    UINT8 urldata[APP_BLE_EDDYSTONE_URL_DAT_LEN];

    tBSA_DM_BLE_ADV_CONFIG adv_conf;

    APP_INFO0("app_ble_eddystone_start_eddystone_url_adv");

    if(app_ble_eddystone_url_in_use)
    {
        APP_INFO0("already advertising");
        return 0;
    }

    app_ble_eddystone_url_in_use = TRUE;

#ifdef APP_BLE_EDDYSTONE_MULTI_ADV
    inst_id = app_ble_eddystone_index++;
#endif

    memset(service_data, 0, APP_BLE_EDDYSTONE_SERVICE_DAT_LEN);

    memset(urldata, 0x0e, APP_BLE_EDDYSTONE_URL_DAT_LEN);  /* text expansion RFU */

    memcpy(urldata, szURL, strlen(szURL));

    /* the service data is populated as per Eddystone ERL spec */
    /* https://github.com/google/eddystone/tree/master/eddystone-url */
    memcpy(&service_data[0], &frame_type, sizeof(UINT8));
    memcpy(&service_data[1], &txval, sizeof(UINT8));

    memcpy(&service_data[2], &urlscheme, sizeof(UINT8));
    memcpy(&service_data[3], &urldata, APP_BLE_EDDYSTONE_URL_DAT_LEN);

    memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));

    adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
    adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_SERVICE | BSA_DM_BLE_AD_BIT_SERVICE_DATA | BSA_DM_BLE_AD_BIT_FLAGS ;
    adv_conf.inst_id = inst_id;

    adv_conf.num_service = 1;
    adv_conf.uuid_val[0] = APP_BLE_EDDYSTONE_UUID16;

    adv_conf.service_data_len = APP_BLE_EDDYSTONE_SERVICE_DAT_LEN;
    memcpy(&adv_conf.service_data_val, &service_data, APP_BLE_EDDYSTONE_SERVICE_DAT_LEN);
    adv_conf.service_data_uuid.len = 2;
    adv_conf.service_data_uuid.uu.uuid16 = APP_BLE_EDDYSTONE_UUID16;

    /* set adv params */
    app_ble_eddystone_set_adv_params(inst_id);

    /* start advertising */
    app_dm_set_ble_adv_data(&adv_conf);

    /* Set visible, not connectable */
    app_dm_set_ble_visibility(TRUE, FALSE);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_tlm_adv
 **
 ** Description     start eddystone TLM advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_tlm_adv(void)
{
    UINT8   inst_id = 0;

    UINT8 i = 0;

    UINT8 service_data[APP_BLE_EDDYSTONE_SERVICE_DAT_LEN];

    UINT8 frame_type = APP_BLE_EDDYSTON_FRAME_TYPE_TLM;

    UINT8 version = APP_BLE_EDDYSTONE_TLM_VERSION;

    UINT8 vbatt[APP_BLE_EDDYSTONE_TLM_VBATT_LEN];

    UINT8 temp[APP_BLE_EDDYSTONE_TLM_TEMP_LEN];

    UINT8 adv_cnt[APP_BLE_EDDYSTONE_TLM_ADV_CNT_LEN];

    UINT8 sec_cnt[APP_BLE_EDDYSTONE_TLM_SEC_CNT_LEN];

    tBSA_DM_BLE_ADV_CONFIG adv_conf;

    APP_INFO0("app_ble_eddystone_start_eddystone_tlm_adv");

    if(app_ble_eddystone_tlm_in_use)
    {
        APP_INFO0("already advertising");
        return 0;
    }

    app_ble_eddystone_tlm_in_use = TRUE;

#ifdef APP_BLE_EDDYSTONE_MULTI_ADV
    inst_id = app_ble_eddystone_index++;
#endif

    memset(service_data, 0, APP_BLE_EDDYSTONE_SERVICE_DAT_LEN);

    memset(vbatt, 0, APP_BLE_EDDYSTONE_TLM_VBATT_LEN);
    vbatt[0] = APP_BLE_EDDYSTONE_TLM_VERSION;

    memset(temp, 0, APP_BLE_EDDYSTONE_TLM_TEMP_LEN);
    temp[1] = 0x01; /* app defined sample data */

    memset(adv_cnt, 0, 4);
    adv_cnt[3] = 0x01; /* app defined sample data */

    memset(sec_cnt, 0, 4);
    sec_cnt[3] = 0x01;

    /* the service data is populated as per Eddystone TLM spec */
    /* https://github.com/google/eddystone/tree/master/eddystone-tlm */

    i = 0;
    memcpy(&service_data[i], &frame_type, sizeof(UINT8));
    i = i+1;
    memcpy(&service_data[i], &version, sizeof(UINT8));
    i = i + 1;
    memcpy(&service_data[i], &vbatt, APP_BLE_EDDYSTONE_TLM_VBATT_LEN);
    i = i + APP_BLE_EDDYSTONE_TLM_VBATT_LEN;
    memcpy(&service_data[i], &temp, APP_BLE_EDDYSTONE_TLM_TEMP_LEN);
    i = i + APP_BLE_EDDYSTONE_TLM_TEMP_LEN;
    memcpy(&service_data[i], &adv_cnt, APP_BLE_EDDYSTONE_TLM_ADV_CNT_LEN);
    i = i + APP_BLE_EDDYSTONE_TLM_ADV_CNT_LEN;
    memcpy(&service_data[i], &sec_cnt, APP_BLE_EDDYSTONE_TLM_SEC_CNT_LEN);

    memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));


    adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
    adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_SERVICE | BSA_DM_BLE_AD_BIT_SERVICE_DATA | BSA_DM_BLE_AD_BIT_FLAGS ;
    adv_conf.inst_id = inst_id;

    adv_conf.num_service = 1;
    adv_conf.uuid_val[0] = APP_BLE_EDDYSTONE_UUID16;

    adv_conf.service_data_len = APP_BLE_EDDYSTONE_SERVICE_DAT_LEN;
    memcpy(&adv_conf.service_data_val, &service_data, APP_BLE_EDDYSTONE_SERVICE_DAT_LEN);
    adv_conf.service_data_uuid.len = 2;
    adv_conf.service_data_uuid.uu.uuid16 = APP_BLE_EDDYSTONE_UUID16;

    /* set adv params */
    app_ble_eddystone_set_adv_params(inst_id);

    /* start advertising */
    app_dm_set_ble_adv_data(&adv_conf);

    /* Set visible, not connectable */
    app_dm_set_ble_visibility(TRUE, FALSE);

    /* For 15 sec, change the running count of advertisement frames and
     * time since beacon power-up fields */
    /* This is because the Android validator app expects the values to change */

    while(i < 30)
    {
        adv_cnt[3] = i; /* app defined sample data */
        sec_cnt[3] = i; /* app defined sample data */

        memcpy(&service_data[6], &adv_cnt, APP_BLE_EDDYSTONE_TLM_ADV_CNT_LEN);
        memcpy(&service_data[10], &sec_cnt, APP_BLE_EDDYSTONE_TLM_SEC_CNT_LEN);

        memcpy(&adv_conf.service_data_val, &service_data, APP_BLE_EDDYSTONE_SERVICE_DAT_LEN);

        app_dm_set_ble_adv_data(&adv_conf);

        i++;

        GKI_delay(500);
    }


    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_stop_eddystone_adv
 **
 ** Description     stop eddystone UID advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_stop_eddystone_adv(void)
{
    APP_INFO0("app_ble_eddystone_stop_eddystone_adv");

#ifdef APP_BLE_EDDYSTONE_MULTI_ADV
    /* stop advertising for all adverting instances */
    while(app_ble_eddystone_index > 1)
    {
        app_ble_eddystone_index--;
        app_ble_eddystone_disable_adv(app_ble_eddystone_index);
    }

#else
    app_ble_eddystone_disable_adv(0);

#endif

    app_ble_eddystone_uid_in_use = FALSE;
    app_ble_eddystone_url_in_use = FALSE;
    app_ble_eddystone_tlm_in_use = FALSE;

    return 0;
}


/*******************************************************************************
 **
 ** Function        app_ble_eddystone_start_eddystone_uid_service
 **
 ** Description     start eddystone UID service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_eddystone_uid_service(void)
{
    APP_INFO0("app_ble_eddystone_start_eddystone_uid_service");

    /* register BLE server app */
    app_ble_server_register_eddystone(app_ble_eddystone_profile_cback);
    GKI_delay(1000);

    /* create a BLE service */
    app_ble_eddystone_create_service();
    GKI_delay(1000);

    /* add characteristics */
    /* these are defined in Eddystone URL config service spec */
    /* https://github.com/google/eddystone/blob/master/eddystone-url/docs/config-service-spec.md */

    app_ble_eddystone_add_char(eddystone_char_lock_state_uuid, BSA_GATT_PERM_READ, BSA_GATT_CHAR_PROP_BIT_READ);
    app_ble_eddystone_add_char(eddystone_char_lock_uuid, BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_WRITE );
    app_ble_eddystone_add_char(eddystone_char_unlock_uuid, BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_WRITE );
    app_ble_eddystone_add_char(eddystone_char_uri_data_uuid, BSA_GATT_PERM_READ | BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_READ | BSA_GATT_CHAR_PROP_BIT_WRITE );
    app_ble_eddystone_add_char(eddystone_char_uri_flags_uuid, BSA_GATT_PERM_READ | BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_READ | BSA_GATT_CHAR_PROP_BIT_WRITE );
    app_ble_eddystone_add_char(eddystone_char_ad_tx_pwr_uuid, BSA_GATT_PERM_READ | BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_READ | BSA_GATT_CHAR_PROP_BIT_WRITE );
    app_ble_eddystone_add_char(eddystone_char_tx_pwr_mode_uuid, BSA_GATT_PERM_READ | BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_READ | BSA_GATT_CHAR_PROP_BIT_WRITE );
    app_ble_eddystone_add_char(eddystone_char_beacon_per_uuid, BSA_GATT_PERM_READ | BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_READ | BSA_GATT_CHAR_PROP_BIT_WRITE );
    app_ble_eddystone_add_char(eddystone_char_reset_uuid, BSA_GATT_PERM_WRITE, BSA_GATT_CHAR_PROP_BIT_WRITE );

    GKI_delay(1000);

    /* start service */
    app_ble_eddystone_start_service();
    GKI_delay(1000);

    /* Set visisble and connectable */
    app_dm_set_ble_visibility(TRUE, TRUE);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_server_register_eddystone
 **
 ** Description     Register server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_server_register_eddystone(tBSA_BLE_CBACK *p_cback)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_REGISTER ble_register_param;
    int server_num;

    server_num = app_ble_server_find_free_server();

    status = BSA_BleSeAppRegisterInit(&ble_register_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppRegisterInit failed status = %d", status);
        return -1;
    }

    ble_register_param.uuid.len = 2;
    ble_register_param.uuid.uu.uuid16 = APP_BLE_EDDYSTONE_UUID16;

    if (p_cback == NULL)
    {
        ble_register_param.p_cback = app_ble_server_profile_cback;
    }
    else
    {
        ble_register_param.p_cback = p_cback;
    }

    status = BSA_BleSeAppRegister(&ble_register_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppRegister failed status = %d", status);
        return -1;
    }
    app_ble_cb.ble_server[server_num].enabled = TRUE;
    app_ble_cb.ble_server[server_num].server_if = ble_register_param.server_if;
    APP_INFO1("enabled:%d, server_if:%d", app_ble_cb.ble_server[server_num].enabled,
                    app_ble_cb.ble_server[server_num].server_if);
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_stop_eddystone_uid_service
 **
 ** Description     stop eddystone UID advertisement
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_stop_eddystone_uid_service(void)
{
    /* make undiscoverable & unconnectable */
    app_dm_set_ble_visibility(FALSE, FALSE);

    app_ble_eddystone_deregister();

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_deregister
 **
 ** Description     Deregister server app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_deregister(void)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_DEREGISTER ble_deregister_param;

    APP_INFO0("Bluetooth BLE deregister menu:");
    if(app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].enabled != TRUE)
    {
        APP_ERROR1("Server was not registered! = %d", APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1);
        return -1;
    }

    status = BSA_BleSeAppDeregisterInit(&ble_deregister_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAppDeregisterInit failed status = %d", status);
        return -1;
    }

    ble_deregister_param.server_if = app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].server_if;

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
 ** Function        app_ble_eddystone_create_service
 **
 ** Description     create service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_create_service(void)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_CREATE ble_create_param;

    status = BSA_BleSeCreateServiceInit(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeCreateServiceInit failed status = %d", status);
        return -1;
    }

    ble_create_param.service_uuid.len = 16;
    memcpy(&ble_create_param.service_uuid.uu.uuid128, eddystone_service_uuid, MAX_UUID_SIZE);


    ble_create_param.server_if = app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].server_if;
    ble_create_param.num_handle = 20;
    ble_create_param.is_primary = TRUE;

    app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].attr[APP_BLE_EDDYSTONE_APP_ATTR_NUM-1].wait_flag = TRUE;

    status = BSA_BleSeCreateService(&ble_create_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeCreateService failed status = %d", status);
        app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].attr[APP_BLE_EDDYSTONE_APP_ATTR_NUM-1].wait_flag = FALSE;
        return -1;
    }

    /* store information on control block */
    app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].attr[APP_BLE_EDDYSTONE_APP_ATTR_NUM-1].attr_UUID.len = 16;
    memcpy(&app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].attr[APP_BLE_EDDYSTONE_APP_ATTR_NUM-1].attr_UUID.uu.uuid128, eddystone_service_uuid, MAX_UUID_SIZE);
    app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].attr[APP_BLE_EDDYSTONE_APP_ATTR_NUM-1].is_pri = ble_create_param.is_primary;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_add_char
 **
 ** Description     Add character to service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_add_char(UINT8 *uuid_char, tBSA_BLE_PERM  perm, tBSA_BLE_CHAR_PROP prop)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_ADDCHAR ble_addchar_param;


    status = BSA_BleSeAddCharInit(&ble_addchar_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeAddCharInit failed status = %d", status);
        return -1;
    }

    /* sample characteristic */
    ble_addchar_param.service_id = app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].attr[APP_BLE_EDDYSTONE_APP_ATTR_NUM-1].service_id;
    ble_addchar_param.is_descr = FALSE;
    ble_addchar_param.char_uuid.len = 16;
    memcpy(&ble_addchar_param.char_uuid.uu.uuid128, uuid_char, MAX_UUID_SIZE);
    ble_addchar_param.perm = perm;
    ble_addchar_param.property = prop ;

    APP_INFO1("app_ble_eddystone_add_char service_id:%d", ble_addchar_param.service_id);

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
 ** Function        app_ble_eddystone_start_service
 **
 ** Description     Start Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_service(void)
{
    tBSA_STATUS status;
    tBSA_BLE_SE_START ble_start_param;

    status = BSA_BleSeStartServiceInit(&ble_start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleSeStartServiceInit failed status = %d", status);
        return -1;
    }

    ble_start_param.service_id = app_ble_cb.ble_server[APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1].attr[APP_BLE_EDDYSTONE_APP_ATTR_NUM-1].service_id;
    ble_start_param.sup_transport = BSA_BLE_GATT_TRANSPORT_LE_BR_EDR;

    APP_INFO1("service_id:%d, num:%d", ble_start_param.service_id, APP_BLE_EDDYSTONE_APP_NUM_OF_SERVER-1);

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
 ** Function        app_ble_eddystone_start_le_client
 **
 ** Description     start LE client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_start_le_client(void)
{
    int ret_value = 0;

    /* register LE Client app */
    ret_value = app_ble_client_register(APP_BLE_EDDYSTONE_APP_CLIENT_UUID);
    if (ret_value < 0)
    {
        APP_ERROR1("app_ble_client_register failed ret_value = %d", ret_value);
        return ret_value;
    }

    /* connect to EDDYSTONE Server */
    ret_value = app_ble_client_open();
    if (ret_value < 0)
    {
        APP_ERROR1("app_ble_client_open failed ret_value = %d", ret_value);
        return ret_value;
    }

    return ret_value;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_stop_le_client
 **
 ** Description     stop LE client
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_stop_le_client(void)
{
    /* close connection */
    app_ble_eddystone_le_client_close();

    /* Deregister application */
    app_ble_eddystone_le_client_deregister();

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_le_client_close
 **
 ** Description     This closes connection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_eddystone_le_client_close(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_CLOSE ble_close_param;

    if (app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].enabled == FALSE)
    {
        APP_ERROR1("Wrong client number! = %d", APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1);
        return -1;
    }
    status = BSA_BleClCloseInit(&ble_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClCLoseInit failed status = %d", status);
        return -1;
    }
    ble_close_param.conn_id = app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].conn_id;
    status = BSA_BleClClose(&ble_close_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClClose failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_le_client_deregister
 **
 ** Description     This deregister client app
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_eddystone_le_client_deregister(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_DEREGISTER ble_appdereg_param;

    if (app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].enabled == FALSE)
    {
        APP_ERROR1("Wrong client number! = %d", APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1);
        return -1;
    }
    status = BSA_BleClAppDeregisterInit(&ble_appdereg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClAppDeregisterInit failed status = %d", status);
        return -1;
    }
    ble_appdereg_param.client_if = app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].client_if;
    status = BSA_BleClAppDeregister(&ble_appdereg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleAppDeregister failed status = %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_ble_eddystone_init_disc_cback
 **
 ** Description      Discovery callback
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_eddystone_init_disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data)
{
    UINT16 index;

    switch (event)
    {
    /* a New Device has been discovered */
    case BSA_DISC_NEW_EVT:
        APP_INFO1("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->disc_new.bd_addr[0],
                p_data->disc_new.bd_addr[1],
                p_data->disc_new.bd_addr[2],
                p_data->disc_new.bd_addr[3],
                p_data->disc_new.bd_addr[4],
                p_data->disc_new.bd_addr[5]);
        APP_INFO1("\tName:%s", p_data->disc_new.name);

        if (p_data->disc_new.eir_data[0])
        {
            /* discover EIR to find a specific ADV message */
            if (app_ble_eddystone_find_vs_eir(p_data->disc_new.eir_data))
            {
                /* check if this device has already been received (update) */
                for (index = 0; index < APP_DISC_NB_DEVICES; index++)
                {
                    if ((app_ble_eddystone_init_disc_cb.devs[index].in_use == TRUE) &&
                        (!bdcmp(app_ble_eddystone_init_disc_cb.devs[index].device.bd_addr, p_data->disc_new.bd_addr)))
                    {
                        /* Update device */
                        APP_INFO1("Update device:%d", index);
                        app_ble_eddystone_init_disc_cb.devs[index].device = p_data->disc_new;
                        break;
                    }
                }

                /* if this is new discovered device */
                if (index >= APP_DISC_NB_DEVICES)
                {
                    /* Look for a free place to store dev info */
                    for (index = 0; index < APP_DISC_NB_DEVICES; index++)
                    {
                        if (app_ble_eddystone_init_disc_cb.devs[index].in_use == FALSE)
                        {
                            APP_INFO1("New Discovered device:%d", index);
                            app_ble_eddystone_init_disc_cb.devs[index].in_use = TRUE;
                            memcpy(&app_ble_eddystone_init_disc_cb.devs[index].device, &p_data->disc_new,
                                sizeof(tBSA_DISC_REMOTE_DEV));
                            break;
                        }
                    }
                }
            }
        }
        break;

    case BSA_DISC_CMPL_EVT: /* Discovery complete. */
        break;

    case BSA_DISC_DEV_INFO_EVT: /* Discovery Device Info */
        break;

    default:
        break;
    }
}


/*******************************************************************************
 **
 ** Function         app_ble_eddystone_find_vs_eir
 **
 ** Description      This function is used to find
 **                   a specific ADV content in EIR data
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_ble_eddystone_find_vs_eir(UINT8 *p_eir)
{
    UINT8 *p = p_eir;
    UINT8 eir_length;
    UINT8 eir_tag;
    BOOLEAN discovered = FALSE;

    while(1)
    {
        /* Read Tag's length */
        STREAM_TO_UINT8(eir_length, p);

        if (eir_length == 0)
        {
            break;    /* Last Tag */
        }
        eir_length--;

        /* Read Tag Id */
        STREAM_TO_UINT8(eir_tag, p);
        APP_DEBUG1("app_ble_eddystone_find_vs_eir, eir_tag:%d", eir_tag);

        switch(eir_tag)
        {
        case HCI_EIR_MANUFACTURER_SPECIFIC_TYPE:
            if (eir_length == APP_BLE_EDDYSTONE_MAX_ADV_DATA_LEN)
            {
                if (!memcmp(p, &eddystone_adv_data, APP_BLE_EDDYSTONE_MAX_ADV_DATA_LEN))
                {
                    APP_INFO0("Discovered IBeacon Advertiser!");
                    discovered = TRUE;
                }
            }
            APP_DUMP("MANUFACTURER_SPECIFIC_TYPE", p, eir_length);
            break;
        default:
            break;
        }
        p += eir_length;
    }

    return discovered;
}

/*******************************************************************************
**
** Function         app_ble_eddystone_profile_cback
**
** Description      BLE Server Profile callback.
**
** Returns          void
**
*******************************************************************************/
static void app_ble_eddystone_profile_cback(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    int num, attr_index;

    APP_DEBUG1("event = %d ", event);

    switch (event)
    {
    case BSA_BLE_SE_DEREGISTER_EVT:
        APP_INFO1("BSA_BLE_SE_DEREGISTER_EVT server_if:%d status:%d",
            p_data->ser_deregister.server_if, p_data->ser_deregister.status);
        num = app_ble_server_find_index_by_interface(p_data->ser_deregister.server_if);
        if(num < 0)
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
        memcpy(app_ble_eddystone_received_data, p_data->ser_write.value, p_data->ser_write.len);
        APP_DEBUG1("Received value len:%d, val:%s", p_data->ser_write.len,app_ble_eddystone_received_data);
        break;

    case BSA_BLE_SE_OPEN_EVT:
        APP_INFO1("BSA_BLE_SE_OPEN_EVT status:%d", p_data->ser_open.reason);
        if (p_data->ser_open.reason == BSA_SUCCESS)
        {
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

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_send_data_to_eddystone_server
 **
 ** Description     Write data to Ibeacon Server
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_ble_eddystone_send_data_to_eddystone_server(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    if (app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].write_pending)
    {
        APP_ERROR0("app_ble_wifi_write_information failed : write pending!");
        return -1;
    }

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status = %d", status);
    }

    ble_write_param.char_id.char_id.uuid.len=16;
    memcpy(&ble_write_param.char_id.char_id.uuid.uu.uuid128, eddystone_char_uri_data_uuid, MAX_UUID_SIZE);

    ble_write_param.char_id.srvc_id.id.uuid.len=16;
    memcpy(&ble_write_param.char_id.srvc_id.id.uuid.uu.uuid128, eddystone_service_uuid, MAX_UUID_SIZE);

    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.len = strlen(app_ble_eddystone_write_data);
    memcpy(ble_write_param.value, app_ble_eddystone_write_data, ble_write_param.len);
    ble_write_param.descr = FALSE;
    ble_write_param.conn_id = app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].write_pending = TRUE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        app_ble_cb.ble_client[APP_BLE_EDDYSTONE_APP_NUM_OF_CLIENT-1].write_pending = FALSE;
        APP_ERROR1("BSA_BleClWrite failed status = %d", status);
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function        app_ble_eddystone_power_level_to_byte_value
 **
 ** Description     get eddystone power level in dbm
 **
 ** Parameters      Tx power level
 **
 ** Returns         power level
 **
 *******************************************************************************/
static UINT8 app_ble_eddystone_power_level_to_byte_value(UINT8 txPowerLevel)
{
    switch (txPowerLevel)
    {
      case ADVERTISE_TX_POWER_HIGH:
        return APP_BLE_EDDYSTONE_TX_DBM_HIGH;
      case ADVERTISE_TX_POWER_MEDIUM:
        return APP_BLE_EDDYSTONE_TX_DBM_MEDIUM;
      case ADVERTISE_TX_POWER_LOW:
        return APP_BLE_EDDYSTONE_TX_DBM_LOW;
      case ADVERTISE_TX_POWER_ULTRA_LOW:
      default:
        return APP_BLE_EDDYSTONE_TX_ULTRA_LOW;
    }
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_disable_adv
 **
 ** Description     Disable advertisments
 **
 ** Parameters      instance ID of the advertisement (0 if non multi-adv)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_eddystone_disable_adv(UINT8 inst_id)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_INFO1("app_ble_eddystone_disable_adv ID:%d", inst_id);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set visibility configuration */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_VISIBILITY_MASK;
    bt_config.ble_discoverable = FALSE;
    bt_config.adv_config.inst_id = inst_id;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_eddystone_set_adv_params
 **
 ** Description     set adv params
 **
 ** Parameters      instance ID of the advertisement (0 if non multi-adv)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_ble_eddystone_set_adv_params(UINT8 inst_id)
{
    /* set adv params */
    tBSA_DM_BLE_ADV_PARAM params;
    memset(&params, 0, sizeof(tBSA_DM_BLE_ADV_PARAM));

    /* app configurable sample paramteres */
    params.adv_filter_policy = AP_SCAN_CONN_ALL;
    params.adv_type = BTA_BLE_NON_CONNECT_EVT;
    params.channel_map = BTA_BLE_ADV_CHNL_37 | BTA_BLE_ADV_CHNL_38 | BTA_BLE_ADV_CHNL_39;
    params.tx_power = BTA_BLE_ADV_TX_POWER_MID;
    params.adv_int_min = APP_BLE_EDDYSTONE_ADV_INT_MIN;
    params.adv_int_max = APP_BLE_EDDYSTONE_ADV_INT_MAX;
    params.inst_id = inst_id;
    params.dir_bda.type = BSA_DM_BLE_ADDR_PUBLIC;
    bdcpy(params.dir_bda.bd_addr, bt_config.bd_addr);

    return app_dm_set_ble_adv_param(&params);
}