/*****************************************************************************
 **
 **  Name:           app_tvwakeup.c
 **
 **  Description:    TV WakeUp utility functions for applications
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include "app_tvwakeup.h"

/*
 * Definitions
 */
/* Broadcom VSC which enable/disable VSync detection (for manufacturer tests) */
#define BRCM_HCI_VSC_TVWAKEUP           0x0137

#define BRCM_TVWAKEUP_CMD_RESET         0x00
#define BRCM_TVWAKEUP_CMD_ADD           0x01
#define BRCM_TVWAKEUP_CMD_REMOVE        0x02

/*
 * Global variables
 */

/*
 * Local functions
 */

/*******************************************************************************
 **
 ** Function        app_tvwakeup_add_send
 **
 ** Description     Send an Host WakeUp Add Device command to the local BT Chip
 **
 ** Parameters      Host WakeUp Add for a given peer device
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_tvwakeup_add_send(BD_ADDR bdaddr, LINK_KEY link_key,
        UINT8 *p_class_of_device, tAPP_TVWAKEUP_PROFILE *p_profile,
        UINT8 profile_nb)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p_data;
    UINT8 profile;
    UINT8 record;
    UINT8 vsc_len = 0;
    int i;
    tAPP_TVWAKEUP_RECORD *p_record;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = BRCM_HCI_VSC_TVWAKEUP;
    p_data = bsa_vsc.data;

    /* Write the Add Command */
    UINT8_TO_STREAM(p_data, BRCM_TVWAKEUP_CMD_ADD);

    /* Write BdAddr parameter */
    BDADDR_TO_STREAM(p_data, bdaddr);

    /* Write LinkKey parameter */
    /* Copy LinkKey in HCI format (LSB first) */
    for (i = 0 ; i < LINK_KEY_LEN ; i++)
    {
        *p_data++ = link_key[LINK_KEY_LEN -1 - i];
    }

    /* Write Class Of Device parameter */
    DEVCLASS_TO_STREAM(p_data, p_class_of_device);

    /* Write Number of Profile parameter */
    UINT8_TO_STREAM(p_data, profile_nb);

    vsc_len = sizeof(UINT8) + sizeof(BD_ADDR) + sizeof(LINK_KEY) +
              sizeof(DEV_CLASS) + sizeof(UINT8);

    /* For every Profile */
    for (profile = 0 ; profile < profile_nb ; profile++, p_profile++)
    {
        if (p_profile->records_nb > APP_TVWAKEUP_RECORD_NB_MAX)
        {
            APP_ERROR1("app_tvwakeup_send records_nb:%d for profile:%d unsupported",
                    p_profile->records_nb , profile);
            return(-1);
        }

        /* Write Profile Id */
        UINT8_TO_STREAM(p_data, p_profile->id);
        vsc_len += sizeof(UINT8);

        /* Write Number of Records */
        UINT8_TO_STREAM(p_data, p_profile->records_nb);
        vsc_len += sizeof(UINT8);

        /* For every Record */
        for (record = 0 ; record < p_profile->records_nb ; record++)
        {
            p_record = &p_profile->records[record];

            if (p_record->len > APP_TVWAKEUP_RECORD_LEN_MAX)
            {
                APP_ERROR1("app_tvwakeup_send records_len:%d for record:%d"
                        " profile:%d unsupported",
                        p_record->len, record , profile);
                return(-1);
            }
            /* Write Record Length */
            UINT8_TO_STREAM(p_data, p_record->len);
            vsc_len += sizeof(UINT8);

            /* Write Record Data */
            ARRAY_TO_STREAM(p_data, p_record->data, p_record->len);
            vsc_len += p_record->len;
        }
    }

    bsa_vsc.length = vsc_len;

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_tvwakeup_add_send BSA_TmVsc failed:%d", bsa_status);
        return(-1);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_tvwakeup_remove_send
 **
 ** Description     Send an TV WakeUp Remove Device command to the local BT Chip
 **
 ** Parameters      BT Address of the peer device to remove
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_tvwakeup_remove_send(BD_ADDR bdaddr)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p_data;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = BRCM_HCI_VSC_TVWAKEUP;
    p_data = bsa_vsc.data;

    /* Write the Add Command */
    UINT8_TO_STREAM(p_data, BRCM_TVWAKEUP_CMD_REMOVE);

    /* Write BdAddr parameter */
    BDADDR_TO_STREAM(p_data, bdaddr);

    bsa_vsc.length = sizeof(UINT8) + sizeof(BD_ADDR);

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_tvwakeup_remove_send BSA_TmVsc failed:%d", bsa_status);
        return(-1);
    }

    return 0;

}

/*******************************************************************************
 **
 ** Function        app_tvwakeup_clear_send
 **
 ** Description     Send an TV WakeUp Clear Device command to the local BT Chip
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_tvwakeup_clear_send(void)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p_data;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = BRCM_HCI_VSC_TVWAKEUP;
    p_data = bsa_vsc.data;

    /* Write the Add Command */
    UINT8_TO_STREAM(p_data, BRCM_TVWAKEUP_CMD_RESET);

    bsa_vsc.length = sizeof(UINT8);

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("app_tvwakeup_clear_send BSA_TmVsc failed:%d", bsa_status);
        return(-1);
    }

    return 0;
}


