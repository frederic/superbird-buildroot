/*
 * ../vendor/amlogic/common/gpu/utgard/platform/meson_m400/mali_fix.h
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

#ifndef MALI_FIX_H
#define MALI_FIX_H

#define MMU_INT_NONE 0
#define MMU_INT_HIT  1
#define MMU_INT_TOP  2
#define MMU_INT_BOT  3

extern void malifix_init(void);
extern void malifix_exit(void);
extern void malifix_set_mmu_int_process_state(int, int);
extern int  malifix_get_mmu_int_process_state(int);
extern int mali_meson_is_revb(void);
#endif /* MALI_FIX_H */
