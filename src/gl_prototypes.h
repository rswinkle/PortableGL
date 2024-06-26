

// TODO leave these non gl* functions here?  prefix with pgl?
// TODO could use GLbitfield for masks but then it's less obvious that it needs to be u32
GLboolean init_glContext(glContext* c, u32** back_buffer, GLsizei w, GLsizei h, GLint bitdepth, u32 Rmask, u32 Gmask, u32 Bmask, u32 Amask);
void free_glContext(glContext* context);
void set_glContext(glContext* context);

GLboolean pglResizeFramebuffer(GLsizei w, GLsizei h);

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

void glDebugMessageCallback(GLDEBUGPROC callback, void* userParam);

GLubyte* glGetString(GLenum name);
GLenum glGetError(void);
void glGetBooleanv(GLenum pname, GLboolean* data);
void glGetFloatv(GLenum pname, GLfloat* data);
void glGetIntegerv(GLenum pname, GLint* data);
GLboolean glIsEnabled(GLenum cap);
GLboolean glIsProgram(GLuint program);

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClearDepthf(GLfloat depth);
void glClearDepth(GLdouble depth);
void glDepthFunc(GLenum func);
void glDepthRangef(GLfloat nearVal, GLfloat farVal);
void glDepthRange(GLdouble nearVal, GLdouble farVal);
void glDepthMask(GLboolean flag);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBlendEquation(GLenum mode);
void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClear(GLbitfield mask);
void glProvokingVertex(GLenum provokeMode);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);
void glPolygonMode(GLenum face, GLenum mode);
void glPointSize(GLfloat size);
void glPointParameteri(GLenum pname, GLint param);
void glLineWidth(GLfloat width);
void glLogicOp(GLenum opcode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
void glClearStencil(GLint s);
void glStencilMask(GLuint mask);
void glStencilMaskSeparate(GLenum face, GLuint mask);

//textures
void glGenTextures(GLsizei n, GLuint* textures);
void glDeleteTextures(GLsizei n, const GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);

void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTextureParameteri(GLuint texture, GLenum pname, GLint param);
void glPixelStorei(GLenum pname, GLint param);
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

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
void* glMapBuffer(GLenum target, GLenum access);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
void glVertexAttribDivisor(GLuint index, GLuint divisor);
void glEnableVertexAttribArray(GLuint index);
void glDisableVertexAttribArray(GLuint index);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei drawcount);
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance);
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount);
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount, GLuint baseinstance);

//DSA functions (from OpenGL 4.5+)
#define glCreateBuffers(n, buffers) glGenBuffers(n, buffers)
void glNamedBufferData(GLuint buffer, GLsizei size, const GLvoid* data, GLenum usage);
void glNamedBufferSubData(GLuint buffer, GLsizei offset, GLsizei size, const GLvoid* data);
void* glMapNamedBuffer(GLuint buffer, GLenum access);
void glCreateTextures(GLenum target, GLsizei n, GLuint* textures);

void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index);


//shaders
GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard);
void glDeleteProgram(GLuint program);
void glUseProgram(GLuint program);

// These are here, not in pgl_ext.h/c because they take the place of standard OpenGL
// functions glUniform*() and glProgramUniform*()
void pglSetUniform(void* uniform);
void pglSetProgramUniform(GLuint program, void* uniform);


