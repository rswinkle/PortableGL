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
	float (*test_func)(int, char**, void*);
	int num;
} pgl_perftest;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 v_color;
} My_Uniforms;


float points_perf(int argc, char** argv, void* data);

#define NUM_TESTS 1

pgl_perftest test_suite[NUM_TESTS] =
{
	{ "points_perf", points_perf, 4000 },
//	{ "lines_perf", lines_perf, 3000 },
	//{ "triangles_perf", tris_perf, 1000 }

};


void cleanup_SDL2();
void setup_SDL2();
int handle_events();


void normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_SDL2();

	float fps;

	for (int i=0; i<NUM_TESTS; ++i) {
		if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
			puts("Failed to initialize glContext");
			exit(0);
		}
		set_glContext(&the_Context);

		fps = test_suite[i].test_func(test_suite[i].num, NULL, NULL);

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




void normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_Position = *((mat4*)uniforms) * ((vec4*)vertex_attribs)[0];
}

void normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
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


float points_perf(int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> points;
#define NUM_POINTS 10000

	for (int i=0; i < 10000; ++i) {
		points.push_back(vec3(rsw::rand_float(-1, 1), rsw::rand_float(-1, 1), -1));
	}

	My_Uniforms the_uniforms;
	mat4 identity;

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*points.size(), &points[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(normal_vs, normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;
	the_uniforms.mvp_mat = identity; //only necessary in C of course but that's what I want, to have it work as both

	glClearColor(0, 0, 0, 1);

	int start, end;
	start = SDL_GetTicks();
	for (int j=0; j<argc; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_POINTS, 0, NUM_POINTS);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();

	// return FPS
	return argc / ((end-start)/1000.0f);
}

