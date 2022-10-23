/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#ifndef typedef_h
#define typedef_h

/* change these typedef declarations to correspond with your platform */
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef __int8  int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
#else
#include <stdint.h>
#endif

/*
typedef char Word8;
typedef unsigned char UWord8;
typedef short Word16;
typedef unsigned short UWord16;
typedef long Word32;
typedef double Float64;
typedef float Float32;
*/
typedef int8_t Word8;
typedef uint8_t UWord8;
typedef int16_t Word16;
typedef uint16_t UWord16;
typedef int32_t Word32;
typedef double Float64;
typedef float Float32;

#endif
