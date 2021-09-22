#include "rsw_math.h"

#define MANGLE_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "GLObjects.h"


#include <vector>
#include <iostream>
#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480

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

u32* bbufpix;

glContext the_Context;

typedef struct pgl_perftest
{
	char name[50];
	float (*test_func)(int, int, char**, void*);
	int frames;
	int num;
} pgl_perftest;

// put above includes because we use this in every one
int handle_events();

#include "point_perf.cpp"
#include "line_perf.cpp"
#include "triangle_perf.cpp"
#include "triangle_interp.cpp"


#define NUM_TESTS 5

pgl_perftest test_suite[NUM_TESTS] =
{
	{ "points_perf", points_perf, 4000, 1 },
	{ "pointsize_perf", points_perf, 4000, 4 },
	{ "lines_perf", lines_perf, 2000 },
	{ "triangles_perf", tris_perf, 300 },
	{ "tri_interp_perf", tris_interp_perf, 300 }

};


void cleanup_SDL2();
void setup_SDL2();


int main(int argc, char** argv)
{
	setup_SDL2();

	float fps;

	for (int i=0; i<NUM_TESTS; ++i) {
		bbufpix = NULL;
		if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
			puts("Failed to initialize glContext");
			exit(0);
		}
		set_glContext(&the_Context);

		fps = test_suite[i].test_func(test_suite[i].frames, test_suite[i].num, NULL, NULL);

		printf("%s: %.3f FPS\n", test_suite[i].name, fps);

		free_glContext(&the_Context);
	}

	cleanup_SDL2();	

	return 0;
}

int handle_events()
{
	SDL_Event e;
	int sc;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT)
			return 1;
		if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;
		
			if (sc == SDL_SCANCODE_ESCAPE) {
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

	window = SDL_CreateWindow("performance_tests", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

}

void cleanup_SDL2()
{
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}


