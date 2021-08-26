
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

//#define MANGLE_TYPES
//#define PORTABLEGL_IMPLEMENTATION
//#include "GLObjects.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480



SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

// Write tests in separate files and include here
#include "hello_triangle.c"
#include "hello_interpolation.c"
#include "blending.cpp"
#include "stencil.cpp"


typedef struct pgl_test
{
	char name[50];
	void (*test_func)(int, char**, void*);
} pgl_test;

#define NUM_TESTS 4

pgl_test test_suite[NUM_TESTS] =
{
	{ "hello_triangle", hello_triangle },
	{ "hello_interpolation", hello_interpolation },
	{ "blend_test", blend_test },
	{ "stencil_test", stencil_test }

};





int main(int argc, char** argv)
{

	char strbuf[1024];

	for (int i=0; i<NUM_TESTS; ++i) {
		bbufpix = NULL; // should already be NULL since global/static but meh

		if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000)) {
			puts("Failed to initialize glContext");
			exit(0);
		}

		set_glContext(&the_Context);

		test_suite[i].test_func(0, NULL, NULL);

		snprintf(strbuf, 1024, "test_output/%s.png", test_suite[i].name);
		// TODO handle resizing tests
		if(!stbi_write_png(strbuf, WIDTH, HEIGHT, 4, bbufpix, WIDTH*4)) {
			printf("Failed to write %s\n", strbuf);
		}


		free_glContext(&the_Context);
	}


	return 0;
}

