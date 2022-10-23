/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "wake_lock.h"

#define TAG "aml_musicBox"
#define WAKE_LOCK "aml_musicBox"

#define debug(fmt, args...) \
    printf(TAG "[%s] " fmt "\n", __func__, ##args)

enum {
	ACQUIRE_PARTIAL_WAKE_LOCK = 0,
	RELEASE_WAKE_LOCK,
	OUR_FD_COUNT
};

const char * const OLD_PATHS[] = {
	"/sys/android_power/acquire_partial_wake_lock",
	"/sys/android_power/release_wake_lock",
};

const char * const NEW_PATHS[] = {
	"/sys/power/wake_lock",
	"/sys/power/wake_unlock",
};

//XXX static pthread_once_t g_initialized = THREAD_ONCE_INIT;
static int g_initialized = 0;
static int g_fds[2];
static int g_error = -1;

static int
open_file_descriptors(const char * const paths[])
{
	for (int i = 0 ; i < 2; i++) {
		int fd = open(paths[i], O_RDWR | O_CLOEXEC);
		if (fd < 0) {
			g_error = -errno;
			debug("fatal error opening \"%s\": %s", paths[i],
				  strerror(errno));
			return -1;
		}
		g_fds[i] = fd;
	}

	g_error = 0;
	return 0;
}

static inline void
initialize_fds(void)
{
	// XXX: should be this:
	//pthread_once(&g_initialized, open_file_descriptors);
	// XXX: not this:
	if (g_initialized == 0) {
		if (open_file_descriptors(NEW_PATHS) < 0)
			open_file_descriptors(OLD_PATHS);

		g_initialized = 1;
	}
}

int
acquire_wake_lock(void)
{
	char *id = WAKE_LOCK;
	initialize_fds();

	debug("id=%s", id);

	if (g_error) return g_error;

	int fd;
	size_t len;
	ssize_t ret;


	fd = g_fds[0];

	ret = write(fd, id, strlen(id));
	if (ret < 0) {
		return -errno;
	}

	return ret;
}

int
release_wake_lock(void)
{
	char *id = WAKE_LOCK;
	int fd;
	initialize_fds();

	debug("id=%s", id);

	if (g_error) return g_error;

	fd = g_fds[1];

	ssize_t len = write(fd, id, strlen(id));
	if (len < 0) {
		return -errno;
	}
	return len;
}

void wake_lock_exit(void)
{
	debug("enter");
	release_wake_lock();
	g_initialized = 0;
	memset(g_fds, 0, sizeof(g_fds));
}

