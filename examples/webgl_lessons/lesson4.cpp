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

	float pyramid_verts[] = {
		// Front face
		 0.0,  1.0,  0.0,
		-1.0, -1.0,  1.0,
		 1.0, -1.0,  1.0,
		// Right face
		 0.0,  1.0,  0.0,
		 1.0, -1.0,  1.0,
		 1.0, -1.0, -1.0,
		// Back face
		 0.0,  1.0,  0.0,
		 1.0, -1.0, -1.0,
		-1.0, -1.0, -1.0,
		// Left face
		 0.0,  1.0,  0.0,
		-1.0, -1.0, -1.0,
		-1.0, -1.0,  1.0
	};

	float pyramid_colors[] = {
		// Front face
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		// Right face
		1.0, 0.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		// Back face
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		// Left face
		1.0, 0.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 1.0, 0.0, 1.0
	};

	float cube_verts[] = {
		// Front face
		-1.0, -1.0,  1.0,
		 1.0, -1.0,  1.0,
		 1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		// Back face
		-1.0, -1.0, -1.0,
		-1.0,  1.0, -1.0,
		 1.0,  1.0, -1.0,
		 1.0, -1.0, -1.0,
		// Top face
		-1.0,  1.0, -1.0,
		-1.0,  1.0,  1.0,
		 1.0,  1.0,  1.0,
		 1.0,  1.0, -1.0,
		// Bottom face
		-1.0, -1.0, -1.0,
		 1.0, -1.0, -1.0,
		 1.0, -1.0,  1.0,
		-1.0, -1.0,  1.0,
		// Right face
		 1.0, -1.0, -1.0,
		 1.0,  1.0, -1.0,
		 1.0,  1.0,  1.0,
		 1.0, -1.0,  1.0,
		// Left face
		-1.0, -1.0, -1.0,
		-1.0, -1.0,  1.0,
		-1.0,  1.0,  1.0,
		-1.0,  1.0, -1.0
	};

	float cube_colors[] = {
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,

		1.0, 1.0, 0.0, 1.0,
		1.0, 1.0, 0.0, 1.0,
		1.0, 1.0, 0.0, 1.0,
		1.0, 1.0, 0.0, 1.0,

		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,

		1.0, 0.5, 0.5, 1.0,
		1.0, 0.5, 0.5, 1.0,
		1.0, 0.5, 0.5, 1.0,
		1.0, 0.5, 0.5, 1.0,

		1.0, 0.0, 1.0, 1.0,
		1.0, 0.0, 1.0, 1.0,
		1.0, 0.0, 1.0, 1.0,
		1.0, 0.0, 1.0, 1.0,

		0.0, 0.0, 1.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 0.0, 1.0, 1.0, 
		0.0, 0.0, 1.0, 1.0 
	};

	unsigned short cube_triangles[] = {
		0, 1, 2,      0, 2, 3,    // Front face
		4, 5, 6,      4, 6, 7,    // Back face
		8, 9, 10,     8, 10, 11,  // Top face
		12, 13, 14,   12, 14, 15, // Bottom face
		16, 17, 18,   16, 18, 19, // Right face
		20, 21, 22,   20, 22, 23  // Left face
	};

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint pyramid_buf, pyramid_color_buf;
	glGenBuffers(1, &pyramid_buf);
	glGenBuffers(1, &pyramid_color_buf);
	glBindBuffer(GL_ARRAY_BUFFER, pyramid_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid_verts), pyramid_verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, pyramid_color_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid_colors), pyramid_colors, GL_STATIC_DRAW);

	GLuint cube_buf, cube_color_buf;
	glGenBuffers(1, &cube_buf);
	glGenBuffers(1, &cube_color_buf);
	glBindBuffer(GL_ARRAY_BUFFER, cube_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, cube_color_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_colors), cube_colors, GL_STATIC_DRAW);

#ifdef USE_EBO
	GLuint cube_tri_buf;
	glGenBuffers(1, &cube_tri_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_tri_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangles), cube_triangles, GL_STATIC_DRAW);
#endif


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	GLenum smooth[4] = { PGL_SMOOTH4 };
	GLuint program = pglCreateProgram(smooth_vs, smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);

	mat4 proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	matrix_stack mat_stack;
	mat_stack.load_mat(proj_mat);

	float r_pyramid = 0, r_cube = 0;
	float elapsed;

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	int last_time = SDL_GetTicks();
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
		elapsed = new_time - last_time;
		last_time = new_time;

		r_pyramid += 90 * (elapsed / 1000.0f);
		r_cube -= 75 * (elapsed / 1000.0f);

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		mat_stack.push();

		mat_stack.translate(-1.5, 0, -7.0);
		mat_stack.push();

		mat_stack.rotate(glm::radians(r_pyramid), 0, 1, 0);
		the_uniforms.mvp_mat = mat_stack.get_matrix();

		glBindBuffer(GL_ARRAY_BUFFER, pyramid_buf);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, pyramid_color_buf);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 12);
		mat_stack.pop();

		mat_stack.translate(3.0, 0.0, 0.0);
		mat_stack.rotate(glm::radians(r_cube), 1, 1, 1);

		the_uniforms.mvp_mat = mat_stack.get_matrix();

		glBindBuffer(GL_ARRAY_BUFFER, cube_buf);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, cube_color_buf);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);

#ifdef USE_EBO
		// count is number of indices ie number of vertices
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
		//glDrawElements(GL_TRIANGLES, sizeof(cube_triangles)/sizeof(unsigned short), GL_UNSIGNED_SHORT, 0);

		// indices is a *byte* offset, not an index
		//glDrawElements(GL_TRIANGLES, 33, GL_UNSIGNED_SHORT, (void*)(3*sizeof(short)));

#else
		// Apparently, if no Element Array Buffer is bound, you can pass in an array of indices directly
		// which probably explains why that stupid parameter is a void* instead of a GLsizei or GLuint
		// So to test this make sure USE_EBO is not defined
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, cube_triangles);
#endif

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

	window = SDL_CreateWindow("Lesson 4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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




