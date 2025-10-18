
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <stb_image.h>

#include <stdio.h>

#define WIDTH 640
#define HEIGHT 640


pix_t* bbufpix;

glContext the_Context;

// Write tests in separate files and include here
#include "hello_triangle.c"
#include "hello_indexing.c"
#include "hello_interpolation.c"
#include "lines.c"
#include "polygon_modes.c"
#include "front_back_mode_culling.c"
#include "clipping.c"
#include "scissoring.c"
#include "viewport.c"
#include "blending.cpp"
#ifndef PGL_NO_STENCIL
#include "stencil.cpp"
#endif
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
#include "color_masking.c"

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


	{ "line_interpolation", line_interpolation, 0 },

	// Should think of better names
	{ "client_arrays1", hello_triangle, 1 },
	{ "client_arrays2", hello_interpolation, 1 },
	{ "client_arrays3", hello_indexing, 4 },
	{ "client_arrays4", hello_indexing, 5 },
	{ "client_arrays5", hello_indexing, 6 },
	{ "client_arrays6", hello_indexing, 7 },
	{ "client_arrays7", test_instancing, 2 },
	{ "client_arrays8", test_instancing, 3 },

	{ "polygon_modes", polygon_modes, 1 },
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

	{ "clip_projection", clip_pers_proj, 0 },

	// scissoring with different polygon_modes
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

	// testing GL_LINES and GL_POINTS
	{ "scissor4_pnt_ln", scissoring_test4, 0 },
	{ "scissor4_pnt_ln8", scissoring_test4, 1 },
	{ "scissor4_pnt_ln32", scissoring_test4, 2 },

	{ "viewport_fill", test_viewport, 0 },
	{ "viewport_line", test_viewport, 1 },
	{ "viewport_point", test_viewport, 2 },

	{ "blend_test", blend_test },
#ifndef PGL_NO_STENCIL
	{ "stencil_test", stencil_test },
#endif
	{ "primitives_test", primitives_test },

	{ "zbuf_depthoff", zbuf_test },
	{ "zbuf_depthon", zbuf_test, 1 },
	{ "zbuf_depthon_greater", zbuf_test, 2 },
	{ "zbuf_depthon_fliprange", zbuf_test, 3 },
	{ "zbuf_depthon_maskoff", zbuf_test, 4 },

	{ "test_edges", test_edges },

	{ "texture2D_nearest", test_tex2D_filtering, 0 },
	{ "texture2D_linear", test_tex2D_filtering, 1 },

	{ "texture2D_repeat", test_tex2D_wrap_modes, 0 },
	{ "texture2D_clamp2edge", test_tex2D_wrap_modes, 1 },
	{ "texture2D_mirroredrepeat", test_tex2D_wrap_modes, 2 },

	{ "texrect_nearest", test_texrect_filtering, 0 },
	{ "texrect_linear", test_texrect_filtering, 1 },

	{ "texrect_repeat", test_texrect_wrap_modes, 0 },
	{ "texrect_clamp2edge", test_texrect_wrap_modes, 1 },
	{ "texrect_mirroredrepeat", test_texrect_wrap_modes, 2 },

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	{ "texture2D_clamp2border", test_tex2D_wrap_modes, 3 },
	{ "texrect_clamp2border", test_texrect_wrap_modes, 3 },
#endif

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
	{ "multidraw_elements", test_multidraw, 1 },

	{ "color_masking", color_masking }
};

#define NUM_TESTS (sizeof(test_suite)/sizeof(*test_suite))


int run_test(int i);
int find_test(char* name);
int write_diff_img(pix_t* e_img, pix_t* img, int w, int h, char* filename);
int write_diff_txt(pix_t* e_img, pix_t* img, int w, int h, char* filename);


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
		printf("All %d tests passed\n", total);
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
	char test_name[256];

	bbufpix = NULL;

#ifdef PGL_ABGR32
	// TODO in the long term there may be a test where the output differs
	// between D16 and D24S8...
	snprintf(test_name, sizeof(test_name), "%s", test_suite[i].name);
#else
	snprintf(test_name, sizeof(test_name), "%s_" PGL_PIX_STR, test_suite[i].name);
