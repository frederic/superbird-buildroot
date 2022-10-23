/*****************************************************************************
 **
 **  Name:           app_av.c
 **
 **  Description:    Bluetooth Audio/Video Streaming application
 **
 **  Copyright (c) 2009-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
/* for memcpy etc... */
#include <string.h>
/* for open and open parameters */
#include <fcntl.h>
/* for lseek, read and close */
#include <unistd.h>
/* for EINTR */
#include <errno.h>

#include "bsa_api.h"

#include "gki_int.h"
#include "uipc.h"

#include "bsa_trace.h"
#include "app_av.h"
#ifdef APP_AV_BCST_INCLUDED
#include "app_av_bcst.h"
#endif
#include "app_xml_param.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_wav.h"
#include "app_playlist.h"
#include "app_mutex.h"
#include "app_thread.h"
#include "app_xml_utils.h"
#include "app_dm.h"

#ifdef PCM_ALSA
#include "app_alsa.h"
#endif

/*
 * Defines
 */

#ifndef APP_AV_FEAT_MASK
/* Default: Target (receive command) and Controller (send command) enabled and Copy Protection Enabled*/
#define APP_AV_FEAT_MASK (BSA_AV_FEAT_RCTG     | BSA_AV_FEAT_RCCT   | BSA_AV_FEAT_VENDOR | \
                          BSA_AV_FEAT_METADATA | BSA_AV_FEAT_BROWSE | BSA_AV_FEAT_PROTECT)
#endif

#ifndef APP_AV_USE_RC
#define APP_AV_USE_RC TRUE
#endif

#ifndef BSA_AV_DUMP_TX_DATA
#define BSA_AV_DUMP_TX_DATA  FALSE
#endif

/* Number of simultaneous A2DP connections supported */
#define APP_AV_MAX_CONNECTIONS 2
/* Size of the audio buffer */
#define APP_AV_MAX_AUDIO_BUF 256
/* Name of the XML file containing the saved remote devices */
#define APP_AV_REMOTE_DEVICES_XML_FILE "./bt_devices.xml"
/* Name of the default sound file */
#define APP_AV_SOUND_FILE "./test.wav"

/* Min and Max UIPC parameters (Period and Length) */
#define APP_AV_PERIOD_MIN      (BSA_AV_MIN_SYNCHRONOUS_LATENCY*1000) /* convert ms to us */
#define APP_AV_PERIOD_MAX      (BSA_AV_MAX_SYNCHRONOUS_LATENCY*1000) /* convert ms to us */

#define APP_AV_LENGTH_MIN       100     /* Length must >= 100 bytes */
#define APP_AV_LENGTH_MAX      (8*1024) /* Length must <= 8 Kbytes */

/* Size of the audio buffer use to store the PCM to send to BSA */
#define APP_AV_MAX_AUDIO_BUF_MAX 10000

/* High and low water mark for timer compensation scheme */
#ifndef APP_AV_COMPENSATION_HIGH_WM
#define APP_AV_COMPENSATION_HIGH_WM 1.2 /* 120 % of the period */
#endif

#ifndef APP_AV_COMPENSATION_LOW_WM
#define APP_AV_COMPENSATION_LOW_WM 0.8 /* 80 % of the period*/
#endif

const short sinwaves[2][64] = {{
         0,    488,    957,   1389,   1768,  2079,  2310,  2452,
      2500,   2452,   2310,   2079,   1768,  1389,   957,   488,
         0,   -488,   -957,  -1389,  -1768, -2079, -2310, -2452,
     -2500,  -2452,  -2310,  -2079,  -1768, -1389,  -957,  -488,
         0,    488,    957,   1389,   1768,  2079,  2310,  2452,
      2500,   2452,   2310,   2079,   1768,  1389,   957,   488,
         0,   -488,   -957,  -1389,  -1768, -2079, -2310, -2452,
     -2500,  -2452,  -2310,  -2079,  -1768, -1389,  -957,  -488},{

         0,    244,    488,    722,    957,  1173,  1389,   1578,
      1768,   1923,   2079,   2194,   2310,  2381,  2452,   2476,
      2500,   2476,   2452,   2381,   2310,  2194,  2079,   1923,
      1768,   1578,   1389,   1173,    957,   722,   488,    244,
         0,   -244,   -488,   -722,   -957, -1173, -1389,  -1578,
     -1768,  -1923,  -2079,  -2194,  -2310, -2381, -2452,  -2476,
     -2500,  -2476,  -2452,  -2381,  -2310, -2194, -2079,  -1923,
     -1768,  -1578,  -1389,  -1173,   -957,  -722,  -488,   -244 }};

/* Capabilities of the local apt-X codec */
const tBSA_AV_CODEC_INFO aptx_caps =
{
    BSA_AV_CODEC_APTX, /* BSA codec ID, if 0, means that codec is not supported */
    /* Codec capabilities information
     * byte 0 : LOSC (length of codec info not including LOSC: 0x09)
     * byte 1 : media type (0x00 = audio)
     * byte 2 : media codec type (0xFF = non A2DP standard)
     * byte 3 to 6 : Vendor ID in little endian (0x0000004F = APT Licensing ltd.
     * byte 7 to 8 : Vendor specific codec ID (0x0001 = apt-X)
     * byte 9 :
     *   - bit 5 : 44.1kHz sampling frequency supported
     *   - bit 4 : 48kHz sampling frequency supported
     *   - bit 1 : stereo mode supported
     */
    "\x09\x00\xFF\x4F\x00\x00\x00\x01\x00\x32"
};

/* Capabilities of the local SEC codec */
const tBSA_AV_CODEC_INFO sec_caps =
{
    BSA_AV_CODEC_SEC, /* BSA codec ID, if 0, means that codec is not supported */
    /* Codec capabilities information
     * byte 0 : LOSC (length of codec info not including LOSC: 0x09)
     * byte 1 : media type (0x00 = audio)
     * byte 2 : media codec type (0xFF = non A2DP standard)
     * byte 3 to 6 : Vendor ID in little endian (0x00000075 = Samsung BT ID)
     * byte 7 to 8 : Vendor specific codec ID (0x0001 = SEC)
     * byte 9 :
        *   - bit 6 : 32kHz sampling frequency supported
        *   - bit 5 : 44.1kHz sampling frequency supported
        *   - bit 4 : 48kHz sampling frequency supported
        *   - bit 3 : Mono
        *   - bit 1 : Stereo
     */
    "\x09\x00\xFF\x75\x00\x00\x00\x01\x00\x7A"
};

/* Play states (we use define instead of enum to allow Makefile to use them) */
#define APP_AV_PLAYTYPE_TONE    0   /* Play tone */
#define APP_AV_PLAYTYPE_FILE    1   /* Play file */
#define APP_AV_PLAYTYPE_MIC     2   /* Play Microphone */
#define APP_AV_PLAYTYPE_TEST    3   /* Play test data */
#define APP_AV_PLAYTYPE_AVK     4

/* Sample media players for peer browsing */
#define BSA_AV_PLAYER_ID_INVALID           0
#define BSA_AV_PLAYER_ID_FILES             1
#define BSA_AV_PLAYER_ID_MPLAYER           2
#define BSA_AV_PLAYER_ID_FM                3

#define BSA_AV_NUM_MPLAYERS                3

tBSA_ITEM_PLAYER bsa_av_player1_file =
{
    /* A unique identifier for this media player.*/
    BSA_AV_PLAYER_ID_FILES,
    /* Use AVRC_MJ_TYPE_AUDIO, AVRC_MJ_TYPE_VIDEO, AVRC_MJ_TYPE_BC_AUDIO, or AVRC_MJ_TYPE_BC_VIDEO.*/
    (AVRC_MJ_TYPE_AUDIO|AVRC_MJ_TYPE_VIDEO),
    /* Use AVRC_SUB_TYPE_NONE, AVRC_SUB_TYPE_AUDIO_BOOK, or AVRC_SUB_TYPE_PODCAST*/
    AVRC_SUB_TYPE_NONE,
    /* Use AVRC_PLAYSTATE_STOPPED, AVRC_PLAYSTATE_PLAYING, AVRC_PLAYSTATE_PAUSED, AVRC_PLAYSTATE_FWD_SEEK,
           AVRC_PLAYSTATE_REV_SEEK, or AVRC_PLAYSTATE_ERROR*/
    AVRC_PLAYSTATE_STOPPED,
    {   /* Supported feature bit mask*/
        0x1F, /* octet 0 bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                                   4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP */
        0x02, /* octet 1 bit mask: 0=LEFT_DOWN, 1=ROOT_MENU, 2=SETUP_MENU, 3=CONT_MENU,
                                   4=FAV_MENU, 5=EXIT, 6=0, 7=1 */
        0x00, /* octet 2 bit mask: 0=2, 1=3, 2=4, 3=5,
                                   4=6, 5=7, 6=8, 7=9 */
        0x18, /* octet 3 bit mask: 0=DOT, 1=ENTER, 2=CLEAR, 3=CHAN_UP,
                                   4=CHAN_DOWN, 5=PREV_CHAN, 6=SOUND_SEL, 7=INPUT_SEL */
        0x00, /* octet 4 bit mask: 0=DISP_INFO, 1=HELP, 2=PAGE_UP, 3=PAGE_DOWN,
                                   4=POWER, 5=VOL_UP, 6=VOL_DOWN, 7=MUTE */
        0x87, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x01, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
        0x6C, /* octet 7 bit mask: 0=PassThru Vendor, 1=GroupNavi, 2=AdvCtrl, 3=Browsing,
                                   4=Searching, 5=AddToNowPlaying, 6=UID_unique, 7=BrowsableWhenAddressed */
        0x06, /* octet 8 bit mask: 0=SearchableWhenAddressed, 1=NowPlaying, 2=UID_Persistency */
        0x00, /* octet 9 bit mask: not used */
        0x00, /* octet 10 bit mask: not used */
        0x00, /* octet 11 bit mask: not used */
        0x00, /* octet 12 bit mask: not used */
        0x00, /* octet 13 bit mask: not used */
        0x00, /* octet 14 bit mask: not used */
        0x00  /* octet 15 bit mask: not used */
    },
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        12,
        (UINT8 *)"File Player"
    }
};

tBSA_ITEM_PLAYER bsa_av_player2_media_player =
{
    /* A unique identifier for this media player.*/
    BSA_AV_PLAYER_ID_MPLAYER,
    /* Use AVRC_MJ_TYPE_AUDIO, AVRC_MJ_TYPE_VIDEO, AVRC_MJ_TYPE_BC_AUDIO, or AVRC_MJ_TYPE_BC_VIDEO.*/
    AVRC_MJ_TYPE_AUDIO,
    /* Use AVRC_SUB_TYPE_NONE, AVRC_SUB_TYPE_AUDIO_BOOK, or AVRC_SUB_TYPE_PODCAST*/
    AVRC_SUB_TYPE_NONE,
    /* Use AVRC_PLAYSTATE_STOPPED, AVRC_PLAYSTATE_PLAYING, AVRC_PLAYSTATE_PAUSED, AVRC_PLAYSTATE_FWD_SEEK,
       AVRC_PLAYSTATE_REV_SEEK, or AVRC_PLAYSTATE_ERROR*/
    AVRC_PLAYSTATE_STOPPED,
    {   /* Supported feature bit mask*/
        0x1F, /* octet 0 bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                                   4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP */
        0x02, /* octet 1 bit mask: 0=LEFT_DOWN, 1=ROOT_MENU, 2=SETUP_MENU, 3=CONT_MENU,
                                   4=FAV_MENU, 5=EXIT, 6=0, 7=1 */
        0x00, /* octet 2 bit mask: 0=2, 1=3, 2=4, 3=5,
                                   4=6, 5=7, 6=8, 7=9 */
        0x18, /* octet 3 bit mask: 0=DOT, 1=ENTER, 2=CLEAR, 3=CHAN_UP,
                                   4=CHAN_DOWN, 5=PREV_CHAN, 6=SOUND_SEL, 7=INPUT_SEL */
        0x00, /* octet 4 bit mask: 0=DISP_INFO, 1=HELP, 2=PAGE_UP, 3=PAGE_DOWN,
                                   4=POWER, 5=VOL_UP, 6=VOL_DOWN, 7=MUTE */
        0x87, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x01, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
        0x00, /* octet 7 bit mask: 0=PassThru Vendor, 1=GroupNavi, 2=AdvCtrl, 3=Browsing,
                                   4=Searching, 5=AddToNowPlaying, 6=UID_unique, 7=BrowsableWhenAddressed */
        0x00, /* octet 8 bit mask: 0=SearchableWhenAddressed, 1=NowPlaying, 2=UID_Persistency */
        0x00, /* octet 9 bit mask: not used */
        0x00, /* octet 10 bit mask: not used */
        0x00, /* octet 11 bit mask: not used */
        0x00, /* octet 12 bit mask: not used */
        0x00, /* octet 13 bit mask: not used */
        0x00, /* octet 14 bit mask: not used */
        0x00  /* octet 15 bit mask: not used */
    },
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        12,
        (UINT8 *)"Media Player"
    }
};

tBSA_ITEM_PLAYER bsa_av_player3_fm =
{
    /* A unique identifier for this media player.*/
    BSA_AV_PLAYER_ID_FM,
    /* Use AVRC_MJ_TYPE_AUDIO, AVRC_MJ_TYPE_VIDEO, AVRC_MJ_TYPE_BC_AUDIO, or AVRC_MJ_TYPE_BC_VIDEO.*/
    (AVRC_MJ_TYPE_BC_AUDIO),
    /* Use AVRC_SUB_TYPE_NONE, AVRC_SUB_TYPE_AUDIO_BOOK, or AVRC_SUB_TYPE_PODCAST*/
    AVRC_SUB_TYPE_NONE,
    /* Use AVRC_PLAYSTATE_STOPPED, AVRC_PLAYSTATE_PLAYING, AVRC_PLAYSTATE_PAUSED, AVRC_PLAYSTATE_FWD_SEEK,
           AVRC_PLAYSTATE_REV_SEEK, or AVRC_PLAYSTATE_ERROR*/
    AVRC_PLAYSTATE_STOPPED,
    {   /* Supported feature bit mask*/
        0x1F, /* octet 0 bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                                   4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP */
        0x02, /* octet 1 bit mask: 0=LEFT_DOWN, 1=ROOT_MENU, 2=SETUP_MENU, 3=CONT_MENU,
                                   4=FAV_MENU, 5=EXIT, 6=0, 7=1 */
        0x00, /* octet 2 bit mask: 0=2, 1=3, 2=4, 3=5,
                                   4=6, 5=7, 6=8, 7=9 */
        0x18, /* octet 3 bit mask: 0=DOT, 1=ENTER, 2=CLEAR, 3=CHAN_UP,
                                   4=CHAN_DOWN, 5=PREV_CHAN, 6=SOUND_SEL, 7=INPUT_SEL */
        0x00, /* octet 4 bit mask: 0=DISP_INFO, 1=HELP, 2=PAGE_UP, 3=PAGE_DOWN,
                                   4=POWER, 5=VOL_UP, 6=VOL_DOWN, 7=MUTE */
        0x87, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x01, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
        0x00, /* octet 7 bit mask: 0=PassThru Vendor, 1=GroupNavi, 2=AdvCtrl, 3=Browsing,
                                   4=Searching, 5=AddToNowPlaying, 6=UID_unique, 7=BrowsableWhenAddressed */
        0x00, /* octet 8 bit mask: 0=SearchableWhenAddressed, 1=NowPlaying, 2=UID_Persistency */
        0x00, /* octet 9 bit mask: not used */
        0x00, /* octet 10 bit mask: not used */
        0x00, /* octet 11 bit mask: not used */
        0x00, /* octet 12 bit mask: not used */
        0x00, /* octet 13 bit mask: not used */
        0x00, /* octet 14 bit mask: not used */
        0x00  /* octet 15 bit mask: not used */
    },
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        8,
        (UINT8 *)"FM Radio"
    }
};

/* do not use const - the player state may be changed by platform code */
tBSA_ITEM_PLAYER * bsa_av_players [] =
{
    /* player1 - when using files (wave, SBC, mp3) in btui_app */
    &bsa_av_player1_file,
    /* player2 - when using windows media player */
    &bsa_av_player2_media_player,
    /* player3 - when using FM Radio */
    &bsa_av_player3_fm
};

typedef struct
{
    tBSA_UID           uid;            /* The uid of this media element item */
    UINT8               type;           /* Use AVRC_MEDIA_TYPE_AUDIO or AVRC_MEDIA_TYPE_VIDEO. */
    tBSA_FULL_NAME     name;           /* The media name, name length and character set id. */
    UINT8               attr_count;     /* The number of attributes in p_attr_list */
    tBSA_ATTR_ENTRY    p_attr_list[7];    /* Attribute entry list. */
} tBSA_APP_ITEM_MEDIA;

/* App defined data */

tBSA_APP_ITEM_MEDIA song1 =
{
    . uid = { 0x01 },
    .type = AVRC_MEDIA_TYPE_AUDIO,
    .name =
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        8,
        (UINT8 *)"Beat It ",
    },
    .attr_count = 7,

    .p_attr_list[0] =
    {
        AVRC_MEDIA_ATTR_ID_TITLE,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            8,
            (UINT8 *)"Beat It ",
        },
    },

    .p_attr_list[1] =
    {
        AVRC_MEDIA_ATTR_ID_ARTIST,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            16,
            (UINT8 *)"Michael Jackson "
        },
    },

    .p_attr_list[2] =
    {
        AVRC_MEDIA_ATTR_ID_ALBUM,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            8,
            (UINT8 *)"Thriller"
        },
    },

    .p_attr_list[3] =
    {
        AVRC_MEDIA_ATTR_ID_GENRE,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            8,
            (UINT8 *)"SoftRock"
        },
    },

    .p_attr_list[4] =
    {
        AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"3   "
        },
    },

    .p_attr_list[5] =
    {
        AVRC_MEDIA_ATTR_ID_TRACK_NUM,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"1   "
        },
    },

    .p_attr_list[6] =
    {
        AVRC_MEDIA_ATTR_ID_PLAYING_TIME,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            8,
            (UINT8 *)"269000  "
        },
    },

};


tBSA_APP_ITEM_MEDIA song2 =
{
     .uid = { 0x02 },
    .type = AVRC_MEDIA_TYPE_AUDIO,
    .name =
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        16,
        (UINT8 *)"Happy Nation    ",
    },
    .attr_count = 7,

    .p_attr_list[0] =
    {
        AVRC_MEDIA_ATTR_ID_TITLE,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            16,
            (UINT8 *)"Happy Nation    "
        },
    },

    .p_attr_list[1] =
    {
        AVRC_MEDIA_ATTR_ID_ARTIST,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            16,
            (UINT8 *)"Jonas, Jenny    "
        },
    },


    .p_attr_list[2] =
    {
        AVRC_MEDIA_ATTR_ID_ALBUM,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            16,
            (UINT8 *)"Ace of Base     "
        },
    },

    .p_attr_list[3] =
    {
        AVRC_MEDIA_ATTR_ID_GENRE,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"Pop "
        },
    },

    .p_attr_list[4] =
    {
        AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"2   "
        },
    },

    .p_attr_list[5] =
    {
        AVRC_MEDIA_ATTR_ID_TRACK_NUM,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"3   "
        },
    },

    .p_attr_list[6] =
    {
        AVRC_MEDIA_ATTR_ID_PLAYING_TIME,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            8,
            (UINT8 *)"255000  "
        },
    },


};


