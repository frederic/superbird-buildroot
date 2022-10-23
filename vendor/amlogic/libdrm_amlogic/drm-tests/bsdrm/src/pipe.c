/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

bool bs_pipe_make(void *context, bs_make_pipe_piece *pieces, size_t piece_count, void *out_pipe,
		  size_t pipe_size)
{
	bool success = false;
	char *pipe_stack = calloc(pipe_size, piece_count + 1);
	for (size_t i = 0; i < piece_count;) {
		char *pipe_ptr = pipe_stack + i * pipe_size;
		if (pieces[i](context, pipe_ptr)) {
			i++;
			memcpy(pipe_ptr + pipe_size, pipe_ptr, pipe_size);
			continue;
		}

		if (i == 0)
			goto out;

		i--;
	}

	memcpy(out_pipe, pipe_stack + (piece_count - 1) * pipe_size, pipe_size);
	success = true;

out:
	free(pipe_stack);
	return success;
}
