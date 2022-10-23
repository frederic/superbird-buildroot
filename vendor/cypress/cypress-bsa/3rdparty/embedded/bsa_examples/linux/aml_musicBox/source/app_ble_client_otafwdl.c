/*****************************************************************************
**
**  Name:           app_ble_client_otafwdl.c
**
**  Description:    Bluetooth LE OTA fw download application
**
**  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "app_ble.h"
#include "app_utils.h"
#include "app_ble_client.h"
#include "app_ble_client_otafwdl.h"

/*
 * Definitions
 */

#define MIN(a,b)                                        ((a)>(b)?b:a)

#define  APP_BLE_CLIENT_OTAFWDL_STRING_DATA_LEN         2
#define  APP_BLE_CLIENT_OTAFWDL_STRING_ADDRESS_LEN      4
#define  APP_BLE_CLIENT_OTAFWDL_STRING_DATA_RECORD_TYPE 2
#define  APP_BLE_CLIENT_OTAFWDL_STRING_CHECKSUM_LENGTH  2
#define  APP_BLE_CLIENT_OTAFWDL_STRING_SKIP_CRLF        2
#define  APP_BLE_CLIENT_OTAFWDL_SKIP_BYTE       (APP_BLE_CLIENT_OTAFWDL_STRING_DATA_LEN + \
                                                 APP_BLE_CLIENT_OTAFWDL_STRING_ADDRESS_LEN + \
                                                 APP_BLE_CLIENT_OTAFWDL_STRING_DATA_RECORD_TYPE)

#define  APP_BLE_CLIENT_OTAFWDL_CMD_PREPARE_DOWNLOAD    1
#define  APP_BLE_CLIENT_OTAFWDL_CMD_DOWNLOAD            2
#define  APP_BLE_CLIENT_OTAFWDL_CMD_VERIFY              3

#define  APP_BLE_CLIENT_OTAFWDL_MTU_DEF                 20
#ifndef APP_BLE_CLIENT_OTAFWDL_MTU_REQ_SIZE
#define APP_BLE_CLIENT_OTAFWDL_MTU_REQ_SIZE             243
#endif

#define  APP_BLE_CLIENT_OTAFWDL_WIDTH                   32
#define  APP_BLE_CLIENT_OTAFWDL_TOPBIT                  (1 << (APP_BLE_CLIENT_OTAFWDL_WIDTH - 1))
#define  APP_BLE_CLIENT_OTAFWDL_POLYNOMIAL              0x04C11DB7
#define  APP_BLE_CLIENT_OTAFWDL_INITIAL_REMAINDER       0xFFFFFFFF
#define  APP_BLE_CLIENT_OTAFWDL_FINAL_XOR_VALUE         0xFFFFFFFF

typedef enum
{
    OTAFU_STATE_IDLE,
    OTAFU_STATE_MTU,
    OTAFU_STATE_PRE_INIT,
    OTAFU_STATE_WAIT_DOWNLOAD_NOTIFY,
    OTAFU_STATE_REC_DOWNLOAD_NOTIFY,
    OTAFU_STATE_WAIT_DOWNLOAD_LENGTH_NOTIFY,
    OTAFU_STATE_REC_DOWNLOAD_LENGTH_NOTIFY,
    OTAFU_STATE_WRITE_DATA,
    OTAFU_STATE_DATA_VERIFY,
    OTAFU_STATE_DATA_VERIFY_CHECK,
} tAPP_BLE_CLIENT_OTAFWDL_STATE;

typedef struct
{
    UINT8       *p_ota_binary;
    UINT32      ota_binary_len;
    UINT32      ota_binary_idx;
    UINT32      ota_need_send_cnt;
    UINT32      ota_send_cnt;
    UINT32      mcrc ;
    UINT32      client_num;
    UINT16      conn_id;
    UINT16      mtu;
    tAPP_BLE_CLIENT_OTAFWDL_STATE state;
} tAPP_BLE_CLIENT_OTAFWDL_CB;

/*
 * Local function
 */
static void    app_ble_client_otafwdl_uuip128_convert(char uuid[], UINT8 *p_dest_buf);
static UINT32  app_ble_client_otafwdl_hex2int(char t);
static int     app_ble_client_otafwdl_register_notification(void);
static int     app_ble_client_otafwdl_send_cmd(UINT32 client_num,UINT32 len, unsigned char *data);
static int     app_ble_client_otafwdl_start();
static int     app_ble_client_otafwdl_write_data(UINT32 client_num,UINT32 len, unsigned char *data);
static int     app_ble_client_otafwdl_hex_parse(char *p_ota_fw_file, UINT8 **pp_ota_binary);
static int     app_ble_client_otafwdl_bin_parse(char *p_ota_fw_file, UINT8 **pp_ota_binary);

