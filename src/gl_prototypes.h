

// TODO leave these non gl* functions here?  prefix with pgl?
int init_glContext(glContext* c, u32** back_buffer, int w, int h, int bitdepth, u32 Rmask, u32 Gmask, u32 Bmask, u32 Amask);
void free_glContext(glContext* context);
void set_glContext(glContext* context);

void* pglResizeFramebuffer(size_t w, size_t h);

void glViewport(int x, int y, GLsizei width, GLsizei height);


GLubyte* glGetString(GLenum name);
GLenum glGetError();
void glGetBooleanv(GLenum pname, GLboolean* params);
void glGetDoublev(GLenum pname, GLdouble* params);
void glGetFloatv(GLenum pname, GLfloat* params);
void glGetIntegerv(GLenum pname, GLint* params);
void glGetInteger64v(GLenum pname, GLint64* params);
GLboolean glIsEnabled(GLenum cap);

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth(GLclampf depth);
void glDepthFunc(GLenum func);
void glDepthRange(GLclampf nearVal, GLclampf farVal);
void glDepthMask(GLboolean flag);
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
void glDeleteTextures(GLsizei n, GLuint* textures);
void glBindTexture(GLenum target, GLuint texture);

void glActiveTexture(GLenum texture);
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
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
void glVertexAttribDivisor(GLuint index, GLuint divisor);
void glEnableVertexAttribArray(GLuint index);
void glDisableVertexAttribArray(GLuint index);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, GLsizei offset);
void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, GLsizei* indices, GLsizei drawcount);
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance);
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei primcount);
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei primcount, GLuint baseinstance);

//DSA functions (from OpenGL 4.5+)
void glNamedBufferData(GLuint buffer, GLsizei size, const GLvoid* data, GLenum usage);
void glNamedBufferSubData(GLuint buffer, GLsizei offset, GLsizei size, const GLvoid* data);


//shaders
GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard);
void glDeleteProgram(GLuint program);
void glUseProgram(GLuint program);

void pglSetUniform(void* uniform);


