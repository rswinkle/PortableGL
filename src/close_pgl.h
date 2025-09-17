
#ifdef PGL_PREFIX_TYPES
#undef vec2
#undef vec3
#undef vec4
#undef ivec2
#undef ivec3
#undef ivec4
#undef uvec2
#undef uvec3
#undef uvec4
#undef bvec2
#undef bvec3
#undef bvec4
#undef mat2
#undef mat3
#undef mat4
#undef Color
#undef Line
#undef Plane
#endif

#ifdef PGL_PREFIX_GLSL
#undef smoothstep
#undef clamp_01
#undef clamp
#undef clampi

#elif defined(PGL_SUFFIX_GLSL)
#undef smoothstep
#undef clamp_01
#undef clamp
#undef clampi
#endif

