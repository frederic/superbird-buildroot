/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2011-2018 ARM or its affiliates
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2.
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <asm/div64.h>
#include <linux/sched.h>
#include <linux/videodev2.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-vmalloc.h>

#include "system_interrupts.h"
#include "acamera_command_api.h"
#include "acamera_firmware_settings.h"
#include "application_command_api.h"
#include "acamera_logger.h"
#include "isp-v4l2-common.h"
#include "fw-interface.h"
#include "runtime_initialization_settings.h"
#include <linux/delay.h>

//use main_firmware.c routines to initialize fw
extern int isp_fw_init( uint32_t hw_isp_addr );
extern void isp_fw_exit( void );

static int isp_started = 0;
static int custom_wdr_mode = 0;
static int custom_exp = 0;
static int custom_fps = 0;
/* ----------------------------------------------------------------
 * fw_interface control interface
 */

void custom_initialization( uint32_t ctx_num )
{

}


/* fw-interface isp control interface */
int fw_intf_isp_init(  uint32_t hw_isp_addr )
{

    LOG( LOG_INFO, "Initializing fw ..." );
    int rc = 0;
    /* flag variable update */

    rc = isp_fw_init(hw_isp_addr);

    if ( rc == 0 )
        isp_started = 1;

    custom_wdr_mode = 0;
    custom_exp = 0;
    custom_fps = 0;

    return rc;
}

int fw_intf_is_isp_started( void )
{
    return isp_started;
}

void fw_intf_isp_deinit( void )
{
    LOG( LOG_INFO, "Deinitializing fw interface ..." );

    /* flag variable update */
    isp_started = 0;

    isp_fw_exit();
}

uint32_t fw_calibration_update( uint32_t ctx_id )
{
    uint32_t retval;
    acamera_command( ctx_id, TSYSTEM, CALIBRATION_UPDATE, UPDATE, COMMAND_SET, &retval );
    return retval;
}

int fw_intf_isp_start( void )
{
    LOG( LOG_DEBUG, "Starting context" );


    return 0;
}

void fw_intf_isp_stop( void )
{
    LOG( LOG_DEBUG, "Enter" );
}

int fw_intf_isp_set_current_ctx_id( uint32_t ctx_id )
{
    int active_ctx_id = ctx_id;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

#if defined( TGENERAL ) && defined( ACTIVE_CONTEXT )
    acamera_command( ctx_id, TGENERAL, ACTIVE_CONTEXT, ctx_id, COMMAND_SET, &active_ctx_id );
#endif

    return active_ctx_id;
}

int fw_intf_isp_get_current_ctx_id( uint32_t ctx_id )
{
    int active_ctx_id = -1;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

#if defined( TGENERAL ) && defined( ACTIVE_CONTEXT )
    acamera_command( ctx_id, TGENERAL, ACTIVE_CONTEXT, 0, COMMAND_GET, &active_ctx_id );
#endif

    return active_ctx_id;
}

int fw_intf_isp_get_sensor_info( uint32_t ctx_id, isp_v4l2_sensor_info *sensor_info )
{
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

#if defined( TSENSOR ) && defined( SENSOR_SUPPORTED_PRESETS ) && defined( SENSOR_INFO_PRESET ) && \
    defined( SENSOR_INFO_FPS ) && defined( SENSOR_INFO_WIDTH ) && defined( SENSOR_INFO_HEIGHT )
    LOG( LOG_INFO, "API found, initializing sensor_info from API." );
    int i, j;
    uint32_t ret_val;
    uint32_t preset_num;

    /* reset buffer */
    memset( sensor_info, 0x0, sizeof( isp_v4l2_sensor_info ) );

    /* get preset size */
    acamera_command( ctx_id, TSENSOR, SENSOR_SUPPORTED_PRESETS, 0, COMMAND_GET, &preset_num );
    if ( preset_num > MAX_SENSOR_PRESET_SIZE ) {
        LOG( LOG_ERR, "MAX_SENSOR_PRESET_SIZE is too small ! (preset_num = %d)", preset_num );
        preset_num = MAX_SENSOR_PRESET_SIZE;
    }

    /* fill preset values */
    for ( i = 0; i < preset_num; i++ ) {
        uint32_t width, height, fps, wdrmode, exposures = 1;

        /* get next preset */
        acamera_command( ctx_id, TSENSOR, SENSOR_INFO_PRESET, i, COMMAND_SET, &ret_val );
        acamera_command( ctx_id, TSENSOR, SENSOR_INFO_FPS, i, COMMAND_GET, &fps );
        acamera_command( ctx_id, TSENSOR, SENSOR_INFO_WIDTH, i, COMMAND_GET, &width );
        acamera_command( ctx_id, TSENSOR, SENSOR_INFO_HEIGHT, i, COMMAND_GET, &height );
#if defined( SENSOR_INFO_EXPOSURES )
        acamera_command( ctx_id, TSENSOR, SENSOR_INFO_EXPOSURES, i, COMMAND_GET, &exposures );
#endif
        acamera_command( ctx_id, TSENSOR, SENSOR_INFO_WDR_MODE, i, COMMAND_GET, &wdrmode );
        /* find proper index from sensor_info */
        for ( j = 0; j < sensor_info->preset_num; j++ ) {
            if ( sensor_info->preset[j].width == width &&
                 sensor_info->preset[j].height == height )
                break;
        }

        /* store preset */
        if ( sensor_info->preset[j].fps_num < MAX_SENSOR_FPS_SIZE ) {
            sensor_info->preset[j].width = width;
            sensor_info->preset[j].height = height;
            sensor_info->preset[j].exposures[sensor_info->preset[j].fps_num] = exposures;
            sensor_info->preset[j].fps[sensor_info->preset[j].fps_num] = fps;
            sensor_info->preset[j].idx[sensor_info->preset[j].fps_num] = i;
            sensor_info->preset[j].wdr_mode[sensor_info->preset[j].fps_num] = wdrmode;
            sensor_info->preset[j].fps_num++;
            if ( sensor_info->preset_num <= j )
                sensor_info->preset_num++;
        } else {
            LOG( LOG_ERR, "FPS number overflowed ! (preset#%d / index#%d)", i, j );
        }
    }
    uint32_t spreset, swidth, sheight;
    acamera_command( ctx_id, TSENSOR, SENSOR_PRESET, 0, COMMAND_GET, &spreset );
    acamera_command( ctx_id, TSENSOR, SENSOR_WIDTH, 0, COMMAND_GET, &swidth );
    acamera_command( ctx_id, TSENSOR, SENSOR_HEIGHT, 0, COMMAND_GET, &sheight );

    for ( i = 0; i < sensor_info->preset_num; i++ ) {
        if ( swidth == sensor_info->preset[i].width && sheight == sensor_info->preset[i].height ) {
            sensor_info->preset_cur = i;
            break;
        }
    }

    if ( i < MAX_SENSOR_PRESET_SIZE ) {
        for ( j = 0; j < sensor_info->preset[i].fps_num; j++ ) {
            if ( sensor_info->preset[i].idx[j] == spreset ) {
                sensor_info->preset[i].fps_cur = spreset;
                break;
            }
        }
    }

    LOG( LOG_INFO, "Sensor info current preset:%d v4l2 preset:%d", sensor_info->preset[i].fps_cur, sensor_info->preset_cur );

    //check current sensor settings

    LOG( LOG_INFO, "/* dump sensor info -----------------" );
    for ( i = 0; i < sensor_info->preset_num; i++ ) {
        LOG( LOG_INFO, "   Idx#%02d - W:%04d H:%04d",
             i, sensor_info->preset[i].width, sensor_info->preset[i].height );
        for ( j = 0; j < sensor_info->preset[i].fps_num; j++ )
            LOG( LOG_INFO, "            FPS#%d: %d (preset index = %d) exposures:%d  wdr_mode:%d",
                 j, sensor_info->preset[i].fps[j] / 256, sensor_info->preset[i].idx[j],
                 sensor_info->preset[i].exposures[j], sensor_info->preset[i].wdr_mode[j] );
    }

    LOG( LOG_INFO, "-----------------------------------*/" );

#else
    /* returning default setting (1080p) */
    LOG( LOG_ERR, "API not found, initializing sensor_info to default." );
    sensor_info->preset_num = 1;
    sensor_info->preset[0].fps = 30 * 256;
    sensor_info->preset[0].width = 1920;
    sensor_info->preset[0].height = 1080;
#endif

    return 0;
}

