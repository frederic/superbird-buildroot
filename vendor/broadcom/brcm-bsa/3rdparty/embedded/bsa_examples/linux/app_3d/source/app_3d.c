/*****************************************************************************
**
**  Name:           app_3d.c
**
**  Description:    Bluetooth 3D application
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "app_3d.h"
#include "app_3dtv.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_tm_vse.h"
#include "app_dm.h"

/*
 * Global Variables
 */
tAPP_3D_CB app_3d_cb;

/*
 * Local functions
 */
static int app_3d_set_config(void);
static BOOLEAN app_3d_vse_callback (tBSA_TM_EVT event, tBSA_TM_MSG *p_data);


/*******************************************************************************
 **
 ** Function        app_3d_init
 **
 ** Description     This is the main init function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_init(void)
{
    memset(&app_3d_cb, 0, sizeof(app_3d_cb));
    app_3d_cb.brcm_mask = BSA_DM_3D_CAP_MASK | BSA_DM_RC_PAIRABLE_MASK;
    app_3d_cb.vsync_delay = APP_3DTV_VSYNC_DELAY;
    app_3d_cb.vsync_period = 0xFFFF;
    app_mgt_init();
    app_xml_init();
    app_3dtv_init();
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3d_exit
 **
 ** Description     This is the 3d exit function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_exit(void)
{
    /* Does nothing for the moment */
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3d_start
 **
 ** Description     DTV Start function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_start(void)
{
    /* Init specific 3D */
    if (app_3d_set_config() < 0)
    {
        APP_ERROR0("Unable to apply 3D configuration");
        return -1;
    }

    /* Register for every 3D VSE */
    if (app_tm_vse_register(APP_TM_VSE_3D_MODE_CHANGE, app_3d_vse_callback) < 0)
    {
        APP_INFO0("Unable to register 3D Mode Change VSE. App_tm maybe already started");
    }
    if (app_tm_vse_register(APP_TM_VSE_3D_FRAME_PERIOD, app_3d_vse_callback) < 0)
    {
        APP_INFO0("Unable to register 3D Frame Period VSE. App_tm maybe already started");
    }
    if (app_tm_vse_register(APP_TM_VSE_3D_MULTICAST_LOCK_CPLT, app_3d_vse_callback) < 0)
    {
        APP_INFO0("Unable to register 3D Slave Lock Change VSE. App_tm maybe already started");
    }
    if (app_tm_vse_register(APP_TM_VSE_3D_MULTICAST_SYNC_TO, app_3d_vse_callback) < 0)
    {
        APP_INFO0("Unable to register 3D Slave Timeout VSE. App_tm maybe already started");
    }
    if (app_tm_vse_register(APP_TM_VSE_3D_EIR_HANDSHAKE_CPLT, app_3d_vse_callback) < 0)
    {
        APP_INFO0("Unable to register 3D EIR Handshake VSE. App_tm maybe already started");
    }
    if (app_tm_vse_register(APP_TM_VSE_3D_MULTICAST_RX_DATA, app_3d_vse_callback) < 0)
    {
        APP_INFO0("Unable to register 3D Mcast Rx Data VSE. App_tm maybe already started");
    }

    /* Register for the APP_TM_VSE_FW_CORE_DUMP VSE to save the FW Core dump in a file  */
    if (app_tm_vse_register(APP_TM_VSE_FW_CORE_DUMP, app_3d_vse_callback) < 0)
    {
        APP_INFO0("Unable to register FW Core Dump VSE. App_tm maybe already started");
    }

    /* Switch TV to 3D Idle mode by default */
    if (app_3d_set_idle() < 0)
    {
        APP_ERROR0("Unable to set TV in Idle mode");
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3d_set_config
 **
 ** Description     Configure the BSA server with DTV specific parameters
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
static int app_3d_set_config(void)
{
    /* Set Discoverable and Connectable to allow 3D device to ProxPair & Connect */
    if (app_dm_set_visibility(TRUE, TRUE) < 0)
    {
        APP_ERROR0("app_dm_set_visibility failed");
        return -1;
    }

    /* Set PageScan parameters (rapid PageScan) */
    if (app_dm_set_page_scan_param(512, 18) < 0)
    {
        APP_ERROR0("app_dm_set_page_scan_param failed");
        return -1;
    }

    /* Set InquiryScan parameters (rapid InquiryScan) */
    if (app_dm_set_inquiry_scan_param(512, 18) < 0)
    {
        APP_ERROR0("app_dm_set_page_scan_param failed");
        return -1;
    }

    /* Set COD (to allow 3DG to ProxPair) */
    if (app_dm_set_device_class(0, BTM_COD_MAJOR_AUDIO, BTM_COD_MINOR_VIDDISP_LDSPKR) < 0)
    {
        APP_ERROR0("app_dm_set_device_class failed");
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function        app_3d_set_idle
 **
 ** Description     Function example to set DTV in Idle mode
 **
 ** Parameters      services_mask:services to look for
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_set_idle(void)
{
    tAPP_3DTV_IDLE_MODE idle_req;

    app_3dtv_set_idle_mode_init(&idle_req);
    idle_req.brcm_mask = app_3d_cb.brcm_mask;
    app_3dtv_set_idle_mode(&idle_req);
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3d_set_master
 **
 ** Description     Function example toset DTV in Master mode
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_set_master(void)
{
    tAPP_3DTV_MASTER_MODE master_req;

    app_3dtv_set_master_mode_init(&master_req);
    master_req.brcm_mask = app_3d_cb.brcm_mask;
    app_3dtv_set_master_mode(&master_req);
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3d_set_slave
 **
 ** Description     Function example to set DTV in Master mode
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_set_slave(void)
{
    tAPP_3DTV_SLAVE_MODE slave_req;
    int status;
    int device_index;
    BD_ADDR bd_addr;

    APP_INFO0("Bluetooth 3DTV Slave sub-menu:");

    APP_INFO0("\t0 Device from XML database (already paired)");
    APP_INFO0("\t1 Device found in last discovery");
    device_index = app_get_choice("Enter choice");

    /* Devices from XML database */
    if (device_index == 0)
    {
        /* Read the XML file which contains all the bonded devices */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        device_index = app_get_choice("Enter device number");
        if ((device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
            (device_index >= 0) &&
            (app_xml_remote_devices_db[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
        }
        else
        {
            APP_ERROR1("Bad device index:%d", device_index);
            return -1;
        }
    }
    /* Devices from Discovery */
    else if (device_index == 1)
    {
        app_disc_display_devices();
        device_index = app_get_choice("Enter device number");
        if ((device_index < APP_DISC_NB_DEVICES) &&
            (device_index >= 0) &&
            (app_discovery_cb.devs[device_index].in_use != FALSE))
        {
            bdcpy(bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
        }
        else
        {
            APP_ERROR1("Bad device index:%d", device_index);
            return -1;
        }
    }
    else
    {
        APP_ERROR0("Bad choice [XML(0) or Disc(1) only]");
        return -1;
    }

    app_3dtv_set_slave_mode_init(&slave_req);
    slave_req.brcm_mask = app_3d_cb.brcm_mask | BSA_DM_3D_CAP_MASK;
    bdcpy(slave_req.bd_addr_to_lock, bd_addr);
    status = app_3dtv_set_slave_mode(&slave_req);
    return status;
}

/*******************************************************************************
 **
 ** Function        app_3d_set_3d_offset_delay
 **
 ** Description     Function example to configure 3DTV Broadcasted offsets and delay
 **
 ** Parameters      use_default: indicates if default offsets must be used
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_3d_set_3d_offset_delay(BOOLEAN use_default)
{
    UINT16 left_open;
    UINT16 left_close;
    UINT16 right_open;
    UINT16 right_close;
    UINT16 delay;
    UINT8 dual_view;
    tAPP_3DTV_SET_BEACON beacon;
    int status;

    if (use_default == FALSE)
    {
        left_open = app_get_choice("Enter Left open");
        left_close = app_get_choice("Enter Left Close");
        right_open = app_get_choice("Enter Right Open");
        right_close = app_get_choice("Enter Right Close");
        delay = app_get_choice("Enter Delay");
        dual_view = app_get_choice("Dual View");
        /* Save user's Delay/DualView to reuse it later */
        app_3d_cb.vsync_delay = delay;
        app_3d_cb.dual_view = dual_view;
    }
    else
    {
        /* VSync = 0xFFFF means: no VSync signal */
        if (app_3d_cb.vsync_period == 0xFFFF)
        {
            left_open = 0xFFFF;
            left_close = 0xFFFF;
            right_open = 0xFFFF;
            right_close = 0xFFFF;
            delay = 0;
        }
        else
        {
            /* Compute Offsets based on VSync period */
            left_open = 0;
            left_close = app_3d_cb.vsync_period / 2 - 1;
            right_open = app_3d_cb.vsync_period / 2;
            right_close = app_3d_cb.vsync_period - 1;

            /* Get the delay manually entered by user  */
            delay = app_3d_cb.vsync_delay;
        }
        /* Get the DualView manually entered by user  */
        dual_view = app_3d_cb.dual_view;
    }
    APP_INFO1("Enabling 3DTV with LO:%d LC:%d RO:%d RC:%d Delay:%d DualView:0x%x\n",
            left_open, left_close, right_open, right_close, delay, dual_view);

    app_3dtv_set_beacon_init(&beacon);
    beacon.left_open = left_open;
    beacon.left_close = left_close;
    beacon.right_open = right_open;
    beacon.right_close = right_close;
    beacon.delay = delay;
    beacon.dual_view_mode = dual_view;

    status = app_3dtv_set_beacon(&beacon);
    if (status < 0)
    {
        APP_ERROR1("app_3dtv_set_beacon failed status:%d", status);
        return(-1);
    }
    return 0;
}

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
int app_3d_get_offsets(tAPP_3D_VIEW_MODE view_mode, UINT16 vsync_period, tBSA_DM_3D_TX_DATA *p_3d_data)
{
    switch(view_mode)
     {
     case APP_3D_VIEW_3D:
         /* Regular Offsets */
         p_3d_data->left_open_offset = 0;
         p_3d_data->left_close_offset = vsync_period / 2 - 1;
         p_3d_data->right_open_offset = vsync_period / 2;
         p_3d_data->right_close_offset = vsync_period - 1;
         p_3d_data->dual_view &= ~APP_3D_VIEW_MODE_MASK;
         break;

     case APP_3D_VIEW_DUAL_VIEW:
         /* Regular Offsets */
         p_3d_data->left_open_offset = 0;
         p_3d_data->left_close_offset = vsync_period / 2 - 1;
         p_3d_data->right_open_offset = vsync_period / 2;
         p_3d_data->right_close_offset = vsync_period - 1;
         p_3d_data->dual_view &= ~APP_3D_VIEW_MODE_MASK;
         p_3d_data->dual_view |= APP_3D_DUAL_VIEW;
         break;

     case APP_3D_VIEW_2D_2D:
         /* 2D/2D Offsets */
         p_3d_data->left_open_offset = 0;
         p_3d_data->left_close_offset = vsync_period / 2 - 1;
         p_3d_data->right_open_offset = vsync_period / 2;
         p_3d_data->right_close_offset = vsync_period - 1;
         p_3d_data->dual_view &= ~APP_3D_VIEW_MODE_MASK;
         p_3d_data->dual_view |= APP_3D_DUAL_VIEW | APP_3D_120_240_HZ;
         break;

     case APP_3D_VIEW_2D_3D:
         /* 2D/3D Offsets */
         p_3d_data->left_open_offset = 0;
         p_3d_data->left_close_offset = vsync_period / 2 - 1;
         p_3d_data->right_open_offset = vsync_period / 2;
         p_3d_data->right_close_offset = (3 * vsync_period / 4) - 1;
         p_3d_data->dual_view &= ~APP_3D_VIEW_MODE_MASK;
         p_3d_data->dual_view |= APP_3D_DUAL_VIEW | APP_3D_120_240_HZ;
         break;

     case APP_3D_VIEW_3D_2D:
         /* 3D/2D Offsets */
         p_3d_data->left_open_offset = 0;
         p_3d_data->left_close_offset = vsync_period / 4 - 1;
         p_3d_data->right_open_offset = vsync_period / 2;
         p_3d_data->right_close_offset = vsync_period - 1;
         p_3d_data->dual_view &= ~APP_3D_VIEW_MODE_MASK;
         p_3d_data->dual_view |= APP_3D_DUAL_VIEW | APP_3D_120_240_HZ;
         break;

     case APP_3D_VIEW_3D_3D:
         /* 3D/3D Offsets */
         p_3d_data->left_open_offset = 0;
         p_3d_data->left_close_offset = vsync_period / 4 - 1;
         p_3d_data->right_open_offset = vsync_period / 4;
         p_3d_data->right_close_offset = vsync_period / 2 - 1;
         p_3d_data->dual_view &= ~APP_3D_VIEW_MODE_MASK;
         p_3d_data->dual_view |= APP_3D_DUAL_VIEW | APP_3D_120_240_HZ;
         break;

     case APP_3D_VIEW_QUAL_VIEW:
         /* Quad View Offsets */
         p_3d_data->left_open_offset = 0;
         p_3d_data->left_close_offset = vsync_period / 4 - 1;
         p_3d_data->right_open_offset = vsync_period / 4;
         p_3d_data->right_close_offset = vsync_period / 2 - 1;
         p_3d_data->dual_view &= ~APP_3D_VIEW_MODE_MASK;
         p_3d_data->dual_view |= APP_3D_DUAL_VIEW | APP_3D_120_240_HZ | APP_3D_QUAD_VIEW;
         break;

     default:
         APP_ERROR1("Bad ViewMode=%d", view_mode);
         return -1;
     }
    return 0;
}
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
int app_3d_send_3ds_data(BOOLEAN use_default)
{
    tAPP_3DTV_TX_DATA tx_data;
    tBSA_DM_3D_TX_DATA bsa_tx_data;
    int status;

    if (use_default == FALSE)
    {
        bsa_tx_data.left_open_offset = app_get_choice("Enter Left open");
        bsa_tx_data.left_close_offset = app_get_choice("Enter Left Close");
        bsa_tx_data.right_open_offset = app_get_choice("Enter Right Open");
        bsa_tx_data.right_close_offset = app_get_choice("Enter Right Close");
        bsa_tx_data.delay = app_get_choice("Enter Delay");
        bsa_tx_data.dual_view = app_get_choice("Dual View");
        /* Save user's Delay/DualView to reuse it later */
        app_3d_cb.vsync_delay = bsa_tx_data.delay;
        app_3d_cb.dual_view = bsa_tx_data.dual_view;
    }
    else
    {
        /* VSync = 0xFFFF means: no VSync signal */
        if (app_3d_cb.vsync_period == 0xFFFF)
        {
            bsa_tx_data.left_open_offset = 0xFFFF;
            bsa_tx_data.left_close_offset = 0xFFFF;
            bsa_tx_data.right_open_offset = 0xFFFF;
            bsa_tx_data.right_close_offset = 0xFFFF;
            bsa_tx_data.delay = 0;
            bsa_tx_data.dual_view = 0x00;
        }
        else
        {
            /* Get the current DualView */
            bsa_tx_data.dual_view = app_3d_cb.dual_view;

            if (app_3d_get_offsets(app_3d_cb.view_mode,
                    app_3d_cb.vsync_period, &bsa_tx_data))
            {
                return -1;
            }

            /* Save the computed DualView value */
            app_3d_cb.dual_view = bsa_tx_data.dual_view;

            /* Get the Delay manually entered by user */
            bsa_tx_data.delay = app_3d_cb.vsync_delay;
        }
    }

    APP_INFO1("Send 3D Data with LO:%d LC:%d RO:%d RC:%d delay:%d dualView:0x%02X",
            bsa_tx_data.left_open_offset, bsa_tx_data.left_close_offset,
            bsa_tx_data.right_open_offset, bsa_tx_data.right_close_offset,
            bsa_tx_data.delay, bsa_tx_data.dual_view);

    app_3dtv_send_tx_data_init(&tx_data);
    tx_data.left_open = bsa_tx_data.left_open_offset;
    tx_data.left_close = bsa_tx_data.left_close_offset;
    tx_data.right_open = bsa_tx_data.right_open_offset;
    tx_data.right_close = bsa_tx_data.right_close_offset;
    tx_data.delay = bsa_tx_data.delay;
    tx_data.dual_view_mode = bsa_tx_data.dual_view;

    status = app_3dtv_send_tx_data(&tx_data);
    if (status < 0)
    {
        APP_ERROR1("app_3dtv_send_tx_data failed status:%d", status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3d_toggle_proximity_pairing
 **
 ** Description     This function is used to toggle ProximityPairing
 **
 ** Parameters      None
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_3d_toggle_proximity_pairing(void)
{
    if (app_3d_cb.brcm_mask & BSA_DM_3D_CAP_MASK)
    {
        APP_INFO0("Proximity Pairing is currently Enabled");
        APP_INFO0(" => Disable it");
        app_3d_cb.brcm_mask &= ~BSA_DM_3D_CAP_MASK;
    }
    else
    {
        APP_INFO0("Proximity Pairing is currently Disabled");
        APP_INFO0(" => Enable it");
        app_3d_cb.brcm_mask |= BSA_DM_3D_CAP_MASK;
    }
    switch(app_3dtv_cb.dtv_mode)
    {
    case APP_3DTV_MODE_IDLE:
        app_3d_set_idle();
        break;

    case APP_3DTV_MODE_MASTER:
        app_3d_set_master();
        break;

    case APP_3DTV_MODE_SLAVE:
        app_3d_set_slave();
        break;

    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_3d_toggle_showroom
 **
 ** Description     This function is used to toggle Showroom
 **
 ** Parameters      None
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_3d_toggle_showroom(void)
{
    if (app_3d_cb.brcm_mask & BSA_DM_SHOW_ROOM_MASK)
    {
        APP_INFO0("Showroom is currently Enabled");
        APP_INFO0(" => Disable it");
        app_3d_cb.brcm_mask &= ~BSA_DM_SHOW_ROOM_MASK;

    }
    else
    {
        APP_INFO0("Showroom is currently Disabled");
        APP_INFO0(" => Enable it");
        app_3d_cb.brcm_mask |= BSA_DM_SHOW_ROOM_MASK;

    }
    switch(app_3dtv_cb.dtv_mode)
    {
    case APP_3DTV_MODE_IDLE:
        app_3d_set_idle();
        break;

    case APP_3DTV_MODE_MASTER:
        app_3d_set_master();
        break;

    case APP_3DTV_MODE_SLAVE:
        app_3d_set_slave();
        break;

    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_3d_toggle_rc_pairable
 **
 ** Description     This function is used to toggle RC Pairable
 **
 ** Parameters      None
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_3d_toggle_rc_pairable(void)
{
    if (app_3d_cb.brcm_mask & BSA_DM_RC_PAIRABLE_MASK)
    {
        APP_INFO0("RC Pairable is currently Enabled");
        APP_INFO0(" => Disable it");
        app_3d_cb.brcm_mask &= ~BSA_DM_RC_PAIRABLE_MASK;
    }
    else
    {
        APP_INFO0("RC Pairable is currently Disabled");
        APP_INFO0(" => Enable it");
        app_3d_cb.brcm_mask |= BSA_DM_RC_PAIRABLE_MASK;
    }
    switch(app_3dtv_cb.dtv_mode)
    {
    case APP_3DTV_MODE_IDLE:
        app_3d_set_idle();
        break;

    case APP_3DTV_MODE_MASTER:
        app_3d_set_master();
        break;

    case APP_3DTV_MODE_SLAVE:
        app_3d_set_slave();
        break;

    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_3d_toggle_rc_associated
 **
 ** Description     This function is used to toggle RC Associated
 **
 ** Parameters      None
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_3d_toggle_rc_associated(void)
{
    if (app_3d_cb.brcm_mask & BSA_DM_RC_ASSO_MASK)
    {
        APP_INFO0("RC Associated is currently Enabled");
        APP_INFO0(" => Disable it");
        app_3d_cb.brcm_mask &= ~BSA_DM_RC_ASSO_MASK;
    }
    else
    {
        APP_INFO0("RC Associated is currently Disabled");
        APP_INFO0(" => Enable it");
        app_3d_cb.brcm_mask |= BSA_DM_RC_ASSO_MASK;
    }
    switch(app_3dtv_cb.dtv_mode)
    {
    case APP_3DTV_MODE_IDLE:
        app_3d_set_idle();
        break;

    case APP_3DTV_MODE_MASTER:
        app_3d_set_master();
        break;

    case APP_3DTV_MODE_SLAVE:
        app_3d_set_slave();
        break;

    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_3d_dualview_set
 **
 ** Description     This function is used to set the DualView value
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_3d_dualview_set(void)
{
    int choice;

    APP_INFO1("DualView is currently:0x%02X", app_3d_cb.dual_view);
    choice = app_get_choice("Enter new DualView value");

    app_3d_cb.dual_view = (UINT8)choice;

    /* Compute and send the new offsets */
    app_3d_send_3ds_data(TRUE);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3d_dualview_get
 **
 ** Description     This function is used Get current DualView value
 **
 ** Parameters      None
 **
 ** Returns         UINT8
 **
 *******************************************************************************/
UINT8 app_3d_dualview_get(void)
{
    return app_3d_cb.dual_view;
}

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
void app_3d_set_audio_bits(UINT8 audio_bit_mask)
{
    /* Sanity check */
    if (audio_bit_mask & ~APP_3D_AUDIO_MASK)
    {
        APP_ERROR1("Bad audio Bit Mask:x%x", audio_bit_mask);
    }

    /* Reset the Audio Bit mask */
    app_3d_cb.dual_view &= ~APP_3D_AUDIO_MASK;

    /* Set the bit mask */
    app_3d_cb.dual_view |= (audio_bit_mask & APP_3D_AUDIO_MASK);

    /* Send 3D Data */
    app_3d_send_3ds_data(TRUE);
}

/*******************************************************************************
 **
 ** Function       app_3d_vse_decode_param
 **
 ** Description    Decode VSE Parameters
 **
 ** Parameters     event: TM Event received
 **                p_data: TM Event's data
 **
 ** Returns        BOOLEAN:TRUE if the VSE has been handled
 **
 *******************************************************************************/
static BOOLEAN app_3d_vse_decode_param (tBSA_TM_VSE_MSG *p_vse)
{
    UINT8 *p_data = p_vse->data;
    UINT16 frame_period;
    UINT8 frame_period_fraction;
    UINT8 frame_period_initial;
    BOOLEAN exit_generic_cback= TRUE;

    switch (p_vse->sub_event)
    {
    case APP_TM_VSE_3D_FRAME_PERIOD:
        /* Extract TAvg, TAvg_fraction and Innitial_measurement parameters */
        STREAM_TO_UINT16(frame_period, p_data);
        STREAM_TO_UINT8(frame_period_fraction, p_data);
        STREAM_TO_UINT8(frame_period_initial, p_data);
        if ((frame_period) && (frame_period != 0xFFFF))
        {
            APP_INFO1("3D_FRAME_PERIOD Period:%d (%.2f HZ) PeriodFraction:%d PeriodInitial:%d",
                    frame_period, (float)1000000/(float)frame_period, frame_period_fraction, frame_period_initial);
        }
        else
        {
            APP_INFO1("3D_FRAME_PERIOD Period:%d PeriodFraction:%d PeriodInitial:%d",
                    frame_period, frame_period_fraction, frame_period_initial);
        }
        app_3d_cb.vsync_period = frame_period;
        /* If the DTV is in Master mode and if this is not an initial (not accurate measurement) */
        if ((app_3dtv_cb.dtv_mode == APP_3DTV_MODE_MASTER) &&
            (frame_period_initial == FALSE))
        {
            /* Apply new offset for Multicast devices (via new 3D sync profile) */
            app_3d_send_3ds_data(TRUE);
        }
        break;

    case APP_TM_VSE_3D_MODE_CHANGE:
        APP_INFO0("3D_MODE_CHANGE");
        break;

    default:
        /* Let the generic callback decode the other unknown VSE */
        exit_generic_cback = FALSE;
        break;
    }
    return exit_generic_cback;
}

/*******************************************************************************
 **
 ** Function       app_tm_vse_default_callback
 **
 ** Description    Default TM Callback
 **
 ** Parameters     event: TM Event received
 **                p_data: TM Event's data
 **
 ** Returns        TRUE if the event has been handled (generic handler will return)
 **
 *******************************************************************************/
static BOOLEAN app_3d_vse_callback (tBSA_TM_EVT event, tBSA_TM_MSG *p_data)
{
    BOOLEAN exit_generic_cback= FALSE;

    if (p_data == NULL)
    {
        APP_ERROR0("Null data pointer");
        return exit_generic_cback;
    }

    switch(event)
    {
    case BSA_TM_VSE_EVT:
        /* Decode VSE Parameters */
        exit_generic_cback = app_3d_vse_decode_param(&p_data->vse);
        break;

    default:
        break;
    }

    return exit_generic_cback;
}

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
char *app_3d_viewmode_get_desc(void)
{
    switch(app_3d_cb.view_mode)
    {
    case APP_3D_VIEW_3D:
        return "3D (Regular)";
    case APP_3D_VIEW_DUAL_VIEW:
        return "DualView (Regular)";
    case APP_3D_VIEW_2D_2D:
        return "2D/2D (240Hz)";
    case APP_3D_VIEW_2D_3D:
        return "2D/3D (240Hz)";
    case APP_3D_VIEW_3D_2D:
        return "3D/2D (240Hz)";
    case APP_3D_VIEW_3D_3D:
        return "3D/3D (240Hz)";
    case APP_3D_VIEW_QUAL_VIEW:
        return "QuadView (240Hz)";
    }
    return "Unknown ViewMode";
}


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
int app_3d_viewmode_set(void)
{
    int choice;

    APP_INFO0("ViewMode Control");
    APP_INFO1("\t=> %d: 3D (Regular)", APP_3D_VIEW_3D);
    APP_INFO1("\t=> %d: DualView (Regular)", APP_3D_VIEW_DUAL_VIEW);
    APP_INFO1("\t=> %d: 2D/2D (240Hz)", APP_3D_VIEW_2D_2D);
    APP_INFO1("\t=> %d: 2D/3D (240Hz)", APP_3D_VIEW_2D_3D);
    APP_INFO1("\t=> %d: 3D/2D (240Hz)", APP_3D_VIEW_3D_2D);
    APP_INFO1("\t=> %d: 3D/3D (240Hz)", APP_3D_VIEW_3D_3D);
    APP_INFO1("\t=> %d: QuadView (240Hz)", APP_3D_VIEW_QUAL_VIEW);

    choice = app_get_choice("Enter view mode");
    if ((choice < APP_3D_VIEW_3D) || (choice > APP_3D_VIEW_QUAL_VIEW))
    {
        APP_ERROR1("Bad ViewMode:%d", choice);
        return -1;
    }

    /* Save the selected mode */
    app_3d_cb.view_mode = (tAPP_3D_VIEW_MODE)choice;

    APP_INFO1("%s mode selected", app_3d_viewmode_get_desc());

    /* Send 3D Data */
    return app_3d_send_3ds_data(TRUE);
}

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
int app_3d_active_screen_get(void)
{
    if (app_3d_cb.dual_view & APP_3D_ACTIVE_SCREEN)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

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
int app_3d_active_screen_toggle(void)
{
    if (app_3d_cb.dual_view & APP_3D_ACTIVE_SCREEN)
    {
        APP_INFO0("Active Screen is currently Enabled");
        APP_INFO0(" => Disable it");
        app_3d_cb.dual_view &= ~APP_3D_ACTIVE_SCREEN;

    }
    else
    {
        APP_INFO0("Active Screen is currently Disabled");
        APP_INFO0(" => Enable it");
        app_3d_cb.dual_view |= APP_3D_ACTIVE_SCREEN;

    }
    /* Send 3D Data */
    return app_3d_send_3ds_data(TRUE);
}

