/*****************************************************************************
 **
 **  Name:           app_hh_ble_audio_sink.h
 **
 **  Description:    Bluetooth Low Energy Audio Sink
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef _APP_HH_BLE_AUDIO_SINK_H_
#define _APP_HH_BLE_AUDIO_SINK_H_

#include <sys/time.h>
#include <stdlib.h>     /* for exit() */
#include <unistd.h>     /* for write() */

#include "app_hh.h"
#include "bsa_trace.h"
#include "app_utils.h"

#include "data_types.h"

#include "sbc_decoder.h"

/*******************************************************************************
 **
 ** Function         app_hh_ble_audio_sink_init
 **
 ** Description      Initialize HID BLE Audio Sink
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_hh_ble_audio_sink_init(void);

/*******************************************************************************
 **
 ** Function         app_hh_ble_audio_sink_handler
 **
 ** Description      BLE Audio Sink Handler
 **
 ** Returns          status
 **
 *******************************************************************************/
void app_hh_ble_audio_sink_handler(tAPP_HH_DEVICE *p_hh_dev, tBSA_HH_REPORT_DATA *p_report_data);

/*******************************************************************************
 **
 ** Function         app_hh_ble_msbc_decode
 **
 ** Description      Decode a BLE Report Id containing Audio Data
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_hh_ble_msbc_decode(tAPP_HH_DEVICE *p_hh_dev, UINT8 *p_data, UINT16 len);

/*******************************************************************************
 **
 ** Function         app_hh_ble_audio_sink_seq_init
 **
 ** Description      This function initializes Audio sink sequence number and
 **                  some error counters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_hh_ble_audio_sink_seq_init(void);

#endif /* _APP_HH_BLE_AUDIO_SINK_H_ */