int fw_intf_isp_get_sensor_preset( uint32_t ctx_id )
{
    int value = -1;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

#if defined( TSENSOR ) && defined( SENSOR_PRESET )
    acamera_command( ctx_id, TSENSOR, SENSOR_PRESET, 0, COMMAND_GET, &value );
#endif

    return value;
}

int fw_intf_isp_set_sensor_preset( uint32_t ctx_id, uint32_t preset )
{
    int value = -1;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

#if defined( TSENSOR ) && defined( SENSOR_PRESET )
    LOG( LOG_INFO, "V4L2 setting sensor preset to %d\n", preset );
    acamera_command( ctx_id, TSENSOR, SENSOR_PRESET, preset, COMMAND_SET, &value );
#endif

    return value;
}

/*
 * fw-interface per-stream control interface
 */
int fw_intf_stream_start( uint32_t ctx_id, isp_v4l2_stream_type_t streamType )
{
    LOG( LOG_DEBUG, "Starting stream type %d", streamType );
    uint32_t rc = 0;

#if ISP_DMA_RAW_CAPTURE && ISP_HAS_RAW_CB
    if ( streamType == V4L2_STREAM_TYPE_RAW ) {
#if defined( TFPGA ) && defined( DMA_RAW_CAPTURE_ENABLED_ID )
        uint32_t ret_val;
        acamera_command( ctx_id, TFPGA, DMA_RAW_CAPTURE_ENABLED_ID, ON, COMMAND_SET, &ret_val );
#else
        LOG( LOG_CRIT, "no api for DMA_RAW_CAPTURE_ENABLED_ID" );
#endif
    }
#endif
    if (streamType == V4L2_STREAM_TYPE_FR || streamType == V4L2_STREAM_TYPE_DS1) {
        LOG( LOG_ERR, "Starting stream type %d", streamType );
        acamera_command( ctx_id, TSENSOR, SENSOR_STREAMING, ON, COMMAND_SET, &rc );
    }

#if ISP_HAS_DS2
    if (streamType == V4L2_STREAM_TYPE_DS2) {
        uint32_t ret_val;
        LOG( LOG_ERR, "Starting stream type %d", streamType );
        acamera_command( ctx_id, TAML_SCALER, SCALER_STREAMING_ON, ON, COMMAND_SET, &ret_val );
    }
#endif

    return 0;
}

void fw_intf_stream_stop( uint32_t ctx_id, isp_v4l2_stream_type_t streamType, int stream_on_count )
{
    uint32_t rc = 0;
    LOG( LOG_DEBUG, "Stopping stream type %d", streamType );
#if ISP_DMA_RAW_CAPTURE && ISP_HAS_RAW_CB
    if ( streamType == V4L2_STREAM_TYPE_RAW ) {
#if defined( TFPGA ) && defined( DMA_RAW_CAPTURE_ENABLED_ID )
        uint32_t ret_val;
        acamera_command( ctx_id, TFPGA, DMA_RAW_CAPTURE_ENABLED_ID, OFF, COMMAND_SET, &ret_val );
#else
        LOG( LOG_CRIT, "no api for DMA_RAW_CAPTURE_ENABLED_ID" );
#endif
    }
#endif

    if (streamType == V4L2_STREAM_TYPE_FR) {
		if(stream_on_count == 1)
            acamera_command( ctx_id, TSENSOR, SENSOR_STREAMING, OFF, COMMAND_SET, &rc );
        acamera_api_dma_buff_queue_reset(ctx_id, dma_fr);
    } else if (streamType == V4L2_STREAM_TYPE_DS1) {
		if(stream_on_count == 1)
		    acamera_command( ctx_id, TSENSOR, SENSOR_STREAMING, OFF, COMMAND_SET, &rc );
        acamera_api_dma_buff_queue_reset(ctx_id, dma_ds1);
    }

#if ISP_HAS_DS2
    if (streamType == V4L2_STREAM_TYPE_DS2) {
        uint32_t ret_val;
        LOG( LOG_ERR, "Stopping stream type %d", streamType );
		if(stream_on_count == 1)
			acamera_command( ctx_id, TSENSOR, SENSOR_STREAMING, OFF, COMMAND_SET, &rc );
        acamera_command( ctx_id, TAML_SCALER, SCALER_STREAMING_OFF, OFF, COMMAND_SET, &ret_val );
    }
#endif

    LOG( LOG_CRIT, "Stream off %d, user: %d\n",  streamType, stream_on_count);
}

void fw_intf_stream_pause( uint32_t ctx_id, isp_v4l2_stream_type_t streamType, uint8_t bPause )
{
    LOG( LOG_DEBUG, "Pausing/Resuming stream type %d (Flag=%d", streamType, bPause );

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return;
    }

#if ISP_DMA_RAW_CAPTURE && ISP_HAS_RAW_CB
    if ( streamType == V4L2_STREAM_TYPE_RAW ) {
#if defined( TFPGA ) && defined( DMA_RAW_CAPTURE_ENABLED_ID )
        uint32_t ret_val;
        acamera_command( ctx_id, TFPGA, DMA_RAW_CAPTURE_WRITEON_ID, ( bPause ? 0 : 1 ), COMMAND_SET, &ret_val );
#else
        LOG( LOG_CRIT, "no api for DMA_RAW_CAPTURE_WRITEON_ID" );
#endif
    }
#endif

#if defined( TGENERAL ) && defined( DMA_WRITER_SINGLE_FRAME_MODE )
#if ISP_HAS_RAW_CB
    if ( streamType == V4L2_STREAM_TYPE_RAW ) {
        uint32_t ret_val;

        acamera_command( ctx_id, TGENERAL, DMA_WRITER_SINGLE_FRAME_MODE, bPause ? ON : OFF, COMMAND_SET, &ret_val );
    }
#endif
#endif
}

/* fw-interface sensor hw stream control interface */
int fw_intf_sensor_pause( uint32_t ctx_id )
{
    uint32_t rc = 0;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    application_command( TSENSOR, SENSOR_STREAMING, OFF, COMMAND_SET, &rc );

    return rc;
}

int fw_intf_sensor_resume( uint32_t ctx_id )
{
    uint32_t rc = 0;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    application_command( TSENSOR, SENSOR_STREAMING, ON, COMMAND_SET, &rc );

    return rc;
}

