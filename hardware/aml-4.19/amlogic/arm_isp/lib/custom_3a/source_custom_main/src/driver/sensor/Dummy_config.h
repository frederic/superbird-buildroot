//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#ifndef __DUMMY_CONFIG_H__
#define __DUMMY_CONFIG_H__


#define FULL_EXTRA_HEIGHT 0
#define FULL_EXTRA_WIDTH 0
#define ISP_IMAGE_HEIGHT SENSOR_IMAGE_HEIGHT
#define ISP_IMAGE_WIDTH SENSOR_IMAGE_WIDTH
#define LOG2_SENSOR_AGAIN_MAXIMUM 0
#define LOG2_SENSOR_DGAIN_MAXIMUM 0
#define PREVIEW_EXTRA_HEIGHT 0
#define PREVIEW_EXTRA_WIDTH 0
#define RESOLUTION_CHANGE_ENABLED 0
#define SENSOR_AF_MOVE_DELAY 20
#define SENSOR_ANALOG_GAIN_APPLY_DELAY 1
#define SENSOR_BLACK_LEVEL_CORRECTION 0
#define SENSOR_BOARD_MASTER_CLOCK 24000
#define SENSOR_BUS i2c
#define SENSOR_DAY_LIGHT_INTEGRATION_TIME_LIMIT 300
#define SENSOR_DEV_ADDRESS 54
#define SENSOR_DIGITAL_GAIN_APPLY_DELAY 1
#define SENSOR_EXP_NUMBER 1
#define SENSOR_IMAGE_HEIGHT 1080
#define SENSOR_IMAGE_HEIGHT_FULL 2448
#define SENSOR_IMAGE_HEIGHT_PREVIEW 1080
#define SENSOR_IMAGE_WIDTH 1920
#define SENSOR_IMAGE_WIDTH_FULL 3264
#define SENSOR_IMAGE_WIDTH_PREVIEW 1920
#define SENSOR_INTEGRATION_TIME_APPLY_DELAY 1
#define SENSOR_MAX_INTEGRATION_TIME 200
#define SENSOR_MAX_INTEGRATION_TIME_LIMIT SENSOR_MAX_INTEGRATION_TIME
#define SENSOR_MAX_INTEGRATION_TIME_NATIVE SENSOR_MAX_INTEGRATION_TIME
#define SENSOR_MAX_INTEGRATION_TIME_PREVIEW 2586 - 14
#define SENSOR_MIN_INTEGRATION_TIME 1
#define SENSOR_MIN_INTEGRATION_TIME_NATIVE SENSOR_MIN_INTEGRATION_TIME
#define SENSOR_OUTPUT_BITS 10
#define SENSOR_SEQUENCE_FULL_RES_HALF_FPS 2
#define SENSOR_SEQUENCE_FULL_RES_MAX_FPS 0
#define SENSOR_SEQUENCE_NAME default
#define SENSOR_SEQUENCE_PREVIEW_RES_HALF_FPS 3
#define SENSOR_SEQUENCE_PREVIEW_RES_MAX_FPS 1
#define SENSOR_TOTAL_HEIGHT 1110
#define SENSOR_TOTAL_HEIGHT_PREVIEW 2586
#define SENSOR_TOTAL_WIDTH 2000
#define SENSOR_TOTAL_WIDTH_PREVIEW 3264
#define SPI_CLOCK_DIV 40
#define SPI_CONTROL_MASK ( RX_NEG_MSK | ( CHAR_LEN_MSK & 32 ) | AUTO_SS_MSK | LSB_MSK )
#endif
