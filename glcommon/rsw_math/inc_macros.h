#ifndef CRSW_MATH_H
#define CRSW_MATH_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//#define PGL_PREFIX_TYPES

#ifdef PREFIX_TYPES
#define vec2 glinternal_vec2
#define vec3 glinternal_vec3
#define vec4 glinternal_vec4
#define dvec2 glinternal_dvec2
#define dvec3 glinternal_dvec3
#define dvec4 glinternal_dvec4
#define ivec2 glinternal_ivec2
#define ivec3 glinternal_ivec3
#define ivec4 glinternal_ivec4
#define uvec2 glinternal_uvec2
#define uvec3 glinternal_uvec3
#define uvec4 glinternal_uvec4
#define mat2 glinternal_mat2
#define mat3 glinternal_mat3
#define mat4 glinternal_mat4
#define Color glinternal_Color
#define Line glinternal_Line
#define Plane glinternal_Plane
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

