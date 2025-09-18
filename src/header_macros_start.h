
#ifdef PGL_PREFIX_TYPES
#define vec2 pgl_vec2
#define vec3 pgl_vec3
#define vec4 pgl_vec4
#define ivec2 pgl_ivec2
#define ivec3 pgl_ivec3
#define ivec4 pgl_ivec4
#define uvec2 pgl_uvec2
#define uvec3 pgl_uvec3
#define uvec4 pgl_uvec4
#define bvec2 pgl_bvec2
#define bvec3 pgl_bvec3
#define bvec4 pgl_bvec4
#define mat2 pgl_mat2
#define mat3 pgl_mat3
#define mat4 pgl_mat4
#define Color pgl_Color
#define Line pgl_Line
#define Plane pgl_Plane
#endif


// I really need to think about these
// Maybe suffixes should just be the default since I already give many glsl
// functions suffixes but then we still have the problem if I ever want
// to support doubles with no suffix like C math funcs..

// Add/remove as needed as long as you also modify
// matching undef section in close_pgl.h

#ifdef PGL_PREFIX_GLSL
#define smoothstep pgl_smoothstep
#define clamp_01 pgl_clamp_01
#define clamp pgl_clamp
#define clampi pgl_clampi

#elif defined(PGL_SUFFIX_GLSL)

#define smoothstep smoothstepf
#define clamp_01 clampf_01
#define clamp clampf
#define clampi clampi
#endif


#ifndef GL_H
#define GL_H


#ifdef __cplusplus
extern "C" {
#endif


#ifndef PGLDEF
#ifdef PGL_STATIC
#define PGLDEF static
#else
#define PGLDEF extern
#endif
#endif

#ifndef PGL_ASSERT
#include <assert.h>
#define PGL_ASSERT(x) assert(x)
#endif

#ifndef CVEC_ASSERT
#define CVEC_ASSERT(x) PGL_ASSERT(x)
#endif

#if defined(PGL_MALLOC) && defined(PGL_FREE) && defined(PGL_REALLOC)
/* ok */
#elif !defined(PGL_MALLOC) && !defined(PGL_FREE) && !defined(PGL_REALLOC)
/* ok */
#else
#error "Must define all or none of PGL_MALLOC, PGL_FREE, and PGL_REALLOC."
#endif

#ifndef PGL_MALLOC
#define PGL_MALLOC(sz)      malloc(sz)
#define PGL_REALLOC(p, sz)  realloc(p, sz)
#define PGL_FREE(p)         free(p)
#endif

#define CVEC_MALLOC(sz) PGL_MALLOC(sz)
#define CVEC_REALLOC(p, sz) PGL_REALLOC(p, sz)
#define CVEC_FREE(p) PGL_FREE(p)

#ifndef PGL_MEMMOVE
#include <string.h>
#define PGL_MEMMOVE(dst, src, sz)   memmove(dst, src, sz)
#else
#define CVEC_MEMMOVE(dst, src, sz) PGL_MEMMOVE(dst, src, sz)
#endif

// Get rid of signed/unsigned comparison warnings when looping through vectors
#ifndef CVEC_SIZE_T
#define CVEC_SIZE_T i64
#endif

// Default to RGBA memory order on a little endian architecture
//#ifndef PGL_PIXFORMAT
//#define PGL_PIXFORMAT PGL_ABGR32
//#endif

// TODO more 32-bit formats, then non-32-bit formats eventually
#ifdef PGL_RGBA32
#define PGL_RMASK 0xFF000000
#define PGL_GMASK 0x00FF0000
#define PGL_BMASK 0x0000FF00
#define PGL_AMASK 0x000000FF
#define PGL_RSHIFT 24
#define PGL_GSHIFT 16
#define PGL_BSHIFT 8
#define PGL_ASHIFT 0
#elif defined(PGL_ABGR32)
#define PGL_AMASK 0xFF000000
#define PGL_BMASK 0x00FF0000
#define PGL_GMASK 0x0000FF00
#define PGL_RMASK 0x000000FF
#define PGL_ASHIFT 24
#define PGL_BSHIFT 16
#define PGL_GSHIFT 8
#define PGL_RSHIFT 0
#elif defined(PGL_ARGB32)
#define PGL_AMASK 0xFF000000
#define PGL_RMASK 0x00FF0000
#define PGL_GMASK 0x0000FF00
#define PGL_BMASK 0x000000FF
#define PGL_ASHIFT 24
#define PGL_RSHIFT 16
#define PGL_GSHIFT 8
#define PGL_BSHIFT 0
#endif

// Default to RGBA memory order on a little endian architecture
#ifndef PGL_AMASK
#define PGL_AMASK 0xFF000000
#define PGL_BMASK 0x00FF0000
#define PGL_GMASK 0x0000FF00
#define PGL_RMASK 0x000000FF
#define PGL_ASHIFT 24
#define PGL_BSHIFT 16
#define PGL_GSHIFT 8
#define PGL_RSHIFT 0
#endif


#if !defined(PGL_AMASK) || !defined(PGL_BMASK) || !defined(PGL_GMASK) || !defined(PGL_BMASK) || \
    !defined(PGL_ASHIFT) || !defined(PGL_BSHIFT) || !defined(PGL_GSHIFT) || !defined(PGL_BSHIFT)
#error "Must define all PGL_(RGBA)MASK and PGL_(RGBA)SHIFT"
#endif
