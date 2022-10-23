/*****************************************************************************
 **
 **  Name:           app_hl_xml.c
 **
 **  Description:    This module contains utility functions to access Health
 **                  saved parameters
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "bsa_api.h"
#include "app_hl.h"
#include "app_hl_db.h"
#include "app_hl_xml.h"
#include "app_xml_utils.h"
#include "app_utils.h"

#include "nanoxml.h"

#ifndef tAPP_HL_XML_FILENAME
#define tAPP_HL_XML_FILENAME "./bt_hl_devices.xml"
#endif

#define APP_HL_ROOT_KEY                 "Broadcom_Bluetooth_Health_MDL"

#define APP_HL_APP_KEY                  "application_handle"

#define APP_HL_MDL_KEY                  "mdl_configuration_index"

#define APP_HL_MDL_TIME_KEY             "time"
#define APP_HL_MDL_MTU_KEY              "mtu"
#define APP_HL_MDL_BDADDR_KEY           "bd_addr"
#define APP_HL_MDL_FCS_KEY              "fcs"
#define APP_HL_MDL_MDL_ID_KEY           "mdl_id"
#define APP_HL_MDL_LOCAL_MDEP_ID_KEY    "local_medp_id"
#define APP_HL_MDL_ROLE_KEY             "role"
#define APP_HL_MDL_DCH_MODE_KEY         "dch_mode"

typedef enum
{
    /* MDL database tag */
    MDL_UNKNOWN_TAG,
    APP_HDL_TAG,
    CFG_IDX_TAG,
    MDL_TIME_TAG,
    MDL_MTU_TAG,
    MDL_BDADDR_TAG,
    MDL_FCS_TAG,
    MDL_MDL_ID_TAG,
    MDL_LOCAL_MDEP_ID_TAG,
    MDL_ROLE_TAG,
    MDL_DCH_MODE_TAG
} tAPP_HL_XML_TAG;

typedef struct
{
    tAPP_HL_XML_TAG tag; /* Tag extracted */
    int app_index; /* application index */
    int mdl_index; /* mdl index */
    tAPP_HL_APP_CFG *p_app_cfg; /* Location to save the read database */
} APP_HL_XML_CB;


static void app_hl_xml_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);
static void app_hl_xml_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);
static void app_hl_xml_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len);
static void app_hl_xml_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more);
static void app_hl_xml_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more);

/*
 * Global variables
 */
APP_HL_XML_CB app_hl_xml_cb;


/*******************************************************************************
 **
 ** Function        app_hl_xml_init
 **
 ** Description     Initialize XML parameter system (nothing for the moment)
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_hl_xml_init(void)
{
    return 0;
}

/*
 * Code based on NANOXML library
 */

/*******************************************************************************
 **
 ** Function        app_hl_xml_read
 **
 ** Description     Load the MDL data base from  XML file
 **
 ** Parameters      p_app_cfg: Application Configuration buffer
 **
 ** Returns         0 if ok, otherwise -1
 **
 *******************************************************************************/
