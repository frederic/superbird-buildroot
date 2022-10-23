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

#ifndef ACAMERA_CTRL_CHANNEL_H
#define ACAMERA_CTRL_CHANNEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acamera_types.h"

#define CTRL_CHANNEL_DEV_NAME "ac_isp4uf"
#define CTRL_CHANNEL_DEV_NODE_NAME "/dev/" CTRL_CHANNEL_DEV_NAME
#define CTRL_CHANNEL_MAX_CMD_SIZE ( 8 * 1024 )


enum ctrl_cmd_category {
    CTRL_CMD_CATEGORY_API_COMMAND = 1,
    CTRL_CMD_CATEGORY_API_CALIBRATION,
};

struct ctrl_cmd_item {
    /* command metadata */
    uint32_t cmd_len;
    uint8_t cmd_category;

    /* command content */
#if FIRMWARE_CONTEXT_NUMBER == 2
    uint32_t cmd_ctx_id;
#endif
    uint8_t cmd_type;
    uint8_t cmd_id;
    uint8_t cmd_direction;
    uint32_t cmd_value;
};

int ctrl_channel_init( void );
void ctrl_channel_process( void );
void ctrl_channel_deinit( void );

void ctrl_channel_handle_command( uint32_t cmd_ctx_id, uint8_t command_type, uint8_t command, uint32_t value, uint8_t direction );
void ctrl_channel_handle_api_calibration( uint32_t cmd_ctx_id, uint8_t type, uint8_t id, uint8_t direction, void *data, uint32_t data_size );

#ifdef __cplusplus
}
#endif

#endif /* ACAMERA_CTRL_CHANNEL_H */
