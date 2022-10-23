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

#if !defined( __SENSOR_FSM_H__ )
#define __SENSOR_FSM_H__



typedef struct _sensor_fsm_t sensor_fsm_t;
typedef struct _sensor_fsm_t *sensor_fsm_ptr_t;
typedef const struct _sensor_fsm_t *sensor_fsm_const_ptr_t;

#include "sensor_init.h"
#include "system_spi.h"
#include "acamera_sbus_api.h"
#include "acamera_sensor_api.h"
#include "acamera_firmware_config.h"

uint32_t sensor_boot_init( sensor_fsm_ptr_t p_fsm );
void sensor_configure_buffers( sensor_fsm_ptr_t p_fsm );
void sensor_hw_init( sensor_fsm_ptr_t p_fsm );
void sensor_sw_init( sensor_fsm_ptr_t p_fsm );
void sensor_deinit( sensor_fsm_ptr_t p_fsm );
void sensor_update_black( sensor_fsm_ptr_t p_fsm );
uint32_t sensor_get_lines_second( sensor_fsm_ptr_t p_fsm );

struct _sensor_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    sensor_control_t ctrl;
    void *sensor_ctx;
    uint8_t mode;
    uint8_t preset_mode;
    uint16_t isp_output_mode;
    uint16_t black_level;
    uint8_t is_streaming;
    uint8_t info_preset_num;
    uint32_t boot_status;
};


void sensor_fsm_clear( sensor_fsm_ptr_t p_fsm );
void sensor_fsm_init( void *fsm, fsm_init_param_t *init_param );

int sensor_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size );
int sensor_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );

uint8_t sensor_fsm_process_event( sensor_fsm_ptr_t p_fsm, event_id_t event_id );
void sensor_fsm_process_interrupt( sensor_fsm_const_ptr_t p_fsm, uint8_t irq_event );

void sensor_request_interrupt( sensor_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __SENSOR_FSM_H__ */