#endif

	printf("%s\n====================\n", test_name);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

	test_suite[i].test_func(test_suite[i].num, NULL, NULL);

	// TODO handle resizing tests
	// TODO better way to handle test output of multiple pixel formats
	// Right now I'm just outputing them as YA (grayscale + alpha) which obviously does
	// not map to RGB at all and does not let me really verify that it looks the way
	// it's supposed to (ie same as 32-bit except possibly slightly different shades since
	// fewer colors).  I could do a manually conversion to 32-bit right here but I
	// would still need separate expected outputs and it would just make the tests slower.
	// Somethign to think about
	snprintf(strbuf, 1024, "test_output/%s.png", test_name);
	if (!stbi_write_png(strbuf, WIDTH, HEIGHT, sizeof(pix_t), bbufpix, WIDTH*sizeof(pix_t))) {
		printf("Failed to write %s\n", strbuf);
	}

	// load expected output and compare
	snprintf(strbuf, 1024, "expected_output/%s.png", test_name);
	if (!(image = stbi_load(strbuf, &w, &h, &n, sizeof(pix_t)))) {
		fprintf(stdout, "Error loading image %s: %s\n\n", strbuf, stbi_failure_reason());
		free_glContext(&the_Context);
		return 0;  // not really a failure if nothing to compare
	}
	if (memcmp(image, bbufpix, w*h*sizeof(pix_t))) {
		printf("%s_" PGL_PIX_STR " FAILED\n", test_suite[i].name);
		failed = 1;

		snprintf(strbuf, 1024, "test_output/%s_diff.png", test_name);

		write_diff_img((pix_t*)image, bbufpix, w, h, strbuf);

		snprintf(strbuf, 1024, "test_output/%s_diff.txt", test_name);
		write_diff_txt((pix_t*)image, bbufpix, w, h, strbuf);
	}

	stbi_image_free(image);



	free_glContext(&the_Context);

	return failed;
}

int write_diff_img(pix_t* e_img, pix_t* img, int w, int h, char* filename)
{
	pix_t* diff_px = (pix_t*)calloc(w*h, sizeof(pix_t));
	if (!diff_px) {
		return 0;
	}

	for (int i=0; i<w*h; ++i) {
		if (img[i] != e_img[i]) {
#if PGL_BITDEPTH == 16
			diff_px[i] = UINT16_MAX;
#else
			diff_px[i] = UINT32_MAX;
#endif
		}
	}

	if (!stbi_write_png(filename, w, h, sizeof(pix_t), diff_px, w*sizeof(pix_t))) {
		perror("Failed to write diff img");
		printf("Failed to write %s\n", filename);
		return 0;
	}

	free(diff_px);

	return 1;
}

// Could do a PPM image where the RGB values are the absolute value
// of the difference, but that leaves out alpha and it ascii ppms
// are ridiculously large, wasteful when only a few pixels are
// different
int write_diff_txt(pix_t* e_img, pix_t* img, int w, int h, char* filename)
{
	FILE* txt_file = fopen(filename, "w");
	if (!txt_file) {
		perror("Error opening output ppm for writing");
		return 0;
	}
	int sx, sy, ex, ey;
	u8* p, *q;
	int j;
	for (int i=0; i<w*h; i++) {
		if (img[i] != e_img[i]) {
			sx = i % w;
			sy = i / w;
			for (j=i; j<w*h; j++) {
				if (img[j] == e_img[j]) {
					break;
				}
			}
			--j;
			ex = j % w;
			ey = j / w;
			fprintf(txt_file, "Diff from (%d, %d) to (%d, %d):\n", sx, sy, ex, ey);
			for (int k=i; k<=j; k++) {
				p = (u8*)&img[k];
				//q = (u8*)&e_img[k];
				//fprintf(txt_file, "(%03d %03d %03d %03d) ", p[0]-q[0], p[1]-q[1], p[2]-q[2], p[3]-q[3]);
#if PGL_BITDEPTH == 16
				// NOTE these are just 8 byte values not the actual channels ie
				// RGB565...would have to think about how to structure that
				fprintf(txt_file, "(%03d %03d) ", p[0], p[1]);
#else
				fprintf(txt_file, "(%03d %03d %03d %03d) ", p[0], p[1], p[2], p[3]);
#endif
			}
			fputc('\n', txt_file);
			for (int k=i; k<=j; k++) {
				q = (u8*)&e_img[k];
#if PGL_BITDEPTH == 16
				// NOTE these are just 8 byte values not the actual channels ie
				// RGB565...would have to think about how to structure that
				fprintf(txt_file, "(%03d %03d) ", q[0], q[1]);
#else
				fprintf(txt_file, "(%03d %03d %03d %03d) ", q[0], q[1], q[2], q[3]);
#endif
			}
			fputc('\n', txt_file);

			i = j;
		}
	}
	fclose(txt_file);

	return 1;
}
