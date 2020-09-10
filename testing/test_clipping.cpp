#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"


#include <stdio.h>
#include <stdbool.h>


#include <SDL2/SDL.h>

#define WIDTH 100
#define HEIGHT 100



vec4 Red = { 1.0f, 0.0f, 0.0f, 0.0f };
vec4 Green = { 0.0f, 1.0f, 0.0f, 0.0f };
vec4 Blue = { 0.0f, 0.0f, 1.0f, 0.0f };

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 v_color;
} My_Uniforms;

void cleanup();
void setup_context();
void setup_gl_data();
int handle_events();


void smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);



int polygon_mode;
int pixel_count;
GLenum interp_mode;

GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };
GLenum noperspective[4] = { NOPERSPECTIVE, NOPERSPECTIVE, NOPERSPECTIVE, NOPERSPECTIVE };

int main(int argc, char** argv)
{
	setup_context();

	polygon_mode = 2;
	interp_mode = SMOOTH;

	float points[] = {  0,     0,    0,
	                    1.25,  0,    -1,
	                    0,     1.25, 0 };

	/*
	float points[] = {  0,     0,    0,
	                    0.5,   0,    0,
	                    0,     0.5, 0 };
	                    */

	float color_array[] = { 1.0, 0.0, 0.0, 1.0,
	                        0.0, 1.0, 0.0, 1.0,
	                        0.0, 0.0, 1.0, 1.0 };

	My_Uniforms the_uniforms;
	mat4 identity = IDENTITY_MAT4();

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

	GLuint myshader = pglCreateProgram(smooth_vs, smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(myshader);

	set_uniform(&the_uniforms);

	the_uniforms.v_color = Red;

	memcpy(the_uniforms.mvp_mat, identity, sizeof(mat4));

	glClearColor(0, 0, 0, 1);


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
		pixel_count = 0;
		glDrawArrays(GL_TRIANGLES, 0, 3);
		printf("pixel_count = %d\n", pixel_count);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* v_attribs = (vec4*)vertex_attribs;
	((vec4*)vs_output)[0] = v_attribs[4]; //color

	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), v_attribs[0]);
}

void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	++pixel_count;
	builtins->gl_FragColor = ((vec4*)fs_input)[0];

	if (pixel_count > 1251 && pixel_count <= 1251 + 234) {
		if (isnan(fs_input[0]))
			printf("\n%p\n", fs_input);
		printf("<%f, %f, %f, %f>\n", fs_input[0], fs_input[1], fs_input[2], fs_input[3]);
		print_vec4(builtins->gl_FragColor, "\n");
		print_vec4(builtins->gl_FragCoord, "\n");
	}
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		printf("SDL_init error: %s\n", SDL_GetError());
		exit(0);
	}

	ren = NULL;
	tex = NULL;
	
	SDL_Window* window = SDL_CreateWindow("swrenderer", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
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


int handle_events()
{
	SDL_Event e;
	int sc;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT)
			return 1;
		if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;
			//printf("%c %c\n", event.key.keysym.scancode, event.key.keysym.sym);
			//printf("Physical %s key acting as %s key",
      	  	 // 	  SDL_GetScancodeName(keysym.scancode),
      	  	  //	  SDL_GetKeyName(keysym.sym));
		
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
			} else if (sc == SDL_SCANCODE_I) {
				if (interp_mode == SMOOTH) {
					printf("noperspective\n");
					//todo change this func to DSA style, ie call it with the program to modify
					//rather than always modifying the current shader
					set_vs_interpolation(4, noperspective);
					interp_mode = NOPERSPECTIVE;
				} else {
					set_vs_interpolation(4, smooth);
					interp_mode = SMOOTH;
					printf("smooth\n");
				}
			}
		} else if (e.type == SDL_MOUSEBUTTONDOWN) {
		}
	}
	return 0;
}

