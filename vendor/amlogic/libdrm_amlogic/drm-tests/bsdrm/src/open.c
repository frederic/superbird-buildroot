/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

// Suppresses warnings for our usage of asprintf with one of the parameters.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
void bs_open_enumerate(const char *format, unsigned start, unsigned end,
		       bs_open_enumerate_func body, void *user)
{
	assert(end >= start);
	for (unsigned dev_index = start; dev_index < end; dev_index++) {
		char *file_path = NULL;
		int ret = asprintf(&file_path, format, dev_index);
		if (ret == -1)
			continue;
		assert(file_path);

		int fd = open(file_path, O_RDWR);
		free(file_path);
		if (fd < 0)
			continue;

		bool end = body(user, fd);
		close(fd);

		if (end)
			return;
	}
}
#pragma GCC diagnostic pop

struct bs_open_filtered_user {
	bs_open_filter_func filter;
	int fd;
};

static bool bs_open_filtered_body(void *user, int fd)
{
	struct bs_open_filtered_user *data = (struct bs_open_filtered_user *)user;
	if (data->filter(fd)) {
		data->fd = dup(fd);
		return true;
	}

	return false;
}

int bs_open_filtered(const char *format, unsigned start, unsigned end, bs_open_filter_func filter)
{
	struct bs_open_filtered_user data = { filter, -1 };
	bs_open_enumerate(format, start, end, bs_open_filtered_body, &data);
	return data.fd;
}

struct bs_open_ranked_user {
	bs_open_rank_func rank;
	uint32_t rank_index;
	int fd;
};

static bool bs_open_ranked_body(void *user, int fd)
{
	struct bs_open_ranked_user *data = (struct bs_open_ranked_user *)user;
	uint32_t rank_index = data->rank(fd);

	if (data->rank_index > rank_index) {
		data->rank_index = rank_index;
		if (data->fd >= 0)
			close(data->fd);
		data->fd = dup(fd);
	}

	return rank_index == 0;
}

int bs_open_ranked(const char *format, unsigned start, unsigned end, bs_open_rank_func rank)
{
	struct bs_open_ranked_user data = { rank, UINT32_MAX, -1 };
	bs_open_enumerate(format, start, end, bs_open_ranked_body, &data);
	return data.fd;
}
