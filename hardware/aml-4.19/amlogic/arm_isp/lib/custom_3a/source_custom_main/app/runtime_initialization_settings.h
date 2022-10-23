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

#include "acamera_types.h"
#include "acamera_firmware_config.h"
#include "acamera_firmware_api.h"
#include "acamera_sensor_api.h"
#include "acamera_lens_api.h"


static tframe_t sensor0_dummy_fr_frames[ ] = {
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x0, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x7e9000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x7e9000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0xfd2000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0xfd2000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x17bb000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x17bb000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x1fa4000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x1fa4000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x278d000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x278d000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x2f76000, 7680, 0x7e9000 }}
} ;

static tframe_t sensor0_dummy_ds1_frames[ ] = {
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x2f76000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x375f000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x375f000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x3f48000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x3f48000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x4731000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x4731000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x4f1a000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x4f1a000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x5703000, 7680, 0x7e9000 }},
 {{ FW_OUTPUT_FORMAT, 0, 0, 0x5703000, 7680, 0x7e9000 } , { FW_OUTPUT_FORMAT_SECONDARY, 0, 0, 0x5eec000, 7680, 0x7e9000 }}
} ;

static aframe_t sensor0_dummy_temper_frames[ ] = {
 { FW_OUTPUT_FORMAT, 0, 0, 0x5eec000, 7680, 0x7e9000 },
 { FW_OUTPUT_FORMAT, 0, 0, 0x66d5000, 7680, 0x7e9000 }
} ;


extern void sensor_init_dummy( void** ctx, sensor_control_t*) ;
extern void sensor_deinit_dummy( void *ctx ) ;
extern uint32_t user2kernel_get_calibrations( uint32_t ctx_num,void * sensor_arg,ACameraCalibrations *) ;

extern int32_t lens_init( void** ctx, lens_control_t* ctrl ) ;
extern void lens_deinit( void * ctx) ;

extern void custom_initialization( uint32_t ctx_num ) ;

extern void callback_fr( uint32_t ctx_num,  tframe_t * tframe, const metadata_t *metadata) ;
extern void callback_ds1( uint32_t ctx_num,  tframe_t * tframe, const metadata_t *metadata) ;
extern void callback_ds2( uint32_t ctx_num,  tframe_t * tframe, const metadata_t *metadata) ;

static acamera_settings settings[ FIRMWARE_CONTEXT_NUMBER ] = {    {
        .sensor_init = sensor_init_dummy,
        .sensor_deinit = sensor_deinit_dummy,
        .get_calibrations = user2kernel_get_calibrations,
        .lens_init = lens_init,
        .lens_deinit = lens_deinit,
        .custom_initialization = custom_initialization,
        .isp_base = 0x0,
        .temper_frames = sensor0_dummy_temper_frames,
        .temper_frames_number = sizeof( sensor0_dummy_temper_frames ) / sizeof( aframe_t ),
        .fr_frames = sensor0_dummy_fr_frames,
        .fr_frames_number = sizeof( sensor0_dummy_fr_frames ) / sizeof( tframe_t ),
        .callback_fr = callback_fr,
        .ds1_frames = sensor0_dummy_ds1_frames,
        .ds1_frames_number = sizeof( sensor0_dummy_ds1_frames ) / sizeof( tframe_t ),
        .callback_ds1 = callback_ds1,
    },
#if FIRMWARE_CONTEXT_NUMBER == 2
	{
	    .sensor_init = sensor_init_dummy,
		.sensor_deinit = sensor_deinit_dummy,
		.get_calibrations = user2kernel_get_calibrations,
		.lens_init = lens_init,
		.lens_deinit = lens_deinit,
		.custom_initialization = custom_initialization,
		.isp_base = 0x0,
		.temper_frames = sensor0_dummy_temper_frames,
		.temper_frames_number = sizeof( sensor0_dummy_temper_frames ) / sizeof( aframe_t ),
		.fr_frames = sensor0_dummy_fr_frames,
		.fr_frames_number = sizeof( sensor0_dummy_fr_frames ) / sizeof( tframe_t ),
		.callback_fr = callback_fr,
		.ds1_frames = sensor0_dummy_ds1_frames,
		.ds1_frames_number = sizeof( sensor0_dummy_ds1_frames ) / sizeof( tframe_t ),
		.callback_ds1 = callback_ds1,
	},
#endif
} ;