uint32_t fw_intf_find_proper_present_idx(const isp_v4l2_sensor_info *sensor_info, int w, int h, uint32_t* fps)
{
  /* search resolution from preset table
     *   for now, use the highest fps.
     *   this should be changed properly in the future to pick fps from application
     */
    int i, j;
    uint32_t idx = 0;

    for ( i = 0; i < sensor_info->preset_num; i++ ) {
        if ( sensor_info->preset[i].width == w && sensor_info->preset[i].height == h ) {
            if (custom_wdr_mode == 0) {
               *( (char *)&sensor_info->preset[i].fps_cur ) = 0;
               for ( j = 0; j < sensor_info->preset[i].fps_num; j++ ) {
                   if ( sensor_info->preset[i].wdr_mode[j] == custom_wdr_mode ) {
                       if (0 != custom_fps) {
                           if ( sensor_info->preset[i].fps[j] == custom_fps * 256) {
                               *fps = sensor_info->preset[i].fps[j];
                               idx = sensor_info->preset[i].idx[j];
                               *( (char *)&sensor_info->preset[i].fps_cur ) = j;
                           }
                       } else {
                           if ( sensor_info->preset[i].fps[j] > (*fps)) {
                               *fps = sensor_info->preset[i].fps[j];
                               idx = sensor_info->preset[i].idx[j];
                               *( (char *)&sensor_info->preset[i].fps_cur ) = j;
                           }
                       }
                   }
               }
               break;
            } else if ((custom_wdr_mode == 1) || (custom_wdr_mode == 2)) {
               for (j = 0; j < sensor_info->preset[i].fps_num; j++) {
                  if ((sensor_info->preset[i].exposures[j] == custom_exp) &&
                     (sensor_info->preset[i].wdr_mode[j] == custom_wdr_mode)) {
                     if (0 != custom_fps) {
                         if ( sensor_info->preset[i].fps[j] == custom_fps * 256) {
                             *fps = sensor_info->preset[i].fps[j];
                             idx = sensor_info->preset[i].idx[j];
                             *( (char *)&sensor_info->preset[i].fps_cur ) = j;
                         }
                     } else {
                         if ( sensor_info->preset[i].fps[j] > (*fps)) {
                             idx = sensor_info->preset[i].idx[j];
                             *fps = sensor_info->preset[i].fps[j];
                             LOG( LOG_INFO, "idx = %d, fps = %d\n", idx, *fps);
                             *( (char *)&sensor_info->preset[i].fps_cur ) = j;
                         }
                     }
                  }
               }
               break;
            } else {
               LOG( LOG_ERR, "Not Support wdr mode\n");
            }
        }
    }

    if ( i >= sensor_info->preset_num ) {
        LOG( LOG_CRIT, "invalid resolution (width = %d, height = %d)\n", w, h);
        return 0;
    }

    return idx;
}

/* fw-interface per-stream config interface */
int fw_intf_stream_set_resolution( uint32_t ctx_id, const isp_v4l2_sensor_info *sensor_info,
                                   isp_v4l2_stream_type_t streamType, uint32_t *width, uint32_t *height )
{
    /*
     * StreamType
     *   - FR : directly update sensor resolution since FR doesn't have down-scaler.
     *   - DS : need to be implemented.
     *   - RAW: can't be configured separately from FR.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    LOG( LOG_DEBUG, "streamtype:%d, w:%d h:%d\n", streamType, *width, *height );
    if ( streamType == V4L2_STREAM_TYPE_FR ) {
#if defined( TSENSOR ) && defined( SENSOR_PRESET )
        int result;
        uint32_t ret_val;
        uint32_t idx = 0x0;
        uint32_t fps = 0x0;
        uint32_t w, h;

        w = *width;
        h = *height;

        uint32_t width_cur, height_cur, exposure_cur, wdr_mode_cur, fps_cur;
        wdr_mode_cur = 0;
        exposure_cur = 0;
        fps_cur = 0;
        //check if we need to change sensor preset
        acamera_command( ctx_id, TSENSOR, SENSOR_WIDTH, 0, COMMAND_GET, &width_cur );
        acamera_command( ctx_id, TSENSOR, SENSOR_HEIGHT, 0, COMMAND_GET, &height_cur );
        acamera_command( ctx_id, TSENSOR, SENSOR_EXPOSURES, 0, COMMAND_GET, &exposure_cur );
        acamera_command( ctx_id, TSENSOR, SENSOR_WDR_MODE, 0, COMMAND_GET, &wdr_mode_cur );
        acamera_command( ctx_id, TSENSOR, SENSOR_FPS, 0, COMMAND_GET, &fps_cur );
        LOG( LOG_INFO, "target (width = %d, height = %d, fps = %d) current (w=%d h=%d exposure_cur = %d wdr_mode_cur = %d, fps = %d)",
        w, h, custom_fps, width_cur, height_cur, exposure_cur, wdr_mode_cur, fps_cur / 256);

        if ( width_cur != w || height_cur != h || exposure_cur != custom_exp || wdr_mode_cur != custom_wdr_mode || fps_cur / 256 != custom_fps) {

            idx = fw_intf_find_proper_present_idx(sensor_info, w, h, &fps);

            /* set sensor resolution preset */
            LOG( LOG_CRIT, "Setting new resolution : width = %d, height = %d (preset idx = %d, fps = %d)", w, h, idx, fps / 256 );
            result = acamera_command( ctx_id, TSENSOR, SENSOR_PRESET, idx, COMMAND_SET, &ret_val );
            *( (char *)&sensor_info->preset_cur ) = idx;
            if ( result ) {
                LOG( LOG_CRIT, "Failed to set preset to %u, ret_value: %d.", idx, result );
                return result;
            }
        } else {
            acamera_command( ctx_id, TSENSOR, SENSOR_PRESET, 0, COMMAND_GET, &idx );
            LOG( LOG_CRIT, "Leaving same sensor settings resolution : width = %d, height = %d (preset idx = %d)", w, h, idx );
        }
#endif
    }
#if ISP_HAS_DS1
    else if ( streamType == V4L2_STREAM_TYPE_DS1 ) {

        int result;
        uint32_t ret_val;
        uint32_t w, h;

        w = *width;
        h = *height;

        uint32_t width_cur, height_cur;
        //check if we need to change sensor preset
        acamera_command( ctx_id, TSENSOR, SENSOR_WIDTH, 0, COMMAND_GET, &width_cur );
        acamera_command( ctx_id, TSENSOR, SENSOR_HEIGHT, 0, COMMAND_GET, &height_cur );
        LOG( LOG_INFO, "target (width = %d, height = %d) current (w=%d h=%d)", w, h, width_cur, height_cur );
        if ( w > width_cur || h > height_cur ) {
            LOG( LOG_ERR, "Invalid target size: (width = %d, height = %d), current (w=%d h=%d)", w, h, width_cur, height_cur );
            return -EINVAL;
        }

#if defined( TIMAGE ) && defined( IMAGE_RESIZE_TYPE_ID ) && defined( IMAGE_RESIZE_WIDTH_ID )
        {
            result = acamera_command( ctx_id, TIMAGE, IMAGE_RESIZE_TYPE_ID, SCALER, COMMAND_SET, &ret_val );
            if ( result ) {
                LOG( LOG_CRIT, "Failed to set resize_type, ret_value: %d.", result );
                return result;
            }

            result = acamera_command( ctx_id, TIMAGE, IMAGE_RESIZE_WIDTH_ID, w, COMMAND_SET, &ret_val );
            if ( result ) {
                LOG( LOG_CRIT, "Failed to set resize_width, ret_value: %d.", result );
                return result;
            }
            result = acamera_command( ctx_id, TIMAGE, IMAGE_RESIZE_HEIGHT_ID, h, COMMAND_SET, &ret_val );
            if ( result ) {
                LOG( LOG_CRIT, "Failed to set resize_height, ret_value: %d.", result );
                return result;
            }

            result = acamera_command( ctx_id, TIMAGE, IMAGE_RESIZE_ENABLE_ID, RUN, COMMAND_SET, &ret_val );
            if ( result ) {
                LOG( LOG_CRIT, "Failed to set resize_enable, ret_value: %d.", result );
                return result;
            }
        }
#endif
    }
#endif

