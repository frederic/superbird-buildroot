/*
 * sound/soc/amlogic/auge/audio_utils.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __AML_AUDIO_UTILS_H__
#define __AML_AUDIO_UTILS_H__

#include <sound/soc.h>

enum {
	EARCTX_SPDIF_TO_HDMIRX = 0,
	SPDIFA_TO_HDMIRX = 1,
	SPDIFB_TO_HDMIRX = 2,
	HDMIRX_SPDIF_TO_HDMIRX = 3,
};

int snd_card_add_kcontrols(struct snd_soc_card *card);

void audio_locker_set(int enable);

int audio_locker_get(void);

void fratv_enable(bool enable);

void fratv_src_select(bool src);

void fratv_LR_swap(bool swap);

void cec_arc_enable(int src, bool enable);

void tm2_arc_source_select(int src);

#endif
