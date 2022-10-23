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

#include "acamera_logger.h"
#include "fw_cmn_ctrl.h"

#include <esp_vfs.h>
#include <esp_vfs_fat.h>

// the settings for each firmware context were pre-generated and
// saved in the header file. They are given as a reference and should be changed
// according to the customer needs.
#include "runtime_initialization_settings.h"

int32_t isp_mount_sdmmc(void);
int32_t isp_mount_sdmmc(void)
{
	int32_t ret = 0;
	esp_vfs_fat_mount_config_t mount_cfg;

	mount_cfg.format_if_mount_failed = 1;
	mount_cfg.max_files = 5;

	ret = mount_cfg.max_files; // esp_vfs_fat_sdmmc_mount("/sdmmc", &mount_cfg);
	if (ret != 0) {
		LOG(LOG_ERR, "Failed to mount sdmmc");
	}

	return ret;
}

void isp_unmount_sdmmc(void);
void isp_unmount_sdmmc(void)
{
	//esp_vfs_fat_sdmmc_unmount();
}

static int local_sprintf(char *buf, int size, const char *fmt, ...)
{
	va_list args;
	int val = 0;

	va_start(args, fmt);

	val = sPrintf_ext(buf, size, fmt, args);

	va_end(args);

	return val;
}

void isp_save_image(char *buf, uint32_t size, uint32_t num, uint8_t type);
void isp_save_image(char *buf, uint32_t size, uint32_t num, uint8_t type)
{
	int fd = -1;
	char f_buf[24] = {'\0'};

	if (num % 100 != 0)
		return;

	local_sprintf(f_buf, sizeof(f_buf), "/sdmmc/%x%x.raw", num, type);

	fd = open(f_buf, O_RDWR, O_CREAT | O_TRUNC);
	if (fd < 0) {
		LOG(LOG_ERR, "Failed to open %s, fd %d", f_buf, fd);
		return;
	}

	write(fd, buf, size);

	close(fd);
}

uint32_t i_start = 0;
uint32_t i_end = 0;
uint32_t c_end = 0;
uint32_t f_end = 0;
uint32_t f_int = 0;

void custom_initialization( uint32_t ctx_num )
{
    UNUSED(ctx_num);
}

// Callback from FR output pipe
void callback_fr( uint32_t ctx_num, tframe_t *tframe, const metadata_t *metadata )
{
    UNUSED(ctx_num);
    //UNUSED(tframe);
    //UNUSED(metadata);
    //isp_save_image((char *)tframe->primary.address, tframe->primary.size, metadata->frame_id, 0);

    if (metadata->frame_id <= 7) {
        LOG(LOG_ERR, "frame id %u: addr 0x%08lx size 0x%x",
                metadata->frame_id,
                tframe->primary.address, tframe->primary.size);
        //if(metadata->frame_id == 7)
        //	fw_sensor_stream_off();
    }
}

// Callback from DS1 output pipe
void callback_ds1( uint32_t ctx_num, tframe_t *tframe, const metadata_t *metadata )
{
    UNUSED(ctx_num);
    UNUSED(tframe);
    //UNUSED(metadata);
    //system_flush_dcache(tframe->primary.address, tframe->primary.size);
    //isp_save_image((char *)tframe->primary.address,
    //          tframe->primary.size + tframe->secondary.size, metadata->frame_id, 1);

    if (metadata->frame_id < 7) {
        //LOG(LOG_ERR, "frame id %u: addr 0x%08lx p_size 0x%x s_size 0x%x",
         //       metadata->frame_id,
         //       tframe->primary.address, tframe->primary.size, tframe->secondary.size);

        if (metadata->frame_id == 2)
            system_get_utime(&f_end);
        LOG(LOG_ERR, "LIKE:id %d: i_start %u, i_end %u, c_end %u, f_int %u, f_end %u",
                metadata->frame_id, i_start, i_end, c_end, f_int, f_end);
        LOG(LOG_ERR, "LIKE:id %d:init %u us, cfg %u us, on-cb %u us, sum %u us",
                metadata->frame_id, i_end - i_start, c_end - i_end, f_end - c_end, f_end - i_start);

        LOG(LOG_ERR, "frame id %u: addr 0x%08lx p_size 0x%x s_size 0x%x",
                metadata->frame_id,
                tframe->primary.address, tframe->primary.size, tframe->secondary.size);

        //if(metadata->frame_id == 2)
        //fw_sensor_stream_off();
    }
}

// Callback from DS2 output pipe
void callback_ds2( uint32_t ctx_num, tframe_t *tframe, const metadata_t *metadata )
{
    UNUSED(ctx_num);
    UNUSED(tframe);
    UNUSED(metadata);
}

#define mainCHECK_LIKE_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
void vIspTask(void *arg);
/*
static void delay_init_task(void *arg)
{
    UNUSED(arg);
    int32_t result = -1;

    while (1) {
        if (result == 0)
            break;

        system_mdelay(5000);

        result = fw_init(settings);
        if (result != 0)
            LOG(LOG_ERR, "Failed to init fw");
        else
            vIspTask(NULL);
    }
    vTaskDelete(NULL);
}
*/
static int isp_fw_init(void)
{
    int32_t result = -1;

    //xTaskCreate(delay_init_task, "LikeInitTask", 2048, NULL, mainCHECK_LIKE_TASK_PRIORITY - 1, NULL);

    //return 0;

    system_get_utime(&i_start);
    result = fw_init(settings);
    if (result != 0)
        LOG(LOG_ERR, "Failed to init fw");
    else {
        system_get_utime(&i_end);
        vIspTask(NULL);
        system_get_utime(&c_end);
    }

    return result;
}

