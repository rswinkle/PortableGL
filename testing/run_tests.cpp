
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

//#define MANGLE_TYPES
//#define PORTABLEGL_IMPLEMENTATION
//#include "GLObjects.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <stb_image.h>

#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 640



SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

// Write tests in separate files and include here
#include "hello_triangle.c"
#include "hello_indexing.c"
#include "hello_interpolation.c"
#include "blending.cpp"
#include "stencil.cpp"
#include "primitives.cpp"
#include "test_edges.cpp"
#include "zbuf_test.cpp"
#include "test_texturing.cpp"
#include "test_tex1D.cpp"
#include "mapped_vbuffer.cpp"
#include "map_namedvbuffer.cpp"
#include "pglbufferdata.cpp"
#include "pglteximage2D.cpp"
#include "pglteximage1D.cpp"
#include "test_unpackalignment.cpp"
#include "instancing.cpp"

typedef struct pgl_test
{
	char name[50];
	void (*test_func)(int, char**, void*);
	int num;
} pgl_test;

#define NUM_TESTS 33

pgl_test test_suite[NUM_TESTS] =
{
	{ "hello_triangle", hello_triangle },
	{ "hello_indexing", hello_indexing },
	{ "hello_interpolation", hello_interpolation },
	{ "blend_test", blend_test },
	{ "stencil_test", stencil_test },
	{ "primitives_test", primitives_test },
	{ "zbuf_depthoff", zbuf_test },
	{ "zbuf_depthon", zbuf_test, 1 },
	{ "zbuf_depthon_greater", zbuf_test, 2 },
	{ "zbuf_depthon_fliprange", zbuf_test, 3 },

	{ "test_edges", test_edges },

	{ "texture2D_nearest", test_texturing, 0 },
	{ "texture2D_linear", test_texturing, 1 },
	{ "texture2D_repeat", test_texturing, 2 },
	{ "texture2D_clamp2edge", test_texturing, 3 },
	{ "texture2D_mirroredrepeat", test_texturing, 4 },

	{ "texrect_nearest", test_texturing, 5 },
	{ "texrect_linear", test_texturing, 6 },
	{ "texrect_repeat", test_texturing, 7 },
	{ "texrect_clamp2edge", test_texturing, 8 },
	{ "texrect_mirroredrepeat", test_texturing, 9 },

	{ "texture1D_nearest", test_texture1D, 0 },
	{ "texture1D_linear", test_texture1D, 1 },
	{ "texture1D_repeat", test_texture1D, 2 },
	{ "texture1D_clamp2edge", test_texture1D, 3 },
	{ "texture1D_mirroredrepeat", test_texture1D, 4 },

	{ "map_vbuffer", mapped_vbuffer },
	{ "map_nvbuffer", mapped_nvbuffer },

	{ "pglbufferdata", pglbufdata_test },

	{ "pglteximage2D", test_pglteximage2D },
	{ "pglteximage1D", test_pglteximage1D },

	{ "unpack_alignment", test_unpackalignment },

	{ "instancing", test_instancing }

};


int run_test(int i);
int find_test(char* name);


int main(int argc, char** argv)
{
	int have_failure = 0;
	if (argc == 1) {
		for (int i=0; i<NUM_TESTS; ++i) {
			have_failure |= run_test(i);
			putchar('\n');
		}
	} else {
		int found;
		for (int i=1; i<argc; i++) {
			found = find_test(argv[i]);
			if (found >= 0)
				have_failure |= run_test(found);
			else
				printf("Error: could not find test '%s', skipping\n", argv[i]);
		}
	}

	if (!have_failure) {
		puts("All tests passed");
	}



	return 0;
}

int find_test(char* name)
{
	for (int i=0; i<NUM_TESTS; i++) {
		if (!strcmp(name, test_suite[i].name)) {
			return i;
		}
	}
	return -1;
}



int run_test(int i)
{
	char strbuf[1024];
	int failed = 0;
	int w, h, n;
	u8* image;

	bbufpix = NULL;

	printf("%s\n====================\n", test_suite[i].name);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

	set_glContext(&the_Context);

	test_suite[i].test_func(test_suite[i].num, NULL, NULL);

	snprintf(strbuf, 1024, "test_output/%s.png", test_suite[i].name);
	// TODO handle resizing tests
	if(!stbi_write_png(strbuf, WIDTH, HEIGHT, 4, bbufpix, WIDTH*4)) {
		printf("Failed to write %s\n", strbuf);
	}

	// load expected output and compare
	snprintf(strbuf, 1024, "expected_output/%s.png", test_suite[i].name);
	if (!(image = stbi_load(strbuf, &w, &h, &n, STBI_rgb_alpha))) {
		fprintf(stdout, "Error loading image %s: %s\n\n", strbuf, stbi_failure_reason());
		free_glContext(&the_Context);
		return 0;  // not really a failure if nothing to compare
	}
	if (memcmp(image, bbufpix, w*h*4)) {
		printf("%s FAILED\n", test_suite[i].name);
		failed = 1;
	}

	stbi_image_free(image);



	free_glContext(&the_Context);

	return failed;
}
