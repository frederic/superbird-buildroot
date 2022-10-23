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
"""Thirdparty Starboard Amlogic arm Directfb platform configuration."""
import os
import sys
import re

from third_party.starboard.amlogic.shared import gyp_configuration as shared_configuration
from starboard.build import clang
from starboard.tools import build

class ArmDirectfbConfiguration(shared_configuration.LinuxConfiguration):
  """Thirdparty Starboard Amlogic arm Directfb platform configuration."""

  def __init__(self,
               platform_name='amlogic-directfb',
               asan_enabled_by_default=False,
               goma_supports_compiler=True):
    super(ArmDirectfbConfiguration, self).__init__(
        platform_name, asan_enabled_by_default, goma_supports_compiler)

  def GetEnvironmentVariables(self):
    self.host_compiler_environment = {
      'CC_host': 'gcc',
      'CXX_host': 'g++',
      'LD_host': 'gcc',
      'ARFLAGS_host': 'rcs',
      'ARTHINFLAGS_host': 'rcsT',
    }
    if not hasattr(self, 'host_compiler_environment'):
      self.host_compiler_environment = build.GetHostCompilerEnvironment(
          clang.GetClangSpecification(), self.goma_supports_compiler)

    env_variables = self.host_compiler_environment
    if os.environ.has_key('COBALT_CROSS'):
        target_cross = os.environ['COBALT_CROSS']
        print("use cross compiler "+target_cross)
        env_variables.update({
            'CC': target_cross+"gcc",
            'CXX': target_cross+"g++",
        })
        return env_variables
    env_variables.update({
        'CC': "arm-linux-gnueabihf-gcc",
        'CXX': "arm-linux-gnueabihf-g++",
    })
    return env_variables

  def GetVariables(self, config_name):
    variables = super(ArmDirectfbConfiguration, self).GetVariables(
        config_name)
    sysroot = os.environ['SYS_ROOT'];
    variables.update({
        'sysroot': sysroot,
    })
    if os.environ.has_key('COBALT_CFLAGS'):
        target_cflags = os.environ['COBALT_CFLAGS']
        # -mfloat-abi=softfp
        m=re.search('-mfloat-abi=(\S+)', target_cflags)
        if m:
            print("arm_float_abi set to " + m.group(1))
            variables.update({"arm_float_abi": m.group(1)})
    return variables

def CreatePlatformConfig():
  return ArmDirectfbConfiguration()
