/*****************************************************************************
**
**  Name:           app_nsa.c
**
**  Description:    Bluetooth NSA application
**
**  Copyright (c) 2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bsa_api.h"
#include "app_nsa.h"
#include "app_xml_utils.h"
#include "app_utils.h"


/*
 * Global Variables
 */
tAPP_NSA_CB app_nsa_cb;


/*
 * Local functions
 */
tAPP_NSA_IF *app_nsa_alloc_if(void);
void app_nsa_free_if(tAPP_NSA_IF *p_app_if);
void app_nsa_display_if(void);
tAPP_NSA_IF *app_nsa_find_if_from_port(tBSA_NSA_PORT port);

/*******************************************************************************
 **
 ** Function        app_nsa_init
 **
 ** Description     This is the main init function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_nsa_init(void)
{
    memset(&app_nsa_cb, 0, sizeof(app_nsa_cb));
    return 0;
}


/*******************************************************************************
 **
 ** Function        app_nsa_exit
 **
 ** Description     This is the exit function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_nsa_exit(void)
{
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_nsa_add_if
 **
 ** Description      Add NSA Interface
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_nsa_add_if(void)
{
    tBSA_NSA_ADD_IF nsa_add_if;
    tBSA_STATUS status;
    int device_index;
    int in_report_id, out_report_id;
    BD_ADDR bd_addr;
    tAPP_NSA_IF *p_nsa_if;

    /* Read the xml file which contains all the bonded devices */
    app_read_xml_remote_devices();

    app_xml_display_devices(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    device_index = app_get_choice("Select device");
    if ((device_index >= 0) &&
        (device_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
        (app_xml_remote_devices_db[device_index].in_use != FALSE))
    {
        bdcpy(bd_addr, app_xml_remote_devices_db[device_index].bd_addr);

    }
    else
    {
        APP_ERROR0("Bad device index");
        return -1;
    }

    /* Allocate an Interface */
    p_nsa_if = app_nsa_alloc_if();
    if (p_nsa_if == NULL)
    {
        APP_ERROR0("Cannot allocate NSA interface");
        return -1;
    }

    in_report_id = app_get_choice("Enter Input Report ID (HID/NCI encapsulation)");
    out_report_id = app_get_choice("Enter Output Report ID (HID/NCI encapsulation)");

    p_nsa_if->in_report_id = in_report_id;
    p_nsa_if->out_report_id = out_report_id;
    bdcpy(p_nsa_if->bd_addr, bd_addr);

    BSA_NsaAddIfInit(&nsa_add_if);

    nsa_add_if.type = BSA_NSA_TYPE_HID_IPC;
    nsa_add_if.nsa_config.hid_ipc.in_report_id = (UINT8)in_report_id;
    nsa_add_if.nsa_config.hid_ipc.out_report_id = (UINT8)out_report_id;
    bdcpy(nsa_add_if.nsa_config.hid_ipc.bd_addr, bd_addr);

    status = BSA_NsaAddIf(&nsa_add_if);
    if (status != BSA_SUCCESS)
    {
        /* Free the Interface */
        APP_ERROR1("Unable to Add NSA Interface:%d", status);
        app_nsa_free_if(p_nsa_if);
    }
    else
    {
        APP_INFO1("NSA Interface Port:%d added", nsa_add_if.port);
        p_nsa_if->port = nsa_add_if.port;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_nsa_remove_if
 **
 ** Description      Remove NSA Interface
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_nsa_remove_if(void)
{
    tBSA_STATUS status;
    tBSA_NSA_REMOVE_IF nsa_remove_if;
    int port;
    tAPP_NSA_IF *p_nsa_if;


    BSA_NsaRemoveIfInit(&nsa_remove_if);

    /* Display the currently allocated ports */
    app_nsa_display_if();

    port = app_get_choice("Enter NSA Port to remove");

    nsa_remove_if.port = (tBSA_NSA_PORT)port;

    status = BSA_NsaRemoveIf(&nsa_remove_if);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Remove NSA Interface:%d", status);
    }
    else
    {
        APP_INFO1("NSA Interface Port:%d removed", port);
        p_nsa_if = app_nsa_find_if_from_port((tBSA_NSA_PORT)port);
        if (p_nsa_if != NULL)
        {
            app_nsa_free_if(p_nsa_if);
        }
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_nsa_alloc_if
 **
 ** Description      Allocate an NSA Interface
 **
 ** Returns          int
 **
 *******************************************************************************/
tAPP_NSA_IF *app_nsa_alloc_if(void)
{
    int idx;
    tAPP_NSA_IF *p_app_if;

    for (idx = 0, p_app_if = app_nsa_cb.interface ; idx < APP_NSA_IF_MAX ; idx++, p_app_if++)
    {
        if (p_app_if->in_use == FALSE)
        {
            p_app_if->in_use = TRUE;
            return p_app_if;
        }
    }
    APP_ERROR0("app_nsa_alloc_if failed");
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_nsa_free_if
 **
 ** Description      Allocate an NSA Interface
 **
 ** Returns          int
 **
 *******************************************************************************/
void app_nsa_free_if(tAPP_NSA_IF *p_app_if)
{
    if (p_app_if == NULL)
    {
        APP_ERROR0("app_nsa_free_if null pointer");
        return;
    }
    if (p_app_if->in_use == FALSE)
    {
        APP_ERROR0("app_nsa_free_if this interface was not allocated");
        return;
    }
    p_app_if->in_use = FALSE;
}

/*******************************************************************************
 **
 ** Function         app_nsa_find_if_from_port
 **
 ** Description      Find an NSA Interface from a port number
 **
 ** Returns          int
 **
 *******************************************************************************/
tAPP_NSA_IF *app_nsa_find_if_from_port(tBSA_NSA_PORT port)
{
    int idx;
    tAPP_NSA_IF *p_app_if;

    for (idx = 0, p_app_if = app_nsa_cb.interface ; idx < APP_NSA_IF_MAX ; idx++, p_app_if++)
    {
        if ((p_app_if->in_use != FALSE) &&
            (p_app_if->port == port))
        {
            return p_app_if;
        }
    }
    APP_ERROR1("app_nsa_find_if_from_port no interface matching port:%d found", port);
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_nsa_display_if
 **
 ** Description      Display NSA Interfaces
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_nsa_display_if(void)
{
    int idx;
    tAPP_NSA_IF *p_app_if;

    for (idx = 0, p_app_if = app_nsa_cb.interface ; idx < APP_NSA_IF_MAX ; idx++, p_app_if++)
    {
        if (p_app_if->in_use != FALSE)
        {
            APP_INFO1("NSA Interface Port:%d BdAddr:%02X-%02X-%02X-%02X-%02X-%02X InReportId:x%x OutReportId:%x",
                    p_app_if->port,
                    p_app_if->bd_addr[0], p_app_if->bd_addr[1],p_app_if->bd_addr[2],
                    p_app_if->bd_addr[3], p_app_if->bd_addr[4],p_app_if->bd_addr[5],
                    p_app_if->in_report_id, p_app_if->out_report_id);
        }
    }
}

