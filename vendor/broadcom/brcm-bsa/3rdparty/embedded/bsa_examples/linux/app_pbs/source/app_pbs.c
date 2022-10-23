/*****************************************************************************
 **
 **  Name:           app_pbs.c
 **
 **  Description:    Bluetooth PBAP application for BSA
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
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
#include <dirent.h>

#include "app_utils.h"
#include "app_mgt.h"
#include "app_xml_param.h"
#include "app_op_api.h"

#include "app_pbs.h"

/*
 * Defines
 */

#define APP_BPS_SERVICE_NAME          "Phone Book Server"
#define APP_BPS_DOMAINE               "guest"
#define APP_BPS_ROOT_FOLDER           "."

#define APP_PBS_FEATURES    ( BSA_PBS_FEA_DOWNLOADING | BSA_PBS_FEA_BROWSING | \
                    BSA_PBS_FEA_DATABASE_ID | BSA_PBS_FEA_FOLDER_VER_COUNTER | \
                    BSA_PBS_FEA_VCARD_SELECTING | BSA_PBS_FEA_ENH_MISSED_CALLS | \
                    BSA_PBS_FEA_UCI_VCARD_FIELD | BSA_PBS_FEA_UID_VCARD_FIELD | \
                    BSA_PBS_FEA_CONTACT_REF | BSA_PBS_FEA_DEF_CONTACT_IMAGE_FORMAT )

#define APP_PBS_REPOSITORIES    ( BSA_PBS_REPOSIT_LOCAL | \
                    BSA_PBS_REPOSIT_SPEED_DIAL | BSA_PBS_REPOSIT_FAVORITES )

#define APP_PBS_MAX_VCARDLIST       1024
#define APP_PBS_VCARD30_EXTRA_LEN   100
#define APP_PBS_MAX_PROP            20
#define APP_PBS_FULL_PATH_LEN       128

#define APP_PBS_VCARD_BEGIN         "BEGIN:VCARD\r\n"
#define APP_PBS_VCARD_END           "END:VCARD\r\n"
#define APP_PBS_UID_TAG             "X-BT-UID:"

typedef struct {
    /* Open phonebook status */
    BOOLEAN         open;

    /* Pull event parameters */
    tBSA_PBS_PULL_MSG params;

    /* phonebook/entry operation variables */
    int             fd;
    UINT8           *p_buffer;
    UINT8           *p_buffer_end;
    int             buf_size;
    int             pb_size;
    UINT8           *data_ptr;
    int             curr_entry;
    UINT8           *read_ptr;
    int             read_size;

    /* listing operation variables */
    int             *index_list;
    int             num_list;

    int             new_missed_calls;
} tAPP_PBS_CB;

/*
 * Global Variables
 */

tAPP_PBS_CB app_pbs_cb;

tBSA_PBS_ACCESS_TYPE pbap_access_flag = BSA_PBS_ACCESS_ALLOW;

/*
 * Local Functions
 */
static void process_pull_event(tBSA_PBS_PULL_MSG *p_data);
static void process_data_request_event(tBSA_PBS_DATA_REQ_MSG *p_data);
static void process_get_pb_info_event(tBSA_PBS_GET_PB_INFO_MSG *p_data);
static void process_reset_nmc_event(tBSA_PBS_RESET_NMC_MSG *p_data);
static void process_oper_cmpl_event(tBSA_PBS_OBJECT_MSG *p_data);
static void open_phonebook();
static void close_phonebook();
static int read_pb_data(UINT8 *p_buffer, int size, BOOLEAN* p_final);
static UINT8* find_next_vcard(UINT8 *p_start);
static BOOLEAN get_next_vcard(char *path, int *index_list, int *num_index, char *file, char *name);
static BOOLEAN is_valid_vcf(char *fname, int *index_list, int num_index, int *curr_index);
static BOOLEAN get_next_vcard_by_index(char *path, int *index_list, int *num_index, char *fname);
static BOOLEAN get_next_vcard_by_name(char *path, int *index_list, int *num_index, char *fname);
static char * uid_to_file(char *uid);
static int get_phonebook_size(UINT8 oper, char *path, UINT64 selector, UINT8 selector_op);

