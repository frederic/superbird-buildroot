# Copyright 2018 The Cobalt Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
{
  'includes': ['<(DEPTH)/third_party/starboard/amlogic/shared/starboard_platform.gypi'],

  'variables': {
    'starboard_platform_sources': [
      '<(DEPTH)/third_party/starboard/amlogic/wayland/main.cc',
      '<(DEPTH)/third_party/starboard/amlogic/wayland/system_get_property.cc',
      '<(DEPTH)/third_party/starboard/amlogic/wayland/player_output_mode_supported.cc',
      '<(DEPTH)/third_party/starboard/amlogic/wayland/window_internal.cc',
      '<(DEPTH)/third_party/starboard/amlogic/wayland/dev_input.cc',
      '<(DEPTH)/starboard/shared/starboard/link_receiver.cc',
      '<(DEPTH)/starboard/shared/wayland/application_wayland.cc',
      '<(DEPTH)/starboard/shared/wayland/egl_workaround.cc',
      '<(DEPTH)/starboard/shared/wayland/native_display_type.cc',
      '<(DEPTH)/starboard/shared/wayland/window_create.cc',
      '<(DEPTH)/starboard/shared/wayland/window_destroy.cc',
      '<(DEPTH)/starboard/shared/wayland/window_get_platform_handle.cc',
      '<(DEPTH)/starboard/shared/wayland/window_get_size.cc',
    ],
    'starboard_platform_sources!': [
      '<(DEPTH)/starboard/shared/starboard/player/player_output_mode_supported.cc',
      '<(DEPTH)/starboard/shared/libvpx/vpx_video_decoder.cc',
      '<(DEPTH)/starboard/shared/libvpx/vpx_video_decoder.h',
    ],
    'starboard_platform_dependencies!': [
      '<(DEPTH)/third_party/libvpx/libvpx.gyp:libvpx',
    ],
  },
  'target_defaults': {
    'include_dirs!': [
          '<(DEPTH)/third_party/khronos',
    ],
  },
}
