/*****************************************************************************
**
**  Name:           hci_cmd.c
**
**  Description:    Bluetooth HCI functions
**
**  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsa_api.h"
#include "hci_cmd.h"

#define HCI_DEBUG TRUE

/*******************************************************************************
 **
 ** Function         hci_send_init_param
 **
 ** Description      This function is used to initialize the HCI params
 **
 ** Parameters       Pointer to structure containing API parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int hci_send_init_param(tBSA_TM_HCI_CMD *hci_param)
{
#if defined(HCI_DEBUG) && (HCI_DEBUG == TRUE)
    printf("hci_send_init_param\n");
#endif

    BSA_TmHciInit(hci_param);
    return 0;
}

/*******************************************************************************
 **
 ** Function         hci_send
 **
 ** Description      This function is used to send a Vendor Specific Command
 **
 ** Parameters       Pointer to structure containing API parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int hci_send(tBSA_TM_HCI_CMD *hci_param)
{
    tBSA_STATUS bsa_status;
    UINT8 i;
#if defined(HCI_DEBUG) && (HCI_DEBUG == TRUE)
    int index;

    printf("hci_send\n");

    printf("HCI opcode:0x%04X\n", ((UINT8)hci_param->opcode << 8) | (UINT8)(hci_param->opcode >> 8));
    printf("HCI length:%d\n", hci_param->length);
    printf("HCI Data:");
    for (index = 0 ; index < hci_param->length ; index++)
    {
        printf ("%02X ", hci_param->data[index]);
    }
    printf("\n");
#endif

    bsa_status = BSA_TmHciCmd(hci_param);
    if (bsa_status != BSA_SUCCESS)
    {
        fprintf(stderr, "hci_send: Unable to Send HCI (status:%d)\n", bsa_status);
        return(-1);
    }

#if defined(HCI_DEBUG) && (HCI_DEBUG == TRUE)
    printf("HCI Server status:%d\n", bsa_status);
    printf("opcode : 0x%04X\n", hci_param->opcode);
    printf("length : %02X\n", hci_param->length & 0xff);
    printf("status : %02X\n", hci_param->status);
    printf("Data : \n");
    for(i=0; i<hci_param->length; i++)
    {
        printf("%02X ",hci_param->data[i]);
        if(i!=0 && (i%15) == 0)
            printf("\n");
    }
    printf("\n");
#endif
    return 0;
}



