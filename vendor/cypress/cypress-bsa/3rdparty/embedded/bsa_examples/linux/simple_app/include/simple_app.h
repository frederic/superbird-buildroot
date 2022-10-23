/*****************************************************************************
**
**  Name:           simple_app.h
**
**  Description:    Simple application header
**
**  Copyright (c) 2009-2016, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef SIMPLE_APP_H
#define SIMPLE_APP_H

/* For data types */
#include "bt_target.h"

#include "bsa_api.h"

typedef enum
{
    NO_COMMAND = 0,
    HELP,
    CONFIG,
    DISCOVERY,
    SEC_BOND,
    SEC_ADD,
    HH_OPEN,
    HH_WOPEN,
    STRESS_PING,
    STRESS_HCI,
    AV_PLAY_TONE,
    VSC,
    HCI_CMD
} BSA_COMMAND;

typedef struct
{
    BSA_COMMAND bsa_command;
    BOOLEAN bd_addr_present;
    BD_ADDR device_bd_addr;
    BOOLEAN link_key_present;
    LINK_KEY link_key;
    char pin_code[PIN_CODE_LEN];
    int pin_len;
    int loop;
    tBSA_TM_VSC bsa_vsc;
    tBSA_TM_HCI_CMD bsa_hci;
} tSIMPLE_APP_CB;

extern tSIMPLE_APP_CB simple_app;

#endif
