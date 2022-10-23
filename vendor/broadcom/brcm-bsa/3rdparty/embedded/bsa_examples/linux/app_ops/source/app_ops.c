/*****************************************************************************
 **
 **  Name:           App_ops.h
 **
 **  Description:    Bluetooth Object push server application
 **
 **  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
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
#include "app_disc.h"
#include "app_utils.h"
#include "app_ops.h"

/*
 * Globales Variables
 */

int progress_sum;
UINT8 auto_access_push;
UINT8 auto_access_pull;

/*******************************************************************************
 **
 ** Function         app_ops_auto_accept
 **
 ** Description      Example of a function to set the BSA to automatically accept
 **                  an access request
 **
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ops_auto_accept(void)
{
    tBSA_STATUS status = 0;
    tBSA_OPS_ACCESS ops_access_resp_param;

    BSA_OpsAccessInit(&ops_access_resp_param);

    ops_access_resp_param.oper = BSA_OP_OPER_PUSH;
    ops_access_resp_param.access = BSA_OP_ACCESS_EN_AUTO;
    snprintf(ops_access_resp_param.object_name, sizeof(ops_access_resp_param.object_name) - 1,
            "./push/");

    printf("app_ops_auto_accept\n");

    APP_DEBUG1("app_ops Auto - oper:%x access:%x Name:%s",
            ops_access_resp_param.oper, ops_access_resp_param.access,
            ops_access_resp_param.object_name);

    status = BSA_OpsAccess(&ops_access_resp_param);

    if (status != BSA_SUCCESS)
    {
        APP_DEBUG1("app_access_resp_ops:error status %d", status);
    }

    auto_access_push = TRUE;

    ops_access_resp_param.oper = BSA_OP_OPER_PULL;
    ops_access_resp_param.access = BSA_OP_ACCESS_EN_AUTO;
    snprintf(ops_access_resp_param.object_name, sizeof(ops_access_resp_param.object_name) - 1,
            "./pull/Abdul Iqbal.vcf");

    printf("app_ops Auto - oper:%x access:%x Name:%s\n",
            ops_access_resp_param.oper, ops_access_resp_param.access,
            ops_access_resp_param.object_name);

    status = BSA_OpsAccess(&ops_access_resp_param);

    if (status != BSA_SUCCESS)
    {
        APP_DEBUG1("app_access_resp_ops:error status %d", status);
    }

    auto_access_pull = TRUE;

    return status;
}

/*******************************************************************************
 **
 ** Function         app_access_resp_ops
 **
 ** Description      Example of a function to respond to an access request on
 **                  the OPP Server
 **
 ** Parameters      OPS Message
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_access_resp_ops(tBSA_OPS_MSG *ops_access_req_param)
{
    tBSA_STATUS status;
    tBSA_OPS_ACCESS ops_access_resp_param;

    APP_DEBUG0("app_access_response_ops");
    if (ops_access_req_param->access.responce_required
            == BSA_OP_ACCESS_FS_ERROR)
    {
        BSA_OpsAccessInit(&ops_access_resp_param);

        ops_access_resp_param.oper = ops_access_req_param->access.oper;
        ops_access_resp_param.access = BSA_OP_ACCESS_FORBID;
        snprintf(ops_access_resp_param.object_name, sizeof(ops_access_resp_param.object_name)-1,
                "%s", ops_access_req_param->access.p_name);

        printf("app_ops - File Error oper:%x access:%x Name:%s\n",
                ops_access_resp_param.oper, ops_access_resp_param.access,
                ops_access_resp_param.object_name);

        status = BSA_OpsAccess(&ops_access_resp_param);

        if (status != BSA_SUCCESS)
        {
            APP_DEBUG1("app_access_resp_ops:error status %d", status);
        }
        return;
    }

    BSA_OpsAccessInit(&ops_access_resp_param);
    ops_access_resp_param.oper = ops_access_req_param->access.oper;
    ops_access_resp_param.access = BSA_OP_ACCESS_ALLOW;

    switch (ops_access_req_param->access.oper)
    {
        case BSA_OP_OPER_PUSH :
            if (auto_access_push == FALSE)
            {
                snprintf(ops_access_resp_param.object_name,
                        sizeof(ops_access_resp_param.object_name) - 1, "./push/%s",
                        ops_access_req_param->access.p_name);
                break;
            }
        case BSA_OP_OPER_PULL :
            if (auto_access_pull == FALSE)
            {
                snprintf(ops_access_resp_param.object_name,
                        sizeof(ops_access_resp_param.object_name) - 1, "./pull/test_card.vcf");

                break;
            }
        default :
            printf("app_access_response_ops unrecognized operation type: %02X \n",
                    ops_access_req_param->access.oper);
            return;
    }

    printf("app_ops - oper:%x access:%x Name:%s\n",
            ops_access_resp_param.oper, ops_access_resp_param.access,
            ops_access_resp_param.object_name);
    status = BSA_OpsAccess(&ops_access_resp_param);

    if (status != BSA_SUCCESS)
    {
        APP_DEBUG1("app_access_resp_ops:error status %d", status);
    }
}

/*******************************************************************************
 **
 ** Function         app_ops_cback
 **
 ** Description      Example of OPS callback function
 **
 ** Parameters      event and message
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ops_cback(tBSA_OPS_EVT event, tBSA_OPS_MSG *p_data)
{
    switch (event)
    {

        case BSA_OPS_OPEN_EVT: /* Connection Open (for info) */
            progress_sum = 0;
            APP_DEBUG1("BSA_OPS_OPEN_EVT:Remote bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
                    p_data->bd_addr[0], p_data->bd_addr[1], p_data->bd_addr[2],
                    p_data->bd_addr[3], p_data->bd_addr[4], p_data->bd_addr[5]);
            break;

        case BSA_OPS_PROGRESS_EVT:
            progress_sum += p_data->prog.bytes;
            APP_DEBUG1("BSA_OPS_PROGRESS_EVT %dkB of %dkB", (progress_sum >> 10),
                    ((int) p_data->prog.obj_size >> 10));
            break;

        case BSA_OPS_OBJECT_EVT:
            APP_DEBUG1("BSA_OPS_OBJECT_EVT Object:%s format:%d",
                    p_data->object.p_name, p_data->object.format);
            break;

        case BSA_OPS_CLOSE_EVT:
            APP_DEBUG0("BSA_OPS_CLOSE_EVT");
            break;

        case BSA_OPS_ACCESS_EVT:
            APP_DEBUG0("BSA_OPS_ACCESS_EVT");
            APP_DEBUG1("app_ops1 - oper:%x access:%x Name:%s",
                    p_data->access.oper, BSA_OP_ACCESS_ALLOW,
                    p_data->access.p_name);
            app_access_resp_ops(p_data);
            break;

        default:
            APP_DEBUG1("app_ops_cback unknown event:%d", event);
            break;
    }
}

