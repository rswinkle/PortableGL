#define MANGLE_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <vector>
#include <rsw_matstack.h>


#include <SDL2/SDL.h>

#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

using rsw::vec3;
using rsw::mat4;


SDL_Window* win;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	vec4 color;
} My_Uniforms;

int polygon_mode;
int use_elements;
My_Uniforms the_uniforms;

void cleanup();
void setup_context();
int handle_events();

void basic_transform_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void white_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	// Compare this setup and the MultiDraw calls to
	// https://github.com/rswinkle/opengl_reference/blob/master/src/multidraw.cpp
	vector<vec3> tri_strips;
	vector<GLuint> strip_elems;
	vec3 offset(10, 10, 0);

	int sq_dim = 20;
	vector<GLint> firsts;

	// needs to be the same size as a pointer so GLintptr or GLsizeiptr
	// Possibly I could remove that restriction by changing glMultiDrawElements somehow
	// but better to just match OpenGL for now
	vector<GLintptr> first_elems;
	vector<GLsizei> counts;

	const int cols = 25;
	const int rows = 19;

	for (int j=0; j<rows; j++) {
		for (int i=0; i<cols; i++) {
			firsts.push_back(tri_strips.size());

			// a byte offset into the element array buffer, which is using GLuints
			first_elems.push_back(strip_elems.size()*sizeof(GLuint));
			counts.push_back(4);

			tri_strips.push_back(vec3(i*(sq_dim+5),        j*(sq_dim+5),        0));
			tri_strips.push_back(vec3(i*(sq_dim+5),        j*(sq_dim+5)+sq_dim, 0));
			tri_strips.push_back(vec3(i*(sq_dim+5)+sq_dim, j*(sq_dim+5),        0));
			tri_strips.push_back(vec3(i*(sq_dim+5)+sq_dim, j*(sq_dim+5)+sq_dim, 0));

			strip_elems.push_back((j*cols+i)*4);
			strip_elems.push_back((j*cols+i)*4+1);
			strip_elems.push_back((j*cols+i)*4+2);
			strip_elems.push_back((j*cols+i)*4+3);
		}
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint square_buf;
	glGenBuffers(1, &square_buf);
	glBindBuffer(GL_ARRAY_BUFFER, square_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*tri_strips.size(), &tri_strips[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint elem_buf;
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLsizeiptr)*strip_elems.size(), &strip_elems[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);

	GLuint program = pglCreateProgram(basic_transform_vp, white_fp, 0, NULL, GL_FALSE);
	glUseProgram(program);
	pglSetUniform(&the_uniforms);

	matrix_stack mat_stack;
	//rsw::make_perspective_matrix(mat_stack.stack[mat_stack.top], DEG_TO_RAD(45), WIDTH/(float)HEIGHT, 0.1f, 100.0f);
	rsw::make_orthographic_matrix(mat_stack.stack[mat_stack.top], 0, WIDTH-1, 0, HEIGHT-1, 1, -1);


	unsigned int old_time = 0, new_time=0, counter = 0;
	while (1) {
		if (handle_events())
			break;

		counter++;
		new_time = SDL_GetTicks();
		if (new_time - old_time > 3000) {
			printf("%f FPS\n", counter*1000.f/(new_time-old_time));
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		mat_stack.push();
		mat_stack.translate(10.0, 10.0, 0.0);

		the_uniforms.mvp_mat = mat_stack.get_matrix();

		if (!use_elements) {
			glMultiDrawArrays(GL_TRIANGLE_STRIP, &firsts[0], &counts[0], 100);
		} else {
			glMultiDrawElements(GL_TRIANGLE_STRIP, &counts[0], GL_UNSIGNED_INT, (GLvoid* const*)&first_elems[0], 475);
		}

		mat_stack.pop();

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &square_buf);
	glDeleteProgram(program);

	cleanup();

	return 0;
}

void basic_transform_vp(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	*(vec4*)&builtins->gl_Position = u->mvp_mat * ((vec4*)vertex_attribs)[0];
}

void white_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec4* fragcolor = (vec4*)&builtins->gl_FragColor;
	*fragcolor = vec4(1);;
}

void uniform_color_fp(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	vec4* fragcolor = (vec4*)&builtins->gl_FragColor;
	*fragcolor = u->color;
}


void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	win = SDL_CreateWindow("Multidraw", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!win) {
		cleanup();
		exit(0);
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
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
	SDL_DestroyWindow(win);

	SDL_Quit();
}

int handle_events()
{
	SDL_Event e;
	int sc;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return 1;
		} else if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;

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
			} else if (sc == SDL_SCANCODE_E) {
				use_elements = !use_elements;
				if (use_elements) {
					puts("Using MultiDrawElements");
				} else {
					puts("Using MultiDrawArrays");
				}
			}
		}
	}
	return 0;
}







