/********************************************************************************/
/*                                                                              */
/*    Name:   voice_hdlr.h                                                      */
/*                                                                              */
/*    Description: This file contains definitions and data structures that      */
/*                 are used to handle all the voice functions.                  */
/*                                                                              */
/*                                                                              */
/*  Copyright (c) 2000-2001, WIDCOMM Inc., All Rights Reserved.                 */
/*  Widcomm Bluetooth Application. Proprietary and confidential.                */
/********************************************************************************/
#ifndef VOICE_HDLR_INCLUDE_H
#define VOICE_HDLR_INCLUDE_H

/* Define all audio signals that are used to output to the user. */
typedef enum
{
    VOICE_FAIL_TONE,
    VOICE_SUCCESS_TONE,
    VOICE_CLICK_TONE,
    VOICE_MUTE_TONE,
    VOICE_BATLOW_TONE,
    VOICE_OOR_TONE,
    VOICE_RING_TONE,
    VOICE_TEST_TONE
} audible_tone_enum_type;

extern void    voice_hdlr_init (void);
extern UINT16  voice_hdlr_rw_data (UINT16 sco_data);

#endif /* VOICE_HDLR_INCLUDE_H*/

