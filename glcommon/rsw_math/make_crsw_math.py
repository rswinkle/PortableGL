
# generate crsw_math.h from component libraries/files

import sys, os, glob, argparse



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate the single-file-header portablegl.h from component source files")
    #parser.add_argument("-p", "--prefix_macro", help="Wrap all gl functions in a macro to enable prefix/namespacing", action='store_true')

    args = parser.parse_args()
    print(args, file=sys.stderr)

    math_hs = ''

    math_h = open("crsw_math.h", "w")

    math_hs += open('inc_macros.h').read()
    math_hs += open('inc_macrs.h').read()

    
    
