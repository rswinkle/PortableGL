#ifndef CRSW_MATH_H
#define CRSW_MATH_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Unfortunately this is not supported in gcc even though
// it's in the C99+ spec.  Have to use compiler option
// -ffp-contract=off for gcc (which defaults to =fast)
// unlike clang
//
//  https://stackoverflow.com/questions/43352510/difference-in-gcc-ffp-contract-options
// MSVC does not implement #pragma STDC (C4068 unknown pragma).
#ifndef _MSC_VER
#pragma STDC FP_CONTRACT OFF
#endif

// Key off the *compiler*, not the OS: MinGW is _WIN32 + GCC and wants
// __attribute__; MSVC is _WIN32 without GCC and rejects it.
#ifndef RSW_INLINE
#if defined(__GNUC__) || defined(__clang__)
	#define RSW_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
	#define RSW_INLINE __forceinline
#else
	#define RSW_INLINE inline
#endif
#endif

#define RM_PI (3.14159265358979323846)
#define RM_2PI (2.0 * RM_PI)
#define PI_DIV_180 (0.017453292519943296)
#define INV_PI_DIV_180 (57.2957795130823229)

#define DEG_TO_RAD(x)   ((x)*PI_DIV_180)
#define RAD_TO_DEG(x)   ((x)*INV_PI_DIV_180)

/* Hour angles */
#define HR_TO_DEG(x)    ((x) * (1.0 / 15.0))
#define HR_TO_RAD(x)    DEG_TO_RAD(HR_TO_DEG(x))

#define DEG_TO_HR(x)    ((x) * 15.0)
#define RAD_TO_HR(x)    DEG_TO_HR(RAD_TO_DEG(x))

#define RM_PIf (3.14159265358979323846f)
#define RM_2PIf (2.0f * RM_PIf)
#define PI_DIV_180f (0.017453292519943296f)
#define INV_PI_DIV_180f (57.2957795130823229f)

#define DEG_TO_RADf(x)   ((x)*PI_DIV_180f)
#define RAD_TO_DEGf(x)   ((x)*INV_PI_DIV_180f)

/* Hour angles */
#define HR_TO_DEGf(x)    ((x) * (1.0f / 15.0f))
#define HR_TO_RADf(x)    DEG_TO_RADf(HR_TO_DEGf(x))

#define DEG_TO_HRf(x)    ((x) * 15.0f)
#define RAD_TO_HRf(x)    DEG_TO_HRf(RAD_TO_DEGf(x))

// TODO rename RM_MAX/RSW_MAX?  make proper inline functions?
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

