#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#define CUTILS_SIZE_T int
#include "c_utils.h"

#include <SDL.h>

#include <glm_matstack.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#define my_chdir(x) chdir(x)
#elif defined(_WIN32)
#include <direct.h>
#define my_chdir(x) _chdir(x)
#endif

#define WIDTH 640
#define HEIGHT 480

#ifndef FPS_EVERY_N_SECS
#define FPS_EVERY_N_SECS 1
#endif

#define FPS_DELAY (FPS_EVERY_N_SECS*1000)

using namespace std;

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

pix_t* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mv_mat;
	mat4 proj_mat;

	GLuint tex;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();


void lesson10_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void lesson10_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
int load_world(const char* path, vector<vec3>& verts, vector<vec2>& texcoords);

float pitch, pitchRate;
float yaw, yawRate;

float xPos = 0;
float yPos = 0.4;
float zPos = -0.5;

float speed = 0;

GLuint texture;

int main(int argc, char** argv)
{
	setup_context();

	My_Uniforms the_uniforms;

	vector<vec3> verts;
	vector<vec2> texcoords;

	load_world("../../media/models/world.txt", verts, texcoords);


	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	if (!load_texture2D("../../media/textures/mud.gif", GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_TRUE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vert_buf;
	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(vec3), &verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, texcoords.size()*sizeof(vec2), &texcoords[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//GLuint elem_buf;
	//glGenBuffers(1, &elem_buf);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

	glBindVertexArray(0);


	GLenum vs_outs[] = { PGL_SMOOTH2 };
	GLuint program = pglCreateProgram(lesson10_vs, lesson10_fs, 2, vs_outs, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);
	the_uniforms.tex = texture;

	the_uniforms.proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	float elapsed;

	glEnable(GL_DEPTH_TEST);
	glClearColor(0,0,0,1);

	float joggingAngle = 0;

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

		if (speed) {
			xPos -= sin(glm::radians(yaw)) * speed * elapsed;
			zPos -= cos(glm::radians(yaw)) * speed * elapsed;

			joggingAngle += elapsed * 0.6; // 0.6 "fiddle factor" - makes it feel more realistic :-)
			yPos = sin(glm::radians(joggingAngle)) / 20 + 0.4;
		}
		yaw += yawRate * elapsed;
		pitch += pitchRate * elapsed;

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		the_uniforms.mv_mat = glm::rotate(mat4(1), glm::radians(-pitch), vec3(1, 0, 0));
		the_uniforms.mv_mat = glm::rotate(the_uniforms.mv_mat, glm::radians(-yaw), vec3(0, 1, 0));
		the_uniforms.mv_mat = glm::translate(the_uniforms.mv_mat, vec3(-xPos, -yPos, -zPos));

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, verts.size());

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void lesson10_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	*(vec2*)vs_output = *(vec2*)&vertex_attribs[2]; // smooth out texcoord = in_texcoord

	My_Uniforms* u = (My_Uniforms*)uniforms;

	*(vec4*)&builtins->gl_Position = u->proj_mat * u->mv_mat * ((vec4*)vertex_attribs)[0];
}

void lesson10_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];

	My_Uniforms* u = (My_Uniforms*)uniforms;

	builtins->gl_FragColor = texture2D(u->tex, tex_coords.x, tex_coords.y);
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Lesson 10", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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

	my_chdir(SDL_GetBasePath());
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
			} else if (sc == SDL_SCANCODE_LEFT) {
			} else if (sc == SDL_SCANCODE_RIGHT) {
			} else if (sc == SDL_SCANCODE_UP) {
			} else if (sc == SDL_SCANCODE_DOWN) {
			}
		}
	}

	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	if (state[SDL_SCANCODE_PAGEUP]) {
		pitchRate = 0.1;
	} else if (state[SDL_SCANCODE_PAGEDOWN]) {
		pitchRate = -0.1;
	} else {
		pitchRate = 0;
	}

	if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W]) {
		speed = 0.003;
	} else if (state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S]) {
		speed = -0.003;
	} else {
		speed = 0;
	}

	if (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_A]) {
		yawRate = 0.1;
	} else if (state[SDL_SCANCODE_RIGHT] || state[SDL_SCANCODE_D]) {
		yawRate = -0.1;
	} else {
		yawRate = 0;
	}

	return 1;
}


int load_world(const char* path, vector<vec3>& verts, vector<vec2>& texcoords)
{
	c_array lines, file_contents;
	
	if (!file_open_readlines(path, &lines, &file_contents)) {
		return 0;
	}
	// file is already closed

	vec3 v;
	vec2 t;

	char** lns = (char**)lines.data;
	for (int i=0; i<lines.len; i++) {
		// Honestly should just simplify the format to just geometry data
		// but for now we'll do this
		if (lns[i][0] && lns[i][0] != '/' && lns[i][0] != 'N') {
			if (5 != sscanf(lns[i], "%f %f %f %f %f", &v.x, &v.y, &v.z, &t.x, &t.y)) {
				printf("Error parsing %s line: %s\n", path, lns[i]);
				exit(1);
			}
			verts.push_back(v);
			texcoords.push_back(t);
		}
	}

	free(file_contents.data);
	free(lines.data);
	return 1;
}





