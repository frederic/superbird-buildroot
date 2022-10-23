/********************************************************************************/
/*                                                                              */
/*    Name:   voice_hdlr_int.h                                                  */
/*                                                                              */
/*    Description: This file contains definitions and data structures that      */
/*                 are used to handle all the voice functions.                  */
/*                                                                              */
/*  Copyright (c) 2000-2001, WIDCOMM Inc., All Rights Reserved.                 */
/*  Widcomm Bluetooth Application. Proprietary and confidential.                */
/********************************************************************************/

#ifndef VOICE_HDLR_INT_INCLUDE_H
#define VOICE_HDLR_INT_INCLUDE_H

#define SIGNAL_LOW                     0x2000
#define SIGNAL_HIGH                    0x1fff
#define SIGNAL_QUIET                   0x0000

#define VOICE_INDEX_NOT_FIND           -1
#define VOICE_MAX_QUEUE_TONES          1
#define VOICE_AUDIBLE_MAX_TONES        6

#define VOICE_OOR_MAX_TONES            4
#define VOICE_MUTE_MAX_TONES           4
#define VOICE_RING_MAX_TONES           13
#define VOICE_BATLOW_MAX_TONES         3 
#define VOICE_TEST_MAX_TONES           4

#define VOICE_50MS_DURATION            50
#define VOICE_100MS_DURATION           100
#define VOICE_250MS_DURATION           250
#define VOICE_2000MS_DURATION          2000
#define VOICE_TEST_DURATION            1000

#define VOICE_CONVERT_TO_1MSEC         1000
#define VOICE_OOR_TIME_INTERVAL        500
#define VOICE_RING_TIME_INTERVAL       1000
#define VOICE_MUTE_TIME_INTERVAL       5000
#define VOICE_BATLOW_TIME_INTERVAL     5000

#define VOICE_CONVERT_TO_MICROSEC      1000
#define VOICE_TIME_PER_SAMPLE          125

#define VOICE_ODD_REMAINDER            1
#define VOICE_EVEN_REMAINDER           0
#define VOICE_RING_QUIET_TONE          6       /* middle entry of index start 0..13 */

typedef struct
{
    UINT8  samples_per_array;
    UINT16 cur_array;
    UINT16 cur_sample;
    UINT16 total_arrays;
    UINT16 *value_ptr;
} audible_tone_type;

typedef struct
{
    UINT8   cur_tone;
    UINT8   total_tones;
    BOOLEAN in_use;
    UINT16  repeat_interval;
} audible_info_type;

typedef struct
{
    BOOLEAN           in_use;
    audible_tone_type tone_buf;
} audible_standard_type;

#ifdef PRODUCTION_RELEASE
typedef struct
{
    audible_info_type  mute_info;
    audible_tone_type  mute_tone[VOICE_MUTE_MAX_TONES];
} audible_mute_type;

typedef struct
{
    audible_info_type  batlow_info;
    audible_tone_type  batlow_tone[VOICE_BATLOW_MAX_TONES];
} audible_batlow_type;

typedef struct
{
    audible_info_type  oor_info;
    audible_tone_type  oor_tone[VOICE_OOR_MAX_TONES];
} audible_oor_type; 

#endif

typedef struct
{
    audible_info_type  ring_info;
    audible_tone_type  ring_tone[VOICE_RING_MAX_TONES];
} audible_ring_type; 

typedef struct
{
    audible_info_type  test_info;
    audible_tone_type  test_tone[VOICE_TEST_MAX_TONES];
} audible_test_type; 

typedef struct 
{
#ifdef PRODUCTION_RELEASE
    audible_oor_type      oor_buf;
    audible_mute_type     mute_buf;
    audible_batlow_type   batlow_buf;
#endif
    audible_standard_type fail_buf;
    audible_ring_type     ring_buf;
    audible_standard_type success_buf;
	audible_test_type     test_buf;
} audible_cb_type;

typedef struct
{
    UINT8                  enable_isr;
    UINT8                  num_active_tones;
    BOOLEAN                tone_in_progress;
    audible_tone_enum_type out_tone_name;
} audible_output_type;


#endif /*VOICE_HDLR_INT_INCLUDE_H*/
