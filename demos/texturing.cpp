
#include "rsw_math.h"

#define MANGLE_TYPES
#include "gltools.h"

#define PORTABLEGL_IMPLEMENTATION
#include "GLObjects.h"

#include "stb_image.h"

#include <iostream>
#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

using rsw::vec4;
using rsw::vec3;
using rsw::vec2;
using rsw::mat4;


typedef struct My_Uniforms
{
	mat4 mvp_mat;
	GLuint tex;
	vec4 v_color;
	float time;
	
} My_Uniforms;

bool handle_events();
void cleanup();
void setup_context();
void setup_gl_data();


void normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void texture_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void texture_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void tex_rect_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void tex_array_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void tex_array_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* SDL_tex;

u32* bbufpix;

glContext the_Context;

My_Uniforms the_uniforms;
int tex_index;
int tex_filter;

#define NUM_TEXTURES 5
GLuint textures[NUM_TEXTURES];

GLuint tex_array_shader;
GLuint texture_replace;
GLuint tex_rect_shader;

mat4 scale_mat, rot_mat;

int main(int argc, char** argv)
{

	setup_context();

	//can't turn off C++ destructors
	{

	GLenum smooth[2] = { SMOOTH, SMOOTH };

	float points[] =
	{
		-0.5,  0.5, -0.1,
		-0.5, -0.5, -0.1,
		 0.5,  0.5, -0.1,
		 0.5, -0.5, -0.1
	};

	float tex_coords[] =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 0.0,
		0.0, 511.0,
		511.0, 0.0,
		511.0, 511.0
	};

	rsw::Color test_texture[9];
	for (int i=0; i<9; ++i) {
		if (i % 2)
			test_texture[i] = rsw::Color(0, 0, 0, 255);
		else
			test_texture[i] = rsw::Color(255, 255, 255, 255);
	}



	glGenTextures(NUM_TEXTURES, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (!load_texture2D("../media/textures/test1.jpg", GL_NEAREST, GL_NEAREST, GL_MIRRORED_REPEAT, false, false)) {
		puts("failed to load texture");
		return 0;
	}


	glBindTexture(GL_TEXTURE_2D, textures[1]);

	if (!load_texture2D("../media/textures/test2.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, false, false)) {
		puts("failed to load texture");
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);

	glBindTexture(GL_TEXTURE_2D_ARRAY, textures[3]);

	int frames;
	if (!(frames = load_texture2D_array_gif("../media/textures/Nathan_Fillion_awesome_smaller2.gif", GL_NEAREST, GL_NEAREST, GL_REPEAT))) {
		puts("failed to load texture");
		return 0;
	}

	glBindTexture(GL_TEXTURE_RECTANGLE, textures[4]);
	if (!load_texture_rect("../media/textures/tex04.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, false)) {
		puts("failed to load texture");
		return 0;
	}

	mat4 identity;

	Buffer square(1);
	square.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*12, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer tex_buf(1);
	tex_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*16, tex_coords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint normal_shader = pglCreateProgram(normal_vs, normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(normal_shader);
	pglSetUniform(&the_uniforms);

	tex_array_shader = pglCreateProgram(tex_array_vs, tex_array_fs, 2, smooth, GL_FALSE);
	glUseProgram(tex_array_shader);
	pglSetUniform(&the_uniforms);

	tex_rect_shader = pglCreateProgram(texture_replace_vs, tex_rect_fs, 2, smooth, GL_FALSE);
	glUseProgram(tex_rect_shader);
	pglSetUniform(&the_uniforms);

	texture_replace = pglCreateProgram(texture_replace_vs, texture_replace_fs, 2, smooth, GL_FALSE);
	glUseProgram(texture_replace);
	pglSetUniform(&the_uniforms);


	the_uniforms.v_color = Red;
	the_uniforms.mvp_mat = identity;

	tex_index = 0;
	tex_filter = 0;
	the_uniforms.tex = textures[tex_index];


	glClearColor(0, 0, 0, 1);


	unsigned int orig_time = SDL_GetTicks();
	unsigned int old_time = 0, new_time=0, counter = 0;

	while (1) {
		if (handle_events())
			break;

		++counter;
		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		// TODO time/depth 0-1 or 0-(frames-1)?
		// the former if I tried to use texture3D, the latter for texture2dArray
		the_uniforms.time = (((new_time - orig_time)/50) % frames);// /(float)frames;


		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		SDL_UpdateTexture(SDL_tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, SDL_tex, NULL, NULL);
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

void texture_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	((vec2*)vs_output)[0] = ((vec4*)vertex_attribs)[2].xy(); //tex_coords

	*(vec4*)&builtins->gl_Position = u->mvp_mat * ((vec4*)vertex_attribs)[0];

}

void texture_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 tex_coords = ((vec3*)fs_input)[0];
	GLuint tex = ((My_Uniforms*)uniforms)->tex;


	builtins->gl_FragColor = texture2D(tex, tex_coords.x, tex_coords.y);
	//print_vec4(stdout, builtins->gl_FragColor, "\n");
}

void tex_rect_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 tex_coords = ((vec3*)fs_input)[0];
	GLuint tex = ((My_Uniforms*)uniforms)->tex;


	builtins->gl_FragColor = texture_rect(tex, tex_coords.x, tex_coords.y);
	//print_vec4(stdout, builtins->gl_FragColor, "\n");
}

void tex_array_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	((vec2*)vs_output)[0] = ((vec4*)vertex_attribs)[2].xy(); //uv tex_coords (layer is uniform in fs)

	*(vec4*)&builtins->gl_Position = u->mvp_mat * ((vec4*)vertex_attribs)[0];

}

void tex_array_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	vec3 tex_coords = { fs_input[0], fs_input[1], u->time };
	GLuint tex = u->tex;

	builtins->gl_FragColor = texture2DArray(tex, tex_coords.x, tex_coords.y, tex_coords.z);
	//print_vec4(builtins->gl_FragColor, "\n");
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("Texturing", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	SDL_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
	set_glContext(&the_Context);
}

void cleanup()
{
	free_glContext(&the_Context);

	SDL_DestroyTexture(SDL_tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

bool handle_events()
{
	SDL_Event event;
	SDL_Scancode sc;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			sc = event.key.keysym.scancode;
			
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				return true;
			case SDL_SCANCODE_1:
				tex_index = (tex_index + 1) % NUM_TEXTURES;
				the_uniforms.tex = textures[tex_index];
				if (tex_index == NUM_TEXTURES - 2) {
					glUseProgram(tex_array_shader);
					glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
				} else if (tex_index == NUM_TEXTURES - 1) {
					glUseProgram(tex_rect_shader);
					glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, sizeof(GLfloat)*8);
				} else {
					glUseProgram(texture_replace);
					glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
				}
				break;
			case SDL_SCANCODE_F:
			{
				int filter;
				if (tex_filter == 0) {
					puts("Switching to GL_LINEAR");
					filter = GL_LINEAR;
				} else {
					puts("Switching to GL_NEAREST");
					filter = GL_NEAREST;
				}
				for (int i=0; i<NUM_TEXTURES-2; ++i) {
					glBindTexture(GL_TEXTURE_2D, textures[i]);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
				}
				glBindTexture(GL_TEXTURE_2D_ARRAY, textures[NUM_TEXTURES-2]);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filter);

				glBindTexture(GL_TEXTURE_RECTANGLE, textures[NUM_TEXTURES-1]);
				glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, filter);

				tex_filter = !tex_filter;
			}
			default:
				;
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

#define MOVE_SPEED DEG_TO_RAD(30)

	mat4 tmp;
	
	if (state[SDL_SCANCODE_LEFT]) {
		rsw::load_rotation_mat4(tmp, vec3(0, 0, 1), time * MOVE_SPEED);
		the_uniforms.mvp_mat = the_uniforms.mvp_mat * tmp;
	}
	if (state[SDL_SCANCODE_RIGHT]) {
		rsw::load_rotation_mat4(tmp, vec3(0, 0, 1), -time * MOVE_SPEED);
		the_uniforms.mvp_mat = the_uniforms.mvp_mat * tmp;
	}
	if (state[SDL_SCANCODE_UP]) {
		the_uniforms.mvp_mat = the_uniforms.mvp_mat * rsw::scale_mat4(1.01, 1.01, 1);
	}
	if (state[SDL_SCANCODE_DOWN]) {
		the_uniforms.mvp_mat = the_uniforms.mvp_mat * rsw::scale_mat4(0.99, 0.99, 1);
	}

	last_time = cur_time;

	return false;
}
