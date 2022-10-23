/*****************************************************************************
 **
 **  Name:           app_ftc.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2010-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gki.h"
#include "uipc.h"

#include "bsa_api.h"

#include "app_xml_param.h"
#include "app_xml_utils.h"

#include "app_disc.h"
#include "app_mgt.h"
#include "app_utils.h"

/*
 * Global Variables
 */
tAPP_XML_CONFIG app_xml_config;

int progress_sum;

/*******************************************************************************
 **
 ** Function         app_ftc_handle_list_evt
 **
 ** Description      Example of list event callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ftc_handle_list_evt(tBSA_FTC_LIST_EVT *p_data)
{
    UINT8 * p_buf = p_data->data;
    UINT16 Index;

    if(p_data->is_xml)
    {
        for(Index = 0; Index < p_data->len; Index++)
        {
            printf("%c", p_buf[Index]);
        }
        printf("\n");

    }
    else
    {
        for (Index = 0;Index < p_data->num_entry ; Index++)
        {
            printf("%s \n", p_buf);
            p_buf += 1+strlen((char *) p_buf);
        }
    }

    printf("BSA_FTC_LIST_EVT num entry %d,is final %d, xml %d, len %d\n",
         p_data->num_entry, p_data->final, p_data->is_xml, p_data->len);
}

/*******************************************************************************
 **
 ** Function         app_ftc_cback
 **
 ** Description      Example of FTC callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ftc_cback(tBSA_FTC_EVT event, tBSA_FTC_MSG *p_data)
{
    switch (event)
    {
    case BSA_FTC_OPEN_EVT: /* Connection Open (for info) */
        progress_sum = 0;
        printf("BSA_FTC_OPEN_EVT Status %d\n", p_data->status);
        break;
    case BSA_FTC_PROGRESS_EVT:
        progress_sum += p_data->prog.bytes;
        printf("BSA_FTC_PROGRESS_EVT %dkB of %dkB\n", (progress_sum >> 10),
                ((int) p_data->prog.file_size >> 10));
        break;
    case BSA_FTC_PUTFILE_EVT:
        printf("BSA_FTC_PUTFILE_EVT status %d\n", p_data->status);
        break;
    case BSA_FTC_GETFILE_EVT:
        printf("BSA_FTC_GETFILE_EVT status %d\n", p_data->status);
        break;
    case BSA_FTC_CLOSE_EVT:
        printf("BSA_FTC_CLOSE_EVT\n");
        break;
    case BSA_FTC_CHDIR_EVT:
        printf("BSA_FTC_CHDIR_EVT\n");
        break;
    case BSA_FTC_MKDIR_EVT:
        printf("BSA_FTC_MKDIR_EVT\n");
        break;
    case BSA_FTC_REMOVE_EVT:
        printf("BSA_FTC_REMOVE_EVT\n");
        break;
    case BSA_FTC_LIST_EVT:
        app_ftc_handle_list_evt(&(p_data->list));
        break;
    default:
        fprintf(stderr, "app_ftc_cback unknown event:%d\n", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_start_ftc
 **
 ** Description      Example of function to start OPP Client application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_start_ftc(void)
{
    tBSA_STATUS status = BSA_SUCCESS;

    tBSA_FTC_ENABLE enable_param;

    printf("app_start_ftc\n");

    status = BSA_FtcEnableInit(&enable_param);

    enable_param.p_cback = app_ftc_cback;

    status = BSA_FtcEnable(&enable_param);
    printf("app_start_ftc Status: %d \n", status);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_start_ftc: Unable to enable FTC status:%d\n", status);
    }
    return status;
}
/*******************************************************************************
 **
 ** Function         app_stop_ftc
 **
 ** Description      Example of function to sttop OPP Server application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_stop_ftc(void)
{
    tBSA_STATUS status;

    tBSA_FTC_DISABLE disable_param;

    printf("app_stop_ftc\n");

    status = BSA_FtcDisableInit(&disable_param);

    status = BSA_FtcDisable(&disable_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_stop_ftc: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ftc_close
 **
 ** Description      Example of function to disconnect current device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_close(void)
{
    tBSA_STATUS status;

    tBSA_FTC_CLOSE close_param;

    printf("app_ftc_close\n");

    status = BSA_FtcCloseInit(&close_param);

    status = BSA_FtcClose(&close_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_close: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ftc_put_file
 **
 ** Description      Example of function to put a file in the
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_put_file(char * p_file_name)
{
    tBSA_STATUS status = 0;
    tBSA_FTC_PUT param;

    status = BSA_FtcPutFileInit(&param);

    strncpy(param.name, p_file_name, BSA_FT_FILENAME_MAX-1);

    status = BSA_FtcPutFile(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_put_file: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ftc_get_file
 **
 ** Description      Example of function to get a file from peer current dir.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_get_file(char * p_local_file_name, char * p_remote_file_name)
{
    tBSA_STATUS status = 0;
    tBSA_FTC_GET param;

    printf("app_ftc_get_file remote : %s -> local :  %s \n", p_remote_file_name, p_local_file_name);

    status = BSA_FtcGetFileInit(&param);

    strncpy(param.local_name, p_local_file_name, BSA_FT_FILENAME_MAX-1);
    strncpy(param.remote_name, p_remote_file_name, BSA_FT_FILENAME_MAX-1);

    status = BSA_FtcGetFile(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_get_file: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ftc_cd
 **
 ** Description      Example of function to get a file from peer current dir.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_cd(void)
{
    tBSA_STATUS status = 0;
    tBSA_FTC_CH_DIR param;
    int dir_len=0;

    printf("app_ftc_cd\n");

    status = BSA_FtcChDirInit(&param);

    dir_len=app_get_string("Dir name to enter. If you want to go back, type ..",param.dir,BSA_FT_FILENAME_MAX-1);
    if(dir_len==0)
    {
        printf("Nothing entered.");
        return status;
    }

    if(strncmp(param.dir, "..", BSA_FT_FILENAME_MAX-1) == 0)
    {
        param.flag = BSA_FTC_FLAG_BACKUP;
    }

    status = BSA_FtcChDir(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_get_file: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ftc_mkdir
 **
 ** Description      Example of function to get a file from peer current dir.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_mkdir(void)
{
    tBSA_STATUS status = 0;
    int dir_len=0;
    tBSA_FTC_MK_DIR param;

    printf("app_ftc_mkdir \n");

    status = BSA_FtcMkDirInit(&param);

    dir_len=app_get_string("Dir name to create",param.dir,BSA_FT_FILENAME_MAX-1);
    if(dir_len==0)
    {
        printf("Nothing entered");
        return status;
    }

    status = BSA_FtcMkDir(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_mkdir: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ftc_mv_file
 **
 ** Description      Example of function to perform a file move operation on the server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_mv_file(char * p_dst_file_name, char * p_src_file_name)
{
    tBSA_STATUS status = 0;
    tBSA_FTC_MOVE param;

    printf("app_ftc_mv_file src : %s -> dest :  %s \n", p_src_file_name, p_dst_file_name);
    status = BSA_FtcMoveInit(&param);

    strncpy(param.dest, p_dst_file_name, BSA_FT_FILENAME_MAX-1);
    strncpy(param.src, p_src_file_name, BSA_FT_FILENAME_MAX-1);

    status = BSA_FtcMove(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_mv_file: Error:%d\n", status);
    }
    return status;
}
/*******************************************************************************
 **
 ** Function         app_ftc_rm_file
 **
 ** Description      Example of function to perform a file remove operation on the server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_rm_file(char * p_file_name)
{
    tBSA_STATUS status = 0;
    tBSA_FTC_REMOVE param;

    printf("app_ftc_rm_file name : %s\n", p_file_name);

    status = BSA_FtcRemoveInit(&param);

    strncpy(param.name, p_file_name, BSA_FT_FILENAME_MAX-1);

    status = BSA_FtcRemove(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_rm_file: Error:%d\n", status);
    }
    return status;
}
/*******************************************************************************
 **
 ** Function         app_ftc_cp_file
 **
 ** Description      Example of function to perform a file copy on the server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_cp_file(char * p_dst_file_name, char * p_src_file_name)
{
    tBSA_STATUS status = 0;
    tBSA_FTC_MOVE param;

    status = BSA_FtcCopyInit(&param);

    strncpy(param.dest, p_dst_file_name, BSA_FT_FILENAME_MAX-1);
    strncpy(param.src, p_src_file_name, BSA_FT_FILENAME_MAX-1);

    status = BSA_FtcCopy(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_cp_file: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ftc_list_dir
 **
 ** Description      Example of function to perform a get directory listing request
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_list_dir(char * p_dir, BOOLEAN is_xml)
{
    tBSA_STATUS status = 0;
    tBSA_FTC_LIST_DIR param;

    status = BSA_FtcListDirInit(&param);

    strncpy(param.dir, p_dir, BSA_FT_FILENAME_MAX-1);
    param.is_xml = is_xml;

    status = BSA_FtcListDir(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_list_dir: Error:%d\n", status);
    }
    return status;
}
/*******************************************************************************
 **
 ** Function         app_ftc_open
 **
 ** Description      Example of function to exhcange a vcard with current peer device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ftc_open(void)
{
    tBSA_STATUS status = 0;
    int device_index;
    tBSA_FTC_OPEN param;

    printf("app_ftc_open\n");
    BSA_FtcOpenInit(&param);

    printf("Bluetooth FTC menu:\n");
    printf("    0 Device from XML database (already paired)\n");
    printf("    1 Device found in last discovery\n");
    device_index = app_get_choice("Select source");
    switch (device_index)
    {
        case 0 :
            /* Devices from XML databased */
            /* Read the Remote device xml file to have a fresh view */
            app_read_xml_remote_devices();
            app_xml_display_devices(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db));
            device_index = app_get_choice("Select device");
            if ((device_index >= APP_NUM_ELEMENTS(app_xml_remote_devices_db)) || (device_index < 0))
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }

            if (app_xml_remote_devices_db[device_index].in_use == FALSE)
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }
            bdcpy(param.bd_addr, app_xml_remote_devices_db[device_index].bd_addr);
            break;
        case 1 :
            /* Devices from Discovery */
            app_disc_display_devices();
            device_index = app_get_choice("Select device");
            if ((device_index >= APP_DISC_NB_DEVICES) || (device_index < 0))
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }
            if (app_discovery_cb.devs[device_index].in_use == FALSE)
            {
                printf("Wrong Remote device index\n");
                return BSA_SUCCESS;
            }
            bdcpy(param.bd_addr, app_discovery_cb.devs[device_index].device.bd_addr);
            break;
        default:
            printf("Wrong selection !!:\n");
            return BSA_SUCCESS;
    }

    status = BSA_FtcOpen(&param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_ftc_open: Error:%d\n", status);
    }
    return status;
}

