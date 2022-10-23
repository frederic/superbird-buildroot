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

#include "starboard/drm.h"
#include "starboard/media.h"
#include "starboard/log.h"
#include "starboard/string.h"

#include "drm_system_widevine.h"

extern "C" {
__attribute__((visibility("default"))) 
int cobalt_widevine_cdm_init(struct CobaltWidevineSymbols * symbols) {
    symbols->InitLogging = &::wvcdm::InitLogging;
    symbols->a2bs_hex = &::wvcdm::a2bs_hex;
    symbols->version = &::widevine::Cdm::version;
    symbols->initialize = &::widevine::Cdm::initialize;
    symbols->create = &::widevine::Cdm::create;
    symbols->CopyBuffer = &::OEMCrypto_CopyBuffer;
    return 0;
};

}
