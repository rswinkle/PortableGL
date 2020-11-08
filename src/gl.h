
#ifdef MANGLE_TYPES
#define vec2 glinternal_vec2
#define vec3 glinternal_vec3
#define vec4 glinternal_vec4
#define dvec2 glinternal_dvec2
#define dvec3 glinternal_dvec3
#define dvec4 glinternal_dvec4
#define ivec2 glinternal_ivec2
#define ivec3 glinternal_ivec3
#define ivec4 glinternal_ivec4
#define uvec2 glinternal_uvec2
#define uvec3 glinternal_uvec3
#define uvec4 glinternal_uvec4
#define mat3 glinternal_mat3
#define mat4 glinternal_mat4
#define Color glinternal_Color
#define Line glinternal_Line
#define Plane glinternal_Plane
#endif

#ifndef GL_H
#define GL_H



#ifdef __cplusplus
extern "C" {
#endif


#include "crsw_math.h"




/** Non-GL type vectors *********/


#include "cvector_macro.h"

// NOTE(rswinkle): I guess I don't really need these vectors
// currently only use reserve_float (and vec_float, free_vec_float)
// They are mostly just for user/client if they're using C or avoiding std::vector
CVEC_NEW_DECLS(float)
CVEC_NEW_DECLS(vec3)
CVEC_NEW_DECLS(vec4)





/** OpenGL stuff ****************/


#include <stdint.h>

typedef uint32_t GLuint;
typedef int32_t  GLint;
typedef uint16_t GLushort;
typedef int16_t  GLshort;
typedef uint8_t  GLubyte;
typedef int8_t   GLbyte;
typedef char     GLchar;
typedef int32_t  GLsizei;  //they use plain int not unsigned like you'd think
typedef int      GLenum;
typedef int      GLbitfield;
typedef float    GLfloat;
typedef float    GLclampf;
typedef double   GLdouble;
typedef void     GLvoid;
typedef uint8_t  GLboolean;


/*
 * If you want to use any of these old functions from the
 * fixed function pipeline you can use TinyGL or Mesa
 * for software rendering version or implement them as wrappers
 * around modern OpenGL
void     glBegin(GLenum);
void     glClear(GLbitfield);
void     glClearColor(GLclampf, GLclmapf, GLclampf, GLclampf);
void     glColor3f(GLfloat, GLfloat, GLfloat);
void     glColor4f(GLfloat, GLfloat, GLfloat, GLfloat);
void     glCullFace(GLenum);
void     glDisable(GLenum);
void     glEnable(GLenum);
void     glEnd(void);
void     glFrustum(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
GLubyte *glGetString(GLenum);
void     glLoadIdentity(void);
void     glMatrixMode(GLenum);
void     glRotatef(GLfloat, GLfloat, GLfloat, GLfloat);
void     glRotated(GLdouble, GLdouble, GLdouble, GLdouble);
void     glScalef(GLfloat, GLfloat, GLfloat);
void     glScaled(GLdouble, GLdouble, GLdouble);
void     glScissor(GLint, GLint, GLsizei, GLsizei);
void     glTexCoord1f(GLfloat);
void     glTexCoord2f(GLfloat, GLfloat);
void     glTexCoord3f(GLfloat, GLfloat, GLfloat);
void     glTexCoord4f(GLfloat, GLfloat, GLfloat, GLfloat);
void     glTexCoord1d(GLdouble);
void     glTexCoord2d(GLdouble, GLdouble);
void     glTexCoord3d(GLdouble, GLdouble, GLdouble);
void     glTexCoord4d(GLdouble, GLdouble, GLdouble, GLdouble);
void     glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
void     glTexSubImage2D(GLenum GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
void     glTranslatef(GLfloat, GLfloat, GLfloat);
void     glTranslated(GLdouble, GLdouble, GLdouble);
void     glVertex2f(GLfloat, GLfloat);
void     glVertex3f(GLfloat, GLfloat, GLfloat);
void     glVertex4f(GLfloat, GLfloat, GLfloat);
void     glViewport(GLint, GLint, GLsizei, GLsizei);
*/


/* 3.3 core
void     glActiveTexture(GLenum);
void     glAttachShader(GLuint, GLuint);
void     glBeginConditionalRender(GLuint, GLenum);
void     glBeginQuery(GLenum, GLuint);
void     glBeginTransformFeedback(GLenum);
void     glBindAttribLocation(GLuint, GLuint, const GLchar*);
void     glBindBuffer(GLenum, GLuint);
void     glBindBufferBase(GLenum, GLuint, GLuint);
void     glBindBufferRange(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);
void     glBindFragDataLocation(GLuint, GLuint, const char*);
void     glBindFragDataLocationIndexed(GLuint, GLuint, GLuint, const char*);
void     glBindFramebuffer(GLenum, GLuint);
void     glBindRenderbuffer(GLenum, GLuint);
void     glBindSampler(GLuint, GLuint);
void     glBindTexture(GLenum, GLuint);
void     glBindVertexArray(GLuint);
void     glBlendColor(GLclampf, GLclampf, GLclampf, GLclampf);
void     glBlendEquation(GLenum);
void     glBlendEquationSeparate(GLenum, GLenum);
void     glBlendFunc(GLenum, GLenum);
void     glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum);
void     glBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
void     glBufferData(GLenum, GLsizeiptr, const GLvoid*, GLenum);
void     glBufferSubData(GLenum, GLintptr, GLsizeiptr, const GLvoid*);
void     glCheckFramebufferStatus(GLenum);
void     glClampColor(GLenum, GLenum);
void     glClear(GLbitfield);

void     glClearBufferiv(GLenum, GLint, const GLint*);
void     glClearBufferuiv(GLenum, GLint, const GLint*);
void     glClearBufferfv(GLenum, GLint, const GLfloat*);
void     glClearBufferfi(GLenum, GLint, GLfloat, GLint);

void     glClearColor(GLclampf, GLclampf, GLclampf, GLclampf);
void     glClearDepth(GLclampd);
void     glClearStencil(GLint);
void     glCLientWaitSync(GLsync, GLbitfield, GLuint64);
void     glColorMask(GLboolean, GLboolean, GLboolean, GLboolean);
void     glCompileShader(GLuint);
void     glCompressedTexImage1D(GLenum, GLint, GLsizei, GLint, GLsizei, const GLvoid*);
void     glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
void     glCompressedTexImage3D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
void     glCompressedTexSubImage1D(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);


*/


enum
{
	//gl error codes
	GL_NO_ERROR = 0,
	GL_INVALID_ENUM,
	GL_INVALID_VALUE,
	GL_INVALID_OPERATION,
	GL_INVALID_FRAMEBUFFER_OPERATION,
	GL_OUT_OF_MEMORY,

	//buffer types
	GL_ARRAY_BUFFER,
	GL_COPY_READ_BUFFER,
	GL_COPY_WRITE_BUFFER,
	GL_ELEMENT_ARRAY_BUFFER,
	GL_PIXEL_PACK_BUFFER,
	GL_PIXEL_UNPACK_BUFFER,
	GL_TEXTURE_BUFFER,
	GL_TRANSFORM_FEEDBACK_BUFFER,
	GL_UNIFORM_BUFFER,
	GL_NUM_BUFFER_TYPES,

	//buffer use hints (not used currently)
	GL_STREAM_DRAW,
	GL_STREAM_READ,
	GL_STREAM_COPY,
	GL_STATIC_DRAW,
	GL_STATIC_READ,
	GL_STATIC_COPY,
	GL_DYNAMIC_DRAW,
	GL_DYNAMIC_READ,
	GL_DYNAMIC_COPY,

	//polygon modes
	GL_POINT,
	GL_LINE,
	GL_FILL,

	//primitive types
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_LINE_STRIP_AJACENCY,
	GL_LINES_AJACENCY,
	GL_TRIANGLES_AJACENCY,
	GL_TRIANGLE_STRIP_AJACENCY,

	//depth functions
	GL_LESS,
	GL_LEQUAL,
	GL_GREATER,
	GL_GEQUAL,
	GL_EQUAL,
	GL_NOTEQUAL,
	GL_ALWAYS,
	GL_NEVER,

	//blend functions
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_CONSTANT_COLOR,
	GL_ONE_MINUS_CONSTANT_COLOR,
	GL_CONSTANT_ALPHA,
	GL_ONE_MINUS_CONSTANT_ALPHA,
	GL_SRC_ALPHA_SATURATE,
	NUM_BLEND_FUNCS,
	GL_SRC1_COLOR,
	GL_ONE_MINUS_SRC1_COLOR,
	GL_SRC1_ALPHA,
	GL_ONE_MINUS_SRC1_ALPHA,
	//NUM_BLEND_FUNCS

	//blend equations
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX,
	NUM_BLEND_EQUATIONS,

	//texture types
	GL_TEXTURE_UNBOUND,
	GL_TEXTURE_1D,
	GL_TEXTURE_2D,
	GL_TEXTURE_3D,
	GL_TEXTURE_1D_ARRAY,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_RECTANGLE,
	GL_TEXTURE_CUBE_MAP,
	GL_NUM_TEXTURE_TYPES,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,

	//texture parameters i
	GL_TEXTURE_BASE_LEVEL,
	GL_TEXTURE_BORDER_COLOR, // doesn't actually do anything
	GL_TEXTURE_COMPARE_FUNC,
	GL_TEXTURE_COMPARE_MODE,
	GL_TEXTURE_LOD_BIAS,
	GL_TEXTURE_MIN_FILTER,
	GL_TEXTURE_MAG_FILTER,
	GL_TEXTURE_MIN_LOD,
	GL_TEXTURE_MAX_LOD,
	GL_TEXTURE_MAX_LEVEL,
	GL_TEXTURE_SWIZZLE_R,
	GL_TEXTURE_SWIZZLE_G,
	GL_TEXTURE_SWIZZLE_B,
	GL_TEXTURE_SWIZZLE_A,
	GL_TEXTURE_SWIZZLE_RGBA,
	GL_TEXTURE_WRAP_S,
	GL_TEXTURE_WRAP_T,
	GL_TEXTURE_WRAP_R,

	//texture parameter values
	GL_REPEAT,
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,  // not supported, alias to CLAMP_TO_EDGE
	GL_MIRRORED_REPEAT,
	GL_NEAREST,
	GL_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR,

	//texture formats
	GL_RED,
	GL_RG,
	GL_RGB,
	GL_BGR,
	GL_RGBA,
	GL_BGRA,
	GL_COMPRESSED_RED,
	GL_COMPRESSED_RG,
	GL_COMPRESSED_RGB,
	GL_COMPRESSED_RGBA,
	//lots more go here but not important
	
	//PixelStore parameters
	GL_UNPACK_ALIGNMENT,
	
	//implemented glEnable options
	GL_CULL_FACE,
	GL_DEPTH_TEST,
	GL_DEPTH_CLAMP,
	GL_LINE_SMOOTH,
	GL_BLEND,

	//provoking vertex
	GL_FIRST_VERTEX_CONVENTION,
	GL_LAST_VERTEX_CONVENTION,

	//point sprite stuff
	GL_POINT_SPRITE_COORD_ORIGIN,
	GL_UPPER_LEFT,
	GL_LOWER_LEFT,
	

	//buffer clearing selection
	GL_COLOR_BUFFER_BIT,
	GL_DEPTH_BUFFER_BIT,
	GL_STENCIL_BUFFER_BIT,

	//front face determination/culling
	GL_FRONT,
	GL_BACK,
	GL_FRONT_AND_BACK,
	GL_CCW,
	GL_CW,

	//data types
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_BITMAP,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT,
	GL_FLOAT,

	//glGetString info
	GL_VENDOR,
	GL_RENDERER,
	GL_VERSION,
	GL_SHADING_LANGUAGE_VERSION,


	//shader types etc. not used, just here for compatibility add what you
	//need so you can use your OpenGL code with PortableGL with minimal changes
	GL_COMPUTE_SHADER,
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,

	GL_INFO_LOG_LENGTH,
	GL_COMPILE_STATUS,
	GL_LINK_STATUS

};

#define GL_FALSE 0
#define GL_TRUE 1





#define MAX_VERTICES 500000
#define GL_MAX_VERTEX_ATTRIBS 16
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS 64
#define GL_MAX_DRAW_BUFFERS 8


enum { SMOOTH, FLAT, NOPERSPECTIVE };




//TODO NOT USED YET
typedef struct PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[6];
} PerVertex;

