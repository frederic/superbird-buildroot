/*****************************************************************************
**
**  Name:           bta_ma_util.h
**
**  Description:    This is the interface file for the Message Access Profile
**                  (MAP) utility functions.
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_MA_UTIL_H
#define BTA_MA_UTIL_H

#include "bta_ma_def.h"
#include "bta_mse_api.h"
#include "bta_mse_co.h"
#include "bta_ma_api.h"


#define BTA_MA_MAX_SIZE     (100)

/* Here are a set of property flags used to keep track
** of properties that are successfully parsed.  We use
** this information to determine if all the *required*
** properties have been provided in the parsed object.
*/
#define BTA_MA_PROP_VERSION     0x00000001
#define BTA_MA_BMSG_BODY_TAG_CTL_LENGTH  22 /* see MAP Spec. Errata 3603 */
                                            /* BEGIN:MSG<CRTL>+<CRTL>+END:MSG<CRTL> */ 
#ifdef __cplusplus
extern "C"
{
#endif

    const char * bta_ma_evt_typ_to_string(tBTA_MSE_NOTIF_TYPE notif_type);
    const char * bta_ma_msg_typ_to_string(tBTA_MA_MSG_TYPE msg_typ);
    const char * bta_ma_rcv_status_to_string(tBTA_MSE_CO_RCV_STATUS rcv_status);

    BOOLEAN bta_ma_stream_str(tBTA_MA_STREAM * p_stream,
                                     const char * p_str);

    BOOLEAN bta_ma_stream_buf(tBTA_MA_STREAM * p_stream,
                                     UINT16 len,
                                     UINT8 * p_buf);

    BOOLEAN bta_ma_stream_boolean_yes_no(tBTA_MA_STREAM * p_stream, BOOLEAN val);

    BOOLEAN bta_ma_stream_value(tBTA_MA_STREAM * p_stream, UINT32 val);

    BOOLEAN bta_ma_stream_handle(tBTA_MA_STREAM * p_stream, 
                                        tBTA_MA_MSG_HANDLE handle);

    UINT16  bta_ma_stream_used_size(tBTA_MA_STREAM * p_stream);

    BOOLEAN bta_ma_stream_ok(tBTA_MA_STREAM * p_stream);

    void bta_ma_convert_hex_str_to_64bit_handle(char *p_hex_str, tBTA_MA_MSG_HANDLE handle);

    BOOLEAN bta_ma_get_char(tBTA_MA_STREAM * p_stream, char * p_char);
    BOOLEAN bta_ma_str_to_charset(char * psz, tBTA_MA_CHARSET * p_charset);
    BOOLEAN bta_ma_str_to_encoding(char * psz, tBTA_MA_BMSG_ENCODING * p_encoding);
    BOOLEAN bta_ma_str_to_language(char * psz, tBTA_MA_BMSG_LANGUAGE * p_language);
    BOOLEAN bta_ma_str_to_msg_typ(char * psz, tBTA_MA_MSG_TYPE * p_msg_type);

    void bta_ma_stream_vcards(tBTA_MA_STREAM *, tBTA_MA_BMSG_VCARD *);
    void bta_ma_stream_envelopes(tBTA_MA_STREAM * p_stream, tBTA_MA_BMSG_ENVELOPE * p_envelope);
    void bta_ma_stream_body(tBTA_MA_STREAM * p_stream, tBTA_MA_BMSG_BODY * p_body);
    void bta_ma_stream_body_content(tBTA_MA_STREAM * p_stream, tBTA_MA_BMSG_CONTENT * p_content);
    void bta_ma_stream_vcard_prop(tBTA_MA_STREAM * p_stream, tBTA_MA_BMSG_VCARD * p_vcard, tBTA_MA_VCARD_PROP prop);

    extern UINT32 bta_ma_get_body_length(tBTA_MA_BMSG_BODY * p_body);

    UINT16 bta_ma_get_tag(tBTA_MA_STREAM * p_stream, char * psz, UINT16 max_size);
    UINT16 bta_ma_get_value(tBTA_MA_STREAM * p_stream, char * psz, UINT16 max_size);

    extern tBTA_MA_STATUS bta_ma_parse_vcard(tBTA_MA_BMSG_VCARD * p_vcard, tBTA_MA_STREAM * p_stream);
    extern tBTA_MA_STATUS bta_ma_parse_envelope(tBTA_MA_BMSG_ENVELOPE * p_envelope, tBTA_MA_STREAM * p_stream);

    void * bta_ma_bmsg_alloc(size_t cb);
    void bta_ma_bmsg_free(void * p);
    void bta_ma_bmsg_free_vcards(tBTA_MA_BMSG_VCARD * p_vcard);
    void bta_ma_bmsg_free_envelope(tBTA_MA_BMSG_ENVELOPE * p_envelope);
    void bta_ma_bmsg_free_body(tBTA_MA_BMSG_BODY * p_body);
    void bta_ma_bmsg_free_content(tBTA_MA_BMSG_CONTENT * p_content);
    void bta_ma_bmsg_free_message_text(tBTA_MA_BMSG_MESSAGE * p_message);
    void bta_ma_bmsg_free_vcard_prop(tBTA_MA_VCARD_PROPERTY * p_prop);

#ifdef __cplusplus
}
#endif

#endif /* BTA_MA_UTIL_H */
