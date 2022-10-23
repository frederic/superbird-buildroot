/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#	define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#	define PLATFORM_LINUX
#elif defined(unix) || defined(__unix)
#   define PLATFORM_QNX
#else
#	define PLATFORM_NON_OS
#endif

#if defined(_MSC_VER)
#	include <windows.h>
#	define inline _inline
#elif defined(__GNUC__)
#elif defined(__ARMCC__)
#else
#  error "Unknown compiler."
#endif

#define API_VERSION_MAJOR       5
#define API_VERSION_MINOR       5
#define API_VERSION_PATCH       59
#define API_VERSION             ((API_VERSION_MAJOR<<16) | (API_VERSION_MINOR<<8) | API_VERSION_PATCH)

#if defined(PLATFORM_NON_OS) || defined (ANDROID) || defined(MFHMFT_EXPORTS) || defined(PLATFORM_QNX) || defined(CNM_SIM_PLATFORM)
//#define SUPPORT_FFMPEG_DEMUX
#else
#define SUPPORT_FFMPEG_DEMUX
#endif

//------------------------------------------------------------------------------
// COMMON
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID) || defined(CNM_FPGA_HAPS_INTERFACE)
#define SUPPORT_MULTI_INST_INTR
#endif

// do not define BIT_CODE_FILE_PATH in case of multiple product support.
#define CORE_6_BIT_CODE_FILE_PATH   "chagall.bin"

//------------------------------------------------------------------------------
// OMX
//------------------------------------------------------------------------------








//------------------------------------------------------------------------------
// VP521 or VP521C
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// VP511
//------------------------------------------------------------------------------


//#define SUPPORT_SW_UART
//#define SUPPORT_SW_UART_V2	// VP511 or VP521 or VP521C
#endif /* __CONFIG_H__ */