typedef struct Shader_Builtins
{
	//PerVertex gl_PerVertex;
	vec4 gl_Position;
	GLint gl_InstanceID;
	vec2 gl_PointCoord;

	GLboolean gl_FrontFacing;
	vec4 gl_FragCoord;
	vec4 gl_FragColor;

	//vec4 gl_FragData[GL_MAX_DRAW_BUFFERS];
	
	float gl_FragDepth;
	GLboolean discard;

} Shader_Builtins;

typedef void (*vert_func)(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
typedef void (*frag_func)(float* fs_input, Shader_Builtins* builtins, void* uniforms);

typedef struct glProgram
{
	vert_func vertex_shader;
	frag_func fragment_shader;
	void* uniform;
	int vs_output_size;
	GLenum interpolation[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	GLboolean use_frag_depth;
	GLboolean deleted;

} glProgram;

typedef struct glBuffer
{
	/*
	GLenum usage;
	GLenum access;
	GLint access_flags;
	void* map_pointer;
	GLsizei map_offset;
	GLsizei map_length;
	*/

	GLsizei size;
	GLenum type;
	u8* data;

	GLboolean deleted;
	GLboolean mapped;
} glBuffer;

typedef struct glVertex_Attrib
{
	GLint size;      // number of components 1-4
	GLenum type;     // GL_FLOAT, default
	GLsizei stride;  //
	GLsizei offset;  //
	GLboolean normalized;
	unsigned int buf;
	GLboolean enabled;
	GLuint divisor;
} glVertex_Attrib;

void init_glVertex_Attrib(glVertex_Attrib* v);
//void init_glVertex_Attrib(glVertex_Attrib* v, GLint size, GLenum type, GLsizei stride, GLsizei offset, GLboolean normalized, Buffer* buf);


typedef struct glVertex_Array
{
	glVertex_Attrib vertex_attribs[GL_MAX_VERTEX_ATTRIBS];

	//GLuint n_array_bufs;
	GLuint element_buffer;
	GLboolean deleted;

} glVertex_Array;

void init_glVertex_Array(glVertex_Array* v);


typedef struct glTexture
{
	unsigned int w;
	unsigned int h;
	unsigned int d;

	int base_level;
//	vec4 border_color;
	GLenum mag_filter;
	GLenum min_filter;
	GLenum wrap_s;
	GLenum wrap_t;
	GLenum wrap_r;

	GLenum type;

	GLboolean deleted;
	GLboolean mapped;

	u8* data;
} glTexture;

typedef struct glVertex
{
	vec4 clip_space;
	vec4 screen_space;
	int clip_code;
	int edge_flag;
	float* vs_out;
} glVertex;

typedef struct glFramebuffer
{
	u8* buf;
	u8* lastrow; //better or worse than + h-1 every pixel draw?
	size_t w;
	size_t h;
} glFramebuffer;

typedef struct Vertex_Shader_output
{
	int size;
	GLenum* interpolation;
	cvector_float output_buf;
} Vertex_Shader_output;


typedef void (*draw_triangle_func)(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);


CVEC_NEW_DECLS(glVertex_Array)
CVEC_NEW_DECLS(glBuffer)
CVEC_NEW_DECLS(glTexture)
CVEC_NEW_DECLS(glProgram)
CVEC_NEW_DECLS(glVertex)

/*
#include "vector_glVertex_Array.h"
#include "vector_glBuffer.h"
#include "vector_glTexture.h"
#include "vector_glProgram.h"
#include "vector_glVertex.h"
*/


typedef struct glContext
{
	mat4 vp_mat;

	int x_min, y_min;
	size_t x_max, y_max;

	cvector_glVertex_Array vertex_arrays;
	cvector_glBuffer buffers;
	cvector_glTexture textures;
	cvector_glProgram programs;

	GLuint cur_vertex_array;
	GLuint bound_buffers[GL_NUM_BUFFER_TYPES-GL_ARRAY_BUFFER];
	GLuint bound_textures[GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1];
	GLuint cur_texture2D;
	GLuint cur_program;

	GLenum error;

	void* uniform;

	vec4 vertex_attribs_vs[GL_MAX_VERTEX_ATTRIBS];
	Shader_Builtins builtins;
	Vertex_Shader_output vs_output;
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	unsigned int provoking_vert;
	GLboolean depth_test;
	GLboolean line_smooth;
	GLboolean cull_face;
	GLboolean frag_depth_used;
	GLboolean depth_clamp;
	GLboolean blend;
	GLenum blend_sfactor;
	GLenum blend_dfactor;
	GLenum blend_equation;
	GLenum cull_mode;
	GLenum front_face;
	GLenum poly_mode_front;
	GLenum poly_mode_back;
	GLenum depth_func;
	GLenum point_spr_origin;

	GLint unpack_alignment;
	GLint pack_alignment;

	Color clear_color;
	vec4 blend_color;
	float point_size;
	float clear_depth;
	float depth_range_near;
	float depth_range_far;

	draw_triangle_func draw_triangle_front;
	draw_triangle_func draw_triangle_back;

	glFramebuffer zbuf;
	glFramebuffer back_buffer;

	int bitdepth;
	u32 Rmask;
	u32 Gmask;
	u32 Bmask;
	u32 Amask;
	int Rshift;
	int Gshift;
	int Bshift;
	int Ashift;
	


	cvector_glVertex glverts;
} glContext;


int init_glContext(glContext* c, u32* back_buffer, int w, int h, int bitdepth, u32 Rmask, u32 Gmask, u32 Bmask, u32 Amask);
void free_glContext(glContext* context);
void set_glContext(glContext* context);

//shader texture functions
vec4 texture1D(GLuint tex, float x);
vec4 texture2D(GLuint tex, float x, float y);
vec4 texture3D(GLuint tex, float x, float y, float z);
vec4 texture_cubemap(GLuint texture, float x, float y, float z);


void resize_framebuffer(size_t w, size_t h);
void glViewport(int x, int y, GLsizei width, GLsizei height);


GLubyte* glGetString(GLenum name);
GLenum glGetError();

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth(GLclampf depth);
void glDepthFunc(GLenum func);
void glDepthRange(GLclampf nearVal, GLclampf farVal);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBlendEquation(GLenum mode);
void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClear(GLbitfield mask);
void glProvokingVertex(GLenum provokeMode);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);
void glPolygonMode(GLenum face, GLenum mode);
void glPointSize(GLfloat size);
void glPointParameteri(GLenum pname, GLint param);

//textures
void glGenTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);

