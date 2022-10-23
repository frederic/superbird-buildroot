/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AML_IPC_ISP_H

#define AML_IPC_ISP_H

#include "aml_ipc_component.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ISP component channel ID
 */
enum AmlISPChannel { AML_ISP_FR, AML_ISP_META, AML_ISP_DS1, AML_ISP_DS2, AML_ISP_SDK_MAX };

struct AmlIPCISPPriv;
struct AmlIPCISP {
    AML_OBJ_EXTENDS(AmlIPCISP, AmlIPCComponent, AmlIPCComponentClass);
    struct AmlIPCISPPriv *priv;
    char *device;
    int active;
    int per_chan_thread;
};
AML_OBJ_DECLARE_TYPEID(AmlIPCISP, AmlIPCComponent, AmlIPCComponentClass);

#define AML_ISP_WIDTH_MIN (128)
#define AML_ISP_WIDTH_MAX (32768)
#define AML_ISP_HEIGHT_MIN (128)
#define AML_ISP_HEIGHT_MAX (32768)
#define AML_ISP_FPS_MIN (1)
#define AML_ISP_FPS_MAX (240)

#define AML_WB_GAIN_AUTO (-1)
#define AML_WB_GAIN_MIN (0)
#define AML_WB_GAIN_MAX (65535)
#define AML_EXPOSURE_AUTO (-1)
#define AML_EXPOSURE_MIN (1)
#define AML_EXPOSURE_MAX (1000)

#define AML_SHARPNESS_MIN (0)
#define AML_SHARPNESS_MAX (255)
#define AML_BRIGHTNESS_MIN (0)
#define AML_BRIGHTNESS_MAX (255)
#define AML_CONTRAST_MIN (0)
#define AML_CONTRAST_MAX (255)
#define AML_HUE_MIN (0)
#define AML_HUE_MAX (360)
#define AML_SATURATION_MIN (0)
#define AML_SATURATION_MAX (255)
#define AML_SENSOR_FPS_MIN (0)
#define AML_SENSOR_FPS_MAX (120)
#define AML_FPS_MIN (0)
#define AML_FPS_MAX (120)
#define AML_COMPENSATION_MIN (0)
#define AML_COMPENSATION_MAX (255)
#define AML_NR_NONE (-1)
#define AML_NR_MIN (0)
#define AML_NR_MAX (255)

/**
 * @brief create isp component
 *
 * @param dev, v4l2 device name, such as "/dev/video0"
 * @param nbuf_fr, configure FR buffer number
 * @param nbuf_ds1, configure DS1 buffer number, 0 to disable DS1
 * @param nbuf_ds2, configure DS2 buffer number, 0 to disable DS2
 *
 * @return
 */
struct AmlIPCISP *aml_ipc_isp_new(const char *dev, int nbuf_fr, int nbuf_ds1, int nbuf_ds2);
AmlStatus aml_ipc_isp_set_capture_format(struct AmlIPCISP *isp, enum AmlISPChannel chan,
                                         struct AmlIPCVideoFormat *fmt);
AmlStatus aml_ipc_isp_set_picture_params(struct AmlIPCISP *isp, enum AmlISPChannel chan,
                                         int brightness, int contrast, int saturation, int hue,
                                         int sharpness);
AmlStatus aml_ipc_isp_set_wdr(struct AmlIPCISP *isp, bool enabled);
AmlStatus aml_ipc_isp_get_wdr(struct AmlIPCISP *isp, bool *enabled);
/**
 * @brief set white balance
 *
 * @param isp
 * @param crgain
 * @param cbgain
 * if both of 'crgain' and 'cbgain' are 'AML_WB_GAIN_AUTO', it will configue auto white balance
 *
 * @return AML_STATUS_OK if success
 */