/*******************************************************************************
 **
 ** Function         app_pbs_cback
 **
 ** Description      Example of PBS callback function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_pbs_cback(tBSA_PBS_EVT event, tBSA_PBS_MSG *p_data)
{
    tBSA_PBS_ACCESSRSP access_rsp_param;

    switch (event)
    {
        case BSA_PBS_OPEN_EVT: /* Connection Open (for info) */
            APP_DEBUG1("BSA_PBS_OPEN_EVT:Remote bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
                    p_data->open.bd_addr[0], p_data->open.bd_addr[1],
                    p_data->open.bd_addr[2], p_data->open.bd_addr[3],
                    p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
            break;

        case BSA_PBS_CLOSE_EVT: /* Connection Closed (for info)*/
            APP_DEBUG0("BSA_PBS_CLOSE_EVT");
            break;

        case BSA_PBS_AUTH_EVT: /* Authorization */
            APP_DEBUG1("BSA_PBS_AUTH_EVT userid required %d, len %d",
                    p_data->auth.userid_required, p_data->auth.userid_len);
            break;

        case BSA_PBS_ACCESS_EVT: /* Access requested Event */
            APP_DEBUG0("BSA_PBS_ACCESS_EVT");

            switch (p_data->access_req.oper)
            {
                case BSA_PBS_OPER_PULL_PB:
                    APP_DEBUG0("PBS Pull PB ");
                    break;
                case BSA_PBS_OPER_SET_PB:
                    APP_DEBUG0("PBS Set Path ");
                    break;
                case BSA_PBS_OPER_PULL_VCARD_LIST:
                    APP_DEBUG0("PBS Pull Vcard List ");
                    break;
                case BSA_PBS_OPER_PULL_VCARD_ENTRY:
                    APP_DEBUG0("PBS Pull Vcard Entry ");
                    break;
                default:
                    APP_DEBUG0("PBS: unknown Access request ");
                    break;
            }

            APP_DEBUG1("Name [%s] access %d; %s", p_data->access_req.name,
                    pbap_access_flag, pbap_access_flag==BSA_PBS_ACCESS_ALLOW?"GRANTED":"DENIED");

            BSA_PbsAccessRspInit(&access_rsp_param);
            access_rsp_param.oper = p_data->access_req.oper;
            access_rsp_param.access = pbap_access_flag;
            strncpy(access_rsp_param.name, p_data->access_req.name, sizeof(access_rsp_param.name)-1);
            access_rsp_param.name[sizeof(access_rsp_param.name)-1] = 0;

            BSA_PbsAccessRsp(&access_rsp_param);
            break;

        case BSA_PBS_OPER_CMPL_EVT: /* Get complete Event (for info) */
            APP_DEBUG1("BSA_PBS_OPER_CMPL_EVT Object:%s status:%d", p_data->oper_complete.name,
                    p_data->oper_complete.status);
            process_oper_cmpl_event(&p_data->oper_complete);
            break;

        case BSA_PBS_PULL_EVT: /* Pull phonebook/vList/vCard */
            APP_DEBUG0("BSA_PBS_PULL_EVT");
            process_pull_event(&p_data->pull);
            break;

        case BSA_PBS_DATA_REQ_EVT: /* Request phonebook/vList/vCard data */
            APP_DEBUG0("BSA_PBS_DATA_REQ_EVT");
            process_data_request_event(&p_data->data_req);
            break;

        case BSA_PBS_GET_PB_INFO_EVT: /* Get phonebook info */
            APP_DEBUG0("BSA_PBS_GET_PB_INFO_EVT");
            process_get_pb_info_event(&p_data->get_info);
            break;

        case BSA_PBS_RESET_NMC_EVT: /* Reset new missed calls */
            APP_DEBUG0("BSA_PBS_RESET_NMC_EVT");
            process_reset_nmc_event(&p_data->reset_nmc);
            break;

        default:
            APP_DEBUG1("app_pbs_cback unknown event:%d", event);
            break;
    }
}

