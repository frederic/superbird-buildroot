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
  'variables': {
# oemctyptoimpl optee ref
    'oemctyptoimpl':'optee',
  },
  'targets': [
    {
      'target_name': 'widevine_cdm_cobalt_none',
      'type': 'none',
      'conditions': [
          ['oemctyptoimpl=="optee"', {
              'all_dependent_settings': {
                  'defines': [
                      'COBALT_WIDEVINE_OPTEE',
                  ],
                  'include_dirs': [
                      '<!(echo $WIDEVINE_CE_CDM_INC)',
                  ],},
          }],  # oemctyptoimpl=="optee"
      ],  # conditions
    }
  ],
}
