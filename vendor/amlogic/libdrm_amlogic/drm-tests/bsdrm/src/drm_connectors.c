/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bs_drm.h"

const uint32_t bs_drm_connectors_main_rank[] = { DRM_MODE_CONNECTOR_LVDS,
						 0x01,
						 DRM_MODE_CONNECTOR_eDP,
						 0x02,
						 DRM_MODE_CONNECTOR_DSI,
						 0x03,
						 DRM_MODE_CONNECTOR_HDMIA,
						 0x04,
						 bs_drm_connectors_any,
						 0xFF,
						 0,
						 0 };

const uint32_t bs_drm_connectors_internal_rank[] = { DRM_MODE_CONNECTOR_LVDS,
						     0x01,
						     DRM_MODE_CONNECTOR_eDP,
						     0x02,
						     DRM_MODE_CONNECTOR_DSI,
						     0x03,
						     0,
						     0 };

const uint32_t bs_drm_connectors_external_rank[] = { DRM_MODE_CONNECTOR_LVDS,
						     bs_rank_skip,
						     DRM_MODE_CONNECTOR_eDP,
						     bs_rank_skip,
						     DRM_MODE_CONNECTOR_DSI,
						     bs_rank_skip,
						     DRM_MODE_CONNECTOR_HDMIA,
						     0x01,
						     bs_drm_connectors_any,
						     0x02,
						     0,
						     0 };

uint32_t bs_drm_connectors_rank(const uint32_t *ranks, uint32_t connector_type)
{
	for (size_t rank_index = 0; ranks[rank_index] != 0; rank_index += 2)
		if (ranks[rank_index] == bs_drm_connectors_any ||
		    connector_type == ranks[rank_index])
			return ranks[rank_index + 1];
	return bs_rank_skip;
}
