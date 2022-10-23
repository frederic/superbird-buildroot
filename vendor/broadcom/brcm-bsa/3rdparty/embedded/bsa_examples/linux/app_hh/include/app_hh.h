/*****************************************************************************
 **
 **  Name:           app_hh.h
 **
 **  Description:    Bluetooth HID Host application
 **
 **  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef __APP_HH_H__
#define __APP_HH_H__

/* for BSA functions and types */
#include "bsa_api.h"
/* for tAPP_XML_REM_DEVICE */
#include "app_xml_param.h"

#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
#include "app_hh_audio.h"
#endif

#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
#include "app_wav.h"
#endif

#if defined(APP_HH_OTA_FW_DL_INCLUDED) && (APP_HH_OTA_FW_DL_INCLUDED == TRUE)
#include "app_hh_otafwdl.h"
#endif
/*
 * Definitions
 */
#ifndef APP_HH_DEVICE_MAX
#define APP_HH_DEVICE_MAX               10
#endif

#define APP_HH_VOICE_FILE_NAME_FORMAT   "./app_hh_voice_%u.wav"

/* Information mask of the HH devices */
#define APP_HH_DEV_USED     0x01    /* Indicates if this element is allocated */
#define APP_HH_DEV_OPENED   0x02    /* Indicates if this device is connected */

/* APP HH callback functions */
typedef BOOLEAN (tAPP_HH_EVENT_CBACK)(tBSA_HH_EVT event, tBSA_HH_MSG *p_data);
typedef BOOLEAN (tAPP_HH_CUSTOM_UIPC_CBACK)(BT_HDR *p_msg);
typedef void (tAPP_HH_ADD_DEV_CBACK)(tBSA_HH_ADD_DEV *p_params, tAPP_XML_REM_DEVICE *p_device);

#define tAPP_HH_UIPC_CBACK tAPP_HH_CUSTOM_UIPC_CBACK /* For backward compatibility */

#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
typedef struct
{
    tAPP_WAV_FILE_FORMAT format;
    int fd;
    UINT16 codec_setting_bits;
    BOOLEAN is_audio_active;
    BOOLEAN is_audio_device;
    BOOLEAN custom_mode;
    BOOLEAN is_ble_audio_device;
#if defined(APP_HH_AUDIO_STATS_ENABLED) && (APP_HH_AUDIO_STATS_ENABLED == TRUE)
    UINT32 lastseqnum;
    UINT32 firstseqnum;
    UINT32 missseqnum;
#endif
} tAPP_HH_AUDIO;
#endif

typedef struct
{
    UINT8 info_mask;
    BD_ADDR bd_addr;
    tBSA_HH_HANDLE handle;
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    int bthid_fd;
#endif
#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
    tAPP_HH_AUDIO audio;
#endif
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    BOOLEAN le_hid;
#endif
} tAPP_HH_DEVICE;

typedef struct
{
    tUIPC_CH_ID uipc_channel;
    tAPP_HH_DEVICE devices[APP_HH_DEVICE_MAX];
    tBSA_HH_HANDLE mip_handle;
    UINT8 mip_device_nb;
    tBSA_SEC_AUTH sec_mask_in;
    tBSA_SEC_AUTH sec_mask_out;
    tAPP_HH_EVENT_CBACK *p_event_cback;
    tAPP_HH_CUSTOM_UIPC_CBACK *p_uipc_cback;
    tAPP_HH_ADD_DEV_CBACK *p_add_dev_cback;
#if defined(APP_HH_OTA_FW_DL_INCLUDED) && (APP_HH_OTA_FW_DL_INCLUDED == TRUE)
    tAPP_HH_OTAFWDL ota_fw_dl;
#endif
} tAPP_HH_CB;

/*
 * Global variables
 */
extern tAPP_HH_CB app_hh_cb;

/*
 * Interface functions
 */
int app_hh_init(void);
int app_hh_start(void);
#if defined(BSA_HH_USE_DEPRECATED) && (BSA_HH_USE_DEPRECATED == TRUE)
/* DEPRECATED - not to use anymore */
int app_hh_dev_find_by_handle(tBSA_HH_HANDLE handle);
#endif
tAPP_HH_DEVICE *app_hh_pdev_find_by_handle(tBSA_HH_HANDLE handle);
tAPP_HH_DEVICE *app_hh_pdev_find_by_bdaddr(const BD_ADDR bda);
int app_hh_event_cback_register(tAPP_HH_EVENT_CBACK *p_funct);
int app_hh_uipc_cback_register(tAPP_HH_CUSTOM_UIPC_CBACK *p_funct);
int app_hh_add_dev_cback_register(tAPP_HH_ADD_DEV_CBACK *p_funct);
int app_hh_exit(void);
int app_hh_connect(tBSA_HH_PROTO_MODE mode);
int app_hh_open_tbfc(void);
int app_hh_disconnect(void);
int app_hh_get_dscpinfo(UINT8 handle);
int app_hh_set_report(void);
int app_hh_set_ucd_brr_mode(void);
int app_hh_cfg_hid_ucd(UINT8 handle, UINT8 command, UINT8 flags);
int app_hh_send_vc_unplug(void);
int app_hh_send_hid_ctrl(void);
int app_hh_send_ctrl_data(void);
int app_hh_send_int_data(void);
int app_hh_set_3dsg_mode(BOOLEAN enable, int req_handle, int vsync_period);
int app_hh_enable_3dsg_mode_manual(void);
int app_hh_display_status(void);
int app_hh_enable_mip(void);
int app_hh_disable_mip(void);
int app_hh_get_report(void);
int app_hh_get_report_len_fixed(void);
int app_hh_remove_dev(void);
int app_hh_set_idle(void);
int app_hh_get_idle(void);
int app_hh_change_proto_mode(tBSA_HH_HANDLE handle, tBSA_HH_PROTO_MODE mode);
int app_hh_set_proto_mode(void);
int app_hh_get_proto_mode(void);
int app_hh_set_prio_audio(void);
int app_hh_send_set_prio(tBSA_HH_HANDLE handle, BD_ADDR bd_addr, UINT8 priority, UINT8 direction);

#if defined(APP_HH_AUDIO_INCLUDED) && (APP_HH_AUDIO_INCLUDED == TRUE)
tAPP_HH_DEVICE *app_hh_audio_find_active(void);
int app_hh_audio_send(tBSA_HH_HANDLE handle, UINT8 *p_data, UINT8 length);
int app_hh_audio_mic_start(tAPP_HH_DEVICE *p_hh_dev, UINT8 mode);
int app_hh_audio_mic_stop(tAPP_HH_DEVICE *p_hh_dev, UINT8 mode);
#endif

#endif