static void    app_ble_client_otafwdl_crc_calculate(void);
static UINT32  app_ble_client_otafwdl_crc_reflect(UINT32 data,UINT32 nbits);
static UINT32  app_ble_client_otafwdl_crc_slow(UINT32 crc32 , UINT8 *data, UINT32 nbytes);
static UINT32  app_ble_client_otafwdl_crc_complete(UINT32 crc32);;
static UINT32  app_ble_client_otafwdl_crc_reflect_data(UINT32 data);
static UINT32  app_ble_client_otafwdl_crc_reflect_remainder(UINT32 data);
static void    app_ble_client_otafwdl_done(tAPP_BLE_CLIENT_OTAFWDL_STATUS status);
static int     app_ble_client_otafwdl_mtu_req(UINT16 mtu);

static void     app_ble_client_otafwdl_state_set(tAPP_BLE_CLIENT_OTAFWDL_STATE state);
static tAPP_BLE_CLIENT_OTAFWDL_STATE app_ble_client_otafwdl_state_get(void);
static char     *app_ble_client_otafwdl_state_get_desc(tAPP_BLE_CLIENT_OTAFWDL_STATE state);

/*
 * Global variable
 */
static tAPP_BLE_CLIENT_OTAFWDL_CB app_ble_client_otafwdl_cb;

#define APP_BLE_20735_LE2_FWDL


#ifdef APP_BLE_20735_LE2_FWDL

const char APP_BLE_CLIENT_FWDL_OTA_SERVICE_ID[]       =  "C7261110F425447AA1BD9D7246768BD8";
const char APP_BLE_CLIENT_FWDL_OTA_CHAR_NOTIFY_ID[]   =  "A3DD50BFF7A74E99838E570A086C661B ";
const char APP_BLE_CLIENT_FWDL_OTA_CHAR_HEX_DATA_ID[] =  "A2E86C7AD9614091B74F2409E72EFE26";

#else

const char APP_BLE_CLIENT_FWDL_OTA_SERVICE_ID[]       =  "9E5D1E475C1343A0863582AD38A1386F";
const char APP_BLE_CLIENT_FWDL_OTA_CHAR_NOTIFY_ID[]   =  "E3DD50BFF7A74E99838E570A086C666B";
const char APP_BLE_CLIENT_FWDL_OTA_CHAR_HEX_DATA_ID[] =  "92E86C7AD9614091B74F2409E72EFE36";

#endif

