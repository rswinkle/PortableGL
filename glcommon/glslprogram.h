#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>
#include <cstdarg>
using std::string;

#include "rsw_math.h"

//using namespace std;

using rsw::vec3;
using rsw::ivec3;
using rsw::vec4;
using rsw::vec2;
using rsw::mat4;
using rsw::mat3;

namespace GLSLShader {
    enum GLSLShaderType {
        VERTEX, FRAGMENT, GEOMETRY,
        TESS_CONTROL, TESS_EVALUATION
    };

	enum AttributeLcations {
		ATTR_VERTEX = 0,
		ATTR_NORMAL = 1,
		ATTR_TEXCOORD = 2
	};

}

class GLSLProgram
{
private:
    int  handle;
    bool linked;
    string logString;

    int  get_uniform_location(const char * name);
    bool file_exists(const string & fileName);

public:
    GLSLProgram();

    bool   compile_shader_file(const char * fileName, GLSLShader::GLSLShaderType type);
    bool   compile_shader_string(const string & source, GLSLShader::GLSLShaderType type);
    bool   link();
    bool   validate();
    void   use();
	void   delete_program();

    string log();

    int    get_handle();
    bool   isLinked();

    void   bindAttribLocation(GLuint location, const char * name);
    void   bindFragDataLocation(GLuint location, const char * name);

    void   pglSetUniform(const char *name, float x, float y);
    void   pglSetUniform(const char *name, float x, float y, float z);
    void   pglSetUniform(const char *name, const vec2 & v);
    void   pglSetUniform(const char *name, const vec3 & v);
    void   pglSetUniform(const char *name, const vec4 & v);
    void   pglSetUniform(const char *name, const mat4 & m);
    void   pglSetUniform(const char *name, const mat3 & m);
    void   pglSetUniform(const char *name, float val );
    void   pglSetUniform(const char *name, int val);
    void   pglSetUniform(const char *name, bool val);
	
	
	int get_uniform_block_info(unsigned int block_index, GLenum info);

    void   print_active_uniforms();
    void   print_active_attribs();
};


void compile_link_shaders(GLSLProgram& prog, int num_shaders, ...);

#endif // GLSLPROGRAM_H
