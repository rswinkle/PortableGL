#pragma once
#ifndef GLTOOLS_H
#define GLTOOLS_H

#define MANGLE_TYPES
#include "portablegl.h"

// There is a static block allocated for loading shaders to 
// prevent heap fragmentation
#define MAX_SHADER_LENGTH   8192


// // Bring in OpenGL 
// // Windows
// #ifdef WIN32
// #include <windows.h>		// Must have for Windows platform builds
// #ifndef GLEW_STATIC
// #define GLEW_STATIC
// #endif
// 
// #include <gl\glew.h>			// OpenGL Extension "autoloader"
// #include <gl\gl.h>			// Microsoft OpenGL headers (version 1.1 by themselves)
// #endif
// 
// // Mac OS X
// #ifdef __APPLE__
// #include <stdlib.h>
// 
// #include <TargetConditionals.h>
// #if TARGET_OS_IPHONE | TARGET_IPHONE_SIMULATOR
// #include <OpenGLES/ES2/gl.h>
// #include <OpenGLES/ES2/glext.h>
// #define OPENGL_ES
// #else
// #include <GL/glew.h>
// #include <OpenGL/gl.h>		// Apple OpenGL haders (version depends on OS X SDK version)
// #endif
// #endif
// 
// // Linux
// #ifdef linux
// #define GLEW_STATIC
// #include <GL/glew.h>
// #endif

/*
#include <GL/glew.h>

//#include <SFML/Graphics.hpp>
*/
// Universal includes
#include <stdio.h>
#include <math.h>

/*
// Get the OpenGL version
void gltGetOpenGLVersion(GLint &nMajor, GLint &nMinor);

// Check to see if an exension is supported
int gltIsExtSupported(const char *szExtension);


// Shader loading support
void	gltLoadShaderSrc(const char *szShaderSrc, GLuint shader);
bool	gltLoadShaderFile(const char *szFile, GLuint shader);

GLuint	gltLoadShaderPair(const char *szVertexProg, const char *szFragmentProg);
GLuint   gltLoadShaderPairWithAttributes(const char *szVertexProg, const char *szFragmentProg, ...);

GLuint gltLoadShaderPairSrc(const char *szVertexSrc, const char *szFragmentSrc);
GLuint gltLoadShaderPairSrcWithAttributes(const char *szVertexProg, const char *szFragmentProg, ...);

bool gltCheckErrors(GLuint progName = 0);

*/
//my smaller error checking
void check_errors(int n=0, const char* str="Errors cleared");

//texture functions

bool load_texture2D(const char* filename, GLenum minFilter, GLenum magFilter, GLenum wrapMode, bool flip=true);
bool load_texture_cubemap(const char* filename[], GLenum minFilter, GLenum magFilter, bool flip=true);







#endif /* GLTOOLS_H_ */
