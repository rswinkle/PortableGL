
#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <iostream>
#include <stdio.h>

#include <vector>

#include "rsw_math.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define WIDTH 640
#define HEIGHT 480
#define MAX_POINTS 2000

using namespace std;

using rsw::vec2;
using rsw::vec3;
using rsw::vec4;
using rsw::mat4;

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

struct point_data
{
	vec3 pos;
	vec3 vel;
	float start_t;
};

typedef struct My_Uniforms
{
	vec3 v_color;
	vec2 gravity;
	float time;
	float lifetime;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events(unsigned int last_time, unsigned int cur_time);

vec3 ball_rand(float r);
void update_points(vector<point_data>& points, float time);


void particles_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void particles_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

float x, y;
float width, height;
float pt_sz;
int n_pts;
float rate;

My_Uniforms the_uniforms;

int to_adjust;
vector<point_data> points;

int main(int argc, char** argv)
{
	setup_context();

	width = WIDTH;
	height = HEIGHT;
	the_uniforms.lifetime = 1.0f;
	pt_sz = 4.0f;
	n_pts = MAX_POINTS;
	rate = the_uniforms.lifetime/n_pts;
	the_uniforms.gravity = vec2(0, -0.5f);
	the_uniforms.v_color = vec3(1.0f, 0, 0);


	//to_adjust = POINTSIZE;


	point_data tmp = {};
	float theta, phi;
	float start = SDL_GetTicks()/1000.0f;
	for (int i=0; i<MAX_POINTS; i++) {
		//theta = mix(0, RM_PI/6.0f, rsw_randf())
		//phi = mix(0, RM_2PI, rsw_randf())

		// float tmp = sinf(theta) * cosf(phi);
		//tmp.vel = make_vec3(tmp, tmp, tmp)

		tmp.pos = vec3(0, 0, 0);
		tmp.vel = ball_rand(0.3);
		tmp.start_t = start;
		start += rate;

		points.push_back(tmp);
	}


	GLuint vao, buffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(point_data), &points[0], GL_DYNAMIC_DRAW);

	// pos
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point_data), 0);

	// vel
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(point_data), (void*)(sizeof(vec3)));

	// start_t
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(point_data), (void*)(2*sizeof(vec3)));

	GLenum flat = PGL_FLAT;
	GLuint myshader = pglCreateProgram(particles_vs, particles_fs, 1, &flat, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	glDisable(GL_DEPTH_TEST);
	glClearColor(0, 0, 0, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(pt_sz);

	unsigned int last_time = 0, new_time=0, counter = 0;
	float time;
	while (1) {
		new_time = SDL_GetTicks();
		if (handle_events(last_time, new_time)) {
			break;
		}

		counter++;
		if (!(counter % 300)) {
			printf("%d  %f FPS\n", new_time-last_time, 300000/((float)(new_time-last_time)));
			last_time = new_time;
			counter = 0;
		}

		time = new_time/1000.0f;
		update_points(points, time);

		glClear(GL_COLOR_BUFFER_BIT);

		the_uniforms.time = time;
		glDrawArrays(GL_POINTS, 0, n_pts);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void particles_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* v_atts = (vec4*)vertex_attribs;
	My_Uniforms* u = (My_Uniforms*)uniforms;

	vec3 vertex = v_atts[0].vec3h();
	vec3 vel = v_atts[1].vec3h();
	float start_t = v_atts[2].x;

	float transp = 1.0f;
	vec3 g = vec3(u->gravity.x, u->gravity.y, 0);
	vec3 pos = vertex;

	float t = u->time - start_t;

	if (t >= 0) {
		pos = vertex + vel*t + g*t*t;
		transp = 1.0f - t/u->lifetime;
	}

	vs_output[0] = transp;
	*(vec4*)&builtins->gl_Position = vec4(pos.x, pos.y, pos.z, 1);
}

void particles_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 color = ((My_Uniforms*)uniforms)->v_color;
	*(vec4*)&builtins->gl_FragColor = vec4(color.x, color.y, color.z, fs_input[0]);
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("ex1", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
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

vec3 ball_rand(float r)
{
	vec3 p;
	do {
		p = vec3(rsw::randf_range(-r,r),
		         rsw::randf_range(-r,r),
		         rsw::randf_range(-r,r));
	} while (length(p) >= r);
	return p;
}


void update_points(vector<point_data>& points, float time)
{
	point_data tmp = {};
	float theta, phi;

	for (int i=0; i<n_pts; i++) {
		// if it's been alive longer than our lifetime, reset start to recycle
		if (time - points[i].start_t > the_uniforms.lifetime) {
			points[i].start_t = time;
			points[i].pos = vec3(x, y, 0);
			//points[i].vel = ball_rand(0.3);
		}
	}

	glBufferSubData(GL_ARRAY_BUFFER, 0, n_pts*sizeof(point_data), &points[0]);
}

int handle_events(unsigned int last_time, unsigned int cur_time)
{
	SDL_Event event;
	int sc;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return 1;

		case SDL_KEYDOWN:
			sc = event.key.keysym.scancode;

			if (sc == SDL_SCANCODE_ESCAPE) {
				return 1;
			}
			break;

		case SDL_MOUSEMOTION:
		{
			x = event.motion.x*2.0f/(float)width - 1.0f;
			y = -(event.motion.y*2.0f/(float)height - 1.0f);
			//printf("%f %f\n", x, y);
		}
			break;

			/*
		case SDL_MOUSEWHEEL:
		{
			int change = event.wheel.y;
			if (to_adjust == POINTSIZE) {
				point_size += change;
				if (point_size > 20)
					point_size = 20;
				if (point_size < 1) {
					point_size = 1;
				}
				glPointSize(point_size);
			} else if (to_adjust == LIFETIME) {
				lifetime += change*0.1f;
				if (lifetime < 0.1f) {
					lifetime = 0.1f;
				}
				glUniform1f(lifetime_loc, lifetime);
				refresh_points(points);
			} else if (to_adjust == NUM_POINTS) {
				num_points += change*50;
				if (num_points < 0) {
					num_points = 0;
				}
				if (num_points > points.size()) {
					num_points = points.size();
				}
				refresh_points(points);
			}

		}
		break;
		*/


		}
	}
	return 0;
}

