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

#if !defined(__ACAMERA_FSM_MGR_H__)
#define __ACAMERA_FSM_MGR_H__



#include "acamera_firmware_config.h"
#include "acamera_types.h"

#if !defined(__ACAMERA_FW_H__)
#error "acamera_fsm_mgr.h file should not be included from source file, include acamera_fw.h instead!"
#endif

typedef enum _event_id_t
{
    event_id_WB_matrix_ready,
    event_id_acamera_reset_sensor_hw,
    event_id_ae_stats_ready,
    event_id_af_converged,
    event_id_af_fast_search_finished,
    event_id_af_refocus,
    event_id_af_stats_ready,
    event_id_af_step,
    event_id_antiflicker_changed,
    event_id_awb_stats_ready,
    event_id_cmos_refresh,
    event_id_exposure_changed,
    event_id_frame_end,
    event_id_gamma_contrast_stats_ready,
    event_id_gamma_stats_ready,
    event_id_monitor_frame_end,
    event_id_monitor_notify_other_fsm,
    event_id_new_frame,
    event_id_sensor_not_ready,
    event_id_sensor_ready,
    event_id_sensor_sw_reset,
    event_id_sharp_lut_update,
    event_id_update_iridix,
    event_id_update_sharp_lut,
    event_id_user_data_ready,
    number_of_event_ids
} event_id_t;


typedef enum _fsm_id_t
{
    FSM_ID_USER2KERNEL,
    FSM_ID_SENSOR,
    FSM_ID_CMOS,
    FSM_ID_GENERAL,
    FSM_ID_AE,
    FSM_ID_AWB,
    FSM_ID_COLOR_MATRIX,
    FSM_ID_IRIDIX,
    FSM_ID_MATRIX_YUV,
    FSM_ID_GAMMA_CONTRAST,
    FSM_ID_AF,
    FSM_ID_MONITOR,
    FSM_ID_SHARPENING,
    FSM_ID_MAX
} fsm_id_t;

#include "fsm_intf.h"



#include "acamera_event_queue.h"

struct _acamera_fsm_mgr_t
{
    uint8_t ctx_id;
    uintptr_t isp_base;
    acamera_context_ptr_t p_ctx;
    isp_info_t info;
    fsm_common_t *fsm_arr[FSM_ID_MAX];
    acamera_event_queue_t event_queue;
    uint8_t event_queue_data[ACAMERA_EVENT_QUEUE_SIZE];
    uint32_t reserved;
};

void acamera_fsm_mgr_raise_event(acamera_fsm_mgr_t *p_fsm_mgr, event_id_t event_id);

#define fsm_raise_event(p_fsm,event_id) \
    acamera_fsm_mgr_raise_event((p_fsm)->p_fsm_mgr,event_id)

void acamera_fsm_mgr_init(acamera_fsm_mgr_t *p_fsm_mgr);
void acamera_fsm_mgr_deinit(acamera_fsm_mgr_t *p_fsm_mgr);
void acamera_fsm_mgr_process_interrupt(acamera_fsm_mgr_t *p_fsm_mgr, uint8_t event);
void acamera_fsm_mgr_process_events(acamera_fsm_mgr_t *p_fsm_mgr, int n_max_events);

#endif
