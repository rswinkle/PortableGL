import sys, os, glob


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

def get_c_file(filename):
	c_text = open(filename).read()
	ret_str = c_text[c_text.find("IMPLEMENTATION")+15:c_text.find("#if defined(")]

	start_loc = c_text.find("#endif\n\ncvector_") + 8
	ret_str += c_text[start_loc:c_text.rfind("#endif")]
	return ret_str


shared_header = """
#ifndef CVEC_SIZE_T
#include <stdlib.h>
#define CVEC_SIZE_T size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_T cvec_sz;
#endif
"""

shared_impl = """
#if defined(CVEC_MALLOC) && defined(CVEC_FREE) && defined(CVEC_REALLOC)
/* ok */
#elif !defined(CVEC_MALLOC) && !defined(CVEC_FREE) && !defined(CVEC_REALLOC)
/* ok */
#else
#error "Must define all or none of CVEC_MALLOC, CVEC_FREE, and CVEC_REALLOC."
#endif

#ifndef CVEC_MALLOC
#include <stdlib.h>
#define CVEC_MALLOC(sz)      malloc(sz)
#define CVEC_REALLOC(p, sz)  realloc(p, sz)
#define CVEC_FREE(p)         free(p)
#endif

#ifndef CVEC_MEMMOVE
#include <string.h>
#define CVEC_MEMMOVE(dst, src, sz)  memmove(dst, src, sz)
#endif

#ifndef CVEC_ASSERT
#include <assert.h>
#define CVEC_ASSERT(x)       assert(x)
#endif
"""




vector_files = [
	"cvector_glVertex_Array.h",
	"cvector_glBuffer.h",
	"cvector_glTexture.h",
	"cvector_glProgram.h",
	"cvector_glVertex.h"
]

hfile_str = shared_header
cfile_str = shared_impl
for f in vector_files:
	#file_string = open(sys.argv[1]).read();
	hfile_str += get_header(f)
	cfile_str += get_c_file(f)


hfile = open("cvector_combined.h", "wt")
cfile = open("cvector_combined.c", "wt")

hfile.write(hfile_str)
hfile.close()

cfile.write(cfile_str)
cfile.close()

