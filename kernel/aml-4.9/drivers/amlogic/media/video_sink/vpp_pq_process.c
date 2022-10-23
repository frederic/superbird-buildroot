/*
 * drivers/amlogic/media/video_sink/vpp_pq_process.c
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

#include "vpp_pq.h"
#include <linux/types.h>
#include <linux/amlogic/media/amvecm/amvecm.h>

/*BLUE_SCENE             default 0*/
/*GREEN_SCENE            default 0*/
/*SKIN_TONE_SCENE        default 0*/
/*PEAKING_SCENE          default 100*/
/*SATURATION_SCENE       default 30*/
/*DYNAMIC_CONTRAST_SCENE default 0*/
/*NOISE_SCENE            default 0*/

/*
 * vpp_pq
 * 0 faceskin
 * 1 bluesky
 * 2 flowers
 * 3 foods
 * 4 breads
 * 5 fruits
 * 6 meats
 * 7 document
 * 8 ocean
 * 9 pattern
 * 10 group
 * 11 animals
 * 12 grass
 * 13 iceskate
 * 14 leaves
 * 15 racetrack
 * 16 fireworks
 * 17 waterfall
 * 18 beach
 * 19 waterside
 * 20 snows
 * 21 nightscop
 * 22 sunset
 * 23 default setting
 */

int vpp_pq_data[AI_SCENES_MAX][SCENES_VALUE] = {
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{60, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 70, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 70, 0, 0, 0, 0, 0},
	{0, 0, 0, 60, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 70, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 70, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 80, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 80, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0},
	{0, 0, 0, 100, 30, 0, 0, 0, 0, 0}
};

void vf_pq_process(struct vframe_s *vf,
		   struct ai_scenes_pq *vpp_scenes,
		   int *pq_debug)
{
	int pq_value;
	int i = 0;

	pq_value = vf->nn_value[0].maxclass;
	if (pq_debug[0] != -1)
		pq_value = pq_debug[0];

	if (vf->nn_value[0].maxprob == 0 && pq_debug[0] == -1)
		return;

	if (pq_debug[1])
		pq_value = 23;

	if (pq_debug[2])
		pr_info("top5:%d,%d; %d,%d; %d,%d; %d,%d; %d,%d;\n",
			vf->nn_value[0].maxclass, vf->nn_value[0].maxprob,
			vf->nn_value[1].maxclass, vf->nn_value[1].maxprob,
			vf->nn_value[2].maxclass, vf->nn_value[2].maxprob,
			vf->nn_value[3].maxclass, vf->nn_value[3].maxprob,
			vf->nn_value[4].maxclass, vf->nn_value[4].maxprob);
	while (i < SCENE_MAX) {
		vpp_scenes[pq_value].pq_scenes = pq_value;
		detected_scenes[i].func(vpp_scenes[pq_value].pq_values[i], 1);
		i++;
	}
}

