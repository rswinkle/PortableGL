# generate all in one portablegl.h from component libraries/files

import sys, os, glob, argparse

prefix_types = """
#ifdef PGL_PREFIX_TYPES
#define vec2 pgl_vec2
#define vec3 pgl_vec3
#define vec4 pgl_vec4
#define dvec2 pgl_dvec2
#define dvec3 pgl_dvec3
#define dvec4 pgl_dvec4
#define ivec2 pgl_ivec2
#define ivec3 pgl_ivec3
#define ivec4 pgl_ivec4
#define uvec2 pgl_uvec2
#define uvec3 pgl_uvec3
#define uvec4 pgl_uvec4
#define mat2 pgl_mat2
#define mat3 pgl_mat3
#define mat4 pgl_mat4
#define Color pgl_Color
#define Line pgl_Line
#define Plane pgl_Plane
#endif
"""

# Could always define CVEC_MALLOC et al to PGL_MALLOC, rather than putting
# it in an else block...probably not any preprocessing speed difference so
# just personal taste.  I'll have to think about it
macros = """

#ifndef PGL_ASSERT
#include <assert.h>
#define PGL_ASSERT(x) assert(x)
#endif

#define CVEC_ASSERT(x) PGL_ASSERT(x)

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
#else
#define CVEC_MALLOC(sz) PGL_MALLOC(sz)
#define CVEC_REALLOC(p, sz) PGL_REALLOC(p, sz)
#define CVEC_FREE(p) PGL_FREE(p)
#endif

#ifndef PGL_MEMMOVE
#include <string.h>
#define PGL_MEMMOVE(dst, src, sz)   memmove(dst, src, sz)
#else
#define CVEC_MEMMOVE(dst, src, sz) PGL_MEMMOVE(dst, src, sz)
#endif

#ifndef PGL_SIMPLE_THICK_LINES
#define DRAW_THICK_LINE draw_thick_line
#else
#define DRAW_THICK_LINE draw_thick_line_simple
#endif

"""

# I really need to think about these
# Maybe suffixes should just be the default since I already
# give many glsl functions suffixes
# but then we still have the problem if I ever want
# to support doubles with no suffix like C math funcs..
prefix_suffix_glsl = """

// Add/remove as needed as long as you also modify
// matching undef section

#ifdef PGL_PREFIX_GLSL
#define mix pgl_mix
#define radians pgl_radians
#define degrees pgl_degrees
#define smoothstep pgl_smoothstep
#define clamp_01 pgl_clamp_01
#define clamp pgl_clamp
#define clampi pgl_clampi

#elif defined(PGL_SUFFIX_GLSL)

#define mix mixf
#define radians radiansf
#define degrees degreesf
#define smoothstep smoothstepf
#define clamp_01 clampf_01
#define clamp clampf
#define clampi clampi
#endif

"""

unprefix_suffix_glsl = """
#ifdef PGL_PREFIX_GLSL
#undef mix
#undef radians
#undef degrees
#undef smoothstep
#undef clamp_01
#undef clamp
#undef clampi

#elif defined(PGL_SUFFIX_GLSL)
#undef mix
#undef radians
#undef degrees
#undef smoothstep
#undef clamp_01
#undef clamp
#undef clampi
#endif

"""

unprefix_types = """
#ifdef PGL_PREFIX_TYPES
#undef vec2
#undef vec3
#undef vec4
#undef dvec2
#undef dvec3
#undef dvec4
#undef ivec2
#undef ivec3
#undef ivec4
#undef uvec2
#undef uvec3
#undef uvec4
#undef mat2
#undef mat3
#undef mat4
#undef Color
#undef Line
#undef Plane
#endif
"""

open_header = """
#ifndef GL_H
#define GL_H


#ifdef __cplusplus
extern "C" {
#endif

"""

close_header = """
#ifdef __cplusplus
}
#endif

// end GL_H
#endif

"""

prefix_macro = """

#ifndef PGL_PREFIX
#define PGL_PREFIX(x) x
#endif

"""



def cvector_impl(type_name):
    s = "#define CVECTOR_" + type_name + "_IMPLEMENTATION\n"
    s += open("cvector_" + type_name + ".h").read()
    return s


def get_gl_funcs():
    functions = [l.rstrip() for l in open("gl_function_list.c").readlines() if l.startswith('gl')]
    return functions