void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
void glPixelStorei(GLenum pname, GLint param);
void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data);


void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
void glBindVertexArray(GLuint array);
void glGenBuffers(GLsizei n, GLuint* buffers);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage);
void glBufferSubData(GLenum target, GLsizei offset, GLsizei size, const GLvoid* data);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLsizei offset);
void glVertexAttribDivisor(GLuint index, GLuint divisor);
void glEnableVertexAttribArray(GLuint index);
void glDisableVertexAttribArray(GLuint index);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, GLsizei offset);
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance);
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei primcount);
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei primcount, GLuint baseinstance);


//shaders
GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean use_frag_depth);
void glDeleteProgram(GLuint program);
void glUseProgram(GLuint program);

void set_uniform(void* uniform);

//This isn't possible in regular OpenGL, changing the interpolation of vs output of
//an existing shader.  You'd have to switch between 2 almost identical shaders.
void set_vs_interpolation(GLsizei n, GLenum* interpolation);


// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need
void glGenerateMipmap(GLenum target);


void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void glAttachShader(GLuint program, GLuint shader);
void glCompileShader(GLuint shader);
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
GLuint glCreateProgram();
void glLinkProgram(GLuint program);
void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
GLuint glCreateShader(GLenum shaderType);
void glDeleteShader(GLuint shader);

