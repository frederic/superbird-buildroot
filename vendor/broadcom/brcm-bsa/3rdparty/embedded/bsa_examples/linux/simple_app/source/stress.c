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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bsa_api.h"
#include "stress.h"


/*******************************************************************************
 **
 ** Function         app_stress_ping_init
 **
 ** Description      initializes ping structure
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_stress_ping_init(tAPP_STRESS_PING *p_stress_ping)
{
    memset(p_stress_ping, 0, sizeof(tAPP_STRESS_PING));
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_stress_ping
 **
 ** Description      initiates a ping
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_stress_ping(tAPP_STRESS_PING *p_stress_ping)
{
    int                 status;
    tBSA_TM_PING        bsa_ping_param;
    int                 index;

    printf("app_stress_ping\n");


    status = BSA_TmPingInit(&bsa_ping_param);
    printf("starting to ping BT server daemon %d times\n", p_stress_ping->nb_ping);
    for (index = 0 ; index < p_stress_ping->nb_ping ; index++)
    {
        status = BSA_TmPing(&bsa_ping_param);
        if (status != BSA_SUCCESS)
        {
            fprintf(stderr, "bsa_ping fail status:%d loop:%d\n", status, index);
            sleep(1);
            return status;
        }
    }
    printf("ping test done\n");

    return status;
}


/*******************************************************************************
 **
 ** Function         app_stress_hci_init
 **
 ** Description      initializes stress HCI structure
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_stress_hci_init(tAPP_STRESS_HCI *p_stress_hci)
{
    memset(p_stress_hci, 0, sizeof(*p_stress_hci));
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_stress_hci
 **
 ** Description      initiates a stress HCI
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_stress_hci(tAPP_STRESS_HCI *p_stress_hci)
{
    int                 status;
    tBSA_TM_VSC         vsc_param;
    int                 index;

    printf("app_stress_hci\n");

    printf("starting to Stress HCI %d times\n", p_stress_hci->nb_loop);

    for (index = 0 ; index < p_stress_hci->nb_loop ; index++)
    {
        BSA_TmVscInit(&vsc_param);

        vsc_param.opcode = 0xFC79;      /* HCI Opcode : ReadVerboseConfigVersionInfo */
        vsc_param.length = 0;           /* Data Len */

        status = BSA_TmVsc(&vsc_param);
        if (status != BSA_SUCCESS)
        {
            fprintf(stderr, "VSC fail status:%d loop:%d\n", status, index);
            return -1;
        }
    }
    printf("HCI Stress test done\n");

    return 0;
}