AmlStatus aml_ipc_isp_set_WB_gain(struct AmlIPCISP *isp, int crgain, int cbgain);
AmlStatus aml_ipc_isp_get_WB_gain(struct AmlIPCISP *isp, int *crgain, int *cbgain);
AmlStatus aml_ipc_isp_set_bw_mode(struct AmlIPCISP *isp, bool bw);
AmlStatus aml_ipc_isp_get_bw_mode(struct AmlIPCISP *isp, int *bw);
AmlStatus aml_ipc_isp_set_exposure(struct AmlIPCISP *isp, int exposure);
AmlStatus aml_ipc_isp_get_exposure(struct AmlIPCISP *isp, int *exposure);
AmlStatus aml_ipc_isp_set_sharpness(struct AmlIPCISP *isp, int sharpness);
AmlStatus aml_ipc_isp_get_sharpness(struct AmlIPCISP *isp, int *sharpness);
AmlStatus aml_ipc_isp_set_brightness(struct AmlIPCISP *isp, int brightness);
AmlStatus aml_ipc_isp_get_brightness(struct AmlIPCISP *isp, int *brightness);
AmlStatus aml_ipc_isp_set_contrast(struct AmlIPCISP *isp, int contrast);
AmlStatus aml_ipc_isp_get_contrast(struct AmlIPCISP *isp, int *contrast);
AmlStatus aml_ipc_isp_set_hue(struct AmlIPCISP *isp, int hue);
AmlStatus aml_ipc_isp_get_hue(struct AmlIPCISP *isp, int *hue);
AmlStatus aml_ipc_isp_set_saturation(struct AmlIPCISP *isp, int saturation);
AmlStatus aml_ipc_isp_get_saturation(struct AmlIPCISP *isp, int *saturation);
AmlStatus aml_ipc_isp_set_hflip(struct AmlIPCISP *isp, bool hflip);
AmlStatus aml_ipc_isp_get_hflip(struct AmlIPCISP *isp, bool *hflip);
AmlStatus aml_ipc_isp_set_vflip(struct AmlIPCISP *isp, bool vflip);
AmlStatus aml_ipc_isp_get_vflip(struct AmlIPCISP *isp, bool *vflip);
AmlStatus aml_ipc_set_sensor_fps(struct AmlIPCISP *isp, int fps);
AmlStatus aml_ipc_get_sensor_fps(struct AmlIPCISP *isp, int *fps);
AmlStatus aml_ipc_isp_set_fps(struct AmlIPCISP *isp, enum AmlISPChannel chan, int fps);
AmlStatus aml_ipc_isp_get_fps(struct AmlIPCISP *isp, enum AmlISPChannel chan, int *fps);
AmlStatus aml_ipc_isp_set_compensation(struct AmlIPCISP *isp, int compensation);
AmlStatus aml_ipc_isp_get_compensation(struct AmlIPCISP *isp, int *compensation);
AmlStatus aml_ipc_isp_set_nr(struct AmlIPCISP *isp, int space_zone, int time_zone);
AmlStatus aml_ipc_isp_get_nr(struct AmlIPCISP *isp, int *space_zone, int *time_zone);
AmlStatus aml_ipc_isp_set_defog(struct AmlIPCISP *isp, int mode, int value);
AmlStatus aml_ipc_isp_get_defog(struct AmlIPCISP *isp, int *mode, int *value);
AmlStatus aml_ipc_isp_set_gain(struct AmlIPCISP *isp, bool bauto, int value);
AmlStatus aml_ipc_isp_get_gain(struct AmlIPCISP *isp, bool *bauto, int *value);

/**
 * @brief enumerate all supported formats
 *
 * @param isp
 * @param chan
 * @param fmt, specify what data to enumerate
 *   1. if fmt is NULL, enumerate all supported pixfmt and resolutions and framerate, very slow
 *   2. if fmt.pixfmt is 0, enumerate all supported pixfmts
 *   3. if fmt.widht/height is 0, enumerate all supported resolutions
 *   4. if fmt.fps is 0, enumerate all supported frame rates
 *
 * examples:
 *   enumerate all supported resolutions of NV12 format:
 *      struct AmlIPCVideoFormat qfmt = {AML_PIXFMT_NV12, 0, 0, 30};
 *   enumerate all supported pixfmt:
 *      struct AmlIPCVideoFormat qfmt = {0, 1920, 1080, 30};
 *   enumerate all supported resolutions and framerates of NV12 format:
 *      struct AmlIPCVideoFormat qfmt = {AML_PIXFMT_NV12, 0, 0, 0};
 *   enumerate all supported framerates of 1280x720 NV12 format:
 *      struct AmlIPCVideoFormat qfmt = {AML_PIXFMT_NV12, 1280, 720, 0};
 *
 * @param param, parameter passed to the callback
 * @param cb, callback function
 *
 * @return
 */
AmlStatus aml_ipc_isp_enumerate_capture_formats(struct AmlIPCISP *isp, enum AmlISPChannel chan,
                                                struct AmlIPCVideoFormat *fmt, void *param,
                                                void (*cb)(struct AmlIPCVideoFormat *, void *));

/**
 * @brief query isp supported resolutions
 *
 * @param isp
 * @param chan
 * @param pixfmt
 * @param numres, input number of res, and output actualy number of resolutions written to res
 * @param res, if not NULL, output the resolutions, only width and height is valid
 *      if NULL, will only output the number of all resolutions in numres
 *
 * @return  AML_STATUS_OK: all resolutions are queried
 *          AML_STATUS_FAIL: other error happens
 */
AmlStatus aml_ipc_isp_query_resolutions(struct AmlIPCISP *isp, enum AmlISPChannel chan,
                                        enum AmlPixelFormat pixfmt, int *numres,
                                        struct AmlIPCVideoFormat *res);
/**
 * @brief set isp device
 *
 * @param isp
 * @param dev
 *
 * @return
 */
AmlStatus aml_ipc_set_device(struct AmlIPCISP *isp, char *dev);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_ISP_H */
