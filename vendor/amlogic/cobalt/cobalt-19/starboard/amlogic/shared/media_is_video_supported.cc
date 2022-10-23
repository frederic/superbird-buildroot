// Copyright 2017 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/shared/starboard/media/media_support_internal.h"

#include "starboard/configuration.h"
#include "starboard/media.h"
#include "starboard/string.h"
#include <stdlib.h>

static int get_env_bool(const char *envname, int def) {
  const char *env = getenv(envname);
  if (env == NULL)
    return def;
  return ((SbStringCompareAll(env, "1") == 0) ||
          (SbStringCompareAll(env, "yes") == 0) ||
          (SbStringCompareAll(env, "true") == 0));
}

SB_EXPORT bool SbMediaIsVideoSupported(SbMediaVideoCodec video_codec,
                                       int frame_width, int frame_height,
                                       int64_t bitrate, int fps,
                                       bool decode_to_texture_required) {
  static int cobalt_disable_4K = -1;
  if (cobalt_disable_4K == -1) {
    cobalt_disable_4K = get_env_bool("COBALT_DISABLE_4K", 0);
  }
  int max_supported_width = cobalt_disable_4K ? 1920 : 3840;
  int max_supported_height = cobalt_disable_4K ? 1080 : 2160;
  int max_supported_fps = decode_to_texture_required ? 30 : 60;
  if (video_codec == kSbMediaVideoCodecVp9) {
    static int cobalt_disable_vp9 = -1;
    if (cobalt_disable_vp9 == -1) {
      cobalt_disable_vp9 = get_env_bool("COBALT_DISABLE_VP9", 0);
    }
    if (cobalt_disable_vp9)
      return false;
  } else {
    //if (frame_width > 1920 || frame_height > 1080)
    //  max_supported_fps = 30;
    if (decode_to_texture_required) {
      max_supported_width = 1920;
      max_supported_height = 1080;
    }
  }
  return (video_codec == kSbMediaVideoCodecH264 ||
          video_codec == kSbMediaVideoCodecVp9) &&
         frame_width <= max_supported_width &&
         frame_height <= max_supported_height &&
         bitrate <= SB_MEDIA_MAX_VIDEO_BITRATE_IN_BITS_PER_SECOND &&
         fps <= max_supported_fps;
}
