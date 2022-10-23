/*****************************************************************************
**
**  Name:           app_manager.c
**
**  Description:    Bluetooth Manager application
**
**  Copyright (c) 2010-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "buildcfg.h"

#include "app_xml_param.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "gki.h"
#include "uipc.h"

#include "bsa_api.h"

#include "app_xml_utils.h"

#include "app_disc.h"
#include "app_utils.h"
#include "app_dm.h"

#include "app_services.h"
#include "app_mgt.h"


/* Default path to UIPC */
#define APP_DEFAULT_UIPC_PATH "./"

/* Default BdAddr */
#define APP_DEFAULT_BD_ADDR             {0xBE, 0xEF, 0xBE, 0xEF, 0x00, 0x01}

/* Default local Name */
#define APP_DEFAULT_BT_NAME             "BSA Sample app"

/* Default COD Car-kit (Major Service = Render/Audio) (MajorDevclass = Audio/Video) (Minor hands-free) */
#define APP_DEFAULT_CLASS_OF_DEVICE    {0x24, 0x04, 0x08}

#define APP_DEFAULT_ROOT_PATH           "./pictures"

#define APP_DEFAULT_PIN_CODE            "0000"
#define APP_DEFAULT_PIN_LEN             4

#define APP_XML_CONFIG_FILE_PATH            "./bt_config.xml"
#define APP_XML_REM_DEVICES_FILE_PATH       "./bt_devices.xml"

/*
* Simple Pairing Input/Output Capabitilies:
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


#define BSA_SAMPLE_APP


/* Menu items */
enum
{
    APP_MGR_MENU_ABORT_DISC         = 1,
    APP_MGR_MENU_DISCOVERY,
    APP_MGR_MENU_DISCOVERY_TEST,
    APP_MGR_MENU_BOND,
    APP_MGR_MENU_BOND_CANCEL,
    APP_MGR_MENU_SVC_DISCOVERY,
    APP_MGR_MENU_DI_DISCOVERY,
    APP_MGR_MENU_SET_LOCAL_DI,
    APP_MGR_MENU_GET_LOCAL_DI,
    APP_MGR_MENU_STOP_BT,
    APP_MGR_MENU_START_BT,
    APP_MGR_MENU_SP_ACCEPT,
    APP_MGR_MENU_SP_REFUSE,
    APP_MGR_MENU_READ_CONFIG,
    APP_MGR_MENU_DISCOVERABLE,
    APP_MGR_MENU_NON_DISCOVERABLE,
    APP_MGR_MENU_SET_AFH_CHANNELS,
    APP_MGR_MENU_CLASS2,
    APP_MGR_MENU_CLASS1_5,
    APP_MGR_MENU_DUAL_STACK,
    APP_MGR_MENU_KILL_SERVER        = 96,
    APP_MGR_MENU_MGR_INIT           = 97,
    APP_MGR_MENU_MGR_CLOSE          = 98,
    APP_MGR_MENU_QUIT               = 99
};

typedef struct
{
    tBSA_DM_DUAL_STACK_MODE dual_stack_mode; /* Dual Stack Mode */
} tAPP_MGR_CB;
/* Callback provided to the application  for security events*/
typedef void (tSecurity_callback)(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data);
tSecurity_callback *g_pCallback = NULL;

/*
* Global variables
*/
tAPP_XML_CONFIG         app_xml_config;
BD_ADDR                 app_sec_db_addr;    /* BdAddr of peer device requesting SP */
BOOLEAN                 app_sec_is_ble;

tAPP_MGR_CB app_mgr_cb;

/*
* Local functions
*/
int app_mgr_open(char *uipc_path);
int app_mgr_config(tSecurity_callback cb);
void app_mgr_close(void);
char *app_mgr_get_dual_stack_mode_desc(void);


/*******************************************************************************
**
** Function         app_mgr_display_main_menu
**
** Description      This is the main menu
**
** Parameters       None
**
** Returns          None
**
*******************************************************************************/
void app_mgr_display_main_menu(void)
{
    APP_INFO0("Bluetooth Application Manager Main menu:");
    APP_INFO1("\t%d => Abort Discovery", APP_MGR_MENU_ABORT_DISC);
    APP_INFO1("\t%d => Discovery", APP_MGR_MENU_DISCOVERY);
    APP_INFO1("\t%d => Discovery test", APP_MGR_MENU_DISCOVERY_TEST);
    APP_INFO1("\t%d => Bonding", APP_MGR_MENU_BOND);
    APP_INFO1("\t%d => Cancel Bonding", APP_MGR_MENU_BOND_CANCEL);
    APP_INFO1("\t%d => Services Discovery (all services)", APP_MGR_MENU_SVC_DISCOVERY);
    APP_INFO1("\t%d => Device Id Discovery", APP_MGR_MENU_DI_DISCOVERY);
    APP_INFO1("\t%d => Set local Device Id", APP_MGR_MENU_SET_LOCAL_DI);
    APP_INFO1("\t%d => Get local Device Id", APP_MGR_MENU_GET_LOCAL_DI);
    APP_INFO1("\t%d => Stop Bluetooth", APP_MGR_MENU_STOP_BT);
    APP_INFO1("\t%d => Restart Bluetooth", APP_MGR_MENU_START_BT);
    APP_INFO1("\t%d => Accept Simple Pairing", APP_MGR_MENU_SP_ACCEPT);
    APP_INFO1("\t%d => Refuse Simple Pairing", APP_MGR_MENU_SP_REFUSE);
    APP_INFO1("\t%d => Read Device configuration", APP_MGR_MENU_READ_CONFIG);
    APP_INFO1("\t%d => Set device discoverable", APP_MGR_MENU_DISCOVERABLE);
    APP_INFO1("\t%d => Set device non discoverable", APP_MGR_MENU_NON_DISCOVERABLE);
    APP_INFO1("\t%d => Set AFH Configuration", APP_MGR_MENU_SET_AFH_CHANNELS);
    APP_INFO1("\t%d => Set Tx Power Class2 (specific FW needed)", APP_MGR_MENU_CLASS2);
    APP_INFO1("\t%d => Set Tx Power Class1.5 (specific FW needed)", APP_MGR_MENU_CLASS1_5);
    APP_INFO1("\t%d => Change Dual Stack Mode (currently:%s)",
            APP_MGR_MENU_DUAL_STACK, app_mgr_get_dual_stack_mode_desc());
    APP_INFO1("\t%d => Kill BSA server", APP_MGR_MENU_KILL_SERVER);
    APP_INFO1("\t%d => Connect to BSA server", APP_MGR_MENU_MGR_INIT);
    APP_INFO1("\t%d => Disconnect from BSA server", APP_MGR_MENU_MGR_CLOSE);
    APP_INFO1("\t%d => Quit", APP_MGR_MENU_QUIT);
}

