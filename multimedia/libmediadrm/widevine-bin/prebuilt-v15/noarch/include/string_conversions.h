// Copyright 2018 Google LLC. All Rights Reserved. This file and proprietary
// source code may only be used and distributed under the Widevine Master
// License Agreement.

#ifndef WVCDM_UTIL_STRING_CONVERSIONS_H_
#define WVCDM_UTIL_STRING_CONVERSIONS_H_

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "util_common.h"

namespace wvcdm {

CORE_UTIL_EXPORT std::vector<uint8_t> a2b_hex(const std::string& b);
CORE_UTIL_EXPORT std::vector<uint8_t> a2b_hex(const std::string& label,
                                              const std::string& b);
CORE_UTIL_EXPORT std::string a2bs_hex(const std::string& b);
CORE_UTIL_EXPORT std::string b2a_hex(const std::vector<uint8_t>& b);
CORE_UTIL_EXPORT std::string b2a_hex(const std::string& b);
CORE_UTIL_EXPORT std::string Base64Encode(
    const std::vector<uint8_t>& bin_input);
CORE_UTIL_EXPORT std::vector<uint8_t> Base64Decode(
    const std::string& bin_input);
CORE_UTIL_EXPORT std::string Base64SafeEncode(
    const std::vector<uint8_t>& bin_input);
CORE_UTIL_EXPORT std::string Base64SafeEncodeNoPad(
    const std::vector<uint8_t>& bin_input);
CORE_UTIL_EXPORT std::vector<uint8_t> Base64SafeDecode(
    const std::string& bin_input);
CORE_UTIL_EXPORT std::string HexEncode(const uint8_t* bytes, unsigned size);
CORE_UTIL_EXPORT std::string IntToString(int value);
CORE_UTIL_EXPORT int64_t htonll64(int64_t x);
CORE_UTIL_EXPORT inline int64_t ntohll64(int64_t x) { return htonll64(x); }
CORE_UTIL_EXPORT std::string BytesToString(const uint8_t* bytes, unsigned size);

}  // namespace wvcdm

#endif  // WVCDM_UTIL_STRING_CONVERSIONS_H_