tBSA_APP_ITEM_MEDIA song3 =
{
    . uid = { 0x03 },
    .type = AVRC_MEDIA_TYPE_AUDIO,
    .name =
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        16,
        (UINT8 *)"River of Dreams ",
    },
    .attr_count = 7,
    .p_attr_list[0] =
    {
        AVRC_MEDIA_ATTR_ID_TITLE,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            16,
            (UINT8 *)"River of Dreams "
        },
    },

    .p_attr_list[1] =
    {
        AVRC_MEDIA_ATTR_ID_ARTIST,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            16,
            (UINT8 *)"Billy Joel      "
        },
    },


    .p_attr_list[2] =
    {
        AVRC_MEDIA_ATTR_ID_ALBUM,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            16,
            (UINT8 *)"River of Dreams "
        },
    },

    .p_attr_list[3] =
    {
        AVRC_MEDIA_ATTR_ID_GENRE,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"Soul"
        },
    },

    .p_attr_list[4] =
    {
        AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"3    "
        },
    },

    .p_attr_list[5] =
    {
        AVRC_MEDIA_ATTR_ID_TRACK_NUM,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            4,
            (UINT8 *)"3   "
        },
    },

    .p_attr_list[6] =
    {
        AVRC_MEDIA_ATTR_ID_PLAYING_TIME,
        {   /* The player name, name length and character set id.*/
            AVRC_CHARSET_ID_UTF8,
            8,
            (UINT8 *)"211000  "
        },
    },
};

#define BSA_AV_NUMSONGS                3

tBSA_APP_ITEM_MEDIA * bsa_av_songs[] =
{
    &song1,
    &song2,
    &song3
};

typedef UINT16          tBSA_AV_EVT_MASK;
typedef struct {
    tBSA_AV_EVT_MASK    evt_mask;
    UINT8               label[AVRC_NUM_NOTIF_EVENTS];
} tBSA_AV_REG_EVT;

#define BSA_ATTR_STRING_LEN 15

typedef struct {
    UINT8   val1[BSA_ATTR_STRING_LEN];
    UINT8   val2[BSA_ATTR_STRING_LEN];
} tBSA_AV_META_ATTRIB_SETTINGS;

typedef struct {
    UINT8   attrib_id;
    UINT8   curr_value;
    BOOLEAN value_updated;
    UINT8   attrib_str[BSA_ATTR_STRING_LEN];
    tBSA_AV_META_ATTRIB_SETTINGS attr_settings[2];
} tBSA_AV_META_ATTRIB;

typedef struct {
    tBSA_AV_META_ATTRIB     equalizer;      /* Equalizer Status */
    tBSA_AV_META_ATTRIB     repeat;         /* Repeat Mode Status */
    tBSA_AV_META_ATTRIB     shuffle;        /* Shuffle Status */
} tBSA_AV_PAS_INFO;


typedef struct {
    UINT32  song_length;
    UINT32  song_pos;
    UINT8   play_status;
} tBSA_AV_META_PLAYSTAT;


typedef struct {
    UINT8   event_id;
    UINT32  playback_interval;
    UINT32  playback_pos;
    UINT8   bat_stat;
    UINT8   sys_stat;
} tBSA_AV_STAT_INFO;


typedef struct {
    tBSA_AV_REG_EVT        registered_events;
    UINT16                 charset_id;
    UINT8                  max_attrib_num;     /* app dependent value */
    tBSA_AV_PAS_INFO       pas_info;
    tBSA_AV_META_PLAYSTAT  play_status;
    tBSA_AV_STAT_INFO      notif_info;
    UINT8                  track_num;
    UINT16                 addr_player_id;
    UINT16                 browsed_player_id;
    UINT16                 cur_uid_counter;    /* always 0 -> for database unaware players */
} tBSA_AV_METADATA;


typedef struct
{
    UINT8               cur_play;
    UINT8               play_count;
    tBSA_AV_METADATA    meta_info;
} tBSA_AV_CB;

tBSA_AV_CB *p_bsa_av_cb = NULL;

/* UID is composed of FOLDER ID and FILE ID */
#define AVRCP_NOW_PLAYING_FOLDER_ID_FIRST_BYTE  0x02/* First byte of NOW_PLAYING folder UID 0x02 */
#define AVRCP_NOW_PLAYING_FOLDER_ID     0x02000000  /* NOW_PLAYING folder UID */
#define AVRCP_PLAY_LISTS_FOLDER_ID      0x00000000  /* PLAY_LISTS folder UID */
#define AVRCP_NOW_PLAYING_FILE_ID       0x00000000  /* First now playing file UID */
#define AVRCP_NO_NOW_PLAYING_FOLDER_ID  0xFFFFFFFF  /* No folder is selected */
#define AVRCP_NO_NOW_PLAYING_FILE_ID    0xFFFFFFFF  /* No file is selected */


/*
 * Global variables
 */

/* local application control block (current status) */
struct
{
    /* UIPC channel ID for the audio streaming */
    tUIPC_CH_ID stream_uipc_channel;
    /* Array of A2DP instances */
    tAPP_AV_CONNECTION connections[APP_AV_MAX_CONNECTIONS];
    /* Return value of the last start */
    tBSA_STATUS last_start_status;
    /* Stream encoding information */
    tBSA_AV_MEDIA_FEEDINGS media_feeding;
    /* Play state */
    volatile UINT8 play_state;
    /* Indicate the play type */
    UINT8 play_type;
    /* Indicate that we're playing a playlist */
    BOOLEAN play_list;
#ifdef PCM_ALSA
    /* ALSA capture is opened*/
    BOOLEAN alsa_capture_opened;
#endif
    /* Information about the sound files list */
    char **soundfile_list;
    int soundfile_list_size;

    /* Information about currently played file */
    int file_index;
    char file_name[1000];

    /* Information about the current tone generation */
    UINT8 sinus_index;
    UINT8 sinus_type;

    /* Info about the UIPC configuration */
    tAPP_AV_UIPC uipc_cfg;

    /* PCM audio buffer */
    short audio_buf[APP_AV_MAX_AUDIO_BUF_MAX];

    /* Tone generation sampling frequency */
    UINT16 tone_sample_freq;

    UINT32 sec_frame_size;

    UINT8 label;

    /* Content Protection */
    tBSA_AV_CP_ID cp_id;
    UINT8 cp_scms_flag;

    /* Current sampling frequency index set in test_sec_codec */
    int test_sec_sampfreq_index;

    tBSA_AV_CBACK *p_Callback;
    tAPP_THREAD t_app_rc_thread;
    int s_command;

    tAPP_MUTEX app_stream_tx_mutex;
    tAPP_THREAD app_uipc_tx_thread_struct;
} app_av_cb;


tBSA_AV_META_PLAYSTAT playst =
{
    255000,
    120000,
    BSA_AVRC_PLAYSTATE_STOPPED,
};



/*
 * Local functions
 */
static void app_avk_rc_command_thread(void);
static char *app_av_display_vendor_command(UINT8 command);
static int app_av_uipc_reconfig(void);
static void app_av_delay_start(tAPP_AV_DELAY *p_delay);
static int app_av_stop_current(void);
static int app_av_build_notification_response(UINT8 event_id, tBSA_AV_META_RSP_CMD *bsa_av_meta_rsp_cmd);
static void app_av_init_meta_data();

/*******************************************************************************
 **
 ** Function         app_av_display_vendor_commands
 **
 ** Description      This function display the name of vendor dependent command
 **
 ** Returns          void
 **
 *******************************************************************************/
static char *app_av_display_vendor_command(UINT8 command)
{
    switch(command)
    {
    case BSA_AV_RC_VD_GET_CAPABILITIES:
        return ("BSA_AV_RC_VD_GET_CAPABILITIES");
    case BSA_AV_RC_VD_LIST_PLAYER_APP_ATTR:
        return ("BSA_AV_RC_VD_LIST_PLAYER_APP_ATTR");
    case BSA_AV_RC_VD_LIST_PLAYER_APP_VALUES:
        return("BSA_AV_RC_VD_LIST_PLAYER_APP_VALUES");
    case BSA_AV_RC_VD_GET_CUR_PLAYER_APP_VALUE:
        return ("BSA_AV_RC_VD_GET_CUR_PLAYER_APP_VALUE");
    case BSA_AV_RC_VD_SET_PLAYER_APP_VALUE:
        return ("BSA_AV_RC_VD_SET_PLAYER_APP_VALUE");
    case BSA_AV_RC_VD_GET_PLAYER_APP_ATTR_TEXT:
        return ("BSA_AV_RC_VD_GET_PLAYER_APP_ATTR_TEXT");
    case BSA_AV_RC_VD_GET_PLAYER_APP_VALUE_TEXT:
        return ("BSA_AV_RC_VD_GET_PLAYER_APP_VALUE_TEXT");
    case BSA_AV_RC_VD_INFORM_DISPLAY_CHARSET:
        return ("BSA_AV_RC_VD_INFORM_DISPLAY_CHARSET");
    case BSA_AV_RC_VD_INFORM_BATTERY_STAT_OF_CT:
        return ("BSA_AV_RC_VD_INFORM_BATTERY_STAT_OF_CT");
    case BSA_AV_RC_VD_GET_ELEMENT_ATTR:
        return ("BSA_AV_RC_VD_GET_ELEMENT_ATTR");
    case BSA_AV_RC_VD_GET_PLAY_STATUS:
        return ("BSA_AV_RC_VD_GET_PLAY_STATUS");
    case BSA_AV_RC_VD_REGISTER_NOTIFICATION:
        return ("BSA_AV_RC_VD_REGISTER_NOTIFICATION");
    case BSA_AV_RC_VD_ABORT_CONTINUATION_RSP:
        return ("BSA_AV_RC_VD_ABORT_CONTINUATION_RSP");
    case BSA_AV_RC_VD_SET_ABSOLUTE_VOLUME:
        return ("BSA_AV_RC_VD_SET_ABSOLUTE_VOLUME");
    case BSA_AV_RC_VD_SET_ADDRESSED_PLAYER:
        return ("BSA_AV_RC_VD_SET_ADDRESSED_PLAYER");
    case BSA_AV_RC_VD_SET_BROWSED_PLAYER:
        return ("BSA_AV_RC_VD_SET_BROWSED_PLAYER");
    case BSA_AV_RC_VD_GET_FOLDER_ITEMS:
        return ("BSA_AV_RC_VD_GET_FOLDER_ITEMS");
    case BSA_AV_RC_VD_CHANGE_PATH:
        return ("BSA_AV_RC_VD_CHANGE_PATH");
    case BSA_AV_RC_VD_GET_ITEM_ATTRIBUTES:
        return ("BSA_AV_RC_VD_GET_ITEM_ATTRIBUTES");
    case BSA_AV_RC_VD_PLAY_ITEM:
        return ("BSA_AV_RC_VD_PLAY_ITEM");
    case BSA_AV_RC_VD_SEARCH:
        return ("BSA_AV_RC_VD_SEARCH");
    case BSA_AV_RC_VD_ADD_TO_NOW_PLAYING:
        return ("BSA_AV_RC_VD_ADD_TO_NOW_PLAYING");
    default:
        break;
    }

    return ("UNKNOWN");
}


/*******************************************************************************
 **
 ** Function         app_av_find_connection_by_handle
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AV_CONNECTION *app_av_find_connection_by_handle(tBSA_AV_HNDL handle)
{
    int index;
    for (index = 0; index < APP_AV_MAX_CONNECTIONS; index++)
    {
        if (app_av_cb.connections[index].handle == handle)
            return &app_av_cb.connections[index];
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_find_connection_by_bd_addr
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
static tAPP_AV_CONNECTION *app_av_find_connection_by_bd_addr(BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < APP_AV_MAX_CONNECTIONS; index++)
    {
        if (bdcmp(app_av_cb.connections[index].bd_addr, bd_addr) == 0)
            return &app_av_cb.connections[index];
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_find_connection_by_status
 **
 ** Description      This function finds a connection structure
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
static tAPP_AV_CONNECTION *app_av_find_connection_by_status(BOOLEAN registered, BOOLEAN open)
{
    int cnt;
    for (cnt = 0; cnt < APP_AV_MAX_CONNECTIONS; cnt++)
    {
        if ((app_av_cb.connections[cnt].is_registered == registered) &&
            (app_av_cb.connections[cnt].is_open == open))
            return &app_av_cb.connections[cnt];
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_display_connections
 **
 ** Description      This function displays the connections
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_av_display_connections(void)
{
    int index;
    for (index = 0; index < APP_AV_MAX_CONNECTIONS; index++)
    {
        APP_INFO1("    Connection index %d:", index);
        if (app_av_cb.connections[index].is_registered)
        {
            APP_INFO1("        Registered -> handle = %d", app_av_cb.connections[index].handle);
            if (app_av_cb.connections[index].is_open)
            {
                APP_INFO1("        Connected to %02X:%02X:%02X:%02X:%02X:%02X",
                    app_av_cb.connections[index].bd_addr[0], app_av_cb.connections[index].bd_addr[1],
                    app_av_cb.connections[index].bd_addr[2], app_av_cb.connections[index].bd_addr[3],
                    app_av_cb.connections[index].bd_addr[4], app_av_cb.connections[index].bd_addr[5]);
                if (app_av_cb.connections[index].is_rc)
                {
                    APP_INFO1("        RC -> handle %d", app_av_cb.connections[index].rc_handle);
                }
                else
                {
                    APP_INFO0("        does not have RC");
                }
            }
            else
            {
                APP_INFO0("        Not connected");
            }
        }
        else
        {
            APP_INFO0("        Not registered");
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_cback
 **
 ** Description      This function is the AV callback function
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_av_cback(tBSA_AV_EVT event, tBSA_AV_MSG *p_data)
{
    int status, index;
    UINT8 command;
    tAPP_AV_CONNECTION *connection;

    switch (event)
    {
    case BSA_AV_OPEN_EVT:
        APP_INFO1("BSA_AV_OPEN_EVT status:%d handle:%d cp:%d aptx:%d sec:%d",
                p_data->open.status, p_data->open.handle,
                p_data->open.cp_supported,
                p_data->open.aptx_supported,
                p_data->open.sec_supported);
        connection = app_av_find_connection_by_handle(p_data->open.handle);
        if (connection == NULL)
        {
            APP_ERROR1("unknown connection handle %d", p_data->open.handle);
        }
        else
        {
            if (p_data->open.status == BSA_SUCCESS)
            {


                /* Copy the BD address of the connected device */
                bdcpy(connection->bd_addr, p_data->open.bd_addr);
                connection->is_open = TRUE;

                /* XML Database update */
                app_read_xml_remote_devices();

                /* Add AV service for this devices in XML database */
                app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                        APP_NUM_ELEMENTS(app_xml_remote_devices_db), connection->bd_addr,
                        BSA_A2DP_SERVICE_MASK | BSA_AVRCP_SERVICE_MASK);

                /* Check if the name in the inquiry responses database */
                for (index = 0; index < APP_NUM_ELEMENTS(app_discovery_cb.devs); index++)
                {
                    if (!bdcmp(app_discovery_cb.devs[index].device.bd_addr, connection->bd_addr))
                    {
                        app_xml_update_name_db(app_xml_remote_devices_db,
                                APP_NUM_ELEMENTS(app_xml_remote_devices_db), connection->bd_addr,
                                app_discovery_cb.devs[index].device.name);
                        APP_INFO1("\tClassOfDevice:%02x:%02x:%02x => %s",
                                app_discovery_cb.devs[index].device.class_of_device[0],
                                app_discovery_cb.devs[index].device.class_of_device[1],
                                app_discovery_cb.devs[index].device.class_of_device[2],
                                app_get_cod_string(
                                        app_discovery_cb.devs[index].device.class_of_device));
                        app_xml_update_cod_db(app_xml_remote_devices_db,
                                APP_NUM_ELEMENTS(app_xml_remote_devices_db), connection->bd_addr,
                                app_discovery_cb.devs[index].device.class_of_device);
                        break;
                    }
                }
                status = app_write_xml_remote_devices();
                if (status < 0)
                {
                    APP_ERROR1("app_av_write_remote_devices failed: %d", status);
                }

                /* Check if autoplay is needed */
#if defined (APP_AV_AUTOPLAY)
                if (app_av_cb.play_state == APP_AV_PLAY_STOPPED)
                {
#if (APP_AV_AUTOPLAY==APP_AV_PLAYTYPE_TONE)
                    /* Play tone */
                    APP_INFO0("Start Playing Tone");
                    app_av_play_tone();
#endif
#if (APP_AV_AUTOPLAY==APP_AV_PLAYTYPE_MIC)
                    /* Play Music received on audio input */
                    APP_INFO0("Start Playing Microphone's input");
                    app_av_play_mic();
#endif
#if (APP_AV_AUTOPLAY==APP_AV_PLAYTYPE_FILE)
                    APP_INFO0("Start Playing playlist");
                    app_av_play_playlist(APP_AV_START);
#endif
                    APP_INFO1("SCMS-T: cp_supported = %s", p_data->open.cp_supported ? "TRUE" : "FALSE");
                }
