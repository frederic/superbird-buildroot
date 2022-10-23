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

#include <linux/delay.h>
#include "acamera_types.h"
#include "system_spi.h"
#include "system_sensor.h"
#include "sensor_bus_config.h"
#include "acamera_command_api.h"
#include "acamera_sbus_api.h"
#include "acamera_sensor_api.h"
#include "system_timer.h"
#include "sensor_init.h"
#include "SC2232h_seq.h"
#include "SC2232h_config.h"
#include "acamera_math.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"
#include "sensor_bsp_common.h"

#define AGAIN_PRECISION 10
#define NEED_CONFIG_BSP 1   //config bsp by sensor driver owner

static int sen_mode = 0;
static void start_streaming( void *ctx );
static void stop_streaming( void *ctx );

static sensor_mode_t supported_modes[] = {
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 10,
        .exposures = 1,
        .lanes = 2,
        .bps = 390,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_NON,
        .num = 0,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 25 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 10,
        .exposures = 1,
        .lanes = 2,
        .bps = 390,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_NON,
        .num = 0,
    },
};

typedef struct _sensor_context_t {
    uint8_t address; // Sensor address for direct write (not used currently)
    uint8_t seq_width;
    uint8_t streaming_flg;
    uint16_t again[4];
    uint8_t again_delay;
    uint16_t dgain;
    uint8_t dgain_change;
    uint16_t int_time_S;
    uint16_t int_time_M;
    uint16_t int_time_L;
    uint32_t shs1;
    uint32_t shs2;
    uint32_t shs3;
    uint32_t shs1_old;
    uint32_t shs2_old;
    uint32_t rhs1;
    uint32_t rhs2;
    uint32_t again_limit;
    uint32_t dgain_limit;
    uint8_t s_fps;
    uint32_t vmax;
    uint8_t int_cnt;
    uint8_t gain_cnt;
    uint32_t pixel_clock;
    uint16_t max_S;
    uint16_t max_M;
    uint16_t max_L;
    uint16_t frame;
    uint32_t wdr_mode;
    acamera_sbus_t sbus;
    sensor_param_t param;
    void *sbp;
} sensor_context_t;

//-------------------------------------------------------------------------------------
#if SENSOR_BINARY_SEQUENCE
static const char p_sensor_data[] = SENSOR_SC2232H_SEQUENCE_DEFAULT;
static const char p_isp_data[] = SENSOR_SC2232H_ISP_CONTEXT_SEQUENCE;
#else
static const acam_reg_t **p_sensor_data = sc2232h_seq_table;
static const acam_reg_t **p_isp_data = isp_seq_table;

#endif
//--------------------RESET------------------------------------------------------------
static void sensor_hw_reset_enable( void )
{
    system_reset_sensor( 0 );
}

static void sensor_hw_reset_disable( void )
{
    system_reset_sensor( 3 );
}

static const uint16_t GainIndex[80] =
{
    1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472, 1536, 1600, 1664, 1728, 1792, 1856, 1920, 1984,
    2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072, 3200, 3328, 3456, 3584, 3712, 3840, 3968,
    4096, 4352, 4608, 4864, 5120, 5376, 5632, 5888, 6144, 6400, 6656, 6912, 7168, 7424, 7680, 7936,
    8192, 8704, 9216, 9728, 10240, 10752, 11264, 11776, 12288, 12800, 13312, 13824, 14336, 14848, 15360, 15872,
    16384, 17408, 18432, 19456, 20480, 21504, 22528, 23552, 24576, 25600, 26624, 27648, 28672, 29696, 30720, 31744
};

static const uint16_t AGainTable[64] =
{
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f,
    0x0c10, 0x0c11, 0x0c12, 0x0c13, 0x0c14, 0x0c15, 0x0c16, 0x0c17, 0x0c18, 0x0c19, 0x0c1a, 0x0c1b, 0x0c1c, 0x0c1d, 0x0c1e, 0x0c1f,
    0x1c10, 0x1c11, 0x1c12, 0x1c13, 0x1c14, 0x1c15, 0x1c16, 0x1c17, 0x1c18, 0x1c19, 0x1c1a, 0x1c1b, 0x1c1c, 0x1c1d, 0x1c1e, 0x1c1f
};

