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

#include "system_chardev.h"
#include "acamera_logger.h"
#include "acamera_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>


#include "acamera_ctrl_channel.h"
#include "application_command_api.h"
#include "acamera_command_api.h"
#include "acamera_3aalg_preset.h"

static int fd_isp = -1;

int ctrl_channel_init( void )
{

    int32_t result = 0;
    const char *dev_name = CTRL_CHANNEL_DEV_NODE_NAME;

    fd_isp = open( dev_name, O_RDWR | O_SYNC );
    if ( fd_isp != -1 ) {
        LOG( LOG_INFO, "ISP device successfully opened %s", dev_name );
    } else {
        LOG( LOG_ERR, "Failed to open device %s", dev_name );
        result = -1;
    }

    return result;
}

void ctrl_channel_process( void )
{
    static uint8_t *p_buf = NULL;
    static uint32_t buf_allocated = 0;

    struct ctrl_cmd_item cmd;
    int rc = 0;
    uint32_t ret_value;

    if ( fd_isp < 0 ) {
        LOG( LOG_ERR, "ctrl channel is not opened." );
        return;
    }

    rc = read( fd_isp, &cmd, sizeof( cmd ) );
    if ( rc < 0 ) {
        LOG( LOG_ERR, "read failed, rc: %d", rc );
        return;
    }

    if ( 0 == rc ) {
        LOG( LOG_DEBUG, "no data available, return" );
        return;
    }

    if ( rc != sizeof( cmd ) ) {
        LOG( LOG_ERR, "wrong cmd is read out, rc: %d, expected: %zd, reinit it.", rc, sizeof( cmd ) );
        ctrl_channel_deinit();
        ctrl_channel_init();
#if FIRMWARE_CONTEXT_NUMBER == 2
		acamera_command( cmd.cmd_ctx_id, TALGORITHMS, IRIDIX_FRMAEID, 0, COMMAND_SET, &ret_value );
		acamera_command( cmd.cmd_ctx_id, TALGORITHMS, AE_FRMAEID, 0, COMMAND_SET, &ret_value );
		acamera_command( cmd.cmd_ctx_id, TALGORITHMS, GAMMA_FRMAEID, 0, COMMAND_SET, &ret_value );
		acamera_command( cmd.cmd_ctx_id, TALGORITHMS, AWB_FRMAEID, 0, COMMAND_SET, &ret_value );
#else
        acamera_command( 0, TALGORITHMS, IRIDIX_FRMAEID, 0, COMMAND_SET, &ret_value );
        acamera_command( 0, TALGORITHMS, AE_FRMAEID, 0, COMMAND_SET, &ret_value );
        acamera_command( 0, TALGORITHMS, GAMMA_FRMAEID, 0, COMMAND_SET, &ret_value );
        acamera_command( 0, TALGORITHMS, AWB_FRMAEID, 0, COMMAND_SET, &ret_value );
#endif
        return;
    }

    if ( CTRL_CMD_CATEGORY_API_COMMAND == cmd.cmd_category ) {
        LOG( LOG_INFO, "api_command: cmd_type: %u, cmd_id: %u, cmd_direction: %u, cmd_value: %u.", cmd.cmd_type, cmd.cmd_id, cmd.cmd_direction, cmd.cmd_value );
        application_command( cmd.cmd_type, cmd.cmd_id, cmd.cmd_value, cmd.cmd_direction, &ret_value );

    } else if ( CTRL_CMD_CATEGORY_API_CALIBRATION == cmd.cmd_category ) {

        uint32_t new_len = cmd.cmd_value;

        if ( new_len >= CTRL_CHANNEL_MAX_CMD_SIZE ) {
            LOG( LOG_ERR, "Unexpected cmd len, reset." );
            ctrl_channel_deinit();
            ctrl_channel_init();
            return;
        }

        if ( buf_allocated < new_len ) {
            uint8_t *tmp;

            tmp = malloc( new_len );
            if ( !tmp ) {
                LOG( LOG_ERR, "out of memory, reset." );
                ctrl_channel_deinit();
                ctrl_channel_init();
                return;
            }

            /* free the buffer allocated previously */
            if ( p_buf )
                free( p_buf );

            p_buf = tmp;
            buf_allocated = new_len;
        }

        rc = read( fd_isp, p_buf, new_len );
        if ( rc != new_len ) {
            LOG( LOG_ERR, "wrong data is read out, reset." );
            ctrl_channel_deinit();
            ctrl_channel_init();
            return;
        }

        if ( CTRL_CMD_CATEGORY_API_CALIBRATION == cmd.cmd_category ) {
            LOG( LOG_INFO, "api_calibration: cmd_type: %u, cmd_id: %u, cmd_direction: %u, cmd_value: %u.", cmd.cmd_type, cmd.cmd_id, cmd.cmd_direction, cmd.cmd_value );
            application_api_calibration( cmd.cmd_type, cmd.cmd_id, cmd.cmd_direction, (void *)p_buf, cmd.cmd_value, &ret_value );
        }

    } else {
        LOG( LOG_ERR, "Wrong command cmd_category: %u", cmd.cmd_category );
    }
}

void ctrl_channel_deinit( void )
{
    if ( fd_isp >= 0 ) {
        int result = close( fd_isp );
        fd_isp = -1;
        if ( result != 0 ) {
            LOG( LOG_ERR, "Failed to close the file %s ", CTRL_CHANNEL_DEV_NODE_NAME );
        }
    } else {
        LOG( LOG_ERR, "Failed to close %s file. File descriptor is not opened", CTRL_CHANNEL_DEV_NODE_NAME );
    }
    return;
}

/* Below are some empty interface functions for compiler */
void ctrl_channel_handle_command( uint32_t cmd_ctx_id, uint8_t command_type, uint8_t command, uint32_t value, uint8_t direction )
{
}

void ctrl_channel_handle_api_calibration( uint32_t cmd_ctx_id, uint8_t type, uint8_t id, uint8_t direction, void *data, uint32_t data_size )
{
}

int ctrl_channel_3aalg_param_init(isp_ae_preset_t *ae, isp_awb_preset_t *awb, isp_gamma_preset_t *gamma, isp_iridix_preset_t *iridix)
{
    if ( fd_isp < 0 ) {
        LOG( LOG_ERR, "ctrl channel is not opened." );
        return -1;
    }

	if (ae)
	    ioctl(fd_isp, IOCTL_3AALG_AEPRE, ae);

	if(awb)
        ioctl(fd_isp, IOCTL_3AALG_AWBPRE, awb);

	if(gamma)
        ioctl(fd_isp, IOCTL_3AALG_GAMMAPRE, gamma);

	if(iridix)
        ioctl(fd_isp, IOCTL_3AALG_IRIDIXPRE, iridix);

    return 0;
}