#endif /* APP_AV_AUTOPLAY */
            }
            else
            {
                connection->is_open = FALSE;
            }
        }
        break;

    case BSA_AV_CLOSE_EVT:
        APP_INFO1("BSA_AV_CLOSE_EVT status:%d handle:%d", p_data->close.status, p_data->close.handle);
        connection = app_av_find_connection_by_handle(p_data->close.handle);
        if (connection == NULL)
        {
            APP_ERROR1("unknown connection handle %d", p_data->close.handle);
        }
        else
        {
            connection->is_open = FALSE;
        }
        break;

    case BSA_AV_DELAY_RPT_EVT:
        APP_INFO1("BSA_AV_DELAY_RPT_EVT channel:%d handle:%d delay: %d",
                  p_data->delay.channel, p_data->delay.handle, p_data->delay.delay);
        connection = app_av_find_connection_by_handle(p_data->delay.handle);
        if (connection == NULL)
        {
            APP_ERROR1("unknown connection handle %d", p_data->delay.handle);
        }
        else
        {
            connection->delay = p_data->delay.delay;
        }
        break;

    case BSA_AV_PENDING_EVT:
        {
            tAPP_AV_CONNECTION *p_con;
            tBSA_AV_OPEN open_param;

            APP_INFO1("BSA_AV_PENDING_EVT address=%x:%x:%x:%x:%x:%x",p_data->pend.bd_addr[0],
            p_data->pend.bd_addr[1],p_data->pend.bd_addr[2],p_data->pend.bd_addr[3],
            p_data->pend.bd_addr[4],p_data->pend.bd_addr[5]);
            p_con = app_av_find_connection_by_status(TRUE, FALSE);
            if (p_con == NULL)
            {
                APP_ERROR0("No available connection structure to open stream");
                return;
            }

            /* Open AV stream */
            APP_INFO0("Connecting to AV device");
            BSA_AvOpenInit(&open_param);
            bdcpy(open_param.bd_addr, p_data->pend.bd_addr);
            open_param.handle = p_con->handle;
            /* Indicate if AVRC must be used */
            open_param.use_rc = APP_AV_USE_RC;
            status = BSA_AvOpen(&open_param);
            if (status != BSA_SUCCESS)
            {
                APP_ERROR1("BSA_AvOpen(%02X:%02X:%02X:%02X:%02X:%02X) failed: %d",
                       open_param.bd_addr[0], open_param.bd_addr[1], open_param.bd_addr[2],
                       open_param.bd_addr[3], open_param.bd_addr[4], open_param.bd_addr[5], status);
                return ;
            }
        }
        break;

    case BSA_AV_START_EVT:
        APP_INFO1("BSA_AV_START_EVT status:%d channel:%x UIPC:%d SCMS:%d,%d",
                p_data->start.status, p_data->start.channel, p_data->start.uipc_channel,
                p_data->start.cp_enabled, p_data->start.cp_flag);
        /* If this is a Point to Point AV connection */
        if (p_data->start.channel == BSA_AV_CHNL_AUDIO)
        {
            if (p_data->start.status == BSA_SUCCESS)
            {
                app_av_cb.play_state = APP_AV_PLAY_STARTED;
                app_av_rc_change_play_status(BSA_AVRC_PLAYSTATE_PLAYING);
                /* Copy the feeding information */
                app_av_cb.media_feeding = p_data->start.media_feeding;
                switch (app_av_cb.media_feeding.format)
                {
                case BSA_AV_CODEC_PCM:
                    APP_INFO1("    Start PCM feeding: freq=%d / channels=%d / bits=%d",
                            app_av_cb.media_feeding.cfg.pcm.sampling_freq,
                            app_av_cb.media_feeding.cfg.pcm.num_channel,
                            app_av_cb.media_feeding.cfg.pcm.bit_per_sample);
                    break;
                case BSA_AV_CODEC_APTX:
                    APP_INFO1("    Start apt-X feeding: freq=%d / mode=%d",
                            app_av_cb.media_feeding.cfg.aptx.sampling_freq,
                            app_av_cb.media_feeding.cfg.aptx.ch_mode);
                    break;
                case BSA_AV_CODEC_SEC:
                    APP_INFO1("    Start SEC feeding: freq=%d / mode=%d",
                            app_av_cb.media_feeding.cfg.sec.sampling_freq,
                            app_av_cb.media_feeding.cfg.sec.ch_mode);
                    break;

                default:
                    APP_ERROR1("Unsupported feeding format code: %d", app_av_cb.media_feeding.format);
                    break;
                }

                APP_INFO1("    SCMS-T: cp_enabled = %s, cp_flag = %d",
                        p_data->start.cp_enabled ? "TRUE" : "FALSE",
                                p_data->start.cp_flag);

                if (app_dm_get_dual_stack_mode() == BSA_DM_DUAL_STACK_MODE_BSA)
                {
                    /* Reconfigure the UIPC to adapt to the feeding mode */
                    app_av_uipc_reconfig();

                    /* Unlock the TX thread if it was locked */
                    status = app_unlock_mutex(&app_av_cb.app_stream_tx_mutex);
                    if (status < 0)
                    {
                        APP_ERROR1("app_unlock_mutex failed:%d", status);
                    }
                }
                else
                {
                    APP_INFO0("Stack is not in BSA mode. Do not Start AV thread");
                }
            }
            else
            {
                /* In case we are playing a list, finish */
                app_av_cb.play_list = FALSE;
            }
            app_av_cb.last_start_status = p_data->start.status;
        }
        /* Else, this is a Broadcast channel */
        else
        {
#ifdef APP_AV_BCST_INCLUDED
            app_av_bcst_start_event_hdlr(&p_data->start);
#endif
        }
        break;

    case BSA_AV_STOP_EVT:
        APP_DEBUG1("BSA_AV_STOP_EVT pause:%d Channel:%d UIPC:%d",
                p_data->stop.pause, p_data->stop.channel, p_data->stop.uipc_channel);
        /* If this is a Point to Point AV connection */
        if (p_data->stop.channel == BSA_AV_CHNL_AUDIO)
        {
            /* Check if it is a pause stop response */
            if (p_data->stop.pause)
            {
                app_av_cb.play_state = APP_AV_PLAY_PAUSED;
                app_av_rc_change_play_status(BSA_AVRC_PLAYSTATE_PAUSED);
            }
            else
            {
                /* all AV streams are stopped */
                app_av_cb.play_state = APP_AV_PLAY_STOPPED;
                app_av_rc_change_play_status(BSA_AVRC_PLAYSTATE_STOPPED);

                if (app_dm_get_dual_stack_mode() == BSA_DM_DUAL_STACK_MODE_BSA)
                {
                    /* Unlock pause if ever we were on pause */
                    status = app_unlock_mutex(&app_av_cb.app_stream_tx_mutex);
                    if (status < 0)
                    {
                        APP_ERROR1("app_unlock_mutex failed:%d", status);
                    }
                }
                else
                {
                    APP_INFO0("Stack is not in BSA mode. Do not Stop AV thread");
                }
            }
        }
        /* Else, if this is a Broadcast channel */
        else  if (p_data->stop.channel == BSA_AV_CHNL_AUDIO_BCST)
        {
#ifdef APP_AV_BCST_INCLUDED
            app_av_bcst_stop_event_hdlr(&p_data->stop);
#endif
        }
        else
        {
            APP_ERROR1("Bad channel:%d Stopped", p_data->stop.channel);
        }
        break;

    case BSA_AV_RC_OPEN_EVT:
        APP_DEBUG1("BSA_AV_RC_OPEN_EVT status:%d handle:%d", p_data->rc_open.status, p_data->rc_open.rc_handle);

        connection = app_av_find_connection_by_bd_addr(p_data->rc_open.peer_addr);
        if (connection == NULL)
        {
            APP_ERROR1("unknown connection bd addr for %02X:%02X:%02X:%02X:%02X:%02X",
                p_data->rc_open.peer_addr[0], p_data->rc_open.peer_addr[1], p_data->rc_open.peer_addr[2],
                p_data->rc_open.peer_addr[3], p_data->rc_open.peer_addr[4], p_data->rc_open.peer_addr[5]);
        }
        else if (p_data->rc_open.status == BSA_SUCCESS)
        {
            connection->is_rc = TRUE;
            connection->rc_handle = p_data->rc_open.rc_handle;
        }
        break;

    case BSA_AV_RC_CLOSE_EVT:
        APP_DEBUG1("BSA_AV_RC_CLOSE_EVT handle:%d bdaddr %02X:%02X:%02X:%02X:%02X:%02X", p_data->rc_close.rc_handle,
                p_data->rc_close.peer_addr[0], p_data->rc_close.peer_addr[1], p_data->rc_close.peer_addr[2],
                p_data->rc_close.peer_addr[3], p_data->rc_close.peer_addr[4], p_data->rc_close.peer_addr[5]);

        connection = app_av_find_connection_by_bd_addr(p_data->rc_close.peer_addr);
        if (connection == NULL)
        {
            APP_ERROR1("unknown connection bd addr for %02X:%02X:%02X:%02X:%02X:%02X",
                p_data->rc_close.peer_addr[0], p_data->rc_close.peer_addr[1], p_data->rc_close.peer_addr[2],
                p_data->rc_close.peer_addr[3], p_data->rc_close.peer_addr[4], p_data->rc_close.peer_addr[5]);
        }
        else
        {
            connection->is_rc = FALSE;
        }
        break;

    case BSA_AV_REMOTE_CMD_EVT:
        APP_DEBUG1("BSA_AV_REMOTE_CMD_EVT handle:%d", p_data->remote_cmd.rc_handle);
        switch(p_data->remote_cmd.rc_id)
        {
        case BSA_AV_RC_PLAY:
            APP_INFO0("Play key");
            command = APP_AV_START;
            break;
        case BSA_AV_RC_STOP:
            APP_INFO0("Stop key");
            command = APP_AV_STOP;
            break;
        case BSA_AV_RC_PAUSE:
            APP_INFO0("Pause key");
            command = APP_AV_PAUSE;
            break;
        case BSA_AV_RC_FORWARD:
            APP_INFO0("Forward key");
            command = APP_AV_FORWARD;
            break;
        case BSA_AV_RC_BACKWARD:
            APP_INFO0("Backward key");
            command = APP_AV_BACKWARD;
            break;
        default:
            APP_INFO1("key:0x%x", p_data->remote_cmd.rc_id);
            command = APP_AV_IDLE;
            break;
        }
        if (p_data->remote_cmd.key_state == BSA_AV_STATE_PRESS)
        {
            APP_INFO1("Key pressed -> executing command %d", command);
            switch(command)
            {
            case APP_AV_START:
                switch (app_av_cb.play_state)
                {
                case APP_AV_PLAY_PAUSED:
                    app_av_resume();
                    break;
                case APP_AV_PLAY_STOPPED:
                    switch (app_av_cb.play_type)
                    {
                    case APP_AV_PLAYTYPE_FILE:
                        app_av_play_playlist(command);
                        break;
                    case APP_AV_PLAYTYPE_TONE:
                        app_av_play_tone();
                        break;
                    case APP_AV_PLAYTYPE_MIC:
                        app_av_play_mic();
                        break;
                    case APP_AV_PLAYTYPE_TEST:
                        app_av_test_sec_codec(TRUE);
                        break;
                    default:
                        APP_ERROR1("Unsupported play type (%d)", app_av_cb.play_type);
                        break;
                    }
                    break;
                case APP_AV_PLAY_STARTED:
                    /* Already started */
                    APP_DEBUG0("Stream already started, not calling start");
                    break;
                case APP_AV_PLAY_STOPPING:
                    /* Stop in progress */
                    APP_DEBUG0("Stream stopping, not calling start");
                    break;
                default:
                    APP_ERROR1("Unsupported play state (%d)", app_av_cb.play_state);
                    break;
                }
                break;
            case APP_AV_STOP:
                app_av_stop();
                break;
            case APP_AV_PAUSE:
                app_av_pause();
                break;
            case APP_AV_FORWARD:
            case APP_AV_BACKWARD:
                if (app_av_cb.play_state != APP_AV_PLAY_STOPPED)
                    if (app_av_cb.play_type != APP_AV_PLAYTYPE_AVK)
                    /* stop streaming */
                    app_av_stop();

                /* Resume streaming in a different thread so we can wait for callback to confirm
                 * that streaming has stopped */
                app_av_cb.s_command = command;
                status = app_create_thread(app_avk_rc_command_thread, 0, 0,
                            &app_av_cb.t_app_rc_thread);

                break;
            }
        }
        else  if (p_data->remote_cmd.key_state == BSA_AV_STATE_RELEASE)
        {
            APP_INFO0(" released -> just informative");
        }
        else
        {
            APP_ERROR0("Unknown key state");
        }
        break;

    case BSA_AV_REMOTE_RSP_EVT:
        APP_DEBUG1("BSA_AV_REMOTE_RSP_EVT handle:%d", p_data->remote_rsp.rc_handle);
        break;

    case BSA_AV_VENDOR_CMD_EVT:
        APP_DEBUG1("BSA_AV_VENDOR_CMD_EVT handle:%d", p_data->vendor_cmd.rc_handle);
        APP_DEBUG1("    len=%d, label=%d, code=%d, company_id=0x%x",
             p_data->vendor_cmd.len, p_data->vendor_cmd.label, p_data->vendor_cmd.code,
             p_data->vendor_cmd.company_id);
        if(p_data->vendor_cmd.len)
        {
            APP_DEBUG1("Name:%s",app_av_display_vendor_command(p_data->vendor_cmd.data[0]));
            APP_DUMP("Data", (UINT8 *)p_data->vendor_cmd.data, p_data->vendor_cmd.len);
        }
        break;

    case BSA_AV_VENDOR_RSP_EVT:
        APP_DEBUG1("BSA_AV_VENDOR_RSP_EVT handle:%d", p_data->vendor_rsp.rc_handle);
        APP_DEBUG1("    len=%d, label=%d, code=%d, company_id=0x%x",
             p_data->vendor_rsp.len, p_data->vendor_rsp.label, p_data->vendor_rsp.code,
             p_data->vendor_rsp.company_id);
        if(p_data->vendor_rsp.len)
        {
            APP_DEBUG1("Name:%s",app_av_display_vendor_command(p_data->vendor_rsp.data[0]));
            APP_DUMP("Data", (UINT8 *)p_data->vendor_rsp.data, p_data->vendor_rsp.len);
        }
        break;

    case BSA_AV_META_MSG_EVT:
        APP_DEBUG1("BSA_AV_META_MSG_EVT handle:%d", p_data->meta_msg.rc_handle);

        APP_DEBUG1("    label=%d, pdu=%d, company_id=0x%x",
            p_data->meta_msg.label, p_data->meta_msg.pdu, p_data->meta_msg.company_id);

        app_av_cb.label = p_data->meta_msg.label;

        switch(p_data->meta_msg.pdu)
        {
        case BSA_AVRC_PDU_GET_ELEMENT_ATTR:
            app_av_rc_send_get_element_attributes_meta_response(0);
            break;

        case BSA_AVRC_PDU_GET_PLAY_STATUS:
            app_av_rc_send_play_status_meta_response(0);
            break;

        case BSA_AVRC_PDU_SET_ADDRESSED_PLAYER:
            app_av_rc_set_addr_player_meta_response(0, &p_data->meta_msg);
            break;

        case BSA_AVRC_PDU_GET_FOLDER_ITEMS:
            app_av_rc_get_folder_items(0, &p_data->meta_msg);
            break;

        case BSA_AVRC_PDU_REGISTER_NOTIFICATION:
            app_av_rc_register_notifications(0, &p_data->meta_msg);
            break;

        case BSA_AVRC_PDU_SET_BROWSED_PLAYER:
            app_av_rc_set_browsed_player_meta_response(0, &p_data->meta_msg);
            break;

        case BSA_AVRC_PDU_CHANGE_PATH:
            app_av_rc_change_path_meta_response(0, &p_data->meta_msg);
            break;

        case BSA_AVRC_PDU_GET_ITEM_ATTRIBUTES:
            app_av_rc_get_item_attr_meta_response(0, &p_data->meta_msg);
            break;

        case BSA_AVRC_PDU_PLAY_ITEM:
            app_av_rc_play_item_meta_response(0, &p_data->meta_msg);
            break;
        }

        break;

    case BSA_AV_FEAT_EVT:
        APP_DEBUG1("BSA_AV_FEAT_EVT Peer feature:%x", p_data->rc_feat.peer_features);
        break;

    default:
        APP_ERROR1("Unknown msg %d", event);
        break;
    }

    /* Forward app callback */
    if(app_av_cb.p_Callback)
        app_av_cb.p_Callback(event, p_data);
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_command_thread
 **
 ** Description      Thread to handle RC pass thru command (FORWARD/REVIND)
 **
 *******************************************************************************/
static void app_avk_rc_command_thread(void)
{
    int waitcnt = 0;

    /* Previous command issued command to stop streaming. Now wait till
     * the callback is received that streaming has stopped */
    while((app_av_get_play_state() == APP_AV_PLAY_STOPPING) &&
          (app_av_get_play_state()!= APP_AV_PLAY_STOPPED) &&
          waitcnt < 5)
    {
        sleep(1);
        waitcnt++;
    }

    if(app_av_cb.play_type == APP_AV_PLAYTYPE_FILE)
        app_av_play_playlist(app_av_cb.s_command);

    if(app_av_cb.s_command == APP_AV_FORWARD)
    {
        p_bsa_av_cb->cur_play++;
        if(p_bsa_av_cb->cur_play >= BSA_AV_NUMSONGS)
            p_bsa_av_cb->cur_play = 0;
        app_av_rc_change_track();
    }
    else if(app_av_cb.s_command == APP_AV_BACKWARD)
    {
        if(p_bsa_av_cb->cur_play > 0)
            p_bsa_av_cb->cur_play--;
        else
            p_bsa_av_cb->cur_play = BSA_AV_NUMSONGS -1;
        app_av_rc_change_track();
    }
}

