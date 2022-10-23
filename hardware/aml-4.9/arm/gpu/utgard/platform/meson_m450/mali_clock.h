/*
 * ../vendor/amlogic/common/gpu/utgard/platform/meson_m450/mali_clock.h
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

#ifndef _MALI_CLOCK_H_
#define _MALI_CLOCK_H_

typedef int (*critical_t)(size_t param);
int mali_clock_critical(critical_t critical, size_t param);

int mali_clock_init(mali_plat_info_t*);
int mali_clock_set(unsigned int index);
void disable_clock(void);
void enable_clock(void);
u32 get_mali_freq(u32 idx);
void set_str_src(u32 data);
#endif /* _MALI_CLOCK_H_ */
