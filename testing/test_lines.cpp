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

vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;
bool line_smooth, depth_test;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 v_color;
} My_Uniforms;

void cleanup();
void setup_context();

bool user_exit();

void normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{

	setup_context();

	//can't turn off C++ destructors
	{

	float points[] = { -0.9,  0.7,  0.5,
	                    0.9,  0.7, -0.5,
	                    0.7,  0.9,  0.5,
	                    0.7, -0.9, -0.5,
	                    0.9, -0.7,  0.5,
	                   -0.9, -0.7, -0.5,
	                   -0.7, -0.9,  0.5,
	                   -0.7,  0.9, -0.5
	};


	My_Uniforms the_uniforms;
	mat4 identity;

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*24, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(normal_vs, normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;
	the_uniforms.mvp_mat = identity; //only necessary in C of course but that's what I want, to have it work as both

	line_smooth = false;
	depth_test = false;

	//glViewport(0, 0, 320, 240);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SDL_Event e;

	unsigned int old_time = 0, new_time=0, counter = 0;
	unsigned int frame_cap_old = 0, frame_cap_new = 0, last_frame = 0;
	float frame_time = 0;

	while (1) {
		if (user_exit())
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


		if (!depth_test)
			glClear(GL_COLOR_BUFFER_BIT);
		else
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays(GL_LINES, 0, 2);
		the_uniforms.v_color = Red;
		glDrawArrays(GL_LINES, 2, 2);
		the_uniforms.v_color = Green;
		glDrawArrays(GL_LINES, 4, 2);
		the_uniforms.v_color = Red;
		glDrawArrays(GL_LINES, 6, 2);
		the_uniforms.v_color = Blue;

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

	window = SDL_CreateWindow("test_lines", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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


bool user_exit()
{
	static SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			//printf("%c %c %c\n", event.key.keysym.scancode, event.key.keysym.sym, event.key.keysym.unicode);
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				return true;
				break;

			case SDLK_l:
				if (line_smooth)
					glDisable(GL_LINE_SMOOTH);
				else
					glEnable(GL_LINE_SMOOTH);

				line_smooth = !line_smooth;
				break;

			case SDLK_d:
				if (depth_test)
					glDisable(GL_DEPTH_TEST);
				else
					glEnable(GL_DEPTH_TEST);

				depth_test = !depth_test;
				break;
		/*	
			else if (event.key.keysym.sym == SDLK_v) {
				provoking_mode = (provoking_mode == GL_LAST_VERTEX_CONVENTION) ? GL_FIRST_VERTEX_CONVENTION : GL_LAST_VERTEX_CONVENTION;
				glProvokingVertex(provoking_mode);
			} else if (event.key.keysym.sym == SDLK_i) {
				if (interp_mode == SMOOTH) {
					printf("noperspective\n");
					pglSetInterp(3, noperspective);
					interp_mode = NOPERSPECTIVE;
				} else {
					pglSetInterp(3, smooth);
					interp_mode = SMOOTH;
					printf("smooth\n");
				}
			} else if (event.key.keysym.sym == SDLK_s) {
				if (myshader) {
					printf("default shader\n");
					myshader = 0;
					glUseProgram(myshader);
				} else {
					printf("interp shader\n");
					myshader = 1;
					glUseProgram(myshader);
				}
			}
			*/
			}



			break;
		case SDL_QUIT:
			return true;
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				printf("window size %d x %d\n", event.window.data1, event.window.data2);
			//	rsw::make_perspective_matrix(MVP, DEG_TO_RAD(40), float(event.window.data1)/float(event.window.data2), 1, 30);

				break;
			}
			/*SDL_Surface* tmp = SDL_SetVideoMode(event.resize.w, event.resize.h, 32, SDL_SWSURFACE | SDL_RESIZABLE);
			if(!tmp) {
				cerr<<"Failed ot create surface\nusing old one";
				return 0;
			} else {
				screen = tmp;
				reset back buf too
			}
			*/
			//TODO re-make backbuf and screen with new size
			//add function to glContext that reallcs all glFramebuffers
			break;
		}
	}
	return false;
}
