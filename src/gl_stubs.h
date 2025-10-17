
#ifndef PGL_EXCLUDE_STUBS

// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need
//
PGLDEF const GLubyte* glGetStringi(GLenum name, GLuint index);

PGLDEF void glColorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);

PGLDEF void glGenerateMipmap(GLenum target);
PGLDEF void glActiveTexture(GLenum texture);

PGLDEF void glTexParameterf(GLenum target, GLenum pname, GLfloat param);

PGLDEF void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param);

// TODO what the heck are these?
PGLDEF void glTexParameterliv(GLenum target, GLenum pname, const GLint* params);
PGLDEF void glTexParameterluiv(GLenum target, GLenum pname, const GLuint* params);

PGLDEF void glTextureParameterliv(GLuint texture, GLenum pname, const GLint* params);
PGLDEF void glTextureParameterluiv(GLuint texture, GLenum pname, const GLuint* params);

PGLDEF void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data);
PGLDEF void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
PGLDEF void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);

PGLDEF void glGetDoublev(GLenum pname, GLdouble* params);
PGLDEF void glGetInteger64v(GLenum pname, GLint64* params);

// Draw buffers
PGLDEF void glDrawBuffers(GLsizei n, const GLenum* bufs);
PGLDEF void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs);

// Framebuffers/Renderbuffers
PGLDEF void glGenFramebuffers(GLsizei n, GLuint* ids);
PGLDEF void glBindFramebuffer(GLenum target, GLuint framebuffer);
PGLDEF void glDeleteFramebuffers(GLsizei n, GLuint* framebuffers);
PGLDEF void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);

PGLDEF void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PGLDEF void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PGLDEF void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
PGLDEF GLboolean glIsFramebuffer(GLuint framebuffer);

PGLDEF void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
PGLDEF void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);

PGLDEF void glReadBuffer(GLenum mode);
PGLDEF void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode);

PGLDEF void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PGLDEF void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

PGLDEF void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
PGLDEF void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
PGLDEF void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
PGLDEF void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GLboolean glIsRenderbuffer(GLuint renderbuffer);
PGLDEF void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GLenum glCheckFramebufferStatus(GLenum target);

PGLDEF void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PGLDEF void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

PGLDEF void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value);
PGLDEF void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value);
PGLDEF void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value);
PGLDEF void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
PGLDEF void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value);
PGLDEF void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value);
PGLDEF void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value);
PGLDEF void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);


PGLDEF void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
PGLDEF void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
PGLDEF void glAttachShader(GLuint program, GLuint shader);
PGLDEF void glCompileShader(GLuint shader);
PGLDEF void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);

// use pglCreateProgram()
PGLDEF GLuint glCreateProgram(void);

PGLDEF void glLinkProgram(GLuint program);
PGLDEF void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
PGLDEF void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
PGLDEF GLuint glCreateShader(GLenum shaderType);
PGLDEF void glDeleteShader(GLuint shader);
PGLDEF void glDetachShader(GLuint program, GLuint shader);

PGLDEF GLint glGetUniformLocation(GLuint program, const GLchar* name);
PGLDEF GLint glGetAttribLocation(GLuint program, const GLchar* name);

PGLDEF GLboolean glUnmapBuffer(GLenum target);
PGLDEF GLboolean glUnmapNamedBuffer(GLuint buffer);

PGLDEF void glUniform1f(GLint location, GLfloat v0);
PGLDEF void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
PGLDEF void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
PGLDEF void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

PGLDEF void glUniform1i(GLint location, GLint v0);
PGLDEF void glUniform2i(GLint location, GLint v0, GLint v1);
PGLDEF void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
PGLDEF void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

PGLDEF void glUniform1ui(GLint location, GLuint v0);
PGLDEF void glUniform2ui(GLint location, GLuint v0, GLuint v1);
PGLDEF void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
PGLDEF void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

PGLDEF void glUniform1fv(GLint location, GLsizei count, const GLfloat* value);
PGLDEF void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
PGLDEF void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
PGLDEF void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);

PGLDEF void glUniform1iv(GLint location, GLsizei count, const GLint* value);
PGLDEF void glUniform2iv(GLint location, GLsizei count, const GLint* value);
PGLDEF void glUniform3iv(GLint location, GLsizei count, const GLint* value);
PGLDEF void glUniform4iv(GLint location, GLsizei count, const GLint* value);

PGLDEF void glUniform1uiv(GLint location, GLsizei count, const GLuint* value);
PGLDEF void glUniform2uiv(GLint location, GLsizei count, const GLuint* value);
PGLDEF void glUniform3uiv(GLint location, GLsizei count, const GLuint* value);
PGLDEF void glUniform4uiv(GLint location, GLsizei count, const GLuint* value);

PGLDEF void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

#endif