static const uint16_t DGainTable[80] =
{
    0x0080, 0x0088, 0x0090, 0x0098, 0x00a0, 0x00a8, 0x00b0, 0x00b8, 0x00c0, 0x00c8, 0x00d0, 0x00d8, 0x00e0, 0x00e8, 0x00f0, 0x00f8,
    0x0180, 0x0188, 0x0190, 0x0198, 0x01a0, 0x01a8, 0x01b0, 0x01b8, 0x01c0, 0x01c8, 0x01d0, 0x01d8, 0x01e0, 0x01e8, 0x01f0, 0x01f8,
    0x0380, 0x0388, 0x0390, 0x0398, 0x03a0, 0x03a8, 0x03b0, 0x03b8, 0x03c0, 0x03c8, 0x03d0, 0x03d8, 0x03e0, 0x03e8, 0x03f0, 0x03f8,
    0x0780, 0x0788, 0x0790, 0x0798, 0x07a0, 0x07a8, 0x07b0, 0x07b8, 0x07c0, 0x07c8, 0x07d0, 0x07d8, 0x07e0, 0x07e8, 0x07f0, 0x07f8,
    0x0f80, 0x0f88, 0x0f90, 0x0f98, 0x0fa0, 0x0fa8, 0x0fb0, 0x0fb8, 0x0fc0, 0x0fc8, 0x0fd0, 0x0fd8, 0x0fe0, 0x0fe8, 0x0ff0, 0x0ff8
};

//-------------------------------------------------------------------------------------
static int32_t sensor_alloc_analog_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;
    uint16_t again_sc = 1024;
    uint16_t i;
    uint32_t again = acamera_math_exp2( gain, LOG2_GAIN_SHIFT, AGAIN_PRECISION );

    if ( again >= GainIndex[63]) {
        again = GainIndex[63];
        again_sc = AGainTable[63];
    }

    for (i = 1; i < 64; i++)
    {
        if (again < GainIndex[i]) {
            again = GainIndex[i - 1];
            again_sc = AGainTable[i - 1];
            break;
        }
    }

    if ( p_ctx->again[0] != again_sc ) {
        p_ctx->gain_cnt = p_ctx->again_delay + 1;
        p_ctx->again[0] = again_sc;
    }

    return acamera_log2_fixed_to_fixed( again, AGAIN_PRECISION, LOG2_GAIN_SHIFT );
}

static int32_t sensor_alloc_digital_gain( void *ctx, int32_t gain )
{
    return 0;
}

static void sensor_alloc_integration_time( void *ctx, uint16_t *int_time_S, uint16_t *int_time_M, uint16_t *int_time_L )
{
    sensor_context_t *p_ctx = ctx;

    if ( *int_time_S > p_ctx->param.integration_time_max ) *int_time_S = p_ctx->param.integration_time_max;
    if ( *int_time_S < p_ctx->param.integration_time_min ) *int_time_S = p_ctx->param.integration_time_min;
    if ( p_ctx->int_time_S != *int_time_S ) {
        p_ctx->int_cnt = 2;
        p_ctx->int_time_S = *int_time_S;
    }
}

