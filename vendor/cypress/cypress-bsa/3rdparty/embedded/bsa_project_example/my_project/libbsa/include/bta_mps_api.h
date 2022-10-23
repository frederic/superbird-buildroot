/*****************************************************************************
**
**  Name:           bta_mps_api.h
**
**  Description:    This is the public interface file for BTA, Widcomm's
**                  Bluetooth application layer for mobile phones.
**
**  Copyright (c) 2014-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_MPS_API_H
#define BTA_MPS_API_H

#include "bt_types.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#define BTA_MPS_MPMD_UNSUPPORTED    ((UINT8) 0x00)
#define BTA_MPS_MPMD_SUPPORTED      ((UINT8) 0x01)

#define BTA_MPS_VERSION_1_0         ((UINT16) 0x0100)
#define BTA_MPS_VERSION_1_1         ((UINT16) 0x0101)

#ifndef BTA_MPS_VERSION
#define BTA_MPS_VERSION             BTA_MPS_VERSION_1_0
#endif

/***************************************************
**   Supported Features bit mask for MPSD scenarios
**      (64-bits)
****************************************************/

/* Byte 0 */
/* Answer Incoming Call during Audio Streaming (AG/SRC) */
#define MPSD_FEAT_INCCALL_AUDSTR_AGSRC_BIT      ((UINT64)1 << 0)

/* Answer Incoming Call during Audio Streaming (HF/SNK) */
#define MPSD_FEAT_INCCALL_AUDSTR_HFSNK_BIT      ((UINT64)1 << 1)

/* Outgoing Call during Audio Streaming (AG/SRC) */
#define MPSD_FEAT_OUTCALL_AUDSTR_AGSRC_BIT      ((UINT64)1 << 2)

/* Outgoing Call during Audio Streaming (HF/SNK) */
#define MPSD_FEAT_OUTCALL_AUDSTR_HFSNK_BIT      ((UINT64)1 << 3)

/* Reject/Ignore Incoming Call during Audio Streaming (AG/SRC) */
#define MPSD_FEAT_REJCALL_AUDSTR_AGSRC_BIT      ((UINT64)1 << 4)

/* Reject/Ignore Incoming Call during Audio Streaming (HF/SNK) */
#define MPSD_FEAT_REJCALL_AUDSTR_HFSNK_BIT      ((UINT64)1 << 5)

/* HFP call termination during AVP connection (AG/SRC) */
#define MPSD_FEAT_CALLTERM_AVP_AGSRC_BIT        ((UINT64)1 << 6)

/* HFP call termination during AVP connection (HF/SNK) */
#define MPSD_FEAT_CALLTERM_AVP_HFSNK_BIT        ((UINT64)1 << 7)

/* Byte 1 */
/* Press Play on Audio Player during active call (AG/SRC) */
#define MPSD_FEAT_PLAY_DUR_CALL_AGSRC_BIT       ((UINT64)1 << 8)

/* Press Play on Audio Player during active call (HF/SNK) */
#define MPSD_FEAT_PLAY_DUR_CALL_HFSNK_BIT       ((UINT64)1 << 9)

/* Start Audio Streaming after AVRCP Play Command (AG/SRC) */
#define MPSD_FEAT_AUDSTR_AVPLAY_AGSRC_BIT       ((UINT64)1 << 10)

/* Start Audio Streaming after AVRCP Play Command (HF/SNK) */
#define MPSD_FEAT_AUDSTR_AVPLAY_HFSNK_BIT       ((UINT64)1 << 11)

/* Suspend Audio Streaming after AVRCP Pause/Stop (AG/SRC) */
#define MPSD_FEAT_SUSSTR_AVPAUSE_AGSRC_BIT      ((UINT64)1 << 12)

/* Suspend Audio Streaming after AVRCP Pause/Stop (HF/SNK) */
#define MPSD_FEAT_SUSSTR_AVPAUSE_HFSNK_BIT      ((UINT64)1 << 13)

/* Data communication under PSDM (DUN) during active voice call (AG/GW) */
#define MPSD_FEAT_DUN_DURCALL_AGGW_BIT          ((UINT64)1 << 14)

/* Data communication under PSDM (DUN) during active voice call (HF/DT) */
#define MPSD_FEAT_DUN_DURCALL_HFDT_BIT          ((UINT64)1 << 15)


/* Byte 2 */
/* Outgoing voice call during Data communication under PSDM (DUN) (AG/GW) */
#define MPSD_FEAT_OUTCALL_DATA_RATE_AGGW_BIT    ((UINT64)1 << 16)

/* Outgoing voice call during Data communication under PSDM (DUN) (HF/DT) */
#define MPSD_FEAT_OUTCALL_DATA_RATE_HFDT_BIT    ((UINT64)1 << 17)

