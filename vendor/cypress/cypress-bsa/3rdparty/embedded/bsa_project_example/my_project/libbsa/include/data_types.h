/****************************************************************************
**
**  Name        data_types.h
**
**  Description:    This file contains common data type definitions used
**                  throughout the Widcomm Bluetooth code
**
**  Copyright (c) 2009-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdint.h>


#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#ifdef BUILDCFG
#include "buildcfg.h"
#endif

#include "string.h"

#ifndef NULL
#define NULL     0
#endif

#ifndef FALSE
#define FALSE  0
#endif

#if 1

typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32, *PUINT32;
typedef int32_t   INT32,  *PINT32;
typedef int8_t    INT8;
typedef int16_t   INT16;
typedef uint8_t   BOOLEAN;
typedef uint32_t  BOOL32;
typedef uint8_t   BOOL8;
typedef uint64_t  UINT64, *PUINT64;

typedef struct
{
    uint64_t    uLow64;
    uint64_t    uHigh64;
} UINT128, *PUINT128;

typedef uintptr_t TIME_STAMP;
typedef uintptr_t UINTPTR;

#else

#ifndef UINT8
typedef unsigned char   UINT8;
#endif
#ifndef UINT16
typedef unsigned short  UINT16;
#endif
#ifndef UINT32
typedef unsigned int    UINT32;
#endif
#ifndef UINT64
typedef unsigned long long  UINT64;
#endif
#ifndef INT32
typedef signed   int    INT32;
#endif
#ifndef INT8
typedef signed   char   INT8;
#endif
#ifndef INT16
typedef signed   short  INT16;
#endif
#ifndef BOOLEAN
typedef unsigned char   BOOLEAN;
#endif
#ifndef UINTPTR
typedef unsigned long   UINTPTR;
#endif

#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

#ifdef __arm
#ifndef PACKED
#define PACKED  __packed
#endif
#ifndef INLINE
#define INLINE  __inline
#endif
#else
#ifndef PACKED
#define PACKED
#endif
#ifndef INLINE
#define INLINE
#endif
#endif

#if (!defined(__GNUC__)) || (__GNUC__ < 3)
#define __builtin_expect(x, n) (x)
#endif

#define BCM_LIKELY(x) (__builtin_expect(!!(x), 1))
#define BCM_UNLIKELY(x) (__builtin_expect(!!(x), 0))

#ifndef BCM_STRCAT_S
#define BCM_STRCAT_S(x1,x2,x3)      strcat((x1),(x3))
#endif
#ifndef BCM_STRNCAT_S
#define BCM_STRNCAT_S(x1,x2,x3,x4)  strncat((x1),(x3),(x4))
#endif
#ifndef BCM_STRCPY_S
#define BCM_STRCPY_S(x1,x2,x3)      strcpy((x1),(x3))
#endif
#ifndef BCM_STRNCPY_S
#define BCM_STRNCPY_S(x1,x2,x3,x4)  strncpy((x1),(x3),(x4))
#endif

#ifndef TIMER_PARAM_TYPE
#define TIMER_PARAM_TYPE    UINTPTR
#endif

#endif

