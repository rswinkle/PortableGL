#ifndef GLTOOLS_H
#define GLTOOLS_H


#include "portablegl.h"

//#include <GL/glew.h>
//#include <GL/gl.h>

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

void pglSetUniform1i(GLuint program, const char* name, int val);
void pglSetUniform2i(GLuint program, const char* name, int x, int y);
void pglSetUniform3i(GLuint program, const char* name, int x, int y, int z);
void pglSetUniform4i(GLuint program, const char* name, int x, int y, int z, int w);

void pglSetUniform1f(GLuint program, const char* name, float val);
void pglSetUniform2f(GLuint program, const char* name, float x, float y);
void pglSetUniform3f(GLuint program, const char* name, float x, float y, float z);
void pglSetUniform4f(GLuint program, const char* name, float x, float y, float z, float w);

void pglSetUniform2fv(GLuint program, const char* name, GLfloat* v);
void pglSetUniform3fv(GLuint program, const char* name, GLfloat* v);
void pglSetUniform4fv(GLuint program, const char* name, GLfloat* v);

void pglSetUniform_mat4f(GLuint program, const char* name, GLfloat* mat);
void pglSetUniform_mat3f(GLuint program, const char* name, GLfloat* mat);






//textures
GLboolean load_texture2D(const char* filename, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode, GLboolean flip, GLboolean mapdata);
int load_texture2D_array_gif(const char* filename, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode);
GLboolean load_texture_rect(const char* filename, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode, GLboolean flip);
GLboolean load_texture_cubemap(const char* filename[], GLenum min_filter, GLenum mag_filter, GLboolean flip);

#endif