/* Incoming voice call during Data communication under PSDM (DUN) (AG/GW) */
#define MPSD_FEAT_INCCALL_DUN_AGGW_BIT          ((UINT64)1 << 18)

/* Incoming voice call during Data communication under PSDM (DUN) (HF/DT) */
#define MPSD_FEAT_INCCALL_DUN_HFDT_BIT          ((UINT64)1 << 19)

/* Start Audio Streaming during Data communication under PSDM (DUN) (SRC/GW) */
#define MPSD_FEAT_STRTSTRM_DUN_SRCGW_BIT        ((UINT64)1 << 20)

/* Start Audio Streaming during Data communication under PSDM (DUN) (SNK/DT) */
#define MPSD_FEAT_STRTSTRM_DUN_SNKDT_BIT        ((UINT64)1 << 21)

/* Data communication establishment under PSDM (DUN) during Audio Streaming (SRC/GW) */
#define MPSD_FEAT_DUN_AUDSTRM_SRCGW_BIT         ((UINT64)1 << 22)

/* Data communication establishment under PSDM (DUN) during Audio Streaming (SNK/DT) */
#define MPSD_FEAT_DUN_AUDSTRM_SNKDT_BIT         ((UINT64)1 << 23)


/* Byte 3 */
/* Terminate voice call / data call during data communication and voice call (AG/GW) */
#define MPSD_FEAT_CALLTERM_DUN_AGGW_BIT         ((UINT64)1 << 24)

/* Terminate voice call / data call during data communication and voice call (HF/DT) */
#define MPSD_FEAT_CALLTERM_DUN_HFDT_BIT         ((UINT64)1 << 25)

/* Data communication in Personal Area Network during active voice call (AG/NAP) */
#define MPSD_FEAT_PAN_DURCALL_AGNAP_BIT         ((UINT64)1 << 26)

/* Data communication in Personal Area Network during active voice call (HF/PANU) */
#define MPSD_FEAT_PAN_DURCALL_HFPANU_BIT        ((UINT64)1 << 27)

/* Outgoing voice call during Data communication in Personal Area Network (AG/NAP) */
#define MPSD_FEAT_OUTCALL_PAN_AGNAP_BIT         ((UINT64)1 << 28)

/* Outgoing voice call during Data communication in Personal Area Network (HF/PANU) */
#define MPSD_FEAT_OUTCALL_PAN_HFPANU_BIT        ((UINT64)1 << 29)

/* Incoming voice call during Data communication in Personal Area Network (AG/NAP) */
#define MPSD_FEAT_INCCALL_PAN_AGNAP_BIT         ((UINT64)1 << 30)

/* Incoming voice call during Data communication in Personal Area Network (HF/PANU) */
#define MPSD_FEAT_INCCALL_PAN_HFPANU_BIT        ((UINT64)1 << 31)


/* Byte 4 */
/* Start Audio Streaming during Data communication under PSDM (PAN) (SRC/NAP) */
#define MPSD_FEAT_STRTSTRM_PAN_SRCNAP_BIT       ((UINT64)1 << 32)

/* Start Audio Streaming during Data communication under PSDM (PAN) (SNK/PANU) */
#define MPSD_FEAT_STRTSTRM_PAN_SNKPANU_BIT      ((UINT64)1 << 33)

/* Data communication establishment under PSDM (PAN) during Audio Streaming (SRC/NAP) */
#define MPSD_FEAT_PAN_AUDSTRM_SRCNAP_BIT        ((UINT64)1 << 34)

/* Data communication establishment under PSDM (PAN) during Audio Streaming (SNK/PANU) */
#define MPSD_FEAT_PAN_AUDSTRM_SNKPANU_BIT       ((UINT64)1 << 35)

/* Phonebook Download during Audio Streaming (SRC/PSE) */
#define MPSD_FEAT_AUDSTRM_PB_SRCPSE_BIT         ((UINT64)1 << 36)

/* Phonebook Download during Audio Streaming (SNK/PCE) */
#define MPSD_FEAT_AUDSTRM_PB_SNKPCE_BIT         ((UINT64)1 << 37)

/* Byte 5 */
/* Byte 6 */
/* Byte 7 */

/* ( Bits 38-63 are RFU ) */
/****************************************************************
** End of MPSD bits defined in D0.9 R09 version of spec (Oct 2012)
*****************************************************************/

/***************************************************
**   Supported Features bit mask for MPMD scenarios
**      (64-bits)
****************************************************/

/* Byte 0 */
/* Answer Incoming Call during Audio Streaming (HF/SNK/CT) */
#define MPMD_FEAT_INCCALL_AUDSTR_HFSNK_BIT      ((UINT64)1 << 0)