static int32_t sensor_ir_cut_set( void *ctx, int32_t ir_cut_state )
{
    sensor_context_t *t_ctx = ctx;
    int ret;
    sensor_bringup_t* sensor_bp = t_ctx->sbp;

    LOG( LOG_ERR, "ir_cut_state = %d", ir_cut_state);
    LOG( LOG_INFO, "entry ir cut" );

    //ir_cut_GPIOZ_7 =1 && ir_cut_GPIOZ_11=0, open ir cut
    //ir_cut_GPIOZ_7 =0 && ir_cut_GPIOZ_11=1, close ir cut
    //ir_cut_srate, 2: no operation

   if (sensor_bp->ir_gname[0] <= 0 && sensor_bp->ir_gname[1] <= 0) {
       pr_err("get gpio id fail\n");
       return 0;
   }

   if (ir_cut_state == 0)
        {
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[1], 1);
            if (ret < 0 )
            pr_err("set power fail\n");

            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 0);
            if (ret < 0 )
            pr_err("set power fail\n");

            mdelay(500);
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 1);
            if (ret < 0 )
            pr_err("set power fail\n");
        }
    else if(ir_cut_state == 1)
        {
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[1], 0);
            if (ret < 0 )
            pr_err("set power fail\n");

            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 1);
            if (ret < 0 )
            pr_err("set power fail\n");

            mdelay(500);
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[1], 1);
            if (ret < 0 )
            pr_err("set power fail\n");
       }
    else if(ir_cut_state == 2)
        return 0;
    else
        LOG( LOG_ERR, "sensor ir cut set failed" );

    LOG( LOG_INFO, "exit ir cut" );
    return 0;
}

static void sensor_update( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;

    if ( p_ctx->int_cnt || p_ctx->gain_cnt ) {
        // ---------- Analog Gain -------------
        if ( p_ctx->gain_cnt ) {
            acamera_sbus_write_u8( p_sbus, 0x3812, 0x00 ); //group hold
            if ( p_ctx->again[p_ctx->again_delay] < 0x001f) {  // again<2x
                acamera_sbus_write_u8( p_sbus, 0x3301, 0x12);
                acamera_sbus_write_u8( p_sbus, 0x3632, 0x08);
            }
            else if ( p_ctx->again[p_ctx->again_delay] < 0x041f) {  // 2x<=again<4x
                acamera_sbus_write_u8( p_sbus, 0x3301, 0x20);
                acamera_sbus_write_u8( p_sbus, 0x3632, 0x08);
            }
            else if ( p_ctx->again[p_ctx->again_delay] < 0x0c1f) {  // 4x<=again<8x
                acamera_sbus_write_u8( p_sbus, 0x3301, 0x28);
                acamera_sbus_write_u8( p_sbus, 0x3632, 0x08);
            }
            else if ( p_ctx->again[p_ctx->again_delay] < 0x1c1f) {  // 8x<=again<15.5x
                acamera_sbus_write_u8( p_sbus, 0x3301, 0x64);
                acamera_sbus_write_u8( p_sbus, 0x3632, 0x08);
            }
            else {  // again >= 15.5x
                acamera_sbus_write_u8( p_sbus, 0x3301, 0x64);
                acamera_sbus_write_u8( p_sbus, 0x3632, 0x48);
            }
            acamera_sbus_write_u8( p_sbus, 0x3812, 0x30 );//group hold
            acamera_sbus_write_u8( p_sbus, 0x3e08, (p_ctx->again[p_ctx->again_delay]>> 8 ) |0x03 );
            acamera_sbus_write_u8( p_sbus, 0x3e09, (p_ctx->again[p_ctx->again_delay]>> 0 ) & 0xFF );
            p_ctx->gain_cnt--;
        }

        // -------- Integration Time ----------
        if ( p_ctx->int_cnt ) {
            acamera_sbus_write_u8( p_sbus, 0x3e00, ( p_ctx->int_time_S >> 12 ) & 0x0F );
            acamera_sbus_write_u8( p_sbus, 0x3e01, ( p_ctx->int_time_S >> 4 ) & 0xFF );
            acamera_sbus_write_u8( p_sbus, 0x3e02,( ( p_ctx->int_time_S >> 0 ) & 0x0F ) << 4);
            if ( p_ctx->int_time_S < 0x250 ) {
                acamera_sbus_write_u8( p_sbus, 0x3314, 0x14 );
            } else if ( p_ctx->int_time_S > 0x450 ) {
                acamera_sbus_write_u8( p_sbus, 0x3314, 0x04 );
            }
            p_ctx->int_cnt--;
        }
    }

    p_ctx->again[3] = p_ctx->again[2];
    p_ctx->again[2] = p_ctx->again[1];
    p_ctx->again[1] = p_ctx->again[0];
}