/*******************************************************************************
 **
 ** Function         app_av_open
 **
 ** Description      Function to open AV connection
 **
 ** Parameters       BD_ADDR of the deivce to connect (if null, user will be prompted)
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_open(BD_ADDR *bd_addr_in)
{
    int status;
    int choice;
    BOOLEAN connect = FALSE;
    BD_ADDR bd_addr;
    tAPP_AV_CONNECTION *p_con;
    tBSA_AV_OPEN open_param;

    if(bd_addr_in == NULL)
    {

    APP_INFO0("Bluetooth AV Open menu:");
    APP_INFO0("    0 Device from XML database (already paired)");
    APP_INFO0("    1 Device found in last discovery");
    choice = app_get_choice("Select source");

    switch(choice)
    {
    case 0:
        /* Read the Remote device XML file to have a fresh view */
        app_read_xml_remote_devices();

        app_xml_display_devices(app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));
        choice = app_get_choice("Select device");
        if ((choice >= 0) && (choice < APP_NUM_ELEMENTS(app_xml_remote_devices_db)))
        {
            if (app_xml_remote_devices_db[choice].in_use != FALSE)
            {
                bdcpy(bd_addr, app_xml_remote_devices_db[choice].bd_addr);
                connect = TRUE;
            }
            else
            {
                APP_ERROR0("Device entry not in use");
            }
        }
        else
        {
            APP_ERROR0("Unsupported device number");
        }

        break;
    case 1:
        app_disc_display_devices();
        choice = app_get_choice("Select device");
        if ((choice >= 0) && (choice < APP_NUM_ELEMENTS(app_discovery_cb.devs)))
        {
            if (app_discovery_cb.devs[choice].in_use != FALSE)
            {
                bdcpy(bd_addr, app_discovery_cb.devs[choice].device.bd_addr);
                connect = TRUE;
            }
            else
            {
                APP_ERROR0("Device entry not in use");
            }
        }
        else
        {
            APP_ERROR0("Unsupported device number");
        }
        break;
    default:
        APP_ERROR0("Unsupported choice");
        break;
    }
    }

    else
    {
        bdcpy(bd_addr, *bd_addr_in);
        connect = TRUE;
    }



    if (connect)
    {
        /* find an available connection */
        p_con = app_av_find_connection_by_status(TRUE, FALSE);
        if (p_con == NULL)
        {
            APP_ERROR0("No available connection structure to open stream");
            return -1;
        }

        /* Open AV stream */
        APP_INFO0("Connecting to AV device");
        BSA_AvOpenInit(&open_param);
        bdcpy(open_param.bd_addr, bd_addr);
        open_param.handle = p_con->handle;
        /* Indicate if AVRC must be used */
        open_param.use_rc = APP_AV_USE_RC;
        status = BSA_AvOpen(&open_param);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_AvOpen(%02X:%02X:%02X:%02X:%02X:%02X) failed: %d",
                   open_param.bd_addr[0], open_param.bd_addr[1], open_param.bd_addr[2],
                   open_param.bd_addr[3], open_param.bd_addr[4], open_param.bd_addr[5], status);
            return -1;
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_close
 **
 ** Description      Function to close AV connection
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_close(void)
{
    int status;
    int choice;
    tAPP_AV_CONNECTION *p_con;
    tBSA_AV_CLOSE close_param;

    choice = 0;
#ifndef QT_APP
    app_av_display_connections();
    choice = app_get_choice("Select the connection to close");
    /* Sanity check */
    if ((choice < 0) || (choice >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }
#endif

    p_con = &app_av_cb.connections[choice];
    if (!p_con->is_open)
    {
        APP_ERROR0("not connected");
        return -1;
    }
    /* Close av connection */
    status = BSA_AvCloseInit(&close_param);
    close_param.handle = p_con->handle;
    status = BSA_AvClose(&close_param);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvClose failed status = %d", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function         app_av_read_tone
 **
 ** Description      fill up a PCM buffer
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_av_read_tone(short * pOut, int nb_bytes, UINT8 sinus_type, UINT8 *p_sinus_index)
{
    int index;
    UINT8 sinus_index = *p_sinus_index;

    /* Generate a standard PCM stereo interlaced sinewave */
    for (index = 0; index < (nb_bytes / 4); index++)
    {
        pOut[index * 2] = sinwaves[sinus_type][sinus_index % 64];
        pOut[index * 2 + 1] = sinwaves[sinus_type][sinus_index % 64];
        sinus_index++;
    }
    *p_sinus_index = sinus_index;
}
/*******************************************************************************
 **
 ** Function         app_av_read_test
 **
 ** Description      fill up a PCM buffer
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_av_read_test(short * pOut, int nb_bytes)
{
    int index;
    static short data = 1;

    data++;
    for (index = 0; index < nb_bytes/2 ; index++)
    {
        pOut[index] = data;
    }

    APP_DEBUG1("app_av_read_test data = 0x%x, length = %d", data, nb_bytes);
}

/*******************************************************************************
 **
 ** Function         app_av_set_tone_sample_frequency
 **
 ** Description      Change the tone sampling frequency before start
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_av_set_tone_sample_frequency(UINT16 sample_freq)
{
    APP_DEBUG1("app_av_set_tone_sample_frequency %d", sample_freq);
    app_av_cb.tone_sample_freq = sample_freq;
}

/*******************************************************************************
 **
 ** Function         app_av_toggle_tone
 **
 ** Description      Toggle the tone type
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_av_toggle_tone(void)
{
    if (app_av_cb.sinus_type == 0)
        app_av_cb.sinus_type = 1;
    else
        app_av_cb.sinus_type = 0;
}

/*******************************************************************************
 **
 ** Function         app_av_play_tone
 **
 ** Description      Example of function to play a tone
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_tone(void)
{
    int status;
    tBSA_AV_START start_param;

    if( ((app_av_cb.play_state != APP_AV_PLAY_STOPPED) || app_av_cb.play_list) &&
            (app_av_cb.play_state != APP_AV_PLAY_PAUSED))
    {
        APP_INFO0("Could not perform the play operation, please stop the stream first");
        return -1;
    }

    /* reset the current tone */
    app_av_cb.sinus_index = 0;
    app_av_cb.sinus_type = 0;

    /* start AV stream */
    BSA_AvStartInit(&start_param);
    start_param.media_feeding.format = BSA_AV_CODEC_PCM;

    start_param.media_feeding.cfg.pcm.sampling_freq = app_av_cb.tone_sample_freq;
    start_param.media_feeding.cfg.pcm.num_channel = 2;
    start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    start_param.feeding_mode = app_av_cb.uipc_cfg.is_blocking? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
    start_param.latency = app_av_cb.uipc_cfg.period/1000; /* convert us to ms, synchronous feeding mode only*/

    /* Content Protection */
    start_param.cp_id = app_av_cb.cp_id;
    start_param.scmst_flag = app_av_cb.cp_scms_flag;

    app_av_cb.play_type = APP_AV_PLAYTYPE_TONE;
    app_av_cb.play_list = FALSE;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed:%d", status);
        return status;
    }

    return 0;
}