/*******************************************************************************
**
** Function         app_mgr_management_callback
**
** Description      This callback function is called in case of server
**                  disconnection (e.g. server crashes)
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_mgr_management_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable == FALSE)
        {
            APP_DEBUG0("Bluetooth Stopped");
        }
        else
        {
            /* Re-Init App manager */
            /* The FALSE parameter indicate Application start => open BSA connection */
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
            app_mgr_config(NULL);
        }

        break;
    case BSA_MGT_DISCONNECT_EVT:
        APP_DEBUG1("BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
        exit(-1);
        break;
    }
}

/*******************************************************************************
**
** Function         app_mgr_read_config
**
** Description      This function is used to read the XML bluetooth configuration file
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_read_config(void)
{
    int status;

    status = app_xml_read_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
    if (status < 0)
    {
        APP_ERROR1("app_xml_read_cfg failed:%d", status);
        return -1;
    }
    else
    {
        APP_DEBUG1("Enable:%d", app_xml_config.enable);
        APP_DEBUG1("Discoverable:%d", app_xml_config.discoverable);
        APP_DEBUG1("Connectable:%d", app_xml_config.connectable);
        APP_DEBUG1("Name:%s", app_xml_config.name);
        APP_DEBUG1("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            app_xml_config.bd_addr[0], app_xml_config.bd_addr[1],
            app_xml_config.bd_addr[2], app_xml_config.bd_addr[3],
            app_xml_config.bd_addr[4], app_xml_config.bd_addr[5]);
        APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x => %s", app_xml_config.class_of_device[0],
            app_xml_config.class_of_device[1], app_xml_config.class_of_device[2],
            app_get_cod_string(app_xml_config.class_of_device));
        APP_DEBUG1("RootPath:%s", app_xml_config.root_path);

        APP_DEBUG1("Default PIN (%d):%s", app_xml_config.pin_len, app_xml_config.pin_code);
    }
    return 0;
}



/*******************************************************************************
**
** Function         app_mgr_write_config
**
** Description      This function is used to write the XML bluetooth configuration file
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_write_config(void)
{
    int status;

    status = app_xml_write_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
    if (status < 0)
    {
        APP_ERROR1("app_xml_write_cfg failed:%d", status);
        return -1;
    }
    else
    {
        APP_DEBUG1("Enable:%d", app_xml_config.enable);
        APP_DEBUG1("Discoverable:%d", app_xml_config.discoverable);
        APP_DEBUG1("Connectable:%d", app_xml_config.connectable);
        APP_DEBUG1("Name:%s", app_xml_config.name);
        APP_DEBUG1("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            app_xml_config.bd_addr[0], app_xml_config.bd_addr[1],
            app_xml_config.bd_addr[2], app_xml_config.bd_addr[3],
            app_xml_config.bd_addr[4], app_xml_config.bd_addr[5]);
        APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x", app_xml_config.class_of_device[0],
            app_xml_config.class_of_device[1], app_xml_config.class_of_device[2]);
        APP_DEBUG1("RootPath:%s", app_xml_config.root_path);
    }
    return 0;
}

/*******************************************************************************
**
** Function         app_mgr_read_remote_devices
**
** Description      This function is used to read the XML bluetooth remote device file
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_read_remote_devices(void)
{
    int status;
    int index;

    for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; index++)
    {
        app_xml_remote_devices_db[index].in_use = FALSE;
    }

    status = app_xml_read_db(APP_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
        APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    if (status < 0)
    {
        APP_ERROR1("app_xml_read_db failed:%d", status);
        return -1;
    }
    return 0;
}



/*******************************************************************************
**
** Function         app_mgr_write_remote_devices
**
** Description      This function is used to write the XML bluetooth remote device file
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_write_remote_devices(void)
{
    int status;

    status = app_xml_write_db(APP_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
        APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    if (status < 0)
    {
        APP_ERROR1("app_xml_write_db failed:%d", status);
        return -1;
    }
    else
    {
        APP_DEBUG0("app_xml_write_db ok");
    }
    return 0;
}


/*******************************************************************************
**
** Function         app_mgr_set_bt_config
**
** Description      This function is used to get the bluetooth configuration
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_set_bt_config(BOOLEAN enable)
{
    int status;
    tBSA_DM_SET_CONFIG bt_config;

    /* Init config parameter */
    status = BSA_DmSetConfigInit(&bt_config);

    /*
    * Set bluetooth configuration (from XML file)
    */
    bt_config.enable = enable;
    bt_config.discoverable = app_xml_config.discoverable;
    bt_config.connectable = app_xml_config.connectable;
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
** Function         app_mgr_get_bt_config
**
** Description      This function is used to get the bluetooth configuration
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_get_bt_config(void)
{
    int status;
    tBSA_DM_GET_CONFIG bt_config;

    /*
    * Get bluetooth configuration
    */
    status = BSA_DmGetConfigInit(&bt_config);
    status = BSA_DmGetConfig(&bt_config);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmGetConfig failed:%d", status);
        return(-1);
    }
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

    return 0;

}