static uint16_t sensor_get_id( void *ctx )
{
    /* return that sensor id register does not exist */

	sensor_context_t *p_ctx = ctx;
	uint16_t sensor_id = 0;

	sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x3107) << 8;
	sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x3108);

        if (sensor_id != SENSOR_CHIP_ID) {
            LOG(LOG_CRIT, "%s: Failed to read sensor id\n", __func__);
            return 0xFFFF;
        }

	LOG(LOG_CRIT, "%s: success to read sensor id: 0x%x\n", __func__, sensor_id);
        return sensor_id;
}

static void sensor_set_iface(sensor_mode_t *mode)
{
    am_mipi_info_t mipi_info;
    struct am_adap_info info;

    if (mode == NULL) {
        LOG(LOG_ERR, "Error input param\n");
        return;
    }

    memset(&mipi_info, 0, sizeof(mipi_info));
    memset(&info, 0, sizeof(struct am_adap_info));
    mipi_info.fte1_flag = get_fte1_flag();
    mipi_info.lanes = mode->lanes;
    mipi_info.ui_val = 1000 / mode->bps;

    if ((1000 % mode->bps) != 0)
        mipi_info.ui_val += 1;

    am_mipi_init(&mipi_info);

    switch (mode->bits) {
    case 10:
        info.fmt = AM_RAW10;
        break;
    case 12:
        info.fmt = AM_RAW12;
        break;
    default :
        info.fmt = AM_RAW10;
        break;
    }

    info.img.width = mode->resolution.width;
    info.img.height = mode->resolution.height;
    info.offset.offset_x = 0;
    info.path = PATH0;
    if (mode->wdr_mode == WDR_MODE_FS_LIN) {
        info.mode = DOL_MODE;
        info.type = mode->dol_type;
    } else
        info.mode = DIR_MODE;
    am_adap_set_info(&info);
    am_adap_init();
    am_adap_start(0);
}

static void sensor_set_mode( void *ctx, uint8_t mode )
{
    sensor_context_t *p_ctx = ctx;
    sensor_param_t *param = &p_ctx->param;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    uint8_t setting_num = 0;
    uint16_t s_id = 0xff;
    sen_mode = mode;

    sensor_hw_reset_enable();
    system_timer_usleep( 10000 );
    sensor_hw_reset_disable();
    system_timer_usleep( 10000 );

    setting_num = param->modes_table[mode].num;

    s_id = sensor_get_id(ctx);
    if (s_id != SENSOR_CHIP_ID)
    {
        LOG(LOG_INFO, "%s: check sensor failed\n", __func__);
        return;
    }

    switch ( param->modes_table[mode].wdr_mode ) {
    case WDR_MODE_LINEAR:
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
        p_ctx->s_fps = param->modes_table[mode].fps;
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        break;
    case WDR_MODE_FS_LIN:
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
        p_ctx->s_fps = 30;
        break;
    default:
        LOG(LOG_ERR, "Invalide wdr mode. Returning!");
        return;
    }

    if ( param->modes_table[mode].fps == 25 * 256 ) {
        acamera_sbus_write_u8( p_sbus, 0x320e, 0x05 );
        acamera_sbus_write_u8( p_sbus, 0x320f, 0xdc );
        p_ctx->s_fps = 25;
        p_ctx->vmax = 1500;
    }else  if ( param->modes_table[mode].fps == 30 * 256 ) {
        p_ctx->s_fps = 30;
        p_ctx->vmax = 1250;
    }
    else {
        p_ctx->vmax = 1250;
    }

    param->active.width = param->modes_table[mode].resolution.width;
    param->active.height = param->modes_table[mode].resolution.height;

    param->total.width =(( (uint16_t)acamera_sbus_read_u8( p_sbus, 0x320c ) << 8 ) |acamera_sbus_read_u8( p_sbus, 0x320d )) >> 1;
    param->lines_per_second = p_ctx->pixel_clock / param->total.width;
    param->total.height = (uint16_t)p_ctx->vmax;
    param->pixels_per_line = param->total.width;
    param->integration_time_min = SENSOR_MIN_INTEGRATION_TIME;

    param->integration_time_limit = 2 * p_ctx->vmax - 4;
    param->integration_time_max = 2* p_ctx->vmax - 4;

    param->sensor_exp_number = param->modes_table[mode].exposures;
    param->mode = mode;
    p_ctx->wdr_mode = param->modes_table[mode].wdr_mode;
    param->bayer = param->modes_table[mode].bayer;

    sensor_set_iface(&param->modes_table[mode]);

    LOG( LOG_CRIT, "Mode %d, Setting num: %d, RES:%dx%d\n", mode, setting_num,
                (int)param->active.width, (int)param->active.height );
}

