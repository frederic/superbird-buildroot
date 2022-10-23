/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

void bs_debug_print(const char *prefix, const char *func, const char *file, int line,
		    const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "%s:%s():%s:%d:", prefix, func, basename(file), line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

int64_t bs_debug_gettime_ns()
{
	struct timespec t;
	int ret = clock_gettime(CLOCK_MONOTONIC, &t);
	if (ret)
		return -1;
	const int64_t billion = 1000000000;
	return (int64_t)t.tv_nsec + (int64_t)t.tv_sec * billion;
}