/*******************************************************************************
 **
 ** Function         app_start_ops
 **
 ** Description      Example of function to start OPP Server application
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_start_ops(void)
{
    int status = 0;
    tBSA_OPS_ENABLE ops_enable_param;

    printf("app_start_ops\n");

    auto_access_push = FALSE;
    auto_access_pull = FALSE;

    status = BSA_OpsEnableInit(&ops_enable_param);

    ops_enable_param.sec_mask = BSA_SEC_NONE;
    ops_enable_param.api_fmt = BSA_OP_OBJECT_FMT;
    ops_enable_param.formats = BSA_OP_ANY_MASK | BSA_OP_VCARD21_MASK
            | BSA_OP_VCARD30_MASK | BSA_OP_VCAL_MASK | BSA_OP_ICAL_MASK
            | BSA_OP_VNOTE_MASK | BSA_OP_VMSG_MASK;

    snprintf(ops_enable_param.service_name, sizeof(ops_enable_param.service_name) - 1,
            "Object Push Server Service");
    ops_enable_param.p_cback = app_ops_cback;

    status = BSA_OpsEnable(&ops_enable_param);
    printf("app_start_ops Status: %d \n", status);

    if (status != BSA_SUCCESS)
    {
        APP_DEBUG1("app_start_ops: Unable to enable OPS status:%d",
                status);
    }
    return status;
}
/*******************************************************************************
 **
 ** Function         app_stop_ops
 **
 ** Description      Example of function to sttop OPP Server application
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_stop_ops(void)
{
    int status = 0;

    tBSA_OPS_DISABLE ops_disable_param;

    printf("app_stop_ops\n");

    status = BSA_OpsDisableInit(&ops_disable_param);

    status = BSA_OpsDisable(&ops_disable_param);

    if (status != BSA_SUCCESS)
    {
        APP_DEBUG1("app_stop_ops: Error:%d", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_ops_disconnect
 **
 ** Description      Example of function to disconnect current device
 **
 ** Parameters      BSA_SUCCESS success error code for failure
 **
 ** Returns          void
 **
 *******************************************************************************/
tBSA_STATUS app_ops_disconnect(void)
{
    int status = 0;

    tBSA_OPS_CLOSE ops_close_param;

    printf("app_stop_ops\n");

    status = BSA_OpsCloseInit(&ops_close_param);

    status = BSA_OpsClose(&ops_close_param);

    if (status != BSA_SUCCESS)
    {
        APP_DEBUG1("ops_close: Error:%d", status);
    }
    return status;
}

