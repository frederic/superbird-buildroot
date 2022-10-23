// Copyright 2018 Google LLC. All Rights Reserved. This file and proprietary
// source code may only be used and distributed under the Widevine Master
// License Agreement.

#ifndef WVCDM_UTIL_UTIL_COMMON_H_
#define WVCDM_UTIL_UTIL_COMMON_H_

#ifdef _WIN32
# ifdef CORE_UTIL_IMPLEMENTATION
#  define CORE_UTIL_EXPORT __declspec(dllexport)
# else
#  define CORE_UTIL_EXPORT __declspec(dllimport)
# endif
#else
# ifdef CORE_UTIL_IMPLEMENTATION
#  define CORE_UTIL_EXPORT __attribute__((visibility("default")))
# else
#  define CORE_UTIL_EXPORT
# endif
#endif

#endif  // WVCDM_UTIL_UTIL_COMMON_H_
