#include "rsw_math.h"

#define PGL_UNSAFE
#define PGL_PREFIX_TYPES
#define PGL_DISABLE_COLOR_MASK
#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"


#include <vector>
#include <iostream>
#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL.h>

#define WIDTH 640
#define HEIGHT 640

using namespace std;

using rsw::vec4;
using rsw::vec3;
using rsw::mat4;

vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

pix_t* bbufpix;

glContext the_Context;

typedef struct pgl_perftest
{
	char name[50];
	float (*test_func)(int, int, char**, void*);
	int frames;
	int num;
	char** argc;
	const void* data;
} pgl_perftest;

// put above includes because we use this in every one
// TODO why do I even have this?  Do I really need to be able
// to exit in the middle of performance tests?  Would taking it
// out make them noticeably faster?
int handle_events();

#include "point_perf.cpp"
#include "line_perf.cpp"
#include "triangle_perf.cpp"
#include "triangle_interp.cpp"
#include "tri_clip_perf.cpp"
#include "blending_perf.cpp"
#include "texture_perf.cpp"

#ifndef TEX_PATH
#define TEX_PATH "../media/textures/tex04.jpg"
#endif

pgl_perftest test_suite[] =
{
	{ "points_perf", points_perf, 5000, 1 },
	{ "pointsize_perf", points_perf, 5000, 4 },
	{ "lines_perf", lines_perf, 2000, 1 },
	{ "lines8_perf", lines_perf, 1000, 8 },
	{ "lines16_perf", lines_perf, 250, 16 },
	{ "triangles_perf", tris_perf, 300 },
	{ "tri_interp_perf", tris_interp_perf, 300 },
	{ "tri_clipxy_perf", tri_clipxy_perf, 4000 },
	{ "tri_clipz_perf", tri_clipz_perf, 4000 },
	{ "tri_clipxyz_perf", tri_clipxyz_perf, 4000 },
	{ "blend_perf", blend_test, 2000 },
	{ "tex_nearest_perf", texture_perf, 1000, 0, NULL, TEX_PATH },
	{ "tex_linear_perf", texture_perf, 1000, 1, NULL, TEX_PATH },
	{ "drawframe_tex_nearest_perf", drawframe_tex_perf, 1000, 0, NULL, TEX_PATH },
	{ "drawframe_tex_linear_perf", drawframe_tex_perf, 1000, 1, NULL, TEX_PATH },
	{ "drawgeometry_tex_nearest_perf", drawgeometry_tex_perf, 1000, 0, NULL, TEX_PATH },
	{ "drawgeometry_tex_linear_perf", drawgeometry_tex_perf, 1000, 1, NULL, TEX_PATH }

};

#define NUM_TESTS (sizeof(test_suite)/sizeof(*test_suite))

void cleanup_SDL2();
void setup_SDL2();
int find_test(char* name);
void run_test(int i);



int main(int argc, char** argv)
{
	setup_SDL2();

	int total;
	if (argc == 1) {
		printf("Running %ld tests...\n", NUM_TESTS);
		for (int i=0; i<NUM_TESTS; ++i) {
			run_test(i);
		}
	} else {
		int found;
		total = argc-1;
		printf("Attempting to run %d tests...\n", total);
		for (int i=1; i<argc; i++) {
			found = find_test(argv[i]);
			if (found >= 0) {
				run_test(found);
			} else {
				printf("Error: could not find test '%s', skipping\n", argv[i]);
			}
		}
	}

	cleanup_SDL2();

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

int handle_events()
{
	SDL_Event e;
	int sc;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT)
			exit(0);
		if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;
		
			if (sc == SDL_SCANCODE_ESCAPE) {
				exit(0);
			} else if (sc == SDL_SCANCODE_RETURN) {
				return 1;
			}
		}
	}
	return 0;
}

void setup_SDL2()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("performance_tests", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	// Just assuring that SDL2 defaults to blending off for renderer and textures
	/*SDL_BlendMode blendmode;*/
	/*if (!SDL_GetRenderDrawBlendMode(ren, &blendmode)) {*/
	/*	printf("Renderer blendmode: %u\n", blendmode);*/
	/*}*/
	/*if (!SDL_GetTextureBlendMode(tex, &blendmode)) {*/
	/*	printf("Image blendmode: %u\n", blendmode);*/
	/*}*/
	/*SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);*/
	/*SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);*/
	/*if (!SDL_GetRenderDrawBlendMode(ren, &blendmode)) {*/
	/*	printf("Renderer blendmode: %u\n", blendmode);*/
	/*}*/
	/*if (!SDL_GetTextureBlendMode(tex, &blendmode)) {*/
	/*	printf("Image blendmode: %u\n", blendmode);*/
	/*}*/
}

void cleanup_SDL2()
{
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}


void run_test(int i)
{
	bbufpix = NULL;
	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

	// TODO I really shouldn't use "argc" as just a flag. Maybe I should just remove argv and rename argc
	// in all the prototypes...
	float fps = test_suite[i].test_func(test_suite[i].frames, test_suite[i].num, NULL, (void*)test_suite[i].data);

	printf("%s: %.3f FPS\n", test_suite[i].name, fps);

	free_glContext(&the_Context);
}