/*******************************************************************************
**
** Function         app_mgr_security_callback
**
** Description      Security callback
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_mgr_security_callback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data)
{
    int status;
    int indexDisc;
    tBSA_SEC_PIN_CODE_REPLY pin_code_reply;
    tBSA_SEC_AUTH_REPLY autorize_reply;

    APP_DEBUG1("event:%d", event);

    switch(event)
    {
    case BSA_SEC_LINK_UP_EVT:       /* A device is physically connected (for info) */
        APP_DEBUG1("BSA_SEC_LINK_UP_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->link_up.bd_addr[0], p_data->link_up.bd_addr[1],
            p_data->link_up.bd_addr[2], p_data->link_up.bd_addr[3],
            p_data->link_up.bd_addr[4], p_data->link_up.bd_addr[5]);
        APP_DEBUG1("ClassOfDevice:%02x:%02x:%02x => %s",
            p_data->link_up.class_of_device[0],
            p_data->link_up.class_of_device[1],
            p_data->link_up.class_of_device[2],
            app_get_cod_string(p_data->link_up.class_of_device));
        break;
    case BSA_SEC_LINK_DOWN_EVT:     /* A device is physically disconnected (for info)*/
        APP_DEBUG1("BSA_SEC_LINK_DOWN_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->link_down.bd_addr[0], p_data->link_down.bd_addr[1],
            p_data->link_down.bd_addr[2], p_data->link_down.bd_addr[3],
            p_data->link_down.bd_addr[4], p_data->link_down.bd_addr[5]);
        APP_DEBUG1("Reason: %d", p_data->link_down.status);
        break;

    case BSA_SEC_PIN_REQ_EVT:
        APP_DEBUG1("BSA_SEC_PIN_REQ_EVT (Pin Code Request) received from: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->pin_req.bd_addr[0], p_data->pin_req.bd_addr[1],
            p_data->pin_req.bd_addr[2], p_data->pin_req.bd_addr[3],
            p_data->pin_req.bd_addr[4],p_data->pin_req.bd_addr[5]);

        APP_DEBUG1("call BSA_SecPinCodeReply pin:%s len:%d",
            app_xml_config.pin_code, app_xml_config.pin_len);

        BSA_SecPinCodeReplyInit(&pin_code_reply);
        bdcpy(pin_code_reply.bd_addr, p_data->pin_req.bd_addr);
        pin_code_reply.pin_len = app_xml_config.pin_len;
        strncpy((char *)pin_code_reply.pin_code, (char *)app_xml_config.pin_code,
            app_xml_config.pin_len);
        /* note that this code will not work if pin_len = 16 */
        pin_code_reply.pin_code[PIN_CODE_LEN-1] = '\0';
        status = BSA_SecPinCodeReply(&pin_code_reply);
        break;

    case BSA_SEC_AUTH_CMPL_EVT:
        APP_DEBUG1("BSA_SEC_AUTH_CMPL_EVT (name=%s,success=%d)",
            p_data->auth_cmpl.bd_name, p_data->auth_cmpl.success);
        if (!p_data->auth_cmpl.success)
        {
            APP_DEBUG1("    fail_reason=%d", p_data->auth_cmpl.fail_reason);
        }
        APP_DEBUG1("    bd_addr:%02x:%02x:%02x:%02x:%02x:%02x",
            p_data->auth_cmpl.bd_addr[0], p_data->auth_cmpl.bd_addr[1], p_data->auth_cmpl.bd_addr[2],
            p_data->auth_cmpl.bd_addr[3], p_data->auth_cmpl.bd_addr[4], p_data->auth_cmpl.bd_addr[5]);
        if (p_data->auth_cmpl.key_present != FALSE)
        {
            APP_DEBUG1("    LinkKey:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->auth_cmpl.key[0], p_data->auth_cmpl.key[1], p_data->auth_cmpl.key[2], p_data->auth_cmpl.key[3],
                p_data->auth_cmpl.key[4], p_data->auth_cmpl.key[5], p_data->auth_cmpl.key[6], p_data->auth_cmpl.key[7],
                p_data->auth_cmpl.key[8], p_data->auth_cmpl.key[9], p_data->auth_cmpl.key[10], p_data->auth_cmpl.key[11],
                p_data->auth_cmpl.key[12], p_data->auth_cmpl.key[13], p_data->auth_cmpl.key[14], p_data->auth_cmpl.key[15]);
        }

        /* If success */
        if (p_data->auth_cmpl.success != 0)
        {
            /* Read the Remote device xml file to have a fresh view */
            app_mgr_read_remote_devices();

            app_xml_update_name_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                p_data->auth_cmpl.bd_addr,
                p_data->auth_cmpl.bd_name);

            if (p_data->auth_cmpl.key_present != FALSE)
            {
                app_xml_update_key_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                    p_data->auth_cmpl.bd_addr,
                    p_data->auth_cmpl.key,
                    p_data->auth_cmpl.key_type);
            }

            /* Unfortunately, the BSA_SEC_AUTH_CMPL_EVT does not contain COD, let's look in the Discovery database */
            for (indexDisc = 0 ; indexDisc < APP_DISC_NB_DEVICES ; indexDisc++)
            {
                if ((app_discovery_cb.devs[indexDisc].in_use != FALSE) &&
                    (bdcmp(app_discovery_cb.devs[indexDisc].device.bd_addr, p_data->auth_cmpl.bd_addr) == 0))

                {
                    app_xml_update_cod_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                        p_data->auth_cmpl.bd_addr,
                        app_discovery_cb.devs[indexDisc].device.class_of_device);
                }
            }

            status = app_mgr_write_remote_devices();
            if (status < 0)
            {
                APP_ERROR1("app_mgr_write_remote_devices failed:%d", status);
            }

            /* Start device info discovery */
            app_disc_start_dev_info(p_data->auth_cmpl.bd_addr, NULL);
        }
        break;

    case BSA_SEC_BOND_CANCEL_CMPL_EVT:
        APP_DEBUG1("BSA_SEC_BOND_CANCEL_CMPL_EVT status=%d",
            p_data->bond_cancel.status);
        break;

    case BSA_SEC_AUTHORIZE_EVT:  /* Authorization request */
        APP_DEBUG0("BSA_SEC_AUTHORIZE_EVT");
        APP_DEBUG1("    Remote device:%s", p_data->authorize.bd_name);
        APP_DEBUG1("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->authorize.bd_addr[0], p_data->authorize.bd_addr[1],
            p_data->authorize.bd_addr[2], p_data->authorize.bd_addr[3],
            p_data->authorize.bd_addr[4],p_data->authorize.bd_addr[5]);
        APP_DEBUG1("    Request access to service:%x (%s)",
            (int)p_data->authorize.service,
            app_service_id_to_string(p_data->authorize.service));
        APP_DEBUG0("    Access Granted (permanently)");
        status = BSA_SecAuthorizeReplyInit(&autorize_reply);
        bdcpy(autorize_reply.bd_addr, p_data->authorize.bd_addr);
        autorize_reply.trusted_service = p_data->authorize.service;
        autorize_reply.auth = BSA_SEC_AUTH_PERM;
        status = BSA_SecAuthorizeReply(&autorize_reply);
        /*
        * Update XML database
        */
        /* Read the Remote device xml file to have a fresh view */
        app_mgr_read_remote_devices();
        /* Add AV service for this devices in XML database */
        app_xml_add_trusted_services_db(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->authorize.bd_addr,
            1 << p_data->authorize.service);

        if (strlen((char *)p_data->authorize.bd_name) > 0)
            app_xml_update_name_db(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db),
            p_data->authorize.bd_addr, p_data->authorize.bd_name);

        /* Update database => write on disk */
        status = app_mgr_write_remote_devices();
        if (status < 0)
        {
            APP_ERROR1("app_mgr_write_remote_devices failed:%d", status);
        }
        break;

    case BSA_SEC_SP_CFM_REQ_EVT: /* Simple Pairing confirm request */
        APP_DEBUG0("BSA_SEC_SP_CFM_REQ_EVT");
        APP_DEBUG1("    Remote device:%s", p_data->cfm_req.bd_name);
        APP_DEBUG1("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->cfm_req.bd_addr[0], p_data->cfm_req.bd_addr[1],
            p_data->cfm_req.bd_addr[2], p_data->cfm_req.bd_addr[3],
            p_data->cfm_req.bd_addr[4], p_data->cfm_req.bd_addr[5]);
        APP_DEBUG1("    ClassOfDevice:%02x:%02x:%02x => %s",
            p_data->cfm_req.class_of_device[0], p_data->cfm_req.class_of_device[1], p_data->cfm_req.class_of_device[2],
            app_get_cod_string(p_data->cfm_req.class_of_device));
        APP_DEBUG1("    Just Work:%s", p_data->cfm_req.just_works == TRUE ? "TRUE" : "FALSE");
        APP_DEBUG1("    Numeric Value:%d", p_data->cfm_req.num_val);
        APP_DEBUG1("    You must accept or refuse using menu (%d) or (%d)",
            APP_MGR_MENU_SP_ACCEPT, APP_MGR_MENU_SP_REFUSE);
        bdcpy(app_sec_db_addr, p_data->cfm_req.bd_addr);
        app_sec_is_ble = p_data->cfm_req.is_ble;
        break;

    case BSA_SEC_SP_KEY_NOTIF_EVT: /* Simple Pairing Passkey Notification */
        APP_DEBUG0("BSA_SEC_SP_KEY_NOTIF_EVT");
        APP_DEBUG1("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->key_notif.bd_addr[0], p_data->key_notif.bd_addr[1], p_data->key_notif.bd_addr[2],
            p_data->key_notif.bd_addr[3], p_data->key_notif.bd_addr[4], p_data->key_notif.bd_addr[5]);
        APP_DEBUG1("    ClassOfDevice:%02x:%02x:%02x => %s",
            p_data->key_notif.class_of_device[0], p_data->key_notif.class_of_device[1], p_data->key_notif.class_of_device[2],
            app_get_cod_string(p_data->key_notif.class_of_device));
        APP_DEBUG1("    Numeric Value:%d", p_data->key_notif.passkey);
        APP_DEBUG0("    You must enter this value on peer device's keyboard");
        break;

    case BSA_SEC_SP_KEYPRESS_EVT: /* Simple Pairing Key press notification event. */
        APP_DEBUG1("BSA_SEC_SP_KEYPRESS_EVT (type:%d)", p_data->key_press.notif_type);
        APP_DEBUG1("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->key_press.bd_addr[0], p_data->key_press.bd_addr[1], p_data->key_press.bd_addr[2],
            p_data->key_press.bd_addr[3], p_data->key_press.bd_addr[4], p_data->key_press.bd_addr[5]);
        break;

    case BSA_SEC_SP_RMT_OOB_EVT: /* Simple Pairing Remote OOB Data request. */
        APP_DEBUG0("BSA_SEC_SP_RMT_OOB_EVT received");
        APP_DEBUG0("Not Yet Implemented");
        break;

    case BSA_SEC_SUSPENDED_EVT: /* Connection Suspended */
        APP_INFO1("BSA_SEC_SUSPENDED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->suspended.bd_addr[0], p_data->suspended.bd_addr[1],
            p_data->suspended.bd_addr[2], p_data->suspended.bd_addr[3],
            p_data->suspended.bd_addr[4], p_data->suspended.bd_addr[5]);
        break;

    case BSA_SEC_RESUMED_EVT: /* Connection Resumed */
        APP_INFO1("BSA_SEC_RESUMED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
            p_data->resumed.bd_addr[0], p_data->resumed.bd_addr[1],
            p_data->resumed.bd_addr[2], p_data->resumed.bd_addr[3],
            p_data->resumed.bd_addr[4], p_data->resumed.bd_addr[5]);
        break;

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    case BSA_SEC_BLE_KEY_EVT: /* BLE KEY event */
        APP_INFO0("BSA_SEC_BLE_KEY_EVT");
        switch (p_data->ble_key.key_type)
        {
        case BSA_LE_KEY_PENC:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_PENC(%d)", p_data->ble_key.key_type);
            APP_DUMP("LTK", p_data->ble_key.key_value.penc_key.ltk, 16);
            APP_DUMP("RAND", p_data->ble_key.key_value.penc_key.rand, 8);
            APP_DEBUG1("ediv: 0x%x:", p_data->ble_key.key_value.penc_key.ediv);
            APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.penc_key.sec_level);
            APP_DEBUG1("key_size: %d:", p_data->ble_key.key_value.penc_key.key_size);
            break;
        case BSA_LE_KEY_PID:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_PID(%d)", p_data->ble_key.key_type);
            APP_DUMP("IRK", p_data->ble_key.key_value.pid_key.irk, 16);
            APP_DEBUG1("addr_type: 0x%x:", p_data->ble_key.key_value.pid_key.addr_type);
            APP_DEBUG1("static_addr: %02X-%02X-%02X-%02X-%02X-%02X",
                p_data->ble_key.key_value.pid_key.static_addr[0],
                p_data->ble_key.key_value.pid_key.static_addr[1],
                p_data->ble_key.key_value.pid_key.static_addr[2],
                p_data->ble_key.key_value.pid_key.static_addr[3],
                p_data->ble_key.key_value.pid_key.static_addr[4],
                p_data->ble_key.key_value.pid_key.static_addr[5]);
            break;
        case BSA_LE_KEY_PCSRK:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_PCSRK(%d)", p_data->ble_key.key_type);
            APP_DEBUG1("counter: 0x%x:", p_data->ble_key.key_value.pcsrk_key.counter);
            APP_DUMP("CSRK", p_data->ble_key.key_value.pcsrk_key.csrk, 16);
            APP_DEBUG1("sec_level: %d:", p_data->ble_key.key_value.pcsrk_key.sec_level);
            break;
        case BSA_LE_KEY_LCSRK:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_LCSRK(%d)", p_data->ble_key.key_type);
            APP_DEBUG1("counter: 0x%x:", p_data->ble_key.key_value.lcsrk_key.counter);
            APP_DEBUG1("div: %d:", p_data->ble_key.key_value.lcsrk_key.div);
            APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.lcsrk_key.sec_level);
            break;
        case BSA_LE_KEY_LENC:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_LENC(%d)", p_data->ble_key.key_type);
            APP_DEBUG1("div: 0x%x:", p_data->ble_key.key_value.lenc_key.div);
            APP_DEBUG1("key_size: %d:", p_data->ble_key.key_value.lenc_key.key_size);
            APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.lenc_key.sec_level);
            break;
        case BSA_LE_KEY_LID:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_LID(%d)", p_data->ble_key.key_type);
            break;
    default:
            APP_DEBUG1("\t key_type: Unknown key(%d)", p_data->ble_key.key_type);
        break;
    }

        /* Read the Remote device xml file to have a fresh view */
        app_mgr_read_remote_devices();

        app_xml_update_ble_key_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                p_data->ble_key.bd_addr,
                p_data->ble_key.key_value,
                p_data->ble_key.key_type);

        status = app_mgr_write_remote_devices();
        if (status < 0)
        {
            APP_ERROR1("app_mgr_write_remote_devices failed:%d", status);
        }
        break;
