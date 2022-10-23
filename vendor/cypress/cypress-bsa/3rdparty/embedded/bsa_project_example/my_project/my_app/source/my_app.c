/*****************************************************************************
**
**  Name:           my_app.c
**
**  Description:    My Bluetooth Application
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "my_app.h"

/*
 * Definitions
 */
/* Default BdAddr */
#define APP_DEFAULT_BD_ADDR             {0xBE, 0xEF, 0xBE, 0xEF, 0x00, 0x01}

/* Default local Name */
#define APP_DEFAULT_BT_NAME             "my BSA Bluetooth Device"

/* Default COD SetTopBox (Major Service = none) (MajorDevclass = Audio/Video) (Minor=Display&Speaker) */
#define APP_DEFAULT_CLASS_OF_DEVICE     {0x00, 0x04, 0x3C}

#define APP_DEFAULT_ROOT_PATH           "./"

#define APP_DEFAULT_PIN_CODE            "0000"
#define APP_DEFAULT_PIN_LEN             4

/*
 * Simple Pairing Input/Output Capabilities:
 * BSA_SEC_IO_CAP_OUT  =>  DisplayOnly
 * BSA_SEC_IO_CAP_IO   =>  DisplayYesNo
 * BSA_SEC_IO_CAP_IN   =>  KeyboardOnly
 * BSA_SEC_IO_CAP_NONE =>  NoInputNoOutput
* * #if BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE
 * BSA_SEC_IO_CAP_KBDISP =>  KeyboardDisplay
 * #endif
 */
#ifndef APP_SEC_IO_CAPABILITIES
#define APP_SEC_IO_CAPABILITIES BSA_SEC_IO_CAP_IO
#endif

#ifdef APP_BLE_INCLUDED
#define APP_BLE_MAIN_DEFAULT_APPL_UUID 9000
#endif

/*
 * Simple Pairing Input/Output Capabilities:
 * BSA_SEC_IO_CAP_OUT  =>  DisplayOnly
 * BSA_SEC_IO_CAP_IO   =>  DisplayYesNo
 * BSA_SEC_IO_CAP_NONE =>  NoInputNoOutput
 */
#define APP_DEFAULT_IO_CAPABILITIES    BSA_SEC_IO_CAP_NONE

typedef struct
{
    int dummy;
} tAPP_CB;

/*
 * Global Variables
 */
tAPP_CB app_cb;

/*
 * Local functions
 */
static int my_app_set_local_config(void);
static int my_app_set_visibility(void);
static int my_app_set_security(void);

/*******************************************************************************
 **
 ** Function        my_app_init
 **
 ** Description     This is the main init function
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int my_app_init(void)
{
    int status;

    memset(&app_cb, 0, sizeof(app_cb));
    app_mgt_init();

    app_xml_init();
#ifdef APP_BLE_INCLUDED
    status = app_ble_init();
    if (status < 0)
    {
        APP_ERROR0("Couldn't Initialize BLE, exiting");
        return status;
    }
#endif
#ifdef APP_HH_INCLUDED
    app_hh_init();
#endif
    return 0;
}

/*******************************************************************************
 **
 ** Function        my_app_start
 **
 ** Description     tart function
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int my_app_start(void)
{
    if (my_app_set_local_config() < 0)
    {
        APP_ERROR0("my_app_set_local_config failed");
        return -1;
    }

    if (my_app_set_security() < 0)
    {
        APP_ERROR0("my_app_set_security failed");
        return -1;
    }

#ifdef APP_BLE_INCLUDED
    /* Start BLE application */
    if (app_ble_start() < 0)
    {
        APP_ERROR0("app_ble_start failed");
        return -1;
    }

    if (app_ble_client_register(APP_BLE_MAIN_DEFAULT_APPL_UUID) < 0)
    {
        APP_ERROR0("app_ble_client_register failed");
        return -1;
    }

#endif

#ifdef APP_HH_INCLUDED
    if (app_hh_start() < 0)
    {
        APP_ERROR0("app_hh_start failed");
        return -1;
    }
#endif

#ifdef APP_AVK_INCLUDED
    if (app_avk_init(NULL) < 0)
    {
        APP_ERROR0("app_avk_init failed");
        return -1;
    }
    /* Register One AVK Sink endpoint */
    if (app_avk_register() < 0)
    {
        APP_ERROR0("app_avk_register failed");
        return -1;
    }
#endif