/*******************************************************************************
**
**   Function       app_ble_client_otafwdl_init
**
**  Description     initial fw control block
**
**
**  Returns         none
**
***************************************************************************/
int app_ble_client_otafwdl_init(void)
{
    memset(&app_ble_client_otafwdl_cb, 0, sizeof(app_ble_client_otafwdl_cb));
    app_ble_client_otafwdl_state_set(OTAFU_STATE_IDLE);
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_client_otafwdl
 **
 ** Description     do preparation for fw upgrade
 **
 ** Returns         Status
 **
 ***************************************************************************/
int app_ble_client_otafwdl(void)
{
    int len;
    char filename[100];
    int status;
    UINT32 client_num;
    UINT16 conn_id;

    if (app_ble_client_otafwdl_state_get() != OTAFU_STATE_IDLE)
    {
        APP_ERROR0("BLE OTA FW Update Already ongoing");
        return -1;
    }

    len = app_get_string("Enter fw file name:", filename, sizeof(filename));
    if (len < 0)
    {
        APP_ERROR0("app_get_string failed");
        return -1;
    }

    app_ble_client_display(0);
    client_num = app_get_choice("Select Client Index");
    conn_id = app_ble_client_find_conn_id_by_index(client_num);
    if(conn_id == BSA_BLE_INVALID_CONN)
    {
        APP_ERROR1("BLE Client Index:%d nor connected", client_num);
        return -1;
    }

    app_ble_client_otafwdl_cb.client_num = client_num;
    app_ble_client_otafwdl_cb.conn_id = conn_id;
    app_ble_client_otafwdl_cb.mtu = APP_BLE_CLIENT_OTAFWDL_MTU_DEF;

    app_ble_client_otafwdl_state_set(OTAFU_STATE_MTU);

#ifdef APP_BLE_20735_LE2_FWDL
    /* Read bin file */
    status = app_ble_client_otafwdl_bin_parse(filename, &app_ble_client_otafwdl_cb.p_ota_binary);
#else
    /* Parse the HEX file */
    status = app_ble_client_otafwdl_hex_parse(filename, &app_ble_client_otafwdl_cb.p_ota_binary);
#endif

    if (status < 0)
    {
        goto otafwdl_end;
    }
    app_ble_client_otafwdl_cb.ota_binary_len = (UINT32)status;
    APP_INFO1("download size:%d", app_ble_client_otafwdl_cb.ota_binary_len);

    app_ble_client_otafwdl_cb.ota_need_send_cnt = app_ble_client_otafwdl_cb.ota_binary_len;
    app_ble_client_otafwdl_cb.ota_need_send_cnt /=  APP_BLE_CLIENT_OTAFWDL_MTU_DEF;
    app_ble_client_otafwdl_cb.ota_need_send_cnt += 1;

    /* Calculate the CRC of the Binary data */
    app_ble_client_otafwdl_crc_calculate();

    /* Register BLE 'Download' notification */
    status = app_ble_client_otafwdl_register_notification();
    if (status < 0)
    {
        goto otafwdl_end;
    }

    /* Send MTU Request to try to speedup download */
    status = app_ble_client_otafwdl_mtu_req(APP_BLE_CLIENT_OTAFWDL_MTU_REQ_SIZE);
    if (status < 0)
    {
        goto otafwdl_end;
    }

otafwdl_end:
    if (status < 0)
    {
        app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
    }
    return status;
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_hex2int
**
** Description     hex number to integer
**
**
**  Returns         the mapped number
**
*********************************************************************************/
static UINT32 app_ble_client_otafwdl_hex2int(char t)
{
    t = toupper(t);
    switch(t)
    {
        case '0':return 0;
        case '1':return 1;
        case '2':return 2;
        case '3':return 3;
        case '4':return 4;
        case '5':return 5;
        case '6':return 6;
        case '7':return 7;
        case '8':return 8;
        case '9':return 9;
        case 'A':return 10;
        case 'B':return 11;
        case 'C':return 12;
        case 'D':return 13;
        case 'E':return 14;
        case 'F':return 15;
        default :return 0;   /* actually should not go to defalut hanlde, unless the hexfile is wrong */
    }
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_hex_parse
**
** Description     calculate each hex chunk size
**
**  Returns         the chunk size
**
 ***************************************************************************/
static int app_ble_client_otafwdl_hex_parse(char *p_ota_fw_file, UINT8 **pp_ota_binary)
{
    FILE *fp;
    UINT16 len, offset;
    UINT8 linebuf[1024], type;
    BOOLEAN successful;
    int binary_len;
    UINT8 *p_ota_bin, *p;

    /* Open the file */
    fp = fopen(p_ota_fw_file, "r");
    if (fp == NULL)
    {
        APP_ERROR1("Failed opening %s file", p_ota_fw_file);
        return -1;
    }

    /*
     * Parse the Hex file one time to calculate the Binary size
     */

    /* Read all the records in the firmware file */
    successful = FALSE;
    binary_len = 0;
    while (TRUE)
    {
        len = sizeof(linebuf);
        if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
        {
            /* Not successful, should finish on a type 1 record */
            break;
        }

        if (type == 0) /* DATA record found */
        {
            binary_len += len;
        }
        else if (type == 1) /* END record */
        {
            successful = TRUE;
            break;
        }
    }

    if (successful == FALSE)
    {
        APP_ERROR0("No End Record section found");
        fclose(fp);
        return -1;
    }

    /* Seek to the beginning of the file */
    fseek(fp, 0, SEEK_SET);

    /* Allocate a buffer to save the FW */
    p_ota_bin = malloc(binary_len);
    if (p_ota_bin == NULL)
    {
        APP_ERROR0("No more memory");
        fclose(fp);
        return -1;
    }

    /*
     * Parse again the FW, to extract every 'Data' section
     */
    p = p_ota_bin;
    while (TRUE)
    {
        len = sizeof(linebuf);
        if (app_hex_read(fp, &type, &offset, linebuf, &len) == -1)
        {
            /* Not successful, should finish on a type 1 record */
            break;
        }

        if (type == 0) /* DATA record found */
        {
            /* Extract (copy) it */
            memcpy(p, linebuf, len);
            p += len;
        }
    }

    *pp_ota_binary = p_ota_bin;

    return binary_len;
}


/*******************************************************************************
**
** Function        app_ble_client_otafwdl_bin_parse
**
** Description     read signed bin file
**
**  Returns         the chunk size
**
 ***************************************************************************/
static int app_ble_client_otafwdl_bin_parse(char *p_ota_fw_file, UINT8 **pp_ota_binary)
{
    FILE *fp;
    int binary_len = 0;
    UINT8 *p_ota_bin;
    struct stat st;

    /* Open the file */
    fp = fopen(p_ota_fw_file, "r");
    if (fp == NULL)
    {
        APP_ERROR1("Failed opening %s file", p_ota_fw_file);
        return -1;
    }

    stat(p_ota_fw_file, &st);
    binary_len = st.st_size;
    APP_DEBUG1("FW Binary size %d byte ", binary_len);
    p_ota_bin = malloc(binary_len);
    if (p_ota_bin == NULL)
    {
        APP_ERROR0("No more memory");
        fclose(fp);
        return -1;
    }

    /* Read all the records in the firmware file */
    fread(p_ota_bin, binary_len, 1, fp);

    fclose(fp);

    *pp_ota_binary = p_ota_bin;

    return binary_len;
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_uuip128_convert
**
** Description     convert UUID128 to hex number
**
**
**  Returns         None
**
***************************************************************************/
static void app_ble_client_otafwdl_uuip128_convert(char *p_uuid_str, UINT8 *p_dest_buf)
{
   int len = strlen(p_uuid_str);
   int i;
   int left, right;
   for(i = 0 ; i < len ; i += 2)
   {
       left = app_ble_client_otafwdl_hex2int(p_uuid_str[i]) << 4;
       right = app_ble_client_otafwdl_hex2int(p_uuid_str[i + 1]);
       p_dest_buf[(len - i) / 2 - 1]= left + right;
   }
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_register_notification
**
** Description     This is the register function to receive a notification
**
** Parameters      None
**
** Returns         status: 0 if success / -1 otherwise
**
*******************************************************************************/
static int app_ble_client_otafwdl_register_notification(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_NOTIFREG ble_notireg_param;
    UINT32  client_num = app_ble_client_otafwdl_cb.client_num;

    status = BSA_BleClNotifRegisterInit(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegisterInit failed status:%d", status);
        return -1;
    }
    app_ble_client_otafwdl_uuip128_convert((char *)APP_BLE_CLIENT_FWDL_OTA_SERVICE_ID,
            ble_notireg_param.notification_id.srvc_id.id.uuid.uu.uuid128);

    ble_notireg_param.notification_id.srvc_id.id.uuid.len = 16;
    ble_notireg_param.notification_id.srvc_id.is_primary = 1;
    ble_notireg_param.notification_id.srvc_id.id.inst_id = 0;

    app_ble_client_otafwdl_uuip128_convert((char *)APP_BLE_CLIENT_FWDL_OTA_CHAR_NOTIFY_ID,
            ble_notireg_param.notification_id.char_id.uuid.uu.uuid128);

    ble_notireg_param.notification_id.char_id.uuid.len = 16;
    ble_notireg_param.notification_id.char_id.inst_id = 0;

    bdcpy(ble_notireg_param.bd_addr, app_ble_cb.ble_client[client_num].server_addr);
    ble_notireg_param.client_if = app_ble_cb.ble_client[client_num].client_if;

    status = BSA_BleClNotifRegister(&ble_notireg_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClNotifRegister failed status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_client_otafwdl_start
 **
 ** Description     indicate the remote the fw upgrade procedure start
 **
 **  Returns        Status
 **
 *****************************************************************************/
static int app_ble_client_otafwdl_start(void)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;
    UINT32 client_num = app_ble_client_otafwdl_cb.client_num;

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status:%d", status);
        return -1;
    }
    app_ble_client_otafwdl_uuip128_convert((char *)APP_BLE_CLIENT_FWDL_OTA_SERVICE_ID,
            ble_write_param.char_id.srvc_id.id.uuid.uu.uuid128);
    ble_write_param.char_id.srvc_id.id.uuid.len = 16;
    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.char_id.srvc_id.id.inst_id = 0;

    /*  Client Characteristic Configuration */
    ble_write_param.char_id.char_id.uuid.uu.uuid16 = GATT_UUID_CHAR_CLIENT_CONFIG;
    ble_write_param.char_id.char_id.inst_id = 0;
    ble_write_param.char_id.char_id.uuid.len = 2;
    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;

    ble_write_param.len = 2;

#ifdef APP_BLE_20735_LE2_FWDL
    ble_write_param.value[0] = 0x01;
#else
    ble_write_param.value[0] = 0x02;
#endif

    ble_write_param.value[1] = 0x00;
    ble_write_param.descr = FALSE;
    ble_write_param.char_id.srvc_id.id.inst_id = 0;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWrite failed status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_send_cmd
**
** Description
**
**
**  Returns         None
**
* ***************************************************************************/
static int app_ble_client_otafwdl_send_cmd(UINT32 client_num, UINT32 len, UINT8 *data)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status:%d", status);
        return -1;
    }
    app_ble_client_otafwdl_uuip128_convert((char *)APP_BLE_CLIENT_FWDL_OTA_SERVICE_ID,
            ble_write_param.char_id.srvc_id.id.uuid.uu.uuid128);
    ble_write_param.char_id.srvc_id.id.uuid.len = 16;
    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.char_id.srvc_id.id.inst_id =0;

    app_ble_client_otafwdl_uuip128_convert((char *)APP_BLE_CLIENT_FWDL_OTA_CHAR_NOTIFY_ID,
            ble_write_param.char_id.char_id.uuid.uu.uuid128);
    ble_write_param.char_id.char_id.inst_id = 0;
    ble_write_param.char_id.char_id.uuid.len = 16;
    ble_write_param.conn_id = app_ble_cb.ble_client[client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;

    ble_write_param.len = len;
    memcpy(&ble_write_param.value[0],data,len);
    ble_write_param.descr = FALSE;
    ble_write_param.char_id.srvc_id.id.inst_id= 0;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWrite failed status:%d", status);
        return -1;
    }
    return 0;
}
/*******************************************************************************
**
** Function        app_ble_client_otafwdl_write_data
**
** Description     send fw data to remote
**
**
**  Returns         None
**
 * ***************************************************************************/
static int app_ble_client_otafwdl_write_data(UINT32 client_num,UINT32 len, unsigned char *data)
{
    tBSA_STATUS status;
    tBSA_BLE_CL_WRITE ble_write_param;

    status = BSA_BleClWriteInit(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWriteInit failed status:%d", status);
        return -1;
    }
    app_ble_client_otafwdl_uuip128_convert((char *)APP_BLE_CLIENT_FWDL_OTA_SERVICE_ID,
            ble_write_param.char_id.srvc_id.id.uuid.uu.uuid128);
    ble_write_param.char_id.srvc_id.id.uuid.len = 16;
    ble_write_param.char_id.srvc_id.is_primary = 1;
    ble_write_param.char_id.srvc_id.id.inst_id =0;

    app_ble_client_otafwdl_uuip128_convert((char *)APP_BLE_CLIENT_FWDL_OTA_CHAR_HEX_DATA_ID,
            ble_write_param.char_id.char_id.uuid.uu.uuid128);
    ble_write_param.char_id.char_id.inst_id = 0;
    ble_write_param.char_id.char_id.uuid.len = 16;
    ble_write_param.conn_id = app_ble_cb.ble_client[app_ble_client_otafwdl_cb.client_num].conn_id;
    ble_write_param.auth_req = BTA_GATT_AUTH_REQ_NONE;

    ble_write_param.len = len;
    memcpy(&ble_write_param.value[0],data,len);
    ble_write_param.descr = FALSE;
    ble_write_param.char_id.srvc_id.id.inst_id= 0;
    ble_write_param.write_type = BTA_GATTC_TYPE_WRITE;

    status = BSA_BleClWrite(&ble_write_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClWrite failed status:%d", status);
        return -1;
    }
    return 0;
}
/*******************************************************************************
**
** Function        app_ble_client_otafwdl_event_handler
**
** Description     handle fw upgrade state machine
**                 I use app_ble_profile_cback to return the final confirmation
**
**
**  Returns         None
**
 ***************************************************************************/
void app_ble_client_otafwdl_event_handler(tBSA_BLE_EVT event ,tBSA_BLE_MSG *p_data)
{
    UINT8 response[5];
    UINT32 data_len;
    UINT32 progresscnt;
    int status;
    UINT8 notification_status;
    tAPP_BLE_CLIENT_OTAFWDL_STATE state = app_ble_client_otafwdl_state_get();

    /* If OTA FW DL not ongoing */
    if (state == OTAFU_STATE_IDLE)
    {
        return;
    }

    switch(event)
    {
    case BSA_BLE_CL_CFG_MTU_EVT:
        switch(state)
        {
            case OTAFU_STATE_MTU:
                if (p_data->cli_cfg_mtu.conn_id != app_ble_client_otafwdl_cb.conn_id)
                {
                    return;             /* Ignore it */
                }
                if (p_data->cli_cfg_mtu.status == BSA_SUCCESS)
                {
                    app_ble_client_otafwdl_cb.mtu = MIN(p_data->cli_cfg_mtu.mtu - 3,
                                                        BSA_BLE_CL_WRITE_MAX);
                }
                app_ble_client_otafwdl_cb.ota_need_send_cnt =
                        app_ble_client_otafwdl_cb.ota_binary_len;
                app_ble_client_otafwdl_cb.ota_need_send_cnt /=  app_ble_client_otafwdl_cb.mtu;
                if (app_ble_client_otafwdl_cb.ota_binary_len % app_ble_client_otafwdl_cb.mtu)
                    app_ble_client_otafwdl_cb.ota_need_send_cnt += 1;

                APP_DEBUG1("FW Binary size to download %d byte (%d chunks of %d bytes)",
                        app_ble_client_otafwdl_cb.ota_binary_len,
                        app_ble_client_otafwdl_cb.ota_need_send_cnt,
                        app_ble_client_otafwdl_cb.mtu);

                status = app_ble_client_otafwdl_start();
                if (status < 0)
                {
                    app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                    return;
                }
                app_ble_client_otafwdl_state_set(OTAFU_STATE_PRE_INIT);
                break;

            default:
                /* Ignore event */
                break;
        }
        break;

    case BSA_BLE_CL_WRITE_EVT:
        /* If this is not an event for the BLE OTA FW device */
        if (p_data->cli_write.conn_id != app_ble_client_otafwdl_cb.conn_id)
        {
            return;             /* Ignore it */
        }
        if (p_data->cli_write.status != BSA_SUCCESS)
        {
            APP_ERROR1("BLE Write Failed status:%d", p_data->cli_write.status);
            app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
            return;
        }
        switch(state)
        {
            case OTAFU_STATE_PRE_INIT:
                APP_DEBUG0("Sending FW OTA Prepare Command");
                response[0]= APP_BLE_CLIENT_OTAFWDL_CMD_PREPARE_DOWNLOAD;
                status = app_ble_client_otafwdl_send_cmd(app_ble_client_otafwdl_cb.client_num,
                        1, response);
                if (status < 0)
                {
                    app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                    return;
                }
                app_ble_client_otafwdl_state_set(OTAFU_STATE_WAIT_DOWNLOAD_NOTIFY);
                break;

            case OTAFU_STATE_REC_DOWNLOAD_NOTIFY:
#ifndef APP_BLE_20735_LE2_FWDL
                if(app_ble_client_otafwdl_cb.ota_binary_len < 65535)
                {
                    APP_DEBUG1("Sending FW OTA Length:%d (0x%04x)",
                            app_ble_client_otafwdl_cb.ota_binary_len,
                            app_ble_client_otafwdl_cb.ota_binary_len);
                    response[0] = APP_BLE_CLIENT_OTAFWDL_CMD_DOWNLOAD;
                    response[1] = app_ble_client_otafwdl_cb.ota_binary_len & 0xffff;
                    response[2] = (app_ble_client_otafwdl_cb.ota_binary_len >> 8) & 0xffff;
                    status = app_ble_client_otafwdl_send_cmd(app_ble_client_otafwdl_cb.client_num,
                            3, response);
                    if (status < 0)
                    {
                        app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                        return;
                    }
                }
                else
#endif
                {
                    APP_DEBUG1("Sending FW OTA Length:%d (0x%08x)",
                            app_ble_client_otafwdl_cb.ota_binary_len,
                            app_ble_client_otafwdl_cb.ota_binary_len);
                    response[0] = APP_BLE_CLIENT_OTAFWDL_CMD_DOWNLOAD;
                    response[1] = app_ble_client_otafwdl_cb.ota_binary_len & 0xff;
                    response[2] = (app_ble_client_otafwdl_cb.ota_binary_len >> 8) & 0xff;
                    response[3] = (app_ble_client_otafwdl_cb.ota_binary_len >> 16) & 0xff;
                    response[4] = (app_ble_client_otafwdl_cb.ota_binary_len >> 24) & 0xff;
                    APP_DEBUG1("Sending FW OTA Length:0x%02x, 0x%02x, 0x%02x, 0x%02x,)",
                            response[1], response[2], response[3], response[4]);
                    status = app_ble_client_otafwdl_send_cmd(app_ble_client_otafwdl_cb.client_num,
                            5, response);
                    if (status < 0)
                    {
                        app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                        return;
                    }
                }

                app_ble_client_otafwdl_state_set(OTAFU_STATE_WAIT_DOWNLOAD_LENGTH_NOTIFY);
                break;

            case OTAFU_STATE_REC_DOWNLOAD_LENGTH_NOTIFY:
                app_ble_client_otafwdl_state_set(OTAFU_STATE_WRITE_DATA);
                /* No break on purpose */

            case OTAFU_STATE_WRITE_DATA:
                data_len = app_ble_client_otafwdl_cb.ota_binary_len -
                           app_ble_client_otafwdl_cb.ota_binary_idx;
                if (data_len > app_ble_client_otafwdl_cb.mtu)
                    data_len = app_ble_client_otafwdl_cb.mtu;

                APP_DEBUG1("Send FW Chunk of %d bytes", data_len);
                status = app_ble_client_otafwdl_write_data(app_ble_client_otafwdl_cb.client_num,
                        data_len, &app_ble_client_otafwdl_cb.p_ota_binary[app_ble_client_otafwdl_cb.ota_binary_idx]);
                if (status < 0)
                {
                    app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                    return;
                }

                app_ble_client_otafwdl_cb.ota_binary_idx += data_len;
                app_ble_client_otafwdl_cb.ota_send_cnt++;
                progresscnt = app_ble_client_otafwdl_cb.ota_send_cnt * 100 / \
                        app_ble_client_otafwdl_cb.ota_need_send_cnt;
                APP_INFO1("OTA FW DL Progress[%d%%]", progresscnt);
                if(app_ble_client_otafwdl_cb.ota_binary_idx ==
                        app_ble_client_otafwdl_cb.ota_binary_len)
                {
                    APP_DEBUG0("Last FW Chunk Sent");
                    app_ble_client_otafwdl_state_set(OTAFU_STATE_DATA_VERIFY);
                }
                break;

            case OTAFU_STATE_DATA_VERIFY:
                response[0] = APP_BLE_CLIENT_OTAFWDL_CMD_VERIFY;
                response[1] = app_ble_client_otafwdl_cb.mcrc & 0xffff;
                response[2] = (app_ble_client_otafwdl_cb.mcrc >> 8) & 0xffff;
                response[3] = (app_ble_client_otafwdl_cb.mcrc >> 16) & 0xffff;
                response[4] = (app_ble_client_otafwdl_cb.mcrc >> 24) & 0xffff;
                APP_DEBUG1("Sending FW OTA CRC Verify Command CRC:0x%x",
                        app_ble_client_otafwdl_cb.mcrc);
                status = app_ble_client_otafwdl_send_cmd(app_ble_client_otafwdl_cb.client_num,
                        5, response);
                if (status < 0)
                {
                    app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                    return;
                }

                app_ble_client_otafwdl_state_set(OTAFU_STATE_DATA_VERIFY_CHECK);
                break;

            case OTAFU_STATE_DATA_VERIFY_CHECK:
                /*nothing to do */
                break;

            default:
                APP_DEBUG0("Error when upgrading firmware");
                app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                break;
        }
        break;

    case BSA_BLE_CL_NOTIF_EVT:
        /* If this is not an event for the BLE OTA FW device */
        if (p_data->cli_notif.conn_id != app_ble_client_otafwdl_cb.conn_id)
        {
            return;             /* Ignore it */
        }
        if (p_data->cli_notif.len == 0)
        {
            APP_ERROR0("No notification data");
            app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
            return;
        }
        if (p_data->cli_notif.len > 1)
        {
            APP_ERROR1("Expecting 1 byte notification data, but received %d bytes",
                    p_data->cli_notif.len);
        }
        /* Use the first byte of the notification as 'status' */
        notification_status = p_data->cli_notif.value[0];

        if (notification_status != 0x00)
        {
            APP_ERROR1("Notification Error (0x%x)", notification_status);
            app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
        }

        switch(state)
        {
            case OTAFU_STATE_WAIT_DOWNLOAD_NOTIFY:
                app_ble_client_otafwdl_state_set(OTAFU_STATE_REC_DOWNLOAD_NOTIFY);
                break;

            case OTAFU_STATE_WAIT_DOWNLOAD_LENGTH_NOTIFY:
                app_ble_client_otafwdl_state_set(OTAFU_STATE_REC_DOWNLOAD_LENGTH_NOTIFY);
                break;

            case OTAFU_STATE_DATA_VERIFY_CHECK:
                app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_SUCCESS);
                break;

            default:
                APP_DEBUG0("Bad State");
                app_ble_client_otafwdl_done(APP_BLE_CLIENT_OTAFWDL_STATUS_FAIL);
                break;
        }
        break;

    default:
        /* Ignore other BLE Client events */
        break;
    }
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_crc_calculate
**
** Description     start fw CRC calculate
**
**
**  Returns         None
**
****************************************************************************/
static void app_ble_client_otafwdl_crc_calculate(void)
{
    UINT32 data_len = 0;
    UINT32 binary_idx = 0;

    app_ble_client_otafwdl_cb.mcrc = APP_BLE_CLIENT_OTAFWDL_INITIAL_REMAINDER;

    while(1)
    {
        data_len = app_ble_client_otafwdl_cb.ota_binary_len - binary_idx;
        if (data_len > APP_BLE_CLIENT_OTAFWDL_MTU_DEF)
            data_len = APP_BLE_CLIENT_OTAFWDL_MTU_DEF;
        app_ble_client_otafwdl_cb.mcrc = app_ble_client_otafwdl_crc_slow(
                app_ble_client_otafwdl_cb.mcrc,
                &app_ble_client_otafwdl_cb.p_ota_binary[binary_idx],data_len);
        binary_idx += data_len;
        if (binary_idx == app_ble_client_otafwdl_cb.ota_binary_len)
            break;
    }

    app_ble_client_otafwdl_cb.mcrc =
            app_ble_client_otafwdl_crc_complete(app_ble_client_otafwdl_cb.mcrc);
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_crc_reflect
**
** Description     handle fw upgrade state machine
**
**
**  Returns         None
**
****************************************************************************/
static UINT32 app_ble_client_otafwdl_crc_reflect(UINT32 data, UINT32 nbits)
{
    UINT32 app_ble_client_otafwdl_crc_reflection = 0x00000000;
    UINT32 bit;

    for(bit = 0 ; bit < nbits ; ++bit)
    {
        if((data & 0x01)!=0)
        {
            app_ble_client_otafwdl_crc_reflection |= (1 << ((nbits-1) - bit ));
        }
        data = (data >>1);
    }
    return app_ble_client_otafwdl_crc_reflection;
}
/*******************************************************************************
**
** Function        app_ble_client_otafwdl_crc_slow
**
** Description     calculate crc slow
**
**
**  Returns         None
**
****************************************************************************/
static UINT32 app_ble_client_otafwdl_crc_slow(UINT32 crc32 , UINT8 *data, UINT32 nbytes)
{
    UINT32 b, bit;

    for(b = 0 ; b < nbytes ; b++)
    {
       crc32 ^= ((app_ble_client_otafwdl_crc_reflect_data(data[b])) << (APP_BLE_CLIENT_OTAFWDL_WIDTH - 8));
       for(bit = 8;bit > 0;--bit)
       {
          if((crc32 & APP_BLE_CLIENT_OTAFWDL_TOPBIT) != 0)
          {
              crc32 = (crc32 <<1) ^ APP_BLE_CLIENT_OTAFWDL_POLYNOMIAL;
          }
          else
          {
              crc32 =(crc32<<1);
          }
       }
    }
    return crc32;
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_crc_complect
**
** Description     calculate the final crc value
**
**
**  Returns         None
**
***************************************************************************/
static UINT32 app_ble_client_otafwdl_crc_complete(UINT32 crc32)
{
   return (app_ble_client_otafwdl_crc_reflect_remainder(crc32) ^ APP_BLE_CLIENT_OTAFWDL_FINAL_XOR_VALUE);
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_crc_reflect_data
**
** Description     crc operation
**
**
**  Returns         None
**
****************************************************************************/
static UINT32 app_ble_client_otafwdl_crc_reflect_data(UINT32 data)
{
    return app_ble_client_otafwdl_crc_reflect(data, 8);
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_crc_reflect_remainder
**
** Description
**
**
**  Returns         None
**
***************************************************************************/
static UINT32 app_ble_client_otafwdl_crc_reflect_remainder(UINT32 data)
{
    return app_ble_client_otafwdl_crc_reflect(data, APP_BLE_CLIENT_OTAFWDL_WIDTH);
}

/*******************************************************************************
**
** Function        app_ble_client_otafwdl_done
**
** Description
**
**  Returns         None
**
***************************************************************************/
static void app_ble_client_otafwdl_done(tAPP_BLE_CLIENT_OTAFWDL_STATUS status)
{
    if (status == APP_BLE_CLIENT_OTAFWDL_STATUS_SUCCESS)
        APP_INFO0("OTA FW DL: Success");
    else
        APP_ERROR0("OTA FW DL: Failed");

    if (app_ble_client_otafwdl_cb.p_ota_binary)
    {
        free(app_ble_client_otafwdl_cb.p_ota_binary);
        app_ble_client_otafwdl_cb.p_ota_binary = NULL;
    }

    app_ble_client_otafwdl_init();
}

/*******************************************************************************
**
** Function         app_ble_client_otafwdl_done
**
** Description      Send BLE MTU Request
**
** Returns          status
**
***************************************************************************/
static int app_ble_client_otafwdl_mtu_req(UINT16 mtu)
{
    tBSA_BLE_CL_CFG_MTU cfg_mtu;
    tBSA_STATUS bsa_status;

    APP_INFO1("OTA FW Download: Set MTU to:%d", mtu);

    BSA_BleClCfgMtuInit(&cfg_mtu);
    cfg_mtu.conn_id = app_ble_client_otafwdl_cb.conn_id;
    cfg_mtu.mtu = mtu;
    bsa_status = BSA_BleClCfgMtu(&cfg_mtu);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_BleClCfgMtu mtu:%d failed status:%d", mtu, bsa_status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function         app_ble_client_otafwdl_state_set
**
** Description      Set OTA FW Download State
**
**  Returns         None
**
***************************************************************************/
static void app_ble_client_otafwdl_state_set(tAPP_BLE_CLIENT_OTAFWDL_STATE state)
{
    APP_DEBUG1("OldState:%s NewState:%s",
            app_ble_client_otafwdl_state_get_desc(app_ble_client_otafwdl_cb.state),
            app_ble_client_otafwdl_state_get_desc(state));
    app_ble_client_otafwdl_cb.state = state;
}

/*******************************************************************************
**
** Function         app_ble_client_otafwdl_state_get
**
** Description      Get OTA FW Download State
**
**  Returns         None
**
***************************************************************************/
static tAPP_BLE_CLIENT_OTAFWDL_STATE app_ble_client_otafwdl_state_get(void)
{
    APP_DEBUG1("State:%s", app_ble_client_otafwdl_state_get_desc(app_ble_client_otafwdl_cb.state));
    return app_ble_client_otafwdl_cb.state;
}

/*******************************************************************************
**
** Function         app_ble_client_otafwdl_state_get_desc
**
** Description      Get OTA FW Download State Description
**
**  Returns         String
**
***************************************************************************/
static char *app_ble_client_otafwdl_state_get_desc(tAPP_BLE_CLIENT_OTAFWDL_STATE state)
{
    switch(state)
    {
    case OTAFU_STATE_IDLE: return "IDLE";
    case OTAFU_STATE_MTU: return "MTU";
    case OTAFU_STATE_PRE_INIT: return "PRE_INIT";
    case OTAFU_STATE_WAIT_DOWNLOAD_NOTIFY: return "WAIT_DOWNLOAD_NOTIFY";
    case OTAFU_STATE_REC_DOWNLOAD_NOTIFY: return "REC_DOWNLOAD_NOTIFY";
    case OTAFU_STATE_WAIT_DOWNLOAD_LENGTH_NOTIFY: return "WAIT_DOWNLOAD_LENGTH_NOTIFY";
    case OTAFU_STATE_REC_DOWNLOAD_LENGTH_NOTIFY: return "REC_DOWNLOAD_LENGTH_NOTIFY";
    case OTAFU_STATE_WRITE_DATA: return "WRITE_DATA";
    case OTAFU_STATE_DATA_VERIFY: return "DATA_VERIFY";
    case OTAFU_STATE_DATA_VERIFY_CHECK: return "DATA_VERIFY_CHECK";
    default: return "Unknown";
    }
    return "Error";
}