GLint glGetUniformLocation(GLuint program, const GLchar* name);

void glUniform1f(GLint location, GLfloat v0);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

void glUniform1i(GLint location, GLint v0);
void glUniform2i(GLint location, GLint v0, GLint v1);
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

void glUniform1ui(GLuint location, GLuint v0);
void glUniform2ui(GLuint location, GLuint v0, GLuint v1);
void glUniform3ui(GLuint location, GLuint v0, GLuint v1, GLuint v2);
void glUniform4ui(GLuint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

void glUniform1fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);

void glUniform1iv(GLint location, GLsizei count, const GLint* value);
void glUniform2iv(GLint location, GLsizei count, const GLint* value);
void glUniform3iv(GLint location, GLsizei count, const GLint* value);
void glUniform4iv(GLint location, GLsizei count, const GLint* value);

void glUniform1uiv(GLint location, GLsizei count, const GLuint* value);
void glUniform2uiv(GLint location, GLsizei count, const GLuint* value);
void glUniform3uiv(GLint location, GLsizei count, const GLuint* value);
void glUniform4uiv(GLint location, GLsizei count, const GLuint* value);

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);






//Raw clear, raw primitive drawing, bypass gl state, customize, etc.
void pglDrawFrame();


void clear_screen();
void put_pixel(Color c, int x, int y);
void put_line(Color the_color, float x1, float y1, float x2, float y2);
void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3);



/* GLSL(ish) functions */

float clampf(float f, float min, float max);
int clampi(int i, int min, int max);
float clampf_01(float f);


/******************************/

#ifdef __cplusplus
}
#endif


// end GL_H
#endif


#ifdef UNMANGLE_TYPES
#undef vec2
#undef vec3
#undef vec4
#undef dvec2
#undef dvec3
#undef dvec4
#undef ivec2
#undef ivec3
#undef ivec4
#undef uvec2
#undef uvec3
#undef uvec4
#undef mat3
#undef mat4
#undef Color
#undef Line
#undef Plane
#endif