#endif

    default:
        APP_ERROR1("unknown event:%d", event);
        break;
    }
    if(g_pCallback)
        g_pCallback(event, p_data);
}

/*******************************************************************************
**
** Function         app_mgr_sec_set_security
**
** Description      Security configuration
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_sec_set_security(void)
{
    int status;
    tBSA_SEC_SET_SECURITY set_security;

    APP_DEBUG0("");

    BSA_SecSetSecurityInit(&set_security);
    set_security.simple_pairing_io_cap = app_xml_config.io_cap;
    set_security.sec_cback = app_mgr_security_callback;
    status = BSA_SecSetSecurity(&set_security);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_SecSetSecurity failed:%d", status);
        return -1;
    }
    return 0;
}


/*******************************************************************************
**
** Function         app_mgr_sp_cfm_reply
**
** Description      Function used to accept/refuse Simple Pairing
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_sp_cfm_reply(BOOLEAN accept, BD_ADDR bd_addr)
{
    int status;
    tBSA_SEC_SP_CFM_REPLY app_sec_sp_cfm_reply;

    APP_DEBUG0("");

    BSA_SecSpCfmReplyInit(&app_sec_sp_cfm_reply);
    app_sec_sp_cfm_reply.accept = accept;
    bdcpy(app_sec_sp_cfm_reply.bd_addr, bd_addr);
    app_sec_sp_cfm_reply.is_ble = app_sec_is_ble;
    status = BSA_SecSpCfmReply(&app_sec_sp_cfm_reply);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_SecSpCfmReply failed:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function         app_mgr_sec_bond
**
** Description      Bond a device
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_sec_bond(void)
{
    int status;
    int device_index;
    tBSA_SEC_BOND sec_bond;

    APP_INFO0("Bluetooth Bond menu:");
    app_disc_display_devices();
    device_index = app_get_choice("Select device");

    if ((device_index >= 0) &&
        (device_index < APP_DISC_NB_DEVICES) &&
        (app_discovery_cb.devs[device_index].in_use != FALSE))
    {
        BSA_SecBondInit(&sec_bond);
        bdcpy(sec_bond.bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
        status = BSA_SecBond(&sec_bond);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_SecBond failed:%d", status);
            return -1;
        }
    }
    else
    {
        APP_ERROR1("Bad device number:%d", device_index);
        return -1;
    }

    return 0;
}

/*******************************************************************************
**
** Function         app_mgr_sec_bond_cancel
**
** Description      Cancel a bonding procedure
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_sec_bond_cancel(void)
{
    int status;
    int device_index;
    tBSA_SEC_BOND_CANCEL sec_bond_cancel;

    APP_INFO0("Bluetooth Bond Cancel menu:");
    app_disc_display_devices();
    device_index = app_get_choice("Select device");

    if ((device_index >= 0) &&
        (device_index < APP_DISC_NB_DEVICES) &&
        (app_discovery_cb.devs[device_index].in_use != FALSE))
    {
        BSA_SecBondCancelInit(&sec_bond_cancel);
        bdcpy(sec_bond_cancel.bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
        status = BSA_SecBondCancel(&sec_bond_cancel);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_SecBondCancel failed:%d", status);
            return -1;
        }
    }
    else
    {
        APP_ERROR1("Bad device number:%d", device_index);
        return -1;
    }

    return 0;
}

/*******************************************************************************
**
** Function         app_mgr_set_discoverable
**
** Description      Set the device discoverable for a specific time
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_mgr_set_discoverable(void)
{
    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        APP_ERROR0("BSA_DmSetConfigInit failed");
        return;
    }
    bsa_dm_set_config.discoverable = 1;
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        APP_ERROR0("BSA_DmSetConfig failed");
        return;
    }
}

/*******************************************************************************
**
** Function         app_mgr_set_non_discoverable
**
** Description      Set the device non discoverable
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_mgr_set_non_discoverable(void)
{
    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        APP_ERROR0("BSA_DmSetConfigInit failed");
        return;
    }
    bsa_dm_set_config.discoverable = 0;
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        APP_ERROR0("BSA_DmSetConfig failed");
        return;
    }
}


/*******************************************************************************
**
** Function         app_mgr_di_discovery
**
** Description      Perform a device Id discovery
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_di_discovery(void)
{
    int index, status;
    BD_ADDR bd_addr;

    do
    {
        APP_INFO0("Bluetooth Manager DI Discovery menu:");
        APP_INFO0("    0 Device from XML database (previously paired)");
        APP_INFO0("    1 Device found in last discovery");
        index = app_get_choice("Select source");
        if (index == 0)
        {
            /* Read the Remote device xml file to have a fresh view */
            app_mgr_read_remote_devices();

            app_xml_display_devices(app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));
            index = app_get_choice("Select device");
            if ((index >= 0) &&
                (index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
                (app_xml_remote_devices_db[index].in_use != FALSE))
            {
                bdcpy(bd_addr, app_xml_remote_devices_db[index].bd_addr);
            }
            else
            {
                break;
            }
        }
        /* Devices from Discovery */
        else if (index == 1)
        {
            app_disc_display_devices();
            index = app_get_choice("Select device");
            if ((index >= 0) &&
                (index < APP_DISC_NB_DEVICES) &&
                (app_discovery_cb.devs[index].in_use != FALSE))
            {
                bdcpy(bd_addr, app_discovery_cb.devs[index].device.bd_addr);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }

        /* Start Discovery Device Info */
        status = app_disc_start_dev_info(bd_addr, NULL);
        return status;
    } while (0);

    APP_ERROR1("Bad Device index:%d", index);
    return -1;
}

