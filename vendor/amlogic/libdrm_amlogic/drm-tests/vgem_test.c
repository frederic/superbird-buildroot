/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This is a test meant to exercise the VGEM DRM kernel module's PRIME
 * import/export functions. It will create a gem buffer object, mmap, write, and
 * then verify it. Then the test will repeat that with the same gem buffer, but
 * exported and then imported. Finally, a new gem buffer object is made in a
 * different driver which exports into VGEM and the mmap, write, verify sequence
 * is repeated on that.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xf86drm.h>

#include "bs_drm.h"

#define WITH_COLOR 1

#if WITH_COLOR
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#else
#define ANSI_COLOR_RED ""
#define ANSI_COLOR_GREEN ""
#define ANSI_COLOR_YELLOW ""
#define ANSI_COLOR_BLUE ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN ""
#define ANSI_COLOR_RESET ""
#endif

#define FAIL_COLOR ANSI_COLOR_RED "failed" ANSI_COLOR_RESET
#define SUCCESS_COLOR(x) ANSI_COLOR_GREEN x ANSI_COLOR_RESET

const uint32_t g_bo_pattern = 0xdeadbeef;
const char g_dev_card_path_format[] = "/dev/dri/card%d";

int create_vgem_bo(int fd, size_t size, uint32_t *handle)
{
	struct drm_mode_create_dumb create;
	int ret;

	memset(&create, 0, sizeof(create));
	create.height = size;
	create.width = 1;
	create.bpp = 8;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if (ret)
		return ret;

	assert(create.size >= size);

	*handle = create.handle;

	return 0;
}

void *mmap_dumb_bo(int fd, int handle, size_t size)
{
	struct drm_mode_map_dumb mmap_arg;
	void *ptr;
	int ret;

	memset(&mmap_arg, 0, sizeof(mmap_arg));

	mmap_arg.handle = handle;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
	assert(ret == 0);
	assert(mmap_arg.offset != 0);

	ptr = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, mmap_arg.offset);

	return ptr;
}

void write_pattern(volatile uint32_t *bo_ptr, size_t bo_size)
{
	volatile uint32_t *ptr;
	for (ptr = bo_ptr; ptr < bo_ptr + (bo_size / sizeof(*bo_ptr)); ptr++) {
		*ptr = g_bo_pattern;
	}
}

bool verify_pattern(volatile uint32_t *bo_ptr, size_t bo_size)
{
	volatile uint32_t *ptr;
	for (ptr = bo_ptr; ptr < bo_ptr + (bo_size / sizeof(*bo_ptr)); ptr++) {
		if (*ptr != g_bo_pattern) {
			fprintf(stderr,
				"buffer object verify " FAIL_COLOR " at offset %td = 0x%X\n",
				(void *)ptr - (void *)bo_ptr, *ptr);
			return false;
		}
	}

	return true;
}

static const char help_text[] =
    "Usage: %s [OPTIONS]\n"
    " -h          Print this help.\n"
    " -d [DEVICE] Open the given vgem device file (defaults to trying all cards under "
    "/dev/dri/).\n"
    " -c [SIZE]   Create a buffer objects of the given size in bytes.\n";

void print_help(const char *argv0)
{
	printf(help_text, argv0);
}

static const char optstr[] = "hd:c:";