/* Answer Incoming Call during Audio Streaming (SRC/TG) */
#define MPMD_FEAT_INCCALL_AUDSTR_SRCTG_BIT      ((UINT64)1 << 1)

/* Outgoing Call during Audio Streaming (HF/SNK/CT) */
#define MPMD_FEAT_OUTCALL_AUDSTR_HFSNK_BIT      ((UINT64)1 << 2)

/* Outgoing Call during Audio Streaming (AG/SRC/TG) */
#define MPMD_FEAT_OUTCALL_AUDSTR_SRCTG_BIT      ((UINT64)1 << 3)

/* Reject/Ignore Incoming Call during Audio Streaming (HF/SNK/CT) */
#define MPMD_FEAT_REJCALL_AUDSTR_HFSNK_BIT      ((UINT64)1 << 4)

/* Reject/Ignore Incoming Call during Audio Streaming (AG/SRC/TG) */
#define MPMD_FEAT_REJCALL_AUDSTR_SRCTG_BIT      ((UINT64)1 << 5)

/* HFP call termination during AVP connection (AG/SRC) */
#define MPMD_FEAT_CALLTERM_AVP_AGSRC_BIT        ((UINT64)1 << 6)

/* HFP call termination during AVP connection (HF/SNK) */
#define MPMD_FEAT_CALLTERM_AVP_HFSNK_BIT        ((UINT64)1 << 7)


/* Byte 1 */
/* HFP call termination during AVP connection (SRC/TG) */
#define MPMD_FEAT_CALLTERM_AVP_SRCTG_BIT        ((UINT64)1 << 8)

/* Press Play on Audio Player during active call (HF/SNK/CT) */
#define MPMD_FEAT_PLAY_DUR_CALL_HFCT_BIT        ((UINT64)1 << 9)

/* Press Play on Audio Player during active call (AG/SRC/TG) */
#define MPMD_FEAT_PLAY_DUR_CALL_AGTG_BIT        ((UINT64)1 << 10)

/* Start Audio Streaming after AVRCP Play Command (CT/SNK) */
#define MPMD_FEAT_AUDSTR_AVPLAY_CTSNK_BIT       ((UINT64)1 << 11)

/* Start Audio Streaming after AVRCP Play Command (TG/SRC) */
#define MPMD_FEAT_AUDSTR_AVPLAY_TGSRC_BIT       ((UINT64)1 << 12)

/* Suspend Audio Streaming after AVRCP Pause/Stop (CT/SNK) */
#define MPMD_FEAT_SUSSTR_AVPAUSE_CTSNK_BIT      ((UINT64)1 << 13)

/* Suspend Audio Streaming after AVRCP Pause/Stop (TG/SRC) */
#define MPMD_FEAT_SUSSTR_AVPAUSE_TGSRC_BIT      ((UINT64)1 << 14)

/* Start Audio Streaming during Data communication under PSDM (DUN) (TG/GW) */
#define MPMD_FEAT_STRTSTRM_DUN_TGGW_BIT         ((UINT64)1 << 15)


/* Byte 2 */
/* Start Audio Streaming during Data communication under PSDM (DUN) (CT/DT) */
#define MPMD_FEAT_STRTSTRM_DUN_CTDT_BIT         ((UINT64)1 << 16)

/* Start packet data communication PSDM (DUN) during Audio Streaming (GW/SRC) */
#define MPMD_FEAT_DUN_AUDSTRM_GWSRC_BIT         ((UINT64)1 << 17)

/* Start packet data communication PSDM (DUN) during Audio Streaming (DT/SNK) */
#define MPMD_FEAT_DUN_AUDSTRM_DTSNK_BIT         ((UINT64)1 << 18)


/* ( Bits 19-63 are RFU ) */
/****************************************************************
** End of MPSD bits defined in D0.9 R09 version of spec (Oct 2012)
*****************************************************************/

/***************************************************
**   Supported Features bit mask for PPD scenarios
**      (16-bits)
****************************************************/

/* Byte 0 */
/* L2CAP Streaming Optimization: Omitted from final 1.0 specification?? Errata 6092 **
#define PPD_FEAT_L2CAP_OPT_BIT              ((UINT16)1 << 0)
*/

/* Sniff Mode Handling */
#define PPD_FEAT_SNIFF_HANDLING_BIT         ((UINT16)1 << 1)

/* Media Stream Suspension */
#define PPD_FEAT_MEDSTR_SUSP_BIT            ((UINT16)1 << 2)

/* (Dis-)Connection Order / Behavior  */
#define PPD_FEAT_DISC_ORDER_BIT             ((UINT16)1 << 3)

/* ( Bits 4-15 are RFU ) */
/****************************************************************
** End of PPD bits defined in D1.0 version of spec (Mar 2013)
*****************************************************************/

#endif /* BTA_MPS_API_H */
