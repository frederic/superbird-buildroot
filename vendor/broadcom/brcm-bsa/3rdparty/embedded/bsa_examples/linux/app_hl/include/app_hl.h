/*****************************************************************************
 **
 **  Name:           app_hh.h
 **
 **  Description:    Bluetooth HID Host application
 **
 **  Copyright (c) 2011-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_HL_H
#define APP_HL_H

/* Defined Data Type (from IEEE 11073 Personal Health Devices Working Group) */
#define APP_HL_DATA_TYPE_PULSE_OXIMETER         0x1004
#define APP_HL_DATA_TYPE_BLOOD_PRESSURE_MON     0x1007
#define APP_HL_DATA_TYPE_BODY_THERMOMETER       0x1008
#define APP_HL_DATA_TYPE_BODY_WEIGHT_SCALE      0x100F
#define APP_HL_DATA_TYPE_GLUCOSE_METER          0x1011
#define APP_HL_DATA_TYPE_PEDOMETER              0x1029
#define APP_HL_DATA_TYPE_STEP_COUNTER           0x1068

/* Maximum number of Application/Service */
#define APP_HL_LOCAL_APP_MAX        3

/* Maximum number of MDL saved per application */
#define APP_HL_MDL_INDEX_MAX        16


int app_hl_init(BOOLEAN boot);
int app_hl_exit(void);

int app_hl_register_sink1(void);
int app_hl_register_source1(void);
int app_hl_register_dual1(void);
int app_hl_sdp_query(void);
int app_hl_open(void);
int app_hl_reconnect(void);
int app_hl_delete(void);
int app_hl_close(void);
int app_hl_send_file(char * p_file_name);
int app_hl_send_data(void);

void app_hl_con_display(void);
char *app_hl_get_srv_desc(tBSA_SERVICE_ID service);

void app_hl_con_display(void);
char *app_hl_get_channel_cfg_desc(tBSA_HL_DCH_CFG channel_cfg);
char *app_hl_get_channel_mode_desc(tBSA_HL_DCH_MODE channel_mode);
char *app_hl_get_role_desc(tBTA_HL_MDEP_ROLE role);


#endif
