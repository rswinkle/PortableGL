
#define PGL_PREFIX_TYPES
#define PGL_EXCLUDE_STUBS
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <SDL.h>

#include <glm_matstack.h>
#include <glm/glm.hpp>

#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

#ifndef FPS_EVERY_N_SECS
#define FPS_EVERY_N_SECS 1
#endif

#define FPS_DELAY (FPS_EVERY_N_SECS*1000)

using glm::vec4;
using glm::vec3;
using glm::mat4;

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

pix_t* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();


void smooth_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	My_Uniforms the_uniforms;

	float triangle_verts[] = {
 	 	 0.0,  1.0,  0.0,
		-1.0, -1.0,  0.0,
 	 	 1.0, -1.0,  0.0
	};

	float triangle_colors[] = {
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};

	float square_verts[] = {
		 1.0,  1.0,  0.0,
		-1.0,  1.0,  0.0,
		 1.0, -1.0,  0.0,
		-1.0, -1.0,  0.0
	};

	float square_colors[] = {
		0.5, 0.5, 1.0, 1.0,
		0.5, 0.5, 1.0, 1.0,
		0.5, 0.5, 1.0, 1.0,
		0.5, 0.5, 1.0, 1.0
	};

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint tri_buf, tri_color_buf;
	glGenBuffers(1, &tri_buf);
	glGenBuffers(1, &tri_color_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tri_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_verts), triangle_verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, tri_color_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_colors), triangle_colors, GL_STATIC_DRAW);

	GLuint square_buf, square_color_buf;
	glGenBuffers(1, &square_buf);
	glGenBuffers(1, &square_color_buf);
	glBindBuffer(GL_ARRAY_BUFFER, square_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(square_verts), square_verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, square_color_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(square_colors), square_colors, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	GLenum smooth[4] = { PGL_SMOOTH4 };
	GLuint program = pglCreateProgram(smooth_vs, smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);

	mat4 proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	matrix_stack mat_stack;
	mat_stack.load_mat(proj_mat);

	int old_time = 0, new_time=0, counter = 0;
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

		mat_stack.push();
		mat_stack.translate(-1.5, 0, -7.0);
		the_uniforms.mvp_mat = mat_stack.get_matrix();

		glBindBuffer(GL_ARRAY_BUFFER, tri_buf);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, tri_color_buf);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		mat_stack.translate(3.0, 0.0, 0.0);

		the_uniforms.mvp_mat = mat_stack.get_matrix();

		glBindBuffer(GL_ARRAY_BUFFER, square_buf);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, square_color_buf);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		mat_stack.pop();

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void smooth_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((pgl_vec4*)vs_output)[0] = vertex_attribs[1]; //color

	My_Uniforms* u = (My_Uniforms*)uniforms;

	*(vec4*)&builtins->gl_Position = u->mvp_mat * ((vec4*)vertex_attribs)[0];
}

void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((vec4*)fs_input)[0];
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Lesson 1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("SDL_CreateWindow error: %s\n", SDL_GetError());
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
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


