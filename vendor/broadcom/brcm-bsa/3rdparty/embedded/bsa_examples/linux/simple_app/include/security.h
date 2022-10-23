/*****************************************************************************
**
**  Name:           security.h
**
**  Description:    Bluetooth security functions
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/


typedef struct
{
    BD_ADDR         bd_addr;
} tSEC_BOND_PARAM;

typedef struct
{
    BD_ADDR             bd_addr;
    LINK_KEY            link_key;
    BOOLEAN             link_key_present;
    tBSA_SERVICE_MASK   trusted_services;
    BOOLEAN             trusted;
    unsigned char       key_type;
    tBSA_SEC_IO_CAP     io_cap;
    DEV_CLASS           class_of_device;
} tSEC_ADD_PARAM;

extern int sec_bond_init_param(tSEC_BOND_PARAM *sec_bond_param);
extern int sec_bond(tSEC_BOND_PARAM *sec_bond_param);

extern int sec_add_init_param(tSEC_ADD_PARAM *sec_add_param);
extern int sec_add(tSEC_ADD_PARAM *sec_add_param);
extern int sec_set_local_config(tBSA_SEC_SET_SECURITY* p_sec_local_config);
