#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <stdio.h>

#include <SDL.h>

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
	mat4 mvp_mat;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();


void smooth_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	GLenum smooth[4] = { PGL_SMOOTH4 };

	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };

	float color_array[] = { 1.0, 0.0, 0.0, 1.0,
	                        0.0, 1.0, 0.0, 1.0,
	                        0.0, 0.0, 1.0, 1.0 };

	mat4 proj_mat, trans_mat, rot_mat, vp_mat;
	mat4 save_rot = IDENTITY_MAT4();
	mat4 tmp_mat = IDENTITY_MAT4();

	My_Uniforms the_uniforms;

	make_perspective_matrix(proj_mat, DEG_TO_RAD(45), WIDTH/HEIGHT, 1, 20);
	translation_mat4(trans_mat, 0, 0, -5);
	mult_mat4_mat4(vp_mat, proj_mat, trans_mat);

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint colors;
	glGenBuffers(1, &colors);
	glBindBuffer(GL_ARRAY_BUFFER, colors);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, color_array, GL_STATIC_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint program = pglCreateProgram(smooth_vs, smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);

	int old_time = 0, new_time=0, counter = 0;
	int ms = 0;
	int last_frame;
	float frame_time = 0;
	while (handle_events()) {
		new_time = SDL_GetTicks();
		counter++;
		ms = new_time - old_time;
		if (ms >= FPS_DELAY) {
			printf("%d  %f FPS\n", ms, counter*1000.0f/ms);
			old_time = new_time;
			counter = 0;
		}

		frame_time = (new_time - last_frame)/1000.0f;
		last_frame = new_time;

		glClear(GL_COLOR_BUFFER_BIT);

		vec3 y_axis = { 0, 1, 0 };
		load_rotation_mat4(rot_mat, y_axis, DEG_TO_RAD(30)*frame_time);
		mult_mat4_mat4(tmp_mat, rot_mat, save_rot);

		memcpy(save_rot, tmp_mat, sizeof(mat4));
		mult_mat4_mat4(the_uniforms.mvp_mat, vp_mat, save_rot);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void smooth_vs(float* vs_output, vec4* v_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec4*)vs_output)[0] = v_attribs[4]; //color

	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), v_attribs[0]);
}

void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((vec4*)fs_input)[0];
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("c_ex3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

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



