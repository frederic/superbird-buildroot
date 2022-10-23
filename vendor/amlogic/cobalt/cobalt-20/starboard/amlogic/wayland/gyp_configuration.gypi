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
  'target_defaults': {
    'default_configuration': 'amlogic-wayland_debug',
    'configurations': {
      'amlogic-wayland_debug': {
        'inherit_from': ['debug_base'],
      },
      'amlogic-wayland_devel': {
        'inherit_from': ['devel_base'],
      },
      'amlogic-wayland_qa': {
        'inherit_from': ['qa_base'],
      },
      'amlogic-wayland_gold': {
        'inherit_from': ['gold_base'],
      },
    }, # end of configurations
  },

  'includes': [
    '<(DEPTH)/third_party/starboard/amlogic/shared/gyp_configuration.gypi',
    '<(DEPTH)/third_party/starboard/amlogic/shared/compiler_flags.gypi',
  ],

  'variables': {
    'target_arch': '<!(echo $COBALT_ARCH)',
    'arm_version': 7,
    'arm_float_abi%': 'hard',
    'arm_neon': 1,
    'gl_type': 'system_gles2',
    'enable_map_to_mesh': 1,
    'cobalt_minimum_frame_time_in_milliseconds': '20.0',
    'rasterizer_type': 'hardware',

    'platform_libraries': [
      '-lEGL',
      '-lGLESv2',
      '-lwayland-egl',
      '-lwayland-client',
    ],
    'linker_flags': [
      '-Wl,--wrap=eglGetDisplay',
      '--sysroot=<(sysroot)',
    ],
    'compiler_flags_qa!': [
      '-gline-tables-only',
    ],
  },
}
