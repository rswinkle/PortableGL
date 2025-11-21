
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
//
// For now it's just functions that are used inside PortableGL itself
// as that is what will definitely break without them if these macros
// are used

// Add/remove as needed as long as you also modify
// matching undef section in close_pgl.h

#ifdef PGL_PREFIX_GLSL
#define smoothstep pgl_smoothstep
#define clamp_01 pgl_clamp_01
#define clamp_01_vec4 pgl_clamp_01_vec4
#define clamp pgl_clamp
#define clampi pgl_clampi

#elif defined(PGL_SUFFIX_GLSL)

#define smoothstep smoothstepf
#define clamp_01 clampf_01
#define clamp_01_vec4 clampf_01_vec4
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