#if ISP_HAS_DS2
    else if ( streamType == V4L2_STREAM_TYPE_DS2 ) {

        uint32_t ret_val;
        uint32_t w, h;

        w = *width;
        h = *height;

        uint32_t width_cur, height_cur;
        //check if we need to change sensor preset
        acamera_command( ctx_id, TAML_SCALER, SCALER_WIDTH, 0, COMMAND_GET, &width_cur );
        acamera_command( ctx_id, TAML_SCALER, SCALER_HEIGHT, 0, COMMAND_GET, &height_cur );
        LOG( LOG_ERR, "target (width = %d, height = %d) current (w=%d h=%d)", w, h, width_cur, height_cur );
        if ( w != width_cur || h != height_cur ) {
            acamera_command(ctx_id, TAML_SCALER, SCALER_WIDTH, w, COMMAND_SET, &ret_val );
            acamera_command(ctx_id,  TAML_SCALER, SCALER_HEIGHT, h, COMMAND_SET, &ret_val );
        } else {
            LOG(LOG_ERR, "target resolution equal current resolution");
        }
    }
#endif

    return 0;
}

int fw_intf_stream_set_output_format( uint32_t ctx_id, isp_v4l2_stream_type_t streamType, uint32_t format )
{

#if defined( TIMAGE ) && defined( FR_FORMAT_BASE_PLANE_ID )
    uint32_t value;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    switch ( format ) {
#ifdef DMA_FORMAT_RGB32
    case V4L2_PIX_FMT_RGB32:
        value = RGB32;
        break;
#endif
#ifdef DMA_FORMAT_NV12_Y
    case V4L2_PIX_FMT_NV12:
        value = NV12_YUV;
        break;
    case V4L2_PIX_FMT_GREY:
        value = NV12_GREY;
        break;
    case V4L2_PIX_FMT_NV21:
        value = NV12_YVU;
        break;
#endif
#ifdef DMA_FORMAT_A2R10G10B10
    case ISP_V4L2_PIX_FMT_ARGB2101010:
        value = A2R10G10B10;
        break;
#endif
#ifdef DMA_FORMAT_RGB24
    case V4L2_PIX_FMT_RGB24:
        value = RGB24;
        break;
#endif
#ifdef DMA_FORMAT_AYUV
    case V4L2_PIX_FMT_YUV444:
        value = AYUV;
        break;
#endif
#ifdef DMA_FORMAT_YUY2
    case V4L2_PIX_FMT_YUYV:
        value = YUY2;
        break;
#endif
#ifdef DMA_FORMAT_UYVY
    case V4L2_PIX_FMT_UYVY:
        value = UYVY;
        break;
#endif
#ifdef DMA_FORMAT_RAW16
    case V4L2_PIX_FMT_SBGGR16:
        value = RAW16;
        break;
#endif
#ifdef DMA_FORMAT_DISABLE
    case ISP_V4L2_PIX_FMT_NULL:
        value = DMA_DISABLE;
        break;
#endif
    case ISP_V4L2_PIX_FMT_META:
        LOG( LOG_INFO, "Meta format 0x%x doesn't need to be set to firmware", format );
        return 0;
        break;
    default:
        LOG( LOG_ERR, "Requested format 0x%x is not supported by firmware !", format );
        return -1;
        break;
    }

    if ( streamType == V4L2_STREAM_TYPE_FR ) {

        uint8_t result;
        uint32_t ret_val;

        result = acamera_command( ctx_id, TIMAGE, FR_FORMAT_BASE_PLANE_ID, value, COMMAND_SET, &ret_val );
        LOG( LOG_INFO, "set format for stream %d to %d (0x%x)", streamType, value, format );
        if ( result ) {
            LOG( LOG_ERR, "TIMAGE - FR_FORMAT_BASE_PLANE_ID failed (value = 0x%x, result = %d)", value, result );
        }
    }
#if ISP_HAS_DS1
    else if ( streamType == V4L2_STREAM_TYPE_DS1 ) {

        uint8_t result;
        uint32_t ret_val;

        result = acamera_command( ctx_id, TIMAGE, DS1_FORMAT_BASE_PLANE_ID, value, COMMAND_SET, &ret_val );
        LOG( LOG_INFO, "set format for stream %d to %d (0x%x)", streamType, value, format );
        if ( result ) {
            LOG( LOG_ERR, "TIMAGE - DS1_FORMAT_BASE_PLANE_ID failed (value = 0x%x, result = %d)", value, result );
        }
    }
#endif

#if ISP_HAS_DS2
    else if ( streamType == V4L2_STREAM_TYPE_DS2 ) {

        uint8_t result;
        uint32_t ret_val;

        result = acamera_command( ctx_id, TAML_SCALER, SCALER_OUTPUT_MODE, value, COMMAND_SET, &ret_val );
        LOG( LOG_ERR, "set format for stream %d to %d (0x%x)", streamType, value, format );
        if ( result ) {
            LOG( LOG_ERR, "TIMAGE - DS2_FORMAT_BASE_PLANE_ID failed (value = 0x%x, result = %d)", value, result );
        }
    }
#endif

#else
    LOG( LOG_ERR, "cannot find proper API for fr base mode ID" );
#endif


    return 0;
}

static int fw_intf_set_fr_fps(uint32_t ctx_id, uint32_t fps)
{
    uint32_t cur_fps = 0;

    acamera_command(ctx_id, TSENSOR, SENSOR_FPS, 0, COMMAND_GET, &cur_fps);
    if (cur_fps == 0) {
        LOG(LOG_ERR, "Error input param\n");
        return -1;
    }

    cur_fps = cur_fps / 256;

    acamera_api_set_fps(ctx_id, dma_fr, cur_fps, fps);

    return 0;
}

static int fw_intf_set_sensor_testpattern(uint32_t ctx_id, uint32_t val)
{
    uint32_t mode = val;
    uint32_t ret_val;
    acamera_command(ctx_id, TSENSOR, SENSOR_TESTPATTERN, mode, COMMAND_SET, &ret_val);
    if (mode <= 0) {
        LOG(LOG_ERR, "Error input param\n");
        return -1;
    }
    return 0;
}

static int fw_intf_set_sensor_ir_cut_set(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t ir_cut_state = ctrl_val;
    acamera_command(ctx_id, TSENSOR, SENSOR_IR_CUT, 0, COMMAND_SET, &ir_cut_state);
    return 0;
}

static int fw_intf_set_ds1_fps(uint32_t ctx_id, uint32_t fps)
{
    uint32_t cur_fps = 0;

    acamera_command(ctx_id, TSENSOR, SENSOR_FPS, 0, COMMAND_GET, &cur_fps);
    if (cur_fps == 0) {
        LOG(LOG_ERR, "Error input param\n");
        return -1;
    }

    cur_fps = cur_fps / 256;

    acamera_api_set_fps(ctx_id, dma_ds1, cur_fps, fps);

    return 0;
}


static int fw_intf_set_ae_zone_weight(uint32_t ctx_id, unsigned long ctrl_val)
{
    acamera_command(ctx_id, TALGORITHMS, AE_ZONE_WEIGHT, 0, COMMAND_SET, (uint32_t *)ctrl_val);

    return 0;
}

static int fw_intf_set_awb_zone_weight(uint32_t ctx_id, unsigned long ctrl_val)
{
    acamera_command(ctx_id, TALGORITHMS, AWB_ZONE_WEIGHT, 0, COMMAND_SET, (uint32_t *)ctrl_val);

    return 0;
}

static int fw_intf_set_sensor_integration_time(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t manual_sensor_integration_time = ctrl_val;
    acamera_command(ctx_id, TSYSTEM, SYSTEM_INTEGRATION_TIME, manual_sensor_integration_time, COMMAND_SET, &ctrl_val );

    return 0;
}

