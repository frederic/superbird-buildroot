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
    'default_configuration': 'amlogic-directfb_debug',
    'configurations': {
      'amlogic-directfb_debug': {
        'inherit_from': ['debug_base'],
      },
      'amlogic-directfb_devel': {
        'inherit_from': ['devel_base'],
      },
      'amlogic-directfb_qa': {
        'inherit_from': ['qa_base'],
      },
      'amlogic-directfb_gold': {
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
    'gl_type': 'none',
    'enable_map_to_mesh': 0,

    'platform_libraries': [
      '-ldirectfb',
      '-ldirect',
    ],
    'linker_flags': [
      '--sysroot=<(sysroot)',
    ],
    'compiler_flags_qa!': [
      '-gline-tables-only',
    ],
  },
}
