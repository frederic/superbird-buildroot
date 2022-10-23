/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

static bool display_filter(int fd)
{
	bool has_connection = false;
	drmModeRes *res = drmModeGetResources(fd);
	if (!res)
		return false;

	if (res->count_crtcs == 0)
		goto out;

	for (int connector_index = 0; connector_index < res->count_connectors; connector_index++) {
		drmModeConnector *connector =
		    drmModeGetConnector(fd, res->connectors[connector_index]);
		if (connector == NULL)
			continue;

		has_connection =
		    connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0;
		drmModeFreeConnector(connector);
		if (has_connection)
			break;
	}

out:
	drmModeFreeResources(res);
	return has_connection;
}

int bs_drm_open_for_display()
{
	return bs_open_filtered("/dev/dri/card%u", 0, DRM_MAX_MINOR, display_filter);
}

static bool connector_has_crtc(int fd, drmModeRes *res, drmModeConnector *connector)
{
	for (int encoder_index = 0; encoder_index < connector->count_encoders; encoder_index++) {
		drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoders[encoder_index]);
		if (encoder == NULL)
			continue;

		uint32_t possible_crtcs = encoder->possible_crtcs;
		drmModeFreeEncoder(encoder);

		for (int crtc_index = 0; crtc_index < res->count_crtcs; crtc_index++)
			if ((possible_crtcs & (1 << crtc_index)) != 0)
				return true;
	}

	return false;
}

static uint32_t display_rank_connector_type(uint32_t connector_type)
{
	switch (connector_type) {
		case DRM_MODE_CONNECTOR_LVDS:
			return 0x01;
		case DRM_MODE_CONNECTOR_eDP:
			return 0x02;
		case DRM_MODE_CONNECTOR_DSI:
			return 0x03;
	}
	return 0xFF;
}

static uint32_t display_rank(int fd)
{
	drmModeRes *res = drmModeGetResources(fd);
	if (!res)
		return bs_rank_skip;

	uint32_t best_rank = bs_rank_skip;
	if (res->count_crtcs == 0)
		goto out;

	for (int connector_index = 0; connector_index < res->count_connectors; connector_index++) {
		drmModeConnector *connector =
		    drmModeGetConnector(fd, res->connectors[connector_index]);
		if (connector == NULL)
			continue;

		bool has_connection = connector->connection == DRM_MODE_CONNECTED &&
				      connector->count_modes > 0 &&
				      connector_has_crtc(fd, res, connector);
		if (!has_connection)
			continue;

		uint32_t rank = display_rank_connector_type(connector->connector_type);
		if (best_rank > rank)
			best_rank = rank;
		drmModeFreeConnector(connector);
	}

out:
	drmModeFreeResources(res);
	return best_rank;
}

int bs_drm_open_main_display()
{
	return bs_open_ranked("/dev/dri/card%u", 0, DRM_MAX_MINOR, display_rank);
}

static bool vgem_filter(int fd)
{
	drmVersion *version = drmGetVersion(fd);
	if (!version)
		return false;

	bool is_vgem = (strncmp("vgem", version->name, version->name_len) == 0);
	drmFreeVersion(version);
	return is_vgem;
}

int bs_drm_open_vgem()
{
	return bs_open_filtered("/dev/dri/card%u", 0, DRM_MAX_MINOR, vgem_filter);
}
