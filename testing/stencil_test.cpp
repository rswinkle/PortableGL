#include "rsw_math.h"

#define MANGLE_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "GLObjects.h"


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

typedef struct My_Uniforms
{
	mat4 mvp_mat;
} My_Uniforms;

void cleanup();
void setup_context();
void setup_gl_data();

void smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };

	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };


	float color_array[] = { 1.0, 0.0, 0.0, 1.0,
	                        0.0, 1.0, 0.0, 1.0,
	                        0.0, 0.0, 1.0, 1.0 };

	mat4 proj_mat, trans_mat, save_rot, rot_mat, vp_mat;
	My_Uniforms the_uniforms;

	rsw::make_perspective_matrix(proj_mat, DEG_TO_RAD(45), WIDTH/HEIGHT, 1, 20);
	trans_mat = rsw::translation_mat4(0, 0, -5);
	vp_mat = proj_mat * trans_mat;

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer colors(1);
	colors.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*12, color_array, GL_STATIC_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(smooth_vs, smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(myshader);
	set_uniform(&the_uniforms);

	glUseProgram(0);
	set_uniform(&the_uniforms);

	glClearColor(0, 0, 0, 1);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);


	SDL_Event e;
	bool quit = false;

	unsigned int old_time = 0, new_time=0, counter = 0;
	unsigned int last_frame = 0;
	float frame_time = 0;

	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				quit = true;
			if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				quit = true;
			if (e.type == SDL_MOUSEBUTTONDOWN)
				quit = true;
		}

		new_time = SDL_GetTicks();
		frame_time = (new_time - last_frame)/1000.0f;
		last_frame = new_time;

		counter++;
		if (!(counter % 300)) {
			printf("%d  %f FPS\n", new_time-old_time, 300000/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}


		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glUseProgram(myshader);
		load_rotation_mat4(rot_mat, vec3(0, 1, 0), DEG_TO_RAD(30)*frame_time);
		save_rot = rot_mat * save_rot;

		the_uniforms.mvp_mat = vp_mat * save_rot;

		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glUseProgram(0);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);

		the_uniforms.mvp_mat = the_uniforms.mvp_mat * rsw::scale_mat4(1.2, 1.2, 1.2);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		//Render the scene
		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));

		//SDL_RenderClear(ren); //not necessary
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}

void smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec4*)vs_output)[0] = ((vec4*)vertex_attribs)[4]; //color

	*(vec4*)&builtins->gl_Position = *((mat4*)uniforms) * ((vec4*)vertex_attribs)[0];
}

void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((vec4*)fs_input)[0];
}


void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("stencil_test", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	//about 10% faster to write directly to texture
	//int pitch;
	//SDL_LockTexture(tex, NULL, (void**)&bbufpix, &pitch);
	//printf("pitch of texture = %d  compared to %d\n", pitch, WIDTH*4);
	//assert(pitch == WIDTH*4);
	//SDL_UnlockTexture(tex);
	
	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
	set_glContext(&the_Context);
}

void cleanup()
{

	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

