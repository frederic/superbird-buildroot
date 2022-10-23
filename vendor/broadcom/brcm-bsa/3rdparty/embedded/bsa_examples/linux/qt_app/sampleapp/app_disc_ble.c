/*****************************************************************************
 **
 **  Name:           app_disc.c
 **
 **  Description:    Bluetooth Discovery functions
 **
 **  Copyright (c) 2009-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include "app_disc.h"

#include "bsa_api.h"
#include "app_xml_param.h"
#include "app_utils.h"
#include "app_services.h"
#include "app_utils.h"
#include "app_xml_utils.h"

/*
 * Service UUID 16 bits definitions (X-Macro)
 */
typedef struct
{
    UINT16 uuid;
    char *desc;
} tAPP_DISC_UUID16_DESC;
#define X(uuid, desc) {uuid, desc},
static tAPP_DISC_UUID16_DESC uuid16_desc[]=
{
#include "../../app_common/source/app_uuid16_desc.txt"
};

/*******************************************************************************
 **
 ** Function         app_disc_get_uuid16_desc
 **
 ** Description      This function search UUID16 description
 **
 ** Returns          String pointer
 **
 *******************************************************************************/
char *app_disc_get_uuid16_desc(UINT16 uuid16)
{
    int index;
    int max = sizeof(uuid16_desc) / sizeof(uuid16_desc[0]);
    tAPP_DISC_UUID16_DESC *p_uuid16_desc = uuid16_desc;

    for(index = 0 ; index < max ; index++, p_uuid16_desc++)
    {
        if (p_uuid16_desc->uuid == uuid16)
        {
            return p_uuid16_desc->desc;
        }
    }
    return NULL;
}

extern void BleDiscNewService16(UINT16 uuid16,char * desc, BD_ADDR bda);

/*******************************************************************************
 **
 ** Function         app_disc_parse_eir_uuid16
 **
 ** Description      This function is used to parse EIR UUID16 data
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_disc_parse_eir_uuid16(char *p_type, UINT8 *p_eir, UINT8 data_length, tBSA_DISC_NEW_MSG *disc_new)
{
    UINT16 uuid16;
    char *p_desc;

    APP_INFO1("\t    %s [UUID16]:", p_type);
    while (data_length >= sizeof(UINT16))
    {
        STREAM_TO_UINT16(uuid16, p_eir);
        data_length -= sizeof(UINT16);
        p_desc = app_disc_get_uuid16_desc(uuid16);
        if (disc_new->device_type == BT_DEVICE_TYPE_BLE)
            BleDiscNewService16(uuid16,p_desc,disc_new->bd_addr);
    }
}

void app_disc_parse_eir_new(tBSA_DISC_NEW_MSG *disc_new)
{
    UINT8 *p_eir = disc_new->eir_data;
    UINT8 *p = p_eir;
    UINT8 eir_length;
    UINT8 eir_tag;

    while(1)
    {
        /* Read Tag's length */
        STREAM_TO_UINT8(eir_length, p);
        if (eir_length == 0)
        {
            break;    /* Last Tag */
        }
        eir_length--;

        /* Read Tag Id */
        STREAM_TO_UINT8(eir_tag, p);

        switch(eir_tag)
        {
            case HCI_EIR_MORE_16BITS_UUID_TYPE:
                app_disc_parse_eir_uuid16("Incomplete Service", p, eir_length,disc_new);
                break;
            case HCI_EIR_COMPLETE_16BITS_UUID_TYPE:
                app_disc_parse_eir_uuid16("Complete Service", p, eir_length,disc_new);
                break;
        }
        p += eir_length;
    }
}


