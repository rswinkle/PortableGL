#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#include <SDL.h>

#include <glm_matstack.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


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

float z = -5;
float x_rot, y_rot;
float x_speed, y_speed;
int filter;

GLuint texture;

int main(int argc, char** argv)
{
	setup_context();

	//My_Uniforms the_uniforms;
	pgl_uniforms the_uniforms;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	if (!load_texture2D("../../media/textures/crate.gif", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_TRUE, NULL, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}

	float vertices[] = {
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

	float texcoords[] = {
		// Front face
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		// Back face
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		// Top face
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		// Bottom face
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		// Right face
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		// Left face
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0
	};

	GLuint triangles[] = {
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

	GLuint vert_buf;
	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint elem_buf;
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);


	// create standard programs and select one we need
	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	GLuint program = std_shaders[PGL_SHADER_TEX_REPLACE];

	glUseProgram(program);

	pglSetUniform(&the_uniforms);
	the_uniforms.tex0 = texture;

	matrix_stack mat_stack;
	mat4 proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);
	mat_stack.load_mat(proj_mat);

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

		x_rot += x_speed*elapsed / 1000.0f;
		y_rot += y_speed*elapsed / 1000.0f;

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		mat_stack.push();
		mat_stack.translate(0, 0, z);

		mat_stack.rotate(glm::radians(x_rot), 1, 0, 0);
		mat_stack.rotate(glm::radians(y_rot), 0, 1, 0);

		// TODO add another function or figure out how to convert to pgl_mat4
		// better
		//the_uniforms.mvp_mat = mat_stack.get_matrix();
		//mat_stack.get_matrix(*(mat4*)&the_uniforms.mvp_mat);
		mat4 tmp = mat_stack.get_matrix();
		memcpy(the_uniforms.mvp_mat, glm::value_ptr(tmp), sizeof(mat4));

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, sizeof(triangles)/sizeof(triangles[0]), GL_UNSIGNED_INT, 0);

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

	window = SDL_CreateWindow("Lesson 6", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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

			if (sc == SDL_SCANCODE_ESCAPE) {
				return 0;
			} else if (sc == SDL_SCANCODE_F) {
				filter = (filter + 1) % 2;
				// NOTE, only magfilter is meaningful, and it applies universally
				// regardless of the texture size relative to display size
				//
				// Using DSA functions since I can
				if (filter == 0) {
					puts("GL_NEAREST\n");
					glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				} else {
					puts("GL_LINEAR\n");
					glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}

			} else if (sc == SDL_SCANCODE_LEFT) {
				y_speed -= 1;
			} else if (sc == SDL_SCANCODE_RIGHT) {
				y_speed += 1;
			} else if (sc == SDL_SCANCODE_UP) {
				x_speed -= 1;
			} else if (sc == SDL_SCANCODE_DOWN) {
				x_speed += 1;
			}
		}
	}

	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	if (state[SDL_SCANCODE_PAGEUP]) {
		z += 0.05;
	} else if (state[SDL_SCANCODE_PAGEDOWN]) {
		z -= 0.05;
	}

	return 1;
}






