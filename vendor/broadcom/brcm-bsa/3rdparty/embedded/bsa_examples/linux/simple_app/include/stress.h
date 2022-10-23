/*****************************************************************************
**
**  Name:           stress.c
**
**  Description:    Bluetooth BSA Stress functions
**
**  Copyright (c) 2009-2016, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef _STRESS_H_
#define _STRESS_H_

typedef struct
{
    int nb_ping;
} tAPP_STRESS_PING;

typedef struct
{
    int nb_loop;
} tAPP_STRESS_HCI;

extern int app_stress_ping_init(tAPP_STRESS_PING *p_stress_ping);
extern int app_stress_ping(tAPP_STRESS_PING *p_stress_ping);

extern int app_stress_hci_init(tAPP_STRESS_HCI *p_stress_ping);
extern int app_stress_hci(tAPP_STRESS_HCI *p_stress_ping);

#endif