/*******************************************************************************
**
** Function         app_mgr_set_local_di
**
** Description      Set local device Id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_set_local_di(void)
{
    tBSA_STATUS bsa_status;
    tBSA_DM_SET_DI di;

    di.vendor_id = 0x000F;
    di.product_id = 0x4356;
    di.primary = TRUE;
    di.vendor_id_source = 0x0001; // 0x0001 - Bluetooth SIG, 0x0002 -- USB Forum
    di.version = 0x0B00; // xJJMN for version JJ.M.N (JJ:major,M:minor,N:sub-minor)

    /* Start Discovery Device Info */
    bsa_status = BSA_DmSetLocalDiRecord(&di);

    return bsa_status;
}

/*******************************************************************************
 **
 ** Function         app_mgr_get_local_di
 **
 ** Description      This function is used to read local primary DI record
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_get_local_di(void)
{
    tBSA_DM_GET_DI bsa_read_di;
    tBSA_STATUS bsa_status;
    char *tmpstr;

    bsa_status = BSA_DmGetLocalDiRecord(&bsa_read_di);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmGetLocalDiRecord failed status:%d", bsa_status);
        return(-1);
    }

    APP_INFO0("Local Primary Device ID --");
    APP_INFO1("\tVendor ID: 0x%04x", bsa_read_di.vendor_id);
    APP_INFO1("\tProduct ID: 0x%04x", bsa_read_di.product_id);
    APP_INFO1("\tMajor version: %d", (bsa_read_di.version & 0xFF00) >> 8);
    APP_INFO1("\tMinor version: %d", (bsa_read_di.version & 0x00F0) >> 4);
    APP_INFO1("\tSub-minor version: %d", bsa_read_di.version & 0x000F);
    APP_INFO1("\t%s a primary record", bsa_read_di.primary ? "is" : "not");
    if (bsa_read_di.vendor_id_source == 0x0001)
        tmpstr = "Bluetooth SIG";
    else if (bsa_read_di.vendor_id_source == 0x0002)
        tmpstr = "USB Forum";
    else
        tmpstr = "unknown";
    APP_INFO1("\tVendor ID source is %s", tmpstr);

    return 0;
}

/*******************************************************************************
**
** Function         app_mgr_open
**
** Description      Open Manager application
**
** Parameters       uipc_path: path to UIPC channels
**
** Returns          The status of the operation
**
*******************************************************************************/
int app_mgr_open(char *uipc_path)
{
    tBSA_MGT_OPEN bsa_open_param;
    int i, status;
    /*
    * Connect to Bluetooth daemon
    */
    BSA_MgtOpenInit(&bsa_open_param);
    bsa_open_param.callback = app_mgr_management_callback;

    if (strlen(uipc_path) >= sizeof(bsa_open_param.uipc_path))
    {
        APP_ERROR1("uipc_path too long to fit API (%s)", uipc_path);
        return -1;
    }
    strncpy((char *)bsa_open_param.uipc_path, uipc_path, sizeof(bsa_open_param.uipc_path));
    bsa_open_param.uipc_path[sizeof(bsa_open_param.uipc_path) -1] = '\0';

    for (i = 0; i < 5; i++)
    {
        APP_DEBUG1("BSA_MgtOpen try %d ...", i);
        status = BSA_MgtOpen(&bsa_open_param);
        if (status == BSA_SUCCESS)
        {
            break;
        }
        else
        {
            APP_ERROR1("BSA_MgtOpen failed:%d, try %d ...", status, i);
            sleep(1);
        }
    }
    if (status != BSA_SUCCESS)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Init XML state machine */
    app_xml_init();

    return status;
}

