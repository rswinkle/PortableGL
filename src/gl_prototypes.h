

// TODO leave these non gl* functions here?  prefix with pgl?
PGLDEF GLboolean init_glContext(glContext* c, pix_t** back_buffer, GLsizei width, GLsizei height);
PGLDEF void free_glContext(glContext* context);
PGLDEF void set_glContext(glContext* context);

PGLDEF GLboolean pglResizeFramebuffer(GLsizei width, GLsizei height);

PGLDEF void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

PGLDEF void glDebugMessageCallback(GLDEBUGPROC callback, void* userParam);

PGLDEF GLubyte* glGetString(GLenum name);
PGLDEF GLenum glGetError(void);
PGLDEF void glGetBooleanv(GLenum pname, GLboolean* data);
PGLDEF void glGetFloatv(GLenum pname, GLfloat* data);
PGLDEF void glGetIntegerv(GLenum pname, GLint* data);
PGLDEF GLboolean glIsEnabled(GLenum cap);
PGLDEF GLboolean glIsProgram(GLuint program);

PGLDEF void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
PGLDEF void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
PGLDEF void glClearDepthf(GLfloat depth);
PGLDEF void glClearDepth(GLdouble depth);
PGLDEF void glDepthFunc(GLenum func);
PGLDEF void glDepthRangef(GLfloat nearVal, GLfloat farVal);
PGLDEF void glDepthRange(GLdouble nearVal, GLdouble farVal);
PGLDEF void glDepthMask(GLboolean flag);
PGLDEF void glBlendFunc(GLenum sfactor, GLenum dfactor);
PGLDEF void glBlendEquation(GLenum mode);
PGLDEF void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
PGLDEF void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
PGLDEF void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
PGLDEF void glClear(GLbitfield mask);
PGLDEF void glProvokingVertex(GLenum provokeMode);
PGLDEF void glEnable(GLenum cap);
PGLDEF void glDisable(GLenum cap);
PGLDEF void glCullFace(GLenum mode);
PGLDEF void glFrontFace(GLenum mode);
PGLDEF void glPolygonMode(GLenum face, GLenum mode);
PGLDEF void glPointSize(GLfloat size);
PGLDEF void glPointParameteri(GLenum pname, GLint param);
PGLDEF void glLineWidth(GLfloat width);
PGLDEF void glLogicOp(GLenum opcode);
PGLDEF void glPolygonOffset(GLfloat factor, GLfloat units);
PGLDEF void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
#ifndef PGL_NO_STENCIL
PGLDEF void glStencilFunc(GLenum func, GLint ref, GLuint mask);
PGLDEF void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
PGLDEF void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
PGLDEF void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
PGLDEF void glClearStencil(GLint s);
PGLDEF void glStencilMask(GLuint mask);
PGLDEF void glStencilMaskSeparate(GLenum face, GLuint mask);
#endif

// textures
PGLDEF void glGenTextures(GLsizei n, GLuint* textures);
PGLDEF void glDeleteTextures(GLsizei n, const GLuint* textures);
PGLDEF void glBindTexture(GLenum target, GLuint texture);

PGLDEF void glTexParameteri(GLenum target, GLenum pname, GLint param);
PGLDEF void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
PGLDEF void glTexParameteriv(GLenum target, GLenum pname, const GLint* params);
PGLDEF void glTextureParameteri(GLuint texture, GLenum pname, GLint param);
PGLDEF void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params);
PGLDEF void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params);

PGLDEF void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
PGLDEF void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params);
PGLDEF void glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params);
PGLDEF void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params);
PGLDEF void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params);
PGLDEF void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params);
PGLDEF void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params);
PGLDEF void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params);

PGLDEF void glPixelStorei(GLenum pname, GLint param);
PGLDEF void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

PGLDEF void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data);


PGLDEF void glGenVertexArrays(GLsizei n, GLuint* arrays);
PGLDEF void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
PGLDEF void glBindVertexArray(GLuint array);
PGLDEF void glGenBuffers(GLsizei n, GLuint* buffers);
PGLDEF void glDeleteBuffers(GLsizei n, const GLuint* buffers);
PGLDEF void glBindBuffer(GLenum target, GLuint buffer);
PGLDEF void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
PGLDEF void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
PGLDEF void* glMapBuffer(GLenum target, GLenum access);
PGLDEF void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
PGLDEF void glVertexAttribDivisor(GLuint index, GLuint divisor);
PGLDEF void glEnableVertexAttribArray(GLuint index);
PGLDEF void glDisableVertexAttribArray(GLuint index);
PGLDEF void glDrawArrays(GLenum mode, GLint first, GLsizei count);
PGLDEF void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount);
PGLDEF void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
PGLDEF void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei drawcount);
PGLDEF void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
PGLDEF void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance);
PGLDEF void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount);
PGLDEF void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount, GLuint baseinstance);

//DSA functions (from OpenGL 4.5+)
#define glCreateBuffers(n, buffers) glGenBuffers(n, buffers)
PGLDEF void glNamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage);
PGLDEF void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data);
PGLDEF void* glMapNamedBuffer(GLuint buffer, GLenum access);
PGLDEF void glCreateTextures(GLenum target, GLsizei n, GLuint* textures);

PGLDEF void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
PGLDEF void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index);


//shaders
PGLDEF GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard);
PGLDEF void glDeleteProgram(GLuint program);
PGLDEF void glUseProgram(GLuint program);

// These are here, not in pgl_ext.h/c because they take the place of standard OpenGL
// functions glUniform*() and glProgramUniform*()
PGLDEF void pglSetUniform(void* uniform);
PGLDEF void pglSetProgramUniform(GLuint program, void* uniform);