#ifdef APP_AV_INCLUDED
    /* Init AV */
    if (app_av_init(TRUE) < 0)
    {
        APP_ERROR0("app_av_init failed");
        return -1;
    }
    /* Register One AV source endpoint */
    if (app_av_register() < 0)
    {
        APP_ERROR0("app_av_register(1) failed");
        return -1;
    }
#endif /* APP_AV_INCLUDED */

#ifdef APP_HEADLESS_INCLUDED
    /* Disable Headless (to re-enable TBFC) */
    /* This is in case the BT controller was in Headless mode when we started */
    if (app_headless_control(0) < 0)
    {
        APP_ERROR0("app_headless_control failed");
        /* Do not return an error for backward compatibility (old FW may not support TBFC) */
    }
#endif

#if 0
    /* Set Visibility (PageScan/InquiryScan) once everything is ready */
    if (my_app_set_visibility() < 0)
    {
        APP_ERROR0("my_app_set_visibility failed");
        return -1;
    }
#endif

#if 0
    if (app_mgr_config() < 0)
    {
        return -1;
    }
#endif
    if (my_app_set_visibility() < 0)
    {
        APP_ERROR0("my_app_set_visibility failed");
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        my_app_exit
 **
 ** Description     This is the main exit function
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int my_app_exit(void)
{
#ifdef APP_HH_INCLUDED
    /* Exit HH */
    app_hh_exit();
#endif

#ifdef APP_AV_INCLUDED
    /* Exit AV */
    app_av_end();
#endif

#ifdef APP_AVK_INCLUDED
    /* Exit AVK */
    app_avk_end();
#endif

    return 0;
}

/*******************************************************************************
 **
 ** Function         my_app_set_local_config
 **
 ** Description      Set local configuration
 **
 ** Returns          Status of the operation
 **
 *******************************************************************************/
static int my_app_set_local_config(void)
{
    int                 status;
    BD_ADDR             local_bd_addr = APP_DEFAULT_BD_ADDR;
    DEV_CLASS           local_class_of_device = APP_DEFAULT_CLASS_OF_DEVICE;
    struct timeval      tv;
    unsigned int        rand_seed;
    tBSA_DM_SET_CONFIG bt_config;

    /*
     * Example of function to read the XML file which contains the Local Bluetooth configuration
     */
    status = app_mgr_read_config();
    if (status < 0)
    {
        /* If the file does not exist, creates a default one */
        APP_ERROR0("Creating default XML config file");
        app_xml_config.enable = TRUE;
        app_xml_config.discoverable = TRUE;
        app_xml_config.connectable = TRUE;
        strncpy((char *)app_xml_config.name, APP_DEFAULT_BT_NAME, sizeof(app_xml_config.name));
        app_xml_config.name[sizeof(app_xml_config.name) - 1] = '\0';
        bdcpy(app_xml_config.bd_addr, local_bd_addr);
        /* let's use a random number for the last two bytes of the BdAddr */
        gettimeofday(&tv, NULL);
        rand_seed = tv.tv_sec * tv.tv_usec * getpid();
        app_xml_config.bd_addr[4] = rand_r(&rand_seed);
        app_xml_config.bd_addr[5] = rand_r(&rand_seed);
        memcpy(app_xml_config.class_of_device, local_class_of_device, sizeof(DEV_CLASS));
        strncpy(app_xml_config.root_path, APP_DEFAULT_ROOT_PATH, sizeof(app_xml_config.root_path));
        app_xml_config.root_path[sizeof(app_xml_config.root_path) - 1] = '\0';
        strncpy((char *)app_xml_config.pin_code, APP_DEFAULT_PIN_CODE, APP_DEFAULT_PIN_LEN);
        /* The following code will not work if APP_DEFAULT_PIN_LEN is 16 bytes */
        app_xml_config.pin_code[APP_DEFAULT_PIN_LEN] = '\0';
        app_xml_config.pin_len = APP_DEFAULT_PIN_LEN;
        app_xml_config.io_cap = APP_SEC_IO_CAPABILITIES;

        status = app_mgr_write_config();
        if (status < 0)
        {
            APP_ERROR0("Unable to Create default XML config file");
            app_mgt_close();
            return status;
        }
    }

    /*
     * Set bluetooth configuration (from XML file)
     */
    /* Init config parameter */
    BSA_DmSetConfigInit(&bt_config);

    bt_config.enable = app_xml_config.enable;
    /* Do not set Discoverable/Connectable for the moment */
    bt_config.discoverable = 0;
    bt_config.connectable = 0;
    bdcpy(bt_config.bd_addr, app_xml_config.bd_addr);
    strncpy((char *)bt_config.name, (char *)app_xml_config.name, sizeof(bt_config.name));
    bt_config.name[sizeof(bt_config.name) - 1] = '\0';
    memcpy(bt_config.class_of_device, app_xml_config.class_of_device, sizeof(DEV_CLASS));

    APP_DEBUG1("Enable:%d", bt_config.enable);
    APP_DEBUG1("Discoverable:%d", bt_config.discoverable);
    APP_DEBUG1("Connectable:%d", bt_config.connectable);
    APP_DEBUG1("Name:%s", bt_config.name);
    APP_DEBUG1("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            bt_config.bd_addr[0], bt_config.bd_addr[1],
            bt_config.bd_addr[2], bt_config.bd_addr[3],
            bt_config.bd_addr[4], bt_config.bd_addr[5]);
    APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x", bt_config.class_of_device[0],
            bt_config.class_of_device[1], bt_config.class_of_device[2]);
    APP_DEBUG1("First host disabled channel:%d", bt_config.first_disabled_channel);
    APP_DEBUG1("Last host disabled channel:%d", bt_config.last_disabled_channel);

    /* Apply BT config */
    status = BSA_DmSetConfig(&bt_config);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed:%d", status);
        return(-1);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         my_app_set_security
 **
 ** Description      Set Security
 **
 ** Returns          Status of the operation
 **
 *******************************************************************************/
static int my_app_set_security(void)
{
    int status;
    int index;
    tBSA_SEC_ADD_DEV    bsa_add_dev_param;

    /* Set automatic reply for security */
    if(app_sec_set_pairing_mode(APP_SEC_AUTO_PAIR_MODE) < 0)
    {
        APP_ERROR0("app_sec_set_pairing_mode failed");
        return -1;
    }

    /* Set security */
    if(app_sec_set_security(NULL) < 0)
    {
        APP_ERROR0("app_sec_set_security failed");
        return -1;
    }

    /* Example of function to read the database of remote devices */
    status = app_mgr_read_remote_devices();
    if (status < 0)
    {
        APP_ERROR0("No remote device database found");
    }
    else
    {
        app_xml_display_devices(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    }

    /* Add every devices found in remote device database */
    /* They will be able to connect to our device */
    APP_INFO0("Add all devices found in database");
    for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; index++)
    {
        if (app_xml_remote_devices_db[index].in_use != FALSE)
        {
            APP_INFO1("Adding device:%s", app_xml_remote_devices_db[index].name);
            BSA_SecAddDeviceInit(&bsa_add_dev_param);
            bdcpy(bsa_add_dev_param.bd_addr,
                    app_xml_remote_devices_db[index].bd_addr);
            memcpy(bsa_add_dev_param.class_of_device,
                    app_xml_remote_devices_db[index].class_of_device,
                    sizeof(DEV_CLASS));
            memcpy(bsa_add_dev_param.link_key,
                    app_xml_remote_devices_db[index].link_key,
                    sizeof(LINK_KEY));
            bsa_add_dev_param.link_key_present = app_xml_remote_devices_db[index].link_key_present;
            bsa_add_dev_param.trusted_services = app_xml_remote_devices_db[index].trusted_services;
            bsa_add_dev_param.is_trusted = TRUE;
            bsa_add_dev_param.key_type = app_xml_remote_devices_db[index].key_type;
            bsa_add_dev_param.io_cap = app_xml_remote_devices_db[index].io_cap;
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
            bsa_add_dev_param.ble_addr_type = app_xml_remote_devices_db[index].ble_addr_type;
            bsa_add_dev_param.device_type = app_xml_remote_devices_db[index].device_type;
            bsa_add_dev_param.inq_result_type = app_xml_remote_devices_db[index].inq_result_type;
            if(app_xml_remote_devices_db[index].ble_link_key_present)
            {
                bsa_add_dev_param.ble_link_key_present = TRUE;

                /* Fill PENC Key */
                memcpy(bsa_add_dev_param.le_penc_key.ltk,
                    app_xml_remote_devices_db[index].penc_ltk,
                    sizeof(app_xml_remote_devices_db[index].penc_ltk));
                memcpy(bsa_add_dev_param.le_penc_key.rand,
                    app_xml_remote_devices_db[index].penc_rand,
                    sizeof(app_xml_remote_devices_db[index].penc_rand));
                bsa_add_dev_param.le_penc_key.ediv = app_xml_remote_devices_db[index].penc_ediv;
                bsa_add_dev_param.le_penc_key.sec_level = app_xml_remote_devices_db[index].penc_sec_level;
                bsa_add_dev_param.le_penc_key.key_size = app_xml_remote_devices_db[index].penc_key_size;

                /* Fill PID Key */
                memcpy(bsa_add_dev_param.le_pid_key.irk,
                    app_xml_remote_devices_db[index].pid_irk,
                    sizeof(app_xml_remote_devices_db[index].pid_irk));
                bsa_add_dev_param.le_pid_key.addr_type = app_xml_remote_devices_db[index].pid_addr_type;
                memcpy(bsa_add_dev_param.le_pid_key.static_addr,
                    app_xml_remote_devices_db[index].pid_static_addr,
                    sizeof(app_xml_remote_devices_db[index].pid_static_addr));

                /* Fill PCSRK Key */
                bsa_add_dev_param.le_pcsrk_key.counter = app_xml_remote_devices_db[index].pcsrk_counter;
                memcpy(bsa_add_dev_param.le_pcsrk_key.csrk,
                    app_xml_remote_devices_db[index].pcsrk_csrk,
                    sizeof(app_xml_remote_devices_db[index].pcsrk_csrk));
                bsa_add_dev_param.le_pcsrk_key.sec_level = app_xml_remote_devices_db[index].pcsrk_sec_level;

                /* Fill LCSRK Key */
                bsa_add_dev_param.le_lcsrk_key.counter = app_xml_remote_devices_db[index].lcsrk_counter;
                bsa_add_dev_param.le_lcsrk_key.div = app_xml_remote_devices_db[index].lcsrk_div;
                bsa_add_dev_param.le_lcsrk_key.sec_level = app_xml_remote_devices_db[index].lcsrk_sec_level;

                /* Fill LENC Key */
                bsa_add_dev_param.le_lenc_key.div = app_xml_remote_devices_db[index].lenc_div;
                bsa_add_dev_param.le_lenc_key.key_size = app_xml_remote_devices_db[index].lenc_key_size;
                bsa_add_dev_param.le_lenc_key.sec_level = app_xml_remote_devices_db[index].lenc_sec_level;
            }
#endif
            /* Add the device */
            status = BSA_SecAddDevice(&bsa_add_dev_param);
            if (status < 0)
            {
                APP_ERROR1("BSA_SecAddDevice failed:%d", status);
                return status;
            }
        }
    }

    return 0;
}

/*******************************************************************************
**
** Function         my_app_set_visibility
**
** Description      Set Visibility
**
** Returns          Status of the operation
**
*******************************************************************************/
static int my_app_set_visibility(void)
{
    int                 status;
    tBSA_DM_SET_CONFIG bt_config;

    /*
     * Example of function to read the XML file which contains the Local Bluetooth configuration
     */
    status = app_mgr_read_config();
    if (status < 0)
    {
        /* The local configuration file shoulds exist */
        APP_ERROR0("XML config file missing");
        return -1;
    }

    /* If Bluetooth is enabled and either Discoverable or Connectable */
    if ((app_xml_config.enable) &&
        (app_xml_config.discoverable || app_xml_config.connectable))
    {
        /* Init config parameter */
        BSA_DmSetConfigInit(&bt_config);
        /* Visibility flag only */
        bt_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
        bt_config.enable = app_xml_config.enable;;
        bt_config.discoverable = app_xml_config.discoverable;
        bt_config.connectable = app_xml_config.connectable;

        /* Apply BT config */
        status = BSA_DmSetConfig(&bt_config);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_DmSetConfig failed:%d", status);
            return -1;
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        my_app_mgt_callback
 **
 ** Description     This callback function is called in case of server
 **                 disconnection (e.g. server crashes)
 **
 ** Parameters      event: Management event
 **                 p_data: parameters
 **
 ** Returns         BOOLEAN: True if the generic callback must return (do not handle the event)
 **                          False if the generic callback must handle the event
 **
 *******************************************************************************/
BOOLEAN my_app_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_INFO0("my_app_mgt_callback BSA_MGT_STATUS_EVT");
        if (p_data->status.enable == FALSE)
        {
            APP_INFO0("\tBluetooth Stopped");
        }
        else
        {
            APP_INFO0("\tBluetooth restarted => re-start the application");
            my_app_start();
        }

        break;
    case BSA_MGT_DISCONNECT_EVT:
        /* Connection with the Server lost => Application will have to reconnect */
        APP_INFO1("my_app_mgt_callback BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
        exit(1);
        break;
    }

    /* Do not execute generic Management callback */
    return TRUE;
}