/*******************************************************************************
**
** Function         app_mgr_config
**
** Description      Configure the BSA server
**
** Parameters       None
**
** Returns          Status of the operation
**
*******************************************************************************/
int app_mgr_config(tSecurity_callback cb)
{
    int                 status;
    int                 index;
    BD_ADDR             local_bd_addr = APP_DEFAULT_BD_ADDR;
    DEV_CLASS           local_class_of_device = APP_DEFAULT_CLASS_OF_DEVICE;
    tBSA_SEC_ADD_DEV    bsa_add_dev_param;
    tBSA_SEC_ADD_SI_DEV bsa_add_si_dev_param;
    struct timeval      tv;
    unsigned int        rand_seed;

    /*
    * The rest of the initialization function must be done
    * for application boot and when Bluetooth is restarted
    */

    /* Example of function to read the XML file which contains the
    * Local Bluetooth configuration
    * */

    g_pCallback = cb;

    status = app_mgr_read_config();
    if (status < 0)
    {
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
            app_mgr_close();
            return status;
        }
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

    /* Example of function to get the Local Bluetooth configuration */
    app_mgr_get_bt_config();

    /* Example of function to set the Local Bluetooth configuration */
    app_mgr_set_bt_config(app_xml_config.enable);

    /* Example of function to set the Bluetooth Security */
    status = app_mgr_sec_set_security();
    if (status < 0)
    {
        APP_ERROR1("app_mgr_sec_set_security failed:%d", status);
        app_mgr_close();
        return status;
    }



    /* Add every devices found in remote device database */
    /* They will be able to connect to our device */
    APP_INFO0("Add all devices found in database");
    for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; index++)
    {
        if (app_xml_remote_devices_db[index].in_use != FALSE)
        {
            APP_INFO1("Adding:%s", app_xml_remote_devices_db[index].name);
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
            BSA_SecAddDevice(&bsa_add_dev_param);
        }
    }

    /* Add stored SI devices to BSA server */
    app_read_xml_si_devices();
    for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_si_devices_db) ; index++)
    {
        if (app_xml_si_devices_db[index].in_use)
        {
            BSA_SecAddSiDevInit(&bsa_add_si_dev_param);
            bdcpy(bsa_add_si_dev_param.bd_addr, app_xml_si_devices_db[index].bd_addr);
            bsa_add_si_dev_param.platform = app_xml_si_devices_db[index].platform;
            BSA_SecAddSiDev(&bsa_add_si_dev_param);
        }
    }

    return 0;
}