/*******************************************************************************
 **
 ** Function         app_stop_pbs
 **
 ** Description      Example of function to start PBAP server application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_stop_pbs(void)
{
    tBSA_PBS_DISABLE param;

    BSA_PbsDisableInit(&param);
    BSA_PbsDisable(&param);
}

/*******************************************************************************
 **
 ** Function         app_start_pbs
 **
 ** Description      Example of function to start PBAP server application
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_start_pbs(void)
{
    tBSA_STATUS status;
    tBSA_PBS_ENABLE enable_param;

    /* initialize parameters */
    memset(&app_pbs_cb, 0, sizeof(app_pbs_cb));
    app_pbs_cb.fd = -1;
    app_pbs_cb.new_missed_calls = 4;

    /* enable PBS */
    BSA_PbsEnableInit(&enable_param);

    enable_param.sec_mask = BSA_SEC_NONE;
    strncpy(enable_param.service_name, APP_BPS_SERVICE_NAME, sizeof(enable_param.service_name));
    strncpy(enable_param.root_path, APP_BPS_ROOT_FOLDER, sizeof(enable_param.root_path));
    strncpy(enable_param.realm, APP_BPS_DOMAINE, sizeof(enable_param.realm));
    enable_param.local_features = APP_PBS_FEATURES;
    enable_param.local_repositories = APP_PBS_REPOSITORIES;
    enable_param.p_cback = app_pbs_cback;

    status = BSA_PbsEnable(&enable_param);

    if (status != BSA_SUCCESS)
    {
        APP_DEBUG1("Unable to enable PBS status:%d", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         process_pull_event
 **
 ** Description      Process pull request from peer, prepare phonebook data
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void process_pull_event(tBSA_PBS_PULL_MSG *p_data)
{
    char *p;

    switch (p_data->oper)
    {
        case BSA_PBS_OPER_PULL_PB:
            APP_DEBUG1("Pull Phonebook: %s", p_data->name);
            APP_DEBUG1("Property filter: 0x%llx, format:%s, max count: %d, offset: %d",
                p_data->filter, p_data->format == BSA_PBS_VCF_FMT_21 ? "2.1" : "3.0",
                p_data->max_count, p_data->start_offset);
            APP_DEBUG1("selector: 0x%llx, operator: %s",
                p_data->selector, p_data->selector_op == BSA_PBS_OP_OR ? "OR" : "AND");
            app_pbs_cb.params = *p_data;
            break;

        case BSA_PBS_OPER_PULL_VCARD_LIST:
            APP_DEBUG1("Pull vCard Listing: %s", p_data->name);
            APP_DEBUG1("Order: %d, search attribute: %d, search value:%s",
                p_data->order, p_data->search_attr, p_data->search_value);
            APP_DEBUG1("Max count: %d, offset: %d",
                p_data->max_count, p_data->start_offset);
            APP_DEBUG1("Selector: 0x%llx, operator: %s",
                p_data->selector, p_data->selector_op == BSA_PBS_OP_OR ? "OR" : "AND");
            app_pbs_cb.params = *p_data;
            break;

        case BSA_PBS_OPER_PULL_VCARD_ENTRY:
            APP_DEBUG1("Pull vCard Entry: %s", p_data->name);
            APP_DEBUG1("Property filter: 0x%llx, format:%s",
                p_data->filter, p_data->format == BSA_PBS_VCF_FMT_21 ? "2.1" : "3.0");
            app_pbs_cb.params = *p_data;
            p = strstr(p_data->name, APP_PBS_UID_TAG);
            if (p)
                strncpy(app_pbs_cb.params.name, uid_to_file(p + strlen(APP_PBS_UID_TAG)),
                        BSA_PBS_FILENAME_MAX - 1);
            app_pbs_cb.params.max_count = 1;
            app_pbs_cb.params.start_offset = 0;
            app_pbs_cb.params.selector = 0;
            break;

        default:
            APP_ERROR1("Unsupported operation type: %d", p_data->oper);
            return;
    }

    open_phonebook();
}

/*******************************************************************************
 **
 ** Function         process_data_request_event
 **
 ** Description      Process stack's request for phonebook data
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void process_data_request_event(tBSA_PBS_DATA_REQ_MSG *p_data)
{
    UINT8 *p_buffer;
    tBSA_PBS_VLIST_ENTRY *p_vlist;
    tBSA_PBS_DATA_RSP data_rsp;
    UINT16 i = 0;
    int bytes_read = 0;
    int list_size;

    APP_DEBUG1("Data request: oper = %d, data_size = %d", p_data->oper, p_data->data_size);

    switch (p_data->oper)
    {
        case BSA_PBS_OPER_PULL_PB:
        case BSA_PBS_OPER_PULL_VCARD_ENTRY:
            p_buffer = malloc(p_data->data_size);
            if (p_buffer)
            {
                BSA_PbsDataRspInit(&data_rsp);
                data_rsp.oper = p_data->oper;
                if (app_pbs_cb.open)
                {
                    while (bytes_read == 0 && !data_rsp.final)
                    {
                        bytes_read = read_pb_data(p_buffer, p_data->data_size, &data_rsp.final);
                        if (bytes_read >= 0)
                        {
                            data_rsp.status = BSA_SUCCESS;
                            data_rsp.data_size = (UINT16)bytes_read;
                            data_rsp.p_data = p_buffer;
                        }
                        else
                        {
                            data_rsp.status = BSA_ERROR_CLI_INTERNAL;
                            APP_ERROR0("Failed to read from phonebook");
                            break;
                        }
                    }

                    if (data_rsp.final || data_rsp.status != BSA_SUCCESS)
                    {
                        close_phonebook();
                    }
                }
                else
                {
                    data_rsp.status = BSA_ERROR_CLI_INTERNAL;
                    APP_ERROR0("Phonebook is not open");
                }
                APP_DEBUG1("Sending %d bytes data, final = %d, status = %d",
                        data_rsp.data_size, data_rsp.final, data_rsp.status);
                BSA_PbsDataRsp(&data_rsp);
                free(p_buffer);
            }
            break;

        case BSA_PBS_OPER_PULL_VCARD_LIST:
            list_size = sizeof(tBSA_PBS_VLIST_ENTRY) * p_data->data_size;
            p_vlist = (tBSA_PBS_VLIST_ENTRY *)malloc(list_size);
            if (p_vlist)
            {
                memset(p_vlist, 0, list_size);

                BSA_PbsDataRspInit(&data_rsp);
                data_rsp.oper = p_data->oper;
                data_rsp.p_vlist = p_vlist;
                if (app_pbs_cb.open)
                {
                    while (i < p_data->data_size && !data_rsp.final)
                    {
                        data_rsp.final = get_next_vcard(app_pbs_cb.params.name,
                                app_pbs_cb.index_list, &app_pbs_cb.num_list,
                                p_vlist->handle, p_vlist->name);

                        if (strlen(p_vlist->handle) == 0)
                            continue;

                        i++;
                        p_vlist++;
                    }
                    data_rsp.status = BSA_SUCCESS;
                    data_rsp.data_size = i;

                    if (data_rsp.final)
                    {
                        close_phonebook();
                    }
                }
                else
                {
                    data_rsp.status = BSA_ERROR_CLI_INTERNAL;
                    APP_ERROR0("Phonebook is not open");
                }
                APP_DEBUG1("Sending %d vCard listing entries, final = %d, status = %d",
                        data_rsp.data_size, data_rsp.final, data_rsp.status);
                BSA_PbsDataRsp(&data_rsp);
                free(data_rsp.p_vlist);
            }
            break;

        default:
            APP_ERROR1("Unsupported operation type: %d", p_data->oper);
            break;
    }
}

/*******************************************************************************
 **
 ** Function         process_get_pb_info_event
 **
 ** Description      Get phonebook information
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void process_get_pb_info_event(tBSA_PBS_GET_PB_INFO_MSG *p_data)
{
    tBSA_PBS_PB_INFO pb_info;
    int pb_size;

    APP_DEBUG1("Get Phonebook Info: %s", p_data->name);

    /* following phonebook info is hardcoded for testing purpose only */
    BSA_PbsPbInfoInit(&pb_info);
    pb_info.oper = p_data->oper;
    pb_info.status = BSA_SUCCESS;
    if (p_data->get_pb_size)
    {
        APP_DEBUG1("Get phonebook size, selector: 0x%llx, operator: %s", p_data->selector,
                p_data->selector_op == BSA_PBS_OP_OR ? "OR" : "AND");
        pb_size = get_phonebook_size(p_data->oper, p_data->name, p_data->selector,
                p_data->selector_op);
        if (pb_size < 0)
            pb_info.status = BSA_ERROR_CLI_INTERNAL;
        else
            pb_info.pb_size = (UINT16)pb_size;
    }
    if (p_data->get_new_missed_call)
    {
        APP_DEBUG0("Get new missed call");
        pb_info.new_missed_call = app_pbs_cb.new_missed_calls;
    }
    if (p_data->get_pri_ver)
    {
        APP_DEBUG0("Get primary version");
        pb_info.pri_ver[15] = 1;
    }
    if (p_data->get_sec_ver)
    {
        APP_DEBUG0("Get secondary version");
        pb_info.sec_ver[15] = 1;
    }
    if (p_data->get_db_id)
    {
        APP_DEBUG0("Get database ID");
        pb_info.db_id[15] = 1;
    }
    BSA_PbsPbInfo(&pb_info);
}

/*******************************************************************************
 **
 ** Function         process_reset_nmc_event
 **
 ** Description      Reset new missed calls
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void process_reset_nmc_event(tBSA_PBS_RESET_NMC_MSG *p_data)
{
    APP_DEBUG0("Reset new missed calls");

    app_pbs_cb.new_missed_calls = 0;
}

/*******************************************************************************
 **
 ** Function         process_oper_cmpl_event
 **
 ** Description      Clean up after operation is completed
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void process_oper_cmpl_event(tBSA_PBS_OBJECT_MSG *p_data)
{
    close_phonebook();
}

/*******************************************************************************
**
** Function         open_phonebook
**
** Description      This function opens phonebook for pull event
**
** Parameters       none
**
** Returns          none
**
*******************************************************************************/
static void open_phonebook()
{
    struct stat file_stat;
    DIR *dir;

    if (app_pbs_cb.params.oper == BSA_PBS_OPER_PULL_PB ||
        app_pbs_cb.params.oper == BSA_PBS_OPER_PULL_VCARD_ENTRY)
    {
        app_pbs_cb.fd = open(app_pbs_cb.params.name, O_RDONLY);
        if (app_pbs_cb.fd != -1 && fstat(app_pbs_cb.fd, &file_stat) == 0)
        {
            app_pbs_cb.buf_size = file_stat.st_size + APP_PBS_VCARD30_EXTRA_LEN + 1;
            app_pbs_cb.p_buffer = malloc(app_pbs_cb.buf_size);
            if (app_pbs_cb.p_buffer)
            {
                app_pbs_cb.p_buffer_end = app_pbs_cb.p_buffer + app_pbs_cb.buf_size - 1;
                app_pbs_cb.data_ptr = app_pbs_cb.p_buffer + APP_PBS_VCARD30_EXTRA_LEN;
                app_pbs_cb.pb_size = read(app_pbs_cb.fd, app_pbs_cb.data_ptr, file_stat.st_size);
                app_pbs_cb.data_ptr[app_pbs_cb.pb_size] = 0;
                app_pbs_cb.curr_entry = 0;
                app_pbs_cb.read_size = 0;
                app_pbs_cb.open = TRUE;

                APP_DEBUG1("Open phonebook succeeded, %d bytes read", app_pbs_cb.pb_size);

                if (app_pbs_cb.params.filter != 0)
                {
                    if (app_pbs_cb.params.format == BSA_PBS_VCF_FMT_21)
                        app_pbs_cb.params.filter |= BSA_PBS_FILTER_N | BSA_PBS_FILTER_TEL;
                    else if (app_pbs_cb.params.format == BSA_PBS_VCF_FMT_30)
                        app_pbs_cb.params.filter |= BSA_PBS_FILTER_FN | BSA_PBS_FILTER_N |
                                                    BSA_PBS_FILTER_TEL;
                }
                app_op_set_card_prop_filter_mask(app_pbs_cb.params.filter);
                app_op_set_card_selector_operator(app_pbs_cb.params.selector,
                        app_pbs_cb.params.selector_op);
            }
            else
            {
                APP_ERROR0("Failed to allocate buffer for reading phonebook data");
                close(app_pbs_cb.fd);
                app_pbs_cb.fd = -1;
            }
        }
        else
        {
            APP_ERROR0("Failed to open phonebook file");
        }
    }
    else if (app_pbs_cb.params.oper == BSA_PBS_OPER_PULL_VCARD_LIST)
    {
        dir = opendir(app_pbs_cb.params.name);
        if (dir)
        {
            app_pbs_cb.index_list = (int *)malloc(sizeof(int)*APP_PBS_MAX_VCARDLIST);
            if (app_pbs_cb.index_list)
            {
                memset(app_pbs_cb.index_list, -1, sizeof(int)*APP_PBS_MAX_VCARDLIST);
                app_pbs_cb.num_list = 0;
                app_pbs_cb.curr_entry = 0;
                app_pbs_cb.open = TRUE;

                app_op_set_card_selector_operator(app_pbs_cb.params.selector,
                        app_pbs_cb.params.selector_op);
            }
            closedir(dir);
        }
        else
        {
            APP_ERROR0("Failed to open phonebook directory");
        }
    }
}

/*******************************************************************************
 **
 ** Function         close_phonebook
 **
 ** Description      Close phonebook after pull operation finishes
 **
 ** Parameters       none
 **
 ** Returns          none
 **
 *******************************************************************************/
static void close_phonebook()
{
    if (!app_pbs_cb.open)
        return;

    if (app_pbs_cb.params.oper == BSA_PBS_OPER_PULL_PB ||
        app_pbs_cb.params.oper == BSA_PBS_OPER_PULL_VCARD_ENTRY)
    {
        if (app_pbs_cb.fd != -1)
        {
            close(app_pbs_cb.fd);
            app_pbs_cb.fd = -1;
        }
        if (app_pbs_cb.p_buffer)
        {
            free(app_pbs_cb.p_buffer);
            app_pbs_cb.p_buffer = NULL;
        }
    }
    else if (app_pbs_cb.params.oper == BSA_PBS_OPER_PULL_VCARD_LIST)
    {
        if (app_pbs_cb.index_list)
        {
            free(app_pbs_cb.index_list);
            app_pbs_cb.index_list = NULL;
        }
    }

    app_pbs_cb.open = FALSE;
}

/*******************************************************************************
**
** Function         read_pb_data
**
** Description      This function reads phonebook data from file, modify
**                  the content and format according to parameters from
**                  client.
**
** Parameters       p_buffer: output data buffer
**                  size: output buffer size
**                  p_final: end of data
**
** Returns          int: output data size, -1 if error occurred
**
*******************************************************************************/
static int read_pb_data(UINT8 *p_buffer, int size, BOOLEAN* p_final)
{
    int bytes_read = 0;
    tAPP_OP_STATUS op_status;
    tAPP_OP_PROP prop_array[APP_PBS_MAX_PROP];
    UINT8 num_prop = APP_PBS_MAX_PROP;
    UINT16 data_size;
    tAPP_OP_FMT fmt;
    UINT8 *p_next_vcard;

    /* Build vCard for reading */
    if (app_pbs_cb.read_size == 0 &&
        app_pbs_cb.data_ptr != NULL &&
        app_pbs_cb.params.max_count > app_pbs_cb.curr_entry - app_pbs_cb.params.start_offset)
    {
        /* Calculate vCard data size */
        p_next_vcard = find_next_vcard(app_pbs_cb.data_ptr);
        if (p_next_vcard != NULL)
            data_size = p_next_vcard - app_pbs_cb.data_ptr;
        else
            data_size = app_pbs_cb.p_buffer_end - app_pbs_cb.data_ptr;
        fmt = app_pbs_cb.params.format == BSA_PBS_VCF_FMT_21 ?
              APP_OP_VCARD21_FMT : APP_OP_VCARD30_FMT;
        memset(prop_array, 0, sizeof(tAPP_OP_PROP) * APP_PBS_MAX_PROP);

        /* Parse vCard */
        op_status = app_op_parse_card(prop_array, &num_prop, app_pbs_cb.data_ptr, data_size);
        if (op_status == APP_OP_OK)
        {
            /* Check vCard selector */
            op_status = app_op_check_card(prop_array, num_prop);
            if (op_status == APP_OP_OK)
            {
                /* Check start offset */
                if (app_pbs_cb.curr_entry >= app_pbs_cb.params.start_offset)
                {
                    /* Build vCard */
                    data_size += APP_PBS_VCARD30_EXTRA_LEN;
                    op_status = app_op_build_card(app_pbs_cb.p_buffer, &data_size, fmt,
                            prop_array, num_prop);
                    if (op_status == APP_OP_OK)
                    {
                        app_pbs_cb.read_ptr = app_pbs_cb.p_buffer;
                        app_pbs_cb.read_size = data_size;
                    }
                }
                app_pbs_cb.curr_entry++;
            }
        }

        app_pbs_cb.data_ptr = p_next_vcard;
    }

    /* Read data */
    if (app_pbs_cb.read_size > 0)
    {
        if (app_pbs_cb.read_size > size)
            bytes_read = size;
        else
            bytes_read = app_pbs_cb.read_size;
        memcpy(p_buffer, app_pbs_cb.read_ptr, bytes_read);
        app_pbs_cb.read_ptr += bytes_read;
        app_pbs_cb.read_size -= bytes_read;
    }

    /* Check if reached the end of data */
    if (app_pbs_cb.read_size == 0 && (app_pbs_cb.data_ptr == NULL ||
         app_pbs_cb.params.max_count == app_pbs_cb.curr_entry - app_pbs_cb.params.start_offset))
        *p_final = TRUE;
    else
        *p_final = FALSE;

    return bytes_read;
}

/*******************************************************************************
**
** Function         find_next_vcard
**
** Description      This function finds the beginning of the next vCard
**
** Parameters       p_start: starting point in buffer to search vCard
**
** Returns          Pointer of the next vCard, or NULL if not found
**
*******************************************************************************/
static UINT8* find_next_vcard(UINT8 *p_start)
{
    UINT8 *p_next = (UINT8 *)strstr((char *)p_start, APP_PBS_VCARD_END);
    if (p_next)
        p_next = (UINT8 *)strstr((char *)p_next, APP_PBS_VCARD_BEGIN);

    return p_next;
}

/*******************************************************************************
**
** Function         get_next_vcard
**
** Description      This function gets file name and contact name of the next
**                  vCard in a folder
**
** Parameters       path:   path (absolute path to the directory to get the
**                              the vcard listing from. (ex. c:\pbs\telecom\pb\*)
**                  index_list: list of the handles/indexes previously retrieved
**                              in the 'indexed' order. The next 'handle' shall
**                              be appended to this list by this function.
**                              ex. if the list contained [0,2] (assume: 1.vcf and 3.vcf
**                              do not exist in the directory), then handle=4 will be
**                              added to the list making it [0,2,4] if 4.vcf exists
**                              in the directory.
**                  num_index:  number of entries in the index_list. This will
**                              be incremented if a handle is appended to the list.
**                              Thus in the example above, num_list increases from 2 to 3
**                  file:      Name of the next vCard file is returned by this function.
**                              Thus in the example above, it would be "4.vcf".
**                  name:      Name of the contact in the next vCard
**
** Returns          Pointer of the next vCard, or NULL if not found
**
*******************************************************************************/
static BOOLEAN get_next_vcard(char *path, int *index_list, int *num_index, char *file, char *name)
{
    BOOLEAN final = FALSE;
    int last_num_list;
    char full_path[APP_PBS_FULL_PATH_LEN];
    int fd;
    struct stat file_stat;
    UINT16 file_size = 0;
    UINT8 * p_card_mem;
    tAPP_OP_PROP prop_array[APP_PBS_MAX_PROP];
    UINT8 num_prop = APP_PBS_MAX_PROP;
    UINT16 temp_len = 0;
    tAPP_OP_STATUS op_status;
    char number[64] = "\0";

    /* save number of entries before getting the next vcard */
    last_num_list = *num_index;

    /* retrieve the vcard filename based on order as requested by client */
    switch(app_pbs_cb.params.order)
    {
        case BSA_PBS_ORDER_ALPHA_NUM:
            final = get_next_vcard_by_name(path, index_list, num_index, file);
            break;
        case BSA_PBS_ORDER_INDEX:
        default:
            final = get_next_vcard_by_index(path, index_list, num_index, file);
            break;
    }

    /* If we found any vcard in the dir - path */
    if (++last_num_list == *num_index)
    {
        /* full_path for the .vcf file */
        snprintf(full_path, APP_PBS_FULL_PATH_LEN, "%s/%s", path, file);
        if ((fd = open (full_path, O_RDONLY)) >= 0)
        {
            if(fstat(fd, &file_stat) == 0)
               file_size = (UINT16) file_stat.st_size;

            p_card_mem = (UINT8 *) malloc(file_size);
            if (p_card_mem)
            {
                file_size = read (fd, p_card_mem, file_size);
                memset(prop_array, 0, sizeof(tAPP_OP_PROP)*APP_PBS_MAX_PROP);
                op_status = app_op_parse_card((tAPP_OP_PROP*)prop_array, &num_prop,
                            p_card_mem, file_size);
                if (op_status == APP_OP_OK)
                {
                    op_status = app_op_get_card_property((UINT8 *)name, &temp_len, prop_array,
                            num_prop, (UINT8 *)"N");
                    /* Perform search if we find name and need to do search */
                    if (app_pbs_cb.params.search_attr == BSA_PBS_ATTR_NAME &&
                        app_pbs_cb.params.search_value[0] != 0)
                    {
                        if (op_status != APP_OP_OK ||
                            strstr(name, app_pbs_cb.params.search_value) == NULL)
                            file[0] = 0;
                    }

                    /* Perform search if we find tel number and need to do search */
                    if (app_pbs_cb.params.search_attr == BSA_PBS_ATTR_NUMBER &&
                        app_pbs_cb.params.search_value[0] != 0)
                    {
                        op_status = app_op_get_card_property((UINT8 *)number, &temp_len,
                                prop_array, num_prop, (UINT8 *)"TEL");
                        if (op_status != APP_OP_OK ||
                            strstr(number, app_pbs_cb.params.search_value) == NULL)
                            file[0] = 0;
                    }

                    /* Check vCard selector */
                    if (app_op_check_card(prop_array, num_prop) != APP_OP_OK)
                    {
                        file[0] = 0;
                    }

                }
                else
                {
                    file[0] = 0;
                }

                /* If we have a valid handle, check offset and max list count */
                if (file[0] != 0)
                {
                    /* check for offset */
                    if (app_pbs_cb.curr_entry < app_pbs_cb.params.start_offset)
                    {
                        file[0] = 0;
                    }
                    /* increase the current offset */
                    app_pbs_cb.curr_entry++;
                }

                free(p_card_mem);
            }
            close(fd);
        }

    }
    else
    {
        /* No more vcards in the directory */
        file[0] = 0;
        final = TRUE;
    }

    /* if we exceed max list count return final set to TRUE */
    if (app_pbs_cb.curr_entry - app_pbs_cb.params.start_offset == app_pbs_cb.params.max_count)
        final = TRUE;

    return final;
}

/*******************************************************************************
**
** Function         is_valid_vcf
**
** Description      This function is used to check if the file is a valid vcard
**                  file (ex. 2.vcf is valid) and that it is not already present
**                  in the index_list.
**
** Parameters       fname:      vCard file name to be validated. ex. abc.vcf / 1.vcf
**                  index_list: list of the handles/indexes previously retrieved.
**                  num_index:  number of entries in the index_list.
**                  curr_index: handle of the valid vcf file is returned. (ex.
**                              curr_index =1 is returned for 1.vcf if it did not
**                              already exist in index_list).
**
** Returns          validity:   TRUE if valid vcf and not present in index_list,
**                              else FALSE
**
**
**
*******************************************************************************/
static BOOLEAN is_valid_vcf(char *fname, int *index_list, int num_index, int *curr_index)
{
    BOOLEAN result = FALSE;
    int temp_index;
    char temp_handle[BSA_PBS_FILENAME_MAX];
    char *delimit;
    BOOLEAN found = FALSE;
    int i;

    strncpy(temp_handle, fname, sizeof(temp_handle)-1);
    temp_handle[sizeof(temp_handle)-1] = 0;
    delimit = strchr(temp_handle, '.');
    if (delimit)
    {
        *delimit = '\0';
        if ((temp_index = atoi(temp_handle)) >= 0)
        {
            if (!strcmp(++delimit, "vcf"))
            {
                for (i=0; i<num_index && !found; i++)
                {
                    if (index_list[i] == temp_index)
                        found = TRUE;
                }
                if (!found)
                {
                    /* vcf is valid and not present in the index_list */
                    *curr_index = temp_index;
                    result = TRUE;
                }
            }
        }
    }

    return result;
}

/*******************************************************************************
**
** Function         get_next_vcard_by_index
**
** Description      This function is called to retrieve the next vcard list entry
**                  from the specified path in the 'indexed' order.
**
** Parameters       path:   path (absolute path to the directory to get the
**                              the vcard listing from. (ex. c:\pbs\telecom\pb\*)
**                  index_list: list of the handles/indexes previously retrieved
**                              in the 'indexed' order. The next 'handle' shall
**                              be appended to this list by this function.
**                              ex. if the list contained [0,2] (assume: 1.vcf and 3.vcf
**                              do not exist in the directory), then handle=4 will be
**                              added to the list making it [0,2,4] if 4.vcf exists
**                              in the directory.
**                  num_index:  number of entries in the index_list. This will
**                              be incremented if a handle is appended to the list.
**                              Thus in the example above, num_list increases from 2 to 3
**                  fname:      Name of the next vCard file is returned by this function.
**                              Thus in the example above, it would be "4.vcf".
**
** Returns          final:      TRUE if final item in the vlist else FALSE
**
**
**
*******************************************************************************/
static BOOLEAN get_next_vcard_by_index(char *path, int *index_list, int *num_index, char *fname)
{

    int curr_index = 0;
    int best_index = -1;
    DIR *dir;
    struct dirent *dp;
    int num_valid_cnt = 0;

    if ((dir = opendir(path)) == NULL)
    {
        return FALSE;
    }

    while ((dp = readdir(dir)) != NULL)
    {
        /* Check if the vcard file is valid. If so, get the file handle in curr_index */
        if (is_valid_vcf(dp->d_name, index_list, *num_index, &curr_index))
        {
            /* if the curr_index is the first handle or is better than the currently best handle */
            if ((num_valid_cnt == 0) || (curr_index < best_index))
            {
                /* update the best_index */
                best_index = curr_index;
                strcpy(fname, dp->d_name);
            }
            /* increment the count of num valid vCard files found */
            num_valid_cnt++;
        }
    } /* end while */

    closedir(dir);

    /* No valid vCards found */
    if (num_valid_cnt == 0)
        return TRUE;
    else
    {
        /* update the return values in index_list and num_index */
        index_list[*num_index] = best_index;
        (*num_index)++;

        /* There was only one valid vCard file in the folder. Hence final vcard */

        if (num_valid_cnt == 1)
            return TRUE;
        else /* more valid vCards present in the folder */
            return FALSE;
    }

}

/*******************************************************************************
**
** Function         get_next_vcard_by_name
**
** Description      This function is called to retrieve the next vcard list entry
**                  from the specified path in the 'alpha-numeric' order.
**
** Parameters       path:   path (absolute path to the directory to get the
**                              the vcard listing from. (ex. c:\pbs\telecom\pb\*)
**                  index_list: list of the handles/indexes previously retrieved
**                              in the 'indexed' order. The next 'handle' shall
**                              be appended to this list by this function.
**                              ex. if the list contained [2,5] then handle=1 will be
**                              added to the list making it [2,5,1] if 1.vcf exists
**                              and the conact name (N) in 1.vcf is alphabatically
**                              next to 5.vcf in the temppath directory.
**                  num_index:  number of entries in the index_list. This will
**                              be incremented if a handle is appended to the list.
**                              Thus in the example above, num_list increases from 2 to 3
**                  fname:      Name of the next vCard file is returned by this function.
**                              Thus in the example above, it would be "1.vcf".
**
** Returns          final:      TRUE if final item in the vlist else FALSE
**
**
**
*******************************************************************************/
static BOOLEAN get_next_vcard_by_name(char *path, int *index_list, int *num_index, char *fname)
{
    int curr_index = 0;
    int best_index = -1;
    DIR *dir;
    struct dirent *dp;
    char best_name[BSA_PBS_MAX_CONTACT_NAME_LEN];
    char curr_name[BSA_PBS_MAX_CONTACT_NAME_LEN];
    int fd = -1;
    struct  stat file_stat;
    tAPP_OP_PROP prop_array[APP_PBS_MAX_PROP];
    int  file_size = 0;
    UINT16 temp_len = 0;
    UINT8 num_prop = APP_PBS_MAX_PROP;
    tAPP_OP_STATUS name_status = APP_OP_FAIL;
    tAPP_OP_STATUS bps_status;
    char full_path[APP_PBS_FULL_PATH_LEN];
    int num_valid_cnt = 0;
    UINT8 * p_card_mem;

    if ((dir = opendir(path)) == NULL)
    {
        return FALSE;
    }

    while ((dp = readdir(dir)) != NULL)
    {
        /* Check if the vcard file is valid */
        if (is_valid_vcf(dp->d_name, index_list, *num_index, &curr_index))
        {
            curr_name[0] = '\0';
            temp_len = 0;
            /* get the 'N' property present in the vcf in 'curr_name' */
            snprintf(full_path, APP_PBS_FULL_PATH_LEN, "%s/%s", path, dp->d_name);
            if ((fd = open (full_path, O_RDONLY)) >= 0)
            {
                if(fstat(fd, &file_stat) == 0)
                {
                    file_size = file_stat.st_size;

                    if ((p_card_mem = (UINT8 *) malloc(file_size)) != NULL)
                    {
                        /* read the vCard file into memory */
                        if ((file_size = read (fd, p_card_mem, file_size)) > 0)
                        {
                            memset(prop_array, 0, sizeof(tAPP_OP_PROP)*APP_PBS_MAX_PROP);
                            /* parse the vCard and store the proeprties and values */
                            bps_status = app_op_parse_card((tAPP_OP_PROP*)prop_array, &num_prop,
                                    p_card_mem, file_size);
                            /* Get the contact name 'N' property in the vCard into curr_name */
                            name_status = app_op_get_card_property((UINT8 *)curr_name, &temp_len,
                                    prop_array, num_prop, (UINT8 *)"N");
                        }
                        free(p_card_mem);
                        p_card_mem = NULL;
                    }
                }
                close(fd);
            }

            /* If there is a valid contact name in the vCard, then process further else
               go to next file in the directory */
            if ((name_status == APP_OP_OK) && (temp_len > 0))
            {
                num_valid_cnt++;
                curr_name[temp_len] = '\0'; /* valid contact name in the vcard */

                /* either there is no best_name or current name is better */
                if (num_valid_cnt == 1 ||
                    strcmp(best_name, curr_name) > 0)
                {
                    /* update with new best_name and best_index */
                    best_index = curr_index;
                    strcpy(fname, dp->d_name);
                    strcpy(best_name, curr_name);
                }
            }
        }
    } /* end while */

    closedir(dir);

    /* No valid vCards found */
    if (num_valid_cnt == 0)
        return TRUE;
    else
    {
        /* update the return value in index_list and num_index */
        index_list[*num_index] = best_index;
        (*num_index)++;

        /* There was only one valid vCard file in the folder. Hence final vcard */
        if (num_valid_cnt == 1)
            return TRUE;
        else /* more valid vCards present in the folder */
            return FALSE;
    }
}

/*******************************************************************************
 **
 ** Function         uid_to_file
 **
 ** Description      Find vCard with specific UID and return file name
 **
 ** Parameters       uid - UID
 **
 ** Returns          File name
 **
 *******************************************************************************/
static char * uid_to_file(char *uid)
{
    /* hardcoded file name for testing purpose only */
    return "./telecom/pb/0.vcf";
}

/*******************************************************************************
 **
 ** Function         get_phonebook_size
 **
 ** Description      Get number of entries in a phonebook
 **
 ** Parameters       oper: operation -pull phonebook / pull vCard listing
 **                  path: phonebook path
 **                  selector:  a property mask as vCard selector
 **                  selector_op: selector operator (OR/AND)
 **
 ** Returns          Phonebook size, -1 if error occurred
 **
 *******************************************************************************/
static int get_phonebook_size(UINT8 oper, char *path, UINT64 selector, UINT8 selector_op)
{
    int pb_size = -1;
    int fd;
    struct stat file_stat;
    UINT8 *p_buffer;
    int read_bytes;
    UINT16 upb_size;
    DIR *dir;
    int *index_list;
    int num_list = 0;
    BOOLEAN final = FALSE;
    char handle[BSA_PBS_MAX_HANDLE_LEN];
    char name[BSA_PBS_MAX_CONTACT_NAME_LEN];

    if (oper == BSA_PBS_OPER_PULL_PB)
    {
        fd = open(path, O_RDONLY);
        if (fd != -1 && fstat(fd, &file_stat) == 0)
        {
            p_buffer = malloc(file_stat.st_size + 1);
            if (p_buffer)
            {
                read_bytes = read(fd, p_buffer, file_stat.st_size);
                app_op_set_card_selector_operator(selector, selector_op);
                app_op_get_pb_size(p_buffer, p_buffer + read_bytes, &upb_size);
                pb_size = upb_size;
                free(p_buffer);
            }
            close(fd);
        }
        else
        {
            APP_ERROR1("Failed to open phonebook file %s", path);
        }
    }
    else if (oper == BSA_PBS_OPER_PULL_VCARD_LIST)
    {
        dir = opendir(path);
        if (dir)
        {
            closedir(dir);

            index_list = (int *)malloc(sizeof(int)*APP_PBS_MAX_VCARDLIST);
            if (index_list)
            {
                memset(index_list, -1, sizeof(int)*APP_PBS_MAX_VCARDLIST);
                num_list = 0;
                pb_size = 0;
                app_pbs_cb.curr_entry = 0;

                memset(&app_pbs_cb.params, 0, sizeof(tBSA_PBS_PULL_MSG));
                app_pbs_cb.params.max_count = 0xFFFF;

                app_op_set_card_selector_operator(selector, selector_op);

                while (!final)
                {
                    final = get_next_vcard(path, index_list, &num_list, handle, name);
                    if (handle[0] != 0)
                        pb_size++;
                }
                free(index_list);
            }
        }
        else
        {
            APP_ERROR1("Failed to open phonebook directory %s", path);
        }
    }

    APP_DEBUG1("get_phonebook_size  phonebook size = %d", pb_size);

    return pb_size;
}

