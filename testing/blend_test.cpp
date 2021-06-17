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
using rsw::mat4;

vec4 Red(1.0f, 0.0f, 0.0f, 1.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 1.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 1.0f);
vec4 Black(0.0f, 0.0f, 0.0f, 1.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

mat4 translate;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 v_color;
} My_Uniforms;

void cleanup();
bool handle_events();
void setup_context();
void setup_gl_data();


void normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{

	setup_context();

	//can't turn off C++ destructors
	{

	float points[] = {
		-0.75, 0.75, 0,
		-0.75, 0.25, 0,
		-0.25, 0.75, 0,
		-0.25, 0.25, 0,

		 0.25, 0.75, 0,
		 0.25, 0.25, 0,
		 0.75, 0.75, 0,
		 0.75, 0.25, 0,
						
		-0.75, -0.25, 0,
		-0.75, -0.75, 0,
		-0.25, -0.25, 0,
		-0.25, -0.75, 0,

		 0.25, -0.75, 0,
		 0.25, -0.25, 0,
		 0.75, -0.75, 0,
		 0.75, -0.25, 0,
	
		-0.15, 0.15, -0.1,
		-0.15, -0.15, -0.1,
		 0.15, 0.15, -0.1,
		 0.15, -0.15, -0.1
	};


	My_Uniforms the_uniforms;
	mat4 identity;

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*60, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(normal_vs, normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	set_uniform(&the_uniforms);

	the_uniforms.v_color = Red;
	the_uniforms.mvp_mat = identity; //only necessary in C of course but that's what I want, to have it work as both


	glClearColor(1, 1, 1, 1);

	SDL_Event e;
	bool quit = false;

	unsigned int old_time = 0, new_time=0, counter = 0;
	unsigned int frame_cap_old = 0, frame_cap_new = 0, last_frame = 0;
	float frame_time = 0;

	while (1) {
		if (handle_events())
			break;

		new_time = SDL_GetTicks();
		frame_time = (new_time - last_frame)/1000.0f;
		last_frame = new_time;

		counter++;
		if (!(counter % 300)) {
			printf("%d  %f FPS\n", new_time-old_time, 300000/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}


		glClear(GL_COLOR_BUFFER_BIT);
		
		the_uniforms.mvp_mat = identity;
		the_uniforms.v_color = Red;
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		the_uniforms.v_color = Green;
		glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
		the_uniforms.v_color = Blue;
		glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
		the_uniforms.v_color = Black;
		glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		the_uniforms.mvp_mat = translate;
		SET_VEC4(the_uniforms.v_color, 1, 0, 0, 0.5f);
		glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);

		glDisable(GL_BLEND);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}


	}

	cleanup();

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

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	ren = NULL;
	tex = NULL;

	SDL_Window* window = SDL_CreateWindow("swrenderer", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

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

bool handle_events()
{
	SDL_Event event;
	SDL_Keysym keysym;

	bool remake_projection = false;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			keysym = event.key.keysym;
			//printf("%c %c\n", event.key.keysym.scancode, event.key.keysym.sym);
			//printf("Physical %s key acting as %s key",
      	  	 // 	  SDL_GetScancodeName(keysym.scancode),
      	  	  //	  SDL_GetKeyName(keysym.sym));
			
			if (keysym.sym == SDLK_ESCAPE) {
				return true;
			}

			break; //sdl_keydown

		case SDL_QUIT:
			return true;
			break;
		}
	}

	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	static unsigned int last_time = 0, cur_time;

	cur_time = SDL_GetTicks();
	float time = (cur_time - last_time)/1000.0f;

#define MOVE_SPEED (240.0/WIDTH)

#ifndef ROW_MAJOR
	if (state[SDL_SCANCODE_LEFT]) {
		translate[12] -= time * MOVE_SPEED;
	}
	if (state[SDL_SCANCODE_RIGHT]) {
		translate[12] += time * MOVE_SPEED;
	}
	if (state[SDL_SCANCODE_UP]) {
		translate[13] += time * MOVE_SPEED;
	}
	if (state[SDL_SCANCODE_DOWN]) {
		translate[13] -= time * MOVE_SPEED;
	}
#else
	if (state[SDL_SCANCODE_LEFT]) {
		translate[3] -= time * MOVE_SPEED;
	}
	if (state[SDL_SCANCODE_RIGHT]) {
		translate[3] += time * MOVE_SPEED;
	}
	if (state[SDL_SCANCODE_UP]) {
		translate[7] += time * MOVE_SPEED;
	}
	if (state[SDL_SCANCODE_DOWN]) {
		translate[7] -= time * MOVE_SPEED;
	}
#endif

	last_time = cur_time;

	return false;
}
