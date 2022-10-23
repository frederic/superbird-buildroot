/*****************************************************************************
 **
 **  Name:           app_av_file_info.h
 **
 **  Description:    Information for AVRCP PTS test
 **
 **  Copyright (c) 2016, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/******************************************/
/*  Media player information              */
/******************************************/

/* Sample media players for peer browsing */
#define APP_AV_PLAYER_ID_INVALID           0
#define APP_AV_PLAYER_ID_FILES             1
#define APP_AV_PLAYER_ID_MPLAYER           2
#define APP_AV_PLAYER_ID_FM                3

#define APP_AV_NUM_MPLAYERS                3

tBSA_ITEM_PLAYER app_av_player1_file =
{
    /* A unique identifier for this media player.*/
    APP_AV_PLAYER_ID_FILES,
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

tBSA_ITEM_PLAYER app_av_player2_media_player =
{
    /* A unique identifier for this media player.*/
    APP_AV_PLAYER_ID_MPLAYER,
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

tBSA_ITEM_PLAYER app_av_player3_fm =
{
    /* A unique identifier for this media player.*/
    APP_AV_PLAYER_ID_FM,
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
tBSA_ITEM_PLAYER * app_av_players [] =
{
    /* player1 - when using files (wave, SBC, mp3) in btui_app */
    &app_av_player1_file,
    /* player2 - when using windows media player */
    &app_av_player2_media_player,
    /* player3 - when using FM Radio */
    &app_av_player3_fm
};




/******************************************/
/*  AV Folder information                 */
/******************************************/

tBSA_ITEM_FOLDER app_av_folder1_1 =
{
    .uid = { 0x14 },
    AVRC_FOLDER_TYPE_MIXED,
    TRUE,
    {   /* The folder name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        10,
        (UINT8 *)"folder 1_1"
    }
};

tBSA_ITEM_FOLDER app_av_folder1_2 =
{
    .uid = { 0x15 },
    AVRC_FOLDER_TYPE_MIXED,
    TRUE,
    {   /* The folder name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        10,
        (UINT8 *)"folder 1_2"
    }
};

tBSA_ITEM_FOLDER app_av_folder1_3 =
{
    .uid = { 0x16 },
    AVRC_FOLDER_TYPE_MIXED,
    TRUE,
    {   /* The folder name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        10,
        (UINT8 *)"folder 1_3"
    }
};

/* do not use const - the player state may be changed by platform code */
tBSA_ITEM_FOLDER * app_av_folders1 [] =
{
    &app_av_folder1_1,
    &app_av_folder1_2,
    &app_av_folder1_3,
};


tBSA_ITEM_FOLDER app_av_folder2_1 =
{
    .uid = { 0x24 },
    AVRC_FOLDER_TYPE_MIXED,
    TRUE,
    {   /* The folder name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        10,
        (UINT8 *)"folder 2_1"
    }
};

tBSA_ITEM_FOLDER app_av_folder2_2 =
{
    .uid = { 0x25 },
    AVRC_FOLDER_TYPE_MIXED,
    TRUE,
    {   /* The folder name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        10,
        (UINT8 *)"folder 2_2"
    }
};

tBSA_ITEM_FOLDER app_av_folder2_3 =
{
    .uid = { 0x26 },
    AVRC_FOLDER_TYPE_MIXED,
    TRUE,
    {   /* The folder name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        10,
        (UINT8 *)"folder 2_3"
    }
};

/* do not use const - the player state may be changed by platform code */
tBSA_ITEM_FOLDER * app_av_folders2 [] =
{
    &app_av_folder2_1,
    &app_av_folder2_2,
    &app_av_folder2_3,
};


#define APP_AV_NUMFOLDERS_L1                3
#define APP_AV_NUMFOLDERS_L2                3



/******************************************/
/*  AV Media information                  */
/******************************************/

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
    .uid = { 0x01 },
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
    .uid = { 0x03 },
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

#define APP_AV_NUMSONGS                3

tBSA_APP_ITEM_MEDIA * app_av_songs[] =
{
    &song1,
    &song2,
    &song3
};



/* UID is composed of FOLDER ID and FILE ID */
#define AVRCP_NOW_PLAYING_FOLDER_ID_FIRST_BYTE  0x02/* First byte of NOW_PLAYING folder UID 0x02 */
#define AVRCP_NOW_PLAYING_FOLDER_ID     0x02000000  /* NOW_PLAYING folder UID */
#define AVRCP_PLAY_LISTS_FOLDER_ID      0x00000000  /* PLAY_LISTS folder UID */
#define AVRCP_NOW_PLAYING_FILE_ID       0x00000000  /* First now playing file UID */
#define AVRCP_NO_NOW_PLAYING_FOLDER_ID  0xFFFFFFFF  /* No folder is selected */
#define AVRCP_NO_NOW_PLAYING_FILE_ID    0xFFFFFFFF  /* No file is selected */


#define APP_AV_CHANGE_FOLDER_MAX  3
#define APP_AV_CHANGE_FOLDER_MIN  0
INT8 app_av_cur_folder_position = 3;  /* this value is only for PTS test */
INT8 app_av_cur_folder_empty = 0;  /* this value is only for PTS test */
