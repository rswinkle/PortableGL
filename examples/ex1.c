#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <SDL.h>

#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

#ifndef FPS_EVERY_N_SECS
#define FPS_EVERY_N_SECS 1
#endif

#define FPS_DELAY (FPS_EVERY_N_SECS*1000)

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	vec4 v_color;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();


void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };


	GLuint program = pglCreateProgram(identity_vs, uniform_color_fs, 0, NULL, GL_FALSE);
	glUseProgram(program);

	My_Uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	the_uniforms.v_color = Red;

	// Not actually needed for PGL but there's
	// no default vao in core profile ...
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	int old_time = 0, new_time = 0, counter = 0;
	int ms;
	while (handle_events()) {
		new_time = SDL_GetTicks();

		counter++;
		ms = new_time - old_time;
		if (ms >= FPS_DELAY) {
			printf("%d  %f FPS\n", ms, counter*1000.0f/ms);
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	// Not actually necessary but for completeness
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &triangle);
	glDeleteProgram(program);

	cleanup();

	return 0;
}


void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = vertex_attribs[0];
}

void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("c_ex1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	// bbufpix already NULL since global/static
	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}

void cleanup()
{
	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

int handle_events()
{
	SDL_Event e;
	int sc;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return 0;
		} else if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;

			if (sc == SDL_SCANCODE_ESCAPE)
				return 0;
		}
	}
	return 1;
}

