/*****************************************************************************
**
**  Name:           dm.c
**
**  Description:    Bluetooth DM functions
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "bsa_api.h"
#include "dm.h"


/*
 * Global data/define
 */
/* Default BdAddr */
#define APP_DEFAULT_BD_ADDR             {0xBE, 0xEF, 0xBE, 0xEF, 0x00, 0x01}

/* Default local Name */
#define APP_DEFAULT_BT_NAME             "BSA Bluetooth Device"

/* Default COD SetTopBox (Major Service = none) (MajorDevclass = Audio/Video) (Minor=STB) */
#define APP_DEFAULT_CLASS_OF_DEVICE     {0x00, 0x04, 0x24}

/*******************************************************************************
 **
 ** Function         dm_set_config_init_param
 **
 ** Description      This function initialize the configuration parameters
 **
 ** Parameters       dm_config_param: pointer on config parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int dm_set_config_init_param(tBSA_DM_SET_CONFIG *dm_config_param)
{
    BD_ADDR         local_bd_addr = APP_DEFAULT_BD_ADDR;
    DEV_CLASS       local_class_of_device = APP_DEFAULT_CLASS_OF_DEVICE;

    printf("dm_config_init_param\n");

    BSA_DmSetConfigInit(dm_config_param);

    bdcpy(dm_config_param->bd_addr, local_bd_addr);
    strncpy((char *)dm_config_param->name, APP_DEFAULT_BT_NAME,sizeof(dm_config_param->name));
    memcpy(&dm_config_param->class_of_device[0],
            local_class_of_device, sizeof(DEV_CLASS));

    printf ("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
             dm_config_param->bd_addr[0], dm_config_param->bd_addr[1],
             dm_config_param->bd_addr[2], dm_config_param->bd_addr[3],
             dm_config_param->bd_addr[4], dm_config_param->bd_addr[5]);

    return 0;
}

/*******************************************************************************
 **
 ** Function         dm_set_config
 **
 ** Description      This function sets the configuration parameters
 **
 ** Parameters       dm_config_param: pointer on config parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int dm_set_config(tBSA_DM_SET_CONFIG *dm_config_param)
{
    tBSA_STATUS                 status;

    printf("dm_config\n");
    /*
     * Set bluetooth configuration
     */
    printf("Set Local Bluetooth Config:\n");
    printf("\tEnable:%d\n", dm_config_param->enable);
    printf("\tDiscoverable:%d\n", dm_config_param->discoverable);
    printf("\tConnectable:%d\n", dm_config_param->connectable);
    printf("\tName:%s\n", dm_config_param->name);
    printf ("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
             dm_config_param->bd_addr[0], dm_config_param->bd_addr[1],
             dm_config_param->bd_addr[2], dm_config_param->bd_addr[3],
             dm_config_param->bd_addr[4], dm_config_param->bd_addr[5]);
    printf("\tClassOfDevice:%02x:%02x:%02x\n", dm_config_param->class_of_device[0],
            dm_config_param->class_of_device[1], dm_config_param->class_of_device[2]);

    status = BSA_DmSetConfig(dm_config_param);
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "dm_config: Unable to set BT config to server status:%d\n", status);
    }

    return status;
}

