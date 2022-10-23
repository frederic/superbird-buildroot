/*****************************************************************************
**
**  Name:           security.c
**
**  Description:    Bluetooth security functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "bsa_api.h"
#include "security.h"
#include "app_xml_utils.h"
#include "discovery.h"
#include "simple_app.h"
#include "app_utils.h"
/*
 * Global data
 */
static volatile BOOLEAN sec_auth_complete_rcv = FALSE;


extern tSIMPLE_APP_CB simple_app;

/*******************************************************************************
 **
 ** Function         security_callback
 **
 ** Description      This function is callback function that handles the security
 **                  events
 **
 ** Parameters       event: security event
 **                  p_data: security event data
 **
 ** Returns          void
 **
 *******************************************************************************/
void security_callback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data)
{
    int status;
    int index;

    tBSA_SEC_PIN_CODE_REPLY pin_code_reply;

    switch(event)
    {
    case BSA_SEC_LINK_UP_EVT:       /* A device is physically connected (for info) */
        printf("BSA_SEC_LINK_UP_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                p_data->link_up.bd_addr[0], p_data->link_up.bd_addr[1],
                p_data->link_up.bd_addr[2], p_data->link_up.bd_addr[3],
                p_data->link_up.bd_addr[4], p_data->link_up.bd_addr[5]);
        break;
    case BSA_SEC_LINK_DOWN_EVT:     /* A device is physically disconnected (for info)*/
        printf("BSA_SEC_LINK_DOWN_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                p_data->link_down.bd_addr[0], p_data->link_down.bd_addr[1],
                p_data->link_down.bd_addr[2], p_data->link_down.bd_addr[3],
                p_data->link_down.bd_addr[4], p_data->link_down.bd_addr[5]);
        printf("\tReason: %d\n", p_data->link_down.status);
        break;

    case BSA_SEC_PIN_REQ_EVT:
        printf ("BSA_SEC_PIN_REQ_EVT (Pin Code Request) received from:\n");
        printf("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                p_data->pin_req.bd_addr[0], p_data->pin_req.bd_addr[1],
                p_data->pin_req.bd_addr[2], p_data->pin_req.bd_addr[3],
                p_data->pin_req.bd_addr[4],p_data->pin_req.bd_addr[5]);


        BSA_SecPinCodeReplyInit(&pin_code_reply);
        bdcpy(pin_code_reply.bd_addr, p_data->pin_req.bd_addr);

        pin_code_reply.pin_len = simple_app.pin_len;
        memcpy(pin_code_reply.pin_code, simple_app.pin_code, simple_app.pin_len);
        printf ("App calls BSA_SecPinCodeReply with pin:");
        for(index=0;index<pin_code_reply.pin_len ;index++)
        {
            printf("%d",pin_code_reply.pin_code[index]);
        }
        printf("\n");

        status = BSA_SecPinCodeReply(&pin_code_reply);
        break;

    case BSA_SEC_AUTH_CMPL_EVT:
        printf ("BSA_SEC_AUTH_CMPL_EVT received\n");
        printf("\tName:%s\n", p_data->auth_cmpl.bd_name);
        printf("\tSuccess:%d", p_data->auth_cmpl.success);
        if (p_data->auth_cmpl.success == 0)
        {
            printf(" => FAIL\n");
        }
        else
        {
            printf(" => OK\n");
            /* Read the Remote device xml file to have a fresh view */
            app_read_xml_remote_devices();

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

            /* Look in the Discovery database for COD */
            for (index = 0 ; index < DISC_NB_DEVICES ; index++)
            {
                if ((disc_devices[index].in_use != FALSE) &&
                    (bdcmp(disc_devices[index].device.bd_addr, p_data->auth_cmpl.bd_addr) == 0))

                {
                    app_xml_update_cod_db(app_xml_remote_devices_db,
                                          APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                          p_data->auth_cmpl.bd_addr,
                                          disc_devices[index].device.class_of_device);
                }
            }

            status = app_write_xml_remote_devices();
            if (status < 0)
                printf("fail to store remote devices database!!!\n");

        }
        printf("\tbd_addr:%02x:%02x:%02x:%02x:%02x:%02x\n", p_data->auth_cmpl.bd_addr[0], p_data->auth_cmpl.bd_addr[1],
                p_data->auth_cmpl.bd_addr[2], p_data->auth_cmpl.bd_addr[3], p_data->auth_cmpl.bd_addr[4],p_data->auth_cmpl.bd_addr[5]);
        if (p_data->auth_cmpl.key_present != FALSE)
            printf("\tLinkKey:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
                    p_data->auth_cmpl.key[0], p_data->auth_cmpl.key[1], p_data->auth_cmpl.key[2], p_data->auth_cmpl.key[3],
                    p_data->auth_cmpl.key[4], p_data->auth_cmpl.key[5], p_data->auth_cmpl.key[6], p_data->auth_cmpl.key[7],
                    p_data->auth_cmpl.key[8], p_data->auth_cmpl.key[9], p_data->auth_cmpl.key[10], p_data->auth_cmpl.key[11],
                    p_data->auth_cmpl.key[12], p_data->auth_cmpl.key[13], p_data->auth_cmpl.key[14], p_data->auth_cmpl.key[15]);
        sec_auth_complete_rcv = TRUE;
        break;

    case BSA_SEC_SUSPENDED_EVT: /* Connection Suspended */
        printf("BSA_SEC_SUSPENDED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->suspended.bd_addr[0], p_data->suspended.bd_addr[1],
                p_data->suspended.bd_addr[2], p_data->suspended.bd_addr[3],
                p_data->suspended.bd_addr[4], p_data->suspended.bd_addr[5]);
        break;

    case BSA_SEC_RESUMED_EVT: /* Connection Resumed */
        printf("BSA_SEC_RESUMED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->resumed.bd_addr[0], p_data->resumed.bd_addr[1],
                p_data->resumed.bd_addr[2], p_data->resumed.bd_addr[3],
                p_data->resumed.bd_addr[4], p_data->resumed.bd_addr[5]);
        break;


    default:
        fprintf(stderr, "app_disc_cback unknown event:%d\n", event);
        break;
    }
}