int app_av_play_from_avk()
{
    int status;
    tBSA_AV_START start_param;

    if ((app_av_cb.play_state != APP_AV_PLAY_STOPPED) || app_av_cb.play_list)
    {
        APP_INFO0("Could not perform the play operation, please stop the stream first");
        return -1;
    }

    /* start AV stream */
    BSA_AvStartInit(&start_param);

    app_av_cb.play_type = APP_AV_PLAYTYPE_AVK;
    app_av_cb.play_list = FALSE;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed:%d", status);
        return status;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_play_file
 **
 ** Description      Example of function to Close AV connection
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_file(void)
{
    int status;
    tBSA_AV_START start_param;
    int choice;
    tAPP_WAV_FILE_FORMAT wav_format;

    if ((app_av_cb.play_state != APP_AV_PLAY_STOPPED) || app_av_cb.play_list)
    {
        APP_INFO0("Could not perform the play operation, please stop the stream first");
        return -1;
    }

    app_av_display_playlist();

    choice = app_get_choice("Select file");
    if ((choice < 0 ) || (choice >= app_av_cb.soundfile_list_size))
    {
        APP_ERROR1("File index out of bounds:%d", choice);
        return -1;
    }

    if (app_wav_format(app_av_cb.soundfile_list[choice], &wav_format) < 0)
    {
        APP_ERROR1("Unable to extract WAV format from:%s", app_av_cb.soundfile_list[choice]);
    }
    else
    {
        APP_INFO1("%d :%s", (int)choice, app_av_cb.soundfile_list[choice]);
        APP_INFO1("    codec(%s) ch(%d) bits(%d) rate(%d)", (wav_format.codec==BSA_AV_CODEC_PCM)?"PCM":"apt-X",
                wav_format.nb_channels, wav_format.bits_per_sample, (int)wav_format.sample_rate);
    }

    app_av_cb.file_index = choice;
    strncpy(app_av_cb.file_name, app_av_cb.soundfile_list[choice], sizeof(app_av_cb.file_name)-1);
    app_av_cb.file_name[sizeof(app_av_cb.file_name)-1] = '\0';

    /* start AV stream */
    BSA_AvStartInit(&start_param);

    start_param.media_feeding.format = wav_format.codec;
    switch(wav_format.codec)
    {
    case BSA_AV_CODEC_PCM:
        start_param.media_feeding.cfg.pcm.sampling_freq = wav_format.sample_rate;
        start_param.media_feeding.cfg.pcm.num_channel = wav_format.nb_channels;
        start_param.media_feeding.cfg.pcm.bit_per_sample = wav_format.bits_per_sample;
        break;
    case BSA_AV_CODEC_APTX:
        start_param.media_feeding.cfg.aptx.sampling_freq = wav_format.sample_rate;
        if (wav_format.nb_channels == 2)
        {
            if (wav_format.stereo_mode == 2)
            {
                start_param.media_feeding.cfg.aptx.ch_mode = BSA_AV_CHANNEL_MODE_STEREO;
            }
            else
            {
                start_param.media_feeding.cfg.aptx.ch_mode = BSA_AV_CHANNEL_MODE_JOINT;
            }
        }
        else
        {
            start_param.media_feeding.cfg.aptx.ch_mode = BSA_AV_CHANNEL_MODE_MONO;
        }
        break;
    default:
        APP_ERROR1("Unsupported codec (x%x) in WAV file (%s)", wav_format.codec, app_av_cb.soundfile_list[choice]);
        return -1;
    }
    start_param.feeding_mode = app_av_cb.uipc_cfg.is_blocking? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
    start_param.latency = app_av_cb.uipc_cfg.period/1000; /* convert us to ms, synchronous feeding mode only*/

    /* Content Protection */
    start_param.cp_id = app_av_cb.cp_id;
    start_param.scmst_flag = app_av_cb.cp_scms_flag;

    app_av_cb.play_type = APP_AV_PLAYTYPE_FILE;
    app_av_cb.play_list = FALSE;

    status = BSA_AvStart(&start_param);
    if (status == BSA_ERROR_SRV_AV_FEEDING_NOT_SUPPORTED)
    {
        APP_ERROR0("BSA_AvStart failed because file encoding is not supported by remote devices");
        return status;
    }
    else if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed: %d", status);
        return status;
    }

    /* this is an active wait for demo purpose */
    APP_INFO0("Waiting for AV connection to start");
    while (app_av_cb.play_state != APP_AV_PLAY_STARTED);

    if (app_av_cb.last_start_status != BSA_SUCCESS)
    {
        APP_ERROR1("Failed starting AV connection : status %d", app_av_cb.last_start_status);
        return status;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_play_playlist
 **
 ** Description      Example of start to play a play list
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_playlist(UINT8 command)
{
    int status;
    tBSA_AV_START start_param;
    tAPP_WAV_FILE_FORMAT wav_format;
    BOOLEAN format_known;
    int attempt = 0;

    if ((app_av_get_play_state()!= APP_AV_PLAY_STOPPED) &&
        (app_av_get_play_state() != APP_AV_PLAY_STOPPING) &&
        (app_av_get_play_state() != APP_AV_PLAY_PAUSED))
    {
        APP_INFO0("Could not perform the play operation, please stop the stream first");
        return -1;
    }

    if ((app_av_cb.soundfile_list_size == 0) || (app_av_cb.soundfile_list == NULL))
    {
        APP_ERROR0("No playlist");
        return -1;
    }

    /* Start */
    if (command == APP_AV_START)
    {
        /* Start playing wherever we left off -> use last index */
        APP_DEBUG1("app_av_play_playlist start index:%d", app_av_cb.file_index);
    }
    /* Next file */
    else if (command == APP_AV_FORWARD)
    {
        if (app_av_cb.file_index >= (app_av_cb.soundfile_list_size -1))
            app_av_cb.file_index = 0;
        else
            app_av_cb.file_index++;
    }
    /* Previous file */
    else if (command == APP_AV_BACKWARD)
    {
        if (app_av_cb.file_index == 0)
            app_av_cb.file_index = app_av_cb.soundfile_list_size - 1;
        else
            app_av_cb.file_index--;
    }
    else
    {
        APP_ERROR1("Unsupported command:%d", command);
        return -1;
    }

    app_av_cb.play_type = APP_AV_PLAYTYPE_FILE;
    app_av_cb.play_list = TRUE;

    do
    {
        format_known = TRUE;
        status = -1;

        /* Initialize the start parameters */
        BSA_AvStartInit(&start_param);
        if (app_wav_format(app_av_cb.soundfile_list[app_av_cb.file_index], &wav_format) < 0)
        {
            APP_ERROR1("Unable to extract WAV format from:%s", app_av_cb.soundfile_list[app_av_cb.file_index]);
        }
        else
        {
            APP_INFO1("Let's try to play WAV file:%s format:", app_av_cb.soundfile_list[app_av_cb.file_index]);
            APP_INFO1("    codec(%s) ch(%d) bits(%d) rate(%d)", (wav_format.codec==BSA_AV_CODEC_PCM)?"PCM":"apt-X",
                    wav_format.nb_channels, wav_format.bits_per_sample, (int)wav_format.sample_rate);
            start_param.media_feeding.format = wav_format.codec;
            switch(wav_format.codec)
            {
            case BSA_AV_CODEC_PCM:
                start_param.media_feeding.cfg.pcm.sampling_freq = wav_format.sample_rate;
                start_param.media_feeding.cfg.pcm.num_channel = wav_format.nb_channels;
                start_param.media_feeding.cfg.pcm.bit_per_sample = wav_format.bits_per_sample;
                break;
            case BSA_AV_CODEC_APTX:
                start_param.media_feeding.cfg.aptx.sampling_freq = wav_format.sample_rate;
                if (wav_format.nb_channels == 2)
                {
                    if (wav_format.stereo_mode == 2)
                    {
                        start_param.media_feeding.cfg.aptx.ch_mode = BSA_AV_CHANNEL_MODE_STEREO;
                    }
                    else
                    {
                        start_param.media_feeding.cfg.aptx.ch_mode = BSA_AV_CHANNEL_MODE_JOINT;
                    }
                }
                else
                {
                    start_param.media_feeding.cfg.aptx.ch_mode = BSA_AV_CHANNEL_MODE_MONO;
                }
                break;
            default:
                APP_ERROR1("Unsupported codec (x%x) in WAV file (%s)", wav_format.codec, app_av_cb.soundfile_list[app_av_cb.file_index]);
                format_known = FALSE;
                break;
            }
            start_param.feeding_mode = app_av_cb.uipc_cfg.is_blocking? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
            start_param.latency = app_av_cb.uipc_cfg.period/1000; /* convert us to ms, synchronous feeding mode only */
        }

        if (format_known)
        {
            strncpy(app_av_cb.file_name, app_av_cb.soundfile_list[app_av_cb.file_index], sizeof(app_av_cb.file_name)-1);
            app_av_cb.file_name[sizeof(app_av_cb.file_name)-1] = '\0';

            /* Content Protection */
            start_param.cp_id = app_av_cb.cp_id;
            start_param.scmst_flag = app_av_cb.cp_scms_flag;

            status = BSA_AvStart(&start_param);
            if (status != BSA_SUCCESS)
            {
                APP_ERROR1("BSA_AvStart failed: %d", status);
            }
        }

        /* Try Next file */
        if (status != BSA_SUCCESS)
        {
            if ((command == APP_AV_START) || (command == APP_AV_FORWARD))
            {
                if (app_av_cb.file_index >= (app_av_cb.soundfile_list_size - 1))
                {
                    app_av_cb.file_index = 0;
                }
                else
                {
                    app_av_cb.file_index++;
                }
            }
            /* Try Previous file */
            else if (command == APP_AV_BACKWARD)
            {
                if (app_av_cb.file_index == 0)
                {
                    app_av_cb.file_index = app_av_cb.soundfile_list_size - 1;
                }
                else
                {
                    app_av_cb.file_index--;
                }
            }
            APP_DEBUG1("playlist_index:%d app_av_cb.soundfile_list_size:%d", app_av_cb.file_index, app_av_cb.soundfile_list_size);

            /* to prevent looping forever */
            attempt++;
        }
    } while((status != BSA_SUCCESS) && (attempt < app_av_cb.soundfile_list_size));

    if (status == BSA_SUCCESS)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*******************************************************************************
 **
 ** Function         app_av_play_mic
 **
 ** Description      Example of function to play a microphone input
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_play_mic(void)
{
#ifdef PCM_ALSA
    int status;
    tAPP_ALSA_CAPTURE_OPEN asla_capture;
    tBSA_AV_START start_param;

    if (app_av_cb.play_state != APP_AV_PLAY_STOPPED)
    {
        APP_INFO0("Could not perform the play operation, please stop the stream first");
        return -1;
    }

    /* open and configure ALSA capture device */
    app_alsa_capture_open_init(&asla_capture);

    asla_capture.access = APP_ALSA_PCM_ACCESS_RW_INTERLEAVED;
    asla_capture.blocking = app_av_cb.uipc_cfg.is_blocking; /* Non Blocking */
    asla_capture.format = APP_ALSA_PCM_FORMAT_S16_LE; /* Signed 16 bits Little Endian*/
    asla_capture.sample_rate = 48000; /* 48KHz */
    asla_capture.stereo = TRUE; /* Stereo */
    status = app_alsa_capture_open(&asla_capture);
    if (status < 0)
    {
        APP_ERROR1("app_alsa_capture_open fail:%d", status);
        return -1;
    }
    APP_DEBUG0("ALSA driver opened in capture mode (Microphone/LineIn)");
    app_av_cb.alsa_capture_opened = TRUE;

    /* start Av stream */
    BSA_AvStartInit(&start_param);
    start_param.media_feeding.format = BSA_AV_CODEC_PCM;
    start_param.media_feeding.cfg.pcm.sampling_freq = 48000;
    start_param.media_feeding.cfg.pcm.num_channel = 2;
    start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    start_param.feeding_mode = app_av_cb.uipc_cfg.is_blocking? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
    start_param.latency = app_av_cb.uipc_cfg.period/1000; /* convert us to ms, synchronous feeding mode only */

    /* Content Protection */
    start_param.cp_id = app_av_cb.cp_id;
    start_param.scmst_flag = app_av_cb.cp_scms_flag;

    app_av_cb.play_type = APP_AV_PLAYTYPE_MIC;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed: %d", status);
        return status;
    }

    return 0;
#else
    APP_ERROR0("Microphone input not supported on this target (see PCM_ALSA in makefile)");
    return -1;
#endif
}

/*******************************************************************************
 **
 ** Function         app_av_stop_current
 **
 ** Description      Stop playing current stream
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
static int app_av_stop_current(void)
{
    int status;
    tBSA_AV_STOP stop_param;
    UINT8 play_state;

    /* stopping state before calling BSA api, call back happens asynchronously */
    /* TODO : protect critical section */
    play_state = app_av_cb.play_state;
    app_av_cb.play_state = APP_AV_PLAY_STOPPING;
    app_av_rc_change_play_status(BSA_AVRC_PLAYSTATE_STOPPED);

    /* Stop the AV stream */
    status = BSA_AvStopInit(&stop_param);
    stop_param.pause = FALSE;
    status = BSA_AvStop(&stop_param);
    if (status != BSA_SUCCESS)
    {
        app_av_cb.play_state = play_state;
        APP_ERROR1("BSA_AvStop failed: %d", status);
        return status;
    }

    return 0;

}

/*******************************************************************************
 **
 ** Function         app_av_stop
 **
 ** Description      Stop playing completely
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_stop(void)
{
    app_av_cb.play_list = FALSE;

    return app_av_stop_current();
}

/*******************************************************************************
 **
 ** Function         app_av_pause
 **
 ** Description      Pause playing
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_pause(void)
{
    int status;
    tBSA_AV_STOP stop_param;
    UINT8 play_state;

    /* Stopping state before calling BSA api, call back happens asynchronously */
    /* TODO : protect critical section */
    play_state = app_av_cb.play_state;

    if((app_av_cb.play_state == APP_AV_PLAY_STOPPED) ||
       (app_av_cb.play_state == APP_AV_PLAY_PAUSED))

    {
        APP_DEBUG1("app_av_pause - already paused or stopped, %d", app_av_cb.play_state);
        return BSA_SUCCESS;
    }

    app_av_cb.play_state = APP_AV_PLAY_PAUSED;
    app_av_rc_change_play_status(BSA_AVRC_PLAYSTATE_PAUSED);

    /* Stop the AV stream */
    status = BSA_AvStopInit(&stop_param);
    stop_param.pause = TRUE;
    status = BSA_AvStop(&stop_param);
    if (status != BSA_SUCCESS)
    {
        app_av_cb.play_state = play_state;
        APP_ERROR1("BSA_AvStop failed: %d", status);
        return status;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_resume
 **
 ** Description      Resume playing
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_resume(void)
{
    int status;
    tBSA_AV_START start_param;

    /* start Av stream */
    BSA_AvStartInit(&start_param);
    start_param.media_feeding = app_av_cb.media_feeding;
    start_param.feeding_mode = app_av_cb.uipc_cfg.is_blocking? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
    start_param.latency = app_av_cb.uipc_cfg.period/1000; /* convert us to ms, synchronous feeding mode only*/

    /* Content Protection */
    start_param.cp_id = app_av_cb.cp_id;
    start_param.scmst_flag = app_av_cb.cp_scms_flag;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed: %d", status);
        return status;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_send_command
 **
 ** Description      Example of Send a RemoteControl command
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_command(int index, int command)
{
    int status;
    tBSA_AV_REM_CMD bsa_av_rc_cmd;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    /* Send remote control command */
    status = BSA_AvRemoteCmdInit(&bsa_av_rc_cmd);
    bsa_av_rc_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_rc_cmd.key_state = BSA_AV_STATE_PRESS;
    bsa_av_rc_cmd.rc_id = (tBSA_AV_RC)command;
    bsa_av_rc_cmd.label = app_av_cb.label++; /* Just used to distinguish commands */
    status = BSA_AvRemoteCmd(&bsa_av_rc_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvRemoteCmd failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_send_absolute_volume_vd_command
 **
 ** Description      Example of Send a Vendor Specific command
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_absolute_volume_vd_command(int index, int volume)
{
    int status;
    tBSA_AV_VEN_CMD bsa_av_vd_cmd;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    /* Send remote control command */
    status = BSA_AvVendorCmdInit(&bsa_av_vd_cmd);
    bsa_av_vd_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_vd_cmd.ctype = BSA_AV_CMD_CTRL;
    bsa_av_vd_cmd.data[0] = BSA_AV_RC_VD_SET_ABSOLUTE_VOLUME;
    bsa_av_vd_cmd.data[1] = 0; /* reserved & packet type */
    bsa_av_vd_cmd.data[2] = 0; /* length high*/
    bsa_av_vd_cmd.data[3] = 1; /* length low*/
    bsa_av_vd_cmd.data[4] = volume; /* Absolute volume */
    bsa_av_vd_cmd.length = 5;
    bsa_av_vd_cmd.label = app_av_cb.label++; /* Just used to distinguish commands */
    status = BSA_AvVendorCmd(&bsa_av_vd_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvVendorCmd failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_send_get_element_attributes_vd_command
 **
 ** Description      Example of Send a Vendor Specific command
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_get_element_attributes_vd_command(int index)
{
    int status;
    tBSA_AV_VEN_CMD bsa_av_vd_cmd;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    /* Send remote control command */
    status = BSA_AvVendorCmdInit(&bsa_av_vd_cmd);
    bsa_av_vd_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_vd_cmd.ctype = BSA_AV_CMD_STATUS;
    bsa_av_vd_cmd.data[0] = BSA_AV_RC_VD_GET_ELEMENT_ATTR;
    bsa_av_vd_cmd.data[1] = 0; /* reserved & packet type */
    bsa_av_vd_cmd.data[2] = 0; /* length high*/
    bsa_av_vd_cmd.data[3] = 0x11; /* length low*/

    /* data 4 ~ 11 are 0, which means "identifier 0x0 PLAYING" */

    bsa_av_vd_cmd.data[12] = 2; /* num of attributes */

    /* data 13 ~ 16 are 0x1, which means "attribute ID 1 : Title of media" */
    bsa_av_vd_cmd.data[16] = 0x1;

    /* data 17 ~ 20 are 0x7, which means "attribute ID 2 : Playing Time" */
    bsa_av_vd_cmd.data[20] = 0x7;

    bsa_av_vd_cmd.length = 21;
    bsa_av_vd_cmd.label = app_av_cb.label++; /* Just used to distinguish commands */
    status = BSA_AvVendorCmd(&bsa_av_vd_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvVendorCmd failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_send_get_element_attributes_vd_response
 **
 ** Description      This function demonstrates BSA_AvVendorRsp to respond to
 **                  Vendor Specific command request by manually contructing the
 **                  byte-stream per Bluetooth Specification.
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_get_element_attributes_vd_response(int index)
{
    /* data associated with BSA_AvVendorRsp */
    int status, attr_id;
    UINT8   xx, attr_count;
    tBSA_AV_VEN_RSP bsa_av_vd_rsp;
    char    *p_str;
    UINT16 charset_id, str_len, len;
    UINT8   *p_data;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    /* Send remote control command */
    status = BSA_AvVendorRspInit(&bsa_av_vd_rsp);
    bsa_av_vd_rsp.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_vd_rsp.ctype = 0xc; /*BTA_AV_RSP_IMPL_STBL - Stable*/
    bsa_av_vd_rsp.data[0] = BSA_AV_RC_VD_GET_ELEMENT_ATTR;
    bsa_av_vd_rsp.data[1] = 0; /* reserved & packet type */

    len = 0;
    str_len = 0;
    attr_count =  7;    /*  Fill all the media attributes or only those requested by the AVRCP controller */

    p_data = &bsa_av_vd_rsp.data[4];
    UINT8_TO_BE_STREAM(p_data, attr_count);
    len = 1 ;

    for (xx=0; xx<attr_count; xx++)
    {
        attr_id = xx + 1; /*  List of Media attributes start at index 1  - AVRCP Spec -
                                Appendix E: List of Media Attributes */
        switch(attr_id)
        {
        case AVRC_MEDIA_ATTR_ID_TITLE:
            p_str =  (char*) "Happy Nation";
            break;
        case AVRC_MEDIA_ATTR_ID_ARTIST:
            p_str = (char*) "Jonas, Jenny, Linn, Ulf";
            break;
        case AVRC_MEDIA_ATTR_ID_ALBUM:
            p_str = (char*) "Ace of Base";
            break;
        case AVRC_MEDIA_ATTR_ID_TRACK_NUM:
            p_str = (char*) "7";
            break;
        case AVRC_MEDIA_ATTR_ID_NUM_TRACKS:
            p_str = (char*) "15";
            break;
        case AVRC_MEDIA_ATTR_ID_GENRE:
            p_str = (char*) "Pop";
            break;
        case AVRC_MEDIA_ATTR_ID_PLAYING_TIME:
            p_str = (char*) "255000";
            break;
        default:
            p_str = NULL;
        }

        if (p_str)
        {
            str_len = strlen(p_str);
            charset_id = AVRC_CHARSET_ID_UTF8;

            UINT32_TO_BE_STREAM(p_data, attr_id);
            UINT16_TO_BE_STREAM(p_data, charset_id);
            UINT16_TO_BE_STREAM(p_data, str_len);
            ARRAY_TO_BE_STREAM(p_data, p_str, str_len);

            len = len + 4 + 2 + 2 + str_len;
        }
    }

    /*  reposition and update with length of GetElementAttribute response block */
    p_data = &bsa_av_vd_rsp.data[2];
    UINT16_TO_BE_STREAM(p_data, len);
    /* bsa_av_vd_rsp.data[2] - length high*/
    /* bsa_av_vd_rsp.data[3] - length low*/

    bsa_av_vd_rsp.length = 4 + len; /* Add the length of first four bytes to payload length */

    bsa_av_vd_rsp.label = app_av_cb.label++; /* Just used to distinguish commands */
    status = BSA_AvVendorRsp(&bsa_av_vd_rsp);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvVendorRsp failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_send_get_element_attributes_meta_response
 **
 ** Description      Example of Send response to get element attributes meta command request
 **                  BSA_AvMetaRsp currently supports following requests
 **                  GetElementAttributes - BSA_AVRC_PDU_GET_ELEMENT_ATTR and
 **                  GetPlayStatus - BSA_AVRC_PDU_GET_PLAY_STATUS
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_get_element_attributes_meta_response(int index)
{
    APP_INFO0("app_av_rc_send_get_element_attributes_meta_response");
    /* data associated with BSA_AvMetaRsp */
    int status;
    UINT8   xx, attr_count;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;

    tBSA_AV_ATTR_ENTRY *p_attr = NULL;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    if(p_bsa_av_cb->cur_play >= BSA_AV_NUMSONGS)
    {
        APP_ERROR0("song index out of bounds");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_GET_ELEMENT_ATTR;

    attr_count =  bsa_av_songs[p_bsa_av_cb->cur_play]->attr_count;
    bsa_av_meta_rsp_cmd.param.get_elem_attrs.num_attrs = attr_count;

    p_attr = (tBSA_AV_ATTR_ENTRY *) &bsa_av_meta_rsp_cmd.param.get_elem_attrs.attrs;

    for (xx=0; xx<attr_count; xx++)
    {
        p_attr[xx].name.charset_id = bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.charset_id;
        p_attr[xx].name.str_len = bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.str_len;
        p_attr[xx].attr_id = bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].attr_id;

        if(bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.str_len != 0 && bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.p_str)
        {
            strncpy((char*) p_attr[xx].str, (char*) bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.p_str, BSA_AV_META_INFO_LEN_MAX - 1);
            APP_INFO1("Attr ID %d, string name %s", bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].attr_id, p_attr[xx].str);
        }
    }

    bsa_av_meta_rsp_cmd.label = app_av_cb.label;

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_send_play_status_meta_response
 **
 ** Description      Example of Send response to get play status command request
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_send_play_status_meta_response(int index)
{
    APP_INFO0("app_av_rc_send_play_status_meta_response");
    /* data associated with BSA_AvMetaRsp */
    int status;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_GET_PLAY_STATUS;
    bsa_av_meta_rsp_cmd.label = app_av_cb.label;
    bsa_av_meta_rsp_cmd.param.get_play_status.song_len = (UINT32) 255000;    /*  Sample lenth of song 4' 15 seconds */
    bsa_av_meta_rsp_cmd.param.get_play_status.song_pos = (UINT32) 120000;    /*  Sample current position 2' 00 seconds */

    if(app_av_cb.play_state == APP_AV_PLAY_STARTED)
    bsa_av_meta_rsp_cmd.param.get_play_status.play_status = BSA_AVRC_PLAYSTATE_PLAYING ;
    else if(app_av_cb.play_state == APP_AV_PLAY_PAUSED)
        bsa_av_meta_rsp_cmd.param.get_play_status.play_status = BSA_AVRC_PLAYSTATE_PAUSED;
    else
        bsa_av_meta_rsp_cmd.param.get_play_status.play_status = BSA_AVRC_PLAYSTATE_STOPPED;

    APP_INFO1("Play status = %d", bsa_av_meta_rsp_cmd.param.get_play_status.play_status);

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_set_addr_player_meta_response
 **
 ** Description      Example of set addressed player response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_set_addr_player_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    /* data associated with BSA_AvMetaRsp */
    int status;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;
    UINT8       xx;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    if(pMetaMsg == NULL)
    {
        APP_ERROR0("Null tBSA_AV_META_MSG_MSG pointer");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRspInit failed: %d", status);
        return status;
    }

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_SET_ADDRESSED_PLAYER;
    bsa_av_meta_rsp_cmd.label = app_av_cb.label;
    bsa_av_meta_rsp_cmd.param.addr_player_status.opcode = pMetaMsg->opcode;
    bsa_av_meta_rsp_cmd.param.addr_player_status.status = AVRC_STS_NO_ERROR;


    /* make sure the player_id is valid */
    for (xx=0; xx<BSA_AV_NUM_MPLAYERS; xx++)
    {
        if (bsa_av_players[xx] && pMetaMsg->player_id == bsa_av_players[xx]->player_id)
        {
            APP_INFO1("SetAddressedPlayer succeeded ID %d", pMetaMsg->player_id);
            p_bsa_av_cb->meta_info.addr_player_id = pMetaMsg->player_id;
            break;
        }
    }

    if (xx == BSA_AV_NUM_MPLAYERS)
    {
        bsa_av_meta_rsp_cmd.param.addr_player_status.status = AVRC_STS_BAD_PLAYER_ID;
        APP_INFO1("SetAddressedPlayer failed bad ID %d", pMetaMsg->player_id);
    }

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_av_rc_set_browsed_player_meta_response
 **
 ** Description      Example of browsed addressed player response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_set_browsed_player_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    /* data associated with BSA_AvMetaRsp */
    int status;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;
    UINT8       xx;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    if(pMetaMsg == NULL)
    {
        APP_ERROR0("Null tBSA_AV_META_MSG_MSG pointer");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRspInit failed: %d", status);
        return status;
    }

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_SET_BROWSED_PLAYER;
    bsa_av_meta_rsp_cmd.label = app_av_cb.label;
    bsa_av_meta_rsp_cmd.param.browsed_player_status.opcode = pMetaMsg->opcode;
    bsa_av_meta_rsp_cmd.param.browsed_player_status.status = AVRC_STS_NO_ERROR;


    /* make sure the player_id is valid */
    for (xx=0; xx<BSA_AV_NUM_MPLAYERS; xx++)
    {
        if (bsa_av_players[xx] && pMetaMsg->player_id == bsa_av_players[xx]->player_id &&
            AVRC_PF_BROWSE_SUPPORTED(bsa_av_players[xx]->features))
        {
            APP_INFO1("SetBorwsedPlayer succeeded ID %d", pMetaMsg->player_id);
            p_bsa_av_cb->meta_info.browsed_player_id = pMetaMsg->player_id;

            bsa_av_meta_rsp_cmd.param.browsed_player_status.num_items = BSA_AV_NUMSONGS;
            bsa_av_meta_rsp_cmd.param.browsed_player_status.charset_id = AVRC_CHARSET_ID_UTF8;
            bsa_av_meta_rsp_cmd.param.browsed_player_status.folder_depth = 0;
            bsa_av_meta_rsp_cmd.param.browsed_player_status.folder_name_size = 14;
            strcpy((char *)bsa_av_meta_rsp_cmd.param.browsed_player_status.folder_name_str, "Player 1 files");

            break;
        }
    }

    if (xx == BSA_AV_NUM_MPLAYERS)
    {
        bsa_av_meta_rsp_cmd.param.browsed_player_status.status = AVRC_STS_BAD_PLAYER_ID;
        APP_INFO1("SetBrowsedPlayer failed bad ID %d", pMetaMsg->player_id);
    }

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_change_path_meta_response
 **
 ** Description      Example of change path response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_change_path_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    APP_INFO0("app_av_rc_change_path_meta_response");
    /* data associated with BSA_AvMetaRsp */
    int status;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    if(pMetaMsg == NULL)
    {
        APP_ERROR0("Null tBSA_AV_META_MSG_MSG pointer");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRspInit failed: %d", status);
        return status;
    }

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_CHANGE_PATH;
    bsa_av_meta_rsp_cmd.label = app_av_cb.label;
    bsa_av_meta_rsp_cmd.param.change_path.opcode = pMetaMsg->opcode;

    /* For this app, will respond that change path is not supported, we have just one level flat directory structure */
    bsa_av_meta_rsp_cmd.param.change_path.status = AVRC_STS_BAD_DIR;
    bsa_av_meta_rsp_cmd.param.change_path.num_items = 0;

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_av_rc_get_item_attr_meta_response
 **
 ** Description      Example of GetItemAttr reponse
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_get_item_attr_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    APP_INFO0("app_av_rc_get_item_attr_meta_response");

    /* data associated with BSA_AvMetaRsp */
    UINT8   xx, attr_count;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;
    int status;

    tBSA_AV_ATTR_ENTRY *p_attr = NULL;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_GET_ITEM_ATTRIBUTES;

    bsa_av_meta_rsp_cmd.param.get_item_attrs.opcode = pMetaMsg->param.get_item_attrs.opcode;

    attr_count =  bsa_av_songs[p_bsa_av_cb->cur_play]->attr_count;
    bsa_av_meta_rsp_cmd.param.get_elem_attrs.num_attrs = attr_count;

    p_attr = (tBSA_AV_ATTR_ENTRY *) &bsa_av_meta_rsp_cmd.param.get_elem_attrs.attrs;

    for (xx=0; xx<attr_count; xx++)
    {
        p_attr[xx].name.charset_id = bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.charset_id;
        p_attr[xx].name.str_len = bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.str_len;
        p_attr[xx].attr_id = bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].attr_id;

        if(bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.str_len != 0 && bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.p_str)
        {
            strncpy((char*) p_attr[xx].str, (char*) bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].name.p_str, BSA_AV_META_INFO_LEN_MAX - 1);
            APP_INFO1("Attr ID %d, string name %s", bsa_av_songs[p_bsa_av_cb->cur_play]->p_attr_list[xx].attr_id, p_attr[xx].str);
        }
    }


    bsa_av_meta_rsp_cmd.label = app_av_cb.label;

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_get_folder_items
 **
 ** Description      Example of get folder items response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_get_folder_items(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    APP_INFO0("app_av_rc_get_folder_items");

    int status;
    UINT8       xx;
    /* data associated with BSA_AvMetaRsp */
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    if(pMetaMsg == NULL)
    {
        APP_ERROR0("Null tBSA_AV_META_MSG_MSG pointer");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRspInit failed: %d", status);
        return status;
    }

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_GET_FOLDER_ITEMS;
    bsa_av_meta_rsp_cmd.label = app_av_cb.label;

    bsa_av_meta_rsp_cmd.param.get_items_status.scope = pMetaMsg->param.get_folder_items.scope;
    bsa_av_meta_rsp_cmd.param.get_items_status.opcode = pMetaMsg->opcode;

    if(bsa_av_meta_rsp_cmd.param.get_items_status.scope == AVRC_SCOPE_PLAYER_LIST)
    {
        APP_INFO0("AVRC_SCOPE_PLAYER_LIST");

        bsa_av_meta_rsp_cmd.param.get_items_status.num_items = 0;
        bsa_av_meta_rsp_cmd.param.get_items_status.status = AVRC_STS_NO_ERROR;

        APP_INFO1("GetFolderItems - list player range %d to %d", pMetaMsg->param.get_folder_items.start_item, pMetaMsg->param.get_folder_items.end_item);
        APP_INFO1("num of available players %d", BSA_AV_NUM_MPLAYERS);

        for (xx=pMetaMsg->param.get_folder_items.start_item; xx<BSA_AV_NUM_MPLAYERS && xx <=pMetaMsg->param.get_folder_items.end_item ; xx++)
        {
            if (bsa_av_players[xx])
            {
                bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].item_type = AVRC_ITEM_PLAYER;
                memcpy(&bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].u.player, bsa_av_players[xx], sizeof(tBSA_ITEM_PLAYER));
                strncpy((char *)bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].item_str, (char *)bsa_av_players[xx]->name.p_str, BSA_AV_ITEM_NAME_LEN_MAX - 1);
                bsa_av_meta_rsp_cmd.param.get_items_status.num_items++;
                APP_INFO1("Media player ID %d, name %s", bsa_av_players[xx]->player_id, bsa_av_players[xx]->name.p_str);
            }
        }
    }

    else if(bsa_av_meta_rsp_cmd.param.get_items_status.scope == AVRC_SCOPE_FILE_SYSTEM)
    {
        APP_INFO0("AVRC_SCOPE_FILE_SYSTEM");

        bsa_av_meta_rsp_cmd.param.get_items_status.status = AVRC_STS_NO_ERROR;
        bsa_av_meta_rsp_cmd.param.get_items_status.uid_counter = BSA_AV_NUMSONGS;

        for (xx=pMetaMsg->param.get_folder_items.start_item; xx<BSA_AV_NUMSONGS && xx <=pMetaMsg->param.get_folder_items.end_item ; xx++)
        {
            if (bsa_av_songs[xx])
            {
                bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].item_type = AVRC_ITEM_MEDIA;
                memcpy(&bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].u.media.uid, bsa_av_songs[xx]->uid, sizeof(tBSA_UID));
                bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].u.media.type = bsa_av_songs[xx]->type;
                bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].u.media.name.charset_id = bsa_av_songs[xx]->name.charset_id;
                bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].u.media.name.str_len = bsa_av_songs[xx]->name.str_len;
                bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].u.media.attr_count = 0;

                strncpy((char *)bsa_av_meta_rsp_cmd.param.get_items_status.item_list[xx].item_str, (char *)bsa_av_songs[xx]->name.p_str, BSA_AV_ITEM_NAME_LEN_MAX - 1);
                bsa_av_meta_rsp_cmd.param.get_items_status.num_items++;

                APP_INFO1("song name %s", bsa_av_songs[xx]->name.p_str);
            }
        }
    }
    else
    {
        APP_INFO1("GetFolderItems, scope %d not supported", bsa_av_meta_rsp_cmd.param.get_items_status.scope);
        bsa_av_meta_rsp_cmd.param.get_items_status.status = AVRC_STS_BAD_CMD;
    }

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_play_item_meta_response
 **
 ** Description      Example of play item response
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_play_item_meta_response(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    /* data associated with BSA_AvMetaRsp */
    int status;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;
    int xx=0;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    if(pMetaMsg == NULL)
    {
        APP_ERROR0("Null tBSA_AV_META_MSG_MSG pointer");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRspInit failed: %d", status);
        return status;
    }

    APPL_TRACE_DEBUG3("app_av_rc_play_item_meta_response scope:%d, uid[0]:0x%02d, uid[7]:0x%02",
        pMetaMsg->param.play_item.scope,pMetaMsg->param.play_item.uid[0], pMetaMsg->param.play_item.uid[7]);
    switch (pMetaMsg->param.play_item.scope)
    {
    case AVRC_SCOPE_FILE_SYSTEM:
        bsa_av_meta_rsp_cmd.param.play_item.status = BTA_AV_RSP_REJ;
        for (xx=0; xx<BSA_AV_NUMSONGS; xx++)
        {
            if(memcmp(bsa_av_songs[xx]->uid, pMetaMsg->param.play_item.uid, sizeof(tBSA_UID)) == 0)
            {
                p_bsa_av_cb->cur_play = xx;
                bsa_av_meta_rsp_cmd.param.play_item.status = BTA_AV_RSP_ACCEPT;
                app_av_rc_change_track();
                break;
            }
        }

        break;

    default:
        /* TODO AVRC_SCOPE_SEARCH, AVRC_SCOPE_NOW_PLAYING*/
        bsa_av_meta_rsp_cmd.param.play_item.status = BTA_AV_RSP_NOT_IMPL;
    }

    bsa_av_meta_rsp_cmd.param.play_item.pdu = BSA_AVRC_PDU_PLAY_ITEM;
    bsa_av_meta_rsp_cmd.param.play_item.opcode = pMetaMsg->opcode;
    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_PLAY_ITEM;
    bsa_av_meta_rsp_cmd.label = app_av_cb.label;

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_rc_register_notifications
 **
 ** Description      Example of register for notifications
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/

int app_av_rc_register_notifications(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    UINT16  evt_mask = 1, index_x;

    /* data associated with BSA_AvMetaRsp */
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;

    int status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRspInit failed: %d", status);
        return status;
    }


    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    if(pMetaMsg == NULL)
    {
        APP_ERROR0("Null tBSA_AV_META_MSG_MSG pointer");
        return -1;
    }

    index_x = pMetaMsg->param.reg_notif.event_id - 1;
    evt_mask <<= index_x;

    bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[index].rc_handle;
    bsa_av_meta_rsp_cmd.label = app_av_cb.label;

    bsa_av_meta_rsp_cmd.param.notify_status.opcode = pMetaMsg->opcode;
    bsa_av_meta_rsp_cmd.param.notify_status.code = BTA_AV_RSP_INTERIM;

    /* Register event to the BSA AV control block  */
    p_bsa_av_cb->meta_info.registered_events.evt_mask |= evt_mask;
    p_bsa_av_cb->meta_info.registered_events.label[index_x] = pMetaMsg->label;

    return app_av_build_notification_response(pMetaMsg->param.reg_notif.event_id, &bsa_av_meta_rsp_cmd);
}

/*******************************************************************************
**
** Function         app_av_rc_complete_notification
**
** Description      Send event notification that was registered before with the
**                  given response code
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_complete_notification(UINT8 event_id)
{
    /* data associated with BSA_AvMetaRsp */
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;

    int status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRspInit failed: %d", status);
        return;
    }

    /* Send remote control response */
    tBSA_AV_EVT_MASK evt_mask = 1;
    UINT8 rc_label;


    /* Check if it is one of the supported events  */
    evt_mask <<= (event_id - 1);

   if (p_bsa_av_cb->meta_info.registered_events.evt_mask & evt_mask)
        {
        APPL_TRACE_DEBUG2("Event(0x%x) was registered(0x%x) evt_mask:0x%x",
                           event_id,
                           p_bsa_av_cb->meta_info.registered_events.evt_mask );


        rc_label = p_bsa_av_cb->meta_info.registered_events.label[event_id-1];
            bsa_av_meta_rsp_cmd.param.notify_status.code = BTA_AV_RSP_CHANGED;
            bsa_av_meta_rsp_cmd.rc_handle = app_av_cb.connections[0].rc_handle;
            bsa_av_meta_rsp_cmd.label = app_av_cb.label;

            app_av_build_notification_response(event_id, &bsa_av_meta_rsp_cmd);

            /* De-register the event */
        p_bsa_av_cb->meta_info.registered_events.evt_mask &= ~evt_mask;
        p_bsa_av_cb->meta_info.registered_events.label[event_id-1] = 0xFF;

        APPL_TRACE_DEBUG3("EVENT NOTIF - Registered evt val after 0x%x clearing:0x%x label %d",
                           p_bsa_av_cb->meta_info.registered_events.evt_mask, evt_mask, rc_label );



        }
        else
        {
        APPL_TRACE_DEBUG1("Not registered event rcvd:0x%x", event_id);
        }
    }

