
#define MANGLE_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include "rsw_glframe.h"

#include <stdio.h>
#include <iostream>


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

using rsw::vec3;
using rsw::vec4;
using rsw::mat3;
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
	vec4 color;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events(GLFrame& camera_frame, unsigned int last_time, unsigned int cur_time);


void grass_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void simple_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);







int polygon_mode;


enum Control_Names {
	LEFT=0,
	RIGHT,
	FORWARD,
	BACK,
	UP,
	DOWN,
	TILTLEFT,
	TILTRIGHT,

	HIDECURSOR,
	FOVUP,
	FOVDOWN,
	ZMINUP,
	ZMINDOWN,
	PROVOKING,
	INTERPOLATION,
	SHADER,
	DEPTHTEST,
	POLYGONMODE,

	NCONTROLS
};


SDL_Scancode controls[NCONTROLS] =
{
	SDL_SCANCODE_A,
	SDL_SCANCODE_D,
	SDL_SCANCODE_W,
	SDL_SCANCODE_S,
	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_SPACE,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_E
};




int main(int argc, char** argv)
{
	setup_context();

	polygon_mode = 2;

	GLfloat grass_blade[] =
	{
		-0.3f,  0.0f,
		 0.3f,  0.0f,
		-0.20f, 1.0f,
		 0.1f,  1.3f,
		-0.05f, 2.3f,
		 0.0f,  3.3f
	};


	GLuint vao, buffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(grass_blade), grass_blade, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(grass_vs, simple_color_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	mat4 proj_mat, view_mat;
	make_perspective_matrix(proj_mat, DEG_TO_RAD(45), WIDTH/float(HEIGHT), 0.1f, 1000.0f);
	

	My_Uniforms the_uniforms;

	set_uniform(&the_uniforms);

	the_uniforms.color = Green;

	GLFrame camera(true);
	glEnable(GL_DEPTH_TEST);
	SDL_SetRelativeMouseMode(SDL_TRUE);



	unsigned int old_time = 0, new_time=0, counter = 0, last_time = SDL_GetTicks();
	while (1) {
		new_time = SDL_GetTicks();
		if (handle_events(camera, last_time, new_time))
			break;

		//cout << "origin = " << camera.origin << "\n\n";

		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		view_mat = camera.get_camera_matrix();
		//cout << view_mat << "\n\n";
		the_uniforms.mvp_mat = proj_mat * view_mat;
		//cout << the_uniforms.mvp_mat << "\n\n";
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, 256*256);

		last_time = new_time;
		++counter;

		//Render the scene
		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}




inline int random(int seed, int iterations)
{
	int val = seed;
	int n;
	for (n=0; n<iterations; ++n) {
		val = ((val >> 7) ^ (val << 9)) * 15485863;
	}
	return val;
}


mat4 make_yrot_mat(float angle)
{
	float st = sin(angle);
	float ct = cos(angle);
	return mat4(vec4( ct, 0.0,  st, 0.0),
	            vec4(0.0, 1.0, 0.0, 0.0),
	            vec4(-st, 0.0,  ct, 0.0),
	            vec4(0.0, 0.0, 0.0, 1.0));
}

mat4 make_xrot_mat(float angle)
{
	float st = sin(angle);
	float ct = cos(angle);
	return mat4(vec4(1.0, 0.0, 0.0, 0.0),
	            vec4(0.0,  ct, -st, 0.0),
	            vec4(0.0,  st,  ct, 0.0),
	            vec4(0.0, 0.0, 0.0, 1.0));
}


void grass_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	//convenience
	vec4* v_attribs = (vec4*)vertex_attribs;
	My_Uniforms* u = (My_Uniforms*)uniforms;
	GLint inst = builtins->gl_InstanceID;

	vec4 offset = vec4(float(inst >> 8) - 128.0f, 0.0f,
	                   float(inst & 0xFF) - 128.0f, 0.0f);

	int num1 = random(inst, 3);
	int num2 = random(num1, 2);

	offset += vec4(float(num1 & 0xFF)/128.0f, 0.0f, float(num2 & 0xFF)/128.0f, 0.0f);

	//float max_tilt = 3.14159/6.0;
	//float angle2 = mod(float(num1), max_tilt);

	float angle1 = float(num2);
	mat4 yrot = make_yrot_mat(angle1);
	//mat4 xrot = make_xrot_mat(angle2);

	vec4 pos = yrot * v_attribs[0] + offset;
	//vec4 pos = v_attribs[0] + offset;


	//with MANGLE_TYPES have to either cast gl_Position to vec4 or rhs to glinternal_vec4
	*(vec4*)&builtins->gl_Position = u->mvp_mat * pos;
}

void simple_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	//with MANGLE_TYPES have to either cast FragColor to vec4 or color to glinternal_vec4
	*(vec4*)&builtins->gl_FragColor = ((My_Uniforms*)uniforms)->color;
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Grass", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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

int handle_events(GLFrame& camera_frame, unsigned int last_time, unsigned int cur_time)
{
	SDL_Event event;
	int sc;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			sc = event.key.keysym.scancode;

			if (sc == SDL_SCANCODE_ESCAPE) {
				return 1;
			} else if (sc == SDL_SCANCODE_P) {
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			break;

		case SDL_MOUSEMOTION:
		{
			//printf("%d %d %d %d\n", event.motion.y, event.motion.x, event.motion.xrel, event.motion.yrel);
			float dx = event.motion.xrel;
			float dy = event.motion.yrel;
			
			camera_frame.rotate_local_y(DEG_TO_RAD(-dx/50));
			camera_frame.rotate_local_x(DEG_TO_RAD(dy/25));
			
			if (9 < dx*dx + dy*dy) {
				camera_frame.rotate_local_y(DEG_TO_RAD(-dx/30));
				camera_frame.rotate_local_x(DEG_TO_RAD(dy/25));
				//mousex = width/2;
				//mousey = height/2;
			}
		}
			break;

		case SDL_QUIT:
			return 1;
		}
	}



	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	float time = (cur_time - last_time)/1000.0f;

#define MOVE_SPEED 20
	
	if (state[controls[LEFT]]) {
		camera_frame.move_right(time * MOVE_SPEED);
	}
	if (state[controls[RIGHT]]) {
		camera_frame.move_right(time * -MOVE_SPEED);
	}
	if (state[controls[UP]]) {
		camera_frame.move_up(time * MOVE_SPEED);
	}
	if (state[controls[DOWN]]) {
		camera_frame.move_up(time * -MOVE_SPEED);
	}
	if (state[controls[FORWARD]]) {
		camera_frame.move_forward(time*20);
	}
	if (state[controls[BACK]]) {
		camera_frame.move_forward(time*-20);
	}
	if (state[controls[TILTLEFT]]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(-60*time));
	}
	if (state[controls[TILTRIGHT]]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(60*time));
	}


	return 0;
}