/*******************************************************************************
**
 ** Function        app_mgr_change_dual_stack_mode
 **
 ** Description     Toggle Dual Stack Mode (BSA <=> MM/Kernel)
 **
 ** Parameters      None
 **
 ** Returns         Status
 **
 *******************************************************************************/
int app_mgr_change_dual_stack_mode(void)
{
    int choice;
    tBSA_DM_DUAL_STACK_MODE dual_stack_mode;

    APP_INFO1("Current DualStack mode: %s", app_mgr_get_dual_stack_mode_desc());
    APP_INFO0("Select the new DualStack mode:");
    APP_INFO0("\t0: DUAL_STACK_MODE_BSA");
    APP_INFO0("\t1: DUAL_STACK_MODE_MM/Kernel (requires specific BTUSB driver)");
    APP_INFO0("\t2: DUAL_STACK_MODE_BTC (requires specific FW)");
    choice = app_get_choice("Choice");

    switch(choice)
    {
    case 0:
        APP_INFO0("Switching Dual Stack to BSA");
        dual_stack_mode = BSA_DM_DUAL_STACK_MODE_BSA;
        break;

    case 1:
        APP_INFO0("Switching Dual Stack to MM/Kernel");
        dual_stack_mode = BSA_DM_DUAL_STACK_MODE_MM;
        break;

    case 2:
        APP_INFO0("Switching Dual Stack to BTC");
        dual_stack_mode = BSA_DM_DUAL_STACK_MODE_BTC;
        break;

    default:
        APP_ERROR1("Bad DualStack Mode=%d", choice);
        return -1;
    }

    if (app_dm_set_dual_stack_mode(dual_stack_mode) < 0)
    {
        APP_ERROR0("DualStack Switch Failed");
        return -1;
    }

    app_mgr_cb.dual_stack_mode = dual_stack_mode;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_mgr_get_dual_stack_mode_desc
 **
 ** Description     Get Dual Stack Mode description
 **
 ** Parameters      None
 **
 ** Returns         Description string
 **
 *******************************************************************************/
char *app_mgr_get_dual_stack_mode_desc(void)
{
    switch(app_mgr_cb.dual_stack_mode)
    {
    case BSA_DM_DUAL_STACK_MODE_BSA:
        return "DUAL_STACK_MODE_BSA";
    case BSA_DM_DUAL_STACK_MODE_MM:
        return "DUAL_STACK_MODE_MM/Kernel";
    case BSA_DM_DUAL_STACK_MODE_BTC:
        return "DUAL_STACK_MODE_BTC";
    default:
        return "Unknown Dual Stack Mode";
    }
}

/*******************************************************************************
 **
** Function         app_mgr_discovery_test
**
** Description      This function performs a specific discovery
**
** Parameters       None
**
** Returns          int
**
*******************************************************************************/
int app_mgr_discovery_test(void)
{
    int choice;

    APP_INFO0("Select the parameter to modify:");
    APP_INFO0("\t0: Update type");
    APP_INFO0("\t1: Inquiry TX power");
    choice = app_get_choice("Choice");

    switch(choice)
    {
    case 0:
        APP_INFO0("Select which update is needed:");
        APP_INFO1("\t%d: End of discovery (default)", BSA_DISC_UPDATE_END);
        APP_INFO1("\t%d: Upon any modification", BSA_DISC_UPDATE_ANY);
        APP_INFO1("\t%d: Upon Name Reception", BSA_DISC_UPDATE_NAME);
        choice = app_get_choice("");

        return app_disc_start_update((tBSA_DISC_UPDATE)choice);
        break;

    case 1:
        choice = app_get_choice("Enter new Inquiry TX power");

        return app_disc_start_power((INT8)choice);

        break;

    default:
        APP_ERROR0("Unsupported choice value");
        return -1;
        break;
    }

}

/*******************************************************************************
**
** Function         app_mgr_close
**
** Description      This function is used to closes the BSA connection
**
** Parameters       None
**
** Returns          None
**
*******************************************************************************/
void app_mgr_close(void)
{
    tBSA_MGT_CLOSE  bsa_close_param;

    BSA_MgtCloseInit(&bsa_close_param);
    BSA_MgtClose(&bsa_close_param);
}

/*******************************************************************************
 **
 ** Function         app_mgr_read_version
 **
 ** Description      This function is used to read BSA and FW version
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_read_version(void)
{
    tBSA_TM_READ_VERSION bsa_read_version;
    tBSA_STATUS bsa_status;

    bsa_status = BSA_TmReadVersionInit(&bsa_read_version);
    bsa_status = BSA_TmReadVersion(&bsa_read_version);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_TmReadVersion failed status:%d", bsa_status);
        return(-1);
    }

    APP_INFO1("Server status:%d", bsa_read_version.status);
    APP_INFO1("FW Version:%d.%d.%d.%d",
            bsa_read_version.fw_version.major,
            bsa_read_version.fw_version.minor,
            bsa_read_version.fw_version.build,
            bsa_read_version.fw_version.config);
    APP_INFO1("BSA Server Version:%s", bsa_read_version.bsa_server_version);
    return 0;
}

#ifndef BSA_SAMPLE_APP
/*******************************************************************************
**
** Function         main
**
** Description      This is the main function
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int main(int argc, char **argv)
{
    int choice;
    int i;
    unsigned int afh_ch1,afh_ch2;
    BOOLEAN no_init = FALSE;
    int mode;

    /* Check the CLI parameters */
    for (i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        if (*arg != '-')
        {
            APP_ERROR1("Unsupported parameter #%d : %s", i+1, argv[i]);
            exit(-1);
        }
        /* Bypass the first '-' char */
        arg++;
        /* check if the second char is also a '-'char */
        if (*arg == '-') arg++;
        switch (*arg)
        {
        case 'n': /* No init */
            no_init = TRUE;
            break;
        default:
            break;
        }
    }


    /* Init App manager */
    if (!no_init)
    {
        /* The TRUE parameter indicate Application start => open BSA connection */
        if (app_mgr_open(APP_DEFAULT_UIPC_PATH))
        {
            APP_ERROR0("Couldn't open successfully, exiting");
            exit(-1);
        }
        if (app_mgr_config())
        {
            APP_ERROR0("Couldn't configure successfully, exiting");
            exit(-1);
        }
    }

    /* Display FW versions */
    app_mgr_read_version();

    /* Get the current Stack mode */
    mode = app_dm_get_dual_stack_mode();
    if (mode < 0)
    {
        APP_ERROR0("app_dm_get_dual_stack_mode failed");
        return -1;
    }
    else
    {
        /* Save the current DualStack mode */
        app_mgr_cb.dual_stack_mode = mode;
        APP_INFO1("Current DualStack mode:%s", app_mgr_get_dual_stack_mode_desc());
    }

    do
    {
        app_mgr_display_main_menu();

        choice = app_get_choice("Select action");

        switch(choice)
        {
        case APP_MGR_MENU_ABORT_DISC:
            app_disc_abort();
            break;

        case APP_MGR_MENU_DISCOVERY:
            /* Example to perform Device discovery */
            app_disc_start_regular(NULL);
            break;

        case APP_MGR_MENU_DISCOVERY_TEST:
            /* Example to perform specific discovery */
            app_mgr_discovery_test();
            break;

        case APP_MGR_MENU_BOND:
            /* Example to Bound  */
            app_mgr_sec_bond();
            break;

        case APP_MGR_MENU_BOND_CANCEL:
            /* Example to cancel Bound  */
            app_mgr_sec_bond_cancel();
            break;

        case APP_MGR_MENU_SVC_DISCOVERY:
            /* Example to perform Device Services discovery */
            app_disc_start_services(BSA_ALL_SERVICE_MASK);
            break;

        case APP_MGR_MENU_DI_DISCOVERY:
            /* Example to perform Device Id discovery */
            /* Note that the synchronization (wait discovery complete) must be adapted to the application */
            app_mgr_di_discovery();
            break;

        case APP_MGR_MENU_SET_LOCAL_DI:
            /* Example to perform Set local Device Id */
            app_mgr_set_local_di();
            break;

        case APP_MGR_MENU_GET_LOCAL_DI:
            /* Example to perform Get local PRIMARY Device Id */
            app_mgr_get_local_di();
            break;

        case APP_MGR_MENU_STOP_BT:
            /* Example of function to Stop the Local Bluetooth device */
            app_mgr_set_bt_config(FALSE);
            break;

        case APP_MGR_MENU_START_BT:
            /* Example of function to Restart the Local Bluetooth device */
            app_mgr_set_bt_config(TRUE);
            break;

        case APP_MGR_MENU_SP_ACCEPT:
            /* Example of function to Accept Simple Pairing */
            app_mgr_sp_cfm_reply(TRUE, app_sec_db_addr);
            break;

        case APP_MGR_MENU_SP_REFUSE:
            /* Example of function to Refuse Simple Pairing */
            app_mgr_sp_cfm_reply(FALSE, app_sec_db_addr);
            break;

        case APP_MGR_MENU_READ_CONFIG:
            /* Read and print the device current configuration */
            app_mgr_get_bt_config();
            break;

        case APP_MGR_MENU_DISCOVERABLE:
            /* Set the device discoverable */
            app_mgr_set_discoverable();
            break;

        case APP_MGR_MENU_NON_DISCOVERABLE:
            /* Set the device non discoverable */
            app_mgr_set_non_discoverable();
            break;

        case APP_MGR_MENU_SET_AFH_CHANNELS:
            /* Choose first channel*/
            APP_INFO0("    Enter First AFH CHannel (79 = complete channel map):");
            afh_ch1 = app_get_choice("");
            APP_INFO0("    Enter Last AFH CHannel:");
            afh_ch2 = app_get_choice("");
            app_dm_set_channel_map(afh_ch1,afh_ch2);
            break;

        case APP_MGR_MENU_CLASS2:
            app_dm_set_tx_class(BSA_DM_TX_POWER_CLASS_2);
            break;

        case APP_MGR_MENU_CLASS1_5:
            app_dm_set_tx_class(BSA_DM_TX_POWER_CLASS_1_5);
            break;

        case APP_MGR_MENU_DUAL_STACK:
            app_mgr_change_dual_stack_mode();
            break;

        case APP_MGR_MENU_KILL_SERVER:
            app_mgt_kill_server();
            break;

        case APP_MGR_MENU_MGR_INIT:
            if (app_mgr_open(APP_DEFAULT_UIPC_PATH))
            {
                APP_ERROR0("app_mgr_open failed");
            }
            else if (app_mgr_config())
            {
                APP_ERROR0("app_mgr_config failed");
            }
            break;

        case APP_MGR_MENU_MGR_CLOSE:
            app_mgr_close();
            break;

        case APP_MGR_MENU_QUIT:
            APP_INFO0("Bye Bye");
            break;

        default:
            APP_ERROR1("Unknown choice:%d", choice);
            break;
        }
    } while (choice != APP_MGR_MENU_QUIT);      /* While user don't exit application */

    app_mgr_close();
    return 0;
}

#endif