static int fw_intf_set_sensor_analog_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t manual_sensor_analog_gain = ctrl_val;
    acamera_command(ctx_id, TSYSTEM, SYSTEM_SENSOR_ANALOG_GAIN, manual_sensor_analog_gain, COMMAND_SET, &ctrl_val );

    return 0;
}

static int fw_intf_set_isp_digital_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t manual_isp_digital_gain = ctrl_val;
    acamera_command(ctx_id, TSYSTEM, SYSTEM_ISP_DIGITAL_GAIN, manual_isp_digital_gain, COMMAND_SET, &ctrl_val );

    return 0;
}

static int fw_intf_set_stop_sensor_update(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t stop_sensor_update = ctrl_val;
    LOG(LOG_ERR, "stop_sensor_update = %d\n", stop_sensor_update);
    acamera_command(ctx_id, TSYSTEM, SYSTEM_FREEZE_FIRMWARE, stop_sensor_update, COMMAND_SET, &ctrl_val);
    return 0;
}

static int fw_intf_set_sensor_digital_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t manual_sensor_digital_gain = ctrl_val;
    acamera_command(ctx_id, TSYSTEM, SYSTEM_SENSOR_DIGITAL_GAIN, manual_sensor_digital_gain, COMMAND_SET, &ctrl_val );

    return 0;
}

static int fw_intf_set_awb_red_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t awb_red_gain = ctrl_val;
    acamera_command(ctx_id, TSYSTEM, SYSTEM_AWB_RED_GAIN, awb_red_gain, COMMAND_SET, &ctrl_val );

    return 0;
}

static int fw_intf_set_awb_blue_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    uint32_t awb_blue_gain = ctrl_val;
    acamera_command(ctx_id, TSYSTEM, SYSTEM_AWB_BLUE_GAIN, awb_blue_gain, COMMAND_SET, &ctrl_val );

    return 0;
}

/* ----------------------------------------------------------------
 * Internal handler for control interface functions
 */
static bool isp_fw_do_validate_control( uint32_t id )
{
    return 1;
}

