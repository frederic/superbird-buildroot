/*****************************************************************************
**
**  Name:           bta_dm_co.h
**
**  Description:    This is the interface file for device mananger audio callout
**                  functions.
**
**  Copyright (c) 2006-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_DM_CO_H
#define BTA_DM_CO_H

#include "bta_sys.h"
#include "btm_api.h"

/******************
**  eSCO Constants
*******************/

/************************************************************************************************/

#ifndef BTA_SCO_OUT_PKT_SIZE
#define BTA_SCO_OUT_PKT_SIZE    BTM_SCO_DATA_SIZE_MAX
#endif

/*****************************************************************************
**  Function Declarations
*****************************************************************************/

/*******************************************************************************
**
** Function         bta_dm_co_io_req
**
** Description      This callout function is executed by DM to get IO capabilities
**                  of the local device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req - TRUE, if MITM protection is required.
**
** Returns          void.
**
*******************************************************************************/
BTA_API extern void bta_dm_co_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap,
                                     tBTA_OOB_DATA *p_oob_data,
                                     tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig);

/*******************************************************************************
**
** Function         bta_dm_co_io_rsp
**
** Description      This callout function is executed by DM to report IO capabilities
**                  of the peer device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  io_cap - The remote Input/Output capabilities
**                  oob_data - TRUE, if OOB data is available for the peer device.
**                  auth_req - TRUE, if MITM protection is required.
**
** Returns          void.
**
*******************************************************************************/
BTA_API extern void bta_dm_co_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap,
                                     tBTA_OOB_DATA oob_data, tBTA_AUTH_REQ auth_req);

/*******************************************************************************
**
** Function         bta_dm_co_lk_upgrade
**
** Description      This callout function is executed by DM to check if the
**                  platform wants allow link key upgrade
**
** Parameters       bd_addr  - The peer device
**                  *p_upgrade - TRUE, if link key upgrade is desired.
**
** Returns          void.
**
*******************************************************************************/
BTA_API extern void  bta_dm_co_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade );

/*******************************************************************************
**
** Function         bta_dm_co_loc_oob
**
** Description      This callout function is executed by DM to report the OOB
**                  data of the local device for the Simple Pairing process
**
** Parameters       valid - TRUE, if the local OOB data is retrieved from LM
**                  c     - Simple Pairing Hash C
**                  r     - Simple Pairing Randomnizer R
**
** Returns          void.
**
*******************************************************************************/
BTA_API extern void bta_dm_co_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r);

#if (BTM_BR_SC_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_loc_oob_ext
**
** Description      This callout function is executed by DM to report the OOB
**                  extended data of the local device for the Simple Pairing process
**                  This function is called instead of bta_dm_co_loc_oob(...)
**                  if the local device supports Secure Connections (SC)
**
** Parameters       valid - TRUE, if the local OOB extended data is retrieved from LM
**                  c_192 - Simple Pairing Hash C derived from the P-192 public key
**                  r_192 - Simple Pairing Randomnizer R associated with the P-192
**                          public key
**                  c_256 - Simple Pairing Hash C derived from the P-256 public key
**                  r_256 - Simple Pairing Randomnizer R associated with the P-256
**                          public key
**
** Returns          void.
**
*******************************************************************************/
BTA_API extern void bta_dm_co_loc_oob_ext(BOOLEAN valid, BT_OCTET16 c_192,
                                          BT_OCTET16 r_192, BT_OCTET16 c_256,
                                          BT_OCTET16 r_256);
#endif

/*******************************************************************************
**
** Function         bta_dm_co_rmt_oob
**
** Description      This callout function is executed by DM to request the OOB
**                  data for the remote device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**
** Returns          TRUE    if the data is successfully retrieved,
**                  FALSE   otherwise.
**
*******************************************************************************/
BTA_API extern BOOLEAN bta_dm_co_rmt_oob(BD_ADDR bd_addr);

/*****************************************************************************
**  SCO over HCI Function Declarations
*****************************************************************************/
/*******************************************************************************
**
** Function         bta_dm_sco_co_init
**
** Description      This function can be used by the platform to initialize audio
**                  codec or for other initialization purposes before SCO connection
**                  is opened.
**
**                  Note: Input Data Path is configured in this callout function
**                  p_parms->[in/out]put_data_path
**                          BTM_DATA_PATH_PCM, BTM_DATA_PATH_HCI, BTM_DATA_PATH_TEST
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_dm_sco_co_init(tBTM_ENH_ESCO_PARAMS *p_parms, UINT8 app_id);

/*******************************************************************************
**
** Function         bta_dm_sco_co_open
**
** Description      This function is executed when a SCO connection is open.
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_dm_sco_co_open(UINT16 handle, UINT8 pkt_size, UINT16 event);

/*******************************************************************************
**
** Function         bta_dm_sco_co_close
**
** Description      This function is called when a SCO connection is closed
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_dm_sco_co_close(void);

/*******************************************************************************
**
** Function         bta_dm_sco_co_out_data
**
** Description      This function is called to send SCO data over HCI.
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_dm_sco_co_out_data(BT_HDR  **p_buf);

/*******************************************************************************
**
** Function         bta_dm_sco_co_in_data
**
** Description      This function is called to send incoming SCO data to application.
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_dm_sco_co_in_data(BT_HDR  *p_buf, tBTM_SCO_DATA_FLAG status);



/*******************************************************************************
**
** Function         bta_dm_co_ble_io_req
**
** Description      This callout function is executed by DM to get BLE IO capabilities
**                  before SMP pairing gets going.
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req - Auth req setting (Bonding and MITM required or not)
**                  *p_max_key_size - max key size local device supported.
**                  *p_init_key - initiator keys.
**                  *p_resp_key - responder keys.
**
** Returns          void.
**
*******************************************************************************/
BTA_API extern void bta_dm_co_ble_io_req(BD_ADDR bd_addr,  tBTA_IO_CAP *p_io_cap,
                                        tBTA_OOB_DATA *p_oob_data,
                                        tBTA_LE_AUTH_REQ *p_auth_req,
                                        UINT8 *p_max_key_size,
                                        tBTA_LE_KEY_TYPE *p_init_key,
                                        tBTA_LE_KEY_TYPE  *p_resp_key);


/*******************************************************************************
**
** Function         bta_dm_co_ble_local_key_reload
**
** Description      This callout function is to load the local BLE keys if available
**                  on the device.
**
** Parameters       none
**
** Returns          void.
**
*******************************************************************************/
BTA_API extern
    void bta_dm_co_ble_load_local_keys (tBTA_DM_BLE_LOCAL_KEY_MASK *p_key_mask,
                                        BT_OCTET16 er,
                                        tBTA_BLE_LOCAL_ID_KEYS *p_id_keys);


#endif