if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate the single-file-header portablegl.h from component source files")
    parser.add_argument("-u", "--unsafe", help="Generate with unsafe gl implementation (no error checking)", action='store_true')
    parser.add_argument("-p", "--prefix_macro", help="Wrap all gl functions in a macro to enable prefix/namespacing", action='store_true')
    #parser.add_argument("-m", "--macros", help="Use macros to include vectors", action='store_true')
    args = parser.parse_args()
    print(args, file=sys.stderr)

    gl_impl = ''
    gl_prototypes = open("gl_prototypes.h").read() + open("gl_stubs.h").read()

    if not args.unsafe:
        gl_h = open("portablegl.h", "w")
        gl_impl = open("gl_impl.c").read()
    else:
        gl_h = open("portablegl_unsafe.h", "w")
        gl_impl = open("gl_impl_unsafe.c").read()

    gl_impl += open("gl_stubs.c").read()

    if args.prefix_macro:
        for func in get_gl_funcs():
            # Really hacky ... I could just move pglCreateProgram (and other non-standard funcs) to pgl_ext.c/h
            # Or I could just bite the bullet and use a regex...
            gl_impl = gl_impl.replace(" "+func+"(", " PGL_PREFIX("+func+")(")
            gl_impl = gl_impl.replace("\t"+func+"(", "\tPGL_PREFIX("+func+")(")
            gl_prototypes = gl_prototypes.replace(" "+func+"(", " PGL_PREFIX("+func+")(")

        gl_prototypes = prefix_macro + gl_prototypes


    gl_h.write("/*\n")

    gl_h.write(open("header_docs.txt").read())

    gl_h.write("*/\n")

    #gl_h.write(open("config_macros.h").read())
    gl_h.write(prefix_types)
    gl_h.write(prefix_suffix_glsl)
    gl_h.write(open_header)
    gl_h.write(macros)

    gl_h.write(open("crsw_math.h").read())

    # we actually use this for output_buf in gl_types
    gl_h.write(open("cvector_float.h").read())

    # maybe an option to add vectors for commonly used types
    # vec2/3/4, ivec3 etc.?
    #gl_h.write(open("cvector_vec3.h").read())
    #gl_h.write(open("cvector_vec4.h").read())

    gl_h.write(open("gl_types.h").read())

    # could put these as macros at top of glcontext.h
    # could also remove the redundancies
    gl_h.write(open("cvector_glVertex_Array.h").read())
    gl_h.write(open("cvector_glBuffer.h").read())
    gl_h.write(open("cvector_glTexture.h").read())
    gl_h.write(open("cvector_glProgram.h").read())
    gl_h.write(open("cvector_glVertex.h").read())

    gl_h.write(open("glcontext.h").read())

    gl_h.write(open("gl_glsl.h").read())

    gl_h.write(open("pgl_std_shaders.h").read())

    gl_h.write(gl_prototypes)
    gl_h.write(open("pgl_ext.h").read())


    gl_h.write(close_header)


    # part 2
    gl_h.write("#ifdef PORTABLEGL_IMPLEMENTATION\n\n")

    gl_h.write(open("crsw_math.c").read())

    # handle #define'ing CVECTOR_TYPE_IMPLEMENTATION for each ...
    # maybe I should stick to using cvector_macro.h and use the macros
    gl_h.write(cvector_impl("glVertex_Array"))
    gl_h.write(cvector_impl("glBuffer"))
    gl_h.write(cvector_impl("glTexture"))
    gl_h.write(cvector_impl("glProgram"))
    gl_h.write(cvector_impl("glVertex"))

    gl_h.write(cvector_impl("float"))
    #gl_h.write(cvector_impl("vec3"))
    #gl_h.write(cvector_impl("vec4"))

    # maybe this should go last? does it matter beyond aesthetics?
    gl_h.write(open("gl_internal.c").read())

    gl_h.write(gl_impl)

    gl_h.write(open("gl_glsl.c").read())
    gl_h.write(open("pgl_ext.c").read())
    gl_h.write(open("pgl_std_shaders.c").read())



    # This prevents it from being implemented twice if you include it again in the same
    # file (usually indirectly, ie including the header for some OpenGL helper functions that
    # in turn have to include PortableGL).  Otherwise you'd have to be much more careful about
    # the order and dependencies of inclusions which is a pain
    gl_h.write("#undef PORTABLEGL_IMPLEMENTATION\n")
    gl_h.write("#undef CVECTOR_float_IMPLEMENTATION\n")
    gl_h.write("#endif\n")


    gl_h.write(unprefix_types)
    gl_h.write(unprefix_suffix_glsl)