void isp_fw_deinit(void);
void isp_fw_deinit(void)
{
    fw_deinit();

    //isp_unmount_sdmmc();
}

static int isp_cfg_sensor_mode(void)
{
    struct sensor_mode_info m_info;

    m_info.mode = WDR_MODE_LINEAR;// WDR_MODE_FS_LIN;
    m_info.width = 1920;
    m_info.height = 1080;
    m_info.fps = 30 * 256;
    m_info.exps = 1;// 2;
    fw_set_sensor_mode(&m_info);

    return 0;
}

void isp_alg_prst_cfg(void);
void isp_alg_prst_cfg(void)
{
    struct alg_prst_param a_prst;

    a_prst.ae_prst.integrator = 67500000;

    a_prst.awb_prst.red_wb_log2 = 0x3911d;
    a_prst.awb_prst.blue_wb_log2 = 0x29300;
    a_prst.awb_prst.prst_cnt = 16;

    a_prst.gamma_prst.apply_gain = 262;
    a_prst.gamma_prst.auto_level_offset = 27;
    a_prst.gamma_prst.prst_cnt = 31;

    a_prst.iridix_prst.strength_target = 13700;
    a_prst.iridix_prst.iridix_contrast = 2464;
    a_prst.iridix_prst.dark_enh = 400;
    a_prst.iridix_prst.iridix_global_DG = 256;
    a_prst.iridix_prst.prst_cnt = 91;

    a_prst.alg_prst_enable = 0;

    fw_alg_prst_cfg(&a_prst);
}

int isp_fr_cfg( void );
int isp_fr_cfg( void )
{
    int32_t ret = -1;
    struct module_cfg_info c_info;

    c_info.p_type = dma_fr;
    c_info.x_off = 480;
    c_info.y_off = 360;
    c_info.width = 384;
    c_info.height = 240;
    c_info.fmt = NV12_GREY;
    c_info.auto_enable = 0;

    ret = fw_path_cfg(&c_info);

    return ret;
}

int isp_ds_cfg( void );
int isp_ds_cfg( void )
{
    int32_t ret = -1;
    struct module_cfg_info c_info;

    c_info.p_type = dma_ds1;
    c_info.x_off = 0;
    c_info.y_off = 0;
    c_info.width = 1920;
    c_info.height = 1080;
    c_info.s_width = 384;
    c_info.s_height = 240;
    c_info.fmt = NV12_GREY;
    c_info.auto_enable = 0;
	c_info.auto_cap_base = 0x40000000;
	c_info.auto_cap_cnt = 100;

    ret = fw_path_cfg(&c_info);

	fw_auto_cap_cfg(&c_info);

    return ret;
}

void isp_ds_auto_ping_pong(void);
void isp_ds_auto_ping_pong(void)
{
    struct module_cfg_info c_info;

    c_info.p_type = dma_ds1;

    fw_auto_cap_ping_pong(&c_info);
}

void isp_ds_auto_start(void);
void isp_ds_auto_start(void)
{
    struct module_cfg_info c_info;

    c_info.p_type = dma_ds1;

    fw_auto_cap_start(&c_info);
}

void isp_ds_auto_stop(void);
void isp_ds_auto_stop(void)
{
    struct module_cfg_info c_info;

    c_info.p_type = dma_ds1;

    fw_auto_cap_stop(&c_info);
}

void isp_ds_auto_cnt(void);
void isp_ds_auto_cnt(void)
{
    uint32_t f_cnt = 0;
    struct module_cfg_info c_info;

    c_info.p_type = dma_ds1;

    f_cnt = fw_auto_cap_cnt(&c_info);

    LOG(LOG_ERR, "frame cnt %d", f_cnt);
}

void isp_ds_auto_frm_info(void);
void isp_ds_auto_frm_info(void)
{
	struct frame_info f_info;

	f_info.p_type = dma_ds1;
	f_info.idx = 50;
	fw_auto_cap_frm_info(&f_info);

	LOG(LOG_ERR, "addr 0x%lx, size %d, fmt %d",
			f_info.addr, f_info.size, f_info.fmt);
}

void vIspInit(void);
void vIspInit(void)
{
    isp_fw_init();
}

extern size_t xPortGetFreeHeapSize( void );

//void vIspTask(void *arg);
void vIspTask(void *arg)
{
    UNUSED(arg);
    //uint32_t total = configTOTAL_HEAP_SIZE;
    //uint32_t freesz = xPortGetFreeHeapSize();
    //uint32_t used = total - freesz;

    isp_cfg_sensor_mode();
    isp_alg_prst_cfg();
    //isp_fr_cfg();
    isp_ds_cfg();
    fw_sensor_stream_on();

    //LOG(LOG_ERR, "Total %u, free %u, used %u", total, freesz, used);
}

