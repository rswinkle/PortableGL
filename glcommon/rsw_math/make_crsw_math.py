
# generate crsw_math.h from component libraries/files

import sys, os, glob, argparse

# https://stackoverflow.com/questions/1883980/find-the-nth-occurrence-of-substring-in-a-string 
def find_nth(haystack, needle, n):
        start = haystack.find(needle)
        while start >= 0 and n > 1:
                start = haystack.find(needle, start+len(needle))
                n -= 1
        return start

def get_header(filename):
        header_text = open(filename).read()
        # 3rd because 1st is the CVEC_SIZE_T #endif, 2nd is CVEC_SZ #endif
        start = find_nth(header_text, "#endif", 3) + 6 # #endif of extern "C"
        end = header_text.find("#ifdef", start) #ifdef close of extern "C"
        return header_text[start:end]



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate the single-file-header portablegl.h from component source files")
    parser.add_argument("-p", "--pgl_version", help="generate version for PortableGL", action='store_true')

    args = parser.parse_args()
    print(args, file=sys.stderr)

    math_hs = ''

    if args.pgl_version:
        math_hs += open('inc_macros_pgl.h').read()
    else:
        math_hs += open('inc_macros.h').read()

    math_hs += open('cvec2.h').read()
    math_hs += open('cvec3.h').read()
    math_hs += open('cvec4.h').read()

    math_hs += open('civec2.h').read()
    math_hs += open('civec3.h').read()
    math_hs += open('civec4.h').read()

    # TODO should I include uvec in pgl?
    math_hs += open('cuvec2.h').read()
    math_hs += open('cuvec3.h').read()
    math_hs += open('cuvec4.h').read()

    math_hs += open('cbvec2.h').read()
    math_hs += open('cbvec3.h').read()
    math_hs += open('cbvec4.h').read()

    if not args.pgl_version:
        math_hs += open('cdvec2.h').read()
        math_hs += open('cdvec3.h').read()
        math_hs += open('cdvec4.h').read()
        

    math_hs += open('vec_conversions.h').read()

    math_hs += open('cmat234.h').read()

    math_hs += open('misc.h').read()

    math_hs += open('cglsl.h').read()
    
    
    math_h = open("crsw_math.h", "w")
    math_h.write(math_hs)

    math_h.write("\n#endif\n")

    math_h.close()

    math_cs = ''
    if not args.pgl_version:
        math_cs = '#include "crsw_math.h"\n\n'

    math_cs += open('cvec2.c').read()
    math_cs += open('cvec3.c').read()
    math_cs += open('cvec4.c').read()

    math_cs += open('civec2.c').read()
    math_cs += open('civec3.c').read()
    math_cs += open('civec4.c').read()

    math_cs += open('cuvec2.c').read()
    math_cs += open('cuvec3.c').read()
    math_cs += open('cuvec4.c').read()

    math_cs += open('cbvec2.c').read()
    math_cs += open('cbvec3.c').read()
    math_cs += open('cbvec4.c').read()

    if not args.pgl_version:
        math_cs += open('cdvec2.c').read()
        math_cs += open('cdvec3.c').read()
        math_cs += open('cdvec4.c').read()

    math_cs += open('vec_conversions.c').read()

    math_cs += open('cmat234.c').read()

    math_cs += open('misc.c').read()

    #math_cs += open('cglsl.h').read()

    math_c = open("crsw_math.c", "w")
    math_c.write(math_cs)

    math_c.close()
