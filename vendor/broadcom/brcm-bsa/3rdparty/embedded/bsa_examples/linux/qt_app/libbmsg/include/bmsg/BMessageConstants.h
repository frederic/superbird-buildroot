/*******************************************************************************
 *
 *  Copyright (C) 2013 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *******************************************************************************/
#ifndef BMSGBODYCONTENT_H
#define BMSGBODYCONTENT_H

#include "libbmsg_global.h"
#include "data_types.h"

/***
 * Class containing constants used in BMessage
 *
 *
 *
 */
class BMSGSHARED_EXPORT BMessageConstants {

public:

    BMessageConstants() {}

    ~BMessageConstants(){}

    /***
     * BMessage constant for invalid value
     */
    static const  byte INVALID_VALUE = -1;

    // ------------Message Types------------------------
    /***
     * BMessage Message type for email
     */

    static const byte BTA_MSG_TYPE_EMAIL = 1;

    /***
     * BMessage Message type for SMS GSM
     */
    static const  byte BTA_MSG_TYPE_SMS_GSM = 2;
    /***
     * BMessage Message type for SMS CDMA
     */
    static const  byte BTA_MSG_TYPE_SMS_CDMA = 4;
    /***
     * BMessage Message type for MMS
     */
    static const  byte BTA_MSG_TYPE_MMS = 8;

    // ------------Message Body Encodings------------------------
    /***
     * BMessage body encoding for 8-bit characters
     */
    static const  byte BTA_BMSG_ENC_8BIT = 0;
    /***
     * BMessage body encoding constant for GSM 7-bit Default Alphabet characters
     */
    static const  byte BTA_BMSG_ENC_G7BIT = 1;
    /***
     * BMessage body encoding constant for GSM
     * 7 bit Alphabet with national language extension characters
     */
    static const   byte BTA_BMSG_ENC_G7BITEXT = 2;
    /***
     * BMessage body encoding constant for GUCS2 characters
     */
    static const   byte BTA_BMSG_ENC_GUCS2 = 3;
    /***
     * BMessage body encoding constant for G8-bit characters
     */
    static const   byte BTA_BMSG_ENC_G8BIT = 4;
    /***
     * BMessage body encoding constant for C8-bit characters
     */
    static const   byte BTA_BMSG_ENC_C8BIT = 5;
    /***
     * BMessage body encoding constant for Extended Protocol
     * Message characters
     */
    static const   byte BTA_BMSG_ENC_CEPM = 6;
    /***
     * BMessage body encoding constant for 7-bit ASCII characters
     */
    static const   byte BTA_BMSG_ENC_C7ASCII = 7;
    /***
     * BMessage body encoding constant for IA5 characters
     */
    static const   byte BTA_BMSG_ENC_CIA5 = 8;
    /***
     * BMessage body encoding constant for UNICODE characters
     */
    static const   byte BTA_BMSG_ENC_CUNICODE = 9;
    /***
     * BMessage body encoding constant for Shift-JIS characters
     */
    static const   byte BTA_BMSG_ENC_CSJIS = 10;
    /***
     * BMessage body encoding constant for KOREAN characters
     */
    static const   byte BTA_BMSG_ENC_CKOREAN = 11;
    /***
     * BMessage body encoding constant for LATIN and HEBREW characters
     */
    static const   byte BTA_BMSG_ENC_CLATINHEB = 12;
    /***
     * BMessage body encoding constant for LATIN characters
     */
    static const   byte BTA_BMSG_ENC_CLATIN = 13;
    /***
     * BMessage body encoding constant for unknown characters
     */
    static const   byte BTA_BMSG_ENC_UNKNOWN = 14;

    // --------------------Languages--------------------------------------
    /***
     * BMessage language constant for unspecified language
     */
    static const   byte BTA_BMSG_LANG_UNSPECIFIED = 0;
    /***
     * BMessage language constant for unknown language
     */
    static const   byte BTA_BMSG_LANG_UNKNOWN = 1;
    /***
     * BMessage language constant for Spanish language
     */
    static const   byte BTA_BMSG_LANG_SPANISH = 2;
    /***
     * BMessage language constant for Turkish language
     */
    static const   byte BTA_BMSG_LANG_TURKISH = 3;
    /***
     * BMessage language constant for Portuguese language
     */
    static const   byte BTA_BMSG_LANG_PORTUGUESE = 4;
    /***
     * BMessage language constant for English language
     */
    static const   byte BTA_BMSG_LANG_ENGLISH = 5;
    /***
     * BMessage language constant for French language
     */
    static const   byte BTA_BMSG_LANG_FRENCH = 6;
    /***
     * BMessage language constant for Japanese language
     */
    static const   byte BTA_BMSG_LANG_JAPANESE = 7;
    /***
     * BMessage language constant for Korean language
     */
    static const   byte BTA_BMSG_LANG_KOREAN = 8;
    /***
     * BMessage language constant for Chinese language
     */
    static const   byte BTA_BMSG_LANG_CHINESE = 9;
    /***
     * BMessage language constant for Hebrew language
     */
    static const   byte BTA_BMSG_LANG_HEBREW = 10;

    // ------------Charsets-----------------------
    /***
     * BMessage constant for native characterset
     */
    static const   byte BTA_CHARSET_NATIVE = 0;
    /***
     * BMessage constant for UTF-8 characterset
     */
    static const   byte BTA_CHARSET_UTF_8 = 1;
    /***
     * BMessage constant for unknown characterset
     */
    static const   byte BTA_CHARSET_UNKNOWN = 2;

    // --------------VCard versions--------------------
    /***
     * BMessage constant for vcard version 2.1
     */
    static const   byte BTA_VCARD_VERSION_21 = 0;
    /***
     * BMessage constant for vcard version 3.0
     */
    static const   byte BTA_VCARD_VERSION_30 = 1;

    // -------------VCard properties
    /***
     * BMessage constant for vcard property - Name
     */
    static const   byte BTA_VCARD_PROP_N = 0;
    /***
     * BMessage constant for vcard property - Full Name
     */
    static const   byte BTA_VCARD_PROP_FN = 1;
    /***
     * BMessage constant for vcard property - Telephone number
     */
    static const   byte BTA_VCARD_PROP_TEL = 2;
    /***
     * BMessage constant for vcard property - Email
     */
    static const   byte BTA_VCARD_PROP_EMAIL = 3;

};

#endif
