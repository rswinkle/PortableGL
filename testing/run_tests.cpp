
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <stb_image.h>

#include <stdio.h>

#define WIDTH 640
#define HEIGHT 640


u32* bbufpix;

glContext the_Context;

// Write tests in separate files and include here
#include "hello_triangle.c"
#include "hello_indexing.c"
#include "hello_interpolation.c"
#include "polygon_modes.c"
#include "front_back_mode_culling.c"
#include "clipping.c"
#include "scissoring.c"
#include "viewport.c"
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
#include "glinstanceid.cpp"
#include "baseinstance.cpp"
#include "multidraw.cpp"

typedef struct pgl_test
{
	char name[50];
	void (*test_func)(int, char**, void*);
	int num;
} pgl_test;

pgl_test test_suite[] =
{
	{ "hello_triangle", hello_triangle, 0 },

	{ "hello_indexing0", hello_indexing, 0 },
	{ "hello_indexing1", hello_indexing, 1 },
	{ "hello_indexing2", hello_indexing, 2 },
	{ "hello_indexing3", hello_indexing, 3 },

	{ "hello_interpolation", hello_interpolation, 0 },

	// Should think of better names
	{ "client_arrays1", hello_triangle, 1 },
	{ "client_arrays2", hello_interpolation, 1 },
	{ "client_arrays3", hello_indexing, 4 },
	{ "client_arrays4", hello_indexing, 5 },
	{ "client_arrays5", hello_indexing, 6 },
	{ "client_arrays6", hello_indexing, 7 },
	{ "client_arrays7", test_instancing, 2 },
	{ "client_arrays8", test_instancing, 3 },

	{ "polygon_modes", polygon_modes, 0 },
	{ "polygon_modes_lw_ps", polygon_modes, 8 },

	{ "cull_off", front_back_culling, 0 },
	{ "cull_on", front_back_culling, 1 },
	{ "cull_on_CW_front", front_back_culling, 2 },
	{ "cull_front_on", front_back_culling, 3 },
	{ "cull_front_on_CW_front", front_back_culling, 4 },
	{ "cull_front_and_back", front_back_culling, 5 },

	{ "front_pnt_back_fill", front_back_culling, 6 },
	{ "front_fill_back_pnt", front_back_culling, 7 },
	{ "front_line_back_fill", front_back_culling, 8 },
	{ "front_fill_back_line", front_back_culling, 9 },
	{ "front_line_back_point", front_back_culling, 10 },
	{ "front_line_back_point_CW", front_back_culling, 11 },

	{ "clip_xy_fill", clip_xy, 0 },
	{ "clip_xy_line", clip_xy, 1 },
	{ "clip_xy_point", clip_xy, 2 },
	{ "clip_xy_line_8", clip_xy, 3 },
	{ "clip_xy_point_8", clip_xy, 4 },
	{ "clip_xy_line_32", clip_xy, 5 },
	{ "clip_xy_point_32", clip_xy, 6 },

	{ "clip_z_fill", clip_z, 0 },
	{ "clip_z_line", clip_z, 1 },
	{ "clip_z_point", clip_z, 2 },
	{ "clip_z_line_8", clip_z, 3 },
	{ "clip_z_point_8", clip_z, 4 },
	{ "clip_z_line_32", clip_z, 5 },
	{ "clip_z_point_32", clip_z, 6 },

	{ "clip_pnts_lns", clip_pnts_lns, 0 },
	{ "clip_pnts_lns8", clip_pnts_lns, 1 },
	{ "clip_pnts_lns32", clip_pnts_lns, 2 },

	{ "depth_clamp", clip_z, 7},

	{ "scissor1_fill", scissoring_test1, 0 },
	{ "scissor1_ln", scissoring_test1, 1 },
	{ "scissor1_pnt", scissoring_test1, 2 },
	{ "scissor1_ln8", scissoring_test1, 3 },
	{ "scissor1_pnt8", scissoring_test1, 4 },

	{ "scissor2_fill", scissoring_test2, 0 },
	{ "scissor2_ln", scissoring_test2, 1 },
	{ "scissor2_pnt", scissoring_test2, 2 },
	{ "scissor2_ln8", scissoring_test2, 3 },
	{ "scissor2_pnt8", scissoring_test2, 4 },

	{ "scissor_clear_color", scissoring_test3 },

	{ "viewport_fill", test_viewport, 0 },
	{ "viewport_line", test_viewport, 1 },
	{ "viewport_point", test_viewport, 2 },

	{ "blend_test", blend_test },
	{ "stencil_test", stencil_test },
	{ "primitives_test", primitives_test },

	{ "zbuf_depthoff", zbuf_test },
	{ "zbuf_depthon", zbuf_test, 1 },
	{ "zbuf_depthon_greater", zbuf_test, 2 },
	{ "zbuf_depthon_fliprange", zbuf_test, 3 },
	{ "zbuf_depthon_maskoff", zbuf_test, 4 },

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

	{ "instancing_arrays", test_instancing, 0 },
	{ "instancing_elements", test_instancing, 1 },

	{ "baseinstance_arrays", test_baseinstance, 0 },
	{ "baseinstance_elements", test_baseinstance, 1 },

	// These test client elem buf and client eb with client va's
	// could name client_arrays 9 and 10?
	{ "baseinstance_elements2", test_baseinstance, 2 },
	{ "baseinstance_elements3", test_baseinstance, 3 },

	{ "instanceid", test_instanceid },

	{ "multidraw_arrays", test_multidraw, 0 },
	{ "multidraw_elements", test_multidraw, 1 }
};

#define NUM_TESTS (sizeof(test_suite)/sizeof(*test_suite))


int run_test(int i);
int find_test(char* name);


int main(int argc, char** argv)
{
	int n_fails = 0;
	int total;
	if (argc == 1) {
		total = NUM_TESTS;
		printf("Running %ld tests...\n", NUM_TESTS);
		for (int i=0; i<NUM_TESTS; ++i) {
			n_fails += run_test(i);
			putchar('\n');
		}
	} else {
		int found;
		total = argc-1;
		printf("Attempting to run %d tests...\n", total);
		for (int i=1; i<argc; i++) {
			found = find_test(argv[i]);
			if (found >= 0) {
				n_fails += run_test(found);
			} else {
				printf("Error: could not find test '%s', skipping\n", argv[i]);
			}
			putchar('\n');
		}
	}

	// TODO output nothing except on failure?
	if (!n_fails) {
		puts("All tests passed");
	} else {
		printf("Failed %d/%d tests\n", n_fails, total);
	}



	return n_fails;
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
