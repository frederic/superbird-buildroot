/*****************************************************************************
**
**  Name:           hh.c
**
**  Description:    Bluetooth HH functions
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef __AV_H__
#define __AV_H__

typedef struct
{
    BD_ADDR bd_addr;
} tAV_PLAY_TONE_PARAM;


extern int av_play_tone_init_param(tAV_PLAY_TONE_PARAM *av_play_tone_param);
extern int av_play_tone(tAV_PLAY_TONE_PARAM *av_play_tone_param);

#endif

