/*****************************************************************************
**
**  Name:           vsc.c
**
**  Description:    Bluetooth VSC functions
**
**  Copyright (c) 2011-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsa_api.h"
#include "vsc.h"

/* #define VSC_DEBUG TRUE */

/*******************************************************************************
 **
 ** Function         vsc_send_init_param
 **
 ** Description      This function is used to initialize the VSC params
 **
 ** Parameters       Pointer to structure containing API parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int vsc_send_init_param(tBSA_TM_VSC *vsc_param)
{
#if defined(VSC_DEBUG) && (VSC_DEBUG == TRUE)
    printf("vsc_send_init_param\n");
#endif

    BSA_TmVscInit(vsc_param);
    return 0;
}

/*******************************************************************************
 **
 ** Function         vsc_send
 **
 ** Description      This function is used to send a Vendor Specific Command
 **
 ** Parameters       Pointer to structure containing API parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int vsc_send(tBSA_TM_VSC *vsc_param)
{
    tBSA_STATUS bsa_status;
    UINT8 op1,op2;
    UINT8 *p;
#if defined(VSC_DEBUG) && (VSC_DEBUG == TRUE)
    int index;

    printf("vsc_send\n");

    printf("VSC opcode:0x%03X\n", vsc_param->opcode);
    printf("VSC length:%d\n", vsc_param->length);
    if (vsc_param->length > 0)
    {
        printf("VSC Data:");
        for (index = 0 ; index < vsc_param->length ; index++)
        {
            printf ("%02X ", vsc_param->data[index]);
        }
        printf("\n");
    }
#endif

    bsa_status = BSA_TmVsc(vsc_param);
    if (bsa_status != BSA_SUCCESS)
    {
        fprintf(stderr, "vsc_send: Unable to Send VSC (status:%d)\n", bsa_status);
        return(-1);
    }

#if defined(VSC_DEBUG) && (VSC_DEBUG == TRUE)
    printf("VSC Server status:%d\n", vsc_param->status);
#endif
    printf("04\n");
    printf("0E 04\n");
    p = (UINT8 *)&vsc_param->opcode;
    STREAM_TO_UINT8(op1,p);
    STREAM_TO_UINT8(op2,p);
    printf("01 %02x %02x 00\n",op1,op2);
 
    return 0;
}