static int isp_fw_do_set_test_pattern( uint32_t ctx_id, int enable )
{
#if defined( TSYSTEM ) && defined( TEST_PATTERN_ENABLE_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_CRIT, "test_pattern: %d.", enable );

    if ( enable < 0 )
        return -EIO;

    if ( !isp_started ) {
        LOG( LOG_CRIT, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = application_command( TSYSTEM, TEST_PATTERN_ENABLE_ID, enable ? ON : OFF, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_CRIT, "Failed to set TEST_PATTERN_ENABLE_ID to %u, ret_value: %d.", enable, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_test_pattern_type( uint32_t ctx_id, int pattern_type )
{
#if defined( TSYSTEM ) && defined( TEST_PATTERN_MODE_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "test_pattern_type: %d.", pattern_type );

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSYSTEM, TEST_PATTERN_MODE_ID, pattern_type, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set TEST_PATTERN_MODE_ID to %d, ret_value: %d.", pattern_type, result );
        return result;
    }
#endif

    return 0;
}


static int isp_fw_do_set_af_refocus( uint32_t ctx_id, int val )
{
#if defined( TALGORITHMS ) && defined( AF_MODE_ID )
    int result;
    u32 ret_val;

    result = acamera_command( ctx_id, TALGORITHMS, AF_MODE_ID, AF_AUTO_SINGLE, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AF_MODE_ID to AF_AUTO_SINGLE, ret_value: %u.", ret_val );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_af_roi( uint32_t ctx_id, int val )
{
#if defined( TALGORITHMS ) && defined( AF_ROI_ID )
    int result;
    u32 ret_val;

    result = acamera_command( ctx_id, TALGORITHMS, AF_ROI_ID, (uint32_t)val, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AF_MODE_ID to AF_AUTO_SINGLE, ret_value: %u.", ret_val );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_brightness( uint32_t ctx_id, int brightness )
{
#if defined( TSCENE_MODES ) && defined( BRIGHTNESS_STRENGTH_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "brightness: %d.", brightness );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, BRIGHTNESS_STRENGTH_ID, brightness, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set BRIGHTNESS_STRENGTH_ID to %d, ret_value: %d.", brightness, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_contrast( uint32_t ctx_id, int contrast )
{
#if defined( TSCENE_MODES ) && defined( CONTRAST_STRENGTH_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "contrast: %d.", contrast );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, CONTRAST_STRENGTH_ID, contrast, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set CONTRAST_STRENGTH_ID to %d, ret_value: %d.", contrast, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_saturation( uint32_t ctx_id, int saturation )
{
#if defined( TSCENE_MODES ) && defined( SATURATION_STRENGTH_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "saturation: %d.", saturation );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, SATURATION_STRENGTH_ID, saturation, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set SATURATION_STRENGTH_ID to %d, ret_value: %d.", saturation, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_hue( uint32_t ctx_id, int hue )
{
#if defined( TSCENE_MODES ) && defined( HUE_THETA_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "hue: %d.", hue );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, HUE_THETA_ID, hue, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set HUE_THETA_ID to %d, ret_value: %d.", hue, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_sharpness( uint32_t ctx_id, int sharpness )
{
#if defined( TSCENE_MODES ) && defined( SHARPENING_STRENGTH_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "sharpness: %d.", sharpness );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, SHARPENING_STRENGTH_ID, sharpness, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set SHARPENING_STRENGTH_ID to %d, ret_value: %d.", sharpness, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_color_fx( uint32_t ctx_id, int idx )
{
#if defined( TSCENE_MODES ) && defined( COLOR_MODE_ID )
    int result;
    uint32_t ret_val;
    int color_idx;

    LOG( LOG_INFO, "idx: %d.", idx );

    switch ( idx ) {
    case V4L2_COLORFX_NONE:
        color_idx = NORMAL;
        break;
    case V4L2_COLORFX_BW:
        color_idx = BLACK_AND_WHITE;
        break;
    case V4L2_COLORFX_SEPIA:
        color_idx = SEPIA;
        break;
    case V4L2_COLORFX_NEGATIVE:
        color_idx = NEGATIVE;
        break;
    case V4L2_COLORFX_VIVID:
        color_idx = VIVID;
        break;
    default:
        return -EINVAL;
        break;
    }

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, COLOR_MODE_ID, color_idx, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set SYSTEM_ISP_DIGITAL_GAIN to %d, ret_value: %d.", color_idx, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_hflip( uint32_t ctx_id, bool enable )
{
#if defined( TIMAGE ) && defined( ORIENTATION_HFLIP_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "hflip enable: %d.", enable );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TIMAGE, ORIENTATION_HFLIP_ID, enable ? ENABLE : DISABLE, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set ORIENTATION_HFLIP_ID to %d, ret_value: %d.", enable, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_vflip( uint32_t ctx_id, bool enable )
{
#if defined( TIMAGE ) && defined( ORIENTATION_VFLIP_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "vflip enable: %d.", enable );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TIMAGE, ORIENTATION_VFLIP_ID, enable ? ENABLE : DISABLE, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set ORIENTATION_VFLIP_ID to %d, ret_value: %d.", enable, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_manual_gain( uint32_t ctx_id, bool enable )
{
#if defined( TALGORITHMS ) && defined( AE_MODE_ID )
    int result;
    uint32_t mode = 0;
    uint32_t ret_val;

    LOG( LOG_INFO, "manual_gain enable: %d.", enable );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_MODE_ID, 0, COMMAND_GET, &ret_val );
    LOG( LOG_INFO, "AE_MODE_ID = %d", ret_val );
    if ( enable ) {
        if ( ret_val == AE_AUTO ) {
            mode = AE_MANUAL_GAIN;
        } else if ( ret_val == AE_MANUAL_EXPOSURE_TIME ) {
            mode = AE_FULL_MANUAL;
        } else {
            LOG( LOG_INFO, "Manual gain is already enabled." );
            return 0;
        }
    } else {
        if ( ret_val == AE_MANUAL_GAIN ) {
            mode = AE_AUTO;
        } else if ( ret_val == AE_FULL_MANUAL ) {
            mode = AE_MANUAL_EXPOSURE_TIME;
        } else {
            LOG( LOG_INFO, "Manual gain is already disabled." );
            return 0;
        }
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_MODE_ID, mode, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AE_MODE_ID to %u, ret_value: %d.", mode, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_gain( uint32_t ctx_id, int gain )
{
#if defined( TALGORITHMS ) && defined( AE_GAIN_ID )
    int result;
    int gain_frac;
    uint32_t ret_val;

    LOG( LOG_INFO, "gain: %d.", gain );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_MODE_ID, 0, COMMAND_GET, &ret_val );
    LOG( LOG_INFO, "AE_MODE_ID = %d", ret_val );
    if ( ret_val != AE_FULL_MANUAL && ret_val != AE_MANUAL_GAIN ) {
        LOG( LOG_ERR, "Cannot set gain while AE_MODE is %d", ret_val );
        return 0;
    }

    gain_frac = gain / 100;
    gain_frac += ( gain % 100 ) * 256 / 100;

    result = acamera_command( ctx_id, TALGORITHMS, AE_GAIN_ID, gain_frac, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AE_GAIN_ID to %d, ret_value: %d.", gain, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_exposure_auto( uint32_t ctx_id, int enable )
{
#if defined( TALGORITHMS ) && defined( AE_MODE_ID )
    int result;
    uint32_t mode = 0;
    uint32_t ret_val;

    LOG( LOG_INFO, "manual_exposure enable: %d.", enable );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_MODE_ID, 0, COMMAND_GET, &ret_val );
    LOG( LOG_CRIT, "AE_MODE_ID = %d", ret_val );
    switch ( enable ) {
    case true:
        if ( ret_val == AE_AUTO ) {
            mode = AE_MANUAL_EXPOSURE_TIME;
        } else if ( ret_val == AE_MANUAL_GAIN ) {
            mode = AE_FULL_MANUAL;
        } else {
            LOG( LOG_INFO, "Manual exposure is already enabled." );
            return 0;
        }
        break;
    case false:
        if ( ret_val == AE_MANUAL_EXPOSURE_TIME ) {
            mode = AE_AUTO;
        } else if ( ret_val == AE_FULL_MANUAL ) {
            mode = AE_MANUAL_GAIN;
        } else {
            LOG( LOG_INFO, "Manual exposure is already disabled." );
            return 0;
        }
        break;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_MODE_ID, mode, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AE_MODE_ID to %u, ret_value: %d.", mode, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_manual_exposure( uint32_t ctx_id, int enable )
{
#if defined( TALGORITHMS ) && defined( AE_MODE_ID )
    int result_integration_time, result_sensor_analog_gain, result_sensor_digital_gain, result_isp_digital_gain;
    uint32_t ret_val;

    LOG( LOG_ERR, "manual exposure enable: %d.", enable );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result_integration_time = acamera_command( ctx_id, TSYSTEM, SYSTEM_MANUAL_INTEGRATION_TIME, enable, COMMAND_SET, &ret_val );
    if ( result_integration_time ) {
        LOG( LOG_ERR, "Failed to set manual_integration_time to manual mode, ret_value: %d", result_integration_time );
        return ( result_integration_time );
    }

    result_sensor_analog_gain = acamera_command( ctx_id, TSYSTEM, SYSTEM_MANUAL_SENSOR_ANALOG_GAIN, enable, COMMAND_SET, &ret_val );
    if ( result_sensor_analog_gain ) {
        LOG( LOG_ERR, "Failed to set manual_sensor_analog_gain to manual mode, ret_value: %d", result_sensor_analog_gain );
        return ( result_sensor_analog_gain );
    }

    result_sensor_digital_gain = acamera_command( ctx_id, TSYSTEM, SYSTEM_MANUAL_SENSOR_DIGITAL_GAIN, enable, COMMAND_SET, &ret_val );
    if ( result_sensor_analog_gain ) {
        LOG( LOG_ERR, "Failed to set manual_sensor_digital_gain to manual mode, ret_value: %d", result_sensor_digital_gain );
        return ( result_sensor_digital_gain );
    }

    result_isp_digital_gain = acamera_command( ctx_id, TSYSTEM, SYSTEM_MANUAL_ISP_DIGITAL_GAIN, enable, COMMAND_SET, &ret_val );
    if ( result_isp_digital_gain ) {
        LOG( LOG_ERR, "Failed to set manual_isp_digital_gain to manual mode, ret_value: %d", result_isp_digital_gain );
        return ( result_isp_digital_gain );
    }

#endif

    return 0;
}

/* set exposure in us unit */
static int isp_fw_do_set_exposure( uint32_t ctx_id, int exp )
{
#if defined( TALGORITHMS ) && defined( AE_EXPOSURE_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "exp in ms: %d.", exp );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_EXPOSURE_ID, exp * 1000, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AE_EXPOSURE_ID to %d, ret_value: %d.", exp, result );
        return result;
    }
#endif
    return 0;
}

static int isp_fw_do_set_variable_frame_rate( uint32_t ctx_id, int enable )
{
    // SYSTEM_EXPOSURE_PRIORITY ??
    return 0;
}

static int isp_fw_do_set_white_balance_mode( uint32_t ctx_id, int wb_mode )
{
#if defined( TALGORITHMS ) && defined( AWB_MODE_ID )
#if defined( ISP_HAS_AWB_MESH_FSM ) || defined( ISP_HAS_AWB_MESH_NBP_FSM ) || defined( ISP_HAS_AWB_MANUAL_FSM )
    static int32_t last_wb_request = AWB_DAY_LIGHT;
#else
    static int32_t last_wb_request = AWB_MANUAL;
#endif
    int result;
    uint32_t mode = 0;
    uint32_t ret_val;

    LOG( LOG_INFO, "new white balance request: %d.", wb_mode );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AWB_MODE_ID, 0, COMMAND_GET, &ret_val );
    LOG( LOG_CRIT, "AWB_MODE_ID = %d", ret_val );
    switch ( wb_mode ) {
    case AWB_MANUAL:
        /* we set the last mode instead of MANUAL */
        mode = last_wb_request;
        break;
    case AWB_AUTO:
        /* we set the last mode instead of MANUAL */
        if ( ret_val != AWB_AUTO ) {
            mode = wb_mode;
        } else {
            LOG( LOG_INFO, "Auto WB is already enabled." );
            return 0;
        }
        break;
#if defined( ISP_HAS_AWB_MESH_FSM ) || defined( ISP_HAS_AWB_MESH_NBP_FSM ) || defined( ISP_HAS_AWB_MANUAL_FSM )
    case AWB_DAY_LIGHT:
    case AWB_CLOUDY:
    case AWB_INCANDESCENT:
    case AWB_FLOURESCENT:
    case AWB_TWILIGHT:
    case AWB_SHADE:
    case AWB_WARM_FLOURESCENT:
        if ( ret_val != AWB_AUTO ) {
            mode = wb_mode;
        } else {
            /* wb mode is not updated when it's in auto mode */
            LOG( LOG_INFO, "Auto WB is enabled, remembering mode." );
            last_wb_request = wb_mode;
            return 0;
        }
        break;
#endif
    default:
        return -EINVAL;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AWB_MODE_ID, mode, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AWB_MODE_ID to %u, ret_value: %d.", mode, result );
        return result;
    }
#endif
    return 0;
}

static int isp_fw_do_set_focus_auto( uint32_t ctx_id, int enable )
{
#if defined( TALGORITHMS ) && defined( AF_MODE_ID )
    int result;
    uint32_t mode = 0;
    uint32_t ret_val;

    LOG( LOG_INFO, "new auto focus request: %d.", enable );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AF_MODE_ID, 0, COMMAND_GET, &ret_val );
    LOG( LOG_CRIT, "AF_MODE_ID = %d", ret_val );
    switch ( enable ) {
    case 1:
        /* we set the last mode instead of MANUAL */
        if ( ret_val != AF_AUTO_CONTINUOUS ) {
            mode = AF_AUTO_CONTINUOUS;
        } else {
            LOG( LOG_INFO, "Auto focus is already enabled." );
            return 0;
        }
        break;
    case 0:
        if ( ret_val != AF_MANUAL ) {
            mode = AF_MANUAL;
        } else {
            /* wb mode is not updated when it's in auto mode */
            LOG( LOG_INFO, "Auto WB is enabled, remembering mode." );
            return 0;
        }
        break;
    default:
        return -EINVAL;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AF_MODE_ID, mode, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AF_MODE_ID to %u, ret_value: %d.", mode, result );
        return result;
    }
#endif

    return 0;
}

static int isp_fw_do_set_focus( uint32_t ctx_id, int focus )
{
#if defined( TALGORITHMS ) && defined( AF_MANUAL_CONTROL_ID )
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "focus: %d.", focus );

    /* some controls(such brightness) will call acamera_command()
     * before isp_fw initialed, so we need to check.
     */
    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AF_MANUAL_CONTROL_ID, focus, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AF_MANUAL_CONTROL_ID to %d, ret_value: %d.", focus, result );
        return result;
    }
#endif
    return 0;
}

static int isp_fw_do_set_ae_compensation( uint32_t ctx_id, int val )
{
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "ae_compensation: %d.", val );

    if ( val < 0 )
        return -EIO;

    if ( !isp_started ) {
        LOG( LOG_NOTICE, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_COMPENSATION_ID, val, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AE_COMPENSATION to %u, ret_value: %d.", val, result );
        return result;
    }
    return 0;
}

static int isp_fw_do_set_max_integration_time( uint32_t ctx_id, int val )
{
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "_max_integration_time: %d.", val );

    if ( val < 0 )
        return -EIO;

    if ( !isp_started ) {
        LOG( LOG_NOTICE, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSYSTEM, SYSTEM_MAX_INTEGRATION_TIME, val, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set max_integration_time to %u, ret_value: %d.", val, result );
        return result;
    }
    return 0;
}

static int isp_fw_do_set_snr_manual( uint32_t ctx_id, int val )
{
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "snr maunal: %d.", val );

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, SNR_MANUAL_ID, val, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set SNR_MANUAL_ID to %d, ret_value: %d.", val, result );
        return result;
    }

    return 0;

}

static int isp_fw_do_set_snr_offset( uint32_t ctx_id, int val )
{
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "snr offset: %d.", val );

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, SNR_OFFSET_ID, val, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set SNR_OFFSET_ID to %d, ret_value: %d.", val, result );
        return result;
    }

    return 0;

}

static int isp_fw_do_set_tnr_manual( uint32_t ctx_id, int val )
{
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "tnr maunal: %d.", val );

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, TNR_MANUAL_ID, val, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set TNR_MANUAL_ID to %d, ret_value: %d.", val, result );
        return result;
    }

    return 0;

}

static int isp_fw_do_set_tnr_offset( uint32_t ctx_id, int val )
{
    int result;
    uint32_t ret_val;

    LOG( LOG_INFO, "tnr offset: %d.", val );

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, TNR_OFFSET_ID, val, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set TNR_OFFSET_ID to %d, ret_value: %d.", val, result );
        return result;
    }

    return 0;

}

static int isp_fw_do_set_temper_mode( uint32_t ctx_id, int val )
{
    int result;
    uint32_t ret_val;
    uint32_t temper_mode;

    LOG( LOG_INFO, "temper mode: %d.", val );

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    if ( val == 1 )
        temper_mode = TEMPER2_MODE;
    else
        temper_mode = TEMPER3_MODE;

    result = acamera_command( ctx_id, TSYSTEM, TEMPER_MODE_ID, temper_mode, COMMAND_SET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set TEMPER_MODE_ID to %d, ret_value: %d.", temper_mode, result );
        return result;
    }

    return 0;

}

static int isp_fw_do_get_ae_compensation( uint32_t ctx_id )
{
    int result;
    uint32_t ret_val;

    if ( !isp_started ) {
        LOG( LOG_NOTICE, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TALGORITHMS, AE_COMPENSATION_ID, 0, COMMAND_GET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set AE_COMPENSATION, ret_value: %d.", result );
        return result;
    }

    return ret_val;
}

static int isp_fw_do_get_snr_manual( uint32_t ctx_id )
{
    int result;
    uint32_t ret_val;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, SNR_MANUAL_ID, 0, COMMAND_GET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set SNR_MANUAL_ID, ret_value: %d.", result );
        return result;
    }

    return ret_val;
}

static int isp_fw_do_get_snr_offset( uint32_t ctx_id )
{
    int result;
    uint32_t ret_val;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, SNR_OFFSET_ID, 0, COMMAND_GET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set SNR_OFFSET_ID, ret_value: %d.", result );
        return result;
    }

    return ret_val;

}

static int isp_fw_do_get_tnr_manual( uint32_t ctx_id )
{
    int result;
    uint32_t ret_val;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, TNR_MANUAL_ID, 0, COMMAND_GET, &ret_val );
	if ( result ) {
        LOG( LOG_ERR, "Failed to set TNR_MANUAL_ID, ret_value: %d.", result );
        return result;
    }

    return ret_val;

}

static int isp_fw_do_get_tnr_offset( uint32_t ctx_id )
{
    int result;
    uint32_t ret_val;

    if ( !isp_started ) {
        LOG( LOG_ERR, "ISP FW not inited yet" );
        return -EBUSY;
    }

    result = acamera_command( ctx_id, TSCENE_MODES, TNR_OFFSET_ID, 0, COMMAND_GET, &ret_val );
    if ( result ) {
        LOG( LOG_ERR, "Failed to set TNR_OFFSET_ID, ret_value: %d.", result );
        return result;
    }

    return ret_val;

}

/* ----------------------------------------------------------------
 * fw_interface config interface
 */
bool fw_intf_validate_control( uint32_t id )
{
    return isp_fw_do_validate_control( id );
}

int fw_intf_set_test_pattern( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_test_pattern( ctx_id, val );
}

int fw_intf_set_test_pattern_type( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_test_pattern_type( ctx_id, val );
}

int fw_intf_set_af_refocus( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_af_refocus( ctx_id, val );
}

int fw_intf_set_af_roi( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_af_roi( ctx_id, val );
}

int fw_intf_set_brightness( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_brightness( ctx_id, val );
}

int fw_intf_set_contrast( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_contrast( ctx_id, val );
}

int fw_intf_set_saturation( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_saturation( ctx_id, val );
}

int fw_intf_set_hue( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_hue( ctx_id, val );
}

int fw_intf_set_sharpness( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_sharpness( ctx_id, val );
}

int fw_intf_set_color_fx( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_color_fx( ctx_id, val );
}

int fw_intf_set_hflip( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_hflip( ctx_id, val ? 1 : 0 );
}

int fw_intf_set_vflip( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_vflip( ctx_id, val ? 1 : 0 );
}

int fw_intf_set_autogain( uint32_t ctx_id, int val )
{
    /* autogain enable: disable manual gain.
     * autogain disable: enable manual gain.
     */
    return isp_fw_do_set_manual_gain( ctx_id, val ? 0 : 1 );
}

int fw_intf_set_gain( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_gain( ctx_id, val );
}

int fw_intf_set_exposure_auto( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_exposure_auto( ctx_id, val );
}

int fw_intf_set_exposure( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_exposure( ctx_id, val );
}

int fw_intf_set_variable_frame_rate( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_variable_frame_rate( ctx_id, val );
}

int fw_intf_set_white_balance_auto( uint32_t ctx_id, int val )
{
#ifdef AWB_MODE_ID
    int mode;

    if ( val == true )
        mode = AWB_AUTO;
    else
        mode = AWB_MANUAL;

    return isp_fw_do_set_white_balance_mode( ctx_id, mode );
#endif

    // return SUCCESS for compatibility verfication issue
    return 0;
}

int fw_intf_set_white_balance( uint32_t ctx_id, int val )
{
    int mode = 0;

    switch ( val ) {
#ifdef AWB_MODE_ID
#if defined( ISP_HAS_AWB_MESH_FSM ) || defined( ISP_HAS_AWB_MESH_NBP_FSM ) || defined( ISP_HAS_AWB_MANUAL_FSM )
    case 8000:
        mode = AWB_SHADE;
        break;
    case 7000:
        mode = AWB_CLOUDY;
        break;
    case 6000:
    case 5000:
        mode = AWB_DAY_LIGHT;
        break;
    case 4000:
        mode = AWB_FLOURESCENT;
        break;
    case 3000:
        mode = AWB_WARM_FLOURESCENT;
        break;
    case 2000:
        mode = AWB_INCANDESCENT;
        break;
#endif
#endif
    default:
        // return SUCCESS for compatibility verfication issue
        return 0;
    }

    return isp_fw_do_set_white_balance_mode( ctx_id, mode );
}

int fw_intf_set_focus_auto( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_focus_auto( ctx_id, val );
}

int fw_intf_set_focus( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_focus( ctx_id, val );
}

int fw_intf_set_output_fr_on_off( uint32_t ctx_id, uint32_t ctrl_val )
{
    return fw_intf_stream_set_output_format( ctx_id, V4L2_STREAM_TYPE_FR, ctrl_val );
}

int fw_intf_set_output_ds1_on_off( uint32_t ctx_id, uint32_t ctrl_val )
{
#if ISP_HAS_DS1
    return fw_intf_stream_set_output_format( ctx_id, V4L2_STREAM_TYPE_DS1, ctrl_val );
#else
    return 0;
#endif
}

int fw_intf_set_custom_sensor_wdr_mode(uint32_t ctx_id, uint32_t ctrl_val)
{
    custom_wdr_mode = ctrl_val;
    return 0;
}

int fw_intf_set_custom_sensor_exposure(uint32_t ctx_id, uint32_t ctrl_val)
{
    custom_exp = ctrl_val;
    return 0;
}

int fw_intf_set_custom_sensor_fps(uint32_t ctx_id, uint32_t ctrl_val)
{
    custom_fps = ctrl_val;
    return 0;
}

int fw_intf_set_custom_snr_manual(uint32_t ctx_id, uint32_t ctrl_val)
{
    return isp_fw_do_set_snr_manual(ctx_id, ctrl_val);
}

int fw_intf_set_custom_snr_offset(uint32_t ctx_id, uint32_t ctrl_val)
{
    return isp_fw_do_set_snr_offset(ctx_id, ctrl_val);
}

int fw_intf_set_custom_tnr_manual(uint32_t ctx_id, uint32_t ctrl_val)
{
    return isp_fw_do_set_tnr_manual(ctx_id, ctrl_val);
}

int fw_intf_set_custom_tnr_offset(uint32_t ctx_id, uint32_t ctrl_val)
{
    return isp_fw_do_set_tnr_offset(ctx_id, ctrl_val);
}

int fw_intf_set_custom_fr_fps(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    rtn = fw_intf_set_fr_fps(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_custom_ds1_fps(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    rtn = fw_intf_set_ds1_fps(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_custom_sensor_testpattern(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    rtn = fw_intf_set_sensor_testpattern(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_sensor_ir_cut(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    rtn = fw_intf_set_sensor_ir_cut_set(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_ae_zone_weight(uint32_t ctx_id, unsigned long ctrl_val)
{
    int rtn = -1;

    rtn = fw_intf_set_ae_zone_weight(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_awb_zone_weight(uint32_t ctx_id, unsigned long ctrl_val)
{
    int rtn = -1;

    rtn = fw_intf_set_awb_zone_weight(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_manual_exposure( uint32_t ctx_id, int val )
{
    int rtn = -1;

    if ( val == -1) {
       return 0;
    }

    rtn = isp_fw_do_set_manual_exposure( ctx_id, val );

    return rtn;
}

int fw_intf_set_customer_sensor_integration_time(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    if ( ctrl_val == -1) {
       return 0;
    }

    rtn = fw_intf_set_sensor_integration_time(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_sensor_analog_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    if ( ctrl_val == -1) {
       return 0;
    }

    rtn = fw_intf_set_sensor_analog_gain(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_isp_digital_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    if ( ctrl_val == -1) {
       return 0;
    }

    rtn = fw_intf_set_isp_digital_gain(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_stop_sensor_update(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    if ( ctrl_val == -1) {
       return 0;
    }

    rtn = fw_intf_set_stop_sensor_update(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_ae_compensation( uint32_t ctx_id, int val )
{
    return isp_fw_do_set_ae_compensation( ctx_id, val );
}

int fw_intf_set_customer_sensor_digital_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    if ( ctrl_val == -1) {
       return 0;
    }

    rtn = fw_intf_set_sensor_digital_gain(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_awb_red_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    if ( ctrl_val == -1) {
       return 0;
    }

    rtn = fw_intf_set_awb_red_gain(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_awb_blue_gain(uint32_t ctx_id, uint32_t ctrl_val)
{
    int rtn = -1;

    if ( ctrl_val == -1) {
       return 0;
    }

    rtn = fw_intf_set_awb_blue_gain(ctx_id, ctrl_val);

    return rtn;
}

int fw_intf_set_customer_max_integration_time(uint32_t ctx_id, uint32_t ctrl_val)
{
    if ( ctrl_val == -1) {
       return 0;
    }
    return isp_fw_do_set_max_integration_time(ctx_id, ctrl_val);
}

int fw_intf_set_customer_temper_mode(uint32_t ctx_id, uint32_t ctrl_val)
{
    if ( ctrl_val != 1 || ctrl_val != 2) {
       return 0;
    }

    settings[ctx_id].temper_frames_number = ctrl_val;

    return isp_fw_do_set_temper_mode(ctx_id, ctrl_val);
}

int fw_intf_get_ae_compensation( uint32_t ctx_id )
{
    return isp_fw_do_get_ae_compensation( ctx_id );
}

int fw_intf_get_custom_snr_manual(uint32_t ctx_id)
{
    return isp_fw_do_get_snr_manual(ctx_id);
}

int fw_intf_get_custom_snr_offset(uint32_t ctx_id)
{
    return isp_fw_do_get_snr_offset(ctx_id);
}

int fw_intf_get_custom_tnr_manual(uint32_t ctx_id)
{
    return isp_fw_do_get_tnr_manual(ctx_id);
}

int fw_intf_get_custom_tnr_offset(uint32_t ctx_id)
{
    return isp_fw_do_get_tnr_offset(ctx_id);
}

int fw_intf_get_custom_temper_mode(uint32_t ctx_id)
{
    return settings[ctx_id].temper_frames_number;
}
