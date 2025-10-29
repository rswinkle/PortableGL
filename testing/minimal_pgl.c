
// According to Valgrind:
// Uses 66.4 MB by default
// Uses 1.55 MB with PGL_TINY_MEM
//#define PGL_TINY_MEM
#define PGL_EXCLUDE_STUBS
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"


#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

#ifndef FPS_EVERY_N_SECS
#define FPS_EVERY_N_SECS 2
#endif

#define FPS_DELAY (FPS_EVERY_N_SECS*1000)

pix_t* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	vec4 v_color;
} My_Uniforms;

void cleanup(void);
void setup_context(void);
int handle_events(void);


void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);

	setup_context();

	printf("%d %d\n", GL_MAX_VERTEX_ATTRIBS, PGL_MAX_VERTICES);

	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };


	GLuint program = pglCreateProgram(identity_vs, uniform_color_fs, 0, NULL, GL_FALSE);
	glUseProgram(program);

	My_Uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	the_uniforms.v_color = Red;

	// Not actually needed for PGL but there's
	// no default vao in core profile ...
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	getchar();

	// Not actually necessary but for completeness
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &triangle);
	glDeleteProgram(program);

	cleanup();

	return 0;
}


void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);
	builtins->gl_Position = vertex_attribs[0];
}

void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
}

void setup_context(void)
{
	// bbufpix already NULL since global/static
	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}

void cleanup(void)
{
	free_glContext(&the_Context);
}


