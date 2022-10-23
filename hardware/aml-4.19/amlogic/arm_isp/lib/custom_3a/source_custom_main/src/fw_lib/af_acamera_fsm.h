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

#if !defined( __AF_ACAMERA_FSM_H__ )
#define __AF_ACAMERA_FSM_H__


#include "af_standard_api.h"
#include "af_acamera_core.h"
#include "af_acamera_common.h"


typedef struct _af_acamera_fsm_t af_acamera_fsm_t;
typedef struct _af_acamera_fsm_t *af_acamera_fsm_ptr_t;
typedef const struct _af_acamera_fsm_t *af_acamera_fsm_const_ptr_t;


#define ISP_HAS_AF_FSM 1

#include "acamera_isp_config.h"
#include "acamera_lens_api.h"
#include "acamera_isp_core_nomem_settings.h"

#define ISP_HAS_AF_ACAMERA_FSM 1
#define AF_ZONES_COUNT_MAX ( ISP_METERING_ZONES_MAX_V * ISP_METERING_ZONES_MAX_H )


void af_acamera_init( af_acamera_fsm_ptr_t p_fsm );
void af_acamera_deinit( af_acamera_fsm_ptr_t p_fsm );
void af_acamera_process_stats( af_acamera_fsm_ptr_t p_fsm );
void af_acamera_update( af_acamera_fsm_ptr_t p_fsm );

#include "acamera_metering_stats_mem_config.h"

struct _af_acamera_fsm_t {
    fsm_common_t cmn;

    acamera_fsm_mgr_t *p_fsm_mgr;
    fsm_irq_mask_t mask;
    uint32_t zone_raw_statistic[AF_ZONES_COUNT_MAX][2];
    uint8_t zones_horiz;
    uint8_t zones_vert;


    uint32_t pos_min;
    uint32_t pos_max;
    uint8_t mode;
    uint32_t pos_manual;
    uint32_t roi;
    int32_t lens_driver_ok;
    uint32_t roi_api;
    uint32_t last_position;
    int32_t last_sharp_done;

    uint32_t new_lens_pos;
    int32_t new_sharp_val;

    void *lens_ctx;
    lens_control_t lens_ctrl;
    af_status_t af_status;
    lens_param_t lens_param;

    uint8_t refocus_required;

    void *lib_handle;

    af_std_obj_t af_alg_obj;
};


void af_acamera_fsm_clear( af_acamera_fsm_ptr_t p_fsm );

void af_acamera_fsm_init( void *fsm, fsm_init_param_t *init_param );
void af_acamera_fsm_deinit( void *fsm );
int af_acamera_fsm_set_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size );
int af_acamera_fsm_get_param( void *fsm, uint32_t param_id, void *input, uint32_t input_size, void *output, uint32_t output_size );

uint8_t af_acamera_fsm_process_event( af_acamera_fsm_ptr_t p_fsm, event_id_t event_id );
void af_acamera_fsm_process_interrupt( af_acamera_fsm_const_ptr_t p_fsm, uint8_t irq_event );

void af_acamera_request_interrupt( af_acamera_fsm_ptr_t p_fsm, system_fw_interrupt_mask_t mask );

#endif /* __AF_ACAMERA_FSM_H__ */
