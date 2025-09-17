# generate all in one portablegl.h from component libraries/files

import sys, os, glob, argparse


# I hate that open_header is part of header_macros_start.h but
# close_header has to be its own thing.  Lack of symmetry..
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
    parser.add_argument("-p", "--prefix_macro", help="Wrap all gl functions in a macro to enable prefix/namespacing", action='store_true')
    #parser.add_argument("-m", "--macros", help="Use macros to include vectors", action='store_true')
    args = parser.parse_args()
    print(args, file=sys.stderr)

    gl_impl = ''
    gl_prototypes = open("gl_prototypes.h").read() + open("gl_stubs.h").read()

    gl_h = open("portablegl.h", "w")
    gl_impl = open("gl_impl.c").read()

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

    gl_h.write(open("header_macros_start.h").read())
    #gl_h.write(open_header)

    gl_h.write(open("crsw_math.h").read())

    # we actually use this for output_buf in gl_types..for now
    #gl_h.write(open("cvector_float.h").read())

    gl_h.write(open("gl_types.h").read())

    # could put these as macros at top of glcontext.h
    gl_h.write(open("cvector_combined.h").read())

    gl_h.write(open("glcontext.h").read())

    gl_h.write(open("gl_glsl.h").read())

    gl_h.write(open("pgl_std_shaders.h").read())

    gl_h.write(gl_prototypes)
    gl_h.write(open("pgl_ext.h").read())


    gl_h.write(close_header)


    # part 2
    gl_h.write("#ifdef PORTABLEGL_IMPLEMENTATION\n\n")

    gl_h.write(open("crsw_math.c").read())

    # maybe I should stick to using cvector_macro.h and use the macros
    # maybe an option to add vectors for commonly used types
    gl_h.write(open("cvector_combined.c").read())

    #gl_h.write(open("cvector_float.c").read())

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
    #gl_h.write("#undef CVECTOR_float_IMPLEMENTATION\n")
    gl_h.write("#endif\n")


    gl_h.write(open("close_pgl.h").read())