static const sensor_param_t *sensor_get_parameters( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    return (const sensor_param_t *)&p_ctx->param;
}

static void sensor_disable_isp( void *ctx )
{
}

static uint32_t read_register( void *ctx, uint32_t address )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    return acamera_sbus_read_u8( p_sbus, address );
}

static void write_register( void *ctx, uint32_t address, uint32_t data )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    acamera_sbus_write_u8( p_sbus, address, data );
}

static void stop_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    p_ctx->streaming_flg = 0;

    acamera_sbus_write_u8( p_sbus, 0x0100, 0x00 );

    reset_sensor_bus_counter();
    am_adap_deinit();
    am_mipi_deinit();
}

static void start_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    sensor_param_t *param = &p_ctx->param;
    sensor_set_iface(&param->modes_table[sen_mode]);
    p_ctx->streaming_flg = 1;
    acamera_sbus_write_u8( p_sbus, 0x0100, 0x01 );

}

static void sensor_test_pattern( void *ctx, uint8_t mode )
{

}

#if PLATFORM_C308X
static uint32_t write1_reg(unsigned long addr, uint32_t val)
{
    void __iomem *io_addr;
    io_addr = ioremap_nocache(addr, 8);
    if (io_addr == NULL) {
        LOG(LOG_ERR, "%s: Failed to ioremap addr\n", __func__);
        return -1;
    }
    __raw_writel(val, io_addr);
    iounmap(io_addr);
    return 0;
}
#endif

