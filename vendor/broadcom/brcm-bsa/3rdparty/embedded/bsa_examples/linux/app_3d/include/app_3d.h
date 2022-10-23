/*****************************************************************************
**
**  Name:           app_3d.h
**
**  Description:    Bluetooth 3D include file
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_3D_H
#define APP_3D_H

#include "bsa_api.h"

/* Dual View bit definition */
#ifndef APP_3D_DUAL_VIEW
#define APP_3D_DUAL_VIEW        0x01
#endif

/* Active Screen bit definition */
#ifndef APP_3D_ACTIVE_SCREEN
#define APP_3D_ACTIVE_SCREEN    0x02
#endif

/* 120/240 Hz bit definition */
#ifndef APP_3D_120_240_HZ
#define APP_3D_120_240_HZ       0x04
#endif

/* Quad View bit definition */
#ifndef APP_3D_QUAD_VIEW
#define APP_3D_QUAD_VIEW        0x08
#endif

/* 3D ViewMode mask */
#define APP_3D_VIEW_MODE_MASK   (APP_3D_DUAL_VIEW  | \
                                 APP_3D_120_240_HZ | \
                                 APP_3D_QUAD_VIEW)

/* Dual View Broadcast Audio bit On */
#ifndef APP_3D_AUDIO_ON
#define APP_3D_AUDIO_ON         0x40
#endif

/* Dual View Broadcast Audio bit used */
#ifndef APP_3D_AUDIO_USED
#define APP_3D_AUDIO_USED       0x80
#endif

/* Dual View Broadcast Audio bits mask */
#ifndef APP_3D_AUDIO_MASK
#define APP_3D_AUDIO_MASK       (APP_3D_AUDIO_USED | APP_3D_AUDIO_ON)
#endif

typedef enum
{
    APP_3D_VIEW_3D = 0,
    APP_3D_VIEW_DUAL_VIEW,
    APP_3D_VIEW_2D_2D,
    APP_3D_VIEW_2D_3D,
    APP_3D_VIEW_3D_2D,
    APP_3D_VIEW_3D_3D,
    APP_3D_VIEW_QUAL_VIEW,
} tAPP_3D_VIEW_MODE;


typedef struct
{
    UINT16 vsync_period;    /* Period of the VSync signal (3D) */
    UINT16 vsync_delay;     /* Delay entered by user */
    UINT8 dual_view;        /* Dual View entered by user */
    tBSA_DM_MASK brcm_mask; /* Broadcom Specific Mask */
    tAPP_3D_VIEW_MODE view_mode; /* Viewmode (3D, DualView, 2D/D2, 2D/3D, etc.) */
} tAPP_3D_CB;

/*
 * Global Variables
 */

extern tAPP_3D_CB app_3d_cb;

/*******************************************************************************
 **
 ** Function         app_3d_init
 **
 ** Description      3D initialization function
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_3d_init(void);

/*******************************************************************************
 **
 ** Function         app_3d_start
 **
 ** Description      3D Start function
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_3d_start(void);

/*******************************************************************************
 **
 ** Function         app_3d_exit
 **
 ** Description      3D Exit function
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_3d_exit(void);

/*******************************************************************************
 **
 ** Function         app_3d_toggle_proximity_pairing
 **
 ** Description      This function is used to toggle ProximityPairing
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_3d_toggle_proximity_pairing(void);

/*******************************************************************************
 **
 ** Function         app_3d_toggle_showroom
 **
 ** Description      This function is used to toggle Showroom
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_3d_toggle_showroom(void);

/*******************************************************************************
 **
 ** Function         app_3d_toggle_rc_pairable
 **
 ** Description      This function is used to toggle RC Pairable
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_3d_toggle_rc_pairable(void);

/*******************************************************************************
 **
 ** Function         app_3d_toggle_rc_associated
 **
 ** Description      This function is used to toggle RC Associated
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_3d_toggle_rc_associated(void);

/*******************************************************************************
 **
 ** Function        app_3d_multi_view_control
 **
 ** Description     This function is used Control Multi View
 **
 ** Parameters      None
 **
 ** Returns         int
 **
 *******************************************************************************/
int app_3d_dualview_set(void);

/*******************************************************************************
 **
 ** Function        app_3d_dualview_get
 **
 ** Description     This function is used to Get the current DualView value
 **
 ** Parameters      None
 **
 ** Returns         void
 **
 *******************************************************************************/
UINT8 app_3d_dualview_get(void);

/*******************************************************************************
 **
 ** Function        app_3d_set_audio_bits
 **
 ** Description     This function is used to set the Broadcast Audio Bits
 **
 ** Parameters      Audio bit mask
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_3d_set_audio_bits(UINT8 audio_bit_mask);

/*******************************************************************************
 **
 ** Function         app_3d_set_idle
 **
 ** Description      Function example to set DTV in Idle mode
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_3d_set_idle(void);

/*******************************************************************************
 **
 ** Function         app_3d_set_master
 **
 ** Description      Function example toset DTV in Master mode
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_3d_set_master(void);

/*******************************************************************************
 **
 ** Function         app_3d_set_slave
 **
 ** Description      Function example toset DTV in Master mode
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_3d_set_slave(void);

/*******************************************************************************
 **
 ** Function         app_3d_set_3d_offset_delay
 **
 ** Description      Function example to configure 3DTV Broadcasted offsets and delay
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_3d_set_3d_offset_delay(BOOLEAN use_default);

/*******************************************************************************
 **
 ** Function        app_3d_send_3ds_data
 **
 ** Description     Function example to configure send 3D data (offsets and delay)
 **
 ** Parameters      use_default: indicates if default offsets must be used
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_send_3ds_data(BOOLEAN use_default);

/*******************************************************************************
 **
 ** Function        app_3d_get_offsets
 **
 ** Description     Compute offsets from 3D Mode and period
 **
 ** Parameters      mode: 3D mode (3D, DualView, 2D/2D, etc)
 **                 vsync_period: VSync period (in microseconds)
 **                 p_3d_data: 3D Data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_get_offsets(tAPP_3D_VIEW_MODE view_mode, UINT16 vsync_period, tBSA_DM_3D_TX_DATA *p_3d_data);

/*******************************************************************************
 **
 ** Function        app_3d_viewmode_get_desc
 **
 ** Description     Get 3D mode description
 **
 ** Parameters      void
 **
 ** Returns         3D mode description string
 **
 *******************************************************************************/
char *app_3d_viewmode_get_desc(void);

/*******************************************************************************
 **
 ** Function        app_3d_viewmode_set
 **
 ** Description     This function sets the View Mode
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_3d_viewmode_set(void);

/*******************************************************************************
 **
 ** Function        app_3d_active_screen_get
 **
 ** Description     This function is used to get the current Active Screen bit
 **
 ** Parameters      None
 **
 ** Returns         ShowRoom value
 **
 *******************************************************************************/
int app_3d_active_screen_get(void);

/*******************************************************************************
 **
 ** Function        app_3d_active_screen_toggle
 **
 ** Description     This function is used to toggle Active Screen
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_3d_active_screen_toggle(void);

#endif