int app_hl_xml_read(tAPP_HL_APP_CFG *p_app_cfg)
{
    nxml_t nxmlHandle;
    nxml_settings nxmlSettings;
    char *endp = NULL;
    int rc;
    int maxLen;
    char *file_buffer;
    int fd;
    int len;
    int app_index;
    BOOLEAN create_empty = FALSE;

    /* Open the Config file */
    if ((fd = open(tAPP_HL_XML_FILENAME, O_RDONLY)) >= 0)
    {
        /* Get the length of the device file */
        maxLen = app_file_size(fd);
        if (maxLen == 0)
        {
            APP_ERROR1("file: %s is empty", tAPP_HL_XML_FILENAME);
            close(fd);
            create_empty = TRUE;
        }
        else if (maxLen < 0)
        {
            APP_ERROR1("cannot get file size of: %s", tAPP_HL_XML_FILENAME);
            close(fd);
            create_empty = TRUE;
        }

        if (create_empty == FALSE)
        {
            file_buffer = malloc(maxLen + 1);
            if (file_buffer == NULL)
            {
                APP_ERROR1("cannot alloc: %d bytes", maxLen);
                close(fd);
                create_empty = TRUE;
            }
        }

        if (create_empty == FALSE)
        {
            /* read the XML file */
            len = read(fd, (void *) file_buffer, maxLen);
            file_buffer[maxLen] = '\0';

            close(fd);

            if (len != maxLen)
            {
                free(file_buffer);
                APP_ERROR1("not able to read complete file:%d/%d",
                        len, maxLen);
                create_empty = TRUE;
            }
        }
    }
    else
    {
        create_empty = TRUE;
        APP_DEBUG1("cannot open:%s in read mode", tAPP_HL_XML_FILENAME);
    }

    /* If something has been read */
    if (create_empty == FALSE)
    {
        /* set callback function handlers */
        nxmlSettings.tag_begin = app_hl_xml_tagBeginCallbackFunc;
        nxmlSettings.tag_end = app_hl_xml_tagEndCallbackFunc;
        nxmlSettings.attribute_begin = app_hl_xml_attrBeginCallbackFunc;
        nxmlSettings.attribute_value = app_hl_xml_attrValueCallbackFunc;
        nxmlSettings.data = app_hl_xml_dataCallbackFunc;

        if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
        {
            free(file_buffer);
            APP_ERROR1("cannot open Nanoxml :%d", rc);
            create_empty = TRUE;
        }

        if (create_empty == FALSE)
            {
            app_hl_xml_cb.p_app_cfg = p_app_cfg;
            app_hl_xml_cb.app_index = -1;
            app_hl_xml_cb.mdl_index = -1;
            app_hl_xml_cb.tag = MDL_UNKNOWN_TAG;

            /* Clear database */
            for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
            {
                p_app_cfg[app_index].app_handle = BSA_HL_BAD_APP_HANDLE;
                memset(p_app_cfg[app_index].mdl_cfg, 0, sizeof(p_app_cfg[app_index].mdl_cfg));
            }

            /* Push our data into the nanoxml parser, it will then call our callback funcs */
            rc = xmlWrite(nxmlHandle, file_buffer, maxLen, &endp);
            if (rc != 1)
            {
                APP_ERROR1("xmlWrite returns :%d", rc);
                create_empty = TRUE;
            }

            xmlClose(nxmlHandle);
            free(file_buffer);
        }
    }

    /* in case of failure => create an empty database */
    if (create_empty)
    {
        APP_DEBUG0("Create an empty HL database");
        memset(p_app_cfg, 0, sizeof(*p_app_cfg));
        for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
        {
            p_app_cfg[app_index].app_handle = BSA_HL_BAD_APP_HANDLE;
        }
        if (app_hl_xml_write(p_app_cfg) < 0)
        {
            APP_ERROR0("Unable to create an empty HL database");
            return -1;
        }
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function        app_hl_xml_write
 **
 ** Description     Writes the database file in file system or XML, creates one if it doesn't exist
 **
 ** Parameters      p_app_cfg: Application Configuration to save to file
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hl_xml_write(tAPP_HL_APP_CFG *p_app_cfg)
{
    int fd;
    int app_index;
    int mdl_index;
    tBSA_HL_MDL_CFG *p_cfg;

    if ((fd = open(tAPP_HL_XML_FILENAME, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        APP_ERROR1("Error opening/creating %s file", tAPP_HL_XML_FILENAME);
        return -1;
    }

    app_xml_open_tag(fd, APP_HL_ROOT_KEY, TRUE);

    /* For every Application in device database */
    for (app_index = 0; app_index < APP_HL_LOCAL_APP_MAX; app_index++)
    {
        /* If this application handle is in use */
        if (p_app_cfg[app_index].app_handle != BSA_HL_BAD_APP_HANDLE)
        {
            app_xml_open_tag_with_value(fd, APP_HL_APP_KEY, p_app_cfg[app_index].app_handle);

            /* For every MDL */
            p_cfg = &p_app_cfg[app_index].mdl_cfg[0];
            for (mdl_index = 0; mdl_index < BSA_HL_NUM_MDL_CFGS; mdl_index++, p_cfg++)
            {
                if (p_cfg->active)
                {
                    app_xml_open_tag_with_value(fd, APP_HL_MDL_KEY, mdl_index);

                    app_xml_open_tag(fd, APP_HL_MDL_BDADDR_KEY, FALSE);
                    app_xml_write_data(fd, p_cfg->peer_bd_addr, sizeof(p_cfg->peer_bd_addr), FALSE);
                    app_xml_close_tag(fd, APP_HL_MDL_BDADDR_KEY, FALSE);

                    app_xml_open_close_tag_with_value(fd, APP_HL_MDL_TIME_KEY, p_cfg->time);

                    app_xml_open_close_tag_with_value(fd, APP_HL_MDL_MTU_KEY, p_cfg->mtu);

                    app_xml_open_close_tag_with_value(fd, APP_HL_MDL_FCS_KEY, p_cfg->fcs);

                    app_xml_open_close_tag_with_value(fd, APP_HL_MDL_MDL_ID_KEY, p_cfg->mdl_id);

                    app_xml_open_close_tag_with_value(fd, APP_HL_MDL_LOCAL_MDEP_ID_KEY, p_cfg->local_mdep_id);

                    app_xml_open_close_tag_with_value(fd, APP_HL_MDL_ROLE_KEY, p_cfg->local_mdep_role);

                    app_xml_open_close_tag_with_value(fd, APP_HL_MDL_DCH_MODE_KEY, p_cfg->dch_mode);

                    app_xml_close_tag(fd, APP_HL_MDL_KEY, TRUE);
                }
            }
            app_xml_close_tag(fd, APP_HL_APP_KEY, TRUE);
        }
    }

    app_xml_close_tag(fd, APP_HL_ROOT_KEY, TRUE);

    close(fd);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hl_xml_tagBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hl_xml_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
    if (strncmp(tagName, APP_HL_ROOT_KEY, strlen(APP_HL_ROOT_KEY)) == 0)
    {
        /* nothing to do */
    }
    else if (strncmp(tagName, APP_HL_APP_KEY, strlen(APP_HL_APP_KEY)) == 0)
    {
        app_hl_xml_cb.tag = APP_HDL_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_KEY, strlen(APP_HL_MDL_KEY)) == 0)
    {
        app_hl_xml_cb.tag = CFG_IDX_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_TIME_KEY, strlen(APP_HL_MDL_TIME_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_TIME_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_MTU_KEY, strlen(APP_HL_MDL_MTU_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_MTU_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_BDADDR_KEY, strlen(APP_HL_MDL_BDADDR_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_BDADDR_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_FCS_KEY, strlen(APP_HL_MDL_FCS_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_FCS_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_MDL_ID_KEY, strlen(APP_HL_MDL_MDL_ID_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_MDL_ID_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_LOCAL_MDEP_ID_KEY, strlen(APP_HL_MDL_LOCAL_MDEP_ID_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_LOCAL_MDEP_ID_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_ROLE_KEY, strlen(APP_HL_MDL_ROLE_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_ROLE_TAG;
    }
    else if (strncmp(tagName, APP_HL_MDL_DCH_MODE_KEY, strlen(APP_HL_MDL_DCH_MODE_KEY)) == 0)
    {
        app_hl_xml_cb.tag = MDL_DCH_MODE_TAG;
    }
    else
    {
        APP_ERROR1("Unknown Tag:%s", tagName);
        app_hl_xml_cb.tag = MDL_UNKNOWN_TAG;
    }
}

/*******************************************************************************
 **
 ** Function        app_hl_xml_tagEndCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hl_xml_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
}

/*******************************************************************************
 **
 ** Function        app_hl_xml_attrBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hl_xml_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len)
{
    /* the only attribute supported is "value" */
    if (strncmp(attrName, "value", len) != 0)
    {
        APP_ERROR1("Unsupported attribute (%s)", attrName);
    }
}

/*******************************************************************************
 **
 ** Function        app_hl_xml_attrValueCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hl_xml_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more)
{
    tBSA_HL_MDL_CFG *p_mdl_cfg;
    int value, index;

    /* Get pointer on mdl configuration */
    p_mdl_cfg = &app_hl_xml_cb.p_app_cfg[app_hl_xml_cb.app_index].mdl_cfg[app_hl_xml_cb.mdl_index];

    value = app_xml_read_value(attrValue, len);

    switch (app_hl_xml_cb.tag)
    {
    case APP_HDL_TAG:
        app_hl_xml_cb.app_index = -1;
        /* Look for a free application */
        for (index = 0 ; index < APP_HL_LOCAL_APP_MAX; index++)
        {
            if (app_hl_xml_cb.p_app_cfg[index].app_handle == BSA_HL_BAD_APP_HANDLE)
            {
                app_hl_xml_cb.p_app_cfg[index].app_handle = value;
                app_hl_xml_cb.app_index = index;
                break;
            }
        }
        if (app_hl_xml_cb.app_index == -1)
        {
            APP_ERROR1("No free application found to save app_hdl:%d", value);
        }
        break;

    case CFG_IDX_TAG:
        if (app_hl_xml_cb.app_index != -1)
        {
            /* Check if this mdl_index is free */
            if (app_hl_xml_cb.p_app_cfg[app_hl_xml_cb.app_index].mdl_cfg[value].active == FALSE)
            {
                    app_hl_xml_cb.mdl_index = value;
                    app_hl_xml_cb.p_app_cfg[app_hl_xml_cb.app_index].mdl_cfg[value].active = TRUE;
            }
            else
            {
                APP_ERROR1("app_handle:%d mdl_index:%d was in use", app_hl_xml_cb.app_index, value);
                app_hl_xml_cb.mdl_index = -1;
            }
        }
        else
        {
            APP_ERROR1("bad app_index for mdl_index:%d", value);
            app_hl_xml_cb.mdl_index = -1;
        }
        break;

    case MDL_TIME_TAG:
        p_mdl_cfg->time = value;
        break;

    case MDL_MTU_TAG:
        p_mdl_cfg->mtu = value;
        break;

    case MDL_FCS_TAG:
        p_mdl_cfg->fcs = value;
        break;

    case MDL_MDL_ID_TAG:
        p_mdl_cfg->mdl_id = value;
        break;

    case MDL_LOCAL_MDEP_ID_TAG:
        p_mdl_cfg->local_mdep_id = value;
        break;

    case MDL_ROLE_TAG:
        p_mdl_cfg->local_mdep_role = value;
        break;

    case MDL_DCH_MODE_TAG:
        p_mdl_cfg->dch_mode = value;
        break;

    default:
        APP_ERROR1("Ignoring Data for tag:%d", app_hl_xml_cb.tag);
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_hl_xml_dataCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hl_xml_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more)
{
    tBSA_HL_MDL_CFG *p_mdl_cfg;

    if ((app_hl_xml_cb.tag == MDL_UNKNOWN_TAG) ||
        (app_hl_xml_cb.app_index == -1) ||
        (app_hl_xml_cb.mdl_index == -1))
    {
        APP_ERROR1("Ignoring Data for tag:%d AppIdx:%d MdlIdx:%d",
                app_hl_xml_cb.tag,
                app_hl_xml_cb.app_index,
                app_hl_xml_cb.mdl_index);
        return;
    }

    /* Get pointer on mdl configuration */
    p_mdl_cfg = &app_hl_xml_cb.p_app_cfg[app_hl_xml_cb.app_index].mdl_cfg[app_hl_xml_cb.mdl_index];

    switch (app_hl_xml_cb.tag)
    {
    case MDL_BDADDR_TAG:
        app_xml_read_data_length(p_mdl_cfg->peer_bd_addr, sizeof(p_mdl_cfg->peer_bd_addr),
                data, len);
        break;

    default:
        APP_ERROR1("Ignoring Data for tag:%d", app_hl_xml_cb.tag);
        break;
    }
}