int main(int argc, char *argv[])
{
	int ret = 0;
	const char *device_file = NULL;
	long bo_size = 65536;
	bool test_pattern = true;
	bool export_to_fd = true;
	bool import_to_handle = true;
	bool import_foreign = true;

	int c;
	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
			case 'h':
				print_help(argv[0]);
				exit(0);
				break;
			case 'd':
				device_file = optarg;
				break;
			case 'c':
				bo_size = atol(optarg);
				break;
			default:
				print_help(argv[0]);
				return 1;
		}
	}

	int vgem_fd = -1;
	if (device_file)
		vgem_fd = open(device_file, O_RDWR);
	else
		vgem_fd = bs_drm_open_vgem();

	if (vgem_fd < 0) {
		perror(FAIL_COLOR " to open vgem device");
		return 1;
	}

	printf(SUCCESS_COLOR("opened") " vgem device\n");

	uint32_t bo_handle;
	if (bo_size > 0) {
		ret = create_vgem_bo(vgem_fd, bo_size, &bo_handle);
		if (ret) {
			fprintf(stderr, FAIL_COLOR " to create bo: %d\n", ret);
			ret = 1;
			goto close_vgem_fd;
		}
		printf(SUCCESS_COLOR("created") " vgem buffer object, handle = %u\n", bo_handle);
	}

	if (test_pattern) {
		if (bo_size <= 0) {
			fprintf(stderr,
				"buffer object must be created before it can be written to\n");
			ret = 1;
			goto close_vgem_fd;
		}

		volatile uint32_t *bo_ptr = mmap_dumb_bo(vgem_fd, bo_handle, bo_size);
		if (bo_ptr == MAP_FAILED) {
			fprintf(stderr, FAIL_COLOR " to map buffer object\n");
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("mapped") " vgem buffer object\n");

		write_pattern(bo_ptr, bo_size);

		assert(!munmap((uint32_t *)bo_ptr, bo_size));

		printf(SUCCESS_COLOR("wrote") " to vgem buffer object\n");

		bo_ptr = mmap_dumb_bo(vgem_fd, bo_handle, bo_size);

		if (!verify_pattern(bo_ptr, bo_size)) {
			ret = 1;
			goto close_vgem_fd;
		}

		assert(!munmap((uint32_t *)bo_ptr, bo_size));

		printf(SUCCESS_COLOR("verified") " vgem buffer object writes\n");
	}

	int prime_fd;
	if (export_to_fd) {
		if (bo_size <= 0) {
			fprintf(stderr, "created buffer object required to perform export\n");
			ret = 1;
			goto close_vgem_fd;
		}

		ret = drmPrimeHandleToFD(vgem_fd, bo_handle, O_CLOEXEC, &prime_fd);
		if (ret) {
			fprintf(stderr, FAIL_COLOR " to export buffer object: %d\n", ret);
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("exported") " vgem buffer object, fd = %d\n", prime_fd);
	}

	uint32_t imported_handle;
	if (import_to_handle) {
		if (bo_size <= 0 || !export_to_fd) {
			fprintf(stderr, "exported buffer object required to perform import\n");
			ret = 1;
			goto close_vgem_fd;
		}

		ret = drmPrimeFDToHandle(vgem_fd, prime_fd, &imported_handle);
		if (ret) {
			fprintf(stderr, FAIL_COLOR " to import buffer object: %d\n", ret);
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("imported") " to vgem buffer object, handle = %d\n",
		       imported_handle);

		volatile uint32_t *bo_ptr = mmap_dumb_bo(vgem_fd, imported_handle, bo_size);
		if (bo_ptr == MAP_FAILED) {
			fprintf(stderr, FAIL_COLOR " to map imported buffer object\n");
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("mapped") " imported vgem buffer object\n");

		if (!verify_pattern(bo_ptr, bo_size)) {
			ret = 1;
			goto close_vgem_fd;
		}

		assert(!munmap((uint32_t *)bo_ptr, bo_size));

		printf(SUCCESS_COLOR("verified") " imported vgem buffer object writes\n");
	}

	if (import_foreign) {
		/* Open a card that's _not_ vgem. */
		int dumb_fd = bs_drm_open_for_display();
		if (dumb_fd == -1) {
			perror(FAIL_COLOR " to open non-vgem card\n");
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("opened") " non-vgem device\n");

		struct drm_mode_create_dumb create;

		memset(&create, 0, sizeof(create));
		create.width = 640;
		create.height = 480;
		create.bpp = 32;

		ret = drmIoctl(dumb_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
		if (ret) {
			fprintf(stderr, FAIL_COLOR " to create non-vgem buffer object: %d\n", ret);
			ret = 1;
			goto close_dumb_fd;
		}

		printf(SUCCESS_COLOR("created") " non-vgem buffer object, handle = %u\n",
		       create.handle);

		assert(create.size == create.width * create.height * 4);

		int foreign_prime_fd;
		ret = drmPrimeHandleToFD(dumb_fd, create.handle, O_CLOEXEC, &foreign_prime_fd);
		if (ret) {
			fprintf(stderr, FAIL_COLOR " to export non-vgem buffer object: %d\n", ret);
			ret = 1;
			goto close_dumb_fd;
		}

		printf(SUCCESS_COLOR("exported") " non-vgem buffer object, fd = %d\n",
		       foreign_prime_fd);

		uint32_t foreign_imported_handle;
		ret = drmPrimeFDToHandle(vgem_fd, foreign_prime_fd, &foreign_imported_handle);
		if (ret) {
			fprintf(stderr, FAIL_COLOR " to import non-vgem buffer object: %d\n", ret);
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("imported") " to vgem buffer object, handle = %d\n",
		       foreign_imported_handle);

		volatile uint32_t *bo_ptr =
		    mmap_dumb_bo(vgem_fd, foreign_imported_handle, create.size);
		if (bo_ptr == MAP_FAILED) {
			fprintf(stderr, FAIL_COLOR " to map imported non-vgem buffer object\n");
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("mapped") " imported non-vgem vgem buffer object\n");

		write_pattern(bo_ptr, create.size);

		assert(!munmap((uint32_t *)bo_ptr, create.size));

		printf(SUCCESS_COLOR("wrote") " to non-vgem buffer object\n");

		bo_ptr = mmap_dumb_bo(vgem_fd, foreign_imported_handle, create.size);
		if (bo_ptr == MAP_FAILED) {
			fprintf(stderr, FAIL_COLOR " to map imported non-vgem buffer object\n");
			ret = 1;
			goto close_vgem_fd;
		}

		if (!verify_pattern(bo_ptr, create.size)) {
			ret = 1;
			goto close_vgem_fd;
		}

		assert(!munmap((uint32_t *)bo_ptr, create.size));

		printf(SUCCESS_COLOR("verified") " imported non-vgem buffer object writes\n");

		bo_ptr = mmap_dumb_bo(dumb_fd, create.handle, create.size);
		if (bo_ptr == MAP_FAILED) {
			fprintf(stderr, FAIL_COLOR " to map non-vgem buffer object\n");
			ret = 1;
			goto close_vgem_fd;
		}

		if (!verify_pattern(bo_ptr, create.size)) {
			ret = 1;
			goto close_vgem_fd;
		}

		printf(SUCCESS_COLOR("verified") " non-vgem buffer object writes\n");

		assert(!munmap((uint32_t *)bo_ptr, create.size));

	close_dumb_fd:
		close(dumb_fd);
	}

close_vgem_fd:
	close(vgem_fd);
	return ret;
}