void sensor_deinit_sc2232h( void *ctx )
{
    sensor_context_t *t_ctx = ctx;

    reset_sensor_bus_counter();
    am_adap_deinit();
    am_mipi_deinit();

    acamera_sbus_deinit(&t_ctx->sbus, sbus_i2c);

    if (t_ctx != NULL && t_ctx->sbp != NULL)
        clk_am_disable(t_ctx->sbp);
}
//--------------------Initialization------------------------------------------------------------
void sensor_init_sc2232h( void **ctx, sensor_control_t *ctrl, void* sbp)
{
    // Local sensor data structure
    static sensor_context_t s_ctx;
    int ret;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;
    *ctx = &s_ctx;
    s_ctx.sbp = sbp;

#if PLATFORM_G12B
#if NEED_CONFIG_BSP
    ret = pwr_am_enable(sensor_bp, "power-enable", 0);
    if (ret < 0 )
        pr_err("set power fail\n");
    udelay(30);
#endif

    ret = clk_am_enable(sensor_bp, "g12a_24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
#elif PLATFORM_C308X
    ret = pwr_am_enable(sensor_bp, "power-enable", 0);
    if (ret < 0 )
        pr_err("set power fail\n");
    mdelay(50);
    ret = clk_am_enable(sensor_bp, "g12a_24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
    write1_reg(0xfe000428, 0x11400400);
#endif
    udelay(30);

#if NEED_CONFIG_BSP
    ret = reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_info("set reset fail\n");
#endif

    s_ctx.sbus.mask =
        SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS |
        SBUS_MASK_ADDR_SWAP_BYTES;
    s_ctx.sbus.control = 0;
    s_ctx.sbus.bus = 1;
    s_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &s_ctx.sbus, sbus_i2c );

    sensor_get_id(&s_ctx);

    // Initial local parameters
    s_ctx.address = SENSOR_DEV_ADDRESS;
    s_ctx.seq_width = 1;
    s_ctx.streaming_flg = 0;
    s_ctx.again[0] = 0;
    s_ctx.again[1] = 0;
    s_ctx.again[2] = 0;
    s_ctx.again[3] = 0;
    s_ctx.again_limit = 15872;
    s_ctx.pixel_clock = 78000000;

    s_ctx.param.again_accuracy = 1 << LOG2_GAIN_SHIFT;
    s_ctx.param.sensor_exp_number = 1;
    s_ctx.param.again_log2_max = 16 << LOG2_GAIN_SHIFT;
    s_ctx.param.dgain_log2_max = 0;
    s_ctx.param.integration_time_apply_delay = 2;
    s_ctx.param.isp_exposure_channel_delay = 0;
    s_ctx.param.modes_table = supported_modes;
    s_ctx.param.modes_num = array_size_s( supported_modes );
    s_ctx.param.sensor_ctx = &s_ctx;
    s_ctx.param.isp_context_seq.sequence = p_isp_data;
    s_ctx.param.isp_context_seq.seq_num= SENSOR_SC2232H_CONTEXT_SEQ;
    s_ctx.param.isp_context_seq.seq_table_max = array_size_s( isp_seq_table );

    ctrl->alloc_analog_gain = sensor_alloc_analog_gain;
    ctrl->alloc_digital_gain = sensor_alloc_digital_gain;
    ctrl->alloc_integration_time = sensor_alloc_integration_time;
    ctrl->ir_cut_set= sensor_ir_cut_set;
    ctrl->sensor_update = sensor_update;
    ctrl->set_mode = sensor_set_mode;
    ctrl->get_id = sensor_get_id;
    ctrl->get_parameters = sensor_get_parameters;
    ctrl->disable_sensor_isp = sensor_disable_isp;
    ctrl->read_sensor_register = read_register;
    ctrl->write_sensor_register = write_register;
    ctrl->start_streaming = start_streaming;
    ctrl->stop_streaming = stop_streaming;
    ctrl->sensor_test_pattern = sensor_test_pattern;

    // Reset sensor during initialization
    sensor_hw_reset_enable();
    system_timer_usleep( 1000 ); // reset at least 1 ms
    sensor_hw_reset_disable();
    system_timer_usleep( 1000 );

    LOG(LOG_ERR, "%s: Success subdev init\n", __func__);
}

int sensor_detect_sc2232h( void* sbp)
{
    static sensor_context_t s_ctx;
    int ret = 0;
    s_ctx.sbp = sbp;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;
#if PLATFORM_G12B
    ret = clk_am_enable(sensor_bp, "g12a_24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
#elif PLATFORM_C308X
    write1_reg(0xfe000428, 0x11400400);
#endif

#if NEED_CONFIG_BSP
    ret = reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_info("set reset fail\n");
#endif

    s_ctx.sbus.mask = SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS | SBUS_MASK_ADDR_SWAP_BYTES;
    s_ctx.sbus.control = 0;
    s_ctx.sbus.bus = 0;
    s_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &s_ctx.sbus, sbus_i2c );

    ret = 0;
    if (sensor_get_id(&s_ctx) == 0xFFFF)
        ret = -1;
    else
        pr_info("sensor_detect_sc2232h:%d\n", ret);

    acamera_sbus_deinit(&s_ctx.sbus,  sbus_i2c);
    reset_am_disable(sensor_bp);
    return ret;
}

//*************************************************************************************
