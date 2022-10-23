/*****************************************************************************
 **
 **  Name:           discovery.c
 **
 **  Description:    Bluetooth Discovery functions
 **
 **  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "bsa_api.h"
#include "discovery.h"

/*
 * Definitions
 */

/*
 * Global data
 */
static int discovery_done = FALSE;

tBSA_DISC_DEV disc_devices[DISC_NB_DEVICES];

/*******************************************************************************
 **
 ** Function         disc_cback
 **
 ** Description      This function is the discovery events callback
 **
 ** Parameters       event: discovery event
 **                  p_data: discovery event data pointer
 ** Returns          void
 **
 *******************************************************************************/
void disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data)
{
    int index;
    switch (event)
    {
    /* a New Device has been discovered */
    case BSA_DISC_NEW_EVT:
        for(index=0;index<DISC_NB_DEVICES; index++)
        {
            if(disc_devices[index].in_use == FALSE)
            {
                break;
            }
        }

        if(index < DISC_NB_DEVICES)
        {
            printf("New Discovered device:\n");
            printf("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x",
                    p_data->disc_new.bd_addr[0], p_data->disc_new.bd_addr[1],
                    p_data->disc_new.bd_addr[2], p_data->disc_new.bd_addr[3],
                    p_data->disc_new.bd_addr[4], p_data->disc_new.bd_addr[5]);
            printf("\tName:%s\n", p_data->disc_new.name);
            printf("\tClassOfDevice:%02x:%02x:%02x",
                    p_data->disc_new.class_of_device[0],
                    p_data->disc_new.class_of_device[1],
                    p_data->disc_new.class_of_device[2]);
            printf("\tRssi:%d\n", p_data->disc_new.rssi);
            bdcpy(disc_devices[index].device.bd_addr,p_data->disc_new.bd_addr);
            disc_devices[index].device.rssi = p_data->disc_new.rssi;
            memcpy(disc_devices[index].device.class_of_device,
                   p_data->disc_new.class_of_device,
                   sizeof(disc_devices[index].device.class_of_device));
            disc_devices[index].in_use = TRUE;
        }
        else
        {
            printf("Could not save inquiry information for bd address %02x:%02x:%02x:%02x:%02x:%02x\n",
                        p_data->disc_new.bd_addr[0], p_data->disc_new.bd_addr[1],
                        p_data->disc_new.bd_addr[2], p_data->disc_new.bd_addr[3],
                        p_data->disc_new.bd_addr[4], p_data->disc_new.bd_addr[5]);
        }
        break;

    case BSA_DISC_CMPL_EVT: /* Discovery complete. */
        printf("Discovery complete\n");
        discovery_done = TRUE;
        break;

    default:
        fprintf(stderr, "app_disc_cback unknown event:%d\n", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         discovery_init_param
 **
 ** Description      This function initializes the discovery parameter structure
 **
 ** Parameters       p_disc_param: pointer on discovery parameter structure
 ** Returns          int
 **
 *******************************************************************************/
int discovery_init_param(tDISC_PARAM *p_disc_param)
{
    printf("discovery_init_param\n");
    bzero(p_disc_param, sizeof(tDISC_PARAM));
    p_disc_param->duration = 4;
    return 0;
}

/*******************************************************************************
 **
 ** Function         discovery
 **
 ** Description      This function initiates a discovery
 **
 ** Parameters       p_disc_param: pointer on discovery parameter structure
 ** Returns          int
 **
 *******************************************************************************/
int discovery(tDISC_PARAM *p_disc_param)
{
    int status;
    tBSA_DISC_START disc_start_param;

    printf("discovery\n");

    BSA_DiscStartInit(&disc_start_param);

    disc_start_param.cback = disc_cback;
    disc_start_param.nb_devices = 0;
    disc_start_param.duration = p_disc_param->duration;
    /*
     * Start Discovery
     */
    printf("\nStart Discovery\n");
    status = BSA_DiscStart(&disc_start_param);
    if (status < 0)
    {
        fprintf(stderr, "main: Unable to Start Discovery status:%d\n", status);
        return -1;
    }

    while (discovery_done == FALSE)
    {
        sleep(1);
    }

    return status;
}
