/*****************************************************************************
**
**  Name:           discovery.h
**
**  Description:    Bluetooth discovery functions
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

typedef struct
{
    int     duration;
} tDISC_PARAM;

#define DISC_NB_DEVICES 20

extern int discovery_init_param(tDISC_PARAM *disc_param);
extern int discovery(tDISC_PARAM *disc_param);
extern tBSA_DISC_DEV disc_devices[DISC_NB_DEVICES];