/*******************************************************************************
 **
 ** Function         app_av_is_event_registered
 **
 ** Description      check if an event is registered
 **
 ** Returns          TRUE if registered, FALSE otherwise
 **
 *******************************************************************************/
BOOLEAN app_av_is_event_registered(UINT8 event_id)
{
    tBSA_AV_EVT_MASK evt_mask = 1;
    evt_mask <<= (event_id - 1);
    if (p_bsa_av_cb->meta_info.registered_events.evt_mask & evt_mask)
        return (TRUE);

    return (FALSE);
}

/*******************************************************************************
 **
 ** Function         app_av_build_notification_response
 **
 ** Description      Local function to send notification
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
static int app_av_build_notification_response(UINT8 event_id, tBSA_AV_META_RSP_CMD *p_bsa_av_meta_rsp_cmd)
{
    int status;
    tBSA_AV_META_ATTRIB *p_pas_attrib;
    UINT8       xx;
    UINT8   *p;

    p_bsa_av_meta_rsp_cmd->param.notify_status.status = AVRC_STS_NO_ERROR;

    p_bsa_av_meta_rsp_cmd->param.notify_status.event_id = event_id;
    p_bsa_av_meta_rsp_cmd->pdu = BSA_AVRC_PDU_REGISTER_NOTIFICATION;

    switch(event_id)
    {
    case AVRC_EVT_PLAY_STATUS_CHANGE:   /* 0x01 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.play_status = p_bsa_av_cb->meta_info.play_status.play_status;
        break;

    case AVRC_EVT_TRACK_CHANGE:         /* 0x02 */
        p = p_bsa_av_meta_rsp_cmd->param.notify_status.param.track;
        APPL_TRACE_DEBUG2("play_count %d cur_play %d",p_bsa_av_cb->play_count, p_bsa_av_cb->cur_play);
        /* Check whether folder and file is selected or not */
        if (p_bsa_av_cb->play_count == 0 )
        {
            /* If not, add track ID to be 0xFFFFFFFF-FFFFFFFF */
            UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FOLDER_ID); /* the id from the no now playing folder */
            UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FILE_ID);   /* the id from the no now playing file */
        }
        else
        {
            /* If selected, add track ID to be the current playing file ID  */
            UINT32_TO_BE_STREAM(p, AVRCP_NOW_PLAYING_FOLDER_ID); /* the id from the now playing folder */
            UINT32_TO_BE_STREAM(p, p_bsa_av_cb->cur_play+1);         /* the id from the current playing file */
        }
        break;

    case AVRC_EVT_TRACK_REACHED_END:    /* 0x03 */
    case AVRC_EVT_TRACK_REACHED_START:  /* 0x04 */
        break;

    case AVRC_EVT_PLAY_POS_CHANGED:     /* 0x05 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.play_pos = p_bsa_av_cb->meta_info.play_status.song_pos;
        break;

    case AVRC_EVT_BATTERY_STATUS_CHANGE:/* 0x06 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.battery_status = p_bsa_av_cb->meta_info.notif_info.bat_stat;
        break;

    case AVRC_EVT_SYSTEM_STATUS_CHANGE: /* 0x07 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.system_status = p_bsa_av_cb->meta_info.notif_info.sys_stat;
        break;

    case AVRC_EVT_APP_SETTING_CHANGE:   /* 0x08 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.num_attr = p_bsa_av_cb->meta_info.max_attrib_num;
        p_pas_attrib = &p_bsa_av_cb->meta_info.pas_info.equalizer;
        for (xx=0; xx<p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.num_attr; xx++)
        {
            p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.attr_id[xx] = p_pas_attrib->attrib_id;
            p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.attr_value[xx] = p_pas_attrib->curr_value;
            p_pas_attrib++;
        }
        break;

    case AVRC_EVT_NOW_PLAYING_CHANGE:   /* 0x09 no param */
    case AVRC_EVT_AVAL_PLAYERS_CHANGE:  /* 0x0a no param */
        break;

    case AVRC_EVT_ADDR_PLAYER_CHANGE:   /* 0x0b */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.addr_player.player_id = p_bsa_av_cb->meta_info.addr_player_id;
        /* UID counter is always 0 for Database Unaware Players */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.addr_player.uid_counter = p_bsa_av_cb->meta_info.cur_uid_counter;
        break;

    case AVRC_EVT_UIDS_CHANGE:          /* 0x0c */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.uid_counter = 0;
        break;

    case AVRC_EVT_VOLUME_CHANGE:        /* 0x0d */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.volume = 0;
        break;

    default:
        p_bsa_av_meta_rsp_cmd->param.notify_status.status = BTA_AV_RSP_NOT_IMPL;
    }



    status = BSA_AvMetaRsp(p_bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvMetaRsp failed: %d", status);
        return status;
    }

    APPL_TRACE_DEBUG1("app_av_build_notification_response %d", event_id)
    return 0;
}

/*******************************************************************************
**
** Function         app_av_rc_change_play_status
**
** Description      Change the play status. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_change_play_status(UINT8 new_play_status)
{
    if(new_play_status <= (UINT8)AVRC_PLAYSTATE_REV_SEEK)
    {
        UINT8   old_play_status = p_bsa_av_cb->meta_info.play_status.play_status;

        p_bsa_av_cb->meta_info.play_status.play_status = new_play_status;
        APPL_TRACE_DEBUG2("play_status = %d/%d", old_play_status, p_bsa_av_cb->meta_info.play_status.play_status );
        if (old_play_status != p_bsa_av_cb->meta_info.play_status.play_status)
        {
            app_av_rc_complete_notification(AVRC_EVT_PLAY_STATUS_CHANGE);
            app_av_rc_complete_notification(AVRC_EVT_PLAY_POS_CHANGED);
        }
    }
    else
    {
        APPL_TRACE_ERROR1("Illegal value of play_status = %d", new_play_status);
    }

}

/*******************************************************************************
**
** Function         app_av_rc_change_track
**
** Description      Change the track number. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_change_track()
{
    p_bsa_av_cb->meta_info.track_num++;
    app_av_rc_complete_notification(AVRC_EVT_TRACK_CHANGE);
}

/*******************************************************************************
**
** Function         app_av_rc_addressed_player_change
**
** Description      Change the addressed player. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_addressed_player_change(UINT16 addr_player)
{
    /* vaid addressed players in this app */
    if((addr_player != BSA_AV_PLAYER_ID_FILES) &&
       (addr_player != BSA_AV_PLAYER_ID_MPLAYER) &&
       (addr_player != BSA_AV_PLAYER_ID_FM))
    {
        APPL_TRACE_ERROR1("Illegal value of addressed player = %d", addr_player);
        return;
    }

    p_bsa_av_cb->meta_info.addr_player_id = addr_player;
    app_av_rc_complete_notification(AVRC_EVT_ADDR_PLAYER_CHANGE);
}

/*******************************************************************************
**
** Function         app_av_rc_settings_change
**
** Description      Change the player setting value. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void app_av_rc_settings_change(UINT8 setting, UINT8 value)
{
    /* player settings in this app */
    if(setting != AVRC_PLAYER_SETTING_EQUALIZER &&
       setting != AVRC_PLAYER_SETTING_REPEAT &&
       setting != AVRC_PLAYER_SETTING_SHUFFLE)
    {
        APPL_TRACE_ERROR1("Illegal value of setting = %d", setting);
        return;
    }

    /* player setting values (for this app) are only 0x1 or 0x2 */
    if(value != 0x1 && value != 0x2)
    {
        APPL_TRACE_ERROR1("Illegal setting value = %d", value);
        return;
    }

    switch(setting)
    {
    case 1:
        p_bsa_av_cb->meta_info.pas_info.equalizer.curr_value = value;
        break;
    case 2:
        p_bsa_av_cb->meta_info.pas_info.repeat.curr_value = value;
        break;
    case 3:
        p_bsa_av_cb->meta_info.pas_info.repeat.curr_value = value;
        break;
    }

    app_av_rc_complete_notification(AVRC_EVT_APP_SETTING_CHANGE);
}

/*******************************************************************************
**
** Function         app_av_rc_settings_change
**
** Description      Initialize app meta data vaues
**
** Returns          void
**
*******************************************************************************/

static void app_av_init_meta_data()
{
    if(p_bsa_av_cb == NULL)
        p_bsa_av_cb = malloc(sizeof(tBSA_AV_CB));

    p_bsa_av_cb->meta_info.pas_info.equalizer.attrib_id = 1;
    p_bsa_av_cb->meta_info.pas_info.equalizer.curr_value = 1;

    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.equalizer.attrib_str, "Equalizer");
    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.equalizer.attr_settings[0].val1, "Off");
    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.equalizer.attr_settings[1].val2, "On");

    p_bsa_av_cb->meta_info.pas_info.repeat.attrib_id = 2;
    p_bsa_av_cb->meta_info.pas_info.repeat.curr_value = 2;

    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.repeat.attrib_str, "Repeat");
    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.repeat.attr_settings[0].val1, "Repeat Off");
    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.repeat.attr_settings[1].val2, "Repeat Single");

    p_bsa_av_cb->meta_info.pas_info.shuffle.attrib_id = 3;
    p_bsa_av_cb->meta_info.pas_info.shuffle.curr_value = 2;

    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.shuffle.attrib_str, "Shuffle");
    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.shuffle.attr_settings[0].val1, "Shuffle Off");
    strcpy((char *)p_bsa_av_cb->meta_info.pas_info.shuffle.attr_settings[1].val2, "Shuffle All");

    p_bsa_av_cb->meta_info.max_attrib_num = 3;

    memcpy(&p_bsa_av_cb->meta_info.play_status, &playst, sizeof(tBSA_AV_META_PLAYSTAT));

    p_bsa_av_cb->meta_info.addr_player_id = 1;
    p_bsa_av_cb->meta_info.browsed_player_id = 1;
    p_bsa_av_cb->meta_info.cur_uid_counter = 0;
    p_bsa_av_cb->meta_info.registered_events.evt_mask = 0;
    p_bsa_av_cb->meta_info.track_num = 1;
    p_bsa_av_cb->cur_play = 0;
    p_bsa_av_cb->play_count = BSA_AV_NUMSONGS;

}

/*******************************************************************************
 **
 ** Function         app_av_rc_close
 **
 ** Description      Example to close RC handle
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_rc_close(int index)
{
    int status;
    tBSA_AV_CLOSE_RC bsa_av_close_rc;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    /* Close the RC channel */
    status = BSA_AvCloseRcInit(&bsa_av_close_rc);
    bsa_av_close_rc.rc_handle = app_av_cb.connections[index].rc_handle;
    status = BSA_AvCloseRc(&bsa_av_close_rc);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvCloseRc failed: %d", status);
        return status;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_init_playlist
 **
 ** Description
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_init_playlist(char *path)
{
    tAPP_WAV_FILE_FORMAT wav_format;
    int index;

    app_av_cb.soundfile_list_size = app_playlist_create(path, &app_av_cb.soundfile_list);
    if (app_av_cb.soundfile_list_size <= 0)
    {
        APP_ERROR1("Unable to init play list from:%s", path);
    }
    else
    {
        for (index = 0 ; index < app_av_cb.soundfile_list_size ; index++)
        {
            if (app_wav_format(app_av_cb.soundfile_list[index], &wav_format) < 0)
            {
                APP_ERROR1("Unable to extract WAV format from:%s", app_av_cb.soundfile_list[index]);
            }
            else
            {
                APP_INFO1("WAV format of: %s", app_av_cb.soundfile_list[index]);
                APP_INFO1("    codec(%s) ch(%d) bits(%d) rate(%d)", (wav_format.codec==BSA_AV_CODEC_PCM)?"PCM":"apt-X",
                        wav_format.nb_channels, wav_format.bits_per_sample, (int)wav_format.sample_rate);
            }
        }
    }
    return app_av_cb.soundfile_list_size;
}

/*******************************************************************************
 **
 ** Function         app_av_get_soundfile_path
 **
 ** Description      Get a sound file path from index
 **
 ** Returns          void
 **
 *******************************************************************************/