/*******************************************************************************
 **
 ** Function         sec_bond_init_param
 **
 ** Description      This function initializes the bonding parameter structure
 **
 ** Parameters       sec_bond_param: pointer on security parameter structure
 **
 ** Returns          int
 **
 *******************************************************************************/
int sec_bond_init_param(tSEC_BOND_PARAM *sec_bond_param)
{
    printf("sec_bond_init_param\n");
    bzero(sec_bond_param, sizeof(tSEC_BOND_PARAM));
    return 0;
}

/*******************************************************************************
 **
 ** Function         sec_set_local_config
 **
 ** Description      This function set the local device security configuration
 **
 ** Parameters       p_sec_local_config: pointer on security settings structure
 **
 ** Returns          int
 **
 *******************************************************************************/
int sec_set_local_config(tBSA_SEC_SET_SECURITY* p_sec_local_config)
{
    tBSA_STATUS bsa_status = BSA_SUCCESS;

    BSA_SecSetSecurityInit(p_sec_local_config);
    p_sec_local_config->simple_pairing_io_cap = BSA_SEC_IO_CAP_NONE;
    p_sec_local_config->sec_cback = security_callback;
    bsa_status = BSA_SecSetSecurity(p_sec_local_config);
    printf("\nSet Security local configuration \n");

    if (bsa_status != BSA_SUCCESS)
    {
        fprintf(stderr, "sec_local_config: Unable to Set Security status:%d\n", bsa_status);
    }

    return bsa_status;
}


/*******************************************************************************
 **
 ** Function         sec_bond
 **
 ** Description      This function initiates a bonding
 **
 ** Parameters       sec_bond_param: pointer on bonding parameter structure
 **
 ** Returns          int
 **
 *******************************************************************************/
int sec_bond(tSEC_BOND_PARAM *sec_bond_param)
{
    int                     status;
    tBSA_SEC_SET_SECURITY   set_security;
    tBSA_SEC_BOND           sec_bond;

    printf("sec_bond\n");
    printf("\nSet Bluetooth Security\n");

    BSA_SecSetSecurityInit(&set_security);
    set_security.simple_pairing_io_cap = BSA_SEC_IO_CAP_NONE;
    set_security.sec_cback = security_callback;
    status = BSA_SecSetSecurity(&set_security);
    printf("\nSet Bluetooth Security\n");
    if (status < 0)
    {
        fprintf(stderr, "sec_bond: Unable to Set Security status:%d\n", status);
        return status;
    }


    BSA_SecBondInit(&sec_bond);
    bdcpy(sec_bond.bd_addr, sec_bond_param->bd_addr);
    status = BSA_SecBond(&sec_bond);
    if (status < 0)
    {
        fprintf(stderr, "sec_bond: Unable to Bond status:%d\n", status);
        return status;
    }

    while(sec_auth_complete_rcv == FALSE);


    return status;
}

/*******************************************************************************
 **
 ** Function         sec_add_init_param
 **
 ** Description      This function initializes the add security structure
 **
 ** Parameters       sec_add_param: pointer on add security parameter structure
 **
 ** Returns          int
 **
 *******************************************************************************/
int sec_add_init_param(tSEC_ADD_PARAM *sec_add_param)
{
    printf("sec_add_init_param\n");
    bzero(sec_add_param, sizeof(tSEC_ADD_PARAM));
    sec_add_param->trusted_services = BSA_HID_SERVICE_MASK;
    sec_add_param->trusted = TRUE;
    sec_add_param->key_type = 0;
    sec_add_param->io_cap = BSA_SEC_IO_CAP_NONE;
    return 0;
}

/*******************************************************************************
 **
 ** Function         sec_add
 **
 ** Description      This function add device in security database
 **
 ** Parameters       sec_add_param: pointer on add security parameter structure
 **
 ** Returns          int
 **
 *******************************************************************************/
int sec_add(tSEC_ADD_PARAM *sec_add_param)
{
    int                 status;
    tBSA_SEC_ADD_DEV    bsa_add_dev_param;

    printf("sec_add\n");

    BSA_SecAddDeviceInit(&bsa_add_dev_param);

    bdcpy(bsa_add_dev_param.bd_addr, sec_add_param->bd_addr);
    memcpy(bsa_add_dev_param.class_of_device,
            sec_add_param->class_of_device,
            sizeof(DEV_CLASS));
    memcpy(bsa_add_dev_param.link_key,
            sec_add_param->link_key,
            sizeof(LINK_KEY));
    bsa_add_dev_param.link_key_present = sec_add_param->link_key_present;
    bsa_add_dev_param.trusted_services = sec_add_param->trusted_services;
    bsa_add_dev_param.is_trusted = TRUE;
    bsa_add_dev_param.key_type = sec_add_param->key_type;
    bsa_add_dev_param.io_cap = sec_add_param->io_cap;

    status = BSA_SecAddDevice(&bsa_add_dev_param);
    if (status < 0)
    {
        fprintf(stderr, "sec_add: Unable to Add device status:%d\n", status);
    }

    return status;
}

