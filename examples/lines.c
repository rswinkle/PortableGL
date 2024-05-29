#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <stdio.h>

#include <SDL.h>

#define WIDTH 40
#define HEIGHT 40

#define W_WIDTH 640
#define W_HEIGHT 640

#ifndef FPS_EVERY_N_SECS
#define FPS_EVERY_N_SECS 1
#endif

#define FPS_DELAY (FPS_EVERY_N_SECS*1000)


vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
vec4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
vec4 White = { 1.0f, 1.0f, 1.0f, 1.0f };

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

float width = 1;
int draw_put_line = GL_TRUE;
float inv_speed = 6000.0f;

int pause, mine;

void cleanup();
void setup_context();
int handle_events();

typedef struct vert_data
{
	vec3 pos;
	vec4 col;
} vert_data;

int main(int argc, char** argv)
{
	setup_context();

	pgl_uniforms the_uniforms;
	
	float eps = 0.005f;
	vec3 center = make_vec3(WIDTH/2.0f-eps, HEIGHT/2.0f+eps, 0);
	print_vec3(center, " center\n");
	vec3 glcenter = make_vec3(0, 0, 0);
	vec3 endpt;
	vert_data vdata[2] =
		{
			{ glcenter, Red },
			{ endpt, Blue }
			//{ glcenter, White },
			//{ endpt, White }
		};
	//vec3 points[2] = { glcenter, endpt };
	//vec4 colors[2] = { Red, Blue };

	GLuint line;
	glGenBuffers(1, &line);
	glBindBuffer(GL_ARRAY_BUFFER, line);
	pglBufferData(GL_ARRAY_BUFFER, sizeof(vdata), vdata, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, sizeof(vert_data), 0);
	glEnableVertexAttribArray(PGL_ATTR_COLOR);
	pglVertexAttribPointer(PGL_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(vert_data), sizeof(vec3));

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_SHADED]);

	pglSetUniform(&the_uniforms);
	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);

	glClearColor(0, 0, 0, 1);

	int old_time = 0, new_time=0, counter = 0;
	int ms = 0;
	int last_frame;
	float frame_time = 0;

	mat3 rot_mat;
	Color red = { 255, 0, 0, 255 };
	Color green = { 0, 255, 0, 255 };
	Color blue = { 0, 0, 255, 255 };
	Color white = { 255, 255, 255, 255 };

	vec3 tmp = make_vec3(0.9*WIDTH/2.0f, 0, 0);
	endpt = add_vec3s(center, tmp);


	while (handle_events()) {
		SDL_Delay(14);

		new_time = SDL_GetTicks();
		frame_time = (new_time - last_frame)/1000.0f;
		last_frame = new_time;

		counter++;
		ms = new_time - old_time;
		if (ms >= FPS_DELAY) {
			printf("%d  %f FPS\n", ms, counter*1000.0f/ms);
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		if (!draw_put_line) {
			if (!pause) {
				load_rotation_mat3(rot_mat, make_vec3(0, 0, 1), new_time/inv_speed);
				endpt = mult_mat3_vec3(rot_mat, make_vec3(0.9, 0, 0));

				//vdata[1].pos = add_vec3s(vdata[0].pos, endpt);
				vdata[1].pos = endpt;
			}

			//print_vec3(points[0], " 0 \n");
			//print_vec3(points[1], " 1 \n");

			glDrawArrays(GL_LINES, 0, 2);
		} else {
			if (!pause) {
				load_rotation_mat3(rot_mat, make_vec3(0, 0, 1), new_time/inv_speed);
				vec3 tmp = make_vec3(0.9*WIDTH/2.0f, 0, 0);
				tmp = mult_mat3_vec3(rot_mat, tmp);

				endpt = add_vec3s(center, tmp);
				//endpt = center;
			}

			//put_aa_line(White, 19.5, 20.5, 37.99, 20.5);
			//put_aa_line(White, center.x, center.y, 37.99, center.y);
			//draw_line_antialias(center.x, center.y, 37.99, center.y, 255, 255, 255);

			//put_aa_line(White, center.x, center.y, endpt.x, endpt.y);
			put_aa_line_interp(Red, Blue, center.x, center.y, endpt.x, endpt.y);

			//put_line(white, center.x, center.y, endpt.x, endpt.y);
			//put_wide_line_simple(white, width, center.x, center.y, center.x+endpt.x, center.y+endpt.y);
			//put_wide_line2(white, width, center.x, center.y, center.x+endpt.x, center.y+endpt.y);
			//put_wide_line3(red, blue, width, center.x, center.y, center.x+endpt.x, center.y+endpt.y);
		}

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("line_testing", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W_WIDTH, W_HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	bbufpix = NULL; // should already be NULL since global/static but meh

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
		}
		if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;
			if (sc == SDL_SCANCODE_ESCAPE) {
				return 0;
			} else if (sc == SDL_SCANCODE_UP) {
				width++;
				glLineWidth(width);
				printf("width = %f\n", width);
			} else if (sc == SDL_SCANCODE_DOWN) {
				width--;
				if (width < 1) width = 1;
				glLineWidth(width);
				printf("width = %f\n", width);
			} else if (sc == SDL_SCANCODE_RIGHT) {
				if (inv_speed > 1000)
					inv_speed -= 100;
				printf("inv_speed = %f\n", inv_speed);
			} else if (sc == SDL_SCANCODE_LEFT) {
				inv_speed += 100;
				printf("inv_speed = %f\n", inv_speed);
			} else if (sc == SDL_SCANCODE_L) {
				draw_put_line = !draw_put_line;
				printf("draw_put_line = %d\n", draw_put_line);
			} else if (sc == SDL_SCANCODE_SPACE) {
				pause = !pause;
			} else if (sc == SDL_SCANCODE_M) {
				mine = !mine;
			}
		}
	}
	return 1;
}

