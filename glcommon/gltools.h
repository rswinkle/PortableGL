#ifndef GLTOOLS_H
#define GLTOOLS_H

#ifdef USING_PORTABLEGL
#include "portablegl.h"
#elif defined(USING_GLES2)

#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>
#define GL_COMPRESSED_RGBA GL_RGBA

#else
#include <GL/glew.h>
//#include <glad.h>
#include <GL/gl.h>
#endif

// Universal includes
#include <stdio.h>
#include <math.h>


//general
void check_errors(int n, const char* str);




//shaders
int file_read(FILE* file, char** out);
int file_open_read(const char* filename, const char* mode, char** out);

int compile_shader_str(GLuint shader, const char* shader_str);
int link_program(GLuint program);
GLuint load_shader_pair(const char* vert_shader_src, const char* frag_shader_src);
GLuint load_shader_file_pair(const char* vert_file, const char* frag_file);

void set_uniform1i(GLuint program, const char* name, int val);
void set_uniform2i(GLuint program, const char* name, int x, int y);
void set_uniform3i(GLuint program, const char* name, int x, int y, int z);
void set_uniform4i(GLuint program, const char* name, int x, int y, int z, int w);

void set_uniform1f(GLuint program, const char* name, float val);
void set_uniform2f(GLuint program, const char* name, float x, float y);
void set_uniform3f(GLuint program, const char* name, float x, float y, float z);
void set_uniform4f(GLuint program, const char* name, float x, float y, float z, float w);

void set_uniform2fv(GLuint program, const char* name, const GLfloat* v);
void set_uniform3fv(GLuint program, const char* name, const GLfloat* v);
void set_uniform4fv(GLuint program, const char* name, const GLfloat* v);

void set_uniform_mat4f(GLuint program, const char* name, const GLfloat* mat);
void set_uniform_mat3f(GLuint program, const char* name, const GLfloat* mat);






//textures
GLboolean load_texture2D(const char* filename, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode, GLboolean flip, GLubyte** out_img, int* width, int* height);
GLboolean load_texture_cubemap(const char* filename[], GLenum min_filter, GLenum mag_filter, GLboolean flip);

#ifndef USING_GLES2
int load_texture2D_array_gif(const char* filename, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode);
GLboolean load_texture_rect(const char* filename, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode, GLboolean flip);
#endif

#endif
