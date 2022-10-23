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

#include "acamera_fw.h"
#if defined(ISP_HAS_DMA_WRITER_FSM)
#include "dma_writer_fsm.h"
#endif


int acamera_fsm_mgr_set_param(acamera_fsm_mgr_t * p_fsm_mgr, uint32_t param_id, void * input, uint32_t input_size)
{
    int rc = 0;

    if( param_id >= FSM_PARAM_SET_MAX_ID || param_id <= FSM_PARAM_SET_MIN_ID ) {
        LOG(LOG_CRIT, "Invalid param: param_id: %d, min: %d, max: %d.", param_id, FSM_PARAM_SET_MIN_ID, FSM_PARAM_SET_MAX_ID);
        return -1;
    }

    if( FSM_PARAM_SET_SENSOR_START < param_id && param_id < FSM_PARAM_SET_SENSOR_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_SENSOR]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_SENSOR]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_SENSOR]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "SENSOR FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_CMOS_START < param_id && param_id < FSM_PARAM_SET_CMOS_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_CMOS]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_CMOS]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_CMOS]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "CMOS FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_GENERAL_START < param_id && param_id < FSM_PARAM_SET_GENERAL_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_GENERAL]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_GENERAL]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_GENERAL]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "GENERAL FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_AE_START < param_id && param_id < FSM_PARAM_SET_AE_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_AE]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_AE]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_AE]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "AE FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_AWB_START < param_id && param_id < FSM_PARAM_SET_AWB_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_AWB]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_AWB]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_AWB]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "AWB FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_COLOR_MATRIX_START < param_id && param_id < FSM_PARAM_SET_COLOR_MATRIX_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_COLOR_MATRIX]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_COLOR_MATRIX]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_COLOR_MATRIX]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "COLOR_MATRIX FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_IRIDIX_START < param_id && param_id < FSM_PARAM_SET_IRIDIX_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_IRIDIX]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_IRIDIX]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_IRIDIX]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "IRIDIX FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_MATRIX_YUV_START < param_id && param_id < FSM_PARAM_SET_MATRIX_YUV_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_MATRIX_YUV]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_MATRIX_YUV]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_MATRIX_YUV]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "MATRIX_YUV FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_GAMMA_CONTRAST_START < param_id && param_id < FSM_PARAM_SET_GAMMA_CONTRAST_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_GAMMA_CONTRAST]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_GAMMA_CONTRAST]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_GAMMA_CONTRAST]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "GAMMA_CONTRAST FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_AF_START < param_id && param_id < FSM_PARAM_SET_AF_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_AF]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_AF]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_AF]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "AF FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_MONITOR_START < param_id && param_id < FSM_PARAM_SET_MONITOR_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_MONITOR]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_MONITOR]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_MONITOR]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "MONITOR FSM doesn't support set_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_SET_SHARPENING_START < param_id && param_id < FSM_PARAM_SET_SHARPENING_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_SHARPENING]->ops.set_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_SHARPENING]->ops.set_param( p_fsm_mgr->fsm_arr[FSM_ID_SHARPENING]->p_fsm, param_id, input, input_size);
        } else {
            LOG(LOG_ERR, "SHARPENING FSM doesn't support set_param().");
            rc = -1;
        }
    } else {
        LOG(LOG_CRIT, "Unsupported param_id: %d.", param_id);
        rc = -1;
    }

    return rc;
}

int acamera_fsm_mgr_get_param(acamera_fsm_mgr_t * p_fsm_mgr, uint32_t param_id, void * input, uint32_t input_size, void * output, uint32_t output_size)
{
    int rc = 0;

    if( param_id >= FSM_PARAM_GET_MAX_ID || param_id <= FSM_PARAM_GET_MIN_ID ) {
        LOG(LOG_CRIT, "Invalid param: param_id: %d, min: %d, max: %d.", param_id, FSM_PARAM_GET_MIN_ID, FSM_PARAM_GET_MAX_ID);
        return -1;
    }

    if( FSM_PARAM_GET_USER2KERNEL_START < param_id && param_id < FSM_PARAM_GET_USER2KERNEL_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_USER2KERNEL]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_USER2KERNEL]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_USER2KERNEL]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "USER2KERNEL FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_SENSOR_START < param_id && param_id < FSM_PARAM_GET_SENSOR_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_SENSOR]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_SENSOR]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_SENSOR]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "SENSOR FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_CMOS_START < param_id && param_id < FSM_PARAM_GET_CMOS_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_CMOS]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_CMOS]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_CMOS]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "CMOS FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_GENERAL_START < param_id && param_id < FSM_PARAM_GET_GENERAL_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_GENERAL]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_GENERAL]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_GENERAL]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "GENERAL FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_AE_START < param_id && param_id < FSM_PARAM_GET_AE_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_AE]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_AE]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_AE]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "AE FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_AWB_START < param_id && param_id < FSM_PARAM_GET_AWB_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_AWB]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_AWB]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_AWB]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "AWB FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_COLOR_MATRIX_START < param_id && param_id < FSM_PARAM_GET_COLOR_MATRIX_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_COLOR_MATRIX]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_COLOR_MATRIX]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_COLOR_MATRIX]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "COLOR_MATRIX FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_IRIDIX_START < param_id && param_id < FSM_PARAM_GET_IRIDIX_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_IRIDIX]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_IRIDIX]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_IRIDIX]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "IRIDIX FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_MATRIX_YUV_START < param_id && param_id < FSM_PARAM_GET_MATRIX_YUV_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_MATRIX_YUV]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_MATRIX_YUV]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_MATRIX_YUV]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "MATRIX_YUV FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_GAMMA_CONTRAST_START < param_id && param_id < FSM_PARAM_GET_GAMMA_CONTRAST_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_GAMMA_CONTRAST]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_GAMMA_CONTRAST]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_GAMMA_CONTRAST]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "GAMMA_CONTRAST FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_AF_START < param_id && param_id < FSM_PARAM_GET_AF_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_AF]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_AF]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_AF]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "AF FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_MONITOR_START < param_id && param_id < FSM_PARAM_GET_MONITOR_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_MONITOR]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_MONITOR]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_MONITOR]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "MONITOR FSM doesn't support get_param().");
            rc = -1;
        }
    } else if( FSM_PARAM_GET_SHARPENING_START < param_id && param_id < FSM_PARAM_GET_SHARPENING_END ) {
        if( p_fsm_mgr->fsm_arr[FSM_ID_SHARPENING]->ops.get_param ) {
            rc = p_fsm_mgr->fsm_arr[FSM_ID_SHARPENING]->ops.get_param( p_fsm_mgr->fsm_arr[FSM_ID_SHARPENING]->p_fsm, param_id, input, input_size, output, output_size);
        } else {
            LOG(LOG_ERR, "SHARPENING FSM doesn't support get_param().");
            rc = -1;
        }
    } else {
        LOG(LOG_CRIT, "Unsupported param_id: %d.", param_id);
        rc = -1;
    }

    return rc;
}

void acamera_fsm_mgr_dma_writer_update_address_interrupt( acamera_fsm_mgr_t * p_fsm_mgr, uint8_t irq_event )
{
#if defined(ISP_HAS_DMA_WRITER_FSM)
    dma_writer_update_address_interrupt( p_fsm_mgr->fsm_arr[FSM_ID_DMA_WRITER]->p_fsm, irq_event );
#endif
}