char *app_av_get_soundfile_path(int file_index)
{
    if ((file_index >= 0) && (file_index < app_av_cb.soundfile_list_size))
    {
        return app_av_cb.soundfile_list[file_index];
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_display_playlist
 **
 ** Description      Display the play list
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_av_display_playlist(void)
{
    int index;
    tAPP_WAV_FILE_FORMAT wav_format;

    APP_INFO0("Play list:");
    for (index = 0 ; index < app_av_cb.soundfile_list_size ; index++)
    {
        if (app_wav_format(app_av_cb.soundfile_list[index], &wav_format) < 0)
        {
            APP_ERROR1("Unable to extract WAV format from:%s", app_av_cb.soundfile_list[index]);
        }
        else
        {
            APP_INFO1("    %d : %s", index, app_av_cb.soundfile_list[index]);
            APP_INFO1("        codec(%s) ch(%d) bits(%d) rate(%d)", (wav_format.codec==BSA_AV_CODEC_PCM)?"PCM":"apt-X",
                    wav_format.nb_channels, wav_format.bits_per_sample, (int)wav_format.sample_rate);
        }
    }
    return app_av_cb.soundfile_list_size;
}

/*******************************************************************************
 **
 ** Function           app_av_check_feeding
 **
 ** Description        Function used to check if feeding match WAV file
 **
 ** Returns            Boolean
 **
 *******************************************************************************/
BOOLEAN app_av_check_feeding(const tAPP_WAV_FILE_FORMAT *p_format)
{
    if (app_av_cb.media_feeding.format == p_format->codec)
    {
        switch(p_format->codec)
        {
        case BSA_AV_CODEC_PCM:
            return (app_av_cb.media_feeding.cfg.pcm.num_channel == p_format->nb_channels) &&
                   (app_av_cb.media_feeding.cfg.pcm.sampling_freq == p_format->sample_rate) &&
                   (app_av_cb.media_feeding.cfg.pcm.bit_per_sample == p_format->bits_per_sample);
            break;
        case BSA_AV_CODEC_APTX:
            return (app_av_cb.media_feeding.cfg.aptx.sampling_freq == p_format->sample_rate) &&
                   (((app_av_cb.media_feeding.cfg.aptx.ch_mode == A2D_SBC_IE_CH_MD_MONO)?1:2) == p_format->nb_channels);
            break;
        default:
            APP_DEBUG0("Unsupported codec format");
            return FALSE;
        }
    }
    else
    {
        APP_ERROR0("Feeding does not match!!!");
        return FALSE;
    }
}

/*******************************************************************************
 **
 ** Function           app_uipc_tx_thread
 **
 ** Description        Thread in charge of feeding UIPC channel
 **
 ** Returns            nothing
 **
 *******************************************************************************/
static void app_uipc_tx_thread(void)
{
    int status;
    int nb_bytes = 0;
    int file_desc = -1;
    struct timespec newtime;
    time_t start, elapsed;
    tAPP_AV_DELAY delay;
    tAPP_WAV_FILE_FORMAT wav_format;
    UINT16 uipc_error = 0;

    APP_DEBUG0("Starting UIPC Tx thread");
    while(1)
    {
        APP_DEBUG0("Waiting for play start");
        /* Check if we should lock on the mutex */
        while (app_av_cb.play_state != APP_AV_PLAY_STARTED)
        {
            /* coverity[LOCK] False-positive: MUTEX used to wait for AV_START event */
            status = app_lock_mutex(&app_av_cb.app_stream_tx_mutex);
            if (status < 0)
            {
                APP_ERROR1("app_lock_mutex failed: %d", status);
                break;
            }
        }
        APP_DEBUG0("Play started");

        if (app_av_cb.play_type == APP_AV_PLAYTYPE_TONE)
        {
            APP_DEBUG0("Playing tone");
        }
        else if (app_av_cb.play_type == APP_AV_PLAYTYPE_AVK)
        {
            APP_DEBUG0("Playing from avk source");
        }
        else if (app_av_cb.play_type == APP_AV_PLAYTYPE_FILE)
        {
            APP_DEBUG1("Playing file %s", app_av_cb.file_name);

            file_desc = app_wav_open_file(app_av_cb.file_name, &wav_format);
            if (file_desc < 0)
            {
                APP_ERROR1("open(%s) failed(%d) -> Stop playing", app_av_cb.file_name, errno);
                if (app_av_stop() != 0)
                {
                    continue;
                }

                /* Wait for the stop event */
                APP_DEBUG0("Waiting for AV to stop");
                while (app_av_cb.play_state != APP_AV_PLAY_STOPPED) GKI_delay(10);
                continue;
            }

            /* Check if the file audio format matches the feeding */
            if (app_av_check_feeding(&wav_format) == FALSE)
            {
                APP_ERROR1("Feeding not compatible with file: %s", app_av_cb.file_name);
                close(file_desc);
                if (app_av_stop() != 0)
                {
                    continue;
                }

                /* Wait for the stop event */
                APP_DEBUG0("Waiting for AV to stop");
                while (app_av_cb.play_state != APP_AV_PLAY_STOPPED) GKI_delay(10);

                if (app_av_play_playlist(APP_AV_START) != 0)
                {
                    continue;
                }
                continue;
            }
            APP_DEBUG0("Playing file successfully");
        }
        else if (app_av_cb.play_type == APP_AV_PLAYTYPE_MIC)
        {
            APP_DEBUG0("Playing ALSA Input");
        }
        else if (app_av_cb.play_type == APP_AV_PLAYTYPE_TEST)
        {
            APP_DEBUG0("Playing test data");
        }


        /* Start the timer management */
        app_av_delay_start(&delay);
        elapsed = 0;
        start = delay.timestamp.tv_sec;
        do
        {
            switch (app_av_cb.play_type)
            {
            case APP_AV_PLAYTYPE_TONE:
                app_av_read_tone(app_av_cb.audio_buf, app_av_cb.uipc_cfg.length,
                        app_av_cb.sinus_type, &app_av_cb.sinus_index);
                nb_bytes = app_av_cb.uipc_cfg.length;
                break;
            case APP_AV_PLAYTYPE_TEST:
                app_av_read_test(app_av_cb.audio_buf, app_av_cb.sec_frame_size);
                nb_bytes = app_av_cb.sec_frame_size;
                break;
            case APP_AV_PLAYTYPE_FILE:
                nb_bytes = app_wav_read_data(file_desc, &wav_format, (char *)app_av_cb.audio_buf, app_av_cb.uipc_cfg.length);
                break;
#ifdef PCM_ALSA
            case APP_AV_PLAYTYPE_MIC:
                /* fill the buffer with samples from microphone */
                if (app_av_cb.alsa_capture_opened)
                {
                    nb_bytes = app_alsa_capture_read(app_av_cb.audio_buf, app_av_cb.uipc_cfg.length);
                    if (nb_bytes == 0)
                    {
                        /*
                         * let's simulate that 4 bytes (16bits stereo) have been
                         * to do not exit the while loop */
                        nb_bytes = 4;
                    }
                    else if (nb_bytes < 0)
                    {
                        APP_ERROR1("app_alsa_capture_read fail:%d", nb_bytes);
                    }
                }
                else
                {
                    APP_ERROR0("ALSA not opened");
                    nb_bytes = 0;
                }
                break;
#endif
            case APP_AV_PLAYTYPE_AVK:
                nb_bytes = 0;
                break;

            default:
                APP_ERROR0("Unsupported play_type");
                break;
            }

            /* If no bytes were read, just stop the stream */
            if (nb_bytes <= 0)
            {
                if (app_av_cb.play_type != APP_AV_PLAYTYPE_AVK)
                {
                APP_DEBUG0("No more samples -> stopping current");

                app_av_stop_current();
            }
            }
            else
            {
                /* Send the samples to the AV channel */
                if (UIPC_Send(app_av_cb.stream_uipc_channel, 0, (UINT8 *) app_av_cb.audio_buf, nb_bytes) == FALSE)
                {
                    if(UIPC_Ioctl(app_av_cb.stream_uipc_channel, UIPC_READ_ERROR, &uipc_error))
                    {
                        if (uipc_error == UIPC_ENOMEM)
                        {
                            APP_DEBUG0("UIPC buffer full! Pause playing");
                            app_av_pause();
                            GKI_delay(3000);
                            APP_DEBUG0("UIPC buffer full! Resume playing");
                            app_av_resume();
                        }
                        else
                        {
                            APP_ERROR0("UIPC_Send failed");
                        }
                    }
                }
#if (defined(BSA_AV_DUMP_TX_DATA) && (BSA_AV_DUMP_TX_DATA == TRUE))
                APP_DUMP("A2DP Data", (UINT8 *) app_av_cb.audio_buf, nb_bytes);
#endif
                /* If the UIPC is in non blocking mode */
                if (app_av_cb.uipc_cfg.is_blocking == FALSE)
                {
                    /* Sleep for the duration requested (between UIPC_Send) */
                    app_av_wait_delay(1, &delay, &app_av_cb.uipc_cfg);
                }

                /* print once per sec */
                clock_gettime(CLOCK_MONOTONIC, &newtime);
                if ((newtime.tv_sec - start) > elapsed)
                {
                    elapsed = newtime.tv_sec - start;
                    APP_DEBUG1("Streaming for %d secs", elapsed);
                }

                /* Check if stream paused */
                if (app_av_cb.play_state == APP_AV_PLAY_PAUSED)
                {
                    APP_DEBUG0("Pausing (wait mutex)");
                    status = app_lock_mutex(&app_av_cb.app_stream_tx_mutex);
                    if (status < 0)
                    {
                        APP_ERROR1("app_lock_mutex failed: %d", status);
                        break;
                    }

                    /* Re-start the timer management */
                    app_av_delay_start(&delay);
                    elapsed = 0;

                    APP_DEBUG0("Un-pausing");
                }
            }
        } while ((app_av_cb.play_state != APP_AV_PLAY_STOPPED) &&
                 (app_av_cb.play_state != APP_AV_PLAY_STOPPING));

        APP_DEBUG0("Streaming stopped");

        /* Close audio stream */
        if (app_av_cb.play_type == APP_AV_PLAYTYPE_FILE)
        {
            close(file_desc);
        }
#ifdef PCM_ALSA
        else if (app_av_cb.play_type == APP_AV_PLAYTYPE_MIC)
        {
            /* Close Microphone */
            app_av_cb.alsa_capture_opened = FALSE;
            app_alsa_capture_close();
        }
#endif

        /* Wait until stop has completed (could have happened before) */
        while (app_av_cb.play_state != APP_AV_PLAY_STOPPED) GKI_delay(10);

        /* Check if we are playing a full list */
        if (app_av_cb.play_list)
        {
            /* Then move to the next element in playlist */
            app_av_play_playlist(APP_AV_FORWARD);
        }
    }
    APP_DEBUG0("Exit");
    pthread_exit(NULL);
}

/*******************************************************************************
 **
 ** Function         app_av_register
 **
 ** Description      Register a new AV source point
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_register(void)
{
    tAPP_AV_CONNECTION *connection;
    tBSA_STATUS status;
    tBSA_AV_REGISTER register_param;

    connection = app_av_find_connection_by_status(FALSE, FALSE);

    if (connection == NULL)
    {
        APP_ERROR0("All available connections already registered");
        return -1;
    }

    /* register an audio AV source point */
    BSA_AvRegisterInit(&register_param);
    status = BSA_AvRegister(&register_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvRegister failed: %d", status);
        return -1;
    }
    connection->is_registered = TRUE;
    connection->handle = register_param.handle;

    if (app_av_cb.stream_uipc_channel == UIPC_CH_ID_BAD)
    {
        /* Save the UIPC channel */
        app_av_cb.stream_uipc_channel = register_param.uipc_channel;
        /* Open UIPC channel */
        if (UIPC_Open(app_av_cb.stream_uipc_channel, NULL) == FALSE)
        {
            APP_ERROR1("UIPC_Open failed: %d", status);
            return -1;
        }

        /* In this application we are not feeding the stream real time so we must be blocking */
        if (!UIPC_Ioctl(app_av_cb.stream_uipc_channel, UIPC_WRITE_BLOCK, NULL))
        {
            APP_ERROR0("Failed setting the UIPC non blocking");
        }
    }
    else
    {
        /* Make sure that the UIPC channel used is the same than previous register */
        if (register_param.uipc_channel != app_av_cb.stream_uipc_channel)
        {
            APP_ERROR1("UIPC channel differs between register %d != %d",
                register_param.uipc_channel, app_av_cb.stream_uipc_channel);
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_deregister
 **
 ** Description      DeRegister an AV source point
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_deregister(int index)
{
    tAPP_AV_CONNECTION *connection;
    tBSA_AV_DEREGISTER deregister_param;
    tBSA_STATUS status;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        APP_ERROR0("Connection index out of bounds");
        return -1;
    }

    connection = &app_av_cb.connections[index];
    if (!connection->is_registered)
    {
        APP_ERROR1("connection index %d not registered", index);
        return -1;
    }

    /* In case we were playing, this could trigger an AV stop so we must
     * disable the play list type to prevent confusing STOP before START
     * in list and complete STOP */
    app_av_cb.play_list = FALSE;

    /* Deregister AV */
    BSA_AvDeregisterInit(&deregister_param);
    deregister_param.handle = connection->handle;
    status = BSA_AvDeregister(&deregister_param);
    if(status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvDeregister failed status=%d", status);
    }

    connection->is_registered = FALSE;
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_init
 **
 ** Description      Init Manager application
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_init(BOOLEAN boot)
{
    tBSA_AV_ENABLE enable_param;
    int status;
    tBSA_STATUS bsastatus;

    /* Initialize the control structure */
    memset(&app_av_cb, 0, sizeof(app_av_cb));
    app_av_cb.stream_uipc_channel = UIPC_CH_ID_BAD;
    app_av_cb.tone_sample_freq = 48000;

    /* Init Content Protection */
    app_av_cb.cp_id = BSA_AV_CP_ID_NONE;
    app_av_cb.cp_scms_flag = 0x00;

    /* If boot param is TRUE => the application just start:  */
    /* Read XML config files and open BSA connection */
    if (boot)
    {
        /*
         * Create and init Mutex and Thread
         */
        status = app_init_mutex(&app_av_cb.app_stream_tx_mutex);
        if (status < 0)
        {
            return -1;
        }
        status = app_lock_mutex(&app_av_cb.app_stream_tx_mutex);
        if (status < 0)
        {
            return -1;
        }
        status = app_create_thread(app_uipc_tx_thread, 0, 0,
                    &app_av_cb.app_uipc_tx_thread_struct);
        if (status < 0)
        {
            APP_ERROR1("app_create_thread failed: %d", status);
            return -1;
        }
    }

    /* Default UIPC configuration blocking */
    app_av_cb.uipc_cfg.is_blocking = TRUE;

    /* Initialize and create a playlist from the local folder */
    app_init_playlist("./test_files/av");
    if (app_av_cb.soundfile_list_size > 0)
    {
        APP_INFO1("app_av_cb.soundfile_list_size:%d", app_av_cb.soundfile_list_size);
        /* copy the first file in the current file name */
        strncpy(app_av_cb.file_name, app_av_cb.soundfile_list[0], sizeof(app_av_cb.file_name)-1);
        app_av_cb.file_name[sizeof(app_av_cb.file_name)-1] = '\0';
    }

    /* Get hold of the AV resource */
    BSA_AvEnableInit(&enable_param);

    enable_param.p_cback = app_av_cback;

    /* Set apt-X capabilities */
    enable_param.aptx_caps = aptx_caps;

    /* Set SEC capabilities */
    enable_param.sec_caps = sec_caps;

    /* Set the Features mask needed */
    enable_param.features = APP_AV_FEAT_MASK;

    bsastatus = BSA_AvEnable(&enable_param);
    if (bsastatus != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvEnable failed: %d", bsastatus);
        return -1;
    }

#ifdef APP_AV_BCST_INCLUDED
    /* Init AV Broadcast */
    app_av_bcst_init();
#endif

    /* Initialize app meta data */
    app_av_init_meta_data();

    app_av_cb.play_state = APP_AV_PLAY_STOPPED;

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_end
 **
 ** Description      This function is used to close AV
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_av_end(void)
{
    int index;
    tBSA_AV_DISABLE disable_param;

    /* If AV Stream is started */
    if (app_av_cb.play_state == APP_AV_PLAY_STARTED)
    {
        /* Stop it */
        app_av_stop();

        /* Wait until stop has completed */
        while (app_av_cb.play_state != APP_AV_PLAY_STOPPED)
        {
            GKI_delay(10);
        }
    }

    /* Close UIPC channel */
    if (app_av_cb.stream_uipc_channel != UIPC_CH_ID_BAD)
    {
        APP_INFO0("Closing UIPC Channel");
        UIPC_Close(app_av_cb.stream_uipc_channel);
        app_av_cb.stream_uipc_channel = UIPC_CH_ID_BAD;
    }

    /* Deregister AV End Point(s) */
    for (index = 0; index < APP_AV_MAX_CONNECTIONS ; index++)
    {
        if (app_av_cb.connections[index].is_registered)
        {
            app_av_deregister(index);
        }
    }

#ifdef APP_AV_BCST_INCLUDED
    /* Exit AV Broadcast */
    app_av_bcst_end();
#endif

    /* Disable av */
    BSA_AvDisableInit(&disable_param);
    int iRet = BSA_AvDisable(&disable_param);

    if(p_bsa_av_cb != NULL)
    {
        free(p_bsa_av_cb);
        p_bsa_av_cb = NULL;
    }

    return iRet;

}

/*******************************************************************************
 **
 ** Function         app_av_is_padding_acceptable
 **
 ** Description      This function checks is the padding fits feeding requirement
 **
 ** Returns          boolean
 **
 *******************************************************************************/
BOOLEAN app_av_is_padding_acceptable(int length, tBSA_AV_MEDIA_FEEDINGS *p_media_feeding)
{
    int padding;

    switch (p_media_feeding->format)
    {
    case BSA_AV_CODEC_PCM:
         /* Padding can be either 1, 2 or 4 bytes */
         padding = p_media_feeding->cfg.pcm.bit_per_sample *
         p_media_feeding->cfg.pcm.num_channel / 8;
        break;

    case BSA_AV_CODEC_APTX:
        /* By default apt-X samples are 16 bits wide */
        padding = 2;
        if (p_media_feeding->cfg.aptx.ch_mode != BSA_AV_CHANNEL_MODE_MONO)
        {
            padding *= 2;
        }
        break;

    case BSA_AV_CODEC_SEC:
        /* By default SEC samples are 16 bits wide */
        padding = 2;
        if (p_media_feeding->cfg.sec.ch_mode != BSA_AV_CHANNEL_MODE_MONO)
        {
            padding *= 2;
        }
        break;

    default:
        APP_ERROR1("Unsupported feeding format code: %d", app_av_cb.media_feeding.format);
        return TRUE;
    }

    /* For Mono 8 bit_per_sample, the buffer size can be odd */
    if (padding == 1)
    {
        APP_DEBUG1("packet size:%d ok (8 bits aligned)", length);
        return TRUE;
    }
    /* For Stereo 8 bits or Mono 16 bits, the buffer size must be even */
    else if ((padding == 2) && ((length & 0x01) == 0))
    {
        APP_DEBUG1("packet size:%d is ok (16 bits aligned)", length);
        return TRUE;
    }
    /* For Stereo 16 bits the buffer size must be 32 bit aligned */
    else if ((padding == 4) && ((length & 0x03) == 0))
    {
        APP_DEBUG1("packet size:%d is ok (32 bits aligned)", length);
        return TRUE;
    }
    else
    {
        APP_DEBUG1("packet size:%d does not fit alignment", length);
    }
    return FALSE;
}

/*******************************************************************************
 **
 ** Function         app_av_compute_uipc_param
 **
 ** Description      This function computes the UIPC length/period
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_av_compute_uipc_param(tAPP_AV_UIPC *p_uipc_cfg, tBSA_AV_MEDIA_FEEDINGS *p_media_feeding)
{
    int bytes_per_sec;
    float tmp;
    float remainder;

    APP_DEBUG0("app_av_compute_uipc_param");

    if (p_uipc_cfg->is_blocking)
    {


        /* restore default value */

        p_uipc_cfg->length = APP_AV_MAX_AUDIO_BUF * 2;

        APP_DEBUG1("UIPC is in blocking mode, no need to compute UIPC params length=%d",
            p_uipc_cfg->length);
        return 0;
    }

    /* First compute the stream bandwidth expected by server */
    switch (p_media_feeding->format)
    {
    case BSA_AV_CODEC_PCM:
        bytes_per_sec = p_media_feeding->cfg.pcm.bit_per_sample / 8;
        bytes_per_sec *= p_media_feeding->cfg.pcm.num_channel;
        bytes_per_sec *= p_media_feeding->cfg.pcm.sampling_freq;
        break;
    case BSA_AV_CODEC_APTX:
        /* The number of bytes per second for 44.1kHz stereo is:
         * 44100 * 2(bytes per sample) * 2(num channels)/4(compression)
         * sampling_freq * 2 * (1|2) / 4
         */
        bytes_per_sec = 2 * p_media_feeding->cfg.aptx.sampling_freq;
        if (p_media_feeding->cfg.aptx.ch_mode != BSA_AV_CHANNEL_MODE_MONO)
        {
            bytes_per_sec *= 2;
        }
        bytes_per_sec = bytes_per_sec/4;
        break;
    case BSA_AV_CODEC_SEC:
        /* The number of bytes per second for 44.1kHz stereo is:
         * 44100 * 2(bytes per sample) * 2(num channels)/4(compression)
         * sampling_freq * 2 * (1|2) / 4
         */
        bytes_per_sec = 2 * p_media_feeding->cfg.sec.sampling_freq;
        if (p_media_feeding->cfg.aptx.ch_mode != BSA_AV_CHANNEL_MODE_MONO)
        {
            bytes_per_sec *= 2;
        }
        bytes_per_sec = bytes_per_sec/4;
        break;

    default:
        APP_ERROR1("Unsupported feeding format code: %d", p_media_feeding->format);
        bytes_per_sec = 0;
        break;
    }
    APP_DEBUG1("bytes_per_sec:%d", bytes_per_sec);


    APP_DEBUG1("User asked for a specific Period:%d", p_uipc_cfg->asked_period);
    /* Start with the requested period */
    p_uipc_cfg->period = p_uipc_cfg->asked_period;
    do
    {
        /* Compute the size of each packet */
        tmp = (float)bytes_per_sec * (float)p_uipc_cfg->period / 1000000.0;
        /* Check if it's a whole number */
        remainder = tmp - (int)tmp;
        if (remainder)
        {
            /* APP_DEBUG1("packet size:%f is not a whole number", tmp); */
        }
        else
        {
            /* The Length is a whole number, let's check padding */
            p_uipc_cfg->length = (int)tmp;
            if (app_av_is_padding_acceptable(p_uipc_cfg->length, p_media_feeding) == TRUE)
            {
                break;
            }
        }
        /* Try with a smaller period */
        p_uipc_cfg->period--;
    } while (p_uipc_cfg->period >= APP_AV_PERIOD_MIN);

    /* If the period is too small, let's try to increase it */
    if (p_uipc_cfg->period < APP_AV_PERIOD_MIN)
    {
        /* Start with the requested period */
        p_uipc_cfg->period = p_uipc_cfg->asked_period;
        do
        {
            /* Compute the size of each packet */
            tmp = (float)bytes_per_sec * (float)p_uipc_cfg->period / 1000000.0;
            /* Check if it's a whole number */
            remainder = tmp - (int)tmp;
            if (remainder)
            {
                /* APP_DEBUG1("packet size:%f is not a whole number", tmp); */
            }
            else
            {
                /* The Length is a whole number, let's check padding */
                p_uipc_cfg->length = (int)tmp;
                if (app_av_is_padding_acceptable(p_uipc_cfg->length, p_media_feeding) == TRUE)
                {
                    break;
                }
            }
            /* Try with a bigger period */
            p_uipc_cfg->period++;
        } while (p_uipc_cfg->period <= APP_AV_PERIOD_MAX);

        /* If the period is too big, let's use the requested one */
        if (p_uipc_cfg->period > APP_AV_PERIOD_MAX)
        {
            p_uipc_cfg->period = p_uipc_cfg->asked_period;
            /* Compute the size of each packet */
            p_uipc_cfg->length = bytes_per_sec * p_uipc_cfg->period / 1000000.0;
            APP_DEBUG0("Not able to find period matching alignment");
            APP_DEBUG1("Let's use period:%d length:%d", p_uipc_cfg->period, p_uipc_cfg->length);
            return 1;
        }
        else
        {
            APP_DEBUG0("Found period/length matching alignment");
            APP_DEBUG1("period:%d length:%d", p_uipc_cfg->period, p_uipc_cfg->length);
            return 0;
        }
    }
    else
    {
        APP_DEBUG0("Found period/length matching alignment");
        APP_DEBUG1("period:%d length:%d", p_uipc_cfg->period, p_uipc_cfg->length);
        return 0;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_uipc_reconfig
 **
 ** Description      This function is used to reconfigure the UIPC channel
 **
 ** Returns          void
 **
 *******************************************************************************/
static int app_av_uipc_reconfig(void)
{
    UINT32 uipc_mode;

    app_av_compute_uipc_param(&app_av_cb.uipc_cfg, &app_av_cb.media_feeding);

    if (app_av_cb.uipc_cfg.is_blocking)
    {
        uipc_mode = UIPC_WRITE_BLOCK;
    }
    else
    {
        uipc_mode = UIPC_WRITE_NONBLOCK;
    }
    /* Setting the mode will also flush teh buffer */
    if (!UIPC_Ioctl(app_av_cb.stream_uipc_channel, uipc_mode, NULL))
    {
        APP_ERROR0("Failed setting the UIPC (non) blocking");
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_ask_uipc_config
 **
 ** Description      This function is used to configure the UIPC channel
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_av_ask_uipc_config(void)
{
    int choice;
    tAPP_AV_UIPC uipc_cfg;
    UINT32 uipc_mode = UIPC_WRITE_BLOCK;

    /* user can not configure the UIPC while some operation are ongoing */
    if (app_av_cb.play_state != APP_AV_PLAY_STOPPED)
    {
        APP_INFO0("Could not perform this operation, please stop the stream first");
        return -1;
    }

    /* Check if the media feeding configuration has ever been received */
    if ((app_av_cb.media_feeding.cfg.pcm.bit_per_sample == 0) ||
        (app_av_cb.media_feeding.cfg.pcm.num_channel == 0) ||
        (app_av_cb.media_feeding.cfg.pcm.sampling_freq == 0))
    {
        APP_INFO0("AV not yet started => let's use default values as feeding example (PCM, 48kHz, 16bits, Stereo)");

        app_av_cb.media_feeding.format = BSA_AV_CODEC_PCM;
        app_av_cb.media_feeding.cfg.pcm.sampling_freq = 48000;
        app_av_cb.media_feeding.cfg.pcm.bit_per_sample = 16;
        app_av_cb.media_feeding.cfg.pcm.num_channel = 2;
    }

    /* Set the default mode : blocking */
    uipc_cfg.is_blocking = TRUE;
    uipc_cfg.asked_period = 0;
    uipc_cfg.period = 0;
    uipc_cfg.length = 0;

    APP_INFO0("UIPC blocking/non blocking mode:");
    APP_INFO0("    0 Blocking mode (default)");
    APP_INFO0("    1 Non blocking mode");
    choice = app_get_choice("");
    if (choice == 0)
    {
        uipc_cfg.is_blocking = TRUE;
        APP_INFO0("UIPC Blocking mode selected");
    }
    else if (choice == 1)
    {
        APP_INFO0("UIPC Non Blocking mode selected");
        uipc_cfg.is_blocking = FALSE;
        uipc_mode = UIPC_WRITE_NONBLOCK;
        choice = app_get_choice("Enter Period (in micro)");
        if (choice < 0)
        {
            APP_ERROR1("Bad period:%d", choice);
            return -1;
        }
        uipc_cfg.asked_period = choice;
    }
    else
    {
        APP_ERROR1("Bad UIPC mode:%d", choice);
        return -1;
    }

    app_av_compute_uipc_param(&uipc_cfg, &app_av_cb.media_feeding);

    if (!UIPC_Ioctl(app_av_cb.stream_uipc_channel, uipc_mode, NULL))
    {
        APP_ERROR0("Failed setting the UIPC (non) blocking");
        APP_DEBUG0("It will be configured during next AV stop/start");
    }

    /* Copy the computed parameters in UIPC configuration */
    memcpy(&app_av_cb.uipc_cfg, &uipc_cfg, sizeof(app_av_cb.uipc_cfg));

    return 0;

}

/*******************************************************************************
 **
 ** Function        APP_AV_TIMESPEC_ADD
 **
 ** Description     Add 2 timespec structures together
 **
 ** Returns         The summed timespec structure in the first structure
 **
 *******************************************************************************/
#define APP_AV_TIMESPEC_ADD(__t1, __t2)                                         \
    do {                                                                        \
        __t1.tv_sec += __t2.tv_sec;                                             \
        __t1.tv_nsec += __t2.tv_nsec;                                           \
        if (BCM_UNLIKELY(__t1.tv_nsec >= 1000000000L)) { /* Carry ? */          \
            __t1.tv_sec++;                                                      \
            __t1.tv_nsec -= 1000000000L;                                        \
        }                                                                       \
    } while (0)


/*******************************************************************************
 **
 ** Function         app_av_wait_delay
 **
 ** Description      This function wait for a compensated amount of time such
 **                  that the average interval between app_av_wait_delay is exactly
 **                  app_av_cb.uipc_cfg.period
 **
 ** Parameters:      count : number cycle of app_av_cb.uipc_cfg.period time to wait
 **                  p_delay: structure used by app_av_wait_delay to store timing
 **                  information
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_av_wait_delay(int count, tAPP_AV_DELAY *p_delay, tAPP_AV_UIPC *p_uipc_cfg)
{
    struct timespec pause;

    int err;

    /* Configure the tick timeout */
    pause.tv_sec = 0;
    pause.tv_nsec =  p_uipc_cfg->period * 1000;

    while (count--)
    {
        /* Add the tick to the current time to compute next wake up time */
        APP_AV_TIMESPEC_ADD(p_delay->timestamp, pause);
        do
        {
            err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(p_delay->timestamp), NULL);
        } while (err < 0 && errno == EINTR);



        /* This trace is useful to see the compensation mechanism */
#if 0
        {
            struct timespec after;
            /* Get the current time */
            clock_gettime(CLOCK_MONOTONIC, &after);
            APP_DEBUG1("app_av_wait_delay %d sec; %d ns", after.tv_sec, after.tv_nsec);
        }
#endif
    }
}

/*******************************************************************************
 **
 ** Function         app_av_delay_start
 **
 ** Description      This function starts the delay management
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_av_delay_start(tAPP_AV_DELAY *p_delay)
{
    /* Get the initial time and period */
    clock_gettime(CLOCK_MONOTONIC, &(p_delay->timestamp));
    p_delay->timeout = app_av_cb.uipc_cfg.period;
}

/*******************************************************************************
 **
 ** Function         app_av_test_sec_codec
 **
 ** Description      This function is used to test SEC codec
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_test_sec_codec(BOOLEAN use_prev_samp_freq)
{
    int status, choice;
    tBSA_AV_START start_param;

    if ((app_av_cb.play_state != APP_AV_PLAY_STOPPED) || app_av_cb.play_list)
    {
        APP_INFO0("Could not perform the play operation, please stop the stream first");
        return -1;
    }

    /* start AV stream */
    BSA_AvStartInit(&start_param);
    start_param.media_feeding.format = BSA_AV_CODEC_SEC;

    if( use_prev_samp_freq == TRUE)
    {
        choice = app_av_cb.test_sec_sampfreq_index;
    }
    else
    {
        APP_INFO0("Sampling Frequency:");
        APP_INFO0("    0 - 48kHz");
        APP_INFO0("    1 - 44.1kHz");
        APP_INFO0("    2 - 32kHz");
        choice = app_get_choice("");
        app_av_cb.test_sec_sampfreq_index = choice;
    }

    if(choice == 0)
    {
        start_param.media_feeding.cfg.sec.sampling_freq = 48000;
        app_av_cb.sec_frame_size = 116;
    }
    else if(choice == 1)
    {
        start_param.media_feeding.cfg.sec.sampling_freq = 44100;
        app_av_cb.sec_frame_size = 124;
    }
    else if(choice == 2)
    {
        start_param.media_feeding.cfg.sec.sampling_freq = 32000;
        app_av_cb.sec_frame_size = 176;
    }
    else
    {
        APP_ERROR1("Bad sampling frequency:%d", choice);
        return -1;
    }

    start_param.media_feeding.cfg.sec.ch_mode = 2;
    start_param.feeding_mode = app_av_cb.uipc_cfg.is_blocking? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
    start_param.latency = app_av_cb.uipc_cfg.period/1000; /* convert us to ms, synchronous feeding mode only*/

    APP_INFO1("app_av_test_sec_codec freq:%d, c_m:%d, f_m:%d,lat:%d",
            start_param.media_feeding.cfg.sec.sampling_freq,
            start_param.media_feeding.cfg.sec.ch_mode,
            start_param.feeding_mode,
            start_param.latency);

    app_av_cb.play_type = APP_AV_PLAYTYPE_TEST;
    app_av_cb.play_list = FALSE;

    status = BSA_AvStart(&start_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvStart failed:%d", status);
        return status;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_change_cp
 **
 ** Description      This function is used to Change the Content Protection
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_change_cp(void)
{
    int choice;

    APP_INFO1("The current Content Protection Id is=%s ScmsFlag=%d",
            app_av_get_current_cp_desc(), app_av_cb.cp_scms_flag);

    APP_INFO0("New Content Protection Id:");
    APP_INFO0("    0 - None");
    APP_INFO0("    1 - SCMS");
    choice = app_get_choice("");
    switch(choice)
    {
    case 0:
        app_av_cb.cp_id = BSA_AV_CP_ID_NONE;
        app_av_cb.cp_scms_flag = 0x00;
        break;

    case 1:
        choice = app_get_choice("SCMS Flag/Header 0-Copy Never,1-Copy Once, 2-Copy Free");
        if(choice < 0 || choice >2)
        {
            APP_ERROR1("Invalid Entry:%d", choice);
            return -1;
        }
        app_av_cb.cp_id = BSA_AV_CP_ID_SCMS;
        app_av_cb.cp_scms_flag = (UINT8)choice;
        break;

    default:
        APP_ERROR1("Unknown Content Protection Id=%d", choice);
        return -1;
        break;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_av_get_current_cp_desc
 **
 ** Description      Get the current Content Protection Description
 **
 ** Returns          0Content Protection Description
 **
 *******************************************************************************/
char *app_av_get_current_cp_desc(void)
{
    switch (app_av_cb.cp_id)
    {
    case BSA_AV_CP_ID_NONE:
        return "NONE";
    case BSA_AV_CP_ID_SCMS:
        return "SCMS";
    default:
        return "Unknown";
    }
}


/*******************************************************************************
 **
 ** Function         app_av_set_busy_level
 **
 ** Description      Change busy level
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_av_set_busy_level(UINT8 level)
{
    int status;
    tBSA_AV_BUSY_LEVEL busy_level;

    /* start AV stream */
    BSA_AvBusyLevelInit(&busy_level);
    busy_level.level = level;
    APP_INFO1("app_av_set_busy_level level(0-5):%d", level);
    if(level>5)
    {
        APP_ERROR1("Wrong value:%d", level);
        return -1;
    }

    status = BSA_AvBusyLevel(&busy_level);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_AvBusyLevel failed:%d", status);
        return status;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function         app_av_get_notification_string
 **
 ** Description      get notification string
 **
 ** Returns          string
 **
 *******************************************************************************/
char *app_av_get_notification_string(UINT8 command)
{
    switch(command)
    {
    case AVRC_EVT_PLAY_STATUS_CHANGE:
        return ("AVRC_EVT_PLAY_STATUS_CHANGE");
    case AVRC_EVT_TRACK_CHANGE:
        return ("AVRC_EVT_TRACK_CHANGE");
    case AVRC_EVT_TRACK_REACHED_END:
        return ("AVRC_EVT_TRACK_REACHED_END");
    case AVRC_EVT_TRACK_REACHED_START:
        return ("AVRC_EVT_TRACK_REACHED_START");
    case AVRC_EVT_PLAY_POS_CHANGED:
        return ("AVRC_EVT_PLAY_POS_CHANGED");
    case AVRC_EVT_BATTERY_STATUS_CHANGE:
        return ("AVRC_EVT_BATTERY_STATUS_CHANGE");
    case AVRC_EVT_SYSTEM_STATUS_CHANGE:
        return ("AVRC_EVT_SYSTEM_STATUS_CHANGE");
    case AVRC_EVT_APP_SETTING_CHANGE:
        return ("AVRC_EVT_APP_SETTING_CHANGE");
    case AVRC_EVT_NOW_PLAYING_CHANGE:
        return ("AVRC_EVT_NOW_PLAYING_CHANGE");
    case AVRC_EVT_AVAL_PLAYERS_CHANGE:
        return ("AVRC_EVT_AVAL_PLAYERS_CHANGE");
    case AVRC_EVT_ADDR_PLAYER_CHANGE:
        return ("AVRC_EVT_ADDR_PLAYER_CHANGE");
    case AVRC_EVT_UIDS_CHANGE:
        return ("AVRC_EVT_UIDS_CHANGE");
    case AVRC_EVT_VOLUME_CHANGE:
        return ("AVRC_EVT_VOLUME_CHANGE");
    }

    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_av_set_cback
 **
 ** Description      Set application callback for third party app
 **
 **
 *******************************************************************************/
void app_av_set_cback(tBSA_AV_CBACK pcb)
{
    /* register callback */
    app_av_cb.p_Callback = pcb;
}

/*******************************************************************************
 **
 ** Function         app_av_get_play_state
 **
 ** Description      Get the current play state
 **
 **
 *******************************************************************************/
UINT8 app_av_get_play_state()
{
     return app_av_cb.play_state;
}


UINT8 app_av_get_play_type()
{
    return app_av_cb.play_type;
}
