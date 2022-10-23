/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifndef __DRM_HELP_CLIENT_H
#define __DRM_HELP_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <json.h>
#include <inttypes.h>

#if DEBUG
#include <time.h>
#define COLOR_F (getpid()%6)+1
#define COLOR_B 8
#define DEBUG_INFO(fmt, arg...) do {fprintf(stderr, "[Debug:PID[%5d]:%8ld]\033[3%d;4%dm " fmt "\033[0m [in %s   %s:%d]\n",getpid(), time(NULL), COLOR_F, COLOR_B, ##arg, __FILE__, __func__, __LINE__);}while(0)
#else
#define DEBUG_INFO(fmt, arg...)
#endif //DEBUG
#ifndef DRM_DISPLAY_MODE_LEN
#define DRM_DISPLAY_MODE_LEN 32
#endif

    typedef enum _opt_type {
        OPT_TYPE_NULL = 0,
        OPT_TYPE_DOUBLE,
        OPT_TYPE_INT,
        OPT_TYPE_STRING,
        OPT_TYPE_JSON_OBJECT,
        OPT_TYPE_MAX,
    } opt_type;

    typedef void drm_client_ctx;
    typedef struct _drm_output_mode {
        char name[DRM_DISPLAY_MODE_LEN];
        uint32_t width;
        uint32_t height;
        uint32_t refresh;
    } drm_output_mode;

    typedef struct _drm_output_rect {
        int32_t x,y;
        uint32_t w,h;
    } drm_output_rect;

    typedef struct _drm_output_mode_list {
        drm_output_mode mode;
        struct _drm_output_mode_list* next;
    } drm_output_mode_list;

    typedef struct _drm_connection_list {
        int id;
        int type;
        int encoder_id;
        int mmWidth;
        int mmHeight;
        int count_encoders;
        int count_modes;
        drm_output_mode_list* modes;
        struct _drm_connection_list* next;
    } drm_connection_list;

    /* connect with display server which under compositor */
    drm_client_ctx* drm_help_client_create(void);

    /* disconect with display server */
    void drm_help_client_destory(drm_client_ctx* client);

    /* get connector support display-mode list */
    drm_connection_list* drm_help_client_get_connection(drm_client_ctx* client);

    /* get physical display mode, which include display-mode name resolution and refresh rate. */
    bool drm_help_client_get_display_mode(drm_client_ctx* client, drm_output_mode* mode, int connection_id);

    /* get system logic ui rect, which aware by app. */
    bool drm_help_client_get_ui_rect(drm_client_ctx* client, drm_output_rect* rect, int connection_id);

    /* set system logic ui rect, current not work well */
    void drm_help_client_set_ui_rect(drm_client_ctx* client, drm_output_rect* rect, int connection_id);

    /* set connector display mode
     * support mode string like: "%dx%m[^@]@%d %u:%u": "1920x1080i@60 16:9"
     */
    void drm_help_client_switch_mode_s(drm_client_ctx* client, const char* mode_s, int connection_id);

    /* set connector's drm properties*/
    void drm_help_client_set_connector_properties(drm_client_ctx* client, const char* name, uint64_t value);

    void free_modes(drm_output_mode_list* data);

    void free_connection_list(drm_connection_list* data);

    void send_cmd(drm_client_ctx* client, const char* cmd, void* opt, opt_type type);

    json_object* send_cmd_sync(drm_client_ctx* client, const char* cmd, void* opt, opt_type type);

#ifdef __cplusplus
}
#endif //__cplusplus
#endif //__DRM_HELP_CLIENT_H
